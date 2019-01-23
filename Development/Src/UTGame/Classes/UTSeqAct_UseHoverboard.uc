/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqAct_UseHoverboard extends SequenceAction;

/** reference to the hoverboard that was spawned, so that the scripter can access it */
var UTVehicle Hoverboard;

defaultproperties
{
	ObjName="Use Hoverboard"
	ObjCategory="Pawn"
	ObjClassVersion=2

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Hoverboard",bWriteable=true,PropertyName=Hoverboard)
}
