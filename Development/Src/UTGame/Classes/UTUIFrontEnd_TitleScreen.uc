/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Title screen scene for UT3, handles attract mode launching.
 */

class UTUIFrontEnd_TitleScreen extends UTUIScene
	native(UIFrontEnd);

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}

/** Whether or not we are in the attract mode movie. */
var bool	bInMovie;

/** Amount of time elapsed since last user input. */
var float	TimeElapsed;

/** Name of the attract mode movie. */
var() string	MovieName;

/** Amount of time until the attract movie starts. */
var() float TimeTillAttractMovie;

/** Reference to the main menu scene. */
var string MainMenuScene;

/** Flag to update the LP array on the next tick. */
var transient bool bUpdatePlayersOnNextTick;

/** Post initialize event - Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();
	OnRawInputKey=HandleInputKey;

	// Activate kismet for entering scene
	ActivateLevelEvent('TitleScreenEnter');

	RegisterOnlineDelegates();

	// Update game player's array with players that were logged in before the game was started.
	UpdateGamePlayersArray();
}

event SceneActivated(bool bInitialActivation)
{
	Super.SceneActivated(bInitialActivation);

	// Skip title screen if we are exiting a game.
	CheckTitleSkip();
}

event SceneDeactivated()
{
	Super.SceneDeactivated();

	CleanupOnlineDelegates();
}

/** Registers online delegates to catch global events such as login changes. */
function RegisterOnlineDelegates()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			PlayerInterface.AddLoginChangeDelegate(OnLoginChange);
		}
	}

	// Update logged in profile labels
	UpdateProfileLabels();
}

/** Cleans up any registered online delegates. */
function CleanupOnlineDelegates()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			PlayerInterface.ClearLoginChangeDelegate(OnLoginChange);
		}
	}
}

/** @return Returns the number of currently logged in controllers. */
event int GetNumLoggedInPlayers()
{
	local int NumLoggedIn;
	local int ControllerId;

	NumLoggedIn = 0;

	for(ControllerId=0; ControllerId<MAX_SUPPORTED_GAMEPADS; ControllerId++)
	{
		if(IsLoggedIn(ControllerId))
		{
			NumLoggedIn++;
		}
	}

	return NumLoggedIn;
}

/** Creates a local player for every signed in controller. */
native function UpdateGamePlayersArray();

/** Checks to see if a frontend error message was set by the game before returning to the main menu, if so, we skip to the main menu and display the message. */
function CheckTitleSkip()
{
	local string OutValue;

	if(GetDataStoreStringValue("<Registry:GoStraightToMainMenu>", OutValue))
	{
		if(OutValue=="1")
		{
			OpenSceneByName(MainMenuScene);
		}
	}

	// Clear the display flag.
	SetDataStoreStringValue("<Registry:GoStraightToMainMenu>", "1");
}

/** Sets PlayerIndex 0's controller id to be the specified controller id, this makes it so that the specified controller can control all of the menus. */
function SetMainControllerId(int ControllerId)
{
	local LocalPlayer LP;

	`Log("UTUIFrontEnd_TitleScreen::SetMainControllerId - Setting main controller ID to "$ControllerId);

	LP = GetPlayerOwner(0);
	LP.SetControllerId(ControllerId);

	UpdateProfileLabels();
}

/** Updates the profile labels. */
event UpdateProfileLabels()
{
	local UIScene BackgroundScene;
	local LocalPlayer LP;
	local UILabel PlayerLabel1;
	local UILabel PlayerLabel2;

	BackgroundScene = GetSceneClient().FindSceneByTag('Background');
	PlayerLabel1 = UILabel(BackgroundScene.FindChild('lblPlayerName1', true));
	PlayerLabel2 = UILabel(BackgroundScene.FindChild('lblPlayerName2', true));

	// Player 1
	LP = GetPlayerOwner(0);
	if(LP != None && IsLoggedIn(LP.ControllerId))
	{
		PlayerLabel1.SetDataStoreBinding("<OnlinePlayerData:PlayerNickName>");
	}
	else
	{
		PlayerLabel1.SetDataStoreBinding("<Strings:UTGameUI.Generic.NotLoggedIn>");
	}

	// Player 2
	LP = GetPlayerOwner(1);
	if(LP != None && IsLoggedIn(LP.ControllerId))
	{
		PlayerLabel2.SetDataStoreBinding("<OnlinePlayerData:PlayerNickName>");
	}
	else
	{
		PlayerLabel2.SetDataStoreBinding("<Strings:UTGameUI.Generic.NotLoggedIn>");
	}
}

/** Called when the profile read has completed for any player. */
function OnProfileReadComplete()
{
	UpdateProfileLabels();
}

/** Called any time any player changes their current login status. */
function OnLoginChange()
{
	local GameUISceneClient GameSceneClient;
	local int SceneIdx;
	local UIScene MainMenuSceneInstance;
	local bool bForcedChange;
	local string OutValue;
	GameSceneClient = GetSceneClient();

	bForcedChange = false;
	if(GetDataStoreStringValue("<Registry:bChangingLoginStatus>", OutValue))
	{
		if(OutValue=="1")
		{
			bForcedChange = true;
		}
	}	

	// Kick the players back to the main menu if we aren't purposely changing the login status of a player.
	MainMenuSceneInstance = GameSceneClient.FindSceneByTag('MainMenu');

	if(MainMenuSceneInstance != None && !bForcedChange)
	{	
		for(SceneIdx=0; SceneIdx<GameSceneClient.ActiveScenes.length; SceneIdx++)
		{
			// Find main menu
			if(GameSceneClient.ActiveScenes[SceneIdx]==MainMenuSceneInstance)
			{
				// Close any scenes above this one
				if((SceneIdx+1) < GameSceneClient.ActiveScenes.length)
				{
					CloseScene(GameSceneClient.ActiveScenes[SceneIdx+1]);
				}

				break;
			}
		}
	}

	// Update the game player's array
	bUpdatePlayersOnNextTick = true;
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
		if(bInMovie)
		{
			StopMovie();
		}
		else
		{
			//@todo joew - why not just add the Clicked alias to this scene's list of supported aliases, then map these four keys to that alias?
			if(EventParms.InputKeyName=='XboxTypeS_A' || EventParms.InputKeyName=='XboxTypeS_Start' || EventParms.InputKeyName=='Enter' || EventParms.InputKeyName=='LeftMouseButton')
			{
				// Set main controller id.
				SetMainControllerId(EventParms.ControllerId);

				// Activate kismet for exiting scene
				ActivateLevelEvent('TitleScreenExit');

				OpenSceneByName(MainMenuScene);
				bResult=true;
			}
			else
			{
				TimeElapsed=0.0;
			}
		}
	}

	return bResult;
}

/** Starts the attract mode movie. */
native function StartMovie();

/** Stops the currently playing movie. */
native function StopMovie();

/** Checks to see if a movie is done playing. */
native function UpdateMovieStatus();

defaultproperties
{
	MainMenuScene="UI_Scenes_ChrisBLayout.Scenes.MainMenu""
	TimeTillAttractMovie=90.0
}
