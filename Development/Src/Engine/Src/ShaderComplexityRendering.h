/*=============================================================================
ShaderComplexityRendering.h: Declarations used for the shader complexity viewmode.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
* Pixel shader that renders normalized shader complexity.
*/
class FShaderComplexityAccumulatePixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderComplexityAccumulatePixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	FShaderComplexityAccumulatePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
	{
		NormalizedComplexityParameter.Bind(Initializer.ParameterMap,TEXT("NormalizedComplexity"), TRUE);
	}

	FShaderComplexityAccumulatePixelShader() {}

	void SetParameters(
		FCommandContextRHI* Context,
		UINT NumVertexInstructions, 
		UINT NumPixelInstructions);

	virtual void Serialize(FArchive& Ar)
	{
		FGlobalShader::Serialize(Ar);
		Ar << NormalizedComplexityParameter;
	}

private:
	FShaderParameter NormalizedComplexityParameter;
};
