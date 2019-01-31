/**
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineGameSearch to expose the
 * DM game search settings
 */
class ExampleGameDMSearch_DataStore extends UIDataStore_OnlineGameSearch;

defaultproperties
{
	GameSearchClass=class'ExampleGame.ExampleDMOnlineGameSearch'
	DefaultGameSettingsClass=class'ExampleGame.ExampleDMOnlineGameSettings'
	Tag=ExampleGameDMSearch
}