/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_MidGameMenu extends UTUIScene_Hud;

var transient UTUIButtonBar		ButtonBar;
var transient UITabControl		TabControl;


/**
 * Setup the delegates for the scene and cache all of the various UI Widgets
 */
event PostInitialize( )
{
	Super.PostInitialize();

	// Store a reference to the button bar.
	ButtonBar = UTUIButtonBar(FindChild('ButtonBar', true));
	ButtonBar.ClearButton(0);
	ButtonBar.ClearButton(1);

	// Find the tab control

	TabControl = UITabControl( FindChild('TabControl',true) );

	if ( TabControl != none )
	{
		TabControl.OnPageActivated = OnPageActivated;
	}

	// Setup initial button bar
	SetupButtonBar();
}


/** Function that sets up a buttonbar for this scene, automatically routes the call to the currently selected tab of the scene as well. */
function SetupButtonBar()
{
	if(ButtonBar != none)
	{

		ButtonBar.Clear();
	    ButtonBar.SetButton(0, "<Strings:UTGameUI.ButtonCallouts.Back>", ButtonBarBack);

		if(TabControl != none)
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
 * Back was selected, exit the menu
 * @Param	InButton			The button that selected
 * @Param	InPlayerIndex		Index of the local player that made the selection
 */

function bool ButtonBarBack(UIScreenObject InButton, int InPlayerIndex)
{
	SceneClient.CloseScene(self);
	return true;
}


defaultproperties
{
	SceneInputMode=INPUTMODE_ActiveOnly
	SceneRenderMode=SPLITRENDER_Fullscreen
	bDisplayCursor=true
	bRenderParentScenes=false
	bAlwaysRenderScene=true
	bCloseOnLevelChange=true
}
