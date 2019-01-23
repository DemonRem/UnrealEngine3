/*=============================================================================
	NullRHI.cpp: Null RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#if USE_NULL_RHI

// Globals

/** The maximum number of mip-maps that a texture can contain. */
INT GMaxTextureMipCount = MAX_TEXTURE_MIP_COUNT;

/** The minimum number of mip-maps that always remain resident.	*/
INT	GMinTextureResidentMipCount = 7;

/** TRUE if pixels are clipped to 0<Z<W instead of -W<Z<W. */
const UBOOL GPositiveZRange = TRUE;

/** The offset from the upper left corner of a pixel to the position which is used to sample textures for fragments of that pixel. */
const FLOAT GPixelCenterOffset = 0.5f;

/** TRUE if PF_DepthStencil textures can be created and sampled. */
UBOOL GSupportsDepthTextures = FALSE;

/** 
* TRUE if PF_DepthStencil textures can be created and sampled to obtain PCF values. 
* This is different from GSupportsDepthTextures in three ways:
*	-results of sampling are PCF values, not depths
*	-a color target must be bound with the depth stencil texture even if never written to or read from,
*		due to API restrictions
*	-a dedicated resolve surface may or may not be necessary
*/
UBOOL GSupportsHardwarePCF = FALSE;

/** TRUE if D24 textures can be created and sampled, retrieving 4 neighboring texels in a single lookup. */
UBOOL GSupportsFetch4 = FALSE;

/**
* TRUE if floating point filtering is supported
*/
UBOOL GSupportsFPFiltering = TRUE;

/** Can we handle quad primitives? */
const UBOOL GSupportsQuads = FALSE;

/** Are we using an inverted depth buffer? */
const UBOOL GUsesInvertedZ = FALSE;

/** True if the render hardware has been initialized. */
UBOOL GIsRHIInitialized = TRUE;

/** Shadow buffer size */
INT	GShadowDepthBufferSize = 1024;

/** Bias exponent used to apply to shader color output when rendering to the scene color surface */
INT GCurrentColorExpBias = 0;

/** The shader platform to load from the cache */
EShaderPlatform GRHIShaderPlatform = SP_PCD3D_SM3;

/** Toggle for MSAA tiling support */
UBOOL GUseTilingCode = FALSE;
/**
*	The size to check against for Draw*UP call vertex counts.
*	If greater than this value, the draw call will not occur.
*/
INT GDrawUPVertexCheckCount = MAXINT;		
/**
*	The size to check against for Draw*UP call index counts.
*	If greater than this value, the draw call will not occur.
*/
INT GDrawUPIndexCheckCount = MAXINT;

/** The window handle associated with the null RHI device. */
HWND GD3DDeviceWindow = NULL;

/** TRUE if the rendering hardware supports vertex instancing. */
UBOOL GSupportsVertexInstancing = FALSE;

/** If FALSE code needs to patch up vertex declaration. */
UBOOL GVertexElementsCanShareStreamOffset = TRUE;

FCommandContextRHI* RHIGetGlobalContext()
{
	return NULL;
}

UBOOL RHIGetAvailableResolutions(TArray<ScreenResolutionArray>& ResolutionLists, UBOOL bIgnoreRefreshRate)
{
	return FALSE;
}

/**
 * Return a shared large static buffer that can be used to return from any 
 * function that needs to return a valid pointer (but can be garbage data)
 */
void* GetStaticBuffer()
{
	static void* Buffer = NULL;
	if (!Buffer)
	{
		// allocate a 4 meg buffer, should be big enough for any texture/surface
		Buffer = appMalloc(4 * 1024 * 1024);
	}

	return Buffer;
}

FSamplerStateRHIRef CreateSamplerStateRHI(
	ESamplerFilter Filter,
	ESamplerAddressMode AddressU,
	ESamplerAddressMode AddressV,
	ESamplerAddressMode AddressW,
	FLOAT MipMapLODBias
	)
{
	return FSamplerStateRHIRef(new FNullRHISamplerState);
}

extern FVertexDeclarationRHIRef RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements)
{
	return FVertexDeclarationRHIRef(new FNullRHIVertexDeclaration);
}

FVertexShaderRHIRef RHICreateVertexShader(const TArray<BYTE>& Code)
{
	FVertexShaderRHIRef VertexShader;
	return VertexShader;
}

FPixelShaderRHIRef RHICreatePixelShader(const TArray<BYTE>& Code)
{
	FPixelShaderRHIRef PixelShader;
	return PixelShader;
}

FBoundShaderStateRHIRef RHICreateBoundShaderState(FVertexDeclarationRHIParamRef VertexDeclaration, DWORD *StreamStrides, FVertexShaderRHIParamRef VertexShader, FPixelShaderRHIParamRef PixelShader)
{
	FBoundShaderStateRHIRef BoundShaderState;
	return BoundShaderState;
}

void RHIInitializeShaderPlatform()
{}

FIndexBufferRHIRef RHICreateIndexBuffer(UINT Stride,UINT Size,FResourceArrayInterface* ResourceArray,UBOOL bIsDynamic)
{
	FIndexBufferRHIRef IndexBuffer;
	return IndexBuffer;
}

void* RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer,UINT Offset,UINT Size)
{
	void* Data = GetStaticBuffer();
	return Data;
}

void RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer)
{
}

FVertexBufferRHIRef RHICreateVertexBuffer(UINT Size,FResourceArrayInterface* ResourceArray,UBOOL bIsDynamic)
{
	FVertexBufferRHIRef VertexBuffer;
	return VertexBuffer;
}

void* RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer,UINT Offset,UINT Size,UBOOL bReadOnly)
{
	void* Data = GetStaticBuffer();
	return Data;
}

void RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer)
{
}

void RHIReadSurfaceData(FSurfaceRHIRef &Surface,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY,TArray<BYTE>& OutData,ECubeFace CubeFace)
{
}

void RHISetRenderTarget(FCommandContextRHI* Context, FSurfaceRHIParamRef NewRenderTarget, FSurfaceRHIParamRef NewDepthStencilTarget)
{
}

FNullRHIViewport::FNullRHIViewport(UINT InSizeX,UINT InSizeY,UBOOL bInIsFullscreen)
: SizeX(InSizeX)
, SizeY(InSizeY)
, bIsFullscreen(bInIsFullscreen)
{
	FTexture2DRHIRef Texture2D;
	BackBuffer = RHICreateTargetableSurface(
		SizeX,
		SizeY,
		PF_A8R8G8B8,
		Texture2D,
		TargetSurfCreate_Dedicated|TargetSurfCreate_Readable,
		TEXT("BackBuffer")
		);
}

void RHIClear(FCommandContextRHI* Context,UBOOL bClearColor,const FLinearColor& Color,UBOOL bClearDepth,FLOAT Depth,UBOOL bClearStencil,DWORD Stencil)
{
}

void RHIDrawPrimitive(FCommandContextRHI* Context,UINT PrimitiveType,UINT BaseVertexIndex,UINT NumPrimitives)
{
}

void RHIDrawIndexedPrimitive(FCommandContextRHI* Context,FIndexBufferRHIParamRef IndexBuffer,UINT PrimitiveType,INT BaseVertexIndex,UINT MinIndex,UINT NumVertices,UINT StartIndex,UINT NumPrimitives)
{
}

void RHIDrawPrimitiveUP(FCommandContextRHI* Context,UINT PrimitiveType,UINT NumPrimitives,const void* VertexData,UINT VertexDataStride)
{
}

void RHIDrawIndexedPrimitiveUP(FCommandContextRHI* Context,UINT PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT NumPrimitives,const void* IndexData,UINT IndexDataStride,const void* VertexData,UINT VertexDataStride)
{
}

void RHIDrawSpriteParticles(FCommandContextRHI* Context, const FMeshElement& Mesh)
{
}

void RHIDrawSubUVParticles(FCommandContextRHI* Context, const FMeshElement& Mesh)
{
}

void RHISetBoundShaderState(FCommandContextRHI* Context,FBoundShaderStateRHIParamRef NewShader)
{
}

FSamplerStateRHIRef RHICreateSamplerState(const FSamplerStateInitializerRHI& Initializer)
{
	FSamplerStateRHIRef SamplerState;
	return SamplerState;
}

void RHISetSamplerState(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT SamplerIndex,FSamplerStateRHIParamRef NewState,FTextureRHIParamRef NewTexture)
{
}

void RHISetVertexShaderParameter(FCommandContextRHI* Context,FVertexShaderRHIParamRef PixelShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue)
{
}

void RHISetPixelShaderParameter(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue)
{
}

void RHISetRenderTargetBias( FCommandContextRHI* Context, FLOAT ColorBias )
{
}

void RHISetViewParameters( FCommandContextRHI* Context, const FSceneView* View, const FMatrix& ViewProjectionMatrix, const FVector4& ViewOrigin )
{
}

void RHISetViewPixelParameters( FCommandContextRHI* Context, const class FSceneView* View, FPixelShaderRHIParamRef PixelShader, const FShaderParameter* SceneDepthCalcParameter, const FShaderParameter* ScreenPositionScaleBiasParameter )
{
}

void RHISetShaderRegisterAllocation(UINT NumVertexShaderRegisters, UINT NumPixelShaderRegisters)
{
}

void RHIReduceTextureCachePenalty( FPixelShaderRHIParamRef PixelShader )
{
}

void RHISetStreamSource(FCommandContextRHI* Context,UINT StreamIndex,FVertexBufferRHIParamRef VertexBuffer,UINT Stride,UBOOL bUseInstanceIndex,UINT NumVerticesPerInstance,UINT NumInstances)
{
}


FTexture2DRHIRef RHICreateTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,DWORD Flags)
{
	FTexture2DRHIRef Texture2D;
	return Texture2D;
}

void* RHILockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail)
{
	void* Data = GetStaticBuffer();
	// Stride has no meaning here - there is no 'size' stored with an NullRHI texture.
	DestStride = 0;
	return Data;
}

void RHIUnlockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bLockWithinMiptail)
{
}

INT RHIGetMipTailIdx(FTexture2DRHIParamRef Texture)
{
	return -1;
}

void RHICopyTexture2D(FTexture2DRHIParamRef DstTexture, UINT MipIdx, INT BaseSizeX, INT BaseSizeY, INT Format, const TArray<FCopyTextureRegion2D>& Regions)
{
}

void RHICopyMipToMipAsync(FTexture2DRHIParamRef SrcTexture, INT SrcMipIndex, FTexture2DRHIParamRef DestTexture, INT DestMipIndex, INT Size, FThreadSafeCounter& Counter)
{
}


FTextureCubeRHIRef RHICreateTextureCube( UINT Size, BYTE Format, UINT NumMips, DWORD Flags )
{
	FTextureCubeRHIRef TextureCube;
	return TextureCube;
}

void* RHILockTextureCubeFace(FTextureCubeRHIParamRef Texture,UINT FaceIndex,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail)
{	
	void* Data = GetStaticBuffer();
	// Stride has no meaning here - there is no 'size' stored with an NullRHI texture.
	DestStride = 0;
	return Data;
}

void RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef Texture,UINT FaceIndex,UINT MipIndex,UBOOL bLockWithinMiptail)
{
}

FBlendStateRHIRef RHICreateBlendState(const FBlendStateInitializerRHI& Initializer)
{
	FBlendStateRHIRef BlendState;
	return BlendState;
}

void RHISetBlendState(FCommandContextRHI* Context,FBlendStateRHIParamRef NewState)
{
}

FRasterizerStateRHIRef RHICreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer)
{
	FRasterizerStateRHIRef RasterizerState;
	return RasterizerState;
}

void RHISetRasterizerState(FCommandContextRHI* Context,FRasterizerStateRHIParamRef NewState)
{
}

void RHISetDepthState(FCommandContextRHI* Context,FDepthStateRHIParamRef NewState)
{
}
void RHISetStencilState(FCommandContextRHI* Context,FStencilStateRHIParamRef NewState)
{
}

FDepthStateRHIRef RHICreateDepthState(const FDepthStateInitializerRHI& Initializer)
{
	FDepthStateRHIRef DepthState;
	return DepthState;
}

FStencilStateRHIRef RHICreateStencilState(const FStencilStateInitializerRHI& Initializer)
{
	FStencilStateRHIRef StencilState;
	return StencilState;
}

void RHISetColorWriteEnable(FCommandContextRHI* Context,UBOOL bEnable)
{
}

void RHISetColorWriteMask(FCommandContextRHI* Context, UINT ColorWriteMask)
{
}

FSurfaceRHIRef RHIGetViewportBackBuffer(FViewportRHIParamRef Viewport)
{
	FSurfaceRHIRef BackBuffer;
	return BackBuffer;
}

/*
 * Returns the total GPU time taken to render the last frame. Same metric as appCycles().
 */
DWORD RHIGetGPUFrameCycles()
{
	return 0;
}

/*
 * Returns an approximation of the available memory that textures can use, which is video + AGP where applicable, rounded to the nearest MB, in MB.
 */
DWORD RHIGetAvailableTextureMemory()
{
	return 0;
}

FSurfaceRHIRef RHICreateTargetableSurface(
	UINT SizeX,
	UINT SizeY,
	BYTE Format,
	FTexture2DRHIParamRef ResolveTargetTexture,
	DWORD Flags,
	const TCHAR* UsageStr
	)
{
	FSurfaceRHIRef TargetSurface;
	return TargetSurface;
}

FSurfaceRHIRef RHICreateTargetableCubeSurface(
	UINT SizeX,
	BYTE Format,
	FTextureCubeRHIParamRef ResolveTargetTexture,
	ECubeFace CubeFace,
	DWORD Flags,
	const TCHAR* UsageStr
	)
{
	FSurfaceRHIRef TargetSurface;
	return TargetSurface;
}

void RHICopyFromResolveTarget(FSurfaceRHIParamRef DestSurface)
{
}

void RHICopyFromResolveTargetFast(FSurfaceRHIParamRef DestSurface)
{
}

void RHICopyToResolveTarget(FSurfaceRHIParamRef SourceSurface, UBOOL bKeepOriginalSurface, const FResolveParams& ResolveParams)
{
}

void RHIDiscardSurface(FSurfaceRHIParamRef Surface)
{
}

void RHISetViewport(FCommandContextRHI* Context,UINT MinX,UINT MinY,FLOAT MinZ,UINT MaxX,UINT MaxY,FLOAT MaxZ)
{
}

FOcclusionQueryRHIRef RHICreateOcclusionQuery()
{
	FOcclusionQueryRHIRef OcclusionQuery;
	return OcclusionQuery;
}

void RHIBeginOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery)
{
}

void RHIEndOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery)
{
}

UBOOL RHIGetOcclusionQueryResult(FOcclusionQueryRHIParamRef OcclusionQuery,DWORD& OutNumPixels,UBOOL bWait)
{
	return TRUE;
}

void RHIKickCommandBuffer( FCommandContextRHI* Context )
{
}

void RHIMSAAInitPrepass()
{
}
void RHIMSAAFixViewport()
{
}
void RHIMSAABeginRendering()
{
}
void RHIMSAAEndRendering(const FTexture2DRHIRef& DepthTexture, const FTexture2DRHIRef& ColorTexture)
{
}
void RHIMSAARestoreDepth(const FTexture2DRHIRef& DepthTexture)
{
}


FViewportRHIRef RHICreateViewport(void* WindowHandle, UINT SizeX, UINT SizeY, UBOOL bIsFullscreen)
{
	check( IsInGameThread() );
	return FViewportRHIRef(new FNullRHIViewport(SizeX,SizeY,bIsFullscreen));
}

void RHIResizeViewport(FViewportRHIParamRef Viewport, UINT SizeX, UINT SizeY, UBOOL bIsFullscreen)
{
	check( IsInGameThread() );
}

void RHITick( FLOAT DeltaTime )
{
	check( IsInGameThread() );
}

void RHIBeginDrawingViewport(FViewportRHIParamRef Viewport)
{
	check(IsValidRef(Viewport));

	// update any resources that needed a deferred update
	FDeferredUpdateResource::UpdateResources();
}

void RHIEndDrawingViewport(FViewportRHIParamRef Viewport, UBOOL bPresent, UBOOL bLockToVsync)
{
	check(IsValidRef(Viewport));
}

FSurfaceRHIRef GetViewportBackBufferRHI(FViewportRHIParamRef Viewport)
{
	return FSurfaceRHIRef(Viewport->BackBuffer);
}

UBOOL GD3DRenderingIsSuspended = FALSE;

void RHISuspendRendering()
{
	// Not supported
}

void RHIResumeRendering()
{
	// Not supported
}

UBOOL RHIGetTextureMemoryStats( INT& /*AllocatedMemorySize*/, INT& /*AvailableMemorySize*/ )
{
	return FALSE;
}

void RHIEndDrawIndexedPrimitiveUP(FCommandContextRHI* Context)
{
}

void RHIBeginDrawIndexedPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData, UINT MinVertexIndex, UINT NumIndices, UINT IndexDataStride, void*& OutIndexData)
{
	OutVertexData = GetStaticBuffer();
	OutIndexData = GetStaticBuffer();
}

void RHIBeginDrawPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData)
{
	OutVertexData = GetStaticBuffer();
}

void RHIEndDrawPrimitiveUP(FCommandContextRHI* Context)
{
}

void RHISetScissorRect(FCommandContextRHI* Context,UBOOL bEnable,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY)
{
}

/**
 * Set depth bounds test state.  
 * When enabled, incoming fragments are killed if the value already in the depth buffer is outside of [MinZ, MaxZ]
 */
void RHISetDepthBoundsTest(FCommandContextRHI* Context,UBOOL bEnable,FLOAT MinZ,FLOAT MaxZ)
{
}


// Not supported
void RHIBeginHiStencilRecord(FCommandContextRHI* Context) { }
void RHIBeginHiStencilPlayback(FCommandContextRHI* Context) { }
void RHIEndHiStencil(FCommandContextRHI* Context) { }


void RHISetRasterizerStateImmediate(FCommandContextRHI* Context,const FRasterizerStateInitializerRHI &ImmediateState)
{

}

SIZE_T RHICalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format)
{
	return CalculateImageBytes(SizeX,SizeY,SizeZ,Format);
}



// needed for xenon compiling.  These will not be used but just need to exist for the externing we have

/** Whether we should trace the render tick after present to next one. */
UBOOL GShouldTraceRenderTick;

const INT GNumTilingModes = -1;
static const INT GMaxTiles = -1;
UINT GTilingMode = 0;

/** Enable or disable vsync, as specified by the -novsync command line parameter. */
UBOOL GEnableVSync = TRUE;

FLOAT GDepthBiasOffset = 0.0f;

/** Value between 0-100 that determines the percentage of the vertical scan that is allowed to pass while still allowing us to swap when VSYNC'ed.
This is used to get the same behavior as the old *_OR_IMMEDIATE present modes. */
DWORD GPresentImmediateThreshold = 100;

/** The width of the D3D device's back buffer. */
INT GScreenWidth = 1280;

/** The height of the D3D device's back buffer. */
INT GScreenHeight= 720;


#if XBOX || PS3
void appBeginDrawEvent(const FColor& Color,const TCHAR* Text)
{
}

/**
* Ends the current PIX event
*/
void appEndDrawEvent(void)
{
}
#endif // XBOX

UBOOL XeD3DCompileShader(
						 const TCHAR* SourceFilename,
						 const TCHAR* FunctionName,
						 FShaderTarget Target,
						 const FShaderCompilerEnvironment& Environment,
						 FShaderCompilerOutput& Output
						 )
{
	return FALSE;
}



FSharedTexture2DRHIRef RHICreateSharedTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,FSharedMemoryResourceRHIRef SharedMemory,DWORD Flags)
{
	FSharedTexture2DRHIRef SharedTexture2D(new FNullRHITexture2D() );
	return SharedTexture2D;
}

FSharedMemoryResourceRHIRef RHICreateSharedMemory(SIZE_T Size)
{
	// create the shared memory resource
	FSharedMemoryResourceRHIRef SharedMemory(new FNullRHIResource());
	return SharedMemory;
}


void XePerformSwap( UBOOL bSyncToPresentationInterval, UBOOL bLockToVsync )
{
}

UBOOL HACK_ParamIsMatrix = FALSE;

void* GRSXBaseAddress = 0;

/**
 * Perform PS3 GCM/RHI initialization that needs to wait until after appInit has happened
 */
void PS3PostAppInitRHIInitialization()
{
}

#else

// Suppress linker warning "warning LNK4221: no public symbols found; archive member will be inaccessible"
INT NullRHILinkerHelper;

#endif // USE_NULL_RHI


