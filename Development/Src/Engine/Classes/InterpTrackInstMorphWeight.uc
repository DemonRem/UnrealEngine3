/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class InterpTrackInstMorphWeight extends InterpTrackInst
	native(Interpolation);

cpptext
{
	virtual void RestoreActorState(UInterpTrack* Track);
}