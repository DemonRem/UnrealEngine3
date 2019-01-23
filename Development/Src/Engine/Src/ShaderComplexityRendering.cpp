/*=============================================================================
ShaderComplexityRendering.cpp: Contains definitions for rendering the shader complexity viewmode.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

/**
 * Gets the maximum shader complexity count from the ini settings, based on the current complexity viewmode settings.
 */
FLOAT GetMaxShaderComplexityCount()
{
	if (GEngine->bUsePixelShaderComplexity)
	{
		if (GEngine->bUseAdditiveComplexity)
		{
			return GEngine->MaxPixelShaderAdditiveComplexityCount;
		}
		else
		{
			return GEngine->MaxPixelShaderOpaqueComplexityCount;
		}
	}
	else
	{
		return GEngine->MaxVertexShaderComplexityCount;
	}
}

void FShaderComplexityAccumulatePixelShader::SetParameters(
	FCommandContextRHI* Context,
	UINT NumVertexInstructions, 
	UINT NumPixelInstructions)
{
	//use pixel shader complexity if specified, otherwise use vertex shader complexity
	const UINT NumInstructionsPixelShader = GEngine->bUsePixelShaderComplexity ? NumPixelInstructions : NumVertexInstructions;

	//normalize the complexity so we can fit it in a low precision scene color which is necessary on some platforms
	const FLOAT NormalizedComplexity = FLOAT(NumInstructionsPixelShader) / GetMaxShaderComplexityCount();

	SetPixelShaderValue( Context, GetPixelShader(), NormalizedComplexityParameter, NormalizedComplexity);
}

IMPLEMENT_SHADER_TYPE(,FShaderComplexityAccumulatePixelShader,TEXT("ShaderComplexityAccumulatePixelShader"),TEXT("Main"),SF_Pixel,0,0);

/**
* The number of shader complexity colors from the engine ini that will be passed to the shader. 
* Changing this requires a recompile of the FShaderComplexityApplyPixelShader.
*/
const UINT NumShaderComplexityColors = 6;

/**
* Pixel shader that is used to map shader complexity stored in scene color into color.
*/
class FShaderComplexityApplyPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderComplexityApplyPixelShader,Global);
public:

	/** 
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FShaderComplexityApplyPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SceneTextureParams.Bind(Initializer.ParameterMap);
		ShaderComplexityColorsParameter.Bind(Initializer.ParameterMap,TEXT("ShaderComplexityColors"), TRUE);
	}

	FShaderComplexityApplyPixelShader() 
	{
	}

	/**
	* Sets the current pixel shader params
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	virtual void SetParameters(FCommandContextRHI* Context,
		const FSceneView* View
		)
	{
		SceneTextureParams.Set(Context,View,this);

		//Make sure there are at least NumShaderComplexityColors colors specified in the ini.
		//If there are more than NumShaderComplexityColors they will be ignored.
		check(GEngine->ShaderComplexityColors.Num() >= NumShaderComplexityColors);

		//pass the complexity -> color mapping into the pixel shader
		for(INT ColorIndex = 0; ColorIndex < NumShaderComplexityColors; ColorIndex ++)
		{
			FLinearColor CurrentColor = FLinearColor(GEngine->ShaderComplexityColors(ColorIndex));
			SetPixelShaderValue(
				Context,
				GetPixelShader(),
				ShaderComplexityColorsParameter,
				CurrentColor,
				ColorIndex
				);
		}

	}

	/**
	* @param Platform - hardware platform
	* @return TRUE if this shader should be cached
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("NUM_COMPLEXITY_COLORS"),*FString::Printf(TEXT("%u"), NumShaderComplexityColors));
	}

	/**
	* Serialize shader parameters for this shader
	* @param Ar - archive to serialize with
	*/
	void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << SceneTextureParams;
		Ar << ShaderComplexityColorsParameter;
	}

private:

	FSceneTextureShaderParameters SceneTextureParams;
	FShaderParameter ShaderComplexityColorsParameter;
};

IMPLEMENT_SHADER_TYPE(,FShaderComplexityApplyPixelShader,TEXT("ShaderComplexityApplyPixelShader"),TEXT("Main"),SF_Pixel,0,0);

//reuse the generic filter vertex declaration
extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

FGlobalBoundShaderStateRHIRef FSceneRenderer::ShaderComplexityBoundShaderState;

/**
* Renders shader complexity colors to the ViewFamily's rendertarget. 
*/
void FSceneRenderer::RenderShaderComplexity(const FViewInfo* View)
{
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

	//reuse this generic vertex shader
	TShaderMapRef<FLDRTranslucencyCombineVertexShader> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FShaderComplexityApplyPixelShader> PixelShader(GetGlobalShaderMap());

	SetGlobalBoundShaderState(GlobalContext, ShaderComplexityBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex));

	PixelShader->SetParameters(GlobalContext, View);
	
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

