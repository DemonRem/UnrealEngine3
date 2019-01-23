/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_ToggleHidden extends SeqAct_Toggle;

/**
 * Determines whether this class should be displayed in the list of available ops in the UI's kismet editor.
 *
 * @param	TargetObject	the widget that this SequenceObject would be attached to.
 *
 * @return	TRUE if this sequence object should be available for use in the UI kismet editor
 */
event bool IsValidUISequenceObject( optional UIScreenObject TargetObject )
{
	return false;
}

defaultproperties
{
	ObjName="Toggle Hidden"
	ObjCategory="Toggle"

	InputLinks(0)=(LinkDesc="Hide")
	InputLinks(1)=(LinkDesc="UnHide")
}
