/**
 *	
 *	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "MaterialInstance.h"

IMPLEMENT_CLASS(UMaterialInstance);



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


UMaterial* UMaterialInstance::GetMaterial()
{
	if(ReentrantFlag)
	{
		return GEngine->DefaultMaterial;
	}

	FMICReentranceGuard	Guard(this);
	if(Parent)
	{
		return Parent->GetMaterial();
	}
	else
	{
		return GEngine->DefaultMaterial;
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
		Parent->AllocateResource();
	}

	//otherwise allocate a material resource without specifying a material.
	//the material will be set by AllocateStaticPermutations() before trying to compile this material resource.
	return new FMaterialResource(NULL);
}

/**
 * Gets the static permutation resource if the instance has one
 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
 */
FMaterialResource* UMaterialInstance::GetMaterialResource(EShaderPlatform Platform)
{
	if(bHasStaticPermutationResource)
	{
		//if there is a static permutation resource, use that
		const EMaterialShaderPlatform RequestedPlatform = GetMaterialPlatform(Platform);
		return StaticPermutationResources[RequestedPlatform];
	}

	//there was no static permutation resource
	return NULL;
}

FMaterialRenderProxy* UMaterialInstance::GetRenderProxy(UBOOL Selected) const
{
	check(!Selected || GIsEditor);
	return Resources[Selected];
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
* Passes the override up the MIC chain
*
* @param	Permutation		The set of static parameters to override and their values	
*/
void UMaterialInstance::SetStaticParameterOverrides(const FStaticParameterSet* Permutation)
{
	Parent->SetStaticParameterOverrides(Permutation);
}

/**
* Passes the override clear up the MIC chain
*/
void UMaterialInstance::ClearStaticParameterOverrides()
{
	Parent->ClearStaticParameterOverrides();
}

/**
* Makes a copy of all the instance's inherited and overriden static parameters
*
* @param StaticParameters - The set of static parameters to fill, must be empty
*/
void UMaterialInstance::GetStaticParameterValues(FStaticParameterSet * InStaticParameters)
{
	check(InStaticParameters && InStaticParameters->IsEmpty());
	if (Parent)
	{
		UMaterial * ParentMaterial = Parent->GetMaterial();
		TArray<FName> ParameterNames;

		// Static Switch Parameters
		ParentMaterial->GetAllStaticSwitchParameterNames(ParameterNames);
		InStaticParameters->StaticSwitchParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FStaticSwitchParameter& ParentParameter = InStaticParameters->StaticSwitchParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UBOOL Value = FALSE;
			FGuid ExpressionId = FGuid(0,0,0,0);

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
		ParentMaterial->GetAllStaticComponentMaskParameterNames(ParameterNames);
		InStaticParameters->StaticComponentMaskParameters.AddZeroed(ParameterNames.Num());
		for(INT ParameterIdx=0; ParameterIdx<ParameterNames.Num(); ParameterIdx++)
		{
			FStaticComponentMaskParameter& ParentParameter = InStaticParameters->StaticComponentMaskParameters(ParameterIdx);
			FName ParameterName = ParameterNames(ParameterIdx);
			UBOOL R = FALSE;
			UBOOL G = FALSE;
			UBOOL B = FALSE;
			UBOOL A = FALSE;
			FGuid ExpressionId = FGuid(0,0,0,0);

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
	//mark the static permutation resource dirty if necessary, so it will be recompiled on PostEditChange()
	//only check against the sm3 set, since the sm2 set will have the same static paramters, just a different BaseMaterialId.
	bStaticPermutationDirty = StaticParameters[MSP_SM3]->ShouldMarkDirty(EditorParameters);

	if (Parent)
	{
		UMaterial * ParentMaterial = Parent->GetMaterial();
		const FMaterial * ParentMaterialResource = ParentMaterial->GetMaterialResource();
		const EMaterialShaderPlatform MaterialPlatform = GetMaterialPlatform(GRHIShaderPlatform);

		//check if the BaseMaterialId of the appropriate static parameter set still matches the Id 
		//of the material resource that the base UMaterial owns.  If they are different, the base UMaterial
		//has been edited since the static permutation was compiled.
		if (ParentMaterialResource->GetId() != StaticParameters[MaterialPlatform]->BaseMaterialId)
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
* Compiles the static permutation resource if the base material has changed and updates dirty states
*/
void UMaterialInstance::UpdateStaticPermutation()
{
	if (bStaticPermutationDirty && Parent)
	{
		//a static permutation resource must exist if there are static parameters
		bHasStaticPermutationResource = !StaticParameters[MSP_SM3]->IsEmpty();
		CacheResourceShaders(TRUE, FALSE);
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
		SetStaticParameterValues(&UpdatedStaticParameters);
	}

	//Make sure the material resource shaders are cached.
	CacheResourceShaders();
}

/**
* Compiles material resources for the current platform if the shader map for that resource didn't already exist.
*
* @param bFlushExistingShaderMaps - forces a compile, removes existing shader maps from shader cache.
* @param bForceAllPlatforms - compile for all platforms, not just the current.
*/
void UMaterialInstance::CacheResourceShaders(UBOOL bFlushExistingShaderMaps, UBOOL bForceAllPlatforms)
{
	if (bHasStaticPermutationResource)
	{
		//make sure all material resources are allocated and have their Material member updated
		AllocateStaticPermutations();
		const EMaterialShaderPlatform RequestedMaterialPlatform = GetMaterialPlatform(GRHIShaderPlatform);
		const EShaderPlatform OtherPlatformsToCompile[] = {SP_PCD3D_SM3, SP_PCD3D_SM2};

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
				EShaderPlatform CurrentShaderPlatform = GRHIShaderPlatform;
				//if we're not compiling for the same platform that is running, 
				//lookup what platform we should compile for based on the current material platform
				if (PlatformIndex != RequestedMaterialPlatform)
				{
					CurrentShaderPlatform = OtherPlatformsToCompile[PlatformIndex];
				}

				Parent->CompileStaticPermutation(
					StaticParameters[PlatformIndex], 
					StaticPermutationResources[PlatformIndex], 
					CurrentShaderPlatform, 
					(EMaterialShaderPlatform)PlatformIndex,
					bFlushExistingShaderMaps);

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
	UBOOL bFlushExistingShaderMaps)
{
	return Parent->CompileStaticPermutation(Permutation, StaticPermutation, Platform, MaterialPlatform, bFlushExistingShaderMaps);
}

/**
* Allocates the static permutation resources for all platforms if they haven't been already.
* Also updates the material resource's Material member as it may have changed.
* (This can happen if a Fallback Material is assigned after the material resource is created)
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
			UMaterial * ParentMaterial = Parent->GetMaterial();
			//if a fallback material has been specified and this is the sm2 platform, 
			//set the fallback material as the material resource's Material member.
			if (ParentMaterial->FallbackMaterial && PlatformIndex == MSP_SM2)
			{
				StaticPermutationResources[PlatformIndex]->SetMaterial(ParentMaterial->FallbackMaterial);
			}
			else
			{
				StaticPermutationResources[PlatformIndex]->SetMaterial(ParentMaterial);
			}
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



void UMaterialInstance::PreSave()
{
	Super::PreSave();

	//make sure all material resources for all platforms have been initialized (and compiled if needed)
	//so that the material resources will be complete for saving.  Don't do this during script compile,
	//since only dummy materials are saved, or when cooking, since compiling material shaders is handled explicitly by the commandlet.
	if (!GIsUCCMake && !GIsCooking)
	{
		CacheResourceShaders(FALSE, TRUE);
	}
}

void UMaterialInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	//only serialize the static permutation resource if one exists
	if (Ar.Ver() >= VER_STATIC_MATERIAL_PARAMETERS && bHasStaticPermutationResource)
	{
		if (Ar.IsSaving())
		{
			//remove external private dependencies on the base material
			//@todo dw - come up with a better solution
			StaticPermutationResources[MSP_SM3]->RemoveExpressions();
		}

		if (Ar.IsLoading())
		{
			check(Parent);
			StaticPermutationResources[MSP_SM3] = Parent->AllocateResource();
		}

		StaticPermutationResources[MSP_SM3]->Serialize(Ar);
		StaticParameters[MSP_SM3]->Serialize(Ar);
	}

	if (Ar.Ver() >= VER_MATERIAL_FALLBACKS && bHasStaticPermutationResource)
	{
		if (Ar.IsSaving())
		{
			//remove external private dependencies on the base material
			//@todo dw - come up with a better solution
			StaticPermutationResources[MSP_SM2]->RemoveExpressions();
		}

		if (Ar.IsLoading())
		{
			check(Parent);
			StaticPermutationResources[MSP_SM2] = Parent->AllocateResource();
		}

		StaticPermutationResources[MSP_SM2]->Serialize(Ar);
		StaticParameters[MSP_SM2]->Serialize(Ar);
	}
}


void UMaterialInstance::PostLoad()
{
	Super::PostLoad();

	//skip caching shader maps while cooking for consoles since this is handled by the commandlet
	if (GCookingTarget & UE3::PLATFORM_Console)
	{
		if (bHasStaticPermutationResource)
		{
			//make sure all material resources are allocated and have their Material member set
			AllocateStaticPermutations();
		}
	}
	else
	{
		InitStaticPermutation();
	}
}

void UMaterialInstance::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);

	if( PropertyThatChanged )
	{
		if(PropertyThatChanged->GetName()==TEXT("Parent"))
		{
			if (Parent == NULL)
			{
				bHasStaticPermutationResource = FALSE;
			}
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

void UMaterialInstance::SetVectorParameterValue(FName ParameterName, FLinearColor Value)
{
}

void UMaterialInstance::SetScalarParameterValue(FName ParameterName, float Value)
{
}

void UMaterialInstance::SetScalarCurveParameterValue(FName ParameterName, FInterpCurveFloat Value)
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
