
/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This class encapsulates a common interface to extract multiple animation data and blend it.
 */
class AnimNodeSequenceBlendBase extends AnimNodeSequence
	native(Anim)
	abstract;


/**
 * Structure regrouping all we need to extract an animation.
 */
struct native AnimInfo
{
	/** Animation Name */
	var	const				Name			AnimSeqName;
	
	/** 
	 * Pointer to actual AnimSequence. 
	 * Found from SkeletalMeshComponent using AnimSeqName when you call SetAnim. 
	 */
	var	transient const		AnimSequence	AnimSeq;

	/** 
	 * Bone -> Track mapping info for this player node. 
	 * Index into the LinkupCache array in the AnimSet. 
	 * Found from AnimSet when you call SetAnim. 
	 */
	var	transient const		int				AnimLinkupIndex;
};


/**
 * Structure to define animation blending.
 */
struct native AnimBlendInfo
{
	/** Name of animation sequence*/
	var()	Name		AnimName;

	/** Animation info */
	var		AnimInfo	AnimInfo;

	/** Weight i the blend */
	var		float		Weight;
};

/** Array of animations to blend */
var(Animations)	editfixedsize editinline export Array<AnimBlendInfo>	Anims;

cpptext
{
	// UObject interface
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	virtual void InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent);

	/** AnimSets have been updated, update all animations */
	virtual void AnimSetsUpdated();

	/** make sure animations are up date */
	virtual void CheckAnimsUpToDate();

	/** Lookup animation data for a given animation name */
	void SetAnimInfo(FName InSequenceName, FAnimInfo& InAnimInfo);
	
	/** Extract animations and do the blend. */
	virtual void GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion);
}


defaultproperties
{
	Anims(0)=(Weight=1.0)
	Anims(1)=()
}