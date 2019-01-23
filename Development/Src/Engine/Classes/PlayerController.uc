//=============================================================================
// PlayerController
//
// PlayerControllers are used by human players to control pawns.
//
// This is a built-in Unreal class and it shouldn't be modified.
// for the change in Possess().
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class PlayerController extends Controller
	config(Game)
	native
	nativereplication
	dependson(MusicTrackDataStructures,OnlineSubsystem);

`include(Core/Globals.uci)

var const			Player			Player;						// Player info
var 				Camera			PlayerCamera;				// Camera associated with this Player Controller
var const class<Camera>				CameraClass;

var DebugCameraController           DebugCameraControllerRef;
var class<DebugCameraController>    DebugCameraControllerClass;

/**
 * The class to use for the player owner data store.
 */
var	const	class<PlayerOwnerDataStore>		PlayerOwnerDataStoreClass;

/**
 * The data store instance responsible for presenting state data for this player.
 */
var	protected		PlayerOwnerDataStore		CurrentPlayerData;

/**
 * The data store instance responsible for loading and saving settings data for this player.
 */
var	protected		UIDataStore_PlayerSettings	CurrentPlayerSettings;

// Player control flags

var					bool			bFrozen;					// Set when game ends or player dies to temporarily prevent player from restarting (until cleared by timer)
var					bool			bPressedJump;
var					bool			bDoubleJump;
var					bool			bUpdatePosition;
var					bool			bUpdating;
var globalconfig	bool			bNeverSwitchOnPickup;		// If true, don't automatically switch to picked up weapon
var					bool			bCheatFlying;				// instantly stop in flying mode
var					bool			bCameraPositionLocked;
var bool	bShortConnectTimeOut;	// when true, reduces connect timeout to 15 seconds
var const bool	bPendingDestroy;		// when true, playercontroller is being destroyed
var bool bWasSpeedHack;
var const bool bWasSaturated;		// used by servers to identify saturated client connections
var globalconfig bool bDynamicNetSpeed;
var globalconfig bool bAimingHelp;

var float MaxResponseTime;		 // how long server will wait for client move update before setting position
var					float			WaitDelay;					// Delay time until can restart
var					pawn			AcknowledgedPawn;			// Used in net games so client can acknowledge it possessed a pawn

var					eDoubleClickDir	DoubleClickDir;				// direction of movement key double click (for special moves)

// Camera info.
var const			actor						ViewTarget;
var const			PlayerReplicationInfo		RealViewTarget;

/** field of view angle in degrees */
var	float			FOVAngle;
var 	float			DesiredFOV;
var 	float			DefaultFOV;
/** last used FOV based multiplier to distance to an object when determining if it exceeds the object's cull distance
 * @note: only valid on client
 */
var const float LODDistanceFactor;

// Remote Pawn ViewTargets
var					rotator			TargetViewRotation;
var					float			TargetEyeHeight;

/** used for smoothing the viewrotation of spectated players */
var rotator BlendedTargetViewRotation;

var					HUD				myHUD;						// heads up display info

// Move buffering for network games.  Clients save their un-acknowledged moves in order to replay them
// when they get position updates from the server.

/** SavedMoveClass should be changed for network player move replication where different properties need to be replicated from the base engine implementation.*/
var					class<SavedMove> SavedMoveClass;
var					SavedMove		SavedMoves;					// buffered moves pending position updates
var					SavedMove		FreeMoves;					// freed moves, available for buffering
var					SavedMove		PendingMove;
var					vector			LastAckedAccel;				// last acknowledged sent acceleration
var					float			CurrentTimeStamp;
var					float			LastUpdateTime;
var					float			ServerTimeStamp;
var					float			TimeMargin;
var					float			ClientUpdateTime;
var 	float			MaxTimeMargin;
var float LastActiveTime;		// used to kick idlers

var int ClientCap;

// ping replication and netspeed adjustment based on ping
var globalconfig float DynamicPingThreshold;
var					float			LastPingUpdate;
var float OldPing;
var float LastSpeedHackLog;

/** MAXPOSITIONERRORSQUARED is the square of the max position error that is accepted (not corrected) in net play */
const MAXPOSITIONERRORSQUARED = 3.0;

/** MAXNEARZEROVELOCITYSQUARED is the square of the max velocity that is considered zero (not corrected) in net play */
const MAXNEARZEROVELOCITYSQUARED = 9.0;

/** MAXVEHICLEPOSITIONERRORSQUARED is the square of the max position error that is accepted (not corrected) in net play when driving a vehicle*/
const MAXVEHICLEPOSITIONERRORSQUARED = 900.0;

/** CLIENTADJUSTUPDATECOST is the bandwidth cost in bytes of sending a client adjustment update. 180 is greater than the actual cost, but represents a tweaked value reserving enough bandwidth for
other updates sent to the client.  Increase this value to reduce client adjustment update frequency, or if the amount of data sent in the clientadjustment() call increases */
const CLIENTADJUSTUPDATECOST=180.0;

/** MAXCLIENTUPDATEINTERVAL is the maximum time between movement updates from the client before the server forces an update. */
const MAXCLIENTUPDATEINTERVAL=0.25;

// ClientAdjustPosition replication (event called at end of frame)
struct native ClientAdjustment
{
    var float TimeStamp;
    var EPhysics newPhysics;
    var vector NewLoc;
    var vector NewVel;
    var actor NewBase;
    var vector NewFloor;
	var byte bAckGoodMove;
};
var ClientAdjustment PendingAdjustment;

// Progess Indicator - used by the engine to provide status messages (HUD is responsible for displaying these).
var					string			ProgressMessage[2];
var					color			ProgressColor[2];
var					float			ProgressTimeOut;

// Localized strings
var localized		string			QuickSaveString;
var localized		string			NoPauseMessage;
var localized		string			ViewingFrom;
var localized		string			OwnCamera;

var					int				GroundPitch;

var					vector			OldFloor;				// used by PlayerSpider mode - floor for which old rotation was based;

// Components ( inner classes )
var transient	CheatManager			CheatManager;		// Object within playercontroller that manages "cheat" commands
var						class<CheatManager>		CheatClass;			// class of my CheatManager

var()	transient	editinline	PlayerInput			PlayerInput;		// Object within playercontroller that manages player input.
var						class<PlayerInput>		InputClass;			// class of my PlayerInput

var const				vector					FailedPathStart;
var						CylinderComponent		CylinderComponent;
// Manages gamepad rumble (within)
var config string ForceFeedbackManagerClassName;
var transient ForceFeedbackManager ForceFeedbackManager;

// Interactions.
var	transient			array<interaction>		Interactions;

/** List of players that are explicitly muted (outside of gameplay) */
var array<UniqueNetId> VoiceMuteList;

/** List of players muted via gameplay */
var array<UniqueNetId> GameplayVoiceMuteList;

/** The list of combined players to filter voice packets for */
var array<UniqueNetId> VoicePacketFilter;

/** Cached online subsystem variable */
var OnlineSubsystem OnlineSub;

/** Cached online voice interface variable */
var OnlineVoiceInterface VoiceInterface;

/** The data store that holds any online player data */
var UIDataStore_OnlinePlayerData OnlinePlayerData;

/** Ignores movement input. Stacked state storage, Use accessor function IgnoreMoveInput() */
var byte	bIgnoreMoveInput;
/** Ignores look input. Stacked state storage, use accessor function IgnoreLookInput(). */
var byte	bIgnoreLookInput;

/** Maximum distance to search for interactable actors */
var config float InteractDistance;

/** Is this player currently in cinematic mode?  Prevents rotation/movement/firing/etc */
var		bool		bCinematicMode;

/** The state of the inputs from cinematic mode */
var bool bCinemaDisableInputMove, bCinemaDisableInputLook;

// PLAYER INPUT MATCHING =============================================================

/** Type of inputs the matching code recognizes */
enum EInputTypes
{
	IT_XAxis,
	IT_YAxis,
};

/** How to match an input action */
enum EInputMatchAction
{
	IMA_GreaterThan,
	IMA_LessThan
};

/** Individual entry to input matching sequences */
struct native InputEntry
{
	/** Type of input to match */
	var EInputTypes Type;

	/** Min value required to consider as a valid match */
	var	float Value;

	/** Max amount of time since last match before sequence resets */
	var	float TimeDelta;

	/** What type of match is this? */
	var EInputMatchAction Action;
};

/**
 * Contains information to match a series of a inputs and call the given
 * function upon a match.  Processed by PlayerInput, defined in the
 * PlayerController.
 */
struct native InputMatchRequest
{
	/** Number of inputs to match, in sequence */
	var array<InputEntry> Inputs;

	/** Actor to call below functions on */
	var Actor MatchActor;

	/** Name of function to call upon successful match */
	var Name MatchFuncName;

	/** Name of function to call upon a failed partial match */
	var Name FailedFuncName;

	/** Name of this input request, mainly for debugging */
	var Name RequestName;

	/** Current index into Inputs that is being matched */
	var transient int MatchIdx;

	/** Last time an input entry in Inputs was matched */
	var transient float LastMatchTime;
};
var array<InputMatchRequest> InputRequests;

// MISC VARIABLES ====================================================================

var input byte bRun, bDuck;

var float LastBroadcastTime;
var string LastBroadcastString[4];

var bool	bReplicateAllPawns;			// if true, all pawns will be considered relevant

/** list of names of levels the server is in the middle of sending us for a PrepareMapChange() call */
var array<name> PendingMapChangeLevelNames;

/** Whether this controller is using streaming volumes  **/
var bool bIsUsingStreamingVolumes;

/** True if there is externally controlled UI that should pause the game */
var bool bIsExternalUIOpen;

/** True if the controller is connected for this player */
var bool bIsControllerConnected;

/** If true, do a trace to check if sound is occluded, and reduce the effective sound radius if so */
var bool bCheckSoundOcclusion;

/** handles copying and replicating old cover changes from WorldInfo.CoverReplicatorBase on creation as well as replicating new changes */
var CoverReplicator MyCoverReplicator;

/** List of actors and debug text to draw, @see AddDebugText(), RemoveDebugText(), and DrawDebugTextList() */
struct native DebugTextInfo
{
	/** Actor to draw DebugText over */
	var Actor SrcActor;
	/** Offset from SrcActor.Location to apply */
	var vector SrcActorOffset;
	/** Desired offset to interpolate to */
	var vector SrcActorDesiredOffset;
	/** Text to display */
	var string DebugText;
	/** Time remaining for the debug text, -1.f == infinite */
	var transient float TimeRemaining;
	/** Duration used to lerp desired offset */
	var float Duration;
	/** Text color */
	var color TextColor;
};
var private array<DebugTextInfo> DebugTextList;

/** How fast spectator camera is allowed to move */
var float SpectatorCameraSpeed;

/** index identifying players using the same base connection (splitscreen clients)
 * Used by netcode to match replicated PlayerControllers to the correct splitscreen viewport and child connection
 * replicated via special internal code, not through normal variable replication
 */
var const duplicatetransient byte NetPlayerIndex;

/** minimum time before can respawn after dying */
var float MinRespawnDelay;

/** component pooling for sounds played through PlaySound()/ClientHearSound() */
var globalconfig int MaxConcurrentHearSounds;
var array<AudioComponent> HearSoundActiveComponents;
var array<AudioComponent> HearSoundPoolComponents;
/** option to print out list of sounds when MaxConcurrentHearSounds is exceeded */
var globalconfig bool bLogHearSoundOverflow;

/** The actors which the camera shouldn't see. Used to hide actors which the camera penetrates.
This array used only if PlayerController does not have a playercamera */
var array<Actor> HiddenActors;

cpptext
{
	//  PlayerController interface.
	void SetPlayer( UPlayer* Player );
	void UpdateViewTarget(AActor* NewViewTarget);
	virtual void SmoothTargetViewRotation(APawn* TargetPawn, FLOAT DeltaSeconds);
	/** allows the game code an opportunity to modify post processing settings
	 * @param PPSettings - the post processing settings to apply
	 */
	virtual void ModifyPostProcessSettings(FPostProcessSettings& PPSettings) const;

	// AActor interface.
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );
	virtual UBOOL IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation);
	virtual UBOOL LocalPlayerController();
	virtual UBOOL WantsLedgeCheck();
	virtual UBOOL StopAtLedge();
	virtual APlayerController* GetAPlayerController() { return this; }
	virtual UBOOL IgnoreBlockingBy( const AActor *Other ) const;
	virtual void HearSound(USoundCue* InSoundCue, AActor* SoundPlayer, const FVector& SoundLocation, UBOOL bStopWhenOwnerDestroyed);
	virtual void PostScriptDestroyed();
	virtual FLOAT GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, FLOAT Time, UBOOL bLowBandwidth);

	/** disables SeePlayer() and SeeMonster() checks for PlayerController, since they aren't used for most games */
	virtual UBOOL ShouldCheckVisibilityOf(AController* C) { return FALSE; }
	virtual void UpdateHiddenActors(FVector ViewLocation) {}
}

replication
{
	// Things the server should send to the client.
	if ( bNetOwner && Role==ROLE_Authority && (ViewTarget != Pawn) && (Pawn(ViewTarget) != None) )
		TargetViewRotation, TargetEyeHeight;
}

// DEBUG
reliable client function ClientDrawCoordinateSystem( vector AxisLoc, Rotator AxisRot, float Scale, optional bool bPersistentLines )
{
	`log("ClientDrawCoordinateSystem");
	DrawDebugCoordinateSystem( AxisLoc, AxisRot, Scale, bPersistentLines );
}

// DEBUG END

native final function SetNetSpeed(int NewSpeed);
native final function string GetPlayerNetworkAddress();
native final function string GetServerNetworkAddress();
native function string ConsoleCommand(string Command, optional bool bWriteToLog = true);

/** travel to a different map
 * @param URL the URL to travel to
 * @param TravelType type of URL
 * @param bSeamless whether to use seamless travel (requires TravelType of TRAVEL_Relative)
 */
reliable client native event ClientTravel(string URL, ETravelType TravelType, optional bool bSeamless = false);

native(546) final function UpdateURL(string NewOption, string NewValue, bool bSave1Default);
native final function string GetDefaultURL(string Option);
// Execute a console command in the context of this player, then forward to Actor.ConsoleCommand.
native function CopyToClipboard( string Text );
native function string PasteFromClipboard();

/** Whether or not to allow mature language **/
native function SetAllowMatureLanguage( bool bAllowMatureLanguge );

/** Sets the Audio Group to this the value passed in **/
native function SetAudioGroupVolume( name GroupName, float Volume );


// Validation.
reliable client private native event ClientValidate(string C);
reliable server private native event ServerValidationResponse(string Response);

native final function bool CheckSpeedHack(float DeltaTime);

/* FindStairRotation()
returns an integer to use as a pitch to orient player view along current ground (flat, up, or down)
*/
native(524) final function int FindStairRotation(float DeltaTime);

/**
 * Attempts to pause/unpause the game when the UI opens/closes. Note: pausing
 * only happens in standalone mode
 *
 * @param bIsOpening whether the UI is opening or closing
 */
function OnExternalUIChanged(bool bIsOpening)
{
	bIsExternalUIOpen = bIsOpening;
	SetPause(bIsOpening,CanUnpauseExternalUI);
}

/** Callback that checks the external UI state before allowing unpause */
function bool CanUnpauseExternalUI()
{
	return bIsExternalUIOpen == false;
}


/** called when the actor falls out of the world 'safely' (below KillZ and such) */
simulated event FellOutOfWorld(class<DamageType> dmgType)
{
}

/**
 * Attempts to pause/unpause the game when a controller becomes
 * disconnected/connected
 *
 * @param ControllerId the id of the controller that changed
 * @param bIsConnected whether the controller is connected or not
 */
function OnControllerChanged(int ControllerId,bool bIsConnected)
{
	local LocalPlayer LocPlayer;
	// Don't worry about remote players
	LocPlayer = LocalPlayer(Player);
	// If the controller that changed, is attached to the this playercontroller
	if (LocPlayer != None && LocPlayer.ControllerId == ControllerId)
	{
		bIsControllerConnected = bIsConnected;
		`Log("Controller "$ControllerId$" is connected == "$bIsConnected);
		// Pause if the controller was removed, otherwise unpause
		SetPause(bIsConnected == false,CanUnpauseControllerConnected);
	}
}

/** Callback that checks to see if the controller is connected before unpausing */
function bool CanUnpauseControllerConnected()
{
	return bIsControllerConnected;
}

/** spawns MyCoverReplicator and tells it to replicate any changes that have already occurred */
function CoverReplicator SpawnCoverReplicator()
{
	if (MyCoverReplicator == None && Role == ROLE_Authority && LocalPlayer(Player) == None)
	{
		MyCoverReplicator = Spawn(class'CoverReplicator', self);
		MyCoverReplicator.ReplicateInitialCoverInfo();
	}
	return MyCoverReplicator;
}

simulated event PostBeginPlay()
{
	super.PostBeginPlay();

	ResetCameraMode();

	MaxTimeMargin = class'GameInfo'.Default.MaxTimeMargin;
	MaxResponseTime = Default.MaxResponseTime * WorldInfo.TimeDilation;

	if ( WorldInfo.NetMode == NM_Client )
	{
		SpawnDefaultHUD();
	}
	else if ( WorldInfo.Game.AllowCheats(self) )
	{
		AddCheats();
	}

	SetViewTarget(self);  // MUST have a view target!
	LastActiveTime = WorldInfo.TimeSeconds;

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
}

/**
 * Called after this PlayerController's viewport/net connection is associated with this player controller.
 */
simulated event ReceivedPlayer()
{
	RegisterPlayerDataStores();
}


event PreRender(Canvas Canvas);

function ResetTimeMargin()
{
    TimeMargin = -0.1;
	MaxTimeMargin = class'GameInfo'.Default.MaxTimeMargin;
}

reliable server function ServerShortTimeout()
{
	local Actor A;

	if (!bShortConnectTimeout)
	{
		bShortConnectTimeOut = true;
		ResetTimeMargin();

		// quick update of pickups and gameobjectives since this player is now relevant
		if (WorldInfo.Pauser != None)
		{
			// update everything immediately, as TimeSeconds won't get advanced while paused
			// so otherwise it won't happen at all until the game is unpaused
			// this floods the network, but we're paused, so no gameplay is going on that would care much
			foreach AllActors(class'Actor', A)
			{
				if (!A.bOnlyRelevantToOwner)
				{
					A.NetUpdateTime = WorldInfo.TimeSeconds - 1.0f;
				}
			}
		}
		else if ( WorldInfo.Game.NumPlayers < 8 )
		{
			foreach AllActors(class'Actor', A)
			{
				if ( (A.NetUpdateFrequency < 1) && !A.bOnlyRelevantToOwner )
				{
					A.NetUpdateTime = FMin(A.NetUpdateTime, WorldInfo.TimeSeconds + 0.2 * FRand());
				}
			}
		}
		else
		{
			foreach AllActors(class'Actor', A)
			{
				if ( (A.NetUpdateFrequency < 1) && !A.bOnlyRelevantToOwner )
				{
					A.NetUpdateTime = FMin(A.NetUpdateTime, WorldInfo.TimeSeconds + 0.5 * FRand());
				}
			}
		}
	}
}

function ServerGivePawn()
{
	GivePawn(Pawn);
}

event KickWarning()
{
	ReceiveLocalizedMessage( class'GameMessage', 15 );
}

function AddCheats()
{
	// Assuming that this never gets called for NM_Client
	if ( CheatManager == None && (WorldInfo.NetMode == NM_Standalone) )
		CheatManager = new(Self) CheatClass;
}

exec function EnableCheats()
{
	AddCheats();
}

/* SpawnDefaultHUD()
Spawn a HUD (make sure that PlayerController always has valid HUD, even if \
ClientSetHUD() hasn't been called\
*/
function SpawnDefaultHUD()
{
	if ( LocalPlayer(Player) == None )
		return;
	`log(GetFuncName());
	myHUD = spawn(class'HUD',self);
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	local vehicle DrivenVehicle;

    DrivenVehicle = Vehicle(Pawn);
	if( DrivenVehicle != None )
		DrivenVehicle.DriverLeave(true); // Force the driver out of the car

	if ( Pawn != None )
	{
		PawnDied( Pawn );
		UnPossess();
	}

	super.Reset();

	SetViewTarget( Self );
    ResetCameraMode();
	WaitDelay = WorldInfo.TimeSeconds + 2;
    FixFOV();
    if ( PlayerReplicationInfo.bOnlySpectator )
    	GotoState('Spectating');
    else
		GotoState('PlayerWaiting');
}

reliable client function ClientReset()
{
	ResetCameraMode();
	SetViewTarget(self);
	GotoState(PlayerReplicationInfo.bOnlySpectator ? 'Spectating' : 'PlayerWaiting');
}

function CleanOutSavedMoves()
{
	SavedMoves = None;
	PendingMove = None;
}

/* InitInputSystem()
Spawn the appropriate class of PlayerInput
Only called for playercontrollers that belong to local players
*/
event InitInputSystem()
{
	local Class<ForceFeedbackManager> FFManagerClass;
	local int i;
	local Sequence GameSeq;
	local array<SequenceObject> AllInterpActions;

	if (PlayerInput == None)
	{
		Assert(InputClass != None);
		PlayerInput = new(Self) InputClass;
	}

	if ( Interactions.Find(PlayerInput) == -1 )
	{
		Interactions[Interactions.Length] = PlayerInput;
	}

	// Spawn the waveform manager here
	if (ForceFeedbackManagerClassName != "")
	{
		FFManagerClass = class<ForceFeedbackManager>(DynamicLoadObject(ForceFeedbackManagerClassName,class'Class'));
		if (FFManagerClass != None)
		{
			ForceFeedbackManager = new(Self) FFManagerClass;
		}
	}

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	// If there is an online subsystem, add our callback for UI changes
	if (OnlineSub != None)
	{
		VoiceInterface = OnlineSub.VoiceInterface;
		if (OnlineSub.SystemInterface != None)
		{
			// Register the callback for when external UI is shown/hidden
			// This will pause/unpause a single player game based upon the UI state
			OnlineSub.SystemInterface.AddExternalUIChangeDelegate(OnExternalUIChanged);
			// This will pause/unpause a single player game based upon the controller state
			OnlineSub.SystemInterface.AddControllerChangeDelegate(OnControllerChanged);
		}
		// Register for accepting game invites if possible
		if (OnlineSub.GameInterface != None && LocalPlayer(Player) != None)
		{
			OnlineSub.GameInterface.AddGameInviteAcceptedDelegate(LocalPlayer(Player).ControllerId,OnGameInviteAccepted);
		}
	}

	// add the player to any matinees running so that it gets in on any cinematics already running, etc
	GameSeq = WorldInfo.GetGameSequence();
	if (GameSeq != None)
	{
		// find any matinee actions that exist
		GameSeq.FindSeqObjectsByClass(class'SeqAct_Interp', true, AllInterpActions);

		// tell them all to add this PC to any running Director tracks
		for (i = 0; i < AllInterpActions.Length; i++)
		{
			SeqAct_Interp(AllInterpActions[i]).AddPlayerToDirectorTracks(self);
		}
	}
}

/**
 * Called when a variable with the property flag "RepNotify" is replicated
 *
 * @param VarName the variable that was just replicated
 */
simulated event ReplicatedEvent(name VarName)
{
	local UniqueNetId ZeroUniqueId;

	Super.ReplicatedEvent(VarName);

	if (VarName == 'PlayerReplicationInfo')
	{
		// Now the PRI is valid so we can use it for the UniqueId
		if ( (PlayerReplicationInfo != None) && (ZeroUniqueId == PlayerReplicationInfo.UniqueId) )
		{
			InitUniquePlayerId();
		}
	}
}

/**
 * Used to have script initialize the unique player id. This is the id used
 * in all network calls.
 */
event InitUniquePlayerId()
{
	local LocalPlayer LocPlayer;
	local OnlineGameSettings GameSettings;

	LocPlayer = LocalPlayer(Player);
	// If we have both a local player and the online system, register ourselves
	if (LocPlayer != None &&
		PlayerReplicationInfo != None &&
		OnlineSub != None &&
		OnlineSub.PlayerInterface != None)
	{
		// Get our local id from the online subsystem
		OnlineSub.PlayerInterface.GetUniquePlayerId(LocPlayer.ControllerId,PlayerReplicationInfo.UniqueId);
		if (WorldInfo.NetMode == NM_Client)
		{
			// Grab the game so we can check for being invited
			if (OnlineSub.GameInterface != None)
			{
				GameSettings = OnlineSub.GameInterface.GetGameSettings();
			}
			ServerSetUniquePlayerId(PlayerReplicationInfo.UniqueId,
				GameSettings != None && GameSettings.bWasFromInvite);
		}
	}
}

/**
 * Registers the unique id of the player with the server so it can be replicated
 * to all clients.
 *
 * @param UniqueId the buffer that holds the unique id
 * @param bWasInvited whether the player was invited to play or is joining via search
 */
reliable server function ServerSetUniquePlayerId(UniqueNetId UniqueId,bool bWasInvited)
{
	// Store the unique id, so it will be replicated to all clients
	PlayerReplicationInfo.UniqueId = UniqueId;

	if (OnlineSub != None && OnlineSub.GameInterface != None)
	{
		// Go ahead and register the player as part of the session
		OnlineSub.GameInterface.RegisterPlayer(PlayerReplicationInfo.UniqueId,bWasInvited);
	}
	// Notify the game that we can now be muted and mute others
	if (WorldInfo.NetMode != NM_Client)
	{
		WorldInfo.Game.UpdateGameplayMuteList(self);
		// Now that the unique id is replicated, this player can contribute to skill
		WorldInfo.Game.RecalculateSkillRating();
	}
}

/**
 * Initializes this client's Player data stores after seamless map travel
 */
reliable client function ClientInitializeDataStores()
{
	`log(">> PlayerController::ClientInitializeDataStores for player" @ Self,,'DevDataStore');

	// register the player's data stores and bind the PRI to the PlayerOwner data store.
	RegisterPlayerDataStores();

	`log("<< PlayerController::ClientInitializeDataStores for player" @ Self,,'DevDataStore');
}

/**
 * Creates and initializes the "PlayerOwner" and "PlayerSettings" data stores.  This function assumes that the PlayerReplicationInfo
 * for this player has not yet been created, and that the PlayerOwner data store's PlayerDataProvider will be set when the PRI is registered.
 */
simulated function RegisterPlayerDataStores()
{
	local LocalPlayer LP;
	local DataStoreClient DataStoreManager;
	local class<UIDataStore_PlayerSettings> PlayerSettingsDataStoreClass;
	local class<UIDataStore_OnlinePlayerData> PlayerDataStoreClass;

`if(`notdefined(FINAL_RELEASE))
	local string PlayerName;

	PlayerName = PlayerReplicationInfo != None ? PlayerReplicationInfo.PlayerName : "None";
`endif

	// only create player data store for local players
	LP = LocalPlayer(Player);
	if ( LP != None )
	{
		`log(">> PlayerController::RegisterPlayerDataStores -" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');

		// get a reference to the main data store client
		DataStoreManager = class'UIInteraction'.static.GetDataStoreClient();
		if ( DataStoreManager != None )
		{
			// find the "PlayerOwner" data store registered for this player; there shouldn't be one...
			CurrentPlayerData = PlayerOwnerDataStore(DataStoreManager.FindDataStore('PlayerOwner',LP));
			if ( CurrentPlayerData == None )
			{
				// create the PlayerOwner data store
				CurrentPlayerData = DataStoreManager.CreateDataStore(PlayerOwnerDataStoreClass);
				if ( CurrentPlayerData != None )
				{
					// and register it
					if ( DataStoreManager.RegisterDataStore(CurrentPlayerData, LP) )
					{
						if ( PlayerReplicationInfo != None )
						{
							// if our PRI was created and initialized before we were assigned a Player, then our PlayerDataProvider wasn't
							// linked to the PlayerOwner data store since we didn't have a valid Player, so do that now.
							PlayerReplicationInfo.BindPlayerOwnerDataProvider();
						}
					}
					else
					{
						`log("Failed to register 'PlayerOwner' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ `showobj(CurrentPlayerData),,'DevDataStore');
					}
				}
				else
				{
					`log("Failed to create 'PlayerOwner' data store for player:"@ Self @ "(" $ PlayerName $ ") using class" @ PlayerOwnerDataStoreClass,,'DevDataStore');
				}
			}
			else
			{
				`log("'PlayerOwner' data store already registered for player:"@ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
			}

			// now create the PlayerSettings data store
			CurrentPlayerSettings = UIDataStore_PlayerSettings(DataStoreManager.FindDataStore('PlayerSettings',LP));
			if ( CurrentPlayerSettings == None )
			{
				PlayerSettingsDataStoreClass = class<UIDataStore_PlayerSettings>(DataStoreManager.FindDataStoreClass(class'UIDataStore_PlayerSettings'));
				if ( PlayerSettingsDataStoreClass != None )
				{
					// find the appropriate class to use for the PlayerSettings data store
					// create the PlayerSettings data store
					CurrentPlayerSettings = DataStoreManager.CreateDataStore(PlayerSettingsDataStoreClass);
					if ( CurrentPlayerSettings != None )
					{
						if ( !DataStoreManager.RegisterDataStore(CurrentPlayerSettings, LP) )
						{
							`log("Failed to register 'PlayerSettings' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ `showobj(CurrentPlayerSettings),,'DevDataStore');
						}
					}
					else
					{
						`log("Failed to create 'PlayerSettings' data store for player:"@ Self @ "(" $ PlayerName $ ") using class" @ PlayerSettingsDataStoreClass,,'DevDataStore');
					}
				}
				else
				{
					`log("Failed to find valid data store class while attempting to register the 'PlayerSettings' data store for player:"@ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
				}
			}
			else
			{
				`log("'PlayerSettings' data store already registered for player:"@`showobj(Self),,'DevDataStore');
			}

//@todo ronp -- Automate the creation of these so that they don't need to be hand initialized
			// now create the OnlinePlayerData data store
			OnlinePlayerData = UIDataStore_OnlinePlayerData(DataStoreManager.FindDataStore('OnlinePlayerData',LP));
			if ( OnlinePlayerData == None )
			{
				PlayerDataStoreClass = class<UIDataStore_OnlinePlayerData>(DataStoreManager.FindDataStoreClass(class'UIDataStore_OnlinePlayerData'));
				if ( PlayerDataStoreClass != None )
				{
					// find the appropriate class to use for the PlayerSettings data store
					// create the PlayerSettings data store
					OnlinePlayerData = DataStoreManager.CreateDataStore(PlayerDataStoreClass);
					if ( OnlinePlayerData != None )
					{
						if ( !DataStoreManager.RegisterDataStore(OnlinePlayerData, LP) )
						{
							`log("Failed to register 'OnlinePlayerData' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ `showobj(OnlinePlayerData),,'DevDataStore');
						}
					}
					else
					{
						`log("Failed to create 'OnlinePlayerData' data store for player:"@ Self @ "(" $ PlayerName $ ") using class" @ PlayerDataStoreClass,,'DevDataStore');
					}
				}
				else
				{
					`log("Failed to find valid data store class while attempting to register the 'OnlinePlayerData' data store for player:"@ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
				}
			}
			else
			{
				`log("'OnlinePlayerData' data store already registered for player:"@ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
			}
		}

		`log("<< PlayerController::RegisterPlayerDataStores -" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
	}
}

/**
 * Unregisters the "PlayerOwner" data store for this player.  Called when this PlayerController is destroyed.
 */
simulated function UnregisterPlayerDataStores()
{
	local LocalPlayer LP;
	local DataStoreClient DataStoreManager;
	local UIDataStore_OnlinePlayerData PlayerDataStore;

`if(`notdefined(FINAL_RELEASE))
	local string PlayerName;

	PlayerName = PlayerReplicationInfo != None ? PlayerReplicationInfo.PlayerName : "None";
`endif

	// only execute for local players
	LP = LocalPlayer(Player);
	if ( LP != None )
	{
		`log(">> PlayerController::UnregisterPlayerDataStores -" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');

		// because the PlayerOwner data store is created and registered each match, all we should need to do is
		// unregister it from the data store client and clear our reference
		// get a reference to the main data store client
		DataStoreManager = class'UIInteraction'.static.GetDataStoreClient();
		if ( DataStoreManager != None )
		{
			// unregister the current player data store
			if ( CurrentPlayerData != None )
			{
				if ( !DataStoreManager.UnregisterDataStore(CurrentPlayerData) )
				{
					`log("Failed to unregister 'PlayerOwner' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ `showobj(CurrentPlayerData),,'DevDataStore');
				}

				// clear the reference
				CurrentPlayerData = None;
			}
			else
			{
				`log("'PlayerOwner' data store not registered for player:" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
			}

			// unregister the player settings data store
			if ( CurrentPlayerSettings != None )
			{
				if ( !DataStoreManager.UnregisterDataStore(CurrentPlayerSettings) )
				{
					`log("Failed to unregister 'PlayerSettings' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ CurrentPlayerSettings,,'DevDataStore');
				}

				// clear the reference
				CurrentPlayerSettings = None;
			}
			else
			{
				`log("'PlayerSettings' data store not registered for player:" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
			}

			// Don't hold onto a ref
			OnlinePlayerData = None;
			// Unregister the online player data
			PlayerDataStore = UIDataStore_OnlinePlayerData(DataStoreManager.FindDataStore('OnlinePlayerData',LP));
			if ( PlayerDataStore != None )
			{
				if ( !DataStoreManager.UnregisterDataStore(PlayerDataStore) )
				{
					`log("Failed to unregister 'OnlinePlayerData' data store for player:"@ Self @ "(" $ PlayerName $ ")" @ `showobj(PlayerDataStore),,'DevDataStore');
				}
			}
			else
			{
				`log("'OnlinePlayerData' data store not registered for player:" @ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
			}
		}
		else
		{
			`log("Data store client not found!",,'DevDataStore');
		}

		`log("<< PlayerController::UnregisterPlayerDataStores" @ "(" $ PlayerName $ ")",,'DevDataStore');
	}
}

/**
 * Hooks up the PlayerDataProvider for this player to the PlayerOwner data store.
 *
 * @param	DataProvider	the PlayerDataProvider to associate with this player.
 */
simulated function SetPlayerDataProvider( PlayerDataProvider DataProvider )
{
`if(`notdefined(FINAL_RELEASE))
	local string PlayerName;

	PlayerName = PlayerReplicationInfo != None ? PlayerReplicationInfo.PlayerName : "None";
`endif

	`log(">>" @ Self $ "::SetPlayerDataProvider" @ "(" $ PlayerName $ "):" @ DataProvider,,'DevDataStore');

	if ( CurrentPlayerData == None )
	{
		RegisterPlayerDataStores();
	}

	if ( CurrentPlayerData != None )
	{
		if ( DataProvider != None )
		{
			CurrentPlayerData.SetPlayerDataProvider(DataProvider);
		}
		else
		{
			`log("NULL data provider specified!",,'DevDataStore');
		}
	}
	else
	{
		`log("'PlayerOwner' data store not yet registered for player:"@ Self @ "(" $ PlayerName $ ")",,'DevDataStore');
	}

	`log("<<" @ Self $ "::SetPlayerDataProvider" @ "(" $ PlayerName $ "):" @ DataProvider,,'DevDataStore');
}

/**
 * Scales the amount the rumble will play on the gamepad
 *
 * @param ScaleBy The amount to scale the waveforms by
 */
final function SetRumbleScale(float ScaleBy)
{
	if (ForceFeedbackManager != None)
	{
		ForceFeedbackManager.ScaleAllWaveformsBy = ScaleBy;
	}
}

/**
 * Returns the rumble scale (or 1 if none is specified)
 */
final function float GetRumbleScale()
{
	local float retval;
	retval = 1.0;
	if (ForceFeedbackManager != None)
	{
		retval = ForceFeedbackManager.ScaleAllWaveformsBy;
	}
	return retval;
}

/**
 * @return whether or not this Controller has Tilt Turned on
 **/
native function bool IsControllerTiltActive() const;

/**
 * sets whether or not the Tilt functionality is turned on
 **/
native function SetControllerTiltActive( bool bActive );

/**
 * sets whether or not to ONLY use the tilt input controls
 **/
native function SetOnlyUseControllerTiltInput( bool bActive );

/**
 * sets whether or not to use the tilt forward and back input controls
 **/
native function SetUseTiltForwardAndBack( bool bActive );


/* ClientGotoState()
server uses this to force client into NewState
*/
reliable client function ClientGotoState(name NewState, optional name NewLabel)
{
	if ((NewLabel == 'Begin' || NewLabel == '') && !IsInState(NewState))
	{
		GotoState(NewState);
	}
	else
	{
		GotoState(NewState,NewLabel);
	}
}

reliable server function AskForPawn()
{
	if ( GamePlayEndedState() )
		ClientGotoState(GetStateName(), 'Begin');
	else if ( Pawn != None )
		GivePawn(Pawn);
	else
	{
		bFrozen = false;
		ServerRestartPlayer();
	}
}

reliable client function GivePawn(Pawn NewPawn)
{
	if ( NewPawn == None )
		return;
	Pawn = NewPawn;
	NewPawn.Controller = self;
	ClientRestart(Pawn);
}

// Possess a pawn
event Possess(Pawn aPawn, bool bVehicleTransition)
{
	local Actor A;
	local int i;
	local SeqEvent_Touch TouchEvent;

	if (!PlayerReplicationInfo.bOnlySpectator)
	{
		if (aPawn.Controller != None)
		{
			aPawn.Controller.UnPossess();
		}

		aPawn.PossessedBy(self, bVehicleTransition);
		Pawn = aPawn;
		Pawn.bStasis = false;
		ResetTimeMargin();
		if (Vehicle(Pawn) != None && Vehicle(Pawn).Driver != None)
		{
			PlayerReplicationInfo.bIsFemale = Vehicle(Pawn).Driver.bIsFemale;
		}
		else
		{
			PlayerReplicationInfo.bIsFemale = Pawn.bIsFemale;
		}
		Restart(bVehicleTransition);

		// check if touching any actors with playeronly kismet touch events
		ForEach Pawn.TouchingActors(class'Actor', A)
		{
			for ( i=0; i < A.GeneratedEvents.length; i++)
			{
				TouchEvent = SeqEvent_Touch(A.GeneratedEvents[i]);
				if ( (TouchEvent != None) && TouchEvent.bPlayerOnly )
				{
					TouchEvent.CheckTouchActivate(A,Pawn);
				}
			}
		}
	}
}

function AcknowledgePossession(Pawn P)
{
	if ( LocalPlayer(Player) != None )
	{
		AcknowledgedPawn = P;
		if ( P != None )
		{
			P.SetBaseEyeHeight();
			P.Eyeheight = P.BaseEyeHeight;
		}
		ServerAcknowledgePossession(P);
	}
}

reliable server function ServerAcknowledgePossession(Pawn P)
{
	ResetTimeMargin();
    AcknowledgedPawn = P;
}

// unpossessed a pawn (not because pawn was killed)
event UnPossess()
{
	if ( Pawn != None )
	{
		SetLocation(Pawn.Location);
		Pawn.RemoteRole = ROLE_SimulatedProxy;
		Pawn.UnPossessed();
		CleanOutSavedMoves();  // don't replay moves previous to unpossession
		if ( GetViewTarget() == Pawn )
			SetViewTarget(self);
	}
	Pawn = None;
}

// unpossessed a pawn (because pawn was killed)
function PawnDied(Pawn P)
{
	if ( P != Pawn )
		return;

	if ( Pawn != None )
		Pawn.RemoteRole = ROLE_SimulatedProxy;

    super.PawnDied( P );
}

reliable client function ClientSetHUD(class<HUD> newHUDType, class<Scoreboard> newScoringType)
{
	if ( myHUD != None )
	{
		myHUD.Destroy();
	}

	if (newHUDType == None)
	{
		myHUD = None;
	}
	else
	{
		myHUD = Spawn(newHUDType, self);
		if ( myHUD != None )
		{
			MyHUD.SpawnScoreBoard(newScoringType);
		}
	}
}

function HandlePickup(Inventory Inv)
{
	ReceiveLocalizedMessage(Inv.MessageClass,,,,Inv.class);
}

/* epic ===============================================
* ::CleanupPRI
*
* Called from Destroyed().  Cleans up PlayerReplicationInfo.
* PlayerControllers add their PRI to the gameinfo's InactivePRI list, so disconnected players can rejoin without losing their score.
*
* =====================================================
*/
function CleanupPRI()
{
	WorldInfo.Game.AddInactivePRI(PlayerReplicationInfo, self);
	PlayerReplicationInfo = None;
}

reliable client event ReceiveLocalizedMessage( class<LocalMessage> Message, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	// Wait for player to be up to date with replication when joining a server, before stacking up messages
	if ( WorldInfo.NetMode == NM_DedicatedServer || WorldInfo.GRI == None )
		return;

	Message.Static.ClientReceive( Self, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject );
}

//Play a sound client side (so only client will hear it)
unreliable client event ClientPlaySound(SoundCue ASound)
{
	ClientHearSound(ASound, self, Location, false, false);
}

/** hooked up to the OnAudioFinished delegate of AudioComponents created through PlaySound() to return them to the pool */
simulated function HearSoundFinished(AudioComponent AC)
{
	HearSoundActiveComponents.RemoveItem(AC);
	// if the component is already pending kill (playing actor was destroyed), then we can't put it back in the pool
	if (!AC.IsPendingKill())
	{
		AC.ResetToDefaults();
		HearSoundPoolComponents[HearSoundPoolComponents.length] = AC;
	}
}

/** get an audio component from the HearSound pool
 * creates a new component if the pool is empty and MaxConcurrentHearSounds has not been exceeded
 * the component is initialized with the values passed in, ready to call Play() on
 * its OnAudioFinished delegate is set to this PC's HearSoundFinished() function
 * @param ASound - the sound to play
 * @param SourceActor - the Actor to attach the sound to (if None, attached to self)
 * @param bStopWhenOwnerDestroyed - whether the sound stops if SourceActor is destroyed
 * @param bUseLocation (optional) - whether to use the SourceLocation parameter for the sound's location (otherwise, SourceActor's location)
 * @param SourceLocation (optional) - if bUseLocation, the location for the sound
 * @return the AudioComponent that was found/created
 */
native function AudioComponent GetPooledAudioComponent(SoundCue ASound, Actor SourceActor, bool bStopWhenOwnerDestroyed, optional bool bUseLocation, optional vector SourceLocation);

/* ClientHearSound()
Replicated function from server for replicating audible sounds played on server
*/
unreliable client event ClientHearSound(SoundCue ASound, Actor SourceActor, vector SourceLocation, bool bStopWhenOwnerDestroyed, optional bool bIsOccluded )
{
	local AudioComponent AC;

//    `log("### ClientHearSound:"@ASound@SourceActor@SourceLocation@bStopWhenOwnerDestroyed@VSize(SourceLocation - Pawn.Location));

	if ( SourceActor == None )
	{
		AC = GetPooledAudioComponent(ASound, SourceActor, bStopWhenOwnerDestroyed, true, SourceLocation);
		if (AC == None)
		{
			return;
		}
		AC.bUseOwnerLocation = false;
		AC.Location = SourceLocation;
	}
	else if ( (SourceActor == GetViewTarget()) || (SourceActor == self) )
	{
		AC = GetPooledAudioComponent(ASound, None, bStopWhenOwnerDestroyed);
		if (AC == None)
		{
			return;
		}
		AC.bAllowSpatialization = false;
	}
	else
	{
		AC = GetPooledAudioComponent(ASound, SourceActor, bStopWhenOwnerDestroyed);
		if (AC == None)
		{
			return;
		}
		if (!IsZero(SourceLocation) && SourceLocation != SourceActor.Location)
		{
			AC.bUseOwnerLocation = false;
			AC.Location = SourceLocation;
		}
	}
	if ( bIsOccluded )
	{
		// if occluded reduce volume: @FIXME do something better
		AC.VolumeMultiplier *= 0.5;
	}
	AC.Play();
}

reliable client event Kismet_ClientPlaySound(SoundCue ASound, Actor SourceActor, float VolumeMultiplier, float PitchMultiplier, float FadeInTime, bool bSuppressSubtitles, bool bSuppressSpatialization)
{
	local AudioComponent AC;

	if (SourceActor != None)
	{
		// If we have a FaceFX animation hooked up, play that instead
		if( ASound.FaceFXAnimName != "" &&
			SourceActor.PlayActorFaceFXAnim(ASound.FaceFXAnimSetRef, ASound.FaceFXGroupName, ASound.FaceFXAnimName) )
		{
			// Success - In case of failure, fall back to playing the sound with no Face FX animation, but there will be a warning in the log instead.
		}
		else
		{
			AC = SourceActor.CreateAudioComponent(ASound, false, true);
			if ( AC != None )
			{
				AC.VolumeMultiplier = VolumeMultiplier;
				AC.PitchMultiplier = PitchMultiplier;
				AC.bAutoDestroy = true;
				AC.SubtitlePriority = 10000;
				AC.bSuppressSubtitles = bSuppressSubtitles;
				AC.FadeIn(FadeInTime, 1.f);
				if( bSuppressSpatialization )
				{
					AC.bAllowSpatialization = false;
				}
			}
		}
	}
}

reliable client event Kismet_ClientStopSound(SoundCue ASound, Actor SourceActor, float FadeOutTime)
{
	local AudioComponent AC, CheckAC;

	if (SourceActor == None)
	{
		SourceActor = WorldInfo;
	}
	foreach SourceActor.AllOwnedComponents(class'AudioComponent',CheckAC)
	{
		if (CheckAC.SoundCue == ASound)
		{
			AC = CheckAC;
			break;
		}
	}
	if (AC != None)
	{
		AC.FadeOut(FadeOutTime,0.f);
	}
}

/** plays a FaceFX anim on the specified actor for the client */
reliable client function ClientPlayActorFaceFXAnim(Actor SourceActor, FaceFXAnimSet AnimSet, string GroupName, string SeqName)
{
	if (SourceActor != None)
	{
		SourceActor.PlayActorFaceFXAnim(AnimSet, GroupName, SeqName);
	}
}

reliable client event ClientMessage( coerce string S, optional Name Type, optional float MsgLifeTime )
{
	if ( WorldInfo.NetMode == NM_DedicatedServer || WorldInfo.GRI == None )
		return;

	if (Type == '')
		Type = 'Event';

	TeamMessage(PlayerReplicationInfo, S, Type, MsgLifeTime);
}

reliable client event TeamMessage( PlayerReplicationInfo PRI, coerce string S, name Type, optional float MsgLifeTime  )
{
	local bool bIsUserCreated;

	if ( myHUD != None )
	{
		myHUD.Message( PRI, S, Type, MsgLifeTime );
	}

	if ( ((Type == 'Say') || (Type == 'TeamSay')) && (PRI != None) )
	{
		S = PRI.PlayerName$": "$S;
		// This came from a user so flag as user created
		bIsUserCreated = true;
	}

	// since this is on the client, we can assume that if Player exists, it is a LocalPlayer
	if (Player != None)
	{
		if (!bIsUserCreated ||
			// Don't allow this if the parental controls block it
			(bIsUserCreated && CanViewUserCreatedContent()))
		{
			LocalPlayer(Player).ViewportClient.ViewportConsole.OutputText(S);
		}
	}
}

function PlayBeepSound();

event Destroyed()
{
	local Vehicle	DrivenVehicle;
	local Pawn		Driver;

	// Disable any currently playing rumble
	ClientPlayForceFeedbackWaveform(none);

	// If there is an online subsystem, clear our callback for UI/controller changes
	if (OnlineSub != None && OnlineSub.SystemInterface != None)
	{
		OnlineSub.SystemInterface.ClearExternalUIChangeDelegate(OnExternalUIChanged);
		OnlineSub.SystemInterface.ClearControllerChangeDelegate(OnControllerChanged);

		// Cleanup game invite delegate
		if (OnlineSub.GameInterface != None && LocalPlayer(Player) != None)
		{
			OnlineSub.GameInterface.ClearGameInviteAcceptedDelegate(LocalPlayer(Player).ControllerId,OnGameInviteAccepted);
		}
	}

	// cheatmanager and playerinput cleaned up in C++ PostScriptDestroyed()

	if ( Pawn != None )
	{
		// If its a vehicle, just destroy the driver, otherwise do the normal.
		DrivenVehicle = Vehicle(Pawn);
		if ( DrivenVehicle != None )
		{
			Driver = DrivenVehicle.Driver;
			DrivenVehicle.DriverLeave( true ); // Force the driver out of the car
			if ( Driver != None )
			{
				Driver.Health = 0;
				Driver.Died( self, class'DmgType_Suicided', Driver.Location );
			}
		}
		else
		{
			Pawn.Health = 0;
			Pawn.Died( self, class'DmgType_Suicided', Pawn.Location );
		}
	}
	if ( myHUD != None )
	{
		myHud.Destroy();
	}

	if( PlayerCamera != None )
	{
		PlayerCamera.Destroy();
		PlayerCamera = None;
	}

	// remove this player's data store from the registered data stores..
	UnregisterPlayerDataStores();

	Super.Destroyed();
}


function FixFOV()
{
	FOVAngle = Default.DefaultFOV;
	DesiredFOV = Default.DefaultFOV;
	DefaultFOV = Default.DefaultFOV;
}

function SetFOV(float NewFOV)
{
	DesiredFOV = NewFOV;
	FOVAngle = NewFOV;
}

function ResetFOV()
{
	DesiredFOV = DefaultFOV;
	FOVAngle = DefaultFOV;
}

exec function FOV(float F)
{
	if( PlayerCamera != None )
	{
		PlayerCamera.SetFOV( F );
		return;
	}

	if( (F >= 80.0) || (WorldInfo.NetMode==NM_Standalone) || PlayerReplicationInfo.bOnlySpectator )
	{
		DefaultFOV = FClamp(F, 1, 170);
		DesiredFOV = DefaultFOV;
	}
}

exec function Mutate(string MutateString)
{
	ServerMutate(MutateString);
}

reliable server function ServerMutate(string MutateString)
{
	if( WorldInfo.NetMode == NM_Client )
		return;
	WorldInfo.Game.Mutate(MutateString, Self);
}

// ------------------------------------------------------------------------
// Messaging functions

function bool AllowTextMessage(string Msg)
{
	local int i;

	if ( (WorldInfo.NetMode == NM_Standalone) || PlayerReplicationInfo.bAdmin )
		return true;
	if ( ( WorldInfo.Pauser == none) && (WorldInfo.TimeSeconds - LastBroadcastTime < 2 ) )
		return false;

	// lower frequency if same text
	if ( WorldInfo.TimeSeconds - LastBroadcastTime < 5 )
	{
		Msg = Left(Msg,Clamp(len(Msg) - 4, 8, 64));
		for ( i=0; i<4; i++ )
			if ( LastBroadcastString[i] ~= Msg )
				return false;
	}
	for ( i=3; i>0; i-- )
		LastBroadcastString[i] = LastBroadcastString[i-1];

	LastBroadcastTime = WorldInfo.TimeSeconds;
	return true;
}

// Send a message to all players.
exec function Say( string Msg )
{
	Msg = Left(Msg,128);

	if ( AllowTextMessage(Msg) )
		ServerSay(Msg);
}

unreliable server function ServerSay( string Msg )
{
	local PlayerController PC;

	// center print admin messages which start with #
	if (PlayerReplicationInfo.bAdmin && left(Msg,1) == "#" )
	{
		Msg = right(Msg,len(Msg)-1);
		foreach WorldInfo.AllControllers(class'PlayerController', PC)
		{
			PC.ClearProgressMessages();
			PC.SetProgressTime(6);
			PC.SetProgressMessage(0, Msg, MakeColor(255,255,255,255));
		}
		return;
	}

	WorldInfo.Game.Broadcast(self, Msg, 'Say');
}

exec function TeamSay( string Msg )
{
	Msg = Left(Msg,128);
	if ( AllowTextMessage(Msg) )
		ServerTeamSay(Msg);
}

unreliable server function ServerTeamSay( string Msg )
{
	LastActiveTime = WorldInfo.TimeSeconds;

	if( !WorldInfo.GRI.GameClass.Default.bTeamGame )
	{
		Say( Msg );
		return;
	}

    WorldInfo.Game.BroadcastTeam( self, WorldInfo.Game.ParseMessageString( self, Msg ) , 'TeamSay');
}

// ------------------------------------------------------------------------

event PreClientTravel();

/**
 * Change Camera mode
 *
 * @param	New camera mode to set
 */
exec function Camera( name NewMode )
{
	ServerCamera(NewMode);
}

reliable server function ServerCamera( name NewMode )
{
	if ( NewMode == '1st' )
	{
    	NewMode = 'FirstPerson';
	}
    else if ( NewMode == '3rd' )
	{
    	NewMode = 'ThirdPerson';
	}

	SetCameraMode( NewMode );
	if ( PlayerCamera != None )
		`log("#### " $ PlayerCamera.CameraStyle);
}

/**
 * Replicated function to set camera style on client
 *
 * @param	NewCamMode, name defining the new camera mode
 */
reliable client function ClientSetCameraMode( name NewCamMode )
{
	if ( PlayerCamera != None )
		PlayerCamera.CameraStyle = NewCamMode;
}

/**
 * Set new camera mode
 *
 * @param	NewCamMode, new camera mode.
 */
function SetCameraMode( name NewCamMode )
{
	if ( PlayerCamera != None )
	{
		PlayerCamera.CameraStyle = NewCamMode;
		if ( WorldInfo.NetMode == NM_DedicatedServer )
		{
			ClientSetCameraMode( NewCamMode );
		}
	}
}

/**
 * Reset Camera Mode to default
 */
event ResetCameraMode()
{
	if ( Pawn != None )
	{	// If we have a pawn, let's ask it which camera mode we should use
		SetCameraMode( Pawn.GetDefaultCameraMode( Self ) );
	}
	else
	{	// otherwise default to first person view.
		SetCameraMode( 'FirstPerson' );
	}
}

/**
* return whether viewing in first person mode
*/
function bool UsingFirstPersonCamera()
{
	return ((PlayerCamera == None) || (PlayerCamera.CameraStyle == 'FirstPerson')) && LocalPlayer(Player) != None;
}

function ClientVoiceMessage(PlayerReplicationInfo Sender, PlayerReplicationInfo Recipient, name messagetype, byte messageID);

/* ForceDeathUpdate()
Make sure ClientAdjustPosition immediately informs client of pawn's death
*/
function ForceDeathUpdate()
{
	LastUpdateTime = WorldInfo.TimeSeconds - 10;
}

/* DualServerMove()
- replicated function sent by client to server - contains client movement and firing info for two moves
*/
unreliable server function DualServerMove
(
	float TimeStamp0,
	vector InAccel0,
	byte PendingFlags,
	int View0,
    float TimeStamp,
    vector InAccel,
    vector ClientLoc,
    byte NewFlags,
    byte ClientRoll,
    int View
)
{
    ServerMove(TimeStamp0,InAccel0,vect(1,2,3),PendingFlags,ClientRoll,View0);
    ServerMove(TimeStamp,InAccel,ClientLoc,NewFlags,ClientRoll,View);
}

/* OldServerMove()
- resending an (important) old move.  Process it if not already processed.
//@todo steve - perhaps don't compress so much?
*/
unreliable server function OldServerMove
(
	float OldTimeStamp,
    byte OldAccelX,
    byte OldAccelY,
    byte OldAccelZ,
    byte OldMoveFlags
)
{
	local vector Accel;

	if ( AcknowledgedPawn != Pawn )
		return;

	if ( CurrentTimeStamp < OldTimeStamp - 0.001 )
	{
		// split out components of lost move (approx)
		Accel.X = OldAccelX;
		if ( Accel.X > 127 )
			Accel.X = -1 * (Accel.X - 128);
		Accel.Y = OldAccelY;
		if ( Accel.Y > 127 )
			Accel.Y = -1 * (Accel.Y - 128);
		Accel.Z = OldAccelZ;
		if ( Accel.Z > 127 )
			Accel.Z = -1 * (Accel.Z - 128);
		Accel *= 20;

		//`log("Recovered move from "$OldTimeStamp$" acceleration "$Accel$" from "$OldAccel);
		OldTimeStamp = FMin(OldTimeStamp, CurrentTimeStamp + MaxResponseTime);
		MoveAutonomous(OldTimeStamp - CurrentTimeStamp, OldMoveFlags, Accel, rot(0,0,0));
		CurrentTimeStamp = OldTimeStamp;
	}
}

/* ServerMove()
- replicated function sent by client to server - contains client movement and firing info.
*/
unreliable server function ServerMove
(
	float	TimeStamp,
	vector	InAccel,
	vector	ClientLoc,
	byte	MoveFlags,
	byte	ClientRoll,
	int		View
)
{
	local float		DeltaTime, clientErr;
	local rotator	DeltaRot, Rot, ViewRot;
	local vector Accel, LocDiff;
	local int		maxPitch, ViewPitch, ViewYaw;

	// If this move is outdated, discard it.
	if( CurrentTimeStamp >= TimeStamp )
	{
		return;
	}

	if( AcknowledgedPawn != Pawn )
	{
		InAccel = vect(0,0,0);
		GivePawn(Pawn);
	}

	// View components
	ViewPitch	= (View & 65535);
	ViewYaw		= (View >> 16);

	// Acceleration was scaled by 10x for replication, to keep more precision since vectors are rounded for replication
	Accel = InAccel * 0.1;
	// Save move parameters.
	DeltaTime = FMin(MaxResponseTime,TimeStamp - CurrentTimeStamp);

	if( Pawn == None )
	{
		bWasSpeedHack = FALSE;
		ResetTimeMargin();
	}
	else
	{
		DeltaTime *= Pawn.CustomTimeDilation;
		bWasSpeedHack = FALSE;
	}
/* //@todo steve - reenable - test why GC hitches was triggering this
	else if( !CheckSpeedHack(DeltaTime) )
	{
		if( !bWasSpeedHack )
		{
			if( WorldInfo.TimeSeconds - LastSpeedHackLog > 20 )
			{
				`log("Possible speed hack by "$PlayerReplicationInfo.PlayerName);
				LastSpeedHackLog = WorldInfo.TimeSeconds;
			}
			ClientMessage( "Speed Hack Detected!",'CriticalEvent' );
		}
		else
		{
			bWasSpeedHack = TRUE;
		}
		DeltaTime = 0;
		Pawn.Velocity = vect(0,0,0);
	}
*/

	CurrentTimeStamp = TimeStamp;
	ServerTimeStamp = WorldInfo.TimeSeconds;
	ViewRot.Pitch = ViewPitch;
	ViewRot.Yaw = ViewYaw;
	ViewRot.Roll = 0;

	if( InAccel != vect(0,0,0) )
	{
		LastActiveTime = WorldInfo.TimeSeconds;
	}

	SetRotation(ViewRot);

	if( AcknowledgedPawn != Pawn )
	{
		return;
	}

	if( Pawn != None )
	{
		Rot.Roll	= 256 * ClientRoll;
		Rot.Yaw		= ViewYaw;
		if( (Pawn.Physics == PHYS_Swimming) || (Pawn.Physics == PHYS_Flying) )
		{
			maxPitch = 2;
		}
		else
		{
			maxPitch = 0;
		}

		if( (ViewPitch > maxPitch * Pawn.MaxPitchLimit) && (ViewPitch < 65536 - maxPitch * Pawn.MaxPitchLimit) )
		{
			if( ViewPitch < 32768 )
			{
				Rot.Pitch = maxPitch * Pawn.MaxPitchLimit;
			}
			else
			{
				Rot.Pitch = 65536 - maxPitch * Pawn.MaxPitchLimit;
			}
		}
		else
		{
			Rot.Pitch = ViewPitch;
		}
		DeltaRot = (Rotation - Rot);
		Pawn.FaceRotation(Rot, DeltaTime);
	}

	// Perform actual movement
	if( (WorldInfo.Pauser == None) && (DeltaTime > 0) )
	{
		MoveAutonomous(DeltaTime, MoveFlags, Accel, DeltaRot);
	}

	// Accumulate movement error.
	if( ClientLoc == vect(1,2,3) )
	{
		return;		// first part of double servermove
	}
	else if( WorldInfo.TimeSeconds - LastUpdateTime < CLIENTADJUSTUPDATECOST/Player.CurrentNetSpeed )
	{
		// limit frequency of corrections if connection speed is limited
		return;
	}

	if( Pawn == None )
	{
		LocDiff = Location - ClientLoc;
	}
	else if ( Pawn.bForceRMVelocity )
	{
		// don't do corrections during root motion
		LocDiff = vect(0,0,0);
	}
	else
	{
		LocDiff = Pawn.Location - ClientLoc;
	}
	ClientErr = LocDiff Dot LocDiff;

	// If client has accumulated a noticeable positional error, correct him.
	if( ClientErr > MAXPOSITIONERRORSQUARED )
	{
		if( Pawn == None )
		{
			PendingAdjustment.newPhysics = Physics;
			PendingAdjustment.NewLoc = Location;
			PendingAdjustment.NewVel = Velocity;
		}
		else
		{
			PendingAdjustment.newPhysics = Pawn.Physics;
			PendingAdjustment.NewVel = Pawn.Velocity;
			PendingAdjustment.NewBase = Pawn.Base;
			if( (InterpActor(Pawn.Base) != None) || (Vehicle(Pawn.Base) != None) )
			{
				PendingAdjustment.NewLoc = Pawn.Location - Pawn.Base.Location;
			}
			else
			{
				PendingAdjustment.NewLoc = Pawn.Location;
			}
			PendingAdjustment.NewFloor = Pawn.Floor;
		}

		//`log("Client Error at" @ TimeStamp @ "is" @ ClientErr @ "with acceleration" @ Accel @ "LocDiff" @ LocDiff @ "Physics" @ Pawn.Physics);

		LastUpdateTime = WorldInfo.TimeSeconds;
		PendingAdjustment.TimeStamp = TimeStamp;
		PendingAdjustment.bAckGoodMove = 0;
    }
	else
	{
		// acknowledge receipt of this successful servermove()
		PendingAdjustment.TimeStamp = TimeStamp;
		PendingAdjustment.bAckGoodMove = 1;
	}
	//`log("Server moved stamp "$TimeStamp$" location "$Pawn.Location$" Acceleration "$Pawn.Acceleration$" Velocity "$Pawn.Velocity);
}


/* Called on server at end of tick when PendingAdjustment has been set.
Done this way to avoid ever sending more than one ClientAdjustment per server tick.
*/
event SendClientAdjustment()
{
	if( AcknowledgedPawn != Pawn )
	{
		PendingAdjustment.TimeStamp = 0;
		return;
	}

	if( PendingAdjustment.bAckGoodMove == 1 )
	{
		// just notify client this move was received
		ClientAckGoodMove(PendingAdjustment.TimeStamp);
	}
	else if( (Pawn == None) || (Pawn.Physics != PHYS_Spider) )
	{
		if( PendingAdjustment.NewVel == vect(0,0,0) )
		{
			if (GetStateName() == 'PlayerWalking' && Pawn != None && Pawn.Physics == PHYS_Walking)
			{
				VeryShortClientAdjustPosition
				(
					PendingAdjustment.TimeStamp,
					PendingAdjustment.NewLoc.X,
					PendingAdjustment.NewLoc.Y,
					PendingAdjustment.NewLoc.Z,
					PendingAdjustment.NewBase
				);
			}
			else
			{
				ShortClientAdjustPosition
				(
					PendingAdjustment.TimeStamp,
					GetStateName(),
					PendingAdjustment.newPhysics,
					PendingAdjustment.NewLoc.X,
					PendingAdjustment.NewLoc.Y,
					PendingAdjustment.NewLoc.Z,
					PendingAdjustment.NewBase
				);
			}
		}
		else
		{
			ClientAdjustPosition
			(
				PendingAdjustment.TimeStamp,
				GetStateName(),
				PendingAdjustment.newPhysics,
				PendingAdjustment.NewLoc.X,
				PendingAdjustment.NewLoc.Y,
				PendingAdjustment.NewLoc.Z,
				PendingAdjustment.NewVel.X,
				PendingAdjustment.NewVel.Y,
				PendingAdjustment.NewVel.Z,
				PendingAdjustment.NewBase
			);
		}
    }
	else
	{
		LongClientAdjustPosition
		(
			PendingAdjustment.TimeStamp,
			GetStateName(),
			PendingAdjustment.newPhysics,
			PendingAdjustment.NewLoc.X,
			PendingAdjustment.NewLoc.Y,
			PendingAdjustment.NewLoc.Z,
			PendingAdjustment.NewVel.X,
			PendingAdjustment.NewVel.Y,
			PendingAdjustment.NewVel.Z,
			PendingAdjustment.NewBase,
			PendingAdjustment.NewFloor.X,
			PendingAdjustment.NewFloor.Y,
			PendingAdjustment.NewFloor.Z
		);
	}

	PendingAdjustment.TimeStamp = 0;
	PendingAdjustment.bAckGoodMove = 0;
}

// Only executed on server
unreliable server function ServerDrive(float InForward, float InStrafe, float aUp, bool InJump, int View)
{
	local rotator ViewRotation;

	ViewRotation.Pitch = (View & 65535);
	ViewRotation.Yaw = (View >> 16);
	ViewRotation.Roll = 0;
	SetRotation(ViewRotation);

	ProcessDrive(InForward, InStrafe, aUp, InJump);
}

function ProcessDrive(float InForward, float InStrafe, float InUp, bool InJump)
{
	ClientGotoState(GetStateName(), 'Begin');
}

function ProcessMove( float DeltaTime, vector newAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
{
    if( (Pawn != None) && (Pawn.Acceleration != newAccel) )
	{
		Pawn.Acceleration = newAccel;
	}
}

function MoveAutonomous
(
	float DeltaTime,
	byte CompressedFlags,
	vector newAccel,
	rotator DeltaRot
)
{
	local EDoubleClickDir DoubleClickMove;

	if ( (Pawn != None) && Pawn.bHardAttach )
		return;

	DoubleClickMove = SavedMoveClass.static.SetFlags(CompressedFlags, self);
	HandleWalking();
	ProcessMove(DeltaTime, newAccel, DoubleClickMove, DeltaRot);
	if ( Pawn != None )
		Pawn.AutonomousPhysics(DeltaTime);
	else
		AutonomousPhysics(DeltaTime);
    bDoubleJump = false;
	//`log("Role "$Role$" moveauto time "$100 * DeltaTime$" ("$WorldInfo.TimeDilation$")");
}

/* VeryShortClientAdjustPosition
bandwidth saving version, when velocity is zeroed, and pawn is walking
*/
unreliable client function VeryShortClientAdjustPosition
(
	float TimeStamp,
	float NewLocX,
	float NewLocY,
	float NewLocZ,
	Actor NewBase
)
{
	local vector Floor;

	if( Pawn != None )
	{
		Floor = Pawn.Floor;
	}
	LongClientAdjustPosition(TimeStamp, 'PlayerWalking', PHYS_Walking, NewLocX, NewLocY, NewLocZ, 0, 0, 0, NewBase, Floor.X, Floor.Y, Floor.Z);
}

/* ShortClientAdjustPosition
bandwidth saving version, when velocity is zeroed
*/
unreliable client function ShortClientAdjustPosition
(
	float TimeStamp,
	name newState,
	EPhysics newPhysics,
	float NewLocX,
	float NewLocY,
	float NewLocZ,
	Actor NewBase
)
{
	local vector Floor;

	if( Pawn != None )
	{
		Floor = Pawn.Floor;
	}

	LongClientAdjustPosition(TimeStamp, newState, newPhysics, NewLocX, NewLocY, NewLocZ, 0, 0, 0, NewBase, Floor.X, Floor.Y, Floor.Z);
}

reliable client function ClientCapBandwidth(int Cap)
{
	ClientCap = Cap;
	if( (Player != None) && (Player.CurrentNetSpeed > Cap) )
	{
		SetNetSpeed(Cap);
	}
}

unreliable client function ClientAckGoodMove(float TimeStamp)
{
	UpdatePing(TimeStamp);
	CurrentTimeStamp = TimeStamp;
	ClearAckedMoves();
}

/* ClientAdjustPosition
- pass newloc and newvel in components so they don't get rounded
*/
unreliable client function ClientAdjustPosition
(
	float TimeStamp,
	name newState,
	EPhysics newPhysics,
	float NewLocX,
	float NewLocY,
	float NewLocZ,
	float NewVelX,
	float NewVelY,
	float NewVelZ,
	Actor NewBase
)
{
	local vector Floor;

	if ( Pawn != None )
		Floor = Pawn.Floor;
	LongClientAdjustPosition(TimeStamp,newState,newPhysics,NewLocX,NewLocY,NewLocZ,NewVelX,NewVelY,NewVelZ,NewBase,Floor.X,Floor.Y,Floor.Z);
}

/** sets NetSpeed on the server, so it won't send the client more than this many bytes */
reliable server function ServerSetNetSpeed(int NewSpeed)
{
	SetNetSpeed(NewSpeed);
}

/* epic ===============================================
* ::UpdatePing
update average ping based on newly received round trip timestamp.
Occasionally send ping updates to the server, and also adjust netspeed if connection appears to be saturated
*/
final function UpdatePing(float TimeStamp)
{
	if ( PlayerReplicationInfo != None )
	{
		PlayerReplicationInfo.UpdatePing(TimeStamp);
		if ( WorldInfo.TimeSeconds - LastPingUpdate > 4 )
		{
			if ( bDynamicNetSpeed && (OldPing > DynamicPingThreshold * 0.001) && (PlayerReplicationInfo.ExactPing > DynamicPingThreshold * 0.001) )
			{
				if ( WorldInfo.MoveRepSize < 64 )
					WorldInfo.MoveRepSize += 8;
				else if ( Player.CurrentNetSpeed >= 6000 )
				{
					SetNetSpeed(Player.CurrentNetSpeed - 1000);
					ServerSetNetSpeed(Player.CurrentNetSpeed);
				}
				OldPing = 0;
			}
			else
				OldPing = PlayerReplicationInfo.ExactPing;
			LastPingUpdate = WorldInfo.TimeSeconds;
			ServerUpdatePing(1000 * PlayerReplicationInfo.ExactPing);
		}
	}
}

/* LongClientAdjustPosition
long version, when care about pawn's floor normal
*/
unreliable client function LongClientAdjustPosition
(
	float TimeStamp,
	name newState,
	EPhysics newPhysics,
	float NewLocX,
	float NewLocY,
	float NewLocZ,
	float NewVelX,
	float NewVelY,
	float NewVelZ,
	Actor NewBase,
	float NewFloorX,
	float NewFloorY,
	float NewFloorZ
)
{
    local vector NewLocation, NewVelocity, NewFloor;
	local Actor MoveActor;
    local SavedMove CurrentMove;
	local Actor TheViewTarget;

	`log(WorldInfo.TimeSeconds@GetFuncName(),,'PlayerMove');

	UpdatePing(TimeStamp);
	if( Pawn != None )
	{
		if( Pawn.bTearOff )
		{
			Pawn = None;
			if( !GamePlayEndedState() && !IsInState('Dead') )
			{
				GotoState('Dead');
			}
			return;
		}

		MoveActor = Pawn;
		TheViewTarget = GetViewTarget();

		if( (TheViewTarget != Pawn)
			&& ((TheViewTarget == self) || ((Pawn(TheViewTarget) != None) && (Pawn(TheViewTarget).Health <= 0))) )
		{
			ResetCameraMode();
			SetViewTarget(Pawn);
		}
	}
	else
    {
		MoveActor = self;
 	   	if( GetStateName() != newstate )
		{
			`log("- state change:"@GetStateName()@"->"@newstate,,'PlayerMove');
		    if( NewState == 'RoundEnded' )
			{
			    GotoState(NewState);
			}
			else if( IsInState('Dead') )
			{
		    	if( (NewState != 'PlayerWalking') && (NewState != 'PlayerSwimming') )
		        {
				    GotoState(NewState);
		        }
		        return;
			}
			else if( NewState == 'Dead' )
			{
				GotoState(NewState);
			}
		}
	}

    if( CurrentTimeStamp >= TimeStamp )
	{
		return;
	}
	CurrentTimeStamp = TimeStamp;

	NewLocation.X = NewLocX;
	NewLocation.Y = NewLocY;
	NewLocation.Z = NewLocZ;
    NewVelocity.X = NewVelX;
    NewVelocity.Y = NewVelY;
    NewVelocity.Z = NewVelZ;

	// skip update if no error
    CurrentMove = SavedMoves;

	// note that acked moves are cleared here, instead of calling ClearAckedMoves()
    while( CurrentMove != None )
    {
		if( CurrentMove.TimeStamp <= CurrentTimeStamp )
		{
			SavedMoves = CurrentMove.NextMove;
			CurrentMove.NextMove = FreeMoves;
			FreeMoves = CurrentMove;
			if( CurrentMove.TimeStamp == CurrentTimeStamp )
			{
				LastAckedAccel = CurrentMove.Acceleration;
				FreeMoves.Clear();
				if( ((InterpActor(NewBase) != None) || (Vehicle(NewBase) != None))
					&& (NewBase == CurrentMove.EndBase) )
				{
					if ( (GetStateName() == NewState)
						&& IsInState('PlayerWalking')
						&& ((MoveActor.Physics == PHYS_Walking) || (MoveActor.Physics == PHYS_Falling)) )
					{
						if ( VSizeSq(CurrentMove.SavedRelativeLocation - NewLocation) < MAXPOSITIONERRORSQUARED )
						{
							CurrentMove = None;
							return;
						}
						else if ( (Vehicle(NewBase) != None)
								&& (VSizeSq(Velocity) < MAXNEARZEROVELOCITYSQUARED) && (VSizeSq(NewVelocity) < MAXNEARZEROVELOCITYSQUARED)
								&& (VSizeSq(CurrentMove.SavedRelativeLocation - NewLocation) < MAXVEHICLEPOSITIONERRORSQUARED) )
						{
							CurrentMove = None;
							return;
						}
					}
				}
				else if ( (VSizeSq(CurrentMove.SavedLocation - NewLocation) < MAXPOSITIONERRORSQUARED)
					&& (VSizeSq(CurrentMove.SavedVelocity - NewVelocity) < MAXNEARZEROVELOCITYSQUARED)
					&& (GetStateName() == NewState)
					&& IsInState('PlayerWalking')
					&& ((MoveActor.Physics == PHYS_Walking) || (MoveActor.Physics == PHYS_Falling)) )
				{
					CurrentMove = None;
					return;
				}
				CurrentMove = None;
			}
			else
			{
				FreeMoves.Clear();
				CurrentMove = SavedMoves;
			}
		}
		else
		{
			CurrentMove = None;
		}
    }

	if( MoveActor.bHardAttach )
	{
		if( MoveActor.Base == None )
		{
			if( NewBase != None )
			{
				MoveActor.SetBase(NewBase);
			}
			if( MoveActor.Base == None )
			{
				MoveActor.SetHardAttach(false);
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	NewFloor.X = NewFloorX;
	NewFloor.Y = NewFloorY;
	NewFloor.Z = NewFloorZ;

	//@debug - track down the errors
	`log("- base mismatch:"@MoveActor.Base@NewBase,MoveActor.Base != NewBase,'PlayerMove');
	`log("- location mismatch, delta:"@VSize(MoveActor.Location - NewLocation),MoveActor.Location != NewLocation,'PlayerMove');
	`log("- velocity mismatch, delta:"@VSize(NewVelocity - MoveActor.Velocity)@"client:"@VSize(MoveActor.Velocity)@"server:"@VSize(NewVelocity),MoveActor.Velocity != NewVelocity,'PlayerMove');

	if (Pawn != None && Pawn.Physics != PHYS_Falling && Pawn.Mesh != None && Pawn.Mesh.RootMotionMode != RMM_Ignore)
	{
		`log("- skipping position update for root motion",,'PlayerMove');
		return;
	}
	// don't correct if have upcoming root motion
    CurrentMove = SavedMoves;
    while( CurrentMove != None )
    {
		if ( CurrentMove.bForceRMVelocity )
		{
			//`log("- skipping position update for upcoming root motion",,'PlayerMove');
			return;
		}
		CurrentMove = CurrentMove.NextMove;
	}
	if( (InterpActor(NewBase) != None) || (Vehicle(NewBase) != None) )
	{
		NewLocation += NewBase.Location;
	}

	//`log("Client "$Role$" adjust "$self$" stamp "$TimeStamp$" location "$MoveActor.Location);

	MoveActor.bCanTeleport = FALSE;
	if ( !MoveActor.SetLocation(NewLocation) && (Pawn(MoveActor) != None)
		&& (Pawn(MoveActor).CylinderComponent.CollisionHeight > Pawn(MoveActor).CrouchHeight)
		&& !Pawn(MoveActor).bIsCrouched
		&& (newPhysics == PHYS_Walking)
		&& (MoveActor.Physics != PHYS_RigidBody) )
	{
		MoveActor.SetPhysics(newPhysics);

		if( !MoveActor.SetLocation(NewLocation + vect(0,0,1)*Pawn(MoveActor).MaxStepHeight) )
		{
			Pawn(MoveActor).ForceCrouch();
			MoveActor.SetLocation(NewLocation);
		}
		else
		{
			MoveActor.MoveSmooth(vect(0,0,-1)*Pawn(MoveActor).MaxStepHeight);
		}
	}
	MoveActor.bCanTeleport = TRUE;

	// Hack. Don't let network change physics mode of rigid body stuff on the client.
	if( MoveActor.Physics != PHYS_RigidBody &&
		newPhysics != PHYS_RigidBody )
	{
		MoveActor.SetPhysics(newPhysics);
	}

	if( MoveActor != self )
	{
		MoveActor.SetBase(NewBase, NewFloor);
	}

    MoveActor.Velocity = NewVelocity;
	UpdateStateFromAdjustment(NewState);
	bUpdatePosition = TRUE;
}


/**
Called by LongClientAdjustPosition()
@param NewState is the state recommended by the server
*/
function UpdateStateFromAdjustment(name NewState)
{
	if( GetStateName() != newstate )
	{
		GotoState(newstate);
	}
}

unreliable server function ServerUpdatePing(int NewPing)
{
	PlayerReplicationInfo.Ping = Min(0.25 * NewPing, 250);
}

/* ClearAckedMoves()
clear out acknowledged/missed sent moves
*/
function ClearAckedMoves()
{
	local SavedMove CurrentMove;

	CurrentMove = SavedMoves;
	while ( CurrentMove != None )
	{
		if ( CurrentMove.TimeStamp <= CurrentTimeStamp )
		{
			if ( CurrentMove.TimeStamp == CurrentTimeStamp )
				LastAckedAccel = CurrentMove.Acceleration;
			SavedMoves = CurrentMove.NextMove;
			CurrentMove.NextMove = FreeMoves;
			FreeMoves = CurrentMove;
			FreeMoves.Clear();
			CurrentMove = SavedMoves;
		}
		else
			break;
	}
}

function ClientUpdatePosition()
{
	local SavedMove CurrentMove;
	local int		realbRun, realbDuck;
	local bool		bRealJump;
	local bool		bRealPreciseDestination;

	bUpdatePosition = FALSE;

	// Dont do any network position updates on things running PHYS_RigidBody
	if( Pawn != None && Pawn.Physics == PHYS_RigidBody )
	{
		return;
	}

	realbRun= bRun;
	realbDuck = bDuck;
	bRealJump = bPressedJump;
	bUpdating = TRUE;
	bRealPreciseDestination = bPreciseDestination;

	ClearAckedMoves();
	CurrentMove = SavedMoves;
	while( CurrentMove != None )
	{
		if( (PendingMove == CurrentMove) && (Pawn != None) )
		{
			PendingMove.SetInitialPosition(Pawn);
		}

		if ( Pawn != None )
			Pawn.bForceRMVelocity = CurrentMove.bForceRMVelocity;

		MoveAutonomous(CurrentMove.Delta, CurrentMove.CompressedFlags(), CurrentMove.Acceleration, rot(0,0,0));

		if( Pawn != None )
		{
			Pawn.bForceRMVelocity = false;
			CurrentMove.SavedLocation = Pawn.Location;
			CurrentMove.SavedVelocity = Pawn.Velocity;
			CurrentMove.EndBase = Pawn.Base;
			if( (CurrentMove.EndBase != None) && !CurrentMove.EndBase.bWorldGeometry )
			{
				CurrentMove.SavedRelativeLocation = Pawn.Location - CurrentMove.EndBase.Location;
			}
		}
		CurrentMove = CurrentMove.NextMove;
	}

	bUpdating = FALSE;
	bDuck = realbDuck;
	bRun = realbRun;
	bPressedJump = bRealJump;
	bPreciseDestination = bRealPreciseDestination;
}

final function SavedMove GetFreeMove()
{
	local SavedMove s, first;
	local int i;

	if ( FreeMoves == None )
	{
		// don't allow more than 100 saved moves
		For ( s=SavedMoves; s!=None; s=s.NextMove )
		{
			i++;
			if ( i > 100 )
			{
				first = SavedMoves;
				SavedMoves = SavedMoves.NextMove;
				first.Clear();
				first.NextMove = None;
				// clear out all the moves
				While ( SavedMoves != None )
				{
					s = SavedMoves;
					SavedMoves = SavedMoves.NextMove;
					s.Clear();
					s.NextMove = FreeMoves;
					FreeMoves = s;
				}
				PendingMove = None;
				return first;
			}
		}
		return new(self) SavedMoveClass;
	}
	else
	{
		s = FreeMoves;
		FreeMoves = FreeMoves.NextMove;
		s.NextMove = None;
		return s;
	}
}

function int CompressAccel(int C)
{
	if ( C >= 0 )
		C = Min(C, 127);
	else
		C = Min(abs(C), 127) + 128;
	return C;
}

/*
========================================================================
Here's how player movement prediction, replication and correction works in network games:

Every tick, the PlayerTick() function is called.  It calls the PlayerMove() function (which is implemented
in various states).  PlayerMove() figures out the acceleration and rotation, and then calls ProcessMove()
(for single player or listen servers), or ReplicateMove() (if its a network client).

ReplicateMove() saves the move (in the PendingMove list), calls ProcessMove(), and then replicates the move
to the server by calling the replicated function ServerMove() - passing the movement parameters, the client's
resultant position, and a timestamp.

ServerMove() is executed on the server.  It decodes the movement parameters and causes the appropriate movement
to occur.  It then looks at the resulting position and if enough time has passed since the last response, or the
position error is significant enough, the server calls ClientAdjustPosition(), a replicated function.

ClientAdjustPosition() is executed on the client.  The client sets its position to the servers version of position,
and sets the bUpdatePosition flag to true.

When PlayerTick() is called on the client again, if bUpdatePosition is true, the client will call
ClientUpdatePosition() before calling PlayerMove().  ClientUpdatePosition() replays all the moves in the pending
move list which occured after the timestamp of the move the server was adjusting.
*/

//
// Replicate this client's desired movement to the server.
//
function ReplicateMove
(
	float DeltaTime,
	vector NewAccel,
	eDoubleClickDir DoubleClickMove,
	rotator DeltaRot
)
{
	local SavedMove NewMove, OldMove, AlmostLastMove, LastMove;
	local byte ClientRoll;
	local float NetMoveDelta;

	MaxResponseTime = Default.MaxResponseTime * WorldInfo.TimeDilation;
	DeltaTime = ((Pawn != None) ? Pawn.CustomTimeDilation : CustomTimeDilation) * FMin(DeltaTime, MaxResponseTime);

	// find the most recent move (LastMove), and the oldest (unacknowledged) important move (OldMove)
	// a SavedMove is interesting if it differs significantly from the last acknowledged move
	if ( SavedMoves != None )
	{
		LastMove = SavedMoves;
		AlmostLastMove = LastMove;
		OldMove = None;
		while ( LastMove.NextMove != None )
		{
			// find first important unacknowledged move
			if ( (OldMove == None) && (Pawn != None) && LastMove.IsImportantMove(LastAckedAccel) )
			{
				OldMove = LastMove;
			}
			AlmostLastMove = LastMove;
			LastMove = LastMove.NextMove;
		}
	}

	// Get a SavedMove actor to store the movement in.
	NewMove = GetFreeMove();
	if ( NewMove == None )
	{
		return;
	}
	NewMove.SetMoveFor(self, DeltaTime, NewAccel, DoubleClickMove);

	// Simulate the movement locally.
	bDoubleJump = false;
	ProcessMove(NewMove.Delta, NewMove.Acceleration, NewMove.DoubleClickMove, DeltaRot);

	// see if the two moves could be combined
	if ( (PendingMove != None) && PendingMove.CanCombineWith(NewMove, Pawn, MaxResponseTime) )
	{
		// to combine move, first revert pawn position to PendingMove start position, before playing combined move on client
		Pawn.SetLocation(PendingMove.GetStartLocation());
		Pawn.Velocity = PendingMove.StartVelocity;
		if( PendingMove.StartBase != Pawn.Base )
		{
			Pawn.SetBase(PendingMove.StartBase);
		}
		Pawn.Floor = PendingMove.StartFloor;
		NewMove.Delta += PendingMove.Delta;
		NewMove.SetInitialPosition(Pawn);

		// remove pending move from move list
		if ( LastMove == PendingMove )
		{
			if ( SavedMoves == PendingMove )
			{
				SavedMoves.NextMove = FreeMoves;
				FreeMoves = SavedMoves;
				SavedMoves = None;
			}
			else
			{
				PendingMove.NextMove = FreeMoves;
				FreeMoves = PendingMove;
				if ( AlmostLastMove != None )
				{
					AlmostLastMove.NextMove = None;
					LastMove = AlmostLastMove;
				}
			}
			FreeMoves.Clear();
		}
		PendingMove = None;
	}

	if ( Pawn != None )
		Pawn.AutonomousPhysics(NewMove.Delta);
	else
		AutonomousPhysics(DeltaTime);
	NewMove.PostUpdate(self);

	if ( SavedMoves == None )
		SavedMoves = NewMove;
	else
		LastMove.NextMove = NewMove;

	if ( PendingMove == None )
	{
		// Decide whether to hold off on move
		// send moves more frequently in small games where server isn't likely to be saturated
		if ( (Player.CurrentNetSpeed > 10000) && (WorldInfo.GRI != None) && (WorldInfo.GRI.PRIArray.Length <= 10) )
			NetMoveDelta = 0.011;
		else
			NetMoveDelta = FMax(0.0222,2 * WorldInfo.MoveRepSize/Player.CurrentNetSpeed);

		if ( (WorldInfo.TimeSeconds - ClientUpdateTime) * WorldInfo.TimeDilation < NetMoveDelta )
		{
			PendingMove = NewMove;
			return;
		}
	}

	ClientUpdateTime = WorldInfo.TimeSeconds;

	// Send to the server
	ClientRoll = (Rotation.Roll >> 8) & 255;

	CallServerMove( NewMove,
			((Pawn == None) ? Location : Pawn.Location),
			ClientRoll,
			((Rotation.Yaw & 65535) << 16) + (Rotation.Pitch & 65535),
			OldMove );

	PendingMove = None;
}

/* CallServerMove()
Call the appropriate replicated servermove() function to send a client player move to the server
*/
function CallServerMove
(
	SavedMove NewMove,
    vector ClientLoc,
    byte ClientRoll,
    int View,
    SavedMove OldMove
)
{
	local vector BuildAccel;
	local byte OldAccelX, OldAccelY, OldAccelZ;

	// compress old move if it exists
	if ( OldMove != None )
	{
		// old move important to replicate redundantly
		BuildAccel = 0.05 * OldMove.Acceleration + vect(0.5, 0.5, 0.5);
		OldAccelX = CompressAccel(BuildAccel.X);
		OldAccelY = CompressAccel(BuildAccel.Y);
		OldAccelZ = CompressAccel(BuildAccel.Z);
		OldServerMove(OldMove.TimeStamp,OldAccelX, OldAccelY, OldAccelZ, OldMove.CompressedFlags());
	}

	if ( PendingMove != None )
	{
		// send two moves simultaneously
		DualServerMove
		(
			PendingMove.TimeStamp,
			PendingMove.Acceleration * 10,
			PendingMove.CompressedFlags(),
			((PendingMove.Rotation.Yaw & 65535) << 16) + (PendingMove.Rotation.Pitch & 65535),
			NewMove.TimeStamp,
			NewMove.Acceleration * 10,
			ClientLoc,
			NewMove.CompressedFlags(),
			ClientRoll,
			View
		);
	}
	else
	{
		ServerMove
		(
	    NewMove.TimeStamp,
	    NewMove.Acceleration * 10,
	    ClientLoc,
		NewMove.CompressedFlags(),
		ClientRoll,
	    View
		);
	}
}

/* HandleWalking:
	Called by PlayerController and PlayerInput to set bIsWalking flag, affecting Pawn's velocity */
function HandleWalking()
{
	if ( Pawn != None )
		Pawn.SetWalking( bRun != 0 );
}

reliable server function ServerRestartGame()
{
}

// Send a voice message of a certain type to a certain player.
exec function Speech( name Type, int Index, string Callsign )
{
	ServerSpeech(Type,Index,Callsign);
}

reliable server function ServerSpeech( name Type, int Index, string Callsign );

exec function RestartLevel()
{
	if( WorldInfo.NetMode==NM_Standalone )
		ClientTravel( "?restart", TRAVEL_Relative );
}

exec function LocalTravel( string URL )
{
	if( WorldInfo.NetMode==NM_Standalone )
		ClientTravel( URL, TRAVEL_Relative );
}

// ------------------------------------------------------------------------
// Loading and saving

/**
 * Console exec that initiates a quicksave and displays a string providing visual feedback.
 */
exec function QuickSave()
{
    if ( (Pawn != None) && (Pawn.Health > 0) && (WorldInfo.NetMode == NM_Standalone) )
	{
		ClientMessage(QuickSaveString);
		ConsoleCommand("DEFER SAVEGAME QUICKSAVE.SAV");
	}
}

/**
 * Loads the savegame created by the quicksave exec
 */
exec function QuickLoad()
{
	if ( WorldInfo.NetMode == NM_Standalone )
	{
		ConsoleCommand("DEFER LOADGAME QUICKSAVE.SAV");
	}
}

/** Callback the server uses to determine if the unpause can happen */
delegate bool CanUnpause()
{
	return WorldInfo.Pauser == self;
}

/* SetPause()
 Try to pause game; returns success indicator.
 Replicated to server in network games.
 */
function bool SetPause( bool bPause, optional delegate<CanUnpause> CanUnpauseDelegate )
{
	local bool bResult;

	if (WorldInfo.NetMode != NM_Client)
	{
		if (bPause == true)
		{
			bFire = 0;
			// Pause gamepad rumbling too if needed
			bResult = WorldInfo.Game.SetPause(self,CanUnpauseDelegate);
			if (bResult)
			{
				// Set the pause of gamepad rumbling state
				if (ForceFeedbackManager != None)
				{
					ForceFeedbackManager.PauseWaveform(true);
				}
			}
		}
		else
		{
			WorldInfo.Game.ClearPause();
			// If the unpause is complete, let rumble occur
			if (ForceFeedbackManager != None && WorldInfo.Pauser == None)
			{
				ForceFeedbackManager.PauseWaveform(false);
			}
		}
		}
	return bResult;
}

/** Dumps the pause state of the game */
exec function DebugPause()
{
	WorldInfo.Game.DebugPause();
}

/**
 * Returns whether the game is currently paused.
 */
final simulated function bool IsPaused()
{
	return WorldInfo.Pauser != None;
}

/* Pause()
Command to try to pause the game.
*/
exec function Pause()
{
	ServerPause();
}

reliable server function ServerPause()
{
	// Pause if not already
	if( !IsPaused() )
		SetPause(true);
	else
		SetPause(false);
}

exec function ShowMenu()
{
	// Pause if not already
	if( !IsPaused() )
	{
		SetPause(true);
	}
}

/**
 * Toggles the game's paused state if it does not match the desired pause state.
 *
 * @param	bDesiredPauseState	TRUE indicates that the game should be paused.
 */
event ConditionalPause( bool bDesiredPauseState )
{
	if ( bDesiredPauseState != IsPaused() )
	{
		SetPause(bDesiredPauseState);
	}
}

`if(`notdefined(FINAL_RELEASE))
reliable server function ServerUTrace()
{
	//@todo ronp - don't execute on the server unless we're logged in as admin
//	if ( WorldInfo.NetMode != NM_Standalone && AdminManager == None )
//		return;

	UTrace();
}


exec function UTrace()
{
	// disable the log, or we'll grind the game to a halt
	ConsoleCommand("hidelog");
	if ( Role != ROLE_Authority )
	{
		ServerUTrace();
	}

	SetUTracing( !IsUTracing() );
	`log("UTracing changed to "$ IsUTracing() $ " at "$ WorldInfo.TimeSeconds,,'UTrace');
}
`endif

// ------------------------------------------------------------------------
// Weapon changing functions

/* ThrowWeapon()
Throw out current weapon, and switch to a new weapon
*/
exec function ThrowWeapon()
{
    if ( (Pawn == None) || (Pawn.Weapon == None) )
		return;

    ServerThrowWeapon();
}

reliable server function ServerThrowWeapon()
{
    if ( Pawn.CanThrowWeapon() )
    {
		Pawn.ThrowActiveWeapon();
    }
}

/* PrevWeapon()
- switch to previous inventory group weapon
*/
exec function PrevWeapon()
{
	if ( WorldInfo.Pauser!=None )
		return;

	if ( Pawn.Weapon == None )
	{
		SwitchToBestWeapon();
		return;
	}

	if ( Pawn.InvManager != None )
		Pawn.InvManager.PrevWeapon();
}

/* NextWeapon()
- switch to next inventory group weapon
*/
exec function NextWeapon()
{
	if ( WorldInfo.Pauser!=None )
		return;

	if ( Pawn.Weapon == None )
	{
		SwitchToBestWeapon();
		return;
	}

	if ( Pawn.InvManager != None )
		Pawn.InvManager.NextWeapon();
}

// The player wants to fire.
exec function StartFire( optional byte FireModeNum )
{
	if ( WorldInfo.Pauser == PlayerReplicationInfo )
	{
		SetPause( false );
		return;
	}

	if ( Pawn != None && !bCinematicMode )
	{
		Pawn.StartFire( FireModeNum );
	}
}

exec function StopFire( optional byte FireModeNum )
{
	if ( Pawn != None )
	{
		Pawn.StopFire( FireModeNum );
	}
}

// The player wants to alternate-fire.
exec function StartAltFire( optional Byte FireModeNum )
{
	StartFire( 1 );
}

exec function StopAltFire( optional byte FireModeNum )
{
	StopFire( 1 );
}

/**
 * Looks at all nearby triggers, looking for any that can be
 * interacted with.
 *
 * @param		interactDistanceToCheck - distance to search for nearby triggers
 *
 * @param		crosshairDist - distance from the crosshair that
 * 				triggers must be, else they will be filtered out
 *
 * @param		minDot - minimum dot product between trigger and the
 * 				camera orientation needed to make the list
 *
 * @param		bUsuableOnly - if true, event must return true from
 * 				SequenceEvent::CheckActivate()
 *
 * @param		out_useList - the list of triggers found to be
 * 				usuable
 */
function GetTriggerUseList(float interactDistanceToCheck, float crosshairDist, float minDot, bool bUsuableOnly, out array<Trigger> out_useList)
{
	local int Idx;
	local vector cameraLoc;
	local rotator cameraRot;
	local Trigger checkTrigger;
	local SeqEvent_Used	UseSeq;

	if (Pawn != None)
	{
		// grab camera location/rotation for checking crosshairDist
		GetPlayerViewPoint(cameraLoc, cameraRot);

		// This doesn't work how it should.  It really needs to query ALL of the triggers and get their
		// InteractDistance and then compare those against the pawn's location and then do the various checks

		// search of nearby actors that have use events
		foreach Pawn.CollidingActors(class'Trigger',checkTrigger,interactDistanceToCheck)
		{
			for (Idx = 0; Idx < checkTrigger.GeneratedEvents.Length; Idx++)
			{
				UseSeq = SeqEvent_Used(checkTrigger.GeneratedEvents[Idx]);

				if( ( UseSeq != None )
					// if bUsuableOnly is true then we must get true back from CheckActivate (which tests various validity checks on the player and on the trigger's trigger count and retrigger conditions etc)
					&& ( !bUsuableOnly || ( checkTrigger.GeneratedEvents[Idx].CheckActivate(checkTrigger,Pawn,true)) )
					// check to see if we are looking at the object
					&& ( Normal(checkTrigger.Location-cameraLoc) dot vector(cameraRot) >= minDot )

					// if this is an aimToInteract then check to see if we are aiming at the object and we are inside the InteractDistance (NOTE: we need to do use a number close to 1.0 as the dot will give a number that is very close to 1.0 for aiming at the target)
					&& ( ( ( UseSeq.bAimToInteract && IsAimingAt( checkTrigger, 0.98f ) && ( VSize(Pawn.Location - checkTrigger.Location) <= UseSeq.InteractDistance ) ) )
					      // if we should NOT aim to interact then we need to be close to the trigger
			  || ( !UseSeq.bAimToInteract && ( VSize(Pawn.Location - checkTrigger.Location) <= UseSeq.InteractDistance ) )  // this should be UseSeq.InteractDistance
						  )
				   )
				{
					out_useList[out_useList.Length] = checkTrigger;

					// don't bother searching for more events
					Idx = checkTrigger.GeneratedEvents.Length;
				}
			}
		}
	}
}

/**
 * Entry point function for player interactions with the world,
 * re-directs to ServerUse.
 */
exec function Use()
{
	if( Role < Role_Authority )
	{
		PerformedUseAction();
	}
	ServerUse();
}

/**
 * Player pressed UseKey
 */
unreliable server function ServerUse()
{
	PerformedUseAction();
}

/**
 * return true if player the Use action was handled
 */
function bool PerformedUseAction()
{
	// if the level is paused,
	if( WorldInfo.Pauser == PlayerReplicationInfo )
	{
		if( Role == Role_Authority )
		{
			// unpause and move on
			SetPause( false );
		}
		return true;
	}

    if ( Pawn == None || !Pawn.bCanUse )
	return true;

	// below is only on server
	if( Role < Role_Authority )
	{
		return false;
	}

	// leave vehicle if currently in one
	if( Vehicle(Pawn) != None )
	{
		return Vehicle(Pawn).DriverLeave(false);
	}

	// try to find a vehicle to drive
	if( FindVehicleToDrive() )
	{
		return true;
	}

	// try to interact with triggers
	return TriggerInteracted();
}

/** Tries to find a vehicle to drive within a limited radius. Returns true if successful */
function bool FindVehicleToDrive()
{
	local Vehicle V, Best;
	local vector ViewDir, PawnLoc2D, VLoc2D;
	local float NewDot, BestDot;

	if (Vehicle(Pawn.Base) != None  && Vehicle(Pawn.Base).TryToDrive(Pawn))
	{
		return true;
	}

	// Pick best nearby vehicle
	PawnLoc2D = Pawn.Location;
	PawnLoc2D.Z = 0;
	ViewDir = vector(Pawn.Rotation);

	ForEach Pawn.OverlappingActors(class'Vehicle', V, Pawn.VehicleCheckRadius)
	{
		// Prefer vehicles that Pawn is facing
		VLoc2D = V.Location;
		Vloc2D.Z = 0;
		NewDot = Normal(VLoc2D-PawnLoc2D) Dot ViewDir;
		if ( (Best == None) || (NewDot > BestDot) )
		{
			// check that vehicle is visible
			if ( FastTrace(V.Location,Pawn.Location) )
			{
				Best = V;
				BestDot = NewDot;
			}
		}
	}
	return (Best != None && Best.TryToDrive(Pawn));
}

/**
 * Examines the nearby enviroment and generates a priority sorted
 * list of interactable actors, and then attempts to activate each
 * of them until either one was successfully activated, or no more
 * actors are available.
 */
function bool TriggerInteracted()
{
	local Actor A;
	local int Idx;
	local float Weight;
	local bool bInserted;
	local vector cameraLoc;
	local rotator cameraRot;
	local array<Trigger> useList;
	// the following 2 arrays should always match in length
	local array<Actor> sortedList;
	local array<float> weightList;

	if ( Pawn != None )
	{
		GetTriggerUseList(InteractDistance,60.f,0.f,true,useList);
		// if we have found some interactable actors,
		if (useList.Length > 0)
		{
			// grab the current camera location/rotation for weighting purposes
			GetPlayerViewPoint(cameraLoc, cameraRot);
			// then build the sorted list
			while (useList.Length > 0)
			{
				// pop the actor off this list
				A = useList[useList.Length-1];
				useList.Length = useList.Length - 1;
				// calculate the weight of this actor in terms of optimal interaction
				// first based on the dot product from our view rotation
				weight = Normal(A.Location-cameraLoc) dot vector(cameraRot);
				// and next on the distance
				weight += 1.f - (VSize(A.Location-Pawn.Location)/InteractDistance);
				// find the optimal insertion point
				bInserted = false;
				for (Idx = 0; Idx < sortedList.Length && !bInserted; Idx++)
				{
					if (weightList[Idx] < weight)
					{
						// insert the new entry
						sortedList.Insert(Idx,1);
						weightList.Insert(Idx,1);
						sortedList[Idx] = A;
						weightList[Idx] = weight;
						bInserted = true;
					}
				}
				// if no insertion was made
				if (!bInserted)
				{
					// then tack on the end of the list
					Idx = sortedList.Length;
					sortedList[Idx] = A;
					weightList[Idx] = weight;
				}
			}
			// finally iterate through each actor in the sorted list and
			// attempt to activate it until one succeeds or none are left
			for (Idx = 0; Idx < sortedList.Length; Idx++)
			{
				if (sortedList[Idx].UsedBy(Pawn))
				{
					// skip the rest
					// Idx = sortedList.Length;
					return true;
				}
			}
		}
	}

	return false;
}


exec function Suicide()
{
	ServerSuicide();
}

reliable server function ServerSuicide()
{
	if ( (Pawn != None) && ((WorldInfo.TimeSeconds - Pawn.LastStartTime > 10) || (WorldInfo.NetMode == NM_Standalone)) )
	{
		Pawn.Suicide();
	}
}

exec function SetName( coerce string S)
{
	local string NewName;
	// Conform the name locally.  This insures the URL isn't boinked.  The Server will
	// also conform the player name.

	NewName = S;

	WorldInfo.GetGameClass().static.ConformPlayerName(NewName, 20);

	ChangeName(NewName);
	UpdateURL("Name", NewName, true);
	SaveConfig();
}

reliable server function ChangeName( coerce string S )
{
    WorldInfo.Game.ChangeName( self, S, true );
}

exec function SwitchTeam()
{
	if ( (PlayerReplicationInfo.Team == None) || (PlayerReplicationInfo.Team.TeamIndex == 1) )
	{
		ServerChangeTeam(0);
	}
	else
	{
		ServerChangeTeam(1);
	}
}

exec function ChangeTeam( optional string TeamName )
{
	local int N;

	if ( TeamName ~= "blue" )
		N = 1;
	else if ( (TeamName ~= "red") || (PlayerReplicationInfo == None) || (PlayerReplicationInfo.Team == None) || (PlayerReplicationInfo.Team.TeamIndex > 1) )
		N = 0;
	else
		N = 1 - PlayerReplicationInfo.Team.TeamIndex;

	ServerChangeTeam(N);
}

reliable server function ServerChangeTeam(int N)
{
	local TeamInfo OldTeam;

	OldTeam = PlayerReplicationInfo.Team;
	WorldInfo.Game.ChangeTeam(self, N, true);
	if (WorldInfo.Game.bTeamGame && PlayerReplicationInfo.Team != OldTeam)
	{
		if (Pawn != None)
		{
			Pawn.PlayerChangedTeam();
		}
	}
}


exec function SwitchLevel(string URL)
{
	if (WorldInfo.NetMode == NM_Standalone || WorldInfo.NetMode == NM_ListenServer)
	{
		WorldInfo.ServerTravel(URL);
	}
}

exec function ClearProgressMessages()
{
	ClientClearProgressMessages();
}

reliable client function ClientClearProgressMessages()
{
	local int i;

	for (i=0; i<ArrayCount(ProgressMessage); i++)
	{
		ProgressMessage[i] = "";
		ProgressColor[i] = MakeColor(255,255,255,255);
	}
}

exec event SetProgressMessage( int InIndex, string S, color C )
{
	ClientSetProgressMessage(InIndex, S, C);
}

reliable client event ClientSetProgressMessage( int InIndex, string S, color C )
{
	if ( InIndex < ArrayCount(ProgressMessage) )
	{
		ProgressMessage[InIndex] = S;
		ProgressColor[InIndex] = C;
	}
}

exec event SetProgressTime( float T )
{
	ClientSetProgressTime(T);
}

reliable client event ClientSetProgressTime( float T )
{
	ProgressTimeOut = T + WorldInfo.TimeSeconds;
}

function Restart(bool bVehicleTransition)
{
	Super.Restart(bVehicleTransition);
	ServerTimeStamp = 0;
	ResetTimeMargin();
	EnterStartState();
	ClientRestart(Pawn);
	SetViewTarget(Pawn);
	ResetCameraMode();
}

/** called to notify the server when the client has loaded a new world via seamless travelling
 * @param WorldPackageName the name of the world package that was loaded
 */
reliable server native final event ServerNotifyLoadedWorld(name WorldPackageName);

/** called clientside when it is loaded a new world via seamless travelling
 * @param WorldPackageName the name of the world package that was loaded
 */
event NotifyLoadedWorld(name WorldPackageName)
{
	local PlayerStart P;

	// place the camera at the first playerstart we can find
	foreach WorldInfo.AllNavigationPoints(class'PlayerStart', P)
	{
		SetLocation(P.Location);
		SetRotation(P.Rotation);
		break;
	}
}

/** returns whether the client has completely loaded the server's current world (valid on server only) */
native final function bool HasClientLoadedCurrentWorld();

function EnterStartState()
{
	local name NewState;

	if ( Pawn.PhysicsVolume.bWaterVolume )
	{
		if ( Pawn.HeadVolume.bWaterVolume )
			Pawn.BreathTime = Pawn.UnderWaterTime;
		NewState = Pawn.WaterMovementState;
	}
	else
		NewState = Pawn.LandMovementState;

	if ( IsInState(NewState) )
		BeginState(NewState);
	else
		GotoState(NewState);
}

reliable client function ClientRestart(Pawn NewPawn)
{
	ResetPlayerMovementInput();
    CleanOutSavedMoves();  // don't replay moves previous to possession

	Pawn = NewPawn;
	if ( (Pawn != None) && Pawn.bTearOff )
	{
		UnPossess();
		Pawn = None;
	}

	AcknowledgePossession(Pawn);

	if ( Pawn == None )
	{
		GotoState('WaitingForPawn');
		return;
	}
	Pawn.ClientRestart();
	if (Role < ROLE_Authority)
	{
		SetViewTarget(Pawn);
		ResetCameraMode();
		EnterStartState();
	}
	CleanOutSavedMoves();
}

/* epic ===============================================
* ::GameHasEnded
*
* Called from game info upon end of the game, used to
* transition to proper state.
*
* =====================================================
*/
function GameHasEnded(optional Actor EndGameFocus, optional bool bIsWinner)
{
	// and transition to the game ended state
	SetViewTarget(EndGameFocus);
	GotoState('RoundEnded');
	ClientGameEnded(EndGameFocus, bIsWinner);
}

/* epic ===============================================
* ::ClientGameEnded
*
* Replicated function called by GameHasEnded().
*
 * @param	EndGameFocus - actor to view with camera
 * @param	bIsWinner - true if this controller is on winning team
* =====================================================
*/
reliable client function ClientGameEnded(Actor EndGameFocus, bool bIsWinner)
{
	SetViewTarget(EndGameFocus);
	GotoState('RoundEnded');
}

// Just changed to pendingWeapon
function NotifyChangedWeapon( Weapon PreviousWeapon, Weapon NewWeapon );

/**
 * PlayerTick is only called if the PlayerController has a PlayerInput object.  Therefore, it will not be called on servers for non-locally controlled playercontrollers
 */
event PlayerTick( float DeltaTime )
{
/* //@todo steve
	if ( bForcePrecache )
	{
		if ( WorldInfo.TimeSeconds > ForcePrecacheTime )
		{
			bForcePrecache = false;
			// precache game materials and staticmeshes
		}
	}
	else
*/
	if ( !bShortConnectTimeOut )
	{
		bShortConnectTimeOut = true;
		ServerShortTimeout();
	}

	if ( Pawn != AcknowledgedPawn )
	{
		if ( Role < ROLE_Authority )
		{
			// make sure old pawn controller is right
			if ( (AcknowledgedPawn != None) && (AcknowledgedPawn.Controller == self) )
				AcknowledgedPawn.Controller = None;
		}
		AcknowledgePossession(Pawn);
	}

	PlayerInput.PlayerInput(DeltaTime);
	if ( bUpdatePosition )
		ClientUpdatePosition();
	PlayerMove(DeltaTime);

	AdjustFOV(DeltaTime);
}

function PlayerMove(float DeltaTime);

function bool AimingHelp(bool bInstantHit)
{
 return (WorldInfo.NetMode == NM_Standalone) && bAimingHelp;
}

/** The function called when a CameraLookAt action is deactivated from kismet */
event CameraLookAtFinished(SeqAct_CameraLookAt Action);

/**
 * Adjusts weapon aiming direction.
 * Gives controller a chance to modify the aiming of the pawn. For example aim error, auto aiming, adhesion, AI help...
 * Requested by weapon prior to firing.
 *
 * @param	W, weapon about to fire
 * @param	StartFireLoc, world location of weapon fire start trace, or projectile spawn loc.
 * @param	BaseAimRot, original aiming rotation without any modifications.
 */
function Rotator GetAdjustedAimFor( Weapon W, vector StartFireLoc )
{
	local vector	FireDir, AimSpot, HitLocation, HitNormal, OldAim, AimOffset;
	local actor		BestTarget, HitActor;
	local float		bestAim, bestDist;
	local bool		bNoZAdjust, bInstantHit;
	local rotator	BaseAimRot, AimRot;

	bInstantHit = ( W == None || W.bInstantHit );
	BaseAimRot = (Pawn != None) ? Pawn.GetBaseAimRotation() : Rotation;
	FireDir	= vector(BaseAimRot);
	HitActor = Trace(HitLocation, HitNormal, StartFireLoc + W.GetTraceRange() * FireDir, StartFireLoc, true);

	if ( (HitActor != None) && HitActor.bProjTarget )
	{
		BestTarget = HitActor;
		bNoZAdjust = true;
		OldAim = HitLocation;
		BestDist = VSize(BestTarget.Location - Pawn.Location);
	}
	else
	{
		// adjust aim based on FOV
		bestAim = 0.90;
		if ( AimingHelp(bInstantHit) )
		{
			bestAim = AimHelpDot(bInstantHit);
		}
		else if ( bInstantHit )
			bestAim = 1.0;

		BestTarget = PickTarget(class'Pawn', bestAim, bestDist, FireDir, StartFireLoc, W.WeaponRange);
		if ( BestTarget == None )
		{
			return BaseAimRot;
		}
		OldAim = StartFireLoc + FireDir * bestDist;
	}

	ShotTarget = Pawn(BestTarget);
    if ( !AimingHelp(bInstantHit) )
	{
    	return BaseAimRot;
	}

	// aim at target - help with leading also
	FireDir = BestTarget.Location - StartFireLoc;
	AimSpot = StartFireLoc + bestDist * Normal(FireDir);
	AimOffset = AimSpot - OldAim;

    if ( ShotTarget != None )
    {
	    // adjust Z of shooter if necessary
	    if ( bNoZAdjust )
	        AimSpot.Z = OldAim.Z;
	    else if ( AimOffset.Z < 0 )
	        AimSpot.Z = ShotTarget.Location.Z + 0.4 * ShotTarget.CylinderComponent.CollisionHeight;
	    else
	        AimSpot.Z = ShotTarget.Location.Z - 0.7 * ShotTarget.CylinderComponent.CollisionHeight;
    }
    else
	    AimSpot.Z = OldAim.Z;

	// if not leading, add slight random error ( significant at long distances )
	if ( !bNoZAdjust )
	{
		AimRot = rotator(AimSpot - StartFireLoc);
		if ( FOVAngle < DefaultFOV - 8 )
			AimRot.Yaw = AimRot.Yaw + 200 - Rand(400);
		else
			AimRot.Yaw = AimRot.Yaw + 375 - Rand(750);
		return AimRot;
	}
	return rotator(AimSpot - StartFireLoc);
}

/** AimHelpDot()
* @returns the dot product corresponding to the maximum deflection of target for which aiming help should be applied
*/
function float AimHelpDot(bool bInstantHit)
{
	if ( FOVAngle < DefaultFOV - 8 )
		return 0.99;
	if ( bInstantHit )
		return 0.97;
	return 0.93;
}

event bool NotifyLanded(vector HitNormal, Actor FloorActor)
{
	return bUpdating;
}

//=============================================================================
// Player Control

// Player view.
// Compute the rendering viewpoint for the player.
//

/** AdjustFOV()
FOVAngle smoothly interpolates to DesiredFOV
*/
function AdjustFOV(float DeltaTime )
{
	if ( FOVAngle != DesiredFOV )
	{
		if ( FOVAngle > DesiredFOV )
			FOVAngle = FOVAngle - FMax(7, 0.9 * DeltaTime * (FOVAngle - DesiredFOV));
		else
			FOVAngle = FOVAngle - FMin(-7, 0.9 * DeltaTime * (FOVAngle - DesiredFOV));
		if ( Abs(FOVAngle - DesiredFOV) <= 10 )
			FOVAngle = DesiredFOV;
	}
}

/** returns player's FOV angle */
event float GetFOVAngle()
{
	return (PlayerCamera != None) ? PlayerCamera.GetFOVAngle() : FOVAngle;
}

/** returns whether this Controller is a locally controlled PlayerController
 * @note not valid until the Controller is completely spawned (i.e, unusable in Pre/PostBeginPlay())
 */
native function bool IsLocalPlayerController();

native function SetViewTarget( Actor NewViewTarget, optional float TransitionTime );

reliable client event ClientSetViewTarget( Actor A, optional float TransitionTime )
{
	if( A == None )
	{
		ServerVerifyViewTarget();
	}
    SetViewTarget( A, TransitionTime );
}

native function Actor GetViewTarget();

reliable server function ServerVerifyViewTarget()
{
	local Actor TheViewTarget;

	TheViewTarget = GetViewTarget();

	if( TheViewTarget == Self )
	{
		return;
	}
	ClientSetViewTarget( TheViewTarget );
}

event SpawnPlayerCamera()
{
	if( CameraClass != None && IsLocalPlayerController() )
	{
		// Associate Camera with PlayerController
		PlayerCamera = Spawn( CameraClass, self );
		if( PlayerCamera != None )
		{
			PlayerCamera.InitializeFor( self );
		}
		else
		{
			`Log( "Couldn't Spawn Camera Actor for Player!!" );
		}
	}
	else
	{
		// not having a CameraClass is fine.  Another class will handing the "camera" type duties
	// usually PlayerController
	}
}

/**
 * Returns Player's Point of View
 * For the AI this means the Pawn's 'Eyes' ViewPoint
 * For a Human player, this means the Camera's ViewPoint
 *
 * @output	out_Location, view location of player
 * @output	out_rotation, view rotation of player
 */
simulated event GetPlayerViewPoint( out vector out_Location, out Rotator out_Rotation )
{
	local Actor TheViewTarget;

	// sometimes the PlayerCamera can be none and we probably do not want this
	// so we will check to see if we have a CameraClass.  Having a CameraClass is
	// saying:  we want a camera so make certain one exists by spawning one
	if( PlayerCamera == None )
	{
		if( CameraClass != None )
		{
			// Associate Camera with PlayerController
			PlayerCamera = Spawn(CameraClass, Self);
			if( PlayerCamera != None )
			{
				PlayerCamera.InitializeFor( Self );
			}
			else
			{
				`log("Couldn't Spawn Camera Actor for Player!!");
			}
		}
	}

	if( PlayerCamera != None )
	{
		PlayerCamera.GetCameraViewPoint(out_Location, out_Rotation);
	}
	else
	{
		TheViewTarget = GetViewTarget();

		if( TheViewTarget != None )
		{
			out_Location = TheViewTarget.Location;
			out_Rotation = TheViewTarget.Rotation;
		}
		else
		{
			super.GetPlayerViewPoint(out_Location, out_Rotation);
		}
	}
}

/** Updates any camera view shaking that is going on */
function ViewShake(float DeltaTime);

function UpdateRotation( float DeltaTime )
{
	local Rotator	DeltaRot, newRotation, ViewRotation;

	ViewRotation = Rotation;
	DesiredRotation = ViewRotation; //save old rotation

	// Calculate Delta to be applied on ViewRotation
	DeltaRot.Yaw	= PlayerInput.aTurn;
	DeltaRot.Pitch	= PlayerInput.aLookUp;

	ProcessViewRotation( DeltaTime, ViewRotation, DeltaRot );
	SetRotation(ViewRotation);

	ViewShake( deltaTime );

	NewRotation = ViewRotation;
	NewRotation.Roll = Rotation.Roll;

	if ( Pawn != None )
		Pawn.FaceRotation(NewRotation, deltatime);
}

/**
 * Processes the player's ViewRotation
 * adds delta rot (player input), applies any limits and post-processing
 * returns the final ViewRotation set on PlayerController
 *
 * @param	DeltaTime, time since last frame
 * @param	ViewRotation, current player ViewRotation
 * @param	DeltaRot, player input added to ViewRotation
 */
function ProcessViewRotation( float DeltaTime, out Rotator out_ViewRotation, Rotator DeltaRot )
{
	if( PlayerCamera != None )
	{
		PlayerCamera.ProcessViewRotation( DeltaTime, out_ViewRotation, DeltaRot );
	}

	if ( Pawn != None )
	{	// Give the Pawn a chance to modify DeltaRot (limit view for ex.)
		Pawn.ProcessViewRotation( DeltaTime, out_ViewRotation, DeltaRot );
	}
	else
	{
		// If Pawn doesn't exist, limit view

		// Add Delta Rotation
		out_ViewRotation	+= DeltaRot;
		out_ViewRotation	 = LimitViewRotation(out_ViewRotation, -16384, 16383 );
	}
}


/**
 * Limit the player's view rotation. (Pitch component).
 */
event Rotator LimitViewRotation( Rotator ViewRotation, float ViewPitchMin, float ViewPitchMax )
{
	ViewRotation.Pitch = ViewRotation.Pitch & 65535;

    if( ViewRotation.Pitch > ViewPitchMax &&
		ViewRotation.Pitch < (65535+ViewPitchMin) )
	{
		if( ViewRotation.Pitch < 32768 )
		{
			ViewRotation.Pitch = ViewPitchMax;
		}
		else
		{
			ViewRotation.Pitch = 65535 + ViewPitchMin;
		}
	}

	return ViewRotation;
}

function ClearDoubleClick()
{
	if (PlayerInput != None)
		PlayerInput.DoubleClickTimer = 0.0;
}

/* CheckJumpOrDuck()
Called by ProcessMove()
handle jump and duck buttons which are pressed
*/
function CheckJumpOrDuck()
{
	if ( bPressedJump && (Pawn != None) )
	{
		Pawn.DoJump( bUpdating );
	}
}

// Player movement.
// Player Standing, walking, running, falling.
state PlayerWalking
{
ignores SeePlayer, HearNoise, Bump;

	event NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		if ( NewVolume.bWaterVolume )
			GotoState(Pawn.WaterMovementState);
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		if( Pawn == None )
		{
			return;
		}

		if( Role == Role_Authority && WorldInfo.NetMode != NM_StandAlone )
		{
			// Update ViewPitch for remote clients
			Pawn.SetRemoteViewPitch( Rotation.Pitch );
		}

		Pawn.Acceleration = NewAccel;

		CheckJumpOrDuck();
	}

	function PlayerMove( float DeltaTime )
	{
		local vector			X,Y,Z, NewAccel;
		local eDoubleClickDir	DoubleClickMove;
		local rotator			OldRotation;
		local bool				bSaveJump;

		if( Pawn == None )
		{
			GotoState('Dead');
		}
		else
		{
			GetAxes(Pawn.Rotation,X,Y,Z);

			// Update acceleration.
			NewAccel = PlayerInput.aForward*X + PlayerInput.aStrafe*Y;
			NewAccel.Z	= 0;
			NewAccel = Pawn.AccelRate * Normal(NewAccel);

			DoubleClickMove = PlayerInput.CheckForDoubleClickMove( DeltaTime/WorldInfo.TimeDilation );

			// Update rotation.
			OldRotation = Rotation;
			UpdateRotation( DeltaTime );
			bDoubleJump = false;

			if( bPressedJump && Pawn.CannotJumpNow() )
			{
				bSaveJump = true;
				bPressedJump = false;
			}
			else
			{
				bSaveJump = false;
			}

			if( Role < ROLE_Authority ) // then save this move and replicate it
			{
				ReplicateMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
			}
			else
			{
				ProcessMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
			}
			bPressedJump = bSaveJump;
		}
	}

	event BeginState(Name PreviousStateName)
	{
		DoubleClickDir = DCLICK_None;
		bPressedJump = false;
		GroundPitch = 0;
		if ( Pawn != None )
		{
			Pawn.ShouldCrouch(false);
			if (Pawn.Physics != PHYS_Falling && Pawn.Physics != PHYS_RigidBody) // FIXME HACK!!!
				Pawn.SetPhysics(PHYS_Walking);
		}
	}

	event EndState(Name NextStateName)
	{
		GroundPitch = 0;
		if ( Pawn != None )
		{
			Pawn.SetRemoteViewPitch( 0 );
			if ( bDuck == 0 )
			{
				Pawn.ShouldCrouch(false);
			}
		}
	}

Begin:
}

// player is climbing ladder
state PlayerClimbing
{
ignores SeePlayer, HearNoise, Bump;

	event NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		if( NewVolume.bWaterVolume )
		{
			GotoState( Pawn.WaterMovementState );
		}
		else
		{
			GotoState( Pawn.LandMovementState );
		}
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		if( Pawn == None )
		{
			return;
		}

		if( Role == Role_Authority && WorldInfo.NetMode != NM_StandAlone )
		{
			// Update ViewPitch for remote clients
			Pawn.SetRemoteViewPitch( Rotation.Pitch );
		}

		Pawn.Acceleration	= NewAccel;

		if( bPressedJump )
		{
			Pawn.DoJump( bUpdating );
			if( Pawn.Physics == PHYS_Falling )
			{
				GotoState('PlayerWalking');
			}
		}
	}

	function PlayerMove( float DeltaTime )
	{
		local vector X,Y,Z, NewAccel;
		local rotator OldRotation, ViewRotation;

		GetAxes(Rotation,X,Y,Z);

		// Update acceleration.
		if ( Pawn.OnLadder != None )
		{
			NewAccel = PlayerInput.aForward*Pawn.OnLadder.ClimbDir;
		    if ( Pawn.OnLadder.bAllowLadderStrafing )
				NewAccel += PlayerInput.aStrafe*Y;
		}
		else
			NewAccel = PlayerInput.aForward*X + PlayerInput.aStrafe*Y;
		NewAccel = Pawn.AccelRate * Normal(NewAccel);

		ViewRotation = Rotation;

		// Update rotation.
		SetRotation(ViewRotation);
		OldRotation = Rotation;
		UpdateRotation( DeltaTime );

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
		else
			ProcessMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
		bPressedJump = false;
	}

	event BeginState(Name PreviousStateName)
	{
		Pawn.ShouldCrouch(false);
		bPressedJump = false;
	}

	event EndState(Name NextStateName)
	{
		if ( Pawn != None )
		{
			Pawn.SetRemoteViewPitch( 0 );
			Pawn.ShouldCrouch(false);
		}
	}
}

// Player Driving a vehicle.
state PlayerDriving
{
ignores SeePlayer, HearNoise, Bump;

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot);

	// Set the throttle, steering etc. for the vehicle based on the input provided
	function ProcessDrive(float InForward, float InStrafe, float InUp, bool InJump)
	{
		local Vehicle CurrentVehicle;

		CurrentVehicle = Vehicle(Pawn);
		if (CurrentVehicle != None)
		{
			//`log("Forward:"@InForward@" Strafe:"@InStrafe@" Up:"@InUp);
			bPressedJump = InJump;
			CurrentVehicle.SetInputs(InForward, -InStrafe, InUp);
			CheckJumpOrDuck();
		}
	}

	function PlayerMove( float DeltaTime )
	{
		// update 'looking' rotation
		UpdateRotation(DeltaTime);

		// TODO: Don't send things like aForward and aStrafe for gunners who don't need it
		// Only servers can actually do the driving logic.
		ProcessDrive(PlayerInput.RawJoyUp, PlayerInput.RawJoyRight, PlayerInput.aUp, bPressedJump);
		if (Role < ROLE_Authority)
		{
			ServerDrive(PlayerInput.RawJoyUp, PlayerInput.RawJoyRight, PlayerInput.aUp, bPressedJump, ((Rotation.Yaw & 65535) << 16) + (Rotation.Pitch & 65535));
		}

		bPressedJump = false;
	}

	unreliable server function ServerUse()
	{
		local Vehicle CurrentVehicle;

		CurrentVehicle = Vehicle(Pawn);
		CurrentVehicle.DriverLeave(false);
	}

	event BeginState(Name PreviousStateName)
	{
		CleanOutSavedMoves();
	}

	event EndState(Name NextStateName)
	{
		CleanOutSavedMoves();
	}
}

// Player movement.
// Player Swimming
state PlayerSwimming
{
ignores SeePlayer, HearNoise, Bump;

	event bool NotifyLanded(vector HitNormal, Actor FloorActor)
	{
		if ( Pawn.PhysicsVolume.bWaterVolume )
			Pawn.SetPhysics(PHYS_Swimming);
		else
			GotoState(Pawn.LandMovementState);
		return bUpdating;
	}

	event NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		local actor HitActor;
		local vector HitLocation, HitNormal, Checkpoint;
		local vector X,Y,Z;

		if (Pawn.Physics != PHYS_RigidBody)
		{
			if ( !NewVolume.bWaterVolume )
			{
				Pawn.SetPhysics(PHYS_Falling);
				if ( Pawn.Velocity.Z > 0 )
				{
					GetAxes(Rotation,X,Y,Z);
	 				Pawn.bUpAndOut = ((X Dot Pawn.Acceleration) > 0) && ((Pawn.Acceleration.Z > 0) || (Rotation.Pitch > 2048));
				    if (Pawn.bUpAndOut && Pawn.CheckWaterJump(HitNormal)) //check for waterjump
				    {
					    Pawn.velocity.Z = Pawn.OutOfWaterZ; //set here so physics uses this for remainder of tick
					    GotoState(Pawn.LandMovementState);
				    }
				    else if ( (Pawn.Velocity.Z > 160) || !Pawn.TouchingWaterVolume() )
					    GotoState(Pawn.LandMovementState);
				    else //check if in deep water
				    {
					    Checkpoint = Pawn.Location;
					    Checkpoint.Z -= (Pawn.CylinderComponent.CollisionHeight + 6.0);
					    HitActor = Trace(HitLocation, HitNormal, Checkpoint, Pawn.Location, false);
					    if (HitActor != None)
						    GotoState(Pawn.LandMovementState);
					    else
					    {
						    SetTimer(0.7, false);
					    }
				    }
			    }
			}
			else
			{
				ClearTimer();
				Pawn.SetPhysics(PHYS_Swimming);
			}
		}
		else if (!NewVolume.bWaterVolume)
		{
			// if in rigid body, go to appropriate state, but don't modify pawn physics
			GotoState(Pawn.LandMovementState);
		}
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		Pawn.Acceleration = NewAccel;
	}

	function PlayerMove(float DeltaTime)
	{
		local rotator oldRotation;
		local vector X,Y,Z, NewAccel;

		if (Pawn == None)
		{
			GotoState('Dead');
		}
		else
		{
			GetAxes(Rotation,X,Y,Z);

			NewAccel = PlayerInput.aForward*X + PlayerInput.aStrafe*Y + PlayerInput.aUp*vect(0,0,1);
			NewAccel = Pawn.AccelRate * Normal(NewAccel);

			// Update rotation.
			oldRotation = Rotation;
			UpdateRotation( DeltaTime );

			if ( Role < ROLE_Authority ) // then save this move and replicate it
			{
				ReplicateMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
			}
			else
			{
				ProcessMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
			}
			bPressedJump = false;
		}
	}

	event Timer()
	{
		if (!Pawn.PhysicsVolume.bWaterVolume && Role == ROLE_Authority)
		{
			GotoState(Pawn.LandMovementState);
		}

		ClearTimer();
	}

	event BeginState(Name PreviousStateName)
	{
		ClearTimer();
		if (Pawn.Physics != PHYS_RigidBody)
		{
			Pawn.SetPhysics(PHYS_Swimming);
		}
	}

Begin:
}

state PlayerFlying
{
ignores SeePlayer, HearNoise, Bump;

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;

		GetAxes(Rotation,X,Y,Z);

		Pawn.Acceleration = PlayerInput.aForward*X + PlayerInput.aStrafe*Y + PlayerInput.aUp*vect(0,0,1);;
		Pawn.Acceleration = Pawn.AccelRate * Normal(Pawn.Acceleration);

		if ( bCheatFlying && (Pawn.Acceleration == vect(0,0,0)) )
			Pawn.Velocity = vect(0,0,0);
		// Update rotation.
		UpdateRotation( DeltaTime );

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, Pawn.Acceleration, DCLICK_None, rot(0,0,0));
		else
			ProcessMove(DeltaTime, Pawn.Acceleration, DCLICK_None, rot(0,0,0));
	}

	event BeginState(Name PreviousStateName)
	{
		Pawn.SetPhysics(PHYS_Flying);
	}
}

function bool IsSpectating()
{
	return false;
}

/** when spectating, tells server where the client is (client is authoritative on location when spectating) */
unreliable server function ServerSetSpectatorLocation(vector NewLoc)
{
	ClientGotoState(GetStateName());
}

state BaseSpectating
{
	function bool IsSpectating()
	{
		return true;
	}

	/**
	  * Adjust spectator velocity if "out of bounds"
	  * (above stallz or below killz)
	  */
	function bool LimitSpectatorVelocity()
	{
		if ( Location.Z > WorldInfo.StallZ )
		{
			Velocity.Z = FMin(SpectatorCameraSpeed, WorldInfo.StallZ - Location.Z - 2.0);
			return true;
		}
		else if ( Location.Z < WorldInfo.KillZ )
		{
			Velocity.Z = FMin(SpectatorCameraSpeed, WorldInfo.KillZ - Location.Z + 2.0);
			return true;
		}
		return false;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		local float		VelSize;

		/* smoothly accelerate and decelerate */
		Acceleration = Normal(NewAccel) * SpectatorCameraSpeed;

		VelSize = VSize(Velocity);
		if( VelSize > 0 )
		{
			Velocity = Velocity - (Velocity - Normal(Acceleration) * VelSize) * FMin(DeltaTime * 8, 1);
		}

		Velocity = Velocity + Acceleration * DeltaTime;
		if( VSize(Velocity) > SpectatorCameraSpeed )
		{
			Velocity = Normal(Velocity) * SpectatorCameraSpeed;
		}

		LimitSpectatorVelocity();
		if( VSize(Velocity) > 0 )
		{
			MoveSmooth( (1+bRun) * Velocity * DeltaTime );
			// correct if out of bounds after move
			if ( LimitSpectatorVelocity() )
			{
				MoveSmooth( Velocity.Z * vect(0,0,1) * DeltaTime );
			}
		}
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;

		GetAxes(Rotation,X,Y,Z);
		Acceleration = PlayerInput.aForward*X + PlayerInput.aStrafe*Y + PlayerInput.aUp*vect(0,0,1);
		UpdateRotation(DeltaTime);

		if (Role < ROLE_Authority) // then save this move and replicate it
		{
			ReplicateMove(DeltaTime, Acceleration, DCLICK_None, rot(0,0,0));
		}
		else
		{
			ProcessMove(DeltaTime, Acceleration, DCLICK_None, rot(0,0,0));
		}
	}

	/** when spectating, tells server where the client is (client is authoritative on location when spectating) */
	unreliable server function ServerSetSpectatorLocation(vector NewLoc)
	{
		SetLocation(NewLoc);
	}

	function ReplicateMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		ProcessMove(DeltaTime, NewAccel, DoubleClickMove, DeltaRot);
		// when spectating, client position is authoritative
		ServerSetSpectatorLocation(Location);
	}
}

unreliable server function ServerViewNextPlayer()
{
	ViewAPlayer(+1);
}

unreliable server function ServerViewPrevPlayer()
{
	ViewAPlayer(-1);
}

/**
 * View next active player in PRIArray.
 * @param dir is the direction to go in the array
 */
function ViewAPlayer(int dir)
{
    local int i, CurrentIndex, NewIndex;
	local PlayerReplicationInfo PRI;
	local bool bSuccess;

	CurrentIndex = -1;
	if ( RealViewTarget != None )
	{
		// Find index of current viewtarget's PRI
		For ( i=0; i<WorldInfo.GRI.PRIArray.Length; i++ )
		{
			if ( RealViewTarget == WorldInfo.GRI.PRIArray[i] )
			{
				CurrentIndex = i;
				break;
			}
		}
	}

	// Find next valid viewtarget in appropriate direction
	for ( NewIndex=CurrentIndex+dir; (NewIndex>=0)&&(NewIndex<WorldInfo.GRI.PRIArray.Length); NewIndex=NewIndex+dir )
	{
		PRI = WorldInfo.GRI.PRIArray[NewIndex];
		if ( (PRI != None) && (Controller(PRI.Owner) != None) && (Controller(PRI.Owner).Pawn != None)
			&& WorldInfo.Game.CanSpectate(self, PRI) )
		{
			bSuccess = true;
			break;
		}
	}

	if ( !bSuccess )
	{
		// wrap around
		CurrentIndex = (NewIndex < 0) ? WorldInfo.GRI.PRIArray.Length : -1;
		for ( NewIndex=CurrentIndex+dir; (NewIndex>=0)&&(NewIndex<WorldInfo.GRI.PRIArray.Length); NewIndex=NewIndex+dir )
		{
			PRI = WorldInfo.GRI.PRIArray[NewIndex];
		if ( (PRI != None) && (Controller(PRI.Owner) != None) && (Controller(PRI.Owner).Pawn != None)
			&& WorldInfo.Game.CanSpectate(self, PRI) )
			{
				bSuccess = true;
				break;
			}
		}
	}

	if ( bSuccess )
		SetViewTarget(PRI);
}

unreliable server function ServerViewSelf()
{
	ResetCameraMode();
    SetViewTarget( Self );
    ClientSetViewTarget( Self );
	ClientMessage(OwnCamera, 'Event');
}

state Spectating extends BaseSpectating
{
	ignores RestartLevel, ClientRestart, Suicide, ThrowWeapon, NotifyPhysicsVolumeChange, NotifyHeadVolumeChange;

	exec function StartFire( optional byte FireModeNum )
	{
		ServerViewNextPlayer();
	}

	// Return to spectator's own camera.
	exec function StartAltFire( optional byte FireModeNum )
	{
		ResetCameraMode();
		ServerViewSelf();
	}

	event BeginState(Name PreviousStateName)
	{
		if ( Pawn != None )
		{
			SetLocation(Pawn.Location);
			UnPossess();
		}
		bCollideWorld = true;
	}

	event EndState(Name NextStateName)
	{
		if ( PlayerReplicationInfo.bOnlySpectator )
		{
			`log("WARNING - Spectator only player leaving spectating state to go to "$NextStateName);
		}
		PlayerReplicationInfo.bIsSpectator = false;
		bCollideWorld = false;
	}
}

auto state PlayerWaiting extends BaseSpectating
{
ignores SeePlayer, HearNoise, NotifyBump, TakeDamage, PhysicsVolumeChange, NextWeapon, PrevWeapon, SwitchToBestWeapon;

	exec function Jump();
	exec function Suicide();

	reliable server function ServerSuicide();

	reliable server function ServerChangeTeam( int N )
	{
		WorldInfo.Game.ChangeTeam(self, N, true);
	}

	reliable server function ServerRestartPlayer()
	{

		if ( WorldInfo.TimeSeconds < WaitDelay )
			return;
		if ( WorldInfo.NetMode == NM_Client )
			return;
		if ( WorldInfo.Game.bWaitingToStartMatch )
			PlayerReplicationInfo.bReadyToPlay = true;
		else
			WorldInfo.Game.RestartPlayer(self);
	}

	exec function StartFire( optional byte FireModeNum )
	{
		ServerReStartPlayer();
	}

	event EndState(Name NextStateName)
	{
		if ( PlayerReplicationInfo != None )
		{
			PlayerReplicationInfo.SetWaitingPlayer(false);
		}
		bCollideWorld = false;
	}

	// @note: this must be simulated to execute on the client because at the time the initial state is entered, RemoteRole has not been
	// set yet and so only simulated functions will be executed
	simulated event BeginState(Name PreviousStateName)
	{
		if ( PlayerReplicationInfo != None )
		{
			PlayerReplicationInfo.SetWaitingPlayer(true);
		}
		bCollideWorld = true;
	}
}

state WaitingForPawn extends BaseSpectating
{
	ignores SeePlayer, HearNoise, KilledBy;

	exec function StartFire( optional byte FireModeNum )
	{
		AskForPawn();
	}

	reliable client function ClientGotoState(name NewState, optional name NewLabel)
	{
		if (NewState == 'RoundEnded')
		{
			Global.ClientGotoState(NewState, NewLabel);
		}
	}

	unreliable client function LongClientAdjustPosition
	(
		float TimeStamp,
		name newState,
		EPhysics newPhysics,
		float NewLocX,
		float NewLocY,
		float NewLocZ,
		float NewVelX,
		float NewVelY,
		float NewVelZ,
		Actor NewBase,
		float NewFloorX,
		float NewFloorY,
		float NewFloorZ
	)
	{
		if ( newState == 'RoundEnded' )
			GotoState(newState);
	}

	event PlayerTick(float DeltaTime)
	{
		Global.PlayerTick(DeltaTime);

		if ( Pawn != None )
		{
			Pawn.Controller = self;
			Pawn.BecomeViewTarget(self);
			ClientRestart(Pawn);
		}
		else if ( !IsTimerActive() || GetTimerCount() > 1.f )
		{
			SetTimer(0.2,true);
			AskForPawn();
		}
	}

	function ReplicateMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		ProcessMove(DeltaTime, NewAccel, DoubleClickMove, DeltaRot);
		// do not actually call ServerSetSpectatorLocation() as the server is not in this state and so won't accept it anyway
		// something is wrong if we're in this state for an extended period of time, so the fact that
		// the server side spectator location is not being updated should not be relevant
	}

	event Timer()
	{
		AskForPawn();
	}

	event BeginState(Name PreviousStateName)
	{
		SetTimer(0.2, true);
		AskForPawn();
	}

	event EndState(Name NextStateName)
	{
		ResetCameraMode();
		SetTimer(0.0, false);
	}
}

state RoundEnded
{
ignores SeePlayer, HearNoise, KilledBy, NotifyBump, HitWall, NotifyHeadVolumeChange, NotifyPhysicsVolumeChange, Falling, TakeDamage, Suicide;

	reliable server function ServerReStartPlayer()
	{
	}

	function bool IsSpectating()
	{
		return true;
	}

	exec function ThrowWeapon() {}
	exec function Use() {}

	event Possess(Pawn aPawn, bool bVehicleTransition)
	{
		Global.Possess(aPawn, bVehicleTransition);

		if (Pawn != None)
			Pawn.TurnOff();
	}

	reliable server function ServerReStartGame()
	{
		if (WorldInfo.Game.PlayerCanRestartGame(self))
		{
			WorldInfo.Game.ResetLevel();
		}
	}

	exec function StartFire( optional byte FireModeNum )
	{
		if ( Role < ROLE_Authority)
			return;
		if ( !bFrozen )
			ServerReStartGame();
		else if ( !IsTimerActive() )
			SetTimer(1.5, false);
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;
		local Rotator DeltaRot, ViewRotation;

		GetAxes(Rotation,X,Y,Z);
		// Update view rotation.
		ViewRotation = Rotation;
		// Calculate Delta to be applied on ViewRotation
		DeltaRot.Yaw	= PlayerInput.aTurn;
		DeltaRot.Pitch	= PlayerInput.aLookUp;
		ProcessViewRotation( DeltaTime, ViewRotation, DeltaRot );
		SetRotation(ViewRotation);

		ViewShake(DeltaTime);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		else
			ProcessMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		bPressedJump = false;
	}

	unreliable server function ServerMove
	(
		float TimeStamp,
		vector InAccel,
		vector ClientLoc,
		byte NewFlags,
		byte ClientRoll,
		int View
	)
	{
	Global.ServerMove(	TimeStamp,
							InAccel,
							ClientLoc,
							NewFlags,
							ClientRoll,
							//epic superville: Cleaner compression with no roundoff error
							((Rotation.Yaw & 65535) << 16) + (Rotation.Pitch & 65535)
						);

	}

	function FindGoodView()
	{
		local rotator GoodRotation;

		GoodRotation = Rotation;
		GetViewTarget().FindGoodEndView(self, GoodRotation);
		SetRotation(GoodRotation);
	}

	event Timer()
	{
		bFrozen = false;
	}

	unreliable client function LongClientAdjustPosition
	(
		float TimeStamp,
		name newState,
		EPhysics newPhysics,
		float NewLocX,
		float NewLocY,
		float NewLocZ,
		float NewVelX,
		float NewVelY,
		float NewVelZ,
		Actor NewBase,
		float NewFloorX,
		float NewFloorY,
		float NewFloorZ
	)
	{
	}

	event BeginState(Name PreviousStateName)
	{
		local Pawn P;

		FOVAngle = DesiredFOV;
		bFire = 0;

		if( Pawn != None )
		{
			Pawn.TurnOff();
			Pawn.bSpecialHUD = FALSE;
			StopFiring();
		}

		if( myHUD != None )
		{
			myHUD.bShowScores = TRUE;
		}

		bFrozen = TRUE;
		FindGoodView();
		SetTimer(5, FALSE);

		ForEach DynamicActors(class'Pawn', P)
		{
			P.TurnOff();
		}
	}

	event EndState(name NextStateName)
	{
		if (myHUD != None)
		{
			myHUD.bShowScores = false;
		}
	}

Begin:
}

state Dead
{
	ignores SeePlayer, HearNoise, KilledBy, NextWeapon, PrevWeapon;

	exec function ThrowWeapon()
	{
		//clientmessage("Throwweapon while dead, pawn "$Pawn$" health "$Pawn.health);
	}

	function bool IsDead()
	{
		return true;
	}

	reliable server function ServerReStartPlayer()
	{
		if ( !WorldInfo.Game.PlayerCanRestart( Self ) )
			return;

		super.ServerRestartPlayer();
	}

	exec function StartFire( optional byte FireModeNum )
	{
		if ( bFrozen )
		{
			if ( !IsTimerActive() || GetTimerCount() > 1.f )
				bFrozen = false;
			return;
		}

		ServerReStartPlayer();
	}

	exec function Use()
	{
		StartFire(0);
	}

	exec function Jump()
	{
		StartFire(0);
	}

	unreliable server function ServerMove
	(
		float TimeStamp,
		vector Accel,
		vector ClientLoc,
		byte NewFlags,
		byte ClientRoll,
		int View
	)
	{
		Global.ServerMove(
					TimeStamp,
					Accel,
					ClientLoc,
					0,
					ClientRoll,
					View);
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;
		local rotator DeltaRot, ViewRotation;

		if ( !bFrozen )
		{
			if ( bPressedJump )
			{
				StartFire( 0 );
				bPressedJump = false;
			}
			GetAxes(Rotation,X,Y,Z);
			// Update view rotation.
			ViewRotation = Rotation;
			// Calculate Delta to be applied on ViewRotation
			DeltaRot.Yaw	= PlayerInput.aTurn;
			DeltaRot.Pitch	= PlayerInput.aLookUp;
			ProcessViewRotation( DeltaTime, ViewRotation, DeltaRot );
			SetRotation(ViewRotation);
			if ( Role < ROLE_Authority ) // then save this move and replicate it
					ReplicateMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		}
		else if ( !IsTimerActive() || GetTimerCount() > MinRespawnDelay )
		{
			bFrozen = false;
		}

		ViewShake(DeltaTime);
	}

	function FindGoodView()
	{
		local vector cameraLoc;
		local rotator cameraRot, ViewRotation;
		local int tries, besttry;
		local float bestdist, newdist;
		local int startYaw;
		local Actor TheViewTarget;

		ViewRotation = Rotation;
		ViewRotation.Pitch = 56000;
		tries = 0;
		besttry = 0;
		bestdist = 0.0;
		startYaw = ViewRotation.Yaw;
		TheViewTarget = GetViewTarget();

		for (tries=0; tries<16; tries++)
		{
			cameraLoc = TheViewTarget.Location;
			SetRotation(ViewRotation);
			GetPlayerViewPoint( cameraLoc, cameraRot );
			newdist = VSize(cameraLoc - TheViewTarget.Location);
			if (newdist > bestdist)
			{
				bestdist = newdist;
				besttry = tries;
			}
			ViewRotation.Yaw += 4096;
		}

		ViewRotation.Yaw = startYaw + besttry * 4096;
		SetRotation(ViewRotation);
	}

	event Timer()
	{
		if (!bFrozen)
			return;

		bFrozen = false;
		bPressedJump = false;
	}

	event BeginState(Name PreviousStateName)
	{
		if ( (Pawn != None) && (Pawn.Controller == self) )
			Pawn.Controller = None;
		Pawn = None;
		FOVAngle = DesiredFOV;
		Enemy = None;
		bFrozen = true;
		bPressedJump = false;
		FindGoodView();
	    SetTimer(MinRespawnDelay, false);
		CleanOutSavedMoves();
	}

	event EndState(Name NextStateName)
	{
		CleanOutSavedMoves();
		Velocity = vect(0,0,0);
		Acceleration = vect(0,0,0);
	    if ( !PlayerReplicationInfo.bOutOfLives )
			ResetCameraMode();
		bPressedJump = false;
	    if ( myHUD != None )
		{
			myHUD.bShowScores = false;
		}
	}

Begin:
	if ( LocalPlayer(Player) != None )
	{
		if (myHUD != None)
		{
			myHUD.PlayerOwnerDied();
		}
	}
}

function bool CanRestartPlayer()
{
    return !PlayerReplicationInfo.bOnlySpectator && HasClientLoadedCurrentWorld();
}

/**
 * Hook called from HUD actor. Gives access to HUD and Canvas
 */
function DrawHUD( HUD H )
{
	if ( Pawn != None )
	{
    	Pawn.DrawHUD( H );
	}

	PlayerInput.DrawHUD( H );
}


/* epic ===============================================
 * ::OnToggleInput
 *
 * Looks at the activated input from the SeqAct_ToggleInput
 * op and sets bPlayerInputEnabled accordingly.
 *
 * =====================================================
 */

function OnToggleInput(SeqAct_ToggleInput inAction)
{
	local bool bNewValue;

	if (Role < ROLE_Authority)
	{
		`Warn("Not supported on client");
		return;
	}

	if( inAction.InputLinks[0].bHasImpulse )
	{
		if( inAction.bToggleMovement )
		{
			IgnoreMoveInput( FALSE );
			ClientIgnoreMoveInput(false);
		}
		if( inAction.bToggleTurning )
		{
			IgnoreLookInput( FALSE );
			ClientIgnoreLookInput(false);
		}
	}
	else
	if( inAction.InputLinks[1].bHasImpulse )
	{
		if( inAction.bToggleMovement )
		{
			IgnoreMoveInput( TRUE );
			ClientIgnoreMoveInput(true);
		}
		if( inAction.bToggleTurning )
		{
			IgnoreLookInput( TRUE );
			ClientIgnoreLookInput(true);
		}
	}
	else
	if( inAction.InputLinks[2].bHasImpulse )
	{
		if( inAction.bToggleMovement )
		{
			bNewValue = !IsMoveInputIgnored();
			IgnoreMoveInput(bNewValue);
			ClientIgnoreMoveInput(bNewValue);
		}
		if( inAction.bToggleTurning )
		{
			bNewValue = !IsLookInputIgnored();
			IgnoreLookInput(bNewValue);
			ClientIgnoreLookInput(bNewValue);
		}
	}
}

/** calls IgnoreMoveInput on client */
client reliable function ClientIgnoreMoveInput(bool bIgnore)
{
	IgnoreMoveInput(bIgnore);
}
/** calls IgnoreLookInput on client */
client reliable function ClientIgnoreLookInput(bool bIgnore)
{
	IgnoreLookInput(bIgnore);
}

/**
 * list important PlayerController variables on canvas.  HUD will call DisplayDebug() on the current ViewTarget when
 * the ShowDebug exec is used
 *
 * @param	HUD		- HUD with canvas to draw on
 * @input	out_YL		- Height of the current font
 * @input	out_YPos	- Y position on Canvas. out_YPos += out_YL, gives position to draw text for next debug line.
 */
simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	super.DisplayDebug(HUD, out_YL, out_YPos);

	if (HUD.ShouldDisplayDebug('camera'))
	{
		if( PlayerCamera != None )
		{
			PlayerCamera.DisplayDebug( HUD, out_YL, out_YPos );
		}
		else
		{
			HUD.Canvas.SetDrawColor(255,0,0);
			HUD.Canvas.DrawText("NO CAMERA");
			out_YPos += out_YL;
			HUD.Canvas.SetPos(4, out_YPos);
		}
	}
	if ( HUD.ShouldDisplayDebug('input') )
	{
		HUD.Canvas.SetDrawColor(255,0,0);
		HUD.Canvas.DrawText("Input ignoremove "$bIgnoreMoveInput$" ignore look "$bIgnoreLookInput$" aForward "$PlayerInput.aForward);
		out_YPos += out_YL;
		HUD.Canvas.SetPos(4, out_YPos);
	}
}

/* epic ===============================================
* ::OnSetCameraTarget
*
* Sets the specified view target.
*
* =====================================================
*/
simulated function OnSetCameraTarget(SeqAct_SetCameraTarget inAction)
{
	local Actor	RealCameraTarget;

	RealCameraTarget = inAction.CameraTarget;
	// If we're asked to view a Controller, set its Pawn as out view target instead.
	if( Controller(RealCameraTarget) != None )
	{
		RealCameraTarget = Controller(RealCameraTarget).Pawn;
	}

	SetViewTarget( RealCameraTarget );
}


simulated function OnToggleHUD(SeqAct_ToggleHUD inAction)
{
	if (myHUD != None)
	{
		if (inAction.InputLinks[0].bHasImpulse)
		{
			myHUD.bShowHUD = true;
		}
		else
		if (inAction.InputLinks[1].bHasImpulse)
		{
			myHUD.bShowHUD = false;
		}
		else
		if (inAction.InputLinks[2].bHasImpulse)
		{
			myHUD.bShowHUD = !myHUD.bShowHUD;
		}
	}
}

/**
 * Attempts to match the name passed in to a SeqEvent_Console
 * object and then activate it.
 *
 * @param		eventName - name of the event to cause
 */
unreliable server function ServerCauseEvent(Name EventName)
{
	local array<SequenceObject> AllConsoleEvents;
	local SeqEvent_Console ConsoleEvt;
	local Sequence GameSeq;
	local int Idx;
	local bool bFoundEvt;
	// Get the gameplay sequence.
	GameSeq = WorldInfo.GetGameSequence();
	if(GameSeq != None)
	{
		// Find all SeqEvent_Console objects anywhere.
		GameSeq.FindSeqObjectsByClass(class'SeqEvent_Console', TRUE, AllConsoleEvents);

		// Iterate over them, seeing if the name is the one we typed in.
		for( Idx=0; Idx < AllConsoleEvents.Length; Idx++ )
		{
			ConsoleEvt = SeqEvent_Console(AllConsoleEvents[Idx]);
			if (ConsoleEvt != None &&
				EventName == ConsoleEvt.ConsoleEventName)
			{
				bFoundEvt = TRUE;
				// activate the vent
				ConsoleEvt.CheckActivate(self, Pawn);
			}
		}
	}
	if (!bFoundEvt)
	{
		ListConsoleEvents();
	}
}

exec function CauseEvent(Name EventName)
{
	ServerCauseEvent(EventName);
}

/**
 * Shortcut version for LDs who get tired of typing 'CauseEvent' all day. :-)
 */
exec function CE(Name EventName)
{
	ServerCauseEvent(EventName);
}

/**
 * Lists all console events to the HUD.
 */
exec function ListConsoleEvents()
{
	local array<SequenceObject> ConsoleEvents;
	local SeqEvent_Console ConsoleEvt;
	local Sequence GameSeq;
	local int Idx;
	GameSeq = WorldInfo.GetGameSequence();
	if (GameSeq != None)
	{
		`log("Console events:");
		ClientMessage("Console events:",,15.f);
		GameSeq.FindSeqObjectsByClass(class'SeqEvent_Console',TRUE,ConsoleEvents);
		for (Idx = 0; Idx < ConsoleEvents.Length; Idx++)
		{
			ConsoleEvt = SeqEvent_Console(ConsoleEvents[Idx]);
			if (ConsoleEvt != None &&
				ConsoleEvt.bEnabled)
			{
				`log("-"@ConsoleEvt.ConsoleEventName@ConsoleEvt.EventDesc);
				ClientMessage("-"@ConsoleEvt.ConsoleEventName@ConsoleEvt.EventDesc,,15.f);
			}
		}
	}
}

exec function ListCE()
{
	ListConsoleEvents();
}

/**
 * Notification from pawn that it has received damage
 * via TakeDamage().
 */
function NotifyTakeHit(Controller InstigatedBy, vector HitLocation, int Damage,
	class<DamageType> damageType, vector Momentum)
{
	Super.NotifyTakeHit(InstigatedBy,HitLocation,Damage,damageType,Momentum);

	// Play waveform
	ClientPlayForceFeedbackWaveform(damageType.default.DamagedFFWaveform);
}

/**
 * Kismet interface for playing/stopping force feedback.
 */
function OnForceFeedback(SeqAct_ForceFeedback Action)
{
	if (Action.InputLinks[0].bHasImpulse)
	{
		ClientPlayForceFeedbackWaveform(Action.FFWaveform);
	}
	else
	if (Action.InputLinks[1].bHasImpulse)
	{
		ClientStopForceFeedbackWaveform(Action.FFWaveform);
	}
}

/**
 * Tells the client to play a waveform for the specified damage type
 *
 * @param FFWaveform The forcefeedback waveform to play
 */
reliable client final function ClientPlayForceFeedbackWaveform(ForceFeedbackWaveform FFWaveform)
{
	if( ForceFeedbackManager != None && PlayerReplicationInfo.bControllerVibrationAllowed )
	{
		ForceFeedbackManager.PlayForceFeedbackWaveform(FFWaveform);
	}
}

/**
 * Tells the client to stop any waveform that is playing. Note if the optional
 * parameter is passed in, then the waveform is only stopped if it matches
 *
 * @param FFWaveform The forcefeedback waveform to stop
 */
reliable client final function ClientStopForceFeedbackWaveform(optional ForceFeedbackWaveform FFWaveform)
{
	if( ForceFeedbackManager != None )
	{
		ForceFeedbackManager.StopForceFeedbackWaveform(FFWaveform);
	}
}

/**
 * Camera Shake
 * Plays camera shake effect
 *
 * @param	Duration			Duration in seconds of shake
 * @param	newRotAmplitude		view rotation amplitude (pitch,yaw,roll)
 * @param	newRotFrequency		frequency of rotation shake
 * @param	newLocAmplitude		relative view offset amplitude (x,y,z)
 * @param	newLocFrequency		frequency of view offset shake
 * @param	newFOVAmplitude		fov shake amplitude
 * @param	newFOVFrequency		fov shake frequency
 */
function CameraShake
(
	float	Duration,
	vector	newRotAmplitude,
	vector	newRotFrequency,
	vector	newLocAmplitude,
	vector	newLocFrequency,
	float	newFOVAmplitude,
	float	newFOVFrequency
);

/**
 * Handles switching the player in/out of cinematic mode.
 */
function OnToggleCinematicMode(SeqAct_ToggleCinematicMode Action)
{
	local bool bNewCinematicMode;

	if (Role < ROLE_Authority)
	{
		`Warn("Not supported on client");
		return;
	}

	if (Action.InputLinks[0].bHasImpulse)
	{
		bNewCinematicMode = TRUE;
	}
	else if (Action.InputLinks[1].bHasImpulse)
	{
		bNewCinematicMode = FALSE;
	}
	else if (Action.InputLinks[2].bHasImpulse)
	{
		bNewCinematicMode = !bCinematicMode;
	}

	SetCinematicMode(bNewCinematicMode, Action.bHidePlayer, Action.bHideHUD, Action.bDisableMovement, Action.bDisableTurning, Action.bDisableInput);
}

/**
 * Server/SP only function for changing whether the player is in cinematic mode.  Updates values of various state variables, then replicates the call to the client
 * to sync the current cinematic mode.
 *
 * @param	bInCinematicMode	specify TRUE if the player is entering cinematic mode; FALSE if the player is leaving cinematic mode.
 * @param	bHidePlayer			specify TRUE to hide the player's pawn (only relevant if bInCinematicMode is TRUE)
 * @param	bAffectsHUD			specify TRUE if we should show/hide the HUD to match the value of bCinematicMode
 * @param	bAffectsMovement	specify TRUE to disable movement in cinematic mode, enable it when leaving
 * @param	bAffectsTurning		specify TRUE to disable turning in cinematic mode or enable it when leaving
 * @param	bAffectsButtons		specify TRUE to disable button input in cinematic mode or enable it when leaving.
 */
function SetCinematicMode( bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning, bool bAffectsButtons )
{
	local bool bAdjustMoveInput, bAdjustLookInput;

	bCinematicMode = bInCinematicMode;

	// if now in cinematic mode
	if( bCinematicMode )
	{
		// hide the player
		if (Pawn != None && bHidePlayer)
		{
			Pawn.SetHidden(True);
		}
	}
	else
	{
		if( Pawn != None )
		{
			Pawn.SetHidden(False);
		}
	}

	bAdjustMoveInput = bAffectsMovement && (bCinematicMode != bCinemaDisableInputMove);
	bAdjustLookInput = bAffectsTurning && (bCinematicMode != bCinemaDisableInputLook);
	if ( bAdjustMoveInput )
	{
		IgnoreMoveInput(bCinematicMode);
		bCinemaDisableInputMove = bCinematicMode;
	}
	if ( bAdjustLookInput )
	{
		IgnoreLookInput(bCinematicMode);
		bCinemaDisableInputLook = bCinematicMode;
	}

	ClientSetCinematicMode(bCinematicMode, bAdjustMoveInput, bAdjustLookInput, bAffectsHUD);
}

/** called by the server to synchronize cinematic transitions with the client */
reliable client function ClientSetCinematicMode(bool bInCinematicMode, bool bAffectsMovement, bool bAffectsTurning, bool bAffectsHUD)
{
	bCinematicMode = bInCinematicMode;

	// if there's a hud, set whether it should be shown or not
	if ( (myHUD != None) && bAffectsHUD )
	{
		myHUD.bShowHUD = !bCinematicMode;
	}

	if (bAffectsMovement)
	{
		IgnoreMoveInput(bCinematicMode);
	}
	if (bAffectsTurning)
	{
		IgnoreLookInput(bCinematicMode);
	}
}

/** Toggles move input. FALSE means movement input is cleared. */
function IgnoreMoveInput( bool bNewMoveInput )
{
	bIgnoreMoveInput = Max( bIgnoreMoveInput + (bNewMoveInput ? +1 : -1), 0 );
	//`Log("IgnoreMove: " $ bIgnoreMoveInput);
}


/** return TRUE if movement input is ignored. */
function bool IsMoveInputIgnored()
{
	return (bIgnoreMoveInput > 0);
}


/** Toggles look input. FALSE means look input is cleared. */
function IgnoreLookInput( bool bNewLookInput )
{
	bIgnoreLookInput = Max( bIgnoreLookInput + (bNewLookInput ? +1 : -1), 0 );
	//`Log("IgnoreLook: " $ bIgnoreLookInput);
}


/** return TRUE if look input is ignored. */
function bool IsLookInputIgnored()
{
	return (bIgnoreLookInput > 0);
}


/** reset input to defaults */
function ResetPlayerMovementInput()
{
	bIgnoreMoveInput = default.bIgnoreMoveInput;
	bIgnoreLookInput = default.bIgnoreLookInput;
}


/** Kismet hook to trigger console events */
function OnConsoleCommand( SeqAct_ConsoleCommand inAction )
{
	ConsoleCommand( inAction.Command );
}

/** forces GC at the end of the tick on the client */
reliable client event ClientForceGarbageCollection()
{
	WorldInfo.ForceGarbageCollection();
}

event LevelStreamingStatusChanged(LevelStreaming LevelObject, bool bNewShouldBeLoaded, bool bNewShouldBeVisible, bool bNewShouldBlockOnLoad )
{
	ClientUpdateLevelStreamingStatus(LevelObject.PackageName,bNewShouldBeLoaded,bNewShouldBeVisible,bNewShouldBlockOnLoad);
}

native reliable client final function ClientUpdateLevelStreamingStatus(Name PackageName, bool bNewShouldBeLoaded, bool bNewShouldBeVisible, bool bNewShouldBlockOnLoad);

/** called when the client adds/removes a streamed level
 * the server will only replicate references to Actors in visible levels so that it's impossible to send references to
 * Actors the client has not initialized
 * @param PackageName the name of the package for the level whose status changed
 */
native reliable server final event ServerUpdateLevelVisibility(name PackageName, bool bIsVisible);

/** asynchronously loads the given level in preparation for a streaming map transition.
 * the server sends one function per level name since dynamic arrays can't be replicated
 * @param LevelNames the names of the level packages to load. LevelNames[0] will be the new persistent (primary) level
 * @param bFirst whether this is the first item in the list (so clear the list first)
 * @param bLast whether this is the last item in the list (so start preparing the change after receiving it)
 */
reliable client event ClientPrepareMapChange(name LevelName, bool bFirst, bool bLast)
{
	// Only call on the first local player controller to handle it being called on multiple PCs for splitscreen.
	local PlayerController PC;
	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		if( PC != self )
		{
			return;
		}
		else
		{
			break;
		}
	}

	if (bFirst)
	{
		PendingMapChangeLevelNames.length = 0;
		ClearTimer('DelayedPrepareMapChange');
	}
	PendingMapChangeLevelNames[PendingMapChangeLevelNames.length] = LevelName;
	if (bLast)
	{
		DelayedPrepareMapChange();
	}
}

/** used to wait until a map change can be prepared when one was already in progress */
function DelayedPrepareMapChange()
{
	if (WorldInfo.IsPreparingMapChange())
	{
		// we must wait for the previous one to complete
		SetTimer(0.01, false, 'DelayedPrepareMapChange');
	}
	else
	{
		WorldInfo.PrepareMapChange(PendingMapChangeLevelNames);
	}
}

/** actually performs the level transition prepared by PrepareMapChange() */
reliable client event ClientCommitMapChange(optional bool bShouldSkipLevelStartupEvent, optional bool bShouldSkipLevelBeginningEvent)
{
	if (Pawn != None)
	{
		SetViewTarget(Pawn);
	}
	else
	{
		SetViewTarget(self);
	}
	WorldInfo.CommitMapChange(bShouldSkipLevelStartupEvent, bShouldSkipLevelBeginningEvent);
}

/** tells the client to block until all pending level streaming actions are complete
 * happens at the end of the tick
 * primarily used to force update the client ASAP at join time
 */
reliable client native final event ClientFlushLevelStreaming();

/** sets bRequestedBlockOnAsyncLoading which will later bring up a loading screen and then finish any async loading in progress
 * called automatically on all clients whenever something triggers it on the server
 */
reliable client event ClientSetBlockOnAsyncLoading()
{
	WorldInfo.bRequestedBlockOnAsyncLoading = true;
}

/**
 * Force a save config on the specified class.
 */
exec function SaveClassConfig(coerce string className)
{
	local class<Object> saveClass;
	`log("SaveClassConfig:"@className);
	saveClass = class<Object>(DynamicLoadObject(className,class'class'));
	if (saveClass != None)
	{
		`log("- Saving config on:"@saveClass);
		saveClass.static.StaticSaveConfig();
	}
	else
	{
		`log("- Failed to find class:"@className);
	}
}

/**
 * Force a save config on the specified actor.
 */
exec function SaveActorConfig(coerce Name actorName)
{
	local Actor chkActor;
	`log("SaveActorConfig:"@actorName);
	foreach AllActors(class'Actor',chkActor)
	{
		if (chkActor != None &&
			chkActor.Name == actorName)
		{
			`log("- Saving config on:"@chkActor);
			chkActor.SaveConfig();
		}
	}
}

/**
 * Returns the interaction that manages the UI system.
 */
final function UIInteraction GetUIController()
{
	local LocalPlayer LP;
	local UIInteraction Result;

	LP = LocalPlayer(Player);
	if ( LP != None && LP.GameViewport != None )
	{
		Result = LP.GameViewport.UIController;
	}

	return Result;
}

/**
 * Native function to determine if voice data should be received from this player.
 * Only called on the server to determine whether voice packet replication
 * should happen for the given sender.
 *
 * NOTE: This function is final because it can be called n^2 number of times
 * in a given frame, where n is the number of players. Change/overload this
 * function with caution as this can affect your network performance.
 *
 * @param Sender the player to check for mute status
 *
 * @return TRUE if this player is muted, FALSE otherwise
 */
native final function bool IsPlayerMuted(const out UniqueNetId Sender);


/** called on client during seamless level transitions to get the list of Actors that should be moved into the new level
 * PlayerControllers, Role < ROLE_Authority Actors, and any non-Actors that are inside an Actor that is in the list
 * (i.e. Object.Outer == Actor in the list)
 * are all autmoatically moved regardless of whether they're included here
 * only dynamic (!bStatic and !bNoDelete) actors in the PersistentLevel may be moved (this includes all actors spawned during gameplay)
 * this is called for both parts of the transition because actors might change while in the middle (e.g. players might join or leave the game)
 * @see also GameInfo::GetSeamlessTravelActorList() (the function that's called on servers)
 * @param bToEntry true if we are going from old level -> entry, false if we are going from entry -> new level
 * @param ActorList (out) list of actors to maintain
 */
event GetSeamlessTravelActorList(bool bToEntry, out array<Actor> ActorList)
{
	if (myHUD != None)
	{
		ActorList[ActorList.length] = myHUD;
		if (myHUD.Scoreboard != None)
		{
			ActorList[ActorList.length] = myHUD.Scoreboard;
		}
	}
}

/** tells the server to destroy the PC; primarily used by the internal netcode when the client has successfully received
 * a new PC replicated from the server to get rid of the old one
 */
reliable server final event ServerDestroy()
{
	Destroy();
}

/** called when seamless travelling and the specified PC is being replaced by this one
 * copy over data that should persist
 * (not called if PlayerControllerClass is the same for the from and to gametypes)
 */
function SeamlessTravelFrom(PlayerController OldPC)
{
	// copy PRI data
	OldPC.PlayerReplicationInfo.Reset();
	OldPC.PlayerReplicationInfo.CopyProperties(PlayerReplicationInfo);

	// mark the old PC as not a player, so it doesn't get Logout() etc when they player represented didn't really leave
	OldPC.bIsPlayer = false;
	//@fixme: need a way to replace PRIs that doesn't cause incorrect "player left the game"/"player entered the game" messages
	OldPC.PlayerReplicationInfo.Destroy();
	OldPC.PlayerReplicationInfo = None;
}

/**
 * Looks at the current game state and uses that to set the
 * rich presence strings
 *
 * Licensees should override this in their player controller derived class
 */
reliable client function ClientSetRichPresence();

/**
 * Returns the player controller associated with this net id
 *
 * @param PlayerNetId the id to search for
 *
 * @return the player controller if found, otherwise none
 */
native static function PlayerController GetPlayerControllerFromNetId(UniqueNetId PlayerNetId);

/**
 * Locally mutes a remote player
 *
 * @param PlayerNetId the remote player to mute
 */
reliable client event ClientMutePlayer(UniqueNetId PlayerNetId)
{
	local LocalPlayer LocPlayer;

	if (VoiceInterface != None)
	{
		// Use the local player to determine the controller id
		LocPlayer = LocalPlayer(Player);
		if (LocPlayer != None)
		{
			// Have the voice subsystem mute this player
			VoiceInterface.MuteRemoteTalker(LocPlayer.ControllerId,PlayerNetId);
		}
	}
}

/**
 * Locally unmutes a remote player
 *
 * @param PlayerNetId the remote player to unmute
 */
reliable client event ClientUnmutePlayer(UniqueNetId PlayerNetId)
{
	local LocalPlayer LocPlayer;

	if (VoiceInterface != None)
	{
		// Use the local player to determine the controller id
		LocPlayer = LocalPlayer(Player);
		if (LocPlayer != None)
		{
			// Have the voice subsystem restore voice for this player
			VoiceInterface.UnmuteRemoteTalker(LocPlayer.ControllerId,PlayerNetId);
		}
	}
}

/**
 * Mutes a remote player on the server and then tells the client to mute
 *
 * @param PlayerNetId the remote player to mute
 */
function GameplayMutePlayer(UniqueNetId PlayerNetId)
{
	// Don't add if already muted
	if (GameplayVoiceMuteList.Find('Uid',PlayerNetId.Uid) == INDEX_NONE)
	{
		GameplayVoiceMuteList[GameplayVoiceMuteList.Length] = PlayerNetId;
	}
	// Add to the filter list too, if missing
	if (VoicePacketFilter.Find('Uid',PlayerNetId.Uid) == INDEX_NONE)
	{
		VoicePacketFilter[VoicePacketFilter.Length] = PlayerNetId;
		// Now process on the client needed for splitscreen net play
		ClientMutePlayer(PlayerNetId);
	}
}

/**
 * Unmutes a remote player on the server and then tells the client to unmute
 *
 * @param PlayerNetId the remote player to unmute
 */
function GameplayUnmutePlayer(UniqueNetId PlayerNetId)
{
	local int RemoveIndex;
	local PlayerController Other;

	// Remove from the gameplay mute list
	RemoveIndex = GameplayVoiceMuteList.Find('Uid',PlayerNetId.Uid);
	if (RemoveIndex != INDEX_NONE)
	{
		GameplayVoiceMuteList.Remove(RemoveIndex,1);
	}
	// Find the muted player's player controller so it can be notified
	Other = GetPlayerControllerFromNetId(PlayerNetId);
	if (Other != None)
	{
		// If they were not explicitly muted
		if (VoiceMuteList.Find('Uid',PlayerNetId.Uid) == INDEX_NONE &&
			// and they did not explicitly mute us
			Other.VoiceMuteList.Find('Uid',PlayerReplicationInfo.UniqueId.Uid) == INDEX_NONE)
		{
			// It's safe to remove them from the filter list
			RemoveIndex = VoicePacketFilter.Find('Uid',PlayerNetId.Uid);
			if (RemoveIndex != INDEX_NONE)
			{
				VoicePacketFilter.Remove(RemoveIndex,1);
				// Now process on the client
				ClientUnmutePlayer(PlayerNetId);
			}
		}
	}
}

/**
 * Updates the server side information by adding to the mute list. Tells the
 * player controller that owns the specified net id to also mute this PC.
 *
 * @param PlayerNetId the remote player to mute
 */
reliable server event ServerMutePlayer(UniqueNetId PlayerNetId)
{
	local PlayerController Other;

	// Don't reprocess if they are already muted
	if (VoiceMuteList.Find('Uid',PlayerNetId.Uid) == INDEX_NONE)
	{
		VoiceMuteList[VoiceMuteList.Length] = PlayerNetId;
	}
	// Add them to the packet filter list if not already on it
	if (VoicePacketFilter.Find('Uid',PlayerNetId.Uid) == INDEX_NONE)
	{
		VoicePacketFilter[VoicePacketFilter.Length] = PlayerNetId;
	}
	ClientMutePlayer(PlayerNetId);
	// Find the muted player's player controller so it can be notified
	Other = GetPlayerControllerFromNetId(PlayerNetId);
	if (Other != None)
	{
		// Update their packet filter list too
		if (Other.VoicePacketFilter.Find('Uid',PlayerReplicationInfo.UniqueId.Uid) == INDEX_NONE)
		{
			Other.VoicePacketFilter[Other.VoicePacketFilter.Length] = PlayerReplicationInfo.UniqueId;
			// Tell the other PC to mute this one
			Other.ClientMutePlayer(PlayerReplicationInfo.UniqueId);
		}
	}
}

/**
 * Updates the server side information by removing from the mute list. Tells the
 * player controller that owns the specified net id to also unmute this PC.
 *
 * @param PlayerNetId the remote player to unmute
 */
reliable server event ServerUnmutePlayer(UniqueNetId PlayerNetId)
{
	local PlayerController Other;
	local int RemoveIndex;

	RemoveIndex = VoiceMuteList.Find('Uid',PlayerNetId.Uid);
	// If the player was found, remove them from our explicit list
	if (RemoveIndex != INDEX_NONE)
	{
		VoiceMuteList.Remove(RemoveIndex,1);
	}
	// Make sure this player isn't muted for gameplay reasons
	if (GameplayVoiceMuteList.Find('Uid',PlayerNetId.Uid) == INDEX_NONE)
	{
		ClientUnmutePlayer(PlayerNetId);
	}
	// Find the muted player's player controller so it can be notified
	Other = GetPlayerControllerFromNetId(PlayerNetId);
	if (Other != None)
	{
		// If the other player doesn't have this player muted
		if (Other.VoiceMuteList.Find('Uid',PlayerReplicationInfo.UniqueId.Uid) == INDEX_NONE &&
			Other.GameplayVoiceMuteList.Find('Uid',PlayerReplicationInfo.UniqueId.Uid) == INDEX_NONE)
		{
			// Remove them from the packet filter list
			RemoveIndex = VoicePacketFilter.Find('Uid',PlayerNetId.Uid);
			if (RemoveIndex != INDEX_NONE)
			{
				VoicePacketFilter.Remove(RemoveIndex,1);
			}
			// If found, remove so packets flow to that client too
			RemoveIndex = Other.VoicePacketFilter.Find('Uid',PlayerReplicationInfo.UniqueId.Uid);
			if (RemoveIndex != INDEX_NONE)
			{
				Other.VoicePacketFilter.Remove(RemoveIndex,1);
				// Tell the other PC to unmute this one
				Other.ClientUnmutePlayer(PlayerReplicationInfo.UniqueId);
			}
		}
	}
}

/** notification when a matinee director track starts or stops controlling the ViewTarget of this PlayerController */
event NotifyDirectorControl(bool bNowControlling);

/**
 * This will turn the subtitles on or off depending on the value of bValue
 *
 * @param bValue  to show or not to show
 **/
native simulated function SetShowSubtitles( bool bValue );

/**
 * This will turn return whether the subtitles are on or off
 *
 **/
native simulated function bool IsShowingSubtitles();

/** called when the client loses connection with the server
 * @return true if the function will handle the situation, false to automatically be returned to the default map
 */
event bool NotifyConnectionLost()
{
	if (WorldInfo.Game != None)
	{
		// Mark the server as having a problem
		WorldInfo.Game.bHasNetworkError = true;
	}
	return false;
}

reliable simulated client event ClientWasKicked();

/**
 * Tells the client to register with arbitration. The client must notify the
 * server once that is complete or it will be kicked from the match
 */
reliable client function ClientRegisterForArbitration()
{
	if (OnlineSub != None && OnlineSub.GameInterface != None)
	{
		OnlineSub.GameInterface.AddArbitrationRegistrationCompleteDelegate(OnArbitrationRegisterComplete);
		// Kick off async arbitration registration
		OnlineSub.GameInterface.RegisterForArbitration();
	}
	else
	{
		// Fake completion with the server for flow testing
		ServerRegisteredForArbitration(true);
	}
}

/**
 * Delegate that is notified when registration is complete. Forwards the call
 * to the server so that it can finalize processing
 *
 * @param bWasSuccessful whether registration worked or not
 */
function OnArbitrationRegisterComplete(bool bWasSuccessful)
{
	// Clear the delegate since it isn't needed
	OnlineSub.GameInterface.ClearArbitrationRegistrationCompleteDelegate(OnArbitrationRegisterComplete);
	// Tell the server that we completed registration
	ServerRegisteredForArbitration(bWasSuccessful);
}

/**
 * Notifies the server that the arbitration registration is complete
 *
 * @param bWasSuccessful whether the registration with arbitration worked
 */
reliable server function ServerRegisteredForArbitration(bool bWasSuccessful)
{
	// Tell the game info this PC is done and whether it worked or not
	WorldInfo.Game.ProcessClientRegistrationCompletion(self,bWasSuccessful);
}

/**
 * Called on the client to have them commit their arbitration results
 */
reliable client function ClientWriteArbitrationEndGameData(class<OnlineStatsWrite> OnlineStatsWriteClass)
{
	if (OnlineSub != None && OnlineSub.GameInterface != None)
	{
		// Kick off async arbitration finalization
		ClientWriteOnlinePlayerScores();
		// Force the flush of the stats
		OnlineSub.GameInterface.EndOnlineGame();
	}
	// It would be more efficient to return the value but eventually this code
	// will be made async, so leave here for now
	ServerWritenArbitrationEndGameData();
}

/**
 * Tells the server that the arbitration results were written
 */
reliable server function ServerWritenArbitrationEndGameData()
{
	// Tell the gameinfo that we've completed the write
	WorldInfo.Game.ProcessClientDataWriteCompletion(self);
}

/**
 * Delegate called when the user accepts a game invite externally. This allows
 * the game code a chance to clean up before joining the game via
 * AcceptGameInvite() call.
 *
 * NOTE: There must be space for all signed in players to join the game. All
 * players must also have permission to play online too.
 *
 * @param GameInviteSettings the settings for the game we're to join
 */
function OnGameInviteAccepted(OnlineGameSettings GameInviteSettings)
{
	if (OnlineSub != None && OnlineSub.GameInterface != None)
	{
		if (GameInviteSettings != None)
		{
			// Make sure the new game has space
			if (InviteHasEnoughSpace(GameInviteSettings))
			{
				// Make sure everyone logged in can play online
				if (CanAllPlayersPlayOnline())
				{
					if (WorldInfo.NetMode != NM_Standalone)
					{
						WorldInfo.GRI.bNeedsOnlineCleanup = false;
						// Write arbitration data, if required
						if (OnlineSub.GameInterface.GetGameSettings().bUsesArbitration)
						{
							// Write out our version of the scoring before leaving
							ClientWriteOnlinePlayerScores();
						}
						// Set the end delegate, where we'll destroy the game and then join
						OnlineSub.GameInterface.AddEndOnlineGameCompleteDelegate(OnEndForInviteComplete);
						// Force the flush of the stats
						OnlineSub.GameInterface.EndOnlineGame();
					}
					else
					{
						// Set the delegate for notification of the join completing
						OnlineSub.GameInterface.AddJoinOnlineGameCompleteDelegate(OnInviteJoinComplete);
						// We can immediately accept since there is no online game
						OnlineSub.GameInterface.AcceptGameInvite(LocalPlayer(Player).ControllerId);
					}
				}
				else
				{
					// Display an error message
					NotifyNotAllPlayersCanJoinInvite();
				}
			}
			else
			{
				// Display an error message
				NotifyNotEnoughSpaceInInvite();
			}
		}
		else
		{
			// Display an error message
			NotifyInviteFailed();
		}
	}
}

/**
 * Counts the number of local players to verify there is enough space
 *
 * @return true if there is sufficient space, false if not
 */
function bool InviteHasEnoughSpace(OnlineGameSettings InviteSettings)
{
	local int NumLocalPlayers;
	local PlayerController PC;

	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		NumLocalPlayers++;
	}
	// Invites consume private connections
	return (InviteSettings.NumOpenPrivateConnections + InviteSettings.NumOpenPublicConnections) >= NumLocalPlayers;
}

/**
 * Validates that each local player can play online
 *
 * @return true if there is sufficient space, false if not
 */
function bool CanAllPlayersPlayOnline()
{
	local PlayerController PC;
	local LocalPlayer LocPlayer;

	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		LocPlayer = LocalPlayer(PC.Player);
		if (LocPlayer != None)
		{
			// Check their login status and permissions
			if (OnlineSub.PlayerInterface.GetLoginStatus(LocPlayer.ControllerId) != LS_LoggedIn ||
				OnlineSub.PlayerInterface.CanPlayOnline(LocPlayer.ControllerId) == FPL_Disabled)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

/**
 * Clears all of the invite delegates
 */
function ClearInviteDelegates()
{
	// Clear the end delegate
	OnlineSub.GameInterface.ClearEndOnlineGameCompleteDelegate(OnEndForInviteComplete);
	// Clear the destroy delegate
	OnlineSub.GameInterface.ClearDestroyOnlineGameCompleteDelegate(OnDestroyForInviteComplete);
	// Set the delegate for notification of the join completing
	OnlineSub.GameInterface.ClearJoinOnlineGameCompleteDelegate(OnInviteJoinComplete);
}

/**
 * Delegate called once the destroy of an online game before accepting an invite
 * is complete. From here, the game invite can be accepted
 */
function OnEndForInviteComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// Set the destroy delegate so we can know when that is complete
		OnlineSub.GameInterface.AddDestroyOnlineGameCompleteDelegate(OnDestroyForInviteComplete);
		// Now we can destroy the game
		OnlineSub.GameInterface.DestroyOnlineGame();
	}
	else
	{
		// Do some error handling
		NotifyInviteFailed();
	}
}

/**
 * Delegate called once the destroy of an online game before accepting an invite
 * is complete. From here, the game invite can be accepted
 */
function OnDestroyForInviteComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// Set the delegate for notification of the join completing
		OnlineSub.GameInterface.AddJoinOnlineGameCompleteDelegate(OnInviteJoinComplete);
		// This will have us join async
		OnlineSub.GameInterface.AcceptGameInvite(LocalPlayer(Player).ControllerId);
	}
	else
	{
		// Do some error handling
		NotifyInviteFailed();
	}
}

/**
 * Once the join completes, use the platform specific connection information
 * to connect to it
 *
 * @param bWasSuccessful whether the join worked or not
 */
function OnInviteJoinComplete(bool bWasSuccessful)
{
	local string URL;

	if (bWasSuccessful)
	{
		if (OnlineSub != None && OnlineSub.GameInterface != None)
		{
			// Get the platform specific information
			if (OnlineSub.GameInterface.GetResolvedConnectString(URL))
			{
				`Log("Resulting url is ("$URL$")");
				// Open a network connection to it
				ConsoleCommand("open "$URL);
			}
		}
	}
	else
	{
		// Do some error handling
		NotifyInviteFailed();
	}
	ClearInviteDelegates();
}

/** Override to display a message to the user */
function NotifyInviteFailed()
{
	`Log("Invite handling failed");
	ClearInviteDelegates();
}

/** Override to display a message to the user */
function NotifyNotAllPlayersCanJoinInvite()
{
	`Log("Not all local players have permission to join the game invite");
}

/** Override to display a message to the user */
function NotifyNotEnoughSpaceInInvite()
{
	`Log("Not enough space for all local players in the game invite");
}

/**
 * Called when an arbitrated match has ended and we need to disconnect
 */
reliable client function ClientArbitratedMatchEnded()
{
	ConsoleCommand("Disconnect");
}

/**
 * Writes the scores for all active players. Override this in your
 * playercontroller class to provide custom scoring
 */
reliable client function ClientWriteOnlinePlayerScores()
{
	local GameReplicationInfo GRI;
	local int Index;
	local array<OnlinePlayerScore> PlayerScores;
	local bool bIsTeamGame;

	GRI = WorldInfo.GRI;
	if (GRI != None && OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		// Look at the default object of the gameinfo to determine if this is a
		// team game or not
		bIsTeamGame = GRI.GameClass != None ? GRI.GameClass.default.bTeamGame : FALSE;
		// Allocate an entry for all players
		PlayerScores.Length = GRI.PRIArray.Length;
		// Iterate through the players building their score data
		for (Index = 0; Index < GRI.PRIArray.Length; Index++)
		{
			// Build the skill data for this player
			PlayerScores[Index].PlayerId = GRI.PRIArray[Index].UniqueId;
			if (bIsTeamGame)
			{
				PlayerScores[Index].TeamId = GRI.PRIArray[Index].Team.TeamIndex;
				PlayerScores[Index].Score = GRI.PRIArray[Index].Team.Score;
			}
			else
			{
				PlayerScores[Index].TeamId = Index;
				PlayerScores[Index].Score = GRI.PRIArray[Index].Score;
			}
		}
		// Now write out the scores
		OnlineSub.StatsInterface.WriteOnlinePlayerScores(PlayerScores);
	}
}

/**
 * Sets the host's net id for handling dropped arbitrated matches
 *
 * @param InHostId the host's unique net id to report the drop for
 */
reliable client function ClientSetHostUniqueId(UniqueNetId InHostId);

/** Tells this client that it should not send voice data over the network */
reliable client function ClientStopNetworkedVoice()
{
	local LocalPlayer LocPlayer;

	LocPlayer = LocalPlayer(Player);
	if (LocPlayer != None && OnlineSub != None && OnlineSub.VoiceInterface != None)
	{
		OnlineSub.VoiceInterface.StopNetworkedVoice(LocPlayer.ControllerId);
	}
}

/** Tells this client that it should send voice data over the network */
reliable client function ClientStartNetworkedVoice()
{
	local LocalPlayer LocPlayer;

	LocPlayer = LocalPlayer(Player);
	if (LocPlayer != None && OnlineSub != None && OnlineSub.VoiceInterface != None)
	{
		OnlineSub.VoiceInterface.StartNetworkedVoice(LocPlayer.ControllerId);
	}
}

/** called to update the client on what Kismet controlled music is going on */
reliable server function ServerSendMusicInfo()
{
	if ( WorldInfo.LastMusicAction != None && WorldInfo.LastMusicAction.CurrPlayingTrack != None &&
		WorldInfo.LastMusicAction.CurrPlayingTrack.FadeOutStopTime < 0.f && WorldInfo.LastMusicTrack.TheSoundCue != None )
	{
		ClientCrossFadeMusicTrack_PlayTrack(WorldInfo.LastMusicAction, WorldInfo.LastMusicTrack);
	}
}

/** called to play immediately to a Kismet a new music track on the client */
reliable client event ClientCrossFadeMusicTrack_PlayTrack(SeqAct_CrossFadeMusicTracks MusicAction, MusicTrackStruct MusicTrack)
{
	if (MusicAction != None )
	{
		MusicAction.ClientSideCrossFadeTrackImmediately(MusicTrack);
	}
	else if (Role < ROLE_Authority)
	{
		// ask the server to resend the info, since it must not have been replicated properly,
		// as there's no point in sending the function call with None
		// if this really is correct for some reason, then ServerSendMusicInfo() should do nothing
		ServerSendMusicInfo();
	}
}

/** called to fade out a Kismet played music track on the client */
reliable client event ClientFadeOutMusicTrack(SeqAct_CrossFadeMusicTracks MusicAction, float FadeOutTime, float FadeOutVolumeLevel)
{
	if (MusicAction != None && MusicAction.CurrPlayingTrack != None)
	{
		MusicAction.CurrPlayingTrack.FadeOut(FadeOutTime, FadeOutVolumeLevel);
		MusicAction.CurrTrackType = '';
	}
}

/** called to adjust the volume of a Kismet played music track on the client */
reliable client event ClientAdjustMusicTrackVolume(SeqAct_CrossFadeMusicTracks MusicAction, float AdjustVolumeDuration, float AdjustVolumeLevel)
{
	if (MusicAction != None && MusicAction.CurrPlayingTrack != None)
	{
		MusicAction.CurrPlayingTrack.AdjustVolume(AdjustVolumeDuration, AdjustVolumeLevel);
	}
}

simulated function OnDestroy(SeqAct_Destroy Action)
{
	// Kismet is not allowed to disconnect players from the game
	Action.ScriptLog("Cannot use Destroy action on players");
}

/** console control commands, useful when remote debugging so you can't touch the console the normal way */
exec function ConsoleKey(name Key)
{
	if (LocalPlayer(Player) != None)
	{
		LocalPlayer(Player).ViewportClient.ViewportConsole.InputKey(0, Key, IE_Pressed);
	}
}
exec function SendToConsole(string Command)
{
	if (LocalPlayer(Player) != None)
	{
		LocalPlayer(Player).ViewportClient.ViewportConsole.ConsoleCommand(Command);
	}
}

/**
 * Iterate through list of debug text and draw it over the associated actors in world space.
 * Also handles culling null entries, and reducing the duration for timed debug text.
 */
final simulated function DrawDebugTextList(Canvas Canvas, float RenderDelta)
{
	local vector CameraLoc, ScreenLoc, Offset;
	local rotator CameraRot;
	local int Idx;
	if (DebugTextList.Length > 0)
	{
		GetPlayerViewPoint(CameraLoc,CameraRot);
		Canvas.SetDrawColor(255,255,255);
		Canvas.Font = class'Engine'.static.GetSmallFont();
		for (Idx = 0; Idx < DebugTextList.Length; Idx++)
		{
			if (DebugTextList[Idx].SrcActor == None)
			{
				DebugTextList.Remove(Idx--,1);
				continue;
			}
			if (DebugTextList[Idx].TimeRemaining != -1.f)
			{
				DebugTextList[Idx].TimeRemaining -= RenderDelta;
				if (DebugTextList[Idx].TimeRemaining <= 0.f)
				{
					DebugTextList.Remove(Idx--,1);
					continue;
				}
			}
			Offset = VLerp(DebugTextList[Idx].SrcActorOffset,DebugTextList[Idx].SrcActorDesiredOffset,1.f - (DebugTextList[Idx].TimeRemaining/DebugTextList[Idx].Duration));
			ScreenLoc = Canvas.Project(DebugTextList[Idx].SrcActor.Location + (Offset >> CameraRot));
			Canvas.SetPos(ScreenLoc.X,ScreenLoc.Y);
			Canvas.DrawColor = DebugTextList[Idx].TextColor;
			Canvas.DrawText(DebugTextList[Idx].DebugText);
		}
	}
}

/**
 * Add debug text for a specific actor to be displayed via DrawDebugTextList().  If the debug text is invalid then it will
 * attempt to remove any previous entries via RemoveDebugText().
 */
final reliable client event AddDebugText(string DebugText, optional Actor SrcActor, optional float Duration = -1.f, optional vector Offset, optional vector DesiredOffset, optional color TextColor, optional bool bSkipOverwriteCheck)
{
	local int Idx;
	// set a default color
	if (TextColor.R == 0 && TextColor.G == 0 && TextColor.B == 0 && TextColor.A == 0)
	{
		TextColor.R = 255;
		TextColor.G = 255;
		TextColor.B = 255;
		TextColor.A = 255;
	}
	// and a default source actor of our pawn
	if (SrcActor == None)
	{
		SrcActor = Pawn;
	}
	if (SrcActor != None)
	{
		if (Len(DebugText) == 0)
		{
			RemoveDebugText(SrcActor);
		}
		else
		{
			`log("Adding debug text:"@DebugText@"for actor:");
			// search for an existing entry
			if (!bSkipOverwriteCheck)
			{
				Idx = DebugTextList.Find('SrcActor',SrcActor);
				if (Idx == INDEX_NONE)
				{
					// manually grow the array one struct element
					Idx = DebugTextList.Length;
					DebugTextList.Length = Idx + 1;
				}
			}
			else
			{
				Idx = DebugTextList.Length;
				DebugTextList.Length = Idx + 1;
			}
			// assign the new text and actor
			DebugTextList[Idx].SrcActor = SrcActor;
			DebugTextList[Idx].SrcActorOffset = Offset;
			DebugTextList[Idx].SrcActorDesiredOffset = DesiredOffset;
			DebugTextList[Idx].DebugText = DebugText;
			DebugTextList[Idx].TimeRemaining = Duration;
			DebugTextList[Idx].Duration = Duration;
			DebugTextList[Idx].TextColor = TextColor;
		}
	}
}

/**
 * Remove debug text for the specific actor.
 */
final reliable client event RemoveDebugText(Actor SrcActor)
{
	local int Idx;
	Idx = DebugTextList.Find('SrcActor',SrcActor);
	if (Idx != INDEX_NONE)
	{
		DebugTextList.Remove(Idx,1);
	}
}

/**
 *  Switch controller to debug camera without locking gameplay and with locking
 *  local player controller input
 */
function EnableDebugCamera()
{
    local Player P;
    local vector eyeLoc;
    local rotator eyeRot;

    P = self.Player;
    if( P!= none && Pawn != none && self.IsLocalPlayerController() )
    {
	    if( DebugCameraControllerRef==None ) {
	        DebugCameraControllerRef = spawn(DebugCameraControllerClass);
	}
	DebugCameraControllerRef.OryginalPlayer = P;
	DebugCameraControllerRef.OryginalControllerRef = self;

		GetPlayerViewPoint(eyeLoc,eyeRot);
	    DebugCameraControllerRef.SetLocation(eyeLoc);
	    DebugCameraControllerRef.SetRotation(eyeRot);
	    DebugCameraControllerRef.PlayerCamera.SetFOV( GetFOVAngle() );
	DebugCameraControllerRef.PlayerCamera.UpdateCamera(0.0);

	P.SwitchController( DebugCameraControllerRef );
	DebugCameraControllerRef.OnActivate( self );
    }
}

/**
 * Registers the host's stat guid with the online subsystem
 *
 * @param StatGuid the stat guid to register
 */
reliable client function ClientRegisterHostStatGuid(string StatGuid)
{
	if (OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		OnlineSub.StatsInterface.AddRegisterHostStatGuidCompleteDelegate(OnRegisterHostStatGuidComplete);
		OnlineSub.StatsInterface.RegisterHostStatGuid(StatGuid);
	}
}

/**
 * Called once the host registration has completed. Sends the host this clients stat guid
 *
 * @param bWasSuccessful true if the registration worked, false otherwise
 */
function OnRegisterHostStatGuidComplete(bool bWasSuccessful)
{
	local string StatGuid;

	OnlineSub.StatsInterface.ClearRegisterHostStatGuidCompleteDelegateDelegate(OnRegisterHostStatGuidComplete);
	if (bWasSuccessful)
	{
		StatGuid = OnlineSub.StatsInterface.GetClientStatGuid();
		// Report the client stat guid back to the server
		ServerRegisterClientStatGuid(StatGuid);
	}
}

/**
 * Registers the client's stat guid with the online subsystem
 *
 * @param StatGuid the stat guid to register
 */
reliable server function ServerRegisterClientStatGuid(string StatGuid)
{
	if (OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		OnlineSub.StatsInterface.RegisterStatGuid(PlayerReplicationInfo.UniqueId,StatGuid);
	}
}

/** Checks for parental controls blocking user created content */
function bool CanViewUserCreatedContent()
{
	local LocalPlayer LocPlayer;

	LocPlayer = LocalPlayer(Player);
	if (LocPlayer != None && OnlineSub != None && OnlineSub.PlayerInterface != None)
	{
		return OnlineSub.PlayerInterface.CanDownloadUserContent(LocPlayer.ControllerId) == FPL_Enabled;
	}
	return true;
}

defaultproperties
{
	// The PlayerController is currently required to have collision as there is code that uses the collision
	// for moving the spectator camera around.  Without collision certain assumptions will not hold (e.g. the
	// spectator does not have a pawn so the various game code looks at ALL pawns.  Having a new pawnType will
	// require changes to that code.
	// Removing the collision from the controller and using the controller - pawn mechanic will eventually be coded
	// for spectator-esque functionality
	Begin Object Class=CylinderComponent Name=CollisionCylinder
		CollisionRadius=+22.000000
		CollisionHeight=+22.000000
	End Object
	CollisionComponent=CollisionCylinder
	CylinderComponent=CollisionCylinder
	Components.Add(CollisionCylinder)

    DebugCameraControllerClass=class'DebugCameraController';

	FOVAngle=85.000
	bStasis=False
	NetPriority=3
	bIsPlayer=true
	bCanDoSpecial=true
	Physics=PHYS_None
	PlayerOwnerDataStoreClass=class'Engine.PlayerOwnerDataStore'
	CheatClass=class'Engine.CheatManager'
	InputClass=class'Engine.PlayerInput'
	ProgressTimeOut=8.0

	CameraClass=class'Camera'

	MaxResponseTime=0.125
	ClientCap=0
	LastSpeedHackLog=-100.0
	SavedMoveClass=class'SavedMove'

	DesiredFOV=85.000000
	DefaultFOV=85.000000
	LODDistanceFactor=1.0

	bCinemaDisableInputMove=false
	bCinemaDisableInputLook=false

	bIsUsingStreamingVolumes=TRUE
	SpectatorCameraSpeed=600.0
	MinRespawnDelay=1.0
}
