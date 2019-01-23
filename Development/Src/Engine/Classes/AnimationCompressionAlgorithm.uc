/**
 * Baseclass for animation compression algorithms.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class AnimationCompressionAlgorithm extends Object
	abstract
	native(Anim)
	hidecategories(Object);

/** A human-readable name for this modifier; appears in editor UI. */
var		string		Description;

/** Compression algorithms requiring a skeleton should set this value to TRUE. */
var		bool		bNeedsSkeleton;

/** Format for bitwise compression of translation data. */
var		AnimSequence.AnimationCompressionFormat		TranslationCompressionFormat;

/** Format for bitwise compression of rotation data. */
var()	AnimSequence.AnimationCompressionFormat		RotationCompressionFormat;

cpptext
{
public:
	/**
	 * Reduce the number of keyframes and bitwise compress the specified sequence.
	 *
	 * @param	AnimSet		The animset to compress.
	 * @param	SkelMesh	The skeletal mesh against which to compress the animation.  Not needed by all compression schemes.
	 * @param	bOutput		If FALSE don't generate output or compute memory savings.
	 * @return				FALSE if a skeleton was needed by the algorithm but not provided.
	 */
	UBOOL Reduce(class UAnimSequence* AnimSeq, class USkeletalMesh* SkelMesh, UBOOL bOutput);

	/**
	 * Reduce the number of keyframes and bitwise compress all sequences in the specified animation set.
	 *
	 * @param	AnimSet		The animset to compress.
	 * @param	SkelMesh	The skeletal mesh against which to compress the animation.  Not needed by all compression schemes.
	 * @param	bOutput		If FALSE don't generate output or compute memory savings.
	 * @return				FALSE if a skeleton was needed by the algorithm but not provided.
	 */
	UBOOL Reduce(class UAnimSet* AnimSet, class USkeletalMesh* SkelMesh, UBOOL bOutput);

protected:
	/**
	 * Implemented by child classes, this function reduces the number of keyframes in
	 * the specified sequence, given the specified skeleton (if needed).
	 *
	 * @return		TRUE if the keyframe reduction was successful.
	 */
	virtual void DoReduction(class UAnimSequence* AnimSeq, class USkeletalMesh* SkelMesh, const TArray<class FBoneData>& BoneData) PURE_VIRTUAL(UAnimationCompressionAlgorithm::DoReduction,);
}

defaultproperties
{
	Description="None"
	TranslationCompressionFormat=ACF_None
	RotationCompressionFormat=ACF_Float96NoW
}
