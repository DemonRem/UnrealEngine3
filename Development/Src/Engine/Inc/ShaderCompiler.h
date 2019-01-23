/*=============================================================================
	ShaderCompiler.h: Platform independent shader compilation definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

enum EShaderFrequency
{
	SF_Vertex			= 0,
	SF_Pixel			= 1,

	SF_NumBits			= 1
};

/** @warning: update ShaderPlatformToText and GetMaterialPlatform when the below changes */
enum EShaderPlatform
{
	SP_PCD3D_SM3		= 0,
	SP_PS3				= 1,
	SP_XBOXD3D			= 2,
	SP_PCD3D_SM2		= 3,

	SP_NumPlatforms		= 4,
	SP_NumBits			= 2,
};

/**
 * Checks whether a shader platform supports hardware blending with FP render targets.
 * @param Platform - The shader platform to check.
 * @return TRUE if the platform supports hardware blending with FP render targets.
 */
extern UBOOL CanBlendWithFPRenderTarget(UINT Platform);

/**
 * Converts shader platform to human readable string. 
 *
 * @param ShaderPlatform	Shader platform enum
 * @return text representation of enum
 */
extern const TCHAR* ShaderPlatformToText( EShaderPlatform ShaderPlatform );

/**
 * Material shader platforms represent the different versions of FMaterialResources 
 * that will have to be stored for a single UMaterial.  Note that multiple shader platforms 
 * can map to a single material platform.
 */
enum EMaterialShaderPlatform
{
	MSP_SM3                 =0,
	MSP_SM2                 =1,
	MSP_MAX                 =2
};

/**
 * Gets the material platform matching the input shader platform
 */
FORCEINLINE EMaterialShaderPlatform GetMaterialPlatform(EShaderPlatform ShaderPlatform)
{
	//MaterialPlatformTable needs to be updated if SP_NumPlatforms changes
	checkSlow(SP_NumPlatforms == 4);
	static const EMaterialShaderPlatform MaterialPlatformTable[] = { MSP_SM3, MSP_SM3, MSP_SM3, MSP_SM2 };
	checkSlow(ShaderPlatform >= 0 && ShaderPlatform < SP_NumPlatforms);
	return MaterialPlatformTable[ShaderPlatform];
}

struct FShaderTarget
{
	BITFIELD Frequency : SF_NumBits;
	BITFIELD Platform : SP_NumBits;
};

enum ECompilerFlags
{
	CFLAG_PreferFlowControl = 0,
	CFLAG_Debug = 1,
	CFLAG_AvoidFlowControl = 2
};

/**
 * A map of shader parameter names to registers allocated to that parameter.
 */
class FShaderParameterMap
{
public:
	UBOOL FindParameterAllocation(const TCHAR* ParameterName,WORD& OutBaseRegisterIndex,WORD& OutNumRegisters) const;
	void AddParameterAllocation(const TCHAR* ParameterName,WORD BaseRegisterIndex,WORD NumRegisters);
	DWORD GetCRC() const
	{
		DWORD ParamterCRC = 0;
		for(TMap<FString,FParameterAllocation>::TConstIterator ParameterIt(ParameterMap);ParameterIt;++ParameterIt)
		{
			const FString& ParamName = ParameterIt.Key();
			const FParameterAllocation& ParamValue = ParameterIt.Value();
			ParamterCRC ^= appStrCrc(*ParamName) ^ appMemCrc(&ParamValue, sizeof(FParameterAllocation));
		}
		return ParamterCRC;
	}
private:
	struct FParameterAllocation
	{
		WORD BaseRegisterIndex;
		WORD NumRegisters;
	};
	TMap<FString,FParameterAllocation> ParameterMap;
};

/**
 * The environment used to compile a shader.
 */
struct FShaderCompilerEnvironment
{
	TMap<FString,FString> IncludeFiles;
	TMap<FName,FString> Definitions;
	TArray<ECompilerFlags> CompilerFlags;
};

/**
 * The output of the shader compiler.
 */
struct FShaderCompilerOutput
{
	FShaderCompilerOutput() :
		NumInstructions(0)
	{}

	FShaderParameterMap ParameterMap;
	TArray<FString> Errors;
	FShaderTarget Target;
	TArray<BYTE> Code;
	UINT NumInstructions;
};

/**
 * Loads the shader file with the given name.
 * @return The contents of the shader file.
 */
extern FString LoadShaderSourceFile(const TCHAR* Filename);

/**
 * Compiles a shader by invoking the platform-specific shader compiler for the given platform.
 */
extern UBOOL CompileShader(
	const TCHAR* SourceFilename,
	const TCHAR* FunctionName,
	FShaderTarget Target,
	const FShaderCompilerEnvironment& Environment,
	FShaderCompilerOutput& Output,
	UBOOL bSilent
	);

/** The shader precompilers for each platform.  These are only set during the console shader compilation while cooking or in the PrecompileShaders commandlet. */
extern class FConsoleShaderPrecompiler* GConsoleShaderPrecompilers[SP_NumPlatforms];

enum EShaderCompilingStats
{
	STAT_ShaderCompiling_MaterialShaders = STAT_ShaderCompilingFirstStat,
	STAT_ShaderCompiling_GlobalShaders,
	STAT_ShaderCompiling_RHI,
	STAT_ShaderCompiling_LoadingShaderFiles,
	STAT_ShaderCompiling_CRCingShaderFiles,
	STAT_ShaderCompiling_HLSLTranslation,
	STAT_ShaderCompiling_NumTotalMaterialShaders,
	STAT_ShaderCompiling_NumSpecialMaterialShaders,
	STAT_ShaderCompiling_NumTerrainMaterialShaders,
	STAT_ShaderCompiling_NumDecalMaterialShaders,
	STAT_ShaderCompiling_NumParticleMaterialShaders,
	STAT_ShaderCompiling_NumSkinnedMaterialShaders,
	STAT_ShaderCompiling_NumLitMaterialShaders,
	STAT_ShaderCompiling_NumUnlitMaterialShaders,
	STAT_ShaderCompiling_NumTransparentMaterialShaders,
	STAT_ShaderCompiling_NumOpaqueMaterialShaders,
	STAT_ShaderCompiling_NumMaskedMaterialShaders
};
