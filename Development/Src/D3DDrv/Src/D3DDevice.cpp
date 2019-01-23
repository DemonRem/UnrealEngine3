/*=============================================================================
	D3DViewport.cpp: D3D viewport RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI

//
// RHI constants.
//

/** The maximum number of mip-maps that a texture can contain. */
INT GMaxTextureMipCount = MAX_TEXTURE_MIP_COUNT;

/** The minimum number of mip-maps that always remain resident.	*/
INT	GMinTextureResidentMipCount = 7;

/** TRUE if PF_DepthStencil textures can be created and sampled. */
UBOOL GSupportsDepthTextures = FALSE;

/** 
 * TRUE if PF_DepthStencil textures can be created and sampled to obtain PCF values. 
 * This is different from GSupportsDepthTextures in two ways:
 *	-results of sampling are PCF values, not depths
 *	-a color target must be bound with the depth stencil texture even if never written to or read from,
 *		due to API restrictions
 */
UBOOL GSupportsHardwarePCF = FALSE;

/** TRUE if D24 textures can be created and sampled, retrieving 4 neighboring texels in a single lookup. */
UBOOL GSupportsFetch4 = FALSE;

/**
 * TRUE if floating point filtering is supported
 */
UBOOL GSupportsFPFiltering = TRUE;

/** TRUE if pixels are clipped to 0<Z<W instead of -W<Z<W. */
const UBOOL GPositiveZRange = TRUE;

/** Can we handle quad primitives? */
const UBOOL GSupportsQuads = FALSE;

/** Are we using an inverted depth buffer? */
const UBOOL GUsesInvertedZ = FALSE;

/** The offset from the upper left corner of a pixel to the position which is used to sample textures for fragments of that pixel. */
const FLOAT GPixelCenterOffset = 0.5f;

/** The shader platform to load from the cache */
EShaderPlatform GRHIShaderPlatform = SP_PCD3D_SM3;

/** Shadow buffer size */
INT GShadowDepthBufferSize = 1024;

/** Bias exponent used to apply to shader color output when rendering to the scene color surface */
INT GCurrentColorExpBias = 0;

/** TRUE if the rendering hardware supports vertex instancing. */
UBOOL GSupportsVertexInstancing = FALSE;

/** Enable or disable vsync, overridden by the -novsync command line parameter. */
UBOOL GEnableVSync = FALSE;

/** If FALSE code needs to patch up vertex declaration. */
UBOOL GVertexElementsCanShareStreamOffset = TRUE;

//
// Globals.
//

/** True if the D3D device has been initialized. */
UBOOL GIsRHIInitialized = FALSE;

/** The global D3D interface. */
TD3DRef<IDirect3D9> GDirect3D;

/** The global D3D device. */
TD3DRef<IDirect3DDevice9> GDirect3DDevice;

/** The global D3D device's back buffer. */
FSurfaceRHIRef GD3DBackBuffer;

/** The viewport RHI which is currently fullscreen. */
FD3DViewport* GD3DFullscreenViewport = NULL;

/** The width of the D3D device's back buffer. */
UINT GD3DDeviceSizeX = 0;

/** The height of the D3D device's back buffer. */
UINT GD3DDeviceSizeY = 0;

/** The window handle associated with the D3D device. */
HWND GD3DDeviceWindow = NULL;

/** True if the D3D device is in fullscreen mode. */
UBOOL GD3DIsFullscreenDevice = FALSE;

/** True if the application has lost D3D device access. */
UBOOL GD3DDeviceLost = FALSE;

/**
 * Cleanup the D3D device.
 * This function must be called from the main game thread.
 */
void CleanupD3DDevice()
{
	check( IsInGameThread() );
	if(GIsRHIInitialized)
	{
		// Ask all initialized FRenderResources to release their RHI resources.
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseRHI();
		}
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseDynamicRHI();
		}
		GIsRHIInitialized = FALSE;
		// release dynamic resources from global movie player
		if( GFullScreenMovie )
		{
			GFullScreenMovie->ReleaseDynamicResources();
		}
	}
}

/**
* Initializes GRHIShaderPlatform based on the system's Caps
*/
void InitializeD3DShaderPlatform()
{
	check( IsInGameThread() );

	// Wait for the rendering thread to go idle.
	FlushRenderingCommands();

	if ( !GDirect3D )
	{
		*GDirect3D.GetInitReference() = Direct3DCreate9(D3D_SDK_VERSION);
	}
	if ( !GDirect3D )
	{
		appErrorf(
			NAME_FriendlyError,
			TEXT("Please install DirectX 9.0c or later (see Release Notes for instructions on how to obtain it)")
			);
	}

	D3DCAPS9 DeviceCaps;
	UINT AdapterIndex = D3DADAPTER_DEFAULT;
	D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;
	if (SUCCEEDED(GDirect3D->GetDeviceCaps(AdapterIndex,DeviceType,&DeviceCaps)))
	{
		// Determine whether ps_3_0 is supported.
		UBOOL bSupportsShaderModel3 = (DeviceCaps.PixelShaderVersion & 0xff00) >= 0x0300;

		//only overwrite if not supported, since this may have been specified on the command line
		if (!bSupportsShaderModel3)
		{
			GRHIShaderPlatform = SP_PCD3D_SM2;
		}
	}

	// Read the current display mode.
	D3DDISPLAYMODE CurrentDisplayMode;
	VERIFYD3DRESULT(GDirect3D->GetAdapterDisplayMode(AdapterIndex,&CurrentDisplayMode));

	// Determine the supported lighting buffer formats.
	UBOOL bSupports16bitRT = !FAILED(GDirect3D->CheckDeviceFormat(
		AdapterIndex,
		DeviceType,
		CurrentDisplayMode.Format,
		D3DUSAGE_RENDERTARGET,
		D3DRTYPE_TEXTURE,
		D3DFMT_A16B16G16R16F
		));

	// Determine whether alpha blending with a floating point framebuffer is supported.
	if (FAILED(
		GDirect3D->CheckDeviceFormat(
			AdapterIndex,
			DeviceType,
			CurrentDisplayMode.Format,
			D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
			D3DRTYPE_TEXTURE,
			bSupports16bitRT ? D3DFMT_A16B16G16R16F : D3DFMT_A32B32G32R32F
		)))
	{
		// The engine assumes that hardware that support SM3 also support FP blending.
		// Fall back to SM2 if the hardware doesn't support FP blending.
		GRHIShaderPlatform = SP_PCD3D_SM2;
	}
}

/**
 * Initializes the D3D device for the current viewport state.
 * This function must be called from the main game thread.
 */
void UpdateD3DDeviceFromViewports()
{
	check( IsInGameThread() );

	// Wait for the rendering thread to go idle.
	FlushRenderingCommands();

	// Initialize the RHI shader platform.
	RHIInitializeShaderPlatform();

	if(GD3DViewports.Num())
	{
		// Create the Direct3D object if necessary.
		if ( !GDirect3D )
		{
			*GDirect3D.GetInitReference() = Direct3DCreate9(D3D_SDK_VERSION);
		}
		if ( !GDirect3D )
		{
			appErrorf(
				NAME_FriendlyError,
				TEXT("Please install DirectX 9.0c or later (see Release Notes for instructions on how to obtain it)")
				);
		}

		// Find the maximum viewport dimensions and any fullscreen viewports.
		FD3DViewport*	NewFullscreenViewport = NULL;
		UINT			MaxViewportSizeX = 0,
						MaxViewportSizeY = 0,
						MaxHitViewportSizeX = 0,
						MaxHitViewportSizeY = 0;

		for(INT ViewportIndex = 0;ViewportIndex < GD3DViewports.Num();ViewportIndex++)
		{
			FD3DViewport* D3DViewport = GD3DViewports(ViewportIndex);

			MaxViewportSizeX = Max(MaxViewportSizeX,D3DViewport->GetSizeX());
			MaxViewportSizeY = Max(MaxViewportSizeY,D3DViewport->GetSizeY());

			if(D3DViewport->IsFullscreen())
			{
				check(!NewFullscreenViewport);
				NewFullscreenViewport = D3DViewport;

				// Check that all fullscreen viewports use supported resolutions.
				UINT Width = D3DViewport->GetSizeX();
				UINT Height = D3DViewport->GetSizeY();
				GetSupportedResolution( Width, Height );
				check( Width == D3DViewport->GetSizeX() && Height == D3DViewport->GetSizeY() );
			}
		}
		
		// Determine the adapter and device type to use.
		UINT AdapterIndex = D3DADAPTER_DEFAULT;
		D3DDEVTYPE DeviceType = DEBUG_SHADERS ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;

		// Setup the needed device parameters.
		UINT NewDeviceSizeX = MaxViewportSizeX;
		UINT NewDeviceSizeY = MaxViewportSizeY;
		HWND NewDeviceWindow = NewFullscreenViewport ? (HWND)NewFullscreenViewport->GetWindowHandle() : (HWND)GD3DViewports(0)->GetWindowHandle();
		UBOOL NewDeviceFullscreen = NewFullscreenViewport ? TRUE : FALSE;

		// Check to see if GDirect3DDevice needs to be recreated.
		UBOOL bRecreateDevice = FALSE;
		if(!GDirect3DDevice)
		{
			bRecreateDevice = TRUE;
		}
		else
		{
			if(GD3DDeviceLost)
			{
				// Abort if none of our windows has focus
				HWND FocusWindow = ::GetFocus();
				if ( FocusWindow == NULL || ::IsIconic( FocusWindow ) )
				{
					// Abort and try again next time Present() is called again from RHIEndDrawingViewport().
					return;
				}
				FocusWindow = ::GetFocus();
				bRecreateDevice = TRUE;
			}

			if(NewDeviceFullscreen != GD3DIsFullscreenDevice)
			{
				bRecreateDevice = TRUE;
			}

			if(NewDeviceFullscreen)
			{
				if(GD3DDeviceSizeX != NewDeviceSizeX || GD3DDeviceSizeY != NewDeviceSizeY)
				{
					bRecreateDevice = TRUE;
				}
			}
			else
			{
				if(GD3DDeviceSizeX < NewDeviceSizeX || GD3DDeviceSizeY < NewDeviceSizeY)
				{
					bRecreateDevice = TRUE;
				}
			}

			if(GD3DDeviceWindow != NewDeviceWindow)
			{
				bRecreateDevice	= TRUE;
			}
		}

		if(bRecreateDevice)
		{
			HRESULT Result;

			// Retrieve VSYNC ini setting.
			GConfig->GetBool( TEXT("WinDrv.WindowsClient"), TEXT("UseVSYNC"), GEnableVSync, GEngineIni );

			// Disable VSYNC if -novsync is on the command line.
			GEnableVSync = GEnableVSync && ParseParam(appCmdLine(), TEXT("novsync"));

			// Enable VSYNC if -vsync is on the command line.
			GEnableVSync = GEnableVSync || ParseParam(appCmdLine(), TEXT("vsync"));

			// Setup the present parameters.
			D3DPRESENT_PARAMETERS PresentParameters;
			appMemzero(&PresentParameters,sizeof(PresentParameters));
			PresentParameters.BackBufferCount			= 1;
			PresentParameters.BackBufferFormat			= D3DFMT_A8R8G8B8;
			PresentParameters.BackBufferWidth			= NewDeviceSizeX;
			PresentParameters.BackBufferHeight			= NewDeviceSizeY;
			PresentParameters.SwapEffect				= NewDeviceFullscreen ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
			PresentParameters.Flags						= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
			PresentParameters.EnableAutoDepthStencil	= FALSE;
			PresentParameters.Windowed					= !NewDeviceFullscreen;
			PresentParameters.PresentationInterval		= GEnableVSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
			PresentParameters.hDeviceWindow				= NewDeviceWindow;

			if(GDirect3DDevice)
			{
				TLinkedList<FRenderResource*> *&ResourceList = FRenderResource::GetResourceList();

				// Release dynamic resources and render targets.
				for(TLinkedList<FRenderResource*>::TIterator ResourceIt(ResourceList);ResourceIt;ResourceIt.Next())
				{
					ResourceIt->ReleaseDynamicRHI();
				}
				// release dynamic resources from global movie player
				if( GFullScreenMovie )
				{
					GFullScreenMovie->ReleaseDynamicResources();
				}

				// Release the back-buffer and depth-buffer references to allow resetting the device.
				GD3DBackBuffer.Release();

				// Simple reset the device with the new present parameters.
				do 
				{
#if WITH_PANORAMA
					extern void appPanoramaRenderHookReset(D3DPRESENT_PARAMETERS*);
					// Allow Panorama to reset any resources before reseting the device
					appPanoramaRenderHookReset(&PresentParameters);
#endif
					if( FAILED(Result=GDirect3DDevice->Reset(&PresentParameters) ) )
					{
						// Sleep for a second before trying again if we couldn't reset the device as the most likely
						// cause is the device not being ready/lost which can e.g. occur if a screen saver with "lock"
						// kicks in.
						appSleep(1.0);
					}
				} 
				while( FAILED(Result) );

				// Get pointers to the device's back buffer and depth buffer.
				TD3DRef<IDirect3DSurface9> BackBuffer;
				TD3DRef<IDirect3DSurface9> DepthBuffer;
				VERIFYD3DRESULT(GDirect3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,BackBuffer.GetInitReference()));
				GD3DBackBuffer = FSurfaceRHIRef(FTexture2DRHIRef(),NULL,BackBuffer,TRUE);

				// Reinitialize dynamic resources and render targets.
				for(TLinkedList<FRenderResource*>::TIterator ResourceIt(ResourceList);ResourceIt;ResourceIt.Next())
				{
					ResourceIt->InitDynamicRHI();
				}
			}
			else
			{
				// Get information about the device being used.

				D3DCAPS9 DeviceCaps;
				VERIFYD3DRESULT(GDirect3D->GetDeviceCaps(AdapterIndex,DeviceType,&DeviceCaps));

				D3DADAPTER_IDENTIFIER9	AdapterIdentifier;

				VERIFYD3DRESULT(GDirect3D->GetAdapterIdentifier(AdapterIndex,0,&AdapterIdentifier));

				// Query max texture size supported by the device.
				GMaxTextureMipCount = appCeilLogTwo( Min<INT>( DeviceCaps.MaxTextureHeight, DeviceCaps.MaxTextureWidth ) ) + 1;
				GMaxTextureMipCount = Min<INT>( MAX_TEXTURE_MIP_COUNT, GMaxTextureMipCount );

				// Check for ps_2_0 support.
				if((DeviceCaps.PixelShaderVersion & 0xff00) < 0x0200)
				{
					appErrorf(TEXT("Device does not support pixel shaders 2.0 or greater.  PixelShaderVersion=%08x"),DeviceCaps.PixelShaderVersion);
				}

				// Check for hardware vertex instancing support.
				GSupportsVertexInstancing = (DeviceCaps.PixelShaderVersion & 0xff00) >= 0x0300;

				// Check whether card supports vertex elements sharing offsets.
				GVertexElementsCanShareStreamOffset = (DeviceCaps.DevCaps2 & D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET) ? TRUE : FALSE; 
				
				// Check for required format support.
				D3DDISPLAYMODE	CurrentDisplayMode;
				VERIFYD3DRESULT(GDirect3D->GetAdapterDisplayMode(AdapterIndex,&CurrentDisplayMode));

				// Determine the supported lighting buffer formats.
				UBOOL bSupports16bitRT = !FAILED(GDirect3D->CheckDeviceFormat(
					AdapterIndex,
					DeviceType,
					CurrentDisplayMode.Format,
					D3DUSAGE_RENDERTARGET,
					D3DRTYPE_TEXTURE,
					D3DFMT_A16B16G16R16F
					));

				if(FAILED(GDirect3D->CheckDeviceFormat(
							AdapterIndex,
							DeviceType,
							CurrentDisplayMode.Format,
							D3DUSAGE_RENDERTARGET,
							D3DRTYPE_TEXTURE,
							D3DFMT_A32B32G32R32F
							)))
					appErrorf(TEXT("Device does not support 4 component FP render target format."));

				// Setup the FP RGB pixel formats based on 16-bit FP support.
				GPixelFormats[ PF_FloatRGB	].PlatformFormat	= bSupports16bitRT ? D3DFMT_A16B16G16R16F : D3DFMT_A32B32G32R32F;
				GPixelFormats[ PF_FloatRGB	].BlockBytes		= bSupports16bitRT ? 8 : 16;
				GPixelFormats[ PF_FloatRGBA	].PlatformFormat	= bSupports16bitRT ? D3DFMT_A16B16G16R16F : D3DFMT_A32B32G32R32F;
				GPixelFormats[ PF_FloatRGBA	].BlockBytes		= bSupports16bitRT ? 8 : 16;
				GPixelFormats[ PF_FloatRGBA_Full ].PlatformFormat	= bSupports16bitRT ? D3DFMT_A16B16G16R16F : D3DFMT_A32B32G32R32F;
				GPixelFormats[ PF_FloatRGBA_Full ].BlockBytes		= bSupports16bitRT ? 8 : 16;

				// can't read from depth textures on pc (although certain ATI cards can, but DST's are not optimal for use as depth buffers)
				GSupportsDepthTextures = FALSE;

				// Determine whether hardware shadow mapping is supported.
				GSupportsHardwarePCF = SUCCEEDED(
					GDirect3D->CheckDeviceFormat(
						AdapterIndex, 
						DeviceType, 
						CurrentDisplayMode.Format, 
						D3DUSAGE_DEPTHSTENCIL,
						D3DRTYPE_TEXTURE,
						D3DFMT_D24X8 
						)
					);

				if (GRHIShaderPlatform != SP_PCD3D_SM2)
				{
					// Check for D24 support, which indicates that ATI's Fetch4 is also supported
					GSupportsFetch4 = SUCCEEDED(
						GDirect3D->CheckDeviceFormat(
							AdapterIndex, 
							DeviceType, 
							CurrentDisplayMode.Format, 
							D3DUSAGE_DEPTHSTENCIL,
							D3DRTYPE_TEXTURE,
							(D3DFORMAT)(MAKEFOURCC('D','F','2','4'))
							)
						);
				}

				// Determine whether filtering of floating point textures is supported.
				if (FAILED(
					GDirect3D->CheckDeviceFormat(
						AdapterIndex,
						DeviceType,
						CurrentDisplayMode.Format,
						D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_FILTER,
						D3DRTYPE_TEXTURE,
						bSupports16bitRT ? D3DFMT_A16B16G16R16F : D3DFMT_A32B32G32R32F
					)))
				{
					//only overwrite if not supported, since the previous setting may have been specified on the command line
					GSupportsFPFiltering = FALSE;
				}

				// Verify that the device supports the required single component 32-bit floating point render target format.
				if(FAILED(GDirect3D->CheckDeviceFormat(
							AdapterIndex,
							DeviceType,
							CurrentDisplayMode.Format,
							D3DUSAGE_RENDERTARGET,
							D3DRTYPE_TEXTURE,
							D3DFMT_R32F
							)))
					appErrorf(TEXT("Device does not support 1x32 FP render target format."));

				// Query for YUV texture format support.

				if( SUCCEEDED(GDirect3D->CheckDeviceFormat( AdapterIndex, DeviceType, CurrentDisplayMode.Format, D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, D3DFMT_UYVY	) ) )
				{
					// Query for SRGB read support (gamma correction on texture sampling) for YUV texture format. E.g. not supported on Radeon 9800.
					if( FAILED(GDirect3D->CheckDeviceFormat( AdapterIndex, DeviceType, CurrentDisplayMode.Format, D3DUSAGE_QUERY_SRGBREAD, D3DRTYPE_TEXTURE, D3DFMT_UYVY	) ) )
					{
						GPixelFormats[PF_UYVY].Flags |= PF_REQUIRES_GAMMA_CORRECTION;
					}
					else
					{
						GPixelFormats[PF_UYVY].Flags &= ~ PF_REQUIRES_GAMMA_CORRECTION;
					}

					GPixelFormats[PF_UYVY].Supported = 1;
				}
				else
				{
					GPixelFormats[PF_UYVY].Supported = 0;
				}

				// Try creating a new Direct3D device till it either succeeds or fails with an error code != D3DERR_DEVICELOST.
				
				while( 1 )
				{
#if WITH_PANORAMA
					// Games for Windows Live requires the multithreaded flag or it will BSOD
					DWORD CreationFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED;
#else
					DWORD CreationFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE /*| D3DCREATE_MULTITHREADED*/;
#endif
					// Driver management of video memory currently is FAR from being optimal for streaming!
					CreationFlags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT;

					UINT AdapterNumber = D3DADAPTER_DEFAULT;

					// Automatically detect nvperfhud device
					for( UINT Adapter=0; Adapter < GDirect3D->GetAdapterCount(); Adapter++ )
					{
						D3DADAPTER_IDENTIFIER9 Identifier;
						HRESULT Res = GDirect3D->GetAdapterIdentifier( Adapter, 0, &Identifier );
						if( appStrstr( *FString(Identifier.Description), TEXT("PerfHUD") ) != NULL )
						{
							AdapterNumber = Adapter;
							DeviceType = D3DDEVTYPE_REF;
							break;
						}
					}

					Result = GDirect3D->CreateDevice(
						AdapterNumber == INDEX_NONE ? D3DADAPTER_DEFAULT : AdapterNumber,
						DeviceType,
						NewDeviceWindow,
						CreationFlags,
						&PresentParameters,
						GDirect3DDevice.GetInitReference()
						);

					if( Result != D3DERR_DEVICELOST )
					{
#if WITH_PANORAMA
						extern void appPanoramaRenderHookInit(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
						// Allow G4WLive to allocate any resources it needs
						appPanoramaRenderHookInit(GDirect3DDevice,&PresentParameters);
#endif
						break;
					}

					appSleep( 0.5f );
				}

				VERIFYD3DRESULT(Result);

				// Initialize the platform pixel format map.
				GPixelFormats[ PF_Unknown		].PlatformFormat	= D3DFMT_UNKNOWN;
				GPixelFormats[ PF_A32B32G32R32F	].PlatformFormat	= D3DFMT_A32B32G32R32F;
				GPixelFormats[ PF_A8R8G8B8		].PlatformFormat	= D3DFMT_A8R8G8B8;
				GPixelFormats[ PF_G8			].PlatformFormat	= D3DFMT_L8;
				GPixelFormats[ PF_G16			].PlatformFormat	= D3DFMT_UNKNOWN;	// Not supported for rendering.
				GPixelFormats[ PF_DXT1			].PlatformFormat	= D3DFMT_DXT1;
				GPixelFormats[ PF_DXT3			].PlatformFormat	= D3DFMT_DXT3;
				GPixelFormats[ PF_DXT5			].PlatformFormat	= D3DFMT_DXT5;
				GPixelFormats[ PF_UYVY			].PlatformFormat	= D3DFMT_UYVY;
				GPixelFormats[ PF_DepthStencil	].PlatformFormat	= D3DFMT_D24S8;
				GPixelFormats[ PF_ShadowDepth	].PlatformFormat	= D3DFMT_D24X8;
				GPixelFormats[ PF_FilteredShadowDepth ].PlatformFormat = D3DFMT_D24X8;
				GPixelFormats[ PF_R32F			].PlatformFormat	= D3DFMT_R32F;
				GPixelFormats[ PF_G16R16		].PlatformFormat	= D3DFMT_G16R16;
				GPixelFormats[ PF_G16R16F		].PlatformFormat	= D3DFMT_G16R16F;
				GPixelFormats[ PF_G32R32F		].PlatformFormat	= D3DFMT_G32R32F;
				GPixelFormats[ PF_D24			].PlatformFormat    = (D3DFORMAT)(MAKEFOURCC('D','F','2','4'));

				// Fall back to D3DFMT_A8R8G8B8 if D3DFMT_A2B10G10R10 is not a supported render target format.
				if( FAILED( GDirect3D->CheckDeviceFormat(
					AdapterIndex,
					DeviceType,
					CurrentDisplayMode.Format,
					D3DUSAGE_RENDERTARGET,
					D3DRTYPE_TEXTURE,
					D3DFMT_A2B10G10R10 ) ) )
				{
					GPixelFormats[ PF_A2B10G10R10 ].PlatformFormat = D3DFMT_A8R8G8B8;
				}
				else
				{
					GPixelFormats[ PF_A2B10G10R10 ].PlatformFormat = D3DFMT_A2B10G10R10;
				}

				// Fall back to D3DFMT_A8R8G8B8 if D3DFMT_A16B16G16R16 is not a supported render target format. In theory
				// we could try D3DFMT_A32B32G32R32 but cards not supporting the 16 bit variant are usually too slow to use
				// the full 32 bit format.
				if( FAILED( GDirect3D->CheckDeviceFormat(
					AdapterIndex,
					DeviceType,
					CurrentDisplayMode.Format,
					D3DUSAGE_RENDERTARGET,
					D3DRTYPE_TEXTURE,
					D3DFMT_A16B16G16R16 ) ) )
				{
					GPixelFormats[ PF_A16B16G16R16 ].PlatformFormat = D3DFMT_A8R8G8B8;
				}
				else
				{
					GPixelFormats[ PF_A16B16G16R16 ].PlatformFormat = D3DFMT_A16B16G16R16;
				}

				// Get pointers to the device's back buffer and depth buffer.
				TD3DRef<IDirect3DSurface9> BackBuffer;
				VERIFYD3DRESULT(GDirect3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,BackBuffer.GetInitReference()));
				GD3DBackBuffer = FSurfaceRHIRef(FTexture2DRHIRef(),NULL,BackBuffer,TRUE);

				// Notify all initialized FRenderResources that there's a valid RHI device to create their RHI resources for now.
				check(!GIsRHIInitialized);
				GIsRHIInitialized = TRUE;
				for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
				{
					ResourceIt->InitDynamicRHI();
				}
				for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
				{
					ResourceIt->InitRHI();
				}
			}

			// Update saved device settings.
			GD3DFullscreenViewport = NewFullscreenViewport;
			GD3DDeviceSizeX = NewDeviceSizeX;
			GD3DDeviceSizeY = NewDeviceSizeY;
			GD3DDeviceWindow = NewDeviceWindow;
			GD3DIsFullscreenDevice = NewDeviceFullscreen;
			GD3DDeviceLost = FALSE;

			// Tell the windows to redraw when they can.
			for(INT ViewportIndex = 0;ViewportIndex < GD3DViewports.Num();ViewportIndex++)
			{
				FD3DViewport* D3DViewport = GD3DViewports(ViewportIndex);
				::PostMessage( (HWND) D3DViewport->GetWindowHandle(), WM_PAINT, 0, 0 );
			}
		}
	}
	else
	{
		// If no viewports are open, clean up the existing device.
		CleanupD3DDevice();
		GDirect3DDevice = NULL;
		GD3DBackBuffer.Release();		
	}
}

/**
 * Returns a supported screen resolution that most closely matches the input.
 * @param Width - Input: Desired resolution width in pixels. Output: A width that the platform supports.
 * @param Height - Input: Desired resolution height in pixels. Output: A height that the platform supports.
 */
void GetSupportedResolution( UINT &Width, UINT &Height )
{
	// Create the Direct3D object if necessary.
	if ( !GDirect3D )
	{
		*GDirect3D.GetInitReference() = Direct3DCreate9(D3D_SDK_VERSION);
	}
	if ( !GDirect3D )
	{
		appErrorf(
			NAME_FriendlyError,
			TEXT("Please install DirectX 9.0c or later (see Release Notes for instructions on how to obtain it)")
			);
	}

	// Determine the adapter and device type to use.
	UINT AdapterIndex = D3DADAPTER_DEFAULT;
	D3DDEVTYPE DeviceType = DEBUG_SHADERS ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;

	// Find the screen size that most closely matches the desired resolution.
	D3DDISPLAYMODE BestMode = { 0, 0, 0, D3DFMT_A8R8G8B8 };
	UINT InitializedMode = FALSE;
	UINT NumAdapterModes = GDirect3D->GetAdapterModeCount(AdapterIndex,D3DFMT_X8R8G8B8);
	for(UINT ModeIndex = 0;ModeIndex < NumAdapterModes;ModeIndex++)
	{
		D3DDISPLAYMODE DisplayMode;
		GDirect3D->EnumAdapterModes(AdapterIndex,D3DFMT_X8R8G8B8,ModeIndex,&DisplayMode);

		UBOOL IsEqualOrBetterWidth = Abs((INT)DisplayMode.Width - (INT)Width) <= Abs((INT)BestMode.Width - (INT)Width);
		UBOOL IsEqualOrBetterHeight = Abs((INT)DisplayMode.Height - (INT)Height) <= Abs((INT)BestMode.Height - (INT)Height);
		if(!InitializedMode || (IsEqualOrBetterWidth && IsEqualOrBetterHeight))
		{
			BestMode = DisplayMode;
			InitializedMode = TRUE;
		}
	}
	check(InitializedMode);
	Width = BestMode.Width;
	Height = BestMode.Height;
}

FCommandContextRHI* RHIGetGlobalContext()
{
	return NULL;
}

/**
 *	Retrieve available screen resolutions.
 *
 *	@param	ResolutionLists		TArray<TArray<FScreenResolutionRHI>> parameter that will be filled in.
 *	@param	bIgnoreRefreshRate	If TRUE, ignore refresh rates.
 *
 *	@return	UBOOL				TRUE if successfully filled the array
 */
UBOOL RHIGetAvailableResolutions(TArray<ScreenResolutionArray>& ResolutionLists, UBOOL bIgnoreRefreshRate)
{
	if (!GDirect3D)
	{
		return FALSE;
	}

	INT MinAllowableResolutionX = 0;
	INT MinAllowableResolutionY = 0;
	INT MaxAllowableResolutionX = 10480;
	INT MaxAllowableResolutionY = 10480;
	INT MinAllowableRefreshRate = 0;
	INT MaxAllowableRefreshRate = 10480;

	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MinAllowableResolutionX"), MinAllowableResolutionX, GEngineIni);
	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MinAllowableResolutionY"), MinAllowableResolutionY, GEngineIni);
	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MaxAllowableResolutionX"), MaxAllowableResolutionX, GEngineIni);
	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MaxAllowableResolutionY"), MaxAllowableResolutionY, GEngineIni);
	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MinAllowableRefreshRate"), MinAllowableRefreshRate, GEngineIni);
	GConfig->GetInt(TEXT("WinDrv.WindowsClient"), TEXT("MaxAllowableRefreshRate"), MaxAllowableRefreshRate, GEngineIni);

	if (MaxAllowableResolutionX == 0)
	{
		MaxAllowableResolutionX = 10480;
	}
	if (MaxAllowableResolutionY == 0)
	{
		MaxAllowableResolutionY = 10480;
	}
	if (MaxAllowableRefreshRate == 0)
	{
		MaxAllowableRefreshRate = 10480;
	}

	INT AdapterCount = GDirect3D->GetAdapterCount();
	for (INT AdapterIndex = 0; AdapterIndex < AdapterCount; AdapterIndex++)
	{
		INT TempIndex = ResolutionLists.AddZeroed();
		ScreenResolutionArray& ResolutionArray = ResolutionLists(TempIndex);
		INT ModeCount = GDirect3D->GetAdapterModeCount(AdapterIndex, D3DFMT_X8R8G8B8);
		for (INT ModeIndex = 0; ModeIndex < ModeCount; ModeIndex++)
		{
			D3DDISPLAYMODE DisplayMode;
			HRESULT d3dResult = GDirect3D->EnumAdapterModes(AdapterIndex, D3DFMT_X8R8G8B8, ModeIndex, &DisplayMode);
			if (d3dResult == D3D_OK)
			{
				if (((INT)DisplayMode.Width >= MinAllowableResolutionX) &&
					((INT)DisplayMode.Width <= MaxAllowableResolutionX) &&
					((INT)DisplayMode.Height >= MinAllowableResolutionY) &&
					((INT)DisplayMode.Height <= MaxAllowableResolutionY)
					)
				{
					UBOOL bAddIt = TRUE;
					if (bIgnoreRefreshRate == FALSE)
					{
						if (((INT)DisplayMode.RefreshRate < MinAllowableRefreshRate) ||
							((INT)DisplayMode.RefreshRate > MaxAllowableRefreshRate)
							)
						{
							continue;
						}
					}
					else
					{
						// See if it is in the list already
						for (INT CheckIndex = 0; CheckIndex < ResolutionArray.Num(); CheckIndex++)
						{
							FScreenResolutionRHI& CheckResolution = ResolutionArray(CheckIndex);
							if ((CheckResolution.Width == DisplayMode.Width) &&
								(CheckResolution.Height == DisplayMode.Height))
							{
								// Already in the list...
								bAddIt = FALSE;
								break;
							}
						}
					}

					if (bAddIt)
					{
						// Add the mode to the list
						INT Temp2Index = ResolutionArray.AddZeroed();
						FScreenResolutionRHI& ScreenResolution = ResolutionArray(Temp2Index);

						ScreenResolution.Width = DisplayMode.Width;
						ScreenResolution.Height = DisplayMode.Height;
						ScreenResolution.RefreshRate = DisplayMode.RefreshRate;
					}
				}
			}
		}
	}

	return TRUE;
}

#endif
