/*=============================================================================
	UnDemoRec.h: Demo recording support classes
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_DEMORECORDING
#define _INC_DEMORECORDING

/*-----------------------------------------------------------------------------
	UDemoRecConnection.
-----------------------------------------------------------------------------*/

//
// Simulated network connection for recording and playing back game sessions.
//
class UDemoRecConnection : public UNetConnection
{
	DECLARE_CLASS(UDemoRecConnection,UNetConnection,CLASS_Config|CLASS_Transient|CLASS_Intrinsic,Engine)

	// Constructors.
	UDemoRecConnection();

	/**
	 * Intializes a "addressless" connection with the passed in settings
	 *
	 * @param InDriver the net driver associated with this connection
	 * @param InState the connection state to start with for this connection
	 * @param InURL the URL to init with
	 * @param InConnectionSpeed Optional connection speed override
	 */
	virtual void InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, INT InConnectionSpeed=0);

	// UNetConnection interface.
	FString LowLevelGetRemoteAddress();
	FString LowLevelDescribe();
	void LowLevelSend( void* Data, INT Count );
	INT IsNetReady( UBOOL Saturate );
	void FlushNet();
	void HandleClientPlayer( APlayerController* PC );

	// UDemoRecConnection functions.
	/**
	 * @return The DemoRecording driver object
	 */
	class UDemoRecDriver* GetDriver();
};

/*-----------------------------------------------------------------------------
	UDemoRecDriver.
-----------------------------------------------------------------------------*/

//
// Simulated network driver for recording and playing back game sessions.
//
class UDemoRecDriver : public UNetDriver
{
	DECLARE_CLASS(UDemoRecDriver,UNetDriver,CLASS_Transient|CLASS_Config|CLASS_Intrinsic,Engine)

	// Options.
	/** Whether the demo should be played back as fast as possible or respect recorded time stamps. */
	UBOOL			bNoFrameCap;
	/** Whether the demo has ended/ reached the end of the stream.									*/
	UBOOL			bHasDemoEnded;
	/** Whether to skip rendering the current frame for playback on less powerful machines.			*/
	UBOOL			bNoRender;
	/** Whether to exit the game after playback of last loop iteration has finished.				*/
	UBOOL			bShouldExitAfterPlaybackFinished;
	/** Number of times to play the demo.															*/
	INT				PlayCount;
	/** Allow for playback without doing any package version checking. Can be used to attempt to play a demo that wouldn't normally open */
	UBOOL			bShouldSkipPackageChecking;

	// Variables.
	FString			DemoFilename;
	FStringNoInit	DemoSpectatorClass;
	FArchive*		FileAr;
	DOUBLE			PlaybackStartTime;
	DOUBLE			LastFrameTime;
	INT				FrameNum;
	FLOAT			LastDeltaTime;
	FURL            LoopURL;
	/** for capping client-side demo recording FPS. */
	DOUBLE			LastClientRecordTime; 
	FLOAT			DemoRecMultiFrameDeltaTime;


	// Constructors.
	void StaticConstructor();
	UDemoRecDriver();

	// UNetDriver interface.
	void LowLevelDestroy();
	FString LowLevelGetNetworkNumber();
	UBOOL InitConnect( FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error );
	UBOOL InitListen( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error );
	void TickDispatch( FLOAT DeltaTime );
	virtual UBOOL IsDemoDriver()
	{
		return TRUE;
	}

	// FExec interface.
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );

	// UDemoRecDriver functions.
	UBOOL InitBase( UBOOL Connect, FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error );
	void SpawnDemoRecSpectator( UNetConnection* Connection );
	UBOOL UpdateDemoTime( FLOAT* DeltaTime, FLOAT TimeDilation );
};


/*-----------------------------------------------------------------------------
	UDemoPlayPendingLevel.
-----------------------------------------------------------------------------*/

//
// Class controlling a pending demo playback level.
//
class UDemoPlayPendingLevel : public UPendingLevel
{
	DECLARE_CLASS(UDemoPlayPendingLevel,UPendingLevel,CLASS_Transient,Engine)
	NO_DEFAULT_CONSTRUCTOR(UDemoPlayPendingLevel)

	// Constructors.
	UDemoPlayPendingLevel( const FURL& InURL );

	// FNetworkNotify interface.
	EAcceptConnection NotifyAcceptingConnection() 
	{ 
		return ACCEPTC_Reject; 
	}
	void NotifyAcceptedConnection( class UNetConnection* Connection ) 
	{}
	UBOOL NotifyAcceptingChannel( class UChannel* Channel ) 
	{ 
		return TRUE; 
	}
	UWorld* NotifyGetWorld() 
	{ 
		return NULL; 
	}
	void NotifyReceivedText( UNetConnection* Connection, const TCHAR* Text );
	UBOOL NotifySendingFile( UNetConnection* Connection, FGuid GUID )
	{
		return FALSE;
	}
	void NotifyReceivedFile( UNetConnection* Connection, INT PackageIndex, const TCHAR* Error, UBOOL Skipped ) 
	{}
	void NotifyProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds ) 
	{}

	// UPendingLevel interface.
	void Tick( FLOAT DeltaTime );
	UNetDriver* GetDriver();
	void SendJoin() 
	{ 
		SentJoin = 1; 
	}
	UBOOL TrySkipFile() 
	{ 
		return FALSE; 
	}
};

#endif
