/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTGameStats extends Info
	Native;

/** Local Logging */

var FileWriter 	StatsLog;				// The Reference to the log file
var bool		bKeepLocalStatsLog;		// If True, write local stats to a log

/** Remote Stats */

var string		StatsRemoteAddress;
var bool		bKeepRemoteStatsLog;

/** Runtime variables */

var GameReplicationInfo 	GRI;		// Quick pointer to the GRI

/** Player Statistics */

struct native FireModeStat
{
	var	int						ShotsFired;
	var int						ShotsDamaged;
	var int						ShotsDirectHit;
};

struct native WeaponStat
{
	var	class<UTWeapon> 		WeaponType;				// Type of Weapon
	var array<FireModeStat>		FireStats;
	var	int						Combos;					// # of Combo's pulled off
};

struct native InventoryStat
{
	var class<Actor>			InventoryType;			// Type of inventory
	var	int						NoPickups;				// # of times it has been picked up
	var int						NoDrops;				// # of times it has been dropped
	var int						NoIntentionalDrops;		// # of times it has been dropped on purpose
};

struct native BonusStat
{
	var name					BonusType;
	var int						NoReceived;
};


struct native PlayerStat
{
	var	PlayerReplicationInfo	PRI;					// The PRI of this player.  If the PRI == none then he's no longer playing
	var string					GlobalID;				// The Global ID of this player
	var string					DisplayName;			// The name used when displaying info about this stat

    var int						TotalConnectTime;		// Amount of Time this player played
    var int						TotalScore;				// The Total Score of a player

	var int						NoConnects;				// # of times this player connected
	var int						NoDisconnects;			// # of times this player disconnected

	var array<WeaponStat>		WeaponStats;			// A collection of Weapon stats for a player
	var array<InventoryStat>	InventoryStats;			// A collection of Inventory stats for a player
	var array<BonusStat>		BonusStats;				// A collection of Bonus stats for a player

	var array<int>				ObjectiveStats;			// A collection of Objective Stats for a player

	var array<int>				KillStats;
	var array<int>				DeathStats;				// A collection of Indexes in to the GameStats for Player Kills
	var	int						NoSuicides;				// # of suicides
};

var array<PlayerStat>			PlayerStats;

/** Game Statistics */

struct native GameStat
{
	var int						TimeStamp;				// When did this happen
	var	name					GameStatType;			// What type of stat is this

	var int						Team;					// What team was this event for (-1 = all)

	var int						InstigatorID;			// The ID of the player who caused the stat
	var int						AdditionalID;			// Any additional Player ID's needed
	var class<object>			AdditionalData;			// Any additional data needed
};


var array<GameStat>				GameStats;

var array<Class<UTStatsSummary> >	StatSummaryClasses;
var array<UTStatsSummary>			StatSummaryObjects;

/************************************************************************************
 * Native Functions
 ************************************************************************************/

/**
 * Returns the name of the the current map
 */
native final function string GetMapFilename();

function string GetGlobalID(PlayerReplicationInfo PRI)	// TEMP: update when CDEKYs are active
{
	if (PRI!=None)
		return PRI.PlayerName;
	else
		return "";
}

/**
 * This function will generate a summary of the match
 *  - Needs to be made native and create various HTLM
 */
function Summary()
{
	local int i;

	for (i=0;i<StatSummaryObjects.Length;i++)
	{
		StatSummaryObjects[i].GenerateSummary();
	}
}

/************************************************************************************
 * Init / Shutdown / Misc Helper Functions
 ************************************************************************************/

/**
  * This is a catch all to make sure we clean up the logs
  */

event Destroyed()
{
	ShutdownStats();
	Super.Destroyed();
}

/**
 * Returns the time stamp for the current game
 */

function int GetTimeStamp()
{
	return int(WorldInfo.TimeSeconds);
}

/**
 * Returns the name of the map without the extension
 */

function String GetMapName()
{
	local string mapname;
	local string tab;
	tab = ""$Chr(9);
	mapname = ""$GetMapFilename();

	// Remove the file name extention .ut3
	if( InStr(mapname,".ut3") != -1 )
		mapname = Left( mapname, InStr(mapname,".ut3") );

	ReplaceText(mapname, tab, "_");

	return mapname;
}

/**
 * This function will find a Stat's ID for a given player.
 *
 * returns: -1 = not found
 */

function int GetStatsIDFromGlobalID(string GlobalID)
{
	local int i;

	for (i=0;i<PlayerStats.length;i++)
	{
		if (PlayerStats[i].GlobalID ~= GlobalID)
		{
			return i;
		}
	}

	return -1;
}

/************************************************************************************
 * PlayerStats Accessor Functions
 ************************************************************************************/


/**
 * Adds a new player to the list
 */

function int AddNewPlayer(Controller NewPlayer, string GlobalID)
{
	local int NewID;

	NewID = PlayerStats.Length;
	PlayerStats.Length = NewID + 1;

	PlayerStats[NewID].PRI = NewPlayer.PlayerReplicationInfo;
	PlayerStats[NewID].GlobalID = GlobalID;
	PlayerStats[NewID].DisplayName = NewPlayer.PlayerReplicationInfo.PlayerName;

	return NewID;
}

/**
 * Adds a link for a player dead
 */

function TrackPlayerDeathStat(int StatID, int DeathIndex)
{
	local int NewID;

	if (StatID>=0)
	{
		NewID = PlayerStats[StatID].DeathStats.Length;
		PlayerStats[StatID].DeathStats.Length = NewID + 1;

		PlayerStats[StatID].DeathStats[NewID] = DeathIndex;
	}
}

/**
 * Adds a link for a player dead
 */

function TrackPlayerKillStat(int StatID, int KillIndex)
{
	local int NewID;

	if (StatID>=0)
	{
		NewID = PlayerStats[StatID].KillStats.Length;
		PlayerStats[StatID].KillStats.Length = NewID + 1;

		PlayerStats[StatID].KillStats[NewID] = KillIndex;
	}
}


/**
 * Tracks a bonus given to a player
 */

function TrackPlayerBonus(int StatID, name BonusType)
{
	local int i;

	if (StatId < 0 || StatId >= PlayerStats.Length )
	{
		return;
	}


	for (i=0;i<PlayerStats[StatID].BonusStats.Length;i++)
	{
		if (PlayerStats[StatID].BonusStats[i].BonusType == BonusType)
		{
			PlayerStats[StatID].BonusStats[i].NoReceived++;
			StatLog(GetDisplayName(StatID)@"has received another"@BonusType@"bonus (x"$PlayerStats[StatID].BonusStats[i].NoReceived$")");
			return;
		}
	}

	i = PlayerStats[StatID].BonusStats.Length;
	PlayerStats[StatID].BonusStats.Length = i + 1;

	PlayerStats[StatID].BonusStats[i].BonusType= BonusType;
	PlayerStats[StatID].BonusStats[i].NoReceived=1;
	StatLog(GetDisplayName(StatID)@"has received his first "@BonusType@"bonus");
}



/**
 * Track a player adjusting his inventory
 */

function int TrackPlayerInventoryStat(int StatID, class<Actor> InvType, name Action)
{
	local int i,InvIndex;

	if (StatId < 0 || StatId >= PlayerStats.Length )
	{
		return -1;
	}


	InvIndex = -1;
	for (i=0;i<PlayerStats[StatID].InventoryStats.Length;i++)
	{
		if (PlayerStats[StatID].InventoryStats[i].InventoryType == InvType)
		{
			InvIndex = i;
			break;
		}
	}

	// If it doesn't exist, add the entry

	if (InvIndex<0)
	{
		InvIndex = PlayerStats[StatID].InventoryStats.Length;
		PlayerStats[StatID].InventoryStats.Length = InvIndex + 1;
		PlayerStats[StatID].InventoryStats[InvIndex].InventoryType = InvType;
	}

	// Track what happened

	if (Action == 'tossed')
	{
		PlayerStats[StatID].InventoryStats[InvIndex].NoIntentionalDrops++;
		StatLog(GetDisplayName(StatID)@"tossed"@PlayerStats[StatID].InventoryStats[InvIndex].InventoryType@"to a teammate (x"$PlayerStats[StatID].InventoryStats[InvIndex].NoIntentionalDrops$")");

	}
	else if (Action == 'dropped')
	{
		PlayerStats[StatID].InventoryStats[InvIndex].NoDrops++;
		StatLog(GetDisplayName(StatID)@"dropped"@PlayerStats[StatID].InventoryStats[InvIndex].InventoryType@" (x"$PlayerStats[StatID].InventoryStats[InvIndex].NoDrops$")");
	}
	else if (Action == 'pickup')
	{
		PlayerStats[StatID].InventoryStats[InvIndex].NoPickups++;
		StatLog(GetDisplayName(StatID)@"picked up"@PlayerStats[StatID].InventoryStats[InvIndex].InventoryType@" (x"$PlayerStats[StatID].InventoryStats[InvIndex].NoPickups$")");
	}

	return InvIndex;
}

/**
 * Add a weapon to be tracked.  This function returns the index to the weapon for quick access
 */

function int AddNewWeapon(int StatID, class<UTWeapon> WeaponType)
{
	local int i;

	if (StatId < 0 || StatId >= PlayerStats.Length )
	{
		return -1;
	}

	for (i=0;i<PlayerStats[StatID].WeaponStats.Length;i++)
	{
		if (PlayerStats[StatID].WeaponStats[i].WeaponType == WeaponType)
		{
			return i;
		}
	}

	i = PlayerStats[StatID].WeaponStats.Length;
	PlayerStats[StatID].WeaponStats.Length = i + 1;
	PlayerStats[StatID].WeaponStats[i].WeaponType = WeaponType;

	return i;
}

/**
 * Trace a weapon Stat.  AddWeapon must be called first
 */

function TrackWeaponStat(int StatID, int WeaponID, int FireMode, name Action)
{
	if (StatId < 0 || StatId >= PlayerStats.Length || WeaponID < 0 || WeaponID >= PlayerStats[StatID].WeaponStats.Length)
	{
		return;
	}

	if (Action == 'Combo')
	{
		PlayerStats[StatID].WeaponStats[WeaponID].Combos++;
		StatLog(GetDisplayName(StatID)@"performed a combo (x"$PlayerStats[StatID].WeaponStats[WeaponID].Combos$") with"@PlayerStats[StatID].WeaponStats[WeaponID].WeaponType);
	}
	else
	{
		// Make sure this firemode is represented

		if ( PlayerStats[StatID].WeaponStats[WeaponID].FireStats.Length< (FireMode+1) )
		{
			PlayerStats[StatID].WeaponStats[WeaponID].FireStats.Length = FireMode + 1;
		}

		// Handle the action

		if (Action=='directhit')
		{
			PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsDirectHit++;
			StatLog(GetDisplayName(StatID)@"scored a direct hit (x"$PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsDirectHit$") with"@PlayerStats[StatID].WeaponStats[WeaponID].WeaponType);
		}
		else if (Action=='hit')
		{
			PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsDamaged++;
			StatLog(GetDisplayName(StatID)@"scored a hit (x"$PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsDamaged$") with"@PlayerStats[StatID].WeaponStats[WeaponID].WeaponType);
		}
		else
		{
			PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsFired++;
			StatLog(GetDisplayName(StatID)@"Fired a shot (x"$PlayerStats[StatID].WeaponStats[WeaponID].FireStats[FireMode].ShotsFired$") with"@PlayerStats[StatID].WeaponStats[WeaponID].WeaponType);
		}
	}
}

/**
 * This function Adds an objective link to the game stats table
 */

function AddObjectiveLink(int StatID, int GameStatID)
{
	local int i;

	if (StatId >= 0 && StatId < PlayerStats.Length)
	{
		i = PlayerStats[StatID].ObjectiveStats.Length;
		PlayerStats[StatID].ObjectiveStats.Length = i + 1;

		PlayerStats[StatID].ObjectiveStats[i] = GameStatID;
	}
}

/**
 * This functions looks at a player's stats entry and returns the Total Connect time.
 */

function int GetPlayerConnectTime(int StatId)
{
	local int tm;
	if (StatId >= 0 && StatId < PlayerStats.Length)
	{
		tm = PlayerStats[StatId].TotalConnectTime;
		if (PlayerStats[StatID].PRI != none)
		{
			tm += WorldInfo.TimeSeconds - PlayerStats[StatID].PRI.StartTime;
		}
		return tm;
	}

	return 0;
}

/**
 * This functions looks at a player's stats entry and returns the Total Connect time.
 */

function int GetPlayerScore(int StatId)
{
	local int sc;
	if (StatId >= 0 && StatId < PlayerStats.Length)
	{
		sc = PlayerStats[StatId].TotalScore;
		if (PlayerStats[StatID].PRI != none)
		{
			sc += PlayerStats[StatID].PRI.Score;
		}
		return sc;
	}

	return 0;
}

/**
 * Returns the "Display name" for this stat player
 */

function string GetDisplayName(int StatId)
{
	if (StatId >=0 && StatID < PlayerStats.Length)
	{
		return PlayerStats[StatID].DisplayName;
	}
	else
	{
		return "None";
	}
}

/************************************************************************************
 * GameStats Accessor Functions
 ************************************************************************************/


/**
 * Adds a Game Stat to the collection
 */

function int AddGameStat(name NewStatType, int NewTeam, int NewInstigatorID, int NewAdditionalID, optional class<object> NewAdditionalData)
{
	local int NewID;
	local string s;
	NewID = GameStats.Length;
	GameStats.Length = NewID + 1;

	GameStats[NewID].TimeStamp		= GetTimeStamp();
	GameStats[NewID].GameStatType 	= NewStatType;
	GameStats[NewID].Team			= NewTeam;
	GameStats[NewID].InstigatorID 	= NewInstigatorID;
	GameStats[NewID].AdditionalID  	= NewAdditionalID;
	GameStats[NewID].AdditionalData	= NewAdditionalData;

	if (NewAdditionalID>=0)
	{
		s = "Additonal ID="@(NewAdditionalID < PlayerStats.Length ? PlayerStats[NewAdditionalID].DisplayName : "None");
	}

	if (NewAdditionalData != none)
	{
		s = s@"Data="@NewAdditionalData;
	}

	StatLog("Team="@NewTeam@"Instigator="@GetDisplayName(NewInstigatorID)@"at"@GameStats[NewID].TimeStamp, NewStatType );
	if (s != "")
	{
		StatLog(s,NewStatType);
	}

	return NewID;
}


/************************************************************************************
 * Interface Functions - Utility
 ************************************************************************************/

/**
 * Initialize the stats system.
 */

function Init()
{
	local int i,idx;
	local UTStatsSummary Sum;

	`log("Stats Collection system Active");

	if (bKeepLocalStatsLog)
	{
		StatsLog = spawn(class 'FileWriter');
		if (StatsLog!=None)
		{
			StatsLog.OpenFile("GameStats",FWFT_Log,,,true);
			`log("  Collecting Local Stats to:"@StatsLog.Filename);
		}
	}

	if (bKeepRemoteStatsLog)
	{
		`log("  Collecting Remote Stats to:"@StatsRemoteAddress);
	}

	for (i=0;i<StatSummaryClasses.length;i++)
	{
		Sum = new(self) StatSummaryClasses[i];
		if (Sum!=none)
		{
			idx = StatSummaryObjects.Length;
			StatSummaryObjects.Length = idx + 1;
			StatSummaryObjects[idx] = Sum;
			`log("  Added Summary Object:"@Sum);
		}
	}
}

/**
 * Shutdown the log system
 */
function ShutdownStats()
{
	Summary();

	if (StatsLog!=None)
	{
		StatsLog.Destroy();
	}
}

/**
 * Write a string to the stats log file if it exists
 */

function StatLog(string LogString, optional coerce string Section)
{
	local string OutStr;

	if (Section != "")
	{
		OutStr = "["$Section$"]"@LogString;
	}



	if (StatsLog!=None)
	{
		StatsLog.Logf(OutStr);
	}
}

/************************************************************************************
 * Interface Functions - Events
 ************************************************************************************/

function NextRoundEvent()
{
	AddGameStat('NewRound',-1,-1,-1);
}

function MatchBeginEvent()
{
	AddGameStat('MatchBegins',-1,-1,-1);
}

function MatchEndEvent(name Reason, PlayerReplicationInfo Winner)
{
	AddGameStat(Reason,Winner.GetTeamNum(),-1,-1);
}

/**
 * Connect Events get fired every time a player connects to a server
 *
 * returns the Player's Stats Index
 */

function ConnectEvent(Controller NewPlayer)
{
	local int StatsID;
	local string GlobalID;

	GlobalId = GetGlobalID(NewPlayer.PlayerReplicationInfo);
	StatsId = GetStatsIDFromGlobalID(GlobalID);

	if ( StatsID>=0 )
	{
		PlayerStats[StatsID].PRI = NewPlayer.PlayerReplicationInfo;
		PlayerStats[StatsID].DisplayName = NewPlayer.PlayerReplicationInfo.PlayerName;
		AddGameStat('Reconnect', -1, StatsID, -1);
	}
	else
	{
		StatsID = AddNewPlayer(NewPlayer, GlobalID);
		AddGameStat('Connect', -1, StatsID, -1);
	}

	PlayerStats[StatsID].NoConnects++;

}

// Connect Events get fired every time a player connects or leaves from a server
function DisconnectEvent(Controller DepartingPlayer)
{
	local int StatsID;
	local string GlobalID;

	GlobalId = GetGlobalID(DepartingPlayer.PlayerReplicationInfo);
	StatsId = GetStatsIDFromGlobalID(GlobalID);

	if (StatsID>=0)
	{
		AddGameStat('Disconnect', StatsID, -1, -1);
		PlayerStats[StatsID].PRI = none;
		PlayerStats[StatsID].DisplayName = DepartingPlayer.PlayerReplicationInfo.PlayerName;
		PlayerStats[StatsID].TotalConnectTime += WorldInfo.TimeSeconds - DepartingPlayer.PlayerReplicationInfo.StartTime;
		PlayerStats[StatsID].TotalScore += DepartingPlayer.PlayerReplicationInfo.Score;
		PlayerStats[StatsID].NoDisconnects++;
	}
}


function GameEvent(name EventType, int Team, PlayerReplicationInfo InstigatorPRI)
{
	local int InstigatorID;

	local int StatsID;
	local string GlobalID;


	InstigatorID = GetStatsIDFromGlobalID( GetGlobalID(InstigatorPRI) );
	if (InstigatorID>=0)
	{
		AddGameStat(EventType, Team, InstigatorID,-1);

		if (EventType=='NameChange')
		{
			GlobalId = GetGlobalID(InstigatorPRI);
			StatsId = GetStatsIDFromGlobalID(GlobalID);
			PlayerStats[StatsId].DisplayName = InstigatorPRI.PlayerName;
		}
	}
}

// KillEvents occur when a player kills, is killed, suicides
function KillEvent(PlayerReplicationInfo Killer, PlayerReplicationInfo Victim, class<DamageType> Damage)
{
	local int VictimStatsID, KillerStatsID;
	local string VictimGlobalID, KillerGlobalID;

	local int DeathStatIndex;
	local bool bTypeKill;
	local bool bTeamKill;


	VictimGlobalId = GetGlobalID(Victim);
	VictimStatsId = GetStatsIDFromGlobalID(VictimGlobalID);

	KillerGlobalId = GetGlobalID(Killer);
	KillerStatsId = GetStatsIDFromGlobalID(KillerGlobalID);

	bTeamKill = WorldInfo.GRI.OnSameTeam(Killer,Victim);

	// Type killers tracked as player event (redundant Typing, removed from kill line)
	if ( UTPlayerController(Victim.Owner)!= None && UTPlayerController(Victim.Owner).bIsTyping)
	{
		if ( Killer.Owner != Victim.Owner )
		{
			bTypeKill = true;
		}
	}

	if (bTypeKill)
	{
		if (bTeamKill)
			DeathStatIndex = AddGameStat('TeamDeathWhileTyping', Victim.GetTeamNum(), VictimStatsID, KillerStatsID);
		else
			DeathStatIndex = AddGameStat('DeathWhileTyping', Victim.GetTeamNum(), VictimStatsID, KillerStatsID);
	}
	else
	{
		if (bTeamKill)
			DeathStatIndex = AddGameStat('TeamDeath', Victim.GetTeamNum(), VictimStatsID, KillerStatsID, Damage);
		else
			DeathStatIndex = AddGameStat('Death', Victim.GetTeamNum(), VictimStatsID, KillerStatsID, Damage);
	}

	TrackPlayerKillStat(KillerStatsID, DeathStatIndex);
	TrackPlayerDeathStat(VictimStatsID, DeathStatIndex);

	if (KillerStatsID>=0 && KillerStatsID == VictimStatsID)
	{
		PlayerStats[KillerStatsID].NoSuicides++;
	}
}

function VehicleKillEvent(PlayerReplicationInfo Killer, class<UTVehicle> VehicleClass)
{
	local int InstigatorID;

	InstigatorID = GetStatsIDFromGlobalID( GetGlobalID(Killer) );
	AddGameStat('VehicleKill', Killer.GetTeamNum(), InstigatorID,-1, VehicleClass);
}

function BonusEvent(name BonusType, PlayerReplicationInfo BonusPlayer)
{
	local int StatsID;
	local string GlobalID;

	GlobalId = GetGlobalID( BonusPlayer );
	StatsId = GetStatsIDFromGlobalID(GlobalID);

	TrackPlayerBonus(StatsID, BonusType);
}

function int PickupInventoryEvent(class<Actor> PickupINV, PlayerReplicationInfo PickupPlayer, name Action)
{
	return TrackPlayerInventoryStat( GetStatsIDFromGlobalID( GetGlobalID(PickupPlayer) ), PickupINV, Action);
}

function PickupWeaponEvent(class<UTWeapon> PickupWeapon, PlayerReplicationInfo PickupPlayer, out int OwnerStatsID, out int WeaponStatsID)
{
	OwnerStatsID = GetStatsIDFromGlobalID( GetGlobalID(PickupPlayer) );
	WeaponStatsID = AddNewWeapon(OwnerStatsID, PickupWeapon);
}

function WeaponEvent(int OwnerStatsID, int WeaponStatsID, int CurrentFireMode, PlayerReplicationInfo PawnOwner, name Action)
{
	TrackWeaponStat(OwnerStatsID, WeaponStatsID, CurrentFireMode, Action);
}

function ObjectiveEvent(name StatType, int TeamNo, PlayerReplicationInfo InstigatorPRI, int ObjectiveIndex)
{
	local int GameStatID;
	local int InstigatorStatID;

	InstigatorStatID = GetStatsIDFromGlobalID(GetGlobalID(InstigatorPRI));

	GameStatID = AddGameStat(StatType, TeamNo, InstigatorStatID, ObjectiveIndex);
	AddObjectiveLink(InstigatorStatID,	GameStatID);
}

defaultproperties
{
	bKeepLocalStatsLog=false
	bKeepRemoteStatsLog=false
	StatSummaryClasses(0)=class'UTStats_LocalSummary'
	StatsRemoteAddress="127.1.1.1"
}


