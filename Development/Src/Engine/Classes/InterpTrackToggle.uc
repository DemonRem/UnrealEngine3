class InterpTrackToggle extends InterpTrack
	native(Interpolation);

/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * 
 *	A track containing toggle actions that are triggered as its played back. 
 */

cpptext
{
	// InterpTrack interface
	virtual INT GetNumKeyframes();
	virtual void GetTimeRange(FLOAT& StartTime, FLOAT& EndTime);
	virtual FLOAT GetKeyframeTime(INT KeyIndex);
	virtual INT AddKeyframe(FLOAT Time, UInterpTrackInst* TrInst);
	virtual INT SetKeyframeTime(INT KeyIndex, FLOAT NewKeyTime, UBOOL bUpdateOrder=true);
	virtual void RemoveKeyframe(INT KeyIndex);
	virtual INT DuplicateKeyframe(INT KeyIndex, FLOAT NewKeyTime);
	virtual UBOOL GetClosestSnapPosition(FLOAT InPosition, TArray<INT> &IgnoreKeys, FLOAT& OutPosition);

	virtual void PreviewUpdateTrack(FLOAT NewPosition, UInterpTrackInst* TrInst);
	virtual void UpdateTrack(FLOAT NewPosition, UInterpTrackInst* TrInst, UBOOL bJump);

	/** Get the name of the class used to help out when adding tracks, keys, etc. in UnrealEd.
	* @return	String name of the helper class.*/
	virtual const FString	GetEdHelperClassName() const;

	virtual class UMaterial* GetTrackIcon();

	/** Whether or not this track is allowed to be used on static actors. */
	virtual UBOOL AllowStaticActors() { return TRUE; }

	virtual void DrawTrack(FCanvas* Canvas, INT TrackIndex, INT TrackWidth, INT TrackHeight, FLOAT StartTime, FLOAT PixelsPerSec, TArray<class FInterpEdSelKey>& SelectedKeys);
}

/** Enumeration indicating toggle action	*/
enum ETrackToggleAction
{
	ETTA_Off,
	ETTA_On,
	ETTA_Toggle
};

/** Information for one toggle in the track. */
struct native ToggleTrackKey
{
	var		float				Time;
	var()	ETrackToggleAction	ToggleAction;
};	

/** Array of events to fire off. */
var	array<ToggleTrackKey>	ToggleTrack;

/** If true, events on this track are fired even when jumping forwads through a sequence - for example, skipping a cinematic. */
var() bool	bFireEventsWhenJumpingForwards;

/** 
 *	If true, the track will call ActivateSystem on the emitter each update (the old 'incorrect' behavior).
 *	If false (the default), the System will only be activated if it was previously inactive.
 */
var() bool	bActivateSystemEachUpdate;

defaultproperties
{
	TrackInstClass=class'Engine.InterpTrackInstToggle'
	TrackTitle="Toggle"
	bFireEventsWhenJumpingForwards=true
	bActivateSystemEachUpdate=false
}
