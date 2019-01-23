/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base game search for a DM match.
 */
class UTGameSearchDM extends OnlineGameSearch;

`include(UTOnlineConstants.uci)

defaultproperties
{
	GameSettingsClass=class'UTGame.UTGameSettingsDM'

	// Which server side query to execute
	Query=(ValueIndex=QUERY_DM)

	MaxSearchResults=25

	// Set the DM specific game mode that we are searching for
	LocalizedSettings(0)=(Id=CONTEXT_GAME_MODE,ValueIndex=CONTEXT_GAME_MODE_DM,AdvertisementType=ODAT_OnlineService)
//@todo add other search parameters here

	// Specifies the filter to use for online services that don't have predefined queries
	FilterQuery=(OrClauses=((OrParams=((EntryId=CONTEXT_GAME_MODE,EntryType=OGSET_LocalizedSetting,ComparisonType=OGSCT_Equals))),(OrParams=((EntryId=CONTEXT_PURESERVER,EntryType=OGSET_LocalizedSetting,ComparisonType=OGSCT_Equals))),(OrParams=((EntryId=CONTEXT_LOCKEDSERVER,EntryType=OGSET_LocalizedSetting,ComparisonType=OGSCT_Equals)))))
}
