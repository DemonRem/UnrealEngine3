/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Class that modifies the stats for example game.
 * NOTE: This class will normally be code generated, but the tool hasn't
 * been written yet
 */
class ExampleGameStatsWrite extends OnlineStatsWrite;

/** Stat ids used by example game */
const STATID_OrcsKilled					= 0x10000001;
const STATID_GoldCollected				= 0x10000002;
const STATID_LongBowAccuracyRating		= 0x10000003;
const STATID_ExampleGameRating			= 0x20000004;

/** Learderboards (views) used by example game */
const VIEWID_ExampleGameLeaderboard		= 1;

function AddToOrcsKilled(int Number)
{
	IncrementIntStat(STATID_OrcsKilled,Number);
}

function AddToGold(int Amount)
{
	IncrementIntStat(STATID_GoldCollected,Amount);
}

function SetLongBowAccuracy(float NewRating)
{
	SetFloatStat(STATID_LongBowAccuracyRating,NewRating);
}

defaultproperties
{
	// These are the stats we are collecting
	Properties=((PropertyId=STATID_OrcsKilled,Data=(Type=SDT_Int32,Value1=0)),(PropertyId=STATID_GoldCollected,Data=(Type=SDT_Int32,Value1=0)),(PropertyId=STATID_LongBowAccuracyRating,Data=(Type=SDT_Double,Value1=0)),(PropertyId=STATID_ExampleGameRating,Data=(Type=SDT_Int64,Value1=0)))
	// These are the views we are writing to
	ViewIds(0)=VIEWID_ExampleGameLeaderboard
}