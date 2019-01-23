/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** Holds the settings that are common to all match types */
class UTGameSettingsCommon extends OnlineGameSettings;

`include(UTOnlineConstants.uci)

/**
 * Sets the property that advertises the custom map name
 *
 * @param MapName the string to use
 */
function SetCustomMapName(string MapName)
{
	local SettingsData CustomMap;

	if (Properties[4].PropertyId == PROPERTY_CUSTOMMAPNAME)
	{
		SetSettingsDataString(CustomMap,MapName);
		Properties[4].Data = CustomMap;
	}
	else
	{
		`Log("Failed to set custom map name because property order changed!");
	}
}

/** @todo: Temporary override of the build URL function that will output string values for certain contexts instead of their value index. */
function BuildURL(out string OutURL)
{
	local int SettingIdx;
	local int OutValue;
	local float Ratio;
	local name SettingName;
	local name PropertyName;

	OutURL = "";

	// Iterate through localized settings and append them
	for (SettingIdx = 0; SettingIdx < LocalizedSettings.length; SettingIdx++)
	{
		SettingName = GetStringSettingName(LocalizedSettings[SettingIdx].Id);

		if (SettingName != '')
		{
			// For certain context's, output their string value name instead of their value index.
			switch(LocalizedSettings[SettingIdx].Id)
			{
			case CONTEXT_GOALSCORE: 
			case CONTEXT_TIMELIMIT: 
				OutURL = OutURL $ "?" $ SettingName $ "=" $ GetStringSettingValueNameByName(SettingName);
				break;
			case CONTEXT_NUMBOTS:
				//@todo: This is probably a hack, we will change this if we convert our contexts over to a property.
				if(GetStringSettingValue(CONTEXT_NUMBOTS, OutValue))
				{
					// gotta add 1, since the number of players we want is the number of bots + the number of players
					OutURL = OutURL $ "?NumPlay=" $ (OutValue+1);
				}
				break;
			case CONTEXT_VSBOTS:
				if(GetStringSettingValue(CONTEXT_VSBOTS, OutValue))
				{
					Ratio = -1.0f;

					// Convert the vs bot context value to a floating point ratio of bots to players.
					switch(OutValue)
					{
					case CONTEXT_VSBOTS_1_TO_2:
						Ratio = 0.5f;
						break;
					case CONTEXT_VSBOTS_1_TO_1:
						Ratio = 1.0f;
						break;
					case CONTEXT_VSBOTS_2_TO_1:
						Ratio = 2.0f;
						break;
					case CONTEXT_VSBOTS_3_TO_1:
						Ratio = 3.0f;
						break;
					case CONTEXT_VSBOTS_4_TO_1:
						Ratio = 4.0f;
						break;
					default:
						break;
					}

					if(Ratio > 0)
					{
						OutURL = OutURL $ "?VsBots=" $ Ratio;
					}
					
				}
				break;
			default:
				OutURL = OutURL $ "?" $ SettingName $ "=" $ LocalizedSettings[SettingIdx].ValueIndex;
				break;
			}
		}
	}

	// Now add all properties the same way
	for (SettingIdx = 0; SettingIdx < Properties.length; SettingIdx++)
	{
		PropertyName = GetPropertyName(Properties[SettingIdx].PropertyId);
		if (PropertyName != '')
		{
			OutURL = OutURL $ "?" $ PropertyName $ "=" $ GetPropertyAsString(Properties[SettingIdx].PropertyId);
		}
	}
}

defaultproperties
{
	// Default to 32 public and no private (user sets)
	// NOTE: UI will have to enforce proper numbers on consoles
	NumPublicConnections=32
	NumPrivateConnections=0

	// Contexts and their mappings
	LocalizedSettings(0)=(Id=CONTEXT_GAME_MODE,ValueIndex=CONTEXT_GAME_MODE_DM,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(0)=(Id=CONTEXT_GAME_MODE,Name="GameMode",ValueMappings=((Id=CONTEXT_GAME_MODE_DM)))

	LocalizedSettings(1)=(Id=CONTEXT_BOTSKILL,ValueIndex=CONTEXT_BOTSKILL_AUTOADJUSTSKILL,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(1)=(Id=CONTEXT_BOTSKILL,Name="BotSkill",ValueMappings=((Id=CONTEXT_BOTSKILL_AUTOADJUSTSKILL),(Id=CONTEXT_BOTSKILL_NOVICE),(Id=CONTEXT_BOTSKILL_AVERAGE),(Id=CONTEXT_BOTSKILL_EXPERIENCED),(Id=CONTEXT_BOTSKILL_SKILLED),(Id=CONTEXT_BOTSKILL_ADEPT),(Id=CONTEXT_BOTSKILL_MASTERFUL),(Id=CONTEXT_BOTSKILL_INHUMAN),(Id=CONTEXT_BOTSKILL_GODLIKE)))

	LocalizedSettings(2)=(Id=CONTEXT_MAPNAME,ValueIndex=CONTEXT_MAPNAME_CUSTOM,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(2)=(Id=CONTEXT_MAPNAME,Name="MapName",ValueMappings=((Id=CONTEXT_MAPNAME_CUSTOM)))

	LocalizedSettings(3)=(Id=CONTEXT_GOALSCORE,ValueIndex=CONTEXT_GOALSCORE_10,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(3)=(Id=CONTEXT_GOALSCORE,Name="GoalScore",ValueMappings=((Id=CONTEXT_GOALSCORE_5),(Id=CONTEXT_GOALSCORE_10),(Id=CONTEXT_GOALSCORE_15),(Id=CONTEXT_GOALSCORE_20),(Id=CONTEXT_GOALSCORE_30)))

	LocalizedSettings(4)=(Id=CONTEXT_TIMELIMIT,ValueIndex=CONTEXT_TIMELIMIT_10,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(4)=(Id=CONTEXT_TIMELIMIT,Name="TimeLimit",ValueMappings=((Id=CONTEXT_TIMELIMIT_5),(Id=CONTEXT_TIMELIMIT_10),(Id=CONTEXT_TIMELIMIT_15),(Id=CONTEXT_TIMELIMIT_20),(Id=CONTEXT_TIMELIMIT_30)))

	LocalizedSettings(5)=(Id=CONTEXT_NUMBOTS,ValueIndex=CONTEXT_NUMBOTS_4,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(5)=(Id=CONTEXT_NUMBOTS,Name="NumBots",ValueMappings=((Id=CONTEXT_NUMBOTS_0),(Id=CONTEXT_NUMBOTS_1),(Id=CONTEXT_NUMBOTS_2),(Id=CONTEXT_NUMBOTS_3),(Id=CONTEXT_NUMBOTS_4),(Id=CONTEXT_NUMBOTS_5),(Id=CONTEXT_NUMBOTS_6),(Id=CONTEXT_NUMBOTS_7),(Id=CONTEXT_NUMBOTS_8)))

	LocalizedSettings(6)=(Id=CONTEXT_PURESERVER,ValueIndex=CONTEXT_PURESERVER_YES,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(6)=(Id=CONTEXT_PURESERVER,Name="PureServer",ValueMappings=((Id=CONTEXT_PURESERVER_NO),(Id=CONTEXT_PURESERVER_YES)))

	LocalizedSettings(7)=(Id=CONTEXT_LOCKEDSERVER,ValueIndex=CONTEXT_LOCKEDSERVER_NO,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(7)=(Id=CONTEXT_LOCKEDSERVER,Name="LockedServer",ValueMappings=((Id=CONTEXT_LOCKEDSERVER_NO),(Id=CONTEXT_LOCKEDSERVER_YES)))

	LocalizedSettings(8)=(Id=CONTEXT_VSBOTS,ValueIndex=CONTEXT_VSBOTS_NONE,AdvertisementType=ODAT_OnlineService)
	LocalizedSettingsMappings(8)=(Id=CONTEXT_VSBOTS,Name="VsBots",ValueMappings=((Id=CONTEXT_VSBOTS_NONE),(Id=CONTEXT_VSBOTS_1_TO_2),(Id=CONTEXT_VSBOTS_1_TO_1),(Id=CONTEXT_VSBOTS_2_TO_1),(Id=CONTEXT_VSBOTS_3_TO_1),(Id=CONTEXT_VSBOTS_4_TO_1)))

	// Properties and their mappings
	Properties(0)=(PropertyId=PROPERTY_CUSTOMMAPNAME,Data=(Type=SDT_String),AdvertisementType=ODAT_QoS)
	PropertyMappings(0)=(Id=PROPERTY_CUSTOMMAPNAME,Name="CustomMapName")

	Properties(1)=(PropertyId=PROPERTY_CUSTOMGAMEMODE,Data=(Type=SDT_String),AdvertisementType=ODAT_QoS)
	PropertyMappings(1)=(Id=PROPERTY_CUSTOMGAMEMODE,Name="CustomGameMode")
}