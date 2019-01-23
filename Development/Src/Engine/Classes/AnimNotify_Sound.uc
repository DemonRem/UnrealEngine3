/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AnimNotify_Sound extends AnimNotify
	native(Anim);

var()	SoundCue	SoundCue;
var()	bool		bFollowActor;
var()	Name		BoneName;

cpptext
{
	// AnimNotify interface.
	virtual void Notify( class UAnimNodeSequence* NodeSeq );
}

defaultproperties
{
	bFollowActor=true
}
