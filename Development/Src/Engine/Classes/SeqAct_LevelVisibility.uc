/**
 * SeqAct_LevelVisibility
 *
 * Kismet action exposing associating/ dissociating of levels.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_LevelVisibility extends SeqAct_Latent
	native(Sequence);

/** LevelStreaming object that is going to be associated/ dissociated on request */
var() LevelStreaming Level;

/** LevelStreaming object name */
var() Name LevelName;

cpptext
{
	void Activated();
	UBOOL UpdateOp(FLOAT DeltaTime);
};

defaultproperties
{
	ObjName="Change Level Visibility"
	ObjCategory="Level"
	VariableLinks.Empty
	OutputLinks.Empty
	InputLinks(0)=(LinkDesc="Make Visible")
	InputLinks(1)=(LinkDesc="Hide")
	OutputLinks(0)=(LinkDesc="Finished")
}
