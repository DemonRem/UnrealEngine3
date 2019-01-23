/*=============================================================================
	SceneRenderTargets.cpp: Scene render target implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/*-----------------------------------------------------------------------------
FSceneRenderTargets
-----------------------------------------------------------------------------*/

/** The global render targets used for scene rendering. */
TGlobalResource<FSceneRenderTargets> GSceneRenderTargets;

void FSceneRenderTargets::Allocate(UINT MinSizeX,UINT MinSizeY)
{
#if CONSOLE
	// force to always use the global screen sizes iot avoid reallocating the scene buffers
	MinSizeX = GScreenWidth;
	MinSizeY = GScreenHeight;
#endif

	if(BufferSizeX < MinSizeX || BufferSizeY < MinSizeY)
	{
		// Reinitialize the render targets for the given size.
		BufferSizeX = Max(BufferSizeX,MinSizeX);
		BufferSizeY = Max(BufferSizeY,MinSizeY);
		FilterDownsampleFactor = 4;
		FilterBufferSizeX = BufferSizeX / FilterDownsampleFactor + 2;
		FilterBufferSizeY = BufferSizeY / FilterDownsampleFactor + 2;

		FogAccumulationDownsampleFactor = 2;
		FogAccumulationBufferSizeX = BufferSizeX / FogAccumulationDownsampleFactor;
		FogAccumulationBufferSizeY = BufferSizeY / FogAccumulationDownsampleFactor;

		Release();
		Init();
	}
}

void FSceneRenderTargets::BeginRenderingFilter()
{
	// Set the filter color surface as the render target
	RHISetRenderTarget(GlobalContext, GetFilterColorSurface(), FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingFilter()
{
	// Resolve the filter color surface 
	RHICopyToResolveTarget(GetFilterColorSurface(), FALSE);
}

/**
* Sets the scene color target and restores its contents if necessary
* @param bRestoreContents - if TRUE then copies contents of SceneColorTexture to the SceneColorSurface
*/
void FSceneRenderTargets::BeginRenderingSceneColor(UBOOL bRestoreContents)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColor"));

	if(bRestoreContents)
	{
		// Initialize the scene color surface to its previously resolved contents.
		RHICopyFromResolveTarget(GetSceneColorSurface());
	}

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget(GlobalContext, GetSceneColorSurface(), GetSceneDepthSurface());
}

/**
* Called when finished rendering to the scene color surface
* @param bKeepChanges - if TRUE then the SceneColorSurface is resolved to the SceneColorTexture
*/
void FSceneRenderTargets::FinishRenderingSceneColor(UBOOL bKeepChanges)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingSceneColor"));

	if(bKeepChanges)
	{
		// Resolve the scene color surface to the scene color texture.
		RHICopyToResolveTarget(GetSceneColorSurface(), TRUE);
	}
}

/**
* Sets the LDR version of the scene color target.
*/
void FSceneRenderTargets::BeginRenderingSceneColorLDR()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColorLDR"));

	// Set the light attenuation surface as the render target, and the scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GlobalContext,GetSceneColorLDRSurface(),GetSceneDepthSurface());
}

/**
* Called when finished rendering to the LDR version of the scene color surface.
* @param bKeepChanges - if TRUE then the SceneColorSurface is resolved to the LDR SceneColorTexture
* @param ResolveParams - optional resolve params
*/
void FSceneRenderTargets::FinishRenderingSceneColorLDR(UBOOL bKeepChanges,const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingSceneColorLDR"));

	if(bKeepChanges)
	{
		// Resolve the scene color surface to the scene color texture.
		RHICopyToResolveTarget(GetSceneColorLDRSurface(), TRUE, ResolveParams);
	}
}


/**
* Saves a previously rendered scene color target
*/
void FSceneRenderTargets::ResolveSceneColor()
{
    SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveSceneColor"));
    RHICopyToResolveTarget(GetSceneColorSurface(), TRUE);
}

/**
* Sets the raw version of the scene color target.
*/
void FSceneRenderTargets::BeginRenderingSceneColorRaw()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColorRaw"));

	// Use the raw version of the scene color as the render target, and use the standard scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GlobalContext, GetSceneColorRawSurface(), GetSceneDepthSurface());
}

/**
 * Saves a previously rendered scene color surface in the raw bit format.
 */
void FSceneRenderTargets::SaveSceneColorRaw()
{
	RHICopyToResolveTarget(GetSceneColorRawSurface(), TRUE);
}

/**
 * Restores a previously saved raw-scene color surface.
 */
void FSceneRenderTargets::RestoreSceneColorRaw()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("RestoreSceneColorRaw"));

	// Initialize the scene color surface to its previously resolved contents.
	RHICopyFromResolveTargetFast(GetSceneColorRawSurface());

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget(GlobalContext, GetSceneColorSurface(), GetSceneDepthSurface());
}

void FSceneRenderTargets::BeginRenderingPrePass()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingPrePass"));

	// Set the scene depth surface and a DUMMY buffer as color buffer
	// (as long as it's the same dimension as the depth buffer),
	RHISetRenderTarget(GlobalContext, GetLightAttenuationSurface(), GetSceneDepthSurface());

	// Disable color writes since we only want z depths
	RHISetColorWriteEnable(GlobalContext, FALSE);
}

void FSceneRenderTargets::FinishRenderingPrePass()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingPrePass"));

	// Re-enable color writes
	RHISetColorWriteEnable(GlobalContext, TRUE);
}

void FSceneRenderTargets::BeginRenderingShadowVolumes()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingShadowVolumes"));

	// Make sure we are writing to the same depth stencil buffer as
	// BeginRenderingSceneColor and BeginRenderingLightAttenuation.
	//
	// Note that we're not actually writing anything to the color
	// buffer here. It could be anything with the same dimension.
	RHISetRenderTarget(GlobalContext,GetLightAttenuationSurface(),GetSceneDepthSurface());
	RHISetColorWriteEnable(GlobalContext,FALSE);
}

void FSceneRenderTargets::FinishRenderingShadowVolumes()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingShadowVolumes"));
	RHISetColorWriteEnable(GlobalContext,TRUE);
}

void FSceneRenderTargets::BeginRenderingShadowDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingShadowDepth"));

	if(GSupportsHardwarePCF || GSupportsFetch4)
	{
		// set the shadow z surface as the depth buffer
		// have to bind a color target that is the same size as the depth texture on platforms that support Hardware PCF and Fetch4
		RHISetRenderTarget(GlobalContext,GetShadowDepthColorSurface(), GetShadowDepthZSurface());   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(GlobalContext,FALSE);
	}
	else if( GSupportsDepthTextures)
	{
		// set the shadow z surface as the depth buffer
		RHISetRenderTarget(GlobalContext,FSurfaceRHIRef(), GetShadowDepthZSurface());   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(GlobalContext,FALSE);
	}
	else
	{
		// Set the shadow color surface as the render target, and the shadow z surface as the depth buffer
		RHISetRenderTarget(GlobalContext,GetShadowDepthColorSurface(), GetShadowDepthZSurface());
	}
}

/**
* Called when finished rendering to the subject shadow depths so the surface can be copied to texture
* @param ResolveParams - optional resolve params
*/
void FSceneRenderTargets::FinishRenderingShadowDepth(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingShadowDepth"));

	if( GSupportsDepthTextures || GSupportsHardwarePCF || GSupportsFetch4)
	{
		// Resolve the shadow depth z surface.
		RHICopyToResolveTarget(GetShadowDepthZSurface(), FALSE, ResolveParams);
		// restore color writes
		RHISetColorWriteEnable(GlobalContext,TRUE);
	}
	else
	{
		// Resolve the shadow depth color surface.
		RHICopyToResolveTarget(GetShadowDepthColorSurface(), FALSE, ResolveParams);
	}
}

#if SUPPORTS_VSM
void FSceneRenderTargets::BeginRenderingShadowVariance()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingShadowVariance"));
	// Set the shadow variance surface
	RHISetRenderTarget(GlobalContext,GetShadowVarianceSurface(), FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingShadowVariance(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingShadowVariance"));
	// Resolve the shadow variance surface.
	RHICopyToResolveTarget(GetShadowVarianceSurface(), FALSE,ResolveParams);
}

UINT FSceneRenderTargets::GetShadowVarianceTextureResolution() const
{
	return GShadowDepthBufferSize;
}
#endif //#if SUPPORTS_VSM

void FSceneRenderTargets::BeginRenderingLightAttenuation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingLightAttenuation"));

	// Set the light attenuation surface as the render target, and the scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GlobalContext,GetLightAttenuationSurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingLightAttenuation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingLightAttenuation"));

	// Resolve the light attenuation surface.
	RHICopyToResolveTarget(GetLightAttenuationSurface(), FALSE);
}

void FSceneRenderTargets::BeginRenderingDistortionAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingDistortionAccumulation"));

	// use RGBA8 light target for accumulating distortion offsets	
	// R = positive X offset
	// G = positive Y offset
	// B = negative X offset
	// A = negative Y offset

	RHISetRenderTarget(GlobalContext,GetLightAttenuationSurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingDistortionAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingDistortionAccumulation"));

	RHICopyToResolveTarget(GetLightAttenuationSurface(), FALSE);
}

void FSceneRenderTargets::BeginRenderingDistortionDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingDistortionDepth"));
}

void FSceneRenderTargets::FinishRenderingDistortionDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingDistortionDepth"));
}

/** Starts rendering to the velocity buffer. */
void FSceneRenderTargets::BeginRenderingVelocities()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingVelocities"));

	// Set the motion blur velocity buffer as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget(GlobalContext, GetVelocitySurface(), GetSceneDepthSurface());
}

/** Stops rendering to the velocity buffer. */
void FSceneRenderTargets::FinishRenderingVelocities()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingVelocities"));

	// Resolve the velocity buffer to a texture, so it can be read later.
	RHICopyToResolveTarget(GetVelocitySurface(), FALSE);
}

void FSceneRenderTargets::BeginRenderingFogFrontfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingFogFrontfacesIntegralAccumulation"));
	RHISetRenderTarget(GlobalContext,GetFogFrontfacesIntegralAccumulationSurface(),FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingFogFrontfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingFogFrontfacesIntegralAccumulation"));

	RHICopyToResolveTarget(GetFogFrontfacesIntegralAccumulationSurface(), FALSE);
}

void FSceneRenderTargets::BeginRenderingFogBackfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingFogBackfacesIntegralAccumulation"));
	RHISetRenderTarget(GlobalContext,GetFogBackfacesIntegralAccumulationSurface(),FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingFogBackfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingFogBackfacesIntegralAccumulation"));

	RHICopyToResolveTarget(GetFogBackfacesIntegralAccumulationSurface(), FALSE);
}

void FSceneRenderTargets::ResolveSceneDepthTexture()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveSceneDepthTexture"));

	if(GSupportsDepthTextures)
	{
		// Resolve the scene depth surface.
		RHICopyToResolveTarget(GetSceneDepthSurface(), TRUE);
	}
}

UINT Aling(UINT Size, UINT Alignment)
{
	return (Size + Alignment - 1) & ~(Alignment - 1);
}

void FSceneRenderTargets::InitDynamicRHI()
{
	if(BufferSizeX > 0 && BufferSizeY > 0)
	{
		// Create the filter targetable texture and surface.
		RenderTargets[FilterColor].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,1,TexCreate_ResolveTargetable);
		RenderTargets[FilterColor].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,RenderTargets[FilterColor].Texture,TargetSurfCreate_Dedicated,TEXT("FilterColor"));


		EPixelFormat SceneColorBufferFormat = PF_FloatRGB;
		if(GSupportsDepthTextures)
		{
			// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
			RenderTargets[SceneDepthZ].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_DepthStencil,1,TexCreate_ResolveTargetable);
			RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,PF_DepthStencil,RenderTargets[SceneDepthZ].Texture,0,TEXT("DefaultDepth"));
		}
		else
		{
			// Create a surface to store the unresolved scene depth.
			RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,PF_DepthStencil,FTexture2DRHIRef(),TargetSurfCreate_Dedicated,TEXT("DefaultDepth"));
			// Allocate an alpha channel in the scene color texture to store the resolved scene depth.
			SceneColorBufferFormat = PF_FloatRGBA; 
		}

#if !XBOX
		// Create a texture to store the resolved scene colors, and a dedicated render-targetable surface to hold the unresolved scene colors.
		RenderTargets[SceneColor].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,SceneColorBufferFormat,1,TexCreate_ResolveTargetable);
		RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,SceneColorBufferFormat,RenderTargets[SceneColor].Texture,TargetSurfCreate_Dedicated,TEXT("DefaultColor"));
		RenderTargets[SceneColorRaw].Texture = RenderTargets[SceneColor].Texture;
		RenderTargets[SceneColorRaw].Surface = RenderTargets[SceneColor].Surface;
#else
		// Create a texture to store the resolved scene colors, and a dedicated render-targetable surface to hold the unresolved scene colors.
		const SIZE_T ExpandedSceneColorSize = RHICalculateTextureBytes(BufferSizeX,BufferSizeY,1,SceneColorBufferFormat);
		const SIZE_T RawSceneColorSize = RHICalculateTextureBytes(BufferSizeX,BufferSizeY,1,PF_A2B10G10R10);
		const SIZE_T SharedSceneColorSize = Max(ExpandedSceneColorSize,RawSceneColorSize);
		FSharedMemoryResourceRHIRef MemoryBuffer = RHICreateSharedMemory(SharedSceneColorSize);

		RenderTargets[SceneColor].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,SceneColorBufferFormat,1,MemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,SceneColorBufferFormat,RenderTargets[SceneColor].Texture,TargetSurfCreate_Dedicated,TEXT("DefaultColor"));

		// Create a version of the scene color textures that represent the raw bits (i.e. that can do the resolves without any format conversion)
		RenderTargets[SceneColorRaw].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,PF_A2B10G10R10,1,MemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[SceneColorRaw].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,PF_A2B10G10R10,RenderTargets[SceneColorRaw].Texture,TargetSurfCreate_Dedicated,TEXT("DefaultColorRaw"));
#endif

		//create the shadow depth color surface
		//platforms with GSupportsDepthTextures don't need a depth color target
		//platforms with GSupportsHardwarePCF still need a color target, due to API restrictions
		if (!GSupportsDepthTextures)
		{
			RenderTargets[ShadowDepthColor].Texture = RHICreateTexture2D(GetShadowDepthTextureResolution(),GetShadowDepthTextureResolution(),PF_R32F,1,TexCreate_ResolveTargetable);
			RenderTargets[ShadowDepthColor].Surface = RHICreateTargetableSurface(
				GetShadowDepthTextureResolution(),GetShadowDepthTextureResolution(),PF_R32F,RenderTargets[ShadowDepthColor].Texture,0,TEXT("ShadowDepthRT"));

		}

		//create the shadow depth texture and/or surface
		if (GSupportsHardwarePCF)
		{
			// Create a depth texture, used to sample PCF values
			RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
				GetShadowDepthTextureResolution(),GetShadowDepthTextureResolution(),PF_FilteredShadowDepth,1,TexCreate_DepthStencil);

			// Don't create a dedicated surface
			RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
				GetShadowDepthTextureResolution(),
				GetShadowDepthTextureResolution(),
				PF_FilteredShadowDepth,
				RenderTargets[ShadowDepthZ].Texture,
				0,
				TEXT("ShadowDepthZ")
				);
		}
		else if (GSupportsFetch4)
		{
			// Create a D24 depth stencil texture for use with Fetch4 shadows
			RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
				GetShadowDepthTextureResolution(),GetShadowDepthTextureResolution(),PF_D24,1,TexCreate_DepthStencil);

			// Don't create a dedicated surface
			RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
				GetShadowDepthTextureResolution(),
				GetShadowDepthTextureResolution(),
				PF_D24,
				RenderTargets[ShadowDepthZ].Texture,
				0,
				TEXT("ShadowDepthZ")
				);
		}
		else
		{
			if( GSupportsDepthTextures )
			{
				// Create a texture to store the resolved shadow depth
				RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
					GetShadowDepthTextureResolution(),GetShadowDepthTextureResolution(),PF_ShadowDepth,1,TexCreate_ResolveTargetable);
			}

			// Create a dedicated depth-stencil target surface for shadow depth rendering.
			RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
				GetShadowDepthTextureResolution(),
				GetShadowDepthTextureResolution(),
				PF_ShadowDepth,
				RenderTargets[ShadowDepthZ].Texture,
				TargetSurfCreate_Dedicated,
				TEXT("ShadowDepthZ")
				);
		}

#if SUPPORTS_VSM
		// We need a 2-channel format to support VSM, and a dedicated surface to work with the filtering
		EPixelFormat ShadowVarianceFmt=PF_G16R16F;
		RenderTargets[ShadowVariance].Texture = RHICreateTexture2D(GetShadowVarianceTextureResolution(),GetShadowVarianceTextureResolution(), ShadowVarianceFmt,1,TexCreate_ResolveTargetable);
		RenderTargets[ShadowVariance].Surface = RHICreateTargetableSurface(
			GetShadowVarianceTextureResolution(),
			GetShadowVarianceTextureResolution(), 
			ShadowVarianceFmt,RenderTargets[ShadowVariance].Texture,
			TargetSurfCreate_Dedicated,TEXT("ShadowVariance")
			);
#endif //#if SUPPORTS_VSM

		// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
		RenderTargets[LightAttenuation].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_A8R8G8B8,1,TexCreate_ResolveTargetable);
		RenderTargets[LightAttenuation].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,PF_A8R8G8B8,RenderTargets[LightAttenuation].Texture,0,TEXT("LightAttenuation"));

		// Create a texture to store the resolved velocity 2d-vectors, and a render-targetable surface to hold them.
		RenderTargets[VelocityBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_G16R16,1,TexCreate_ResolveTargetable);
		RenderTargets[VelocityBuffer].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,PF_G16R16,RenderTargets[VelocityBuffer].Texture,0,TEXT("VelocityBuffer"));

#if !XBOX
		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			//FogFrontfacesIntegralAccumulation texture and surface not used
			//allocate the highest precision render target with blending and filtering
			//@todo dw - ATI x1x00 cards don't filter this format!
			RenderTargets[FogBackfacesIntegralAccumulation].Texture = RHICreateTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,SceneColorBufferFormat,1,TexCreate_ResolveTargetable);
			RenderTargets[FogBackfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
				FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,SceneColorBufferFormat,RenderTargets[FogBackfacesIntegralAccumulation].Texture,0,TEXT("FogBackfacesIntegralAccumulationRT"));
		}
		else
#endif
		{
			//have to use a low precision format that supports blending and filtering since the only fp format Xenon can blend to (7e3) doesn't have enough precision
			RenderTargets[FogFrontfacesIntegralAccumulation].Texture = RHICreateTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,1,TexCreate_ResolveTargetable);
			RenderTargets[FogFrontfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
				FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,RenderTargets[FogFrontfacesIntegralAccumulation].Texture,0,TEXT("FogFrontfacesIntegralAccumulationRT"));

			RenderTargets[FogBackfacesIntegralAccumulation].Texture = RHICreateTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,1,TexCreate_ResolveTargetable);
			RenderTargets[FogBackfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
				FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,RenderTargets[FogBackfacesIntegralAccumulation].Texture,0,TEXT("FogBackfacesIntegralAccumulationRT"));
		}
	}

	GlobalContext = RHIGetGlobalContext();
}

void FSceneRenderTargets::ReleaseDynamicRHI()
{
	// make sure no scene render targets and textures are in use before releasing them
	RHISetRenderTarget(GlobalContext,FSurfaceRHIRef(),FSurfaceRHIRef());

	for( INT RTIdx=0; RTIdx < MAX_SCENE_RENDERTARGETS; RTIdx++ )
	{
		RenderTargets[RTIdx].Texture.Release();
		RenderTargets[RTIdx].Surface.Release();
	}
}

UINT FSceneRenderTargets::GetShadowDepthTextureResolution() const
{
	return GShadowDepthBufferSize;
}

/*-----------------------------------------------------------------------------
FSceneTextureShaderParameters
-----------------------------------------------------------------------------*/

//
void FSceneTextureShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	// only used if Material has an expression that requires SceneColorTexture
	SceneColorTextureParameter.Bind(ParameterMap,TEXT("SceneColorTexture"),TRUE);
	// only used if Material has an expression that requires SceneDepthTexture
	SceneDepthTextureParameter.Bind(ParameterMap,TEXT("SceneDepthTexture"),TRUE);
	// only used if Material has an expression that requires SceneDepthTexture
	SceneDepthCalcParameter.Bind(ParameterMap,TEXT("MinZ_MaxZRatio"),TRUE);
	// only used if Material has an expression that requires ScreenPosition biasing
	ScreenPositionScaleBiasParameter.Bind(ParameterMap,TEXT("ScreenPositionScaleBias"),TRUE);
}

//
void FSceneTextureShaderParameters::Set(FCommandContextRHI* Context, const FSceneView* View,FShader* PixelShader, ESamplerFilter ColorFilter/*=SF_Nearest*/) const
{
	if (SceneColorTextureParameter.IsBound() == TRUE)
	{
		FSamplerStateRHIRef Filter;
		switch ( ColorFilter )
		{
			case SF_Linear:
				Filter = TStaticSamplerState<SF_Linear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_AnisotropicLinear:
				Filter = TStaticSamplerState<SF_AnisotropicLinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_Nearest:
			default:
				Filter = TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
		}

		SetTextureParameter(
			Context,
			PixelShader->GetPixelShader(),
			SceneColorTextureParameter,
			Filter,
			View->bUseLDRSceneColor ? GSceneRenderTargets.GetSceneColorLDRTexture() : GSceneRenderTargets.GetSceneColorTexture()
			);
	}
	if (SceneDepthTextureParameter.IsBound() == TRUE &&
		IsValidRef(GSceneRenderTargets.GetSceneDepthTexture()) == TRUE)
	{
		SetTextureParameter(
			Context,
			PixelShader->GetPixelShader(),
			SceneDepthTextureParameter,
			TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetSceneDepthTexture()
			);
	}
	RHISetViewPixelParameters( Context, View, PixelShader->GetPixelShader(), &SceneDepthCalcParameter, &ScreenPositionScaleBiasParameter );
}

//
FArchive& operator<<(FArchive& Ar,FSceneTextureShaderParameters& Parameters)
{
	Ar << Parameters.SceneColorTextureParameter;
	Ar << Parameters.SceneDepthTextureParameter;
	Ar << Parameters.SceneDepthCalcParameter;
	Ar << Parameters.ScreenPositionScaleBiasParameter;
	return Ar;
}

/*-----------------------------------------------------------------------------
FSceneRenderTargetProxy
-----------------------------------------------------------------------------*/

/**
* Constructor
*/
FSceneRenderTargetProxy::FSceneRenderTargetProxy()
:	SizeX(0)
,	SizeY(0)
{	
}

/**
* Set SizeX and SizeY of proxy and re-allocate scene targets as needed
*
* @param InSizeX - scene render target width requested
* @param InSizeY - scene render target height requested
*/
void FSceneRenderTargetProxy::SetSizes(UINT InSizeX,UINT InSizeY)
{
	SizeX = InSizeX;
	SizeY = InSizeY;

	if( IsInRenderingThread() )
	{
		GSceneRenderTargets.Allocate(SizeX,SizeY);
	}
	else
	{
		struct FRenderTargetSizeParams
		{
			UINT SizeX;
			UINT SizeY;
		};
		FRenderTargetSizeParams RenderTargetSizeParams = {SizeX,SizeY};
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			RenderTargetAllocProxyCommand,
			FRenderTargetSizeParams,Parameters,RenderTargetSizeParams,
		{
			GSceneRenderTargets.Allocate(Parameters.SizeX, Parameters.SizeY);
		});
	}
}

/**
* @return RHI surface for setting the render target
*/
const FSurfaceRHIRef& FSceneRenderTargetProxy::GetRenderTargetSurface() const
{
	return GSceneRenderTargets.GetSceneColorSurface();
}

/**
* @return width of the scene render target this proxy will render to
*/
UINT FSceneRenderTargetProxy::GetSizeX() const
{
	if( SizeX != 0 )
	{
		return Min<UINT>(SizeX,GSceneRenderTargets.GetBufferSizeX());
	}
	else
	{
		return GSceneRenderTargets.GetBufferSizeX();
	}	
}

/**
* @return height of the scene render target this proxy will render to
*/
UINT FSceneRenderTargetProxy::GetSizeY() const
{
	if( SizeY != 0 )
	{
		return Min<UINT>(SizeY,GSceneRenderTargets.GetBufferSizeY());
	}
	else
	{
		return GSceneRenderTargets.GetBufferSizeY();
	}
}

/**
* @return gamma this render target should be rendered with
*/
FLOAT FSceneRenderTargetProxy::GetDisplayGamma() const
{
	return 1.0f;
}



