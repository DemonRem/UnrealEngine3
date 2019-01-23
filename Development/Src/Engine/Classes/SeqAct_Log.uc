/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_Log extends SequenceAction
	native(Sequence);

cpptext
{
	void Activated();
};

/** Should this message be drawn on the screen as well as placed in the log? */
var() bool bOutputToScreen;

/** Should ObjComment be included in the log? */
var() bool bIncludeObjComment;

/** Time to leave text floating above Target actor */
var() float TargetDuration;

/** Offset to apply to the Target actor location when positioning debug text */
var() vector TargetOffset;

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
	ObjName="Log"
	ObjCategory="Misc"
	bOutputToScreen=TRUE
	bIncludeObjComment=TRUE
	ObjClassVersion=2
	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_String',LinkDesc="String",MinVars=0,bHidden=TRUE)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Float',LinkDesc="Float",MinVars=0,bHidden=TRUE)
	VariableLinks(2)=(ExpectedType=class'SeqVar_Bool',LinkDesc="Bool",MinVars=0,bHidden=TRUE)
	VariableLinks(3)=(ExpectedType=class'SeqVar_Object',LinkDesc="Object",MinVars=0,bHidden=TRUE)
	VariableLinks(4)=(ExpectedType=class'SeqVar_Int',LinkDesc="Int",MinVars=0,bHidden=TRUE)
	VariableLinks(5)=(ExpectedType=class'SeqVar_Object',LinkDesc="Target",PropertyName=Targets)
	TargetDuration=-1.f
}
