/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_ToggleHUD extends SequenceAction;

/**
 * Determines whether this class should be displayed in the list of available ops in the UI's kismet editor.
 *
 * @param	TargetObject	the widget that this SequenceObject would be attached to.
 *
 * @return	TRUE if this sequence object should be available for use in the UI kismet editor
 */
event bool IsValidUISequenceObject( optional UIScreenObject TargetObject )
{
	return true;
}

defaultproperties
{
	ObjName="Toggle HUD"
	ObjCategory="Toggle"
	ObjClassVersion=3

	InputLinks(0)=(LinkDesc="Show")
	InputLinks(1)=(LinkDesc="Hide")
	InputLinks(2)=(LinkDesc="Toggle")

	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Target",PropertyName=Targets)
}
