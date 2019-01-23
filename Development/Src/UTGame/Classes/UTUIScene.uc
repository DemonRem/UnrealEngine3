/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 *  Our UIScenes provide PreRender and tick passes to our Widgets
 */

class UTUIScene extends UIScene
	abstract
	native(UI)
	dependson(OnlinePlayerInterface);

/** Possible platforms the game is running on. */
enum EUTPlatform
{
	UTPlatform_PC,
	UTPlatform_360,
	UTPlatform_PS3
};

/** Possible factions for the bots. */
enum EUTBotTeam
{
	UTBotTeam_Random,
	UTBotTeam_Ironguard,
	UTBotTeam_TwinSouls,
	UTBotTeam_Krall,
	UTBotTeam_Liandri,
	UTBotTeam_Necris
};

/** Demo recording. */
enum EUTRecordDemo
{
	UTRecordDemo_No,
	UTRecordDemo_Yes,
};

var(Editor) transient bool bEditorRealTimePreview;

/** @todo: Maybe move this reference to another class. */
var transient UIScene MessageBoxScene;

/** Pending scene to open since we are waiting for the current scene's exit animation to end. */
var transient UIScene PendingOpenScene;

/** Pending scene to close since we are waiting for the current scene's exit animation to end. */
var transient UIScene PendingCloseScene;

/** Callback for when the scene's show animation has ended. */
delegate OnShowAnimationEnded();

/** Callback for when the scene's hide animation has ended. */
delegate OnHideAnimationEnded();

cpptext
{
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	virtual void Tick( FLOAT DeltaTime );
	virtual void TickChildren(UUIScreenObject* ParentObject, FLOAT DeltaTime);
	virtual void PreRender(FCanvas* Canvas);

	static void AutoPlaceChildren(UUIScreenObject *const BaseObject);
}

/**
 * Returns the WorldInfo
 */
native function WorldInfo GetWorldInfo();

/**
 * Get the UTPlayerController that is associated with this Hud
 */
native function UTPlayerController GetUTPlayerOwner(optional int PlayerIndex=-1);

/**
 * Returns the Pawn associated with this Hud
 */
native function Pawn GetPawnOwner();

/**
 * Returns the PRI associated with this hud
 */
native function UTPlayerReplicationInfo GetPRIOwner();


/** @return Returns the current platform the game is running on. */
native static function EUTPlatform GetPlatform();

/**
 * @Returns the contents of GIsGame
 */
native function bool IsGame();

/** Starts a dedicated server and kills the current process. */
native function StartDedicatedServer(string TravelURL);


/**
 * Executes a console command.
 *
 * @param string Cmd	Command to execute.
 */
function ConsoleCommand(string Cmd)
{
	PlayerOwner.ViewportClient.UIController.ViewportConsole.ConsoleCommand(Cmd);
}


/** @return Returns the player index of the player owner for this scene. */
function int GetPlayerIndex()
{
	return class'UIInteraction'.static.GetPlayerIndex(GetPlayerOwner().ControllerId);
}

/**
 * Opens a UI Scene given a reference to a scene to open.
 *
 * @param SceneToOpen	Scene that we want to open.
 */
function UIScene OpenSceneByName(string SceneToOpen)
{
	local UIScene SceneToOpenReference;
	SceneToOpenReference = UIScene(DynamicLoadObject(SceneToOpen, class'UIScene'));

	if(SceneToOpenReference != None)
	{
		return OpenScene(SceneToOpenReference);
	}
	else
	{
		return None;
	}
}

/**
 * Opens a UI Scene given a reference to a scene to open.
 *
 * @param SceneToOpen	Scene that we want to open.
 */
function UIScene OpenScene(UIScene SceneToOpen)
{
	local UIScene Result;

	/* @todo: Enable
	OnHideAnimationEnded = OnCurrentScene_HideAnimationEnded;
	if(BeginHideAnimation())
	{
		// @todo: Maybe disable input here?
		PendingOpenScene = SceneToOpen;
	}
	else
	{
		Result = FinishOpenScene(SceneToOpen);
	}*/

	Result = FinishOpenScene(SceneToOpen);

	return Result;
}

/** Callback for when the current scene's hide animation has completed. */
function OnCurrentScene_HideAnimationEnded()
{
	FinishOpenScene(PendingOpenScene);
}

/** Finishes opening a scene, usually called when a hide animation has ended. */
function UIScene FinishOpenScene(UIScene SceneToOpen)
{
	local UIScene Temp;
	local UIInteraction UIController;
	local UTUIScene UTSceneToOpen;

	// Clear any references and delegates set
	UTSceneToOpen = UTUIScene(SceneToOpen);

	if(UTSceneToOpen != None)
	{
		UTSceneToOpen.OnHideAnimationEnded = None;
		UTSceneToOpen.OnShowAnimationEnded = None;
	}
	PendingOpenScene = None;
	OnHideAnimationEnded = None;

	Temp = none;

	if ( PlayerOwner != none )
	{
		// Get the UI Controller and try to open the scene.
		UIController = PlayerOwner.ViewportClient.UIController;
		if ( SceneToOpen != none && UIController != none  )
		{
			// Have the UI system look to see if the scene exists.  If it does
			// use that so that split-screen shares scenes
			Temp = UIController.FindSceneByTag(	SceneToOpen.SceneTag, PlayerOwner );
			if (Temp == none)
			{
				// Nothing, just create a new instance
				UIController.OpenScene(SceneToOpen, PlayerOwner, Temp);
			}
		}
	}

	// Activate kismet for opening scene
	ActivateLevelEvent('OpeningMenu');

	return Temp;
}

/**
 * Closes a UI Scene given a reference to an previously open scene.
 *
 * @param SceneToClose	Scene that we want to close.
 */
function CloseScene(UIScene SceneToClose)
{
	/* @todo: Enable
	local UTUIScene UTSceneToClose;

	UTSceneToClose = UTUIScene(SceneToClose);
	
	if(UTSceneToClose != None && UTSceneToClose.BeginHideAnimation())
	{
		UTSceneToClose.OnHideAnimationEnded = OnPendingCloseScene_HideAnimationEnded;
		PendingCloseScene = UTSceneToClose;
	}
	else
	{
		FinishCloseScene(SceneToClose);
	}*/

	FinishCloseScene(SceneToClose);
}

/** Callback for when the scene we are closing's hide animation has completed. */
function OnPendingCloseScene_HideAnimationEnded()
{
	FinishCloseScene(PendingCloseScene);
}

/**
 * Closes a UI Scene given a reference to an previously open scene.
 *
 * @param SceneToClose	Scene that we want to close.
 */
function FinishCloseScene(UIScene SceneToClose)
{
	local UIInteraction UIController;
	local UTUIScene UTSceneToClose;

	UTSceneToClose = UTUIScene(SceneToClose);

	if(UTSceneToClose != None)
	{
		UTSceneToClose.OnHideAnimationEnded = None;
	}

	PendingCloseScene = None;

	if ( PlayerOwner != none )
	{
		// Get the UI Controller and try to open the scene.
		UIController = PlayerOwner.ViewportClient.UIController;
		if ( SceneToClose != none && UIController != none  )
		{
			UIController.CloseScene(SceneToClose);
		}
	}
}

/** 
 * Starts the show animation for the scene. 
 *
 * @return TRUE if there's animation for this scene, FALSE otherwise.
 */
function bool BeginShowAnimation()
{
	return FALSE;	
}

/** 
 * Starts the exit animation for the scene. 
 *
 * @return TRUE if there's animation for this scene, FALSE otherwise.
 */
function bool BeginHideAnimation()
{
	return FALSE;	
}


/** @return Opens the message box scene and returns a reference to it. */
function UTUIScene_MessageBox GetMessageBoxScene()
{
	return UTUIScene_MessageBox(OpenScene(MessageBoxScene));
}

function UTUIScene_MessageBox DisplayMessageBox (string Message, optional string Title="")
{
	local UTUIScene_MessageBox MessageBoxReference;

	MessageBoxReference = GetMessageBoxScene();

	if(MessageBoxReference != none)
	{
		MessageBoxReference.Display(Message, Title);
	}

	return MessageBoxReference;
}

function NotifyGameSessionEnded()
{
	local int i;
	for (i=0;i<Children.Length;i++)
	{
		NotifyChildGameSessionEnded(Children[i]);
		if ( UTUI_Widget(Children[i]) != none )
		{
			UTUI_Widget(Children[i]).NotifyGameSessionEnded();
		}
	}

	Super.NotifyGameSessionEnded();
}

function NotifyChildGameSessionEnded(UIObject Child)
{
	local int i;
	for ( i=0; i<Child.Children.Length; i++ )
	{
		NotifyChildGameSessionEnded(Child.Children[i]);
		if ( UTUI_Widget(Child.Children[i]) != none )
		{
			UTUI_Widget(Child.Children[i]).NotifyGameSessionEnded();
		}
	}
}

event OnAnimationFinished(UIObject AnimTarget, name AnimName, name SeqName);

/**
 * Allows easy access to playing a sound
 *
 * @Param	InSoundCue		The Cue to play
 * @Param	SoundLocation	Where in the world to play it.  Defaults at the Player's position
 */

function PlaySound( SoundCue InSoundCue)
{
	local UTPlayerController PC;

	PC = GetUTPlayerOwner();
	if ( PC != none )
	{
		PC.ClientPlaySound(InSoundCue);
	}
}

/** @return Returns a datastore given its tag and player owner. */
function UIDataStore FindDataStore(name DataStoreTag, optional LocalPlayer InPlayerOwner)
{
	local UIDataStore	Result;

	Result = GetCurrentUIController().DataStoreManager.FindDataStore(DataStoreTag, InPlayerOwner);

	return Result;
}

/** @return Returns the controller id of a player given its player index. */
function int GetPlayerControllerId(int PlayerIndex)
{
	local int Result;

	Result = GetCurrentUIController().GetPlayerControllerId(PlayerIndex);;

	return Result;
}

/** Activates a level remote event in kismet. */
native function ActivateLevelEvent(name EventName);

/**
 *  Returns the Player Profile for a given player index.
 *
 * @param	PlayerIndex		The player who's profile you require
 * @returns the profile precast to UTProfileSettings
 */
function UTProfileSettings GetPlayerProfile(optional int PlayerIndex=GetBestPlayerIndex() )
{
	local LocalPlayer LP;
	local UTProfileSettings Profile;

	LP = GetPlayerOwner(PlayerIndex);
	if ( LP != none && LP.Actor != none )
	{
		Profile = UTProfileSettings( LP.Actor.OnlinePlayerData.ProfileProvider.Profile);
	}
	return Profile;
}

/**
 *  Returns the Player Profile for a given player index.
 *
 * @param	PC	The PlayerContorller of the profile you require.
 * @returns the profile precast to UTProfileSettings
 */
function UTProfileSettings GetPlayerProfileFromPC(PlayerController PC )
{
	local UTProfileSettings Profile;
	Profile = UTProfileSettings( PC.OnlinePlayerData.ProfileProvider.Profile);
	return Profile;
}

/**
 * Saves the profile for the specified player index.
 *
 * @param PlayerIndex	The player index of the player to save the profile for.
 */
function SavePlayerProfile(optional int PlayerIndex=GetBestPlayerIndex())
{
	local UIDataStore_OnlinePlayerData	PlayerDataStore;
	PlayerDataStore = UIDataStore_OnlinePlayerData(FindDataStore('OnlinePlayerData', GetPlayerOwner(PlayerIndex)));

	if(PlayerDataStore != none)
	{
		`Log("UTUIScene::SaveProfile() - Saving player profile for player index "$PlayerIndex);
		PlayerDataStore.SaveProfileData();
	}
}

/** @return Returns the name of the specified player if they are logged in, or "DefaultPlayer" otherwise. */
function string GetPlayerName(int PlayerIndex=INDEX_NONE)
{
	local string PlayerName;

	if(IsLoggedIn(PlayerIndex))
	{
		PlayerName=GetUTPlayerOwner(PlayerIndex).OnlinePlayerData.PlayerNick;
	}
	else
	{
		PlayerName="DefaultPlayer";
	}

	return PlayerName;
}

/** @return string	Returns a bot faction name given its enum. */
function name GetBotTeamNameFromIndex(EUTBotTeam TeamIndex)
{
	local name Result;

	switch(TeamIndex)
	{
	case UTBotTeam_Ironguard:
		Result = 'Ironguard';
		break;
	case UTBotTeam_TwinSouls:
		Result = 'Twin Souls';
		break;
	case UTBotTeam_Krall:
		Result = 'Krall';
		break;
	case UTBotTeam_Liandri:
		Result = 'Liandri';
		break;
	case UTBotTeam_Necris:
		Result = 'Necris';
		break;
	default:
		Result = '';
		break;
	}

	return Result;
}

/** @return Returns the current login status for the specified player index. */
function ELoginStatus GetLoginStatus(int ControllerId=INDEX_NONE)
{
	local ELoginStatus Result;
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	Result = LS_NotLoggedIn;
	
	if(ControllerId==INDEX_NONE)
	{
		ControllerId = GetPlayerOwner().ControllerId;
	}

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// Get status
			Result = PlayerInterface.GetLoginStatus(ControllerId);
		}
	}

	return Result;
}


/** @return Returns the current status of the platform's network connection. */
function bool HasLinkConnection()
{
	local bool bResult;
	local OnlineSubsystem OnlineSub;
	local OnlineSystemInterface SystemInterface;

	bResult = false;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		SystemInterface = OnlineSub.SystemInterface;
		if (SystemInterface != None)
		{
			bResult = SystemInterface.HasLinkConnection();
		}
	}

	return bResult;
}


/** @return Returns whether or not the specified player is logged in at all. */
event bool IsLoggedIn(int ControllerId=INDEX_NONE)
{
	return (GetLoginStatus(ControllerId)!=LS_NotLoggedIn);
}

/** @return Returns whether or not the specified player can play online. */
event bool CanPlayOnline(int ControllerId=INDEX_NONE)
{
	local EFeaturePrivilegeLevel Result;
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	Result = FPL_Disabled;

	if(ControllerId==INDEX_NONE)
	{
		ControllerId = GetPlayerOwner().ControllerId;
	}

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			Result = PlayerInterface.CanPlayOnline(ControllerId);
		}
	}

	return Result!=FPL_Disabled;
}

/** @return Checks to see if the specified player is logged in, if not an error message is shown and FALSE is returned. */
function bool CheckLoginAndError(int ControllerId=INDEX_NONE, optional bool bMustBeLoggedInOnline=false)
{
	local ELoginStatus LoginStatus;

	LoginStatus = GetLoginStatus(ControllerId);

	if((LoginStatus==LS_UsingLocalProfile && bMustBeLoggedInOnline==false) || LoginStatus==LS_LoggedIn)
	{
		return true;
	}
	else
	{
		DisplayMessageBox("<Strings:UTGameUI.Errors.LoginRequired_Message>","<Strings:UTGameUI.Errors.LoginRequired_Title>");
		return false;
	}
}

/** @return Checks to see if the platform is currently connected to a network. */
function bool CheckLinkConnectionAndError()
{
	if(HasLinkConnection())
	{
		return true;
	}
	else
	{
		DisplayMessageBox("<Strings:UTGameUI.Errors.LinkDisconnected_Message>","<Strings:UTGameUI.Errors.LinkDisconnected_Title>");
		return false;
	}
}

/** @return Checks to see if the specified player can play online games, if not an error message is shown and FALSE is returned. */
function bool CheckOnlinePrivilegeAndError(int ControllerId=INDEX_NONE)
{
	local bool bResult;

	bResult = false;

	if(CheckLoginAndError(ControllerId, TRUE))
	{
		if(CheckLinkConnectionAndError())
		{
			if(CanPlayOnline(ControllerId))
			{
				bResult = true;
			}
			else
			{
				DisplayMessageBox("<Strings:UTGameUI.Errors.OnlineRequired_Message>","<Strings:UTGameUI.Errors.OnlineRequired_Title>");
			}
		}
	}

	return bResult;
}

/** @return Returns Returns a unique demo filename. */
function string GenerateDemoFileName()
{
	// @todo: Need to autogenerate a filename
	return "UTDemoPlaceholder";
}

/** @return Generates a set of URL options common to both instant action and host game. */
function string GetCommonOptionsURL()
{
	local int BotTeamIndex;
	local string TeamName;
	local UTUIDataStore_StringList StringListDataStore;
	local string URL;

	URL = "";
	StringListDataStore = UTUIDataStore_StringList(FindDataStore('UTStringList'));

	// Set Bot Faction
	BotTeamIndex = StringListDataStore.GetCurrentValueIndex('BotTeams');
	if(BotTeamIndex != UTBotTeam_Random)
	{
		TeamName = string(GetBotTeamNameFromIndex(EUTBotTeam(BotTeamIndex)));
		URL = URL $ "?BlueFaction='" $ TeamName $ "'?RedFaction='" $ TeamName $ "'";
	}

	// Set if we are recording a demo
	if(StringListDataStore.GetCurrentValueIndex('RecordDemo')==UTRecordDemo_Yes)
	{
		URL = URL $ "?demo=" $ GenerateDemoFileName();
	}

	// Set player name using the OnlinePlayerData
	// @todo: Need to add support for setting 2nd player nick.
	URL = URL$"?name="$GetPlayerName();

	return URL;
}

/** @return Returns a reference to the online subsystem player interface. */
static function OnlinePlayerInterface GetPlayerInterface()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInt;

	// Display the login UI
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		PlayerInt = OnlineSub.PlayerInterface;
	}
	else
	{
		`Log("UTUIScene::GetPlayerInterface() - Unable to find OnlineSubSystem!");
	}

	return PlayerInt;
}

/** @return Returns a reference to the online subsystem player interface. */
static function OnlineAccountInterface GetAccountInterface()
{
	local OnlineSubsystem OnlineSub;
	local OnlineAccountInterface AccountInt;

	// Display the login UI
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		AccountInt = OnlineSub.AccountInterface;
	}
	else
	{
		`Log("UTUIScene::GetAccountInterface() - Unable to find OnlineSubSystem!");
	}

	return AccountInt;
}

/** Displays the login interface using the online subsystem. */
function ShowLoginUI(optional bool bOnlineOnly=false)
{
	local OnlinePlayerInterface PlayerInt;

	PlayerInt = GetPlayerInterface();

	if(PlayerInt != none)
	{
		if (PlayerInt.ShowLoginUI(bOnlineOnly) == false)
		{
			`Log("UTUIScene::ShowLoginUI() - Failed to show login UI!");
		}
	}
}

defaultproperties
{
	MessageBoxScene=UIScene'UI_Scenes_Common.MessageBox'
}
