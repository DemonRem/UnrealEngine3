/*=============================================================================
	UBloomEffect.cpp: Bloom post process effect implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

IMPLEMENT_CLASS(UBloomEffect);

/** Encapsulates the DOF and bloom blend pixel shader. */
class FBloomBlendPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FBloomBlendPixelShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{

	}

	/** Default constructor. */
	FBloomBlendPixelShader() {}

public:

	FShaderParameter BlurredImageParameter;
	FShaderParameter BloomAlphaParameter;
	FSceneTextureShaderParameters SceneTextureParameters;

	/** Initialization constructor. */
	FBloomBlendPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		BlurredImageParameter.Bind(Initializer.ParameterMap,TEXT("BlurredImage"),TRUE);
		BloomAlphaParameter.Bind(Initializer.ParameterMap,TEXT("BloomAlpha"),TRUE);
	}

	// FShader interface.
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << BlurredImageParameter << BloomAlphaParameter;
		Ar << SceneTextureParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FBloomBlendPixelShader,TEXT("BloomBlendPixelShader"),TEXT("Main"),SF_Pixel,VER_FP_BLENDING_FALLBACK,0);

/** Encapsulates the DOF and bloom blend pixel shader. */
class FBloomBlendVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FBloomBlendVertexShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	/** Default constructor. */
	FBloomBlendVertexShader() {}

public:

	/** Initialization constructor. */
	FBloomBlendVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
	}
};

IMPLEMENT_SHADER_TYPE(,FBloomBlendVertexShader,TEXT("BloomBlendVertexShader"),TEXT("Main"),SF_Vertex,0,0);

extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

/** Scene proxy for bloom post process effect */
class FBloomPostProcessSceneProxy : public FPostProcessSceneProxy
{
public:
	/** 
	* Initialization constructor. 
	* @param InEffect - bloom post process effect to mirror in this proxy
	*/
	FBloomPostProcessSceneProxy(const UBloomEffect* InEffect,const FPostProcessSettings* WorldSettings)
		:	FPostProcessSceneProxy(InEffect)
	{

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
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Bloom"));

		UBOOL bDirty=FALSE;

		const FLOAT BlurAttenuation = 0.15f;
		const FLOAT BlurAlpha = 0.15f;

		const UINT BufferSizeX = GSceneRenderTargets.GetBufferSizeX();
		const UINT BufferSizeY = GSceneRenderTargets.GetBufferSizeY();
		const UINT FilterBufferSizeX = GSceneRenderTargets.GetFilterBufferSizeX();
		const UINT FilterBufferSizeY = GSceneRenderTargets.GetFilterBufferSizeY();
		const UINT DownsampledSizeX = View.RenderTargetSizeX / GSceneRenderTargets.GetFilterDownsampleFactor();
		const UINT DownsampledSizeY = View.RenderTargetSizeY / GSceneRenderTargets.GetFilterDownsampleFactor();

		const FLOAT	BlurScaleX = 128.0f / (FLOAT)DownsampledSizeX / View.ProjectionMatrix.M[0][0];
		const FLOAT BlurScaleY = 128.0f / (FLOAT)DownsampledSizeY / View.ProjectionMatrix.M[1][1];

		// Downsample the scene to the filter buffer.
		DrawDownsampledTexture(Context, View);

		const UINT NumIterations = 1;

		for(UINT Iteration = 0;Iteration < NumIterations;Iteration++)
		{
			FVector2D BlurSampleOffsets[MAX_FILTER_SAMPLES];
			FLinearColor BlurSampleWeights[MAX_FILTER_SAMPLES];
			UINT NumSamples = 0;
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex += 2)
			{
				BlurSampleOffsets[NumSamples] = FVector2D(
					-SampleIndex * appPow(MAX_FILTER_SAMPLES,Iteration) - 1.0f + 1.0f / (1.0f + appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX)),
					0
					);
				BlurSampleWeights[NumSamples] = FLinearColor::White * (
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX * SampleIndex) +
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY * (SampleIndex + 1))
					);
				NumSamples++;
			}
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex += 2)
			{
				BlurSampleOffsets[NumSamples] = FVector2D(
					SampleIndex * appPow(MAX_FILTER_SAMPLES,Iteration) + 1.0f - 1.0f / (1.0f + appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX)),
					0
					);
				BlurSampleWeights[NumSamples] = FLinearColor::White * (
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX * SampleIndex) +
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY * (SampleIndex + 1))
					);
				NumSamples++;
			}
			FLinearColor TotalWeight(FLinearColor::Black);
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex++)
			{
				BlurSampleOffsets[SampleIndex].X /= (FLOAT)FilterBufferSizeX;
				BlurSampleOffsets[SampleIndex].Y /= (FLOAT)FilterBufferSizeY;
				TotalWeight += BlurSampleWeights[SampleIndex];
			}
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex++)
			{
				BlurSampleWeights[SampleIndex].R /= TotalWeight.R;
				BlurSampleWeights[SampleIndex].G /= TotalWeight.G;
				BlurSampleWeights[SampleIndex].B /= TotalWeight.B;
				BlurSampleWeights[SampleIndex].A = 0.0f;
			}

			GSceneRenderTargets.BeginRenderingFilter();
			RHIClear(Context,TRUE,FLinearColor::Black,FALSE,0,FALSE,0);
			SetFilterShaders(
				Context, 
				TStaticSamplerState<SF_Linear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				GSceneRenderTargets.GetFilterColorTexture(),
				BlurSampleOffsets,
				BlurSampleWeights,
				MAX_FILTER_SAMPLES
				);
			DrawDenormalizedQuad(Context,1,1,DownsampledSizeX,DownsampledSizeY,1,1,DownsampledSizeX,DownsampledSizeY,FilterBufferSizeX,FilterBufferSizeY,FilterBufferSizeX,FilterBufferSizeY);
			GSceneRenderTargets.FinishRenderingFilter();
		}

		for(UINT Iteration = 0;Iteration < NumIterations;Iteration++)
		{
			FVector2D BlurSampleOffsets[MAX_FILTER_SAMPLES];
			FLinearColor BlurSampleWeights[MAX_FILTER_SAMPLES];
			UINT NumSamples = 0;
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex += 2)
			{
				BlurSampleOffsets[NumSamples] = FVector2D(
					0,
					-SampleIndex * appPow(MAX_FILTER_SAMPLES,Iteration) - 1.0f + 1.0f / (1.0f + appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY))
					);
				BlurSampleWeights[NumSamples] = FLinearColor::White * (
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX * SampleIndex) +
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY * (SampleIndex + 1))
					);
				NumSamples++;
			}
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex += 2)
			{
				BlurSampleOffsets[NumSamples] = FVector2D(
					0,
					SampleIndex * appPow(MAX_FILTER_SAMPLES,Iteration) + 1.0f - 1.0f / (1.0f + appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY))
					);
				BlurSampleWeights[NumSamples] = FLinearColor::White * (
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleX * SampleIndex) +
					appPow(BlurAttenuation,appPow(MAX_FILTER_SAMPLES,Iteration) * BlurScaleY * (SampleIndex + 1))
					);
				NumSamples++;
			}
			FLinearColor TotalWeight(FLinearColor::Black);
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex++)
			{
				BlurSampleOffsets[SampleIndex].X /= (FLOAT)FilterBufferSizeX;
				BlurSampleOffsets[SampleIndex].Y /= (FLOAT)FilterBufferSizeY;
				TotalWeight += BlurSampleWeights[SampleIndex];
			}
			for(INT SampleIndex = 0;SampleIndex < MAX_FILTER_SAMPLES;SampleIndex++)
			{
				BlurSampleWeights[SampleIndex].R /= TotalWeight.R;
				BlurSampleWeights[SampleIndex].G /= TotalWeight.G;
				BlurSampleWeights[SampleIndex].B /= TotalWeight.B;
				BlurSampleWeights[SampleIndex].A = 0.0f;
			}

			GSceneRenderTargets.BeginRenderingFilter();
			RHIClear(Context,TRUE,FLinearColor::Black,FALSE,0,FALSE,0);
			SetFilterShaders(
				Context, 
				TStaticSamplerState<SF_Linear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				GSceneRenderTargets.GetFilterColorTexture(),
				BlurSampleOffsets,
				BlurSampleWeights,
				MAX_FILTER_SAMPLES
				);
			DrawDenormalizedQuad(Context,1,1,DownsampledSizeX,DownsampledSizeY,1,1,DownsampledSizeX,DownsampledSizeY,FilterBufferSizeX,FilterBufferSizeY,FilterBufferSizeX,FilterBufferSizeY);
			GSceneRenderTargets.FinishRenderingFilter();
		}

		// Render to the scene color buffer.
		GSceneRenderTargets.BeginRenderingSceneColor();

		TShaderMapRef<FBloomBlendVertexShader> BlendVertexShader(GetGlobalShaderMap());
		TShaderMapRef<FBloomBlendPixelShader> BlendPixelShader(GetGlobalShaderMap());
		
		SetGlobalBoundShaderState(Context, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *BlendVertexShader, *BlendPixelShader, sizeof(FFilterVertex));

		SetTextureParameter(
			Context,
			BlendPixelShader->GetPixelShader(),
			BlendPixelShader->BlurredImageParameter,
			TStaticSamplerState<SF_Linear>::GetRHI(),
			GSceneRenderTargets.GetFilterColorTexture()
			);

		SetPixelShaderValue(Context,BlendPixelShader->GetPixelShader(),BlendPixelShader->BloomAlphaParameter,BlurAlpha);

		BlendPixelShader->SceneTextureParameters.Set(Context,&View,*BlendPixelShader);

		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			// Use alpha blending.
			RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_One,BF_Zero>::GetRHI());
		}
		else
		{
			//opaque, blending will be done in the shader
			RHISetBlendState(Context,TStaticBlendState<>::GetRHI());	
		}

		// Draw a quad mapping the blurred pixels in the filter buffer to the scene color buffer.
		DrawDenormalizedQuad(
			Context,
			View.RenderTargetX,View.RenderTargetY,
			View.RenderTargetSizeX,View.RenderTargetSizeY,
			1,1,
			DownsampledSizeX,DownsampledSizeY,
			BufferSizeX,BufferSizeY,
			FilterBufferSizeX,FilterBufferSizeY
			);

		// Resolve the scene color buffer.
		GSceneRenderTargets.FinishRenderingSceneColor();

		return bDirty;
	}

private:
	/** cached bound shader state for final blend */
	static FGlobalBoundShaderStateRHIRef BoundShaderState;
};

FGlobalBoundShaderStateRHIRef FBloomPostProcessSceneProxy::BoundShaderState;

/*-----------------------------------------------------------------------------
	UBloomEffect
-----------------------------------------------------------------------------*/

/**
 * Creates a proxy to represent the render info for a post process effect
 * @param WorldSettings - The world's post process settings for the view.
 * @return The proxy object.
 */
FPostProcessSceneProxy* UBloomEffect::CreateSceneProxy(const FPostProcessSettings* WorldSettings)
{
	return new FBloomPostProcessSceneProxy(this,WorldSettings);
}

/**
 * @param View - current view
 * @return TRUE if the effect should be rendered
 */
UBOOL UBloomEffect::IsShown(const FSceneView* View) const
{
	return GSystemSettings.bAllowBloom && Super::IsShown( View );
}


