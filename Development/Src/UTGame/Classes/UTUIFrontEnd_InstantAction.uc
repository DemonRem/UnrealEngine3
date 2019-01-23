/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Instant Action scene for UT3.
 */
class UTUIFrontEnd_InstantAction extends UTUIFrontEnd;

`include(UTOnlineConstants.uci)

/** Name of the game settings datastore that we will use to create the game. */
var() name DataStoreName;

/** Reference to the settings datastore that we will use to create the game. */
var transient UIDataStore_OnlineGameSettings	SettingsDataStore;

/** Reference to the stringlist datastore that we will use to create the game. */
var transient UTUIDataStore_StringList	StringListDataStore;

/** References to the tab control and pages. */
var transient UTUITabPage_MapSelection MapTab;
var transient UTUITabPage_GameModeSelection GameModeTab;
var transient UTUITabPage_Options GameSettingsTab;
var transient UTUITabPage_Mutators MutatorsTab;

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

	// Get a reference to the stringlist datastore.
	StringListDataStore = UTUIDataStore_StringList(FindDataStore('UTStringList'));


	// Game Mode
	GameModeTab = UTUITabPage_GameModeSelection(FindChild('pnlGameModeSelection', true));
	if(GameModeTab != none)
	{
		GameModeTab.SetVisibility(true);
		GameModeTab.OnGameModeSelected = OnGameModeSelected;
		TabControl.InsertPage(GameModeTab, 0, 0, true);
	}

	// Map
	MapTab = UTUITabPage_MapSelection(FindChild('pnlMapSelection', true));
	if(MapTab != none)
	{
		MapTab.SetVisibility(false);
		MapTab.OnMapSelected = OnMapSelected;
		TabControl.InsertPage(MapTab, 0, 1, false);
	}


	// Game Settings
	GameSettingsTab = UTUITabPage_Options(FindChild('pnlGameSettings', true));
	if(GameSettingsTab != none)
	{
		GameSettingsTab.OnAcceptOptions = OnAcceptGameSettings;
		GameSettingsTab.SetVisibility(false);
		TabControl.InsertPage(GameSettingsTab, 0, 2, false);
	}

	// Mutators
	MutatorsTab = UTUITabPage_Mutators(FindChild('pnlMutators', true));

	if(MutatorsTab != none)
	{
		MutatorsTab.OnAcceptMutators = OnAcceptMutators;
		MutatorsTab.SetVisibility(false);
		TabControl.InsertPage(MutatorsTab, 0, 3, false);
	}

	// Set defaults
	SetDataStoreStringValue("<Registry:SelectedGameMode>", GameMode);
	MapTab.MapList.RefreshSubscriberValue();

	// Setup the default button bar.
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
		if(TabControl != None)
		{
			// Let the current tab page try to setup the button bar
			UTTabPage(TabControl.ActivePage).SetupButtonBar(ButtonBar);
		}
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
	local Settings GameSettings;
	local string URL;
	local string GameExec;

	if(SelectedOption==0)
	{
		// Play the startgame sound
		PlayUISound('StartGame');	

		// Generate settings URL
		GameSettings = SettingsDataStore.GetCurrentGameSettings();
		GameSettings.SetStringSettingValue(CONTEXT_VSBOTS,CONTEXT_VSBOTS_NONE,FALSE);
		GameSettings.BuildURL(URL);

		// Append any mutators
		if(Len(Mutators) > 0)
		{
			URL = URL $ "?mutator=" $ Mutators;
		}

		URL = URL $ GetCommonOptionsURL();

		// Start the game.
		GameExec = "open " $ MapName $ "?game=" $ GameMode $ URL;
		`Log("UTUIFrontEnd: Starting Game..." $ GameExec);
		ConsoleCommand(GameExec);
	}
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_StartGame(UIScreenObject InButton, int PlayerIndex)
{
	OnStartGame();

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
	
	if(bResult==false)
	{
		if(EventParms.EventType==IE_Released)
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
	}

	return bResult;
}

defaultproperties
{
	MapName="DM-Deck"
	GameMode="UTGame.UTDeathmatch"
	DataStoreName="UTGameSettings"
}
