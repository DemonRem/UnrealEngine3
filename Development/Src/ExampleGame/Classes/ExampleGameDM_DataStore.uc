/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineGameSettings to expose the
 * DM game type settings
 */
class ExampleGameDM_DataStore extends UIDataStore_OnlineGameSettings;

defaultproperties
{
	GameSettingsClass=class'ExampleGame.ExampleDMOnlineGameSettings'
	Tag=ExampleGameDMSettings
}