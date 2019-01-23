/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_ActivateRemoteEvent extends SequenceAction
	native(Sequence);

cpptext
{
	void Activated();
}

/** Instigator to use in the event */
var() Actor Instigator;

/** Name of the event to activate */
var() Name EventName;

defaultproperties
{
	ObjName="Activate Remote Event"
	ObjCategory="Event"
	ObjClassVersion=2

	EventName=DefaultEvent

	OutputLinks.Empty

	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Instigator",PropertyName=Instigator)
}
