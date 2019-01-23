/*=============================================================================
	ScenePostProcessing.cpp: Scene post processing implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

/** Encapsulates the gamma correction pixel shader. */
class FGammaCorrectionPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FGammaCorrectionPixelShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	/** Default constructor. */
	FGammaCorrectionPixelShader() {}

public:

	FShaderParameter SceneTextureParameter;
	FShaderParameter InverseGammaParameter;
	FShaderParameter ColorScaleParameter;
	FShaderParameter OverlayColorParameter;

	/** Initialization constructor. */
	FGammaCorrectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SceneTextureParameter.Bind(Initializer.ParameterMap,TEXT("SceneColorTexture"));
		InverseGammaParameter.Bind(Initializer.ParameterMap,TEXT("InverseGamma"));
		ColorScaleParameter.Bind(Initializer.ParameterMap,TEXT("ColorScale"));
		OverlayColorParameter.Bind(Initializer.ParameterMap,TEXT("OverlayColor"));
	}

	// FShader interface.
	virtual void Serialize(FArchive& Ar)
	{
		FGlobalShader::Serialize(Ar);
		Ar << SceneTextureParameter << InverseGammaParameter << ColorScaleParameter << OverlayColorParameter;
	}
};

IMPLEMENT_SHADER_TYPE(,FGammaCorrectionPixelShader,TEXT("GammaCorrectionPixelShader"),TEXT("Main"),SF_Pixel,VER_SATURATE_COLOR_SHADER_RECOMPILE,0);

/** Encapsulates the gamma correction vertex shader. */
class FGammaCorrectionVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FGammaCorrectionVertexShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	/** Default constructor. */
	FGammaCorrectionVertexShader() {}

public:

	/** Initialization constructor. */
	FGammaCorrectionVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
	}
};

IMPLEMENT_SHADER_TYPE(,FGammaCorrectionVertexShader,TEXT("GammaCorrectionVertexShader"),TEXT("Main"),SF_Vertex,0,0);

extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

FGlobalBoundShaderStateRHIRef FSceneRenderer::PostProcessBoundShaderState;

/**
* Finish rendering a view, writing the contents to ViewFamily.RenderTarget.
* @param View - The view to process.
*/
void FSceneRenderer::FinishRenderViewTarget(const FViewInfo* View)
{
	// If the bUseLDRSceneColor flag is set then that means the final post-processing shader has already renderered to
	// the view's render target and that one of the post-processing shaders has performed the gamma correction.
	if( View->bUseLDRSceneColor || 
		// Also skip the final copy to View.RenderTarget if disabled by the view family
		!View->Family->bResolveScene )
	{
		return;
	}

	//if the shader complexity viewmode is enabled, use that to render to the view's rendertarget
	if (View->Family->ShowFlags & SHOW_ShaderComplexity)
	{
		RenderShaderComplexity(View);
		return;
	}

	// Set the view family's render target/viewport.
	RHISetRenderTarget(GlobalContext,ViewFamily.RenderTarget->GetRenderTargetSurface(),FSurfaceRHIRef());	

	// Deferred the clear until here so the garbage left in the non rendered regions by the post process effects do not show up
	if( ViewFamily.bDeferClear )
	{
		RHIClear( GlobalContext, TRUE, FLinearColor::Black, FALSE, 0.0f, FALSE, 0 );
		ViewFamily.bDeferClear = FALSE;
	}

	// turn off culling and blending
	RHISetRasterizerState(GlobalContext,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
		
	// turn off depth reads/writes
	RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());

	TShaderMapRef<FGammaCorrectionVertexShader> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FGammaCorrectionPixelShader> PixelShader(GetGlobalShaderMap());

	SetGlobalBoundShaderState(GlobalContext, PostProcessBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex));

	SetPixelShaderValue(
		GlobalContext,
		PixelShader->GetPixelShader(),
		PixelShader->InverseGammaParameter,
		1.0f / ViewFamily.RenderTarget->GetDisplayGamma()
		);
	SetPixelShaderValue(GlobalContext,PixelShader->GetPixelShader(),PixelShader->ColorScaleParameter,View->ColorScale);
	SetPixelShaderValue(GlobalContext,PixelShader->GetPixelShader(),PixelShader->OverlayColorParameter,View->OverlayColor);

	SetTextureParameter(
		GlobalContext,
		PixelShader->GetPixelShader(),
		PixelShader->SceneTextureParameter,
		TStaticSamplerState<>::GetRHI(),
		GSceneRenderTargets.GetSceneColorTexture()
		);

	// Draw a quad mapping the blurred pixels in the filter buffer to the scene color buffer.
	DrawDenormalizedQuad(
		GlobalContext,
		View->X,View->Y,
		View->SizeX,View->SizeY,
		View->RenderTargetX,View->RenderTargetY,
		View->RenderTargetSizeX,View->RenderTargetSizeY,
		ViewFamily.RenderTarget->GetSizeX(),ViewFamily.RenderTarget->GetSizeY(),
		GSceneRenderTargets.GetBufferSizeX(),GSceneRenderTargets.GetBufferSizeY()
		);
}
