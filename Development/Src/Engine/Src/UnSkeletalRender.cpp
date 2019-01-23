/*=============================================================================
	UnSkeletalRender.cpp: Skeletal mesh skinning/rendering code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnSkeletalRender.h"

/*-----------------------------------------------------------------------------
Globals
-----------------------------------------------------------------------------*/

// smallest blend weight for morph targets
const FLOAT MinMorphBlendWeight = 0.01f;
// largest blend weight for morph targets
const FLOAT MaxMorphBlendWeight = 5.0f;

/*-----------------------------------------------------------------------------
FSkeletalMeshObject
-----------------------------------------------------------------------------*/

/** 
*	Given a set of views, update the MinDesiredLODLevel member to indicate the minimum (ie best) LOD we would like to use to render this mesh. 
*	This is called from the rendering thread (PreRender) so be very careful what you read/write to.
*	If this is the first render for the frame, will just set MinDesiredLODLevel - otherwise will set it to min of current MinDesiredLODLevel and calculated value.
*/
void FSkeletalMeshObject::UpdateMinDesiredLODLevel(const FSceneView* View, const FBoxSphereBounds& Bounds, INT FrameNumber)
{
	FVector4 ScreenPosition( View->WorldToScreen(Bounds.Origin) );
	FLOAT ScreenRadius = Max((FLOAT)View->SizeX / 2.0f * View->ProjectionMatrix.M[0][0],
		(FLOAT)View->SizeY / 2.0f * View->ProjectionMatrix.M[1][1]) * Bounds.SphereRadius / Max(ScreenPosition.W,1.0f);
	FLOAT LODFactor = ScreenRadius / 320.0f;

	check( SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num() );

	INT NewLODLevel = 0;
	for(INT LODLevel = 0; LODLevel < SkeletalMesh->LODModels.Num() - 1; LODLevel++) 
	{
		if(SkeletalMesh->LODInfo(LODLevel + 1).DisplayFactor > LODFactor)
		{
			NewLODLevel = LODLevel + 1;
		}
	}

	if(FrameNumber != LastFrameNumber)
	{
		LastFrameNumber = FrameNumber;
		MaxDistanceFactor = LODFactor;
		MinDesiredLODLevel = NewLODLevel;
	}
	else
	{
		MaxDistanceFactor = ::Max(MaxDistanceFactor, LODFactor);
		MinDesiredLODLevel = ::Min(MinDesiredLODLevel, NewLODLevel);
	}
}

/*-----------------------------------------------------------------------------
Global functions
-----------------------------------------------------------------------------*/

/**
 * Utility function that fills in the array of ref-pose to local-space matrices using 
 * the mesh component's updated space bases
 * @param	ReferenceToLocal - matrices to update
 * @param	SkeletalMeshComponent - mesh primitive with updated bone matrices
 * @param	LODIndex - each LOD has its own mapping of bones to update
 */
void UpdateRefToLocalMatrices( TArray<FMatrix>& ReferenceToLocal, USkeletalMeshComponent* SkeletalMeshComponent, INT LODIndex )
{
	USkeletalMesh* ThisMesh = SkeletalMeshComponent->SkeletalMesh;
	USkeletalMeshComponent* ParentComp = SkeletalMeshComponent->ParentAnimComponent;
	FStaticLODModel& LOD = ThisMesh->LODModels(LODIndex);

	ReferenceToLocal.Empty(ThisMesh->RefBasesInvMatrix.Num());
	ReferenceToLocal.Add(ThisMesh->RefBasesInvMatrix.Num());

	// Handle case of using ParentAnimComponent for SpaceBases.
	if( ParentComp && 
		SkeletalMeshComponent->ParentBoneMap.Num() == ThisMesh->RefSkeleton.Num() )
	{
		for(INT BoneIndex = 0;BoneIndex < LOD.ActiveBoneIndices.Num();BoneIndex++)
		{
			// Get the index of the bone in this skeleton, and loop up in table to find index in parent component mesh.
			INT ThisBoneIndex = LOD.ActiveBoneIndices(BoneIndex);
			INT ParentBoneIndex = SkeletalMeshComponent->ParentBoneMap(ThisBoneIndex);

			// If valid, use matrix from parent component.
			if( ThisMesh->RefBasesInvMatrix.IsValidIndex(ThisBoneIndex) &&
				ParentComp->SpaceBases.IsValidIndex(ParentBoneIndex) )
			{
				ReferenceToLocal(ThisBoneIndex) = ThisMesh->RefBasesInvMatrix(ThisBoneIndex) * ParentComp->SpaceBases(ParentBoneIndex);
			}
			// If we can't find this bone in the parent, we just use the reference pose. 
			else
			{
				// In this case we basically want Reference->Reference ie identity.
				ReferenceToLocal(ThisBoneIndex) = FMatrix::Identity;
			}
		}
	}
	else
	{
		for(INT BoneIndex = 0;BoneIndex < LOD.ActiveBoneIndices.Num();BoneIndex++)
		{
			INT ThisBoneIndex = LOD.ActiveBoneIndices(BoneIndex);

			if( ThisMesh->RefBasesInvMatrix.IsValidIndex(ThisBoneIndex) &&
				SkeletalMeshComponent->SpaceBases.IsValidIndex(ThisBoneIndex) )
			{
				ReferenceToLocal(ThisBoneIndex) = ThisMesh->RefBasesInvMatrix(ThisBoneIndex) * SkeletalMeshComponent->SpaceBases(ThisBoneIndex);
			}
			else
			{
				ReferenceToLocal(ThisBoneIndex) = FMatrix::Identity;
			}			
		}
	}
}
