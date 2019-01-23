/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base configuration settings for DM match
 *
 * NOTE: This class will normally be code generated, but the tool doesn't exist yet
 */
class ExampleDMOnlineGameSettings extends OnlineGameSettings;

/** Dynamic values that can be exposed to match making */
const PROP_NonStandardOptions = 0x10000006;
const PROP_DedicatedServer = 0x10000009;

/** Set of localized string settings */
const LSS_MapName = 0;

/** Possible map name entries */
const MAPNAME_Entry = 0;
const MAPNAME_ExampleEntry = 1;

const GAME_MODE									  = 0x0000800B;
const GAME_MODE_DM								  = 0;
const GAME_MODE_TDM								  = 1;

defaultproperties
{
	// String settings and their mappings
	LocalizedSettingsMappings(0)=(Id=LSS_MapName,Name="Map Name",ValueMappings=((Id=MAPNAME_Entry),(Id=MAPNAME_ExampleEntry)))
	LocalizedSettings(0)=(Id=LSS_MapName,ValueIndex=1)
	LocalizedSettingsMappings(1)=(Id=GAME_MODE,Name="Game Mode",ValueMappings=((Id=GAME_MODE_DM),(Id=GAME_MODE_TDM)))
	LocalizedSettings(1)=(Id=GAME_MODE,ValueIndex=0)
	// Properties and their mappings
	Properties(0)=(PropertyId=PROP_NonStandardOptions,Data=(Type=SDT_Int32,Value1=0))
	Properties(1)=(PropertyId=PROP_DedicatedServer,Data=(Type=SDT_Int32,Value1=0))
	PropertyMappings(0)=(Id=PROP_NonStandardOptions,Name="NonStandardOptions")
	PropertyMappings(1)=(Id=PROP_DedicatedServer,Name="DedicatedServer")
}
