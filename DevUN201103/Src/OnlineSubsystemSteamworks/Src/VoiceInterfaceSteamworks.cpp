/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemSteamworks.h"
#include "VoiceInterfaceCommon.h"

#if WITH_UE3_NETWORKING && WITH_STEAMWORKS

#include "EngineSoundClasses.h"
#include "EngineAudioDeviceClasses.h"

#define INVALID_OWNING_INDEX ((DWORD)-1)

struct FQueuedSteamworksVoice
{
	FQueuedSteamworksVoice() : Generation(0), Bytes(0), LastSeen(0.0) {}
	TArray<BYTE> Data;
	WORD Generation;
	WORD Bytes;
	DOUBLE LastSeen;
};

/**
 * This interface is an abstract mechanism for getting voice data. Each platform
 * implements a specific version of this interface.
 */
class FVoiceInterfaceSteamworks :
	public FVoiceInterfaceCommon
{
	/** Singleton instance pointer */
	static FVoiceInterfaceSteamworks* GVoiceInterface;

	typedef TMap<QWORD,UAudioComponent*> TalkingUserMap;

	/** The user that owns the voice devices (Unreal user index for splitscreen, not Steam user) */
	DWORD OwningIndex;
	/** Whether Steamworks let us use the microphone or not. */
	UBOOL bWasStartedOk;
	/** Whether the app is asking Steamworks to capture audio. */
	UBOOL bIsCapturing;
	/** Decompressed audio packets go here. This just helps prevent constant allocation. */
	TArray<BYTE> DecompressBuffer;
	/** Map users to time when their audio playback will end. */
	TalkingUserMap TalkingUsers;
	/** Consistent, growable storage for reading raw audio samples. */
	TArray<BYTE> RawSamples;

	typedef TMap<QWORD,FQueuedSteamworksVoice> VoiceFragmentsMap;
	/** Data from Steamworks, waiting to send to network. */
	FQueuedSteamworksVoice QueuedSendData;
	/** Data from network, waiting to send to Steamworks. */
	VoiceFragmentsMap QueuedReceiveData;

	/** A pool of audio components; assigned to incoming voice data as playback is needed. */
	//TArray<UAudioComponent*> AudioComponentPool;

	/** Simple constructor that zeros members. Hidden due to factory method */
	FVoiceInterfaceSteamworks( void )
		: OwningIndex(INVALID_OWNING_INDEX)
		, bWasStartedOk(FALSE)
		, bIsCapturing(FALSE)
	{
	}

	/**
	 * Creates the Steamworks voice engine
	 *
	 * @param MaxLocalTalkers the maximum number of local talkers to support
	 * @param MaxRemoteTalkers the maximum number of remote talkers to support
	 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled
	 *
	 * @return TRUE if everything initialized correctly, FALSE otherwise
	 */
	virtual UBOOL Init( INT MaxLocalTalkers, INT MaxRemoteTalkers, UBOOL bIsSpeechRecognitionDesired );

	/**
	 * Determines if the specified index is the owner or not
	 *
	 * @param InIndex the index being tested
	 *
	 * @return TRUE if this is the owner, FALSE otherwise
	 */
	FORCEINLINE UBOOL IsOwningIndex(DWORD InIndex)
	{
		return InIndex >= 0 && InIndex < MAX_SPLITSCREEN_TALKERS && OwningIndex == InIndex;
	}

public:

	/** Destructor that releases the engine if allocated */
	virtual ~FVoiceInterfaceSteamworks( void )
	{
		if (bIsCapturing)
		{
			// Steam doesn't actually close the device, as it still uses it in the
			//  overlay for Steam Community chat, etc.
			GSteamUser->StopVoiceRecording();
			GSteamFriends->SetInGameVoiceSpeaking(GSteamUser->GetSteamID(), false);
		}
	}

	/**
	 * Returns an instance of the singleton object
	 *
	 * @param MaxLocalTalkers the maximum number of local talkers to support
	 * @param MaxRemoteTalkers the maximum number of remote talkers to support
	 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled
	 *
	 * @return A pointer to the singleton object or NULL if failed to init
	 */
	static FVoiceInterfaceSteamworks* CreateInstance( INT MaxLocalTalkers, INT MaxRemoteTalkers, UBOOL bIsSpeechRecognitionDesired )
	{
		if( GVoiceInterface == NULL )
		{
			GVoiceInterface = new FVoiceInterfaceSteamworks();
			// Init the Steamworks engine with those defaults
			if( GVoiceInterface->Init( MaxLocalTalkers, MaxRemoteTalkers, bIsSpeechRecognitionDesired ) == FALSE )
			{
				delete GVoiceInterface;
				GVoiceInterface = NULL;
			}
		}
		return GVoiceInterface;
	}

// FVoiceInterface

	/**
	 * Starts local voice processing for the specified user index
	 *
	 * @param UserIndex the user to start processing for
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD StartLocalVoiceProcessing(DWORD UserIndex);

	/**
	 * Stops local voice processing for the specified user index
	 *
	 * @param UserIndex the user to stop processing of
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD StopLocalVoiceProcessing(DWORD UserIndex);

	/**
	 * Starts remote voice processing for the specified user
	 *
	 * @param UniqueId the unique id of the user that will be talking
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD StartRemoteVoiceProcessing(FUniqueNetId UniqueId);

	/**
	 * Stops remote voice processing for the specified user
	 *
	 * @param UniqueId the unique id of the user that will no longer be talking
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD StopRemoteVoiceProcessing(FUniqueNetId UniqueId);

	/**
	 * Tells the voice system to start tracking voice data for speech recognition
	 *
	 * @param UserIndex the local user to recognize voice data for
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD StartSpeechRecognition(DWORD UserIndex);

	/**
	 * Tells the voice system to stop tracking voice data for speech recognition
	 *
	 * @param UserIndex the local user to recognize voice data for
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD StopSpeechRecognition(DWORD UserIndex);

	/**
	 * Registers the user index as a local talker (interested in voice data)
	 *
	 * @param UserIndex the user index that is going to be a talker
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD RegisterLocalTalker(DWORD UserIndex);

	/**
	 * Unregisters the user index as a local talker (not interested in voice data)
	 *
	 * @param UserIndex the user index that is no longer a talker
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD UnregisterLocalTalker(DWORD UserIndex);

	/**
	 * Registers the unique player id as a remote talker (submitted voice data only)
	 *
	 * @param UniqueId the id of the remote talker
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD RegisterRemoteTalker(FUniqueNetId UniqueId);

	/**
	 * Unregisters the unique player id as a remote talker
	 *
	 * @param UniqueId the id of the remote talker
	 *
	 * @return 0 upon success, an error code otherwise
	 */
    virtual DWORD UnregisterRemoteTalker(FUniqueNetId UniqueId);

	/**
	 * Checks whether a local user index has a headset present or not
	 *
	 * @param UserIndex the user to check status for
	 *
	 * @return TRUE if there is a headset, FALSE otherwise
	 */
    virtual UBOOL IsHeadsetPresent(DWORD UserIndex);

	/**
	 * Determines whether a local user index is currently talking or not
	 *
	 * @param UserIndex the user to check status for
	 *
	 * @return TRUE if the user is talking, FALSE otherwise
	 */
    virtual UBOOL IsLocalPlayerTalking(DWORD UserIndex);

	/**
	 * Determines whether a remote talker is currently talking or not
	 *
	 * @param UniqueId the unique id of the talker to check status on
	 *
	 * @return TRUE if the user is talking, FALSE otherwise
	 */
	virtual UBOOL IsRemotePlayerTalking(FUniqueNetId UniqueId);

	/**
	 * Returns which local talkers have data ready to be read from the voice system
	 *
	 * @return Bit mask of talkers that have data to be read (1 << UserIndex)
	 */
	virtual DWORD GetVoiceDataReadyFlags(void);

	/**
	 * Sets the playback priority of a remote talker for the given user. A
	 * priority of 0xFFFFFFFF indicates that the player is muted. All other
	 * priorities sorted from zero being most important to higher numbers
	 * being less important.
	 *
	 * @param UserIndex the local talker that is setting the priority
	 * @param UniqueId the id of the remote talker that is having priority changed
	 * @param Priority the new priority to apply to the talker
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD SetPlaybackPriority(DWORD UserIndex,FUniqueNetId RemoteTalkerId,DWORD Priority);

	/**
	 * Reads local voice data for the specified local talker. The size field
	 * contains the buffer size on the way in and contains the amount read on
	 * the way out
	 *
	 * @param UserIndex the local talker that is having their data read
	 * @param Data the buffer to copy the voice data into
	 * @param Size in: the size of the buffer, out: the amount of data copied
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD ReadLocalVoiceData(DWORD UserIndex,BYTE* Data,DWORD* Size);

	/**
	 * Submits remote voice data for playback by the voice system. No playback
	 * occurs if the priority for this remote talker is 0xFFFFFFFF. Size
	 * indicates how much data to submit for processing. It's also an out
	 * value in case the system could only process a smaller portion of the data
	 *
	 * @param RemoteTalkerId the remote talker that sent this data
	 * @param Data the buffer to copy the voice data into
	 * @param Size in: the size of the buffer, out: the amount of data copied
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD SubmitRemoteVoiceData(FUniqueNetId RemoteTalkerId,BYTE* Data,DWORD* Size);

	/**
	 * Allows for platform specific servicing of devices, etc.
	 *
	 * @param DeltaTime the amount of time that has elapsed since the last update
	 */
	virtual void Tick(FLOAT DeltaTime);
};

/** Singleton instance pointer initialization */
FVoiceInterfaceSteamworks* FVoiceInterfaceSteamworks::GVoiceInterface = NULL;

/**
 * Platform specific method for creating the voice interface to use for all
 * voice data/communication
 *
 * @param MaxLocalTalkers the number of local talkers to handle voice for
 * @param MaxRemoteTalkers the number of remote talkers to handle voice for
 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled or not
 *
 * @return The voice interface to use
 */
FVoiceInterface* appCreateVoiceInterface(INT MaxLocalTalkers,INT MaxRemoteTalkers,UBOOL bIsSpeechRecognitionDesired)
{
	return FVoiceInterfaceSteamworks::CreateInstance(MaxLocalTalkers,MaxRemoteTalkers,bIsSpeechRecognitionDesired);
}

/**
 * Creates the Steamworks voice engine and performs any other initialization
 *
 * @param MaxLocalTalkers the maximum number of local talkers to support
 * @param MaxRemoteTalkers the maximum number of remote talkers to support
 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled
 *
 * @return TRUE if everything initialized correctly, FALSE otherwise
 */
UBOOL FVoiceInterfaceSteamworks::Init( INT MaxLocalTalkers, INT MaxRemoteTalkers, UBOOL bIsSpeechRecognitionDesired )
{
	if (!GSteamworksInitialized)
	{
		return FALSE;   // nothing to be done.
	}

	// Ask the INI file if voice is wanted
	UBOOL bHasVoiceEnabled = FALSE;
	if (GConfig->GetBool(TEXT("VoIP"),TEXT("bHasVoiceEnabled"),bHasVoiceEnabled,GEngineIni))
	{
		// Check to see if voice is enabled/disabled
		if (bHasVoiceEnabled == FALSE)
		{
			return FALSE;
		}
	}
	// Skip if a dedicated server
	if (!GIsClient)
	{
		debugf(NAME_DevOnline,TEXT("Skipping voice initialization for dedicated server"));
		return FALSE;
	}

	// This is a little hacky, but we're just making sure we didn't have a complete failure of the ISteamUser voice interface.
	//  In theory, this could fail if there's no audio hardware or something else completely unexpected.
	uint32 CompressedBytes, RawBytes;
	GSteamUser->StartVoiceRecording();
	bWasStartedOk = (GSteamUser->GetAvailableVoice(&CompressedBytes, &RawBytes) != k_EVoiceResultNotInitialized);
	GSteamUser->StopVoiceRecording();

	if (!bWasStartedOk)
	{
		debugf( NAME_DevOnline, TEXT( "Steamworks voice recording failed!" ) );
		return FALSE;
	}

	return FVoiceInterfaceCommon::Init( MaxLocalTalkers, MaxRemoteTalkers, bIsSpeechRecognitionDesired );
}

/**
 * Starts local voice processing for the specified user index
 *
 * @param UserIndex the user to start processing for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StartLocalVoiceProcessing(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		if ((!bIsCapturing) && (bWasStartedOk))
		{
			GSteamUser->StartVoiceRecording();
			GSteamFriends->SetInGameVoiceSpeaking(GSteamUser->GetSteamID(), true);
			bIsCapturing = TRUE;
		}
		return S_OK;
	}
	else
	{
		debugf(NAME_Error,TEXT("StartLocalVoiceProcessing(): Device is currently owned by another user"));
	}
	return E_FAIL;
}

/**
 * Stops local voice processing for the specified user index
 *
 * @param UserIndex the user to stop processing of
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StopLocalVoiceProcessing(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		if (bIsCapturing)
		{
			GSteamUser->StopVoiceRecording();
			GSteamFriends->SetInGameVoiceSpeaking(GSteamUser->GetSteamID(), false);
			bIsCapturing = FALSE;
		}
		return S_OK;
	}
	else
	{
		debugf(NAME_Error,TEXT("StopLocalVoiceProcessing: Ignoring stop request for non-owning user"));
	}
	return E_FAIL;
}

/**
 * Starts remote voice processing for the specified user
 *
 * @param UniqueId the unique id of the user that will be talking
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StartRemoteVoiceProcessing(FUniqueNetId UniqueId)
{
	return S_OK;  // No-op in Steamworks (GameSpy, too).
}

/**
 * Stops remote voice processing for the specified user
 *
 * @param UniqueId the unique id of the user that will no longer be talking
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StopRemoteVoiceProcessing(FUniqueNetId UniqueId)
{
	return S_OK;  // No-op in Steamworks (GameSpy, too).
}

/**
 * Tells the voice system to start tracking voice data for speech recognition
 *
 * @param UserIndex the local user to recognize voice data for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StartSpeechRecognition(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		StartLocalVoiceProcessing(UserIndex);
		return FVoiceInterfaceCommon::StartSpeechRecognition(UserIndex);
	}
	return E_FAIL;
}

/**
 * Tells the voice system to stop tracking voice data for speech recognition
 *
 * @param UserIndex the local user to recognize voice data for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::StopSpeechRecognition(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		StopLocalVoiceProcessing(UserIndex);
		return FVoiceInterfaceCommon::StopSpeechRecognition(UserIndex);
	}
	return E_FAIL;
}

/**
 * Registers the user index as a local talker (interested in voice data)
 *
 * @param UserIndex the user index that is going to be a talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::RegisterLocalTalker(DWORD UserIndex)
{
	if (OwningIndex == INVALID_OWNING_INDEX)
	{
		OwningIndex = UserIndex;
		return S_OK;
	}
	return E_FAIL;
}

/**
 * Unregisters the user index as a local talker (not interested in voice data)
 *
 * @param UserIndex the user index that is no longer a talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::UnregisterLocalTalker(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		OwningIndex = INVALID_OWNING_INDEX;
		return S_OK;
	}
	return E_FAIL;
}

/**
 * Registers the unique player id as a remote talker (submitted voice data only)
 *
 * @param UniqueId the id of the remote talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::RegisterRemoteTalker(FUniqueNetId UniqueId)
{
	return S_OK;  // No-op in Steamworks (GameSpy, too).
}

/**
 * Unregisters the unique player id as a remote talker
 *
 * @param UniqueId the id of the remote talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::UnregisterRemoteTalker(FUniqueNetId UniqueId)
{
	return S_OK;  // No-op in Steamworks (GameSpy, too).
}

/**
 * Checks whether a local user index has a headset present or not
 *
 * @param UserIndex the user to check status for
 *
 * @return TRUE if there is a headset, FALSE otherwise
 */
UBOOL FVoiceInterfaceSteamworks::IsHeadsetPresent(DWORD UserIndex)
{
	if (IsOwningIndex(UserIndex))
	{
		return S_OK;
	}
	return E_FAIL;
}

/**
 * Determines whether a local user index is currently talking or not
 *
 * @param UserIndex the user to check status for
 *
 * @return TRUE if the user is talking, FALSE otherwise
 */
UBOOL FVoiceInterfaceSteamworks::IsLocalPlayerTalking(DWORD UserIndex)
{
    return ((GetVoiceDataReadyFlags() & (UserIndex << 1)) != 0);
}

/**
 * Determines whether a remote talker is currently talking or not
 *
 * @param UniqueId the unique id of the talker to check status on
 *
 * @return TRUE if the user is talking, FALSE otherwise
 */
UBOOL FVoiceInterfaceSteamworks::IsRemotePlayerTalking(FUniqueNetId UniqueId)
{
	// We keep track of how many milliseconds of audio we submitted per-user.
	return (TalkingUsers.Find(UniqueId.Uid) != NULL);
}

/**
 * Returns which local talkers have data ready to be read from the voice system
 *
 * @return Bit mask of talkers that have data to be read (1 << UserIndex)
 */
DWORD FVoiceInterfaceSteamworks::GetVoiceDataReadyFlags(void)
{
	if ((OwningIndex != INVALID_OWNING_INDEX) && (bIsCapturing))
	{
		uint32 CompressedBytes, RawBytes;
		const EVoiceResult Result = GSteamUser->GetAvailableVoice(&CompressedBytes, &RawBytes);
		if ((Result == k_EVoiceResultOK) && (CompressedBytes > 0))
		{
			return 1 << OwningIndex;
		}
	}

	return 0;
}

/**
 * Sets the playback priority of a remote talker for the given user. A
 * priority of 0xFFFFFFFF indicates that the player is muted. All other
 * priorities sorted from zero being most important to higher numbers
 * being less important.
 *
 * @param UserIndex the local talker that is setting the priority
 * @param UniqueId the id of the remote talker that is having priority changed
 * @param Priority the new priority to apply to the talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::SetPlaybackPriority(DWORD UserIndex,FUniqueNetId RemoteTalkerId,DWORD Priority)
{
	return S_OK;   // unsupported in Steamworks (and GameSpy).
}

/**
 * Reads local voice data for the specified local talker. The size field
 * contains the buffer size on the way in and contains the amount read on
 * the way out
 *
 * @param UserIndex the local talker that is having their data read
 * @param Data the buffer to copy the voice data into
 * @param Size in: the size of the buffer, out: the amount of data copied
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::ReadLocalVoiceData(DWORD UserIndex,BYTE* Data,DWORD* Size)
{
	INT Available = QueuedSendData.Data.Num() - QueuedSendData.Bytes;
	if ((OwningIndex == UserIndex) && ((bIsCapturing) || (Available > 0)))
	{
		// Steamworks gives us data way bigger than *Size (2k vs 100 bytes), so we cache it, and
		//  split it into chunks here. The whole chunk has to arrive for playback, or we drop it,
		//  which isn't ideal, but it works well enough.
		if (Available == 0)
		{
			QueuedSendData.Data.Empty();
			QueuedSendData.Generation++;
			QueuedSendData.Bytes = 0;
			if (QueuedSendData.Generation == 0)
			{
				QueuedSendData.Generation = 1;   // zero is never valid.
			}

			uint32 CompressedBytes = 0;
			uint32 RawBytes = 0;
			EVoiceResult VoiceResult = GSteamUser->GetAvailableVoice(&CompressedBytes, &RawBytes);
			if (VoiceResult != k_EVoiceResultOK)
			{
				return E_FAIL;
			}
			
			// This shouldn't happen, but just in case.
			if ((CompressedBytes == 0) || (RawBytes == 0))
			{
				*Size = 0;
				return S_OK;
			}
		
			Available = CompressedBytes;
			QueuedSendData.Data.AddZeroed(Available);
			RawSamples.Empty(RawBytes);
			RawSamples.Add(RawBytes);
			VoiceResult = GSteamUser->GetVoice(true, &QueuedSendData.Data(0), Available, &CompressedBytes, true, RawSamples.GetData(), RawBytes, &RawBytes);
			if (VoiceResult != k_EVoiceResultOK)
			{
				return E_FAIL;
			}

			// !!! FIXME: we need Steamworks to give us data at 16KHz--not 11025Hz--for Fonix to process. So this is wrong at the moment.
			RawVoiceDataCallback( OwningIndex, (SWORD*)RawSamples.GetData(), RawBytes / sizeof (SWORD) );
		}

		if (Available <= 0)
		{
			*Size = 0;
		}
		else
		{
			// This should probably use serialization.
			const INT MetadataLen = sizeof (WORD) + sizeof (WORD) + sizeof (WORD);
			check(*Size > MetadataLen);
			WORD *WordData = (WORD *) Data;
			*(WordData++) = (WORD) QueuedSendData.Generation;
			*(WordData++) = (WORD) QueuedSendData.Bytes;
			*(WordData++) = (WORD) QueuedSendData.Data.Num();
			const INT CopyLen = Min<INT>(*Size - MetadataLen, Available);
			appMemcpy(WordData, &QueuedSendData.Data(QueuedSendData.Bytes), CopyLen);
			QueuedSendData.Bytes += CopyLen;
			*Size = CopyLen + MetadataLen;
		}

		return S_OK;
	}

	return E_FAIL;
}

/**
 * Submits remote voice data for playback by the voice system. No playback
 * occurs if the priority for this remote talker is 0xFFFFFFFF. Size
 * indicates how much data to submit for processing. It's also an out
 * value in case the system could only process a smaller portion of the data
 *
 * @param RemoteTalkerId the remote talker that sent this data
 * @param Data the buffer to copy the voice data into
 * @param Size in: the size of the buffer, out: the amount of data copied
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceSteamworks::SubmitRemoteVoiceData(FUniqueNetId RemoteTalkerId,BYTE* Data,DWORD* Size)
{
	FQueuedSteamworksVoice *QueuedData = QueuedReceiveData.Find(RemoteTalkerId.Uid);
	if (QueuedData == NULL)
	{
		QueuedReceiveData.Set(RemoteTalkerId.Uid, FQueuedSteamworksVoice());
		QueuedData = QueuedReceiveData.Find(RemoteTalkerId.Uid);
	}

	// This should probably use serialization.
	const INT MetadataLen = sizeof (WORD) + sizeof (WORD) + sizeof (WORD);
	if (*Size < MetadataLen)
	{
		return E_FAIL;
	}

	WORD *WordData = (WORD *) Data;
	const WORD Generation = *(WordData++);
	const WORD Offset = *(WordData++);
	const WORD Total = *(WordData++);

	Data = (BYTE *) WordData;

	if ( (Generation == 0) || (((Offset + *Size) - MetadataLen) > Total) )
	{
		QueuedData->Generation = 0;   // reset next packet.
		return E_FAIL;
	}

	if ((Generation != QueuedData->Generation) || (QueuedData->Data.Num() != Total))
	{
		// new voice packet.
		QueuedData->Data.Empty();
		QueuedData->Data.AddZeroed(Total);
		QueuedData->Generation = Generation;
		QueuedData->Bytes = 0;
		QueuedData->LastSeen = appSeconds();
	}

	appMemcpy(&QueuedData->Data(Offset), Data, *Size - MetadataLen);
	QueuedData->Bytes += (*Size - MetadataLen);

	// Pretend we succeeded if we haven't gotten a complete voice packet reassembled yet...
	//debugf(NAME_DevOnline, TEXT("Have %d bytes of VoIP packet (waiting for %d total)."), (INT) QueuedData->Bytes, (INT) QueuedData->Data.Num());
	if (QueuedData->Bytes != QueuedData->Data.Num())
	{
		return S_OK;
	}

	QueuedData->Generation = 0;   // reset this since we're definitely done getting this packet.

	INT DecompressBytes = (*Size) * 4;
	uint32 BytesWritten = 0;
	while (TRUE)
	{
		// Keep/grow allocation, but mark portion we need as used.
		DecompressBuffer.Empty(DecompressBuffer.Num());
		DecompressBuffer.Add(DecompressBytes);

		const EVoiceResult VoiceResult = GSteamUser->DecompressVoice(&QueuedData->Data(0), QueuedData->Bytes, DecompressBuffer.GetData(), DecompressBytes, &BytesWritten);
		if (VoiceResult == k_EVoiceResultBufferTooSmall)
		{
			DecompressBytes *= 2;  // increase buffer, try again.
			continue;
		}
		else if (VoiceResult != k_EVoiceResultOK)
		{
			*Size = 0;
			return E_FAIL;
		}
		break;
	}

	// flag part we didn't need as unused.
	DecompressBuffer.Remove(BytesWritten, DecompressBuffer.Num() - BytesWritten);

	const QWORD IdValue = RemoteTalkerId.Uid;
	UAudioComponent **ppAudioComponent = TalkingUsers.Find(IdValue);
	UAudioComponent *AudioComponent = (ppAudioComponent ? *ppAudioComponent : NULL);

	if (AudioComponent == NULL || AudioComponent->IsPendingKill())
	{
		// Build a new UAudioComponent, which we'll use to play incoming voice.
		if( GEngine && GEngine->Client && GEngine->Client->GetAudioDevice())
		{
			UAudioDevice *AudioDevice = GEngine->Client->GetAudioDevice();
			USoundCue *SoundCue = ConstructObject<USoundCue>( USoundCue::StaticClass(), UObject::GetTransientPackage() );
			if( SoundCue )
			{
				USoundNodeWaveStreaming* SoundNodeWave = ConstructObject<USoundNodeWaveStreaming>( USoundNodeWaveStreaming::StaticClass(), SoundCue );
				if( SoundNodeWave )
				{
					SoundNodeWave->SampleRate = 11025;  // this is hardcoded in Steamworks.
					SoundNodeWave->NumChannels = 1;

					// Sound data keeps getting pushed into the node, so it has no set duration
					SoundNodeWave->Duration = INDEFINITELY_LOOPING_DURATION;

					SoundCue->SoundClass = FName( TEXT( "VoiceChat" ) );
					SoundCue->FirstNode = SoundNodeWave;
					AudioComponent = AudioDevice->CreateComponent(SoundCue, GWorld->Scene);
					if (AudioComponent)
					{
						TalkingUsers.Set(IdValue, AudioComponent);
					}
				}
			}
		}
	}

	if (AudioComponent == NULL)
	{
		return E_FAIL;
	}

	USoundNodeWaveStreaming *Wave = (USoundNodeWaveStreaming *) AudioComponent->SoundCue->FirstNode;
	Wave->QueueAudio(DecompressBuffer);

	// Start it playing if we haven't yet, and there's at least .25 seconds of audio queued.
	if ((!AudioComponent->IsPlaying()) && (Wave->AvailableAudioBytes() > ((sizeof (WORD) * 11025) / 4)))
	{
		AudioComponent->Play();
	}

	return S_OK;  // okay, we submitted it!
}

/**
 * Voice processing tick function
 */
void FVoiceInterfaceSteamworks::Tick(FLOAT DeltaTime)
{
	// remove users that are done talking.
	for(TalkingUserMap::TIterator It(TalkingUsers); It; ++It)
	{
		UAudioComponent *AudioComponent = It.Value();

		if( !AudioComponent || AudioComponent->IsPendingKill() ||
			!((USoundNodeWaveStreaming*) AudioComponent->SoundCue->FirstNode)->eventAvailableAudioBytes())
		{
			if (AudioComponent && !AudioComponent->IsPendingKill())
				AudioComponent->Stop();

			//AudioComponentPool.Push(AudioComponent);
			It.RemoveCurrent();
		}
	}

	// Remove voice packet fragments for players that haven't sent more data in X seconds.
	const DOUBLE Now = appSeconds();
	for(VoiceFragmentsMap::TIterator It(QueuedReceiveData); It; ++It)
	{
		// We are aggressive and dump the audio if it's been more than 1 second; a given
		//  packet is way less than a second, so you'd be having serious skips in the audio
		//  by this point anyhow.
		if((Now - It.Value().LastSeen) >= 1.0)
		{
			It.RemoveCurrent();
		}
	}
}

#endif	//#if WITH_UE3_NETWORKING && WITH_STEAMWORKS
