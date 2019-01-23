/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Warfare specific datastore for TDM game searches
 */
class UTDataStore_GameSearchDM extends UIDataStore_OnlineGameSearch;

`include(UTOnlineConstants.uci)

defaultproperties
{
	GameSearchCfgList(0)=(GameSearchClass=class'UTGame.UTGameSearchDM',DefaultGameSettingsClass=class'UTGame.UTGameSettingsDM',SearchResultsProviderClass=class'UTGame.UTUIDataProvider_SearchResult',SearchName="DM")
	Tag=UTGameSearch
}
