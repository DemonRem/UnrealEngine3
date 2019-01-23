/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Slot for Matinee controlled Animation Trees.
 * Each slot will be able to blend a defined number of channels (AnimNodeSequence connections).
 */

class AnimNodeSlot extends AnimNodeBlendBase
	native(Anim)
	hidecategories(Object);

/** True, when we're playing a custom animation */
var const bool		bIsPlayingCustomAnim;
/** save blend out time when playing a one shot animation. */
var const FLOAT		PendingBlendOutTime;
/** Child index playing a custom animation */
var const INT		CustomChildIndex;
/** Child currently active, being blended to */
var const INT		TargetChildIndex;
/** Array of target weights for each child. Size must be the same as the Children array. */
var	Array<FLOAT>	TargetWeight;
/** How long before current blend is complete (ie. target child reaches 100%) */
var	const FLOAT		BlendTimeToGo;

/** SynchNode, used for multiple node synchronization */
var	const transient	AnimNodeSynch	SynchNode;

cpptext
{
	// AnimNode interface
	virtual void InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent);

	/** Update position of given channel */
	virtual void MAT_SetAnimPosition(INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bFireNotifies, UBOOL bLooping);
	/** Update weight of channels */
	virtual void MAT_SetAnimWeights(const FAnimSlotInfo& SlotInfo);
	/** Rename all child nodes upon Add/Remove, so they match their position in the array. */
	virtual void RenameChildConnectors();

	// AnimNode interface
	virtual	void	TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );

	// AnimNodeBlendBase interface
	virtual void	OnAddChild(INT ChildNum);
	virtual void	OnRemoveChild(INT ChildNum);

	virtual INT		GetNumSliders() const { return 0; }

	/**
	 * When requested to play a new animation, we need to find a new child.
	 * We'd like to find one that is unused for smooth blending, 
	 * but that may be a luxury that is not available.
	 */
	INT		FindBestChildToPlayAnim(FName AnimToPlay);

	void	SetActiveChild(INT ChildIndex, FLOAT BlendTime);
}

/**
 * Play a custom animation.
 * Supports many features, including blending in and out.
 *
 * @param	AnimName		Name of animation to play.
 * @param	Rate			Rate animation should be played at.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 *
 * @return	PlayBack length of animation.
 */
final native function float PlayCustomAnim
(
	name	AnimName,
	float	Rate,
	optional	float	BlendInTime,
	optional	float	BlendOutTime,
	optional	bool	bLooping,
	optional	bool	bOverride
);


/**
 * Play a custom animation.
 * Supports many features, including blending in and out.
 *
 * @param	AnimName		Name of animation to play.
 * @param	Duration		duration in seconds the animation should be played.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 */
final native function PlayCustomAnimByDuration
(
	name	AnimName,
	float	Duration,
	optional	float	BlendInTime,
	optional	float	BlendOutTime,
	optional	bool	bLooping,
	optional	bool	bOverride = TRUE
);

/** Returns the Name of the currently played animation or '' otherwise. */
final native function Name GetPlayedAnimation();

/** 
 * Stop playing a custom animation. 
 * Used for blending out of a looping custom animation.
 */
final native function StopCustomAnim(float BlendOutTime);


/**
 * Switch currently played animation to another one.
 */
final native function SetCustomAnim(Name AnimName);


/** Set bCauseActorAnimEnd flag */
final native function SetActorAnimEndNotification(bool bNewStatus);


/** 
 * Returns AnimNodeSequence currently selected for playing animations.
 * Note that calling PlayCustomAnim *may* change which node plays the animation.
 * (Depending on the blend in time, and how many nodes are available, to provide smooth transitions.
 */
final native function AnimNodeSequence GetCustomAnimNodeSeq();


/**
 * Set custom animation root bone options.
 */
final native function SetRootBoneAxisOption
(
 optional ERootBoneAxis AxisX = RBA_Default,
 optional ERootBoneAxis AxisY = RBA_Default,
 optional ERootBoneAxis AxisZ = RBA_Default
 );


/** 
 * Synchronize this animation with others. 
 * @param GroupName	Add node to synchronization group named group name.
 */
final native function AddToSynchGroup(Name GroupName);

defaultproperties
{
	TargetWeight(0)=1.f
	Children(0)=(Name="Source",Weight=1.0)
	Children(1)=(Name="Channel 01")
	bFixNumChildren=FALSE

	NodeName="SlotName"
}
