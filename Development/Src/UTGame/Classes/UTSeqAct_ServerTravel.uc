/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqAct_ServerTravel extends SequenceAction
	native;

var() string TravelURL;

cpptext
{
	virtual void OnReceivedImpulse( class USequenceOp* ActivatorOp, INT InputLinkIndex );
};

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
	ObjCategory="Level"
	ObjName="Server Travel"

}
