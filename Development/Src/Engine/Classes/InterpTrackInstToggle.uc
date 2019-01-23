/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class InterpTrackInstToggle extends InterpTrackInst
	native(Interpolation);

var()	ETrackToggleAction	Action;
/** 
 *	Position we were in last time we evaluated.
 *	During UpdateTrack, toggles between this time and the current time will be processed.
 */
var		float				LastUpdatePosition; 

cpptext
{
	/** 
	 */
	virtual void InitTrackInst(UInterpTrack* Track);
}

defaultproperties
{
}
