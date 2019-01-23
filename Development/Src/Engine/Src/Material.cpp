/*=============================================================================
	UnMaterial.cpp: Shader implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "EngineDecalClasses.h"

IMPLEMENT_CLASS(UMaterial);

FMaterialResource::FMaterialResource(UMaterial* InMaterial):
	Material(InMaterial)
{
}

INT FMaterialResource::CompileProperty(EMaterialProperty Property,FMaterialCompiler* Compiler) const
{
	INT SelectionColorIndex = Compiler->ComponentMask(Compiler->VectorParameter(NAME_SelectionColor,FLinearColor::Black),1,1,1,0);

	switch(Property)
	{
	case MP_EmissiveColor:
		return Compiler->Add(Compiler->ForceCast(Material->EmissiveColor.Compile(Compiler,FColor(0,0,0)),MCT_Float3),SelectionColorIndex);
	case MP_Opacity: return Material->Opacity.Compile(Compiler,1.0f);
	case MP_OpacityMask: return Material->OpacityMask.Compile(Compiler,1.0f);
	case MP_Distortion: return Material->Distortion.Compile(Compiler,FVector2D(0,0));
	case MP_TwoSidedLightingMask: return Compiler->Mul(Compiler->ForceCast(Material->TwoSidedLightingMask.Compile(Compiler,0.0f),MCT_Float),Material->TwoSidedLightingColor.Compile(Compiler,FColor(255,255,255)));
	case MP_DiffuseColor:
		return Compiler->Mul(Compiler->ForceCast(Material->DiffuseColor.Compile(Compiler,FColor(0,0,0)),MCT_Float3),Compiler->Sub(Compiler->Constant(1.0f),SelectionColorIndex));
	case MP_SpecularColor: return Material->SpecularColor.Compile(Compiler,FColor(0,0,0));
	case MP_SpecularPower: return Material->SpecularPower.Compile(Compiler,15.0f);
	case MP_Normal: return Material->Normal.Compile(Compiler,FVector(0,0,1));
	case MP_CustomLighting: return Material->CustomLighting.Compile(Compiler,FColor(0,0,0));
	default:
		return INDEX_NONE;
	};
}

/**
 * A resource which represents the default instance of a UMaterial to the renderer.
 * Note that default parameter values are stored in the FMaterialUniformExpressionXxxParameter objects now.
 * This resource is only responsible for the selection color.
 */
class FDefaultMaterialInstance : public FMaterialRenderProxy
{
public:

	// FMaterialRenderProxy interface.
	virtual const class FMaterial* GetMaterial() const
	{
		const FMaterialResource * MaterialResource = Material->GetMaterialResource();
		if (MaterialResource && MaterialResource->GetShaderMap())
		{
			return MaterialResource;
		}

		// this check is to stop the infinite "retry to compile DefaultMaterial" which can occur when MSP types are mismatched or another similar error state
		check(this != GEngine->DefaultMaterial->GetRenderProxy(bSelected));	
		return GEngine->DefaultMaterial->GetRenderProxy(bSelected)->GetMaterial();
	}
	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		const FMaterialResource * MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			if(ParameterName == NAME_SelectionColor)
			{
				static const FLinearColor SelectionColor(10.0f/255.0f,5.0f/255.0f,60.0f/255.0f,1);
				*OutValue = bSelected ? SelectionColor : FLinearColor::Black;
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected)->GetVectorValue(ParameterName, OutValue, Context);
		}
	}
	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		const FMaterialResource * MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected)->GetScalarValue(ParameterName, OutValue, Context);
		}
	}
	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue) const
	{
		const FMaterialResource * MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected)->GetTextureValue(ParameterName,OutValue);
		}
	}

	// Constructor.
	FDefaultMaterialInstance(UMaterial* InMaterial,UBOOL bInSelected):
		Material(InMaterial),
		bSelected(bInSelected)
	{}

private:

	UMaterial* Material;
	UBOOL bSelected;
};

UMaterial::UMaterial()
{
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		DefaultMaterialInstances[FALSE] = new FDefaultMaterialInstance(this,FALSE);
		if(GIsEditor)
		{
			DefaultMaterialInstances[TRUE] = new FDefaultMaterialInstance(this,TRUE);
		}
	}

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		MaterialResources[PlatformIndex] = NULL;
	}
}

/** @return TRUE if the material uses distortion */
UBOOL UMaterial::HasDistortion() const
{
    return bUsesDistortion && IsTranslucentBlendMode((EBlendMode)BlendMode);
}

/** @return TRUE if the material uses the scene color texture */
UBOOL UMaterial::UsesSceneColor() const
{
	return bUsesSceneColor;
}

/**
* Allocates a material resource off the heap to be stored in MaterialResource.
*/
FMaterialResource* UMaterial::AllocateResource()
{
	return new FMaterialResource(this);
}

/**
 * Checks if the material has the appropriate flag set to be used with skeletal meshes.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with the skeletal mesh, False if the default material should be used.
 */
UBOOL UMaterial::UseWithSkeletalMesh()
{
	// Check that the material has been flagged for use with skeletal meshes.
	if(!bUsedWithSkeletalMesh && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithSkeletalMesh = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			debugf(TEXT("Material %s used with skeletal mesh, but missing bUsedWithSkeletalMesh=True"),*GetPathName());

			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with particle sprites.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with particle sprites, False if the default material should be used.
 */
UBOOL UMaterial::UseWithParticleSprites()
{
	if(!bUsedWithParticleSprites && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithParticleSprites = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			warnf(TEXT("Material %s used with particle sprites, but missing bUsedWithParticleSprites=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with beam trails.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with beam trails, False if the default material should be used.
 */
UBOOL UMaterial::UseWithBeamTrails()
{
	if(!bUsedWithBeamTrails && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithBeamTrails = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			warnf(TEXT("Material %s used with beam trails, but missing bUsedWithBeamTrails=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with SubUV Particles.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with SubUV Particles, False if the default material should be used.
 */
UBOOL UMaterial::UseWithParticleSubUV()
{
	if(!bUsedWithParticleSubUV && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithParticleSubUV = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			warnf(TEXT("Material %s used with particle Sub UV, but missing bUsedWithParticleSubUV=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with foliage.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with foliage, False if the default material should be used.
 */
UBOOL UMaterial::UseWithFoliage()
{
	// Check that the material has been flagged for use with skeletal meshes.
	if(!bUsedWithFoliage && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithFoliage = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			debugf(TEXT("Material %s used with foliage, but missing bUsedWithFoliage=True"),*GetPathName());

			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with speed trees.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with speed trees, False if the default material should be used.
 */
UBOOL UMaterial::UseWithSpeedTree()
{
	// Check that the material has been flagged for use with speed tree.
	if(!bUsedWithSpeedTree && !bUsedAsSpecialEngineMaterial)
	{
		if( GIsEditor )
		{
			// If the flag is missing in the editor, set it, recompile shaders and mark package dirty.
			bUsedWithSpeedTree = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			debugf(TEXT("Material %s used with SpeedTree, but missing bUsedWithSpeedTree=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}
	return TRUE;
}


/**
 * Checks if the material has the appropriate flag set to be used with static lighting.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with static lighting, False if the default material should be used.
 */
UBOOL UMaterial::UseWithStaticLighting()
{
	// Check that the material has been flagged for use with speed tree.
	if(!bUsedWithStaticLighting && !bUsedAsSpecialEngineMaterial)
	{
		if( GIsEditor )
		{
			// If the flag is missing in the editor, set it, recompile shaders and mark package dirty.
			bUsedWithStaticLighting = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			debugf(TEXT("Material %s used with static lighting, but missing bUsedWithStaticLighting=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Checks if the material has the appropriate flag set to be used with lens flares.
 * If it's the editor, and it isn't set, it sets it.
 * If it's the game, and it isn't set, it will return False.
 * @return True if the material may be used with lens flares, False if the default material should be used.
 */
UBOOL UMaterial::UseWithLensFlare()
{
	if(!bUsedWithLensFlare && !bUsedAsSpecialEngineMaterial)
	{
		if(GIsEditor)
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			bUsedWithLensFlare = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			warnf(TEXT("Material %s used with lens flare, but missing bUsedWithLensFlare=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}

	return TRUE;
}

/**
* Checks if the material has the appropriate flag set to be used with gamma correction.
* If it's the editor, and it isn't set, it sets it.
* If it's the game, and it isn't set, it will return False.
* @return True if the material may be used with gamma correction, False if the non-gamma correction version can be used
*/
UBOOL UMaterial::UseWithGammaCorrection()
{
	// Check that the material has been flagged for use with speed tree.
	if(!bUsedWithGammaCorrection && !bUsedAsSpecialEngineMaterial)
	{
		if( GIsEditor )
		{
			// If the flag is missing in the editor, set it, recompile shaders and mark package dirty.
			bUsedWithGammaCorrection = TRUE;
			CacheResourceShaders();
			MarkPackageDirty();
		}
		else
		{
			debugf(TEXT("Material %s used needs gamma correction, but missing bUsesGammaCorrection=True"),*GetPathName());
			// If the flag is missing in the game, return failure.
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of vector parameter names used in this material.
 */
void UMaterial::GetAllVectorParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionVectorParameter* VectorSampleParameter =
			Cast<UMaterialExpressionVectorParameter>(Expressions(ExpressionIndex));

		if(VectorSampleParameter)
		{
			OutParameterNames.AddUniqueItem(VectorSampleParameter->ParameterName);
		}
	}
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of scalar parameter names used in this material.
 */
void UMaterial::GetAllScalarParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionScalarParameter* ScalarSampleParameter =
			Cast<UMaterialExpressionScalarParameter>(Expressions(ExpressionIndex));

		if(ScalarSampleParameter)
		{
			OutParameterNames.AddUniqueItem(ScalarSampleParameter->ParameterName);
		}
	}
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of texture parameter names used in this material.
 */
void UMaterial::GetAllTextureParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionTextureSampleParameter* TextureSampleParameter =
			Cast<UMaterialExpressionTextureSampleParameter>(Expressions(ExpressionIndex));

		if(TextureSampleParameter)
		{
			OutParameterNames.AddUniqueItem(TextureSampleParameter->ParameterName);
		}
	}
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of font parameter names used in this material.
 */
void UMaterial::GetAllFontParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionFontSampleParameter* FontSampleParameter =
			Cast<UMaterialExpressionFontSampleParameter>(Expressions(ExpressionIndex));

		if(FontSampleParameter)
		{
			OutParameterNames.AddUniqueItem(FontSampleParameter->ParameterName);
		}
	}
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of static switch parameter names used in this material.
 */
void UMaterial::GetAllStaticSwitchParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticSwitchParameter* SwitchParameter =
			Cast<UMaterialExpressionStaticSwitchParameter>(Expressions(ExpressionIndex));

		if(SwitchParameter)
		{
			OutParameterNames.AddUniqueItem(SwitchParameter->ParameterName);
		}
	}
}

/**
 * @param	OutParameterNames		Storage array for the parameter names we are returning.
 *
 * @return	Returns a array of static component mask parameter names used in this material.
 */
void UMaterial::GetAllStaticComponentMaskParameterNames(TArray<FName> &OutParameterNames)
{
	OutParameterNames.Empty();
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticComponentMaskParameter* ComponentMaskParameter =
			Cast<UMaterialExpressionStaticComponentMaskParameter>(Expressions(ExpressionIndex));

		if(ComponentMaskParameter)
		{
			OutParameterNames.AddUniqueItem(ComponentMaskParameter->ParameterName);
		}
	}
}

/**
 * Builds a string of parameters in the fallback material that do not exist in the base material.
 * These parameters won't be set by material instances, which get their parameter list from the base material.
 *
 * @param ParameterMismatches - string of unmatches material names to populate
 */
void UMaterial::GetFallbackParameterInconsistencies(FString &ParameterMismatches)
{
	ParameterMismatches.Empty();
	if (FallbackMaterial)
	{
		const UINT NumParamTypes = 5;
		TArray<FName> BaseParameterNames[NumParamTypes];
		TArray<FName> FallbackParameterNames[NumParamTypes];

		//gather parameter names from the fallback material and the base material
		GetAllVectorParameterNames(BaseParameterNames[0]);
		FallbackMaterial->GetAllVectorParameterNames(FallbackParameterNames[0]);

		GetAllScalarParameterNames(BaseParameterNames[1]);
		FallbackMaterial->GetAllScalarParameterNames(FallbackParameterNames[1]);

		GetAllTextureParameterNames(BaseParameterNames[2]);
		FallbackMaterial->GetAllTextureParameterNames(FallbackParameterNames[2]);

		GetAllStaticSwitchParameterNames(BaseParameterNames[3]);
		FallbackMaterial->GetAllStaticSwitchParameterNames(FallbackParameterNames[3]);

		GetAllStaticComponentMaskParameterNames(BaseParameterNames[4]);
		FallbackMaterial->GetAllStaticComponentMaskParameterNames(FallbackParameterNames[4]);

		INT FoundIndex = 0;

		//go through each parameter type
		for (INT ParamTypeIndex = 0; ParamTypeIndex < NumParamTypes; ParamTypeIndex++)
		{
			//and iterate over each parameter in the fallback material
			for (INT ParamIndex = 0; ParamIndex < FallbackParameterNames[ParamTypeIndex].Num(); ParamIndex++)
			{
				//check if the fallback material parameter exists in the base material (this material)
				if (!BaseParameterNames[ParamTypeIndex].FindItem(FallbackParameterNames[ParamTypeIndex](ParamIndex), FoundIndex))
				{
					//the parameter wasn't found, add a warning
					FString ParameterName;
					FallbackParameterNames[ParamTypeIndex](ParamIndex).ToString(ParameterName);
					ParameterMismatches += FString(FString::Printf( TEXT("\n	 Parameter '%s' not found"), *ParameterName));
				}
			}
		}
	}
}

UMaterial* UMaterial::GetMaterial()
{
	//pass the request to the fallback material if one exists and this is a sm2 platform
	if (FallbackMaterial && GRHIShaderPlatform == SP_PCD3D_SM2)
	{
		return FallbackMaterial->GetMaterial();
	}

	const FMaterialResource * MaterialResource = GetMaterialResource();

	if(MaterialResource)
	{
		return this;
	}
	else
	{
		return GEngine ? GEngine->DefaultMaterial : NULL;
	}
}

UBOOL UMaterial::GetVectorParameterValue(FName ParameterName, FLinearColor& OutValue)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionVectorParameter* VectorParameter =
			Cast<UMaterialExpressionVectorParameter>(Expressions(ExpressionIndex));

		if(VectorParameter && VectorParameter->ParameterName == ParameterName)
		{
			OutValue = VectorParameter->DefaultValue;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

UBOOL UMaterial::GetScalarParameterValue(FName ParameterName, FLOAT& OutValue)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionScalarParameter* ScalarParameter =
			Cast<UMaterialExpressionScalarParameter>(Expressions(ExpressionIndex));

		if(ScalarParameter && ScalarParameter->ParameterName == ParameterName)
		{
			OutValue = ScalarParameter->DefaultValue;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

UBOOL UMaterial::GetTextureParameterValue(FName ParameterName, UTexture*& OutValue)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionTextureSampleParameter* TextureSampleParameter =
			Cast<UMaterialExpressionTextureSampleParameter>(Expressions(ExpressionIndex));

		if(TextureSampleParameter && TextureSampleParameter->ParameterName == ParameterName)
		{
			OutValue = TextureSampleParameter->Texture;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

UBOOL UMaterial::GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue,INT& OutFontPage)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionFontSampleParameter* FontSampleParameter =
			Cast<UMaterialExpressionFontSampleParameter>(Expressions(ExpressionIndex));

		if(FontSampleParameter && FontSampleParameter->ParameterName == ParameterName)
		{
			OutFontValue = FontSampleParameter->Font;
			OutFontPage = FontSampleParameter->FontTexturePage;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

/**
 * Gets the value of the given static switch parameter
 *
 * @param	ParameterName	The name of the static switch parameter
 * @param	OutValue		Will contain the value of the parameter if successful
 * @return					True if successful
 */
UBOOL UMaterial::GetStaticSwitchParameterValue(FName ParameterName,UBOOL &OutValue,FGuid &OutExpressionGuid)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticSwitchParameter* StaticSwitchParameter =
			Cast<UMaterialExpressionStaticSwitchParameter>(Expressions(ExpressionIndex));

		if(StaticSwitchParameter && StaticSwitchParameter->ParameterName == ParameterName)
		{
			OutValue = StaticSwitchParameter->DefaultValue;
			OutExpressionGuid = StaticSwitchParameter->ExpressionGUID;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

/**
 * Gets the value of the given static component mask parameter
 *
 * @param	ParameterName	The name of the parameter
 * @param	R, G, B, A		Will contain the values of the parameter if successful
 * @return					True if successful
 */
UBOOL UMaterial::GetStaticComponentMaskParameterValue(FName ParameterName, UBOOL &OutR, UBOOL &OutG, UBOOL &OutB, UBOOL &OutA, FGuid &OutExpressionGuid)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskParameter =
			Cast<UMaterialExpressionStaticComponentMaskParameter>(Expressions(ExpressionIndex));

		if(StaticComponentMaskParameter && StaticComponentMaskParameter->ParameterName == ParameterName)
		{
			OutR = StaticComponentMaskParameter->DefaultR;
			OutG = StaticComponentMaskParameter->DefaultG;
			OutB = StaticComponentMaskParameter->DefaultB;
			OutA = StaticComponentMaskParameter->DefaultA;
			OutExpressionGuid = StaticComponentMaskParameter->ExpressionGUID;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}

FMaterialRenderProxy* UMaterial::GetRenderProxy(UBOOL Selected) const
{
	check(!Selected || GIsEditor);
	return DefaultMaterialInstances[Selected];
}

UPhysicalMaterial* UMaterial::GetPhysicalMaterial() const
{
	return PhysMaterial;
}

/**
* Compiles a FMaterialResource on the given platform with the given static parameters
*
* @param StaticParameters - The set of static parameters to compile for
* @param StaticPermutation - The resource to compile
* @param Platform - The platform to compile for
* @param MaterialPlatform - The material platform to compile for
* @param bFlushExistingShaderMaps - Indicates that existing shader maps should be discarded
* @return TRUE if compilation was successful or not necessary
*/
UBOOL UMaterial::CompileStaticPermutation(
	FStaticParameterSet* StaticParameters, 
	FMaterialResource* StaticPermutation,  
	EShaderPlatform Platform,
	EMaterialShaderPlatform MaterialPlatform,
	UBOOL bFlushExistingShaderMaps)
{
	UBOOL CompileSucceeded = FALSE;

	const UBOOL bIsSM2Platform = MaterialPlatform == MSP_SM2;

	//pass the request to the fallback material if one exists and this is a sm2 platform
	if (FallbackMaterial && bIsSM2Platform)
	{
		return FallbackMaterial->CompileStaticPermutation(StaticParameters, StaticPermutation, Platform, MaterialPlatform, bFlushExistingShaderMaps);
	}

	//update the static parameter set with the base material's Id
	StaticParameters->BaseMaterialId = MaterialResources[MaterialPlatform]->GetId();

	SetStaticParameterOverrides(StaticParameters);

#if CONSOLE
	if (GUseSeekFreeLoading)
	{
		//uniform expressions are guaranteed to be updated since they are always generated during cooking
		CompileSucceeded = StaticPermutation->InitShaderMap(StaticParameters, Platform, bIsSM2Platform && !bIsFallbackMaterial);
	}
	else
#endif
	{
		//material instances with static permutations always need to regenerate uniform expressions, so InitShaderMap() is not used
		CompileSucceeded = StaticPermutation->CacheShaders(StaticParameters, Platform, bIsSM2Platform && !bIsFallbackMaterial, bFlushExistingShaderMaps);
	}
	
	ClearStaticParameterOverrides();
	
	return CompileSucceeded;
}

/**
 * Sets overrides in the material's static parameters
 *
 * @param	Permutation		The set of static parameters to override and their values	
 */
void UMaterial::SetStaticParameterOverrides(const FStaticParameterSet* Permutation)
{
	//go through each expression
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticSwitchParameter* StaticSwitchExpression =
			Cast<UMaterialExpressionStaticSwitchParameter>(Expressions(ExpressionIndex));

		//if it's a static switch, setup the override
		if (StaticSwitchExpression)
		{
			for(INT SwitchIndex = 0;SwitchIndex < Permutation->StaticSwitchParameters.Num();SwitchIndex++)
			{
				const FStaticSwitchParameter * InstanceSwitchParameter = &Permutation->StaticSwitchParameters(SwitchIndex);
				if(StaticSwitchExpression->ParameterName == InstanceSwitchParameter->ParameterName)
				{
					StaticSwitchExpression->InstanceOverride = InstanceSwitchParameter;
					break;
				}
			}
		}
		else
		{
			UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskExpression =
				Cast<UMaterialExpressionStaticComponentMaskParameter>(Expressions(ExpressionIndex));

			//if it's a static component mask, setup the override
			if (StaticComponentMaskExpression)
			{
				for(INT ComponentMaskIndex = 0;ComponentMaskIndex < Permutation->StaticComponentMaskParameters.Num();ComponentMaskIndex++)
				{
					const FStaticComponentMaskParameter * InstanceComponentMaskParameter = &Permutation->StaticComponentMaskParameters(ComponentMaskIndex);
					if(StaticComponentMaskExpression->ParameterName == InstanceComponentMaskParameter->ParameterName)
					{
						StaticComponentMaskExpression->InstanceOverride = InstanceComponentMaskParameter;
						break;
					}
				}
			}
		}
	}

}

/**
 * Clears static parameter overrides so that static parameter expression defaults will be used
 *	for subsequent compiles.
 */
void UMaterial::ClearStaticParameterOverrides()
{
	//go through each expression
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionStaticSwitchParameter* StaticSwitchExpression =
			Cast<UMaterialExpressionStaticSwitchParameter>(Expressions(ExpressionIndex));

		//if it's a static switch, clear the override
		if(StaticSwitchExpression)
		{
			StaticSwitchExpression->InstanceOverride = NULL;
		}
		else
		{
			UMaterialExpressionStaticComponentMaskParameter* StaticComponentMaskExpression =
				Cast<UMaterialExpressionStaticComponentMaskParameter>(Expressions(ExpressionIndex));

			//if it's a static component mask, clear the override
			if(StaticComponentMaskExpression)
			{
				StaticComponentMaskExpression->InstanceOverride = NULL;
			}
		}
	}
}

/**
 * Compiles material resources for the current platform if the shader map for that resource didn't already exist.
 *
 * @param bFlushExistingShaderMaps - forces a compile, removes existing shader maps from shader cache.
 * @param bForceAllPlatforms - compile for all platforms, not just the current.
 */
void UMaterial::CacheResourceShaders(UBOOL bFlushExistingShaderMaps, UBOOL bForceAllPlatforms)
{
	// Initialize GRHIShaderPlatform so we know which shaders to compile.
	RHIInitializeShaderPlatform();

	const EMaterialShaderPlatform RequestedMaterialPlatform = GetMaterialPlatform(GRHIShaderPlatform);
	const EShaderPlatform OtherPlatformsToCompile[] = {SP_PCD3D_SM3, SP_PCD3D_SM2};

	//go through each material resource
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		//allocate it if it hasn't already been
		if(!MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex] = AllocateResource();
		}

		//don't ever compile for sm2 if a fallback material is present, 
		//since the fallback material overrides this material's sm2 material resource
		if (PlatformIndex == MSP_SM2 && FallbackMaterial)
		{
			continue;
		}

		const UBOOL bIsSM2Platform = PlatformIndex == MSP_SM2;
		EShaderPlatform CurrentShaderPlatform = GRHIShaderPlatform;
		//if we're not compiling for the same platform that is running, 
		//lookup what platform we should compile for based on the current material platform
		if (PlatformIndex != RequestedMaterialPlatform)
		{
			CurrentShaderPlatform = OtherPlatformsToCompile[PlatformIndex];
		}

		//don't compile fallback materials for sm3 unless we're running the editor (where it will be needed for the preview material)
		if (PlatformIndex == MSP_SM3 && bIsFallbackMaterial && !GIsEditor)
		{
			continue;
		}

		//mark the package dirty if the material resource has never been compiled
		if (GIsEditor && !MaterialResources[PlatformIndex]->GetId().IsValid())
		{
			MarkPackageDirty();
		}

		//if the current material platform matches the one that we're running under, compile the material resource.
		if (bForceAllPlatforms || PlatformIndex == RequestedMaterialPlatform)
		{
			//only try to generate a fallback if we are running sm2 and the current material is not a user-specified fallback material
			if (bFlushExistingShaderMaps)
			{
				MaterialResources[PlatformIndex]->CacheShaders(CurrentShaderPlatform, bIsSM2Platform && !bIsFallbackMaterial);
			}
			else
			{
				MaterialResources[PlatformIndex]->InitShaderMap(CurrentShaderPlatform, bIsSM2Platform && !bIsFallbackMaterial);
			}
		}
	}
}

/**
 * Flushes existing resource shader maps and resets the material resource's Ids.
 */
void UMaterial::FlushResourceShaderMaps()
{
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if(MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex]->FlushShaderMap();
			MaterialResources[PlatformIndex]->SetId(FGuid(0,0,0,0));
		}
	}
}

/**
 * Gets the material resource based on the input platform
 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
 */
FMaterialResource * UMaterial::GetMaterialResource(EShaderPlatform Platform)
{
	const EMaterialShaderPlatform RequestedPlatform = GetMaterialPlatform(Platform);

	//if a fallback material is specified and this is the sm2 platform, pass the request on
	if (RequestedPlatform == MSP_SM2 && FallbackMaterial)
	{
		return FallbackMaterial->GetMaterialResource(Platform);
	}

	return MaterialResources[RequestedPlatform];
}

/**
 * Called before serialization on save to propagate referenced textures. This is not done
 * during content cooking as the material expressions used to retrieve this information will
 * already have been dissociated via RemoveExpressions
 */
void UMaterial::PreSave()
{
	Super::PreSave();

	if( !IsTemplate() )
	{
		// Ensure that the ReferencedTextures array is up to date.
		ReferencedTextures.Empty();
		GetTextures(ReferencedTextures);
	}

	//make sure all material resources for all platforms have been initialized (and compiled if needed)
	//so that the material resources will be complete for saving.  Don't do this during script compile,
	//since only dummy materials are saved, or when cooking, since compiling material shaders is handled explicitly by the commandlet.
	if (!GIsUCCMake && !GIsCooking)
	{
		CacheResourceShaders(FALSE, TRUE);
	}
	
}

void UMaterial::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(!MaterialResources[MSP_SM3])
	{
		if(!IsTemplate())
		{
			// Construct the material resource.
			MaterialResources[MSP_SM3] = AllocateResource();
		}
	}

	if(MaterialResources[MSP_SM3])
	{
		if(Ar.Ver() >= VER_RENDERING_REFACTOR)
		{
			if (Ar.IsLoading() == TRUE)
			{
				UBOOL bSerialize = TRUE;
				if (Ar.Ver() < VER_DECAL_REFACTOR)
				{
					if (IsA(UDecalMaterial::StaticClass()) == TRUE)
					{
						bSerialize = FALSE;
					}
				}
				
				if (bSerialize == TRUE)
				{
					// Serialize the material resource.
					MaterialResources[MSP_SM3]->Serialize(Ar);
				}
			}
			else
			{
				// Serialize the material resource.
				MaterialResources[MSP_SM3]->Serialize(Ar);
			}
		}
		else
		{
			// Copy over the legacy persistent ID to the FMaterialResource.
			MaterialResources[MSP_SM3]->SetId(PersistentIds[0]);
		}
	}

	if (Ar.Ver() >= VER_MATERIAL_FALLBACKS)
	{
		if (!MaterialResources[MSP_SM2] && !IsTemplate())
		{
			MaterialResources[MSP_SM2] = AllocateResource();
		}

		if (MaterialResources[MSP_SM2])
		{
			MaterialResources[MSP_SM2]->Serialize(Ar);
		}
	}

	if (Ar.IsLoading() && (Ar.Ver() < VER_RENDERING_REFACTOR))
	{
		// check for distortion in material 
		bUsesDistortion = FALSE;
		// can only have distortion with translucent blend modes
		if(IsTranslucentBlendMode((EBlendMode)BlendMode))
		{
			// check for a distortion value
			if( Distortion.Expression ||
				(Distortion.UseConstant && !Distortion.Constant.IsNearlyZero()) )
			{
				bUsesDistortion = TRUE;
			}
		}
	}	

	if ( Ar.IsLoading() && Ar.Ver() < VER_MATERIAL_ISMASKED_FLAG )
	{
		// Check if the material is masked and uses a custom opacity (that's not 1.0f).
		bIsMasked = EBlendMode(BlendMode) == BLEND_Masked && (OpacityMask.Expression || (OpacityMask.UseConstant && OpacityMask.Constant<0.999f));
	}

	if( Ar.IsLoading() && Ar.Ver() < VER_MATERIAL_USES_SCENECOLOR_FLAG )
	{
		// check for scene color usage
		bUsesSceneColor = FALSE;
		for( INT ExpressionIdx=0; ExpressionIdx < Expressions.Num(); ExpressionIdx++ )
		{
			UMaterialExpression * Expr = Expressions(ExpressionIdx);
			UMaterialExpressionSceneTexture* SceneTextureExpr = Cast<UMaterialExpressionSceneTexture>(Expr);
			UMaterialExpressionDestColor* DestColorExpr = Cast<UMaterialExpressionDestColor>(Expr);
			if( SceneTextureExpr || DestColorExpr )
			{
				bUsesSceneColor = TRUE;
				break;
			}
		}
	}
}

void UMaterial::PostLoad()
{
	Super::PostLoad();

	// Propogate the deprecated lighting model variables to the LightingModel enumeration.
	if(Unlit)
	{
		LightingModel = MLM_Unlit;
	}
	else if(NonDirectionalLighting)
	{
		LightingModel = MLM_NonDirectional;
	}
	Unlit = 0;
	NonDirectionalLighting = 0;

	if( GetLinkerVersion() < VER_RENDERING_REFACTOR )
	{
		// Populate expression array for old content.
		if(!Expressions.Num())
		{
			for(TObjectIterator<UMaterialExpression> It;It;++It)
			{
				if(It->GetOuter() == this)
				{
					Expressions.AddItem(*It);
				}
			}
		}
	}

	//if the legacy bUsedWithParticleSystem flag is set, set all of the particle usage flags
	if (bUsedWithParticleSystem)
	{
		bUsedWithParticleSprites = TRUE;
		bUsedWithBeamTrails = TRUE;
		bUsedWithParticleSubUV = TRUE;
		bUsedWithParticleSystem = FALSE;
	}

	// The point of this check is to not compile PC material shaders when cooking non PC platforms (it is a cooking speed operation)
	// Make sure the material resource shaders are cached, unless we are cooking for console,
	// since the commandlet handles materials explicitly. (i.e. compiling the shaders, making sure the uniform expressions (which are stored in the FMaterial) are up to date)
	// when cooking we set the GCookingPlatform so that editoronly/notforconsole properties are correctly NOT loaded and thus not cooked  (which saves memory!)
    // if this check is active then when cooking you will not get the defaultmaterial on the consoles :-(
	//if (!(GCookingTarget & UE3::PLATFORM_Console))
	//{
		CacheResourceShaders();
	//}

	if (FallbackMaterial)
	{
		MaterialResources[MSP_SM2]->ClearDroppedFallbackComponents();
	}
}

void UMaterial::PreEditChange(UProperty* PropertyThatChanged)
{
	Super::PreEditChange(PropertyThatChanged);

	// Flush all pending rendering commands.
	FlushRenderingCommands();
}

void UMaterial::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);

	if( PropertyThatChanged )
	{
		if(PropertyThatChanged->GetName()==TEXT("bIsFallbackMaterial"))
		{
			//don't allow setting bIsFallbackMaterial to true if a fallback material has been specified
			if (FallbackMaterial)
			{
				bIsFallbackMaterial = FALSE;
				return;
			}
		}
		else if(PropertyThatChanged->GetName()==TEXT("FallbackMaterial"))
		{
			//don't allow setting a fallback material if bIsFallbackMaterial has already been specified on this material
			if (bIsFallbackMaterial)
			{
				FallbackMaterial = NULL;
				return;
			}

			//don't allow setting a fallback material that hasn't been marked as a fallback material
			if (FallbackMaterial && !FallbackMaterial->bIsFallbackMaterial)
			{
				const FString ErrorMsg = FString::Printf(*LocalizeUnrealEd("Error_MaterialEditorSelectedMaterialNotFallback"), *FallbackMaterial->GetName());
				appMsgf(AMT_OK, *ErrorMsg);
				FallbackMaterial = NULL;
				return;
			}
		}
		else if(PropertyThatChanged->GetName()==TEXT("bUsedWithFogVolumes") && bUsedWithFogVolumes)
		{
			//check that an emissive input has been supplied, otherwise nothing will be rendered
			if ((!EmissiveColor.UseConstant && EmissiveColor.Expression == NULL))
			{
				const FString ErrorMsg = FString::Printf(*LocalizeUnrealEd("Error_MaterialEditorFogVolumeMaterialNotSetup"));
				appMsgf(AMT_OK, *ErrorMsg);
				bUsedWithFogVolumes = FALSE;
				return;
			}

			//set states that are needed by fog volumes
			BlendMode = BLEND_Additive;
			LightingModel = MLM_Unlit;
		}
	}

	// check for distortion in material 
	bUsesDistortion = FALSE;
	// can only have distortion with translucent blend modes
	if(IsTranslucentBlendMode((EBlendMode)BlendMode))
	{
		// check for a distortion value
		if( Distortion.Expression ||
			(Distortion.UseConstant && !Distortion.Constant.IsNearlyZero()) )
		{
			bUsesDistortion = TRUE;
		}
	}

	// check for scene color usage
	bUsesSceneColor = FALSE;
	for( INT ExpressionIdx=0; ExpressionIdx < Expressions.Num(); ExpressionIdx++ )
	{
		UMaterialExpression * Expr = Expressions(ExpressionIdx);
		UMaterialExpressionSceneTexture* SceneTextureExpr = Cast<UMaterialExpressionSceneTexture>(Expr);
		UMaterialExpressionDestColor* DestColorExpr = Cast<UMaterialExpressionDestColor>(Expr);
		if( SceneTextureExpr || DestColorExpr )
		{
			bUsesSceneColor = TRUE;
			break;
		}
	}

	// Check if the material is masked and uses a custom opacity (that's not 1.0f).
	bIsMasked = EBlendMode(BlendMode) == BLEND_Masked && (OpacityMask.Expression || (OpacityMask.UseConstant && OpacityMask.Constant<0.999f));

	UBOOL bRequiresCompilation = TRUE;
	if( PropertyThatChanged ) 
	{
		// Don't recompile the material if we only changed the PhysMaterial property.
		if( PropertyThatChanged->GetName() == TEXT("PhysMaterial"))
		{
			bRequiresCompilation = FALSE;
		}
		else if (PropertyThatChanged->GetName() == TEXT("FallbackMaterial"))
		{
			bRequiresCompilation = FALSE;
			if (FallbackMaterial)
			{
				MaterialResources[MSP_SM2]->ClearDroppedFallbackComponents();
			}
		}
	}

	if (bRequiresCompilation)
	{
		FlushResourceShaderMaps();
		CacheResourceShaders(TRUE, bIsFallbackMaterial);
		// Ensure that any components with static elements using this material are reattached so changes
		// are propagated to them. The preview material is only applied to the preview mesh component,
		// and that reattach is handled by the material editor.
		if( !bIsPreviewMaterial )
		{
			FGlobalComponentReattachContext RecreateComponents;
		}
	}
} 

void UMaterial::BeginDestroy()
{
	Super::BeginDestroy();

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if(MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex]->ReleaseFence.BeginFence();
		}
	}
}

UBOOL UMaterial::IsReadyForFinishDestroy()
{
	UBOOL bReady = Super::IsReadyForFinishDestroy();

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		bReady = bReady && (!MaterialResources[PlatformIndex] || !MaterialResources[PlatformIndex]->ReleaseFence.GetNumPendingFences());
	}

	return bReady;
}

void UMaterial::FinishDestroy()
{
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if (MaterialResources[PlatformIndex])
		{
			check(!MaterialResources[PlatformIndex]->ReleaseFence.GetNumPendingFences());
			delete MaterialResources[PlatformIndex];
			MaterialResources[PlatformIndex] = NULL;
		}
	}

	delete DefaultMaterialInstances[FALSE];
	delete DefaultMaterialInstances[TRUE];
	Super::FinishDestroy();
}

/**
 * @return		Sum of the size of textures referenced by this material.
 */
INT UMaterial::GetResourceSize()
{
	INT ResourceSize = 0;
	TArray<UTexture*> TheReferencedTextures;
	for ( INT ExpressionIndex= 0 ; ExpressionIndex < Expressions.Num() ; ++ExpressionIndex )
	{
		UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>( Expressions(ExpressionIndex) );
		if ( TextureSample && TextureSample->Texture )
		{
			UTexture* Texture						= TextureSample->Texture;
			const UBOOL bTextureAlreadyConsidered	= TheReferencedTextures.ContainsItem( Texture );
			if ( !bTextureAlreadyConsidered )
			{
				TheReferencedTextures.AddItem( Texture );
				ResourceSize += Texture->GetResourceSize();
			}
		}
	}
	return ResourceSize;
}

/**
 * Used by various commandlets to purge Editor only data from the object.
 * 
 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
 */
void UMaterial::StripData(UE3::EPlatformType TargetPlatform)
{
	Super::StripData(TargetPlatform); 
}

/**
 * Null any material expression references for this material
 */
void UMaterial::RemoveExpressions()
{
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if (MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex]->RemoveExpressions();
		}
	}

	// Remove all non-parameter expressions from the material's expressions array.
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpression* Expression = Expressions(ExpressionIndex);
		
		// Skip the expression if it is a parameter expression
		if(Expression)
		{
			if(Expression->IsA(UMaterialExpressionScalarParameter::StaticClass()))
			{
				continue;
			}
			if(Expression->IsA(UMaterialExpressionVectorParameter::StaticClass()))
			{
				continue;
			}
			if(Expression->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
			{
				continue;
			}
		}

		// Otherwise, remove the expression.
		Expressions.Remove(ExpressionIndex--,1);
	}
	Expressions.Shrink();

	DiffuseColor.Expression = NULL;
	SpecularColor.Expression = NULL;
	SpecularPower.Expression = NULL;
	Normal.Expression = NULL;
	EmissiveColor.Expression = NULL;
	Opacity.Expression = NULL;
	OpacityMask.Expression = NULL;
	Distortion.Expression = NULL;
	CustomLighting.Expression = NULL;
	TwoSidedLightingMask.Expression = NULL;
	TwoSidedLightingColor.Expression = NULL;
}

/**
 * Goes through every material, flushes the specified types and re-initializes the material's shader maps.
 */
void UMaterial::UpdateMaterialShaders(TArray<FShaderType*>& ShaderTypesToFlush, TArray<FVertexFactoryType*>& VFTypesToFlush)
{
	FlushRenderingCommands();
	for( TObjectIterator<UMaterial> It; It; ++It )
	{
		UMaterial* Material = *It;
		if( Material )
		{
			FMaterialResource* MaterialResource = Material->GetMaterialResource();
			if (MaterialResource && MaterialResource->GetShaderMap())
			{
				FMaterialShaderMap* ShaderMap = MaterialResource->GetShaderMap();

				for(INT ShaderTypeIndex = 0; ShaderTypeIndex < ShaderTypesToFlush.Num(); ShaderTypeIndex++)
				{
					ShaderMap->FlushShadersByShaderType(ShaderTypesToFlush(ShaderTypeIndex));
				}
				for(INT VFTypeIndex = 0; VFTypeIndex < VFTypesToFlush.Num(); VFTypeIndex++)
				{
					ShaderMap->FlushShadersByVertexFactoryType(VFTypesToFlush(VFTypeIndex));
				}

				Material->CacheResourceShaders(FALSE, FALSE);
			}
		}
	}
	FGlobalComponentReattachContext RecreateComponents;
}

