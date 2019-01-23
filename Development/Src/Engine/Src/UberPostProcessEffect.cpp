/*=============================================================================
UberPostProcessEffect.cpp: combines DOF, bloom and material coloring.

Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "DOFAndBloomEffect.h"

IMPLEMENT_CLASS(UUberPostProcessEffect);

/**
* Called when properties change.
*/
void UUberPostProcessEffect::PostEditChange(UProperty* PropertyThatChanged)
{
	// clamp saturation to 0..1
	SceneDesaturation = Clamp(SceneDesaturation, 0.f, 1.f);

	// UberPostProcessEffect should only ever exists in the SDPG_PostProcess scene
	SceneDPG=SDPG_PostProcess;

	Super::PostEditChange(PropertyThatChanged);
}	

/**
* Called after this instance has been serialized.
*/
void UUberPostProcessEffect::PostLoad()
{
	Super::PostLoad();

	// UberPostProcessEffect should only ever exists in the SDPG_PostProcess scene
	SceneDPG=SDPG_PostProcess;

	// clamp desaturation to 0..1 (fixup for old data)
	SceneDesaturation = Clamp(SceneDesaturation, 0.f, 1.f);
}



/*-----------------------------------------------------------------------------
FMaterialShaderParameters
-----------------------------------------------------------------------------*/

/** Encapsulates the Material parameters. */
class FMaterialShaderParameters
{
public:

	/** Default constructor. */
	FMaterialShaderParameters() {}

	/** Initialization constructor. */
	FMaterialShaderParameters(const FShaderParameterMap& ParameterMap)
	{
		SceneShadowsAndDesaturation.Bind(ParameterMap,TEXT("SceneShadowsAndDesaturation"),TRUE);
		SceneInverseHighLights.Bind(ParameterMap,TEXT("SceneInverseHighLights"),TRUE);
		SceneMidTones.Bind(ParameterMap,TEXT("SceneMidTones"),TRUE);
		SceneScaledLuminanceWeights.Bind(ParameterMap,TEXT("SceneScaledLuminanceWeights"),TRUE);
	}

	/** Set the material shader parameter values. */
	void Set(FCommandContextRHI* Context, FShader* PixelShader, FVector4 const& Shadows, FVector4 const& HighLights, FVector4 const& MidTones, FLOAT Desaturation)
	{
		// SceneInverseHighlights

		FVector4 InvHighLights;

		InvHighLights.X = 1.f / HighLights.X;
		InvHighLights.Y = 1.f / HighLights.Y;
		InvHighLights.Z = 1.f / HighLights.Z;
		InvHighLights.W = 0.f; // Unused

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			SceneInverseHighLights,
			InvHighLights
			);

		// SceneShadowsAndDesaturation

		FVector4 ShadowsAndDesaturation;

		ShadowsAndDesaturation.X = Shadows.X;
		ShadowsAndDesaturation.Y = Shadows.Y;
		ShadowsAndDesaturation.Z = Shadows.Z;
		ShadowsAndDesaturation.W = (1.f - Desaturation);

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			SceneShadowsAndDesaturation,
			ShadowsAndDesaturation
			);

		// MaterialPower

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			SceneMidTones,
			MidTones
			);

		// SceneScaledLuminanceWeights

		FVector4 ScaledLuminanceWeights;

		ScaledLuminanceWeights.X = 0.30000001f * Desaturation;
		ScaledLuminanceWeights.Y = 0.58999997f * Desaturation;
		ScaledLuminanceWeights.Z = 0.11000000f * Desaturation;
		ScaledLuminanceWeights.W = 0.f; // Unused

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			SceneScaledLuminanceWeights,
			ScaledLuminanceWeights
			);
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FMaterialShaderParameters& P)
	{
		Ar << P.SceneShadowsAndDesaturation;
		Ar << P.SceneInverseHighLights;
		Ar << P.SceneMidTones;
		Ar << P.SceneScaledLuminanceWeights;
		return Ar;
	}

private:
	FShaderParameter SceneShadowsAndDesaturation;
	FShaderParameter SceneInverseHighLights;
	FShaderParameter SceneMidTones;
	FShaderParameter SceneScaledLuminanceWeights;
};


/*-----------------------------------------------------------------------------
FMGammaShaderParameters
-----------------------------------------------------------------------------*/

/** Encapsulates the gamma correction parameters. */
class FGammaShaderParameters
{
public:

	/** Default constructor. */
	FGammaShaderParameters() {}

	/** Initialization constructor. */
	FGammaShaderParameters(const FShaderParameterMap& ParameterMap)
	{
		GammaColorScaleAndInverse.Bind(ParameterMap,TEXT("GammaColorScaleAndInverse"),TRUE);
		GammaOverlayColor.Bind(ParameterMap,TEXT("GammaOverlayColor"),TRUE);
	}

	/** Set the material shader parameter values. */
	void Set(FCommandContextRHI* Context, FShader* PixelShader, FLOAT DisplayGamma, FLinearColor const& ColorScale, FLinearColor const& ColorOverlay)
	{
		// GammaColorScaleAndInverse

		FLOAT InvDisplayGamma = 1.f / DisplayGamma;
		FLOAT OneMinusOverlayBlend = 1.f - ColorOverlay.A;

		FVector4 ColorScaleAndInverse;

		ColorScaleAndInverse.X = ColorScale.R * OneMinusOverlayBlend;
		ColorScaleAndInverse.Y = ColorScale.G * OneMinusOverlayBlend;
		ColorScaleAndInverse.Z = ColorScale.B * OneMinusOverlayBlend;
		ColorScaleAndInverse.W = InvDisplayGamma;

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			GammaColorScaleAndInverse,
			ColorScaleAndInverse
			);

		// GammaOverlayColor

		FVector4 OverlayColor;

		OverlayColor.X = ColorOverlay.R * ColorOverlay.A / ColorScaleAndInverse.X; // Note that the divide by the ColorScale is just done to work around some weirdness in the HLSL compiler
		OverlayColor.Y = ColorOverlay.G * ColorOverlay.A / ColorScaleAndInverse.Y; // (see UberPostProcessBlendPixelShader.usf)
		OverlayColor.Z = ColorOverlay.B * ColorOverlay.A / ColorScaleAndInverse.Z; 
		OverlayColor.W = 0.f; // Unused

		SetPixelShaderValue(
			Context,
			PixelShader->GetPixelShader(),
			GammaOverlayColor,
			OverlayColor
			);
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FGammaShaderParameters& P)
	{
		Ar << P.GammaColorScaleAndInverse;
		Ar << P.GammaOverlayColor;
		return Ar;
	}

private:
	FShaderParameter GammaColorScaleAndInverse;
	FShaderParameter GammaOverlayColor;
};

/*-----------------------------------------------------------------------------
FUberPostProcessBlendPixelShader
-----------------------------------------------------------------------------*/

/** Encapsulates the blend pixel shader. */
class FUberPostProcessBlendPixelShader : public FDOFAndBloomBlendPixelShader
{
	DECLARE_SHADER_TYPE(FUberPostProcessBlendPixelShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	/** Default constructor. */
	FUberPostProcessBlendPixelShader() {}

public:
	FMaterialShaderParameters     MaterialParameters;
	FGammaShaderParameters        GammaParameters;

	/** Initialization constructor. */
	FUberPostProcessBlendPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FDOFAndBloomBlendPixelShader(Initializer)
		,	MaterialParameters(Initializer.ParameterMap)
		,	GammaParameters(Initializer.ParameterMap)
	{
	}

	// FShader interface.
	virtual void Serialize(FArchive& Ar)
	{
		FDOFAndBloomBlendPixelShader::Serialize(Ar);
		Ar << MaterialParameters << GammaParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FUberPostProcessBlendPixelShader,TEXT("UberPostProcessBlendPixelShader"),TEXT("Main"),SF_Pixel,VER_UBERPOSTPROCESSBLEND_PS_RECOMPILE_2,0);


/*-----------------------------------------------------------------------------
FUberPostProcessSceneProxy
-----------------------------------------------------------------------------*/

extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

class FUberPostProcessSceneProxy : public FDOFAndBloomPostProcessSceneProxy
{
public:
	/** 
	* Initialization constructor. 
	* @param InEffect - Uber post process effect to mirror in this proxy
	*/
	FUberPostProcessSceneProxy(const UUberPostProcessEffect* InEffect,const FPostProcessSettings* WorldSettings)
		:	FDOFAndBloomPostProcessSceneProxy(InEffect, WorldSettings)
		,	SceneShadows(WorldSettings ? WorldSettings->Scene_Shadows : InEffect->SceneShadows)
		,	SceneHighLights(WorldSettings ? WorldSettings->Scene_HighLights : InEffect->SceneHighLights)
		,	SceneMidTones(WorldSettings ? WorldSettings->Scene_MidTones : InEffect->SceneMidTones)
		,	SceneDesaturation(WorldSettings ? WorldSettings->Scene_Desaturation : InEffect->SceneDesaturation)
	{
		if(WorldSettings && !WorldSettings->bEnableSceneEffect)
		{
			SceneShadows = FVector(0.f, 0.f, 0.f);
			SceneHighLights = FVector(1.f, 1.f, 1.f);
			SceneMidTones = FVector(1.f, 1.f, 1.f);
			SceneDesaturation = 0.f;
		}
	}

	/**
	* Render the post process effect
	* Called by the rendering thread during scene rendering
	* @param InDepthPriorityGroup - scene DPG currently being rendered
	* @param View - current view
	* @param CanvasTransform - same canvas transform used to render the scene
	* @return TRUE if anything was rendered
	*/
	UBOOL Render(FCommandContextRHI* Context, UINT InDepthPriorityGroup,FViewInfo& View,const FMatrix& CanvasTransform)
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("UberPostProcessing"));

		check(SDPG_PostProcess==InDepthPriorityGroup);
		check(FALSE==View.bUseLDRSceneColor);

		const UINT BufferSizeX = GSceneRenderTargets.GetBufferSizeX();
		const UINT BufferSizeY = GSceneRenderTargets.GetBufferSizeY();
		const UINT FilterBufferSizeX = GSceneRenderTargets.GetFilterBufferSizeX();
		const UINT FilterBufferSizeY = GSceneRenderTargets.GetFilterBufferSizeY();
		const UINT FilterDownsampleFactor = GSceneRenderTargets.GetFilterDownsampleFactor();
		const UINT DownsampledSizeX = View.RenderTargetSizeX / FilterDownsampleFactor;
		const UINT DownsampledSizeY = View.RenderTargetSizeY / FilterDownsampleFactor;

		RenderDOFAndBloomGatherPass(Context, View);
	
		// Blur the filter buffer.
		GaussianBlurFilterBuffer(Context, DownsampledSizeX,DownsampledSizeY,BlurKernelSize);

		// The combined (uber) post-processing shader does depth of field, bloom, material colorization and gamma correction all in one
		// pass.
		//
		// This means it takes an HDR (64-bit) input and produces a low dynamic range (32-bit) output.  If it is the final post processing
		// shader in the post processing chain then it can render directly into the view's render target.  Otherwise it has to render into
		// a 32-bit render target.
		//
		// Note: Any post-processing shader that follows the uber shader needs to be able to handle an LDR input.  The shader can can check
		//       for this by looking at the bUseLDRSceneColor flag in the FViewInfo structure.  Also the final shader following the uber
		//       shader needs to write into the view's render target (or needs to write the result out to a 64-bit render-target).

		FLOAT DisplayGamma = View.Family->RenderTarget->GetDisplayGamma();

		// if the scene wasn't meant to be resolved to LDR then continue rendering to HDR
		if( !View.Family->bResolveScene )			
		{
			// Using 64-bit (HDR) surface
			GSceneRenderTargets.BeginRenderingSceneColor();
			// disable gamma correction
			DisplayGamma = 1.0f;
		}
		else
		{
			if (FinalEffectInGroup)
			{
				RHISetRenderTarget(Context,View.Family->RenderTarget->GetRenderTargetSurface(), FSurfaceRHIRef()); // Render to the final render target
			}
			else
			{
				GSceneRenderTargets.BeginRenderingSceneColorLDR();
			}
		}
		

		// Set the blend vertex shader.
		TShaderMapRef<FDOFAndBloomBlendVertexShader> BlendVertexShader(GetGlobalShaderMap());

		// Set the blend pixel shader.
		TShaderMapRef<FUberPostProcessBlendPixelShader> BlendPixelShader(GetGlobalShaderMap());

		// Set the DOF and bloom parameters
		BlendPixelShader->DOFParameters.Set(
			Context,
			*BlendPixelShader,
			FocusDistance,
			FocusInnerRadius,
			FalloffExponent,
			MaxNearBlurAmount,
			MaxFarBlurAmount
			);

		// Set the material colorization parameters
		BlendPixelShader->MaterialParameters.Set(
			Context,
			*BlendPixelShader,
			SceneShadows,
			SceneHighLights,
			SceneMidTones,
			// ensure that desat values out of range get clamped (this can happen in the editor currently)
			Clamp(SceneDesaturation, 0.f, 1.f)
			);

		// Set the gamma correction parameters
		BlendPixelShader->GammaParameters.Set(
			Context,
			*BlendPixelShader,
			DisplayGamma,
			View.ColorScale,
			View.OverlayColor
			);
			
		// Setup the scene texture
		BlendPixelShader->SceneTextureParameters.Set(Context,&View,*BlendPixelShader);

		// Setup the pre-filtered DOF/Bloom texture
		SetTextureParameter(
			Context,
			BlendPixelShader->GetPixelShader(),
			BlendPixelShader->BlurredImageParameter,
			TStaticSamplerState<SF_Linear>::GetRHI(),
			GSceneRenderTargets.GetFilterColorTexture()
			);

		SetGlobalBoundShaderState(Context, UberBlendBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *BlendVertexShader, *BlendPixelShader, sizeof(FFilterVertex));

		if( FinalEffectInGroup &&
			View.Family->bResolveScene )
		{
			// We need to adjust the UV coordinate calculation for the scene color texture when rendering directly to
			// the view's render target (to support the editor).

			UINT TargetSizeX = View.Family->RenderTarget->GetSizeX();
			UINT TargetSizeY = View.Family->RenderTarget->GetSizeY();

			FLOAT ScaleX = TargetSizeX / static_cast<float>(BufferSizeX);
			FLOAT ScaleY = TargetSizeY / static_cast<float>(BufferSizeY);

			FLOAT UVScaleX  = +0.5f * ScaleX;
			FLOAT UVScaleY  = -0.5f * ScaleY;

			// we are not going to sample RenderTarget. the pixel size should be taken from the sampled buffer
			FLOAT UVOffsetX = +0.5f * ScaleX + GPixelCenterOffset / static_cast<float>(BufferSizeX);
			FLOAT UVOffsetY = +0.5f * ScaleY + GPixelCenterOffset / static_cast<float>(BufferSizeY);

			// For split-screen and 4:3 views we also need to take into account the viewport correctly; however, the
			// DOFAndBloomBlendVertex shader computes the UV coordinates for the SceneColor texture directly from the
			// screen coordinates that are used to render and since the view-port may not be located at 0,0 we need
			// to adjust for that by modifying the UV offset and scale.

			// we are not going to sample RenderTarget. the pixel size should be taken from the sampled buffer
			UVOffsetX -= (View.X - View.RenderTargetX ) / BufferSizeX;
			UVOffsetY -= (View.Y - View.RenderTargetY ) / BufferSizeY;

			SetVertexShaderValue(
				Context,
				BlendVertexShader->GetVertexShader(),
				BlendVertexShader->SceneCoordinateScaleBiasParameter,
				FVector4(
					UVScaleX,
					UVScaleY,
					UVOffsetY,
					UVOffsetX
					)
				);

			// Draw a quad mapping the blurred pixels in the filter buffer to the scene color buffer.
			DrawDenormalizedQuad( Context,
				View.X, View.Y, View.SizeX, View.SizeY,
				1,1, DownsampledSizeX,DownsampledSizeY,
				TargetSizeX, TargetSizeY,
				FilterBufferSizeX, FilterBufferSizeY);
		}
		else
		{
			SetVertexShaderValue(
				Context,
				BlendVertexShader->GetVertexShader(),
				BlendVertexShader->SceneCoordinateScaleBiasParameter,
				FVector4(
					+0.5f,
					-0.5f,
					+0.5f + GPixelCenterOffset / BufferSizeY,
					+0.5f + GPixelCenterOffset / BufferSizeX
					)
				);

			// Draw a quad mapping the blurred pixels in the filter buffer to the scene color buffer.
			DrawDenormalizedQuad( Context,
				View.RenderTargetX,View.RenderTargetY, View.RenderTargetSizeX,View.RenderTargetSizeY,
				1,1, DownsampledSizeX,DownsampledSizeY,
				BufferSizeX, BufferSizeY,
				FilterBufferSizeX, FilterBufferSizeY);

			FResolveParams ResolveParams;
			ResolveParams.X1 = View.RenderTargetX;
			ResolveParams.Y1 = View.RenderTargetY;
			ResolveParams.X2 = View.RenderTargetX + View.RenderTargetSizeX;
			ResolveParams.Y2 = View.RenderTargetY + View.RenderTargetSizeY;

			if( View.Family->bResolveScene )
			{
				// Resolve the scene color LDR buffer.
				GSceneRenderTargets.FinishRenderingSceneColorLDR(TRUE,ResolveParams);
			}
			else
			{
				// Resolve the scene color HDR buffer.
				GSceneRenderTargets.FinishRenderingSceneColor(TRUE);
			}
		}

		View.bUseLDRSceneColor = TRUE; // Indicate that from now on the scene color is in an LDR surface.

		return TRUE;
	}

protected:
	/** mirrored material properties (see UberPostProcessEffect.uc) */
	FVector			SceneShadows;
	FVector			SceneHighLights;
	FVector			SceneMidTones;
	FLOAT			SceneDesaturation;
	
	/** bound shader state for the blend pass */
	static FGlobalBoundShaderStateRHIRef UberBlendBoundShaderState;
};

FGlobalBoundShaderStateRHIRef FUberPostProcessSceneProxy::UberBlendBoundShaderState;

/**
 * Creates a proxy to represent the render info for a post process effect
 * @param WorldSettings - The world's post process settings for the view.
 * @return The proxy object.
 */
FPostProcessSceneProxy* UUberPostProcessEffect::CreateSceneProxy(const FPostProcessSettings* WorldSettings)
{
    return new FUberPostProcessSceneProxy(this,WorldSettings);
}

