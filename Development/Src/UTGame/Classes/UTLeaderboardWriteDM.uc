/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** The class that writes the DM stats */
class UTLeaderboardWriteDM extends OnlineStatsWrite;

`include(UTOnlineConstants.uci)

/**
 * Sets the kills stat to a value
 *
 * @param Kills the number of kills for the player
 */
function SetKills(int Kills)
{
	SetIntStat(PROPERTY_KILLS,Kills);
	// Make sure to create a non-zero rating value
	if (Kills == 0)
	{
		Kills = -1;
	}
	SetIntStat(PROPERTY_LEADERBOARDRATING,Kills);
}

/**
 * Sets the deaths stat to a value
 *
 * @param Deaths the number of deaths for the player
 */
function SetDeaths(int Deaths)
{
	SetIntStat(PROPERTY_DEATHS,Deaths);
}

defaultproperties
{
	// These are the stats we are collecting
	Properties=((PropertyId=PROPERTY_KILLS,Data=(Type=SDT_Int32,Value1=0)),(PropertyId=PROPERTY_DEATHS,Data=(Type=SDT_Int32,Value1=0)),(PropertyId=PROPERTY_LEADERBOARDRATING,Data=(Type=SDT_Int32,Value1=0)))

	// Sort the leaderboard by this property
	RatingId=PROPERTY_LEADERBOARDRATING

	// Views being written to depending on type of match (ranked or player)
	ViewIds=(STATS_VIEW_DM_PLAYER_ALLTIME)
	ArbitratedViewIds=(STATS_VIEW_DM_RANKED_ALLTIME)
}
