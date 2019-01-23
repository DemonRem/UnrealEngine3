/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineGameSettings to expose the
 * TDM game type settings
 */
class ExampleGameTDM_DataStore extends UIDataStore_OnlineGameSettings;

defaultproperties
{
	GameSettingsClass=class'ExampleGame.ExampleTDMOnlineGameSettings'
	Tag=ExampleGameTeamDMSettings
}