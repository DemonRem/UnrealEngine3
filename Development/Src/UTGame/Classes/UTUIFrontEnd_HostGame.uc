/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Host Game scene for UT3.  Contains the host game flow.
 */
class UTUIFrontEnd_HostGame extends UTUIFrontEnd;

const SERVERTYPE_LAN = 0;
const SERVERTYPE_UNRANKED = 1;
const SERVERTYPE_RANKED = 2;

//@todo: This should probably be INI set.
const MAXIMUM_PLAYER_COUNT = 8;

/** Name of the game settings datastore that we will use to create the online game. */
var() name DataStoreName;

/** Reference to the settings datastore that we will use to create the game. */
var transient UIDataStore_OnlineGameSettings	SettingsDataStore;

/** Tab page references for this scene. */
var transient UTUITabPage_MapSelection MapTab;
var transient UTUITabPage_GameModeSelection GameModeTab;
var transient UTUITabPage_Options ServerSettingsTab;
var transient UTUITabPage_Options GameSettingsTab;
var transient UTUITabPage_Mutators MutatorsTab;

/** Reference to the string list datastore. */
var transient UTUIDataStore_StringList StringListDataStore;

/** Temporary members until we get the game settings datastore up and running .*/
var transient string  MapName;
var transient string  GameMode;
var transient string  Mutators;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();

	// Get a reference to the settings datastore.
	SettingsDataStore = UIDataStore_OnlineGameSettings(FindDataStore(DataStoreName));
	StringListDataStore = UTUIDataStore_StringList(FindDataStore('UTStringList'));

	// Game mode
	GameModeTab = UTUITabPage_GameModeSelection(FindChild('pnlGameModeSelection', true));

	if(GameModeTab != none)
	{
		GameModeTab.OnGameModeSelected = OnGameModeSelected;
		TabControl.InsertPage(GameModeTab, 0, INDEX_NONE, true);
	}

	// Map
	MapTab = UTUITabPage_MapSelection(FindChild('pnlMapSelection', true));

	if(MapTab != none)
	{
		MapTab.OnMapSelected = OnMapSelected;
		TabControl.InsertPage(MapTab, 0, INDEX_NONE, false);
	}

	// Server Settings
	ServerSettingsTab = UTUITabPage_Options(FindChild('pnlServerSettings', true));

	if(ServerSettingsTab != none)
	{
		ServerSettingsTab.OnAcceptOptions = OnAcceptServerSettings;
		ServerSettingsTab.SetVisibility(false);
		ServerSettingsTab.SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_ServerSettings>");
		TabControl.InsertPage(ServerSettingsTab, 0, INDEX_NONE, false);
	}

	// Game Settings
	GameSettingsTab = UTUITabPage_Options(FindChild('pnlGameSettings', true));

	if(GameSettingsTab != none)
	{
		GameSettingsTab.OnAcceptOptions = OnAcceptGameSettings;
		GameSettingsTab.SetVisibility(false);
		TabControl.InsertPage(GameSettingsTab, 0, INDEX_NONE, false);
	}

	// Mutators
	MutatorsTab = UTUITabPage_Mutators(FindChild('pnlMutators', true));

	if(MutatorsTab != none)
	{
		MutatorsTab.OnAcceptMutators = OnAcceptMutators;
		MutatorsTab.SetVisibility(false);
		TabControl.InsertPage(MutatorsTab, 0, INDEX_NONE, false);
	}

	// Set defaults
	SetDataStoreStringValue("<UTGameSettings:NumBots>", "0");	// @todo: This will be unnecessary when we move to seperate settings classes.
	SetDataStoreStringValue("<Registry:SelectedGameMode>", GameMode);
	MapTab.MapList.RefreshSubscriberValue();
	GetDataStoreStringValue("<Registry:EnabledMutators>", Mutators);

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
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.QuickStartGame>", OnButtonBar_StartGame);

		if(GetPlatform()==UTPlatform_PC)
		{
			ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.StartDedicated>", OnButtonBar_StartDedicated);
		}

		if(TabControl != None)
		{
			// Let the current tab page try to setup the button bar
			UTTabPage(TabControl.ActivePage).SetupButtonBar(ButtonBar);
		}
	}
}

/** Creates the online game and travels to the map we are hosting a server on. */
function CreateOnlineGame(int PlayerIndex)
{
	local OnlineSubsystem OnlineSub;
	local OnlineGameInterface GameInterface;
	local OnlineGameSettings GameSettings;
	local int ValueIndex;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the game interface to verify the subsystem supports it
		GameInterface = OnlineSub.GameInterface;
		if (GameInterface != None)
		{
			// Play the startgame sound
			PlayUISound('StartGame');	

			// Force widgets to save out their values to their respective datastores.
			SaveSceneDataValues(FALSE);

			// Setup server options based on server type.
			GameSettings = SettingsDataStore.GetCurrentGameSettings();

			if(GetPlatform()==UTPlatform_360)
			{
				ValueIndex = StringListDataStore.GetCurrentValueIndex('ServerType360');
			}
			else
			{
				ValueIndex = StringListDataStore.GetCurrentValueIndex('ServerType');
			}

			switch(ValueIndex)
			{
			case SERVERTYPE_LAN:
				`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - Setting up a LAN match.");
				GameSettings.bIsLanMatch=TRUE;
				GameSettings.bUsesArbitration=FALSE;
				break;
			case SERVERTYPE_UNRANKED:
				`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - Setting up an unranked match.");
				GameSettings.bIsLanMatch=FALSE;
				GameSettings.bUsesArbitration=FALSE;
				break;
			case SERVERTYPE_RANKED:
				`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - Setting up a ranked match.");
				GameSettings.bIsLanMatch=FALSE;
				GameSettings.bUsesArbitration=TRUE;
				GameSettings.NumPrivateConnections = 0;
				break;
			}

			GameSettings.NumPublicConnections = MAXIMUM_PLAYER_COUNT - GameSettings.NumPrivateConnections;

			// Set the map name we are playing on.
			SetDataStoreStringValue("<UTGameSettings:CustomMapName>", MapName);
			SetDataStoreStringValue("<UTGameSettings:CustomGameMode>", GameMode);

			// Create the online game
			GameInterface.AddCreateOnlineGameCompleteDelegate(OnGameCreated);

			if(SettingsDataStore.CreateGame(GetWorldInfo(), GetPlayerControllerId(PlayerIndex))==FALSE)
			{
				GameInterface.ClearCreateOnlineGameCompleteDelegate(OnGameCreated);
				`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - Failed to create online game.");
			}
		}
		else
		{
			`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - No GameInterface found.");
		}
	}
	else
	{
		`Log("UTUIFrontEnd_HostGame::CreateOnlineGame - No OnlineSubSystem found.");
	}
}

/** Callback for when the game is finish being created. */
function OnGameCreated(bool bWasSuccessful)
{
	local OnlineGameSettings GameSettings;
	local string TravelURL;
	local OnlineSubsystem OnlineSub;
	local OnlineGameInterface GameInterface;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the game interface to verify the subsystem supports it
		GameInterface = OnlineSub.GameInterface;
		if (GameInterface != None)
		{
			// Clear the delegate we set.
			GameInterface.ClearCreateOnlineGameCompleteDelegate(OnGameCreated);

			// If we were successful, then travel.
			if(bWasSuccessful)
			{
				// Setup server options based on server type.
				GameSettings = SettingsDataStore.GetCurrentGameSettings();

				// Do the server travel.
				GameSettings.BuildURL(TravelURL);
				TravelURL = TravelURL $ "?game=" $ GameMode;

				// Append any mutators
				if(Len(Mutators) > 0)
				{
					TravelURL = TravelURL $ "?mutator=" $ Mutators;
				}

				// Append Extra Common Options
				TravelURL = TravelURL $ GetCommonOptionsURL();

				TravelURL = "open " $ MapName $ TravelURL $ "?listen";

				`Log("UTUIFrontEnd_HostGame::OnGameCreated - Game Created, Traveling: " $ TravelURL);

				ConsoleCommand(TravelURL);
			}
			else
			{
				`Log("UTUIFrontEnd_HostGame::OnGameCreated - Game Creation Failed.");
			}
		}
		else
		{
			`Log("UTUIFrontEnd_HostGame::OnGameCreated - No GameInterface found.");
		}
	}
	else
	{
		`Log("UTUIFrontEnd_HostGame::OnGameCreated - No OnlineSubSystem found.");
	}
}

function OnMapSelected(string InMapName, bool bSelectionSubmitted)
{
	MapName = InMapName;

	if(bSelectionSubmitted)
	{
		ShowNextTab();
	}
}

function OnGameModeSelected(string InGameMode, string InDefaultMap, bool bSelectionSubmitted)
{
	GameMode = InGameMode;
	MapName = InDefaultMap;

	// Refresh map list.
	SetDataStoreStringValue("<Registry:SelectedGameMode>", GameMode);
	MapTab.MapList.RefreshSubscriberValue();

	if(bSelectionSubmitted)
	{
		ShowNextTab();
	}
}

function OnAcceptServerSettings(UIScreenObject InObject, int PlayerIndex)
{
	ShowNextTab();
}

function OnAcceptGameSettings(UIScreenObject InObject, int PlayerIndex)
{
	ShowNextTab();
}

function OnAcceptMutators(string InEnabledMutators)
{
	Mutators = InEnabledMutators;
	SetDataStoreStringValue("<Registry:EnabledMutators>", Mutators);

	ShowNextTab();
}

/** Attempts to start a dedicated server. */
function OnStartDedicated()
{
	// Make sure the user wants to start the game.
	local UTUIScene_MessageBox MessageBoxReference;
	local array<string> MessageBoxOptions;

	MessageBoxReference = GetMessageBoxScene();

	if(MessageBoxReference != none)
	{
		MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.StartDedicated>");
		MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.Cancel>");

		MessageBoxReference.SetPotentialOptions(MessageBoxOptions);
		MessageBoxReference.Display("<Strings:UTGameUI.MessageBox.StartDedicated_Message>", "<Strings:UTGameUI.MessageBox.StartDedicated_Title>", OnStartDedicated_Confirm);
	}
}

/** Callback for the start game message box. */
function OnStartDedicated_Confirm(int SelectedOption, int PlayerIndex)
{
	local OnlineGameSettings GameSettings;
	local string TravelURL;

	// If they want to start the game, then create the online game.
	if(SelectedOption==0)
	{
		// Setup server options based on server type.
		GameSettings = SettingsDataStore.GetCurrentGameSettings();

		// @todo: Is this the correct URL to use?
		GameSettings.BuildURL(TravelURL);
		TravelURL = TravelURL $ "?game=" $ GameMode;
		TravelURL = TravelURL $ "?mutator=" $ Mutators;
		TravelURL = MapName $ TravelURL;

		// Setup dedicated server
		StartDedicatedServer(TravelURL);
	}
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_StartGame(UIScreenObject InButton, int PlayerIndex)
{
	OnStartGame();

	return true;
}

function bool OnButtonBar_StartDedicated(UIScreenObject InButton, int PlayerIndex)
{
	OnStartDedicated();

	return true;
}


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
	if(TabControl.ActivateNextPage(0,true,false)==false)
	{
		OnStartGame();
	}
}

/** Attempts to start an instant action game. */
function OnStartGame()
{
	// Make sure the user wants to start the game.
	local UTUIScene_MessageBox MessageBoxReference;
	local array<string> MessageBoxOptions;

	MessageBoxReference = GetMessageBoxScene();

	if(MessageBoxReference != none)
	{
		MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.StartGame>");
		MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.Cancel>");

		MessageBoxReference.SetPotentialOptions(MessageBoxOptions);
		MessageBoxReference.Display("<Strings:UTGameUI.MessageBox.StartGame_Message>", "<Strings:UTGameUI.MessageBox.StartGame_Title>", OnStartGame_Confirm);
	}
}

/** Callback for the start game message box. */
function OnStartGame_Confirm(int SelectedOption, int PlayerIndex)
{
	// If they want to start the game, then create the online game.
	if(SelectedOption==0)
	{
		CreateOnlineGame(PlayerIndex);
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

	// Let the currently active tab try to handle input first.
	bResult=UTTabPage(TabControl.ActivePage).HandleInputKey(EventParms);

	if(bResult==false && EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_Start')
		{
			OnStartGame();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			ShowPrevTab();
			bResult=true;
		}
	}

	return bResult;
}

defaultproperties
{
	DataStoreName="UTGameSettings"
	MapName="DM-Deck"
	GameMode="UTGame.UTDeathmatch"
}
