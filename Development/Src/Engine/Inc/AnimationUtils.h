/*=============================================================================
	AnimationUtils.h: Skeletal mesh animation utilities.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#ifndef __ANIMATIONUTILS_H__
#define __ANIMATIONUTILS_H__

// Forward declarations.
class UAnimationCompressionAlgorithm;
class UAnimSequence;
class UAnimSet;
class USkeletalMesh;
struct FAnimSetMeshLinkup;
struct FBoneAtom;
struct FMeshBone;
struct FRotationTrack;
struct FTranslationTrack;

/**
 * Encapsulates commonly useful data about bones.
 */
class FBoneData
{
public:
	FQuat		Orientation;
	FVector		Position;
	/** Bone name. */
	FName		Name;
	/** Direct descendants.  Empty for end effectors. */
	TArray<INT> Children;
	/** List of bone indices from parent up to root. */
	TArray<INT>	BonesToRoot;
	/** List of end effectors for which this bone is an ancestor.  End effectors have only one element in this list, themselves. */
	TArray<INT>	EndEffectors;

	/**	@return		Index of parent bone; -1 for the root. */
	INT GetParent() const
	{
		return GetDepth() ? BonesToRoot(0) : -1;
	}
	/**	@return		Distance to root; 0 for the root. */
	INT GetDepth() const
	{
		return BonesToRoot.Num();
	}
	/** @return		TRUE if this bone is an end effector (has no children). */
	UBOOL IsEndEffector() const
	{
		return Children.Num() == 0;
	}
};

/**
 * A collection of useful functions for skeletal mesh animation.
 */
class FAnimationUtils
{
public:
	/**
	 * Builds the local-to-component transformation for all bones.
	 */
	static void BuildComponentSpaceTransforms(TArray<FMatrix>& OutTransforms,
												const TArray<FBoneAtom>& LocalAtoms,
												const TArray<BYTE>& RequiredBones,
												const TArray<FMeshBone>& BoneData);

	/**
	* Builds the local-to-component matrix for the specified bone.
	*/
	static void BuildComponentSpaceTransform(FMatrix& OutTransform,
												INT BoneIndex,
												const TArray<FBoneAtom>& LocalAtoms,
												const TArray<FBoneData>& BoneData);

	static void BuildPoseFromRawSequenceData(TArray<FBoneAtom>& OutAtoms,
												UAnimSequence* AnimSeq,
												const TArray<BYTE>& RequiredBones,
												USkeletalMesh* SkelMesh,
												FLOAT Time,
												UBOOL bLooping);

	static void BuildPoseFromReducedKeys(TArray<FBoneAtom>& OutAtoms,
											const FAnimSetMeshLinkup& AnimLinkup,
											const TArray<FTranslationTrack>& TranslationData,
											const TArray<FRotationTrack>& RotationData,
											const TArray<BYTE>& RequiredBones,
											USkeletalMesh* SkelMesh,
											FLOAT SequenceLength,
											FLOAT Time,
											UBOOL bLooping);

	static void BuildPoseFromReducedKeys(TArray<FBoneAtom>& OutAtoms,
											UAnimSequence* AnimSeq,
											const TArray<BYTE>& RequiredBones,
											USkeletalMesh* SkelMesh,
											FLOAT Time,
											UBOOL bLooping);

	static void BuildSekeltonMetaData(const TArray<FMeshBone>& Skeleton, TArray<FBoneData>& OutBoneData);

	/**
	 * @return		The default animation compression algorithm singleton, instantiating it if necessary.
	 */
	static UAnimationCompressionAlgorithm* GetDefaultAnimationCompressionAlgorithm();
};

#endif // __ANIMATIONUTILS_H__
