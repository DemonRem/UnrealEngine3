/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTTabPage extends UITabPage
	native(UI);

/** If true, this object require tick */
var bool bRequiresTick;

cpptext
{
	virtual void Tick_Widget(FLOAT DeltaTime){};
}

function OnChildRepositioned( UIScreenObject Sender );


/** Callback allowing the tabpage to setup the button bar for the current scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	// Do nothing by default.
}

/**
 * Provides a hook for unrealscript to respond to input using actual input key names (i.e. Left, Tab, etc.)
 *
 * Called when an input key event is received which this widget responds to and is in the correct state to process.  The
 * keys and states widgets receive input for is managed through the UI editor's key binding dialog (F8).
 *
 * This delegate is called BEFORE kismet is given a chance to process the input.
 *
 * @param	EventParms	information about the input event.
 *
 * @return	TRUE to indicate that this input key was processed; no further processing will occur on this input key event.
 */
function bool HandleInputKey( const out InputEventParameters EventParms )
{
	return false;
}


defaultproperties
{
}
