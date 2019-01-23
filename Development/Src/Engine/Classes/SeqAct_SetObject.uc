/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_SetObject extends SeqAct_SetSequenceVariable
	native(Sequence);

cpptext
{
	virtual void Activated();
};

/** Default value to use if no variables are linked */
var() Object DefaultValue;

var Object Value;

defaultproperties
{
	ObjName="Object"

	ObjClassVersion=2
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Value",PropertyName=Value)
}
