/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the list of settings the example game is interested and their default values
 *
 * NOTE: This class will normally be code generated, but the tool doesn't exist yet
 */
class ExampleProfileSettings extends OnlineProfileSettings;

/** Used to illustrate per game custom settings */
enum ECustomGameSettingsWeaponPref
{
	CGSWP_Dagger,
	CGSWP_Mace,
	CGSWP_Sword
};

/** Used to illustrate per game custom settings */
enum ECustomGameSettingsArmorPref
{
	CGSAP_Leather,
	CGSAP_Chainmail,
	CGSAP_Splintmail
};

// PSI_MAX is where to start. Can't inherit enums so hard code
const PreferredWeapon = 35;
const PreferredArmor = 36;

defaultproperties
{
	VersionNumber=10
	// Only read the properties that we care about
	ProfileSettingIds(0)=PSI_ControllerVibration
	ProfileSettingIds(1)=PSI_YInversion
	ProfileSettingIds(2)=PSI_VoiceMuted
	ProfileSettingIds(3)=PSI_VoiceThruSpeakers
	ProfileSettingIds(4)=PSI_VoiceVolume
	ProfileSettingIds(5)=PSI_GameDifficulty
	ProfileSettingIds(6)=PSI_ControllerSensitivity
	ProfileSettingIds(7)=PSI_PreferredColor1
	ProfileSettingIds(8)=PSI_PreferredColor2
	ProfileSettingIds(9)=PSI_AutoAim
	ProfileSettingIds(10)=PSI_AutoCenter
	ProfileSettingIds(11)=PSI_MovementControl
	// Defaults for the values if not specified by the online service
	DefaultSettings(0)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_ControllerVibration,Data=(Type=SDT_Float,Value1=PCVTO_On)))
	DefaultSettings(1)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_YInversion,Data=(Type=SDT_Int32,Value1=PYIO_Off)))
	DefaultSettings(2)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_VoiceMuted,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings(3)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_VoiceThruSpeakers,Data=(Type=SDT_Int32,Value1=PVTSO_On)))
	DefaultSettings(4)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_VoiceVolume,Data=(Type=SDT_Float,Value1=1)))
	DefaultSettings(5)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_GameDifficulty,Data=(Type=SDT_Int32,Value1=PDO_Normal)))
	DefaultSettings(6)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_ControllerSensitivity,Data=(Type=SDT_Int32,Value1=PCSO_Medium)))
	DefaultSettings(7)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PreferredColor1,Data=(Type=SDT_Int32,Value1=PPCO_Red)))
	DefaultSettings(8)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PreferredColor2,Data=(Type=SDT_Int32,Value1=PPCO_Blue)))
	DefaultSettings(9)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_AutoAim,Data=(Type=SDT_Int32,Value1=PAAO_On)))
	DefaultSettings(10)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_AutoCenter,Data=(Type=SDT_Int32,Value1=PACO_On)))
	DefaultSettings(11)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_MovementControl,Data=(Type=SDT_Int32,Value1=PMCO_L_Thumbstick)))

	// Custom game settings follow
	//
	// To read the data
	ProfileSettingIds(12)=PreferredWeapon
	ProfileSettingIds(13)=PreferredArmor
	// These are defaults for first time read
	DefaultSettings(12)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PreferredWeapon,Data=(Type=SDT_Int32,Value1=CGSWP_Sword)))
	DefaultSettings(13)=(Owner=OPPO_Game,ProfileSetting=(PropertyId=PreferredArmor,Data=(Type=SDT_Int32,Value1=CGSAP_Splintmail)))
	// These tell the UI what to display
	ProfileMappings(16)=(Id=PreferredWeapon,Name="Preferred Weapon",MappingType=VMT_IdMapped,ValueMappings=((Id=CGSWP_Dagger),(Id=CGSWP_Mace),(Id=CGSWP_Sword)))
	ProfileMappings(17)=(Id=PreferredArmor,Name="Preferred Armor",MappingType=VMT_IdMapped,ValueMappings=((Id=CGSAP_Leather),(Id=CGSAP_Chainmail),(Id=CGSAP_Splintmail)))
}
