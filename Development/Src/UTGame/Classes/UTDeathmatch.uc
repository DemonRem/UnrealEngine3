/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeathmatch extends UTGame
	config(game);

function bool WantsPickups(UTBot B)
{
	return true;
}

/** return a value based on how much this pawn needs help */
function int GetHandicapNeed(Pawn Other)
{
	local float ScoreDiff;

	if ( Other.PlayerReplicationInfo == None )
	{
		return 0;
	}

	// base handicap on how far pawn is behind top scorer
	GameReplicationInfo.SortPRIArray();
	ScoreDiff = GameReplicationInfo.PriArray[0].Score - Other.PlayerReplicationInfo.Score;

	if ( ScoreDiff < 3 )
	{
		// ahead or close
		return 0;
	}
	return ScoreDiff/3;
}

/**
 * Writes out the stats for the game type
 */
function WriteOnlineStats()
{
	local UTLeaderboardWriteDM Stats;
	local UTPlayerController PC;
	local UTPlayerReplicationInfo PRI;

	// Only calc this if the subsystem can write stats
	if (OnlineSub != None && OnlineSub.StatsInterface != None)
	{
		Stats = UTLeaderboardWriteDM(new OnlineStatsWriteClass);
		// Iterate through the playercontroller list updating the stats
		foreach WorldInfo.AllControllers(class'UTPlayerController',PC)
		{
			PRI = UTPlayerReplicationInfo(PC.PlayerReplicationInfo);
			if (PRI != None)
			{
				Stats.SetKills(PRI.Kills);
				Stats.SetDeaths(PRI.Deaths);
				// This will copy them into the online subsystem where they will be
				// sent via the network in EndOnlineGame()
				OnlineSub.StatsInterface.WriteOnlineStats(PC.PlayerReplicationInfo.UniqueId,Stats);
			}
		}
	}
}

defaultproperties
{
    Acronym="DM"
    MapPrefix="DM"
    DefaultEnemyRosterClass="UTGame.UTDMRoster"

	// Class used to write stats to the leaderboard
	OnlineStatsWriteClass=class'UTGame.UTLeaderboardWriteDM'
}
