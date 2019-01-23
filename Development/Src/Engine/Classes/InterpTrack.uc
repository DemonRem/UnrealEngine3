/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Abstract base class for a track of interpolated data. Contains the actual data.
 * The Outer of an InterpTrack is the InterpGroup.
 */

class InterpTrack extends Object
	native
	noexport
	collapsecategories
	hidecategories(Object)
	inherits(FInterpEdInputInterface)
	abstract;


/** FCurveEdInterface virtual function table. */
var private native noexport pointer	CurveEdVTable;

/** Subclass of InterpTrackInst that should be created when instancing this track. */
var	class<InterpTrackInst>	TrackInstClass;

/** Title of track type. */
var		string	TrackTitle;

/** Whether there may only be one of this track in an InterpGroup. */
var		bool	bOnePerGroup; 

/** If this track can only exist inside the Director group. */
var		bool	bDirGroupOnly;

/** Whether or not this track should actually update the target actor. */
var		bool	bDisableTrack;

/** If true, the Actor this track is working on will have BeginAnimControl/FinishAnimControl called on it. */
var		bool	bIsAnimControlTrack;

defaultproperties
{
	TrackInstClass=class'Engine.InterpTrackInst'

	TrackTitle="Track"
}
