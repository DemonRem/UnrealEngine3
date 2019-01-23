/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_SetString extends SeqAct_SetSequenceVariable
	native(Sequence);

cpptext
{
	void Activated()
	{
		// assign the new value
		Target = Value;
	}
};

/** Target property use to write to */
var String Target;

/** Value to apply */
var() String Value;

defaultproperties
{
	ObjName="String"
	ObjClassVersion=1

	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_String',LinkDesc="Target",bWriteable=true,PropertyName=Target)
	VariableLinks(1)=(ExpectedType=class'SeqVar_String',LinkDesc="Value",PropertyName=Value)
}
