/**
 * Copyright (c) 2006 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemLive.h"
#include "VoiceInterface.h"

/**
 * This interface is an abstract mechanism for getting voice data. Each platform
 * implements a specific version of this interface. The 
 */
class FVoiceInterfaceXe :
	public FVoiceInterface
{
	/** Singleton instance pointer */
	static FVoiceInterfaceXe* GVoiceInterface;
	/** The pointer to the XHV engine which this class aggregates */
	IXHVEngine* VoiceEngine;
	/** Whether the speech recognition should happen or not */
	UBOOL bIsSpeechRecogOn;

	/**
	 * Static wrapper that re-routes the call to the singleton
	 */
	static void _XhvCallback(DWORD UserIndex,void* Data,DWORD Size,BOOL* VoiceDetected)
	{
		GVoiceInterface->RawVoiceDataCallback(UserIndex,Data,Size,VoiceDetected);
	}

	/** Simple constructor that zeros members. Hidden due to factory method */
	FVoiceInterfaceXe(void) :
		VoiceEngine(NULL)
	{
	}

	/**
	 * Creates the XHV engine and performs any other initialization
	 *
	 * @param MaxLocalTalkers the maximum number of local talkers to support
	 * @param MaxRemoteTalkers the maximum number of remote talkers to support
	 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled
	 *
	 * @return TRUE if everything initialized correctly, FALSE otherwise
	 */
	UBOOL Init(INT MaxLocalTalkers,INT MaxRemoteTalkers,UBOOL bIsSpeechRecognitionDesired);

	/**
	 * Callback that occurs when XHV has raw data for the speech recognition to process
	 *
	 * @param UserIndex the user the data is from
	 * @param Data the buffer holding the data
	 * @param Size the amount of data in the buffer
	 * @param VoiceDetected ?
	 */
	void RawVoiceDataCallback(DWORD UserIndex,void* Data,DWORD Size,BOOL* VoiceDetected);

public:

	/** Destructor that releases the engine if allocated */
	virtual ~FVoiceInterfaceXe(void)
	{
		if (VoiceEngine)
		{
			VoiceEngine->Release();
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
	static FVoiceInterfaceXe* CreateInstance(INT MaxLocalTalkers,
		INT MaxRemoteTalkers,UBOOL bIsSpeechRecognitionDesired)
	{
		if (GVoiceInterface == NULL)
		{
			GVoiceInterface = new FVoiceInterfaceXe();
			// Init the XHV engine with those defaults
			if (GVoiceInterface->Init(MaxLocalTalkers,MaxRemoteTalkers,
				bIsSpeechRecognitionDesired) == FALSE)
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
	 * Tells the voice system which vocabulary to use for the specified user
	 * when performing voice recognition
	 *
	 * @param UserIndex the local user to associate the vocabulary with
	 * @param VocabularyIndex the index of the vocabulary file to use
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD SelectVocabulary(DWORD UserIndex,DWORD VocabularyIndex);

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
	 * Determines if the recognition process for the specified user has
	 * completed or not
	 *
	 * @param UserIndex the user that is being queried
	 *
	 * @return TRUE if completed, FALSE otherwise
	 */
	virtual UBOOL HasRecognitionCompleted(DWORD UserIndex);

	/**
	 * Gets the results of the voice recognition
	 *
	 * @param UserIndex the local user to read the results of
	 * @param Words the set of words that were recognized by the voice analyzer
	 *
	 * @return 0 upon success, an error code otherwise
	 */
	virtual DWORD GetRecognitionResults(DWORD UserIndex,TArray<FSpeechRecognizedWord>& Words);
};

/** Singleton instance pointer initialization */
FVoiceInterfaceXe* FVoiceInterfaceXe::GVoiceInterface = NULL;

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
	return FVoiceInterfaceXe::CreateInstance(MaxLocalTalkers,MaxRemoteTalkers,bIsSpeechRecognitionDesired);
}

/**
 * Creates the XHV engine and performs any other initialization
 *
 * @param MaxLocalTalkers the maximum number of local talkers to support
 * @param MaxRemoteTalkers the maximum number of remote talkers to support
 * @param bIsSpeechRecognitionDesired whether speech recognition should be enabled
 *
 * @return TRUE if everything initialized correctly, FALSE otherwise
 */
UBOOL FVoiceInterfaceXe::Init(INT MaxLocalTalkers,INT MaxRemoteTalkers,
	UBOOL bIsSpeechRecognitionDesired)
{
    XHV_INIT_PARAMS InitParams;
	// Zero everything out
	appMemzero(&InitParams,sizeof(XHV_INIT_PARAMS));
	// Now set up our settings
    InitParams.dwMaxLocalTalkers  = Min<DWORD>(MaxLocalTalkers,XHV_MAX_LOCAL_TALKERS);
    InitParams.dwMaxRemoteTalkers = Min<DWORD>(MaxRemoteTalkers,XHV_MAX_REMOTE_TALKERS);
	// Set the array of function pointers for the supported modes
	XHV_PROCESSING_MODE LocalModes[] = { XHV_VOICECHAT_MODE };
    XHV_PROCESSING_MODE RemoteModes[] = { XHV_VOICECHAT_MODE };
	InitParams.dwNumLocalTalkerEnabledModes = 1;
    InitParams.dwNumRemoteTalkerEnabledModes = 1;
    // Set the processing modes
    InitParams.localTalkerEnabledModes = LocalModes;
    InitParams.remoteTalkerEnabledModes = RemoteModes;
	// Set the callback for speech recognition
	InitParams.pfnMicrophoneRawDataReady = _XhvCallback;
	HANDLE hCreatedThread = NULL;
    // Create our XHV engine
    HRESULT hr = XHVCreateEngine(&InitParams,&hCreatedThread,&VoiceEngine);
	if (SUCCEEDED(hr))
	{
		// Assign the XHV thread to specified processor (see UnXenon.h)
		if (XSetThreadProcessor(hCreatedThread,XHV_HWTHREAD) == (DWORD)-1)
		{
			debugf(NAME_Error,TEXT("Failed to set thread affinity for XHV thread %d"),
				GetLastError());
			// Badness happened so shut down
			VoiceEngine->Release();
			VoiceEngine = NULL;
			hr = E_FAIL;
		}
	}
	// Only init speech recognition if desired by the game
	if (bIsSpeechRecognitionDesired)
	{
		//@todo: john scott fill this in
	}
	return SUCCEEDED(hr);
}

/**
 * Starts local voice processing for the specified user index
 *
 * @param UserIndex the user to start processing for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StartLocalVoiceProcessing(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	// Just forward to XHV
	return VoiceEngine->StartLocalProcessingModes(UserIndex,&XHV_VOICECHAT_MODE,1);
}

/**
 * Stops local voice processing for the specified user index
 *
 * @param UserIndex the user to stop processing of
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StopLocalVoiceProcessing(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	// Just forward to XHV
	return VoiceEngine->StopLocalProcessingModes(UserIndex,&XHV_VOICECHAT_MODE,1);
}

/**
 * Starts remote voice processing for the specified user
 *
 * @param UniqueId the unique id of the user that will be talking
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StartRemoteVoiceProcessing(FUniqueNetId UniqueId)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->StartRemoteProcessingModes((XUID&)UniqueId,&XHV_VOICECHAT_MODE,1);
}

/**
 * Stops remote voice processing for the specified user
 *
 * @param UniqueId the unique id of the user that will no longer be talking
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StopRemoteVoiceProcessing(FUniqueNetId UniqueId)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->StopRemoteProcessingModes((XUID&)UniqueId,&XHV_VOICECHAT_MODE,1);
}

/**
 * Registers the user index as a local talker (interested in voice data)
 *
 * @param UserIndex the user index that is going to be a talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::RegisterLocalTalker(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->RegisterLocalTalker(UserIndex);
}

/**
 * Unregisters the user index as a local talker (not interested in voice data)
 *
 * @param UserIndex the user index that is no longer a talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::UnregisterLocalTalker(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->UnregisterLocalTalker(UserIndex);
}

/**
 * Registers the unique player id as a remote talker (submitted voice data only)
 *
 * @param UniqueId the id of the remote talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::RegisterRemoteTalker(FUniqueNetId UniqueId)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->RegisterRemoteTalker((XUID&)UniqueId,NULL,NULL,NULL);
}

/**
 * Unregisters the unique player id as a remote talker
 *
 * @param UniqueId the id of the remote talker
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::UnregisterRemoteTalker(FUniqueNetId UniqueId)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->UnregisterRemoteTalker((XUID&)UniqueId);
}

/**
 * Checks whether a local user index has a headset present or not
 *
 * @param UserIndex the user to check status for
 *
 * @return TRUE if there is a headset, FALSE otherwise
 */
UBOOL FVoiceInterfaceXe::IsHeadsetPresent(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->IsHeadsetPresent(UserIndex);
}

/**
 * Determines whether a local user index is currently talking or not
 *
 * @param UserIndex the user to check status for
 *
 * @return TRUE if the user is talking, FALSE otherwise
 */
UBOOL FVoiceInterfaceXe::IsLocalPlayerTalking(DWORD UserIndex)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->IsLocalTalking(UserIndex);
}

/**
 * Determines whether a remote talker is currently talking or not
 *
 * @param UniqueId the unique id of the talker to check status on
 *
 * @return TRUE if the user is talking, FALSE otherwise
 */
UBOOL FVoiceInterfaceXe::IsRemotePlayerTalking(FUniqueNetId UniqueId)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->IsRemoteTalking((XUID&)UniqueId);
}

/**
 * Returns which local talkers have data ready to be read from the voice system
 *
 * @return Bit mask of talkers that have data to be read (1 << UserIndex)
 */
DWORD FVoiceInterfaceXe::GetVoiceDataReadyFlags(void)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->GetDataReadyFlags();
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
DWORD FVoiceInterfaceXe::SetPlaybackPriority(DWORD UserIndex,FUniqueNetId RemoteTalkerId,DWORD Priority)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->SetPlaybackPriority((XUID&)RemoteTalkerId,UserIndex,(XHV_PLAYBACK_PRIORITY)Priority);
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
DWORD FVoiceInterfaceXe::ReadLocalVoiceData(DWORD UserIndex,BYTE* Data,DWORD* Size)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	DWORD Ignored;
	return VoiceEngine->GetLocalChatData(UserIndex,Data,Size,&Ignored);
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
DWORD FVoiceInterfaceXe::SubmitRemoteVoiceData(FUniqueNetId RemoteTalkerId,BYTE* Data,DWORD* Size)
{
	check(VoiceEngine && "Call FVoiceInterfaceXe::Init() before using");
	return VoiceEngine->SubmitIncomingChatData((XUID&)RemoteTalkerId,Data,Size);
}

/**
 * Callback that occurs when XHV has raw data for the speech recognition to process
 *
 * @param UserIndex the user the data is from
 * @param Data the buffer holding the data
 * @param Size the amount of data in the buffer
 * @param VoiceDetected ?
 */
void FVoiceInterfaceXe::RawVoiceDataCallback(DWORD UserIndex,void* Data,DWORD Size,BOOL* VoiceDetected)
{
//@todo: john scott fill this in
}

/**
 * Tells the voice system which vocabulary to use for the specified user
 * when performing voice recognition
 *
 * @param UserIndex the local user to associate the vocabulary with
 * @param VocabularyIndex the index of the vocabulary file to use
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::SelectVocabulary(DWORD UserIndex,DWORD VocabularyIndex)
{
//@todo: john scott fill this in
	return E_NOTIMPL;
}

/**
 * Tells the voice system to start tracking voice data for speech recognition
 *
 * @param UserIndex the local user to recognize voice data for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StartSpeechRecognition(DWORD UserIndex)
{
	bIsSpeechRecogOn = TRUE;
	return S_OK;
}

/**
 * Tells the voice system to stop tracking voice data for speech recognition
 *
 * @param UserIndex the local user to recognize voice data for
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::StopSpeechRecognition(DWORD UserIndex)
{
	bIsSpeechRecogOn = FALSE;
	return S_OK;
}

/**
 * Determines if the recognition process for the specified user has
 * completed or not
 *
 * @param UserIndex the user that is being queried
 *
 * @return TRUE if completed, FALSE otherwise
 */
UBOOL FVoiceInterfaceXe::HasRecognitionCompleted(DWORD UserIndex)
{
//@todo: john scott fill this in
	return FALSE;
}

/**
 * Gets the results of the voice recognition
 *
 * @param UserIndex the local user to read the results of
 * @param Words the set of words that were recognized by the voice analyzer
 *
 * @return 0 upon success, an error code otherwise
 */
DWORD FVoiceInterfaceXe::GetRecognitionResults(DWORD UserIndex,TArray<FSpeechRecognizedWord>& Words)
{
//@todo: john scott fill this in
	return E_NOTIMPL;
}
