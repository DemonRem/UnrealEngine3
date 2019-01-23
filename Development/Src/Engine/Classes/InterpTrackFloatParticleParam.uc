class InterpTrackFloatParticleParam extends InterpTrackFloatBase
	native(Interpolation);

/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

cpptext
{
	// InterpTrack interface
	virtual INT AddKeyframe(FLOAT Time, UInterpTrackInst* TrInst);
	virtual void PreviewUpdateTrack(FLOAT NewPosition, UInterpTrackInst* TrInst);
	virtual void UpdateTrack(FLOAT NewPosition, UInterpTrackInst* TrInst, UBOOL bJump);
	
	//virtual class UMaterial* GetTrackIcon();
}

/** Name of property in the Emitter which this track mill modify over time. */
var()	name		ParamName;

defaultproperties
{
	TrackInstClass=class'Engine.InterpTrackInstFloatParticleParam'
	TrackTitle="Float Particle Param"
}