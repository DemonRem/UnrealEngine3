/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Warfare specific datastore for TDM game creation
 */
class UTDataStore_GameSettingsDM extends UIDataStore_OnlineGameSettings;

`include(UTOnlineConstants.uci)

defaultproperties
{
	GameSettingsCfgList(0)=(GameSettingsClass=class'UTGame.UTGameSettingsDM',SettingsName="DM")
	Tag=UTGameSettings
}