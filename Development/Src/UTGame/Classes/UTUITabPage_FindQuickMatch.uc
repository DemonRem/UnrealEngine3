/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for finding a quick match.
 */

class UTUITabPage_FindQuickMatch extends UTTabPage
	placeable;

const SERVERBROWSER_SERVERTYPE_LAN = 0;
const SERVERBROWSER_SERVERTYPE_UNRANKED = 1;
const SERVERBROWSER_SERVERTYPE_RANKED = 2;

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
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.CancelSearch>", OnButtonBar_Back);
}

/** Joins the currently selected server. */
function JoinServer(int ResultIndex, int InPlayerIndex)
{
	local int ControllerId;
	local OnlineGameSearchResult GameToJoin;
	local UTUIScene OwnerUTScene;

	ControllerId = GetCurrentUIController().GetPlayerControllerId(InPlayerIndex);

	if(ResultIndex >= 0)
	{
		if(SearchDataStore.GetSearchResultFromIndex(ResultIndex, GameToJoin))
		{
			`Log("UTUITabPage_ServerBrowser::JoinServer - Joining Search Result " $ ResultIndex);
		
			// Play the startgame sound
			PlayUISound('StartGame');	

			if (GameToJoin.GameSettings != None)
			{
				// Grab the game interface to verify the subsystem supports it
				if (GameInterface != None)
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
			}
			else
			{
				OnJoinGameComplete(false);
			}
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
	else
	{
		OnBack();
		UTUIScene(GetScene()).DisplayMessageBox("<Strings:UTGameUI.Errors.UnableToFindMatch_Message>","<Strings:UTGameUI.Errors.UnableToFindMatch_Title>");
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
	ValueIndex = StringListDataStore.GetCurrentValueIndex('MatchType');
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
	local OnlineGameSearchResult GameToJoin;

	// Clear delegate
	GameInterface.ClearFindOnlineGamesCompleteDelegate(OnFindOnlineGamesComplete);

	// Join the game if we found one, otherwise pop up a error message.
	if(bWasSuccessful && SearchDataStore.GetSearchResultFromIndex(0,GameToJoin)==true )
	{
		//@todo: Try to join the first game for now, later we need to choose the 'best' game to join.
		JoinServer(0, 0);
	}
	else
	{
		OnBack();
		UTUIScene(GetScene()).DisplayMessageBox("<Strings:UTGameUI.Errors.JoinGameFailed_Message>","<Strings:UTGameUI.Errors.JoinGameFailed_Title>");
	}
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

	if(IsVisible() && EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}

/** ButtonBar - Back */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();
	return true;
}
