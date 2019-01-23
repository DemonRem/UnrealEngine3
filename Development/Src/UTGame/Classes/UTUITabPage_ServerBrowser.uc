/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a server browser.
 */

class UTUITabPage_ServerBrowser extends UTTabPage
	placeable;

const SERVERBROWSER_SERVERTYPE_LAN = 0;
const SERVERBROWSER_SERVERTYPE_UNRANKED = 1;
const SERVERBROWSER_SERVERTYPE_RANKED = 2;

/** Reference to the server list. */
var transient UIList						ServerList;

/** Reference to a label to display when refreshing. */
var transient UIObject						RefreshingLabel;

/** Reference to the search datastore. */
var transient UIDataStore_OnlineGameSearch	SearchDataStore;

/** Reference to the string list datastore. */
var transient UTUIDataStore_StringList StringListDataStore;

/** World info used for building URL to join games. */
var transient WorldInfo						CachedWorldInfo;

/** Cached online subsystem pointer */
var transient OnlineSubsystem OnlineSub;

/** Cached game interface pointer */
var transient OnlineGameInterface GameInterface;

/** Go back delegate for this page. */
delegate transient OnBack();

/** PostInitialize event - Sets delegates for the page. */
event PostInitialize( )
{
	Super.PostInitialize();

	// Find the server list.
	ServerList = UIList(FindChild('lstServers', true));
	if(ServerList != none)
	{
		ServerList.OnSubmitSelection = OnServerList_SubmitSelection;
		ServerList.OnValueChanged = OnServerList_ValueChanged;
	}

	// Get reference to the refreshing/searching label.
	RefreshingLabel = FindChild('lblRefreshing', true);
	RefreshingLabel.SetVisibility(false);

	// Get a reference to the datastore we are working with.
	// @todo: This should probably come from the list.
	SearchDataStore = UIDataStore_OnlineGameSearch(GetCurrentUIController().DataStoreManager.FindDataStore('UTGameSearch'));
	StringListDataStore = UTUIDataStore_StringList(GetCurrentUIController().DataStoreManager.FindDataStore('UTStringList'));

	// Store a reference to the game interface
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		GameInterface = OnlineSub.GameInterface;
	}

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.JoinGame.Servers>");
}

/** Sets buttons for the scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.JoinServer>", OnButtonBar_JoinServer);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Refresh>", OnButtonBar_Refresh);
}

/** Joins the currently selected server. */
function JoinServer(int InPlayerIndex)
{
	local int ControllerId;
	local int CurrentSelection;
	local OnlineGameSearchResult GameToJoin;
	local UTUIScene OwnerUTScene;

	CurrentSelection = ServerList.GetCurrentItem();
	ControllerId = GetCurrentUIController().GetPlayerControllerId(InPlayerIndex);

	if(CurrentSelection >= 0)
	{
		if(GameInterface != None)
		{
			if(SearchDataStore.GetSearchResultFromIndex(CurrentSelection, GameToJoin))
			{
				`Log("UTUITabPage_ServerBrowser::JoinServer - Joining Search Result " $ CurrentSelection);
			
				// Play the startgame sound
				PlayUISound('StartGame');	

				if (GameToJoin.GameSettings != None)
				{
					OwnerUTScene = UTUIScene(GetScene());
					CachedWorldInfo = OwnerUTScene.GetWorldInfo();

					// Set the delegate for notification
					GameInterface.AddJoinOnlineGameCompleteDelegate(OnJoinGameComplete);

					// Start the async task
					if (!GameInterface.JoinOnlineGame(ControllerId,GameToJoin))
					{
						OnJoinGameComplete(false);
					}
				}
				else
				{
					`Log("UTUITabPage_ServerBrowser::JoinServer - Failed to join game because of a NULL GameSettings object in the search result.");
					OnJoinGameComplete(false);
				}
			}
			else
			{
				`Log("UTUITabPage_ServerBrowser::JoinServer - Unable to get search result for index "$CurrentSelection);
			}
		}
		else
		{
			`Log("UTUITabPage_ServerBrowser::JoinServer - Unable to join game, GameInterface is NULL!");
		}
	}
}

/** Callback for when the join completes. */
function OnJoinGameComplete(bool bSuccessful)
{
	local string URL;

	`Log("UTUITabPage_ServerBrowser::OnJoinGameComplete: bSuccessful=" $ bSuccessful);

	if (bSuccessful)
	{
		// Figure out if we have an online subsystem registered
		if (GameInterface != None)
		{
			// Get the platform specific information
			if (GameInterface.GetResolvedConnectString(URL))
			{
				// Call the game specific function to appending/changing the URL
				URL = BuildJoinURL(URL);

				// @TODO: This is only temporary
				URL = URL $ "?name=" $ UTUIScene(GetScene()).GetPlayerName();

				`Log("UTUITabPage_ServerBrowser::OnJoinGameComplete - Join Game Successful, Traveling: "$URL$"");
				// Get the resolved URL and build the part to start it
				CachedWorldInfo.ConsoleCommand(URL);
			}

			// Remove the delegate from the list
			GameInterface.ClearJoinOnlineGameCompleteDelegate(OnJoinGameComplete);
		}
	}
	CachedWorldInfo = None;
}

/**
 * Builds the string needed to join a game from the resolved connection:
 *		"open 172.168.0.1"
 *
 * NOTE: Overload this method to modify the URL before exec-ing it
 *
 * @param ResolvedConnectionURL the platform specific URL information
 *
 * @return the final URL to use to open the map
 */
function string BuildJoinURL(string ResolvedConnectionURL)
{
	return "open "$ResolvedConnectionURL;
}

/** Refreshes the server list. */
function RefreshServerList(int InPlayerIndex)
{
	local OnlineGameSearch GameSearch;
	local int ValueIndex;

	// Play the refresh sound
	PlayUISound('RefreshServers');	

	// Get current filter from the string list datastore
	GameSearch = SearchDataStore.GetCurrentGameSearch();

	// Get the match type based on the platform.
	if(UTUIScene(GetScene()).GetPlatform()==UTPlatform_360)
	{
		ValueIndex = StringListDataStore.GetCurrentValueIndex('MatchType360');
	}
	else
	{
		ValueIndex = StringListDataStore.GetCurrentValueIndex('MatchType');
	}

	switch(ValueIndex)
	{
	case SERVERBROWSER_SERVERTYPE_LAN:
		`Log("UTUITabPage_ServerBrowser::RefreshServerList - Searching for a LAN match.");
		GameSearch.bIsLanQuery=TRUE;
		GameSearch.bUsesArbitration=FALSE;
		break;
	case SERVERBROWSER_SERVERTYPE_UNRANKED:
		`Log("UTUITabPage_ServerBrowser::RefreshServerList - Searching for an unranked match.");
		GameSearch.bIsLanQuery=FALSE;
		GameSearch.bUsesArbitration=FALSE;
		break;
	case SERVERBROWSER_SERVERTYPE_RANKED:
		`Log("UTUITabPage_ServerBrowser::RefreshServerList - Searching for a ranked match.");
		GameSearch.bIsLanQuery=FALSE;
		GameSearch.bUsesArbitration=TRUE;
		break;
	}

	// Add a delegate for when the search completes.  We will use this callback to do any post searching work.
	RefreshingLabel.SetVisibility(true);
	GameInterface.AddFindOnlineGamesCompleteDelegate(OnFindOnlineGamesComplete);

	// Start a search
	if(SearchDataStore.SubmitGameSearch(GetCurrentUIController().GetPlayerControllerId(InPlayerIndex))==false)
	{
		// Search failed to start, so clear our delegate.
		GameInterface.ClearFindOnlineGamesCompleteDelegate(OnFindOnlineGamesComplete);
	}
}

/**
 * Delegate fired when the search for an online game has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
function OnFindOnlineGamesComplete(bool bWasSuccessful)
{
	// Hide refreshing label.
	RefreshingLabel.SetVisibility(false);

	// Clear delegate
	GameInterface.ClearFindOnlineGamesCompleteDelegate(OnFindOnlineGamesComplete);
}

/** Refreshes the game details list using the currently selected item in the server list. */
function RefreshDetailsList()
{

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

	bResult=false;

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_X')
		{
			RefreshServerList(EventParms.PlayerIndex);
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}

/** ButtonBar - JoinServer */
function bool OnButtonBar_JoinServer(UIScreenObject InButton, int InPlayerIndex)
{
	JoinServer(InPlayerIndex);
	return true;
}

/** ButtonBar - Back */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();
	return true;
}

/** ButtonBar - Refresh */
function bool OnButtonBar_Refresh(UIScreenObject InButton, int InPlayerIndex)
{
	RefreshServerList(InPlayerIndex);
	return true;
}

/** Server List - Submit Selection. */
function OnServerList_SubmitSelection( UIList Sender, int PlayerIndex )
{
	JoinServer(PlayerIndex);
}

/** Server List - Value Changed. */
function OnServerList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	RefreshDetailsList();
}
