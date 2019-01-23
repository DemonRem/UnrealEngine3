/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** Common columns to be read from the DM tables */
class UTLeaderboardBaseDM extends OnlineStatsRead;

`include(UTOnlineConstants.uci)

defaultproperties
{
	ColumnIds=(STATS_COLUMN_DM_KILLS,STATS_COLUMN_DM_DEATHS)

	// The metadata for the columns
	ColumnMappings(0)=(Id=STATS_COLUMN_DM_KILLS,Name="Kills")
	ColumnMappings(1)=(Id=STATS_COLUMN_DM_DEATHS,Name="Deaths")
}
