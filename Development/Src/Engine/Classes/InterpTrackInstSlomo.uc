/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class InterpTrackInstSlomo extends InterpTrackInst
	native(Interpolation);


/** Backup of initial LevelInfo TimeDilation setting when interpolation started. */
var	float	OldTimeDilation;

cpptext
{
	// InterpTrackInst interface
	virtual void SaveActorState(UInterpTrack* Track);
	virtual void RestoreActorState(UInterpTrack* Track);
	virtual void InitTrackInst(UInterpTrack* Track);
	virtual void TermTrackInst(UInterpTrack* Track);
}
