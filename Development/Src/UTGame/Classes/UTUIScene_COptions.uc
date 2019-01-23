/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_COptions extends UTUIScene_Campaign
	config(Game);

var transient UIImage SkillBkgImage;
var transient UILabel MenuLabel;
var transient UICheckbox PrivateGame;
var transient UTUIOptionButton SkillLevel;

var transient bool bNewGame;
var config string FirstCinematicMapName;

/** Reference to the settings datastore that we will use to create the game. */
var transient UIDataStore_OnlineGameSettings	SettingsDataStore;

var() Texture2D SkillImages[4];

var string LaunchURL;

event PostInitialize( )
{
	Super.PostInitialize();

	PrivateGame = UICheckBox(FindChild('PrivateGame',true));
	SkillLevel = UTUIOptionButton(FindChild('SkillLevel',true));
	SkillLevel.OnValueChanged = OnSkillLevelChanged;

	SettingsDataStore = UIDataStore_OnlineGameSettings(FindDataStore('UTGameSettings'));

	SkillBkgImage = UIImage(FindChild('SkillBackground',true));

	ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.StartGame>", OnButtonBar_Start);
	MenuLabel = UILabel(FindChild('Title',true));

    OnSkillLevelChanged( none, 0 );
}

function OnSkillLevelChanged( UIObject Sender, int PlayerIndex )
{
	local int Skill;
	Skill = SkillLevel.GetCurrentIndex();
	SkillBkgImage.ImageComponent.SetImage(SkillImages[Skill]);
}

function Configure(bool bIsNewGame)
{
	local UTProfileSettings Profile;

	MenuLabel.SetValue( "[Options - "@(bIsNewGame ? "New Game]" : "Continue Game]") );
	bNewGame = bIsNewGame;

	if ( bIsNewGame )
	{
		Profile = GetPlayerProfile();
		if ( Profile != none )
		{
			Profile.NewGame();
		}
		SavePlayerProfile(0);
	}

}

function bool OnButtonBar_Start(UIScreenObject InButton, int InPlayerIndex)
{
	StartGame(InPlayerIndex);
	return true;
}


function StartGame(int InPlayerIndex)
{
	local int Skill;

	Skill = SkillLevel.GetCurrentIndex() * 2;

	if ( bNewGame )
	{
		LaunchURL = "open "$FirstCinematicMapName$"?Difficulty="$Skill$"?Listen";
	}
	else
	{
		LaunchURL = "open UTM-MissionSelection?Difficulty="$Skill$"?Listen";
	}

	if(PrivateGame.IsChecked())
	{
		ConsoleCommand(LaunchURL);
	}
	else
	{
		if(CheckOnlinePrivilegeAndError())
		{
			CreateOnlineGame(InPlayerIndex);
		}
	}
}

/************************ Online Game Interface **************************/

/** Creates the online game and travels to the map we are hosting a server on. */
function CreateOnlineGame(int PlayerIndex)
{
	local OnlineSubsystem OnlineSub;
	local OnlineGameInterface GameInterface;
	local OnlineGameSettings GameSettings;

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

			// Setup server options based on server type.
			GameSettings = SettingsDataStore.GetCurrentGameSettings();
			GameSettings.bUsesArbitration=FALSE;
			GameSettings.NumPrivateConnections = 0;
			GameSettings.NumPublicConnections = 4;;

			// Create the online game
			GameInterface.AddCreateOnlineGameCompleteDelegate(OnGameCreated);

			if(SettingsDataStore.CreateGame(GetWorldInfo(), GetPlayerControllerId(PlayerIndex))==FALSE)
			{
				GameInterface.ClearCreateOnlineGameCompleteDelegate(OnGameCreated);
				`Log("UTUIScene_COption::CreateOnlineGame - Failed to create online game.");
			}
		}
		else
		{
			`Log("UTUIScene_COption::CreateOnlineGame - No GameInterface found.");
		}
	}
	else
	{
		`Log("UTUIScene_COption::CreateOnlineGame - No OnlineSubSystem found.");
	}
}

/** Callback for when the game is finish being created. */
function OnGameCreated(bool bWasSuccessful)
{
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
				ConsoleCommand(LaunchURL);
			}
			else
			{
				`Log("UTUIScene_COption::OnGameCreated - Game Creation Failed.");
			}
		}
		else
		{
			`Log("UTUIScene_COption::OnGameCreated - No GameInterface found.");
		}
	}
	else
	{
		`Log("UTUIScene_COption::OnGameCreated - No OnlineSubSystem found.");
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
	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_A')
		{
			StartGame(EventParms.PlayerIndex);
			return true;
		}
	}

	return Super.HandleInputKey(EventParms);
}


defaultproperties
{
}
