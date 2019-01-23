/**
 *	
 *	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "MaterialInstance.h"

IMPLEMENT_CLASS(UMaterialInstance);

const FMaterial* FMaterialInstanceResource::GetMaterial() const
{
	checkSlow(IsInRenderingThread());
	const FMaterial * InstanceMaterial = Owner->StaticPermutationResources[GCurrentMaterialPlatform];
	//if the instance contains a static permutation resource, use that
	if (InstanceMaterial && InstanceMaterial->GetShaderMap())
	{
		// Verify that compilation has been finalized, the rendering thread shouldn't be touching it otherwise
		checkSlow(InstanceMaterial->GetShaderMap()->IsCompilationFinalized());
		// The shader map reference should have been NULL'ed if it did not compile successfully
		checkSlow(InstanceMaterial->GetShaderMap()->CompiledSuccessfully());
		return InstanceMaterial;
	}
	else if (Owner->bHasStaticPermutationResource)
	{
		//there was an error, use the default material's resource
		return GEngine->DefaultMaterial->GetRenderProxy(bSelected,bHovered)->GetMaterial();
	}
	else
	{
		//use the parent's material resource
		return Parent->GetRenderProxy(bSelected,bHovered)->GetMaterial();
	}
}

/** Called from the game thread to update DistanceFieldPenumbraScale. */
void FMaterialInstanceResource::UpdateDistanceFieldPenumbraScale(FLOAT NewDistanceFieldPenumbraScale)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateDistanceFieldPenumbraScaleCommand,
		FLOAT*,DistanceFieldPenumbraScale,&DistanceFieldPenumbraScale,
		FLOAT,NewDistanceFieldPenumbraScale,NewDistanceFieldPenumbraScale,
	{
		*DistanceFieldPenumbraScale = NewDistanceFieldPenumbraScale;
	});
}

void FMaterialInstanceResource::GameThread_SetParent(UMaterialInterface* InParent)
{
	check(IsInGameThread());

	if( GameThreadParent != InParent )
	{
		// Set the game thread accessible parent.
		UMaterialInterface* OldParent = GameThreadParent;
		GameThreadParent = InParent;

		// Set the rendering thread's parent and instance pointers.
		check(InParent != NULL);
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitMaterialInstanceResource,
			FMaterialInstanceResource*,Resource,this,
			UMaterialInterface*,Parent,InParent,
		{
			Resource->Parent = Parent;
		});

		if( OldParent )
		{
			// make sure that the old parent sticks around until we've set the new parent on FMaterialInstanceResource
			OldParent->ParentRefFence.BeginFence();
		}
	}
}

/**
 * For UMaterials, this will return the flattened texture for platforms that don't 
 * have full material support
 *
 * @return the FTexture object that represents the flattened texture for this material (can be NULL)
 */
FTexture* FMaterialInstanceResource::GetMobileTexture(const INT MobileTextureUnit) const
{
	UTexture* MobileTexture = Owner->GetMobileTexture(MobileTextureUnit);
	return MobileTexture ? MobileTexture->Resource : NULL;
}

UMaterialInstance::UMaterialInstance() :
	bStaticPermutationDirty(FALSE)
{
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		//don't allocate anything for the class default object
		//otherwise child classes will copy the reference to that memory and it will be deleted twice
		for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
		{
			StaticPermutationResources[PlatformIndex] = NULL;
			StaticParameters[PlatformIndex] = NULL;
		}
	}
	else
	{
		for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
		{
			//allocate static parameter sets
			StaticPermutationResources[PlatformIndex] = NULL;
			StaticParameters[PlatformIndex] = new FStaticParameterSet();
		}
	}
}

void UMaterialInstance::InitResources()
{
	// Find the instance's parent.
	UMaterialInterface* SafeParent = NULL;
	if(Parent)
	{
		SafeParent = Parent;
	}

	// Don't use the instance's parent if it has a circular dependency on the instance.
	if(SafeParent && SafeParent->IsDependent(this))
	{
		SafeParent = NULL;
	}

	// If the instance doesn't have a valid parent, use the default material as the parent.
	if(!SafeParent)
	{
		if(GEngine && GEngine->DefaultMaterial)
		{
			SafeParent = GEngine->DefaultMaterial;
		}
		else
		{
			// A material instance was loaded with an invalid GEngine.
			// This is probably because loading the default properties for the GEngine class lead to a material instance being loaded
			// before GEngine has been created.  In this case, we'll just pull the default material config value straight from the INI.
			SafeParent = LoadObject<UMaterialInterface>(NULL,TEXT("engine-ini:Engine.Engine.DefaultMaterialName"),NULL,LOAD_None,NULL);
		}
	}

	check(SafeParent);

	// Set the material instance's parent on its resources.
	for( INT CurResourceIndex = 0; CurResourceIndex < ARRAY_COUNT( Resources ); ++CurResourceIndex )
	{	
		if( Resources[ CurResourceIndex ] != NULL )
		{
			Resources[ CurResourceIndex ]->GameThread_SetParent( SafeParent );
		}
	}
}

/**
 * Get the material which this is an instance of.
 * Warning - This is platform dependent!  Do not call GetMaterial(GCurrentMaterialPlatform) and save that reference,
 * as it will be different depending on the current platform.  Instead call GetMaterial(MSP_BASE) to get the base material and save that.
 * When getting the material for rendering/checking usage, GetMaterial(GCurrentMaterialPlatform) is fine.
 *
 * @param Platform - The platform to get material for.
 */
UMaterial* UMaterialInstance::GetMaterial(EMaterialShaderPlatform Platform)
{
	if(ReentrantFlag)
	{
		return GEngine->DefaultMaterial;
	}

	FMICReentranceGuard	Guard(this);
	if(Parent)
	{
		return Parent->GetMaterial(Platform);
	}
	else
	{
		return GEngine->DefaultMaterial;
	}
}

/** Returns the textures used to render this material instance for the given platform. */
void UMaterialInstance::GetUsedTextures(TArray<UTexture*> &OutTextures, EMaterialShaderPlatform Platform, UBOOL bAllPlatforms)
{
	OutTextures.Empty();

	//Do not care if we're running dedicated server
	if ((appGetPlatformType() & UE3::PLATFORM_WindowsServer) != 0)
	{
		return;
	}

	for (INT PlatformIndex = (bAllPlatforms ? 0 : Platform); (bAllPlatforms ? PlatformIndex < MSP_MAX : PlatformIndex == Platform); PlatformIndex++)
	{
		const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >* ExpressionsByType[2];
		
		const FMaterialResource* SourceMaterialResource = NULL;
		if (bHasStaticPermutationResource)
		{
			check(StaticPermutationResources[Platform]);
			ExpressionsByType[0] = &StaticPermutationResources[Platform]->GetUniform2DTextureExpressions();
			ExpressionsByType[1] = &StaticPermutationResources[Platform]->GetUniformCubeTextureExpressions();
			SourceMaterialResource = StaticPermutationResources[Platform];
		}
		else
		{
			UMaterial* Material = GetMaterial(Platform);
			if(!Material)
			{
				// If the material instance has no material, use the default material.
				GEngine->DefaultMaterial->GetUsedTextures(OutTextures, Platform, bAllPlatforms);
				return;
			}

			check((PlatformIndex == 0) ? (Material->MaterialResources[PlatformIndex] != NULL) : 1);
			if (Material->MaterialResources[PlatformIndex])
			{
				// Iterate over both the 2D textures and cube texture expressions.
				ExpressionsByType[0] = &Material->MaterialResources[Platform]->GetUniform2DTextureExpressions();
				ExpressionsByType[1] = &Material->MaterialResources[Platform]->GetUniformCubeTextureExpressions();
				SourceMaterialResource = Material->MaterialResources[Platform];
			}
		}

		if (SourceMaterialResource != NULL)
		{
			for(INT TypeIndex = 0;TypeIndex < ARRAY_COUNT(ExpressionsByType);TypeIndex++)
			{
				const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >& Expressions = *ExpressionsByType[TypeIndex];

				// Iterate over each of the material's texture expressions.
				for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
				{
					FMaterialUniformExpressionTexture* Expression = Expressions(ExpressionIndex);

					// Evaluate the expression in terms of this material instance.
					UTexture* Texture = NULL;
					Expression->GetGameThreadTextureValue(this,*SourceMaterialResource,Texture);
					OutTextures.AddUniqueItem(Texture);
				}
			}
		}
	}
}

/**
* Checks whether the specified texture is needed to render the material instance.
* @param CheckTexture	The texture to check.
* @return UBOOL - TRUE if the material uses the specified texture.
*/
UBOOL UMaterialInstance::UsesTexture(const UTexture* CheckTexture)
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
void UMaterialInstance::OverrideTexture( UTexture* InTextureToOverride, UTexture* OverrideTexture )
{
	// Gather texture references from every material resource
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		const TArray<TRefCountPtr<FMaterialUniformExpressionTexture> >* ExpressionsByType[2];

		const FMaterialResource* SourceMaterialResource = NULL;
		if (bHasStaticPermutationResource)
		{
			check(StaticPermutationResources[PlatformIndex]);
			// Iterate over both the 2D textures and cube texture expressions.
			ExpressionsByType[0] = &StaticPermutationResources[PlatformIndex]->GetUniform2DTextureExpressions();
			ExpressionsByType[1] = &StaticPermutationResources[PlatformIndex]->GetUniformCubeTextureExpressions();
			SourceMaterialResource = StaticPermutationResources[PlatformIndex];
		}
		else
		{
			UMaterial* Material = GetMaterial((EMaterialShaderPlatform)PlatformIndex);
			if(!Material)
			{
				continue;
			}

			check(Material->MaterialResources[PlatformIndex]);
			// Iterate over both the 2D textures and cube texture expressions.
			ExpressionsByType[0] = &Material->MaterialResources[PlatformIndex]->GetUniform2DTextureExpressions();
			ExpressionsByType[1] = &Material->MaterialResources[PlatformIndex]->GetUniformCubeTextureExpressions();
			SourceMaterialResource = Material->MaterialResources[PlatformIndex];
		}
		
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
				Expression->GetGameThreadTextureValue(this,*SourceMaterialResource,Texture,bAllowOverride);

				if( Texture != NULL && Texture == InTextureToOverride )
				{
					// Override this texture!
					Expression->SetTransientOverrideTextureValue( OverrideTexture );
				}
			}
		}
	}
}

UBOOL UMaterialInstance::CheckMaterialUsage(EMaterialUsage Usage)
{
	checkSlow(IsInGameThread());
	UMaterial* Material = GetMaterial(MSP_BASE);
	if(Material)
	{
		UBOOL bNeedsRecompile = FALSE;
		UBOOL bUsageSetSuccessfully = Material->SetMaterialUsage(bNeedsRecompile, Usage);
		if (bNeedsRecompile)
		{
			CacheResourceShaders(GRHIShaderPlatform, FALSE, FALSE);
			MarkPackageDirty();
		}
		return bUsageSetSuccessfully;
	}
	else
	{
		return FALSE;
	}
}

UBOOL UMaterialInstance::IsDependent(UMaterialInterface* TestDependency)
{
	if(TestDependency == this)
	{
		return TRUE;
	}
	else if(Parent)
	{
		if(ReentrantFlag)
		{
			return TRUE;
		}

		FMICReentranceGuard	Guard(this);
		return Parent->IsDependent(TestDependency);
	}
	else
	{
		return FALSE;
	}
}

/**
* Passes the allocation request up the MIC chain
* @return	The allocated resource
*/
FMaterialResource* UMaterialInstance::AllocateResource()
{
	//pass the allocation request on if the Parent exists
	if (Parent)
	{
		FMaterialResource* NewResource = Parent->AllocateResource();
		if (NewResource)
		{
			return NewResource;
		}
	}

	//otherwise allocate a material resource without specifying a material.
	//the material will be set by AllocateStaticPermutations() before trying to compile this material resource.
	return new FMaterialResource(NULL);
}

/**
 * Gets the static permutation resource if the instance has one
 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
 */
FMaterialResource* UMaterialInstance::GetMaterialResource(EMaterialShaderPlatform Platform)
{
	if(bHasStaticPermutationResource)
	{
		//if there is a static permutation resource, use that
		check(StaticPermutationResources[Platform]);
		return StaticPermutationResources[Platform];
	}

	//there was no static permutation resource
	return Parent ? Parent->GetMaterialResource(Platform) : NULL;
}

FMaterialRenderProxy* UMaterialInstance::GetRenderProxy(UBOOL Selected, UBOOL bHovered) const
{
	check(!( Selected || bHovered ) || GIsEditor);
	return Resources[Selected ? 1 : ( bHovered ? 2 : 0 )];
}

UPhysicalMaterial* UMaterialInstance::GetPhysicalMaterial() const
{
	if(ReentrantFlag)
	{
		return GEngine->DefaultMaterial->GetPhysicalMaterial();
	}

	FMICReentranceGuard	Guard(const_cast<UMaterialInstance*>(this));  // should not need this to determine loop
	if(PhysMaterial)
	{
		return PhysMaterial;
	}
	else if(Parent)
	{
		// If no physical material has been associated with this instance, simply use the parent's physical material.
		return Parent->GetPhysicalMaterial();
	}
	else
	{
		return NULL;
	}
}

/**
 * Returns the lookup texture to be used in the physical material mask.  Tries to get the parents lookup texture if not overridden here. 
 */
UTexture2D* UMaterialInstance::GetPhysicalMaterialMaskTexture() const
{
	if(ReentrantFlag)
	{
		return NULL;
	}

	FMICReentranceGuard	Guard(const_cast<UMaterialInstance*>(this)); 

	if( PhysMaterialMask )
	{
		return PhysMaterialMask;
	}
	else if( Parent )
	{
		return Parent->GetPhysicalMaterialMaskTexture();
	}
	else
	{
		return NULL;
	}
}
	
/** 
 * Returns the UV channel that should be used to look up physical material mask information. Tries to get the parents UV channel if not present here.
 */
INT UMaterialInstance::GetPhysMaterialMaskUVChannel() const
{
	if(ReentrantFlag)
	{
		return -1;
	}

	FMICReentranceGuard	Guard(const_cast<UMaterialInstance*>(this)); 

	if( PhysMaterialMaskUVChannel != -1 )
	{
		return PhysMaterialMaskUVChannel;
	}
	else if( Parent )
	{
		return Parent->GetPhysMaterialMaskUVChannel();
	}
	else
	{
		return -1;
	}
}

/**
 * Returns the black physical material to be used in the physical material mask.  Tries to get the parents black phys mat if not overridden here. 
 */
UPhysicalMaterial* UMaterialInstance::GetBlackPhysicalMaterial() const
{
	if(ReentrantFlag)
	{
		return NULL;
	}

	FMICReentranceGuard	Guard(const_cast<UMaterialInstance*>(this)); 

	if( BlackPhysicalMaterial )
	{
		return BlackPhysicalMaterial;
	}
	else if( Parent )
	{
		return Parent->GetBlackPhysicalMaterial();
	}
	else
	{
		return NULL;
	}
}

/**
 * Returns the white physical material to be used in the physical material mask.  Tries to get the parents white phys mat if not overridden here. 
 */
UPhysicalMaterial* UMaterialInstance::GetWhitePhysicalMaterial() const
{
	if(ReentrantFlag)
	{
		return NULL;
	}

	FMICReentranceGuard	Guard(const_cast<UMaterialInstance*>(this)); 

	if( WhitePhysicalMaterial )
	{
		return WhitePhysicalMaterial;
	}
	else if( Parent )
	{
		return Parent->GetWhitePhysicalMaterial();
	}
	else
	{
		return NULL;
	}
}

/**
* Makes a copy of all the instance's inherited and overridden static parameters
*
* @param StaticParameters - The set of static parameters to fill, must be empty
*/
void UMaterialInstance::GetStaticParameterValues(FStaticParameterSet* InStaticParameters)
{
	check(IsInGameThread());
	check(InStaticParameters && InStaticParameters->IsEmpty());
	if (Parent)
	{
		UMaterial * ParentMaterial = Parent->GetMaterial(MSP_BASE);
		TArray<FName> ParameterNames;
		TArray<FGuid> Guids;

		// Static Switch Parameters
		ParentMaterial->GetAllStaticSwitchParameterNames(ParameterNames, Guids);
		InStaticParameters->StaticSwitchParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FStaticSwitchParameter& ParentParameter = InStaticParameters->StaticSwitchParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UBOOL Value = FALSE;
			FGuid ExpressionId = Guids(ParameterIdx);

			ParentParameter.bOverride = FALSE;
			ParentParameter.ParameterName = ParameterName;

			//get the settings from the parent in the MIC chain
			if(Parent->GetStaticSwitchParameterValue(ParameterName, Value, ExpressionId))
			{
				ParentParameter.Value = Value;
			}
			ParentParameter.ExpressionGUID = ExpressionId;

			//if the SourceInstance is overriding this parameter, use its settings
			for(INT ParameterIdx = 0; ParameterIdx < StaticParameters[MSP_SM3]->StaticSwitchParameters.Num(); ParameterIdx++)
			{
				const FStaticSwitchParameter &StaticSwitchParam = StaticParameters[MSP_SM3]->StaticSwitchParameters(ParameterIdx);

				if(ParameterName == StaticSwitchParam.ParameterName)
				{
					ParentParameter.bOverride = StaticSwitchParam.bOverride;
					if (StaticSwitchParam.bOverride)
					{
						ParentParameter.Value = StaticSwitchParam.Value;
					}
				}
			}
		}

		// Static Component Mask Parameters
		ParentMaterial->GetAllStaticComponentMaskParameterNames(ParameterNames, Guids);
		InStaticParameters->StaticComponentMaskParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FStaticComponentMaskParameter& ParentParameter = InStaticParameters->StaticComponentMaskParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UBOOL R = FALSE;
			UBOOL G = FALSE;
			UBOOL B = FALSE;
			UBOOL A = FALSE;
			FGuid ExpressionId = Guids(ParameterIdx);

			ParentParameter.bOverride = FALSE;
			ParentParameter.ParameterName = ParameterName;

			//get the settings from the parent in the MIC chain
			if(Parent->GetStaticComponentMaskParameterValue(ParameterName, R, G, B, A, ExpressionId))
			{
				ParentParameter.R = R;
				ParentParameter.G = G;
				ParentParameter.B = B;
				ParentParameter.A = A;
			}
			ParentParameter.ExpressionGUID = ExpressionId;

			//if the SourceInstance is overriding this parameter, use its settings
			for(INT ParameterIdx = 0; ParameterIdx < StaticParameters[MSP_SM3]->StaticComponentMaskParameters.Num(); ParameterIdx++)
			{
				const FStaticComponentMaskParameter &StaticComponentMaskParam = StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ParameterIdx);

				if(ParameterName == StaticComponentMaskParam.ParameterName)
				{
					ParentParameter.bOverride = StaticComponentMaskParam.bOverride;
					if (StaticComponentMaskParam.bOverride)
					{
						ParentParameter.R = StaticComponentMaskParam.R;
						ParentParameter.G = StaticComponentMaskParam.G;
						ParentParameter.B = StaticComponentMaskParam.B;
						ParentParameter.A = StaticComponentMaskParam.A;
					}
				}
			}
		}

		// Normal Parameters
		ParentMaterial->GetAllNormalParameterNames(ParameterNames, Guids);
		InStaticParameters->NormalParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FNormalParameter& ParentParameter = InStaticParameters->NormalParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			BYTE CompressionSettings = TC_Normalmap;
			FGuid ExpressionId = Guids(ParameterIdx);

			ParentParameter.bOverride = FALSE;
			ParentParameter.ParameterName = ParameterName;

			//get the settings from the parent in the MIC chain
			if(Parent->GetNormalParameterValue(ParameterName, CompressionSettings, ExpressionId))
			{
				ParentParameter.CompressionSettings = CompressionSettings;
			}
			ParentParameter.ExpressionGUID = ExpressionId;

			//if the SourceInstance is overriding this parameter, use its settings
			for(INT ParameterIdx = 0; ParameterIdx < StaticParameters[MSP_SM3]->NormalParameters.Num(); ParameterIdx++)
			{
				const FNormalParameter &NormalParam = StaticParameters[MSP_SM3]->NormalParameters(ParameterIdx);

				if(ParameterName == NormalParam.ParameterName)
				{
					ParentParameter.bOverride = NormalParam.bOverride;
					if (NormalParam.bOverride)
					{
						ParentParameter.CompressionSettings = NormalParam.CompressionSettings;
					}
				}
			}
		}

		// TerrainLayerWeight Parameters
		ParentMaterial->GetAllTerrainLayerWeightParameterNames(ParameterNames, Guids);
		InStaticParameters->TerrainLayerWeightParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FStaticTerrainLayerWeightParameter& ParentParameter = InStaticParameters->TerrainLayerWeightParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			FGuid ExpressionId = Guids(ParameterIdx);

			ParentParameter.bOverride = FALSE;
			ParentParameter.ParameterName = ParameterName;
			ParentParameter.WeightmapIndex = INDEX_NONE;

			//if the SourceInstance is overriding this parameter, use its settings
			for(INT ParameterIdx = 0; ParameterIdx < StaticParameters[MSP_SM3]->TerrainLayerWeightParameters.Num(); ParameterIdx++)
			{
				const FStaticTerrainLayerWeightParameter &TerrainLayerWeightParam = StaticParameters[MSP_SM3]->TerrainLayerWeightParameters(ParameterIdx);

				if(ParameterName == TerrainLayerWeightParam.ParameterName)
				{
					ParentParameter.bOverride = TerrainLayerWeightParam.bOverride;
					if (TerrainLayerWeightParam.bOverride)
					{
						ParentParameter.WeightmapIndex = TerrainLayerWeightParam.WeightmapIndex;
					}
				}
			}
		}
	}
}

/**
* Sets the instance's static parameters and marks it dirty if appropriate. 
*
* @param	EditorParameters	The new static parameters.  If the set does not contain any static parameters,
*								the static permutation resource will be released.
* @return		TRUE if the static permutation resource has been marked dirty
*/
UBOOL UMaterialInstance::SetStaticParameterValues(const FStaticParameterSet* EditorParameters)
{
	check(IsInGameThread());

	//mark the static permutation resource dirty if necessary, so it will be recompiled on PostEditChange()
	bStaticPermutationDirty = StaticParameters[MSP_SM3]->ShouldMarkDirty(EditorParameters);

	if (Parent)
	{
		const EMaterialShaderPlatform CurrentMaterialPlatform = GetMaterialPlatform(GRHIShaderPlatform);
		UMaterial * ParentMaterial = Parent->GetMaterial(CurrentMaterialPlatform);
		const FMaterial * ParentMaterialResource = ParentMaterial->GetMaterialResource(CurrentMaterialPlatform);

		//check if the BaseMaterialId of the appropriate static parameter set still matches the Id 
		//of the material resource that the base UMaterial owns.  If they are different, the base UMaterial
		//has been edited since the static permutation was compiled.
		if (ParentMaterialResource->GetId() != StaticParameters[CurrentMaterialPlatform]->BaseMaterialId 
			&& !StaticParameters[CurrentMaterialPlatform]->IsEmpty())
		{
			bStaticPermutationDirty = TRUE;
		}

		if (bStaticPermutationDirty)
		{
			//copy the new static parameter set
			//no need to preserve StaticParameters.BaseMaterialId since this will be regenerated
			//now that the static resource has been marked dirty
			for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
			{
				*StaticParameters[PlatformIndex] = *EditorParameters;
			}
		}
	}

	return bStaticPermutationDirty;
}

/**
* Checks if any of the static parameter values are outdated based on what they reference (eg a normalmap has changed format)
*
* @param	EditorParameters	The new static parameters. 
*/
void UMaterialInstance::CheckStaticParameterValues(FStaticParameterSet* EditorParameters)
{
}

/**
* Compiles the static permutation resource if the base material has changed and updates dirty states
*/
void UMaterialInstance::UpdateStaticPermutation()
{
	if (bStaticPermutationDirty && Parent)
	{
		//a static permutation resource must exist if there are static parameters
		bHasStaticPermutationResource = !StaticParameters[MSP_SM3]->IsEmpty();
		CacheResourceShaders(GRHIShaderPlatform, TRUE, FALSE);
		if (bHasStaticPermutationResource)
		{
			FGlobalComponentReattachContext RecreateComponents;	
		}
		bStaticPermutationDirty = FALSE;
	}
}

/**
* Updates static parameters and recompiles the static permutation resource if necessary
*/
void UMaterialInstance::InitStaticPermutation()
{
	if (Parent && bHasStaticPermutationResource)
	{
		//update the static parameters, since they may have changed in the parent material
		FStaticParameterSet UpdatedStaticParameters;
		GetStaticParameterValues(&UpdatedStaticParameters);
		//update any normal parameters that have that have CompressionSettings that no longer match their referenced texture.
		CheckStaticParameterValues(&UpdatedStaticParameters);
		SetStaticParameterValues(&UpdatedStaticParameters);
	}

	if (GForceMinimalShaderCompilation)
	{
		// do nothing
	}
	else if (GCookingTarget & (UE3::PLATFORM_Windows|UE3::PLATFORM_WindowsConsole))
	{
		//If we are cooking for PC, cache shaders for all PC platforms.
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
		//Make sure the material resource shaders are cached.
		CacheResourceShaders(GCookingShaderPlatform, FALSE, FALSE);
	}
	else
	{
		//Make sure the material resource shaders are cached.
		CacheResourceShaders(GRHIShaderPlatform, FALSE, FALSE);
	}
}

/**
* Compiles material resources for the given platform if the shader map for that resource didn't already exist.
*
* @param ShaderPlatform - the platform to compile for.
* @param bFlushExistingShaderMaps - forces a compile, removes existing shader maps from shader cache.
* @param bForceAllPlatforms - compile for all platforms, not just the current.
*/
void UMaterialInstance::CacheResourceShaders(EShaderPlatform ShaderPlatform, UBOOL bFlushExistingShaderMaps, UBOOL bForceAllPlatforms, UBOOL bDebugDump)
{
	// Fix-up the parent lighting guid if it has changed...
	if (Parent && (Parent->LightingGuid != ParentLightingGuid))
	{
		LightingGuid = appCreateGuid();
		ParentLightingGuid = Parent ? Parent->LightingGuid : FGuid(0,0,0,0);
	}

	if (bHasStaticPermutationResource)
	{
		//make sure all material resources are allocated and have their Material member updated
		AllocateStaticPermutations();
		const EMaterialShaderPlatform RequestedMaterialPlatform = GetMaterialPlatform(ShaderPlatform);
		const EShaderPlatform OtherPlatformsToCompile[] = {SP_PCD3D_SM3};

		if (appGetPlatformType() & UE3::PLATFORM_WindowsServer)
		{	
			//Only allocate shader resources if not running dedicated server
			return;
		}

		//go through each material resource
		for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
		{
			//mark the package dirty if the material resource has never been compiled
			if (GIsEditor && !StaticPermutationResources[PlatformIndex]->GetId().IsValid()
				|| bFlushExistingShaderMaps)
			{
				MarkPackageDirty();
			}

			//only compile if the current resource matches the running platform, unless all platforms are forced
			if (bForceAllPlatforms || PlatformIndex == RequestedMaterialPlatform)
			{
				EShaderPlatform CurrentShaderPlatform = ShaderPlatform;
				//if we're not compiling for the same platform that was requested, 
				//lookup what platform we should compile for based on the current material platform
				if (PlatformIndex != RequestedMaterialPlatform)
				{
					CurrentShaderPlatform = OtherPlatformsToCompile[PlatformIndex];
				}

				const UBOOL bSuccess = Parent->CompileStaticPermutation(
					StaticParameters[PlatformIndex], 
					StaticPermutationResources[PlatformIndex], 
					CurrentShaderPlatform, 
					(EMaterialShaderPlatform)PlatformIndex,
					bFlushExistingShaderMaps,
					bDebugDump);

				if (bSuccess)
				{
					// After the compile, the material resource has references to all textures used by uniform expressions
					// Add references to textures used for rendering that might be different from what the uniform expressions reference (FMaterialUniformExpressionTextureParameter)

					TArray<UTexture*> Textures;
					
					GetUsedTextures(Textures, (EMaterialShaderPlatform)PlatformIndex, FALSE);
					StaticPermutationResources[PlatformIndex]->AddReferencedTextures(Textures);
				}

				if (!bSuccess)
				{
					const UMaterial* BaseMaterial = GetMaterial((EMaterialShaderPlatform)PlatformIndex);
					warnf(NAME_Warning, TEXT("Failed to compile Material Instance %s with Base %s for platform %s, Default Material will be used in game."), 
						*GetPathName(), 
						BaseMaterial ? *BaseMaterial->GetName() : TEXT("Null"), 
						ShaderPlatformToText(CurrentShaderPlatform));

					const TArray<FString>& CompileErrors = StaticPermutationResources[PlatformIndex]->GetCompileErrors();
					for (INT ErrorIndex = 0; ErrorIndex < CompileErrors.Num(); ErrorIndex++)
					{
						warnf(NAME_DevShaders, TEXT("	%s"), *CompileErrors(ErrorIndex));
					}
				}

				bStaticPermutationDirty = FALSE;
			}
		}
	}
	else
	{
		ReleaseStaticPermutations();
	}
}

/**
* Passes the compile request up the MIC chain
*
* @param StaticParameters - The set of static parameters to compile for
* @param StaticPermutation - The resource to compile
* @param Platform - The platform to compile for
* @param MaterialPlatform - The material platform to compile for
* @param bFlushExistingShaderMaps - Indicates that existing shader maps should be discarded
* @return TRUE if compilation was successful or not necessary
*/
UBOOL UMaterialInstance::CompileStaticPermutation(
	FStaticParameterSet* Permutation, 
	FMaterialResource* StaticPermutation, 
	EShaderPlatform Platform, 
	EMaterialShaderPlatform MaterialPlatform,
	UBOOL bFlushExistingShaderMaps,
	UBOOL bDebugDump)
{
	UBOOL bCompileSucceeded = FALSE;
	if (Parent)
	{
		bCompileSucceeded = Parent->CompileStaticPermutation(Permutation, StaticPermutation, Platform, MaterialPlatform, bFlushExistingShaderMaps, bDebugDump);
	}
	return bCompileSucceeded;
}

/**
* Allocates the static permutation resources for all platforms if they haven't been already.
* Also updates the material resource's Material member as it may have changed.
*/
void UMaterialInstance::AllocateStaticPermutations()
{
	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		//allocate the material resource if it hasn't already been
		if (!StaticPermutationResources[PlatformIndex])
		{
			StaticPermutationResources[PlatformIndex] = Parent->AllocateResource();
		}

		if (Parent)
		{
			UMaterial* ParentMaterial = Parent->GetMaterial((EMaterialShaderPlatform)PlatformIndex);
			StaticPermutationResources[PlatformIndex]->SetMaterial(ParentMaterial);
		}
	}
}

/**
* Releases the static permutation resource if it exists, in a thread safe way
*/
void UMaterialInstance::ReleaseStaticPermutations()
{
	for (INT MaterialPlatformIndex = 0; MaterialPlatformIndex < MSP_MAX; MaterialPlatformIndex++)
	{
		if (StaticPermutationResources[MaterialPlatformIndex])
		{
			StaticPermutationResources[MaterialPlatformIndex]->ReleaseFence.BeginFence();
			while (StaticPermutationResources[MaterialPlatformIndex]->ReleaseFence.GetNumPendingFences())
			{
				appSleep(0);
			}
			delete StaticPermutationResources[MaterialPlatformIndex];
			StaticPermutationResources[MaterialPlatformIndex] = NULL;
		}
	}
}



/**
* Gets the value of the given static switch parameter.  If it is not found in this instance then
*		the request is forwarded up the MIC chain.
*
* @param	ParameterName	The name of the static switch parameter
* @param	OutValue		Will contain the value of the parameter if successful
* @return					True if successful
*/
UBOOL UMaterialInstance::GetStaticSwitchParameterValue(FName ParameterName, UBOOL &OutValue,FGuid &OutExpressionGuid)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	UBOOL* Value = NULL;
	FGuid* Guid = NULL;
	for (INT ValueIndex = 0;ValueIndex < StaticParameters[MSP_SM3]->StaticSwitchParameters.Num();ValueIndex++)
	{
		if (StaticParameters[MSP_SM3]->StaticSwitchParameters(ValueIndex).ParameterName == ParameterName)
		{
			Value = &StaticParameters[MSP_SM3]->StaticSwitchParameters(ValueIndex).Value;
			Guid = &StaticParameters[MSP_SM3]->StaticSwitchParameters(ValueIndex).ExpressionGUID;
			break;
		}
	}
	if(Value)
	{
		OutValue = *Value;
		OutExpressionGuid = *Guid;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetStaticSwitchParameterValue(ParameterName,OutValue,OutExpressionGuid);
	}
	else
	{
		return FALSE;
	}
}

/**
* Gets the value of the given static component mask parameter. If it is not found in this instance then
*		the request is forwarded up the MIC chain.
*
* @param	ParameterName	The name of the parameter
* @param	R, G, B, A		Will contain the values of the parameter if successful
* @return					True if successful
*/
UBOOL UMaterialInstance::GetStaticComponentMaskParameterValue(FName ParameterName, UBOOL &OutR, UBOOL &OutG, UBOOL &OutB, UBOOL &OutA, FGuid &OutExpressionGuid)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	UBOOL* R = NULL;
	UBOOL* G = NULL;
	UBOOL* B = NULL;
	UBOOL* A = NULL;
	FGuid* ExpressionId = NULL;
	for (INT ValueIndex = 0;ValueIndex < StaticParameters[MSP_SM3]->StaticComponentMaskParameters.Num();ValueIndex++)
	{
		if (StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).ParameterName == ParameterName)
		{
			R = &StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).R;
			G = &StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).G;
			B = &StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).B;
			A = &StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).A;
			ExpressionId = &StaticParameters[MSP_SM3]->StaticComponentMaskParameters(ValueIndex).ExpressionGUID;
			break;
		}
	}
	if(R && G && B && A)
	{
		OutR = *R;
		OutG = *G;
		OutB = *B;
		OutA = *A;
		OutExpressionGuid = *ExpressionId;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetStaticComponentMaskParameterValue(ParameterName, OutR, OutG, OutB, OutA, OutExpressionGuid);
	}
	else
	{
		return FALSE;
	}
}


/**
* Gets the compression format of the given normal parameter
*
* @param	ParameterName	The name of the parameter
* @param	CompressionSettings	Will contain the values of the parameter if successful
* @return					True if successful
*/
UBOOL UMaterialInstance::GetNormalParameterValue(FName ParameterName, BYTE& OutCompressionSettings, FGuid &OutExpressionGuid)
{
	if(ReentrantFlag)
	{
		return FALSE;
	}

	BYTE* CompressionSettings = NULL;

	FGuid* ExpressionId = NULL;
	for (INT ValueIndex = 0;ValueIndex < StaticParameters[MSP_SM3]->NormalParameters.Num();ValueIndex++)
	{
		if (StaticParameters[MSP_SM3]->NormalParameters(ValueIndex).ParameterName == ParameterName)
		{
			CompressionSettings = &StaticParameters[MSP_SM3]->NormalParameters(ValueIndex).CompressionSettings;
			ExpressionId = &StaticParameters[MSP_SM3]->NormalParameters(ValueIndex).ExpressionGUID;
			break;
		}
	}
	if(CompressionSettings)
	{
		OutCompressionSettings = *CompressionSettings;
		OutExpressionGuid = *ExpressionId;
		return TRUE;
	}
	else if(Parent)
	{
		FMICReentranceGuard	Guard(this);
		return Parent->GetNormalParameterValue(ParameterName, OutCompressionSettings, OutExpressionGuid);
	}
	else
	{
		return FALSE;
	}
}



/** === USurface interface === */
/** 
 * Method for retrieving the width of this surface.
 *
 * This implementation returns the maximum width of all textures applied to this material - not exactly accurate, but best approximation.
 *
 * @return	the width of this surface, in pixels.
 */
FLOAT UMaterialInstance::GetSurfaceWidth() const
{
	FLOAT MaxTextureWidth = 0.f;
	TArray<UTexture*> Textures;
	
	const_cast<UMaterialInstance*>(this)->GetUsedTextures(Textures);
	for ( INT TextureIndex = 0; TextureIndex < Textures.Num(); TextureIndex++ )
	{
		UTexture* AppliedTexture = Textures(TextureIndex);
		if ( AppliedTexture != NULL )
		{
			MaxTextureWidth = Max(MaxTextureWidth, AppliedTexture->GetSurfaceWidth());
		}
	}

	if ( Abs(MaxTextureWidth) < DELTA && Parent != NULL )
	{
 		MaxTextureWidth = Parent->GetSurfaceWidth();
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
FLOAT UMaterialInstance::GetSurfaceHeight() const
{
	FLOAT MaxTextureHeight = 0.f;
	TArray<UTexture*> Textures;
	
	const_cast<UMaterialInstance*>(this)->GetUsedTextures(Textures);
	for ( INT TextureIndex = 0; TextureIndex < Textures.Num(); TextureIndex++ )
	{
		UTexture* AppliedTexture = Textures(TextureIndex);
		if ( AppliedTexture != NULL )
		{
			MaxTextureHeight = Max(MaxTextureHeight, AppliedTexture->GetSurfaceHeight());
		}
	}

	if ( Abs(MaxTextureHeight) < DELTA && Parent != NULL )
	{
		MaxTextureHeight = Parent->GetSurfaceHeight();
	}

	return MaxTextureHeight;
}

void UMaterialInstance::AddReferencedObjects(TArray<UObject*>& ObjectArray)
{
	Super::AddReferencedObjects(ObjectArray);

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if(StaticPermutationResources[PlatformIndex])
		{
			StaticPermutationResources[PlatformIndex]->AddReferencedObjects(ObjectArray);
		}
	}
}

void UMaterialInstance::PreSave()
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

void UMaterialInstance::Serialize(FArchive& Ar)
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

	//only serialize the static permutation resource if one exists
	if (bHasStaticPermutationResource)
	{
		if (Ar.IsSaving())
		{
			//remove external private dependencies on the base material
			//@todo dw - come up with a better solution
			StaticPermutationResources[MSP_SM3]->RemoveExpressions();
		}

		if (Ar.IsLoading())
		{
			StaticPermutationResources[MSP_SM3] = AllocateResource();
		}

		StaticPermutationResources[MSP_SM3]->Serialize(Ar);
		if (Ar.Ver() < VER_UNIFORM_EXPRESSIONS_IN_SHADER_CACHE)
		{
			// If we are loading a material resource saved before texture references were managed by the material resource,
			// Pass the legacy texture references to the material resource.
			StaticPermutationResources[MSP_SM3]->AddLegacyTextures(ReferencedTextures_DEPRECATED);
		}
		StaticParameters[MSP_SM3]->Serialize(Ar);
		if (bSerializeShaderMap == TRUE)
		{
			if (Ar.IsSaving())
			{
				SerializeShaderMap(StaticPermutationResources[MSP_SM3]->GetShaderMap(), Ar);
			}
			else
			if (Ar.IsLoading())
			{
				FMaterialShaderMap* SerializedSM = SerializeShaderMap(NULL, Ar);
				StaticPermutationResources[MSP_SM3]->SetShaderMap(SerializedSM);
				SerializedSM->BeginInit();
			}
		}
	}

	if (bHasStaticPermutationResource && Ar.Ver() < VER_REMOVED_SHADER_MODEL_2)
	{
		FMaterialResource* LegacySM2Resource = NULL;
		if (Ar.IsLoading())
		{
			LegacySM2Resource = AllocateResource();
		}

		LegacySM2Resource->Serialize(Ar);
		FStaticParameterSet LegacySM2Paramters;
		LegacySM2Paramters.Serialize(Ar);
	}

	if (Ar.Ver() < VER_UNIFORM_EXPRESSIONS_IN_SHADER_CACHE)
	{
		// Empty legacy texture references on load
		ReferencedTextures_DEPRECATED.Empty();
	}

	if (Ar.Ver() < VER_INTEGRATED_LIGHTMASS)
	{
		ParentLightingGuid = Parent ? Parent->LightingGuid : FGuid(0,0,0,0);
	}
}

void UMaterialInstance::PostLoad()
{
	Super::PostLoad();

	if( GetName() == TEXT("LandscapeComponent_148_Inst") )
	{
		INT i=0;
	}


	//fixup the instance if the parent doesn't exist anymore
	if (bHasStaticPermutationResource && !Parent)
	{
		bHasStaticPermutationResource = FALSE;
	}

	// Make sure static parameters are up to date and shaders are cached for the current platform
	InitStaticPermutation();

	if( GIsEditor && !IsTemplate() )
	{
		// Ensure that the ReferencedTextureGuids array is up to date.
		UpdateLightmassTextureTracking();
	}

	for (INT i = 0; i < ARRAY_COUNT(Resources); i++)
	{
		if (Resources[i])
		{
			Resources[i]->UpdateDistanceFieldPenumbraScale(GetDistanceFieldPenumbraScale());
		}
	}
}

void UMaterialInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if( PropertyThatChanged )
	{
		if(PropertyThatChanged->GetName()==TEXT("Parent"))
		{
			if (Parent == NULL)
			{
				bHasStaticPermutationResource = FALSE;
			}

			ParentLightingGuid = Parent ? Parent->LightingGuid : FGuid(0,0,0,0);
		}
	}

	// Ensure that the ReferencedTextureGuids array is up to date.
	if (GIsEditor)
	{
		UpdateLightmassTextureTracking();
	}

	for (INT i = 0; i < ARRAY_COUNT(Resources); i++)
	{
		if (Resources[i])
		{
			Resources[i]->UpdateDistanceFieldPenumbraScale(GetDistanceFieldPenumbraScale());
		}
	}
}

void UMaterialInstance::BeginDestroy()
{
	Super::BeginDestroy();

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if(StaticPermutationResources[PlatformIndex])
		{
			StaticPermutationResources[PlatformIndex]->ReleaseFence.BeginFence();
		}
	}
}

UBOOL UMaterialInstance::IsReadyForFinishDestroy()
{
	UBOOL bIsReady = Super::IsReadyForFinishDestroy();

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		bIsReady = bIsReady && (!StaticPermutationResources[PlatformIndex] || !StaticPermutationResources[PlatformIndex]->ReleaseFence.GetNumPendingFences());
	}

	return bIsReady;
}

void UMaterialInstance::FinishDestroy()
{
	if(!GIsUCCMake&&!HasAnyFlags(RF_ClassDefaultObject))
	{
		BeginCleanup(Resources[0]);
		if(GIsEditor)
		{
			BeginCleanup(Resources[1]);
			BeginCleanup(Resources[2]);
		}
	}

	for (INT PlatformIndex = 0; PlatformIndex < MSP_MAX; PlatformIndex++)
	{
		if (StaticPermutationResources[PlatformIndex])
		{
			check(!StaticPermutationResources[PlatformIndex]->ReleaseFence.GetNumPendingFences());
			delete StaticPermutationResources[PlatformIndex];
			StaticPermutationResources[PlatformIndex] = NULL;
		}

		if (StaticParameters[PlatformIndex])
		{
			delete StaticParameters[PlatformIndex];
			StaticParameters[PlatformIndex] = NULL;
		}
	}

	Super::FinishDestroy();
}

/**
* Refreshes parameter names using the stored reference to the expression object for the parameter.
*/
void UMaterialInstance::UpdateParameterNames()
{
}

void UMaterialInstance::SetParent(UMaterialInterface* NewParent)
{
	Parent = NewParent;
}

void UMaterialInstance::SetVectorParameterValue(FName ParameterName,const FLinearColor& Value)
{
}

void UMaterialInstance::SetScalarParameterValue(FName ParameterName, float Value)
{
}

void UMaterialInstance::SetScalarCurveParameterValue(FName ParameterName, const FInterpCurveFloat& Value)
{
}

void UMaterialInstance::SetTextureParameterValue(FName ParameterName, UTexture* Value)
{
}

void UMaterialInstance::SetFontParameterValue(FName ParameterName,class UFont* FontValue,INT FontPage)
{
}

void UMaterialInstance::ClearParameterValues()
{
}

UBOOL UMaterialInstance::IsInMapOrTransientPackage() const
{
	const UBOOL bInMap = GetOutermost()->ContainsMap();
	const UBOOL bInTransient = GetOutermost() == UObject::GetTransientPackage();
#if !FINAL_RELEASE
	// if these are both false then the MI is in some terrible place
	if( ( bInMap == FALSE ) && ( bInTransient == FALSE ) && GAreScreenMessagesEnabled )
	{
		// this will print out of order but these usually SPAMMMMMM so much that getting the info is what is important.
		const FString WhereTheMIIsLocated = FString::Printf(TEXT("%s  GetOutermost(): %s  bInMap: %d  bInTransient: %d"), *GetName(), *GetOutermost()->GetName(), bInMap, bInTransient );
		debugf(*WhereTheMIIsLocated);
	}
#endif

	return (bInMap || bInTransient);
}

#if !FINAL_RELEASE
/** Displays warning if this material instance is not safe to modify in the game (is in content package) */
void UMaterialInstance::CheckSafeToModifyInGame(const TCHAR* FuncName) const
{
	if(GIsGame && !IsInMapOrTransientPackage() && GAreScreenMessagesEnabled)
	{
		const FString Message = FString::Printf(TEXT("%s : Modifying '%s' during gameplay."), FuncName, *GetName());
		GWorld->GetWorldInfo()->AddOnScreenDebugMessage((QWORD)((PTRINT)this), 5.f, FColor(0,255,255), *Message);
		debugf(*Message);
	}
}
#endif

/**
 *	Check if the textures have changed since the last time the material was
 *	serialized for Lightmass... Update the lists while in here.
 *	It will update the LightingGuid if the textures have changed.
 *	NOTE: This will NOT mark the package dirty if they have changed.
 *
 *	@return	UBOOL	TRUE if the textures have changed.
 *					FALSE if they have not.
 */
UBOOL UMaterialInstance::UpdateLightmassTextureTracking()
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

/** @return	The bCastShadowAsMasked value for this material. */
UBOOL UMaterialInstance::GetCastShadowAsMasked() const
{
	if (LightmassSettings.bOverrideCastShadowAsMasked)
	{
		return LightmassSettings.bCastShadowAsMasked;
	}

	if (Parent)
	{
		return Parent->GetCastShadowAsMasked();
	}

	return FALSE;
}

/** @return	The Emissive boost value for this material. */
FLOAT UMaterialInstance::GetEmissiveBoost() const
{
	if (LightmassSettings.bOverrideEmissiveBoost)
	{
		return LightmassSettings.EmissiveBoost;
	}

	if (Parent)
	{
		return Parent->GetEmissiveBoost();
	}

	return 1.0f;
}

/** @return	The Diffuse boost value for this material. */
FLOAT UMaterialInstance::GetDiffuseBoost() const
{
	if (LightmassSettings.bOverrideDiffuseBoost)
	{
		return LightmassSettings.DiffuseBoost;
	}

	if (Parent)
	{
		return Parent->GetDiffuseBoost();
	}

	return 1.0f;
}

/** @return	The Specular boost value for this material. */
FLOAT UMaterialInstance::GetSpecularBoost() const
{
	if (LightmassSettings.bOverrideSpecularBoost)
	{
		return LightmassSettings.SpecularBoost;
	}

	if (Parent)
	{
		return Parent->GetSpecularBoost();
	}

	return 1.0f;
}

/** @return	The ExportResolutionScale value for this material. */
FLOAT UMaterialInstance::GetExportResolutionScale() const
{
	if (LightmassSettings.bOverrideExportResolutionScale)
	{
		return LightmassSettings.ExportResolutionScale;
	}

	if (Parent)
	{
		return Parent->GetExportResolutionScale();
	}

	return 1.0f;
}

FLOAT UMaterialInstance::GetDistanceFieldPenumbraScale() const
{
	if (LightmassSettings.bOverrideDistanceFieldPenumbraScale)
	{
		return LightmassSettings.DistanceFieldPenumbraScale;
	}

	if (Parent)
	{
		return Parent->GetDistanceFieldPenumbraScale();
	}

	return 1.0f;
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
UBOOL UMaterialInstance::GetTexturesInPropertyChain(EMaterialProperty InProperty, TArray<UTexture*>& OutTextures,  TArray<FName>* OutTextureParamNames)
{
	if (Parent != NULL)
	{
		TArray<FName> LocalTextureParamNames;
		UBOOL bResult = Parent->GetTexturesInPropertyChain(InProperty, OutTextures, &LocalTextureParamNames);
		if (LocalTextureParamNames.Num() > 0)
		{
			// Check textures set in parameters as well...
			for (INT TPIdx = 0; TPIdx < LocalTextureParamNames.Num(); TPIdx++)
			{
				UTexture* ParamTexture = NULL;
				if (GetTextureParameterValue(LocalTextureParamNames(TPIdx), ParamTexture) == TRUE)
				{
					if (ParamTexture != NULL)
					{
						OutTextures.AddUniqueItem(ParamTexture);
					}
				}

				if (OutTextureParamNames != NULL)
				{
					OutTextureParamNames->AddUniqueItem(LocalTextureParamNames(TPIdx));
				}
			}
		}
		return bResult;
	}
	return FALSE;
}
