/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Join Game scene for UT3.
 */
class UTUIFrontEnd_JoinGame extends UTUIFrontEnd;

/** Tab page references for this scene. */
var UTUITabPage_ServerBrowser ServerBrowserTab;
var UTUITabPage_ServerFilter ServerFilterTab;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();

	// Grab a reference to the server filter tab.
	ServerFilterTab = UTUITabPage_ServerFilter(FindChild('pnlServerFilter', true));

	if(ServerFilterTab != none)
	{
		TabControl.InsertPage(ServerFilterTab, 0, INDEX_NONE, true);
		ServerFilterTab.OnAcceptOptions = OnServerFilter_AcceptOptions;
	}

	// Grab a reference to the server browser tab.
	ServerBrowserTab = UTUITabPage_ServerBrowser(FindChild('pnlServerBrowser', true));

	if(ServerBrowserTab != none)
	{
		TabControl.InsertPage(ServerBrowserTab, 0, INDEX_NONE, false);
		ServerBrowserTab.OnBack = OnServerBrowser_Back;
	}

	// Let the currently active page setup the button bar.
	SetupButtonBar();
}

/** Sets up the button bar for the scene. */
function SetupButtonBar()
{
	if(ButtonBar != None)
	{
		ButtonBar.Clear();

		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Next>", OnButtonBar_Next);
	
		if(TabControl != None)
		{
			// Let the current tab page try to setup the button bar
			UTTabPage(TabControl.ActivePage).SetupButtonBar(ButtonBar);
		}
	}
}



/** Shows the previous tab page, if we are at the first tab, then we close the scene. */
function ShowPrevTab()
{
	if(TabControl.ActivatePreviousPage(0,true,false)==false)
	{
		CloseScene(self);
	}
}

/** Shows the next tab page, if we are at the last tab, then we start the game. */
function ShowNextTab()
{
	TabControl.ActivateNextPage(0,true,false);
}

/** Called when the user accepts their filter settings and wants to go to the server browser. */
function OnServerFilter_AcceptOptions(UIScreenObject InObject, int PlayerIndex)
{
	ShowNextTab();

	// Start a game search
	ServerBrowserTab.RefreshServerList(PlayerIndex);
}

/** Called when the user wants to back out of the server browser. */
function OnServerBrowser_Back()
{
	ShowPrevTab();
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_Next(UIScreenObject InButton, int PlayerIndex)
{
	ShowNextTab();

	return true;
}


function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	ShowPrevTab();

	return true;
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
	local bool bResult;
	local UTTabPage CurrentTabPage;

	// Let the tab page's get first chance at the input
	CurrentTabPage = UTTabPage(TabControl.ActivePage);
	bResult=CurrentTabPage.HandleInputKey(EventParms);

	// If the tab page didn't handle it, let's handle it ourselves.
	if(bResult==false)
	{
		if(EventParms.EventType==IE_Released)
		{
			if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
			{
				ShowPrevTab();
				bResult=true;
			}
		}
	}

	return bResult;
}
