/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTLeaderboardBaseRankedSkill extends OnlineStatsRead;

`include(UTOnlineConstants.uci)

defaultproperties
{
	ColumnIds=(STATS_COLUMN_TRUESKILL,STATS_COLUMN_SKILL_GAMESPLAYED)

	// The metadata for the columns
	ColumnMappings(0)=(Id=STATS_COLUMN_TRUESKILL,Name="TrueSkill")
	ColumnMappings(1)=(Id=STATS_COLUMN_SKILL_GAMESPLAYED,Name="NumGamesPlayed")
}
