/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_Teleport extends SequenceAction;

var() bool bUpdateRotation;

defaultproperties
{
	ObjName="Teleport"
	ObjCategory="Actor"
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Destination")
	bUpdateRotation=TRUE
}
