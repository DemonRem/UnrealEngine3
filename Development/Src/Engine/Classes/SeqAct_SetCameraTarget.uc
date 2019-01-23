/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_SetCameraTarget extends SequenceAction
	native(Sequence);

cpptext
{
	void Activated();
};

var transient Actor			CameraTarget;

defaultproperties
{
	ObjName="Set Camera Target"
	ObjCategory="Camera"

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Cam Target")
}
