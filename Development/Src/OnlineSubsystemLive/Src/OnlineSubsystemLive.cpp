/**
 * Copyright © 2006 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemLive.h"

IMPLEMENT_CLASS(UOnlineSubsystemLive);

#define DEBUG_PROFILE_DATA 0
#define DEBUG_SYSLINK 0

/**
 * This value indicates which packet version the server is sending. Clients with
 * differing versions will ignore these packets. This prevents crashing when
 * changing the packet format and there are existing servers on the network
 */
#define SYSTEM_LINK_PACKET_VERSION (BYTE)2

/** The first 11 bytes are the header for validation */
#define SYSTEM_LINK_PACKET_HEADER_SIZE 11

#if DEBUG_SYSLINK
/**
 * Used to build string information of the contexts being placed in a system link
 * packet. Builds the name and value in a human readable form
 *
 * @param Settings the settings object that has the metadata
 * @param Context the context object to build the string for
 */
FString BuildContextString(USettings* Settings,const FLocalizedStringSetting& Context)
{
	FName ContextName = Settings->GetStringSettingName(Context.Id);
	FName Value = Settings->GetStringSettingValueName(Context.Id,Context.ValueIndex);
	return FString::Printf(TEXT("Context '%s' = %s"),*ContextName,*Value);
}

/**
 * Used to build string information of the properties being placed in a system link
 * packet. Builds the name and value in a human readable form
 *
 * @param Settings the settings object that has the metadata
 */
FString BuildPropertyString(USettings* Settings,const FSettingsProperty& Property)
{
	FName PropName = Settings->GetPropertyName(Property.PropertyId);
	FString Value = Settings->GetPropertyAsString(Property.PropertyId);
	return FString::Printf(TEXT("Property '%s' = %s"),*PropName,*Value);
}
#endif

#if DEBUG_PROFILE_DATA
/**
 * Logs each profile entry so we can verify profile read/writes
 *
 * @param Profile the profile object to dump the settings for
 */
void DumpProfile(UOnlineProfileSettings* Profile)
{
	debugf(NAME_DevLive,TEXT("Profile object %s of type %s"),
		*Profile->GetName(),*Profile->GetClass()->GetName());
	for (INT Index = 0; Index < Profile->ProfileSettings.Num(); Index++)
	{
		FOnlineProfileSetting& Setting = Profile->ProfileSettings(Index);
		// Dump the value as a string
		FString Value = Setting.ProfileSetting.Data.ToString();
		debugf(NAME_DevLive,TEXT("ProfileSettings(%d) ProfileId(%d) = %s"),
			Index,Setting.ProfileSetting.PropertyId,*Value);
	}
}
#endif

#if _DEBUG
/**
 * Logs the set of contexts and properties for debugging purposes
 *
 * @param GameSearch the game to log the information for
 */
void DumpContextsAndProperties(UOnlineGameSearch* GameSearch)
{
	debugf(NAME_DevLive,TEXT("Search contexts:"));
	// Iterate through all contexts and log them
	for (INT Index = 0; Index < GameSearch->LocalizedSettings.Num(); Index++)
	{
		const FLocalizedStringSetting& Context = GameSearch->LocalizedSettings(Index);
		// Check for wildcard status
		if (GameSearch->IsWildcardStringSetting(Context.Id) == FALSE)
		{
			debugf(NAME_DevLive,TEXT("0x%08X = %d"),Context.Id,Context.ValueIndex);
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Using wildcard for 0x%08X"),Context.Id);
		}
	}
	debugf(NAME_DevLive,TEXT("Search properties:"));
	// Iterate through all properties and log them
	for (INT Index = 0; Index < GameSearch->Properties.Num(); Index++)
	{
		const FSettingsProperty& Property = GameSearch->Properties(Index);
		debugf(NAME_DevLive,TEXT("0x%08X (%d) = %s"),Property.PropertyId,
			Property.Data.Type,*Property.Data.ToString());
	}
}
#endif

/**
 * Given an SettingsData object determines the size we'll be sending
 *
 * @param SettingsData the object to inspect
 */
static inline DWORD GetSettingsDataSize(const FSettingsData& SettingsData)
{
	DWORD SizeOfData = sizeof(DWORD);
	// Figure out if we have a type that isn't the default size
	switch (SettingsData.Type)
	{
		case SDT_Int64:
		case SDT_Double:
		{
			SizeOfData = sizeof(DOUBLE);
			break;
		}
		case SDT_Blob:
		{
			// Read the setting that is set by Value1
			SizeOfData = (DWORD)SettingsData.Value1;
			break;
		}
		case SDT_String:
		{			// The null terminator needs to be counted too
			SizeOfData = (DWORD)SettingsData.Value1 + 1;
			break;
		}
	}
	return SizeOfData;
}

/**
 * Given an SettingsData object determines the data pointer to give to Live
 *
 * @param SettingsData the object to inspect
 */
static inline const void* GetSettingsDataPointer(const FSettingsData& SettingsData)
{
	const void* DataPointer = NULL;
	// Determine where to get the pointer from
	switch (SettingsData.Type)
	{
		case SDT_Float:
		case SDT_Int32:
		case SDT_Int64:
		case SDT_Double:
		{
			DataPointer = &SettingsData.Value1;
			break;
		}
		case SDT_Blob:
		case SDT_String:
		{
			DataPointer = SettingsData.Value2;
			break;
		}
	}
	return DataPointer;
}

/**
 * Copies the data from the Live structure to the Epic structure
 *
 * @param Data the Epic structure that is the destination
 * @param XData the Live strucuture that is the source
 */
static inline void CopyXDataToSettingsData(FSettingsData& Data,const XUSER_DATA& XData)
{
	// Copy based upon data type
	switch ((ESettingsDataType)XData.type)
	{
		case SDT_Float:
		{
			Data.SetData(XData.fData);
			break;
		}
		case SDT_Int32:
		{
			Data.SetData((INT)XData.nData);
			break;
		}
		case SDT_Int64:
		{
			Data.SetData((QWORD)XData.i64Data);
			break;
		}
		case SDT_Double:
		{
			Data.SetData(XData.dblData);
			break;
		}
		case SDT_Blob:
		{
			// Deep copy the data
			Data.SetData(XData.binary.cbData,XData.binary.pbData);
			break;
		}
		case SDT_String:
		{
			Data.SetData(XData.string.pwszData);
			break;
		}
		case SDT_DateTime:
		{
			Data.SetData(XData.ftData.dwLowDateTime,XData.ftData.dwHighDateTime);
			break;
		}
	}
}

/** Global mapping of Unreal enum values to Live values */
const DWORD LiveProfileSettingIDs[] =
{
	// Live read only settings
	XPROFILE_OPTION_CONTROLLER_VIBRATION,
	XPROFILE_GAMER_YAXIS_INVERSION,
	XPROFILE_GAMERCARD_CRED,
	XPROFILE_GAMERCARD_REP,
	XPROFILE_OPTION_VOICE_MUTED,
	XPROFILE_OPTION_VOICE_THRU_SPEAKERS,
	XPROFILE_OPTION_VOICE_VOLUME,
	XPROFILE_GAMERCARD_PICTURE_KEY,
	XPROFILE_GAMERCARD_TITLES_PLAYED,
	XPROFILE_GAMERCARD_MOTTO,
	XPROFILE_GAMERCARD_ACHIEVEMENTS_EARNED,
	XPROFILE_GAMER_DIFFICULTY,
	XPROFILE_GAMER_CONTROL_SENSITIVITY,
	XPROFILE_GAMER_PREFERRED_COLOR_FIRST,
	XPROFILE_GAMER_PREFERRED_COLOR_SECOND,
	XPROFILE_GAMER_ACTION_AUTO_AIM,
	XPROFILE_GAMER_ACTION_AUTO_CENTER,
	XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL,
	XPROFILE_GAMER_RACE_TRANSMISSION,
	XPROFILE_GAMER_RACE_CAMERA_LOCATION,
	XPROFILE_GAMER_RACE_BRAKE_CONTROL,
	XPROFILE_GAMER_RACE_ACCELERATOR_CONTROL,
	XPROFILE_GAMERCARD_TITLE_CRED_EARNED,
	XPROFILE_GAMERCARD_TITLE_ACHIEVEMENTS_EARNED
};

/**
 * Determines if the specified ID is outside the range of standard Live IDs
 *
 * @param Id the id in question
 *
 * @return TRUE of the ID is game specific, FALSE otherwise
 */
FORCEINLINE UBOOL IsProfileSettingIdGameOnly(DWORD Id)
{
	return Id >= PSI_ProfileVersionNum;
}

/**
 * Converts an EProfileSettingID enum to the Live equivalent
 *
 * @param EnumValue the value to convert to a Live value
 *
 * @return the Live specific value for the specified enum
 */
inline DWORD ConvertToLiveValue(EProfileSettingID EnumValue)
{
	check(ARRAY_COUNT(LiveProfileSettingIDs) == PSI_ProfileVersionNum - 1 &&
		"Live profile mapping array isn't in synch");
	check(EnumValue > PSI_Unknown && EnumValue < PSI_ProfileVersionNum);
	return LiveProfileSettingIDs[EnumValue - 1];
}

/**
 * Converts a Live profile id to an EProfileSettingID enum value
 *
 * @param LiveValue the Live specific value to convert
 *
 * @return the Unreal enum value representing that Live profile id
 */
EProfileSettingID ConvertFromLiveValue(DWORD LiveValue)
{
	BYTE EnumValue = PSI_Unknown;
	// Figure out which enum value to use
	switch (LiveValue)
	{
		case XPROFILE_OPTION_CONTROLLER_VIBRATION:
		{
			EnumValue = 1;
			break;
		}
		case XPROFILE_GAMER_YAXIS_INVERSION:
		{
			EnumValue = 2;
			break;
		}
		case XPROFILE_GAMERCARD_CRED:
		{
			EnumValue = 3;
			break;
		}
		case XPROFILE_GAMERCARD_REP:
		{
			EnumValue = 4;
			break;
		}
		case XPROFILE_OPTION_VOICE_MUTED:
		{
			EnumValue = 5;
			break;
		}
		case XPROFILE_OPTION_VOICE_THRU_SPEAKERS:
		{
			EnumValue = 6;
			break;
		}
		case XPROFILE_OPTION_VOICE_VOLUME:
		{
			EnumValue = 7;
			break;
		}
		case XPROFILE_GAMERCARD_PICTURE_KEY:
		{
			EnumValue = 8;
			break;
		}
		case XPROFILE_GAMERCARD_TITLES_PLAYED:
		{
			EnumValue = 9;
			break;
		}
		case XPROFILE_GAMERCARD_MOTTO:
		{
			EnumValue = 10;
			break;
		}
		case XPROFILE_GAMERCARD_ACHIEVEMENTS_EARNED:
		{
			EnumValue = 11;
			break;
		}
		case XPROFILE_GAMER_DIFFICULTY:
		{
			EnumValue = 12;
			break;
		}
		case XPROFILE_GAMER_CONTROL_SENSITIVITY:
		{
			EnumValue = 13;
			break;
		}
		case XPROFILE_GAMER_PREFERRED_COLOR_FIRST:
		{
			EnumValue = 14;
			break;
		}
		case XPROFILE_GAMER_PREFERRED_COLOR_SECOND:
		{
			EnumValue = 15;
			break;
		}
		case XPROFILE_GAMER_ACTION_AUTO_AIM:
		{
			EnumValue = 16;
			break;
		}
		case XPROFILE_GAMER_ACTION_AUTO_CENTER:
		{
			EnumValue = 17;
			break;
		}
		case XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL:
		{
			EnumValue = 18;
			break;
		}
		case XPROFILE_GAMER_RACE_TRANSMISSION:
		{
			EnumValue = 19;
			break;
		}
		case XPROFILE_GAMER_RACE_CAMERA_LOCATION:
		{
			EnumValue = 20;
			break;
		}
		case XPROFILE_GAMER_RACE_BRAKE_CONTROL:
		{
			EnumValue = 21;
			break;
		}
		case XPROFILE_GAMER_RACE_ACCELERATOR_CONTROL:
		{
			EnumValue = 22;
			break;
		}
		case XPROFILE_GAMERCARD_TITLE_CRED_EARNED:
		{
			EnumValue = 23;
			break;
		}
		case XPROFILE_GAMERCARD_TITLE_ACHIEVEMENTS_EARNED:
		{
			EnumValue = 24;
			break;
		}
	};
	return (EProfileSettingID)EnumValue;
}

/**
 * Converts the Unreal enum values into an array of Live values
 *
 * @param ProfileIds the Unreal values to convert
 * @param DestIds an out array that gets the converted data
 */
static inline void BuildLiveProfileReadIDs(const TArray<DWORD>& ProfileIds,
	DWORD* DestIds)
{
	// Loop through using the helper to convert
	for (INT Index = 0; Index < ProfileIds.Num(); Index++)
	{
		DestIds[Index] = ConvertToLiveValue((EProfileSettingID)ProfileIds(Index));
	}
}

#if !FINAL_RELEASE
/**
 * Validates that the specified write stats object has the proper number
 * of views and stats per view
 *
 * @param WriteStats the object to validate
 *
 * @return TRUE if acceptable, FALSE otherwise
 */
UBOOL IsValidStatsWrite(UOnlineStatsWrite* WriteStats)
{
	// Validate the number of views
	if (WriteStats->ViewIds.Num() > 0 && WriteStats->ViewIds.Num() <= 5)
	{
		return WriteStats->Properties.Num() >= 0 &&
			WriteStats->Properties.Num() < 64;
	}
	return FALSE;
}
#endif

/**
 * Finds the player controller associated with the specified index
 *
 * @param Index the id of the user to find
 *
 * @return the player controller for that id
 */
inline APlayerController* GetPlayerControllerFromUserIndex(INT Index)
{
	// Find the local player that has the same controller id as the index
	for (FPlayerIterator It(GEngine); It; ++It)
	{
		ULocalPlayer* Player = *It;
		if (Player->ControllerId == Index)
		{
			// The actor is the corresponding player controller
			return Player->Actor;
		}
	}
	return NULL;
}

/**
 * Initializes the 2 sockets
 *
 * @param Port the port to listen on
 *
 * @return TRUE if both sockets were create successfully, FALSE otherwise
 */
UBOOL FSystemLinkBeacon::Init(INT Port)
{
	UBOOL bSuccess = FALSE;
	// Set our broadcast address
	BroadcastAddr.SetIp(INADDR_BROADCAST);
	BroadcastAddr.SetPort(Port);
	// Now the listen address
	ListenAddr.SetPort(Port);
	ListenAddr.SetIp(getlocalbindaddr(*GWarn));
	// Now create and set up our sockets (no VDP)
	ListenSocket = GSocketSubsystem->CreateDGramSocket(TRUE);
	if (ListenSocket != NULL)
	{
		ListenSocket->SetReuseAddr();
		ListenSocket->SetNonBlocking();
		ListenSocket->SetRecvErr();
		// Bind to our listen port
		if (ListenSocket->Bind(ListenAddr))
		{
			// Set it to broadcast mode, so we can send on it
			// NOTE: You must set this to broadcast mode or the secure layer will
			// eat any packets sent
			bSuccess = ListenSocket->SetBroadcast();
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to bind listen socket to addr (%s) for system link"),
				*ListenAddr.ToString(TRUE));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Failed to create listen socket for system link"));
	}
	return bSuccess && ListenSocket;
}

/**
 * Called to poll the socket for pending data. Any data received is placed
 * in the specified packet buffer
 *
 * @param PacketData the buffer to get the socket's packet data
 * @param BufferSize the size of the packet buffer
 *
 * @return the number of bytes read (0 if none or an error)
 */
INT FSystemLinkBeacon::Poll(BYTE* PacketData,INT BufferSize)
{
	check(PacketData && BufferSize);
	// Default to no data being read
	INT BytesRead = 0;
	if (ListenSocket != NULL)
	{
		FInternetIpAddr SockAddr;
		// Read from the socket
		ListenSocket->RecvFrom(PacketData,BufferSize,BytesRead,SockAddr);
	}
	return BytesRead;
}

/**
 * Routes the call to the function on the subsystem for parsing search results
 *
 * @param LiveSubsystem the object to make the final call on
 */
UBOOL FLiveAsyncTaskSearch::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	UBOOL bDelete = FALSE;
	DWORD Result = GetCompletionCode();
	if (Result == ERROR_SUCCESS)
	{
		// See if we are just waiting for live to send back matchmaking
		if (bIsWaitingForLive == TRUE)
		{
			bIsWaitingForLive = FALSE;
			// Parse the Live results
			LiveSubsystem->ParseSearchResults(LiveSubsystem->GameSearch,
				*(FLiveAsyncTaskDataSearch*)TaskData);
			// Kick off QoS searches for the servers that were returned
			if (LiveSubsystem->CheckServersQoS((FLiveAsyncTaskDataSearch*)TaskData) == FALSE)
			{
				// QoS failed so don't wait
				bDelete = TRUE;
			}
		}
		// We are waiting on QoS results
		else
		{
			FLiveAsyncTaskDataSearch* AsyncData = (FLiveAsyncTaskDataSearch*)TaskData;
			// Make sure we have data and then check it for completion
			XNQOS* QosData = *AsyncData->GetXNQOS();
			if (QosData != NULL)
			{
				// Check if all results are back
				if (QosData->cxnqosPending == 0)
				{
					// Have the subsystem update its search results data
					LiveSubsystem->ParseQoSResults(QosData);
					bDelete = TRUE;
				}
			}
			else
			{
				debugf(NAME_DevLive,TEXT("NULL XNQOS pointer, aborting QoS code"));
				// Something is messed up
				bDelete = TRUE;
			}
		}
	}
	else
	{
		// Stuff is broked
		debugf(NAME_DevLive,TEXT("XSessionSearch() completed with error 0x%08X"),Result);
		bDelete = TRUE;
	}
	// Mark the search as complete
	if (bDelete == TRUE && LiveSubsystem->GameSearch)
	{
		LiveSubsystem->GameSearch->bIsSearchInProgress = FALSE;
	}
	return bDelete;
}

/**
 * Coalesces the game settings data into one buffer instead of 3
 */
void FLiveAsyncTaskDataReadProfileSettings::CoalesceGameSettings(void)
{
	PXUSER_READ_PROFILE_SETTING_RESULT ReadResults = GetGameSettingsBuffer();
	// Copy each binary buffer into the working buffer
	for (DWORD Index = 0; Index < ReadResults->dwSettingsLen; Index++)
	{
		XUSER_PROFILE_SETTING& LiveSetting = ReadResults->pSettings[Index];
		// Don't bother copying data for no value settings and the data
		// should only be binary. Ignore otherwise
		if (LiveSetting.source != XSOURCE_NO_VALUE &&
			LiveSetting.data.type == XUSER_DATA_TYPE_BINARY)
		{
			// Figure out how much data to copy
			appMemcpy(&WorkingBuffer[WorkingBufferUsed],
				LiveSetting.data.binary.pbData,
				LiveSetting.data.binary.cbData);
			// Increment our offset for the next copy
			WorkingBufferUsed += LiveSetting.data.binary.cbData;
		}
	}
}

/**
 * Reads the online profile settings from the buffer into the specified array
 *
 * @param Settings the array to populate from the game settings
 */
void FLiveAsyncTaskReadProfileSettings::SerializeGameSettings(TArray<FOnlineProfileSetting>& Settings)
{
	FLiveAsyncTaskDataReadProfileSettings* Data =(FLiveAsyncTaskDataReadProfileSettings*)TaskData;
	// Don't bother if the buffer wasn't there
	if (Data->GetWorkingBufferSize() > 0)
	{
		// Used to serialize the data out of the buffer
		FSystemLinkPacketReader Reader(Data->GetWorkingBuffer(),Data->GetWorkingBufferSize());
		INT NumProfileSettings;
		Reader >> NumProfileSettings;
		// Presize the array
		Settings.Empty(NumProfileSettings);
		Settings.AddZeroed(NumProfileSettings);
		// Read all the settings we stored in the buffer
		for (INT Index = 0; Index < NumProfileSettings; Index++)
		{
			Reader >> Settings(Index);
		}
	}
}

/**
 * Routes the call to the function on the subsystem for parsing the results
 *
 * @param LiveSubsystem the object to make the final call on
 */
UBOOL FLiveAsyncTaskReadProfileSettings::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	UBOOL bDone = FALSE;
	FLiveAsyncTaskDataReadProfileSettings* Data = (FLiveAsyncTaskDataReadProfileSettings*)TaskData;
	DWORD Result = GetCompletionCode();
	if (Result == ERROR_SUCCESS)
	{
		// Figure out what our next step is
		if (Data->GetCurrentAction() == FLiveAsyncTaskDataReadProfileSettings::ReadingGameSettings)
		{
			// Build one big buffer out of the returned data
			Data->CoalesceGameSettings();
			TArray<FOnlineProfileSetting> Settings;
			// Serialize from the buffer
			SerializeGameSettings(Settings);
			TArray<DWORD> MissingSettingIds;
			// Get the Ids that the game is interested in
			DWORD* Ids = Data->GetIds();
			DWORD NumIds = Data->GetIdsCount();
			// For each ID that we need to read
			for (DWORD IdIndex = 0; IdIndex < NumIds; IdIndex++)
			{
				DWORD SettingId = Ids[IdIndex];
				UBOOL bFound = FALSE;
				// Search the resulting array for the data
				for (INT Index = 0; Index < Settings.Num(); Index++)
				{
					const FOnlineProfileSetting& Setting = Settings(Index);
					// If found, copy the data from the array to the profile data
					if (Setting.ProfileSetting.PropertyId == SettingId)
					{
						// Place the data in the user's profile results array
						LiveSubsystem->AppendProfileSetting(Data->GetUserIndex(),Setting);
						bFound = TRUE;
						break;
					}
				}
				// The requested ID wasn't in the game settings list, so add the
				// ID to the list we need to read from Live
				if (bFound == FALSE)
				{
					MissingSettingIds.AddItem(SettingId);
				}
			}
			// If there are IDs we need to read from Live and/or the game defaults
			if (MissingSettingIds.Num() > 0)
			{
				// Fill any game specific settings that aren't Live aware from the defaults
				LiveSubsystem->ProcessProfileDefaults(Data->GetUserIndex(),MissingSettingIds);
				// The game defaults may have fulfilled the remaining ids
				if (MissingSettingIds.Num() > 0)
				{
					check(MissingSettingIds.Num() <= (INT)Data->GetIdsCount());
					// Map the unreal IDs to Live ids
					BuildLiveProfileReadIDs(MissingSettingIds,Data->GetIds());
					// Allocate a buffer for the ones we need to read from Live
					Data->AllocateBuffer(MissingSettingIds.Num());
					appMemzero(&Overlapped,sizeof(XOVERLAPPED));
					// Need to indicate the buffer size
					DWORD SizeNeeded = Data->GetIdsSize();
					// Kick off a new read with just the missing ids
					DWORD Return = XUserReadProfileSettings(0,
						Data->GetUserIndex(),
						MissingSettingIds.Num(),
						Data->GetIds(),
						&SizeNeeded,
						Data->GetProfileBuffer(),
						&Overlapped);
					if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
					{
						bDone = FALSE;
						// Mark this task as still processing
						Data->SetCurrentAction(FLiveAsyncTaskDataReadProfileSettings::ReadingLiveSettings);
					}
					else
					{
						debugf(NAME_Error,TEXT("Failed to read Live IDs 0x%08X"),Return);
						bDone = TRUE;
					}
				}
				else
				{
					// All requested profile settings were met
					bDone = TRUE;
				}
			}
			else
			{
				// All requested profile settings were met
				bDone = TRUE;
			}
		}
		else
		{
			// Append anything that comes back
			LiveSubsystem->ParseReadProfileResults(Data->GetUserIndex(),
				Data->GetProfileBuffer());
			bDone = TRUE;
		}
	}
	else
	{
		bDone = TRUE;
		debugf(NAME_DevLive,TEXT("Profile read failed with 0x%08X"),Result);
		// Set the profile to the defaults in this case
		if (LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile != NULL)
		{
			LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->SetToDefaults();
		}
	}
	if (bDone)
	{
		// In case this gets cleared while in progress
		if (LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile != NULL)
		{
			INT ReadVersion = LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->GetVersionNumber();
			// Check the version number and reset to defaults if they don't match
			if (LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->VersionNumber != ReadVersion)
			{
				debugf(NAME_DevLive,
					TEXT("Detected profile version mismatch (%d != %d), setting to defaults"),
					LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->VersionNumber,
					ReadVersion);
				LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->SetToDefaults();
			}
			// Done with the reading, so mark the async state as done
			LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->AsyncState = OPAS_None;
		}
	}
#if DEBUG_PROFILE_DATA
	if (LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile != NULL)
	{
		DumpProfile(LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile);
	}
#endif
	return bDone;
}

/**
 * Clears the active profile write reference since it is no longer needed
 *
 * @param LiveSubsystem the object to make the final call on
 */
UBOOL FLiveAsyncTaskWriteProfileSettings::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	FLiveAsyncTaskDataWriteProfileSettings* Data = (FLiveAsyncTaskDataWriteProfileSettings*)TaskData;
	// In case this gets cleared while in progress
	if (LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile != NULL)
	{
		// Done with the writing, so mark the async state
		LiveSubsystem->ProfileCache[Data->GetUserIndex()].Profile->AsyncState = OPAS_None;
	}
	return TRUE;
}

/**
 * Changes the state of the game session to the one specified at construction
 *
 * @param LiveSubsystem the object to make the final call on
 */
UBOOL FLiveAsyncTaskSessionStateChange::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	LiveSubsystem->CurrentGameState = StateToTransitionTo;
	return TRUE;
}

/**
 * Checks the arbitration flag and issues a session size change if arbitrated
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskStartSession::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	if (bUsesArbitration)
	{
		// Kick off another task to shrink the session size
		LiveSubsystem->ShrinkToArbitratedRegistrantSize();
	}
	return FLiveAsyncTaskSessionStateChange::ProcessAsyncResults(LiveSubsystem);
}

/**
 * Changes the state of the game session to pending and registers all of the
 * local players
 *
 * @param LiveSubsystem the object to make the final call on
 */
UBOOL FLiveAsyncTaskCreateSession::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	// Figure out if we are at the end of the list or not
	DWORD Result = XGetOverlappedExtendedError(&Overlapped);
	// If the task completed ok, then continue the create process
	if (Result == ERROR_SUCCESS)
	{
		// We only register QoS when creating a server
		if (bIsCreate)
		{
			// Register the host with QoS
			LiveSubsystem->RegisterQoS();
		}
		// Set the game state as pending (not started)
		LiveSubsystem->CurrentGameState = OGS_Pending;
		// Register all local folks as participants/talkers
		LiveSubsystem->RegisterLocalPlayers(bIsFromInvite);
	}
	else
	{
		// Clean up partial create
		delete LiveSubsystem->SessionInfo;
		LiveSubsystem->SessionInfo = NULL;
		LiveSubsystem->GameSettings = NULL;
		LiveSubsystem->CurrentGameState = OGS_NoSession;
	}
	return TRUE;
}

/**
 * Routes the call to the function on the subsystem for parsing friends
 * results. Also, continues searching as needed until there are no more
 * friends to read
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskReadFriends::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
//@todo joeg -- Support counting and etc (need the info back from XDS)
	FLiveAsyncTaskDataEnumeration* FriendsData = (FLiveAsyncTaskDataEnumeration*)TaskData;
	// Figure out if we are at the end of the list or not
	DWORD Result = XGetOverlappedExtendedError(&Overlapped);
	// If the task completed ok, then parse the results and start another async task
	if (Result == ERROR_SUCCESS)
	{
		// Add the data to the friends cache
		LiveSubsystem->ParseFriendsResults(FriendsData->GetPlayerIndex(),
			(PXONLINE_FRIEND)FriendsData->GetBuffer());
		// Zero between uses or results will be incorrect
		appMemzero(&Overlapped,sizeof(XOVERLAPPED));
		// Do another async friends read
		Result = XEnumerate(FriendsData->GetHandle(),FriendsData->GetBuffer(),
			sizeof(XONLINE_FRIEND),0,&Overlapped);
	}
	else
	{
		// Done reading, need to mark the cache as finished
		LiveSubsystem->FriendsCache[FriendsData->GetPlayerIndex()].ReadState = OERS_Done;
	}
	// When this is true, there is no more data left to read
	return Result != ERROR_SUCCESS && Result != ERROR_IO_PENDING;
}

/**
 * Routes the call to the function on the subsystem for parsing content
 * results. Also, continues searching as needed until there are no more
 * content to read
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskReadContent::ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem)
{
	FLiveAsyncTaskContent* ContentTaskData = (FLiveAsyncTaskContent*)TaskData;
	
	// what is our current state?
	DWORD Result = GetCompletionCode();

	// Zero between uses or results will be incorrect
	appMemzero(&Overlapped,sizeof(XOVERLAPPED));

	// look to see how our enumeration is progressing
	if (TaskMode == CTM_Enumerate)
	{
		// If the task completed ok, then parse the results and start another async task to open the content
		if (Result == ERROR_SUCCESS)
		{
// @todo josh: support iterating over more than one item, right now FLiveAsyncTaskDataEnumeration only supports 1 read
			// process a piece of content found by the enumerator
			XCONTENT_DATA* Content = (XCONTENT_DATA*)ContentTaskData->GetBuffer();

			// create a virtual drive that the content will be mapped to (number is unimportant, just needs to be unique)
			static ContentNum = 0;
			FString ContentDrive = FString::Printf(TEXT("DLC%d"), ContentNum++);

			// open up the package (make sure it's openable)
			DWORD Return = XContentCreate(ContentTaskData->GetPlayerIndex(), TCHAR_TO_ANSI(*ContentDrive), 
				Content, XCONTENTFLAG_OPENEXISTING, NULL, NULL, &Overlapped);

			if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
			{
				// remember the data for the async task
				ContentTaskData->SetContentDrive(ContentDrive);
				ContentTaskData->SetFriendlyName(Content->szDisplayName);

				// switch modes
				TaskMode = CTM_Create;
			}
			// if we failed to open the content, then kick off another enumeration
			else
			{
				// Do another async content read
				Result = XEnumerate(ContentTaskData->GetHandle(), ContentTaskData->GetBuffer(), sizeof(XCONTENT_DATA), 0, &Overlapped);
			}
		}
		else
		{
			// Done reading, need to mark the cache as finished
			LiveSubsystem->ContentCache[ContentTaskData->GetPlayerIndex()].ReadState = OERS_Done;
		}
	}
	// 
	else
	{
		// If the task completed ok, then parse the results and start another enumeration
		if (Result == ERROR_SUCCESS)
		{
			// get the cache to fill out
			FContentListCache& Cache = LiveSubsystem->ContentCache[ContentTaskData->GetPlayerIndex()];
			// add a new empty content structure
			INT Index = Cache.Content.AddZeroed(1);

			// find the newly added content
			FOnlineContent& NewContent = Cache.Content(Index);
			// friendly name is the displayable for the content (not a map name)
			NewContent.FriendlyName = ContentTaskData->GetFriendlyName();
			// remember the virtual drive for the content (so we can close it later)
		// @todo josh: Close the content when the content goes away (user logs out)
			NewContent.ContentPath = ContentTaskData->GetContentDrive();

			// find all the packages in the content
			appFindFilesInDirectory(NewContent.ContentPackages, *(ContentTaskData->GetContentDrive() + TEXT(":\\")), TRUE, FALSE);

			// find all the non-packages in the content
			appFindFilesInDirectory(NewContent.ContentFiles, *(ContentTaskData->GetContentDrive() + TEXT(":\\")), FALSE, TRUE);

			debugf(TEXT("Using Downloaded Content Package '%s'"), *NewContent.FriendlyName);

			// let the system process the content
			LiveSubsystem->ProcessDownloadedContent(NewContent);

			// Do another async content read
			DWORD Return = XEnumerate(ContentTaskData->GetHandle(), ContentTaskData->GetBuffer(), sizeof(XCONTENT_DATA), 0, &Overlapped);

			if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
			{
				// switch modes
				TaskMode = CTM_Enumerate;
			}
			// if this failed, we need to stop
			else
			{
				// Done reading, need to mark the cache as finished
				LiveSubsystem->ContentCache[ContentTaskData->GetPlayerIndex()].ReadState = OERS_Done;
			}
		}
	}

	// When this is true, there is no more data left to read
	return LiveSubsystem->ContentCache[ContentTaskData->GetPlayerIndex()].ReadState == OERS_Done;
}

/**
 * Copies the download query results into the per user storage on the subsystem
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskQueryDownloads::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	FLiveAsyncTaskDataQueryDownloads* Data = (FLiveAsyncTaskDataQueryDownloads*)TaskData;
	// Copy to our cached data for this user
	LiveSubsystem->ContentCache[Data->GetUserIndex()].NewDownloadCount = Data->GetQuery()->dwNewOffers;
	LiveSubsystem->ContentCache[Data->GetUserIndex()].TotalDownloadCount = Data->GetQuery()->dwTotalOffers;
	return TRUE;
}

/**
 * Copies the resulting string into subsytem buffer. Optionally, will start
 * another async task to validate the string if requested
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskKeyboard::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	FLiveAsyncTaskDataKeyboard* KeyboardData = (FLiveAsyncTaskDataKeyboard*)TaskData;
	UBOOL bShouldCleanup = TRUE;
	// Determine if we are validating or just processing the keyboard results
	if (bIsValidating == FALSE)
	{
		// Keyboard results are back
		if (KeyboardData->NeedsStringValidation())
		{
			appMemzero(&Overlapped,sizeof(XOVERLAPPED));
			// Set up the string input data
			TCHAR* String = *KeyboardData;
			STRING_DATA* StringData = *KeyboardData;
			StringData->wStringSize = appStrlen(String);
			StringData->pszString = String;
			// Kick off the validation as an async task
			DWORD Return = XStringVerify(0,
//@todo joeg -- figure out what the different strings are based upon language
				"en-us",
				1,
				StringData,
				sizeof(STRING_VERIFY_RESPONSE) + sizeof(HRESULT),
				*KeyboardData,
				&Overlapped);
			if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
			{
				bShouldCleanup = FALSE;
				bIsValidating = TRUE;
			}
			else
			{
				debugf(NAME_Error,TEXT("Failed to validate string (%s) with 0x%08X"),
					String,Return);
			}
		}
		else
		{
			// Copy the data into our subsystem buffer
			LiveSubsystem->KeyboardInputResults = *KeyboardData;
		}
	}
	else
	{
		// Validation is complete, so copy the string if ok otherwise zero it
		STRING_VERIFY_RESPONSE* Response = *KeyboardData;
		if (Response->pStringResult[0] == S_OK)
		{
			// String is ok, so copy
			LiveSubsystem->KeyboardInputResults = *KeyboardData;
		}
		else
		{
			// String was a bad word so empty
			LiveSubsystem->KeyboardInputResults = TEXT("");
		}
	}
	return bShouldCleanup;
}

/**
 * Tells the Live subsystem to parse the results of the stats read
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskReadStats::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	UBOOL bShouldDelete = TRUE;
	DWORD Result = GetCompletionCode();
	debugf(NAME_DevLive,TEXT("XUserReadStats() returned 0x%08X"),Result);
	if (Result == ERROR_SUCCESS)
	{
		LiveSubsystem->ParseStatsReadResults(GetReadResults());
		// Update the player buffers and see if there are more to read
		UpdatePlayersToRead();
		if (NumToRead > 0)
		{
			appMemzero(&Overlapped,sizeof(XOVERLAPPED));
			bShouldDelete = FALSE;
			// Kick off another async read
			DWORD Return = XUserReadStats(0,
				NumToRead,
				GetPlayers(),
				1,
				GetSpecs(),
				&BufferSize,
				GetReadResults(),
				&Overlapped);
			debugf(NAME_DevLive,
				TEXT("Paged XUserReadStats(0,%d,Players,1,Specs,%d,Buffer,Overlapped) returned 0x%08X"),
				NumToRead,BufferSize,Return);
			if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
			{
				bShouldDelete = TRUE;
			}
		}
	}
	// If we are done processing, zero the read state
	if (bShouldDelete)
	{
		LiveSubsystem->CurrentStatsRead = NULL;
	}
	return bShouldDelete;
}

/**
 * Tells the Live subsystem to parse the results of the stats read
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskReadStatsByRank::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	DWORD Result = GetCompletionCode();
	if (Result == ERROR_SUCCESS)
	{
		LiveSubsystem->ParseStatsReadResults(GetReadResults());
	}
	LiveSubsystem->CurrentStatsRead = NULL;
	debugf(NAME_DevLive,TEXT("XEnumerate() returned 0x%08X"),Result);
	return TRUE;
}

/**
 * Tells the Live subsystem to continue the async game invite join
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskJoinGameInvite::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	DWORD Result = GetCompletionCode();
	if (Result == ERROR_SUCCESS)
	{
		// Need to create a search object to append the results to
		UOnlineGameSearch* Search = ConstructObject<UOnlineGameSearch>(UOnlineGameSearch::StaticClass());
		// Store the search so it can be used on accept
		LiveSubsystem->InviteCache[UserNum].InviteSearch = Search;
		// We need to parse the search results
		LiveSubsystem->ParseSearchResults(Search,GetResults());
		// Get the game object so we can mark it as being from an invite
		UOnlineGameSettings* InviteSettings = Search->Results.Num() > 0 ? Search->Results(0).GameSettings : NULL;
		if (InviteSettings != NULL)
		{
			InviteSettings->bWasFromInvite = TRUE;
		}
		// Fire off the delegate with the results (possibly NULL if not found)
		OnlineSubsystemLive_eventOnGameInviteAccepted_Parms Parms(EC_EventParm);
		Parms.InviteSettings = InviteSettings;
		LiveSubsystem->ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
	}
	// Don't fire automatically since we manually fired it off
	ScriptDelegate = NULL;
	return TRUE;
}

/**
 * Parses the arbitration results and stores them in the arbitration list
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncTaskArbitrationRegistration::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	// Forward the call to the subsystem
	LiveSubsystem->ParseArbitrationResults(GetResults());
	return TRUE;
}

/**
 * Checks to see if the match is arbitrated and shrinks it by one if it is
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncPlayer::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	// Shrink the session size by one if using arbitration
	if (LiveSubsystem->GameSettings &&
		LiveSubsystem->GameSettings->bUsesArbitration &&
		LiveSubsystem->CurrentGameState >= OGS_InProgress)
	{
		LiveSubsystem->ModifySessionSize(LiveSubsystem->GameSettings->NumPublicConnections - 1,0);
	}
	return TRUE;
}

/**
 * Checks to see if the join worked. If this was an invite it may need to
 * try private and then public.
 *
 * @param LiveSubsystem the object to make the final call on
 *
 * @return TRUE if this task should be cleaned up, FALSE to keep it
 */
UBOOL FLiveAsyncRegisterPlayer::ProcessAsyncResults(UOnlineSubsystemLive* LiveSubsystem)
{
	// Figure out if we are at the end of the list or not
	DWORD Result = XGetOverlappedExtendedError(&Overlapped);
	// If the task failed and we tried private first, then try public
	if (Result != ERROR_SUCCESS && bPrivateInvite == TRUE && bIsSecondTry == FALSE)
	{
		debugf(NAME_DevLive,TEXT("Private invite failed with 0x%08X. Trying public"),Result);
		bIsSecondTry = TRUE;
		bPrivateInvite = FALSE;
		appMemzero(&Overlapped,sizeof(XOVERLAPPED));
		// Kick off the async join request
		Result = XSessionJoinRemote(LiveSubsystem->SessionInfo->Handle,
			1,
			GetXuids(),
			GetPrivateInvites(),
			&Overlapped);
		debugf(NAME_DevLive,TEXT("XSessionJoinRemote(0x%016I64X) returned 0x%08X"),
			*GetXuids(),Result);
		return FALSE;
	}
	return TRUE;
}

/**
 * Live specific initialization. Sets all the interfaces to point to this
 * object as it implements them all
 *
 * @return always returns TRUE
 */
UBOOL UOnlineSubsystemLive::Init(void)
{
	// Set the player interface to be the same as the object
	eventSetPlayerInterface(this);
	// Set the Live specific player interface to be the same as the object
	eventSetPlayerInterfaceEx(this);
	// Set the system interface to be the same as the object
	eventSetSystemInterface(this);
	// Set the game interface to be the same as the object
	eventSetGameInterface(this);
	// Set the content interface to be the same as the object
	eventSetContentInterface(this);
	// Set the stats reading/writing interface
	eventSetStatsInterface(this);
	// Create the voice engine and if successful register the interface
	VoiceEngine = appCreateVoiceInterface(MaxLocalTalkers,MaxRemoteTalkers,
		bIsUsingSpeechRecognition);
	if (VoiceEngine != NULL)
	{
		// Set the voice interface to this object
		eventSetVoiceInterface(this);
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Failed to create the voice interface"));
	}
	XINPUT_CAPABILITIES InputCaps;
	appMemzero(&InputCaps,sizeof(XINPUT_CAPABILITIES));
	// any users that got auto-logged in need to have DLC checked for them
	for (INT UserIndex = 0; UserIndex < 4; UserIndex++)
	{
		// Wipe their profile cache
		ProfileCache[UserIndex].Profile = NULL;
		// Clear and then cache the last logged in xuid
		(XUID&)LastXuids[UserIndex] = 0;
		XUserGetXUID(UserIndex,&(XUID&)LastXuids[UserIndex]);
		// See if they are signed in
		UBOOL bIsSignedIn = XUserGetSigninState(UserIndex) != eXUserSigninState_NotSignedIn;
		// Update the last sign in mask with their sign in state
		LastSignInMask |= ((bIsSignedIn) << UserIndex);
		if (bIsSignedIn)
		{
			ReadContentList(UserIndex);
		}
		// Init the base controller connection state
		if (XInputGetCapabilities(UserIndex,XINPUT_FLAG_GAMEPAD,&InputCaps) == ERROR_SUCCESS)
		{
			LastInputDeviceConnectedMask |= 1 << UserIndex;
		}
	}
	// Register the notifications we are interested in
	NotificationHandle = XNotifyCreateListener(XNOTIFY_SYSTEM | XNOTIFY_LIVE | XNOTIFY_FRIENDS);
	if (NotificationHandle == NULL)
	{
		debugf(NAME_Error,TEXT("Failed to create Live notification listener"));
	}
	return NotificationHandle != NULL;
}

/**
 * Checks for new Live notifications and passes them out to registered delegates
 *
 * @param DeltaTime the amount of time that has passed since the last tick
 */
void UOnlineSubsystemLive::Tick(FLOAT DeltaTime)
{
	DWORD Notification = 0;
	ULONG_PTR Data = NULL;
	// Check Live for notification events
	while (XNotifyGetNext(NotificationHandle,0,&Notification,&Data))
	{
		// Now process the event
		ProcessNotification(Notification,Data);
	}
	// Now tick any outstanding async tasks
	TickAsyncTasks();
	// Tick any tasks needed for LAN support
	TickLanTasks(DeltaTime);
	// Tick voice processing
	TickVoice(DeltaTime);
}

/**
 * Processes a notification that was returned during polling
 *
 * @param Notification the notification event that was fired
 * @param Data the notification specifc data
 */
void UOnlineSubsystemLive::ProcessNotification(DWORD Notification,ULONG_PTR Data)
{
	switch (Notification)
	{
		case XN_SYS_SIGNINCHANGED:
		{
			ProcessSignInNotification(Data);
			break;
		}
		case XN_SYS_MUTELISTCHANGED:
		{
			if (DELEGATE_IS_SET(OnMutingChange))
			{
				delegateOnMutingChange();
			}
			// Have the voice code re-evaluate its mute settings
			ProcessMuteChangeNotification();
			break;
		}
		case XN_FRIENDS_FRIEND_ADDED:
		case XN_FRIENDS_FRIEND_REMOVED:
		{
			if (DELEGATE_IS_SET(OnFriendsChange))
			{
				delegateOnFriendsChange();
			}
			break;
		}
		case XN_SYS_UI:
		{
			// Data will be non-zero if opening
			UBOOL bIsOpening = Data != 0;
			ProcessExternalUINotification(bIsOpening);
			break;
		}
		case XN_SYS_INPUTDEVICESCHANGED:
		{
			ProcessControllerNotification();
			break;
		}
		case XN_LIVE_LINK_STATE_CHANGED:
		{
			// Data will be non-zero if connected
			UBOOL bIsConnected = Data != 0;
			// Notify registered delegates
			ProcessLinkStateNotification(bIsConnected);
			break;
		}
		case XN_LIVE_CONTENT_INSTALLED:
		{
			// remove all downloaded content, it will be added again below
			FlushAllDownloadedContent(4);
			// Build the last sign in mask and refresh content
			for (INT Index = 0; Index < 4; Index++)
			{
				// See if they are signed in
				if (XUserGetSigninState(Index) != eXUserSigninState_NotSignedIn)
				{
					ReadContentList(Index);
				}
			}
			break;
		}
		case XN_LIVE_INVITE_ACCEPTED:
		{
			// Accept the invite for the specified user
			ProcessGameInvite((DWORD)Data);
			break;
		}
		case XN_LIVE_CONNECTIONCHANGED:
		{
			// Fire off any events needed for this notification
			ProcessConnectionStatusNotification((HRESULT)Data);
			break;
		}
		case XN_SYS_PROFILESETTINGCHANGED:
		{
			ProcessProfileDataNotification(Data);
			break;
		}
	}
}

/**
 * Handles any sign in change processing (firing delegates, etc)
 *
 * @param Data the mask of changed sign ins
 */
void UOnlineSubsystemLive::ProcessSignInNotification(ULONG_PTR Data)
{
	// Notify each subscriber
	for (INT Index = 0; Index < AllLoginDelegates.Delegates.Num(); Index++)
	{
		if (AllLoginDelegates.Delegates(Index).IsCallable())
		{
			ProcessDelegate(ONLINESUBSYSTEMLIVE_OnLoginChange,&AllLoginDelegates.Delegates(Index),NULL);
		}
	}
	// remove all downloaded content, it will be added again below
	FlushAllDownloadedContent(4);
	// Loop through the valid bits and send notifications
	for (DWORD Index = 0; Index < 4; Index++)
	{
		DWORD SignInState = XUserGetSigninState(Index);
		// Clear and get the currently logged in xuid
		XUID CurrentXuid = 0;
		XUserGetXUID(Index,&CurrentXuid);
		// Is the user signed in? or were they signed in?
		if ((SignInState != eXUserSigninState_NotSignedIn && !(LastSignInMask & (1 << Index))) ||
			((SignInState == eXUserSigninState_NotSignedIn && (LastSignInMask & (1 << Index)))) ||
			// Make sure their XUID matches
			CurrentXuid != (XUID&)LastXuids[Index])
		{
			debugf(NAME_DevLive,TEXT("Discarding cached profile for user %d"),Index);
			// Zero the cached profile so we don't use the wrong profile
			ProfileCache[Index].Profile = NULL;
			// Fire the delegate for each registered delegate
			for (INT Index2 = 0; Index2 < PlayerLoginDelegates[Index].Delegates.Num(); Index2++)
			{
				if (PlayerLoginDelegates[Index].Delegates(Index2).IsCallable())
				{
					ProcessDelegate(ONLINESUBSYSTEMLIVE_OnLoginChange,
						&PlayerLoginDelegates[Index].Delegates(Index2),NULL);
				}
			}
		}
	}
// @todo josh: update maplist data store/provider here!!!
	LastSignInMask = 0;
	// Build the last sign in mask and refresh content
	for (INT Index = 0; Index < 4; Index++)
	{
		// Clear and then cache the last logged in xuid
		(XUID&)LastXuids[Index] = 0;
		XUserGetXUID(Index,&(XUID&)LastXuids[Index]);
		// See if they are signed in
		UBOOL bIsSignedIn = XUserGetSigninState(Index) != eXUserSigninState_NotSignedIn;
		// Update the last sign in mask with their sign in state
		LastSignInMask |= ((bIsSignedIn) << Index);
		// read content for newly signed in users
		if (bIsSignedIn)
		{
			ReadContentList(Index);
		}
	}
	// Update voice's registered talkers
	UpdateVoiceFromLoginChange();
	bIsInSignInUI = FALSE;
}

/**
 * Handles notifying interested parties when a signin is cancelled
 */
void UOnlineSubsystemLive::ProcessSignInCancelledNotification(void)
{
	// Notify each subscriber of the user canceling
	for (INT Index = 0; Index < LoginCancelledDelegates.Num(); Index++)
	{
		if (LoginCancelledDelegates(Index).IsCallable())
		{
			ProcessDelegate(ONLINESUBSYSTEMLIVE_OnLoginCancelled,&LoginCancelledDelegates(Index),NULL);
		}
	}
	bIsInSignInUI = FALSE;
}

/**
 * Handles accepting a game invite for the specified user
 *
 * @param UserNum the user that accepted the game invite
 */
void UOnlineSubsystemLive::ProcessGameInvite(DWORD UserNum)
{
	// This code assumes XNKID is 8 bytes
	check(sizeof(QWORD) == sizeof(XNKID));
	XINVITE_INFO* Info;
	// Allocate space on demand
	if (InviteCache[UserNum].InviteData == NULL)
	{
		InviteCache[UserNum].InviteData = new XINVITE_INFO;
	}
	// If for some reason the data didn't get cleaned up, do so now
	if (InviteCache[UserNum].InviteSearch != NULL &&
		InviteCache[UserNum].InviteSearch->Results.Num() > 0)
	{
		// Clean up the invite data
		delete (XSESSION_INFO*)InviteCache[UserNum].InviteSearch->Results(0).PlatformData;
		InviteCache[UserNum].InviteSearch->Results(0).PlatformData = NULL;
		InviteCache[UserNum].InviteSearch = NULL;
	}
	// Get the buffer to use and clear the previous contents
	Info = InviteCache[UserNum].InviteData;
	appMemzero(Info,sizeof(XINVITE_INFO));
	// Ask Live for the game details (session info)
	DWORD Return = XInviteGetAcceptedInfo(UserNum,Info);
	debugf(NAME_DevLive,TEXT("XInviteGetAcceptedInfo(%d,Data) returned 0x%08X"),
		UserNum,Return);
	if (Return == ERROR_SUCCESS)
	{
		// Don't trigger an invite notification if we are in this session
		if (SessionInfo == NULL ||
			(QWORD&)SessionInfo->XSessionInfo.sessionID != (QWORD&)Info->hostInfo.sessionID)
		{
			DWORD Size = 0;
			// Kick off an async search of the game by its session id. This is because
			// we need the gamesettings to understand what options to set
			//
			// First read the size needed to hold the results
			Return = XSessionSearchByID(Info->hostInfo.sessionID,UserNum,&Size,NULL,NULL);
			if (Return == ERROR_INSUFFICIENT_BUFFER && Size > 0)
			{
				// Create the async task with the proper data size
				FLiveAsyncTaskJoinGameInvite* AsyncTask = new FLiveAsyncTaskJoinGameInvite(UserNum,
					Size,&InviteCache[UserNum].InviteDelegate);
				// Now kick off the task
				Return = XSessionSearchByID(Info->hostInfo.sessionID,
					UserNum,
					&Size,
					AsyncTask->GetResults(),
					*AsyncTask);
				debugf(NAME_DevLive,TEXT("XSessionSearchByID(SessId,%d,%d,Data,Overlapped) returned 0x%08X"),
					UserNum,Size,Return);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Don't leak the task/data
					delete AsyncTask;
				}
			}
			else
			{
				debugf(NAME_Error,
					TEXT("Couldn't determine the size needed for searching for the game invite information 0x%08X"),
					Return);
			}
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Ignoring a game invite to a session we're already in"));
		}
	}
}

/**
 * Handles external UI change notifications
 *
 * @param bIsOpening whether the UI is opening or closing
 */
void UOnlineSubsystemLive::ProcessExternalUINotification(UBOOL bIsOpening)
{
    OnlineSubsystemLive_eventOnExternalUIChange_Parms Parms(EC_EventParm);
    Parms.bIsOpening = bIsOpening ? FIRST_BITFIELD : 0;
	// Notify all registered delegates of the UI changes
	for (INT Index = 0; Index < ExternalUIChangeDelegates.Num(); Index++)
	{
		ProcessDelegate(NAME_None,&ExternalUIChangeDelegates(Index),&Parms);
	}
	// Handle user cancelling a signin request
	if (bIsOpening == FALSE && bIsInSignInUI == TRUE)
	{
		ProcessSignInCancelledNotification();
	}
}

/**
 * Handles controller connection state changes
 */
void UOnlineSubsystemLive::ProcessControllerNotification(void)
{
	XINPUT_CAPABILITIES InputCaps;
	appMemzero(&InputCaps,sizeof(XINPUT_CAPABILITIES));
	// Default to none connected
	INT CurrentMask = 0;
	// Iterate the controllers cheching their state
	for (INT ControllerIndex = 0; ControllerIndex < 4; ControllerIndex++)
	{
		INT ControllerMask = 1 << ControllerIndex;
		// See if this controller is connected or not
		if (XInputGetCapabilities(ControllerIndex,XINPUT_FLAG_GAMEPAD,&InputCaps) == ERROR_SUCCESS)
		{
			CurrentMask |= ControllerMask;
		}
		// Only fire the event if the connection status has changed
		if ((CurrentMask & ControllerMask) != (LastInputDeviceConnectedMask & ControllerMask))
		{
			OnlineSubsystemLive_eventOnControllerChange_Parms Parms(EC_EventParm);
			Parms.ControllerId = ControllerIndex;
			// If the current mask and the controller mask match, the controller was inserted
			// otherwise it was removed
			Parms.bIsConnected = (CurrentMask & ControllerMask) ? FIRST_BITFIELD : 0;
			// Notify all registered delegates of the controller changes
			for (INT Index = 0; Index < ControllerChangeDelegates.Num(); Index++)
			{
				ProcessDelegate(NAME_None,&ControllerChangeDelegates(Index),&Parms);
			}
		}
	}
	// Set to the new state mask
	LastInputDeviceConnectedMask = CurrentMask;
}

/**
 * Handles notifying interested parties when the Live connection status
 * has changed
 *
 * @param Status the type of change that has happened
 */
void UOnlineSubsystemLive::ProcessConnectionStatusNotification(HRESULT Status)
{
	EOnlineServerConnectionStatus ConnectionStatus;
	// Map the Live code to ours
	switch (Status)
	{
		case XONLINE_S_LOGON_CONNECTION_ESTABLISHED:
			ConnectionStatus = OSCS_Connected;
			break;
		case XONLINE_S_LOGON_DISCONNECTED:
			ConnectionStatus = OSCS_NotConnected;
			break;
		case XONLINE_E_LOGON_NO_NETWORK_CONNECTION:
			ConnectionStatus = OSCS_NoNetworkConnection;
			break;
		case XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE:
			ConnectionStatus = OSCS_ServiceUnavailable;
			break;
		case XONLINE_E_LOGON_UPDATE_REQUIRED:
			ConnectionStatus = OSCS_UpdateRequired;
			break;
		case XONLINE_E_LOGON_SERVERS_TOO_BUSY:
			ConnectionStatus = OSCS_ServersTooBusy;
			break;
		case XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON:
			ConnectionStatus = OSCS_DuplicateLoginDetected;
			break;
		case XONLINE_E_LOGON_INVALID_USER:
			ConnectionStatus = OSCS_InvalidUser;
			break;
		default:
			ConnectionStatus = OSCS_ConnectionDropped;
			break;
	}
    OnlineSubsystemLive_eventOnConnectionStatusChange_Parms Parms(EC_EventParm);
    Parms.ConnectionStatus = ConnectionStatus;
	// Now send to any registered delegates
	for (INT Index = 0; Index < ConnectionStatusChangeDelegates.Num(); Index++)
	{
		ProcessDelegate(NAME_None,&ConnectionStatusChangeDelegates(Index),&Parms);
	}
}

/**
 * Handles notifying interested parties when the link state changes
 *
 * @param bIsConnected whether the link has a connection or not
 */
void UOnlineSubsystemLive::ProcessLinkStateNotification(UBOOL bIsConnected)
{
    OnlineSubsystemLive_eventOnLinkStatusChange_Parms Parms(EC_EventParm);
    Parms.bIsConnected = bIsConnected ? FIRST_BITFIELD : 0;
	// Now send to any registered delegates
	for (INT Index = 0; Index < LinkStatusChangeDelegates.Num(); Index++)
	{
		ProcessDelegate(NAME_None,&LinkStatusChangeDelegates(Index),&Parms);
	}
}

/**
 * Handles notifying interested parties when the player changes profile data
 *
 * @param ChangeStatus bit flags indicating which user just changed status
 */
void UOnlineSubsystemLive::ProcessProfileDataNotification(DWORD ChangeStatus)
{
	// Check each user for a change
	for (DWORD Index = 0; Index < 4; Index++)
	{
		if ((1 << Index) & ChangeStatus)
		{
			// Notify this delegate of the change
			ProcessDelegate(NAME_None,&ProfileDataChangedDelegates[Index],NULL);
		}
	}
}

/**
 * Iterates the list of outstanding tasks checking for their completion
 * status. Upon completion, it fires off the corresponding delegate
 * notification
 */
void UOnlineSubsystemLive::TickAsyncTasks(void)
{
	// Check each task for completion
	for (INT Index = 0; Index < AsyncTasks.Num(); Index++)
	{
		if (AsyncTasks(Index)->HasTaskCompleted())
		{
			// Perform any task specific finalization of data before having
			// the script delegate fired off
			if (AsyncTasks(Index)->ProcessAsyncResults(this) == TRUE)
			{
				// Triggering of script delegates must be done from a UObject
				// so it's done here instead of inside the async task class
				FScriptDelegate* ScriptDelegate = AsyncTasks(Index)->GetDelegate();
				if (ScriptDelegate != NULL)
				{
					// Pass in the data that indicates whether the call worked or not
					FAsyncTaskDelegateResults Results(AsyncTasks(Index)->GetCompletionCode());
					// Send the notification of completion
					ProcessDelegate(NAME_None,ScriptDelegate,&Results);
				}
				// Free the memory and remove it from our list
				delete AsyncTasks(Index);
				AsyncTasks.Remove(Index);
				Index--;
			}
		}
	}
}

/**
 * Ticks any system link background tasks
 *
 * @param DeltaTime the time since the last tick
 */
void UOnlineSubsystemLive::TickLanTasks(FLOAT DeltaTime)
{
	if (SystemLinkState > SLS_NotUsingSystemLink && SysLinkSocket != NULL)
	{
		BYTE PacketData[512];
		UBOOL bShouldRead = TRUE;
		// Read each pending packet and pass it out for processing
		while (bShouldRead)
		{
			INT NumRead = SysLinkSocket->Poll(PacketData,512);
			if (NumRead > 0)
			{
				// Hand this packet off for processing
				ParseSystemLinkPacket(PacketData,NumRead);
				// Reset the timeout since a packet came in
				SystemLinkQueryTimeLeft = SystemLinkQueryTimeout;
			}
			else
			{
				if (SystemLinkState == SLS_Searching)
				{
					// Decrement the amount of time remaining
					SystemLinkQueryTimeLeft -= DeltaTime;
					// Check for a timeout on the search packet
					if (SystemLinkQueryTimeLeft <= 0.f)
					{
						// Trigger the delegate so the UI knows we didn't find any
						delegateOnFindOnlineGamesComplete(TRUE);
						// Stop future timeouts since we aren't searching any more
						SystemLinkState = SLS_NotUsingSystemLink;
						if (GameSearch != NULL)
						{
							GameSearch->bIsSearchInProgress = FALSE;
						}
					}
				}
				bShouldRead = FALSE;
			}
		}
	}
}

/**
 * Ticks voice subsystem for reading/submitting any voice data
 *
 * @param DeltaTime the time since the last tick
 */
void UOnlineSubsystemLive::TickVoice(FLOAT DeltaTime)
{
	// If we aren't using VDP or aren't in a networked match, no need to update
	// networked voice
	if (VoiceEngine != NULL &&
		GSocketSubsystem->RequiresChatDataBeSeparate() &&
		GameSettings != NULL)
	{
		// Queue local packets for sending via the network
		ProcessLocalVoicePackets();
		// Submit queued packets to XHV
		ProcessRemoteVoicePackets();
		// Fire off any talking notifications for hud display
		ProcessTalkingDelegates();
	}
	// Check the speech recognition engine for pending notifications
	ProcessSpeechRecognitionDelegates();
}

/**
 * Reads any data that is currently queued in XHV
 */
void UOnlineSubsystemLive::ProcessLocalVoicePackets(void)
{
	// Read the data from any local talkers
	DWORD DataReadyFlags = VoiceEngine->GetVoiceDataReadyFlags();
	// Skip processing if there is no data from a local talker
	if (DataReadyFlags)
	{
		// Process each talker with a bit set
		for (DWORD Index = 0; DataReadyFlags; Index++, DataReadyFlags >>= 1)
		{
			// Talkers needing processing will always be in lsb due to shifts
			if (DataReadyFlags & 1)
			{
				// Mark the person as talking
				LocalTalkers[Index].bWasTalking = TRUE;
				DWORD SpaceAvail = MAX_VOICE_DATA_SIZE - GVoiceData.LocalPackets[Index].Length;
				// Figure out if there is space for this packet
				if (SpaceAvail > 0)
				{
					DWORD NumPacketsCopied = 0;
					// Figure out where to append the data
					BYTE* BufferStart = GVoiceData.LocalPackets[Index].Buffer;
					BufferStart += GVoiceData.LocalPackets[Index].Length;
					// Copy the sender info
					XUserGetXUID(Index,&GVoiceData.LocalPackets[Index].Sender);
					// Process this user
					HRESULT hr = VoiceEngine->ReadLocalVoiceData(Index,
						BufferStart,
						&SpaceAvail);
					if (SUCCEEDED(hr))
					{
						// Update the length based on what it copied
						GVoiceData.LocalPackets[Index].Length += SpaceAvail;
					}
				}
				else
				{
					debugf(NAME_DevLive,TEXT("Dropping voice data due to network layer not processing fast enough"));
					// Buffer overflow, so drop previous data
					GVoiceData.LocalPackets[Index].Length = 0;
				}
			}
		}
	}
}

/**
 * Submits network packets to XHV for playback
 */
void UOnlineSubsystemLive::ProcessRemoteVoicePackets(void)
{
	// Now process all pending packets from the server
	for (INT Index = 0; Index < GVoiceData.RemotePackets.Num(); Index++)
	{
		FVoicePacket* VoicePacket = GVoiceData.RemotePackets(Index);
		if (VoicePacket != NULL)
		{
			// Get the size since it is an in/out param
			DWORD PacketSize = VoicePacket->Length;
			// Submit this packet to the XHV engine
			HRESULT hr = VoiceEngine->SubmitRemoteVoiceData((FUniqueNetId&)VoicePacket->Sender,
				VoicePacket->Buffer,
				&PacketSize);
			if (FAILED(hr))
			{
				debugf(NAME_Error,TEXT("SubmitIncomingChatData() failed with 0x%08X"),hr);
			}
			// Skip all delegate handling if none are registered
			if (TalkingDelegates.Num() > 0)
			{
				// Find the remote talker and mark them as talking
				for (INT Index2 = 0; Index2 < RemoteTalkers.Num(); Index2++)
				{
					FRemoteTalker& Talker = RemoteTalkers(Index2);
					// Compare the xuids
					if ((XUID&)Talker.TalkerId == VoicePacket->Sender)
					{
						Talker.bWasTalking = TRUE;
					}
				}
			}
			VoicePacket->DecRef();
		}
	}
	// Zero the list without causing a free/realloc
	GVoiceData.RemotePackets.Reset();
}

/**
 * Processes any talking delegates that need to be fired off
 */
void UOnlineSubsystemLive::ProcessTalkingDelegates(void)
{
	// Skip all delegate handling if none are registered
	if (TalkingDelegates.Num() > 0)
	{
		// Fire off any talker notification delegates for local talkers
		for (DWORD Index = 0; Index < 4; Index++)
		{
			// Only check players with voice
			if (LocalTalkers[Index].bHasVoice && LocalTalkers[Index].bWasTalking)
			{
				OnlineSubsystemLive_eventOnPlayerTalking_Parms Parms(EC_EventParm);
				// Read the XUID from Live
				DWORD Return = XUserGetXUID(Index,(XUID*)&Parms.Player);
				if (Return == ERROR_SUCCESS)
				{
					// Fire the delegates off
					for (INT Index2 = 0; Index2 < TalkingDelegates.Num(); Index2++)
					{
						ProcessDelegate(ONLINESUBSYSTEMLIVE_OnPlayerTalking,
							&TalkingDelegates(Index2),
							&Parms);
					}
				}
				// Clear the flag so it only activates when needed
				LocalTalkers[Index].bWasTalking = FALSE;
			}
		}
		// Now check all remote talkers
		for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
		{
			FRemoteTalker& Talker = RemoteTalkers(Index);
			// Make sure to not fire off the delegate if this talker is muted
			if (Talker.bWasTalking && Talker.NumberOfMutes == 0)
			{
				OnlineSubsystemLive_eventOnPlayerTalking_Parms Parms(EC_EventParm);
				Parms.Player = Talker.TalkerId;
				// Fire the delegates off
				for (INT Index2 = 0; Index2 < TalkingDelegates.Num(); Index2++)
				{
					ProcessDelegate(ONLINESUBSYSTEMLIVE_OnPlayerTalking,
						&TalkingDelegates(Index2),
						&Parms);
				}
				// Clear the flag so it only activates when needed
				Talker.bWasTalking = FALSE;
			}
		}
	}
}

/**
 * Processes any speech recognition delegates that need to be fired off
 */
void UOnlineSubsystemLive::ProcessSpeechRecognitionDelegates(void)
{
	// Skip all delegate handling if we aren't using speech recognition
	if (bIsUsingSpeechRecognition)
	{
		// Fire off any talker notification delegates for local talkers
		for (DWORD Index = 0; Index < 4; Index++)
		{
			if (VoiceEngine->HasRecognitionCompleted(Index))
			{
				ProcessDelegate(ONLINESUBSYSTEMLIVE_OnRecognitionComplete,
					&SpeechRecognitionDelegates[Index],
					NULL);
			}
		}
	}
}

/**
 * Processes a system link packet. For a host, responds to discovery
 * requests. For a client, parses the discovery response and places
 * the resultant data in the current search's search results array
 *
 * @param PacketData the packet data to parse
 * @param PacketLength the amount of data that was received
 */
void UOnlineSubsystemLive::ParseSystemLinkPacket(BYTE* PacketData,INT PacketLength)
{
	// Check our mode to determine the type of allowed packets
	if (SystemLinkState == SLS_Hosting)
	{
		// Don't respond to queries when the match is full
		if (GameSettings->NumOpenPublicConnections > 0)
		{
			// We can only accept Server Query packets
			if (PacketLength == 10 && PacketData[0] == 'S' && PacketData[1] == 'Q')
			{
				BYTE* ClientNonce = &PacketData[2];
				FSystemLinkPacketWriter Packet;
				// Build a response packet containing client nonce, host info,
				// and game settings
				Packet << SYSTEM_LINK_PACKET_VERSION << 'S' << 'R'
					// Append the client nonce as 8 individual bytes
					<< ClientNonce[0] << ClientNonce[1] << ClientNonce[2]
					<< ClientNonce[3] << ClientNonce[4] << ClientNonce[5]
					<< ClientNonce[6] << ClientNonce[7];
				// Write host info (host addr, session id, and key)
				Packet << SessionInfo->XSessionInfo.hostAddress
					<< SessionInfo->XSessionInfo.sessionID
					<< SessionInfo->XSessionInfo.keyExchangeKey;
				// Now append per game settings
				AppendGameSettings(Packet,GameSettings);
				// Broadcast this response so the client can see us
				if (SysLinkSocket->BroadcastPacket(Packet,Packet.GetByteCount()) == FALSE)
				{
					debugf(NAME_Error,TEXT("Failed to send response packet %d"),
						GSocketSubsystem->GetLastErrorCode());
				}
			}
		}
	}
	else if (SystemLinkState == SLS_Searching)
	{
		// Anything shorter than 11 is malformed and should be ignored
		if (PacketLength >= SYSTEM_LINK_PACKET_HEADER_SIZE)
		{
			// We can only accept Server Response packets
			if (PacketData[0] == SYSTEM_LINK_PACKET_VERSION &&
				PacketData[1] == 'S' && PacketData[2] == 'R' &&
				// Make sure this our nonce or we should ignore the packet
				appMemcmp(&PacketData[3],SystemLinkNonce,8) == 0)
			{
				// Create an object that we'll copy the data to
				UOnlineGameSettings* NewServer = ConstructObject<UOnlineGameSettings>(
					GameSearch->GameSettingsClass);
				if (NewServer != NULL)
				{
					// Add space in the search results array
					INT NewSearch = GameSearch->Results.Add();
					FOnlineGameSearchResult& Result = GameSearch->Results(NewSearch);
					// Link the settings to this result
					Result.GameSettings = NewServer;
					// Strip off the type and nonce since it's been validated
					FSystemLinkPacketReader Packet(&PacketData[SYSTEM_LINK_PACKET_HEADER_SIZE],
						PacketLength - SYSTEM_LINK_PACKET_HEADER_SIZE);
					// Allocate and read the session data
					XSESSION_INFO* SessInfo = new XSESSION_INFO;
					// Read the connection data
					Packet >> SessInfo->hostAddress
						>> SessInfo->sessionID
						>> SessInfo->keyExchangeKey;
					// Store this in the results
					Result.PlatformData = SessInfo;
					// Read any per object data using the server object
					ReadGameSettings(Packet,NewServer);
					// Let any registered consumers know the data has changed
					if (__OnFindOnlineGamesComplete__Delegate.IsCallable())
					{
						delegateOnFindOnlineGamesComplete(TRUE);
					}
				}
				else
				{
					debugf(NAME_Error,TEXT("Failed to create new online game settings object"));
				}
			}
		}
	}
}

/**
 * Adds the game settings data to the packet that is sent by the host
 * in reponse to a server query
 *
 * @param Packet the writer object that will encode the data
 * @param GameSettings the game settings to add to the packet
 */
void UOnlineSubsystemLive::AppendGameSettings(FSystemLinkPacketWriter& Packet,
	UOnlineGameSettings* GameSettings)
{
//@todo joeg -- consider using introspection (property iterator or databinding
// iterator) to handle subclasses

#if DEBUG_SYSLINK
	debugf(NAME_DevLive,TEXT("Sending game settings to client"));
#endif
	// Members of the game settings class
	Packet << GameSettings->NumOpenPublicConnections
		<< GameSettings->NumOpenPrivateConnections
		<< GameSettings->NumPublicConnections
		<< GameSettings->NumPrivateConnections
		<< (BYTE)GameSettings->bShouldAdvertise
		<< (BYTE)GameSettings->bIsLanMatch
		<< (BYTE)GameSettings->bUsesStats
		<< (BYTE)GameSettings->bAllowJoinInProgress
		<< (BYTE)GameSettings->bAllowInvites
		<< (BYTE)GameSettings->bUsesPresence
		<< (BYTE)GameSettings->bAllowJoinViaPresence
		<< (BYTE)GameSettings->bUsesArbitration;
	// Write the player id so we can show gamercard
	Packet << GameSettings->OwningPlayerId;
	Packet << GameSettings->OwningPlayerName;
#if DEBUG_SYSLINK
	BYTE* Xuid = GameSettings->OwningPlayerId.Uid;
	debugf(NAME_DevLive,TEXT("%u,%u,%u,%u,%u,%u,%u,%u"),Xuid[0],Xuid[1],Xuid[2],
		Xuid[3],Xuid[4],Xuid[5],Xuid[6],Xuid[7]);
	debugf(NAME_DevLive,*GameSettings->OwningPlayerName);
#endif
	// Now add the contexts and properties from the settings class
	// First, add the number contexts involved
	INT Num = GameSettings->LocalizedSettings.Num();
	Packet << Num;
	// Now add each context individually
	for (INT Index = 0; Index < GameSettings->LocalizedSettings.Num(); Index++)
	{
		Packet << GameSettings->LocalizedSettings(Index);
#if DEBUG_SYSLINK
		debugf(NAME_DevLive,*BuildContextString(GameSettings,GameSettings->LocalizedSettings(Index)));
#endif
	}
	// Next, add the number of properties involved
	Num = GameSettings->Properties.Num();
	Packet << Num;
	// Now add each property
	for (INT Index = 0; Index < GameSettings->Properties.Num(); Index++)
	{
		Packet << GameSettings->Properties(Index);
#if DEBUG_SYSLINK
		debugf(NAME_DevLive,*BuildPropertyString(GameSettings,GameSettings->Properties(Index)));
#endif
	}
}

/**
 * Reads the game settings data from the packet and applies it to the
 * specified object
 *
 * @param Packet the reader object that will read the data
 * @param GameSettings the game settings to copy the data to
 */
void UOnlineSubsystemLive::ReadGameSettings(FSystemLinkPacketReader& Packet,
	UOnlineGameSettings* GameSettings)
{
//@todo joeg -- consider using introspection (property iterator or databinding
// iterator) to handle subclasses

#if DEBUG_SYSLINK
	debugf(NAME_DevLive,TEXT("Reading game settings from server"));
#endif
	// Members of the game settings class
	Packet >> GameSettings->NumOpenPublicConnections
		>> GameSettings->NumOpenPrivateConnections
		>> GameSettings->NumPublicConnections
		>> GameSettings->NumPrivateConnections;
	BYTE Read;
	// Read all the bools as bytes
	Packet >> Read;
	GameSettings->bShouldAdvertise = Read == TRUE;
	Packet >> Read;
	GameSettings->bIsLanMatch = Read == TRUE;
	Packet >> Read;
	GameSettings->bUsesStats = Read == TRUE;
	Packet >> Read;
	GameSettings->bAllowJoinInProgress = Read == TRUE;
	Packet >> Read;
	GameSettings->bAllowInvites = Read == TRUE;
	Packet >> Read;
	GameSettings->bUsesPresence = Read == TRUE;
	Packet >> Read;
	GameSettings->bAllowJoinViaPresence = Read == TRUE;
	Packet >> Read;
	GameSettings->bUsesArbitration = Read == TRUE;
	// Read the owning player id
	Packet >> GameSettings->OwningPlayerId;
	// Read the owning player name
	Packet >> GameSettings->OwningPlayerName;
#if DEBUG_SYSLINK
	BYTE* Xuid = GameSettings->OwningPlayerId.Uid;
	debugf(NAME_DevLive,TEXT("%u,%u,%u,%u,%u,%u,%u,%u"),Xuid[0],Xuid[1],Xuid[2],
		Xuid[3],Xuid[4],Xuid[5],Xuid[6],Xuid[7]);
	debugf(NAME_DevLive,*GameSettings->OwningPlayerName);
#endif
	// Now read the contexts and properties from the settings class
	INT NumContexts;
	// First, read the number contexts involved, so we can presize the array
	Packet >> NumContexts;
	GameSettings->LocalizedSettings.Empty(NumContexts);
	GameSettings->LocalizedSettings.AddZeroed(NumContexts);
	// Now read each context individually
	for (INT Index = 0; Index < GameSettings->LocalizedSettings.Num(); Index++)
	{
		Packet >> GameSettings->LocalizedSettings(Index);
#if DEBUG_SYSLINK
		debugf(NAME_DevLive,*BuildContextString(GameSettings,GameSettings->LocalizedSettings(Index)));
#endif
	}
	INT NumProps;
	// Next, read the number of properties involved for array presizing
	Packet >> NumProps;
	GameSettings->Properties.Empty(NumProps);
	GameSettings->Properties.AddZeroed(NumProps);
	// Now read each property from the packet
	for (INT Index = 0; Index < GameSettings->Properties.Num(); Index++)
	{
		Packet >> GameSettings->Properties(Index);
#if DEBUG_SYSLINK
		debugf(NAME_DevLive,*BuildPropertyString(GameSettings,GameSettings->Properties(Index)));
#endif
	}
}

/**
 * Displays the Xbox Guide to perform the login
 *
 * @param bShowOnlineOnly whether to only display online enabled profiles or not
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowLoginUI(UBOOL bShowOnlineOnly)
{
	// We allow 1, 2, or 4 on Xe. Should be 1 for Windows Live
	if (NumLogins != 1 && NumLogins != 2 && NumLogins != 4)
	{
		NumLogins = 1;
	}
	DWORD Result = XShowSigninUI(NumLogins,bShowOnlineOnly ? XSSUI_FLAGS_SHOWONLYONLINEENABLED : 0);
	if (Result == ERROR_SUCCESS)
	{
		bIsInSignInUI = TRUE;
	}
	else
	{
		debugf(NAME_Error,TEXT("XShowSigninUI(%d,0) failed with 0x%08X"),NumLogins,Result);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Fetches the login status for a given player from Live
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the enum value of their status
 */
BYTE UOnlineSubsystemLive::GetLoginStatus(BYTE LocalUserNum)
{
	ELoginStatus Status = LS_NotLoggedIn;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Ask Live for the login status
		XUSER_SIGNIN_STATE State = XUserGetSigninState(LocalUserNum);
		if (State == eXUserSigninState_SignedInToLive)
		{
			Status = LS_LoggedIn;
		}
		else if (State == eXUserSigninState_SignedInLocally)
		{
			Status = LS_UsingLocalProfile;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to GetLoginStatus()"),
			(DWORD)LocalUserNum);
	}
	return Status;
}

/**
 * Gets the platform specific unique id for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param UniqueId the byte array that will receive the id
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::GetUniquePlayerId(BYTE LocalUserNum,FUniqueNetId& UniqueId)
{
	check(sizeof(XUID) == 8);
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Read the XUID from Live
		Result = XUserGetXUID(LocalUserNum,(XUID*)&UniqueId);
		if (Result != ERROR_SUCCESS)
		{
			debugf(TEXT("XUserGetXUID(%d) failed 0x%08X"),LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to GetUniquePlayerId()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Reads the player's nick name from the online service
 *
 * @param UniqueId the unique id of the player being queried
 *
 * @return a string containing the players nick name
 */
FString UOnlineSubsystemLive::GetPlayerNickname(BYTE LocalUserNum)
{
	ANSICHAR Buffer[32];
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Read the gamertag from Live
		Result = XUserGetName(LocalUserNum,Buffer,sizeof(Buffer));
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to GetPlayerNickname()"),
			(DWORD)LocalUserNum);
	}
	if (Result != ERROR_SUCCESS)
	{
		debugf(TEXT("XUserGetName(%d) failed 0x%08X"),LocalUserNum,Result);
		Buffer[0] = '\0';
	}
	return FString(ANSI_TO_TCHAR(Buffer));
}

/**
 * Determines whether the player is allowed to play online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanPlayOnline(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		Result = XUserCheckPrivilege(LocalUserNum,XPRIVILEGE_MULTIPLAYER_SESSIONS,
			&bCan);
		if (Result == ERROR_SUCCESS)
		{
			Priv = bCan == TRUE ? FPL_Enabled : FPL_Disabled;
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_MULTIPLAYER_SESSIONS) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanPlayOnline()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Determines whether the player is allowed to use voice or text chat online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanCommunicate(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		Result = XUserCheckPrivilege(LocalUserNum,XPRIVILEGE_COMMUNICATIONS,
			&bCan);
		if (Result == ERROR_SUCCESS)
		{
			if (bCan == TRUE)
			{
				// Universally ok
				Priv = FPL_Enabled;
			}
			else
			{
				// Not valid for everyone so check for friends only
				Result = XUserCheckPrivilege(LocalUserNum,
					XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY,&bCan);
				if (Result == ERROR_SUCCESS)
				{
					// Can only do this with friends or not at all
					Priv = bCan == TRUE ? FPL_EnabledFriendsOnly : FPL_Disabled;
				}
				else
				{
					debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY) failed with 0x%08X"),
						LocalUserNum,Result);
				}
			}
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_COMMUNICATIONS) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanCommunicate()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Determines whether the player is allowed to download user created content
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanDownloadUserContent(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		Result = XUserCheckPrivilege(LocalUserNum,
			XPRIVILEGE_USER_CREATED_CONTENT,&bCan);
		if (Result == ERROR_SUCCESS)
		{
			if (bCan == TRUE)
			{
				// Universally ok
				Priv = FPL_Enabled;
			}
			else
			{
				// Not valid for everyone so check for friends only
				Result = XUserCheckPrivilege(LocalUserNum,
					XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY,&bCan);
				if (Result == ERROR_SUCCESS)
				{
					// Can only do this with friends or not at all
					Priv = bCan == TRUE ? FPL_EnabledFriendsOnly : FPL_Disabled;
				}
				else
				{
					debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY) failed with 0x%08X"),
						LocalUserNum,Result);
				}
			}
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_USER_CREATED_CONTENT) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanDownloadUserContent()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Determines whether the player is allowed to view other people's player profile
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanViewPlayerProfiles(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		Result = XUserCheckPrivilege(LocalUserNum,XPRIVILEGE_PROFILE_VIEWING,
			&bCan);
		if (Result == ERROR_SUCCESS)
		{
			if (bCan == TRUE)
			{
				// Universally ok
				Priv = FPL_Enabled;
			}
			else
			{
				// Not valid for everyone so check for friends only
				Result = XUserCheckPrivilege(LocalUserNum,
					XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY,&bCan);
				if (Result == ERROR_SUCCESS)
				{
					// Can only do this with friends or not at all
					Priv = bCan == TRUE ? FPL_EnabledFriendsOnly : FPL_Disabled;
				}
				else
				{
					debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY) failed with 0x%08X"),
						LocalUserNum,Result);
				}
			}
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_PROFILE_VIEWING) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanViewPlayerProfiles()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Determines whether the player is allowed to have their online presence
 * information shown to remote clients
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanShowPresenceInformation(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		Result = XUserCheckPrivilege(LocalUserNum,XPRIVILEGE_PRESENCE,
			&bCan);
		if (Result == ERROR_SUCCESS)
		{
			if (bCan == TRUE)
			{
				// Universally ok
				Priv = FPL_Enabled;
			}
			else
			{
				// Not valid for everyone so check for friends only
				Result = XUserCheckPrivilege(LocalUserNum,
					XPRIVILEGE_PRESENCE_FRIENDS_ONLY,&bCan);
				if (Result == ERROR_SUCCESS)
				{
					// Can only do this with friends or not at all
					Priv = bCan == TRUE ? FPL_EnabledFriendsOnly : FPL_Disabled;
				}
				else
				{
					debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_PRESENCE_FRIENDS_ONLY) failed with 0x%08X"),
						LocalUserNum,Result);
				}
			}
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_PRESENCE) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanShowPresenceInformation()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Determines whether the player is allowed to purchase downloadable content
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemLive::CanPurchaseContent(BYTE LocalUserNum)
{
	// Default to enabled for non-Live accounts
	EFeaturePrivilegeLevel Priv = FPL_Enabled;
	BOOL bCan;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Check for the priveledge
		DWORD Result = XUserCheckPrivilege(LocalUserNum,XPRIVILEGE_PURCHASE_CONTENT,
			&bCan);
		if (Result == ERROR_SUCCESS)
		{
			Priv = bCan == TRUE ? FPL_Enabled : FPL_Disabled;
		}
		else
		{
			debugf(TEXT("XUserCheckPrivilege(%d,XPRIVILEGE_PURCHASE_CONTENT) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to CanPurchaseContent()"),
			(DWORD)LocalUserNum);
		// Force it off because this is a bogus player
		Priv = FPL_Disabled;
	}
	return Priv;
}

/**
 * Checks that a unique player id is part of the specified user's friends list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param UniqueId the id of the player being checked
 *
 * @return TRUE if a member of their friends list, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsFriend(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(sizeof(XUID) == 8);
	BOOL bIsFriend = FALSE;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Ask Live if the local user is a friend of the specified player
		Result = XUserAreUsersFriends(LocalUserNum,(XUID*)&UniqueId,1,
			&bIsFriend,NULL);
		if (Result != ERROR_SUCCESS)
		{
			debugf(TEXT("XUserAreUsersFriends(%d,(%u,%u,%u,%u,%u,%u,%u,%u)) failed with 0x%08X"),
				LocalUserNum,UniqueId.Uid[0],UniqueId.Uid[1],UniqueId.Uid[2],UniqueId.Uid[3],
				UniqueId.Uid[4],UniqueId.Uid[5],UniqueId.Uid[6],UniqueId.Uid[7],Result);
			bIsFriend = FALSE;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to IsFriend()"),
			(DWORD)LocalUserNum);
	}
	return bIsFriend;
}

/**
 * Checks that whether a group of player ids are among the specified player's
 * friends
 *
 * @param LocalUserNum the controller number of the associated user
 * @param Query an array of players to check for being included on the friends list
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::AreAnyFriends(BYTE LocalUserNum,TArray<FFriendsQuery>& Query)
{
	check(sizeof(XUID) == 8);
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Perform the check for each query
		for (INT Index = 0; Index < Query.Num(); Index++)
		{
			BOOL bIsFriend;
			// Ask Live if the local user is a friend of the specified player
			Result = XUserAreUsersFriends(LocalUserNum,(XUID*)&Query(Index).UniqueId,1,
				&bIsFriend,NULL);
			if (Result != ERROR_SUCCESS)
			{
				Query(Index).bIsFriend = bIsFriend;
			}
			else
			{
				debugf(TEXT("XUserAreUsersFriends(%d,(%u,%u,%u,%u,%u,%u,%u,%u)) failed with 0x%08X"),
					LocalUserNum,Query(Index).UniqueId.Uid[0],Query(Index).UniqueId.Uid[1],
					Query(Index).UniqueId.Uid[2],Query(Index).UniqueId.Uid[3],
					Query(Index).UniqueId.Uid[4],Query(Index).UniqueId.Uid[5],
					Query(Index).UniqueId.Uid[6],Query(Index).UniqueId.Uid[7],Result);
				// Failure means no friendship
				Query(Index).bIsFriend = FALSE;
				break;
			}
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to AreAnyFriends()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Checks that a unique player id is on the specified user's mute list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param UniqueId the id of the player being checked
 *
 * @return TRUE if a member of their friends list, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsMuted(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(sizeof(XUID) == 8);
	BOOL bIsMuted = FALSE;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Ask Live if the local user has muted the specified player
		XUserMuteListQuery(LocalUserNum,(XUID&)UniqueId,&bIsMuted);
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to IsMuted()"),
			(DWORD)LocalUserNum);
	}
	return bIsMuted;
}

/**
 * Displays the Xbox Guide Friends UI
 *
 * @param LocalUserNum the controller number of the user where are showing the friends for
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowFriendsUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the friends UI for the specified controller num
		Result = XShowFriendsUI(LocalUserNum);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowFriendsUI(%d) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowFriendsUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the Xbox Guide Friends Request UI
 *
 * @param LocalUserNum the controller number of the user where are showing the friends for
 * @param UniqueId the id of the player being invited (null or 0 to have UI pick)
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowFriendsInviteUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(sizeof(XUID) == 8);
	// Figure out whether to use a specific XUID or not
	XUID RequestedId = (XUID&)UniqueId;
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the friends UI for the specified controller num and player
		Result = XShowFriendRequestUI(LocalUserNum,RequestedId);
		if (Result != ERROR_SUCCESS)
		{
			BYTE* Xuid = (BYTE*)&RequestedId;
			debugf(NAME_Error,TEXT("XShowFriendsRequestUI(%d,(%u,%u,%u,%u,%u,%u,%u,%u)) failed with 0x%08X"),
				LocalUserNum,Xuid[0],Xuid[1],Xuid[2],Xuid[3],
				Xuid[4],Xuid[5],Xuid[6],Xuid[7],Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowFriendsInviteUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the UI that allows a player to give feedback on another player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param UniqueId the id of the player having feedback given for
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowFeedbackUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(sizeof(XUID) == 8);
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the live guide ui for player review
		Result = XShowPlayerReviewUI(LocalUserNum,(XUID&)UniqueId);
		if (Result != ERROR_SUCCESS)
		{
			BYTE* Xuid = (BYTE*)&UniqueId;
			debugf(NAME_Error,TEXT("XShowPlayerReviewUI(%d,(%u,%u,%u,%u,%u,%u,%u,%u)) failed with 0x%08X"),
				LocalUserNum,Xuid[0],Xuid[1],Xuid[2],Xuid[3],
				Xuid[4],Xuid[5],Xuid[6],Xuid[7],Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowFeedbackUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the gamer card UI for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param UniqueId the id of the player to show the gamer card of
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowGamerCardUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(sizeof(XUID) == 8);
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the live guide ui for gamer cards
		Result = XShowGamerCardUI(LocalUserNum,(XUID&)UniqueId);
		if (Result != ERROR_SUCCESS)
		{
			BYTE* Xuid = (BYTE*)&UniqueId;
			debugf(NAME_Error,TEXT("XShowGamerCardUI(%d,(%u,%u,%u,%u,%u,%u,%u,%u)) failed with 0x%08X"),
				LocalUserNum,Xuid[0],Xuid[1],Xuid[2],Xuid[3],
				Xuid[4],Xuid[5],Xuid[6],Xuid[7],Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowGamerCardUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the messages UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowMessagesUI(BYTE LocalUserNum)
{
	// Show the live guide ui for player messages
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		Result = XShowMessagesUI(LocalUserNum);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowMessagesUI(%d) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowMessagesUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the achievements UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowAchievementsUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the live guide ui for player achievements
		Result = XShowAchievementsUI(LocalUserNum);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowAchievementsUI(%d) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowAchievementsUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the achievements UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowPlayersUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the live guide ui for the player list
		Result = XShowPlayersUI(LocalUserNum);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowPlayersUI(%d) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowPlayersUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the UI that shows the keyboard for inputing text
 *
 * @param LocalUserNum the controller number of the associated user
 * @param TitleText the title to display to the user
 * @param DescriptionText the text telling the user what to input
 * @param bShouldValidate whether to apply the string validation API after input or not
 * @param DefaultText the default string to display
 * @param MaxResultLength the maximum length string expected to be filled in
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowKeyboardUI(BYTE LocalUserNum,
	const FString& TitleText,const FString& DescriptionText,
	UBOOL bShouldValidate,const FString& DefaultText,INT MaxResultLength)
{
	DWORD Return = E_FAIL;
	// Validate the user index passed in
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		DWORD Flags = VKBD_HIGHLIGHT_TEXT;
		// Determine which keyboard features based upon dash language
		switch (XGetLanguage())
		{
			case XC_LANGUAGE_JAPANESE:
			{
				Flags |= VKBD_JAPANESE_FULL;
				break;
			}
			case XC_LANGUAGE_KOREAN:
			{
				Flags |= VKBD_KOREAN_FULL;
				break;
			}
			case XC_LANGUAGE_TCHINESE:
			{
				Flags |= VKBD_TCH_FULL;
				break;
			}
			default:
			{
				Flags |= VKBD_LATIN_FULL;
				break;
			}
		}
		// Allocate an async task to hold the data while in process
		FLiveAsyncTaskDataKeyboard* AsyncData = new FLiveAsyncTaskDataKeyboard(TitleText,
			DefaultText,DescriptionText,MaxResultLength,bShouldValidate);
		FLiveAsyncTaskKeyboard* AsyncTask = new FLiveAsyncTaskKeyboard(&__OnKeyboardInputComplete__Delegate,AsyncData);
		// Show the live guide ui for inputing text
		Return = XShowKeyboardUI(LocalUserNum,
			Flags,
			AsyncData->GetDefaultText(),
			AsyncData->GetTitleText(),
			AsyncData->GetDescriptionText(),
			*AsyncData,
			MaxResultLength,
			*AsyncTask);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnKeyboardInputComplete__Delegate,&Results);
			// Don't leak the task/data
			delete AsyncTask;
			debugf(NAME_Error,TEXT("XShowKeyboardUI(%d,%d,'%s','%s','%s',data,data,data) failed with 0x%08X"),
				LocalUserNum,Flags,*DefaultText,*TitleText,*DescriptionText,Return);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowKeyboardUI()"),
			(DWORD)LocalUserNum);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Determines if the ethernet link is connected or not
 */
UBOOL UOnlineSubsystemLive::HasLinkConnection(void)
{
	return (XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE) != 0;
}

/**
 * Sets a new position for the network notification icons/images
 *
 * @param NewPos the new location to use
 */
void UOnlineSubsystemLive::SetNetworkNotificationPosition(BYTE NewPos)
{
	CurrentNotificationPosition = NewPos;
	// Map our enum to Live's
	switch (CurrentNotificationPosition)
	{
		case NNP_TopLeft:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_TOPLEFT);
			break;
		}
		case NNP_TopCenter:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_TOPCENTER);
			break;
		}
		case NNP_TopRight:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_TOPRIGHT);
			break;
		}
		case NNP_CenterLeft:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_CENTERLEFT);
			break;
		}
		case NNP_Center:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_CENTER);
			break;
		}
		case NNP_CenterRight:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_CENTERRIGHT);
			break;
		}
		case NNP_BottomLeft:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_BOTTOMLEFT);
			break;
		}
		case NNP_BottomCenter:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_BOTTOMCENTER);
			break;
		}
		case NNP_BottomRight:
		{
			XNotifyPositionUI(XNOTIFYUI_POS_BOTTOMRIGHT);
			break;
		}
	}
}

/**
 * Determines if the specified controller is connected or not
 *
 * @param ControllerId the controller to query
 *
 * @return true if connected, false otherwise
 */
UBOOL UOnlineSubsystemLive::IsControllerConnected(INT ControllerId)
{
	return (LastInputDeviceConnectedMask & (1 << ControllerId)) ? TRUE : FALSE;
}

/**
 * Determines the NAT type the player is using
 */
BYTE UOnlineSubsystemLive::GetNATType(void)
{
	ENATType NatType = NAT_Unknown;
	// Ask the system for it
	switch (XOnlineGetNatType())
	{
		case XONLINE_NAT_OPEN:
			NatType = NAT_Open;
			break;

		case XONLINE_NAT_MODERATE:
			NatType = NAT_Moderate;
			break;

		case XONLINE_NAT_STRICT:
			NatType = NAT_Strict;
			break;
	}
	return NatType;
}

/**
 * Creates the session flags value from the game settings object. First looks
 * for the standard settings and then checks for Live specific settings
 *
 * @param InSettings the game settings of the new session
 *
 * @return the flags needed to set up the session
 */
DWORD UOnlineSubsystemLive::BuildSessionFlags(UOnlineGameSettings* InSettings)
{
	DWORD Flags = 0;
	// Base setting is that we are using the peer network (secure with
	// parental controls)
	Flags |= XSESSION_CREATE_USES_PEER_NETWORK;
	// We are a public host only if lan only wasn't chosen
	if (InSettings->bIsLanMatch == FALSE)
	{
		Flags |= XSESSION_CREATE_HOST;
	}
	// Whether to advertise the server or not
	if (InSettings->bShouldAdvertise == TRUE)
	{
		Flags |= XSESSION_CREATE_USES_MATCHMAKING;
	}
	// Whether to use stats or not
	if (InSettings->bUsesStats == TRUE)
	{
		Flags |= XSESSION_CREATE_USES_STATS;
	}
	// Whether to allow join in progress or not
	if (InSettings->bAllowJoinInProgress == FALSE)
	{
		Flags |= XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;
	}
	// Whether to allow invites or not
	if (InSettings->bAllowInvites == FALSE)
	{
		Flags |= XSESSION_CREATE_INVITES_DISABLED;
	}
	// Whether to require arbitration or not
	if (InSettings->bUsesArbitration == TRUE)
	{
		Flags |= XSESSION_CREATE_USES_ARBITRATION;
	}
	// Whether the session uses presence or not
	if (InSettings->bUsesPresence == TRUE)
	{
		Flags |= XSESSION_CREATE_USES_PRESENCE;
	}
	// Whether to allow join via presence information or not
	if (InSettings->bAllowJoinViaPresence == FALSE)
	{
		Flags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
	}
	return Flags;
}

/**
 * Sets the list contexts for the player
 *
 * @param PlayerNum the index of the player hosting the match
 * @param Contexts the list of contexts to set
 */
void UOnlineSubsystemLive::SetContexts(BYTE PlayerNum,const TArray<FLocalizedStringSetting>& Contexts)
{
	// Iterate through all contexts and set them
	for (INT Index = 0; Index < Contexts.Num(); Index++)
	{
		const FLocalizedStringSetting& Context = Contexts(Index);
		// Set the context data
		DWORD Result = XUserSetContextEx(PlayerNum,Context.Id,Context.ValueIndex,NULL);
		// Log it for debug purposes
		debugf(NAME_DevLive,TEXT("XUserSetContextEx(%d,0x%08X,%d,NULL) returned 0x%08X"),
			PlayerNum,Context.Id,Context.ValueIndex,Result);
	}
}

/**
 * Sets the list properties for the player
 *
 * @param PlayerNum the index of the player hosting the match
 * @param Properties the list of properties to set
 */
void UOnlineSubsystemLive::SetProperties(BYTE PlayerNum,const TArray<FSettingsProperty>& Properties)
{
	// Iterate through all properties and set those too
	for (INT Index = 0; Index < Properties.Num(); Index++)
	{
		const FSettingsProperty& Property = Properties(Index);
		// Get the size of data that we'll be sending
		DWORD SizeOfData = GetSettingsDataSize(Property.Data);
		// Get the pointer to the data we are sending
		const void* DataPointer = GetSettingsDataPointer(Property.Data);
		// Set the context data
		DWORD Result = XUserSetPropertyEx(PlayerNum,Property.PropertyId,SizeOfData,DataPointer,NULL);
#if !FINAL_RELEASE
		// Log it for debug purposes
		FString StringVal = Property.Data.ToString();
		debugf(NAME_DevLive,TEXT("XUserSetPropertyEx(%d,0x%08X,%d,%s,NULL) returned 0x%08X"),
			PlayerNum,Property.PropertyId,SizeOfData,*StringVal,Result);
#endif
	}
}

/**
 * Sets the contexts and properties for this game settings object
 *
 * @param PlayerNum the index of the player performing the search/hosting the match
 * @param GameSettings the game settings of the new session
 *
 * @return TRUE if successful, FALSE otherwise
 */
void UOnlineSubsystemLive::SetContextsAndProperties(BYTE PlayerNum,
	UOnlineGameSettings* InSettings)
{
	DWORD GameType = X_CONTEXT_GAME_TYPE_STANDARD;
	// Add arbitration flag if requested
	if (InSettings->bUsesArbitration == TRUE)
	{
		GameType = X_CONTEXT_GAME_TYPE_RANKED;
	}
	// Set the game type (standard or ranked)
	DWORD Result = XUserSetContextEx(PlayerNum,X_CONTEXT_GAME_TYPE,GameType,NULL);
	debugf(NAME_DevLive,TEXT("XUserSetContextEx(%d,X_CONTEXT_GAME_TYPE,%d,NULL) returned 0x%08X"),
		PlayerNum,GameType,Result);
	// Use the common methods for setting the lists of contexts & properties
	SetContexts(PlayerNum,InSettings->LocalizedSettings);
	SetProperties(PlayerNum,InSettings->Properties);
}

/**
 * Creates an online game based upon the settings object specified.
 *
 * @param HostingPlayerNum the index of the player hosting the match
 * @param GameSettings the settings to use for the new game session
 *
 * @return true if successful creating the session, false otherwsie
 */
UBOOL UOnlineSubsystemLive::CreateOnlineGame(BYTE HostingPlayerNum,
	UOnlineGameSettings* NewGameSettings)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't set if we already have a session going
	if (GameSettings == NULL)
	{
		GameSettings = NewGameSettings;
		if (GameSettings != NULL)
		{
			check(SessionInfo == NULL);
			SessionInfo = new FSecureSessionInfo();
			// Init the game settings counts so the host can use them later
			GameSettings->NumOpenPrivateConnections = GameSettings->NumPrivateConnections;
			GameSettings->NumOpenPublicConnections = GameSettings->NumPublicConnections;
			// Read the XUID of the owning player for gamertag and gamercard support
			XUserGetXUID(HostingPlayerNum,(XUID*)&GameSettings->OwningPlayerId);
			ANSICHAR Buffer[32];
			// Read the name of the owning player
			XUserGetName(HostingPlayerNum,Buffer,sizeof(Buffer));
			GameSettings->OwningPlayerName = Buffer;
			// Determine if we are registering a Live session or system link
			if (GameSettings->bIsLanMatch == FALSE)
			{
				// Register via Live
				Return = CreateLiveGame(HostingPlayerNum);
			}
			else
			{
				// System link only
				Return = CreateLanGame(HostingPlayerNum);
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't create an online session with null game settings"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't create a new online session when one is in progress"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Tells the QoS thread to start its listening process. Builds the packet
 * of custom data to send back to clients in the query.
 *
 * @return The success/error code of the operation
 */
DWORD UOnlineSubsystemLive::RegisterQoS(void)
{
	check(GameSettings && SessionInfo);
	// Copy over the Nonce for replicating to clients
	(ULONGLONG&)GameSettings->ServerNonce = SessionInfo->Nonce;
	// Build our custom QoS packet
	FSystemLinkPacketWriter Packet;
//@todo joe -- Add support for game specific additions to the QoS info
	Packet << GameSettings->OwningPlayerId;
	Packet << GameSettings->OwningPlayerName;
	Packet << (QWORD&)GameSettings->ServerNonce;
	// Determine the size of the packet data
	DWORD QoSPacketLen = Packet.GetByteCount();
	// Copy the data into our persistent buffer since QoS will access it async
	appMemcpy(QoSPacket,(BYTE*)Packet,QoSPacketLen);
	// Register with the QoS listener
	DWORD Return = XNetQosListen(&SessionInfo->XSessionInfo.sessionID,
		QoSPacket,
		QoSPacketLen,
		// Uses the default (16kbps)
		0,
		XNET_QOS_LISTEN_ENABLE | XNET_QOS_LISTEN_SET_DATA);
	debugf(NAME_DevLive,
		TEXT("XNetQosListen(Key,Data,%d,0,XNET_QOS_LISTEN_ENABLE | XNET_QOS_LISTEN_SET_DATA) returned 0x%08X"),
		QoSPacketLen,Return);
	return Return;
}

/**
 * Kicks off the list of returned servers' QoS queries
 *
 * @param AsyncData the object that holds the async QoS data
 *
 * @return TRUE if the call worked and the results should be polled for,
 *		   FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::CheckServersQoS(FLiveAsyncTaskDataSearch* AsyncData)
{
	UBOOL bOk = FALSE;
	if (GameSearch != NULL)
	{
		check(AsyncData);
		// Figure out how many servers we need to ping
		DWORD NumServers = (DWORD)GameSearch->Results.Num();
		if (NumServers > 0)
		{
			// The QoS arrays are hardcoded to 50
			check(NumServers < 50);
			// Get the pointers to the arrays used in the QoS calls
			XNADDR** ServerAddrs = AsyncData->GetXNADDRs();
			XNKID** ServerKids = AsyncData->GetXNKIDs();
			XNKEY** ServerKeys = AsyncData->GetXNKEYs();
			// Loop through the results and build the arrays needed for our queries
			for (INT Index = 0; Index < GameSearch->Results.Num(); Index++)
			{
				const FOnlineGameSearchResult& Game = GameSearch->Results(Index);
				const XSESSION_INFO* XSessInfo = (XSESSION_INFO*)Game.PlatformData;
				// Copy the addr, id, and key
				ServerAddrs[Index] = (XNADDR*)&XSessInfo->hostAddress;
				ServerKids[Index] = (XNKID*)&XSessInfo->sessionID;
				ServerKeys[Index] = (XNKEY*)&XSessInfo->keyExchangeKey;
			}
			// Kick off the QoS set of queries
			DWORD Return = XNetQosLookup(NumServers,
				(const XNADDR**)ServerAddrs,
				(const XNKID**)ServerKids,
				(const XNKEY**)ServerKeys,
				// We skip all gateway services
				0,0,0,
				// 8 probes are recommended
				8,
				64 * 1024,
				// Flags are unsupported and we'll poll
				0,NULL,
				// The out parameter that holds the data
				AsyncData->GetXNQOS());
			debugf(NAME_DevLive,
				TEXT("XNetQosLookup(%d,Addrs,Kids,Keys,0,0,0,8,64K,0,NULL,Data) returned 0x%08X"),
				Return);
			bOk = Return == ERROR_SUCCESS;
		}
	}
	return bOk;
}

/**
 * Parses the results from the QoS queries and places those results in the
 * corresponding search results info
 *
 * @param QosData the data to parse the results of
 */
void UOnlineSubsystemLive::ParseQoSResults(XNQOS* QosData)
{
	check(QosData);
	check(GameSearch);
	// If these don't match, we don't know who the data belongs to
	if (GameSearch->Results.Num() == QosData->cxnqos)
	{
		// Iterate through the results
		for (DWORD Index = 0; Index < QosData->cxnqos; Index++)
		{
			// Get the game settings object to add data to
			UOnlineGameSettings* ServerSettings = GameSearch->Results(Index).GameSettings;
			// Read the custom data if present
			if (QosData->axnqosinfo[Index].cbData > 0 &&
				QosData->axnqosinfo[Index].pbData != NULL)
			{
				// Create a packet reader to read the data out
				FSystemLinkPacketReader Packet(QosData->axnqosinfo[Index].pbData,
					QosData->axnqosinfo[Index].cbData);
				// Read the gamertag and XUID
				Packet >> ServerSettings->OwningPlayerId;
				Packet >> ServerSettings->OwningPlayerName;
				Packet >> (QWORD&)ServerSettings->ServerNonce;
			}
			// Set the ping that the QoS estimated
			ServerSettings->PingInMs = QosData->axnqosinfo[Index].wRttMedInMsecs;
			debugf(NAME_DevLive,TEXT("QoS for %s is %d"),
				*ServerSettings->OwningPlayerName,ServerSettings->PingInMs);
		}
		// Make a second pass through the search results and pull out any
		// that had partial QoS data. This can't be done during QoS parsing
		// since the indices need to match then
		for (INT Index = 0; Index < GameSearch->Results.Num(); Index++)
		{
			FOnlineGameSearchResult& SearchResult = GameSearch->Results(Index);
			// If any of the fields are missing, remove this item from the list
			if ((ULONGLONG&)SearchResult.GameSettings->ServerNonce == 0 ||
				SearchResult.GameSettings->OwningPlayerName.Len() == 0 ||
				(XUID&)SearchResult.GameSettings->OwningPlayerId == 0)
			{
				debugf(NAME_DevLive,TEXT("Removing server with malformed QoS data at index %d"),Index);
				// Free the data
				delete (XSESSION_INFO*)SearchResult.PlatformData;
				// And then remove from the list
				GameSearch->Results.Remove(Index);
				Index--;
			}
		}
	}
	else
	{
		debugf(NAME_Warning,TEXT("QoS data for servers doesn't match up, skipping"));
	}
}

/**
 * Creates a new Live enabled game for the requesting player using the
 * settings specified in the game settings object
 *
 * @param HostingPlayerNum the player hosting the game
 *
 * @return The result from the Live APIs
 */
DWORD UOnlineSubsystemLive::CreateLiveGame(BYTE HostingPlayerNum)
{
	check(GameSettings && SessionInfo);
	debugf(NAME_DevLive,TEXT("Creating a Live match"));
	// For each local player, force them to use the same props/contexts
	for (DWORD Index = 0; Index < 4; Index++)
	{
		// Ignore non-Live enabled profiles
		if (XUserGetSigninState(Index) == eXUserSigninState_SignedInToLive)
		{
			// Register all of the context/property information for the session
			SetContextsAndProperties(Index,GameSettings);
		}
	}
	// Get the flags for the session
	DWORD Flags = BuildSessionFlags(GameSettings);
	// Create a new async task for handling the creation
	FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskCreateSession(&__OnCreateOnlineGameComplete__Delegate);
	// Now create the session
	DWORD Return = XSessionCreate(Flags,
		HostingPlayerNum,
		GameSettings->NumPublicConnections,
		GameSettings->NumPrivateConnections,
		&SessionInfo->Nonce,
		&SessionInfo->XSessionInfo,
		*AsyncTask,
		&SessionInfo->Handle);
	debugf(NAME_DevLive,TEXT("XSessionCreate(%d,%d,%d,%d,Nonce,SessInfo,Data,OutHandle) returned 0x%08X"),
		Flags,(DWORD)HostingPlayerNum,GameSettings->NumPublicConnections,
		GameSettings->NumPrivateConnections,Return);
	if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
	{
		// Add the task for tracking since the call worked
		AsyncTasks.AddItem(AsyncTask);
	}
	else
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&__OnCreateOnlineGameComplete__Delegate,&Results);
		// Clean up the session info so we don't get into a confused state
		delete SessionInfo;
		SessionInfo = NULL;
		// Don't leak the task in this case
		delete AsyncTask;
		GameSettings = NULL;
	}
	return Return;
}

/**
 * Creates a new system link enabled game. Registers the keys/nonce needed
 * for secure communication
 *
 * @param HostingPlayerNum the player hosting the game
 *
 * @return The result code from the nonce/key APIs
 */
DWORD UOnlineSubsystemLive::CreateLanGame(BYTE HostingPlayerNum)
{
	check(SessionInfo);
	DWORD Return = ERROR_SUCCESS;
	// Don't create a system link beacon if advertising is off
	if (GameSettings->bShouldAdvertise == TRUE)
	{
		// Get our address
		while (XNetGetTitleXnAddr(&SessionInfo->XSessionInfo.hostAddress) == XNET_GET_XNADDR_PENDING)
		{
			appSleep(0.f);
		}
		// Fill the key/key id pair for the session
		Return = XNetCreateKey(&SessionInfo->XSessionInfo.sessionID,
			&SessionInfo->XSessionInfo.keyExchangeKey);
		if (Return == ERROR_SUCCESS)
		{
			// Register this key/id pair so they can be used
			Return = XNetRegisterKey(&SessionInfo->XSessionInfo.sessionID,
				&SessionInfo->XSessionInfo.keyExchangeKey);
			if (Return == ERROR_SUCCESS)
			{
				// Bind a socket for system link activity
				SysLinkSocket = new FSystemLinkBeacon();
				if (SysLinkSocket->Init(SystemLinkAnnouncePort))
				{
					// We successfully created everything so mark the socket as
					// needing polling
					SystemLinkState = SLS_Hosting;
					debugf(NAME_DevLive,TEXT("Listening for beacon requestes on %d"),
						SystemLinkAnnouncePort);
				}
				else
				{
					debugf(NAME_Error,TEXT("Failed to init to system link beacon %d"),
						GSocketSubsystem->GetSocketError());
					Return = XNET_CONNECT_STATUS_LOST;
				}
			}
			else
			{
				debugf(NAME_Error,TEXT("Failed to register key for secure connection 0x%08X"),Return);
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to create key for secure connection 0x%08X"),Return);
		}
	}
	if (Return == ERROR_SUCCESS)
	{
		// Register all local talkers
		RegisterLocalTalkers();
		CurrentGameState = OGS_Pending;
	}
	else
	{
		// Clean up the session info so we don't get into a confused state
		delete SessionInfo;
		SessionInfo = NULL;
		GameSettings = NULL;
	}
	FAsyncTaskDelegateResults Results(Return);
	ProcessDelegate(NAME_None,&__OnCreateOnlineGameComplete__Delegate,&Results);
	return Return;
}

/**
 * Destroys the current online game
 *
 * @return true if successful destroying the session, false otherwsie
 */
UBOOL UOnlineSubsystemLive::DestroyOnlineGame(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't shut down if it isn't valid
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Stop all local talkers (avoids a debug runtime warning)
		UnregisterLocalTalkers();
		// Stop all remote voice before ending the session
		RemoveAllRemoteTalkers();
		// Determine if this is a lan match or Live
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Unregister our QoS packet data and stop hanlding the queries
			UnregisterQoS();
			Return = DestroyLiveGame();
		}
		else
		{
			Return = DestroyLanGame();
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't destroy a null online session"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Terminates a Live session
 *
 * @return The result from the Live APIs
 */
DWORD UOnlineSubsystemLive::DestroyLiveGame(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Create a new async task for handling the deletion
	FLiveAsyncTask* AsyncTask = new FLiveAsyncDestroySession(SessionInfo->Handle,&__OnDestroyOnlineGameComplete__Delegate);
	// Shutdown the session asynchronously
	Return = XSessionDelete(SessionInfo->Handle,*AsyncTask);
	debugf(NAME_DevLive,TEXT("XSessionDelete() returned 0x%08X"),Return);
	if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
	{
		// Add the task to the list to be ticked later
		AsyncTasks.AddItem(AsyncTask);
	}
	else
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&__OnDestroyOnlineGameComplete__Delegate,&Results);
		// Don't leak the task
		delete AsyncTask;
		// Manually clean up
		CloseHandle(SessionInfo->Handle);
	}
	// The session info is no longer needed
	delete SessionInfo;
	SessionInfo = NULL;
	// Null out the no longer valid game settings
	GameSettings = NULL;
	return Return;
}

/**
 * Terminates a LAN session. Unregisters the key for the secure connection
 *
 * @return an error/success code
 */
DWORD UOnlineSubsystemLive::DestroyLanGame(void)
{
	check(SessionInfo);
	DWORD Return = ERROR_SUCCESS;
	// Only tear down the beacon if it was advertising
	if (GameSettings->bShouldAdvertise)
	{
		// Tear down the system link beacon
		StopSystemLinkBeacon();
		// Unregister this key/id pair to prevent leaks
		Return = XNetUnregisterKey(&SessionInfo->XSessionInfo.sessionID);
		if (Return != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("Failed to unregister key for secure connection 0x%08X"),Return);
		}
	}
	// Clean up before firing the delegate
	delete SessionInfo;
	SessionInfo = NULL;
	// Null out the no longer valid game settings
	GameSettings = NULL;
	// Fire the delegate off immediately
	FAsyncTaskDelegateResults Results(Return);
	ProcessDelegate(NAME_None,&__OnDestroyOnlineGameComplete__Delegate,&Results);
	return Return;
}

/**
 * Allocates the space/structure needed for holding the search results plus
 * any resources needed for async support
 *
 * @param SearchingPlayerNum the index of the player searching for the match
 * @param QueryNum the unique id of the query to be run
 * @param MaxSearchResults the maximum number of search results we want
 * @param NumBytes the out param indicating the size that was allocated
 *
 * @return The data allocated for the search (space plus overlapped)
 */
FLiveAsyncTaskDataSearch* UOnlineSubsystemLive::AllocateSearch(BYTE SearchingPlayerNum,
	DWORD QueryNum,DWORD MaxSearchResults,DWORD& NumBytes)
{
	// Use the search code to determine the size buffer we need
    DWORD Return = XSessionSearch(QueryNum,SearchingPlayerNum,MaxSearchResults,
        0,0,NULL,NULL,&NumBytes,NULL,NULL);
	FLiveAsyncTaskDataSearch* Data = NULL;
	// Only allocate the buffer if the call worked ok
	if (Return == ERROR_INSUFFICIENT_BUFFER && NumBytes > 0)
	{
		Data = new FLiveAsyncTaskDataSearch(NumBytes);
	}
	return Data;
}

/**
 * Builds a Live/system link search query and sends it off to be processed. Uses
 * the search settings passed in to populate the query.
 *
 * @param SearchingPlayerNum the index of the player searching for the match
 * @param SearchSettings the game settings that we are interested in
 *
 * @return TRUE if the search was started successfully, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::FindOnlineGames(BYTE SearchingPlayerNum,
	UOnlineGameSearch* SearchSettings)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Verify that we have valid search settings
	if (SearchSettings != NULL)
	{
		// Don't start another while in progress or multiple entries for the
		// same server will show up in the server list
		if (SearchSettings->bIsSearchInProgress == FALSE)
		{
			// Free up previous results
			FreeSearchResults();
			// Check for Live or Systemlink
			if (SearchSettings->bIsLanQuery == FALSE)
			{
				Return = FindLiveGames(SearchingPlayerNum,SearchSettings);
			}
			else
			{
				Return = FindLanGames(SearchSettings);
			}
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Ignoring game search request while one is pending"));
			Return = ERROR_IO_PENDING;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't search with null criteria"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Copies the Epic structures into the Live equivalent
 *
 * @param DestProps the destination properties
 * @param SourceProps the source properties
 */
void UOnlineSubsystemLive::CopyPropertiesForSearch(PXUSER_PROPERTY DestProps,
	const TArray<FSettingsProperty>& SourceProps)
{
	appMemzero(DestProps,sizeof(XUSER_PROPERTY) * SourceProps.Num());
	// Loop through the properties and copy them over
	for (INT Index = 0; Index < SourceProps.Num(); Index++)
	{
		const FSettingsProperty& Prop = SourceProps(Index);
		// Copy property id and type
		DestProps[Index].dwPropertyId = Prop.PropertyId;
		DestProps[Index].value.type = Prop.Data.Type;
		// Now copy the held data (no strings or blobs as they aren't supported)
		switch (Prop.Data.Type)
		{
			case SDT_Float:
			{
				Prop.Data.GetData(DestProps[Index].value.fData);
				break;
			}
			case SDT_Int32:
			{
				Prop.Data.GetData((INT&)DestProps[Index].value.nData);
				break;
			}
			case SDT_Int64:
			{
				Prop.Data.GetData((QWORD&)DestProps[Index].value.i64Data);
				break;
			}
			case SDT_Double:
			{
				Prop.Data.GetData(DestProps[Index].value.dblData);
				break;
			}
			case SDT_Blob:
			case SDT_String:
			{
				DestProps[Index].value.type = 0;
				debugf(NAME_DevLive,
					TEXT("Ignoring property (%d) for search as blobs/strings aren't supported by Live"),
					Prop.PropertyId);
				break;
			}
			case SDT_DateTime:
			{
				DestProps[Index].value.ftData.dwLowDateTime = Prop.Data.Value1;
				DestProps[Index].value.ftData.dwHighDateTime = (DWORD)Prop.Data.Value2;
				break;
			}
		}
	}
}

/**
 * Copies the Epic structures into the Live equivalent
 *
 * @param Search the object to use when determining
 * @param DestContexts the destination contexts
 * @param SourceContexts the source contexts
 *
 * @return the number of items copied (handles skipping for wildcards)
 */
DWORD UOnlineSubsystemLive::CopyContextsForSearch(UOnlineGameSearch* Search,
	PXUSER_CONTEXT DestContexts,
	const TArray<FLocalizedStringSetting>& SourceContexts)
{
	DWORD Count = 0;
	// Iterate through the source contexts and copy any that aren't wildcards
	for (INT Index = 0; Index < SourceContexts.Num(); Index++)
	{
		const FLocalizedStringSetting& Setting = SourceContexts(Index);
		// Don't copy if the item is meant to use wildcard matching
		if (Search->IsWildcardStringSetting(Setting.Id) == FALSE)
		{
			DestContexts[Count].dwContextId = Setting.Id;
			DestContexts[Count].dwValue = Setting.ValueIndex;
			Count++;
		}
	}
	return Count;
}

/**
 * Builds a Live game query and submits it to Live for processing
 *
 * @param SearchingPlayerNum the player searching for games
 * @param SearchSettings the settings that the player is interested in
 *
 * @return The result from the Live APIs
 */
DWORD UOnlineSubsystemLive::FindLiveGames(BYTE SearchingPlayerNum,UOnlineGameSearch* SearchSettings)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Get to our Live specific settings so we can check arbitration & max results
	DWORD MaxSearchResults = Clamp(SearchSettings->MaxSearchResults,0,XSESSION_SEARCH_MAX_RETURNS);
	DWORD QueryId = SearchSettings->Query.ValueIndex;
	DWORD NumResultBytes = 0;
	// Now allocate the search data bucket
	FLiveAsyncTaskDataSearch* SearchData = AllocateSearch(SearchingPlayerNum,
		QueryId,MaxSearchResults,NumResultBytes);
	if (SearchData != NULL)
	{
		// Figure out the game type we want to search for
		DWORD GameType = X_CONTEXT_GAME_TYPE_STANDARD;
		if (SearchSettings->bUsesArbitration == TRUE)
		{
			GameType = X_CONTEXT_GAME_TYPE_RANKED;
		}
		// Append the required contexts if missing
		SearchSettings->SetStringSettingValue(X_CONTEXT_GAME_TYPE,
			GameType,TRUE);
		// Allocate space to hold the properties array
		PXUSER_PROPERTY Properties = SearchData->AllocateProperties(SearchSettings->Properties.Num());
		// Copy property data over
		CopyPropertiesForSearch(Properties,SearchSettings->Properties);
		// Allocate space to hold the contexts array
		PXUSER_CONTEXT Contexts = SearchData->AllocateContexts(SearchSettings->LocalizedSettings.Num());
		// Copy contexts data over
		DWORD NumContexts = CopyContextsForSearch(SearchSettings,Contexts,SearchSettings->LocalizedSettings);
#ifdef _DEBUG
		// Log properties and contexts
		DumpContextsAndProperties(SearchSettings);
#endif
		// Create a new async task for handling the async
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskSearch(&__OnFindOnlineGamesComplete__Delegate,
			SearchData);
		// Kick off the async search
		Return = XSessionSearch(QueryId,
			SearchingPlayerNum,
			MaxSearchResults,
			SearchSettings->Properties.Num(),
			NumContexts,
			Properties,
			Contexts,
			&NumResultBytes,
			*SearchData,
			*AsyncTask);

		debugf(NAME_DevLive,TEXT("XSessionSearch(%d,%d,%d,%d,%d,data,data,%d,data,data) returned 0x%08X"),
			QueryId,(DWORD)SearchingPlayerNum,MaxSearchResults,SearchSettings->Properties.Num(),
			SearchSettings->LocalizedSettings.Num(),NumResultBytes,Return);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Mark the search in progress
			SearchSettings->bIsSearchInProgress = TRUE;
			// Add the task to the list to be ticked later
			AsyncTasks.AddItem(AsyncTask);
			// Copy the search pointer so we can keep it around
			GameSearch = SearchSettings;
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnFindOnlineGamesComplete__Delegate,&Results);
			// Don't leak the task
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Failed to allocate space for the online game search"));
	}
	return Return;
}

/**
 * Registers a nonce for identification. Builds a LAN query to broadcast.
 *
 * @param SearchSettings the settings to search for games matching
 *
 * @return an error/success code
 */
DWORD UOnlineSubsystemLive::FindLanGames(UOnlineGameSearch* SearchSettings)
{
	// Recreate the unique identifier for this client
	DWORD Return = XNetRandom(SystemLinkNonce,8);
	// Create the system link socket if we don't already have one
	if (SysLinkSocket == NULL)
	{
		// Bind a socket for system link activity
		SysLinkSocket = new FSystemLinkBeacon();
		if (SysLinkSocket->Init(SystemLinkAnnouncePort) == FALSE)
		{
			debugf(NAME_Error,TEXT("Failed to create socket for system link announce port %d"),
				GSocketSubsystem->GetSocketError());
			Return = XNET_CONNECT_STATUS_LOST;
		}
	}
	// If we have a socket and a nonce, broadcast a discovery packet
	if (SysLinkSocket && Return == ERROR_SUCCESS)
	{
		FSystemLinkPacketWriter Packet;
		// Build the discovery packet
		Packet << 'S' << 'Q'
			// Append the nonce as 8 individual bytes
			<< SystemLinkNonce[0] << SystemLinkNonce[1] << SystemLinkNonce[2]
			<< SystemLinkNonce[3] << SystemLinkNonce[4] << SystemLinkNonce[5]
			<< SystemLinkNonce[6] << SystemLinkNonce[7];
		// Now kick off our broadcast which hosts will respond to
		if (SysLinkSocket->BroadcastPacket(Packet,Packet.GetByteCount()))
		{
#if !FINAL_RELEASE
			check(Packet.GetByteCount() == 10);
			char StrVer[11];
			appMemcpy(StrVer,Packet,Packet.GetByteCount());
			StrVer[10] = '\0';
			debugf(NAME_DevLive,TEXT("Sent query packet '%s'"),ANSI_TO_TCHAR(StrVer));
#endif
			// We need to poll for the return packets
			SystemLinkState = SLS_Searching;
			// Copy the search pointer so we can keep it around
			GameSearch = SearchSettings;
			// Set the timestamp for timing out a search
			SystemLinkQueryTimeLeft = SystemLinkQueryTimeout;
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to send discovery broadcast %d"),
				GSocketSubsystem->GetSocketError());
			Return = XNET_CONNECT_STATUS_LOST;
		}
	}
	if (Return != ERROR_SUCCESS)
	{
		delete SysLinkSocket;
		SysLinkSocket = NULL;
		SystemLinkState = SLS_NotUsingSystemLink;
	}
	return Return;
}

/**
 * Reads the contexts and properties from the Live search data and populates the
 * game settings object with them
 *
 * @param SearchResult the data that was returned from Live
 * @param GameSettings the game settings that we are setting the data on
 */
void UOnlineSubsystemLive::ParseContextsAndProperties(
	XSESSION_SEARCHRESULT& SearchResult,UOnlineGameSettings* GameSettings)
{
	// Clear any settings that were in the defualts
	GameSettings->LocalizedSettings.Empty();
	GameSettings->Properties.Empty();
	// Check the number of contexts
	if (SearchResult.cContexts > 0)
	{
		// Pre add them so memory isn't copied
		GameSettings->LocalizedSettings.AddZeroed(SearchResult.cContexts);
		// Iterate through the contexts and add them to the GameSettings
		for (INT Index = 0; Index < (INT)SearchResult.cContexts; Index++)
		{
			FLocalizedStringSetting& Context = GameSettings->LocalizedSettings(Index);
			Context.Id = SearchResult.pContexts[Index].dwContextId;
			Context.ValueIndex = SearchResult.pContexts[Index].dwValue;
		}
	}
	// And now the number of properties
	if (SearchResult.cProperties > 0)
	{
		// Pre add them so memory isn't copied
		GameSettings->Properties.AddZeroed(SearchResult.cProperties);
		// Iterate through the properties and add them to the GameSettings
		for (INT Index = 0; Index < (INT)SearchResult.cProperties; Index++)
		{
			FSettingsProperty& Property = GameSettings->Properties(Index);
			Property.PropertyId = SearchResult.pProperties[Index].dwPropertyId;
			// Copy the data over (may require allocs for strings)
			CopyXDataToSettingsData(Property.Data,SearchResult.pProperties[Index].value);
		}
	}
}

/**
 * Parses the search results into something the game play code can handle
 *
 * @param Search the Unreal search object
 * @param SearchResults the buffer filled by Live
 */
void UOnlineSubsystemLive::ParseSearchResults(UOnlineGameSearch* Search,
	PXSESSION_SEARCHRESULT_HEADER SearchResults)
{
	if (Search != NULL)
	{
		check(SearchResults != NULL);
		// Loop through the results copying the info over
		for (DWORD Index = 0; Index < SearchResults->dwSearchResults; Index++)
		{
			// Matchmaking should never return full servers, but just in case
			if (SearchResults->pResults[Index].dwOpenPrivateSlots > 0 ||
				SearchResults->pResults[Index].dwOpenPublicSlots > 0)
			{
				// Create an object that we'll copy the data to
				UOnlineGameSettings* NewServer = ConstructObject<UOnlineGameSettings>(
					Search->GameSettingsClass);
				if (NewServer != NULL)
				{
					// Add space in the search results array
					INT NewSearch = Search->Results.Add();
					FOnlineGameSearchResult& Result = Search->Results(NewSearch);
					// Whether arbitration is used or not comes from the search
					NewServer->bUsesArbitration = Search->bUsesArbitration;
					// Now copy the data
					Result.GameSettings = NewServer;
					NewServer->NumOpenPrivateConnections = SearchResults->pResults[Index].dwOpenPrivateSlots;
					NewServer->NumOpenPublicConnections = SearchResults->pResults[Index].dwOpenPublicSlots;
					// Determine the total slots for the match (used + open)
					NewServer->NumPrivateConnections = SearchResults->pResults[Index].dwOpenPrivateSlots +
						SearchResults->pResults[Index].dwFilledPrivateSlots;
					NewServer->NumPublicConnections = SearchResults->pResults[Index].dwOpenPublicSlots +
						SearchResults->pResults[Index].dwFilledPublicSlots;
					// Read the various contexts and properties from the search
					ParseContextsAndProperties(SearchResults->pResults[Index],NewServer);
					// Allocate and copy the Live specific data
					XSESSION_INFO* SessInfo = new XSESSION_INFO;
					appMemcpy(SessInfo,&SearchResults->pResults[Index].info,sizeof(XSESSION_INFO));
					// Store this in the results and mark them as needin proper clean ups
					Result.PlatformData = SessInfo;
				}
			}
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("No search object to store results on!"));
	}
}

/**
 * Cleans up the Live specific session data contained in the search results
 */
void UOnlineSubsystemLive::FreeSearchResults(void)
{
	if (GameSearch != NULL)
	{
		if (GameSearch->bIsSearchInProgress == FALSE)
		{
			// Loop through the results freeing the session info pointers
			for (INT Index = 0; Index < GameSearch->Results.Num(); Index++)
			{
				FOnlineGameSearchResult& Result = GameSearch->Results(Index);
				if (Result.PlatformData != NULL)
				{
					// Free the data and clear the leak detection flag
					delete (XSESSION_INFO*)Result.PlatformData;
				}
			}
			GameSearch->Results.Empty();
			GameSearch = NULL;
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Can't free search results while the search is in progress"));
		}
	}
}

/**
 * Joins the game specified. This creates the session, decodes the IP address,
 * then kicks off the connection process
 *
 * @param PlayerNum the index of the player searching for a match
 * @param DesiredGame the desired game to join
 *
 * @return true if successful destroying the session, false otherwsie
 */
UBOOL UOnlineSubsystemLive::JoinOnlineGame(BYTE PlayerNum,
	const FOnlineGameSearchResult& DesiredGame)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't join a session if already in one or hosting one
	if (SessionInfo == NULL)
	{
		Return = ERROR_SUCCESS;
		// Make the selected game our current game settings
		GameSettings = DesiredGame.GameSettings;
		check(SessionInfo == NULL);
		// Create an empty session and fill it based upon game type
		SessionInfo = new FSecureSessionInfo();
		// Copy the session info over
		appMemcpy(&SessionInfo->XSessionInfo,DesiredGame.PlatformData,
			sizeof(XSESSION_INFO));
		// The session info is created/filled differently depending on type
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Fill in Live specific data
			Return = JoinLiveGame(PlayerNum,DesiredGame,GameSettings->bWasFromInvite);
		}
		else
		{
			// Lan needs the keys registered for secure communication
			Return = XNetRegisterKey(&SessionInfo->XSessionInfo.sessionID,
				&SessionInfo->XSessionInfo.keyExchangeKey);
			if (Return == ERROR_SUCCESS)
			{
				// Live registers talkers in the async task, LAN matches do here
				if (GameSettings->bIsLanMatch)
				{
					// Register all local talkers for voice
					RegisterLocalTalkers();
				}
				// Set the state so that StartOnlineGame() can work
				CurrentGameState = OGS_Pending;
			}
			else
			{
				debugf(NAME_Error,TEXT("Failed to register host's security keys 0x%08X"),Return);
			}
			delegateOnJoinOnlineGameComplete(Return == ERROR_SUCCESS);
		}
		// Handle clean up in one place
		if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
		{
			// Clean up the session info so we don't get into a confused state
			delete SessionInfo;
			SessionInfo = NULL;
			GameSettings = NULL;
		}
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Joins a Live game by creating the session without hosting it
 *
 * @param PlayerNum the player joining the game
 * @param DesiredGame the full search results information for joinging a game
 * @param bIsFromInvite whether this join is from invite or search
 *
 * @return The result from the Live APIs
 */
DWORD UOnlineSubsystemLive::JoinLiveGame(BYTE PlayerNum,
	const FOnlineGameSearchResult& DesiredGame,UBOOL bIsFromInvite)
{
	debugf(NAME_DevLive,TEXT("Joining a Live match"));
	// Register all of the context/property information for the session
	SetContextsAndProperties(PlayerNum,DesiredGame.GameSettings);
	// Get the flags for the session
	DWORD Flags = BuildSessionFlags(DesiredGame.GameSettings);
	// Strip off the hosting flag if specified
	Flags &= ~XSESSION_CREATE_HOST;
	// Create a new async task for handling the creation/joining
	FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskCreateSession(&__OnJoinOnlineGameComplete__Delegate,FALSE,bIsFromInvite);
	// Now create the session so we can decode the IP address
	DWORD Return = XSessionCreate(Flags,
		PlayerNum,
		GameSettings->NumPublicConnections,
		GameSettings->NumPrivateConnections,
		&SessionInfo->Nonce,
		&SessionInfo->XSessionInfo,
		*AsyncTask,
		&SessionInfo->Handle);
	debugf(NAME_DevLive,TEXT("XSessionCreate(%d,%d,%d,%d,Nonce,SessInfo,Data,OutHandle) (join request) returned 0x%08X"),
		Flags,(DWORD)PlayerNum,GameSettings->NumPublicConnections,
		GameSettings->NumPrivateConnections,Return);
	if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
	{
		// Add the task for tracking since the call worked
		AsyncTasks.AddItem(AsyncTask);
	}
	else
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&__OnJoinOnlineGameComplete__Delegate,&Results);
		// Clean up the session info so we don't get into a confused state
		delete SessionInfo;
		SessionInfo = NULL;
		// Don't leak the task in this case
		delete AsyncTask;
		GameSettings = NULL;
	}
	return Return;
}

/**
 * Returns the platform specific connection information for joining the match.
 * Call this function from the delegate of join completion
 *
 * @param ConnectInfo the out var containing the platform specific connection information
 *
 * @return true if the call was successful, false otherwise
 */
UBOOL UOnlineSubsystemLive::GetResolvedConnectString(FString& ConnectInfo)
{
	UBOOL bOk = FALSE;
	if (SessionInfo != NULL)
	{
		in_addr Addr;
		// Try to decode the secure address so we can connect to it
		if (XNetXnAddrToInAddr(&SessionInfo->XSessionInfo.hostAddress,
			&SessionInfo->XSessionInfo.sessionID,&Addr) == 0)
		{
			FInternetIpAddr IpAddr;
			IpAddr.SetIp(Addr);
			// Copy the destination IP
			ConnectInfo = IpAddr.ToString(FALSE);
			bOk = TRUE;
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Failed to decrypt target host IP"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't decrypt a NULL session's IP"));
	}
	return bOk;
}

/**
 * Registers a player with the online service as being part of the online game
 *
 * @param NewPlayer the player to register with the online service
 * @param bWasInvited whether to use private or public slots first
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemLive::RegisterPlayer(FUniqueNetId UniquePlayerId,UBOOL bWasInvited)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't try to join a non-existant game
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Determine if this player is really remote or not
		if (IsLocalPlayer(UniquePlayerId) == FALSE)
		{
			FLiveAsyncRegisterPlayer* AsyncTask = NULL;
			// Don't register if the owning player is this player or if we are
			// playing a system link match
			if (GameSettings->bIsLanMatch == FALSE)
			{
				// Create a new async task for handling the async notification
				AsyncTask = new FLiveAsyncRegisterPlayer((XUID&)UniquePlayerId,
					bWasInvited,&__OnRegisterPlayerComplete__Delegate);
				// Kick off the async join request
				Return = XSessionJoinRemote(SessionInfo->Handle,
					1,
					AsyncTask->GetXuids(),
					AsyncTask->GetPrivateInvites(),
					*AsyncTask);
				debugf(NAME_DevLive,TEXT("XSessionJoinRemote(0x%016I64X) returned 0x%08X"),
					(XUID&)UniquePlayerId,Return);
			}
			else
			{
				Return = ERROR_SUCCESS;
			}
			// Only queue the task up if the call worked
			if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
			{
				// Register this player for voice
				RegisterRemoteTalker(UniquePlayerId);
				if (AsyncTask != NULL)
				{
					// Add the async task to be ticked
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					delegateOnRegisterPlayerComplete(TRUE);
				}
			}
			else
			{
				delegateOnRegisterPlayerComplete(FALSE);
				delete AsyncTask;
			}
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("No game present to join"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Unregisters a player with the online service as being part of the online game
 *
 * @param UniquePlayerId the player to unregister with the online service
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemLive::UnregisterPlayer(FUniqueNetId UniquePlayerId)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't try to leave a non-existant game
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		FLiveAsyncPlayer* AsyncTask = NULL;
		// Don't unregister if the owning player is this player or if we are
		// playing a system link match
		if (GameSettings->bIsLanMatch == FALSE &&
			IsEqualXUID((XUID&)UniquePlayerId,(XUID&)GameSettings->OwningPlayerId) == FALSE)
		{
			// Create a new async task for handling the async notification
			AsyncTask = new FLiveAsyncPlayer((XUID&)UniquePlayerId,
				&__OnUnregisterPlayerComplete__Delegate,TEXT("XSessionLeaveRemote()"));
			// Kick off the async join request
			Return = XSessionLeaveRemote(SessionInfo->Handle,
				1,
				AsyncTask->GetXuids(),
				*AsyncTask);
			debugf(NAME_DevLive,TEXT("XSessionLeaveRemote() returned 0x%08X"),
				Return);
		}
		else
		{
			Return = ERROR_SUCCESS;
		}
		// Only queue the task up if the call worked
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			if (AsyncTask != NULL)
			{
				// Add the async task to be ticked
				AsyncTasks.AddItem(AsyncTask);
			}
			else
			{
				delegateOnUnregisterPlayerComplete(FALSE);
			}
		}
		else
		{
			delegateOnUnregisterPlayerComplete(FALSE);
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("No game present to leave"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Reads the online profile settings for a given user from Live using an async task.
 * First, the game settings are read. If there is data in those, then the settings
 * are pulled from there. If not, the settings come from Live, followed by the
 * class defaults.
 *
 * @param LocalUserNum the user that we are reading the data for
 * @param ProfileSettings the object to copy the results to and contains the list of items to read
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemLive::ReadProfileSettings(BYTE LocalUserNum,
	UOnlineProfileSettings* ProfileSettings)
{
 	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Only read if we don't have a profile for this player
	if (ProfileCache[LocalUserNum].Profile == NULL)
	{
		if (ProfileSettings != NULL)
		{
			ProfileCache[LocalUserNum].Profile = ProfileSettings;
			ProfileSettings->AsyncState = OPAS_Read;
			// Clear the previous set of results
			ProfileSettings->ProfileSettings.Empty();
			// Make sure the version number is requested
			ProfileSettings->AppendVersionToReadIds();
			// If they are not logged in, give them all the defaults
			if (XUserGetSigninState(LocalUserNum) != eXUserSigninState_NotSignedIn)
			{
				DWORD NumIds = ProfileSettings->ProfileSettingIds.Num();
				DWORD* ProfileIds = (DWORD*)ProfileSettings->ProfileSettingIds.GetData();
				// Create the read buffer
				FLiveAsyncTaskDataReadProfileSettings* ReadData = new FLiveAsyncTaskDataReadProfileSettings(LocalUserNum,NumIds);
				// Copy the IDs for later use when inspecting the game settings blobs
				appMemcpy(ReadData->GetIds(),ProfileIds,sizeof(DWORD) * NumIds);
				// Create a new async task for handling the async notification
				FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskReadProfileSettings(
					&ProfileCache[LocalUserNum].ReadDelegate,ReadData);
				// Tell Live the size of our buffer
				DWORD SizeNeeded = ReadData->GetGameSettingsSize();
				// Start by reading the game settings fields
				Return = XUserReadProfileSettings(0,
					LocalUserNum,
					ReadData->GetGameSettingsIdsCount(),
					ReadData->GetGameSettingsIds(),
					&SizeNeeded,
					ReadData->GetGameSettingsBuffer(),
					*AsyncTask);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					// Queue the async task for ticking
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Just trigger the delegate as having failed
					FAsyncTaskDelegateResults Results(Return);
					ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].ReadDelegate,&Results);
					delete AsyncTask;
					ProfileCache[LocalUserNum].Profile = NULL;
				}
				debugf(NAME_DevLive,TEXT("XUserReadProfileSettings(0,%d,3,GameSettingsIds,%d,data,data) returned 0x%08X"),
					LocalUserNum,SizeNeeded,Return);
			}
			else
			{
				debugf(NAME_DevLive,TEXT("User (%d) not logged in, using defaults"),
					(DWORD)LocalUserNum);
				// Use the defaults for this player
				ProfileSettings->SetToDefaults();
				ProfileSettings->AsyncState = OPAS_None;
				// Just trigger the delegate as having failed
				FAsyncTaskDelegateResults Results(ERROR_SUCCESS);
				ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].ReadDelegate,&Results);
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't specify a null profile settings object"));
		}
	}
	// Make sure the profile isn't already being read, since this is going to
	// complete immediately
	else if (ProfileCache[LocalUserNum].Profile->AsyncState != OPAS_Read)
	{
		debugf(NAME_DevLive,TEXT("Using cached profile data instead of reading"));
		// If the specified read isn't the same as the cached object, copy the
		// data from the cache
		if (ProfileCache[LocalUserNum].Profile != ProfileSettings)
		{
			ProfileSettings->ProfileSettings = ProfileCache[LocalUserNum].Profile->ProfileSettings;
		}
		// Just trigger the read delegate as being done
		FAsyncTaskDelegateResults Results(ERROR_SUCCESS);
		// Send the notification of completion
		ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].ReadDelegate,&Results);
		Return = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_Error,TEXT("Profile read for player (%d) is already in progress"),LocalUserNum);
		// Just trigger the read delegate as failed
		FAsyncTaskDelegateResults Results(XONLINE_E_SESSION_WRONG_STATE);
		ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].ReadDelegate,&Results);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Parses the read profile results into something the game play code can handle
 *
 * @param PlayerNum the number of the user being processed
 * @param ReadResults the buffer filled by Live
 */
void UOnlineSubsystemLive::ParseReadProfileResults(BYTE PlayerNum,PXUSER_READ_PROFILE_SETTING_RESULT ReadResults)
{
	check(PlayerNum >=0 && PlayerNum < 4);
	UOnlineProfileSettings* ProfileRead = ProfileCache[PlayerNum].Profile;
	if (ProfileRead != NULL)
	{
		// Make sure the profile settings have a version number
		ProfileRead->SetDefaultVersionNumber();
		check(ReadResults != NULL);
		// Loop through the results copying the info over
		for (DWORD Index = 0; Index < ReadResults->dwSettingsLen; Index++)
		{
			XUSER_PROFILE_SETTING& LiveSetting = ReadResults->pSettings[Index];
			// Convert to our property id
			INT PropId = ConvertFromLiveValue(LiveSetting.dwSettingId);
			INT UpdateIndex = INDEX_NONE;
			// Search the settings for the property so we can replace if needed
			for (INT FindIndex = 0; FindIndex < ProfileRead->ProfileSettings.Num(); FindIndex++)
			{
				if (ProfileRead->ProfileSettings(FindIndex).ProfileSetting.PropertyId == PropId)
				{
					UpdateIndex = FindIndex;
					break;
				}
			}
			// Add if not already in the settings
			if (UpdateIndex == INDEX_NONE)
			{
				UpdateIndex = ProfileRead->ProfileSettings.AddZeroed();
			}
			// Now update the setting
			FOnlineProfileSetting& Setting = ProfileRead->ProfileSettings(UpdateIndex);
			// Copy the source and id is set to game since you can't write to Live ones
			Setting.Owner = OPPO_Game;
			Setting.ProfileSetting.PropertyId = PropId;
			// Don't bother copying data for no value settings
			if (LiveSetting.source != XSOURCE_NO_VALUE)
			{
				// Use the helper to copy the data over
				CopyXDataToSettingsData(Setting.ProfileSetting.Data,LiveSetting.data);
			}
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Skipping read profile results parsing due to no read object"));
	}
}

/**
 * Copies Unreal data to Live structures for the Live property writes
 *
 * @param Profile the profile object to copy the data from
 * @param LiveData the Live data structures to copy the data to
 */
void UOnlineSubsystemLive::CopyLiveProfileSettings(UOnlineProfileSettings* Profile,
	PXUSER_PROFILE_SETTING LiveData)
{
	check(Profile && LiveData);
	// Make a copy of the data for each setting
	for (INT Index = 0; Index < Profile->ProfileSettings.Num(); Index++)
	{
		FOnlineProfileSetting& Setting = Profile->ProfileSettings(Index);
		// Copy the data
		LiveData[Index].dwSettingId = ConvertToLiveValue((EProfileSettingID)Setting.ProfileSetting.PropertyId);
		LiveData[Index].source = (XUSER_PROFILE_SOURCE)Setting.Owner;
		// Shallow copy requires the data to live throughout the duration of the call
		LiveData[Index].data.type = Setting.ProfileSetting.Data.Type;
		LiveData[Index].data.binary.cbData = Setting.ProfileSetting.Data.Value1;
		LiveData[Index].data.binary.pbData = (BYTE*)Setting.ProfileSetting.Data.Value2;
	}
}

/**
 * Determines whether the specified settings should come from the game
 * default settings. If so, the defaults are copied into the players
 * profile results and removed from the settings list
 *
 * @param PlayerNum the id of the player
 * @param SettingsIds the set of ids to filter against the game defaults
 */
void UOnlineSubsystemLive::ProcessProfileDefaults(BYTE PlayerNum,TArray<DWORD>& SettingsIds)
{
	check(PlayerNum >=0 && PlayerNum < 4);
	check(ProfileCache[PlayerNum].Profile);
	// Copy the current settings so that setting the defaults doesn't clobber them
	TArray<FOnlineProfileSetting> Copy = ProfileCache[PlayerNum].Profile->ProfileSettings; 
	// Tell the profile to replace it's defaults
	ProfileCache[PlayerNum].Profile->SetToDefaults();
	TArray<FOnlineProfileSetting>& Settings = ProfileCache[PlayerNum].Profile->ProfileSettings;
	// Now reapply the copied settings
	for (INT Index = 0; Index < Copy.Num(); Index++)
	{
		UBOOL bFound = FALSE;
		const FOnlineProfileSetting& CopiedSetting = Copy(Index);
		// Search the profile settings and replace the setting with copied one
		for (INT Index2 = 0; Index2 < Settings.Num(); Index2++)
		{
			if (Settings(Index2).ProfileSetting.PropertyId == CopiedSetting.ProfileSetting.PropertyId)
			{
				Settings(Index2) = CopiedSetting;
				bFound = TRUE;
				break;
			}
		}
		// Add if it wasn't in the defaults
		if (bFound == FALSE)
		{
			Settings.AddItem(CopiedSetting);
		}
	}
	// Now remove the IDs that the defaults set from the missing list
	for (INT Index = 0; Index < Settings.Num(); Index++)
	{
		INT FoundIdIndex = INDEX_NONE;
		// Search and remove if found because it isn't missing then
		if (SettingsIds.FindItem(Settings(Index).ProfileSetting.PropertyId,FoundIdIndex) &&
			FoundIdIndex != INDEX_NONE &&
			Settings(Index).Owner != OPPO_OnlineService)
		{
			SettingsIds.Remove(FoundIdIndex);
		}
	}
}

/**
 * Adds one setting to the users profile results
 *
 * @param Profile the profile object to copy the data from
 * @param LiveData the Live data structures to copy the data to
 */
void UOnlineSubsystemLive::AppendProfileSetting(BYTE PlayerNum,const FOnlineProfileSetting& Setting)
{
	check(PlayerNum >=0 && PlayerNum < 4);
	check(ProfileCache[PlayerNum].Profile);
	INT AddIndex = ProfileCache[PlayerNum].Profile->ProfileSettings.AddZeroed();
	// Deep copy the data
	ProfileCache[PlayerNum].Profile->ProfileSettings(AddIndex) = Setting;
}

/**
 * Writes the online profile settings for a given user Live using an async task
 *
 * @param LocalUserNum the user that we are writing the data for
 * @param ProfileSettings the list of settings to write out
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemLive::WriteProfileSettings(BYTE LocalUserNum,
	UOnlineProfileSettings* ProfileSettings)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Don't allow a write if there is a task already in progress
	if (ProfileCache[LocalUserNum].Profile == NULL ||
		ProfileCache[LocalUserNum].Profile->AsyncState == OPAS_None)
	{
		if (ProfileSettings != NULL)
		{
			// Mark this as a write in progress
			ProfileSettings->AsyncState = OPAS_Write;
			// Make sure the profile settings have a version number
			ProfileSettings->AppendVersionToSettings();
			// Cache to make sure GC doesn't collect this while we are waiting
			// for the Live task to complete
			ProfileCache[LocalUserNum].Profile = ProfileSettings;
			// Skip the write if the user isn't signed in
			if (XUserGetSigninState(LocalUserNum) != eXUserSigninState_NotSignedIn)
			{
				// Used to write the profile settings into a blob
				FProfileSettingsWriter Writer;
				INT NumSettings = ProfileSettings->ProfileSettings.Num();
				Writer << NumSettings;
				// Write all of the profile data into the buffer
				for (INT Index = 0; Index < NumSettings; Index++)
				{
					Writer << ProfileSettings->ProfileSettings(Index);
				}
				// Create the write buffer
				FLiveAsyncTaskDataWriteProfileSettings* WriteData =
					new FLiveAsyncTaskDataWriteProfileSettings(LocalUserNum,Writer,Writer.GetByteCount());
				// Create a new async task to hold the data during the lifetime of
				// the call. It will be freed once the call is complete.
				FLiveAsyncTaskWriteProfileSettings* AsyncTask = new FLiveAsyncTaskWriteProfileSettings(
					&ProfileCache[LocalUserNum].WriteDelegate,WriteData);
				// Call a second time to fill in the data
				Return = XUserWriteProfileSettings(LocalUserNum,
					3,
					*WriteData,
					*AsyncTask);
				debugf(NAME_DevLive,TEXT("XUserWriteProfileSettings(%d,3,data,data) returned 0x%08X"),
					LocalUserNum,Return);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					// Queue the async task for ticking
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Send the notification of error completion
					FAsyncTaskDelegateResults Results(Return);
					ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].WriteDelegate,&Results);
					delete AsyncTask;
				}
			}
			else
			{
				// Remove the write state so that subsequent writes work
				ProfileCache[LocalUserNum].Profile->AsyncState = OPAS_None;
				// Send the notification of error completion
				FAsyncTaskDelegateResults Results(ERROR_SUCCESS);
				ProcessDelegate(NAME_None,&ProfileCache[LocalUserNum].WriteDelegate,&Results);
				debugf(NAME_DevLive,TEXT("Skipping profile write for non-signed in user. Caching in profile cache"));
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't write a null profile settings object"));
		}
	}
	else
	{
		debugf(NAME_Error,
			TEXT("Can't write profile as an async profile task is already in progress for player (%d)"),
			LocalUserNum);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Sets a rich presence information to use for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PresenceMode the rich presence mode to use
 * @param Contexts the list of contexts to set
 * @param Properties the list of properties to set for the contexts
 */
void UOnlineSubsystemLive::SetRichPresence(BYTE LocalUserNum,INT PresenceMode,
	const TArray<FLocalizedStringSetting>& Contexts,
	const TArray<FSettingsProperty>& Properties)
{
	// Set all of the contexts/properties before setting the presence mode
	SetContexts(LocalUserNum,Contexts);
	SetProperties(LocalUserNum,Properties);
	debugf(NAME_DevLive,TEXT("XUserSetContext(%d,X_CONTEXT_PRESENCE,%d)"),
		LocalUserNum,PresenceMode);
	// Update the presence mode
	XUserSetContext(LocalUserNum,X_CONTEXT_PRESENCE,PresenceMode);
	// Update system link information if needed
	if (GameSettings != NULL && GameSettings->bIsLanMatch)
	{
		GameSettings->UpdateStringSettings(Contexts);
		GameSettings->UpdateProperties(Properties);
	}
}

/**
 * Displays the invite ui
 *
 * @param LocalUserNum the local user sending the invite
 * @param InviteText the string to prefill the UI with
 */
UBOOL UOnlineSubsystemLive::ShowInviteUI(BYTE LocalUserNum,const FString& InviteText)
{
	DWORD Result = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the game invite UI for the specified controller num
		Result = XShowGameInviteUI(LocalUserNum,NULL,0,
			InviteText.Len() > 0 ? *InviteText : NULL);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowInviteUI(%d) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowInviteUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the marketplace UI for content
 *
 * @param LocalUserNum the local user viewing available content
 */
UBOOL UOnlineSubsystemLive::ShowContentMarketplaceUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the marketplace for content
		Result = XShowMarketplaceUI(LocalUserNum,
			XSHOWMARKETPLACEUI_ENTRYPOINT_CONTENTLIST,0,(DWORD)-1);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowMarketplaceUI(%d,XSHOWMARKETPLACEUI_ENTRYPOINT_CONTENTLIST,0,-1) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowContentMarketplaceUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the marketplace UI for memberships
 *
 * @param LocalUserNum the local user viewing available memberships
 */
UBOOL UOnlineSubsystemLive::ShowMembershipMarketplaceUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Show the marketplace for memberships
		Result = XShowMarketplaceUI(LocalUserNum,
			XSHOWMARKETPLACEUI_ENTRYPOINT_MEMBERSHIPLIST,0,(DWORD)-1);
		if (Result != ERROR_SUCCESS)
		{
			debugf(NAME_Error,TEXT("XShowMarketplaceUI(%d,XSHOWMARKETPLACEUI_ENTRYPOINT_MEMBERSHIPLIST,0,-1) failed with 0x%08X"),
				LocalUserNum,Result);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to ShowMembershipMarketplaceUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the UI that allows the user to choose which device to save content to
 *
 * @param LocalUserNum the controller number of the associated user
 * @param SizeNeeded the size of the data to be saved in bytes
 * @param bForceShowUI true to always show the UI, false to only show the
 *		  UI if there are multiple valid choices
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemLive::ShowDeviceSelectionUI(BYTE LocalUserNum,INT SizeNeeded,UBOOL bForceShowUI)
{
	DWORD Return = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		ULARGE_INTEGER BytesNeeded;
		BytesNeeded.HighPart = 0;
		BytesNeeded.LowPart = SizeNeeded;
		// Allocate an async task for deferred calling of the delegates
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(&DeviceCache[LocalUserNum].DeviceSelectionMulticast,NULL,TEXT("XShowDeviceSelectorUI()"));
		ULARGE_INTEGER ContentSize = {0,0};
		ContentSize.QuadPart = XContentCalculateSize(BytesNeeded.QuadPart,1);
		// Show the live guide for selecting a device
		Return = XShowDeviceSelectorUI(LocalUserNum,
			XCONTENTTYPE_SAVEDGAME,
			bForceShowUI ? XCONTENTFLAG_FORCE_SHOW_UI : XCONTENTFLAG_NONE,
			ContentSize,
			(PXCONTENTDEVICEID)&DeviceCache[LocalUserNum].DeviceID,
			*AsyncTask);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&DeviceCache[LocalUserNum].DeviceSelectionMulticast,&Results);
			// Don't leak the task/data
			delete AsyncTask;
			debugf(NAME_Error,TEXT("XShowDeviceSelectorUI(%d,XCONTENTTYPE_SAVEDGAME,%s,%d,data,data) failed with 0x%08X"),
				LocalUserNum,bForceShowUI ? TEXT("XCONTENTFLAG_FORCE_SHOW_UI") : TEXT("XCONTENTFLAG_NONE"),Return);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified ShowDeviceSelectionUI()"),
			(DWORD)LocalUserNum);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Fetches the results of the device selection
 *
 * @param LocalUserNum the controller number of the associated user
 * @param DeviceName out param that gets a copy of the string
 *
 * @return the ID of the device that was selected
 */
INT UOnlineSubsystemLive::GetDeviceSelectionResults(BYTE LocalUserNum,FString& DeviceName)
{
	DeviceName.Empty();
	DWORD DeviceId = 0;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Zero means they haven't selected a device
		if (DeviceCache[LocalUserNum].DeviceID > 0)
		{
			// Live asserts if you call with a non-signed in player
			if (XUserGetSigninState(LocalUserNum) != eXUserSigninState_NotSignedIn)
			{
				XDEVICE_DATA DeviceData;
				appMemzero(&DeviceData,sizeof(XDEVICE_DATA));
				// Fetch the data, so we can get the friendly name
				DWORD Return = XContentGetDeviceData(DeviceCache[LocalUserNum].DeviceID,&DeviceData);
				if (Return == ERROR_SUCCESS)
				{
					DeviceName = DeviceData.wszFriendlyName;
					DeviceId = DeviceCache[LocalUserNum].DeviceID;
				}
			}
			else
			{
				debugf(NAME_Error,TEXT("User (%d) is not signed in, returning zero as an error"),
					(DWORD)LocalUserNum);
				DeviceCache[LocalUserNum].DeviceID = 0;
			}
		}
		else
		{
			debugf(NAME_Warning,TEXT("User (%d) has not selected a device yet, returning zero as an error"),
				(DWORD)LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified GetDeviceSelectionResults()"),
			(DWORD)LocalUserNum);
	}
	return DeviceId;
}

/**
 * Checks the device id to determine if it is still valid (could be removed)
 *
 * @param DeviceId the device to check
 *
 * @return TRUE if valid, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsDeviceValid(INT DeviceId)
{
	// Live asserts for a device id of zero
	if (DeviceId > 0)
	{
		return XContentGetDeviceState(DeviceId,NULL) == ERROR_SUCCESS;
	}
	return FALSE;
}

/**
 * Unlocks the specified achievement for the specified user
 *
 * @param LocalUserNum the controller number of the associated user
 * @param AchievementId the id of the achievement to unlock
 *
 * @return TRUE if the call worked, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::UnlockAchievement(BYTE LocalUserNum,INT AchievementId)
{
	DWORD Return = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		FLiveAsyncTaskDataWriteAchievement* AsyncTaskData = new FLiveAsyncTaskDataWriteAchievement(LocalUserNum,AchievementId);
		// Create a new async task to hold the data
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(&AchievementDelegates[LocalUserNum],AsyncTaskData,TEXT("XUserWriteAchievements()"));
		// Write the achievement to Live
		Return = XUserWriteAchievements(1,
			AsyncTaskData->GetAchievement(),
			*AsyncTask);
		debugf(NAME_DevLive,TEXT("XUserWriteAchievements() (%d),(%d) returned 0x%08X"),
			(DWORD)LocalUserNum,AchievementId,Return);
		// Clean up the task if it didn't succeed
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Queue the async task for ticking
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&AchievementDelegates[LocalUserNum],&Results);
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to UnlockAchievement()"),
			(DWORD)LocalUserNum);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Unlocks a gamer picture for the local user
 *
 * @param LocalUserNum the user to unlock the picture for
 * @param PictureId the id of the picture to unlock
 *
 * @return TRUE if the call worked, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::UnlockGamerPicture(BYTE LocalUserNum,INT PictureId)
{
	DWORD Return = E_FAIL;
	// Validate the user index
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Create a new async task to hold the data
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(NULL,NULL,TEXT("XUserAwardGamerPicture()"));
		// Unlock the picture with Live
		Return = XUserAwardGamerPicture(LocalUserNum,
			PictureId,
			0,
			*AsyncTask);
		debugf(NAME_DevLive,TEXT("XUserAwardGamerPicture(%d,%d,0,Overlapped) returned 0x%08X"),
			(DWORD)LocalUserNum,PictureId,Return);
		// Clean up the task if it didn't succeed
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Queue the async task for ticking
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Invalid player index (%d) specified to UnlockGamerPicture()"),
			(DWORD)LocalUserNum);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Tells Live that the session is in progress. Matches with join in progress
 * disabled will no longer show up in the search results.
 *
 * @return TRUE if the call succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::StartOnlineGame(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Lan matches don't report starting to Live
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Can't start a match multiple times
			if (CurrentGameState == OGS_Pending)
			{
				// Stop handling QoS queries if we aren't join in progress
				if (GameSettings->bAllowJoinInProgress == FALSE ||
					GameSettings->bUsesArbitration == TRUE)
				{
					DisableQoS();
				}
				// Allocate the object that will hold the data throughout the async lifetime
				FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskStartSession(GameSettings->bUsesArbitration,
					&__OnStartOnlineGameComplete__Delegate,TEXT("XSessionStart()"));
				// Do an async start request
				Return = XSessionStart(SessionInfo->Handle,0,*AsyncTask);
				debugf(NAME_DevLive,TEXT("XSessionStart() returned 0x%08X"),Return);
				// Clean up the task if it didn't succeed
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					// Queue the async task for ticking
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Just trigger the delegate as having failed
					FAsyncTaskDelegateResults Results(Return);
					ProcessDelegate(NAME_None,&__OnStartOnlineGameComplete__Delegate,&Results);
					delete AsyncTask;
				}
			}
			else
			{
				debugf(NAME_Error,TEXT("Can't start an online game in state %d"),CurrentGameState);
			}
		}
		else
		{
			// If this lan match has join in progress disabled, shut down the beacon
			if (GameSettings->bAllowJoinInProgress == FALSE)
			{
				StopSystemLinkBeacon();
			}
			Return = ERROR_SUCCESS;
			CurrentGameState = OGS_InProgress;
			// Indicate that the start completed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnStartOnlineGameComplete__Delegate,&Results);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't start an online game that hasn't been created"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Tells Live that the session has ended
 *
 * @return TRUE if the call succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::EndOnlineGame(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Lan matches don't report ending to Live
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Can't end a match that isn't in progress
			if (CurrentGameState == OGS_InProgress)
			{
				// Arbitrated true skill is handled differently
				if (GameSettings->bUsesArbitration == FALSE)
				{
					// Write the true skills stats before ending the session
					WriteTrueSkillStats();
				}
				CurrentGameState = OGS_Ending;
				// Task that will change the state
				FLiveAsyncTask* AsyncTask = new FLiveAsyncTaskSessionStateChange(OGS_Ended,
					&__OnEndOnlineGameComplete__Delegate,TEXT("XSessionEnd()"));
				// Do an async end request
				Return = XSessionEnd(SessionInfo->Handle,*AsyncTask);
				debugf(NAME_DevLive,TEXT("XSessionEnd() returned 0x%08X"),Return);
				// Clean up the task if it didn't succeed
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					// Queue the async task for ticking
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					delete AsyncTask;
				}
			}
			else
			{
				debugf(NAME_DevLive,TEXT("Can't end an online game in state %d"),CurrentGameState);
			}
		}
		else
		{
			Return = ERROR_SUCCESS;
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnEndOnlineGameComplete__Delegate,&Results);
			CurrentGameState = OGS_Ended;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't end an online game that hasn't been created"));
	}
	if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&__OnEndOnlineGameComplete__Delegate,&Results);
		CurrentGameState = OGS_Ended;
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Writes the true skill stats for players that are in the controller list
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::WriteTrueSkillStats(void)
{
	UBOOL CallWorked = TRUE;
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	check(WorldInfo);
	// Write true skill on all clients/server if arbitrated
	// Otherwise just write them if we are the server
	if (WorldInfo->GRI != NULL)
	{
		// Shouldn't be writing stats without a session
		check(GameSettings != NULL && SessionInfo != NULL);
		// Cache the GRI to get the list of players
		AGameReplicationInfo* GRI = WorldInfo->GRI;
		// Cache the team game setting
		AGameInfo* GameInfo = GRI->GameClass->GetDefaultObject<AGameInfo>();
		UBOOL bIsTeamGame = GameInfo ? GameInfo->bTeamGame : FALSE;
		// Iterate the controller list looking for players to report score on
		for (INT Index = 0; Index < GRI->PRIArray.Num() && CallWorked; Index++)
		{
			APlayerReplicationInfo* PlayerReplicationInfo = GRI->PRIArray(Index);
			if (PlayerReplicationInfo->bIsInactive == FALSE)
			{
				if (bIsTeamGame)
				{
					if (PlayerReplicationInfo &&
						PlayerReplicationInfo->Team)
					{
						// Use the common method to write the skill info
						CallWorked = WriteTrueSkillForPlayer((XUID&)PlayerReplicationInfo->UniqueId,
							(DWORD)PlayerReplicationInfo->Team->TeamIndex,
							PlayerReplicationInfo->Team->Score);
					}
					else
					{
						debugf(NAME_Error,TEXT("Unable to write TrueSkill for GRI->PRIArray(%d)"),Index);
					}
				}
				else
				{
					if (PlayerReplicationInfo)
					{
						// Use the common method to write the skill info
						CallWorked = WriteTrueSkillForPlayer((XUID&)PlayerReplicationInfo->UniqueId,
							(DWORD)PlayerReplicationInfo->PlayerID,
							PlayerReplicationInfo->Score);
					}
					else
					{
						debugf(NAME_Error,TEXT("Unable to write TrueSkill for GRI->PRIArray(%d)"),Index);
					}
				}
			}
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Failed to write TrueSkill stats due to null GRI"));
	}
	return CallWorked;
}

/**
 * Writes the true skill stat for a single player
 *
 * @param Xuid the xuid of the player to write the true skill for
 * @param TeamId the team the player is on
 * @param Score the score this player had
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::WriteTrueSkillForPlayer(XUID Xuid,DWORD TeamId,
	FLOAT Score)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (SessionInfo != NULL && SessionInfo->Handle != NULL && CurrentGameState == OGS_InProgress)
	{
		XUSER_PROPERTY Skill[2];
		// Write the score for the player (convert to INT)
		Skill[0].dwPropertyId = X_PROPERTY_RELATIVE_SCORE;
		Skill[0].value.nData = appTrunc(Score);
		Skill[0].value.type = XUSER_DATA_TYPE_INT32;
		// And place them on a team for true skill to do its magic
		Skill[1].dwPropertyId = X_PROPERTY_SESSION_TEAM;
		Skill[1].value.nData = TeamId;
		Skill[1].value.type = XUSER_DATA_TYPE_INT32;
		XSESSION_VIEW_PROPERTIES View;
		// Fill in the view structure to tell Live the true skill stats
		View.dwNumProperties = 2;
		View.dwViewId = X_STATS_VIEW_SKILL;
		View.pProperties = Skill;
//@todo joeg -- Determine if async is needed. According to the docs, these
// are batched and not flushed until session end or stats flush, so doing
// this synchronously should be fine
		// Now write the data, synchronously
		Return = XSessionWriteStats(SessionInfo->Handle,Xuid,1,&View,NULL);
		debugf(NAME_DevLive,TEXT("TrueSkill write (Player = 0x%016I64X, Team = %d, Score = %d) returned 0x%08X"),
			Xuid,TeamId,appTrunc(Score),Return);
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Can't write TrueSkill when not in a game"));
	}
	return Return == ERROR_SUCCESS;
}

/**
 * Parses the arbitration results into something the game play code can handle
 *
 * @param ArbitrationResults the buffer filled by Live
 */
void UOnlineSubsystemLive::ParseArbitrationResults(PXSESSION_REGISTRATION_RESULTS ArbitrationResults)
{
	ArbitrationList.Empty();
	// Dump out arbitration data if configured
	if (bShouldLogArbitrationData)
	{
		debugf(NAME_DevLive,TEXT("Aribitration registration results %d"),
			(DWORD)ArbitrationResults->wNumRegistrants);
	}
	// Iterate through the list and set each entry
	for (DWORD Index = 0; Index < ArbitrationResults->wNumRegistrants; Index++)
	{
		const XSESSION_REGISTRANT& LiveEntry = ArbitrationResults->rgRegistrants[Index];
		// Add a new item for each player listed
		for (DWORD PlayerIndex = 0; PlayerIndex < LiveEntry.bNumUsers; PlayerIndex++)
		{
			INT AddAtIndex = ArbitrationList.AddZeroed();
			FOnlineArbitrationRegistrant& Entry = ArbitrationList(AddAtIndex);
			// Copy the data over
			Entry.MachineId = LiveEntry.qwMachineID;
			(XUID&)Entry.PlayerID = LiveEntry.rgUsers[PlayerIndex];
			Entry.Trustworthiness = LiveEntry.bTrustworthiness;
			// Dump out arbitration data if configured
			if (bShouldLogArbitrationData)
			{
				debugf(NAME_DevLive,TEXT("MachineId = 0x%016I64X, PlayerId = 0x%016I64X, Trustworthiness = %d"),
					Entry.MachineId,(XUID&)Entry.PlayerID,Entry.Trustworthiness);
			}
		}
	}
}

/**
 * Tells the game to register with the underlying arbitration server if available
 */
UBOOL UOnlineSubsystemLive::RegisterForArbitration(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Can't register for arbitration without the host information
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Lan matches don't use arbitration
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Verify that the game is meant to use arbitration
			if (GameSettings->bUsesArbitration == TRUE)
			{
				// Make sure the game state is pending as registering after that is silly
				if (CurrentGameState == OGS_Pending)
				{
					DWORD BufferSize = 0;
					// Determine the amount of space needed for arbitration
					Return = XSessionArbitrationRegister(SessionInfo->Handle,
						0,
						(ULONGLONG&)GameSettings->ServerNonce,
						&BufferSize,
						NULL,
						NULL);
					if (Return == ERROR_INSUFFICIENT_BUFFER && BufferSize > 0)
					{
						// Async task to parse the results
						FLiveAsyncTaskArbitrationRegistration* AsyncTask = new FLiveAsyncTaskArbitrationRegistration(BufferSize,
							&__OnArbitrationRegistrationComplete__Delegate);
						// Now kick off the async task to do the registration
						Return = XSessionArbitrationRegister(SessionInfo->Handle,
							0,
							(ULONGLONG&)GameSettings->ServerNonce,
							&BufferSize,
							AsyncTask->GetResults(),
							*AsyncTask);
						debugf(NAME_DevLive,TEXT("XSessionArbitrationRegister() returned 0x%08X"),Return);
						// Clean up the task if it didn't succeed
						if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
						{
							// Queue the async task for ticking
							AsyncTasks.AddItem(AsyncTask);
						}
						else
						{
							delete AsyncTask;
						}
					}
					else
					{
						debugf(NAME_DevLive,TEXT("Failed to determine buffer size for arbitration with 0x%08X"),Return);
					}
				}
				else
				{
					debugf(NAME_DevLive,TEXT("Can't register for arbitration when the game is not pending"));
				}
			}
			else
			{
				debugf(NAME_DevLive,TEXT("Can't register for arbitration on non-arbitrated games"));
			}
		}
		else
		{
			debugf(NAME_DevLive,TEXT("LAN matches don't use arbitration, ignoring call"));
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("Can't register for arbitration on a non-existant game"));
	}
	// If there is an error, fire the delegate indicating the error
	if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
	{
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&__OnArbitrationRegistrationComplete__Delegate,&Results);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Writes the final arbitration information for a match
 */
UBOOL UOnlineSubsystemLive::WriteArbitrationData(void)
{
	return WriteTrueSkillStats();
}

/**
 * Tells the online subsystem to accept the game invite that is currently pending
 *
 * @param LocalUserNum the local user accepting the invite
 */
UBOOL UOnlineSubsystemLive::AcceptGameInvite(BYTE LocalUserNum)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Fail if we don't have an invite pending for this user
	if (InviteCache[LocalUserNum].InviteData != NULL)
	{
		FInviteData& Invite = InviteCache[LocalUserNum];
		// Zero out so we know this invite has been handled
		Invite.InviteData = NULL;
		// And now we can join the session
		if (JoinOnlineGame(LocalUserNum,Invite.InviteSearch->Results(0)) == TRUE)
		{
			Return = ERROR_SUCCESS;
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to join the invite game, aborting"));
		}
		// Clean up the invite data
		delete (XSESSION_INFO*)Invite.InviteSearch->Results(0).PlatformData;
		Invite.InviteSearch->Results(0).PlatformData = NULL;
		delete Invite.InviteData;
		// Zero the search so it can be GCed
		Invite.InviteSearch = NULL;
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Registers all local players with the current session
 *
 * @param bIsFromInvite whether this is from an invite or from searching
 */
void UOnlineSubsystemLive::RegisterLocalPlayers(UBOOL bIsFromInvite)
{
	check(SessionInfo);
	FLiveAsyncTaskDataRegisterLocalPlayers* AsyncTaskData = new FLiveAsyncTaskDataRegisterLocalPlayers();
	// Loop through the 4 available players and register them if they
	// are valid
	for (DWORD Index = 0; Index < 4; Index++)
	{
		// Ignore non-Live enabled profiles
		if (XUserGetSigninState(Index) == eXUserSigninState_SignedInToLive)
		{
			AsyncTaskData->AddPlayer(Index);
			// Register the local player as a local talker
			RegisterLocalTalker(Index);
		}
	}
	DWORD PlayerCount = AsyncTaskData->GetCount();
	// This should never happen outside of testing, but check anyway
	if (PlayerCount > 0)
	{
		// If this match is an invite match or there were never any public slots (private only)
		// then join as private
		if (bIsFromInvite || GameSettings->NumPublicConnections == 0)
		{
			// Adjust the number of private slots based upon invites and space
			DWORD NumPrivateToUse = Min<DWORD>(PlayerCount,GameSettings->NumOpenPrivateConnections);
			debugf(NAME_DevLive,TEXT("Using %d private slots"),NumPrivateToUse);
			AsyncTaskData->SetPrivateSlotsUsed(NumPrivateToUse);
		}
		// Create a new async task to hold the data
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(NULL,AsyncTaskData,TEXT("XSessionJoinLocal()"));
		// Now register them as a group, asynchronously
		DWORD Return = XSessionJoinLocal(SessionInfo->Handle,
			AsyncTaskData->GetCount(),
			AsyncTaskData->GetPlayers(),
			AsyncTaskData->GetPrivateSlots(),
			*AsyncTask);
		debugf(NAME_DevLive,TEXT("XSessionJoinLocal() returned 0x%08X"),Return);
		// Clean up the task if it didn't succeed
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Queue the async task for ticking
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_DevLive,TEXT("No locally signed in players to join game"));
		delete AsyncTaskData;
	}
}

/**
 * Starts an async task that retrieves the list of friends for the player from the
 * online service. The list can be retrieved in whole or in part.
 *
 * @param LocalUserNum the user to read the friends list of
 * @param Count the number of friends to read or zero for all
 * @param StartingAt the index of the friends list to start at (for pulling partial lists)
 *
 * @return true if the read request was issued successfully, false otherwise
 */
UBOOL UOnlineSubsystemLive::ReadFriendsList(BYTE LocalUserNum,INT Count,INT StartingAt)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Throw out the old friends list
	FriendsCache[LocalUserNum].Friends.Empty();
	FriendsCache[LocalUserNum].ReadState = OERS_NotStarted;
	// Can't read more than MAX_FRIENDS at a time, so don't request too much
//	DWORD NumPerRead = Min(Count,MAX_FRIENDS);
	//@todo joeg -- Change this once XDS explains how async is supposed to handle multiple
	DWORD NumPerRead = 1;
	HANDLE Handle;
	DWORD SizeNeeded;
	// Create a new enumerator for the friends list
	Return = XFriendsCreateEnumerator(LocalUserNum,StartingAt,NumPerRead,&SizeNeeded,&Handle);
	debugf(NAME_DevLive,TEXT("XFriendsCreateEnumerator(%d,%d,%d,out,out) returned 0x%0X"),
		LocalUserNum,StartingAt,Count,Return);
	if (Return == ERROR_SUCCESS)
	{
		// Create the async data object that holds the buffers, etc.
		FLiveAsyncTaskDataEnumeration* AsyncTaskData = new FLiveAsyncTaskDataEnumeration(LocalUserNum,Handle,SizeNeeded,Count);
		// Create the async task object
		FLiveAsyncTaskReadFriends* AsyncTask = new FLiveAsyncTaskReadFriends(&FriendsCache[LocalUserNum].ReadCompleteDelegate,AsyncTaskData);
		// Start the async read
		Return = XEnumerate(Handle,AsyncTaskData->GetBuffer(),SizeNeeded,0,*AsyncTask);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Mark this as being read
			FriendsCache[LocalUserNum].ReadState = OERS_InProgress;
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&FriendsCache[LocalUserNum].ReadCompleteDelegate,&Results);
			// Delete the async task
			delete AsyncTask;
		}
	}
	UBOOL bFireDelegate = FALSE;
	// Friends list might be empty
	if (Return == ERROR_NO_MORE_FILES)
	{
		bFireDelegate = TRUE;
		Return = ERROR_SUCCESS;
		// Set it to done, since there is nothing to read
		FriendsCache[LocalUserNum].ReadState = OERS_Done;
	}
	else if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
	{
		bFireDelegate = TRUE;
		FriendsCache[LocalUserNum].ReadState = OERS_Failed;
	}
	// Fire off the delegate if needed
	if (bFireDelegate && DELEGATE_IS_SET(OnReadFriendsComplete))
	{
		delegateOnReadFriendsComplete(Return == ERROR_SUCCESS);
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Copies the list of friends for the player previously retrieved from the online
 * service. The list can be retrieved in whole or in part.
 *
 * @param LocalUserNum the user to read the friends list of
 * @param Friends the out array that receives the copied data
 * @param Count the number of friends to read or zero for all
 * @param StartingAt the index of the friends list to start at (for pulling partial lists)
 *
 * @return OERS_Done if the read has completed, otherwise one of the other states
 */
BYTE UOnlineSubsystemLive::GetFriendsList(BYTE LocalUserNum,
	TArray<FOnlineFriend>& Friends,INT Count,INT StartingAt)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	// Check to see if the last read request has completed
	BYTE Return = FriendsCache[LocalUserNum].ReadState;
	if (Return == OERS_Done)
	{
		// See if they requested all of the data
		if (Count == 0)
		{
			Count = FriendsCache[LocalUserNum].Friends.Num();
		}
		// Presize the out array
		INT AmountToAdd = Min(Count,FriendsCache[LocalUserNum].Friends.Num() - StartingAt);
		Friends.Empty(AmountToAdd);
		Friends.AddZeroed(AmountToAdd);
		// Copy the data from the starting point to the number desired
		for (INT Index = 0;
			Index < Count && (Index + StartingAt) < FriendsCache[LocalUserNum].Friends.Num();
			Index++)
		{
			Friends(Index) = FriendsCache[LocalUserNum].Friends(Index + StartingAt);
		}
	}
	return Return;
}

/**
 * Parses the friends results into something the game play code can handle
 *
 * @param PlayerIndex the index of the player that this read was for
 * @param LiveFriend the buffer filled by Live
 */
void UOnlineSubsystemLive::ParseFriendsResults(DWORD PlayerIndex,
	PXONLINE_FRIEND LiveFriend)
{
	check(PlayerIndex >= 0 && PlayerIndex < 4);
	// Figure out where we are appending our friends to
	INT Offset = FriendsCache[PlayerIndex].Friends.AddZeroed(1);
	// Get the friend we just added
	FOnlineFriend& Friend = FriendsCache[PlayerIndex].Friends(Offset);
	// Copy the name
	Friend.NickName = LiveFriend->szGamertag;
	// Copy the presence string
	Friend.PresenceInfo = LiveFriend->wszRichPresence;
	// Copy the XUID
	appMemcpy(&Friend.UniqueId,&LiveFriend->xuid,sizeof(XUID));
	// Set booleans
	Friend.bHasVoiceSupport = LiveFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_VOICE;
	Friend.bIsJoinable = LiveFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_JOINABLE;
	Friend.bIsOnline = LiveFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE;
	Friend.bIsPlaying = LiveFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING;
	// Check that the title id is the one we expect
	Friend.bIsPlayingThisGame = LiveFriend->dwTitleID == appGetTitleId();
}

/**
 * Starts an async task that retrieves the list of downloaded content for the player.
 *
 * @param LocalUserNum The user to read the content list of
 *
 * @return true if the read request was issued successfully, false otherwise
 */
UBOOL UOnlineSubsystemLive::ReadContentList(BYTE LocalUserNum)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Throw out the old friends list
	ContentCache[LocalUserNum].Content.Empty();
	ContentCache[LocalUserNum].ReadState = OERS_NotStarted;

	// if the user is logging in, search for any DLC
	DWORD SizeNeeded;
	HANDLE Handle;
	// return 1 at a time per XEnumerate call
	DWORD NumToRead = 1;

	// start looking for this user's downloadable content
	Return = XContentCreateEnumerator(LocalUserNum, XCONTENTDEVICE_ANY, XCONTENTTYPE_MARKETPLACE, 
		0, NumToRead, &SizeNeeded, &Handle);
	
	// make sure we succeeded
	if (Return == ERROR_SUCCESS)
	{
		// Create the async data object that holds the buffers, etc (using 0 for number to retrieve for all)
		FLiveAsyncTaskContent* AsyncTaskData = new FLiveAsyncTaskContent(LocalUserNum, Handle, SizeNeeded, 0);
		// Create the async task object
		FLiveAsyncTaskReadContent* AsyncTask = new FLiveAsyncTaskReadContent(&ContentCache[LocalUserNum].ReadCompleteDelegate, AsyncTaskData);
		// Start the async read
		Return = XEnumerate(Handle, AsyncTaskData->GetBuffer(), SizeNeeded, 0, *AsyncTask);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			// Mark this as being read
			ContentCache[LocalUserNum].ReadState = OERS_InProgress;
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&ContentCache[LocalUserNum].ReadCompleteDelegate,&Results);
			// Delete the async task
			delete AsyncTask;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("XContentCreateEnumerator(%d, XCONTENTDEVICE_ANY, XCONTENTTYPE_MARKETPLACE, 0, 1, &BufferSize, &EnumerateHandle) failed with 0x%08X"),
			LocalUserNum, Return);
	}

	UBOOL bFireDelegate = FALSE;
	// Friends list might be empty
	if (Return == ERROR_NO_MORE_FILES)
	{
		bFireDelegate = TRUE;
		Return = ERROR_SUCCESS;
		// Set it to done, since there is nothing to read
		ContentCache[LocalUserNum].ReadState = OERS_Done;
	}
	else if (Return != ERROR_SUCCESS && Return != ERROR_IO_PENDING)
	{
		bFireDelegate = TRUE;
		ContentCache[LocalUserNum].ReadState = OERS_Failed;
	}
	// Fire off the delegate if needed
	if (bFireDelegate && DELEGATE_IS_SET(OnReadContentComplete))
	{
		delegateOnReadContentComplete(Return == ERROR_SUCCESS);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Retrieve the list of content the given user has downloaded or otherwise retrieved
 * to the local console.
 *
 * @param LocalUserNum The user to read the content list of
 * @param ContentList The out array that receives the list of all content
 */
BYTE UOnlineSubsystemLive::GetContentList(BYTE LocalUserNum, TArray<FOnlineContent>& ContentList)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	// Check to see if the last read request has completed
	BYTE Return = ContentCache[LocalUserNum].ReadState;
	if (Return == OERS_Done)
	{
		ContentList = ContentCache[LocalUserNum].Content;
	}
	return Return;
}

/**
 * Asks the online system for the number of new and total content downloads
 *
 * @param LocalUserNum the user to check the content download availability for
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::QueryAvailableDownloads(BYTE LocalUserNum)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	// Create the async task and data objects
	FLiveAsyncTaskDataQueryDownloads* AsyncData = new FLiveAsyncTaskDataQueryDownloads(LocalUserNum);
	FLiveAsyncTaskQueryDownloads* AsyncTask = new FLiveAsyncTaskQueryDownloads(&ContentCache[LocalUserNum].QueryDownloadsDelegate,AsyncData);
	// Do an async query for the content counts
	DWORD Return = XContentGetMarketplaceCounts(LocalUserNum,
		(DWORD)-1,
		sizeof(XOFFERING_CONTENTAVAILABLE_RESULT),
		AsyncData->GetQuery(),
		*AsyncTask);
	debugf(NAME_DevLive,TEXT("XContentGetMarketplaceCounts(%d) returned 0x%08X"),
		(DWORD)LocalUserNum,Return);
	// Clean up the task if it didn't succeed
	if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
	{
		// Queue the async task for ticking
		AsyncTasks.AddItem(AsyncTask);
	}
	else
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Results(Return);
		ProcessDelegate(NAME_None,&ContentCache[LocalUserNum].QueryDownloadsDelegate,&Results);
		delete AsyncTask;
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Registers the user as a talker
 *
 * @param LocalUserNum the local player index that is a talker
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::RegisterLocalTalker(BYTE LocalUserNum)
{
	check(VoiceEngine);
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = E_FAIL;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Get at the local talker's cached data
		FLocalTalker& Talker = LocalTalkers[LocalUserNum];
		// Don't register non-Live enabled accounts
		if (XUserGetSigninState(LocalUserNum) != eXUserSigninState_NotSignedIn)
		{
			// Register the talker locally
			Return = VoiceEngine->RegisterLocalTalker(LocalUserNum);
			debugf(NAME_DevLive,TEXT("RegisterLocalTalker(%d) returned 0x%08X"),
				LocalUserNum,Return);
			if (Return == S_OK)
			{
				Talker.bHasVoice = TRUE;
				APlayerController* PlayerController = GetPlayerControllerFromUserIndex(LocalUserNum);
				if (PlayerController != NULL)
				{
					// Update the muting information for this local talker
					UpdateMuteListForLocalTalker(LocalUserNum,PlayerController);
				}
				else
				{
					debugf(NAME_Error,TEXT("Can't update mute list for %d due to:"),LocalUserNum);
					debugf(NAME_Error,TEXT("Failed to find player controller for %d"),LocalUserNum);
				}
				// Kick off the processing mode
				Return = VoiceEngine->StartLocalVoiceProcessing(LocalUserNum);
				debugf(NAME_DevLive,TEXT("StartLocalProcessing(%d) returned 0x%08X"),
					(DWORD)LocalUserNum,Return);
			}
		}
		else
		{
			// Not properly logged in, so skip voice for them
			Talker.bHasVoice = Talker.bUseLoopback = FALSE;
		}
	}
	return Return == S_OK;
}

/**
 * Unregisters the user as a talker
 *
 * @param LocalUserNum the local player index to be removed
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::UnregisterLocalTalker(BYTE LocalUserNum)
{
	check(VoiceEngine);
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = S_OK;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Get at the local talker's cached data
		FLocalTalker& Talker = LocalTalkers[LocalUserNum];
		// Skip the unregistration if not registered
		if (Talker.bHasVoice == TRUE)
		{
			// Remove them from XHV too
			Return = VoiceEngine->UnregisterLocalTalker(LocalUserNum);
			debugf(NAME_DevLive,TEXT("UnregisterLocalTalker(%d) returned 0x%08X"),
				LocalUserNum,Return);
			Talker.bHasVoice = Talker.bUseLoopback = FALSE;
		}
	}
	return Return == S_OK;
}

/**
 * Registers a remote player as a talker
 *
 * @param UniqueId the unique id of the remote player that is a talker
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::RegisterRemoteTalker(FUniqueNetId UniqueId)
{
	check(VoiceEngine);
	DWORD Return = E_FAIL;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// See if this talker has already been registered or not
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker == NULL)
		{
			// Add a new talker to our list
			INT AddIndex = RemoteTalkers.AddZeroed();
			Talker = &RemoteTalkers(AddIndex);
			// Copy the XUID
			(XUID&)Talker->TalkerId = (XUID&)UniqueId;
			// Register the remote talker locally
			Return = VoiceEngine->RegisterRemoteTalker(UniqueId);
			debugf(NAME_DevLive,TEXT("RegisterRemoteTalker(0x%016I64X) returned 0x%08X"),
				(XUID&)UniqueId,Return);
			// Update muting all of the local talkers with this remote talker
			ProcessMuteChangeNotification();
		}
		else
		{
			debugf(NAME_DevLive,TEXT("Remote talker is being re-registered"));
			Return = S_OK;
		}
		// Now start processing the remote voices
		Return = VoiceEngine->StartRemoteVoiceProcessing(UniqueId);
		debugf(NAME_DevLive,TEXT("StartRemoteVoiceProcessing(0x%016I64X) returned 0x%08X"),
			(XUID&)UniqueId,Return);
	}
	return Return == S_OK;
}

/**
 * Unregisters a remote player as a talker
 *
 * @param UniqueId the unique id of the remote player to be removed
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::UnregisterRemoteTalker(FUniqueNetId UniqueId)
{
	check(VoiceEngine);
	DWORD Return = E_FAIL;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Make sure the talker is valid
		if (FindRemoteTalker(UniqueId) != NULL)
		{
			// Find them in the talkers array and remove them
			for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
			{
				const FRemoteTalker& Talker = RemoteTalkers(Index);
				// Is this the remote talker?
				if ((XUID&)Talker.TalkerId == (XUID&)UniqueId)
				{
					RemoteTalkers.Remove(Index);
					break;
				}
			}
			// Remove them from XHV too
			Return = VoiceEngine->UnregisterRemoteTalker(UniqueId);
			debugf(NAME_DevLive,TEXT("UnregisterRemoteTalker(0x%016I64X) returned 0x%08X"),
				(XUID&)UniqueId,Return);
		}
		else
		{
			debugf(NAME_Error,TEXT("Unknown remote talker specified to UnregisterRemoteTalker()"));
		}
	}
	return Return == S_OK;
}

/**
 * Determines if the specified player is actively talking into the mic
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsLocalPlayerTalking(BYTE LocalUserNum)
{
	check(VoiceEngine);
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	return VoiceEngine->IsLocalPlayerTalking(LocalUserNum);
}

/**
 * Determines if the specified remote player is actively talking into the mic
 * NOTE: Network latencies will make this not 100% accurate
 *
 * @param UniqueId the unique id of the remote player being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsRemotePlayerTalking(FUniqueNetId UniqueId)
{
	check(VoiceEngine);
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		return VoiceEngine->IsRemotePlayerTalking(UniqueId);
	}
	return FALSE;		
}

/**
 * Determines if the specified player has a headset connected
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player has a headset plugged in, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::IsHeadsetPresent(BYTE LocalUserNum)
{
	check(VoiceEngine);
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	return VoiceEngine->IsHeadsetPresent(LocalUserNum);
}

/**
 * Sets the relative priority for a remote talker. 0 is highest
 *
 * @param LocalUserNum the user that controls the relative priority
 * @param UniqueId the remote talker that is having their priority changed for
 * @param Priority the relative priority to use (0 highest, < 0 is muted)
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::SetRemoteTalkerPriority(BYTE LocalUserNum,FUniqueNetId UniqueId,INT Priority)
{
	check(VoiceEngine);
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = E_INVALIDARG;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Find the remote talker to modify
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker != NULL)
		{
			// Cache the old and set the new current priority
			Talker->LocalPriorities[LocalUserNum].LastPriority = Talker->LocalPriorities[LocalUserNum].CurrentPriority;
			Talker->LocalPriorities[LocalUserNum].CurrentPriority = Priority;
			Return = VoiceEngine->SetPlaybackPriority(LocalUserNum,UniqueId,
				(XHV_PLAYBACK_PRIORITY)Priority);
			debugf(NAME_DevLive,TEXT("SetPlaybackPriority(%d,0x%016I64X,%d) return 0x%08X"),
				LocalUserNum,(XUID&)UniqueId,Priority,Return);
		}
		else
		{
			debugf(NAME_Error,TEXT("Unknown remote talker specified to SetRemoteTalkerPriority()"));
		}
	}
	return Return == S_OK;
}

/**
 * Mutes a remote talker for the specified local player. NOTE: This is separate
 * from the user's permanent online mute list
 *
 * @param LocalUserNum the user that is muting the remote talker
 * @param UniqueId the remote talker that is being muted
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::MuteRemoteTalker(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	DWORD Return = E_INVALIDARG;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Find the specified talker
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker != NULL)
		{
			// Mark this player as muted locally
			Talker->NumberOfMutes++;
			// This is the talker in question, so cache the last priority
			// and change the current to muted
			Talker->LocalPriorities[LocalUserNum].LastPriority = Talker->LocalPriorities[LocalUserNum].CurrentPriority;
			Talker->LocalPriorities[LocalUserNum].CurrentPriority = XHV_PLAYBACK_PRIORITY_NEVER;
			// Set their priority to never
			Return = VoiceEngine->SetPlaybackPriority(LocalUserNum,UniqueId,XHV_PLAYBACK_PRIORITY_NEVER);
			debugf(NAME_DevLive,TEXT("SetPlaybackPriority(%d,0x%016I64X,-1) return 0x%08X"),
				LocalUserNum,(XUID&)UniqueId,Return);
		}
		else
		{
			debugf(NAME_Error,TEXT("Unknown remote talker specified to MuteRemoteTalker()"));
		}
	}
	return Return == S_OK;
}

/**
 * Allows a remote talker to talk to the specified local player. NOTE: This call
 * will fail for remote talkers on the user's permanent online mute list
 *
 * @param LocalUserNum the user that is allowing the remote talker to talk
 * @param UniqueId the remote talker that is being restored to talking
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::UnmuteRemoteTalker(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	check(LocalUserNum >= 0 && LocalUserNum < 4);
	check(VoiceEngine != NULL);
	DWORD Return = E_INVALIDARG;
	// Skip this if the session isn't active
	if (GameSettings != NULL)
	{
		// Find the specified talker
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker != NULL)
		{
			UBOOL bIsMuted = FALSE;
			// Verify that this talker isn't on the mute list
			XUserMuteListQuery(LocalUserNum,(XUID&)UniqueId,&bIsMuted);
			// Only restore their priority if they aren't muted
			if (bIsMuted == FALSE)
			{
				if (Talker->NumberOfMutes > 0)
				{
					// Mark this player as unmuted locally
					Talker->NumberOfMutes--;
				}
				Talker->LocalPriorities[LocalUserNum].LastPriority = Talker->LocalPriorities[LocalUserNum].CurrentPriority;
				Talker->LocalPriorities[LocalUserNum].CurrentPriority = 0;
				// Set their priority to unmuted
				Return = VoiceEngine->SetPlaybackPriority(LocalUserNum,UniqueId,
					Talker->LocalPriorities[LocalUserNum].CurrentPriority);
				debugf(NAME_DevLive,TEXT("SetPlaybackPriority(%d,0x%016I64X,%d) returned 0x%08X"),
					LocalUserNum,(XUID&)UniqueId,
					Talker->LocalPriorities[LocalUserNum].CurrentPriority,Return);
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Unknown remote talker specified to UnmuteRemoteTalker()"));
		}
	}
	return Return == S_OK;
}

/**
 * Tells the voice system to start tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemLive::StartSpeechRecognition(BYTE LocalUserNum)
{
	HRESULT Return = E_FAIL;
	if (bIsUsingSpeechRecognition)
	{
		Return = VoiceEngine->StartSpeechRecognition(LocalUserNum);
		debugf(NAME_DevLive,TEXT("StartSpeechRecognition(%d) returned 0x%08X"),
			LocalUserNum,Return);
	}
	return SUCCEEDED(Return);
}

/**
 * Tells the voice system to stop tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemLive::StopSpeechRecognition(BYTE LocalUserNum)
{
	HRESULT Return = E_FAIL;
	if (bIsUsingSpeechRecognition)
	{
		Return = VoiceEngine->StopSpeechRecognition(LocalUserNum);
		debugf(NAME_DevLive,TEXT("StopSpeechRecognition(%d) returned 0x%08X"),
			LocalUserNum,Return);
	}
	return SUCCEEDED(Return);
}

/**
 * Gets the results of the voice recognition
 *
 * @param LocalUserNum the local user to read the results of
 * @param Words the set of words that were recognized by the voice analyzer
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemLive::GetRecognitionResults(BYTE LocalUserNum,TArray<FSpeechRecognizedWord>& Words)
{
	HRESULT Return = E_FAIL;
	if (bIsUsingSpeechRecognition)
	{
		Return = VoiceEngine->GetRecognitionResults(LocalUserNum,Words);
		debugf(NAME_DevLive,TEXT("GetRecognitionResults(%d,Array) returned 0x%08X"),
			LocalUserNum,Return);
	}
	return SUCCEEDED(Return);
}

/**
 * Re-evaluates the muting list for all local talkers
 */
void UOnlineSubsystemLive::ProcessMuteChangeNotification(void)
{
	check(VoiceEngine);
	// Nothing to update if there isn't an active session
	if (GameSettings != NULL)
	{
		// For each local user with voice
		for (INT Index = 0; Index < 4; Index++)
		{
			APlayerController* PlayerController = GetPlayerControllerFromUserIndex(Index);
			// If there is a player controller, we can mute/unmute people
			if (LocalTalkers[Index].bHasVoice && PlayerController != NULL)
			{
				// Use the common method of checking muting
				UpdateMuteListForLocalTalker(Index,PlayerController);
			}
		}
	}
}

/**
 * Figures out which remote talkers need to be muted for a given local talker
 *
 * @param TalkerIndex the talker that needs the mute list checked for
 * @param PlayerController the player controller associated with this talker
 */
void UOnlineSubsystemLive::UpdateMuteListForLocalTalker(INT TalkerIndex,
	APlayerController* PlayerController)
{
	// For each registered remote talker
	for (INT RemoteIndex = 0; RemoteIndex < RemoteTalkers.Num(); RemoteIndex++)
	{
		FRemoteTalker& Talker = RemoteTalkers(RemoteIndex);
		UBOOL bIsMuted = FALSE;
		// Is the remote talker on this local player's mute list?
		XUserMuteListQuery(TalkerIndex,(XUID&)Talker.TalkerId,&bIsMuted);
		// Figure out which priority to use now
		if (bIsMuted == FALSE)
		{
			// If they were previously muted, set them to zero priority
			if (Talker.LocalPriorities[TalkerIndex].CurrentPriority == XHV_PLAYBACK_PRIORITY_NEVER)
			{
				Talker.LocalPriorities[TalkerIndex].LastPriority = Talker.LocalPriorities[TalkerIndex].CurrentPriority;
				Talker.LocalPriorities[TalkerIndex].CurrentPriority = 0;
				// Unmute on the server
				PlayerController->eventServerUnmutePlayer(Talker.TalkerId);
				// Mark this player as unmuted locally
				if (Talker.NumberOfMutes > 0)
				{
					Talker.NumberOfMutes--;
				}
			}
			else
			{
				// Use their current priority without changes
			}
		}
		else
		{
			// Mute this remote talker
			Talker.LocalPriorities[TalkerIndex].LastPriority = Talker.LocalPriorities[TalkerIndex].CurrentPriority;
			Talker.LocalPriorities[TalkerIndex].CurrentPriority = XHV_PLAYBACK_PRIORITY_NEVER;
			// Mute on the server
			PlayerController->eventServerMutePlayer(Talker.TalkerId);
			// Mark this player as muted locally
			Talker.NumberOfMutes++;
		}
		// The ServerUn/MutePlayer() functions will perform the muting based
		// upon gameplay settings and other player's mute list
	}
}

/**
 * Registers/unregisters local talkers based upon login changes
 */
void UOnlineSubsystemLive::UpdateVoiceFromLoginChange(void)
{
	// Nothing to update if there isn't an active session
	if (GameSettings != NULL)
	{
		// Check each user index for a sign in change
		for (INT Index = 0; Index < 4; Index++)
		{
			XUSER_SIGNIN_STATE SignInState = XUserGetSigninState(Index);
			// Was this player registered last time around but is no longer signed in
			if (LocalTalkers[Index].bHasVoice == TRUE &&
				SignInState != eXUserSigninState_SignedInToLive)
			{
				UnregisterLocalTalker(Index);
			}
			// Was this player not registered, but now is logged in
			else if (LocalTalkers[Index].bHasVoice == FALSE &&
				SignInState == eXUserSigninState_SignedInToLive)
			{
				RegisterLocalTalker(Index);
			}
			else
			{
				// Logged in and registered, so do nothing
			}
		}
	}
}

/**
 * Iterates the current remote talker list unregistering them with XHV
 * and our internal state
 */
void UOnlineSubsystemLive::RemoveAllRemoteTalkers(void)
{
	check(VoiceEngine);
	debugf(NAME_DevLive,TEXT("Removing all remote talkers"));
	// Work backwards through array removing the talkers
	for (INT Index = RemoteTalkers.Num() - 1; Index >= 0; Index--)
	{
		const FRemoteTalker& Talker = RemoteTalkers(Index);
		// Remove them from XHV
		DWORD Return = VoiceEngine->UnregisterRemoteTalker(Talker.TalkerId);
		debugf(NAME_DevLive,TEXT("UnregisterRemoteTalker(0x%016I64X) returned 0x%08X"),
			(XUID&)Talker.TalkerId,Return);
	}
	// Empty the array now that they are all unregistered
	RemoteTalkers.Empty();
}

/**
 * Registers all signed in local talkers
 */
void UOnlineSubsystemLive::RegisterLocalTalkers(void)
{
	debugf(NAME_DevLive,TEXT("Registering all local talkers"));
	// Loop through the 4 available players and register them
	for (DWORD Index = 0; Index < 4; Index++)
	{
		// Register the local player as a local talker
		RegisterLocalTalker(Index);
	}
}

/**
 * Unregisters all signed in local talkers
 */
void UOnlineSubsystemLive::UnregisterLocalTalkers(void)
{
	debugf(NAME_DevLive,TEXT("Unregistering all local talkers"));
	// Loop through the 4 available players and unregister them
	for (DWORD Index = 0; Index < 4; Index++)
	{
		// Unregister the local player as a local talker
		UnregisterLocalTalker(Index);
	}
}

/**
 * Parses the read results and copies them to the stats read object
 *
 * @param ReadResults the data to add to the stats object
 */
void UOnlineSubsystemLive::ParseStatsReadResults(XUSER_STATS_READ_RESULTS* ReadResults)
{
	check(CurrentStatsRead && ReadResults);
	// Copy over the view's info
	CurrentStatsRead->TotalRowsInView = ReadResults->pViews->dwTotalViewRows;
	CurrentStatsRead->ViewId = ReadResults->pViews->dwViewId;
	// Now copy each row that was returned
	for (DWORD RowIndex = 0; RowIndex < ReadResults->pViews->dwNumRows; RowIndex++)
	{
		INT NewIndex = CurrentStatsRead->Rows.AddZeroed();
		FOnlineStatsRow& Row = CurrentStatsRead->Rows(NewIndex);
		const XUSER_STATS_ROW& XRow = ReadResults->pViews->pRows[RowIndex];
		// Copy the row data over
		Row.NickName = XRow.szGamertag;
		(XUID&)Row.PlayerID = XRow.xuid;
		// See if they are ranked on the leaderboard or not
		if (XRow.dwRank > 0)
		{
			Row.Rank.SetData((INT)XRow.dwRank);
		}
		else
		{
			Row.Rank.SetData(TEXT("--"));
		}
		// Now allocate our columns
		Row.Columns.Empty(XRow.dwNumColumns);
		Row.Columns.AddZeroed(XRow.dwNumColumns);
		// And copy the columns
		for (DWORD ColIndex = 0; ColIndex < XRow.dwNumColumns; ColIndex++)
		{
			FOnlineStatsColumn& Col = Row.Columns(ColIndex);
			const XUSER_STATS_COLUMN& XCol = XRow.pColumns[ColIndex];
			// Copy the column id and the data object
			Col.ColumnNo = XCol.wColumnId;
			CopyXDataToSettingsData(Col.StatValue,XCol.Value);
			// Handle Live sending "empty" values when they should be zeroed
			if (Col.StatValue.Type == SDT_Empty)
			{
				Col.StatValue.SetData(TEXT("--"));
			}
		}
	}
}

/**
 * Builds the data that we want to read into the Live specific format. Live
 * uses WORDs which script doesn't support, so we can't map directly to it
 *
 * @param DestSpecs the destination stat specs to fill in
 * @param ViewId the view id that is to be used
 * @param Columns the columns that are being requested
 */
void UOnlineSubsystemLive::BuildStatsSpecs(XUSER_STATS_SPEC* DestSpecs,INT ViewId,
	const TArrayNoInit<INT>& Columns)
{
	debugf(NAME_DevLive,TEXT("ViewId = %d, NumCols = %d"),ViewId,Columns.Num());
	// Copy the view data over
	DestSpecs->dwViewId = ViewId;
	DestSpecs->dwNumColumnIds = Columns.Num();
	// Iterate through the columns and copy those over
	// NOTE: These are different types so we can't just memcpy
	for (INT Index = 0; Index < Columns.Num(); Index++)
	{
		DestSpecs->rgwColumnIds[Index] = (WORD)Columns(Index);
		if (bShouldLogStatsData)
		{
			debugf(NAME_DevLive,TEXT("rgwColumnIds[%d] = %d"),Index,Columns(Index));
		}
	}
}

/**
 * Reads a set of stats for the specified list of players
 *
 * @param Players the array of unique ids to read stats for
 * @param StatsRead holds the definitions of the tables to read the data from and
 *		  results are copied into the specified object
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::ReadOnlineStats(const TArray<FUniqueNetId>& Players,
	UOnlineStatsRead* StatsRead)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (CurrentStatsRead == NULL)
	{
		CurrentStatsRead = StatsRead;
		// Clear previous results
		CurrentStatsRead->Rows.Empty();
		// Validate that players were specified
		if (Players.Num() > 0)
		{
			// Create an async task to read the stats and kick that off
			FLiveAsyncTaskReadStats* AsyncTask = new FLiveAsyncTaskReadStats(Players,&__OnReadOnlineStatsComplete__Delegate);
			// Get the read specs so they can be populated
			XUSER_STATS_SPEC* Specs = AsyncTask->GetSpecs();
			// Fill in the Live data
			BuildStatsSpecs(Specs,StatsRead->ViewId,StatsRead->ColumnIds);
			// Copy the player info
			XUID* XPlayers = AsyncTask->GetPlayers();
			DWORD NumPlayers = AsyncTask->GetPlayerCount();
			DWORD BufferSize = 0;
			// First time through figure out how much memory to allocate for search results
			Return = XUserReadStats(0,
				NumPlayers,
				XPlayers,
				1,
				Specs,
				&BufferSize,
				NULL,
				NULL);
			if (Return == ERROR_INSUFFICIENT_BUFFER && BufferSize > 0)
			{
				// Allocate the results buffer
				AsyncTask->AllocateResults(BufferSize);
				// Now kick off the async read
				Return = XUserReadStats(0,
					NumPlayers,
					XPlayers,
					1,
					Specs,
					&BufferSize,
					AsyncTask->GetReadResults(),
					*AsyncTask);
				debugf(NAME_DevLive,
					TEXT("XUserReadStats(0,%d,Players,1,Specs,%d,Buffer,Overlapped) returned 0x%08X"),
					NumPlayers,BufferSize,Return);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					CurrentStatsRead = NULL;
					// Just trigger the delegate as having failed
					FAsyncTaskDelegateResults Results(Return);
					ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
					// Don't leak the task/data
					delete AsyncTask;
				}
			}
			else
			{
				CurrentStatsRead = NULL;
				// Just trigger the delegate as having failed
				FAsyncTaskDelegateResults Results(Return);
				ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
				delete AsyncTask;
				debugf(NAME_Error,
					TEXT("Failed to determine buffer size needed for stats read 0x%08X"),
					Return);
			}
		}
		else
		{
			CurrentStatsRead = NULL;
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
			debugf(NAME_Error,TEXT("Can't read stats for zero players"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't perform a stats read while one is in progress"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Reads a player's stats and all of that player's friends stats for the
 * specified set of stat views. This allows you to easily compare a player's
 * stats to their friends.
 *
 * @param LocalUserNum the local player having their stats and friend's stats read for
 * @param StatsRead holds the definitions of the tables to read the data from and
 *		  results are copied into the specified object
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::ReadOnlineStatsForFriends(BYTE LocalUserNum,
	UOnlineStatsRead* StatsRead)
{
	if (CurrentStatsRead == NULL)
	{
		// Validate that it won't be outside our friends cache
		if (LocalUserNum >= 0 && LocalUserNum <= 4)
		{
			const FFriendsListCache& Cache = FriendsCache[LocalUserNum];
			TArray<FUniqueNetId> Players;
			// Allocate space for all of the friends plus the player
			Players.AddZeroed(Cache.Friends.Num() + 1);
			// Copy the player into the first
			XUserGetXUID(LocalUserNum,(XUID*)&Players(0));
			// Iterate through the friends list and add them to the list
			for (INT Index = 0; Index < Cache.Friends.Num(); Index++)
			{
				Players(Index + 1) = Cache.Friends(Index).UniqueId;
			}
			// Now use the common method to read the stats
			return ReadOnlineStats(Players,StatsRead);
		}
		else
		{
			debugf(NAME_Error,TEXT("Invalid player index specified %d"),
				(DWORD)LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't perform a stats read while one is in progress"));
	}
	return FALSE;
}

/**
 * Reads stats by ranking. This grabs the rows starting at StartIndex through
 * NumToRead and places them in the StatsRead object.
 *
 * @param StatsRead holds the definitions of the tables to read the data from and
 *		  results are copied into the specified object
 * @param StartIndex the starting rank to begin reads at (1 for top)
 * @param NumToRead the number of rows to read (clamped at 100 underneath)
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::ReadOnlineStatsByRank(UOnlineStatsRead* StatsRead,
	INT StartIndex,INT NumToRead)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (CurrentStatsRead == NULL)
	{
		CurrentStatsRead = StatsRead;
		// Clear previous results
		CurrentStatsRead->Rows.Empty();
		FLiveAsyncTaskReadStatsByRank* AsyncTask = new FLiveAsyncTaskReadStatsByRank(&__OnReadOnlineStatsComplete__Delegate);
		DWORD BufferSize = 0;
		HANDLE hEnumerate = NULL;
		// Get the read specs so they can be populated
		XUSER_STATS_SPEC* Specs = AsyncTask->GetSpecs();
		// Fill in the Live data
		BuildStatsSpecs(Specs,StatsRead->ViewId,StatsRead->ColumnIds);
		// Figure out how much space is needed
		Return = XUserCreateStatsEnumeratorByRank(0,
			StartIndex,
			NumToRead,
			1,
			Specs,
			&BufferSize,
			&hEnumerate);
		debugf(NAME_DevLive,
			TEXT("XUserCreateStatsEnumeratorByRank(0,%d,%d,1,Specs,OutSize,OutHandle) returned 0x%08X"),
			StartIndex,NumToRead,Return);
		if (Return == ERROR_SUCCESS)
		{
			AsyncTask->Init(hEnumerate,BufferSize);
			// Start the async enumeration
			Return = XEnumerate(hEnumerate,
				AsyncTask->GetReadResults(),
				BufferSize,
				NULL,
				*AsyncTask);
			debugf(NAME_DevLive,
				TEXT("XEnumerate(hEnumerate,Data,%d,Data,Overlapped) returned 0x%08X"),
				BufferSize,Return);
			if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
			{
				AsyncTasks.AddItem(AsyncTask);
			}
			else
			{
				// Just trigger the delegate as having failed
				FAsyncTaskDelegateResults Results(Return);
				ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
				// Don't leak the task/data
				delete AsyncTask;
			}
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
			delete AsyncTask;
			debugf(NAME_Error,
				TEXT("Failed to determine buffer size needed for stats enumeration 0x%08X"),
				Return);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't perform a stats read while one is in progress"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Reads stats by ranking centered around a player. This grabs a set of rows
 * above and below the player's current rank
 *
 * @param LocalUserNum the local player having their stats being centered upon
 * @param StatsRead holds the definitions of the tables to read the data from and
 *		  results are copied into the specified object
 * @param NumRows the number of rows to read above and below the player's rank
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::ReadOnlineStatsByRankAroundPlayer(BYTE LocalUserNum,
	UOnlineStatsRead* StatsRead,INT NumRows)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	if (CurrentStatsRead == NULL)
	{
		CurrentStatsRead = StatsRead;
		// Validate the index
		if (LocalUserNum >= 0 && LocalUserNum <= 4)
		{
			XUID Player;
			// Get the XUID of the player in question
			XUserGetXUID(LocalUserNum,&Player);
			FLiveAsyncTaskReadStatsByRank* AsyncTask = new FLiveAsyncTaskReadStatsByRank(&__OnReadOnlineStatsComplete__Delegate);
			DWORD BufferSize = 0;
			HANDLE hEnumerate = NULL;
			// Get the read specs so they can be populated
			XUSER_STATS_SPEC* Specs = AsyncTask->GetSpecs();
			// Fill in the Live data
			BuildStatsSpecs(Specs,StatsRead->ViewId,StatsRead->ColumnIds);
			// Figure out how much space is needed
			Return = XUserCreateStatsEnumeratorByXuid(0,
				Player,
				NumRows,
				1,
				Specs,
				&BufferSize,
				&hEnumerate);
			debugf(NAME_DevLive,
				TEXT("XUserCreateStatsEnumeratorByXuid(0,%d,%d,1,Specs,OutSize,OutHandle) returned 0x%08X"),
				(DWORD)LocalUserNum,NumRows,Return);
			if (Return == ERROR_SUCCESS)
			{
				AsyncTask->Init(hEnumerate,BufferSize);
				// Start the async enumeration
				Return = XEnumerate(hEnumerate,
					AsyncTask->GetReadResults(),
					BufferSize,
					NULL,
					*AsyncTask);
				debugf(NAME_DevLive,
					TEXT("XEnumerate(hEnumerate,Data,%d,Data,Overlapped) returned 0x%08X"),
					BufferSize,Return);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Just trigger the delegate as having failed
					FAsyncTaskDelegateResults Results(Return);
					ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
					// Don't leak the task/data
					delete AsyncTask;
				}
			}
			else
			{
				// Just trigger the delegate as having failed
				FAsyncTaskDelegateResults Results(Return);
				ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
				delete AsyncTask;
				debugf(NAME_Error,
					TEXT("Failed to determine buffer size needed for stats enumeration 0x%08X"),
					Return);
			}
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnReadOnlineStatsComplete__Delegate,&Results);
			debugf(NAME_Error,TEXT("Invalid player index specified %d"),
				(DWORD)LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't perform a stats read while one is in progress"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Copies the stats data from our Epic form to something Live can handle
 *
 * @param Stats the destination buffer the stats are written to
 * @param Properties the Epic structures that need copying over
 * @param RatingId the id to set as the rating for this leaderboard set
 */
void UOnlineSubsystemLive::CopyStatsToProperties(XUSER_PROPERTY* Stats,
	const TArray<FSettingsProperty>& Properties,const DWORD RatingId)
{
	check(Properties.Num() < 64);
	// Copy each Epic property into the Live form
	for (INT Index = 0; Index < Properties.Num(); Index++)
	{
		const FSettingsProperty& Property = Properties(Index);
		// Assign the values
		Stats[Index].dwPropertyId = Property.PropertyId;
		Stats[Index].value.type = Property.Data.Type;
		// Do per type init
		switch (Property.Data.Type)
		{
			case SDT_Int32:
			{
				// Determine if this property needs to be promoted
				if (Property.PropertyId != RatingId)
				{
					Property.Data.GetData((INT&)Stats[Index].value.nData);
				}
				else
				{
					INT Value;
					// Promote this value from Int32 to 64
					Property.Data.GetData(Value);
					Stats[Index].value.i64Data = (QWORD)Value;
					Stats[Index].value.type = SDT_Int64;
				}
				break;
			}
			case SDT_Double:
			{
				Property.Data.GetData(Stats[Index].value.dblData);
				break;
			}
			case SDT_Int64:
			{
				Property.Data.GetData((QWORD&)Stats[Index].value.i64Data);
				break;
			}
			default:
			{
				Stats[Index].value.type = 0;
				debugf(NAME_DevLive,
					TEXT("Ignoring stat type %d at index %d as it is unsupported by Live"),
					Property.Data.Type,Index);
				break;
			}
		}
		if (bShouldLogStatsData)
		{
			debugf(NAME_DevLive,TEXT("Writing stat (%d) of type (%d) with value %s"),
				Property.PropertyId,Property.Data.Type,*Property.Data.ToString());
		}
	}
}

/**
 * Cleans up any platform specific allocated data contained in the stats data
 *
 * @param StatsRead the object to handle per platform clean up on
 */
void UOnlineSubsystemLive::FreeStats(UOnlineStatsRead* StatsRead)
{
	check(StatsRead);
	// We just empty these without any special platform data...yet
	StatsRead->Rows.Empty();
}

/**
 * Writes out the stats contained within the stats write object to the online
 * subsystem's cache of stats data. Note the new data replaces the old. It does
 * not write the data to the permanent storage until a FlushOnlineStats() call
 * or a session ends. Stats cannot be written without a session or the write
 * request is ignored. No more than 5 stats views can be written to at a time
 * or the write request is ignored.
 *
 * @param Player the player to write stats for
 * @param StatsWrite the object containing the information to write
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::WriteOnlineStats(FUniqueNetId Player,
	UOnlineStatsWrite* StatsWrite)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Error if there isn't a session going
	if (GameSettings && SessionInfo && SessionInfo->Handle)
	{
		if (StatsWrite != NULL)
		{
#if !FINAL_RELEASE
			// Validate the number of views and the number of stats per view
			if (IsValidStatsWrite(StatsWrite))
#endif
			{
				if (bShouldLogStatsData)
				{
					debugf(NAME_DevLive,TEXT("Writing stats object type %s"),
						*StatsWrite->GetClass()->GetName());
				}
				// Allocate the object that holds the async data
				FLiveAsyncTaskWriteStats* AsyncTask = new FLiveAsyncTaskWriteStats(NULL);
				// Get access to the async data buffers
				XSESSION_VIEW_PROPERTIES* Views = AsyncTask->GetViews();
				XUSER_PROPERTY* Stats = AsyncTask->GetStats();
				// Copy stats properties to the Live data
				CopyStatsToProperties(Stats,StatsWrite->Properties,StatsWrite->RatingId);
				// Get the number of views/stats involved
				DWORD StatsCount = StatsWrite->Properties.Num();
				// Switch upon the arbitration setting which set of view ids to use
				DWORD ViewCount = GameSettings->bUsesArbitration ?
					StatsWrite->ArbitratedViewIds.Num() :
					StatsWrite->ViewIds.Num();
				INT* ViewIds = GameSettings->bUsesArbitration ?
					(INT*)StatsWrite->ArbitratedViewIds.GetData() :
					(INT*)StatsWrite->ViewIds.GetData();
				// Initialize the view data for each view involved
				for (DWORD Index = 0; Index < ViewCount; Index++)
				{
					Views[Index].dwViewId = ViewIds[Index];
					Views[Index].dwNumProperties = StatsCount;
					Views[Index].pProperties = Stats;
					if (bShouldLogStatsData)
					{
						debugf(NAME_DevLive,TEXT("ViewId = %d, NumProps = %d"),ViewIds[Index],StatsCount);
					}
				}
				// Kick off the async write
				Return = XSessionWriteStats(SessionInfo->Handle,
					(XUID&)Player,
					ViewCount,
					Views,
					*AsyncTask);
				debugf(NAME_DevLive,TEXT("XSessionWriteStats() return 0x%08X"),Return);
				if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
				{
					AsyncTasks.AddItem(AsyncTask);
				}
				else
				{
					// Don't leak the task/data
					delete AsyncTask;
				}
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't write stats using a null object"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't write stats without a Live session in progress"));
	}
	return Return == ERROR_SUCCESS;
}

/**
 * Commits any changes in the online stats cache to the permanent storage
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemLive::FlushOnlineStats(void)
{
	DWORD Return = XONLINE_E_SESSION_WRONG_STATE;
	// Error if there isn't a session going
	if (GameSettings && SessionInfo && SessionInfo->Handle)
	{
		// Allocate an async task for deferred calling of the delegates
		FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(&__OnFlushOnlineStatsComplete__Delegate,NULL,TEXT("XSessionFlushStats()"));
		// Show the live guide ui for inputing text
		Return = XSessionFlushStats(SessionInfo->Handle,*AsyncTask);
		if (Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING)
		{
			AsyncTasks.AddItem(AsyncTask);
		}
		else
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResults Results(Return);
			ProcessDelegate(NAME_None,&__OnFlushOnlineStatsComplete__Delegate,&Results);
			// Don't leak the task/data
			delete AsyncTask;
			debugf(NAME_Error,TEXT("XSessionFlushStats() failed with 0x%08X"),Return);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't flush stats without a Live session in progress"));
	}
	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

/**
 * Changes the number of public/private slots for the session
 *
 * @param PublicSlots the number of public slots to use
 * @param PrivateSlots the number of private slots to use
 */
void UOnlineSubsystemLive::ModifySessionSize(DWORD PublicSlots,DWORD PrivateSlots)
{
	check(GameSettings && SessionInfo && GameSettings->bUsesArbitration);
	// We must pass the same flags in (zero doesn't mean please don't change stuff)
	DWORD SessionFlags = BuildSessionFlags(GameSettings);
	// Update the number of public slots on the game settings
	GameSettings->NumPublicConnections = PublicSlots;
	GameSettings->NumOpenPublicConnections = PublicSlots - 1;
	GameSettings->NumPrivateConnections = PrivateSlots;
	GameSettings->NumOpenPrivateConnections = PrivateSlots;
	// Allocate the object that will hold the data throughout the async lifetime
	FLiveAsyncTask* AsyncTask = new FLiveAsyncTask(NULL,NULL,TEXT("XSessionModify()"));
	// Tell Live to shrink to the number of public/private specified
	DWORD Result = XSessionModify(SessionInfo->Handle,
		SessionFlags,
		PublicSlots,
		PrivateSlots,
		*AsyncTask);
	debugf(NAME_DevLive,TEXT("Changing session size to public = %d, private = %d XSessionModify() returned 0x%08X"),
		PublicSlots,PrivateSlots,Result);
	// Clean up the task if it didn't succeed
	if (Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING)
	{
		// Queue the async task for ticking
		AsyncTasks.AddItem(AsyncTask);
	}
	else
	{
		// Just trigger the delegate as having failed
		delete AsyncTask;
	}
}

/**
 * Shrinks the session to the number of arbitrated registrants
 */
void UOnlineSubsystemLive::ShrinkToArbitratedRegistrantSize(void)
{
	ModifySessionSize(ArbitrationList.Num(),0);
}
