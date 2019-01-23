//=============================================================================
// ExamplePlayerController
// Example specific playercontroller
// PlayerControllers are used by human players to control pawns.
//
// Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ExamplePlayerController extends GamePlayerController
	config(Game);

var()				float			WeaponImpulse;
var()				float			HoldDistanceMin;
var()				float			HoldDistanceMax;
var()				float			ThrowImpulse;
var()				float			ChangeHoldDistanceIncrement;

var()				RB_Handle		PhysicsGrabber;
var					float			HoldDistance;
var					Quat			HoldOrientation;


/** Live achievement defines */
enum EExampleGameAchievements
{
	// Achievement IDs start from 1 so skip Zero
	EGA_InvalidAchievement,
	EGA_Achievement1,
	EGA_Achievement2,
	EGA_Achievement3,
	EGA_Achievement4,
	EGA_Achievement5
};

/** Holds a ref to the online stats object */
var ExampleGameStatsWrite StatsWrite;

/** Holds a ref to the online stats read object */
var ExampleGameStatsRead StatsRead;

/** The profile settings object for this player */
var ExampleProfileSettings ProfileSettings;

/** Inits online stats object */
simulated event PostBeginPlay()
{
	Super.PostBeginPlay();
	// Create our stats collection object
	StatsWrite = new(Outer) class'ExampleGameStatsWrite';
	StatsRead = new(Outer) class'ExampleGameStatsRead';
}

/**
 * Registers any game specific datastores
 */
simulated function RegisterPlayerDataStores()
{
	Super.RegisterPlayerDataStores();

	if ( OnlinePlayerData != None && OnlinePlayerData.ProfileProvider != None )
	{
		// Get the profile data from the online player data store
		ProfileSettings = ExampleProfileSettings(OnlinePlayerData.ProfileProvider.Profile);
		`Log("Got profile object "$ProfileSettings);
	}
}

/**
 * Unregisters any game specific datastores
 */
simulated function UnregisterPlayerDataStores()
{
	Super.UnregisterPlayerDataStores();
	// Clear the ref for GC purposes
	ProfileSettings = None;
}

/** Start as PhysicsSpectator by default */
auto state PlayerWaiting
{
Begin:
	PlayerReplicationInfo.bOnlySpectator = false;
	WorldInfo.Game.bRestartLevel = false;
	WorldInfo.Game.RestartPlayer( Self );
	WorldInfo.Game.bRestartLevel = true;
	SetCameraMode('ThirdPerson');
}


// Previous weapon entrypoint. Here we just alter the hold distance for the physics weapon
exec function PrevWeapon()
{
	HoldDistance += ChangeHoldDistanceIncrement;
	HoldDistance = FMin(HoldDistance, HoldDistanceMax);
}

// Next weapon entrypoint. Here we just alter the hold distance for the physics weapon
exec function NextWeapon()
{
	HoldDistance -= ChangeHoldDistanceIncrement;
	HoldDistance = FMax(HoldDistance, HoldDistanceMin);
}

//
// Primary fire has been triggered. This sends impulses from the default physics gun
//
exec function StartFire( optional byte FireModeNum )
{
	local vector		CamLoc, StartShot, EndShot, X, Y, Z;
	local vector		HitLocation, HitNormal, ZeroVec;
	local actor			HitActor;
	local TraceHitInfo	HitInfo;
	local rotator		CamRot;

	GetPlayerViewPoint(CamLoc, CamRot);

	GetAxes( CamRot, X, Y, Z );
	ZeroVec = vect(0,0,0);

	if ( PhysicsGrabber.GrabbedComponent == None )
	{
		// Do simple line check then apply impulse
		StartShot	= CamLoc;
		EndShot		= StartShot + (10000.0 * X);
		HitActor	= Trace(HitLocation, HitNormal, EndShot, StartShot, True, ZeroVec, HitInfo);

		if ( HitActor != None && HitInfo.HitComponent != None )
		{
			HitInfo.HitComponent.AddImpulse(X * WeaponImpulse, HitLocation, HitInfo.BoneName);
		}
	}
	else
	{
		PhysicsGrabber.GrabbedComponent.AddImpulse(X * ThrowImpulse, , PhysicsGrabber.GrabbedBoneName);
		PhysicsGrabber.ReleaseComponent();
	}
}

// Alternate-fire has been triggered.  This is the physics grabber
exec function StartAltFire( optional byte FireModeNum )
{
	local vector					CamLoc, StartShot, EndShot, X, Y, Z;
	local vector					HitLocation, HitNormal, Extent;
	local actor						HitActor;
	local float						HitDistance;
	local Quat						PawnQuat, InvPawnQuat, ActorQuat;
	local TraceHitInfo				HitInfo;
	local SkeletalMeshComponent		SkelComp;
	local rotator					CamRot;

	GetPlayerViewPoint(CamLoc, CamRot);

	// Do ray check and grab actor
	GetAxes( CamRot, X, Y, Z );
	StartShot	= CamLoc;
	EndShot		= StartShot + (10000.0 * X);
	Extent		= vect(0,0,0);
	HitActor	= Trace(HitLocation, HitNormal, EndShot, StartShot, True, Extent, HitInfo);
	HitDistance = VSize(HitLocation - StartShot);

	if( HitActor != None &&
		HitActor != WorldInfo &&
		HitDistance > HoldDistanceMin &&
		HitDistance < HoldDistanceMax )
	{
		// If grabbing a bone of a skeletal mesh, dont constrain orientation.
		PhysicsGrabber.GrabComponent(HitInfo.HitComponent, HitInfo.BoneName, HitLocation, bRun==0);

		// If we succesfully grabbed something, store some details.
		if( PhysicsGrabber.GrabbedComponent != None )
		{
			HoldDistance	= HitDistance;
			PawnQuat		= QuatFromRotator( CamRot );
			InvPawnQuat		= QuatInvert( PawnQuat );

			if ( HitInfo.BoneName != '' )
			{
				SkelComp = SkeletalMeshComponent(HitInfo.HitComponent);
				ActorQuat = SkelComp.GetBoneQuaternion(HitInfo.BoneName);
			}
			else
			{
				ActorQuat = QuatFromRotator( PhysicsGrabber.GrabbedComponent.Owner.Rotation );
			}

			HoldOrientation = QuatProduct(InvPawnQuat, ActorQuat);
		}
	}
}

// Alt-fire is a grab, so a release function is needed
exec function StopAltFire( optional byte FireModeNum )
{
	if ( PhysicsGrabber.GrabbedComponent != None )
	{
		PhysicsGrabber.ReleaseComponent();
	}
}

//
// Logic tick for the player controller. Just updates physics weapon logic
//
event PlayerTick(float DeltaTime)
{
	local vector	CamLoc, NewHandlePos, X, Y, Z;
	local Quat		PawnQuat, NewHandleOrientation;
	local rotator	CamRot;

	super.PlayerTick(DeltaTime);

	if ( PhysicsGrabber.GrabbedComponent == None )
	{
		return;
	}

	PhysicsGrabber.GrabbedComponent.WakeRigidBody( PhysicsGrabber.GrabbedBoneName );

	// Update handle position on grabbed actor.
	GetPlayerViewPoint(CamLoc, CamRot);
	GetAxes( CamRot, X, Y, Z );
	NewHandlePos = CamLoc + (HoldDistance * X);
	PhysicsGrabber.SetLocation( NewHandlePos );

	// Update handle orientation on grabbed actor.
	PawnQuat = QuatFromRotator( CamRot );
	NewHandleOrientation = QuatProduct(PawnQuat, HoldOrientation);
	PhysicsGrabber.SetOrientation( NewHandleOrientation );
}

exec function ShowMenu()
{
	DisplayGameMenu();
}

function DisplayGameMenu()
{
	local UIInteraction UIController;

	UIController = GetUIController();
	if ( UIController != None )
	{
		UIController.OpenScene( UIScene'EngineScenes.GameMenu', LocalPlayer(Player) );
	}
}

/**
 * Handles the Kismet action to unlock an achievement
 *
 * @param Action the action containing which achievement to unlock
 */
function OnUnlockAchievement(ExampleGame_UnlockAchievement Action)
{
	ClientUnlockAchievement(Action.AchievementId);
}

/**
 * Unlocks the achievement on the client (can only be done clientside)
 *
 * @param AchievementId the achievement to unlock
 */
reliable client function ClientUnlockAchievement(int AchievementId)
{
	local LocalPlayer LocPlayer;
	LocPlayer = LocalPlayer(Player);
	if (OnlineSub != None)
	{
		// If the extended interface is supported
		if (OnlineSub.PlayerInterfaceEx != None)
		{
			OnlineSub.PlayerInterfaceEx.UnlockAchievement(LocPlayer.ControllerId,AchievementId);
		}
		else
		{
			`Log("Interface is not supported. Can't unlock an achievement");
		}
	}
	else
	{
		`Log("No online subsystem. Can't unlock an achievement");
	}
}


/** Pops up the guide's login ui */
exec function ShowLoginUI(optional bool bShowOnlineOnly)
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface.ShowLoginUI(bShowOnlineOnly))
		{
			`Log("ShowLoginUI worked");
		}
		else
		{
			`Log("ShowLoginUI failed");
		}
	}
}

/** Dumps login info to the log */
exec function IterateLogins()
{
	local int Ports;
	local UniqueNetId Xuid;
	if (OnlineSub != None)
	{
		for (Ports = 0; Ports < 4; Ports++)
		{
			`Log("Login status for ("$Ports$") is "$OnlineSub.PlayerInterface.GetLoginStatus(Ports));
			OnlineSub.PlayerInterface.GetUniquePlayerId(Ports,Xuid);
			`Log("XUID for ("$Ports$") is ("$Xuid.Uid[0]$","$Xuid.Uid[1]$","$Xuid.Uid[2]$","$Xuid.Uid[3]$","$Xuid.Uid[4]$","$Xuid.Uid[5]$","$Xuid.Uid[6]$","$Xuid.Uid[7]$")");
			`Log("Gamertag for ("$Ports$") is "$OnlineSub.PlayerInterface.GetPlayerNickname(Ports));
		}
	}
}

/**
 * Checks to see if controller 0 & 1 are friends. NOTE: Requires that both
 * controllers are signed in
 */
exec function CheckFriends()
{
	local UniqueNetId Xuid2;
	if (OnlineSub != None)
	{
		// Check controllers 0 & 1 for friendship
		OnlineSub.PlayerInterface.GetUniquePlayerId(1,Xuid2);
		if (OnlineSub.PlayerInterface.IsFriend(0,Xuid2) == true)
		{
			`Log("Controller 1 is a friend of 0");
		}
	}
}

/**
 * Pops up the friends ui for controller 0
 */
exec function ShowFriendsUI()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface.ShowFriendsUI(0) == true)
		{
			`Log("ShowFriendsUI() worked");
		}
		else
		{
			`Log("ShowFriendsUI() failed");
		}
	}
}

/** Pops up the friends invite ui for controller zero inviting controller one */
exec function ShowFriendsInviteUI()
{
	local UniqueNetId Xuid1;
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.GetUniquePlayerId(1,Xuid1);
		if (OnlineSub.PlayerInterfaceEx.ShowFriendsInviteUI(0,Xuid1) == true)
		{
			`Log("ShowFriendsInviteUI() worked");
		}
		else
		{
			`Log("ShowFriendsInviteUI() failed");
		}
	}
}

/** Prints the privilege setting in a human readable form */
function LogPriv(string Msg,EFeaturePrivilegeLevel Priv)
{
`if(`notdefined(FINAL_RELEASE))
	local string PrivStr;
	if (Priv == FPL_Disabled)
	{
		PrivStr = "not allowed";
	}
	else if (Priv == FPL_Enabled)
	{
		PrivStr = "allowed";
	}
	else
	{
		PrivStr = "allowed only with friends";
	}
	`Log(Msg$" "$PrivStr);
`endif
}

/** Displays the privileges that the player signed in as controller 0 has */
exec function DumpPrivs()
{
	local EFeaturePrivilegeLevel Priv;
	if (OnlineSub != None)
	{
		Priv = OnlineSub.PlayerInterface.CanPlayOnline(0);
		LogPriv("Online play is",Priv);
		Priv = OnlineSub.PlayerInterface.CanCommunicate(0);
		LogPriv("Voice/text chat is",Priv);
		Priv = OnlineSub.PlayerInterface.CanDownloadUserContent(0);
		LogPriv("Download user content is",Priv);
		Priv = OnlineSub.PlayerInterface.CanPurchaseContent(0);
		LogPriv("Purchasing content is",Priv);
		Priv = OnlineSub.PlayerInterface.CanViewPlayerProfiles(0);
		LogPriv("Player profile viewing is",Priv);
		Priv = OnlineSub.PlayerInterface.CanShowPresenceInformation(0);
		LogPriv("Showing presence information is",Priv);
	}
}

/** Logs the change in login status to show the event is working */
function LoginChanged()
{
	`Log("Login status changed");
}

/** Logs the change in muting list to show the event is working */
function MutingChanged()
{
	`Log("Muting list changed");
}

/** Logs the change in friends list to show the event is working */
function FriendsChanged()
{
	`Log("Friends list changed");
}

/** Logs the change in content to show the event is working */
function ContentChanged()
{
	`Log("Content changed");
}

/** Logs the change in link status to show the event works */
function LinkStatusChanged(bool bIsConnected)
{
	if (bIsConnected == true)
	{
		`Log("Ethernet cable is plugged in");
	}
	else
	{
		`Log("Ethernet cable is unplugged");
	}
}

/** Sets the delegates for notifications */
exec function InitCallbacks()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface != None)
		{
			OnlineSub.PlayerInterface.AddLoginChangeDelegate(LoginChanged);
			OnlineSub.PlayerInterface.AddMutingChangeDelegate(MutingChanged);
			OnlineSub.PlayerInterface.AddFriendsChangeDelegate(0,FriendsChanged);
			OnlineSub.PlayerInterface.AddReadFriendsCompleteDelegate(0,ReadFriendsListDone);
		}
		if (OnlineSub.ContentInterface != None)
		{
			OnlineSub.ContentInterface.AddQueryAvailableDownloadsComplete(0,DownloadableContentQueryDone);
		}

		// Online game related notifications
		if (OnlineSub.GameInterface != None)
		{
			OnlineSub.GameInterface.AddCreateOnlineGameCompleteDelegate(GameCreateDone);
			OnlineSub.GameInterface.AddDestroyOnlineGameCompleteDelegate(GameDestroyDone);
			OnlineSub.GameInterface.AddFindOnlineGamesCompleteDelegate(GameSearchDone);
			OnlineSub.GameInterface.AddJoinOnlineGameCompleteDelegate(GameJoinDone);
			OnlineSub.GameInterface.AddStartOnlineGameCompleteDelegate(GameStartDone);
			OnlineSub.GameInterface.AddEndOnlineGameCompleteDelegate(GameEndDone);
		}
	}
}

/** Shows the messages UI */
exec function ShowMessagesUI()
{
	local OnlinePlayerInterfaceEx LivePI;
	if (OnlineSub != None)
	{
		LivePI = OnlineSub.PlayerInterfaceEx;
		if (LivePI != None)
		{
			if (LivePI.ShowMessagesUI(0))
			{
				`Log("ShowMessagesUI(0) worked");
			}
			else
			{
				`Log("ShowMessagesUI(0) failed");
			}
		}
		else
		{
			`Log("Doesn't support Live extended player interface");
		}
	}
}

/** Shows the gamer card UI for player 1*/
exec function ShowGamerCardUI()
{
	local OnlinePlayerInterfaceEx LivePI;
	local OnlinePlayerInterface PlayerInt;
	local UniqueNetId Xuid1;
	if (OnlineSub != None)
	{
		PlayerInt = OnlineSub.PlayerInterface;
		LivePI = OnlineSub.PlayerInterfaceEx;
		if (LivePI != None && PlayerInt != None)
		{
			PlayerInt.GetUniquePlayerId(1,Xuid1);
			if (LivePI.ShowGamerCardUI(0,Xuid1))
			{
				`Log("ShowGamerCardUI worked");
			}
			else
			{
				`Log("ShowGamerCardUI failed");
			}
		}
		else
		{
			`Log("Doesn't support Live extended player interface");
		}
	}
}

/** Shows the feedback UI for player 1*/
exec function ShowFeedbackUI()
{
	local OnlinePlayerInterfaceEx LivePI;
	local OnlinePlayerInterface PlayerInt;
	local UniqueNetId Xuid1;
	if (OnlineSub != None)
	{
		PlayerInt = OnlineSub.PlayerInterface;
		LivePI = OnlineSub.PlayerInterfaceEx;
		if (LivePI != None && PlayerInt != None)
		{
			PlayerInt.GetUniquePlayerId(1,Xuid1);
			if (LivePI.ShowFeedbackUI(0,Xuid1))
			{
				`Log("ShowFeedbackUI worked");
			}
			else
			{
				`Log("ShowFeedbackUI failed");
			}
		}
		else
		{
			`Log("Doesn't support Live extended player interface");
		}
	}
}

/** Shows the achievements UI */
exec function ShowAchievementsUI()
{
	local OnlinePlayerInterfaceEx LivePI;
	if (OnlineSub != None)
	{
		LivePI = OnlineSub.PlayerInterfaceEx;
		if (LivePI != None)
		{
			if (LivePI.ShowAchievementsUI(0))
			{
				`Log("ShowAchievementsUI(0) worked");
			}
			else
			{
				`Log("ShowAchievementsUI(0) failed");
			}
		}
		else
		{
			`Log("Doesn't support Live extended player interface");
		}
	}
}

/** Shows the players UI */
exec function ShowPlayersUI()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx.ShowPlayersUI(0))
		{
			`Log("ShowPlayersUI(0) worked");
		}
		else
		{
			`Log("ShowPlayersUI(0) failed");
		}
	}
}

/** checks the ethernet link status */
exec function CheckLinkStatus()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.SystemInterface.HasLinkConnection())
		{
			`Log("Ethernet cable is plugged in");
		}
		else
		{
			`Log("Ethernet cable is unplugged");
		}
	}
}

/** Creates an online session with the base defaults and 2 players */
exec function CreateOnlineSession()
{
	local OnlineGameSettings GameSettings;

	if (OnlineSub != None)
	{
		GameSettings = new(Outer) class'ExampleDMOnlineGameSettings';
		GameSettings.NumPublicConnections = 2;
		GameSettings.bIsLanMatch = false;
		// Register the settings and create the session
		if (OnlineSub.GameInterface.CreateOnlineGame(0,GameSettings) == false)
		{
			`Log("Failed to create game session");
		}
	}
}

/** Creates a lan session with the base defaults and 2 players */
exec function CreateLanSession()
{
	local OnlineGameSettings GameSettings;
	if (OnlineSub != None)
	{
		GameSettings = new(Outer) class'ExampleDMOnlineGameSettings';
		GameSettings.NumPublicConnections = 2;
		GameSettings.bIsLanMatch = true;
		// Register the settings and create the session
		if (OnlineSub.GameInterface.CreateOnlineGame(0,GameSettings) == false)
		{
			`Log("Failed to create game session");
		}
	}
}

/** Destroys the current game session */
exec function DestroySession()
{
	if (OnlineSub != None)
	{
		// Shutdown the session
		if (OnlineSub.GameInterface.DestroyOnlineGame() == false)
		{
			`Log("Failed to destroy game session");
		}
	}
}

/** Joins the first available online session */
exec function JoinFirstOnlineSession()
{
	local OnlineGameSearch GameSearch;
	if (OnlineSub != None)
	{
		GameSearch = new(Outer) class'OnlineGameSearch';
		GameSearch.bIsLanQuery = false;
		// Search all sessions
		if (OnlineSub.GameInterface.FindOnlineGames(0,GameSearch) == false)
		{
			`Log("Failed to search for sessions");
		}
	}
}

/** Joins the first available LAN session */
exec function JoinFirstLanSession()
{
	local OnlineGameSearch GameSearch;
	if (OnlineSub != None)
	{
		GameSearch = new(Outer) class'OnlineGameSearch';
		GameSearch.bIsLanQuery = true;
		// Search all sessions
		if (OnlineSub.GameInterface.FindOnlineGames(0,GameSearch) == false)
		{
			`Log("Failed to search for sessions");
		}
	}
}

/** Logs the online game we created has completed */
function GameCreateDone(bool bWasSuccessful)
{
	`Log("Online game creation is finished and was successful "$bWasSuccessful);
}

/** Logs the online game we deleted has completed */
function GameDestroyDone(bool bWasSuccessful)
{
	`Log("Online game destroy is finished and was successful "$bWasSuccessful);
}

/** Logs the online game search we kicked off has completed */
function GameSearchDone(bool bWasSuccessful)
{
	local OnlineGameSearch Search;
	local OnlineGameSearch.OnlineGameSearchResult SearchResult;

	`Log("Online game search is finished and was successful "$bWasSuccessful);

	if (OnlineSub != None)
	{
		// Get the object so we can inspect the results
		Search = OnlineSub.GameInterface.GetGameSearch();
		if (Search.Results.Length > 0)
		{
			//@fixme ronp - TTPRO #40059
			SearchResult = Search.Results[0];
			OnlineSub.GameInterface.JoinOnlineGame(0,SearchResult);
		}
		else
		{
			`Log("No servers available");
		}
	}
}

/** Releases the search results allocated data */
function GameJoinDone(bool bWasSuccessful)
{

	`Log("Online game join is finished and was successful "$bWasSuccessful);

	if (OnlineSub != None)
	{
		OnlineSub.GameInterface.FreeSearchResults();
		`Log("Freed the search data");
	}
}

/**
 * Sets the rich presence context for player 0 to the specified value
 *
 * @param ContextValue the context value to set for the test data (0 or 1)
 */
exec function UsePresence(optional int ValueIndex)
{
	local OnlinePlayerInterfaceEx LivePI;
	local array<LocalizedStringSetting> LocalizedSettings;
	local array<SettingsProperty> Properties;

	if (OnlineSub != None)
	{
		LivePI = OnlineSub.PlayerInterfaceEx;
		if (LivePI != None)
		{
			LocalizedSettings.Length = 1;
			LocalizedSettings[0].AdvertisementType = ODAT_OnlineService;
			// Set to our test context JG_TEST_CONTEXT
			LocalizedSettings[0].Id = 1;
			// Prevent errors and correct bogus context values
			if (ValueIndex != 1 && ValueIndex != 0)
			{
				`Log("Invalid context value specified, using 0");
				ValueIndex = 0;
			}
			// Use the value passed to the exec
			LocalizedSettings[0].ValueIndex = ValueIndex;
			// Hard code the user (0) and the presence string to JG_TEST_PRESENCE
			LivePI.SetRichPresence(0,0,LocalizedSettings,Properties);
		}
	}
}

/**
 * Starts a match that has been registered
 */
exec function StartOnlineGame()
{
	`Log("Starting the online game");

	if (OnlineSub != None)
	{
		OnlineSub.GameInterface.StartOnlineGame();
	}
}

/** Logs the completion of an async task */
function GameStartDone(bool bWasSuccessful)
{
	`Log("Online game starting has finished and was successful "$bWasSuccessful);
}

/** Logs the completion of an async task */
function GameEndDone(bool bWasSuccessful)
{
	`Log("Online game ending has finished and was successful "$bWasSuccessful);
}

/**
 * Ends a match that has been registered and started
 */
exec function EndOnlineGame()
{
	`Log("Ending the online game");

	if (OnlineSub != None)
	{
		OnlineSub.GameInterface.EndOnlineGame();
	}
}

/** Starts a read of the friends data */
exec function ReadFriends()
{
	`Log("Reading the friends list of player 0");

	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.ReadFriendsList(0);
	}
}

/** Dumps all of the friends data when called */
function ReadFriendsListDone(bool bWasSuccessful)
{
	local array<OnlineFriend> Friends;
	local int Index;

	`Log("Friends list request has completed and was successful "$bWasSuccessful);

	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.GetFriendsList(0,Friends);
	}
	// Dump each friend in the list
	for (Index = 0; Index < Friends.Length; Index++)
	{
		`Log("("$Index$") "$Friends[Index].NickName);
		`Log("("$Index$") Presence: "$Friends[Index].PresenceInfo);
		`Log("("$Index$") is online: "$Friends[Index].bIsOnline);
		`Log("("$Index$") is playing: "$Friends[Index].bIsPlaying);
		`Log("("$Index$") is playing this game: "$Friends[Index].bIsPlayingThisGame);
		`Log("("$Index$") is joinable: "$Friends[Index].bIsJoinable);
		`Log("("$Index$") has voice: "$Friends[Index].bHasVoiceSupport);
	}
}

/** Tests the keyboard functionality */
exec function TestKeyboard()
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterfaceEx.AddKeyboardInputDoneDelegate(OnKeyboardEntryDone);
		if (OnlineSub.PlayerInterfaceEx.ShowKeyboardUI(0,"This is Joe's cool title",
			"Please enter Joe's name",true,"This is Joe's default string"))
		{
			`Log("Failed to show the virtual keyboard");
		}
	}
}

function OnKeyboardEntryDone(bool bWasSuccessful)
{
	if (OnlineSub != None)
	{
		if (bWasSuccessful)
		{
			`Log("User entered ("$OnlineSub.PlayerInterfaceEx.GetKeyboardInputResults()$")");
		}
		else
		{
			`Log("Failed to read keyboad input or was cancelled by user");
		}
	}
}

/** Tests the non-friend invite functionality */
exec function ShowInviteUI()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx.ShowInviteUI(0,"Come play with Joe") == false)
		{
			`Log("ShowInviteUI() failed");
		}
	}
}

/** Tests the content marketplace functionality */
exec function TestContentMarketplace()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx.ShowContentMarketplaceUI(0) == false)
		{
			`Log("ShowContentMarketplaceUI() failed");
		}
	}
}

/** Tests the membership marketplace functionality */
exec function TestMembershipMarketplace()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx.ShowMembershipMarketplaceUI(0) == false)
		{
			`Log("ShowMembershipMarketplaceUI() failed");
		}
	}
}

/** Tests the device selection functionality */
exec function TestDeviceSelection()
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterfaceEx.AddDeviceSelectionDoneDelegate(0,OnDeviceSelectionComplete);
		if (OnlineSub.PlayerInterfaceEx.ShowDeviceSelectionUI(0,100) == false)
		{
			`Log("ShowDeviceSelectionUI() failed");
		}
	}
}

/** Tests the device selection functionality */
exec function TestDeviceSelection2()
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterfaceEx.AddDeviceSelectionDoneDelegate(0,OnDeviceSelectionComplete);
		if (OnlineSub.PlayerInterfaceEx.ShowDeviceSelectionUI(0,100,true) == false)
		{
			`Log("ShowDeviceSelectionUI() failed");
		}
	}
}

function OnDeviceSelectionComplete(bool bWasSuccessful)
{
`if(`notdefined(FINAL_RELEASE))
	local int DeviceId;
    local string DeviceName;

	if (OnlineSub != None)
	{
		if (bWasSuccessful == true)
		{
			DeviceId = OnlineSub.PlayerInterfaceEx.GetDeviceSelectionResults(0,DeviceName);
			`Log("User selected device id ("$DeviceId$") "$DeviceName);
		}
		else
		{
			`Log("User device selection failed or was canceled by user");
		}
	}
`endif
}

exec function ReadProfile()
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.ReadProfileSettings(0,ProfileSettings);
	}
}

exec function WriteProfile()
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.WriteProfileSettings(0,ProfileSettings);
	}
}

/** Resets a profile to the default data */
exec function ResetProfile()
{
	local ExampleProfileSettings ProfSettings;
	if (OnlineSub != None)
	{
		ProfSettings = new(Outer) class'ExampleProfileSettings';
		ProfSettings.SetToDefaults();
		OnlineSub.PlayerInterface.WriteProfileSettings(0,ProfSettings);
	}
}

/**
 * Adds an achievement to the signed in user
 *
 * @param AchievementId the achievement to unlock
 */
exec function UnlockAchievement(optional int AchievementId)
{
	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterfaceEx.AddUnlockAchievementCompleteDelegate(0,AchievementDone);
		if (OnlineSub.PlayerInterfaceEx.UnlockAchievement(0,AchievementId) == false)
		{
			`Log("UnlockAchievement(0,"$AchievementId$") failed");
		}
	}
}

/** Shows the achievement UI so you can enjoy the new shiny achievement */
function AchievementDone(bool bWasSuccessful)
{
	if (bWasSuccessful == false)
	{
		`Log("Failed to unlock achievement");
	}

	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx.ShowAchievementsUI(0) == false)
		{
			`Log("ShowAchievementsUI(0) failed");
		}
	}
}

/**
 * Adds to your orc kills stat the number specified
 *
 * @param Number the amount to add
 */
exec function KillOrcs(int Number)
{
	StatsWrite.AddToOrcsKilled(Number);
}

/**
 * Adds to your gold collected stat the number specified
 *
 * @param Amount the amount to add
 */
exec function GatherGold(int Amount)
{
	StatsWrite.AddToGold(Amount);
}

/**
 * Sets your accuracy rating
 *
 * @param Rating the rating to use
 */
exec function SetLongBowRating(float Rating)
{
	StatsWrite.SetLongBowAccuracy(Rating);
}

/** Writes stats to the online subsystem stats cache */
exec function WriteStats()
{
	local UniqueNetId PlayerId;

	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.GetUniquePlayerId(0,PlayerId);
		OnlineSub.StatsInterface.WriteOnlineStats(PlayerId,StatsWrite);
	}
}

/** Reads stats from the online persistent storage */
exec function ReadStats()
{
	local UniqueNetId PlayerId;
	local array<UniqueNetId> Players;

	if (OnlineSub != None)
	{
		OnlineSub.PlayerInterface.GetUniquePlayerId(0,PlayerId);
		Players[0] = PlayerId;
		OnlineSub.StatsInterface.ReadOnlineStats(Players,StatsRead);
	}
}

/** Reads stats from the online persistent storage */
exec function ReadStatsCentered()
{
	if (OnlineSub != None)
	{
		OnlineSub.StatsInterface.ReadOnlineStatsByRankAroundPlayer(0,StatsRead);
	}
}

/** Reads stats from the online persistent storage */
exec function ReadStatsByRank()
{
	if (OnlineSub != None)
	{
		OnlineSub.StatsInterface.ReadOnlineStatsByRank(StatsRead,1,10);
	}
}

/** Reads stats from the online persistent storage */
exec function ReadStatsForFriends()
{
	if (OnlineSub != None)
	{
		OnlineSub.StatsInterface.ReadOnlineStatsForFriends(0,StatsRead);
	}
}

/** Flushes any cached stats to the permanent storage */
exec function FlushStats()
{
	if (OnlineSub != None)
	{
		OnlineSub.StatsInterface.FlushOnlineStats();
	}
}

/** Function to query if there are any downloable content packages available */
exec function QueryDownloableContent()
{
	if (OnlineSub != None && OnlineSub.ContentInterface != None)
	{
		OnlineSub.ContentInterface.QueryAvailableDownloads(0);
	}
}

/** Delegate that logs the content availability */
function DownloadableContentQueryDone(bool bWasSuccessful)
{
	local int NewDownloads;
	local int TotalDownloads;
	if (OnlineSub != None && OnlineSub.ContentInterface != None)
	{
		if (bWasSuccessful)
		{
			OnlineSub.ContentInterface.GetAvailableDownloadCounts(0,NewDownloads,TotalDownloads);
		}
		else
		{
			`Log("Query for downloadable content counts failed");
		}
	}
	`Log("New downloads: "$NewDownloads$" Total downloads: "$TotalDownloads);
}

/** Pops up the online menus for testing purposes */
exec function ShowOnlineMenus()
{
	local UIInteraction UIController;

	UIController = GetUIController();
	if ( UIController != None )
	{
		UIController.OpenScene( UIScene'ExampleGameMenus.ExampleGameMainMenu', LocalPlayer(Player) );
	}
}

/** Registers the talker delegate so I can log who is talking */
exec function RegisterTalkerDelegate()
{
	if (OnlineSub != None && OnlineSub.VoiceInterface != None)
	{
		OnlineSub.VoiceInterface.AddPlayerTalkingDelegate(OnPlayerTalking);
	}
}

function OnPlayerTalking(UniqueNetId TalkingPlayer)
{
	`Log("UniqueNetId for talking player is ("$TalkingPlayer.Uid[0]$","$TalkingPlayer.Uid[1]$","$TalkingPlayer.Uid[2]$","$TalkingPlayer.Uid[3]$","$TalkingPlayer.Uid[4]$","$TalkingPlayer.Uid[5]$","$TalkingPlayer.Uid[6]$","$TalkingPlayer.Uid[7]$")");
}

/** Unlocks a gamer picture for player 0 */
exec function UnlockPicture()
{
	if (OnlineSub != None && OnlineSub.PlayerInterfaceEx != None)
	{
		if (OnlineSub.PlayerInterfaceEx.UnlockGamerPicture(0,1) == false)
		{
			`Log("UnlockGamerPicture() failed");
		}
	}
}

/** Dumps the connected status for the first 4 controllers */
exec function CheckControllerStates()
{
	local int ControllerIndex;
	local OnlineSystemInterface SysInt;
	if (OnlineSub != None)
	{
		SysInt = OnlineSub.SystemInterface;
		if (SysInt != None)
		{
			// Log each controllers connection state
			for (ControllerIndex = 0; ControllerIndex < 4; ControllerIndex++)
			{
				if (SysInt.IsControllerConnected(ControllerIndex))
				{
					`Log("Controller "$ControllerIndex$" is connected");
				}
				else
				{
					`Log("Controller "$ControllerIndex$" is not connected");
				}
			}
		}
	}
}

/**
 * Reads the NAT type for this machine
 */
exec function ReadNatType()
{
	local OnlineSystemInterface SysInt;

	if (OnlineSub != None)
	{
		SysInt = OnlineSub.SystemInterface;
		if (SysInt != None)
		{
			// Read and log the type
			switch (SysInt.GetNATType())
			{
				case NAT_Open:
					`Log("NAT type is Open");
					break;
				case NAT_Moderate:
					`Log("NAT type is Moderate");
					break;
				case NAT_Strict:
					`Log("NAT type is Strict");
					break;
				case NAT_Unknown:
					`Log("NAT type is Unknown");
					break;
			}
		}
	}
}

/**
 * Checks to see if the specified device is valid or not
 *
 * @param DeviceId the device to check
 */
exec function TestDeviceId(optional int DeviceId = 1)
{
	local OnlinePlayerInterfaceEx ExInt;

	if (OnlineSub != None)
	{
		ExInt = OnlineSub.PlayerInterfaceEx;
		if (ExInt != None)
		{
			if (ExInt.IsDeviceValid(DeviceId))
			{
				`Log("Device "$DeviceId$" is valid");
			}
			else
			{
				`Log("Device "$DeviceId$" is *not* valid");
			}
		}
	}
}

/**
 * Registers an arbitrated match
 */
exec function ArbitrationTest()
{
	`Log("Registering with arbitration");

	if (OnlineSub != None)
	{
		OnlineSub.GameInterface.AddArbitrationRegistrationCompleteDelegate(ArbitrationRegistrationComplete);
		OnlineSub.GameInterface.RegisterForArbitration();
	}
}

/**
 * Callback for arbitration registration results
 *
 * @param bWasSuccessful whether the call completed ok or not
 */
function ArbitrationRegistrationComplete(bool bWasSuccessful)
{
	local array<OnlineArbitrationRegistrant> Registrants;
	local int Index;

	OnlineSub.GameInterface.ClearArbitrationRegistrationCompleteDelegate(ArbitrationRegistrationComplete);
	if (bWasSuccessful)
	{
		`Log("Arbitration returned:");
		// Read the results and dump them
		Registrants = OnlineSub.GameInterface.GetArbitratedPlayers();
		// Iterate through them and dump their info
		for (Index = 0; Index < Registrants.Length; Index++)
		{
			`Log("Player is ("$Registrants[Index].PlayerId.Uid[0]$","$Registrants[Index].PlayerId.Uid[1]$","$Registrants[Index].PlayerId.Uid[2]$","$Registrants[Index].PlayerId.Uid[3]$","$Registrants[Index].PlayerId.Uid[4]$","$Registrants[Index].PlayerId.Uid[5]$","$Registrants[Index].PlayerId.Uid[6]$","$Registrants[Index].PlayerId.Uid[7]$") trusted as "$Registrants[Index].Trustworthiness);
		}
	}
	else
	{
		`Log("Failed to register for arbitration");
	}
}

defaultproperties
{
	CameraClass=class'ExamplePlayerCamera'
	CheatClass=class'ExampleCheatManager'
	
	HoldDistanceMin=50.0
	HoldDistanceMax=750.0
	WeaponImpulse=600.0
	ThrowImpulse=800.0
	ChangeHoldDistanceIncrement=50.0

	Begin Object Class=RB_Handle Name=RB_Handle0
	End Object
	PhysicsGrabber=RB_Handle0
	Components.Add(RB_Handle0)
}

