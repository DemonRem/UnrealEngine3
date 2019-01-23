/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Allows the user to pick a bunch of filters and then jump straight into a game.
 */
class UTUIFrontEnd_QuickMatch extends UTUIFrontEnd;

/** Tab page references for this scene. */
var UTUITabPage_ServerFilter ServerFilterTab;
var UTUITabPage_FindQuickMatch FindQuickMatchTab;

/** Whether or not we are currently searching. */
var bool bSearching;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();

	// Grab a reference to the server filter tab.
	ServerFilterTab = UTUITabPage_ServerFilter(FindChild('pnlServerFilter', true));
	ServerFilterTab.OnAcceptOptions = OnServerFilter_AcceptOptions;

	FindQuickMatchTab = UTUITabPage_FindQuickMatch(FindChild('pnlFindQuickMatch', true));
	FindQuickMatchTab.OnBack = OnBack;
	FindQuickMatchTab.SetVisibility(false);


	// Let the currently active page setup the button bar.
	SetupButtonBar();
}

/** Setup the button bar for this scene. */
function SetupButtonBar()
{
	if(bSearching)
	{
		FindQuickMatchTab.SetupButtonBar(ButtonBar);
	}
	else
	{
		ButtonBar.Clear();
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.FindMatch>", OnButtonBar_FindMatch);
	}
}

/** Starts searching for a game. */
function StartSearch()
{
	if(bSearching==false)
	{
		bSearching = true;

		SetupButtonBar();

		ServerFilterTab.SetVisibility(false);
		FindQuickMatchTab.SetVisibility(true);
		FindQuickMatchTab.RefreshServerList(GetPlayerIndex());
	}
}

/** Callback for when the user wants to go back or cancel the current search. */
function OnBack()
{
	if(bSearching)
	{
		bSearching = false;
		ServerFilterTab.SetVisibility(true);
		FindQuickMatchTab.SetVisibility(false);
		SetupButtonBar();
	}
	else
	{
		CloseScene(self);
	}
}

/** Called when the user accepts their filter settings and wants to go to the server browser. */
function OnServerFilter_AcceptOptions(UIScreenObject InObject, int PlayerIndex)
{
	StartSearch();
}

/** Buttonbar callbacks. */
function bool OnButtonBar_FindMatch(UIScreenObject InButton, int PlayerIndex)
{
	StartSearch();

	return true;
}


function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	OnBack();

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
				OnBack();
				bResult=true;
			}
		}
	}

	return bResult;
}
