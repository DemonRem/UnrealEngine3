
/*=============================================================================
	UnSkeletalComponentClothing.cpp: Implments methods for ApexClothing
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"
#include "UnFracturedStaticMesh.h"
#include "EngineMeshClasses.h"
#include "EngineAnimClasses.h"
#include "UnSkeletalMesh.h"
#include "UnSkeletalRender.h"
#include "UnTerrain.h"

#if WITH_NOVODEX
#include "UnNovodexSupport.h"
#include "UnSoftBodySupport.h"
#endif // WITH_NOVODEX

#include "NvApexManager.h"

#if WITH_APEX
#include "NxParameterized.h"
#include "NvApexCommands.h"
#include "NvApexScene.h"
#endif

#if WITH_APEX 
static void SetGroupsMask(USkeletalMeshComponent* SkelComp, ::NxParameterized::Interface *np)
{
	// Need to set up the correct collision filters.
	if ( np )
	{
		NxGroupsMask nClothingMask = CreateGroupsMask(SkelComp->ApexClothingRBChannel, &SkelComp->ApexClothingRBCollideWithChannels);
		NxGroupsMask nClothingCollisionMask = CreateGroupsMask(SkelComp->ApexClothingCollisionRBChannel, NULL);

		NxParameterized::setParamU32(*np,"clothDescTemplate.collisionGroup", SkelComp->ApexClothingRBChannel);
		NxParameterized::setParamU32(*np,"clothDescTemplate.groupsMask.bits0", nClothingMask.bits0 );
		NxParameterized::setParamU32(*np,"clothDescTemplate.groupsMask.bits1", nClothingMask.bits1 );
		NxParameterized::setParamU32(*np,"clothDescTemplate.groupsMask.bits2", nClothingMask.bits2 );
		NxParameterized::setParamU32(*np,"clothDescTemplate.groupsMask.bits3", nClothingMask.bits3 );

		NxParameterized::setParamU32(*np,"shapeDescTemplate.collisionGroup", SkelComp->ApexClothingCollisionRBChannel);
		NxParameterized::setParamU32(*np,"shapeDescTemplate.groupsMask.bits0", nClothingCollisionMask.bits0 );
		NxParameterized::setParamU32(*np,"shapeDescTemplate.groupsMask.bits1", nClothingCollisionMask.bits1 );
		NxParameterized::setParamU32(*np,"shapeDescTemplate.groupsMask.bits2", nClothingCollisionMask.bits2 );
		NxParameterized::setParamU32(*np,"shapeDescTemplate.groupsMask.bits3", nClothingCollisionMask.bits3 );
	}
}
#endif

void USkeletalMeshComponent::TickApexClothing(FLOAT DeltaTime)
{
#if WITH_APEX
	if ( ApexClothing )
	{
		// Apply Wind to Apex Clothing
		FIApexClothing *Clothing = GetApexClothing();
		if ( Clothing && Clothing->IsVisible(TRUE) && (!PhysicsAssetInstance || PhysicsWeight == 0))
		{
			// Get wind in world space
			FVector WorldSpaceWind;
			if(bLocalSpaceWind)
			{
				FMatrix LocalToWorldNoScale = LocalToWorld;
				LocalToWorldNoScale.RemoveScaling();
				WorldSpaceWind = LocalToWorldNoScale.TransformNormal(WindVelocity);
			}
			else
			{
				WorldSpaceWind = WindVelocity;
			}


			physx::PxF32 WindVector[3];
			WindVector[0] = WorldSpaceWind.X * U2PScale;
			WindVector[1] = WorldSpaceWind.Y * U2PScale;
			WindVector[2] = WorldSpaceWind.Z * U2PScale;
			FLOAT AdaptTime = (WindVelocityBlendTime == 0) ? 0: 1.0f/WindVelocityBlendTime;
			ApexClothing->SetWind(AdaptTime, WindVector);
		}

		ApexClothing->Tick(DeltaTime);

		// check to see if we have lost any of our previous assets and, if so, re-create them.  Kind of like a 'devicelost' with D3D
		if ( ApexClothing->NeedsRefreshAsset()	&& SkeletalMesh->Materials.Num() == SkeletalMesh->ClothingAssets.Num() )
		{
    		for (INT i=0; i<SkeletalMesh->Materials.Num(); i++)
    		{
    		   UMaterialInterface *mat = SkeletalMesh->Materials(i);
    		   if ( mat )
    		   {
    			   UMaterial *umat = mat->GetMaterial();
    			   if ( umat )
    			   {
    				   UApexClothingAsset* pa = SkeletalMesh->ClothingAssets(i);
    				   if ( pa && pa->MApexAsset )
    				   {
							::NxParameterized::Interface *np =  (::NxParameterized::Interface *)pa->GetNxParameterized();
							if ( np )
							{
								SetGroupsMask(this, np);
								ApexClothing->RefreshApexClothingPiece(pa->MApexAsset,np);
							}
						}
    			   }
    		   }
    		}
		}
	}
#endif
}

void USkeletalMeshComponent::SetApexClothingMaxDistanceScale(FLOAT StartScale, FLOAT EndScale, INT ScaleMode, FLOAT Duration)
{
#if WITH_APEX 
	if ( ApexClothing )
		ApexClothing->SetMaxDistanceScale(StartScale, EndScale, ScaleMode, Duration);
#endif
}

void USkeletalMeshComponent::InitApexClothing(FRBPhysScene* RBPhysScene)
{
#if WITH_APEX 
	if ( SkeletalMesh && RBPhysScene && RBPhysScene->ApexScene && ApexClothing == NULL )
	{
		if ( SkeletalMesh->Materials.Num() != SkeletalMesh->ClothingAssets.Num() )
		{
			// Pre-clothing SkeletalMesh
			bSkipInitClothing = TRUE;
			return;
		}

		for (INT i=0; i<SkeletalMesh->ClothingAssets.Num(); i++)
		{
		   UApexClothingAsset* pa = SkeletalMesh->ClothingAssets(i);
		   if ( pa && pa->MApexAsset )
		   {
			   	if ( ApexClothing == 0 )
			   	{
				    NxU32 boneCount = (NxU32)SkeletalMesh->RefSkeleton.Num();
					ApexClothing = RBPhysScene->ApexScene->CreateApexClothing();
					if ( ApexClothing )
					{
						for (NxU32 i=0; i<boneCount; i++)
						{
							const FMeshBone &bone = SkeletalMesh->RefSkeleton(i);
							ApexClothing->AddBoneName(TCHAR_TO_ANSI( *bone.Name.GetNameString()));
						}
					}
			   	}
				if ( ApexClothing )
				{
					::NxParameterized::Interface *np =  (::NxParameterized::Interface *)pa->GetNxParameterized();
					if ( np )
					{
						SetGroupsMask(this, np);
						ApexClothing->AddApexClothingPiece(pa->MApexAsset,np,(physx::PxU32)i, this);
					}

					// Ensure that the material is usable for APEX clothing
					InitMaterialForApex( SkeletalMesh->Materials(i) );
				}
		   }
		}

		if ( ApexClothing == NULL )
		{
			// No clothing asset referenced by the SkeletalMesh; don't check for this again
			bSkipInitClothing = TRUE;
		}

		if ( MeshObject && ApexClothing )
		{
			// Clothing needs updated transforms even when attached to sleeping KAssets.
			// Otherwise if a ragdoll goes to sleep when you aren't looking at it, the clothing
			// will appear in the wrong place when you do look at it.
			bSkipAllUpdateWhenPhysicsAsleep = FALSE;
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		        FSetApexClothingCommand, 	
		        FSkeletalMeshObject*, MeshObject, MeshObject, 
		        FIApexClothing*, ApexClothing, ApexClothing, 
			{
			        MeshObject->ApexClothing = ApexClothing;
			});
		}
	}
#else
	bSkipInitClothing = TRUE;
#endif
}

void USkeletalMeshComponent::ReleaseApexClothing()
{
#if WITH_APEX
	if ( ApexClothing )
	{
		ApexClothing->Release();
		ApexClothing = NULL;
		if ( MeshObject )
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		        FSetApexClothingCommand, 	
		        FSkeletalMeshObject*, MeshObject, MeshObject, 
		        FIApexClothing*, ApexClothing, ApexClothing, 
			{
			        MeshObject->ApexClothing = NULL;
			});


		}
	}
#endif
}

/** Toggle active simulation of cloth. Cheaper than doing SetEnableClothSimulation, and keeps its shape while frozen. */
void USkeletalMeshComponent::SetEnableClothingSimulation(UBOOL bInEnable)
{
#if WITH_APEX_CLOTHING
	FIApexClothing* Clothing = GetApexClothing();
	if( Clothing )
	{
		Clothing->ToggleClothingSimulation(bInEnable);
	}
#endif
}

/** Script version of SetEnableClothingSimulation. */
void USkeletalMeshComponent::execSetEnableClothingSimulation( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL(bInEnable);
	P_FINISH;

	SetEnableClothingSimulation(bInEnable);
}
