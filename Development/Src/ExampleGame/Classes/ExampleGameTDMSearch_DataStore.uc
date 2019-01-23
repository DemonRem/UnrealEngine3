/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineGameSearch to expose the
 * DM game search settings
 */
class ExampleGameTDMSearch_DataStore extends UIDataStore_OnlineGameSearch;

defaultproperties
{
	GameSearchClass=class'ExampleGame.ExampleTDMOnlineGameSearch'
	DefaultGameSettingsClass=class'ExampleGame.ExampleTDMOnlineGameSettings'
	Tag=ExampleGameTeamDMSearch
}
