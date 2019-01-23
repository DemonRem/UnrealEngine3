/*=============================================================================
	AnimationUtils.cpp: Skeletal mesh animation utilities.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "AnimationUtils.h"

void FAnimationUtils::BuildSekeltonMetaData(const TArray<FMeshBone>& Skeleton, TArray<FBoneData>& OutBoneData)
{
	// Assemble bone data.
	OutBoneData.Empty();
	OutBoneData.AddZeroed( Skeleton.Num() );

	for ( INT BoneIndex = 0 ; BoneIndex < Skeleton.Num() ; ++BoneIndex )
	{
		FBoneData& BoneData = OutBoneData(BoneIndex);

		// Copy over data from the skeleton.
		const FMeshBone& SrcBone = Skeleton(BoneIndex);
		BoneData.Orientation = SrcBone.BonePos.Orientation;
		BoneData.Position = SrcBone.BonePos.Position;
		BoneData.Name = SrcBone.Name;

		if ( BoneIndex > 0 )
		{
			// Compute ancestry.
			INT ParentIndex = Skeleton(BoneIndex).ParentIndex;
			BoneData.BonesToRoot.AddItem( ParentIndex );
			while ( ParentIndex > 0 )
			{
				ParentIndex = Skeleton(ParentIndex).ParentIndex;
				BoneData.BonesToRoot.AddItem( ParentIndex );
			}
		}
	}

	// Enumerate children (bones that refer to this bone as parent).
	for ( INT BoneIndex = 0 ; BoneIndex < OutBoneData.Num() ; ++BoneIndex )
	{
		FBoneData& BoneData = OutBoneData(BoneIndex);
		// Exclude the root bone as it is the child of nothing.
		for ( INT BoneIndex2 = 1 ; BoneIndex2 < OutBoneData.Num() ; ++BoneIndex2 )
		{
			if ( OutBoneData(BoneIndex2).GetParent() == BoneIndex )
			{
				BoneData.Children.AddItem(BoneIndex2);
			}
		}
	}

	// Enumerate end effectors.  For each end effector, propagate its index up to all ancestors.
	for ( INT BoneIndex = 0 ; BoneIndex < OutBoneData.Num() ; ++BoneIndex )
	{
		FBoneData& BoneData = OutBoneData(BoneIndex);
		if ( BoneData.IsEndEffector() )
		{
			// End effectors have themselves as an ancestor.
			BoneData.EndEffectors.AddItem( BoneIndex );
			// Add the end effector to the list of end effectors of all ancestors.
			for ( INT i = 0 ; i < BoneData.BonesToRoot.Num() ; ++i )
			{
				const INT AncestorIndex = BoneData.BonesToRoot(i);
				OutBoneData(AncestorIndex).EndEffectors.AddItem( BoneIndex );
			}
		}
	}
#if 0
	debugf( TEXT("====END EFFECTORS:") );
	INT NumEndEffectors = 0;
	for ( INT BoneIndex = 0 ; BoneIndex < OutBoneData.Num() ; ++BoneIndex )
	{
		const FBoneData& BoneData = OutBoneData(BoneIndex);
		if ( BoneData.IsEndEffector() )
		{
			FString Message( FString::Printf(TEXT("%s(%i): "), *BoneData.Name, BoneData.GetDepth()) );
			for ( INT i = 0 ; i < BoneData.BonesToRoot.Num() ; ++i )
			{
				const INT AncestorIndex = BoneData.BonesToRoot(i);
				Message += FString::Printf( TEXT("%s "), *OutBoneData(AncestorIndex).Name );
			}
			debugf( *Message );
			++NumEndEffectors;
		}
	}
	debugf( TEXT("====NUM EFFECTORS %i(%i)"), NumEndEffectors, OutBoneData(0).Children.Num() );
	debugf( TEXT("====NON END EFFECTORS:") );
	for ( INT BoneIndex = 0 ; BoneIndex < OutBoneData.Num() ; ++BoneIndex )
	{
		const FBoneData& BoneData = OutBoneData(BoneIndex);
		if ( !BoneData.IsEndEffector() )
		{
			FString Message( FString::Printf(TEXT("%s(%i): "), *BoneData.Name, BoneData.GetDepth()) );
			Message += TEXT("Children: ");
			for ( INT i = 0 ; i < BoneData.Children.Num() ; ++i )
			{
				const INT ChildIndex = BoneData.Children(i);
				Message += FString::Printf( TEXT("%s "), *OutBoneData(ChildIndex).Name );
			}
			Message += TEXT("  EndEffectors: ");
			for ( INT i = 0 ; i < BoneData.EndEffectors.Num() ; ++i )
			{
				const INT EndEffectorIndex = BoneData.EndEffectors(i);
				Message += FString::Printf( TEXT("%s "), *OutBoneData(EndEffectorIndex).Name );
				check( OutBoneData(EndEffectorIndex).IsEndEffector() );
			}
			debugf( *Message );
		}
	}
	debugf( TEXT("===================") );
#endif
}

/**
* Builds the local-to-component transformation for all bones.
*/
void FAnimationUtils::BuildComponentSpaceTransforms(TArray<FMatrix>& OutTransforms,
												const TArray<FBoneAtom>& LocalAtoms,
												const TArray<BYTE>& RequiredBones,
												const TArray<FMeshBone>& RefSkel)
{
	OutTransforms.Empty();
	OutTransforms.Add( RefSkel.Num() );

	for ( INT i = 0 ; i < RequiredBones.Num() ; ++i )
	{
		const INT BoneIndex = RequiredBones(i);
		LocalAtoms(BoneIndex).ToTransform( OutTransforms(BoneIndex) );

		// For all bones below the root, final component-space transform is relative transform * component-space transform of parent.
		if ( BoneIndex > 0 )
		{
			const INT ParentIndex = RefSkel(BoneIndex).ParentIndex;

			// Check the precondition that parents occur before children in the RequiredBones array.
			const INT ReqBoneParentIndex = RequiredBones.FindItemIndex(ParentIndex);
			check( ReqBoneParentIndex != INDEX_NONE );
			check( ReqBoneParentIndex < i );

			OutTransforms(BoneIndex) *= OutTransforms(ParentIndex);
		}
	}
}

/**
* Builds the local-to-component matrix for the specified bone.
*/
void FAnimationUtils::BuildComponentSpaceTransform(FMatrix& OutTransform,
											   INT BoneIndex,
											   const TArray<FBoneAtom>& LocalAtoms,
											   const TArray<FBoneData>& BoneData)
{
	// Put root-to-component in OutTransform.
	LocalAtoms(0).ToTransform( OutTransform );

	if ( BoneIndex > 0 )
	{
		const FBoneData& Bone = BoneData(BoneIndex);

		checkSlow( Bone.BonesToRoot.Num()-1 == 0 );

		// Compose BoneData.BonesToRoot down.
		FMatrix ToParent;
		for ( INT i = Bone.BonesToRoot.Num()-2 ; i >=0 ; --i )
		{
			const INT AncestorIndex = Bone.BonesToRoot(i);
			LocalAtoms(AncestorIndex).ToTransform( ToParent );
			ToParent *= OutTransform;
			OutTransform = ToParent;
		}

		// Finally, include the bone's local-to-parent.
		LocalAtoms(BoneIndex).ToTransform( ToParent );
		ToParent *= OutTransform;
		OutTransform = ToParent;
	}
}

void FAnimationUtils::BuildPoseFromRawSequenceData(TArray<FBoneAtom>& OutAtoms,
											   UAnimSequence* AnimSeq,
											   const TArray<BYTE>& RequiredBones,
											   USkeletalMesh* SkelMesh,
											   FLOAT Time,
											   UBOOL bLooping)
{
	// Get/create a linkup cache for the mesh to compress against.
	UAnimSet* AnimSet						= AnimSeq->GetAnimSet();
	const INT AnimLinkupIndex				= AnimSet->GetMeshLinkupIndex( SkelMesh );
	check( AnimLinkupIndex != INDEX_NONE );
	check( AnimLinkupIndex < AnimSet->LinkupCache.Num() );

	const FAnimSetMeshLinkup& AnimLinkup	= AnimSet->LinkupCache( AnimLinkupIndex );
	const TArray<FMeshBone>& RefSkel		= SkelMesh->RefSkeleton;
	check( AnimLinkup.BoneToTrackTable.Num() == RefSkel.Num() );

	for ( INT i = 0 ; i < RequiredBones.Num() ; ++i )
	{
		// Map the bone index to sequence track.
		const INT BoneIndex					= RequiredBones(i);
		const INT TrackIndex				= AnimLinkup.BoneToTrackTable(BoneIndex);
		if ( TrackIndex == INDEX_NONE )
		{
			// No track for the bone was found, so use the reference pose.
			OutAtoms(BoneIndex)	= FBoneAtom( RefSkel(BoneIndex).BonePos.Orientation, RefSkel(BoneIndex).BonePos.Position, 1.f );
		}
		else
		{
			// Get pose information using the raw data.
			AnimSeq->GetBoneAtom( OutAtoms(BoneIndex), TrackIndex, Time, bLooping, TRUE );
		}

		// Apply quaternion fix for ActorX-exported quaternions.
		if( BoneIndex > 0 ) 
		{
			OutAtoms(BoneIndex).Rotation.W *= -1.0f;
		}
	}
}

void FAnimationUtils::BuildPoseFromReducedKeys(TArray<FBoneAtom>& OutAtoms,
										   const FAnimSetMeshLinkup& AnimLinkup,
										   const TArray<FTranslationTrack>& TranslationData,
										   const TArray<FRotationTrack>& RotationData,
										   const TArray<BYTE>& RequiredBones,
										   USkeletalMesh* SkelMesh,
										   FLOAT SequenceLength,
										   FLOAT Time,
										   UBOOL bLooping)
{
	const TArray<FMeshBone>& RefSkel		= SkelMesh->RefSkeleton;
	check( AnimLinkup.BoneToTrackTable.Num() == RefSkel.Num() );

	for ( INT i = 0 ; i < RequiredBones.Num() ; ++i )
	{
		// Map the bone index to sequence track.
		const INT BoneIndex					= RequiredBones(i);
		const INT TrackIndex				= AnimLinkup.BoneToTrackTable(BoneIndex);
		if ( TrackIndex == INDEX_NONE )
		{
			// No track for the bone was found, so use the reference pose.
			OutAtoms(BoneIndex)	= FBoneAtom( RefSkel(BoneIndex).BonePos.Orientation, RefSkel(BoneIndex).BonePos.Position, 1.f );
		}
		else
		{
			// Build the bone pose using the key data.
			UAnimSequence::ReconstructBoneAtom( OutAtoms(BoneIndex), TranslationData(TrackIndex), RotationData(TrackIndex), SequenceLength, Time, bLooping );
		}

		// Apply quaternion fix for ActorX-exported quaternions.
		if( BoneIndex > 0 ) 
		{
			OutAtoms(BoneIndex).Rotation.W *= -1.0f;
		}
	}
}

void FAnimationUtils::BuildPoseFromReducedKeys(TArray<FBoneAtom>& OutAtoms,
										   UAnimSequence* AnimSeq,
										   const TArray<BYTE>& RequiredBones,
										   USkeletalMesh* SkelMesh,
										   FLOAT Time,
										   UBOOL bLooping)
{
	// Get/create a linkup cache for the mesh to compress against.
	UAnimSet* AnimSet						= AnimSeq->GetAnimSet();
	const INT AnimLinkupIndex				= AnimSet->GetMeshLinkupIndex( SkelMesh );
	check( AnimLinkupIndex != INDEX_NONE );
	check( AnimLinkupIndex < AnimSet->LinkupCache.Num() );

	FAnimationUtils::BuildPoseFromReducedKeys( OutAtoms,
		AnimSet->LinkupCache( AnimLinkupIndex ),
		AnimSeq->TranslationData,
		AnimSeq->RotationData,
		RequiredBones,
		SkelMesh,
		AnimSeq->SequenceLength,
		Time,
		bLooping );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Defualt animation compression algorithm.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

/**
 * @return		A new instance of the default animation compression algorithm singleton, attached to the root set.
 */
static inline UAnimationCompressionAlgorithm* ConstructDefaultCompressionAlgorithm()
{
	// Algorithm.
	FString DefaultCompressionAlgorithm( TEXT("AnimationCompressionAlgorithm_BitwiseCompressOnly") );
	GConfig->GetString( TEXT("AnimationCompression"), TEXT("DefaultCompressionAlgorithm"), DefaultCompressionAlgorithm, GEngineIni );

	// Rotation compression format.
	AnimationCompressionFormat RotationCompressionFormat = ACF_Float96NoW;
	GConfig->GetInt( TEXT("AnimationCompression"), TEXT("RotationCompressionFormat"), (INT&)RotationCompressionFormat, GEngineIni );
	Clamp( RotationCompressionFormat, ACF_None, ACF_MAX );

	// Translation compression format.
	AnimationCompressionFormat TranslationCompressionFormat = ACF_None;
	GConfig->GetInt( TEXT("AnimationCompression"), TEXT("TranslationCompressionFormat"), (INT&)TranslationCompressionFormat, GEngineIni );
	Clamp( TranslationCompressionFormat, ACF_None, ACF_MAX );

	// Find a class that inherits
	UClass* CompressionAlgorithmClass = NULL;
	for ( TObjectIterator<UClass> It ; It ; ++It )
	{
		UClass* Class = *It;
		if( !(Class->ClassFlags & CLASS_Abstract) && !(Class->ClassFlags & CLASS_Deprecated) )
		{
			if ( Class->IsChildOf(UAnimationCompressionAlgorithm::StaticClass()) && DefaultCompressionAlgorithm == Class->GetName() )
			{
				CompressionAlgorithmClass = Class;
				break;

			}
		}
	}

	if ( !CompressionAlgorithmClass )
	{
		appErrorf( TEXT("Couldn't find animation compression algorithm named %s"), *DefaultCompressionAlgorithm );
	}

	UAnimationCompressionAlgorithm* NewAlgorithm = ConstructObject<UAnimationCompressionAlgorithm>( CompressionAlgorithmClass );
	NewAlgorithm->RotationCompressionFormat = RotationCompressionFormat;
	NewAlgorithm->TranslationCompressionFormat = TranslationCompressionFormat;
	NewAlgorithm->AddToRoot();
	return NewAlgorithm;
}

} // namespace

/**
 * @return		The default animation compression algorithm singleton, instantiating it if necessary.
 */
UAnimationCompressionAlgorithm* FAnimationUtils::GetDefaultAnimationCompressionAlgorithm()
{
	static UAnimationCompressionAlgorithm* SAlgorithm = ConstructDefaultCompressionAlgorithm();
	return SAlgorithm;
}
