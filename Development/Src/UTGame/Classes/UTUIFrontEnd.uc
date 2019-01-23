/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Base class for frontend scenes, looks for buttonbar and tab control references.
 */
class UTUIFrontEnd extends UTUIScene
	native(UIFrontEnd);

/** Pointer to the button bar for this scene. */
var transient UTUIButtonBar	ButtonBar;

/** Pointer to the tab control for this scene. */
var transient UTUITabControl TabControl;

/** Post initialize callback. */
event PostInitialize()
{
	Super.PostInitialize();

	// Setup handler for input keys
	OnRawInputKey=HandleInputKey;

	// Store a reference to the button bar and tab control.
	ButtonBar = UTUIButtonBar(FindChild('pnlButtonBar', true));
	TabControl = UTUITabControl(FindChild('pnlTabControl', true));

	if(TabControl != none)
	{
		TabControl.OnPageActivated = OnPageActivated;
	}

	// Setup initial button bar
	SetupButtonBar();
}


/** Function that sets up a buttonbar for this scene, automatically routes the call to the currently selected tab of the scene as well. */
function SetupButtonBar()
{
	if(ButtonBar != None)
	{
		ButtonBar.Clear();
	
		if(TabControl != None)
		{
			// Let the current tab page try to setup the button bar
			UTTabPage(TabControl.ActivePage).SetupButtonBar(ButtonBar);
		}
	}
}


/**
 * Called when a new page is activated.
 *
 * @param	Sender			the tab control that activated the page
 * @param	NewlyActivePage	the page that was just activated
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this event.
 */
function OnPageActivated( UITabControl Sender, UITabPage NewlyActivePage, int PlayerIndex )
{
	// Anytime the tab page is changed, update the buttonbar.
	SetupButtonBar();
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
	bPauseGameWhileActive=false
}
