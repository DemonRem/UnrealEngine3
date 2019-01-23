//=============================================================================
// GameReplicationInfo.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class GameReplicationInfo extends ReplicationInfo
	config(Game)
	native nativereplication;

`include(Core/Globals.uci)

var class<GameInfo> GameClass;				// Class of the server's gameinfo, assigned by GameInfo.

/**
 * The data store instance responsible for presenting state data for the current game session.
 */
var	private		CurrentGameDataStore		CurrentGameData;

var bool bStopCountDown;
var repnotify bool bMatchHasBegun;
var repnotify bool bMatchIsOver;
/**
 * Used to determine if the end of match/session clean up is needed. Game invites
 * might have already cleaned up the match/session so doing so again would break
 * the traveling to the invited game
 */
var bool bNeedsOnlineCleanup;
/** Used to determine who handles session ending */
var bool bIsArbitrated;

var databinding int  RemainingTime, ElapsedTime, RemainingMinute;
var databinding float SecondCount;
var databinding int GoalScore;
var databinding int TimeLimit;
var databinding int MaxLives;

var databinding TeamInfo Teams[2];

var() databinding globalconfig string ServerName;		// Name of the server, i.e.: Bob's Server.
var() databinding globalconfig string ShortName;		// Abbreviated name of server, i.e.: B's Serv (stupid example)
var() databinding globalconfig string AdminName;		// Name of the server admin.
var() databinding globalconfig string AdminEmail;		// Email address of the server admin.
var() databinding globalconfig int	  ServerRegion;		// Region of the game server.

var() databinding globalconfig string MessageOfTheDay;

var databinding Actor Winner;			// set by gameinfo when game ends

var		array<PlayerReplicationInfo> PRIArray;

/** This list mirror's the GameInfo's list of inactive PRI objects */
var		array<PlayerReplicationInfo> InactivePRIArray;

// stats

var int MatchID;
var bool bTrackStats;

cpptext
{
	// AActor interface.
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	if ( bNetDirty && (Role == ROLE_Authority) )
		bStopCountDown, Winner, Teams, bMatchHasBegun, bMatchIsOver, MatchID;

	if ( !bNetInitial && bNetDirty && (Role == ROLE_Authority) )
		RemainingMinute;

	if ( bNetInitial && (Role==ROLE_Authority) )
		GameClass,
		RemainingTime, ElapsedTime,MessageOfTheDay,
		ServerName, ShortName, AdminName,
		AdminEmail, ServerRegion, GoalScore, MaxLives, TimeLimit,
		bTrackStats, bIsArbitrated;
}


simulated event PostBeginPlay()
{
	local PlayerReplicationInfo PRI;
	if( WorldInfo.NetMode == NM_Client )
	{
		// clear variables so we don't display our own values if the server has them left blank
		ServerName = "";
		AdminName = "";
		AdminEmail = "";
		MessageOfTheDay = "";
	}

	SecondCount = WorldInfo.TimeSeconds;
	SetTimer(WorldInfo.TimeDilation, true);

	WorldInfo.GRI = self;

	// associate this GRI with the "CurrentGame" data store
	InitializeGameDataStore();

	ForEach DynamicActors(class'PlayerReplicationInfo',PRI)
	{
		AddPRI(PRI);
	}

	bNeedsOnlineCleanup = true;
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	Super.Reset();
	Winner = None;
}

/**
 * Called when this actor is destroyed
 */
simulated event Destroyed()
{
	Super.Destroyed();

	// de-associate this GRI with the "CurrentGame" data store
	CleanupGameDataStore();
}

simulated event Timer()
{
	if ( (WorldInfo.Game == None) || WorldInfo.Game.MatchIsInProgress() )
	{
		ElapsedTime++;
	}
	if ( WorldInfo.NetMode == NM_Client )
	{
		// sync remaining time with server once a minute
		if ( RemainingMinute != 0 )
		{
			RemainingTime = RemainingMinute;
			RemainingMinute = 0;
		}
	}
	if ( (RemainingTime > 0) && !bStopCountDown )
	{
		RemainingTime--;
		if ( WorldInfo.NetMode != NM_Client )
		{
			if ( RemainingTime % 60 == 0 )
			{
				RemainingMinute = RemainingTime;
			}
		}
	}
	SetTimer(WorldInfo.TimeDilation, true);
}

/**
 * Checks to see if two actors are on the same team.
 *
 * @return	true if they are, false if they aren't
 */
simulated event bool OnSameTeam(Actor A, Actor B)
{
	local byte ATeamIndex, BTeamIndex;

	if ( !GameClass.Default.bTeamGame || (A == None) || (B == None) )
		return false;

	ATeamIndex = A.GetTeamNum();
	if ( ATeamIndex == 255 )
		return false;

	BTeamIndex = B.GetTeamNum();
	if ( BTeamIndex == 255 )
		return false;

	return ( ATeamIndex == BTeamIndex );
}

simulated function PlayerReplicationInfo FindPlayerByID( int PlayerID )
{
    local int i;

    for( i=0; i<PRIArray.Length; i++ )
    {
	if( PRIArray[i].PlayerID == PlayerID )
	    return PRIArray[i];
    }
    return None;
}

simulated function AddPRI(PlayerReplicationInfo PRI)
{
	local int i;

	// Determine whether it should go in the active or inactive list
	if (!PRI.bIsInactive)
	{
		// make sure no duplicates
		for (i=0; i<PRIArray.Length; i++)
		{
			if (PRIArray[i] == PRI)
				return;
		}

		PRIArray[PRIArray.Length] = PRI;
	}
	else
	{
		// Add once only
		if (InactivePRIArray.Find(PRI) == INDEX_NONE)
		{
			InactivePRIArray[InactivePRIArray.Length] = PRI;
		}
	}

    if ( CurrentGameData == None )
    {
    	InitializeGameDataStore();
    }

	if ( CurrentGameData != None )
	{
		CurrentGameData.AddPlayerDataProvider(PRI);
	}
}

simulated function RemovePRI(PlayerReplicationInfo PRI)
{
    local int i;

    for (i=0; i<PRIArray.Length; i++)
    {
		if (PRIArray[i] == PRI)
		{
			if ( CurrentGameData != None )
			{
				CurrentGameData.RemovePlayerDataProvider(PRI);
			}

		    PRIArray.Remove(i,1);
			return;
		}
    }
}

/**
 * Assigns the specified TeamInfo to the location specified.
 *
 * @param	Index	location in the Teams array to place the new TeamInfo.
 * @param	TI		the TeamInfo to assign
 */
simulated function SetTeam( int Index, TeamInfo TI )
{
	if ( Index >= 0 && Index < ArrayCount(Teams) )
	{
	if ( CurrentGameData == None )
	{
    	    InitializeGameDataStore();
	}

		if ( CurrentGameData != None )
		{
			if ( Teams[Index] != None )
			{
				CurrentGameData.RemoveTeamDataProvider( Teams[Index] );
			}

			if ( TI != None )
			{
				CurrentGameData.AddTeamDataProvider(TI);
			}
		}

		Teams[Index] = TI;
	}
}

simulated function GetPRIArray(out array<PlayerReplicationInfo> pris)
{
    local int i;
    local int num;

    pris.Remove(0, pris.Length);
    for (i=0; i<PRIArray.Length; i++)
    {
		if (PRIArray[i] != None)
			pris[num++] = PRIArray[i];
    }
}

/**
  * returns true if P1 should be sorted before P2
  */
simulated function bool InOrder( PlayerReplicationInfo P1, PlayerReplicationInfo P2 )
{
	// spectators are sorted last
    if( P1.bOnlySpectator )
    {
		return P2.bOnlySpectator;
    }
    else if ( P2.bOnlySpectator )
	{
		return true;
	}

	// sort by Score
    if( P1.Score < P2.Score )
	{
		return false;
	}
    if( P1.Score == P2.Score )
    {
		// if score tied, use deaths to sort
		if ( P1.Deaths > P2.Deaths )
			return false;

		// keep local player highest on list
		if ( (P1.Deaths == P2.Deaths) && (PlayerController(P2.Owner) != None) && (LocalPlayer(PlayerController(P2.Owner).Player) != None) )
			return false;
	}
    return true;
}

simulated function SortPRIArray()
{
    local int i,j;
    local PlayerReplicationInfo tmp;

    for (i=0; i<PRIArray.Length-1; i++)
    {
	for (j=i+1; j<PRIArray.Length; j++)
	{
	    if( !InOrder( PRIArray[i], PRIArray[j] ) )
	    {
			tmp = PRIArray[i];
			PRIArray[i] = PRIArray[j];
			PRIArray[j] = tmp;
	    }
	}
    }
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'bMatchHasBegun' )
	{
		if (bMatchHasBegun)
		{
			WorldInfo.NotifyMatchStarted();
			OnlineSession_StartMatch();
		}
	}
	else if ( VarName == 'bMatchIsOver' )
	{
		if ( bMatchIsOver )
		{
			EndGame();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/**
 * Creates and registers a data store for the current game session.
 */
simulated function InitializeGameDataStore()
{
	local DataStoreClient DataStoreManager;

	DataStoreManager = class'UIInteraction'.static.GetDataStoreClient();
	if ( DataStoreManager != None )
	{
		CurrentGameData = CurrentGameDataStore(DataStoreManager.FindDataStore('CurrentGame'));
		if ( CurrentGameData != None )
		{
			CurrentGameData.CreateGameDataProvider(Self);
		}
		else
		{
			`log("Primary game data store not found!");
		}
	}
}

/**
 * Unregisters the data store for the current game session.
 */
simulated function CleanupGameDataStore()
{
	`log(`location,,'DataStore');
	if ( CurrentGameData != None )
	{
		CurrentGameData.ClearDataProviders();
	}

	CurrentGameData = None;
}

/**
 * Called on the server when the match has begin
 *
 * Network - Server and Client (Via ReplicatedEvent)
 */

simulated function StartMatch()
{
	// Notify the Online Session that the Match has begun

	if (WorldInfo.NetMode != NM_Standalone && !bMatchHasBegun)
	{
		OnlineSession_StartMatch();
	}

	bMatchHasBegun = true;

}

/**
 * Called on the server when the match is over
 *
 * Network - Server and Client (Via ReplicatedEvent)
 */

simulated function EndGame()
{
	bMatchIsOver = true;
	if (WorldInfo.NetMode != NM_StandAlone)
	{
		OnlineSession_EndMatch();
	}
}

/**
 * @Returns a reference to the OnlineGameInterface if one exists.
 *
 * Network: Client and Server
 */
simulated function OnlineGameInterface GetOnlineGameInterface()
{
	local OnlineSubsystem OnlineSub;

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		return OnlineSub.GameInterface;
	}
	else
	{
		`log("Could not get Online SubSystem");
	}
	return none;
}


/**
 * Signal that this match has begun
 *
 * NETWORK - Both Client and Server
 */

simulated event OnlineSession_StartMatch()
{
	local OnlineGameInterface GI;

	if ( WorldInfo.NetMode == NM_Client )
	{
		GI = GetOnlineGameInterface();

		if ( GI != None )
		{
			GI.StartOnlineGame();
		}
	}

}


/**
 * Signal that this match is over.
 *
 * NETWORK - Both Client and Server
 */

simulated event OnlineSession_EndMatch()
{
	local OnlineGameInterface GI;
	GI = GetOnlineGameInterface();

	if ( WorldInfo.NetMode == NM_Client )
	{
		// No Stats in Single Player Games
		if ( GI != none )
		{
			// Only clean up if required
			if (bNeedsOnlineCleanup && !bIsArbitrated)
			{
				// Send the end request, which will flush any stats
				GI.EndOnlineGame();
			}
		}
	}
}

/**
 * Signal that this session is over.  Called natively
 *
 * NETWORK - Both Client and Server
 */

simulated event OnlineSession_EndSession(bool bForced)
{
	local OnlineGameInterface GI;
	GI = GetOnlineGameInterface();
	if ( bForced || WorldInfo.NetMode != NM_StandAlone )
	{
		// Only clean up if required
		if (bNeedsOnlineCleanup)
		{
			GI.DestroyOnlineGame();
		}
	}
}

/** Is the current gametype a multiplayer game? */
simulated function bool IsMultiplayerGame()
{
	return (WorldInfo.NetMode != NM_Standalone);
}

/** Is the current gametype a coop multiplayer game? */
simulated function bool IsCoopMultiplayerGame()
{
	return FALSE;
}

defaultproperties
{
	TickGroup=TG_DuringAsyncWork

	bStopCountDown=true
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=True
}
