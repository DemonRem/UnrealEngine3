//=============================================================================
// GameInfo.
//
// The GameInfo defines the game being played: the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.  While the
// GameInfo class is the public interface, much of this functionality is delegated
// to several classes to allow easy modification of specific game components.  These
// classes include GameInfo, AccessControl, Mutator, BroadcastHandler, and GameRules.
// A GameInfo actor is instantiated when the level is initialized for gameplay (in
// C++ UGameEngine::LoadMap() ).  The class of this GameInfo actor is determined by
// (in order) either the URL ?game=xxx, or the
// DefaultGame entry in the game's .ini file (in the Engine.Engine section), unless
// its a network game in which case the DefaultServerGame entry is used.
// The GameType used can be overridden in the GameInfo script event SetGameType(), called
// on the game class picked by the above process.
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class GameInfo extends Info
	config(Game)
	native;

//-----------------------------------------------------------------------------
// Variables.

var bool				      bRestartLevel;			// Level should be restarted when player dies
var bool				      bPauseable;				// Whether the game is pauseable.
var bool				      bTeamGame;				// This is a team game.
var	bool					  bGameEnded;				// set when game ends
var	bool					  bOverTime;
var bool					  bDelayedStart;
var bool					  bWaitingToStartMatch;
var globalconfig bool		  bChangeLevels;
var		bool				  bAlreadyChanged;
var bool						bLoggingGame;           // Does this gametype log?
var globalconfig bool			bAdminCanPause;
var bool						bGameRestarted;
var bool						bAllowVehicles;			// whether vehicles are allowed in this game type (hint for networking)
var bool						bLevelChange;			// level transition in progress
var globalconfig	bool		bKickLiveIdlers;		// if true, even playercontrollers with pawns can be kicked for idling

/** Whether this match is going to use arbitration or not */
var bool bUsingArbitration;

/**
 * Whether the arbitrated handshaking has occured or not.
 *
 * NOTE: The code will reject new connections once handshaking has started
 */
var bool bHasArbitratedHandshakeBegun;

/** Whether the arbitrated handshaking has occured or not. */
var bool bNeedsEndGameHandshake;

/** Whether the arbitrated handshaking has completed or not. */
var bool bIsEndGameHandshakeComplete;

/** Used to indicate when an arbitrated match has started its end sequence */
var bool bHasEndGameHandshakeBegun;

/** Whether the game is currently in automated perf test mode. */
var bool	bAutomatedPerfTesting;
/** Amount of time remaining before match ends -- used for auto performance test shutdown */
var int		AutomatedPerfRemainingTime;


/**
 * List of player controllers we're awaiting handshakes with
 *
 * NOTE: Any PC in this list that does not complete the handshake within
 * ArbitrationHandshakeTimeout will be kicked from the match
 */
var array<PlayerController> PendingArbitrationPCs;

/**
 * Holds the list of players that passed handshaking and require finalization
 * of arbitration data written to the online subsystem
 */
var array<PlayerController> ArbitrationPCs;

/** Amount of time a client can take for arbitration handshaking before being kicked */
var globalconfig float ArbitrationHandshakeTimeout;

var globalconfig float        GameDifficulty;
var	  globalconfig int		  GoreLevel;				// 0=Normal, increasing values=less gore
var   float					  GameSpeed;				// Scale applied to game rate.

var   class<Pawn>			  DefaultPawnClass;

// user interface
var   class<Scoreboard>       ScoreBoardType;           // Type of class<Menu> to use for scoreboards.
var	  class<HUD>			  HUDType;					// HUD class this game uses.

var   globalconfig int	      MaxSpectators;			// Maximum number of spectators allowed by this server.
var	  int					  MaxSpectatorsAllowed;		// Maximum number of spectators ever allowed (MaxSpectators is clamped to this in initgame()
var	  int					  NumSpectators;			// Current number of spectators.
var   globalconfig int		  MaxPlayers;				// Maximum number of players allowed by this server.
var	  int					  MaxPlayersAllowed;		// Maximum number of players ever allowed (MaxSPlayers is clamped to this in initgame()
var   int					  NumPlayers;				// number of human players
var	  int					  NumBots;					// number of non-human players (AI controlled but participating as a player)
var   int					  CurrentID;				// used to assign unique PlayerIDs to each PlayerReplicationInfo
var localized string	      DefaultPlayerName;
var localized string	      GameName;
var float					  FearCostFallOff;			// how fast the FearCost in NavigationPoints falls off
var	bool					  bDoFearCostFallOff;		// If true, FearCost will fall off over time in NavigationPoints.  Reset to false once all reach FearCosts reach 0.

var config int                GoalScore;                // what score is needed to end the match
var config int                MaxLives;	                // max number of lives for match, unless overruled by level's GameDetails
var config int                TimeLimit;                // time limit in minutes

// Message classes.
var class<LocalMessage>		  DeathMessageClass;
var class<GameMessage>		  GameMessageClass;

//-------------------------------------
// GameInfo components
var Mutator BaseMutator;				// linked list of Mutators (for modifying actors as they enter the game)
var class<AccessControl> AccessControlClass;
var private AccessControl AccessControl;		// AccessControl controls whether players can enter and/or become admins
var private GameRules GameRulesModifiers;		// linked list of modifier classes which affect game rules
var class<BroadcastHandler> BroadcastHandlerClass;
var private BroadcastHandler BroadcastHandler;	// handles message (text and localized) broadcasts

var class<PlayerController> PlayerControllerClass;	// type of player controller to spawn for players logging in
var class<PlayerReplicationInfo> 		PlayerReplicationInfoClass;

/** Name of DialogueManager class */
var String			DialogueManagerClass;
/** Pointer to Manager */
var DialogueManager DialogueManager;

// ReplicationInfo
var() class<GameReplicationInfo> GameReplicationInfoClass;
var GameReplicationInfo GameReplicationInfo;

var globalconfig float MaxIdleTime;		// maximum time players are allowed to idle before being kicked

// speed hack detection
var globalconfig	float					MaxTimeMargin;
var globalconfig	float					TimeMarginSlack;
var globalconfig	float					MinTimeMargin;

var		array<PlayerReplicationInfo> InactivePRIArray;	/** PRIs of players who have left game (saved in case they reconnect) */

/** The list of delegates to check before unpausing a game */
var array<delegate<CanUnpause> > Pausers;

/** Cached online subsystem variable */
var OnlineSubsystem OnlineSub;

/** Cached online game interface variable */
var OnlineGameInterface GameInterface;

/** Cached online game settings object */
var OnlineGameSettings GameSettings;

/** Class sent to clients to use to create and hold their stats */
var class<OnlineStatsWrite> OnlineStatsWriteClass;

/** perform map travels using SeamlessTravel() which loads in the background and doesn't disconnect clients
 * @see WorldInfo::SeamlessTravel()
 */
var bool bUseSeamlessTravel;

/** Base copy of cover changes that need to be replicated to clients on join */
var protected CoverReplicator CoverReplicatorBase;

/** Tracks whether the server can travel due to a critical network error or not */
var bool bHasNetworkError;

/** Whether this game type requires voice to be push to talk or not */
var const bool bRequiresPushToTalk;

/** The class to use when registering dedicated servers with the online service */
var const class<OnlineGameSettings> OnlineGameSettingsClass;

/** The options to apply for dedicated server when it starts to register */
var string ServerOptions;

cpptext
{
	/** called on the default object of the class specified by DefaultGame in the [Engine.GameInfo] section of Game.ini
	 * whenever worlds are saved.
	 * Gives the game a chance to add supported gametypes to the WorldInfo's GameTypesSupportedOnThisMap array
	 * (used for console cooking)
	 * @param Info: the WorldInfo of the world being saved
	 */
	virtual void AddSupportedGameTypes(AWorldInfo* Info, const TCHAR* WorldFilename) const
	{
	}
}

//------------------------------------------------------------------------------
// Engine notifications.

event PreBeginPlay()
{
	if (GameInterface != None)
	{
		// Grab the current game settings object out
		GameSettings = GameInterface.GetGameSettings();
		if (GameSettings != None)
		{
			// Check for an arbitrated match
			bUsingArbitration = GameSettings.bUsesArbitration;
		}
	}

	SetGameSpeed(GameSpeed);
	GameReplicationInfo = Spawn(GameReplicationInfoClass);
	WorldInfo.GRI = GameReplicationInfo;
	// Send over whether this is an arbitrated match or not
	GameReplicationInfo.bIsArbitrated = bUsingArbitration;

	InitGameReplicationInfo();

	if( DialogueManagerClass != "" )
	{
		DialogueManager = Spawn( class<DialogueManager>(DynamicLoadObject( DialogueManagerClass, class'Class' )) );
	}
}

function string FindPlayerByID( int PlayerID )
{
    local PlayerReplicationInfo PRI;

	PRI = GameReplicationInfo.FindPlayerByID(PlayerID);
	if ( PRI != None )
		return PRI.PlayerName;
    return "";
}

static function bool UseLowGore()
{
	return ( Default.GoreLevel > 0 );
}

function CoverReplicator GetCoverReplicator()
{
	if (CoverReplicatorBase == None && WorldInfo.NetMode != NM_Standalone)
	{
		CoverReplicatorBase = Spawn(class'CoverReplicator');
	}
	return CoverReplicatorBase;
}

event PostBeginPlay()
{
	if ( MaxIdleTime > 0 )
	{
		MaxIdleTime = FMax(MaxIdleTime, 20);
	}
}

/* Reset() - reset actor to initial state - used when restarting level without reloading.
	@note: GameInfo::Reset() called after all other actors have been reset */
function Reset()
{
	super.Reset();

	bGameEnded = false;
	bOverTime = false;
	InitGameReplicationInfo();
}

/** @return true if ActorToReset should have Reset() called on it while restarting the game, false if the GameInfo will manually reset it
	or if the actor does not need to be reset
*/
function bool ShouldReset(Actor ActorToReset)
{
	return true;
}

/** Resets level by calling Reset() on all actors */
function ResetLevel()
{
	local Controller C;
	local Actor A;
	local Sequence GameSeq;
	local array<SequenceObject> AllSeqEvents;
	local int i;

	`Log("Reset" @ self);
	// Reset ALL controllers first
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if ( C.PlayerReplicationInfo == None || !C.PlayerReplicationInfo.bOnlySpectator )
		{
			if ( PlayerController(C) != None )
				PlayerController(C).ClientReset();
			C.Reset();
		}
	}

	// Reset all actors (except controllers, the GameInfo, and any other actors specified by ShouldReset())
	foreach AllActors(class'Actor', A)
	{
		if (A != self && !A.IsA('Controller') && ShouldReset(A))
		{
			A.Reset();
		}
	}

	// reset the GameInfo
	Reset();

	// reset Kismet and activate any Level Reset events
	GameSeq = WorldInfo.GetGameSequence();
	if (GameSeq != None)
	{
		// reset the game sequence
		GameSeq.Reset();

		// find any Level Reset events that exist
		GameSeq.FindSeqObjectsByClass(class'SeqEvent_LevelReset', true, AllSeqEvents);

		// activate them
		for (i = 0; i < AllSeqEvents.Length; i++)
		{
			SeqEvent_LevelReset(AllSeqEvents[i]).CheckActivate(WorldInfo, None);
		}
	}
}


event Timer()
{
	BroadcastHandler.UpdateSentText();

	// Update navigation point fear cost fall off.
	if ( bDoFearCostFallOff )
	{
		DoNavFearCostFallOff();
	}

	if( bAutomatedPerfTesting && AutomatedPerfRemainingTime > 0 )
	{
		AutomatedPerfRemainingTime--;
		if( AutomatedPerfRemainingTime <= 0 )
		{
			// Exit at the end of the match if automated perf testing is enabled.
			ConsoleCommand("EXIT");
		}
	}
}

/** Update navigation point fear cost fall off. */
final native function DoNavFearCostFallOff();

/** notification when a NavigationPoint becomes blocked or unblocked */
function NotifyNavigationChanged(NavigationPoint N);

// Called when game shutsdown.
event GameEnding()
{
	EndLogging("serverquit");
}

/* KickIdler() called if
		if ( (Pawn != None) || (PlayerReplicationInfo.bOnlySpectator && (ViewTarget != self))
			|| (WorldInfo.Pauser != None) || WorldInfo.Game.bWaitingToStartMatch || WorldInfo.Game.bGameEnded )
		{
			LastActiveTime = WorldInfo.TimeSeconds;
		}
		else if ( (WorldInfo.Game.MaxIdleTime > 0) && (WorldInfo.TimeSeconds - LastActiveTime > WorldInfo.Game.MaxIdleTime) )
			KickIdler(self);
*/
event KickIdler(PlayerController PC)
{
	`log("Kicking idle player "$PC.PlayerReplicationInfo.PlayerName);
	AccessControl.KickPlayer(PC, AccessControl.IdleKickReason);
}

//------------------------------------------------------------------------------
// Replication

function InitGameReplicationInfo()
{
	GameReplicationInfo.GameClass = Class;
    GameReplicationInfo.MaxLives = MaxLives;
}

native function string GetNetworkNumber();

function int GetNumPlayers()
{
	return NumPlayers;
}

//------------------------------------------------------------------------------
// Misc.

// Return the server's port number.
function int GetServerPort()
{
	local string S;
	local int i;

	// Figure out the server's port.
	S = WorldInfo.GetAddressURL();
	i = InStr( S, ":" );
	assert(i>=0);
	return int(Mid(S,i+1));
}

/**
 * Default delegate that provides an implementation for those that don't have
 * special needs other than a toggle
 */
delegate bool CanUnpause()
{
	return true;
}

/**
 * Adds the delegate to the list if the player controller has the right to pause
 * the game. The delegate is called to see if it is ok to unpause the game, e.g.
 * the reason the game was paused has been cleared.
 *
 * @param PC the player controller to check for admin privs
 * @param CanUnpauseDelegate the delegate to query when checking for unpause
 */
function bool SetPause(PlayerController PC, optional delegate<CanUnpause> CanUnpauseDelegate)
{
	local int FoundIndex;

    if (bPauseable ||
		(bAdminCanPause && (PC.IsA('Admin') || PC.PlayerReplicationInfo.bAdmin)) ||
		WorldInfo.NetMode == NM_Standalone)
	{
		// Don't add the delegate twice (no need)
		FoundIndex = Pausers.Find(CanUnpauseDelegate);
		if (FoundIndex == INDEX_NONE)
		{
			// Not in the list so add it for querying
			FoundIndex = Pausers.Length;
			Pausers.Length = FoundIndex + 1;
			Pausers[FoundIndex] = CanUnpauseDelegate;
		}
		// Let the first one in "own" the pause state
		if (WorldInfo.Pauser == None)
		{
			WorldInfo.Pauser = PC.PlayerReplicationInfo;
		}
		return true;
	}
	return false;
}

/**
 * Checks the list of delegates to determine if the pausing can be cleared. If
 * the delegate says it's ok to unpause, that delegate is removed from the list
 * and the rest are checked. The game is considered unpaused when the list is
 * empty.
 */
function ClearPause()
{
	local int Index;
	local delegate<CanUnpause> CanUnpauseCriteriaMet;

	for (Index = 0; Index < Pausers.Length; Index++)
	{
		CanUnpauseCriteriaMet = Pausers[Index];
		if (CanUnpauseCriteriaMet())
		{
			Pausers.Remove(Index,1);
			Index--;
		}
	}
	// Clear the pause state if the list is empty
	if (Pausers.Length == 0)
	{
		WorldInfo.Pauser = None;
	}
}

/**
 * Dumps the pause delegate list to track down who has the game paused
 */
function DebugPause()
{
	local int Index;
	local delegate<CanUnpause> CanUnpauseCriteriaMet;

	for (Index = 0; Index < Pausers.Length; Index++)
	{
		CanUnpauseCriteriaMet = Pausers[Index];
		if (CanUnpauseCriteriaMet())
		{
			`Log("Pauser in index "$Index$" thinks it's ok to unpause");
		}
		else
		{
			`Log("Pauser in index "$Index$" thinks the game should remain paused");
		}
	}
}

//------------------------------------------------------------------------------
// Game parameters.

//
// Set gameplay speed.
//
function SetGameSpeed( Float T )
{
	GameSpeed = FMax(T, 0.1);
	WorldInfo.TimeDilation = GameSpeed;
	SetTimer(WorldInfo.TimeDilation, true);
}

//------------------------------------------------------------------------------
// Player start functions

//
// Grab the next option from a string.
//
static function bool GrabOption( out string Options, out string Result )
{
	if( Left(Options,1)=="?" )
	{
		// Get result.
		Result = Mid(Options,1);
		if( InStr(Result,"?")>=0 )
			Result = Left( Result, InStr(Result,"?") );

		// Update options.
		Options = Mid(Options,1);
		if( InStr(Options,"?")>=0 )
			Options = Mid( Options, InStr(Options,"?") );
		else
			Options = "";

		return true;
	}
	else return false;
}

//
// Break up a key=value pair into its key and value.
//
static function GetKeyValue( string Pair, out string Key, out string Value )
{
	if( InStr(Pair,"=")>=0 )
	{
		Key   = Left(Pair,InStr(Pair,"="));
		Value = Mid(Pair,InStr(Pair,"=")+1);
	}
	else
	{
		Key   = Pair;
		Value = "";
	}
}

/* ParseOption()
 Find an option in the options string and return it.
*/
static function string ParseOption( string Options, string InKey )
{
	local string Pair, Key, Value;
	while( GrabOption( Options, Pair ) )
	{
		GetKeyValue( Pair, Key, Value );
		if( Key ~= InKey )
			return Value;
	}
	return "";
}

//
// HasOption - return true if the option is specified on the command line.
//
static function bool HasOption( string Options, string InKey )
{
    local string Pair, Key, Value;
    while( GrabOption( Options, Pair ) )
    {
	GetKeyValue( Pair, Key, Value );
	if( Key ~= InKey )
	    return true;
    }
    return false;
}

static event class<GameInfo> SetGameType(string MapName, string Options)
{
	return Default.Class;
}

/* Initialize the game.
 The GameInfo's InitGame() function is called before any other scripts (including
 PreBeginPlay() ), and is used by the GameInfo to initialize parameters and spawn
 its helper classes.
 Warning: this is called before actors' PreBeginPlay.
*/
event InitGame( string Options, out string ErrorMessage )
{
	local string InOpt, LeftOpt;
	local int pos;
	local class<AccessControl> ACClass;

    MaxPlayers = Clamp(GetIntOption( Options, "MaxPlayers", MaxPlayers ),0,MaxPlayersAllowed);
    MaxSpectators = Clamp(GetIntOption( Options, "MaxSpectators", MaxSpectators ),0,MaxSpectatorsAllowed);
    GameDifficulty = FMax(0,GetIntOption(Options, "Difficulty", GameDifficulty));

	InOpt = ParseOption( Options, "GameSpeed");
	if( InOpt != "" )
	{
		`log("GameSpeed"@InOpt);
		SetGameSpeed(float(InOpt));
	}

	TimeLimit = Max(0,GetIntOption( Options, "TimeLimit", TimeLimit ));
	AutomatedPerfRemainingTime = 60 * TimeLimit;

	BroadcastHandler = spawn(BroadcastHandlerClass);

	InOpt = ParseOption( Options, "AccessControl");
	if( InOpt != "" )
		ACClass = class<AccessControl>(DynamicLoadObject(InOpt, class'Class'));
    if ( ACClass == None )
	{
		ACClass = AccessControlClass;
	}

	LeftOpt = ParseOption( Options, "AdminName" );
	InOpt = ParseOption( Options, "AdminPassword");
	/* @FIXME: this is a compiler error, "can't set defaults of non-config properties". Fix or remove at some point.
	if( LeftOpt!="" && InOpt!="" )
		ACClass.default.bDontAddDefaultAdmin = true;
	*/

	// Only spawn access control if we are a server
	if (WorldInfo.NetMode == NM_ListenServer || WorldInfo.NetMode == NM_DedicatedServer )
	{
		AccessControl = Spawn(ACClass);
	}

	InOpt = ParseOption( Options, "Mutator");
	if ( InOpt != "" )
	{
		`log("Mutators"@InOpt);
		while ( InOpt != "" )
		{
			pos = InStr(InOpt,",");
			if ( pos > 0 )
			{
				LeftOpt = Left(InOpt, pos);
				InOpt = Right(InOpt, Len(InOpt) - pos - 1);
			}
			else
			{
				LeftOpt = InOpt;
				InOpt = "";
			}
	    	AddMutator(LeftOpt, true);
		}
	}

	InOpt = ParseOption( Options, "GamePassword");
    if( InOpt != "" && AccessControl != None)
	{
		AccessControl.SetGamePassWord(InOpt);
		`log( "GamePassword" @ InOpt );
	}

	InOpt = ParseOption(Options,"AutomatedPerfTesting");
	if ( InOpt != "" )
	{
		bAutomatedPerfTesting = TRUE;
	}

	if (BaseMutator != none)
	{
		BaseMutator.InitMutator(Options, ErrorMessage);
	}

	// Cache a pointer to the online subsystem
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// And grab one for the game interface since it will be used often
		GameInterface = OnlineSub.GameInterface;
	}

	if (WorldInfo.NetMode != NM_Standalone &&
		// Don't register the session if the UI already has
		OnlineSub != None &&
		OnlineSub.GameInterface != None &&
		GameInterface.GetGameSettings() == None)
	{
		// Cache this so it can be used later by async processes
		ServerOptions = Options;
		// If there isn't a login to process, immediately register the server
		// Otherwise the server will be registered when the login completes
		if (ProcessServerLogin() == false)
		{
			RegisterServer();
		}
	}
}

function AddMutator(string mutname, optional bool bUserAdded)
{
    local class<Mutator> mutClass;
    local Mutator mut;

	if ( !Static.AllowMutator(MutName) )
		return;

    mutClass = class<Mutator>(DynamicLoadObject(mutname, class'Class'));
    if (mutClass == None)
		return;

	if ( (mutClass.Default.GroupName != "") && (BaseMutator != None) )
	{
		// make sure no mutators with same groupname
		for ( mut=BaseMutator; mut!=None; mut=mut.NextMutator )
			if ( mut.GroupName == mutClass.Default.GroupName )
			{
				`log("Not adding "$mutClass$" because already have a mutator in the same group - "$mut);
				return;
			}
	}

	// make sure this mutator is not added already
	for ( mut=BaseMutator; mut!=None; mut=mut.NextMutator )
		if ( mut.Class == mutClass )
		{
			`log("Not adding "$mutClass$" because this mutator is already added - "$mut);
			return;
		}

    mut = Spawn(mutClass);
	// mc, beware of mut being none
	if (mut == None)
		return;

	// Meant to verify if this mutator was from Command Line parameters or added from other Actors
	mut.bUserAdded = bUserAdded;

    if (BaseMutator == None)
    {
		BaseMutator = mut;
	}
    else
    {
		BaseMutator.AddMutator(mut);
	}
}

function AddGameRules(class<GameRules> GRClass)
{
	if ( GRClass != None )
	{
		if ( GameRulesModifiers == None )
			GameRulesModifiers = Spawn(GRClass);
		else
			GameRulesModifiers.AddGameRules(Spawn(GRClass));
	}
}

/* RemoveMutator()
Remove a mutator from the mutator list
*/
function RemoveMutator( Mutator MutatorToRemove )
{
	local Mutator M;

	// remove from mutator list
	if ( BaseMutator == MutatorToRemove )
	{
		BaseMutator = MutatorToRemove.NextMutator;
	}
	else if ( BaseMutator != None )
	{
		for ( M=BaseMutator; M!=None; M=M.NextMutator )
		{
			if ( M.NextMutator == MutatorToRemove )
			{
				M.NextMutator = MutatorToRemove.NextMutator;
				break;
			}
		}
	}
}

//
// Return beacon text for serverbeacon.
//
event string GetBeaconText()
{
	return
		WorldInfo.ComputerName
    $   " "
    $   Left(WorldInfo.Title,24)
    $   "\\t"
    $   GetNumPlayers()
	$	"/"
	$	MaxPlayers;
}

/* ProcessServerTravel()
 Optional handling of ServerTravel for network games.
*/
function ProcessServerTravel(string URL)
{
	local PlayerController P, LocalPlayer;
	local bool bSeamless;

	bLevelChange = true;
	EndLogging("mapchange");

	// force an old style load screen if the server has been up for a long time so that TimeSeconds doesn't overflow and break everything
	bSeamless = (bUseSeamlessTravel && WorldInfo.TimeSeconds < 172800.0f); // 172800 seconds == 48 hours

	// Notify clients we're switching level and give them time to receive.
	// We call PreClientTravel directly on any local PlayerPawns (ie listen server)
	`log("ProcessServerTravel:"@URL);
	foreach WorldInfo.AllControllers(class'PlayerController', P)
	{
		if (NetConnection(P.Player) != None)
		{
			P.ClientTravel(URL, TRAVEL_Relative, bSeamless);
		}
		else
		{
			LocalPlayer = P;
			P.PreClientTravel();
		}
	}

	if (WorldInfo.NetMode == NM_ListenServer && LocalPlayer != None)
	{
		WorldInfo.NextURL = WorldInfo.NextURL
					$"?Team="$LocalPlayer.GetDefaultURL("Team")
					$"?Name="$LocalPlayer.GetDefaultURL("Name")
					$"?Class="$LocalPlayer.GetDefaultURL("Class")
					$"?Character="$LocalPlayer.GetDefaultURL("Character");
	}

	if (bSeamless)
	{
		WorldInfo.SeamlessTravel(WorldInfo.NextURL);
		WorldInfo.NextURL = "";
	}
	// Switch immediately if not networking.
	else if (WorldInfo.NetMode != NM_DedicatedServer && WorldInfo.NetMode != NM_ListenServer)
	{
		WorldInfo.NextSwitchCountdown = 0.0;
	}
}

function bool RequiresPassword()
{
	return ( (AccessControl != None) && AccessControl.RequiresPassword() );
}

//
// Accept or reject a player on the server.
// Fails login if you set the Error to a non-empty string.
//
event PreLogin
(
	string Options,
	string Address,
	out string ErrorMessage,
	out string FailCode
)
{
	local bool bSpectator;
	local bool bPerfTesting;

	// Check for an arbitrated match in progress and kick if needed
	if (WorldInfo.NetMode != NM_Standalone && bUsingArbitration && bHasArbitratedHandshakeBegun)
	{
		//@todo joeg -- Put proper error message in here
		ErrorMessage = GameMessageClass.Default.ArbitrationMessage;
		return;
	}

	bPerfTesting = ( ParseOption( Options, "AutomatedPerfTesting" ) ~= "1" );
    bSpectator = bPerfTesting || ( ParseOption( Options, "SpectatorOnly" ) ~= "1" );

    if (AccessControl != None)
	{
		AccessControl.PreLogin(Options, Address, ErrorMessage, FailCode, bSpectator);
	}
}

function int GetIntOption( string Options, string ParseString, int CurrentValue)
{
	local string InOpt;

	InOpt = ParseOption( Options, ParseString );
	if ( InOpt != "" )
	{
		return int(InOpt);
	}
	return CurrentValue;
}

function bool AtCapacity(bool bSpectator)
{
	if ( WorldInfo.NetMode == NM_Standalone )
		return false;

	if ( bSpectator )
		return ( (NumSpectators >= MaxSpectators)
			&& ((WorldInfo.NetMode != NM_ListenServer) || (NumPlayers > 0)) );
	else
		return ( (MaxPlayers>0) && (NumPlayers>=MaxPlayers) );
}

//
// Log a player in.
// Fails login if you set the Error string.
// PreLogin is called before Login, but significant game time may pass before
// Login is called, especially if content is downloaded.
//
event PlayerController Login
(
	string Portal,
	string Options,
	out string ErrorMessage
)
{
	local NavigationPoint StartSpot;
	local PlayerController NewPlayer;
    local string          InName, InCharacter;
	local byte            InTeam;
    local bool bSpectator, bAdmin, bPerfTesting;
	local rotator SpawnRotation;

	bAdmin = false;

	bPerfTesting = ( ParseOption( Options, "AutomatedPerfTesting" ) ~= "1" );
    bSpectator = bPerfTesting || ( ParseOption( Options, "SpectatorOnly" ) ~= "1" );

	// Kick the player if they joined during the handshake process
	if (bUsingArbitration && bHasArbitratedHandshakeBegun)
	{
		ErrorMessage = GameMessageClass.Default.MaxedOutMessage;
		return None;
	}

	// Make sure there is capacity except for admins. (This might have changed since the PreLogin call).
    if ( !bAdmin && AtCapacity(bSpectator) )
	{
		ErrorMessage = GameMessageClass.Default.MaxedOutMessage;
		return None;
	}

	// If admin, force spectate mode if the server already full of reg. players
	if ( bAdmin && AtCapacity(false))
		bSpectator = true;

	if ( BaseMutator != None )
		BaseMutator.ModifyLogin(Portal, Options);

	// Get URL options.
	InName     = Left(ParseOption ( Options, "Name"), 20);
	InTeam     = GetIntOption( Options, "Team", 255 ); // default to "no team"
    //InAdminName= ParseOption ( Options, "AdminName");
	//InPassword = ParseOption ( Options, "Password" );
	//InChecksum = ParseOption ( Options, "Checksum" );

	// Pick a team (if need teams)
    InTeam = PickTeam(InTeam,None);

	// Find a start spot.
	StartSpot = FindPlayerStart( None, InTeam, Portal );

	if( StartSpot == None )
	{
		ErrorMessage = GameMessageClass.Default.FailedPlaceMessage;
		return None;
	}

	SpawnRotation.Yaw = StartSpot.Rotation.Yaw;
	NewPlayer = spawn(PlayerControllerClass,,,StartSpot.Location,SpawnRotation);

	// Handle spawn failure.
	if( NewPlayer == None )
	{
		`log("Couldn't spawn player controller of class "$PlayerControllerClass);
		ErrorMessage = GameMessageClass.Default.FailedSpawnMessage;
		return None;
	}

	NewPlayer.StartSpot = StartSpot;

	// Set the player's ID.
	NewPlayer.PlayerReplicationInfo.PlayerID = CurrentID++;

	// Init player's name
	if( InName=="" )
		InName=DefaultPlayerName$NewPlayer.PlayerReplicationInfo.PlayerID;
	ChangeName( NewPlayer, InName, false );

    InCharacter = ParseOption(Options, "Character");
    NewPlayer.SetCharacter(InCharacter);

    if ( bSpectator || NewPlayer.PlayerReplicationInfo.bOnlySpectator || !ChangeTeam(newPlayer, InTeam, false) )
	{
		NewPlayer.GotoState('Spectating');
		NewPlayer.PlayerReplicationInfo.bOnlySpectator = true;
		NewPlayer.PlayerReplicationInfo.bIsSpectator = true;
		NewPlayer.PlayerReplicationInfo.bOutOfLives = true;
		NumSpectators++;
		return NewPlayer;
	}

	NumPlayers++;
	// Tell the online subsystem the number of players in the game
	UpdateGameSettingsCounts();

	// if delayed start, don't give a pawn to the player yet
	// Normal for multiplayer games
	if ( bDelayedStart )
	{
		NewPlayer.GotoState('PlayerWaiting');
		return NewPlayer;
	}

	return newPlayer;
}

exec function TestLevel()
{
	local Actor A, Found;
	local bool bFoundErrors;

	ForEach AllActors(class'Actor', A)
	{
		bFoundErrors = bFoundErrors || A.CheckForErrors();
		if ( bFoundErrors && (Found == None) )
			Found = A;
	}

	if ( bFoundErrors )
	{
		`log("Found problem with "$Found);
		assert(false);
	}
}

/* StartMatch()
Start the game - inform all actors that the match is starting, and spawn player pawns
*/
function StartMatch()
{
	local Actor A;

	// tell all actors the game is starting
	ForEach AllActors(class'Actor', A)
	{
		A.MatchStarting();
	}

	// start human players first
	StartHumans();

	// start AI players
	StartBots();

	bWaitingToStartMatch = false;

	StartOnlineGame();

	// fire off any level startup events
	WorldInfo.NotifyMatchStarted();
}

/**
 * Tells the online system to start the game and waits for the callback. Tells
 * each connected client to register their
 */
function StartOnlineGame()
{
	if (GameInterface != None)
	{
		// Register the start callback so that the stat guid can be read
		GameInterface.AddStartOnlineGameCompleteDelegate(OnStartOnlineGameComplete);
		// Start the game locally and wait for it to complete
		GameInterface.StartOnlineGame();
	}
	else
	{
		// Notify all clients that the match has begun
		GameReplicationInfo.StartMatch();
	}
}

/**
 * Callback when the start completes
 *
 * @param bWasSuccessful true if it worked, false otherwise
 */
function OnStartOnlineGameComplete(bool bWasSuccessful)
{
	local PlayerController PC;
	local string StatGuid;

	GameInterface.ClearStartOnlineGameCompleteDelegate(OnStartOnlineGameComplete);
	if (bWasSuccessful && OnlineSub.StatsInterface != None)
	{
		// Get the stat guid for the server
		StatGuid = OnlineSub.StatsInterface.GetHostStatGuid();
		if (StatGuid != "")
		{
			// Send the stat guid to all clients
			foreach WorldInfo.AllControllers(class'PlayerController',PC)
			{
				if (PC.IsLocalPlayerController() == false)
				{
					PC.ClientRegisterHostStatGuid(StatGuid);
				}
			}
		}
	}
	// Notify all clients that the match has begun
	GameReplicationInfo.StartMatch();
}

function StartHumans()
{
	local PlayerController P;

	foreach WorldInfo.AllControllers(class'PlayerController', P)
	{
		if (P.Pawn == None)
		{
			if ( bGameEnded )
			{
				return; // telefrag ended the game with ridiculous frag limit
			}
			else if (P.CanRestartPlayer())
			{
				RestartPlayer(P);
			}
		}
	}
}

function StartBots()
{
	local Controller P;

	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		if (P.bIsPlayer && !P.IsA('PlayerController'))
		{
			if (WorldInfo.NetMode == NM_Standalone)
			{
				RestartPlayer(P);
			}
			else
			{
				P.GotoState('Dead','MPStart');
			}
		}
	}
}
//
// Restart a player.
//
function RestartPlayer(Controller NewPlayer)
{
	local NavigationPoint startSpot;
	local int TeamNum, Idx;
	local array<SequenceObject> Events;
	local SeqEvent_PlayerSpawned SpawnedEvent;

	if( bRestartLevel && WorldInfo.NetMode!=NM_DedicatedServer && WorldInfo.NetMode!=NM_ListenServer )
	{
		`warn("bRestartLevel && !server, abort from RestartPlayer"@WorldInfo.NetMode);
		return;
	}
	// figure out the team number and find the start spot
	TeamNum = ((NewPlayer.PlayerReplicationInfo == None) || (NewPlayer.PlayerReplicationInfo.Team == None)) ? 255 : NewPlayer.PlayerReplicationInfo.Team.TeamIndex;
	StartSpot = FindPlayerStart(NewPlayer, TeamNum);

	// if a start spot wasn't found,
	if (startSpot == None)
	{
		// check for a previously assigned spot
		if (NewPlayer.StartSpot != None)
		{
			StartSpot = NewPlayer.StartSpot;
			`warn("Player start not found, using last start spot");
		}
		else
		{
			// otherwise abort
			`warn("Player start not found, failed to restart player");
			return;
		}
	}
	// try to create a pawn to use of the default class for this player
	if (NewPlayer.Pawn == None)
	{
		NewPlayer.Pawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
	}
	if (NewPlayer.Pawn == None)
	{
		`log("failed to spawn player at "$StartSpot);
		NewPlayer.GotoState('Dead');
		if ( PlayerController(NewPlayer) != None )
		{
			PlayerController(NewPlayer).ClientGotoState('Dead','Begin');
		}
	}
	else
	{
		// initialize and start it up
		NewPlayer.Pawn.SetAnchor(startSpot);
		if ( PlayerController(NewPlayer) != None )
		{
			PlayerController(NewPlayer).TimeMargin = -0.1;
			startSpot.AnchoredPawn = None; // SetAnchor() will set this since IsHumanControlled() won't return true for the Pawn yet
		}
		NewPlayer.Pawn.LastStartSpot = PlayerStart(startSpot);
		NewPlayer.Pawn.LastStartTime = WorldInfo.TimeSeconds;
		NewPlayer.Possess(NewPlayer.Pawn, false);
		NewPlayer.Pawn.PlayTeleportEffect(true, true);
		NewPlayer.ClientSetRotation(NewPlayer.Pawn.Rotation);

		if (!WorldInfo.bNoDefaultInventoryForPlayer)
		{
			AddDefaultInventory(NewPlayer.Pawn);
		}
		SetPlayerDefaults(NewPlayer.Pawn);

		// activate spawned events
		if (WorldInfo.GetGameSequence() != None)
		{
			WorldInfo.GetGameSequence().FindSeqObjectsByClass(class'SeqEvent_PlayerSpawned',TRUE,Events);
			for (Idx = 0; Idx < Events.Length; Idx++)
			{
				SpawnedEvent = SeqEvent_PlayerSpawned(Events[Idx]);
				if (SpawnedEvent != None &&
					SpawnedEvent.CheckActivate(NewPlayer,NewPlayer))
				{
					SpawnedEvent.SpawnPoint = startSpot;
					SpawnedEvent.PopulateLinkedVariableValues();
				}
			}
		}
	}
}

/**
 * Returns a pawn of the default pawn class
 *
 * @param	NewPlayer - Controller for whom this pawn is spawned
 * @param	StartSpot - PlayerStart at which to spawn pawn
 *
 * @return	pawn
 */
function Pawn SpawnDefaultPawnFor(Controller NewPlayer, NavigationPoint StartSpot)
{
	local class<Pawn> DefaultPlayerClass;
	local Rotator StartRotation;
	local Pawn ResultPawn;

	DefaultPlayerClass = GetDefaultPlayerClass(NewPlayer);

	// don't allow pawn to be spawned with any pitch or roll
	StartRotation.Yaw = StartSpot.Rotation.Yaw;

	ResultPawn = Spawn(DefaultPlayerClass,,,StartSpot.Location,StartRotation);
	if ( ResultPawn == None )
	{
		`log("Couldn't spawn player of type "$DefaultPlayerClass$" at "$StartSpot);
	}
	return ResultPawn;
}

/**
 * Returns the default pawn class for the specified controller,
 *
 * @param	C - controller to figure out pawn class for
 *
 * @return	default pawn class
 */
function class<Pawn> GetDefaultPlayerClass(Controller C)
{
	// default to the game specified pawn class
	return DefaultPawnClass;
}

/** replicates the current level streaming status to the given PlayerController */
function ReplicateStreamingStatus(PlayerController PC)
{
	local int LevelIndex;
	local LevelStreaming TheLevel;

	// don't do this for local players or players after the first on a splitscreen client
	if (LocalPlayer(PC.Player) == None && ChildConnection(PC.Player) == None)
	{
		// if we've loaded levels via CommitMapChange() that aren't normally in the StreamingLevels array, tell the client about that
		if (WorldInfo.CommittedLevelNames.length > 0)
		{
			for (LevelIndex = 0; LevelIndex < WorldInfo.CommittedLevelNames.length; LevelIndex++)
			{
				PC.ClientPrepareMapChange(WorldInfo.CommittedLevelNames[LevelIndex], LevelIndex == 0, LevelIndex == WorldInfo.CommittedLevelNames.length - 1);
			}
			// tell the client to commit the levels immediately
			PC.ClientCommitMapChange();
		}

		if (WorldInfo.StreamingLevels.length > 0)
		{
	 		// Tell the player controller the current streaming level status
	 		for (LevelIndex = 0; LevelIndex < WorldInfo.StreamingLevels.Length; LevelIndex++)
	 		{
				// streamingServer
				TheLevel = WorldInfo.StreamingLevels[LevelIndex];

				if( TheLevel != none )
				{
					`log( "levelStatus: " $ TheLevel.PackageName $ " "
						$ TheLevel.bShouldBeVisible  $ " "
						$ TheLevel.bIsVisible  $ " "
						$ TheLevel.bShouldBeLoaded  $ " "
						$ TheLevel.LoadedLevel  $ " "
						$ TheLevel.bHasLoadRequestPending  $ " "
						) ;

	 				PC.ClientUpdateLevelStreamingStatus(
	 					TheLevel.PackageName,
	 					TheLevel.bShouldBeLoaded,
	 					TheLevel.bShouldBeVisible,
	 					TheLevel.bShouldBlockOnLoad);
	 			}
			}
	 		PC.ClientFlushLevelStreaming();
		}

		// if we're preparing to load different levels using PrepareMapChange() inform the client about that now
		if (WorldInfo.PreparingLevelNames.length > 0)
		{
			for (LevelIndex = 0; LevelIndex < WorldInfo.PreparingLevelNames.length; LevelIndex++)
			{
				PC.ClientPrepareMapChange(WorldInfo.PreparingLevelNames[LevelIndex], LevelIndex == 0, LevelIndex == WorldInfo.PreparingLevelNames.length - 1);
			}
			// DO NOT commit these changes yet - we'll send that when we're done preparing them
		}
	}
}

//
// Called after a successful login. This is the first place
// it is safe to call replicated functions on the PlayerController.
//
event PostLogin( PlayerController NewPlayer )
{
	local string Address;
	local string StatGuid;
	local int pos;

	// save network address for re-associating with reconnecting player, after stripping out port number
	Address = NewPlayer.GetPlayerNetworkAddress();
	pos = InStr(Address,":");
	NewPlayer.PlayerReplicationInfo.SavedNetworkAddress = (pos > 0) ? left(Address,pos) : Address;

	// check if this player is reconnecting and already has PRI
	FindInactivePRI(NewPlayer);

	if ( !bDelayedStart )
	{
		// start match, or let player enter, immediately
		bRestartLevel = false;	// let player spawn once in levels that must be restarted after every death
		if ( 	bWaitingToStartMatch )
			StartMatch();
		else
			RestartPlayer(newPlayer);
		bRestartLevel = Default.bRestartLevel;
	}

	// tell client what hud and scoreboard to use
	NewPlayer.ClientSetHUD( HudType, ScoreboardType );

	if ( NewPlayer.Pawn != None )
		NewPlayer.Pawn.ClientSetRotation(NewPlayer.Pawn.Rotation);

	NewPlayer.ClientCapBandwidth(NewPlayer.Player.CurrentNetSpeed);
	if ( BaseMutator != None )
		BaseMutator.NotifyLogin(NewPlayer);

	ReplicateStreamingStatus(NewPlayer);

	// tell the player about the music, if any
	NewPlayer.ServerSendMusicInfo();

	// see if we need to spawn a CoverReplicator for this player
	if (CoverReplicatorBase != None)
	{
		NewPlayer.SpawnCoverReplicator();
	}

	// Set the rich presence strings on the client (has to be done there)
	NewPlayer.ClientSetRichPresence();

	// Tell the new player the stat guid
	if (GameReplicationInfo.bMatchHasBegun && OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		// Get the stat guid for the server
		StatGuid = OnlineSub.StatsInterface.GetHostStatGuid();
		if (StatGuid != "")
		{
			NewPlayer.ClientRegisterHostStatGuid(StatGuid);
		}
	}

	// Tell the player to disable voice by default and use the push to talk method
	if (bRequiresPushToTalk)
	{
		NewPlayer.ClientStopNetworkedVoice();
	}

	if (NewPlayer.PlayerReplicationInfo.bOnlySpectator)
	{
		NewPlayer.ClientGotoState('Spectating');
	}
}

/**
 * Engine is shutting down.
 */
event PreExit();

//
// Player exits.
//
function Logout( Controller Exiting )
{
	local PlayerController PC;
	local int PCIndex;

	PC = PlayerController(Exiting);
	if ( PC != None )
	{
		//FIXMESTEVE		if ( AccessControl.AdminLogout( PlayerController(Exiting) ) )
		//			AccessControl.AdminExited( PlayerController(Exiting) );

		if ( PC.PlayerReplicationInfo.bOnlySpectator )
		{
			NumSpectators--;
		}
		else
		{
			NumPlayers--;
			// Tell the online subsystem the number of players in the game
			UpdateGameSettingsCounts();
		}
		// This person has left during an arbitration period
		if (bUsingArbitration && bHasArbitratedHandshakeBegun && !bHasEndGameHandshakeBegun)
		{
			`Log("Player "$PC.PlayerReplicationInfo.PlayerName$" has dropped");
		}
		// If there is a game unregister this remote player
		if (WorldInfo.NetMode != NM_Standalone &&
			GameInterface != None &&
			GameInterface.GetGameSettings() != None)
		{
			// Unregister the player from the session
			GameInterface.UnregisterPlayer(PC.PlayerReplicationInfo.UniqueId);
		}
		// Remove from the arbitrated PC list if in an arbitrated match
		if (bUsingArbitration)
		{
			// Find the PC in the list and remove it if found
			PCIndex = ArbitrationPCs.Find(PC);
			if (PCIndex != INDEX_NONE)
			{
				ArbitrationPCs.Remove(PCIndex,1);
			}
		}
	}
	//notify mutators that a player exited
	if (BaseMutator != None)
	{
		BaseMutator.NotifyLogout(Exiting);
	}
}

//
// Examine the passed player's inventory, and accept or discard each item.
// AcceptInventory needs to gracefully handle the case of some inventory
// being accepted but other inventory not being accepted (such as the default
// weapon).  There are several things that can go wrong: A weapon's
// AmmoType not being accepted but the weapon being accepted -- the weapon
// should be killed off. Or the player's selected inventory item, active
// weapon, etc. not being accepted, leaving the player weaponless or leaving
// the HUD inventory rendering messed up (AcceptInventory should pick another
// applicable weapon/item as current).
//
event AcceptInventory(pawn PlayerPawn)
{
	//default accept all inventory except default weapon (spawned explicitly)
}

//
// Spawn any default inventory for the player.
//
event AddDefaultInventory(Pawn P)
{
    // Allow the pawn itself to modify its inventory
    P.AddDefaultInventory();

	if ( P.InvManager == None )
	{
		`warn("GameInfo::AddDefaultInventory - P.InvManager == None");
	}
}

/* Mutate()
Pass an input string to the mutator list.  Used by PlayerController.Mutate(), intended to allow
mutators to have input exec functions (by binding mutate xxx to keys)
*/
function Mutate(string MutateString, PlayerController Sender)
{
	if ( BaseMutator != None )
		BaseMutator.Mutate(MutateString, Sender);
}

/* SetPlayerDefaults()
 first make sure pawn properties are back to default, then give mutators an opportunity
 to modify them
*/
function SetPlayerDefaults(Pawn PlayerPawn)
{
	PlayerPawn.AirControl = PlayerPawn.Default.AirControl;
	PlayerPawn.GroundSpeed = PlayerPawn.Default.GroundSpeed;
	PlayerPawn.WaterSpeed = PlayerPawn.Default.WaterSpeed;
	PlayerPawn.AirSpeed = PlayerPawn.Default.AirSpeed;
	PlayerPawn.Acceleration = PlayerPawn.Default.Acceleration;
	PlayerPawn.AccelRate = PlayerPawn.Default.AccelRate;
	PlayerPawn.JumpZ = PlayerPawn.Default.JumpZ;
	if ( BaseMutator != None )
		BaseMutator.ModifyPlayer(PlayerPawn);
	PlayerPawn.PhysicsVolume.ModifyPlayer(PlayerPawn);
}

function NotifyKilled(Controller Killer, Controller Killed, Pawn KilledPawn )
{
	local Controller C;

	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		C.NotifyKilled(Killer, Killed, KilledPawn);
	}
}

function Killed( Controller Killer, Controller KilledPlayer, Pawn KilledPawn, class<DamageType> damageType )
{
    if( KilledPlayer != None && KilledPlayer.bIsPlayer )
	{
		KilledPlayer.PlayerReplicationInfo.Deaths += 1;
		KilledPlayer.PlayerReplicationInfo.NetUpdateTime = FMin(KilledPlayer.PlayerReplicationInfo.NetUpdateTime, WorldInfo.TimeSeconds + 0.3 * FRand());
		BroadcastDeathMessage(Killer, KilledPlayer, damageType);
	}

    if( KilledPlayer != None )
	{
		ScoreKill(Killer, KilledPlayer);
	}

	DiscardInventory(KilledPawn);
    NotifyKilled(Killer, KilledPlayer, KilledPawn);
}

function bool PreventDeath(Pawn KilledPawn, Controller Killer, class<DamageType> damageType, vector HitLocation)
{
	if ( GameRulesModifiers == None )
		return false;
	return GameRulesModifiers.PreventDeath(KilledPawn,Killer, damageType,HitLocation);
}

function BroadcastDeathMessage(Controller Killer, Controller Other, class<DamageType> damageType)
{
	if ( (Killer == Other) || (Killer == None) )
	BroadcastLocalized(self,DeathMessageClass, 1, None, Other.PlayerReplicationInfo, damageType);
	else
	BroadcastLocalized(self,DeathMessageClass, 0, Killer.PlayerReplicationInfo, Other.PlayerReplicationInfo, damageType);
}


// %k = Owner's PlayerName (Killer)
// %o = Other's PlayerName (Victim)
// %w = Owner's Weapon ItemName
static native function string ParseKillMessage( string KillerName, string VictimName, string DeathMessage );

function Kick( string S )
{
	if (AccessControl != None)
		AccessControl.Kick(S);
}

function KickBan( string S )
{
	if (AccessControl != None)
		AccessControl.KickBan(S);
}

//-------------------------------------------------------------------------------------
// Level gameplay modification.

//
// Return whether Viewer is allowed to spectate from the
// point of view of ViewTarget.
//
function bool CanSpectate( PlayerController Viewer, PlayerReplicationInfo ViewTarget )
{
	return true;
}

/* ReduceDamage:
	Use reduce damage for teamplay modifications, etc. */
function ReduceDamage( out int Damage, pawn injured, Controller instigatedBy, vector HitLocation, out vector Momentum, class<DamageType> DamageType )
{
	local int OriginalDamage;

	OriginalDamage = Damage;

	if ( injured.PhysicsVolume.bNeutralZone || injured.InGodMode() )
	{
		Damage = 0;
		return;
	}
	else if ( (damage > 0) && (injured.InvManager != None) ) // then check if carrying items that can reduce damage
		injured.InvManager.ModifyDamage( Damage, instigatedBy, HitLocation, Momentum, DamageType );

	if ( GameRulesModifiers != None )
		GameRulesModifiers.NetDamage( OriginalDamage, Damage,injured,instigatedBy,HitLocation,Momentum,DamageType );
}

/* CheckRelevance()
returns true if actor is relevant to this game and should not be destroyed.  Called in Actor.PreBeginPlay(), intended to allow
mutators to remove or replace actors being spawned
*/
function bool CheckRelevance(Actor Other)
{
	if ( BaseMutator == None )
		return true;
	return BaseMutator.CheckRelevance(Other);
}

//
// Return whether an item should respawn.
//
function bool ShouldRespawn( PickupFactory Other )
{
	return ( WorldInfo.NetMode != NM_Standalone );
}

/** PickupQuery:
 *	Called when pawn has a chance to pick Item up (i.e. when
 *	the pawn touches a weapon pickup). Should return true if
 *	he wants to pick it up, false if he does not want it.
 * @param Other the Pawn that wants the item
 * @param ItemClass the Inventory class the Pawn can pick up
 * @param Pickup the Actor containing that item (this may be a PickupFactory or it may be a DroppedPickup)
 * @return whether or not the Pickup actor should give its item to Other
 */
function bool PickupQuery(Pawn Other, class<Inventory> ItemClass, Actor Pickup)
{
	local byte bAllowPickup;

	if (GameRulesModifiers != None && GameRulesModifiers.OverridePickupQuery(Other, ItemClass, Pickup, bAllowPickup))
	{
		return bool(bAllowPickup);
	}

	if ( Other.InvManager == None )
	{
		return false;
	}
	else
	{
		return Other.InvManager.HandlePickupQuery(ItemClass, Pickup);
	}
}

/* DiscardInventory:
	Discard a player's inventory after he dies. */
function DiscardInventory( Pawn Other )
{
	if ( Other.InvManager != None )
		Other.InvManager.DiscardInventory();
}

/**
 * Makes sure a player's name conforms to set of rules
 *
 * @Param NewName  	The name to conform.
 * @Param MaxLen	The Max. # of characters in the name
 */

static function ConformPlayerName(out string NewName, int MaxLen)
{
	if ( NewName != "" )
	{
		if ( Len(NewName) > MaxLen )
		{
			NewName = Left(NewName,MaxLen);
		}

		// Replace spaces and potentially dangerous characters

	    ReplaceText(NewName, " ", "_");
    	ReplaceText(NewName, "\"", "");
    }
}


/* Try to change a player's name.
*/
function ChangeName( Controller Other, coerce string S, bool bNameChange )
{
	if( S == "" )
	{
		return;
	}

	ConformPlayerName(S,20);
	Other.PlayerReplicationInfo.SetPlayerName(S);
}

/* Return whether a team change is allowed.
*/
function bool ChangeTeam(Controller Other, int N, bool bNewTeam)
{
	return true;
}

/* Return a picked team number if none was specified
*/
function byte PickTeam(byte Current, Controller C)
{
	return Current;
}

/* Send a player to a URL.
*/
function SendPlayer( PlayerController aPlayer, string URL )
{
	aPlayer.ClientTravel( URL, TRAVEL_Relative );
}

/** @return the map we should travel to for the next game */
function string GetNextMap();

/* Restart the game.
*/
function RestartGame()
{
	local string NextMap;

	// If we are using arbitration and haven't done the end game handshaking,
	// do that process first and then come back here afterward
	if (bUsingArbitration)
	{
		if (bIsEndGameHandshakeComplete)
		{
			// All arbitrated matches must exit after one match
			NotifyArbitratedMatchEnd();
		}
		return;
	}

	if ( (GameRulesModifiers != None) && GameRulesModifiers.HandleRestartGame() )
		return;

	if ( bGameRestarted )
		return;
	bGameRestarted = true;

	// these server travels should all be relative to the current URL
	if ( bChangeLevels && !bAlreadyChanged )
	{
		// get the next map and start the transition
		bAlreadyChanged = true;

		NextMap = GetNextMap();

		if (NextMap != "")
		{
			WorldInfo.ServerTravel(NextMap);
			return;
		}
	}

	WorldInfo.ServerTravel("?Restart");
}

//==========================================================================
// Message broadcasting functions (handled by the BroadCastHandler)

event Broadcast( Actor Sender, coerce string Msg, optional name Type )
{
	BroadcastHandler.Broadcast(Sender,Msg,Type);
}

function BroadcastTeam( Controller Sender, coerce string Msg, optional name Type )
{
	BroadcastHandler.BroadcastTeam(Sender,Msg,Type);
}

/*
 Broadcast a localized message to all players.
 Most message deal with 0 to 2 related PRIs.
 The LocalMessage class defines how the PRI's and optional actor are used.
*/
event BroadcastLocalized( actor Sender, class<LocalMessage> Message, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	BroadcastHandler.AllowBroadcastLocalized(Sender,Message,Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject);
}

/*
 Broadcast a localized message to all players on a team.
 Most message deal with 0 to 2 related PRIs.
 The LocalMessage class defines how the PRI's and optional actor are used.
*/
event BroadcastLocalizedTeam( int TeamIndex, actor Sender, class<LocalMessage> Message, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	BroadcastHandler.AllowBroadcastLocalizedTeam(TeamIndex, Sender,Message,Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject);
}

//==========================================================================
function bool CheckModifiedEndGame(PlayerReplicationInfo Winner, string Reason)
{
	return ( (GameRulesModifiers != None) && !GameRulesModifiers.CheckEndGame(Winner, Reason) );
}

function bool CheckEndGame(PlayerReplicationInfo Winner, string Reason)
{
	local Controller P;

	if ( CheckModifiedEndGame(Winner, Reason) )
		return false;

	// all player cameras focus on winner or final scene (picked by gamerules)
	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		P.GameHasEnded();
	}
	return true;
}

/**
 * Overload this method so that your game writes out stats
 */
function WriteOnlineStats();

/**
 * Overload this method so that your game writes out player scores
 */
function WriteOnlinePlayerScores()
{
	local int Index;
	local array<OnlinePlayerScore> PlayerScores;

	if (OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		// Allocate an entry for all players
		PlayerScores.Length = GameReplicationInfo.PRIArray.Length;
		// Iterate through the players building their score data
		for (Index = 0; Index < GameReplicationInfo.PRIArray.Length; Index++)
		{
			// Build the skill data for this player
			PlayerScores[Index].PlayerId = GameReplicationInfo.PRIArray[Index].UniqueId;
			if (bTeamGame)
			{
				PlayerScores[Index].TeamId = GameReplicationInfo.PRIArray[Index].Team.TeamIndex;
				PlayerScores[Index].Score = GameReplicationInfo.PRIArray[Index].Team.Score;
			}
			else
			{
				PlayerScores[Index].TeamId = Index;
				PlayerScores[Index].Score = GameReplicationInfo.PRIArray[Index].Score;
			}
		}
		// Now write out the scores
		OnlineSub.StatsInterface.WriteOnlinePlayerScores(PlayerScores);
	}
}

/* End of game.
*/
function EndGame( PlayerReplicationInfo Winner, string Reason )
{
	local int Index;

	// don't end game if not really ready
	if ( !CheckEndGame(Winner, Reason) )
	{
		bOverTime = true;
		return;
	}

	// The non-arbitrated route, the server writes all stats
	if (!bUsingArbitration)
	{
		// Write out any online stats
		WriteOnlineStats();
		// Write the player data used in determining skill ratings
		WriteOnlinePlayerScores();
		// Have clients end their games
		GameReplicationInfo.EndGame();
		// Server is handled here
		GameInterface.EndOnlineGame();
	}
	// Arbitrated matches require all participants to report stats
	// This is handled in the end game handshaking process
	else
	{
		GameReplicationInfo.bMatchIsOver = true;
		if (bNeedsEndGameHandshake)
		{
			// Iterate through the inactive list and send them to the clients
			for (Index = 0; Index < InactivePRIArray.Length; Index++)
			{
				InactivePRIArray[Index].bIsInactive = true;
				InactivePRIArray[Index].RemoteRole = InactivePRIArray[Index].default.RemoteRole;
			}
			// Delay a bit so that replication can happen before processing
			SetTimer(2.0,false,'ProcessEndGameHandshake');
			bNeedsEndGameHandshake = false;
			bIsEndGameHandshakeComplete = false;
		}
	}

	bGameEnded = true;
	EndLogging(Reason);
}


function EndLogging(string Reason);	// Stub function

/** returns whether the given Controller StartSpot property should be used as the spawn location for its Pawn */
function bool ShouldSpawnAtStartSpot(Controller Player)
{
	return ( WorldInfo.NetMode == NM_Standalone && Player != None && Player.StartSpot != None &&
	     (bWaitingToStartMatch || (Player.PlayerReplicationInfo != None && Player.PlayerReplicationInfo.bWaitingPlayer)) );
}

/** FindPlayerStart()
* Return the 'best' player start for this player to start from.  PlayerStarts are rated by RatePlayerStart().
* @param Player is the controller for whom we are choosing a playerstart
* @param InTeam specifies the Player's team (if the player hasn't joined a team yet)
* @param IncomingName specifies the tag of a teleporter to use as the Playerstart
* @returns NavigationPoint chosen as player start (usually a PlayerStart)
 */
function NavigationPoint FindPlayerStart( Controller Player, optional byte InTeam, optional string IncomingName )
{
	local NavigationPoint N, BestStart;
	local Teleporter Tel;

	// allow GameRulesModifiers to override playerstart selection
	if ( GameRulesModifiers != None )
	{
		N = GameRulesModifiers.FindPlayerStart(Player,InTeam,incomingName);
		if ( N != None )
			return N;
	}

	// if incoming start is specified, then just use it
	if( incomingName!="" )
	{
		ForEach WorldInfo.AllNavigationPoints( class 'Teleporter', Tel )
			if( string(Tel.Tag)~=incomingName )
				return Tel;
	}

	// always pick StartSpot at start of match
    if (ShouldSpawnAtStartSpot(Player))
	{
		return Player.StartSpot;
	}

	BestStart = ChoosePlayerStart(Player, InTeam);

	if ( (BestStart == None) && (Player == None) )
	{
		// no playerstart found, so pick any NavigationPoint to keep player from failing to enter game
		`log("Warning - PATHS NOT DEFINED or NO PLAYERSTART with positive rating");
		ForEach AllActors( class 'NavigationPoint', N )
		{
			BestStart = N;
			break;
		}
	}
	return BestStart;
}

/** ChoosePlayerStart()
* Return the 'best' player start for this player to start from.  PlayerStarts are rated by RatePlayerStart().
* @param Player is the controller for whom we are choosing a playerstart
* @param InTeam specifies the Player's team (if the player hasn't joined a team yet)
* @returns NavigationPoint chosen as player start (usually a PlayerStart)
 */
function PlayerStart ChoosePlayerStart( Controller Player, optional byte InTeam )
{
	local PlayerStart P, BestStart;
	local float BestRating, NewRating;
	local byte Team;

	// use InTeam if player doesn't have a team yet
	Team = ( (Player != None) && (Player.PlayerReplicationInfo != None) && (Player.PlayerReplicationInfo.Team != None) )
			? byte(Player.PlayerReplicationInfo.Team.TeamIndex)
			: InTeam;

	// Find best playerstart
	foreach WorldInfo.AllNavigationPoints(class'PlayerStart', P)
	{
		NewRating = RatePlayerStart(P,Team,Player);
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	return BestStart;
}

/** RatePlayerStart()
* Return a score representing how desireable a playerstart is.
* @param P is the playerstart being rated
* @param Team is the team of the player choosing the playerstart
* @param Player is the controller choosing the playerstart
* @returns playerstart score
*/
function float RatePlayerStart(PlayerStart P, byte Team, Controller Player)
{
	if ( !P.bEnabled )
		return 10;
	return ( P.bPrimaryStart ? 100 : 20 );
}

function AddObjectiveScore(PlayerReplicationInfo Scorer, Int Score)
{
	if ( Scorer != None )
	{
		Scorer.Score += Score;
	}
	if ( GameRulesModifiers != None )
		GameRulesModifiers.ScoreObjective(Scorer,Score);
}

function ScoreObjective(PlayerReplicationInfo Scorer, Int Score)
{
	AddObjectiveScore(Scorer, Score);
	CheckScore(Scorer);
}

/* CheckScore()
see if this score means the game ends
*/
function bool CheckScore(PlayerReplicationInfo Scorer)
{
	return ( (GameRulesModifiers == None) || GameRulesModifiers.CheckScore(Scorer) );
}

function ScoreKill(Controller Killer, Controller Other)
{
	if( (killer == Other) || (killer == None) )
	{
    	if ( (Other!=None) && (Other.PlayerReplicationInfo != None) )
		{
			Other.PlayerReplicationInfo.Score -= 1;
			Other.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
	}
	else if ( killer.PlayerReplicationInfo != None )
	{
		Killer.PlayerReplicationInfo.Score += 1;
		Killer.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		Killer.PlayerReplicationInfo.Kills++;
	}

	if ( GameRulesModifiers != None )
		GameRulesModifiers.ScoreKill(Killer, Other);

    if ( (Killer != None) || (MaxLives > 0) )
	{
		CheckScore(Killer.PlayerReplicationInfo);
	}
}

// - Parse out % vars for various messages

static function string ParseMessageString(Controller Who, String Message)
{
	return Message;
}

function DriverEnteredVehicle(Vehicle V, Pawn P)
{
	if ( BaseMutator != None )
		BaseMutator.DriverEnteredVehicle(V, P);
}

function bool CanLeaveVehicle(Vehicle V, Pawn P)
{
	if ( BaseMutator == None )
		return true;
	return BaseMutator.CanLeaveVehicle(V, P);
}

function DriverLeftVehicle(Vehicle V, Pawn P)
{
	if ( BaseMutator != None )
		BaseMutator.DriverLeftVehicle(V, P);
}

exec function KillBots();

function bool PlayerCanRestartGame( PlayerController aPlayer )
{
	return true;
}

// Player Can be restarted ?
function bool PlayerCanRestart( PlayerController aPlayer )
{
	return true;
}

// Returns whether a mutator should be allowed with this gametype
static function bool AllowMutator( string MutatorClassName )
{
	return !class'WorldInfo'.static.IsDemoBuild();
}


function bool AllowCheats(PlayerController P)
{
	return ( WorldInfo.NetMode == NM_Standalone );
}

/**
 * Called from C++'s CommitMapChange before unloading previous level
 * @param PreviousMapName Name of the previous persistent level
 * @param NextMapName Name of the persistent level being streamed to
 */
event PreCommitMapChange(string PreviousMapName, string NextMapName);

/**
 * Called from C++'s CommitMapChange after unloading previous level and loading new level+sublevels
 */
event PostCommitMapChange();

/** AddInactivePRI()
* Add PRI to the inactive list, remove from the active list
*/
function AddInactivePRI(PlayerReplicationInfo PRI, PlayerController PC)
{
	local int i;
	local PlayerReplicationInfo NewPRI;
	local bool bIsConsole;

	NewPRI = PRI.Duplicate();
	WorldInfo.GRI.RemovePRI(NewPRI);

	// make PRI inactive
	NewPRI.RemoteRole = ROLE_None;

	// delete after 5 minutes
	NewPRI.LifeSpan = 300;

	// On console, we have to check the unique net id as network address isn't valid
	bIsConsole = WorldInfo.IsConsoleBuild();

	// make sure no duplicates
    for (i=0; i<InactivePRIArray.Length; i++)
    {
		if ( (InactivePRIArray[i] == None) || InactivePRIArray[i].bDeleteMe ||
//@todo joeg -- Change this once the PC master server is issuing net ids
			(!bIsConsole && (InactivePRIArray[i].SavedNetworkAddress == NewPRI.SavedNetworkAddress)) ||
			(bIsConsole && InactivePRIArray[i].AreUniqueNetIdsEqual(NewPRI)) )
		{
			InactivePRIArray.Remove(i,1);
			i--;
		}
    }
    InactivePRIArray[InactivePRIArray.Length] = NewPRI;

	// cap at 16 saved PRIs
	if ( InactivePRIArray.Length > 16 )
		InactivePRIArray.Remove(0, InactivePRIArray.Length - 16);

	PRI.Destroy();
	// Readjust the skill rating now that this player has left
	RecalculateSkillRating();
}

/** FindInactivePRI()
* returns the PRI associated with this re-entering player
*/
function bool FindInactivePRI(PlayerController PC)
{
	local string NewNetworkAddress, NewName;
	local int i;
	local PlayerReplicationInfo OldPRI;
	local bool bIsConsole;

	// On console, we have to check the unique net id as network address isn't valid
	bIsConsole = WorldInfo.IsConsoleBuild();

	NewNetworkAddress = PC.PlayerReplicationInfo.SavedNetworkAddress;
	NewName = PC.PlayerReplicationInfo.PlayerName;
    for (i=0; i<InactivePRIArray.Length; i++)
    {
		if ( (InactivePRIArray[i] == None) || InactivePRIArray[i].bDeleteMe )
		{
			InactivePRIArray.Remove(i,1);
			i--;
		}
//@todo joeg -- Change this once the PC master server is issuing net ids
		else if ( (bIsConsole && InactivePRIArray[i].AreUniqueNetIdsEqual(PC.PlayerReplicationInfo)) ||
			(!bIsConsole && (InactivePRIArray[i].SavedNetworkAddress ~= NewNetworkAddress) && (InactivePRIArray[i].PlayerName ~= NewName)) )
		{
			// found it!
			OldPRI = PC.PlayerReplicationInfo;
			PC.PlayerReplicationInfo = InactivePRIArray[i];
			PC.PlayerReplicationInfo.SetOwner(PC);
			PC.PlayerReplicationInfo.RemoteRole = ROLE_SimulatedProxy;
			PC.PlayerReplicationInfo.Lifespan = 0;
			OverridePRI(PC, OldPRI);
			WorldInfo.GRI.AddPRI(PC.PlayerReplicationInfo);
			InactivePRIArray.Remove(i,1);
			OldPRI.Destroy();
			return true;
		}
	}
	return false;
}

/** OverridePRI()
* override as needed properties of NewPRI with properties from OldPRI which were assigned during the login process
*/
function OverridePRI(PlayerController PC, PlayerReplicationInfo OldPRI)
{
	PC.PlayerReplicationInfo.OverrideWith(OldPRI);
}

/** called on server during seamless level transitions to get the list of Actors that should be moved into the new level
 * PlayerControllers, Role < ROLE_Authority Actors, and any non-Actors that are inside an Actor that is in the list
 * (i.e. Object.Outer == Actor in the list)
 * are all autmoatically moved regardless of whether they're included here
 * only dynamic (!bStatic and !bNoDelete) actors in the PersistentLevel may be moved (this includes all actors spawned during gameplay)
 * this is called for both parts of the transition because actors might change while in the middle (e.g. players might join or leave the game)
 * @see also PlayerController::GetSeamlessTravelActorList() (the function that's called on clients)
 * @param bToEntry true if we are going from old level -> entry, false if we are going from entry -> new level
 * @param ActorList (out) list of actors to maintain
 */
event GetSeamlessTravelActorList(bool bToEntry, out array<Actor> ActorList)
{
	local int i;

	// always keep PlayerReplicationInfos and TeamInfos, so that after we restart we can keep players on the same team, etc
	for (i = 0; i < WorldInfo.GRI.PRIArray.Length; i++)
	{
		ActorList[ActorList.length] = WorldInfo.GRI.PRIArray[i];
	}

	if (bToEntry)
	{
		// keep general game state until we transition to the final destination
		ActorList[ActorList.length] = WorldInfo.GRI;
		if (BroadcastHandler != None)
		{
			ActorList[ActorList.length] = BroadcastHandler;
		}
	}

	if (BaseMutator != None)
	{
		BaseMutator.GetSeamlessTravelActorList(bToEntry, ActorList);
	}
}

/** used to swap a viewport/connection's PlayerControllers when seamless travelling and the new gametype's
 * controller class is different than the previous
 * includes network handling
 * @param OldPC - the old PC that should be discarded
 * @param NewPC - the new PC that should be used for the player
 */
native final function SwapPlayerControllers(PlayerController OldPC, PlayerController NewPC);

/** called after a seamless level transition has been completed on the *new* GameInfo
 * used to reinitialize players already in the game as they won't have *Login() called on them
 */
event PostSeamlessTravel()
{
	local Controller C;

	// handle players that are already loaded
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (C.bIsPlayer && (PlayerController(C) == None || PlayerController(C).HasClientLoadedCurrentWorld()))
		{
			HandleSeamlessTravelPlayer(C);
		}
	}

	if (bWaitingToStartMatch && !bDelayedStart && NumPlayers + NumBots > 0)
	{
		StartMatch();
	}
}

/** handles reinitializing players that remained through a seamless level transition
 * called from C++ for players that finished loading after the server
 * @param C the Controller to handle
 */
event HandleSeamlessTravelPlayer(Controller C)
{
	local rotator StartRotation;
	local NavigationPoint StartSpot;
	local PlayerController PC, NewPC;
	local PlayerReplicationInfo OldPRI;

	`log(">> GameInfo::HandleSeamlessTravelPlayer:" @ C,,'SeamlessTravel');

	PC = PlayerController(C);
	if (PC != None && PC.Class != PlayerControllerClass)
	{
		// we need to spawn a new PlayerController to replace the old one
		NewPC = Spawn(PlayerControllerClass,,, PC.Location, PC.Rotation);
		if (NewPC == None)
		{
			`Warn("Failed to spawn new PlayerController for" @ PC.GetHumanReadableName() @ "(old class" @ PC.Class $ ")");
			PC.Destroy();
			return;
		}
		else
		{
			NewPC.SeamlessTravelFrom(PC);
			SwapPlayerControllers(PC, NewPC);
			PC = NewPC;
		}
	}
	else
	{
		// clear out data that was only for the previous game
		C.PlayerReplicationInfo.Reset();
		// create a new PRI and copy over info; this is necessary because the old gametype may have used a different PRI class
		OldPRI = C.PlayerReplicationInfo;
		C.InitPlayerReplicationInfo();
		OldPRI.CopyProperties(C.PlayerReplicationInfo);
		// we don't need the old PRI anymore
		//@fixme: need a way to replace PRIs that doesn't cause incorrect "player left the game"/"player entered the game" messages
		OldPRI.Destroy();
	}

	// get rid of team if this is not a team game
	if (!bTeamGame && C.PlayerReplicationInfo.Team != None)
	{
		C.PlayerReplicationInfo.Team.Destroy();
		C.PlayerReplicationInfo.Team = None;
	}

	// Find a start spot.
	StartSpot = FindPlayerStart(C, C.GetTeamNum());

	if (StartSpot == None)
	{
		`warn(GameMessageClass.Default.FailedPlaceMessage);
	}
	else
	{
		StartRotation.Yaw = StartSpot.Rotation.Yaw;
		C.SetLocation(StartSpot.Location);
		C.SetRotation(StartRotation);
	}

	C.StartSpot = StartSpot;

	if (PC != None)
	{
		// tell the player controller to register its data stores again
		PC.ClientInitializeDataStores();

		if (PC.PlayerReplicationInfo.bOnlySpectator)
		{
			PC.GotoState('Spectating');
			PC.PlayerReplicationInfo.bIsSpectator = true;
			PC.PlayerReplicationInfo.bOutOfLives = true;
			NumSpectators++;
		}
		else
		{
			NumPlayers++;
			PC.GotoState('PlayerWaiting');
		}

		// tell client what hud and scoreboard to use
		PC.ClientSetHUD(HudType, ScoreboardType);

		ReplicateStreamingStatus(PC);

		// tell the player about the music, if any
		PC.ServerSendMusicInfo();

		// see if we need to spawn a CoverReplicator for this player
		if (CoverReplicatorBase != None)
		{
			PC.SpawnCoverReplicator();
		}

		// Set the rich presence strings on the client (has to be done there)
		PC.ClientSetRichPresence();
	}
	else
	{
		NumBots++;
		C.GotoState('RoundEnded');
	}

	if (BaseMutator != None)
	{
		BaseMutator.NotifyLogin(C);
	}

	`log("<< GameInfo::HandleSeamlessTravelPlayer:" @ C,,'SeamlessTravel');
}

/**
 * Updates the online subsystem's information for player counts so that
 * LAN matches can show the correct player counts
 */
function UpdateGameSettingsCounts()
{
	if (GameSettings != None && GameSettings.bIsLanMatch)
	{
		// Update the number of open slots available
		GameSettings.NumOpenPublicConnections = GameSettings.NumPublicConnections - NumPlayers;
		if (GameSettings.NumOpenPublicConnections < 0)
		{
			GameSettings.NumOpenPublicConnections = 0;
		}
	}
}

/**
 * This is a base (empty) implementation of the completion notification
 *
 * @param PC the player controller to mark as done
 * @param bWasSuccessful whether the PC was able to register for arbitration or not
 */
function ProcessClientRegistrationCompletion(PlayerController PC,bool bWasSuccessful);

/**
 * Empty implementation of the code that kicks off async registration
 */
function StartArbitrationRegistration();

/**
 * Empty implementation of the code that starts an arbitrated match
 */
function StartArbitratedMatch();

/**
 * Empty implementation of the code that registers the server for arbitration
 */
function RegisterServerForArbitration();

/**
 * Empty implementation of the code that handles the callback for completion
 */
function ArbitrationRegistrationComplete(bool bWasSuccessful);

function bool MatchIsInProgress()
{
	return true;
}

/**
 * This state is used to change the flow of start/end match to handle arbitration
 *
 * Basic flow of events:
 *		Server prepares to start the match and tells all clients to register arbitration
 *		Clients register with arbitration and tell the server when they are done
 *		Server checks for all clients to be registered and kicks any clients if
 *			they don't register in time.
 *		Server registers with arbitration and the match begins
 *
 *		Match ends and the server tells connected clients to write arbitrated stats
 *		Clients write stats and notifies server of completion
 *		Server writes stats and ends the match
 */
auto State PendingMatch
{
	function bool MatchIsInProgress()
	{
		return false;
	}

	/**
	 * Tells all of the currently connected clients to register with arbitration.
	 * The clients will call back to the server once they have done so, which
	 * will tell this state to see if it is time for the server to register with
	 * arbitration.
	 */
	function StartMatch()
	{
		if (bUsingArbitration)
		{
			StartArbitrationRegistration();
		}
		else
		{
			Global.StartMatch();
		}
	}

	/**
	 * Kicks off the async tasks of having the clients register with
	 * arbitration before the server does. Sets a timeout for when
	 * all slow to respond clients get kicked
	 */
	function StartArbitrationRegistration()
	{
		local PlayerController PC;
		local UniqueNetId HostId;

		if (!bHasArbitratedHandshakeBegun)
		{
			// Tell PreLogin() to reject new connections
			bHasArbitratedHandshakeBegun = true;

			// Find the host's net id
			foreach LocalPlayerControllers(class'PlayerController', PC)
			{
				HostId = PC.PlayerReplicationInfo.UniqueId;
				break;
			}

			PendingArbitrationPCs.Length = 0;
			// Iterate the controller list and tell them to register with arbitration
			foreach WorldInfo.AllControllers(class'PlayerController', PC)
			{
				// Skip local PCs as they are handled automatically
				if (!PC.IsLocalPlayerController())
				{
					PC.ClientSetHostUniqueId(HostId);
					PC.ClientRegisterForArbitration();
					// Add to the pending list
					PendingArbitrationPCs[PendingArbitrationPCs.Length] = PC;
				}
			}
			// Start the kick timer
			SetTimer(ArbitrationHandshakeTimeout,false,'ArbitrationTimeout');
		}
	}

	/**
	 * Does the registration for the server. This must be done last as it
	 * includes all the players info from their registration
	 */
	function RegisterServerForArbitration()
	{
		if (GameInterface != None)
		{
			GameInterface.AddArbitrationRegistrationCompleteDelegate(ArbitrationRegistrationComplete);
			GameInterface.RegisterForArbitration();
		}
		else
		{
			// Fake as working without subsystem
			ArbitrationRegistrationComplete(true);
		}
	}

	/**
	 * Callback from the server that starts the match if the registration was
	 * successful. If not, it goes back to the menu
	 *
	 * @param bWasSuccessful whether the registration worked or not
	 */
	function ArbitrationRegistrationComplete(bool bWasSuccessful)
	{
		// Clear the delegate so we don't leak with GC
		GameInterface.AddArbitrationRegistrationCompleteDelegate(ArbitrationRegistrationComplete);
		if (bWasSuccessful)
		{
			// Start the match
			StartArbitratedMatch();
		}
		else
		{
			ConsoleCommand("Disconnect");
		}
	}

	/**
	 * Handles kicking any clients that haven't completed handshaking
	 */
	function ArbitrationTimeout()
	{
		local int Index;

		// Kick any pending players
		for (Index = 0; Index < PendingArbitrationPCs.Length; Index++)
		{
			AccessControl.KickPlayer(PendingArbitrationPCs[Index],GameMessageClass.Default.MaxedOutMessage);
		}
		PendingArbitrationPCs.Length = 0;
		// Do the server registration now that any remaining clients are kicked
		RegisterServerForArbitration();
	}

	function StartArbitratedMatch()
	{
		bNeedsEndGameHandshake = true;
		// Start the match
		Global.StartMatch();
	}

	/**
	 * Removes the player controller from the pending list. Kicks that PC if it
	 * failed to register for arbitration. Starts the match if all clients have
	 * completed their registration
	 *
	 * @param PC the player controller to mark as done
	 * @param bWasSuccessful whether the PC was able to register for arbitration or not
	 */
	function ProcessClientRegistrationCompletion(PlayerController PC,bool bWasSuccessful)
	{
		local int FoundIndex;

		// Search for the specified PC and remove if found
		FoundIndex = PendingArbitrationPCs.Find(PC);
		if (FoundIndex != INDEX_NONE)
		{
			PendingArbitrationPCs.Remove(FoundIndex,1);
			if (bWasSuccessful)
			{
				// Add to the completed list
				ArbitrationPCs[ArbitrationPCs.Length] = PC;
			}
			else
			{
				AccessControl.KickPlayer(PC,GameMessageClass.Default.MaxedOutMessage);
			}
		}
		// Start the match if all clients have responded
		if (PendingArbitrationPCs.Length == 0)
		{
			// Clear the kick timer
			SetTimer(0,false,'ArbitrationTimeout');
			RegisterServerForArbitration();
		}
	}

	event EndState(name NextStateName)
	{
		// Clear the kick timer
		SetTimer(0,false,'ArbitrationTimeout');

		if( GameInterface != None )
		{
			GameInterface.ClearArbitrationRegistrationCompleteDelegate(ArbitrationRegistrationComplete);
		}
	}
}

/**
 * Iterates the arbitrated PCs list and tells them to write their end of
 * game data to arbitration
 */
function ProcessEndGameHandshake()
{
	local int Index;

	bHasEndGameHandshakeBegun = true;
	// Iterate the controller list and tell them to register with arbitration
	for (Index = 0; Index < ArbitrationPCs.Length; Index++)
	{
		if (!ArbitrationPCs[Index].bDeleteMe)
		{
			// Tell the client to write stats out
			ArbitrationPCs[Index].ClientWriteArbitrationEndGameData(OnlineStatsWriteClass);
			// Add to the pending list
			PendingArbitrationPCs[PendingArbitrationPCs.Length] = ArbitrationPCs[Index];
		}
	}
	// If we aren't waiting for any clients to process data, do our processing now
	if (PendingArbitrationPCs.Length == 0)
	{
		ServerWriteArbitrationEndGameData();
	}
	else
	{
		SetTimer(5.0,false,'ServerWriteArbitrationEndGameData');
	}
}

/**
 * Removes the specified client from the pending list and tells the game to
 * restart once all clients have notified
 *
 * @param PC the player controller that completed the write
 */
function ProcessClientDataWriteCompletion(PlayerController PC)
{
	local int FoundIndex;

	// Search for the specified PC and remove if found
	FoundIndex = PendingArbitrationPCs.Find(PC);
	if (FoundIndex != INDEX_NONE)
	{
		PendingArbitrationPCs.Remove(FoundIndex,1);
	}
	// If all clients have responded, tell the server to write its data which
	// will then restart the game upon completion
	if (PendingArbitrationPCs.Length == 0)
	{
		ServerWriteArbitrationEndGameData();
	}
}

/**
 * Commits arbitration data for the server and restarts the match
 */
function ServerWriteArbitrationEndGameData()
{
	SetTimer(0.0,false,'ServerWriteArbitrationEndGameData');
	// Write all of the game stats
	WriteOnlineStats();
	// Write out any player scoring data
	WriteOnlinePlayerScores();
	// Now finalize the data before closing the session down
	if (GameInterface != None)
	{
		// Force the flush of the stats
		GameInterface.EndOnlineGame();
	}
	// Clean up handshaking state
	bIsEndGameHandshakeComplete = true;
	PendingArbitrationPCs.Length = 0;
	ArbitrationPCs.Length = 0;
	// Start the next match
	RestartGame();
}

/**
 * Tells all clients to disconnect and then goes to the menu
 */
function NotifyArbitratedMatchEnd()
{
	local PlayerController PC;
	// Iterate through the controllers telling them to disconnect
	foreach WorldInfo.AllControllers(class'PlayerController',PC)
	{
		if (PC.IsLocalPlayerController() == false)
		{
			PC.ClientArbitratedMatchEnded();
		}
	}
	// Iterate through local controllers telling them to disconnect
	foreach WorldInfo.AllControllers(class'PlayerController',PC)
	{
		if (PC.IsLocalPlayerController())
		{
			PC.ClientArbitratedMatchEnded();
		}
	}
}

/**
 * Used to notify the game type that it is ok to update a player's gameplay
 * specific muting information now. The playercontroller needs to notify
 * the server when it is possible to do so or the unique net id will be
 * incorrect and the muting not work.
 *
 * @param PC the playercontroller that is ready for updates
 */
function UpdateGameplayMuteList(PlayerController PC);

/**
 * Used by the game type to update the advertised skill for this game
 */
function RecalculateSkillRating()
{
	local int Index;
	local array<UniqueNetId> Players;
	local UniqueNetId ZeroId;

	if (WorldInfo.NetMode != NM_Standalone &&
		OnlineSub != None &&
		OnlineSub.GameInterface != None)
	{
		// Iterate through the players adding their unique id for skill calculation
		for (Index = 0; Index < GameReplicationInfo.PRIArray.Length; Index++)
		{
			if (ZeroId != GameReplicationInfo.PRIArray[Index].UniqueId)
			{
				Players[Players.Length] = GameReplicationInfo.PRIArray[Index].UniqueId;
			}
		}
		// Update the skill rating with the list of players
		OnlineSub.GameInterface.RecalculateSkillRating(Players);
	}
}

/** Called when this PC is in cinematic mode, and its matinee is cancelled by the user. */
event MatineeCancelled();


/**
 * Checks for the login parameters being passed on the command line. If
 * present, it does an async login before starting the dedicated server
 * registration process
 *
 * @return true if the login is in progress, false otherwise
 */
function bool ProcessServerLogin()
{
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface != None)
		{
			OnlineSub.PlayerInterface.AddLoginChangeDelegate(OnLoginChange);
			OnlineSub.PlayerInterface.AddLoginFailedDelegate(0,OnLoginFailed);
			// Check the command line for login information and login async
			if (OnlineSub.PlayerInterface.AutoLogin() == false)
			{
				ClearAutoLoginDelegates();
				return false;
			}
			return true;
		}
	}
	return false;
}

/**
 * Clears the login delegates once the login process has passed or failed
 */
function ClearAutoLoginDelegates()
{
	if (OnlineSub.PlayerInterface != None)
	{
		OnlineSub.PlayerInterface.ClearLoginChangeDelegate(OnLoginChange);
		OnlineSub.PlayerInterface.ClearLoginFailedDelegate(0,OnLoginFailed);
	}
}

/**
 * Called if the autologin fails
 *
 * @param LocalUserNum the controller number of the associated user
 * @param ErrorCode the async error code that occurred
 */
function OnLoginFailed(byte LocalUserNum,EOnlineServerConnectionStatus ErrorCode)
{
	ClearAutoLoginDelegates();
}

/**
 * Used to tell the game when the autologin has completed
 */
function OnLoginChange()
{
	ClearAutoLoginDelegates();
	// The login has completed so start the dedicated server
	RegisterServer();
}

/**
 * Registers the dedicated server with the online service
 */
function RegisterServer()
{
	if (OnlineGameSettingsClass != None && OnlineSub != None && OnlineSub.GameInterface != None)
	{
		// Create the default settings to get the standard settings to advertise
		GameSettings = new OnlineGameSettingsClass;
		// Serialize any custom settings from the URL
		GameSettings.UpdateFromURL(ServerOptions);
		// Register the delegate so we can see when it's done
		OnlineSub.GameInterface.AddCreateOnlineGameCompleteDelegate(OnServerCreateComplete);
		// Now kick off the async publish
		OnlineSub.GameInterface.CreateOnlineGame(0,GameSettings);
	}
	else
	{
		`Warn("No game settings to register with the online service. Game won't be advertised");
	}
}

/**
 * Notifies us of the game being registered successfully or not
 *
 * @param bWasSuccessful flag telling us whether it worked or not
 */
function OnServerCreateComplete(bool bWasSuccessful)
{
	OnlineSub.GameInterface.ClearCreateOnlineGameCompleteDelegate(OnServerCreateComplete);
	if (bWasSuccessful == false)
	{
		if (GameSettings.bIsLanMatch == false)
		{
			`Warn("Failed to register game with online service. Registering as a LAN match");
			// Force to be a LAN match
			GameSettings.bIsLanMatch = true;
			// Register the delegate so we can see when it's done
			OnlineSub.GameInterface.AddCreateOnlineGameCompleteDelegate(OnServerCreateComplete);
			// Now kick off the async publish
			OnlineSub.GameInterface.CreateOnlineGame(0,GameSettings);
		}
		else
		{
			`Warn("Failed to register game with online service. Game won't be advertised");
		}
	}
}

defaultproperties
{
	// The game spawns bots/players which can't be done during physics ticking
	TickGroup=TG_PreAsyncWork

	GameSpeed=1.0
	bDelayedStart=true
	HUDType=class'Engine.HUD'
	bWaitingToStartMatch=false
	bLoggingGame=False
    bRestartLevel=True
    bPauseable=True
	AccessControlClass=class'Engine.AccessControl'
	BroadcastHandlerClass=class'Engine.BroadcastHandler'
	DeathMessageClass=class'LocalMessage'
	PlayerControllerClass=class'Engine.PlayerController'
	GameMessageClass=class'GameMessage'
	GameReplicationInfoClass=class'GameReplicationInfo'
    FearCostFalloff=+0.95
	CurrentID=1
	PlayerReplicationInfoClass=Class'Engine.PlayerReplicationInfo'
	bAllowVehicles=true
	MaxSpectatorsAllowed=32
	MaxPlayersAllowed=32

	Components.Remove(Sprite)
}
