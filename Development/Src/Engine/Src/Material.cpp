/*=============================================================================
	UnMaterial.cpp: Shader implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "EngineDecalClasses.h"

IMPLEMENT_CLASS(UMaterial);

FMaterialResource::FMaterialResource(UMaterial* InMaterial):
	Material(InMaterial)
{
}

INT FMaterialResource::CompileProperty(EMaterialShaderPlatform MatPlatform,EMaterialProperty Property,FMaterialCompiler* Compiler) const
{
	const EShaderFrequency ShaderFrequency = GetMaterialPropertyShaderFrequency(Property);
	Compiler->SetMaterialProperty(Property);
	INT SelectionColorIndex = INDEX_NONE;
	if (ShaderFrequency == SF_Pixel)
	{
		SelectionColorIndex = Compiler->ComponentMask(Compiler->VectorParameter(NAME_SelectionColor,FLinearColor::Black),1,1,1,0);
	}
	
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
	case MP_DiffusePower:
		return Material->DiffusePower.Compile(Compiler,1.0f);
	case MP_SpecularColor: return Material->SpecularColor.Compile(Compiler,FColor(0,0,0));
	case MP_SpecularPower: return Material->SpecularPower.Compile(Compiler,15.0f);
	case MP_Normal: return Material->Normal.Compile(Compiler,FVector(0,0,1));
	case MP_CustomLighting: return Material->CustomLighting.Compile(Compiler,FColor(0,0,0));
	case MP_CustomLightingDiffuse: return Material->CustomSkylightDiffuse.Compile(Compiler,FColor(0,0,0));
	case MP_AnisotropicDirection: return Material->AnisotropicDirection.Compile(Compiler,FVector(0,1,0));
	case MP_WorldPositionOffset: return Material->WorldPositionOffset.Compile(Compiler,FVector(0,0,0));
	case MP_WorldDisplacement: return Material->WorldDisplacement.Compile(Compiler,FVector(0,0,0));
	case MP_TessellationFactors: return Material->TessellationFactors.Compile(Compiler,FVector2D(1,1));
	case MP_SubsurfaceInscatteringColor: return Material->SubsurfaceInscatteringColor.Compile(Compiler,FColor(255,255,255));
	case MP_SubsurfaceAbsorptionColor: return Material->SubsurfaceAbsorptionColor.Compile(Compiler,FColor(230,200,200));
	case MP_SubsurfaceScatteringRadius: return Material->SubsurfaceScatteringRadius.Compile(Compiler,0.0f);
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
		const FMaterialResource* MaterialResource = Material->GetMaterialResource();
		if (MaterialResource && MaterialResource->GetShaderMap())
		{
			// Verify that compilation has been finalized, the rendering thread shouldn't be touching it otherwise
			checkSlow(MaterialResource->GetShaderMap()->IsCompilationFinalized());
			// The shader map reference should have been NULL'ed if it did not compile successfully
			checkSlow(MaterialResource->GetShaderMap()->CompiledSuccessfully());
			return MaterialResource;
		}
	
		UMaterial* FallbackMaterial = GEngine->DefaultMaterial;
		if (MaterialResource && MaterialResource->IsUsedWithDecals())
		{
			// Decal materials must fall back to the correct default material
			FallbackMaterial = GEngine->DefaultDecalMaterial;
		}
		// this check is to stop the infinite "retry to compile DefaultMaterial" which can occur when MSP types are mismatched or another similar error state
		check(this != FallbackMaterial->GetRenderProxy(bSelected,bHovered));	
		return FallbackMaterial->GetRenderProxy(bSelected,bHovered)->GetMaterial();
	}
	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		const FMaterialResource* MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			if(ParameterName == NAME_SelectionColor)
			{
				*OutValue = GEngine->UnselectedMaterialColor;
				if( GIsEditor && (Context.View->Family->ShowFlags & SHOW_Selection) )
				{
					if( bSelected )
					{
						*OutValue = GEngine->SelectedMaterialColor;
					}
					else if( bHovered )
					{
						*OutValue = GEngine->DefaultHoveredMaterialColor;
					}
				}
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected,bHovered)->GetVectorValue(ParameterName, OutValue, Context);
		}
	}
	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		const FMaterialResource* MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected,bHovered)->GetScalarValue(ParameterName, OutValue, Context);
		}
	}
	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		const FMaterialResource* MaterialResource = Material->GetMaterialResource();
		if(MaterialResource && MaterialResource->GetShaderMap())
		{
			return FALSE;
		}
		else
		{
			return GEngine->DefaultMaterial->GetRenderProxy(bSelected,bHovered)->GetTextureValue(ParameterName,OutValue,Context);
		}
	}

	virtual FLOAT GetDistanceFieldPenumbraScale() const { return DistanceFieldPenumbraScale; }
	virtual UBOOL IsSelected() const { return bSelected; }
	virtual UBOOL IsHovered() const { return bHovered; }

	// Constructor.
	FDefaultMaterialInstance(UMaterial* InMaterial,UBOOL bInSelected,UBOOL bInHovered):
		Material(InMaterial),
		DistanceFieldPenumbraScale(1.0f),
		bSelected(bInSelected),
		bHovered(bInHovered)
	{}

	/** Called from the game thread to update DistanceFieldPenumbraScale. */
	void UpdateDistanceFieldPenumbraScale(FLOAT NewDistanceFieldPenumbraScale)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateDistanceFieldPenumbraScaleCommand,
			FLOAT*,DistanceFieldPenumbraScale,&DistanceFieldPenumbraScale,
			FLOAT,NewDistanceFieldPenumbraScale,NewDistanceFieldPenumbraScale,
		{
			*DistanceFieldPenumbraScale = NewDistanceFieldPenumbraScale;
		});
	}

private:

	UMaterial* Material;
	FLOAT DistanceFieldPenumbraScale;
	UBOOL bSelected;
	UBOOL bHovered;
};

UMaterial::UMaterial()
{
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		DefaultMaterialInstances[0] = new FDefaultMaterialInstance(this,FALSE,FALSE);
		if(GIsEditor)
		{
			DefaultMaterialInstances[1] = new FDefaultMaterialInstance(this,TRUE,FALSE);
			DefaultMaterialInstances[2] = new FDefaultMaterialInstance(this,FALSE,TRUE);
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
    return !bUseOneLayerDistortion && bUsesDistortion && IsTranslucentBlendMode((EBlendMode)BlendMode);
}

/** @return TRUE if the material uses the scene color texture */
UBOOL UMaterial::UsesSceneColor() const
{
	check(MaterialResources[GCurrentMaterialPlatform]);
	return MaterialResources[GCurrentMaterialPlatform]->GetUsesSceneColor();
}

/**
* Allocates a material resource off the heap to be stored in MaterialResource.
*/
FMaterialResource* UMaterial::AllocateResource()
{
	return new FMaterialResource(this);
}

void UMaterial::GetUsedTextures(TArray<UTexture*> &OutTextures, EMaterialShaderPlatform Platform, UBOOL bAllPlatforms)
{
	OutTextures.Empty();

	if ((appGetPlatformType() & UE3::PLATFORM_WindowsServer) != 0)
	{
		return;
	}

	if (bAllPlatforms)
	{
		for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
		{
			if (MaterialResources[PlatformIndex])
			{
				OutTextures.Append(MaterialResources[PlatformIndex]->GetTextures());
			}
		}
	}
	else
	{
		if (MaterialResources[Platform])
		{
			OutTextures = MaterialResources[Platform]->GetTextures();
		}
	}
}

/**
* Checks whether the specified texture is needed to render the material instance.
* @param CheckTexture	The texture to check.
* @return UBOOL - TRUE if the material uses the specified texture.
*/
UBOOL UMaterial::UsesTexture(const UTexture* CheckTexture)
{
	//Do not care if we're running dedicated server
	if ((appGetPlatformType() & UE3::PLATFORM_WindowsServer) != 0)
	{
		return FALSE;
	}

	TArray<UTexture*> Textures;
	
	GetUsedTextures(Textures, MSP_BASE, TRUE);
	for (INT i = 0; i < Textures.Num(); i++)
	{
		if (Textures(i) == CheckTexture)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Overrides a specific texture (transient)
 *
 * @param InTextureToOverride The texture to override
 * @param OverrideTexture The new texture to use
 */
void UMaterial::OverrideTexture( UTexture* InTextureToOverride, UTexture* OverrideTexture )
{
	// Gather texture references from every material resource
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		UMaterial* Material = GetMaterial((EMaterialShaderPlatform)PlatformIndex);
		if( Material != NULL )
		{
			check(Material->MaterialResources[PlatformIndex]);

			// Iterate over both the 2D textures and cube texture expressions.
			const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >* ExpressionsByType[2] =
			{
				&Material->MaterialResources[PlatformIndex]->GetUniform2DTextureExpressions(),
				&Material->MaterialResources[PlatformIndex]->GetUniformCubeTextureExpressions()
			};
			for(INT TypeIndex = 0;TypeIndex < ARRAY_COUNT(ExpressionsByType);TypeIndex++)
			{
				const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >& Expressions = *ExpressionsByType[TypeIndex];

				// Iterate over each of the material's texture expressions.
				for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
				{
					FMaterialUniformExpressionTexture* Expression = Expressions(ExpressionIndex);

					// Evaluate the expression in terms of this material instance.
					const UBOOL bAllowOverride = FALSE;
					UTexture* Texture = NULL;
					Expression->GetGameThreadTextureValue(this,*Material->MaterialResources[PlatformIndex],Texture,bAllowOverride);

					if( Texture != NULL && Texture == InTextureToOverride )
					{
						// Override this texture!
						Expression->SetTransientOverrideTextureValue( OverrideTexture );
					}
				}
			}
		}
	}
}

/** Gets the value associated with the given usage flag. */
UBOOL UMaterial::GetUsageByFlag(EMaterialUsage Usage) const
{
	UBOOL UsageValue = FALSE;
	switch(Usage)
	{
		case MATUSAGE_SkeletalMesh: UsageValue = bUsedWithSkeletalMesh; break;
		case MATUSAGE_Terrain: UsageValue = bUsedWithTerrain; break;
		case MATUSAGE_Landscape: UsageValue = bUsedWithLandscape; break;
		case MATUSAGE_FracturedMeshes: UsageValue = bUsedWithFracturedMeshes; break;
		case MATUSAGE_ParticleSprites: UsageValue = bUsedWithParticleSprites; break;
		case MATUSAGE_BeamTrails: UsageValue = bUsedWithBeamTrails; break;
		case MATUSAGE_ParticleSubUV: UsageValue = bUsedWithParticleSubUV; break;
		case MATUSAGE_SpeedTree: UsageValue = bUsedWithSpeedTree; break;
		case MATUSAGE_StaticLighting: UsageValue = bUsedWithStaticLighting; break;
		case MATUSAGE_GammaCorrection: UsageValue = bUsedWithGammaCorrection; break;
		case MATUSAGE_LensFlare: UsageValue = bUsedWithLensFlare; break;
		case MATUSAGE_InstancedMeshParticles: UsageValue = bUsedWithInstancedMeshParticles; break;
		case MATUSAGE_FluidSurface: UsageValue = bUsedWithFluidSurfaces; break;
		case MATUSAGE_Decals: UsageValue = bUsedWithDecals; break;
		case MATUSAGE_MaterialEffect: UsageValue = bUsedWithMaterialEffect; break;
		case MATUSAGE_MorphTargets: UsageValue = bUsedWithMorphTargets; break;
		case MATUSAGE_FogVolumes: UsageValue = bUsedWithFogVolumes; break;
		case MATUSAGE_RadialBlur: UsageValue = bUsedWithRadialBlur; break;
		case MATUSAGE_InstancedMeshes: UsageValue = bUsedWithInstancedMeshes; break;
		case MATUSAGE_SplineMesh: UsageValue = bUsedWithSplineMeshes; break;
		case MATUSAGE_ScreenDoorFade: UsageValue = bUsedWithScreenDoorFade; break;
		case MATUSAGE_APEXMesh: UsageValue = bUsedWithAPEXMeshes; break;
		default: appErrorf(TEXT("Unknown material usage: %u"), (INT)Usage);
	};
	return UsageValue;
}

/** Sets the value associated with the given usage flag. */
void UMaterial::SetUsageByFlag(EMaterialUsage Usage, UBOOL NewValue)
{
	switch(Usage)
	{
		case MATUSAGE_SkeletalMesh: bUsedWithSkeletalMesh = NewValue; break;
		case MATUSAGE_Terrain: bUsedWithTerrain = NewValue; break;
		case MATUSAGE_Landscape: bUsedWithLandscape = NewValue; break;
		case MATUSAGE_FracturedMeshes: bUsedWithFracturedMeshes = NewValue; break;
		case MATUSAGE_ParticleSprites: bUsedWithParticleSprites = NewValue; break;
		case MATUSAGE_BeamTrails: bUsedWithBeamTrails = NewValue; break;
		case MATUSAGE_ParticleSubUV: bUsedWithParticleSubUV = NewValue; break;
		case MATUSAGE_SpeedTree: bUsedWithSpeedTree = NewValue; break;
		case MATUSAGE_StaticLighting: bUsedWithStaticLighting = NewValue; break;
		case MATUSAGE_GammaCorrection: bUsedWithGammaCorrection = NewValue; break;
		case MATUSAGE_LensFlare: bUsedWithLensFlare = NewValue; break;
		case MATUSAGE_InstancedMeshParticles: bUsedWithInstancedMeshParticles = NewValue; break;
		case MATUSAGE_FluidSurface: bUsedWithFluidSurfaces = NewValue; break;
		case MATUSAGE_Decals: bUsedWithDecals = NewValue; break;
		case MATUSAGE_MaterialEffect: bUsedWithMaterialEffect = NewValue; break;
		case MATUSAGE_MorphTargets: bUsedWithMorphTargets = NewValue; break;
		case MATUSAGE_FogVolumes: bUsedWithFogVolumes = NewValue; break;
		case MATUSAGE_RadialBlur: bUsedWithRadialBlur = NewValue; break;
		case MATUSAGE_InstancedMeshes: bUsedWithInstancedMeshes = NewValue; break;
		case MATUSAGE_SplineMesh: bUsedWithSplineMeshes = NewValue; break;
		case MATUSAGE_ScreenDoorFade: bUsedWithScreenDoorFade = NewValue; break;
		case MATUSAGE_APEXMesh: bUsedWithAPEXMeshes = NewValue; break;
		default: appErrorf(TEXT("Unknown material usage: %u"), (INT)Usage);
	};
}

/** Gets the name of the given usage flag. */
FString UMaterial::GetUsageName(EMaterialUsage Usage) const
{
	FString UsageName = TEXT("");
	switch(Usage)
	{
		case MATUSAGE_SkeletalMesh: UsageName = TEXT("bUsedWithSkeletalMesh"); break;
		case MATUSAGE_Terrain: UsageName = TEXT("bUsedWithTerrain"); break;
		case MATUSAGE_Landscape: UsageName = TEXT("bUsedWithLandscape"); break;
		case MATUSAGE_FracturedMeshes: UsageName = TEXT("bUsedWithFracturedMeshes"); break;
		case MATUSAGE_ParticleSprites: UsageName = TEXT("bUsedWithParticleSprites"); break;
		case MATUSAGE_BeamTrails: UsageName = TEXT("bUsedWithBeamTrails"); break;
		case MATUSAGE_ParticleSubUV: UsageName = TEXT("bUsedWithParticleSubUV"); break;
		case MATUSAGE_SpeedTree: UsageName = TEXT("bUsedWithSpeedTree"); break;
		case MATUSAGE_StaticLighting: UsageName = TEXT("bUsedWithStaticLighting"); break;
		case MATUSAGE_GammaCorrection: UsageName = TEXT("bUsedWithGammaCorrection"); break;
		case MATUSAGE_LensFlare: UsageName = TEXT("bUsedWithLensFlare"); break;
		case MATUSAGE_InstancedMeshParticles: UsageName = TEXT("bUsedWithInstancedMeshParticles"); break;
		case MATUSAGE_FluidSurface: UsageName = TEXT("bUsedWithFluidSurfaces"); break;
		case MATUSAGE_Decals: UsageName = TEXT("bUsedWithDecals"); break;
		case MATUSAGE_MaterialEffect: UsageName = TEXT("bUsedWithMaterialEffect"); break;
		case MATUSAGE_MorphTargets: UsageName = TEXT("bUsedWithMorphTargets"); break;
		case MATUSAGE_FogVolumes: UsageName = TEXT("bUsedWithFogVolumes"); break;
		case MATUSAGE_RadialBlur: UsageName = TEXT("bUsedWithRadialBlur"); break;
		case MATUSAGE_InstancedMeshes: UsageName = TEXT("bUsedWithInstancedMeshes"); break;
		case MATUSAGE_SplineMesh: UsageName = TEXT("bUsedWithSplineMeshes"); break;
		case MATUSAGE_ScreenDoorFade: UsageName = TEXT("bUsedWithScreenDoorFade"); break;
		case MATUSAGE_APEXMesh: UsageName = TEXT("bUsedWithAPEXMeshes"); break;
		default: appErrorf(TEXT("Unknown material usage: %u"), (INT)Usage);
	};
	return UsageName;
}

/**
 * Checks if the material can be used with the given usage flag.
 * If the flag isn't set in the editor, it will be set and the material will be recompiled with it.
 * @param Usage - The usage flag to check
 * @return UBOOL - TRUE if the material can be used for rendering with the given type.
 */
UBOOL UMaterial::CheckMaterialUsage(EMaterialUsage Usage)
{
	checkSlow(IsInGameThread());
	UBOOL bNeedsRecompile = FALSE;
	return SetMaterialUsage(bNeedsRecompile, Usage);
}

/** Returns TRUE if the given usage flag controls support for a primitive type. */
static UBOOL IsPrimitiveTypeUsageFlag(EMaterialUsage Usage)
{
	return Usage == MATUSAGE_SkeletalMesh
		|| Usage == MATUSAGE_FracturedMeshes
		|| Usage == MATUSAGE_ParticleSprites
		|| Usage == MATUSAGE_BeamTrails
		|| Usage == MATUSAGE_ParticleSubUV
		|| Usage == MATUSAGE_SpeedTree
		|| Usage == MATUSAGE_LensFlare
		|| Usage == MATUSAGE_InstancedMeshParticles
		|| Usage == MATUSAGE_FluidSurface
		|| Usage == MATUSAGE_Decals
		|| Usage == MATUSAGE_MorphTargets
		|| Usage == MATUSAGE_FogVolumes
		|| Usage == MATUSAGE_InstancedMeshes
		|| Usage == MATUSAGE_SplineMesh
		|| Usage == MATUSAGE_APEXMesh;
}

/**
 * Sets the given usage flag.
 * @param bNeedsRecompile - TRUE if the material was recompiled for the usage change
 * @param Usage - The usage flag to set
 * @return UBOOL - TRUE if the material can be used for rendering with the given type.
 */
UBOOL UMaterial::SetMaterialUsage(UBOOL &bNeedsRecompile, EMaterialUsage Usage)
{
	bNeedsRecompile = FALSE;

	// Don't allow materials with decal usage to be applied to meshes requiring a different primitive type,
	// Since decal usage is mutually exclusive with other primitive type usage.
	if (IsPrimitiveTypeUsageFlag(Usage) && Usage != MATUSAGE_Decals && GetUsageByFlag(MATUSAGE_Decals))
	{
		warnf(NAME_Warning, TEXT("Material %s marked bUsedWithDecals being used on a non-decal!  Default material will be used instead."), *GetPathName());
		return FALSE;
	}
	else if (IsPrimitiveTypeUsageFlag(Usage) && Usage != MATUSAGE_FogVolumes && GetUsageByFlag(MATUSAGE_FogVolumes))
	{
		warnf(NAME_Warning, TEXT("Material %s marked bUsedWithFogVolumes being used on a non-fog volume!  Default material will be used instead."), *GetPathName());
		return FALSE;
	}
	// Check that the material has been flagged for use with the given usage flag.
	else if(!GetUsageByFlag(Usage) && !bUsedAsSpecialEngineMaterial)
	{
        // For materials which do not have their bUsedWith____ correctly set the DefaultMaterial<type> should be used in game
        // Leaving this GIsEditor ensures that in game on PC will not look different than on the Consoles as we will not be compiling shaders on the fly
		if( GIsEditor && !GIsGame )
		{
			// If the flag is missing in the editor, set it, and recompile shaders.
			SetUsageByFlag(Usage, TRUE);
			bNeedsRecompile = TRUE;
			CacheResourceShaders(GRHIShaderPlatform);

			// Go through all loaded material instances and recompile their static permutation resources if needed
			// This is necessary since the parent UMaterial stores information about how it should be rendered, (eg bUsesDistortion)
			// but the child can have its own shader map which may not contain all the shaders that the parent's settings indicate that it should.
			// this code is duplicated in NewMaterialEditor.cpp WxMaterialEditor::UpdateOriginalMaterial()
			for (TObjectIterator<UMaterialInstance> It; It; ++It)
			{
				UMaterialInstance* CurrentMaterialInstance = *It;
				UMaterial* BaseMaterial = CurrentMaterialInstance->GetMaterial();
				//only recompile if the instance is a child of the material that got updated
				if (this == BaseMaterial)
				{
					CurrentMaterialInstance->InitStaticPermutation();
				}
			}

			warnf(NAME_Warning, TEXT("Material %s needed to have new flag set %s !"), *GetPathName(), *GetUsageName(Usage));
			// Mark the package dirty so that hopefully it will be saved with the new usage flag.
			MarkPackageDirty();
		}
		else
		{
			warnf(NAME_Warning, TEXT("Material %s missing %s=True!"), *GetPathName(), *GetUsageName(Usage));

			// Return failure if the flag is missing in game, since compiling shaders in game is not supported on some platforms.
			return FALSE;
		}
	}
	return TRUE;
}

/**
* @param	OutParameterNames		Storage array for the parameter names we are returning.
* @param	OutParameterIds			Storage array for the parameter id's we are returning.
*
* @return	Returns a array of vector parameter names used in this material.
*/
template<typename ExpressionType>
void UMaterial::GetAllParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		ExpressionType* Parameter =
			Cast<ExpressionType>(Expressions(ExpressionIndex));

		if(Parameter)
		{
			INT CurrentSize = OutParameterNames.Num();
			OutParameterNames.AddUniqueItem(Parameter->ParameterName);

			if(CurrentSize != OutParameterNames.Num())
			{
				OutParameterIds.AddItem(Parameter->ExpressionGUID);
			}
		}
	}

	check(OutParameterNames.Num() == OutParameterIds.Num());
}

void UMaterial::GetAllVectorParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionVectorParameter>(OutParameterNames, OutParameterIds);
}
void UMaterial::GetAllScalarParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionScalarParameter>(OutParameterNames, OutParameterIds);
}
void UMaterial::GetAllTextureParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionTextureSampleParameter>(OutParameterNames, OutParameterIds);
	// Terrain layers also declare texture parameters
	GetAllParameterNames<UMaterialExpressionTerrainLayerWeight>(OutParameterNames, OutParameterIds);
}

void UMaterial::GetAllFontParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionFontSampleParameter>(OutParameterNames, OutParameterIds);
}

void UMaterial::GetAllStaticSwitchParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionStaticSwitchParameter>(OutParameterNames, OutParameterIds);
}

void UMaterial::GetAllStaticComponentMaskParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionStaticComponentMaskParameter>(OutParameterNames, OutParameterIds);
}

void UMaterial::GetAllNormalParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionTextureSampleParameterNormal>(OutParameterNames, OutParameterIds);
}

void UMaterial::GetAllTerrainLayerWeightParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds)
{
	OutParameterNames.Empty();
	OutParameterIds.Empty();
	GetAllParameterNames<UMaterialExpressionTerrainLayerWeight>(OutParameterNames, OutParameterIds);
}


/**
 * Get the material which this is an instance of.
 * Warning - This is platform dependent!  Do not call GetMaterial(GCurrentMaterialPlatform) and save that reference,
 * as it will be different depending on the current platform.  Instead call GetMaterial(MSP_BASE) to get the base material and save that.
 * When getting the material for rendering/checking usage, GetMaterial(GCurrentMaterialPlatform) is fine.
 *
 * @param Platform - The platform to get material for.
 */
UMaterial* UMaterial::GetMaterial(EMaterialShaderPlatform Platform)
{
	const FMaterialResource* MaterialResource = GetMaterialResource(Platform);

	if(MaterialResource)
	{
		return this;
	}
	else
	{
		return GEngine ? GEngine->DefaultMaterial : NULL;
	}
}

// Get the Description of a Parameter by Parameter Name
UBOOL UMaterial::GetParameterDesc(FName ParameterName, FString& OutDesc)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0; ExpressionIndex < Expressions.Num(); ExpressionIndex++)
	{
		UMaterialExpression* Expression = Expressions(ExpressionIndex);
		// Parameter is a basic Expression Parameter
		if(Expression->IsA(UMaterialExpressionParameter::StaticClass()))
		{
			UMaterialExpressionParameter* Parameter = CastChecked<UMaterialExpressionParameter>(Expression);
			if(Parameter && Parameter->ParameterName == ParameterName)
			{
				OutDesc = Parameter->Desc;
				bSuccess = TRUE;
				break;
			}
		}
		// Parameter is a Texture Sample Parameter
		else if(Expression->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
		{
			UMaterialExpressionTextureSampleParameter* Parameter = CastChecked<UMaterialExpressionTextureSampleParameter>(Expression);
			if(Parameter && Parameter->ParameterName == ParameterName)
			{
				OutDesc = Parameter->Desc;
				bSuccess = TRUE;
				break;
			}
		}
		// Parameter is a Font Sample Parameter
		else if(Expression->IsA(UMaterialExpressionFontSampleParameter::StaticClass()))
		{
			UMaterialExpressionFontSampleParameter* Parameter = CastChecked<UMaterialExpressionFontSampleParameter>(Expression);
			if(Parameter && Parameter->ParameterName == ParameterName)
			{
				OutDesc = Parameter->Desc;
				bSuccess = TRUE;
				break;
			}
		}
	}
	return bSuccess;
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

/**
 * Gets the compression format of the given normal parameter
 *
 * @param	ParameterName	The name of the parameter
 * @param	CompressionSettings	Will contain the values of the parameter if successful
 * @return					True if successful
 */
UBOOL UMaterial::GetNormalParameterValue(FName ParameterName, BYTE& OutCompressionSettings, FGuid &OutExpressionGuid)
{
	UBOOL bSuccess = FALSE;
	for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
	{
		UMaterialExpressionTextureSampleParameterNormal* NormalParameter =
			Cast<UMaterialExpressionTextureSampleParameterNormal>(Expressions(ExpressionIndex));

		if(NormalParameter && NormalParameter->ParameterName == ParameterName && NormalParameter->Texture)
		{
			OutCompressionSettings = NormalParameter->Texture->CompressionSettings;
			OutExpressionGuid = NormalParameter->ExpressionGUID;
			bSuccess = TRUE;
			break;
		}
	}
	return bSuccess;
}


FMaterialRenderProxy* UMaterial::GetRenderProxy(UBOOL Selected,UBOOL Hovered) const
{
	check(!( Selected || Hovered ) || GIsEditor);
	return DefaultMaterialInstances[Selected ? 1 : ( Hovered ? 2 : 0 )];
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
	UBOOL bFlushExistingShaderMaps,
	UBOOL bDebugDump)
{
	UBOOL CompileSucceeded = FALSE;

	check(MaterialResources[MaterialPlatform]);

	//update the static parameter set with the base material's Id
	StaticParameters->BaseMaterialId = MaterialResources[MaterialPlatform]->GetId();

	SetStaticParameterOverrides(StaticParameters);

	if ((appGetPlatformType() & UE3::PLATFORM_Stripped) || UE3_LEAN_AND_MEAN)
	{
		//uniform expressions are guaranteed to be updated since they are always generated during cooking
		CompileSucceeded = StaticPermutation->InitShaderMap(StaticParameters, Platform);
	}
	else
	{
		//material instances with static permutations always need to regenerate uniform expressions, so InitShaderMap() is not used
		CompileSucceeded = StaticPermutation->CacheShaders(StaticParameters, Platform, bFlushExistingShaderMaps, bDebugDump);
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
	check(IsInGameThread());

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
			else
			{
				UMaterialExpressionTextureSampleParameterNormal* NormalParameterExpression =
					Cast<UMaterialExpressionTextureSampleParameterNormal>(Expressions(ExpressionIndex));

				// if it's a normal parameter, setup the override
				if( NormalParameterExpression )
				{
					for(INT NormalParameterIndex = 0;NormalParameterIndex < Permutation->NormalParameters.Num();NormalParameterIndex++)
					{
						const FNormalParameter* InstanceNormalParameter = &Permutation->NormalParameters(NormalParameterIndex);
						if(NormalParameterExpression->ParameterName == InstanceNormalParameter->ParameterName)
						{
							NormalParameterExpression->InstanceOverride = InstanceNormalParameter;
							break;
						}
					}
				}
				else
				{
					UMaterialExpressionTerrainLayerWeight* TerrainLayerWeightParameterExpression =
						Cast<UMaterialExpressionTerrainLayerWeight>(Expressions(ExpressionIndex));

					// if it's a TerrainLayerWeight parameter, setup the override
					if( TerrainLayerWeightParameterExpression )
					{
						for(INT TerrainLayerWeightParameterIndex = 0;TerrainLayerWeightParameterIndex < Permutation->TerrainLayerWeightParameters.Num();TerrainLayerWeightParameterIndex++)
						{
							const FStaticTerrainLayerWeightParameter* InstanceTerrainLayerWeightParameter = &Permutation->TerrainLayerWeightParameters(TerrainLayerWeightParameterIndex);
							if(TerrainLayerWeightParameterExpression->ParameterName == InstanceTerrainLayerWeightParameter->ParameterName)
							{
								TerrainLayerWeightParameterExpression->InstanceOverride = InstanceTerrainLayerWeightParameter;
								break;
							}
						}
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
			else
			{
				UMaterialExpressionTextureSampleParameterNormal* NormalParameterExpression =
					Cast<UMaterialExpressionTextureSampleParameterNormal>(Expressions(ExpressionIndex));

				// if it's a normal parameter, clear the override
				if( NormalParameterExpression )
				{
					NormalParameterExpression->InstanceOverride = NULL;
				}
			}
		}
	}
}

/** Helper functions for text output of properties... */
#ifndef CASE_ENUM_TO_TEXT
#define CASE_ENUM_TO_TEXT(txt) case txt: return TEXT(#txt);
#endif

#ifndef TEXT_TO_ENUM
#define TEXT_TO_ENUM(eVal, txt)		if (appStricmp(TEXT(#eVal), txt) == 0)	return eVal;
#endif

const TCHAR* UMaterial::GetMaterialLightingModelString(EMaterialLightingModel InMaterialLightingModel)
{
	switch (InMaterialLightingModel)
	{
		FOREACH_ENUM_EMATERIALLIGHTINGMODEL(CASE_ENUM_TO_TEXT)
	}
	return TEXT("MLM_Phong");
}

EMaterialLightingModel UMaterial::GetMaterialLightingModelFromString(const TCHAR* InMaterialLightingModelStr)
{
	#define TEXT_TO_LIGHTINGMODEL(m) TEXT_TO_ENUM(m, InMaterialLightingModelStr);
	FOREACH_ENUM_EMATERIALLIGHTINGMODEL(TEXT_TO_LIGHTINGMODEL)
	#undef TEXT_TO_LIGHTINGMODEL
	return MLM_Phong;
}

const TCHAR* UMaterial::GetBlendModeString(EBlendMode InBlendMode)
{
	switch (InBlendMode)
	{
		FOREACH_ENUM_EBLENDMODE(CASE_ENUM_TO_TEXT)
	}
	return TEXT("BLEND_Opaque");
}

EBlendMode UMaterial::GetBlendModeFromString(const TCHAR* InBlendModeStr)
{
	#define TEXT_TO_BLENDMODE(b) TEXT_TO_ENUM(b, InBlendModeStr);
	FOREACH_ENUM_EBLENDMODE(TEXT_TO_BLENDMODE)
	#undef TEXT_TO_BLENDMODE
	return BLEND_Opaque;
}

/**
 * Compiles material resources for the current platform if the shader map for that resource didn't already exist.
 *
 * @param ShaderPlatform - platform to compile for
 * @param bFlushExistingShaderMaps - forces a compile, removes existing shader maps from shader cache.
 * @param bForceAllPlatforms - compile for all platforms, not just the current.
 */
void UMaterial::CacheResourceShaders(EShaderPlatform ShaderPlatform, UBOOL bFlushExistingShaderMaps, UBOOL bForceAllPlatforms)
{
	const EMaterialShaderPlatform RequestedMaterialPlatform = GetMaterialPlatform(ShaderPlatform);
	const EShaderPlatform OtherPlatformsToCompile[] = {SP_PCD3D_SM3};

	check(ARRAY_COUNT(OtherPlatformsToCompile) == MSP_MAX);

	//go through each material resource
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		//allocate it if it hasn't already been
		if(!MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex] = AllocateResource();
		}

		EShaderPlatform CurrentShaderPlatform = ShaderPlatform;
		//if we're not compiling for the same platform that was requested, 
		//lookup what platform we should compile for based on the current material platform
		if (PlatformIndex != RequestedMaterialPlatform)
		{
			CurrentShaderPlatform = OtherPlatformsToCompile[PlatformIndex];
		}

		//don't need to allocate material resources/shaders in dedicated server mode
		if (appGetPlatformType() & UE3::PLATFORM_WindowsServer)
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
			UBOOL bSuccess = FALSE;
			// Force uniform expressions to be regenerated (but allow re-using existing shader maps) if the material resource has legacy uniform expressions
			// This ensures that the uniform expressions will have the correct indices into MaterialResources[PlatformIndex]->UniformExpressionTextures
			if ( bFlushExistingShaderMaps || GetLinkerVersion() < VER_UNIFORMEXPRESSION_POSTLOADFIXUP || MaterialResources[PlatformIndex]->HasLegacyUniformExpressions())
			{
				bSuccess = MaterialResources[PlatformIndex]->CacheShaders(CurrentShaderPlatform, bFlushExistingShaderMaps);
			}
			else
			{
				bSuccess = MaterialResources[PlatformIndex]->InitShaderMap(CurrentShaderPlatform);
			}

			if (!bSuccess)
			{
				warnf(NAME_Warning, TEXT("Failed to compile Material %s for platform %s, Default Material will be used in game."), 
					*GetPathName(), 
					ShaderPlatformToText(CurrentShaderPlatform));

				const TArray<FString>& CompileErrors = MaterialResources[PlatformIndex]->GetCompileErrors();
				for (INT ErrorIndex = 0; ErrorIndex < CompileErrors.Num(); ErrorIndex++)
				{
					warnf(NAME_DevShaders, TEXT("	%s"), *CompileErrors(ErrorIndex));
				}
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
FMaterialResource* UMaterial::GetMaterialResource(EMaterialShaderPlatform Platform)
{
	return MaterialResources[Platform];
}

/**
 * Called before serialization on save to propagate referenced textures. This is not done
 * during content cooking as the material expressions used to retrieve this information will
 * already have been dissociated via RemoveExpressions
 */
void UMaterial::PreSave()
{
	Super::PreSave();

	//make sure all material resources for all platforms have been initialized (and compiled if needed)
	//so that the material resources will be complete for saving.  Don't do this during script compile,
	//since only dummy materials are saved, or when cooking, since compiling material shaders is handled explicitly by the commandlet.
	if (!GIsUCCMake && !GIsCooking)
	{
		CacheResourceShaders(GRHIShaderPlatform, FALSE, TRUE);
	}
}

void UMaterial::AddReferencedObjects(TArray<UObject*>& ObjectArray)
{
	Super::AddReferencedObjects(ObjectArray);

	if(MaterialResources[MSP_SM3])
	{
		MaterialResources[MSP_SM3]->AddReferencedObjects(ObjectArray);
	}
}

void UMaterial::Serialize(FArchive& Ar)
{
	UBOOL bSerializeShaderMap = FALSE;
	UPackage* Package = Cast<UPackage>(GetOutermost());
	if (Package)
	{
		if ((Package->PackageFlags & PKG_ContainsInlinedShaders) != 0)
		{
			bSerializeShaderMap = TRUE;
		}
	}

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
		// Serialize the material resource.
		MaterialResources[MSP_SM3]->Serialize(Ar);
		if (Ar.Ver() < VER_UNIFORM_EXPRESSIONS_IN_SHADER_CACHE)
		{
			// If we are loading a material resource saved before texture references were managed by the material resource,
			// Pass the legacy texture references to the material resource.
			MaterialResources[MSP_SM3]->AddLegacyTextures(ReferencedTextures_DEPRECATED);
			// Empty legacy texture references on load
			ReferencedTextures_DEPRECATED.Empty();
		}
		if (bSerializeShaderMap == TRUE)
		{
			if (Ar.IsSaving())
			{
				SerializeShaderMap(MaterialResources[MSP_SM3]->GetShaderMap(), Ar);
			}
			else
			if (Ar.IsLoading())
			{
				FMaterialShaderMap* SerializedSM = SerializeShaderMap(NULL, Ar);
				MaterialResources[MSP_SM3]->SetShaderMap(SerializedSM);
				if (SerializedSM)
				{
					SerializedSM->BeginInit();
				}
			}
		}
	}

	if (Ar.Ver() < VER_REMOVED_SHADER_MODEL_2)
	{
		FMaterialResource* LegacySM2Resource = NULL;
		if (!IsTemplate())
		{
			LegacySM2Resource = AllocateResource();
		}

		if (LegacySM2Resource)
		{
			LegacySM2Resource->Serialize(Ar);
			delete LegacySM2Resource;
		}
	}

	if (IsFallbackMaterial())
	{
		// Allow legacy fallback materials to be garbage collected
		ClearFlags(RF_Standalone);
		// Objects in startup packages will be part of the root set
		RemoveFromRoot();
	}

	// This fixup should never be needed on consoles, since it will have been done during cooking.
	// Most of the expressions will be removed during cooking as well.
#if !CONSOLE
	if (Ar.IsLoading() && Ar.Ver() < VER_FIXED_SCENECOLOR_USAGE)
	{
		// Fixup old content whose bUsesSceneColor was not set correctly.
		// Mark the material as using scene color based on the presence of material expressions that use it.
		// Note that this is an overly conservative test, just because the expression exists in the material doesn't mean it was actually used in the compiled material.  
		// It could have been disconnected from the graph or culled by a static switch parameter.
		UBOOL bUsesSceneColor = FALSE;
		for( INT ExpressionIdx=0; ExpressionIdx < Expressions.Num(); ExpressionIdx++ )
		{
			UMaterialExpression * Expr = Expressions(ExpressionIdx);
			UMaterialExpressionDepthBiasedBlend* DepthBiasedBlendExpr = Cast<UMaterialExpressionDepthBiasedBlend>(Expr);
			UMaterialExpressionDepthBiasBlend* DepthBiasBlendExpr = Cast<UMaterialExpressionDepthBiasBlend>(Expr);
			UMaterialExpressionSceneTexture* SceneTextureExpr = Cast<UMaterialExpressionSceneTexture>(Expr);
			UMaterialExpressionDestColor* DestColorExpr = Cast<UMaterialExpressionDestColor>(Expr);
			if( DepthBiasedBlendExpr || DepthBiasBlendExpr || SceneTextureExpr || DestColorExpr )
			{
				bUsesSceneColor = TRUE;
				break;
			}
		}

		for (INT i = 0; i < MSP_MAX; i++)
		{
			if (MaterialResources[i])
			{
				MaterialResources[i]->SetUsesSceneColor(bUsesSceneColor);
			}
		}
	}
#endif
}

void UMaterial::PostDuplicate()
{
	// Reset each FMaterial's Id on duplication since it needs to be unique for each material.
	// This will be regenerated when it gets compiled.
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if (MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex]->SetId(FGuid(0,0,0,0));
		}
	}
}

void UMaterial::PostLoad()
{
	Super::PostLoad();

	if (GForceMinimalShaderCompilation)
	{
		// do nothing
	}
	else if (GCookingTarget & (UE3::PLATFORM_Windows|UE3::PLATFORM_WindowsConsole))
	{
		//cache shaders for all PC platforms if we are cooking for PC
		CacheResourceShaders(SP_PCD3D_SM3, FALSE, FALSE);
		if( !ShaderUtils::ShouldForceSM3ShadersOnPC() )
		{
			CacheResourceShaders(SP_PCD3D_SM5, FALSE, FALSE);
		}
	}
	else if (GCookingTarget & UE3::PLATFORM_WindowsServer)
	{
		// do nothing
	}
	else if (GIsCooking)
	{
		//make sure the material's resource shaders are cached
		CacheResourceShaders(GCookingShaderPlatform, FALSE, FALSE);
	}
	else
	{
		//make sure the material's resource shaders are cached
		CacheResourceShaders(GRHIShaderPlatform, FALSE, FALSE);
	}

	if( GIsEditor && !IsTemplate() )
	{
		// Ensure that the ReferencedTextureGuids array is up to date.
		UpdateLightmassTextureTracking();
	}

	for (INT i = 0; i < ARRAY_COUNT(DefaultMaterialInstances); i++)
	{
		if (DefaultMaterialInstances[i])
		{
			DefaultMaterialInstances[i]->UpdateDistanceFieldPenumbraScale(GetDistanceFieldPenumbraScale());
		}
	}
}

void UMaterial::PreEditChange(UProperty* PropertyThatChanged)
{
	Super::PreEditChange(PropertyThatChanged);

	// Flush all pending rendering commands.
	FlushRenderingCommands();
}

void UMaterial::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if( PropertyThatChanged )
	{
		if(PropertyThatChanged->GetName()==TEXT("bUsedWithFogVolumes") && bUsedWithFogVolumes)
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

	// Check if the material is masked and uses a custom opacity (that's not 1.0f).
	bIsMasked = (
		EBlendMode(BlendMode) == BLEND_DitheredTranslucent
	&&	(Opacity.Expression || (Opacity.UseConstant && Opacity.Constant<0.999f))
	) || (
		(EBlendMode(BlendMode) == BLEND_Masked || EBlendMode(BlendMode) == BLEND_SoftMasked)
	&&	(OpacityMask.Expression || (OpacityMask.UseConstant && OpacityMask.Constant<0.999f))
	);

	UBOOL bRequiresCompilation = TRUE;
	if( PropertyThatChanged ) 
	{
		// Don't recompile the material if we only changed the PhysMaterial property.
		if( PropertyThatChanged->GetName() == TEXT("PhysMaterial"))
		{
			bRequiresCompilation = FALSE;
		}
	}

	if (bRequiresCompilation)
	{
		FlushResourceShaderMaps();
		CacheResourceShaders(GRHIShaderPlatform, TRUE, FALSE);

		// Ensure that the ReferencedTextureGuids array is up to date.
		if (GIsEditor)
		{
			UpdateLightmassTextureTracking();
		}

		// Ensure that any components with static elements using this material are reattached so changes
		// are propagated to them. The preview material is only applied to the preview mesh component,
		// and that reattach is handled by the material editor.
		if( !bIsPreviewMaterial )
		{
			FGlobalComponentReattachContext RecreateComponents;
		}
	}
	
	for (INT i = 0; i < ARRAY_COUNT(DefaultMaterialInstances); i++)
	{
		if (DefaultMaterialInstances[i])
		{
			DefaultMaterialInstances[i]->UpdateDistanceFieldPenumbraScale(GetDistanceFieldPenumbraScale());
		}
	}
} 

/**
 * Adds an expression node that represents a parameter to the list of material parameters.
 *
 * @param	Expression	Pointer to the node that is going to be inserted if it's a parameter type.
 */
UBOOL UMaterial::AddExpressionParameter(UMaterialExpression* Expression)
{
	if(!Expression)
	{
		return FALSE;
	}

	UBOOL bRet = FALSE;

	if(Expression->IsA(UMaterialExpressionParameter::StaticClass()))
	{
		UMaterialExpressionParameter *Param = (UMaterialExpressionParameter*)Expression;

		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(Param->ParameterName);

		if(!ExpressionList)
		{
			ExpressionList = &EditorParameters.Set(Param->ParameterName, TArray<UMaterialExpression*>());
		}

		ExpressionList->AddItem(Param);
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
	{
		UMaterialExpressionTextureSampleParameter *Param = (UMaterialExpressionTextureSampleParameter*)Expression;

		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(Param->ParameterName);

		if(!ExpressionList)
		{
			ExpressionList = &EditorParameters.Set(Param->ParameterName, TArray<UMaterialExpression*>());
		}

		ExpressionList->AddItem(Param);
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionFontSampleParameter::StaticClass()))
	{
		UMaterialExpressionFontSampleParameter *Param = (UMaterialExpressionFontSampleParameter*)Expression;

		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(Param->ParameterName);

		if(!ExpressionList)
		{
			ExpressionList = &EditorParameters.Set(Param->ParameterName, TArray<UMaterialExpression*>());
		}

		ExpressionList->AddItem(Param);
		bRet = TRUE;
	}

	return bRet;
}

/**
 * Removes an expression node that represents a parameter from the list of material parameters.
 *
 * @param	Expression	Pointer to the node that is going to be removed if it's a parameter type.
 */
UBOOL UMaterial::RemoveExpressionParameter(UMaterialExpression* Expression)
{
	FName ParmName;

	if(GetExpressionParameterName(Expression, ParmName))
	{
		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(ParmName);

		if(ExpressionList)
		{
			return ExpressionList->RemoveItem(Expression) > 0;
		}
	}

	return FALSE;
}

/**
 * Returns TRUE if the provided expression node is a parameter.
 *
 * @param	Expression	The expression node to inspect.
 */
UBOOL UMaterial::IsParameter(UMaterialExpression* Expression)
{
	UBOOL bRet = FALSE;

	if(Expression->IsA(UMaterialExpressionParameter::StaticClass()))
	{
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
	{
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionFontSampleParameter::StaticClass()))
	{
		bRet = TRUE;
	}

	return bRet;
}

/**
 * Returns TRUE if the provided expression node is a dynamic parameter.
 *
 * @param	Expression	The expression node to inspect.
 */
UBOOL UMaterial::IsDynamicParameter(UMaterialExpression* Expression)
{
	if (Expression->IsA(UMaterialExpressionDynamicParameter::StaticClass()))
	{
		return TRUE;
	}

	return FALSE;
}


/**
 * Iterates through all of the expression nodes in the material and finds any parameters to put in EditorParameters.
 */
void UMaterial::BuildEditorParameterList()
{
	EmptyEditorParameters();

	for(INT MaterialExpressionIndex = 0 ; MaterialExpressionIndex < Expressions.Num() ; ++MaterialExpressionIndex)
	{
		AddExpressionParameter(Expressions( MaterialExpressionIndex ));
	}
}

/**
 * Returns TRUE if the provided expression parameter has duplicates.
 *
 * @param	Expression	The expression parameter to check for duplicates.
 */
UBOOL UMaterial::HasDuplicateParameters(UMaterialExpression* Expression)
{
	FName ExpressionName;

	if(GetExpressionParameterName(Expression, ExpressionName))
	{
		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(ExpressionName);

		if(ExpressionList)
		{
			for(INT ParmIndex = 0; ParmIndex < ExpressionList->Num(); ++ParmIndex)
			{
				UMaterialExpression *CurNode = (*ExpressionList)(ParmIndex);
				if(CurNode != Expression && CurNode->GetClass() == Expression->GetClass())
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/**
 * Returns TRUE if the provided expression dynamic parameter has duplicates.
 *
 * @param	Expression	The expression dynamic parameter to check for duplicates.
 */
UBOOL UMaterial::HasDuplicateDynamicParameters(UMaterialExpression* Expression)
{
	UMaterialExpressionDynamicParameter* DynParam = Cast<UMaterialExpressionDynamicParameter>(Expression);
	if (DynParam)
	{
		for (INT ExpIndex = 0; ExpIndex < Expressions.Num(); ExpIndex++)
		{
			UMaterialExpressionDynamicParameter* CheckDynParam = Cast<UMaterialExpressionDynamicParameter>(Expressions(ExpIndex));
			if (CheckDynParam != Expression)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**
 * Iterates through all of the expression nodes and fixes up changed names on 
 * matching dynamic parameters when a name change occurs.
 *
 * @param	Expression	The expression dynamic parameter.
 */
void UMaterial::UpdateExpressionDynamicParameterNames(UMaterialExpression* Expression)
{
	UMaterialExpressionDynamicParameter* DynParam = Cast<UMaterialExpressionDynamicParameter>(Expression);
	if (DynParam)
	{
		for (INT ExpIndex = 0; ExpIndex < Expressions.Num(); ExpIndex++)
		{
			UMaterialExpressionDynamicParameter* CheckParam = Cast<UMaterialExpressionDynamicParameter>(Expressions(ExpIndex));
			if (CheckParam && (CheckParam != DynParam))
			{
				for (INT NameIndex = 0; NameIndex < 4; NameIndex++)
				{
					CheckParam->ParamNames(NameIndex) = DynParam->ParamNames(NameIndex);
				}
			}
		}
	}
}

/**
 * A parameter with duplicates has to update its peers so that they all have the same value. If this step isn't performed then
 * the expression nodes will not accurately display the final compiled material.
 *
 * @param	Parameter	Pointer to the expression node whose state needs to be propagated.
 */
void UMaterial::PropagateExpressionParameterChanges(UMaterialExpression* Parameter)
{
	FName ParmName;
	UBOOL bRet = GetExpressionParameterName(Parameter, ParmName);

	if(bRet)
	{
		TArray<UMaterialExpression*> *ExpressionList = EditorParameters.Find(ParmName);

		if(ExpressionList && ExpressionList->Num() > 1)
		{
			for(INT Index = 0; Index < ExpressionList->Num(); ++Index)
			{
				CopyExpressionParameters(Parameter, (*ExpressionList)(Index));
			}
		}
		else if(!ExpressionList)
		{
			bRet = FALSE;
		}
	}
}

/**
 * This function removes the expression from the editor parameters list (if it exists) and then re-adds it.
 *
 * @param	Expression	The expression node that represents a parameter that needs updating.
 */
void UMaterial::UpdateExpressionParameterName(UMaterialExpression* Expression)
{
	FName ExpressionName;

	for(TMap<FName, TArray<UMaterialExpression*> >::TIterator Iter(EditorParameters); Iter; ++Iter)
	{
		if(Iter.Value().RemoveItem(Expression) > 0)
		{
			if(Iter.Value().Num() == 0)
			{
				EditorParameters.Remove(Iter.Key());
			}

			AddExpressionParameter(Expression);
			break;
		}
	}
}

/**
 * Gets the name of a parameter.
 *
 * @param	Expression	The expression to retrieve the name from.
 * @param	OutName		The variable that will hold the parameter name.
 * @return	TRUE if the expression is a parameter with a name.
 */
UBOOL UMaterial::GetExpressionParameterName(UMaterialExpression* Expression, FName& OutName)
{
	UBOOL bRet = FALSE;

	if(Expression->IsA(UMaterialExpressionParameter::StaticClass()))
	{
		OutName = ((UMaterialExpressionParameter*)Expression)->ParameterName;
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
	{
		OutName = ((UMaterialExpressionTextureSampleParameter*)Expression)->ParameterName;
		bRet = TRUE;
	}
	else if(Expression->IsA(UMaterialExpressionFontSampleParameter::StaticClass()))
	{
		OutName = ((UMaterialExpressionFontSampleParameter*)Expression)->ParameterName;
		bRet = TRUE;
	}

	return bRet;
}

/**
 * Copies the values of an expression parameter to another expression parameter of the same class.
 *
 * @param	Source			The source parameter.
 * @param	Destination		The destination parameter that will receive Source's values.
 */
UBOOL UMaterial::CopyExpressionParameters(UMaterialExpression* Source, UMaterialExpression* Destination)
{
	if(!Source || !Destination || Source == Destination || Source->GetClass() != Destination->GetClass())
	{
		return FALSE;
	}

	UBOOL bRet = TRUE;

	if(Source->IsA(UMaterialExpressionTextureSampleParameter::StaticClass()))
	{
		UMaterialExpressionTextureSampleParameter *SourceTex = (UMaterialExpressionTextureSampleParameter*)Source;
		UMaterialExpressionTextureSampleParameter *DestTex = (UMaterialExpressionTextureSampleParameter*)Destination;

		DestTex->Modify();
		DestTex->Texture = SourceTex->Texture;
	}
	else if(Source->IsA(UMaterialExpressionVectorParameter::StaticClass()))
	{
		UMaterialExpressionVectorParameter *SourceVec = (UMaterialExpressionVectorParameter*)Source;
		UMaterialExpressionVectorParameter *DestVec = (UMaterialExpressionVectorParameter*)Destination;

		DestVec->Modify();
		DestVec->DefaultValue = SourceVec->DefaultValue;
	}
	else if(Source->IsA(UMaterialExpressionStaticSwitchParameter::StaticClass()))
	{
		UMaterialExpressionStaticSwitchParameter *SourceVec = (UMaterialExpressionStaticSwitchParameter*)Source;
		UMaterialExpressionStaticSwitchParameter *DestVec = (UMaterialExpressionStaticSwitchParameter*)Destination;

		DestVec->Modify();
		DestVec->DefaultValue = SourceVec->DefaultValue;
	}
	else if(Source->IsA(UMaterialExpressionStaticComponentMaskParameter::StaticClass()))
	{
		UMaterialExpressionStaticComponentMaskParameter *SourceVec = (UMaterialExpressionStaticComponentMaskParameter*)Source;
		UMaterialExpressionStaticComponentMaskParameter *DestVec = (UMaterialExpressionStaticComponentMaskParameter*)Destination;

		DestVec->Modify();
		DestVec->DefaultR = SourceVec->DefaultR;
		DestVec->DefaultG = SourceVec->DefaultG;
		DestVec->DefaultB = SourceVec->DefaultB;
		DestVec->DefaultA = SourceVec->DefaultA;
	}
	else if(Source->IsA(UMaterialExpressionScalarParameter::StaticClass()))
	{
		UMaterialExpressionScalarParameter *SourceVec = (UMaterialExpressionScalarParameter*)Source;
		UMaterialExpressionScalarParameter *DestVec = (UMaterialExpressionScalarParameter*)Destination;

		DestVec->Modify();
		DestVec->DefaultValue = SourceVec->DefaultValue;
	}
	else if(Source->IsA(UMaterialExpressionFontSampleParameter::StaticClass()))
	{
		UMaterialExpressionFontSampleParameter *SourceFont = (UMaterialExpressionFontSampleParameter*)Source;
		UMaterialExpressionFontSampleParameter *DestFont = (UMaterialExpressionFontSampleParameter*)Destination;

		DestFont->Modify();
		DestFont->Font = SourceFont->Font;
		DestFont->FontTexturePage = SourceFont->FontTexturePage;
	}
	else
	{
		bRet = FALSE;
	}

	return bRet;
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
			delete MaterialResources[PlatformIndex];
			MaterialResources[PlatformIndex] = NULL;
		}
	}

	delete DefaultMaterialInstances[0];
	delete DefaultMaterialInstances[1];
	delete DefaultMaterialInstances[2];
	Super::FinishDestroy();
}

/**
 * @return		Sum of the size of textures referenced by this material.
 */
INT UMaterial::GetResourceSize()
{
	INT ResourceSize = 0;

	if (!GExclusiveResourceSizeMode)
	{
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
	}

	return ResourceSize;
}

/** === USurface interface === */
/** 
 * Method for retrieving the width of this surface.
 *
 * This implementation returns the maximum width of all textures applied to this material - not exactly accurate, but best approximation.
 *
 * @return	the width of this surface, in pixels.
 */
FLOAT UMaterial::GetSurfaceWidth() const
{
	FLOAT MaxTextureWidth = 0.f;

	TArray<UTexture*> Textures;
	
	const_cast<UMaterial*>(this)->GetUsedTextures(Textures);
	for ( INT TextureIndex = 0; TextureIndex < Textures.Num(); TextureIndex++ )
	{
		UTexture* AppliedTexture = Textures(TextureIndex);
		if ( AppliedTexture != NULL )
		{
			MaxTextureWidth = Max(MaxTextureWidth, AppliedTexture->GetSurfaceWidth());
		}
	}

	if ( Abs(MaxTextureWidth) < DELTA )
	{
		MaxTextureWidth = GetWidth();
	}

	return MaxTextureWidth;
}
/** 
 * Method for retrieving the height of this surface.
 *
 * This implementation returns the maximum height of all textures applied to this material - not exactly accurate, but best approximation.
 *
 * @return	the height of this surface, in pixels.
 */
FLOAT UMaterial::GetSurfaceHeight() const
{
	FLOAT MaxTextureHeight = 0.f;

	TArray<UTexture*> Textures;
	
	const_cast<UMaterial*>(this)->GetUsedTextures(Textures);
	for ( INT TextureIndex = 0; TextureIndex < Textures.Num(); TextureIndex++ )
	{
		UTexture* AppliedTexture = Textures(TextureIndex);
		if ( AppliedTexture != NULL )
		{
			MaxTextureHeight = Max(MaxTextureHeight, AppliedTexture->GetSurfaceHeight());
		}
	}

	if ( Abs(MaxTextureHeight) < DELTA )
	{
		MaxTextureHeight = GetHeight();
	}

	return MaxTextureHeight;
}


/**
 * Null any material expression references for this material
 *
 * @param bRemoveAllExpressions If TRUE, the function will remove every expression and uniform expression from the material and its material resources
 */
void UMaterial::RemoveExpressions(UBOOL bRemoveAllExpressions)
{
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if (MaterialResources[PlatformIndex])
		{
			MaterialResources[PlatformIndex]->RemoveExpressions();

			// also remove the uniform expressions if desired
			if (bRemoveAllExpressions)
			{
				// empty out the texture expressions references from the resource
				MaterialResources[PlatformIndex]->RemoveUniformExpressionTextures();
			}
		}
	}

	// just toss all expressions if requested
	if (bRemoveAllExpressions)
	{
		Expressions.Empty();
	}
	else
	{
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
	}

	DiffuseColor.Expression = NULL;
	DiffusePower.Expression = NULL;
	SpecularColor.Expression = NULL;
	SpecularPower.Expression = NULL;
	Normal.Expression = NULL;
	EmissiveColor.Expression = NULL;
	Opacity.Expression = NULL;
	OpacityMask.Expression = NULL;
	Distortion.Expression = NULL;
	CustomLighting.Expression = NULL;
	CustomSkylightDiffuse.Expression = NULL;
	AnisotropicDirection.Expression = NULL;
	TwoSidedLightingMask.Expression = NULL;
	TwoSidedLightingColor.Expression = NULL;
	WorldPositionOffset.Expression = NULL;
	WorldDisplacement.Expression = NULL;
	TessellationFactors.Expression = NULL;
	SubsurfaceInscatteringColor.Expression = NULL;
	SubsurfaceAbsorptionColor.Expression = NULL;
	SubsurfaceScatteringRadius.Expression = NULL;
}

/**
 * Goes through every material, flushes the specified types and re-initializes the material's shader maps.
 */
void UMaterial::UpdateMaterialShaders(TArray<FShaderType*>& ShaderTypesToFlush, TArray<const FVertexFactoryType*>& VFTypesToFlush)
{
	// Detach all components in the scene while we are modifying materials, they will reattach when this goes out of scope
	FGlobalComponentReattachContext RecreateComponents;
	// Wait until the rendering thread processes the detaches and goes idle so we can modify shader maps
	FlushRenderingCommands();

	// Go through all material shader maps and flush the appropriate shaders
	FMaterialShaderMap::FlushShaderTypes(ShaderTypesToFlush, VFTypesToFlush);

	// There should be no references to the given material shader types at this point
	// If there still are shaders of the given types, they may be reused when we call CacheResourceShaders instead of compiling new shaders
	for (INT ShaderTypeIndex = 0; ShaderTypeIndex < ShaderTypesToFlush.Num(); ShaderTypeIndex++)
	{
		FShaderType* CurrentType = ShaderTypesToFlush(ShaderTypeIndex);
		if (CurrentType->GetMaterialShaderType() || CurrentType->GetMeshMaterialShaderType())
		{
			check(CurrentType->GetNumShaders() == 0);
		}
	}

	// Reinitialize the material shader maps
	for( TObjectIterator<UMaterialInterface> It; It; ++It )
	{
		UMaterialInterface* MaterialInterface = *It;
		UMaterial* BaseMaterial = Cast<UMaterial>(MaterialInterface);
		UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(MaterialInterface);
		if( BaseMaterial )
		{
			BaseMaterial->CacheResourceShaders(GRHIShaderPlatform, FALSE, FALSE);
		}
		else if (MaterialInstance && MaterialInstance->bHasStaticPermutationResource)
		{
			MaterialInstance->CacheResourceShaders(GRHIShaderPlatform, FALSE, FALSE);
		}		
	}

	// Update any FMaterials not belonging to a UMaterialInterface, for example FExpressionPreviews
	// If we did not do this, the editor would crash the next time it tried to render one of those previews
	// And didn't find a shader that had been flushed for the preview's shader map.
	FMaterial::UpdateEditorLoadedMaterialResources();
}

/**
 *	Check if the textures have changed since the last time the material was
 *	serialized for Lightmass... Update the lists while in here.
 *	It will update the LightingGuid if the textures have changed.
 *	NOTE: This will NOT mark the package dirty if they have changed.
 *
 *	@return	UBOOL	TRUE if the textures have changed.
 *					FALSE if they have not.
 */
UBOOL UMaterial::UpdateLightmassTextureTracking()
{
	UBOOL bTexturesHaveChanged = FALSE;
	TArray<UTexture*> UsedTextures;
	
	GetUsedTextures(UsedTextures, MSP_BASE, TRUE);
	if (UsedTextures.Num() != ReferencedTextureGuids.Num())
	{
		bTexturesHaveChanged = TRUE;
		// Just clear out all the guids and the code below will
		// fill them back in...
		ReferencedTextureGuids.Empty(UsedTextures.Num());
		ReferencedTextureGuids.AddZeroed(UsedTextures.Num());
	}
	
	for (INT CheckIdx = 0; CheckIdx < UsedTextures.Num(); CheckIdx++)
	{
		UTexture* Texture = UsedTextures(CheckIdx);
		if (Texture)
		{
			if (ReferencedTextureGuids(CheckIdx) != Texture->LightingGuid)
			{
				ReferencedTextureGuids(CheckIdx) = Texture->LightingGuid;
				bTexturesHaveChanged = TRUE;
			}
		}
		else
		{
			if (ReferencedTextureGuids(CheckIdx) != FGuid(0,0,0,0))
			{
				ReferencedTextureGuids(CheckIdx) = FGuid(0,0,0,0);
				bTexturesHaveChanged = TRUE;
			}
		}
	}

	if ( bTexturesHaveChanged )
	{
		// This will invalidate any cached Lightmass material exports
		LightingGuid = appCreateGuid();
	}

	return bTexturesHaveChanged;
}

/**
*	Get the expression input for the given property
*
*	@param	InProperty				The material property chain to inspect, such as MP_DiffuseColor.
*
*	@return	FExpressionInput*		A pointer to the expression input of the property specified, 
*									or NULL if an invalid property was requested.
*/
FExpressionInput* UMaterial::GetExpressionInputForProperty(EMaterialProperty InProperty)
{
	switch (InProperty)
	{
	case MP_EmissiveColor:
		return &EmissiveColor;
		break;
	case MP_Opacity:
		return &Opacity;
		break;
	case MP_OpacityMask:
		return &OpacityMask;
		break;
	case MP_Distortion:
		return &Distortion;
		break;
	case MP_TwoSidedLightingMask:
		return &TwoSidedLightingMask;
		break;
	case MP_DiffuseColor:
		return &DiffuseColor;
		break;
	case MP_DiffusePower:
		return &DiffusePower;
		break;
	case MP_SpecularColor:
		return &SpecularColor;
		break;
	case MP_SpecularPower:
		return &SpecularPower;
		break;
	case MP_Normal:
		return &Normal;
		break;
	case MP_CustomLighting:
		return &CustomLighting;
		break;
	case MP_CustomLightingDiffuse:
		return &CustomSkylightDiffuse;
		break;
	case MP_AnisotropicDirection:
		return &AnisotropicDirection;
		break;
	case MP_WorldPositionOffset:
		return &WorldPositionOffset;
		break;
	case MP_WorldDisplacement:
		return &WorldDisplacement;
		break;
	case MP_TessellationFactors:
		return &TessellationFactors;
		break;
	case MP_SubsurfaceInscatteringColor:
		return &SubsurfaceInscatteringColor;
		break;
	case MP_SubsurfaceAbsorptionColor:
		return &SubsurfaceInscatteringColor;
		break;
	case MP_SubsurfaceScatteringRadius:
		return &SubsurfaceInscatteringColor;
		break;
	}

	return NULL;
}

/**
 *	Get the expression chain for the given property (ie fill in the given array with all expressions in the chain).
 *
 *	@param	InProperty				The material property chain to inspect, such as MP_DiffuseColor.
 *	@param	OutExpressions			The array to fill in all of the expressions.
 *
 *	@return	UBOOL					TRUE if successful, FALSE if not.
 */
UBOOL UMaterial::GetExpressionsInPropertyChain(EMaterialProperty InProperty, TArray<UMaterialExpression*>& OutExpressions)
{
	OutExpressions.Empty();
	FExpressionInput* StartingExpression = GetExpressionInputForProperty(InProperty);

	if (StartingExpression == NULL)
	{
		// Failed to find the starting expression
		return FALSE;
	}

	TArray<FExpressionInput*> ProcessedInputs;
	if (StartingExpression->Expression)
	{
		ProcessedInputs.AddUniqueItem(StartingExpression);
		RecursiveGetExpressionChain(StartingExpression->Expression, ProcessedInputs, OutExpressions);
	}
	return TRUE;
}

/**
 *	Get all of the textures in the expression chain for the given property (ie fill in the given array with all textures in the chain).
 *
 *	@param	InProperty				The material property chain to inspect, such as MP_DiffuseColor.
 *	@param	OutTextures				The array to fill in all of the textures.
 *	@param	OutTextureParamNames	Optional array to fill in with texture parameter names.
 *
 *	@return	UBOOL			TRUE if successful, FALSE if not.
 */
UBOOL UMaterial::GetTexturesInPropertyChain(EMaterialProperty InProperty, TArray<UTexture*>& OutTextures,  TArray<FName>* OutTextureParamNames)
{
	TArray<UMaterialExpression*> ChainExpressions;
	if (GetExpressionsInPropertyChain(InProperty, ChainExpressions) == TRUE)
	{
		// Extract the texture and texture parameter expressions...
		for (INT ExpressionIdx = 0; ExpressionIdx < ChainExpressions.Num(); ExpressionIdx++)
		{
			UMaterialExpression* MatExp = ChainExpressions(ExpressionIdx);
			if (MatExp != NULL)
			{
				// Is it a texture sample or texture parameter sample?
				UMaterialExpressionTextureSample* TextureSampleExp = Cast<UMaterialExpressionTextureSample>(MatExp);
				if (TextureSampleExp != NULL)
				{
					// Check the default texture...
					if (TextureSampleExp->Texture != NULL)
					{
						TArray<UTexture*> Textures;

						UTextureCube* TextureCube = Cast<UTextureCube>(TextureSampleExp->Texture);
						if (TextureCube != NULL)
						{
							if (TextureCube->FacePosX)
							{
								OutTextures.AddUniqueItem(TextureCube->FacePosX);
							}
							if (TextureCube->FaceNegX)
							{
								OutTextures.AddUniqueItem(TextureCube->FaceNegX);
							}
							if (TextureCube->FacePosY)
							{
								OutTextures.AddUniqueItem(TextureCube->FacePosY);
							}
							if (TextureCube->FaceNegY)
							{
								OutTextures.AddUniqueItem(TextureCube->FaceNegY);
							}
							if (TextureCube->FacePosZ)
							{
								OutTextures.AddUniqueItem(TextureCube->FacePosZ);
							}
							if (TextureCube->FaceNegZ)
							{
								OutTextures.AddUniqueItem(TextureCube->FaceNegZ);
							}
						}
						else
						{
							OutTextures.AddItem(TextureSampleExp->Texture);
						}
					}

					if (OutTextureParamNames != NULL)
					{
						// If the expression is a parameter, add it's name to the texture names array
						UMaterialExpressionTextureSampleParameter* TextureSampleParamExp = Cast<UMaterialExpressionTextureSampleParameter>(MatExp);
						if (TextureSampleParamExp != NULL)
						{
							OutTextureParamNames->AddUniqueItem(TextureSampleParamExp->ParameterName);
						}
					}
				}
			}
		}
	
		return TRUE;
	}

	return FALSE;
}

/**
 *	Recursively retrieve the expressions contained in the chain of the given expression.
 *
 *	@param	InExpression			The expression to start at.
 *	@param	InOutProcessedInputs	An array of processed expression inputs. (To avoid circular loops causing infinite recursion)
 *	@param	OutExpressions			The array to fill in all of the expressions.
 *
 *	@return	UBOOL					TRUE if successful, FALSE if not.
 */
UBOOL UMaterial::RecursiveGetExpressionChain(UMaterialExpression* InExpression, TArray<FExpressionInput*>& InOutProcessedInputs, TArray<UMaterialExpression*>& OutExpressions)
{
	OutExpressions.AddUniqueItem(InExpression);
	TArray<FExpressionInput*> Inputs = InExpression->GetInputs();
	for (INT InputIdx = 0; InputIdx < Inputs.Num(); InputIdx++)
	{
		FExpressionInput* InnerInput = Inputs(InputIdx);
		if (InnerInput != NULL)
		{
			INT DummyIdx;
			if (InOutProcessedInputs.FindItem(InnerInput,DummyIdx) == FALSE)
			{
				InOutProcessedInputs.AddItem(InnerInput);
				if (InnerInput->Expression)
				{
					RecursiveGetExpressionChain(InnerInput->Expression, InOutProcessedInputs, OutExpressions);
				}
			}
		}
	}

	return TRUE;
}


/**
*	Recursively update the bRealtimePreview for each expression based on whether it is connected to something that is time-varying.
*	This is determined based on the result of UMaterialExpression::NeedsRealtimePreview();
*
*	@param	InExpression				The expression to start at.
*	@param	InOutExpressionsToProcess	Array of expressions we still need to process.
*
*/
void UMaterial::RecursiveUpdateRealtimePreview( UMaterialExpression* InExpression, TArray<UMaterialExpression*>& InOutExpressionsToProcess )
{
	// remove ourselves from the list to process
	InOutExpressionsToProcess.RemoveItem(InExpression);

	UBOOL bOldRealtimePreview = InExpression->bRealtimePreview;

	// See if we know ourselves if we need realtime preview or not.
	InExpression->bRealtimePreview = InExpression->NeedsRealtimePreview();

	if( InExpression->bRealtimePreview )
	{
		if( InExpression->bRealtimePreview != bOldRealtimePreview )
		{
			InExpression->bNeedToUpdatePreview = TRUE;
		}

		return;		
	}

	// We need to examine our inputs. If any of them need realtime preview, so do we.
	TArray<FExpressionInput*> Inputs = InExpression->GetInputs();
	for (INT InputIdx = 0; InputIdx < Inputs.Num(); InputIdx++)
	{
		FExpressionInput* InnerInput = Inputs(InputIdx);
		if (InnerInput != NULL && InnerInput->Expression != NULL)
		{
			// See if we still need to process this expression, and if so do that first.
			if (InOutExpressionsToProcess.FindItemIndex(InnerInput->Expression) != INDEX_NONE)
			{
				RecursiveUpdateRealtimePreview(InnerInput->Expression, InOutExpressionsToProcess);
			}

			// If our input expression needed realtime preview, we do too.
			if( InnerInput->Expression->bRealtimePreview )
			{

				InExpression->bRealtimePreview = TRUE;
				if( InExpression->bRealtimePreview != bOldRealtimePreview )
				{
					InExpression->bNeedToUpdatePreview = TRUE;
				}
				return;		
			}
		}
	}

	if( InExpression->bRealtimePreview != bOldRealtimePreview )
	{
		InExpression->bNeedToUpdatePreview = TRUE;
	}
}
