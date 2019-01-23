/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_VelocityLoop extends SeqAct_Latent
	native(Sequence);

cpptext
{
	UBOOL UpdateOp(FLOAT DeltaTime);

	void StartSound();
	void StopSound();
};

var() SoundCue LoopSound;

var bool bPlayingSound;

var transient float TotalVelocity;
var transient array<Actor> SoundSources;

defaultproperties
{
	ObjName="Velocity Loop"
	ObjCategory="DemoGame"

	InputLinks(0)=(LinkDesc="Start")
	InputLinks(1)=(LinkDesc="Stop")
	OutputLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Target")
	VariableLinks(1)=(ExpectedType=class'SeqVar_Float',LinkDesc="Velocity")
}
