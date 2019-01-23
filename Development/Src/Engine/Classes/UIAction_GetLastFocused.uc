/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/**
 * This action gets the widget that was last focused for a specified parent.
 */
class UIAction_GetLastFocused extends UIAction
	native(inherit);

/** Parent that we are searching in for the last focused item.  If NULL the scene is used. */
var() UIScreenObject		Parent;

/** The item that was last focused, this var is used for the writeable variable link. */
var() UIScreenObject		LastFocused;

cpptext
{
	/**
	 * Gets the control that was previously focused in the specified parent and stores it in LastFocused.
	 */
	virtual void Activated();
}

DefaultProperties
{
	ObjName="Get Last Focused"
	bAutoTargetOwner=false

	OutputLinks(0)=(LinkDesc="Success")
	OutputLinks(1)=(LinkDesc="Failed")

	VariableLinks.Empty()

	// add a variable link which allows the designer to specify the index to use for data retrieval
	VariableLinks.Add((ExpectedType=class'SeqVar_Object',LinkDesc="Parent",PropertyName=Parent,MaxVars=1))

	// add a variable link which will receive the value of the last focused widget.
	VariableLinks.Add((ExpectedType=class'SeqVar_Object',LinkDesc="Last Focused",PropertyName=LastFocused,bWriteable=true))
}
