/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTGame extends GameInfo
	config(Game)
	abstract
	native;

/** if set, when this class is compiled, a menu entry for it will be automatically added/updated in its package.ini file
 * (abstract classes are skipped even if this flag is set)
 */
var bool bExportMenuData;

var bool				bWeaponStay;              // Whether or not weapons stay when picked up.
var bool				bTeamScoreRounds;
var bool				bSoaking;
var bool				bPlayersVsBots;
var bool				bCustomBots;
var bool				bNoCustomCharacters;

var string CallSigns[15];
var string Acronym;
var localized string Description;

var globalconfig	int		ServerSkillLevel;	// The Server Skill Level ( 0 - 2, Beginner/Experienced/Expert )
var globalconfig	float	EndTimeDelay;
var globalconfig	float	BotRatio;			// only used when bPlayersVsBots is true
var config			int 	NetWait;       // time to wait for players in netgames w/ bWaitForNetPlayers (typically team games)
/** how long we wait for clients to perform initial processing at the start of the game (UTPlayerController::bInitialProcessingComplete) */
var config int ClientProcessingTimeout;
var globalconfig	int		MinNetPlayers; // how many players must join before net game will start
var globalconfig	int		RestartWait;

var					bool	bAutoNumBots;			// Match bots to map's recommended bot count
var globalconfig	bool	bPlayersMustBeReady;	// players must confirm ready for game to start
var globalconfig	bool	bForceRespawn;			/** Force dead players to respawn immediately if true (configurable) */
var					bool	bTempForceRespawn;		/** Temporary (used in game) version of bForceRespawn */
var globalconfig	bool    bWaitForNetPlayers;     // wait until more than MinNetPlayers players have joined before starting match

var config bool bWarmupRound;			// If true, this match will have a warmup round
var config int WarmupTime;				// How long is the warmup round (In Seconds)
var					int		WarmupRemaining;		// How much time left in the Warmup Round
var globalconfig	bool	bConsoleStartWithWeapons;	// @TODO FIXMESTEVE remove

var bool	bFirstBlood;
var bool	bQuickStart;
var bool	bSkipPlaySound;		// override "play!" sound
var bool	bStartedCountDown;
var bool	bFinalStartup;
var bool	bOverTimeBroadcast;
var bool bMustHaveMultiplePlayers;
var bool bPlayerBecameActive;
var bool    bMustJoinBeforeStart;   // players can only spectate if they join after the game starts
var config bool			bAdjustSkill;
var bool	bDemoMode;				// turn off HUD, etc.
/** whether not yet driven vehicles can take damage */
var bool bUndrivenVehicleDamage;

var byte StartupStage;              // what startup message to display
var int DesiredPlayerCount;			// bots will fill in to reach this value as needed

var float		SpawnProtectionTime;
var int			DefaultMaxLives;
var config int			LateEntryLives;	// defines how many lives in a player can still join

var int RemainingTime, ElapsedTime;
var int CountDown;
var float AdjustedDifficulty;
var int PlayerKills, PlayerDeaths;

var NavigationPoint LastPlayerStartSpot;    // last place current player looking for start spot started from
var NavigationPoint LastStartSpot;          // last place any player started from

var float EndTime;
var int             EndMessageWait;         // wait before playing which team won the match
var transient int   EndMessageCounter;      // end message counter

var   string			      RulesMenuType;			// Type of rules menu to display.
var   string				  GameUMenuType;			// Type of Game dropdown to display.

var actor EndGameFocus;

var() int                     ResetCountDown;
var() config int              ResetTimeDelay;           // time (seconds) before restarting teams

var UTVehicle VehicleList;

var UTTeamInfo EnemyRoster;
var string EnemyRosterName;
var string DefaultEnemyRosterClass;

var string MaleBackupNames[32];
var string FemaleBackupNames[32];
var int MaleBackupNameOffset, FemaleBackupNameOffset;

/** Default inventory added via AddDefaultInventory() */
var array< class<Inventory> >	DefaultInventory;

// translocator
var bool	bAllowTranslocator;
var UTProj_TransDisc BeaconList;
/** class to add to default inventory when translocator is enabled (translocator can't be enabled unless this has a value) */
var class<Inventory> TranslocatorClass;

// hoverboard
var			bool	bAllowHoverboard;

// taunts
var int LastTaunt[2];

var class<UTVictoryMessage>	VictoryMessageClass;
struct native GameTypePrefix
{
	/** map prefix, e.g. "DM" */
	var string Prefix;
	/** gametype used if none specified on the URL */
	var string GameType;
	/** additional gametypes supported by this map prefix via the URL (used for cooking) */
	var array<string> AdditionalGameTypes;
};
var array<GameTypePrefix> MapPrefixes; /** Used for loading appropriate game type if non-specified in URL */
var globalconfig array<GameTypePrefix> CustomMapPrefixes; /** Used for loading appropriate game type if non-specified in URL */
var   string			      MapPrefix;				// Prefix characters for names of maps for this game type.

// Stats
var UTGameStats                 GameStats;				// Holds the GameStats actor
var globalconfig string			GameStatsClass;			// Type of GameStats actor to spawn

// console server @todo steve temp fixme
var bool	bConsoleServer;

/** PlayerController class to use on consoles */
var class<PlayerController> ConsolePlayerControllerClass;

/** Used by the single player game */
var int SinglePlayerMissionIndex;

/** prefix of filename to record a demo to - a number is added on to get a unique filename (empty string means don't record) */
var string DemoPrefix;

/** class used for AI bots */
var class<UTBot> BotClass;

// These variables are set by the UI on the default object and should be read using the default object of class'UTGame'.

/** Game Map Cycles, there is a map cycle per game mode */
struct native GameMapCycle
{
	var name GameClassName;
	var array<string> Maps;
};
var globalconfig array<GameMapCycle> GameSpecificMapCycles;

/** Array of active bot names. */
struct native ActiveBotInfo
{
	/** name of character */
	var string BotName;
	/** whether the bot is currently in the game */
	var bool bInUse;
};
var globalconfig array<ActiveBotInfo> ActiveBots;


/**** Map Voting ************/

/** If true, users will be presented with a map list to move on at the end of the round */
var config bool bAllowMapVoting;

/** How long the voting will take MAX. */
var config int VoteDuration;

/** The object that manages voting */
var UTVoteCollector VoteCollector;

var UTUIScene MidGameMenuTemplate;


cpptext
{
	virtual void AddSupportedGameTypes(AWorldInfo* Info, const TCHAR* WorldFilename) const;
}

function PreBeginPlay()
{
	InitLogging();
	Super.PreBeginPlay();
}

function PostBeginPlay()
{
	Super.PostBeginPlay();

	// add translocator to default inventory list, if allowed
	if (bAllowTranslocator)
	{
		if (TranslocatorClass != None)
		{
			DefaultInventory[DefaultInventory.Length] = TranslocatorClass;
		}
		else
		{
			bAllowTranslocator = false;
		}
	}

	MaleBackupNameOffset = Rand(32);
	FemaleBackupNameOffset = Rand(32);

	GameReplicationInfo.RemainingTime = RemainingTime;
	if ( bPlayersVsBots )
		UTGameReplicationInfo(GameReplicationInfo).BotDifficulty = GameDifficulty;
}

// Returns whether a mutator should be allowed with this gametype
static function bool AllowMutator( string MutatorClassName )
{
	if ( !Default.bAllowTranslocator && (MutatorClassName ~= "UTGame.UTMutator_NoTranslocator") )
	{
		// already no translocator
		return false;
	}
	if ( !Default.bTeamGame && (MutatorClassName ~= "UTGame.UTMutator_FriendlyFire") )
	{
		// Friendly fire only valid in team games
		return false;
	}
	if (!default.bWeaponStay && MutatorClassName ~= "UTGame.UTMutator_WeaponsRespawn")
	{
		// weapon stay already off
		return false;
	}
	if ( MutatorClassName ~= "UTGameContent.UTMutator_Survival")
	{
		// survival mutator only for Duel
		return false;
	}
	if (MutatorClassName ~= "UTGameContent.UTMutator_NoOrbs")
	{
		// No Orbs mutator only for Warfare
		return false;
	}
	return Super.AllowMutator(MutatorClassName);
}

/**
 * Set up statistics logging
 */
function InitLogging()
{
	local class <UTGameStats> MyGameStatsClass;

	if (ROLE==ROLE_Authority)
	{

		MyGameStatsClass=class<UTGameStats>(DynamicLoadObject(GameStatsClass,class'class'));
	    if (MyGameStatsClass!=None)
	    {
			GameStats = spawn(MyGameStatsClass);
	        if (GameStats==None)
	        	`log("Could to create Stats Object");
	        else
	        	GameStats.Init();
	    }
	    else
	    	`log("Error loading GameStats ["$GameStatsClass$"]");
	}
}

// Parse options for this game...
static event class<GameInfo> SetGameType(string MapName, string Options)
{
	local string ThisMapPrefix;
	local int i,pos;
	local class<GameInfo> NewGameType;
	local string GameOption;


	if (Left(MapName, 9) ~= "EnvyEntry" || Left(MapName, 10) ~= "UTFrontEnd")
	{
		return class'UTEntryGame';
	}

	// allow commandline to override game type setting
	GameOption = ParseOption( Options, "Game");
	if ( GameOption != "" )
	{
		return Default.class;
	}

	// strip the UEDPIE_ from the filename, if it exists (meaning this is a Play in Editor game)
	if (Left(MapName, 7) ~= "UEDPIE_")
	{
		MapName = Right(MapName, Len(MapName) - 7);
	}
	else if (Left(MapName, 10) ~= "UEDPOC_PS3")
	{
		MapName = Right(MapName, Len(MapName) - 10);
	}
	else if (Left(MapName, 12) ~= "UEDPOC_Xenon")
	{
		MapName = Right(MapName, Len(MapName) - 12);
	}

	// replace self with appropriate gametype if no game specified
	pos = InStr(MapName,"-");
	ThisMapPrefix = left(MapName,pos);
	if ( ThisMapPrefix ~= Default.MapPrefix )
		return Default.class;

	// change game type
	for ( i=0; i<Default.MapPrefixes.Length; i++ )
	{
		if ( Default.MapPrefixes[i].Prefix ~= ThisMapPrefix )
		{
			NewGameType = class<GameInfo>(DynamicLoadObject(Default.MapPrefixes[i].GameType,class'Class'));
			if ( NewGameType != None )
			{
				return NewGameType;
			}
		}
	}
	for ( i=0; i<Default.CustomMapPrefixes.Length; i++ )
	{
		if ( Default.CustomMapPrefixes[i].Prefix ~= ThisMapPrefix )
		{
			NewGameType = class<GameInfo>(DynamicLoadObject(Default.CustomMapPrefixes[i].GameType,class'Class'));
			if ( NewGameType != None )
			{
				return NewGameType;
			}
		}
	}

	// if no gametype found, use DemoGame
    return class'DemoGame';
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
    $   MapPrefix
    $   "\\t"
    $   GetNumPlayers()
	$	"/"
	$	MaxPlayers;
}

function bool AllowCheats(PlayerController P)
{
	//@todo steve- check not in SP match
	return Super.AllowCheats(P);
}

function bool BecomeSpectator(PlayerController P)
{
	if ( (P.PlayerReplicationInfo == None) || !GameReplicationInfo.bMatchHasBegun
	     || (NumSpectators >= MaxSpectators) || P.IsInState('RoundEnded') )
	{
		P.ReceiveLocalizedMessage(GameMessageClass, 12);
		return false;
	}

	P.PlayerReplicationInfo.bOnlySpectator = true;
	NumSpectators++;
	NumPlayers--;

	return true;
}

function bool AllowBecomeActivePlayer(PlayerController P)
{
	if ( (P.PlayerReplicationInfo == None) || !GameReplicationInfo.bMatchHasBegun || bMustJoinBeforeStart
	     || (NumPlayers >= MaxPlayers) || (MaxLives > 0) || P.IsInState('RoundEnded') )
	{
		P.ReceiveLocalizedMessage(GameMessageClass, 13);
		return false;
	}
	return true;
}

/** Reset() - reset actor to initial state - used when restarting level without reloading. */
function Reset()
{
	local Controller C;

	Super.Reset();

	bOverTimeBroadcast = false;

	// update AI
	FindNewObjectives(None);

	//now respawn all the players
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (C.PlayerReplicationInfo != None && !C.PlayerReplicationInfo.bOnlySpectator)
		{
			RestartPlayer(C);
		}
	}

	//reset timelimit
	RemainingTime = 60 * TimeLimit;
	// if the round lasted less than one minute, we won't be actually changing RemainingMinute
	// which will prevent it from being replicated, so in that case
	// reduce the time limit by one second to ensure that it is unique
	if (RemainingTime == GameReplicationInfo.RemainingMinute)
	{
		RemainingTime--;
	}
	GameReplicationInfo.RemainingMinute = RemainingTime;
}

function ObjectiveDisabled( UTGameObjective DisabledObjective );
/** re-evaluate objectives for players because the specified one has been changed/completed */
function FindNewObjectives(UTGameObjective DisabledObjective);

function bool CanDisableObjective( UTGameObjective GO )
{
	return true;
}

function bool SkipPlaySound()
{
	return bQuickStart || bSkipPlaySound;
}


//
// Set gameplay speed.
//
function SetGameSpeed( Float T )
{
	local float UTTimeDilation;

	UTTimeDilation = bConsoleServer ? 1.0 : 1.1;
	GameSpeed = FMax(T, 0.1);
	WorldInfo.TimeDilation = UTTimeDilation * GameSpeed;
	SetTimer(WorldInfo.TimeDilation, true);
}

function ScoreKill(Controller Killer, Controller Other)
{
	local PlayerReplicationInfo OtherPRI;
	local UTPawn KillerPawn;

	OtherPRI = Other.PlayerReplicationInfo;
    if ( OtherPRI != None )
    {
		OtherPRI.NumLives++;
		if ( (MaxLives > 0) && (OtherPRI.NumLives >=MaxLives) )
			OtherPRI.bOutOfLives = true;
    }

	/* //@todo steve
	if ( bAllowTaunts && (Killer != None) && (Killer != Other) && Killer.AutoTaunt()
		&& (Killer.PlayerReplicationInfo != None) && (Killer.PlayerReplicationInfo.VoiceType != None) )
	{
		if( Killer.IsA('PlayerController') )
			Killer.SendMessage(OtherPRI, 'AUTOTAUNT', Killer.PlayerReplicationInfo.VoiceType.static.PickRandomTauntFor(Killer, false, false), 10, 'GLOBAL');
		else
			Killer.SendMessage(OtherPRI, 'AUTOTAUNT', Killer.PlayerReplicationInfo.VoiceType.static.PickRandomTauntFor(Killer, false, true), 10, 'GLOBAL');
	}
	*/
    Super.ScoreKill(Killer,Other);

    if ( (killer == None) || (Other == None) )
		return;

	// adjust bot skills to match player - only for DM, not team games
	if ( bAdjustSkill && !bTeamGame && (killer.IsA('PlayerController') || Other.IsA('PlayerController')) )
    {
		if ( killer.IsA('AIController') )
			AdjustSkill(AIController(killer), PlayerController(Other),true);
		if ( Other.IsA('AIController') )
			AdjustSkill(AIController(Other), PlayerController(Killer),false);
    }

	KillerPawn = UTPawn(Killer.Pawn);
	if ( (KillerPawn != None) && KillerPawn.bKillsAffectHead )
	{
		KillerPawn.SetBigHead();
	}
}

function AdjustSkill(AIController B, PlayerController P, bool bWinner)
{
    if ( bWinner )
    {
		PlayerKills += 1;
		AdjustedDifficulty = FMax(0, AdjustedDifficulty - 2.0/FMin(PlayerKills, 10.0));
		if ( B.Skill > AdjustedDifficulty )
		{
			B.Skill = AdjustedDifficulty;
			UTBot(B).ResetSkill();
		}
    }
    else
    {
		PlayerDeaths += 1;
		AdjustedDifficulty = FMin(7.0,AdjustedDifficulty + 2.0/FMin(PlayerDeaths, 10.0));
		if ( B.Skill < AdjustedDifficulty )
			B.Skill = AdjustedDifficulty;
    }
    if ( abs(AdjustedDifficulty - GameDifficulty) >= 1 )
    {
		GameDifficulty = AdjustedDifficulty;
		SaveConfig();
    }
}


// Monitor killed messages for fraglimit
function Killed( Controller Killer, Controller KilledPlayer, Pawn KilledPawn, class<DamageType> damageType )
{
	local bool		bEnemyKill;
	local UTPlayerReplicationInfo KillerPRI, KilledPRI;
	local UTVehicle V;

	if ( UTBot(KilledPlayer) != None )
		UTBot(KilledPlayer).WasKilledBy(Killer);

	if ( Killer != None )
		KillerPRI = UTPlayerReplicationInfo(Killer.PlayerReplicationInfo);
	if ( KilledPlayer != None )
		KilledPRI = UTPlayerReplicationInfo(KilledPlayer.PlayerReplicationInfo);

	bEnemyKill = ( ((KillerPRI != None) && (KillerPRI != KilledPRI) && (KilledPRI != None)) && (!bTeamGame || (KillerPRI.Team != KilledPRI.Team)) );

	if ( KilledPRI != None )
	{
		KilledPRI.LastKillerPRI = KillerPRI;
		if ( KilledPRI.Spree > 4 )
		{
			EndSpree(KillerPRI, KilledPRI);
		}
		else
		{
			KilledPRI.Spree = 0;
		}
		if ( KillerPRI != None )
		{
			KillerPRI.LogMultiKills(bEnemyKill);

			if ( bEnemyKill )
				DamageType.static.ScoreKill(KillerPRI, KilledPRI, KilledPawn);

			if ( !bFirstBlood && bEnemyKill )
			{
				bFirstBlood = True;
				BroadcastLocalizedMessage( class'UTFirstBloodMessage', 0, KillerPRI );
			}
			if ( KillerPRI != KilledPRI && (!bTeamGame || (KilledPRI.Team != KillerPRI.Team)) )
			{
				KillerPRI.IncrementSpree();
			}

			// vehicle score kill
			if (KillerPRI != None && UTVehicle(KilledPawn) != None && KilledPlayer != None)
			{
				VehicleScoreKill( Killer, KilledPlayer, UTVehicle(KilledPawn) );
			}
		}
	}
    super.Killed(Killer, KilledPlayer, KilledPawn, damageType);

    if ( bAllowVehicles && (WorldInfo.NetMode == NM_Standalone) && (PlayerController(KilledPlayer) != None) )
    {
		// tell bots not to get into nearby vehicles
		for ( V=VehicleList; V!=None; V=V.NextVehicle )
			if ( WorldInfo.GRI.OnSameTeam(KilledPlayer,V) )
				V.PlayerStartTime = 0;
	}
}

/* VehicleScoreKill() - special scorekill function for vehicles
 This is the place to give additional points for destroying specific vehicles, as well as sending a vehicle kill event to the gamestats
 */
function VehicleScoreKill( Controller Killer, Controller KilledPlayer, UTVehicle DestroyedVehicle )
{
	if (GameStats != none)
	{
		GameStats.VehicleKillEvent(Killer.PlayerReplicationInfo,DestroyedVehicle.Class);
	}
}

/** ForceRespawn()
returns true if dead players should respawn immediately
*/
function bool ForceRespawn()
{
	return ( bForceRespawn || bTempForceRespawn || (MaxLives > 0) || (DefaultMaxLives > 0) );
}

// Parse options for this game...
event InitGame( string Options, out string ErrorMessage )
{
	local string InOpt;
	local int i;
	local name GameClassName;

	// make sure no bots got saved in the .ini as in use
	for (i = 0; i < ActiveBots.length; i++)
	{
		ActiveBots[i].bInUse = false;
	}

	Super.InitGame(Options, ErrorMessage);

	SetGameSpeed(GameSpeed);
	MaxLives = Max(0,GetIntOption( Options, "MaxLives", MaxLives ));
	if ( MaxLives > 0 )
	{
		bTempForceRespawn = true;
	}
	else if ( DefaultMaxLives > 0 )
	{
		bTempForceRespawn = true;
		MaxLives = DefaultMaxLives;
	}
	if( DefaultMaxLives > 0 )
	{
		TimeLimit = 0;
	}
	RemainingTime = 60 * TimeLimit;

	// Set goal score to end match... If automated testing, no score limit (end by timelimit only)
	GoalScore = (!bAutomatedPerfTesting) ? Max(0,GetIntOption( Options, "GoalScore", GoalScore )) : 0;

	InOpt = ParseOption( Options, "Console");
	if ( (InOpt != "") || WorldInfo.IsConsoleBuild() )
	{
		`log("CONSOLE SERVER");
		WorldInfo.bUseConsoleInput = true;
		bConsoleServer = true;
		PlayerControllerClass = ConsolePlayerControllerClass;
	}

	InOpt = ParseOption( Options, "DemoMode");
	if ( InOpt != "" )
	{
		bDemoMode = bool(InOpt);
	}

	bAutoNumBots = (WorldInfo.NetMode == NM_Standalone);
	InOpt = ParseOption( Options, "bAutoNumBots");
	if ( InOpt != "" )
	{
		`log("bAutoNumBots: "$bool(InOpt));
		bAutoNumBots = bool(InOpt);
	}

    if ( bTeamGame && (WorldInfo.NetMode != NM_Standalone) )
    {
		InOpt = ParseOption( Options, "VsBots");
		if ( InOpt != "" )
		{
			BotRatio = float(InOpt);
			bPlayersVsBots = (BotRatio > 0);
			`log("bPlayersVsBots: "$bool(InOpt));
		}
		if ( bPlayersVsBots )
			bAutoNumBots = false;
	}
/*
    InOpt = ParseOption( Options, "AutoAdjust");
    if ( InOpt != "" )
    {
	bAdjustSkill = !bTeamGame && bool(InOpt);
	`log("Adjust skill "$bAdjustSkill);
    }
	EnemyRosterName = ParseOption( Options, "DMTeam");
	if ( EnemyRosterName != "" )
		bCustomBots = true;

    // SP
    if ( single player match )
    {
		MaxLives = 0;
		bAdjustSkill = false;
    }
*/
	if ( HasOption(Options, "NumPlay") )
		bAutoNumBots = false;

	DesiredPlayerCount = bAutoNumBots ? LevelRecommendedPlayers() : Clamp(GetIntOption( Options, "NumPlay", 1 ),1,32);

	InOpt = ParseOption( Options, "PlayersMustBeReady");
	if ( InOpt != "" )
	{
		`log("PlayerMustBeReady: "$Bool(InOpt));
		bPlayersMustBeReady = bool(InOpt);
	}

	InOpt = ParseOption( Options, "MinNetPlayers");
	if (InOpt != "")
	{
		MinNetPlayers = int(InOpt);
		`log("MinNetPlayers: "@MinNetPlayers);
	}

	bWaitForNetPlayers = ( WorldInfo.NetMode != NM_StandAlone );

	InOpt = ParseOption(Options,"QuickStart");
	if ( InOpt != "" )
	{
		bQuickStart = true;
	}
	// Quick start the match if passed in as option or automated testing
	bQuickStart = bQuickStart || bAutomatedPerfTesting;

	InOpt = ParseOption(Options,"NoCustomChars");
	if ( InOpt != "" )
	{
		bNoCustomCharacters = true;
	}

/*
	InOpt = ParseOption(Options, "GameStats");
	if ( InOpt != "")
		bEnableStatLogging = bool(InOpt);
*/

	AdjustedDifficulty = bConsoleServer ? (0.5*GameDifficulty) : GameDifficulty;

	if (WorldInfo.NetMode != NM_StandAlone)
	{
		InOpt = ParseOption( Options, "WarmupTime");
		if (InOpt != "")
		{
			WarmupTime = int(InOpt);
			WarmupRemaining = WarmupTime;
			bWarmupRound = (WarmupTime > 0);
		}
	}
	else
	{
		bWarmupRound = false;
	}

	InOpt = ParseOption(Options,"SPI");
	if ( InOpt != "" )
	{
		bPauseable = true;
		SinglePlayerMissionIndex = int(InOpt);
	}
	else
	{
		SinglePlayerMissionIndex = -1;
	}

	DemoPrefix = ParseOption(Options,"demo");

	// Verify there are maps for map voting

	InOpt = ParseOption(Options,"Vote");
	if ( InOpt != "")
	{
		bAllowMapVoting = bool(InOpt);
	}


	InOpt = ParseOption(Options,"VoteDuration");
	if (InOpt != "")
	{
		VoteDuration = int(InOpt);
	}

	if ( SinglePlayerMissionIndex < 0 && bAllowMapVoting )
	{
		// Default to false, and turn back to true if we have maps.
		bAllowMapVoting = false;
		GameClassName = class.name;
		for (i=0; i<default.GameSpecificMapCycles.Length; i++)
		{
			if (default.GameSpecificMapCycles[i].GameClassName == GameClassName)
			{
				if (default.GameSpecificMapCycles[i].Maps.Length > 0)
				{
					bAllowMapVoting = true;
					break;
				}
			}
		}
	}
}

function int LevelRecommendedPlayers()
{
	local UTMapInfo MapInfo;

	//@FIXME: single player campaign handling needed here?

	MapInfo = UTMapInfo(WorldInfo.GetMapInfo());
	return (MapInfo != None) ? Min(12, (MapInfo.RecommendedPlayersMax + MapInfo.RecommendedPlayersMin) / 2) : 1;
}

event PlayerController Login
(
    string Portal,
    string Options,
    out string ErrorMessage
)
{
	local PlayerController NewPlayer;
	local Controller C;
    local string    InSex;

	if ( MaxLives > 0 )
	{
		// check that game isn't too far along
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if ( (C.PlayerReplicationInfo != None) && (C.PlayerReplicationInfo.NumLives > LateEntryLives) )
			{
				Options = "?SpectatorOnly=1"$Options;
				break;
			}
		}
	}

	NewPlayer = Super.Login(Portal, Options, ErrorMessage);

	if ( UTPlayerController(NewPlayer) != None )
	{
		InSex = ParseOption(Options, "Sex");
		if ( Left(InSex,1) ~= "F" )
			UTPlayerController(NewPlayer).SetPawnFemale();	// only effective if character not valid

		if ( bMustJoinBeforeStart && GameReplicationInfo.bMatchHasBegun )
			UTPlayerController(NewPlayer).bLatecomer = true;

		// custom voicepack
		UTPlayerReplicationInfo(NewPlayer.PlayerReplicationInfo).VoiceTypeName = ParseOption ( Options, "Voice");
	}

	return NewPlayer;
}

function GameEvent(name EventType, int Team, PlayerReplicationInfo InstigatorPRI)
{
	if ( GameStats != None )
	{
		GameStats.GameEvent(EventType, Team, InstigatorPRI);
	}
}

function KillEvent(string Killtype, PlayerReplicationInfo Killer, PlayerReplicationInfo Victim, class<DamageType> Damage)
{
	if (GameStats != none)
	{
		GameStats.KillEvent(Killer,Victim, Damage);
	}
}

function ObjectiveEvent(name StatType, int TeamNo, PlayerReplicationInfo InstigatorPRI, int ObjectiveIndex)
{
	if ( GameStats!=none )
	{
		GameStats.ObjectiveEvent(StatType, TeamNo, InstigatorPRI, ObjectiveIndex);
	}
}

function BonusEvent(name BonusType, PlayerReplicationInfo BonusPlayer)
{
	if ( GameStats != None )
	{
		GameStats.BonusEvent(BonusType, BonusPlayer);
	}
}

/* DEAD
function KillEvent(string Killtype, PlayerReplicationInfo Killer, PlayerReplicationInfo Victim, class<DamageType> Damage)
{
	if ( GameStats != None )
		GameStats.KillEvent(KillType, Killer, Victim, Damage);
}

function ScoreEvent(PlayerReplicationInfo Who, float Points, string Desc)
{
	if ( GameStats != None )
		GameStats.ScoreEvent(Who,Points,Desc);
}

function TeamScoreEvent(int Team, float Points, string Desc)
{
	if ( GameStats != None )
		GameStats.TeamScoreEvent(Team,Points,Desc);
}
*/
function int GetNumPlayers()
{
	if ( NumPlayers > 0 )
		return Max(NumPlayers, Min(NumPlayers+NumBots, MaxPlayers-1));
	return NumPlayers;
}

function bool ShouldRespawn(PickupFactory Other)
{
	return true;
}

function bool WantFastSpawnFor(AIController B)
{
	return ( NumBots < 4 );
}

function float SpawnWait(AIController B)
{
	if ( B.PlayerReplicationInfo.bOutOfLives )
		return 999;
	if ( WorldInfo.NetMode == NM_Standalone )
	{
		if ( WantFastSpawnFor(B) )
			return 0;
		return ( 0.5 * FMax(2,NumBots-4) * FRand() );
	}
	if ( bPlayersVsBots )
		return 0;
	return FRand();
}

function bool TooManyBots(Controller botToRemove)
{
	if ( (WorldInfo.NetMode != NM_Standalone) && bPlayersVsBots )
		return ( NumBots > Min(16,BotRatio*NumPlayers) );
	if ( bPlayerBecameActive )
	{
		bPlayerBecameActive = false;
		return true;
	}
	return ( NumBots + NumPlayers > DesiredPlayerCount );
}

function RestartGame()
{

	if (bAllowMapVoting && VoteCollector != none && !VoteCollector.bVoteDecided)
	{
		VoteCollector.TimesUp();
	}

	if ( bGameRestarted )
		return;

	if ( EndTime > WorldInfo.TimeSeconds ) // still showing end screen
		return;



	Super.RestartGame();
}


function bool CheckEndGame(PlayerReplicationInfo Winner, string Reason)
{
	local Controller P;
	local bool bLastMan;

	if ( bOverTime )
	{
		if ( Numbots + NumPlayers == 0 )
			return true;
		bLastMan = true;
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( (P.PlayerReplicationInfo != None) && !P.PlayerReplicationInfo.bOutOfLives )
			{
				bLastMan = false;
				break;
			}
		}
		if ( bLastMan )
		{
			return true;
		}
	}

	bLastMan = ( Reason ~= "LastMan" );

	if ( !bLastMan && CheckModifiedEndGame(Winner, Reason) )
		return false;

	if ( Winner == None )
	{
		// find winner
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( P.bIsPlayer && !P.PlayerReplicationInfo.bOutOfLives
				&& ((Winner == None) || (P.PlayerReplicationInfo.Score >= Winner.Score)) )
			{
				Winner = P.PlayerReplicationInfo;
			}
		}
	}

	// check for tie
	if ( !bLastMan )
	{
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( P.bIsPlayer &&
				(Winner != P.PlayerReplicationInfo) &&
				(P.PlayerReplicationInfo.Score == Winner.Score)
				&& !P.PlayerReplicationInfo.bOutOfLives )
			{
				if ( !bOverTimeBroadcast )
				{
					StartupStage = 7;
					PlayStartupMessage();
					bOverTimeBroadcast = true;
				}
				return false;
			}
		}
	}

	EndTime = WorldInfo.TimeSeconds + EndTimeDelay;
	GameReplicationInfo.Winner = Winner;

	SetEndGameFocus(Winner);
	return true;
}

function SetEndGameFocus(PlayerReplicationInfo Winner)
{
	local Controller P;

	EndGameFocus = Controller(Winner.Owner).Pawn;
	if ( EndGameFocus != None )
	{
		EndGameFocus.bAlwaysRelevant = true;
	}
	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		P.GameHasEnded(EndGameFocus, (P.PlayerReplicationInfo != None) && (P.PlayerReplicationInfo == Winner) );
	}
}

function bool AtCapacity(bool bSpectator)
{
	local Controller C;
	local bool bForcedSpectator;

    if ( WorldInfo.NetMode == NM_Standalone )
	return false;

	if ( bPlayersVsBots )
		MaxPlayers = Min(MaxPlayers,16);

    if ( MaxLives <= 0 )
		return Super.AtCapacity(bSpectator);

	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if ( (C.PlayerReplicationInfo != None) && (C.PlayerReplicationInfo.NumLives > LateEntryLives) )
		{
			bForcedSpectator = true;
			break;
		}
	}
	if ( !bForcedSpectator )
		return Super.AtCapacity(bSpectator);

	return ( NumPlayers + NumSpectators >= MaxPlayers + MaxSpectators );
}

event PostLogin( playercontroller NewPlayer )
{
	Super.PostLogin(NewPlayer);

	// Log player's login.
	if (GameStats!=None)
	{
		GameStats.ConnectEvent(NewPlayer);
	}

	if ( UTPlayerController(NewPlayer) != None )
	{
		//@todo steve UTPlayerController(NewPlayer).ClientReceiveLoginMenu(LoginMenuClass, bAlwaysShowLoginMenu);
		UTPlayerController(NewPlayer).PlayStartUpMessage(StartupStage);
	}
}

function AssignHoverboard(UTPawn P)
{
	if ( P != None )
		P.bHasHoverboard = bAllowHoverboard;
}

/** return a value based on how much this pawn needs help */
function int GetHandicapNeed(Pawn Other)
{
	return 0;
}

function RestartPlayer(Controller aPlayer)
{
	local UTVehicle V, Best;
	local vector ViewDir;
	local float BestDist, Dist;
	local UTPlayerController PC;

	PC = UTPlayerController(aPlayer);
	if (PC != None)
	{
		// can't respawn if still doing initial processing (loading characters, etc)
		if (!PC.bInitialProcessingComplete && !bQuickStart)
		{
			return;
		}

		// can't respawn if you have to join before the game starts and this player didn't
		if (bMustJoinBeforeStart && PC != None && PC.bLatecomer)
		{
			return;
		}
	}

	// can't respawn if out of lives
	if ( aPlayer.PlayerReplicationInfo.bOutOfLives )
	{
		return;
	}

	if ( aPlayer.IsA('UTBot') && TooManyBots(aPlayer) )
	{
		aPlayer.Destroy();
		return;
	}
	Super.RestartPlayer(aPlayer);

	if ( aPlayer.Pawn == None )
	{
		// pawn spawn failed
		return;
	}

	AssignHoverboard(UTPawn(aPlayer.Pawn));

	if ( bAllowVehicles && (WorldInfo.NetMode == NM_Standalone) && (PlayerController(aPlayer) != None) )
	{
		// tell bots not to get into nearby vehicles for a little while
		BestDist = 2000;
		ViewDir = vector(aPlayer.Pawn.Rotation);
		for ( V=VehicleList; V!=None; V=V.NextVehicle )
		{
			if ( !bTeamGame && V.bTeamLocked )
			{
				// FIXMETEMP
				V.bTeamLocked = false;
			}
			if ( V.bTeamLocked && WorldInfo.GRI.OnSameTeam(aPlayer,V) )
			{
				Dist = VSize(V.Location - aPlayer.Pawn.Location);
				if ( (ViewDir Dot (V.Location - aPlayer.Pawn.Location)) < 0 )
					Dist *= 2;
				if ( Dist < BestDist )
				{
					Best = V;
					BestDist = Dist;
				}
			}
		}
		if ( Best != None )
			Best.PlayerStartTime = WorldInfo.TimeSeconds + 8;
	}
}

function AddDefaultInventory( pawn PlayerPawn )
{
	local int i;

	if ( bConsoleStartWithWeapons )
	{
		for (i=0; i<DefaultInventory.Length; i++)
		{
			DefaultInventory[i] = class'UTGame.UTWeap_RocketLauncher';
		}
		if ( UTSkeletalMeshComponent(class'UTGame.UTWeap_ShockRifle'.Default.Mesh).SkeletalMesh != None )
		{
			DefaultInventory[0] = class'UTGame.UTWeap_ShockRifle';
			if ( (DefaultInventory.Length == 1) && (UTSkeletalMeshComponent(class'UTGame.UTWeap_RocketLauncher'.Default.Mesh).SkeletalMesh != None) )
				DefaultInventory[1] = class'UTGame.UTWeap_RocketLauncher';
		}
	}

	for (i=0; i<DefaultInventory.Length; i++)
	{
		// Ensure we don't give duplicate items
		if (PlayerPawn.FindInventoryType( DefaultInventory[i] ) == None)
		{
			// Only activate the first weapon
			PlayerPawn.CreateInventory(DefaultInventory[i], (i > 0));
		}
	}

	PlayerPawn.AddDefaultInventory();
}

function bool CanSpectate( PlayerController Viewer, PlayerReplicationInfo ViewTarget )
{
    return ( (ViewTarget != None) && ((WorldInfo.NetMode == NM_Standalone) || Viewer.PlayerReplicationInfo.bOnlySpectator) );
}

static function ConformPlayerName(out string NewName, int MaxLen)
{
	MaxLen = 15;
	Super.ConformPlayerName(NewName, MaxLen);
}

function ChangeName(Controller Other, string S, bool bNameChange)
{
    local Controller APlayer;

	ConformPlayerName(S, 15);

    if ( Other.PlayerReplicationInfo.playername~=S )
    {
		return;
	}

	foreach WorldInfo.AllControllers(class'Controller', APlayer)
	{
		if (APlayer.bIsPlayer && APlayer.PlayerReplicationInfo.playername ~= S)
		{
			if ( PlayerController(Other) != None )
			{
				PlayerController(Other).ReceiveLocalizedMessage( GameMessageClass, 8 );
				if ( Other.PlayerReplicationInfo.PlayerName ~= DefaultPlayerName )
				{
					Other.PlayerReplicationInfo.SetPlayerName(DefaultPlayerName$Other.PlayerReplicationInfo.PlayerID);
				}
				return;
			}
			else
			{
				if ( Other.PlayerReplicationInfo.bIsFemale )
				{
					S = FemaleBackupNames[FemaleBackupNameOffset%32];
					FemaleBackupNameOffset++;
				}
				else
				{
					S = MaleBackupNames[MaleBackupNameOffset%32];
					MaleBackupNameOffset++;
				}
			}
		}
	}

	if( bNameChange )
	{
		GameEvent('NameChange',-1,Other.PlayerReplicationInfo);
	}

    Other.PlayerReplicationInfo.SetPlayerName(S);
}

function DiscardInventory( Pawn Other )
{
	if (UTPlayerReplicationInfo(Other.PlayerReplicationInfo) != None && Other.PlayerReplicationInfo.bHasFlag)
	{
		UTPlayerReplicationInfo(Other.PlayerReplicationInfo).GetFlag().Drop();
	}
	else if ( Other.DrivenVehicle != None && UTPlayerReplicationInfo(Other.DrivenVehicle.PlayerReplicationInfo) != None
		&& Other.DrivenVehicle.PlayerReplicationInfo.bHasFlag )
	{
		UTPlayerReplicationInfo(Other.DrivenVehicle.PlayerReplicationInfo).GetFlag().Drop();
	}

	Super.DiscardInventory(Other);
}

function Logout(controller Exiting)
{
	local int i;

	if ( Exiting.PlayerReplicationInfo.bHasFlag )
		UTPlayerReplicationInfo(Exiting.PlayerReplicationInfo).GetFlag().Drop();
	Super.Logout(Exiting);

	if (Exiting.IsA('UTBot') && !UTBot(Exiting).bSpawnedByKismet)
	{
		i = ActiveBots.Find('BotName', Exiting.PlayerReplicationInfo.PlayerName);
		if (i != INDEX_NONE)
		{
			ActiveBots[i].bInUse = false;
		}
		NumBots--;
	}
	if (NeedPlayers())
	{
		AddBot();
	}
	if (MaxLives > 0)
	{
		CheckMaxLives(None);
	}

	if ( GameStats!=None)
		GameStats.DisconnectEvent(Exiting);

	//VotingHandler.PlayerExit(Exiting);
}

exec function KillBots()
{
	local UTBot B;

	DesiredPlayerCount = NumPlayers;
	bPlayersVsBots = false;

	foreach WorldInfo.AllControllers(class'UTBot', B)
	{
		KillBot(B);
	}
}

function KillBot(UTBot B)
{
	if ( B == None )
		return;

	if (GameStats != none)
	{
		GameStats.DisconnectEvent(B);
	}

	if ( (Vehicle(B.Pawn) != None) && (Vehicle(B.Pawn).Driver != None) )
		Vehicle(B.Pawn).Driver.KilledBy(Vehicle(B.Pawn).Driver);
	else if (B.Pawn != None)
	B.Pawn.KilledBy( B.Pawn );
	if (B != None)
		B.Destroy();
}

function bool NeedPlayers()
{
    if ( bMustJoinBeforeStart )
	return false;
    if ( bPlayersVsBots )
		return ( NumBots < Min(16,BotRatio*NumPlayers) );

    return (NumPlayers + NumBots < DesiredPlayerCount);
}

exec function AddBots(int Num)
{
	DesiredPlayerCount = Clamp(Max(DesiredPlayerCount, NumPlayers+NumBots)+Num, 1, 32);

	while ( (NumPlayers + NumBots < DesiredPlayerCount) && AddBot() != none )
	{
		`log("added bot");
	}
}

exec function UTBot AddNamedBot(string BotName)
{
	DesiredPlayerCount = Clamp(Max(DesiredPlayerCount, NumPlayers + NumBots) + 1, 1, 32);
	return AddBot(BotName);
}

function UTBot AddBot(optional string BotName, optional bool bUseTeamIndex, optional int TeamIndex)
{
	local UTBot NewBot;
	local int i;

	if (BotName == "")
	{
		i = ActiveBots.Find('bInUse', false);
		if (i != INDEX_NONE)
		{
			BotName = ActiveBots[i].BotName;
			ActiveBots[i].bInUse = true;
		}
	}
	else
	{
		i = ActiveBots.Find('BotName', BotName);
		if (i != INDEX_NONE)
		{
			ActiveBots[i].bInUse = true;
		}
	}

	NewBot = SpawnBot(BotName, bUseTeamIndex, TeamIndex);
	if ( NewBot == None )
	{
		`warn("Failed to spawn bot.");
		return none;
	}
	NewBot.PlayerReplicationInfo.PlayerID = CurrentID++;
	NumBots++;
	if ( WorldInfo.NetMode == NM_Standalone )
	{
		RestartPlayer(NewBot);
	}
	else
	{
		NewBot.GotoState('Dead','MPStart');
	}

    return NewBot;
}

/* Spawn and initialize a bot
*/
function UTBot SpawnBot(optional string botName,optional bool bUseTeamIndex, optional int TeamIndex)
{
	local UTBot NewBot;
	local UTTeamInfo BotTeam;
	local CharacterInfo BotInfo;

	BotTeam = GetBotTeam(,bUseTeamIndex,TeamIndex);
	BotInfo = BotTeam.GetBotInfo(botName);

	NewBot = Spawn(BotClass);

	if ( NewBot != None )
	{
		InitializeBot(NewBot, BotTeam, BotInfo);

		if (BaseMutator != None)
		{
			BaseMutator.NotifyLogin(NewBot);
		}
		if (GameStats != None)
		{
			GameStats.ConnectEvent(NewBot);
		}
	}

	return NewBot;
}

/* Initialize bot
*/
function InitializeBot(UTBot NewBot, UTTeamInfo BotTeam, const out CharacterInfo BotInfo)
{
	NewBot.Initialize(AdjustedDifficulty, BotInfo);
	BotTeam.AddToTeam(NewBot);
	ChangeName(NewBot, BotInfo.CharName, false);
	BotTeam.SetBotOrders(NewBot);
}

function UTTeamInfo GetBotTeam(optional int TeamBots,optional bool bUseTeamIndex,optional int TeamIndex)
{
	local class<UTTeamInfo> RosterClass;

	if ( EnemyRoster != None )
		return EnemyRoster;
	if ( EnemyRosterName != "" )
	{
		RosterClass = class<UTTeamInfo>(DynamicLoadObject(EnemyRosterName,class'Class'));
		if ( RosterClass != None)
			EnemyRoster = spawn(RosterClass);
	}
	if ( EnemyRoster == None )
	{
		RosterClass = class<UTTeamInfo>(DynamicLoadObject(DefaultEnemyRosterClass,class'Class'));
		if ( RosterClass != None)
			EnemyRoster = spawn(RosterClass);
	}
	EnemyRoster.Initialize(TeamBots);
	return EnemyRoster;
}

function InitGameReplicationInfo()
{
	local UTGameReplicationInfo GRI;
	local UTMutator M;

	Super.InitGameReplicationInfo();

	GRI = UTGameReplicationInfo(GameReplicationInfo);
	GRI.GoalScore = GoalScore;
	GRI.TimeLimit = TimeLimit;
	GRI.MinNetPlayers = MinNetPlayers;
	GRI.bConsoleServer = (WorldInfo.bUseConsoleInput || WorldInfo.IsConsoleBuild());

	// Flag the match if in story mode
	GRI.bStoryMode = SinglePlayerMissionIndex >= 0;


	if ( !bForceRespawn && !GRI.bStoryMode )
	{
		GRI.bSHowMOTD = true;
	}

	// Create the list of mutators.

	M = GetBaseUTMutator();
	while (M != none )
	{
		GRI.MutatorList = GRI.MutatorList == "" ? M.GetHumanReadableName() : (","@M.GetHumanReadableName() );
		M = M.GetNextUTMutator();
	}
}

function ReduceDamage( out int Damage, pawn injured, Controller instigatedBy, vector HitLocation, out vector Momentum, class<DamageType> DamageType )
{
	local float InstigatorSkill;
	local UTVehicle V;

	if ( InstigatedBy != None )
	{
		if ( InstigatedBy != Injured.Controller )
		{
			if (WorldInfo.TimeSeconds - injured.SpawnTime < SpawnProtectionTime && !DamageType.default.bCausedByWorld)
			{
				Damage = 0;
				return;
			}

			V = UTVehicle(Injured);
			if (V != None && !V.bHasBeenDriven && !bUndrivenVehicleDamage)
			{
				Damage = 0;
				Super.ReduceDamage(Damage, Injured, InstigatedBy, HitLocation, Momentum, DamageType);
				return;
			}
		}
		else if ( (DamageType != None) && DamageType.Default.bDontHurtInstigator )
		{
			Damage = 0;
			return;
		}
	}

	Super.ReduceDamage( Damage, injured, InstigatedBy, HitLocation, Momentum, DamageType );

	if ( instigatedBy == None )
		return;

	if ( WorldInfo.Game.GameDifficulty <= 3 )
	{
		if ( injured.IsPlayerPawn() && (injured.Controller == instigatedby) && (WorldInfo.NetMode == NM_Standalone) )
			Damage *= 0.5;

		//skill level modification
		if ( (AIController(instigatedBy) != None) && (WorldInfo.NetMode == NM_Standalone) )
		{
			InstigatorSkill = AIController(instigatedBy).Skill;
			if ( (InstigatorSkill <= 3) && injured.IsHumanControlled() )
				{
					if ( ((instigatedBy.Pawn != None) && (instigatedBy.Pawn.Weapon != None) && instigatedBy.Pawn.Weapon.bMeleeWeapon)
						|| ((injured.Weapon != None) && injured.Weapon.bMeleeWeapon && (VSize(injured.location - instigatedBy.Pawn.Location) < 600)) )
							Damage = Damage * (0.76 + 0.08 * InstigatorSkill);
					else
							Damage = Damage * (0.25 + 0.15 * InstigatorSkill);
			}
		}
	}
	if ( InstigatedBy.Pawn != None )
	{
		Damage = Damage * instigatedBy.Pawn.GetDamageScaling();
	}
}

function NotifySpree(UTPlayerReplicationInfo Other, int num)
{
	local PlayerController PC;

	if ( num == 5 )
		num = 0;
	else if ( num == 10 )
		num = 1;
	else if ( num == 15 )
		num = 2;
	else if ( num == 20 )
		num = 3;
	else if ( num == 25 )
		num = 4;
	else
		return;

	foreach WorldInfo.AllControllers(class'PlayerController', PC)
	{
		PC.ReceiveLocalizedMessage( class'UTKillingSpreeMessage', Num, Other );
	}
}

function EndSpree(UTPlayerReplicationInfo Killer, UTPlayerReplicationInfo Other)
{
	local PlayerController PC;

	if ( Other == None )
		return;

	Other.Spree = 0;
	foreach WorldInfo.AllControllers(class'PlayerController', PC)
	{
		if ( (Killer == Other) || (Killer == None) )
		{
			PC.ReceiveLocalizedMessage( class'UTKillingSpreeMessage', 1, None, Other );
		}
		else
		{
			PC.ReceiveLocalizedMessage( class'UTKillingSpreeMessage', 0, Other, Killer );
		}
	}
}

//------------------------------------------------------------------------------
// Game States

function StartMatch()
{
	local int num;
	local bool bTemp;

    GotoState('MatchInProgress');

    GameReplicationInfo.RemainingMinute = RemainingTime;
    Super.StartMatch();

	bTemp = bMustJoinBeforeStart;
    bMustJoinBeforeStart = false;
    while ( NeedPlayers() && (Num<16) )
    {
		AddBot();
		Num++;
    }
    bMustJoinBeforeStart = bTemp;

	if (GameStats != none)
		GameStats.MatchBeginEvent();

	`log("START MATCH");
}

function EndGame(PlayerReplicationInfo Winner, string Reason )
{
	local Sequence GameSequence;
	local array<SequenceObject> Events;
	local int i;

	if ( (Reason ~= "triggered") ||
	 (Reason ~= "LastMan")   ||
	 (Reason ~= "TimeLimit") ||
	 (Reason ~= "FragLimit") ||
	 (Reason ~= "TeamScoreLimit") )
	{
		Super.EndGame(Winner,Reason);
		if ( bGameEnded )
		{
			// trigger any Kismet "Game Ended" events
			GameSequence = WorldInfo.GetGameSequence();
			if (GameSequence != None)
			{
				GameSequence.FindSeqObjectsByClass(class'UTSeqEvent_GameEnded', true, Events);
				for (i = 0; i < Events.length; i++)
				{
					UTSeqEvent_GameEnded(Events[i]).CheckActivate(self, None);
				}
			}

			if (GameStats != none)
			{
				GameStats.MatchEndEvent( NAME("MatchEnded_"$Reason),Winner);
			}
			GotoState('MatchOver');
		}
	}
}

/** FindPlayerStart()
* Return the 'best' player start for this player to start from.  PlayerStarts are rated by RatePlayerStart().
* @param Player is the controller for whom we are choosing a playerstart
* @param InTeam specifies the Player's team (if the player hasn't joined a team yet)
* @param IncomingName specifies the tag of a teleporter to use as the Playerstart
* @returns NavigationPoint chosen as player start (usually a PlayerStart)
 */
function NavigationPoint FindPlayerStart(Controller Player, optional byte InTeam, optional string incomingName)
{
    local NavigationPoint Best;

	// Save LastPlayerStartSpot for use in RatePlayerStart()
    if ( (Player != None) && (Player.StartSpot != None) )
		LastPlayerStartSpot = Player.StartSpot;

    Best = Super.FindPlayerStart(Player, InTeam, incomingName );

	// Save LastStartSpot for use in RatePlayerStart()
    if ( Best != None )
		LastStartSpot = Best;
    return Best;
}

function bool DominatingVictory()
{
	return ( (PlayerReplicationInfo(GameReplicationInfo.Winner).Deaths == 0)
		&& (PlayerReplicationInfo(GameReplicationInfo.Winner).Score >= 5) );
}

function bool IsAWinner(PlayerController C)
{
	return ( C.PlayerReplicationInfo.bOnlySpectator || (C.PlayerReplicationInfo == GameReplicationInfo.Winner) );
}

function PlayEndOfMatchMessage()
{
	local UTPlayerController PC;

	if ( DominatingVictory() )
	{
		foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
		{
			if (IsAWinner(PC))
			{
				PC.ClientPlayAnnouncement(VictoryMessageClass, 0);
			}
			else
			{
				PC.ClientPlayAnnouncement(VictoryMessageClass, 1);
			}
		}
	}
	else
		PlayRegularEndOfMatchMessage();
}

function PlayRegularEndOfMatchMessage()
{
	local UTPlayerController PC;

	foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		if (!PC.PlayerReplicationInfo.bOnlySpectator )
		{
			if (IsAWinner(PC))
			{
				PC.ClientPlayAnnouncement(VictoryMessageClass, 2);
			}
			else
			{
				PC.ClientPlayAnnouncement(VictoryMessageClass, 3);
			}
		}
	}
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer)
{
	Default.VictoryMessageClass.static.PrecacheGameAnnouncements(Announcer, Default.class);
	Announcer.PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_LastSecondSave');
	Announcer.PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Play');
}

function PlayStartupMessage()
{
	local UTPlayerController P;

	// keep message displayed for waiting players
	foreach WorldInfo.AllControllers(class'UTPlayerController', P)
	{
		P.PlayStartUpMessage(StartupStage);
	}
}

function bool JustStarted(float MaxElapsedTime)
{
	return ElapsedTime < MaxElapsedTime;
}

/** ends the current round; sends the game into state RoundOver and sets the ViewTarget for all players to be the given actor */
function EndRound(Actor EndRoundFocus)
{
	local Controller C;

	//round has ended
	if ( !bGameEnded )
	{
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			C.RoundHasEnded(EndRoundFocus);
		}
	}
	GotoState('RoundOver');
}

function bool MatchIsInProgress()
{
	return false;
}

auto State PendingMatch
{
	function RestartPlayer(Controller aPlayer)
	{
		if (CountDown <= 0 || UTGameReplicationInfo(GameReplicationInfo).bWarmupRound)
		{
			Global.RestartPlayer(aPlayer);
		}
	}

	// Override these 4 functions so that if we are in a warmup round, they get ignored.

	function CheckLives();
	function bool CheckScore(PlayerReplicationInfo Scorer);
	function ScoreKill(Controller Killer, Controller Other);
	function ScoreFlag(Controller Scorer, UTCTFFlag theFlag);

	/*//@todo steve - Adjust for new AddBot()
    function UTBot AddBot(optional string botName)
    {
	if ( botName != "" )
			PreLoadNamedBot(botName);
		else
			PreLoadBot();
	return true;
    }
	*/
	function Timer()
	{
		local PlayerController P;
		local bool bReady;
		local UTPlayerController UTPC;

		Global.Timer();

		// first check if there are enough net players, and enough time has elapsed to give people
		// a chance to join
		if ( NumPlayers == 0 )
		{
			bWaitForNetPlayers = true;

			if (bWarmupRound)
			{
				WarmupRemaining = WarmupTime;
				GameReplicationInfo.RemainingTime = WarmupRemaining;
			}
		}
		else if (bWarmupRound)
		{
			if (WarmupRemaining > 0)
			{
				if ( NeedPlayers() )
				{
					AddBot();
				}
				WarmupRemaining--;
				GameReplicationInfo.RemainingTime = WarmupRemaining;
				if (WarmupRemaining % 60 == 0)
				{
					GameReplicationInfo.RemainingMinute = WarmupRemaining;
				}
		   		return;
			}
			else if (WarmupRemaining == 0)
			{
				WarmupRemaining = -1;
				UTGameReplicationInfo(GameReplicationInfo).bWarmupRound = false;
		   		ResetLevel();
			}
		}

		if ( bWaitForNetPlayers && (WorldInfo.NetMode != NM_Standalone) )
		{
			if ( NumPlayers >= MinNetPlayers )
				ElapsedTime++;
			else
				ElapsedTime = 0;
			if ( (NumPlayers == MaxPlayers) || (ElapsedTime > NetWait) )
			{
				// wait until players finish clientside processing (or it times out)
				if (ElapsedTime <= ClientProcessingTimeout)
				{
					foreach WorldInfo.AllControllers(class'UTPlayerController', UTPC)
					{
						if (!UTPC.bInitialProcessingComplete)
						{
							PlayStartupMessage();
							return;
						}
					}
				}
				bWaitForNetPlayers = false;
				CountDown = Default.CountDown;
			}
			else
			{
				PlayStartupMessage();
				return;
			}
		}
		else if (WorldInfo.NetMode == NM_Standalone)
		{
			foreach WorldInfo.AllControllers(class'UTPlayerController', UTPC)
			{
				if (!UTPC.bInitialProcessingComplete)
				{
					// PlayStartupMessage() intentionally left out here (mesh construction messsage should be visible)
					return;
				}
			}
		}

		// check if players are ready
		bReady = true;

		StartupStage = 1;
		if ( !bStartedCountDown && (bPlayersMustBeReady || (WorldInfo.NetMode == NM_Standalone)) )
		{
			foreach WorldInfo.AllControllers(class'PlayerController', P)
			{
				if ( P.PlayerReplicationInfo != None && P.bIsPlayer && P.PlayerReplicationInfo.bWaitingPlayer
					&& !P.PlayerReplicationInfo.bReadyToPlay )
				{
					bReady = false;
				}
			}
		}
		if ( bReady )
		{
			if (!bStartedCountDown)
			{
				if (DemoPrefix != "")
				{
					//@FIXME: add on number to make filename unique
					ConsoleCommand("demorec" @ DemoPrefix);
				}
				bStartedCountDown = true;
			}
			CountDown--;
			if ( CountDown <= 0 )
				StartMatch();
			else
				StartupStage = 5 - CountDown;
		}
		PlayStartupMessage();
	}

	function BeginState(Name PreviousStateName)
	{
		if (bWarmupRound)
		{
			GameReplicationInfo.RemainingTime = WarmupRemaining;
			GameReplicationInfo.RemainingMinute = WarmupRemaining;
		}
		bWaitingToStartMatch = true;
		UTGameReplicationInfo(GameReplicationInfo).bWarmupRound = bWarmupRound;
		StartupStage = 0;
	}

	function EndState(Name NextStateName)
	{
		UTGameReplicationInfo(GameReplicationInfo).bWarmupRound = false;
	}


Begin:
	// quickstart for solo loading of a map
	if ( (DesiredPlayerCount == 1) && (WorldInfo.NetMode == NM_StandAlone) )
	{
		bQuickStart = TRUE;
	}

	if ( bQuickStart )
	{
		if (DemoPrefix != "")
		{
			//@FIXME: add on number to make filename unique
			ConsoleCommand("demorec" @ DemoPrefix);
		}
		StartMatch();
	}
}

state MatchInProgress
{
	function bool MatchIsInProgress()
	{
		return true;
	}

	function Timer()
	{
		local PlayerController P;

		Global.Timer();
		if ( !bFinalStartup )
		{
			bFinalStartup = true;
			PlayStartupMessage();
		}
		if ( ForceRespawn() )
		{
			foreach WorldInfo.AllControllers(class'PlayerController', P)
			{
				if ( (P.Pawn == None) && !P.PlayerReplicationInfo.bOnlySpectator)
				{
					P.ServerReStartPlayer();
				}
			}
		}
		if ( NeedPlayers() )
		{
			AddBot();
		}

		if ( bOverTime )
		{
			EndGame(None,"TimeLimit");
		}
		else if ( TimeLimit > 0 )
		{
			GameReplicationInfo.bStopCountDown = false;
			RemainingTime--;
			GameReplicationInfo.RemainingTime = RemainingTime;
			if ( RemainingTime % 60 == 0 )
			{
				GameReplicationInfo.RemainingMinute = RemainingTime;
			}
			if ( RemainingTime <= 0 )
			{
				EndGame(None,"TimeLimit");
			}
		}
		else if ( (MaxLives > 0) && (NumPlayers + NumBots != 1) )
		{
			CheckMaxLives(none);
		}

		ElapsedTime++;
		GameReplicationInfo.ElapsedTime = ElapsedTime;
	}

	function BeginState(Name PreviousStateName)
	{
		local PlayerReplicationInfo PRI;

		if (PreviousStateName != 'RoundOver')
		{
			foreach DynamicActors(class'PlayerReplicationInfo', PRI)
			{
				PRI.StartTime = 0;
			}
			ElapsedTime = 0;
			bWaitingToStartMatch = false;
			StartupStage = 5;
			PlayStartupMessage();
			StartupStage = 6;
		}
	}
}

State MatchOver
{
	function RestartPlayer(Controller aPlayer) {}
	function ScoreKill(Controller Killer, Controller Other) {}

	function ReduceDamage( out int Damage, pawn injured, Controller instigatedBy, vector HitLocation, out vector Momentum, class<DamageType> DamageType )
	{
		Damage = 0;
		Momentum = vect(0,0,0);
	}

	function bool ChangeTeam(Controller Other, int num, bool bNewTeam)
	{
		// we don't want newly joining players to get stuck as a spectator for the next match,
		// mark them as out of the game and pretend we succeeded
		Other.PlayerReplicationInfo.bOutOfLives = true;
		return true;
	}

	event PostLogin(PlayerController NewPlayer)
	{
		Global.PostLogin(NewPlayer);

		NewPlayer.GameHasEnded(EndGameFocus);
	}

	function Timer()
	{
		local PlayerController PC;
		local Sequence GameSequence;
		local array<SequenceObject> AllInterpActions;
		local SeqAct_Interp InterpAction;
		local int i, j;
		local bool bIsInCinematic;

		Global.Timer();

		if ( !bGameRestarted && (WorldInfo.TimeSeconds > EndTime + RestartWait) )
		{
			RestartGame();
		}

		if ( EndGameFocus != None )
		{
			EndGameFocus.bAlwaysRelevant = true;

			// if we're not in a cinematic (matinee controlled camera), force all players' ViewTarget to the EndGameFocus
			GameSequence = WorldInfo.GetGameSequence();
			if (GameSequence != None)
			{
				// find any matinee actions that exist
				GameSequence.FindSeqObjectsByClass(class'SeqAct_Interp', true, AllInterpActions);
				for (i = 0; i < AllInterpActions.length && !bIsInCinematic; i++)
				{
					InterpAction = SeqAct_Interp(AllInterpActions[i]);
					if (InterpAction.InterpData != None && InterpAction.GroupInst.length > 0)
					{
						for (j = 0; j < InterpAction.InterpData.InterpGroups.length; j++)
						{
							if (InterpGroupDirector(InterpAction.InterpData.InterpGroups[j]) != None)
							{
								bIsInCinematic = true;
								break;
							}
						}
					}
				}
			}
			if (!bIsInCinematic)
			{
				foreach WorldInfo.AllControllers(class'PlayerController', PC)
				{
					PC.ClientSetViewtarget(EndGameFocus);
				}
			}
		}

		// play end-of-match message for winner/losers (for single and muli-player)
		EndMessageCounter++;
		if ( EndMessageCounter == EndMessageWait )
		{
			PlayEndOfMatchMessage();
		}
	}


	function bool NeedPlayers()
	{
		return false;
	}

	function BeginState(Name PreviousStateName)
	{
		local Pawn P;
	local int i;
	local name GameClassName;

		GameReplicationInfo.bStopCountDown = true;
		foreach WorldInfo.AllPawns(class'Pawn', P)
		{
			P.TurnOff();
		}

		if ( SinglePlayerMissionIndex < 0 && bAllowMapVoting )
		{
			VoteCollector = Spawn(class'UTVoteCollector',none);
			if ( VoteCollector != none )
			{
				GameClassName = class.name;
				for (i=0; i<default.GameSpecificMapCycles.Length; i++)
				{
					if (default.GameSpecificMapCycles[i].GameClassName == GameClassName)
					{
						VoteCollector.Initialize(default.GameSpecificMapCycles[i].Maps);
					}
				}
			}
			else
			{
				`log("Could not create the voting collector.");
			}

			if (RestartWait < VoteDuration)
			{
				RestartWait = VoteDuration;
			}
			UTGameReplicationInfo(GameReplicationInfo).MapVoteTimeRemaining = RestartWait;

		}
	}

	function ResetLevel()
	{
		RestartGame();
	}
}

state RoundOver extends MatchOver
{
	event BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		ResetCountDown = Max(2, ResetTimeDelay);
	}

	function bool ChangeTeam(Controller Other, int Num, bool bNewTeam)
	{
		return Global.ChangeTeam(Other, Num, bNewTeam);
	}

	function ResetLevel()
	{
		// note that we need to change the state BEFORE calling ResetLevel() so that we don't unintentionally override
		// functions that ResetLevel() may call
		GotoState('');
		Global.ResetLevel();
		GotoState('MatchInProgress');
		ResetCountDown = 0;
	}

	event Timer()
	{
		Global.Timer();

		ResetCountdown--;
		/* //@todo steve
		if ( ResetCountDown < 3 )
		{
			// blow up all redeemer guided warheads
			foreach WorldInfo.AllControllers(class'Controller', C)
			{
				if ( (C.Pawn != None) && C.Pawn.IsA('RedeemerWarhead') )
				{
					C.Pawn.StartFire(1);
				}
			}
		}

		if ( ResetCountDown == 8 )
		{
			foreach WorldInfo.AllControllers(class'PlayerController', PC)
			{
				PC.ClientPlayStatusAnnouncement('NewRoundIn',1,true);
			}
		}
		else if ( (ResetCountDown > 1) && (ResetCountDown < 7) )
			BroadcastLocalizedMessage(class'TimerMessage', ResetCountDown-1);
		else
		*/
		if (ResetCountdown == 1)
		{
			ResetLevel();
		}
	}
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
	local array<playerstart> PlayerStarts;
	local int i, RandStart;
	local byte Team;

	// use InTeam if player doesn't have a team yet
	Team = ( (Player != None) && (Player.PlayerReplicationInfo != None) && (Player.PlayerReplicationInfo.Team != None) )
			? byte(Player.PlayerReplicationInfo.Team.TeamIndex)
			: InTeam;

	// make array of enabled playerstarts
	foreach WorldInfo.AllNavigationPoints(class'PlayerStart', P)
	{
		if ( P.bEnabled )
			PlayerStarts[PlayerStarts.Length] = P;
	}

	// start at random point to randomize finding "good enough" playerstart
	RandStart = Rand(PlayerStarts.Length);
	for ( i=RandStart; i<PlayerStarts.Length; i++ )
	{
		P = PlayerStarts[i];
		NewRating = RatePlayerStart(P,Team,Player);
		if ( NewRating >= 30 )
		{
			// this PlayerStart is good enough
			return P;
		}
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	for ( i=0; i<RandStart; i++ )
	{
		P = PlayerStarts[i];
		NewRating = RatePlayerStart(P,Team,Player);
		if ( NewRating >= 30 )
		{
			// this PlayerStart is good enough
			return P;
		}
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
	local float Score, NextDist;
	local Controller OtherPlayer;
	local bool bTwoPlayerGame;

	// Primary starts are more desireable
	Score = P.bPrimaryStart ? 30 : 20;

	if ( (P == LastStartSpot) || (P == LastPlayerStartSpot) )
	{
		// avoid re-using starts
		Score -= 15.0;
	}

	bTwoPlayerGame = ( NumPlayers + NumBots == 2 );

	if (Player != None)
	{
		ForEach WorldInfo.AllControllers(class'Controller', OtherPlayer)
		{
			if ( OtherPlayer.bIsPlayer && (OtherPlayer.Pawn != None) )
			{
				// check if playerstart overlaps this pawn
				if ( (Abs(P.Location.Z - OtherPlayer.Pawn.Location.Z) < P.CylinderComponent.CollisionHeight + OtherPlayer.Pawn.CylinderComponent.CollisionHeight)
					&& (VSize2D(P.Location - OtherPlayer.Pawn.Location) < P.CylinderComponent.CollisionRadius + OtherPlayer.Pawn.CylinderComponent.CollisionRadius) )
				{
					// overlapping - would telefrag
					return -10;
				}

				NextDist = VSize(OtherPlayer.Pawn.Location - P.Location);
				if ( (NextDist < 3000) && !WorldInfo.GRI.OnSameTeam(Player,OtherPlayer) && FastTrace(P.Location, OtherPlayer.Pawn.Location+vect(0,0,1)*OtherPlayer.Pawn.CylinderComponent.CollisionHeight) )
				{
					// avoid starts close to visible enemy
					if ( (OtherPlayer.PlayerReplicationInfo != None) && (UTPlayerReplicationInfo(Player.PlayerReplicationInfo).LastKillerPRI == OtherPlayer.PlayerReplicationInfo) )
					{
						// really avoid guy that killed me last
						Score -= 7;
					}
					Score -= (5 - 0.001*NextDist);
				}
				else if ( (NextDist < 1500) && (OtherPlayer.PlayerReplicationInfo != None) && (UTPlayerReplicationInfo(Player.PlayerReplicationInfo).LastKillerPRI == OtherPlayer.PlayerReplicationInfo) )
				{
					// really avoid guy that killed me last
					Score -= 7;
				}
				else if ( bTwoPlayerGame )
				{
					// in 2 player game, look for any visibility
					Score += FMin(2,0.001*NextDist);
					if ( FastTrace(P.Location, OtherPlayer.Pawn.Location) )
						Score -= 5;
				}
			}
		}
	}
	return FMax(Score, 0.2);
}

// check if all other players are out
function bool CheckMaxLives(PlayerReplicationInfo Scorer)
{
	local Controller C;
	local PlayerReplicationInfo Living;
	local bool bNoneLeft;

	if ( MaxLives > 0 )
	{
		if ( (Scorer != None) && !Scorer.bOutOfLives )
			Living = Scorer;
		bNoneLeft = true;
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if ( (C.PlayerReplicationInfo != None) && C.bIsPlayer
				&& !C.PlayerReplicationInfo.bOutOfLives
				&& !C.PlayerReplicationInfo.bOnlySpectator )
			{
				if ( Living == None )
				{
					Living = C.PlayerReplicationInfo;
				}
				else if (C.PlayerReplicationInfo != Living)
				{
					bNoneLeft = false;
				    	break;
				}
			}
		}
		if ( bNoneLeft )
		{
			if ( Living != None )
			{
				EndGame(Living,"LastMan");
			}
			else
			{
				EndGame(Scorer,"LastMan");
			}
			return true;
		}
	}
	return false;
}

/* CheckScore()
see if this score means the game ends
*/
function bool CheckScore(PlayerReplicationInfo Scorer)
{
	local controller C;

	if ( CheckMaxLives(Scorer) )
	{
		return false;
	}

	if ( !Super.CheckScore(Scorer) )
	{
		return false;
	}

	if ( Scorer != None )
	{
		if ( (GoalScore > 0) && (Scorer.Score >= GoalScore) )
		{
			EndGame(Scorer,"fraglimit");
		}
		else if ( bOverTime )
		{
			// end game only if scorer has highest score
			foreach WorldInfo.AllControllers(class'Controller', C)
			{
				if ( (C.PlayerReplicationInfo != None)
					&& (C.PlayerReplicationInfo != Scorer)
					&& (C.PlayerReplicationInfo.Score >= Scorer.Score) )
				{
					return false;
				}
			}
			EndGame(Scorer,"fraglimit");
		}
	}
	return true;
}

function RegisterVehicle(UTVehicle V)
{
	// add to AI vehicle list
	V.NextVehicle = VehicleList;
	VehicleList = V;
}

/**
ActivateVehicleFactory()
Called by UTVehicleFactory in its PostBeginPlay()
*/
function ActivateVehicleFactory(UTVehicleFactory VF)
{
	local UTGameObjective O, Best;
	local float BestDist, NewDist;

	if ( !bTeamGame )
		VF.bStartNeutral = true;
	if ( VF.bStartNeutral )
	{
		VF.Activate(255);
	}
	else
	{
		ForEach WorldInfo.AllNavigationPoints(class'UTGameObjective',O)
		{
			NewDist = VSize(VF.Location - O.Location);
			if ( (Best == None) || (NewDist < BestDist) )
			{
				Best = O;
				BestDist = NewDist;
			}
		}

		if ( Best != None )
			VF.Activate(Best.DefenderTeamIndex);
		else
			VF.Activate(255);
	}
}


static function int OrderToIndex(int Order)
{
	return Order;
}

function ViewObjective(PlayerController PC)
{
	local int i,Index,Score;
	local Controller C;

	if (WorldInfo.GRI.PRIArray.Length==0)
	{
		return;
	}

	// Prime the score
	Score = -1;
	Index = -1;
	for ( i=0; i<WorldInfo.GRI.PRIArray.Length; i++ )
	{
		if (WorldInfo.GRI.PRIArray[i].Score > Score)
		{
			C = Controller(WorldInfo.GRI.PRIArray[i].Owner);
			if (C!=none && C.Pawn != none)
			{
				Score = WorldInfo.GRI.PRIArray[i].Score;
				Index = i;
			}
		}
	}

	if (Index == -1)
	{
		return;
	}

	if ( Index>=0 && Index<WorldInfo.GRI.PRIArray.Length )
	{
		PC.SetViewTarget( Controller(WorldInfo.GRI.PRIArray[Index].Owner).Pawn );
	}
}

function bool PickupQuery(Pawn Other, class<Inventory> ItemClass, Actor Pickup)
{
	if (bDemoMode)
	{
		if (ItemClass!=class'UTGame.UTWeap_ShockRifle' || ItemClass!=class'UTGame.UTWeap_RocketLauncher')
			return false;
	}

	return Super.PickupQuery(Other, ItemClass, Pickup);
}

function GetServerDetails( out ServerResponseLine ServerState )
{
	Super.GetServerDetails(ServerState);
	AddServerDetail( ServerState, "GameStats", GameStats != None );
}

function EndLogging(string Reason)
{

	Super.EndLogging(Reason);

	if (GameStats == None)
		return;

	GameStats.Destroy();
	GameStats = None;
}

function string DecodeEvent(name EventType, int TeamNo, string InstigatorName, string AdditionalName, class<object> AdditionalObj)
{
	local string s;
	if (EventType == 'TeamDeathWhileTyping')
	{
		return AdditionalName@"killed his teammate"@InstigatorName@"while he was typing! ("$AdditionalObj$")";
	}
	else if (EventType == 'DeathWhileTyping')
	{
		return AdditionalName@"killed"@InstigatorName@"while he was typing! ("$AdditionalObj$")";
	}
	else if (EventType == 'TeamDeath')
	{
		return AdditionalName@"killed his teammate"@InstigatorName$" ("$AdditionalObj$")";
	}
	else if (EventType == 'TeamDeath')
	{
		return AdditionalName@"killed"@InstigatorName$" ("$AdditionalObj$")";
	}
	else if (EventType == 'VehicleKill')
	{
		return InstigatorName$"blew up a"@AdditionalObj;
	}
	else if (EventType == 'Connect')
	{
		return InstigatorName$"has joined the game!";
	}
	else if (EventType == 'Reconnect')
	{
		return InstigatorName$"has returned to the game!";
	}
	else if (EventType == 'Disconnect')
	{
		return InstigatorName$"has left the game!";
	}
	else if (EventType == 'MatchBegins')
	{
		return "... The match begins ...";
	}
	else if (EventType == 'NewRound')
	{
		return "... Next Round ...";
	}
	else if (Left(""$EventType,10) ~= "MatchEnded")
	{
		s = ""$EventType;
		return "... Match Ended ("$Right(s,len(s)-10)$") ...";
	}
	else
		return "";
}

function AddMutator(string mutname, optional bool bUserAdded)
{
	if ( InStr(MutName,".")<0 )
	{
		MutName = "UTGame."$MutName;
	}

	Super.AddMutator(mutname, bUserAdded);
}

/**
 * This function allows the server to override any requested teleport attempts from a client
 *
 * @returns 	returns true if the teleport is allowed
 */
function bool AllowClientToTeleport(UTPlayerReplicationInfo ClientPRI, Actor DestinationActor)
{
	return true;
}

/** displays the path to the given base for the given player */
function ShowPathTo(PlayerController P, int TeamNum);

event HandleSeamlessTravelPlayer(Controller C)
{
	local UTBot B;
	local string BotName;
	local int BotTeamIndex;

	B = UTBot(C);
	if (B != None && B.Class != BotClass)
	{
		// re-create bot
		BotName = B.PlayerReplicationInfo.PlayerName;
		BotTeamIndex = B.GetTeamNum();
		B.Destroy();
		AddBot(BotName, (BotTeamIndex != 255), BotTeamIndex);
	}
	else
	{
		Super.HandleSeamlessTravelPlayer(C);

		// make sure bots get a new squad
		if (B != None && B.Squad == None)
		{
			GetBotTeam().AddToTeam(B);
		}
	}
}

/** @return an objective that should be recommended to the given player based on their auto objective settings and the current game state */
function UTGameObjective GetAutoObjectiveFor(UTPlayerController PC);

/** @return the first mutator in the mutator list that's a UTMutator */
function UTMutator GetBaseUTMutator()
{
	local Mutator M;
	local UTMutator UTMut;

	for (M = BaseMutator; M != None; M = M.NextMutator)
	{
		UTMut = UTMutator(M);
		if (UTMut != None)
		{
			return UTMut;
		}
	}

	return None;
}

/** parses the given player's recognized speech into bot orders, etc */
function ProcessSpeechRecognition(UTPlayerController Speaker, const out array<SpeechRecognizedWord> Words)
{
	local UTMutator UTMut;

	UTMut = GetBaseUTMutator();
	if (UTMut != None)
	{
		UTMut.ProcessSpeechRecognition(Speaker, Words);
	}
}

/**
 * Write player scores used in skill calculations
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
			PlayerScores[Index].Score = GameReplicationInfo.PRIArray[Index].Score;
			// Each player is on their own team (rated as individuals)
			PlayerScores[Index].TeamId = Index;
		}
		// Sorts the scores into relative positions (1st, 2nd, 3rd, etc.)
		SortPlayerScores(PlayerScores);
		// Now write out the scores
		OnlineSub.StatsInterface.WriteOnlinePlayerScores(PlayerScores);
	}
}

/**
 * Sorts the scores and assigns relative positions to the players
 *
 * @param PlayerScores the raw scores before sorting and relative position setting
 */
native function SortPlayerScores(out array<OnlinePlayerScore> PlayerScores);

/**
 * Returns the next map to play.  If we are in story mode, we need to go back to the map selection menu
 */
function string GetNextMap()
{
	local bool SPResult;
	local int GameIndex, MapIndex;
	local string MapName;

	if ( SinglePlayerMissionIndex >= 0)
	{
		// TODO Add code to determine a win or loss

		SPResult = GetSinglePlayerResult();
		return "UTM-MissionSelection?SPResult="$string(SPResult);

	}
	else
	{
		if (VoteCollector != none && VoteCollector.bVoteDecided )
		{
			MapName = VoteCollector.GetWinningMap();
			if (MapName != "")
			{
				return MapName;
			}
		}

		GameIndex = GameSpecificMapCycles.Find('GameClassName', Class.Name);
		if (GameIndex != INDEX_NONE)
		{
			MapIndex = GameSpecificMapCycles[GameIndex].Maps.Find(string(WorldInfo.GetPackageName()));
			if (MapIndex != INDEX_NONE && MapIndex + 1 < GameSpecificMapCycles[GameIndex].Maps.length)
			{
				return GameSpecificMapCycles[GameIndex].Maps[MapIndex + 1];
			}
			else
			{
				return GameSpecificMapCycles[GameIndex].Maps[0];
			}
		}
		else
		{
			return "";
		}
	}
}

function ProcessServerTravel(string URL)
{
	local UTPlayerController PC;

	// clear bInitialProcessingComplete for all travelling players
	// this will be done on the client in UTPlayerController::PreClientTravel()
	foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		PC.bInitialProcessingComplete = false;
	}

	Super.ProcessServerTravel(URL);
}

/**
 * @Retuns the results of the match.
 */
function bool GetSinglePlayerResult()
{
	local UTPlayerController PC;
	foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		if ( PC != none && LocalPlayer(PC.Player) != none )
		{
			return IsAWinner(PC);
		}
	}

	// Default to a winner
	return true;
}


defaultproperties
{
	HUDType=class'UTGame.UTHUD'
	ScoreBoardType=class'UTGame.UTScoreboard'
	PlayerControllerClass=class'UTGame.UTPlayerController'
	ConsolePlayerControllerClass=class'UTGame.UTConsolePlayerController'
	DefaultPawnClass=class'UTPawn'
	PlayerReplicationInfoClass=class'UTGame.UTPlayerReplicationInfo'
	GameReplicationInfoClass=class'UTGame.UTGameReplicationInfo'
	DeathMessageClass=class'UTDeathMessage'
	BotClass=class'UTBot'
   	bRestartLevel=False
	bDelayedStart=True
	bTeamScoreRounds=false
	bUseSeamlessTravel=true
	bWeaponStay=true

	bLoggingGame=true
	bAutoNumBots=false
	CountDown=4
	bPauseable=False
	EndMessageWait=2
	DefaultMaxLives=0

	Callsigns[0]="ALPHA"
	Callsigns[1]="BRAVO"
	Callsigns[2]="CHARLIE"
	Callsigns[3]="DELTA"
	Callsigns[4]="ECHO"
	Callsigns[5]="FOXTROT"
	Callsigns[6]="GOLF"
	Callsigns[7]="HOTEL"
	Callsigns[8]="INDIA"
	Callsigns[9]="JULIET"
	Callsigns[10]="KILO"
	Callsigns[11]="LIMA"
	Callsigns[12]="MIKE"
	Callsigns[13]="NOVEMBER"
	Callsigns[14]="OSCAR"

	Acronym="???"

	MaleBackupNames[0]="Shiva"
	MaleBackupNames[1]="Ares"
	MaleBackupNames[2]="Reaper"
	MaleBackupNames[3]="Samurai"
	MaleBackupNames[4]="Loki"
	MaleBackupNames[5]="CuChulainn"
	MaleBackupNames[6]="Thor"
	MaleBackupNames[7]="Talisman"
	MaleBackupNames[8]="Paladin"
	MaleBackupNames[9]="Scythe"
	MaleBackupNames[10]="Jugular"
	MaleBackupNames[11]="Slash"
	MaleBackupNames[12]="Chisel"
	MaleBackupNames[13]="Chief"
	MaleBackupNames[14]="Prime"
	MaleBackupNames[15]="Oligarch"
	MaleBackupNames[16]="Caliph"
	MaleBackupNames[17]="Duce"
	MaleBackupNames[18]="Kruger"
	MaleBackupNames[19]="Saladin"
	MaleBackupNames[20]="Patriarch"
	MaleBackupNames[21]="Wyrm"
	MaleBackupNames[22]="Praetorian"
	MaleBackupNames[23]="Moghul"
	MaleBackupNames[24]="Assassin"
	MaleBackupNames[25]="Bane"
	MaleBackupNames[26]="Svengali"
	MaleBackupNames[27]="Oblivion"
	MaleBackupNames[28]="Magnate"
	MaleBackupNames[29]="Hadrian"
	MaleBackupNames[30]="Dirge"
	MaleBackupNames[31]="Rajah"

	FemaleBackupNames[0]="Shonna"
	FemaleBackupNames[1]="Athena"
	FemaleBackupNames[2]="Charm"
	FemaleBackupNames[3]="Voodoo"
	FemaleBackupNames[4]="Noranna"
	FemaleBackupNames[5]="Ranu"
	FemaleBackupNames[6]="Chasm"
	FemaleBackupNames[7]="Lynx"
	FemaleBackupNames[8]="Elyss"
	FemaleBackupNames[9]="Malice"
	FemaleBackupNames[10]="Verdict"
	FemaleBackupNames[11]="Kismet"
	FemaleBackupNames[12]="Wyrd"
	FemaleBackupNames[13]="Qira"
	FemaleBackupNames[14]="Exodus"
	FemaleBackupNames[15]="Grimm"
	FemaleBackupNames[16]="Brutality"
	FemaleBackupNames[17]="Adamant"
	FemaleBackupNames[18]="Ruin"
	FemaleBackupNames[19]="Moshica"
	FemaleBackupNames[20]="Demise"
	FemaleBackupNames[21]="Shara"
	FemaleBackupNames[22]="Pestilence"
	FemaleBackupNames[23]="Quark"
	FemaleBackupNames[24]="Fiona"
	FemaleBackupNames[25]="Ulanna"
	FemaleBackupNames[26]="Kara"
	FemaleBackupNames[27]="Scourge"
	FemaleBackupNames[28]="Minerva"
	FemaleBackupNames[29]="Woe"
	FemaleBackupNames[30]="Coral"
	FemaleBackupNames[31]="Torment"
	//@todo steve bAllowVehicles=false

	DefaultInventory(0)=class'UTWeap_Enforcer'
	DefaultInventory(1)=class'UTWeap_ImpactHammer'

	LastTaunt(0)=-1
	LastTaunt(1)=-1

	VictoryMessageClass=class'UTGame.UTVictoryMessage'

	MapPrefixes(0)=(Prefix="DM",GameType="UTGame.UTDeathmatch", AdditionalGameTypes=("UTGame.UTTeamGame","UTGame.UTDuelGame"))
	MapPrefixes(1)=(Prefix="CTF",GameType="UTGameContent.UTCTFGame_Content")
	MapPrefixes(2)=(Prefix="WAR",GameType="UTGameContent.UTOnslaughtGame_Content")
	MapPrefixes(3)=(Prefix="VCTF",GameType="UTGameContent.UTVehicleCTFGame_Content")
	MapPrefixes(4)=(Prefix="DG",GameType="UTGame.DemoGame")
	MapPrefixes(5)=(Prefix="UTM",GameType="UTGame.UTMissionSelectionGame")
	MapPrefixes(6)=(Prefix="UTCIN",GameType="UTGame.UTCinematicGame")
	MapPrefixes(7)=(Prefix="ONS",GameType="UTGameContent.UTOnslaughtGame_Content")

	// Voice is only transmitted when the player is actively pressing a key
	bRequiresPushToTalk=true

	bExportMenuData=true

 	MidGameMenuTemplate=UTUIScene_MidGameMenu'UI_Scenes_HUD.Menus.MidGameMenu'

	SpawnProtectionTime=+2.0
}
