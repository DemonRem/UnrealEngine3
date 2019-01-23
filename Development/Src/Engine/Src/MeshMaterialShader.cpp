/*=============================================================================
	MeshMaterialShader.cpp: Mesh material shader implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/**
* Finds a FMeshMaterialShaderType by name.
*/
FMeshMaterialShaderType* FMeshMaterialShaderType::GetTypeByName(const FString& TypeName)
{
	for(TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next())
	{
		FString CurrentTypeName = FString(It->GetName());
		FMeshMaterialShaderType* CurrentType = It->GetMeshMaterialShaderType();
		if (CurrentType && CurrentTypeName == TypeName)
		{
			return CurrentType;
		}
	}
	return NULL;
}

FShader* FMeshMaterialShaderType::CompileShader(
	EShaderPlatform Platform,
	const FMaterial* Material,
	const TCHAR* MaterialShaderCode,
	FVertexFactoryType* VertexFactoryType,
	TArray<FString>& OutErrors,
	UBOOL bSilent
	)
{
	// Construct the shader environment.
	FShaderCompilerEnvironment Environment;

	// apply the vertex factory changes to the compile environment
	VertexFactoryType->ModifyCompilationEnvironment(Environment);

	FString VertexFactoryFile = LoadShaderSourceFile(VertexFactoryType->GetShaderFilename());
	
	Environment.IncludeFiles.Set(TEXT("Material.usf"),MaterialShaderCode);
	Environment.IncludeFiles.Set(TEXT("VertexFactory.usf"),*VertexFactoryFile);

	switch(Material->GetBlendMode())
	{
	case BLEND_Opaque: Environment.Definitions.Set(TEXT("MATERIALBLENDING_SOLID"),TEXT("1")); break;
	case BLEND_Masked: Environment.Definitions.Set(TEXT("MATERIALBLENDING_MASKED"),TEXT("1")); break;
	case BLEND_Translucent: Environment.Definitions.Set(TEXT("MATERIALBLENDING_TRANSLUCENT"),TEXT("1")); break;
	case BLEND_Additive: Environment.Definitions.Set(TEXT("MATERIALBLENDING_ADDITIVE"),TEXT("1")); break;
	case BLEND_Modulate: Environment.Definitions.Set(TEXT("MATERIALBLENDING_MODULATE"),TEXT("1")); break;
	default: appErrorf(TEXT("Unknown material blend mode: %u"),(INT)Material->GetBlendMode());
	}

	Environment.Definitions.Set(TEXT("MATERIAL_TWOSIDED"),Material->IsTwoSided() ? TEXT("1") : TEXT("0"));

	switch(Material->GetLightingModel())
	{
	case MLM_SHPRT: // For backward compatibility, treat the deprecated SHPRT lighting model as Phong.
	case MLM_Phong: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_PHONG"),TEXT("1")); break;
	case MLM_NonDirectional: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_NONDIRECTIONAL"),TEXT("1")); break;
	case MLM_Unlit: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_UNLIT"),TEXT("1")); break;
	case MLM_Custom: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_CUSTOM"),TEXT("1")); break;
	default: appErrorf(TEXT("Unknown material lighting model: %u"),(INT)Material->GetLightingModel());
	};

	if( Material->GetTransformsUsed() != UsedCoord_None )
	{
		// only use WORLD_COORDS code if a Transform expression was used by the material
		Environment.Definitions.Set(TEXT("WORLD_COORDS"),TEXT("1"));
	}

	if( Material->IsUsedWithGammaCorrection() )
	{
		// only use USE_GAMMA_CORRECTION code when enabled
		Environment.Definitions.Set(TEXT("MATERIAL_USE_GAMMA_CORRECTION"),TEXT("1"));
	}	

	//calculate the CRC for this type and store it in the shader cache.
	//this will be used to determine when shader files have changed.
	DWORD CurrentCRC = GetSourceCRC();
	UShaderCache::SetShaderTypeCRC(this, CurrentCRC, Platform);

	//update material shader stats
	UpdateMaterialShaderCompilingStats(Material);

	// Compile the shader code.
	FShaderCompilerOutput Output;
	if(!FShaderType::CompileShader(Platform,Environment,Output,bSilent))
	{
		OutErrors = Output.Errors;
		return NULL;
	}

	// Check for shaders with identical compiled code.
	FShader* Shader = FindShaderByOutput(Output);
	if(!Shader)
	{
		// Create the shader.
		Shader = (*ConstructCompiledRef)(CompiledShaderInitializerType(this,Output,Material,VertexFactoryType));
	}
	return Shader;
}

/**
* Calculates a CRC based on this shader type's source code and includes
*/
DWORD FMeshMaterialShaderType::GetSourceCRC()
{
	DWORD SourceCRC = FShaderType::GetSourceCRC();
	SourceCRC ^= GetShaderFileCRC(TEXT("MaterialTemplate"));
	return SourceCRC;
}

UBOOL FMeshMaterialShaderMap::Compile(
	const FMaterial* Material,
	const TCHAR* MaterialShaderCode,
	FVertexFactoryType* InVertexFactoryType,
	EShaderPlatform Platform,
	TArray<FString>& OutErrors,
	UBOOL bSilent
	)
{
	UBOOL bSuccess = TRUE;
	UINT NumShadersPerVF = 0;
	VertexFactoryType = InVertexFactoryType;

	// Iterate over all mesh material shader types.
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		FMeshMaterialShaderType* ShaderType = ShaderTypeIt->GetMeshMaterialShaderType();
		if (ShaderType && 
			VertexFactoryType && 
			ShaderType->ShouldCache(Platform, Material, VertexFactoryType) && 
			Material->ShouldCache(Platform, ShaderType, VertexFactoryType) &&
			VertexFactoryType->ShouldCache(Platform, Material, ShaderType)
			)
		{
			// only compile the shader if we don't already have it
			if (!HasShader(ShaderType))
			{

#if DUMP_MATERIAL_SHADER_STATS
				warnf(TEXT("			%s"), ShaderType->GetName());
#endif
				NumShadersPerVF++;

			    // Compile this mesh material shader for this material and vertex factory type.
			    TArray<FString> ShaderErrors;
			    FShader* Shader = ShaderType->CompileShader(
				    Platform,
				    Material,
				    MaterialShaderCode,
				    VertexFactoryType,
				    ShaderErrors,
					bSilent
				    );
			    if(Shader)
			    {
				    AddShader(ShaderType,Shader);
			    }
			    else
			    {
				    for(INT ErrorIndex = 0;ErrorIndex < ShaderErrors.Num();ErrorIndex++)
				    {
					    OutErrors.AddUniqueItem(ShaderErrors(ErrorIndex));
				    }
				    bSuccess = FALSE;
					break;
			    }
			}
		}
	}

#if DUMP_MATERIAL_SHADER_STATS
	if (NumShadersPerVF > 0)
	{
		warnf(TEXT("		%s - %u shaders"), VertexFactoryType->GetName(), NumShadersPerVF);
	}
#endif

	//calculate the CRC for this vertex factory type and store it in the shader cache.
	//this will be used to determine when vertex factory shader files have changed.
	DWORD VertexFactoryCRC = VertexFactoryType->GetSourceCRC();
	UShaderCache::SetVertexFactoryTypeCRC(VertexFactoryType, VertexFactoryCRC, Platform);

	return bSuccess;
}

UBOOL FMeshMaterialShaderMap::IsComplete(EShaderPlatform Platform,const FMaterial* Material,FVertexFactoryType* InVertexFactoryType) const
{
	UBOOL bIsComplete = TRUE;

	// Iterate over all mesh material shader types.
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		FMeshMaterialShaderType* ShaderType = ShaderTypeIt->GetMeshMaterialShaderType();
		if (ShaderType && 
			ShaderType->ShouldCache(Platform, Material, InVertexFactoryType) && 
			Material->ShouldCache(Platform, ShaderType, InVertexFactoryType) &&
			InVertexFactoryType->ShouldCache(Platform, Material, ShaderType) &&
			!HasShader(ShaderType)
			)
		{
			// @debug
			extern UBOOL GDumpIsComplete;
			if (GDumpIsComplete)
			{
				debugf(TEXT("Don't have %s's shader %s [%x]"), VertexFactoryType->GetName(), ShaderType->GetName(), GEngine);
			}
			bIsComplete = FALSE;
			break;
		}
	}

	return bIsComplete;
}

/**
 * Removes all entries in the cache with exceptions based on a shader type
 * @param ShaderType - The shader type to flush or keep (depending on second param)
 * @param bFlushAllButShaderType - TRUE if all shaders EXCEPT the given type should be flush. FALSE will flush ONLY the given shader type
 */
void FMeshMaterialShaderMap::FlushShadersByShaderType(FShaderType* ShaderType, UBOOL bFlushAllButShaderType)
{
	// flush if flushing all but the given type, go over other shader types and remove them
	if (bFlushAllButShaderType)
	{
		for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
		{
			if (ShaderType != *ShaderTypeIt && ShaderTypeIt->GetMeshMaterialShaderType())
			{
				RemoveShaderType(ShaderTypeIt->GetMeshMaterialShaderType());
			}
		}
	}
	// otherwise just remove this type
	else if (ShaderType->GetMeshMaterialShaderType())
	{
		RemoveShaderType(ShaderType->GetMeshMaterialShaderType());
	}
}

FArchive& operator<<(FArchive& Ar,FMeshMaterialShaderMap& S)
{
	S.Serialize(Ar);
	Ar << S.VertexFactoryType;
	if (Ar.IsLoading())
	{
		// Check the required version for the vertex factory type
		// If the package version is less, toss the shaders.
		FVertexFactoryType* VFType = S.GetVertexFactoryType();
		if (VFType && (Ar.Ver() < VFType->GetMinPackageVersion() || Ar.LicenseeVer() < VFType->GetMinLicenseePackageVersion()))
		{
			S.Empty();
		}
	}
	return Ar;
}
