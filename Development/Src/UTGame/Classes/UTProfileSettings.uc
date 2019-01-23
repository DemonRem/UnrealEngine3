/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * List of profile settings for UT
 */
class UTProfileSettings extends OnlineProfileSettings
	config(game);

`include(UTOnlineConstants.uci)

// Generic Yes/No Enum
enum EGenericYesNo
{
	UTPID_VALUE_NO,
	UTPID_VALUE_YES
};

// Gore Level Enum
enum EGoreLevel
{
	GORE_High,
	GORE_Low,
	GORE_None
};

// Announcer Setting Enum
enum EAnnouncerSetting
{
	ANNOUNCE_OFF,
	ANNOUNCE_MINIMAL,
	ANNOUNCE_FULL
};

// Network Type Enum
enum ENetworkType
{
	NETWORKTYPE_Unknown,
	NETWORKTYPE_Modem,
	NETWORKTYPE_ISDN,
	NETWORKTYPE_Cable,
	NETWORKTYPE_LAN
};

/** Post Processing Preset */
enum EPostProcessPreset
{
	PPP_Default,
	PPP_Soft,
	PPP_Lucent,
	PPP_Vibrant
};

// Possible Digital Button Actions
enum EDigitalButtonActions
{
	DBA_Fire,
	DBA_AltFire,
	DBA_Jump,
	DBA_Use,
	DBA_ToggleMelee,
	DBA_ShowScores,
	DBA_ShowMap,
	DBA_FeignDeath,
	DBA_ToggleSpeaking,
	DBA_ToggleMinimap,
	DBA_WeaponPicker,
	DBA_NextWeapon,
	DBA_BestWeapon,
	DBA_SmartDodge
};

// Possible Analog Stick Configurations
enum EAnalogStickActions
{
	ESA_Normal,
	ESA_SouthPaw,
	ESA_Legacy,
	ESA_LegacySouthPaw
};

// Profile settings ids definitions
const UTPID_CustomCharString = 301;

// Audio
const UTPID_SFXVolume = 360;
const UTPID_MusicVolume = 361;
const UTPID_VoiceVolume = 362;
const UTPID_AnnouncerVolume = 363;
const UTPID_AnnounceSetting = 364;
const UTPID_AutoTaunt = 365;
const UTPID_MessageBeep = 366;
const UTPID_TextToSpeechEnable = 367;
const UTPID_TextToSpeechTeamMessagesOnly = 368;

// Video
const UTPID_Gamma = 380;
const UTPID_PostProcessPreset = 381;
const UTPID_ScreenResolutionX = 382;
const UTPID_ScreenResolutionY = 383;
const UTPID_DefaultFOV = 384;

// Game
const UTPID_ViewBob = 400;
const UTPID_GoreLevel = 401;
const UTPID_DodgingEnabled = 402;
const UTPID_WeaponSwitchOnPickup = 403;
const UTPID_NetworkConnection = 404;
const UTPID_DynamicNetspeed = 405;
const UTPID_SpeechRecognition = 406;

// Input
const UTPID_MouseSmoothing = 420;
const UTPID_ReduceMouseLag = 421;
const UTPID_EnableJoystick = 422;
const UTPID_MouseSensitivityGame = 423;
const UTPID_MouseSensitivityMenus = 424;
const UTPID_MouseSmoothingStrength = 425;
const UTPID_MouseAccelTreshold = 426;
const UTPID_DodgeDoubleClickTime = 427;
const UTPID_TurningAccelerationFactor = 428;

const UTPID_GamepadBinding_ButtonA = 429;
const UTPID_GamepadBinding_ButtonB = 430;
const UTPID_GamepadBinding_ButtonX = 431;
const UTPID_GamepadBinding_ButtonY = 432;
const UTPID_GamepadBinding_Back = 433;
const UTPID_GamepadBinding_RightBumper = 434;
const UTPID_GamepadBinding_LeftBumper = 435;
const UTPID_GamepadBinding_RightTrigger = 436;
const UTPID_GamepadBinding_LeftTrigger = 437;
const UTPID_GamepadBinding_RightThumbstickPressed = 438;
const UTPID_GamepadBinding_LeftThumbstickPressed = 439;
const UTPID_GamepadBinding_DPadUp = 440;
const UTPID_GamepadBinding_DPadDown = 441;
const UTPID_GamepadBinding_DPadLeft = 442;
const UTPID_GamepadBinding_DPadRight = 443;

const UTPID_GamepadBinding_AnalogStickPreset = 444;

const UTPID_ControllerSensitivityMultiplier = 450;

// Weapons
const UTPID_WeaponHand = 460;
const UTPID_SmallWeapons = 461;
const UTPID_DisplayWeaponBar = 462;
const UTPID_ShowOnlyAvailableWeapons = 463;


// ===================================================================
// Single Player Keys
// ===================================================================

/**
 * PersistentKeys are meant to be tied to in-game events and persist across
 * different missions.  The mission manager can use these keys to determine next
 * course of action and kismet can query for a key and act accordingly (TBD).
 * Current we have room for 20 persistent keys.
 *
 * To Add a persistent key, extend the enum below.
 */

enum ESinglePlayerPersistentKeys
{
	ESPKey_None,
	ESPKey_MAX
};

const PSI_PersistentKeySlot0	= 200;
const PSI_PersistentKeySlot1	= 201;
const PSI_PersistentKeySlot2	= 202;
const PSI_PersistentKeySlot3	= 203;
const PSI_PersistentKeySlot4	= 204;
const PSI_PersistentKeySlot5	= 205;
const PSI_PersistentKeySlot6	= 206;
const PSI_PersistentKeySlot7	= 207;
const PSI_PersistentKeySlot8	= 208;
const PSI_PersistentKeySlot9	= 209;
const PSI_PersistentKeySlot10	= 210;
const PSI_PersistentKeySlot11	= 211;
const PSI_PersistentKeySlot12	= 212;
const PSI_PersistentKeySlot13	= 213;
const PSI_PersistentKeySlot14	= 214;
const PSI_PersistentKeySlot15	= 215;
const PSI_PersistentKeySlot16	= 216;
const PSI_PersistentKeySlot17	= 217;
const PSI_PersistentKeySlot18	= 218;
const PSI_PersistentKeySlot19	= 219;

const PSI_PersistentKeyMAX		= 220;	// When adding a persistent key slot, make sure you update the max
										// otherwise the new slot won't be processed

// ===================================================================
// Mission Keys
// ===================================================================

/**
 * Mission keys can be used to determine the status of a given mission.  To manage this we use 8
 * 32 bit ints (128 possible missions total) where 2 bits of each int represent the mission status.  They are:
 *
 * 		00 = Never Atempted
 *		01 = Successful
 *		10 = Failed
 *
 */

enum EMissionKeys
{
	EMKey_NeverAttempted,
	EMKey_Successful,
	EMKey_Failed,
	EMKey_Max
};


enum ESinglePlayerSkillLevels
{
	ESPSKILL_SkillLevel0,
	ESPSKILL_SkillLevel1,
	ESPSKILL_SkillLevel2,
	ESPSKILL_SkillLevel3,
	ESPSKILL_SkillLevelMAX
};

const PSI_SinglePlayerSkillLevel = 250;
const PSI_SinglePlayerCurrentMission=251;

var config int TempMissionIndex;

// ===================================================================
// Single Player Keys Accessors
// ===================================================================

/**
 * Check to see if a Persistent Key has been set.
 *
 * @Param	SearchKey		The Persistent Key to look for
 * @Param	PSI_Index		<Optional Out> returns the PSI Index for the slot holding the key
 *
 * @Returns true if the key exists, false if it doesn't
 */
function bool HasPersistentKey(ESinglePlayerPersistentKeys SearchKey, optional out int PSI_Index)
{
	local int Value;

	for (PSI_Index = PSI_PersistentKeySlot0; PSI_Index < PSI_PersistentKeyMAX; PSI_Index++)
	{
		GetProfileSettingValueInt( PSI_Index, Value );
		if ( Value == SearchKey )
		{
			return true;
		}
	}
	return false;
}

/**
 * Add a PersistentKey
 *
 * @Param	AddKey		The Persistent Key to add
 * @Returns true if successful.
 *
 */
function bool AddPersistentKey(ESinglePlayerPersistentKeys AddKey)
{
	local int PSI_Index, Value;


	// Make sure the key does not exist first

	if ( HasPersistentKey(AddKey) )
	{
		`log("Persistent Key"@AddKey@"already exists.");
		return false;
	}

	// Find the first available slot

	for (PSI_Index = PSI_PersistentKeySlot0; PSI_Index < PSI_PersistentKeyMAX; PSI_Index++)
	{
		GetProfileSettingValueInt( PSI_Index, Value );
		if ( Value == ESPKey_None )
		{
			SetProfileSettingValueInt( PSI_Index, AddKey );
			return true;
		}
	}

	return false;
}


/**
 * Remove a PersistentKey
 *
 * @Param	RemoveKey		The Persistent Key to remove
 * @returns true if successful
 */
function bool RemovePersistentKey(ESinglePlayerPersistentKeys RemoveKey)
{
	local iNT PSI_Index;

	if ( HasPersistentKey(RemoveKey, PSI_Index) )
	{
		SetProfileSettingValueInt( PSI_Index, ESPKey_None );
		return true;
	}

	return false;
}

/**
 * @Returns the current Mission Index
 */
function int GetCurrentMissionIndex()
{
	local int CMIdx;
	GetProfileSettingValueInt(PSI_SinglePlayerCurrentMission,CMIdx);
	return CMIdx;
}

function SetCurrentMissionIndex(int NewIndex)
{
	SetProfileSettingValueInt(PSI_SinglePlayerCurrentMission,NewIndex);
}

/**
 * @Returns true if a game is in progress
 */

function bool bGameInProgress()
{
	return GetCurrentMissionIndex() != 0;
}

/**
 * Resets the single player game profile settings for a new game
 */
function NewGame()
{
	local int i;

	for (i=PSI_PersistentKeySlot0;i< PSI_PersistentKeyMAX; i++)
	{
		SetProfileSettingValueInt(i,ESPKey_None);
	}

	SetProfileSettingValueInt(PSI_SinglePlayerSkillLevel,ESPSKILL_SkillLevel1);
	SetCurrentMissionIndex(0);
}
/**
 * Returns the integer value of a profile setting given its name.
 *
 * @return Whether or not the value was retrieved
 */
function bool GetProfileSettingValueIntByName(name SettingName, out int OutValue)
{
	local bool bResult;
	local int SettingId;

	bResult = FALSE;

	if(GetProfileSettingId(SettingName,SettingId))
	{
		bResult = GetProfileSettingValueInt(SettingId, OutValue);
	}

	return bResult;
}

/**
 * Returns the float value of a profile setting given its name.
 *
 * @return Whether or not the value was retrieved
 */
function bool GetProfileSettingValueFloatByName(name SettingName, out float OutValue)
{
	local bool bResult;
	local int SettingId;

	bResult = FALSE;

	if(GetProfileSettingId(SettingName,SettingId))
	{
		bResult = GetProfileSettingValueFloat(SettingId, OutValue);
	}

	return bResult;
}

/**
 * Returns the string value of a profile setting given its name.
 *
 * @return Whether or not the value was retrieved
 */
function bool GetProfileSettingValueStringByName(name SettingName, out string OutValue)
{
	local bool bResult;
	local int SettingId;

	bResult = FALSE;

	if(GetProfileSettingId(SettingName,SettingId))
	{
		bResult = GetProfileSettingValue(SettingId, OutValue);
	}

	return bResult;
}

/**
 * Returns the Id mapped value of a profile setting given its name.
 *
 * @return Whether or not the value was retrieved
 */
function bool GetProfileSettingValueIdByName(name SettingName, out int OutValue)
{
	local bool bResult;
	local int SettingId;

	bResult = FALSE;

	if(GetProfileSettingId(SettingName,SettingId))
	{
		bResult = GetProfileSettingValueId(SettingId, OutValue);
	}

	return bResult;
}


defaultproperties
{
	// If you change any profile ids, increment this number!!!!
	VersionNumber=15

	/////////////////////////////////////////////////////////////
	// ProfileSettingIds - Array of profile setting IDs to use as lookups
	/////////////////////////////////////////////////////////////
	ProfileSettingIds.Empty

	// Online Service - Gamer profile settings that should be read to meet TCR
	ProfileSettingIds.Add(PSI_ControllerVibration)
	ProfileSettingIds.Add(PSI_YInversion)
	ProfileSettingIds.Add(PSI_VoiceMuted)
	ProfileSettingIds.Add(UTPID_ControllerSensitivityMultiplier)
	ProfileSettingIds.Add(PSI_AutoAim)


	// UT Specific Ids
	ProfileSettingIds.Add(UTPID_CustomCharString)

	// Audio
	ProfileSettingIds.Add(UTPID_SFXVolume)
	ProfileSettingIds.Add(UTPID_MusicVolume)
	ProfileSettingIds.Add(UTPID_VoiceVolume)
	ProfileSettingIds.Add(UTPID_AnnouncerVolume)
	ProfileSettingIds.Add(UTPID_AnnounceSetting)
	ProfileSettingIds.Add(UTPID_AutoTaunt)
	ProfileSettingIds.Add(UTPID_MessageBeep)
	ProfileSettingIds.Add(UTPID_TextToSpeechEnable)
	ProfileSettingIds.Add(UTPID_TextToSpeechTeamMessagesOnly)

	// Video
	ProfileSettingIds.Add(UTPID_Gamma)
	ProfileSettingIds.Add(UTPID_PostProcessPreset)
	ProfileSettingIds.Add(UTPID_ScreenResolutionX)
	ProfileSettingIds.Add(UTPID_ScreenResolutionY)
	ProfileSettingIds.Add(UTPID_DefaultFOV)

	// Game
	ProfileSettingIds.Add(UTPID_ViewBob)
	ProfileSettingIds.Add(UTPID_GoreLevel)
	ProfileSettingIds.Add(UTPID_DodgingEnabled)
	ProfileSettingIds.Add(UTPID_WeaponSwitchOnPickup)
	ProfileSettingIds.Add(UTPID_NetworkConnection)
	ProfileSettingIds.Add(UTPID_DynamicNetspeed)
	ProfileSettingIds.Add(UTPID_SpeechRecognition)


	// Input
	ProfileSettingIds.Add(UTPID_MouseSmoothing)
	ProfileSettingIds.Add(UTPID_ReduceMouseLag)
	ProfileSettingIds.Add(UTPID_EnableJoystick)
	ProfileSettingIds.Add(UTPID_MouseSensitivityGame)
	ProfileSettingIds.Add(UTPID_MouseSensitivityMenus)
	ProfileSettingIds.Add(UTPID_MouseSmoothingStrength)
	ProfileSettingIds.Add(UTPID_MouseAccelTreshold)
	ProfileSettingIds.Add(UTPID_DodgeDoubleClickTime)
	ProfileSettingIds.Add(UTPID_TurningAccelerationFactor)
	ProfileSettingIds.Add(UTPID_GamepadBinding_ButtonA)
	ProfileSettingIds.Add(UTPID_GamepadBinding_ButtonB)
	ProfileSettingIds.Add(UTPID_GamepadBinding_ButtonX)
	ProfileSettingIds.Add(UTPID_GamepadBinding_ButtonY)
	ProfileSettingIds.Add(UTPID_GamepadBinding_Back)
	ProfileSettingIds.Add(UTPID_GamepadBinding_RightBumper)
	ProfileSettingIds.Add(UTPID_GamepadBinding_LeftBumper)
	ProfileSettingIds.Add(UTPID_GamepadBinding_RightTrigger)
	ProfileSettingIds.Add(UTPID_GamepadBinding_LeftTrigger)
	ProfileSettingIds.Add(UTPID_GamepadBinding_RightThumbstickPressed)
	ProfileSettingIds.Add(UTPID_GamepadBinding_LeftThumbstickPressed)
	ProfileSettingIds.Add(UTPID_GamepadBinding_DPadUp)
	ProfileSettingIds.Add(UTPID_GamepadBinding_DPadDown)
	ProfileSettingIds.Add(UTPID_GamepadBinding_DPadLeft)
	ProfileSettingIds.Add(UTPID_GamepadBinding_DPadRight)
	ProfileSettingIds.Add(UTPID_GamepadBinding_AnalogStickPreset)

	// Weapons
	ProfileSettingIds.Add(UTPID_WeaponHand)
	ProfileSettingIds.Add(UTPID_SmallWeapons)
	ProfileSettingIds.Add(UTPID_DisplayWeaponBar)
	ProfileSettingIds.Add(UTPID_ShowOnlyAvailableWeapons)

	// Single player Persistent Keys
	ProfileSettingIds.Add(PSI_PersistentKeySlot0);
	ProfileSettingIds.Add(PSI_PersistentKeySlot1);
	ProfileSettingIds.Add(PSI_PersistentKeySlot2);
	ProfileSettingIds.Add(PSI_PersistentKeySlot3);
	ProfileSettingIds.Add(PSI_PersistentKeySlot4);
	ProfileSettingIds.Add(PSI_PersistentKeySlot5);
	ProfileSettingIds.Add(PSI_PersistentKeySlot6);
	ProfileSettingIds.Add(PSI_PersistentKeySlot7);
	ProfileSettingIds.Add(PSI_PersistentKeySlot8);
	ProfileSettingIds.Add(PSI_PersistentKeySlot9);
	ProfileSettingIds.Add(PSI_PersistentKeySlot10);
	ProfileSettingIds.Add(PSI_PersistentKeySlot11);
	ProfileSettingIds.Add(PSI_PersistentKeySlot12);
	ProfileSettingIds.Add(PSI_PersistentKeySlot13);
	ProfileSettingIds.Add(PSI_PersistentKeySlot14);
	ProfileSettingIds.Add(PSI_PersistentKeySlot15);
	ProfileSettingIds.Add(PSI_PersistentKeySlot16);
	ProfileSettingIds.Add(PSI_PersistentKeySlot17);
	ProfileSettingIds.Add(PSI_PersistentKeySlot18);
	ProfileSettingIds.Add(PSI_PersistentKeySlot19);

	ProfileSettingIds.Add(PSI_SinglePlayerSkillLevel);
	ProfileSettingIds.Add(PSI_SinglePlayerCurrentMission);

	/////////////////////////////////////////////////////////////
	// ProfileMappings - Information on how the data is presented to the UI system
	/////////////////////////////////////////////////////////////
	ProfileMappings.Empty

	// Online Service Mappings
	ProfileMappings[0]=(Id=PSI_ControllerVibration,Name="ControllerVibration",MappingType=PVMT_IdMapped,ValueMappings=((Id=PCVTO_Off),(Id=PCVTO_On)))
	ProfileMappings[1]=(Id=PSI_YInversion,Name="InvertY",MappingType=PVMT_IdMapped,ValueMappings=((Id=PYIO_Off),(Id=PYIO_On)))
	ProfileMappings[2]=(Id=PSI_VoiceMuted,Name="MuteVoice",MappingType=PVMT_IdMapped,ValueMappings=((Id=0),(Id=1)))
	ProfileMappings[3]=(Id=UTPID_ControllerSensitivityMultiplier,Name="ControllerSensitivityMultiplier",MappingType=PVMT_RawValue)
	ProfileMappings[4]=(Id=PSI_AutoAim,Name="AutoAim",MappingType=PVMT_IdMapped,ValueMappings=((Id=PAAO_Off),(Id=PAAO_On)))

	// UT Specific Mappings
	ProfileMappings[5]=(Id=UTPID_CustomCharString,Name="CustomCharData",MappingType=PVMT_RawValue)

	// Audio
	ProfileMappings[6]=(Id=UTPID_SFXVolume,Name="SFXVolume",MappingType=PVMT_RawValue)
	ProfileMappings[7]=(Id=UTPID_MusicVolume,Name="MusicVolume",MappingType=PVMT_RawValue)
	ProfileMappings[8]=(Id=UTPID_VoiceVolume,Name="VoiceVolume",MappingType=PVMT_RawValue)
	ProfileMappings[9]=(Id=UTPID_AnnouncerVolume,Name="AnnouncerVolume",MappingType=PVMT_RawValue)
	ProfileMappings[10]=(Id=UTPID_AnnounceSetting,Name="AnnounceSetting",MappingType=PVMT_IdMapped,ValueMappings=((Id=ANNOUNCE_OFF),(Id=ANNOUNCE_MINIMAL),(Id=ANNOUNCE_FULL)))
	ProfileMappings[11]=(Id=UTPID_AutoTaunt,Name="AutoTaunt",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[12]=(Id=UTPID_MessageBeep,Name="MessageBeep",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[13]=(Id=UTPID_TextToSpeechEnable,Name="TextToSpeechEnable",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[14]=(Id=UTPID_TextToSpeechTeamMessagesOnly,Name="TextToSpeechTeamMessagesOnly",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))

	// Video
	ProfileMappings[15]=(Id=UTPID_Gamma,Name="Gamma",MappingType=PVMT_RawValue)
	ProfileMappings[16]=(Id=UTPID_PostProcessPreset,Name="PostProcessPreset",MappingType=PVMT_IdMapped,ValueMappings=((Id=PPP_Default),(Id=PPP_Soft),(Id=PPP_Lucent),(Id=PPP_Vibrant)))
	ProfileMappings[17]=(Id=UTPID_ScreenResolutionX,Name="ScreenResolutionX",MappingType=PVMT_RawValue)
	ProfileMappings[18]=(Id=UTPID_ScreenResolutionY,Name="ScreenResolutionY",MappingType=PVMT_RawValue)
	ProfileMappings[19]=(Id=UTPID_DefaultFOV,Name="DefaultFOV",MappingType=PVMT_RawValue)

	// Game
	ProfileMappings[20]=(Id=UTPID_ViewBob,Name="ViewBob",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[21]=(Id=UTPID_GoreLevel,Name="GoreLevel",MappingType=PVMT_IdMapped,ValueMappings=((Id=GORE_NONE),(Id=GORE_LOW),(Id=GORE_HIGH)))
	ProfileMappings[22]=(Id=UTPID_DodgingEnabled,Name="DodgingEnabled",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[23]=(Id=UTPID_WeaponSwitchOnPickup,Name="WeaponSwitchOnPickup",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[24]=(Id=UTPID_NetworkConnection,Name="NetworkConnection",MappingType=PVMT_IdMapped,ValueMappings=((Id=NETWORKTYPE_Unknown),(Id=NETWORKTYPE_Modem),(Id=NETWORKTYPE_ISDN),(Id=NETWORKTYPE_Cable),(Id=NETWORKTYPE_LAN)))
	ProfileMappings[25]=(Id=UTPID_DynamicNetspeed,Name="DynamicNetspeed",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[26]=(Id=UTPID_SpeechRecognition,Name="SpeechRecognition",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))

	// Input
	ProfileMappings[27]=(Id=UTPID_MouseSmoothing,Name="MouseSmoothing",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[28]=(Id=UTPID_ReduceMouseLag,Name="ReduceMouseLag",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[29]=(Id=UTPID_EnableJoystick,Name="EnableJoystick",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[30]=(Id=UTPID_MouseSensitivityGame,Name="MouseSensitivityGame",MappingType=PVMT_RawValue)
	ProfileMappings[31]=(Id=UTPID_MouseSensitivityMenus,Name="MouseSensitivityMenus",MappingType=PVMT_RawValue)
	ProfileMappings[32]=(Id=UTPID_MouseSmoothingStrength,Name="MouseSmoothingStrength",MappingType=PVMT_RawValue)
	ProfileMappings[33]=(Id=UTPID_MouseAccelTreshold,Name="MouseAccelTreshold",MappingType=PVMT_RawValue)
	ProfileMappings[34]=(Id=UTPID_DodgeDoubleClickTime,Name="DodgeDoubleClickTime",MappingType=PVMT_RawValue)
	ProfileMappings[35]=(Id=UTPID_TurningAccelerationFactor,Name="TurningAccelerationFactor",MappingType=PVMT_RawValue)

	ProfileMappings[36]=(Id=UTPID_MouseSmoothing)
	ProfileMappings[37]=(Id=UTPID_ReduceMouseLag)
	ProfileMappings[38]=(Id=UTPID_EnableJoystick)
	ProfileMappings[39]=(Id=UTPID_MouseSensitivityGame)
	ProfileMappings[40]=(Id=UTPID_MouseSensitivityMenus)
	ProfileMappings[41]=(Id=UTPID_MouseSmoothingStrength)
	ProfileMappings[42]=(Id=UTPID_MouseAccelTreshold)
	ProfileMappings[43]=(Id=UTPID_DodgeDoubleClickTime)
	ProfileMappings[44]=(Id=UTPID_TurningAccelerationFactor)

	ProfileMappings[45]=(Id=UTPID_GamepadBinding_ButtonA,Name="GamepadBinding_ButtonA",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[46]=(Id=UTPID_GamepadBinding_ButtonB,Name="GamepadBinding_ButtonB",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[47]=(Id=UTPID_GamepadBinding_ButtonX,Name="GamepadBinding_ButtonX",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[48]=(Id=UTPID_GamepadBinding_ButtonY,Name="GamepadBinding_ButtonY",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[49]=(Id=UTPID_GamepadBinding_Back,Name="GamepadBinding_Back",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[50]=(Id=UTPID_GamepadBinding_RightBumper,Name="GamepadBinding_RightBumper",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[51]=(Id=UTPID_GamepadBinding_LeftBumper,Name="GamepadBinding_LeftBumper",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[52]=(Id=UTPID_GamepadBinding_RightTrigger,Name="GamepadBinding_RightTrigger",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[53]=(Id=UTPID_GamepadBinding_LeftTrigger,Name="GamepadBinding_LeftTrigger",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[54]=(Id=UTPID_GamepadBinding_RightThumbstickPressed,Name="GamepadBinding_RightThumbstickPressed",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[55]=(Id=UTPID_GamepadBinding_LeftThumbstickPressed,Name="GamepadBinding_LeftThumbstickPressed",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[56]=(Id=UTPID_GamepadBinding_DPadUp,Name="GamepadBinding_DPadUp",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[57]=(Id=UTPID_GamepadBinding_DPadDown,Name="GamepadBinding_DPadDown",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[58]=(Id=UTPID_GamepadBinding_DPadLeft,Name="GamepadBinding_DPadLeft",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[59]=(Id=UTPID_GamepadBinding_DPadRight,Name="GamepadBinding_DPadRight",MappingType=PVMT_IdMapped,ValueMappings=((Id=DBA_Fire),(Id=DBA_AltFire),(Id=DBA_Jump),(Id=DBA_Use),(Id=DBA_ToggleMelee),(Id=DBA_ShowScores),(Id=DBA_ShowMap),(Id=DBA_FeignDeath),(Id=DBA_ToggleSpeaking),(Id=DBA_ToggleMinimap),(Id=DBA_WeaponPicker),(Id=DBA_NextWeapon),(Id=DBA_BestWeapon),(Id=DBA_SmartDodge)))
	ProfileMappings[60]=(Id=UTPID_GamepadBinding_AnalogStickPreset,Name="GamepadBinding_AnalogStickPreset",MappingType=PVMT_IdMapped,ValueMappings=((Id=ESA_Normal),(Id=ESA_SouthPaw),(Id=ESA_Legacy),(Id=ESA_LegacySouthPaw)))

	// Weapons
	ProfileMappings[61]=(Id=UTPID_WeaponHand,Name="WeaponHand",MappingType=PVMT_IdMapped,ValueMappings=((Id=HAND_Right),(Id=HAND_Left),(Id=HAND_Centered),(Id=HAND_Hidden)))
	ProfileMappings[62]=(Id=UTPID_SmallWeapons,Name="SmallWeapons",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[63]=(Id=UTPID_DisplayWeaponBar,Name="DisplayWeaponBar",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))
	ProfileMappings[64]=(Id=UTPID_ShowOnlyAvailableWeapons,Name="ShowOnlyAvailableWeapons",MappingType=PVMT_IdMapped,ValueMappings=((Id=UTPID_VALUE_NO),(Id=UTPID_VALUE_YES)))

	// Single Player
	ProfileMappings[65]=(Id=PSI_SinglePlayerSkillLevel, Name="Skill Level",MappingType=PVMT_IdMapped,ValueMappings=((Id=ESPSKILL_SkillLevel0),(Id=ESPSKILL_SkillLevel1),(Id=ESPSKILL_SkillLevel2),,(Id=ESPSKILL_SkillLevel3)))

	ProfileMappings.Add((Id=PSI_PersistentKeySlot0, Name="PKey0",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot1, Name="PKey1",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot2, Name="PKey2",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot3, Name="PKey3",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot4, Name="PKey4",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot5, Name="PKey5",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot6, Name="PKey6",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot7, Name="PKey7",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot8, Name="PKey8",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot9, Name="PKey9",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot10, Name="PKey10",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot11, Name="PKey11",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot12, Name="PKey12",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot13, Name="PKey13",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot14, Name="PKey14",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot15, Name="PKey15",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot16, Name="PKey16",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot17, Name="PKey17",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot18, Name="PKey18",MappingType=PVMT_RawValue))
	ProfileMappings.Add((Id=PSI_PersistentKeySlot19, Name="PKey19",MappingType=PVMT_RawValue))

	ProfileMappings.Add((Id=PSI_SinglePlayerCurrentMission, NAME="Current Mission",MappingType=PVMT_RawValue))

	/////////////////////////////////////////////////////////////
	// DefaultSettings - Defaults for the profile
	/////////////////////////////////////////////////////////////
	DefaultSettings.Empty

	// Online Service - Defaults for the values if not specified by the online service
	DefaultSettings.Add((Owner=OPPO_OnlineService,ProfileSetting=(PropertyId=PSI_ControllerVibration,Data=(Type=SDT_Int32,Value1=PCVTO_On))))
	DefaultSettings.Add((Owner=OPPO_OnlineService,ProfileSetting=(PropertyId=PSI_YInversion,Data=(Type=SDT_Int32,Value1=PYIO_Off))))
	DefaultSettings.Add((Owner=OPPO_OnlineService,ProfileSetting=(PropertyId=PSI_VoiceMuted,Data=(Type=SDT_Int32,Value1=PYIO_Off))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ControllerSensitivityMultiplier,Data=(Type=SDT_Int32,Value1=10))))
	DefaultSettings.Add((Owner=OPPO_OnlineService,ProfileSetting=(PropertyId=PSI_AutoAim,Data=(Type=SDT_Int32,Value1=PAAO_On))))

	// UT Specific Defaults
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_CustomCharString,Data=(Type=SDT_String))))

	// Audio
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_SFXVolume,Data=(Type=SDT_Int32,Value1=6))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MusicVolume,Data=(Type=SDT_Int32,Value1=6))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_VoiceVolume,Data=(Type=SDT_Int32,Value1=6))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_AnnouncerVolume,Data=(Type=SDT_Int32,Value1=6))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_AnnounceSetting,Data=(Type=SDT_Int32,Value1=ANNOUNCE_FULL))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_AutoTaunt,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MessageBeep,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_TextToSpeechEnable,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_TextToSpeechTeamMessagesOnly,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_NO))))

	// Video
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_Gamma,Data=(Type=SDT_Int32,Value1=22))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_PostProcessPreset,Data=(Type=SDT_Int32,Value1=PPP_Default))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ScreenResolutionX,Data=(Type=SDT_Int32,Value1=1024))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ScreenResolutionY,Data=(Type=SDT_Int32,Value1=768))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_DefaultFOV,Data=(Type=SDT_Int32,Value1=90))))

	// Game
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ViewBob,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GoreLevel,Data=(Type=SDT_Int32,Value1=GORE_HIGH))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_DodgingEnabled,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_WeaponSwitchOnPickup,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_NetworkConnection,Data=(Type=SDT_Int32,Value1=NETWORKTYPE_Unknown))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_DynamicNetspeed,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_SpeechRecognition,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_NO))))

	// Input
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MouseSmoothing,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ReduceMouseLag,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_EnableJoystick,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MouseSensitivityGame,Data=(Type=SDT_Int32,Value1=1))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MouseSensitivityMenus,Data=(Type=SDT_Int32,Value1=1))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MouseSmoothingStrength,Data=(Type=SDT_Int32,Value1=1))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_MouseAccelTreshold,Data=(Type=SDT_Int32,Value1=1))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_DodgeDoubleClickTime,Data=(Type=SDT_Int32,Value1=25))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_TurningAccelerationFactor,Data=(Type=SDT_Int32,Value1=1))))

	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_ButtonA,Data=(Type=SDT_Int32,Value1=DBA_Jump))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_ButtonB,Data=(Type=SDT_Int32,Value1=DBA_ToggleMelee))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_ButtonX,Data=(Type=SDT_Int32,Value1=DBA_Use))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_ButtonY,Data=(Type=SDT_Int32,Value1=DBA_ShowMap))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_Back,Data=(Type=SDT_Int32,Value1=DBA_ShowScores))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_RightBumper,Data=(Type=SDT_Int32,Value1=DBA_SmartDodge))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_LeftBumper,Data=(Type=SDT_Int32,Value1=DBA_WeaponPicker))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_RightTrigger,Data=(Type=SDT_Int32,Value1=DBA_Fire))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_LeftTrigger,Data=(Type=SDT_Int32,Value1=DBA_AltFire))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_RightThumbstickPressed,Data=(Type=SDT_Int32,Value1=DBA_BestWeapon))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_LeftThumbstickPressed,Data=(Type=SDT_Int32,Value1=DBA_Jump))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_DPadUp,Data=(Type=SDT_Int32,Value1=DBA_ToggleMinimap))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_DPadDown,Data=(Type=SDT_Int32,Value1=DBA_FeignDeath))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_DPadLeft,Data=(Type=SDT_Int32,Value1=DBA_ToggleSpeaking))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_DPadRight,Data=(Type=SDT_Int32,Value1=DBA_ToggleSpeaking))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_GamepadBinding_AnalogStickPreset,Data=(Type=SDT_Int32,Value1=ESA_Normal))))

	// Weapons
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_WeaponHand,Data=(Type=SDT_Int32,Value1=HAND_Right))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_SmallWeapons,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_NO))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_DisplayWeaponBar,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_YES))))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=UTPID_ShowOnlyAvailableWeapons,Data=(Type=SDT_Int32,Value1=UTPID_VALUE_yES))))

	// Single Player
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot0,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot1,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot2,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot3,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot4,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot5,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot6,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot7,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot8,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot9,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot10,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot11,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot12,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot13,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot14,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot15,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot16,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot17,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot18,Data=(Type=SDT_Int32,Value1=0)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_PersistentKeySlot19,Data=(Type=SDT_Int32,Value1=0)))

	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_SinglePlayerSkillLevel,Data=(Type=SDT_Int32,Value1=ESPSKILL_SkillLevel1)))
	DefaultSettings.Add((Owner=OPPO_Game,ProfileSetting=(PropertyId=PSI_SinglePlayerCurrentMission,Data=(Type=SDT_Int32,Value1=0)))
}
