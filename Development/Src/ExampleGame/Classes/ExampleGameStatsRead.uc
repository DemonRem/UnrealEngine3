/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Class that modifies the stats for example game.
 * NOTE: This class will normally be code generated, but the tool hasn't
 * been written yet
 */
class ExampleGameStatsRead extends OnlineStatsRead;

/** Stat ids used by example game */
const COLID_OrcsKilled					= 1;
const COLID_GoldCollected				= 2;
const COLID_LongBowAccuracyRating		= 3;

/** Learderboards (views) used by example game */
const VIEWID_ExampleGameLeaderboard		= 1;

defaultproperties
{
	// Read the sample leaderboard and columns 1, 2, and 3
	ViewId=VIEWID_ExampleGameLeaderboard
	ColumnIds=(COLID_OrcsKilled,COLID_GoldCollected,COLID_LongBowAccuracyRating)
	// The metadata for the columns
	ColumnMappings(0)=(Id=COLID_OrcsKilled)
	ColumnMappings(1)=(Id=COLID_GoldCollected)
	ColumnMappings(2)=(Id=COLID_LongBowAccuracyRating)
}
