/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AnimNodeBlendBase extends AnimNode
	native(Anim)
	hidecategories(Object)
	abstract;

cpptext
{
	// UAnimNode interface
	virtual void InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent );

	/** AnimSets have been updated, update all animations */
	virtual void AnimSetsUpdated();

	virtual	void TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );

	virtual void BuildTickArray(TArray<UAnimNode*>& OutTickArray);

	virtual void GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion);

	/**
	 * Get mirrored bone atoms from desired child index.
	 * Bones are mirrored using the SkelMirrorTable.
	 */
	void GetMirroredBoneAtoms(TArray<FBoneAtom>& Atoms, INT ChildIndex, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion);

	/**
	 * Draws this node in the AnimTreeEditor.
	 *
	 * @param	Canvas			The canvas to use.
	 * @param	bSelected		TRUE if this node is selected.
	 * @param	bShowWeight		If TRUE, show the global percentage weight of this node, if applicable.
	 * @param	bCurves			If TRUE, render links as splines; if FALSE, render links as straight lines.
	 */
	virtual void DrawAnimNode(FCanvas* Canvas, UBOOL bSelected, UBOOL bShowWeight, UBOOL bCurves);
	virtual FString GetNodeTitle();

	virtual FIntPoint GetConnectionLocation(INT ConnType, INT ConnIndex);

	// UAnimNodeBlendBase interface
	/** Calculates total weight of children */
	virtual void SetChildrenTotalWeightAccumulator(const INT Index, const FLOAT NodeTotalWeight);


	/** For debugging. Return the sum of the weights of all children nodes. Should always be 1.0. */
	FLOAT GetChildWeightTotal();

	/** Notification to this blend that a child UAnimNodeSequence has reached the end and stopped playing. Not called if child has bLooping set to true or if user calls StopAnim. */
	virtual void OnChildAnimEnd(UAnimNodeSequence* Child, FLOAT PlayedTime, FLOAT ExcessTime);

	/** A child connector has been added */
	virtual void	OnAddChild(INT ChildNum);
	/** A child connector has been removed */
	virtual void	OnRemoveChild(INT ChildNum);

	/** Rename all child nodes upon Add/Remove, so they match their position in the array. */
	virtual void	RenameChildConnectors();

	/** internal code for GetNodes(); should only be called from GetNodes() or from the GetNodesInternal() of this node's parent */
	virtual void GetNodesInternal(TArray<UAnimNode*>& Nodes);
}

/** Link to a child AnimNode. */
struct native AnimBlendChild
{
	/** Name of link. */
	var()					Name		Name;
	/** Child AnimNode. */
	var	editinline export AnimNode	Anim;
	/** Weight with which this child will be blended in. Sum of all weights in the Children array must be 1.0 */
	var						float		Weight;
	/** Total weight of this connection in the final blend of all animations. */
	var	const				float		TotalWeight;
	/** Is this children currently forwarding root motion? */
	var	const transient		int		    bHasRootMotion;
	/** Extracted Root Motion */
	var	const transient		BoneAtom	RootMotion;

	/**
	 * Whether this child's skeleton should be mirrored.
	 * Do not use this lightly, mirroring is rather expensive.
	 * So minimize the number of times mirroring is done in the tree.
	 */
	var						bool		bMirrorSkeleton;

	/** For editor use. */
	var						int			DrawY;
};

/** Array of children AnimNodes. These will be blended together and the results returned by GetBoneAtoms. */
var	editfixedsize editinline export	array<AnimBlendChild>		Children;

/** Whether children connectors (ie elements of the Children array) may be added/removed. */
var						bool						bFixNumChildren;

native function PlayAnim(bool bLoop = false, float Rate = 1.0f, float StartTime = 0.0f);
native function StopAnim();
