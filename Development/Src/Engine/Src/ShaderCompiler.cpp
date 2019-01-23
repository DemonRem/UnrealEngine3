/*=============================================================================
	ShaderCompiler.cpp: Platform independent shader compilation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnConsoleTools.h"

UBOOL CanBlendWithFPRenderTarget(UINT Platform)
{
	if(Platform == SP_PCD3D_SM2)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/**
* Converts shader platform to human readable string. 
*
* @param ShaderPlatform	Shader platform enum
* @return text representation of enum
*/
const TCHAR* ShaderPlatformToText( EShaderPlatform ShaderPlatform )
{
	switch( ShaderPlatform )
	{
	case SP_PCD3D_SM2:
		return TEXT("PC-D3D-SM2");
		break;
	case SP_PCD3D_SM3:
		return TEXT("PC-D3D-SM3");
		break;
	case SP_XBOXD3D:
		return TEXT("Xbox360");
		break;
	case SP_PS3:
		return TEXT("PS3");
		break;
	default:
		return TEXT("Unknown");
		break;
	}
}

FConsoleShaderPrecompiler* GConsoleShaderPrecompilers[SP_NumPlatforms] = { NULL, NULL, NULL, NULL};

DECLARE_STATS_GROUP(TEXT("ShaderCompiling"),STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Total Material Shader Compiling Time"),STAT_ShaderCompiling_MaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Total Global Shader Compiling Time"),STAT_ShaderCompiling_GlobalShaders,STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("RHI Compile Time"),STAT_ShaderCompiling_RHI,STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("CRCing Shader Files"),STAT_ShaderCompiling_CRCingShaderFiles,STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Loading Shader Files"),STAT_ShaderCompiling_LoadingShaderFiles,STATGROUP_ShaderCompiling);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("HLSL Translation"),STAT_ShaderCompiling_HLSLTranslation,STATGROUP_ShaderCompiling);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Total Material Shaders"),STAT_ShaderCompiling_NumTotalMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Special Material Shaders"),STAT_ShaderCompiling_NumSpecialMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Terrain Material Shaders"),STAT_ShaderCompiling_NumTerrainMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Decal Material Shaders"),STAT_ShaderCompiling_NumDecalMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Particle Material Shaders"),STAT_ShaderCompiling_NumParticleMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Skinned Material Shaders"),STAT_ShaderCompiling_NumSkinnedMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Lit Material Shaders"),STAT_ShaderCompiling_NumLitMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Unlit Material Shaders"),STAT_ShaderCompiling_NumUnlitMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Transparent Material Shaders"),STAT_ShaderCompiling_NumTransparentMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Opaque Material Shaders"),STAT_ShaderCompiling_NumOpaqueMaterialShaders,STATGROUP_ShaderCompiling);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num Masked Material Shaders"),STAT_ShaderCompiling_NumMaskedMaterialShaders,STATGROUP_ShaderCompiling);

extern UBOOL D3DCompileShader(
	const TCHAR* SourceFilename,
	const TCHAR* FunctionName,
	FShaderTarget Target,
	const FShaderCompilerEnvironment& Environment,
	FShaderCompilerOutput& Output,
	UBOOL bSilent
	);

extern UBOOL XeD3DCompileShader(
	const TCHAR* SourceFilename,
	const TCHAR* FunctionName,
	FShaderTarget Target,
	const FShaderCompilerEnvironment& Environment,
	FShaderCompilerOutput& Output
	);

UBOOL CompileShader(
	const TCHAR* SourceFilename,
	const TCHAR* FunctionName,
	FShaderTarget Target,
	const FShaderCompilerEnvironment& InEnvironment,
	FShaderCompilerOutput& Output,
	UBOOL bSilent = FALSE
	)
{
	STAT(DOUBLE RHICompileTime = 0);

	UBOOL bResult = FALSE;
	UBOOL bRetry;
	
	FShaderCompilerEnvironment Environment( InEnvironment );

	// #define PIXELSHADER and VERTEXSHADER accordingly
	{
		Environment.Definitions.Set( TEXT("PIXELSHADER"),  (Target.Frequency == SF_Pixel) ? TEXT("1") : TEXT("0") );
		Environment.Definitions.Set( TEXT("VERTEXSHADER"), (Target.Frequency == SF_Vertex) ? TEXT("1") : TEXT("0") );
	}

	do 
	{
		SCOPE_SECONDS_COUNTER(RHICompileTime);
		bRetry = FALSE;

#if defined(_COMPILESHADER_DEBUG_DUMP_)
		warnf(TEXT("   Compiling shader %s[%s] for platform %d"), SourceFilename, FunctionName, Target.Platform);
#endif	//#if defined(_COMPILESHADER_DEBUG_DUMP_)

		if(Target.Platform == GRHIShaderPlatform)
		{
#if XBOX
			check(Target.Platform == SP_XBOXD3D);
			bResult = XeD3DCompileShader(SourceFilename,FunctionName,Target,Environment,Output);
#elif !CONSOLE && !PLATFORM_UNIX  // !!! FIXME
			check(Target.Platform == SP_PCD3D_SM3 || Target.Platform == SP_PCD3D_SM2);
			bResult = D3DCompileShader(SourceFilename,FunctionName,Target,Environment,Output,bSilent);
#else
			appErrorf(TEXT("Attempted to compile \'%s\' shader on platform that doesn't support dynamic shader compilation."),SourceFilename);
#endif
		}
		// If we're compiling for a different platform than we're running on, use the external precompiler.
		else
		{
#if !CONSOLE && !PLATFORM_UNIX  // !!! FIXME
			if( Target.Platform == SP_PCD3D_SM2 || Target.Platform == SP_PCD3D_SM3)
			{
				bResult = D3DCompileShader(SourceFilename,FunctionName,Target,Environment,Output,bSilent);
			}
			else
			{
				// if we have a global shader precompiler object, use that to compile the shader instead
				check(GConsoleShaderPrecompilers[Target.Platform]);
				bResult = GConsoleShaderPrecompilers[Target.Platform]->CallPrecompiler(SourceFilename, FunctionName, Target, Environment, Output);
			}
#else
			appErrorf(TEXT("Attempted to compile \'%s\' shader for non-native platform %d on console."),SourceFilename,Target.Platform);
#endif
		}

#if defined(_DEBUG) && !CONSOLE
		// Give the user a chance to fix the shader error.
		if (!bSilent)
		{
			if ( !bResult && GWarn->YesNof( TEXT("Failed to compile %s:\r\n%s\r\n\r\nWould you like to try again?"), SourceFilename, Output.Errors.Num() ? *Output.Errors(0) : TEXT("<unknown error>") ) )
			{
				FlushShaderFileCache();
				bRetry = TRUE;
			}
		}
		
#endif
	} while ( bRetry );

	INC_FLOAT_STAT_BY(STAT_ShaderCompiling_RHI,(FLOAT)RHICompileTime);

	return bResult;
}
