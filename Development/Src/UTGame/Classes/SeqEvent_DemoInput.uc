/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/**
 * Demonstrates how to write a kismet event which should be activated by some input event.
 */
class SeqEvent_DemoInput extends SequenceEvent;

/** the ID of the controller that must press the button, or -1 if anybody can do it */
var() int ControllerID;
/** the name of the button that must be pushed */
var() name ButtonName;
/** the action the user must perform with that button */
var() EInputEvent InputEvent;
/** whether or not we should eat a matching keypress or let it continue on in the input chain */
var() bool bTrapInput;

/** checks if the given input matches what we're looking for, and if so activates
 * @return true if the input should be discarded now or false if it should be allowed to continue through the input chain
 */
function bool CheckInput(PlayerController Player, int CheckControllerID, name CheckButtonName, EInputEvent CheckInputEvent)
{
	if ( CheckButtonName == ButtonName && CheckInputEvent == InputEvent && (ControllerID == -1 || CheckControllerID == ControllerID) &&
		CheckActivate(Player, Player) )
	{
		return bTrapInput;
	}
	else
	{
		return false;
	}
}

defaultproperties
{
	ObjName="Input"
	ObjCategory="DemoGame"
	bTrapInput=true
	ControllerID=-1
}
