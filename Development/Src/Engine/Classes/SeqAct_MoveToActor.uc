/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
////////// OBSOLETE //////////////
class SeqAct_MoveToActor extends SequenceAction
	native(Sequence)
	deprecated;

/** Should this move be interruptable? */
var() bool bInterruptable;

cpptext
{
	virtual void PostLoad();
}

defaultproperties
{
	ObjName="Obsolete Move To Actor"
	ObjCategory="Obsolete"

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Destination")
}
