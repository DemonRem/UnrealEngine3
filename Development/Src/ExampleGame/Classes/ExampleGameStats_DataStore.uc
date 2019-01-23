/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineStats to expose the
 * example game leaderboards
 */
class ExampleGameStats_DataStore extends UIDataStore_OnlineStats;

defaultproperties
{
	StatsReadClasses(0)=class'ExampleGame.ExampleGameStatsRead'
	Tag=ExampleGameLeaderboard
}