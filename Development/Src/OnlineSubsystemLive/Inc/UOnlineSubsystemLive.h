/**
 * Copyright © 2006 Epic Games, Inc. All Rights Reserved.
 */

protected:
	/** Version of the delegate parms that takes a result code and applies it */
	struct FAsyncTaskDelegateResults
	{
		/** Whether the async task worked or not. Script accessible only! */
	    UBOOL bWasSuccessful;

		/**
		 * Constructor that sets the success flag based upon the passed in results
		 *
		 * @param Result the result code to check
		 */
		FAsyncTaskDelegateResults(DWORD Result)
		{
			// Make sure the generic task results match in size
			check(sizeof(FAsyncTaskDelegateResults) == sizeof(OnlineSubsystemLive_eventOnCreateOnlineGameComplete_Parms));
			bWasSuccessful = (Result == ERROR_SUCCESS) ? FIRST_BITFIELD : 0;
		}

		/**
		 * Constructor that sets the success flag to false
		 */
		FAsyncTaskDelegateResults(void)
		{
			// Make sure the generic task results match in size
			check(sizeof(FAsyncTaskDelegateResults) == sizeof(OnlineSubsystemLive_eventOnCreateOnlineGameComplete_Parms));
			bWasSuccessful = FALSE;
		}
	};

	/**
	 * Processes a notification that was returned during polling
	 *
	 * @param Notification the notification event that was fired
	 * @param Data the notification specifc data
	 */
	virtual void ProcessNotification(DWORD Notification,ULONG_PTR Data);

	/**
	 * Handles any sign in change processing (firing delegates, etc)
	 *
	 * @param Data the mask of changed sign ins
	 */
	void ProcessSignInNotification(ULONG_PTR Data);

	/**
	 * Checks for new Live notifications and passes them out to registered delegates
	 *
	 * @param DeltaTime the amount of time that has passed since the last tick
	 */
	virtual void Tick(FLOAT DeltaTime);

	/**
	 * Iterates the list of outstanding tasks checking for their completion
	 * status. Upon completion, it fires off the corresponding delegate
	 * notification
	 */
	void TickAsyncTasks(void);

	/**
	 * Creates the session flags value from the game settings object
	 *
	 * @param GameSettings the game settings of the new session
	 *
	 * @return the flags needed to set up the session
	 */
	DWORD BuildSessionFlags(UOnlineGameSettings* GameSettings);

	/**
	 * Sets the contexts and properties for this game settings object
	 *
	 * @param HostingPlayerNum the index of the player hosting the match
	 * @param GameSettings the game settings of the new session
	 */
	void SetContextsAndProperties(BYTE HostingPlayerNum,UOnlineGameSettings* GameSettings);

	/**
	 * Sets the list contexts for the player
	 *
	 * @param PlayerNum the index of the player hosting the match
	 * @param Contexts the list of contexts to set
	 */
	void SetContexts(BYTE PlayerNum,const TArray<FLocalizedStringSetting>& Contexts);

	/**
	 * Sets the list properties for the player
	 *
	 * @param PlayerNum the index of the player hosting the match
	 * @param Properties the list of properties to set
	 */
	void SetProperties(BYTE PlayerNum,const TArray<FSettingsProperty>& Properties);

	/**
	 * Reads the contexts and properties from the Live search data and populates the
	 * game settings object with them
	 *
	 * @param SearchResult the data that was returned from Live
	 * @param GameSettings the game settings that we are setting the data on
	 */
	void ParseContextsAndProperties(XSESSION_SEARCHRESULT& SearchResult,UOnlineGameSettings* GameSettings);

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
	FLiveAsyncTaskDataSearch* AllocateSearch(BYTE SearchingPlayerNum,DWORD QueryNum,DWORD MaxSearchResults,DWORD& NumBytes);

	/**
	 * Copies the Epic structures into the Live equivalent
	 *
	 * @param DestProps the destination properties
	 * @param SourceProps the source properties
	 */
	void CopyPropertiesForSearch(PXUSER_PROPERTY DestProps,const TArray<FSettingsProperty>& SourceProps);

	/**
	 * Copies the Epic structures into the Live equivalent
	 *
	 * @param Search the object to use when determining
	 * @param DestContexts the destination contexts
	 * @param SourceContexts the source contexts
	 *
	 * @return the number of items copied (handles skipping for wildcards)
	 */
	DWORD CopyContextsForSearch(UOnlineGameSearch* Search,PXUSER_CONTEXT DestContexts,const TArray<FLocalizedStringSetting>& SourceContexts);

	/**
	 * Copies Unreal data to Live structures for the Live property writes
	 *
	 * @param Profile the profile object to copy the data from
	 * @param LiveData the Live data structures to copy the data to
	 */
	void CopyLiveProfileSettings(UOnlineProfileSettings* Profile,PXUSER_PROFILE_SETTING LiveData);

	/**
	 * Writes the true skill stats for players that are in the controller list
	 *
	 * @return TRUE if the call succeeded, FALSE otherwise
	 */
	UBOOL WriteTrueSkillStats(void);

	/**
	 * Writes the true skill stat for a single player
	 *
	 * @param Xuid the xuid of the player to write the true skill for
	 * @param TeamId the team the player is on
	 * @param Score the score this player had
	 *
	 * @return TRUE if the call succeeded, FALSE otherwise
	 */
	UBOOL WriteTrueSkillForPlayer(XUID Xuid,DWORD TeamId,FLOAT Score);

	/**
	 * Copies the stats data from our Epic form to something Live can handle
	 *
	 * @param Stats the destination buffer the stats are written to
	 * @param Properties the Epic structures that need copying over
	 * @param RatingId the id to set as the rating for this leaderboard set
	 */
	void CopyStatsToProperties(XUSER_PROPERTY* Stats,const TArray<FSettingsProperty>& Properties,const DWORD RatingId);

	/**
	 * Builds the data that we want to read into the Live specific format. Live
	 * uses WORDs which script doesn't support, so we can't map directly to it
	 *
	 * @param DestSpecs the destination stat specs to fill in
	 * @param ViewId the view id that is to be used
	 * @param Columns the columns that are being requested
	 */
	void BuildStatsSpecs(XUSER_STATS_SPEC* DestSpecs,INT ViewId,const TArrayNoInit<INT>& Columns);

	/**
	 * Creates a new Live enabled game for the requesting player using the
	 * settings specified in the game settings object
	 *
	 * @param HostingPlayerNum the player hosting the game
	 *
	 * @return The result from the Live APIs
	 */
	DWORD CreateLiveGame(BYTE HostingPlayerNum);

	/**
	 * Tells the QoS to respond with a "go away" packet and includes our custom
	 * data. Prevents bandwidth from going to QoS probes
	 *
	 * @return The success/error code of the operation
	 */
	inline DWORD DisableQoS(void)
	{
		// Unregister with the QoS listener and releases any memory underneath
		DWORD Return = XNetQosListen(&SessionInfo->XSessionInfo.sessionID,NULL,0,0,
			XNET_QOS_LISTEN_DISABLE);
		debugf(NAME_DevLive,
			TEXT("XNetQosListen(Key,NULL,0,0,XNET_QOS_LISTEN_DISABLE) returned 0x%08X"),
			Return);
		return Return;
	}

	/**
	 * Tells the QoS thread to stop its listening process
	 *
	 * @return The success/error code of the operation
	 */
	inline DWORD UnregisterQoS(void)
	{
		// Unregister with the QoS listener and releases any memory underneath
		DWORD Return = XNetQosListen(&SessionInfo->XSessionInfo.sessionID,NULL,0,0,
			XNET_QOS_LISTEN_RELEASE | XNET_QOS_LISTEN_SET_DATA);
		debugf(NAME_DevLive,
			TEXT("XNetQosListen(Key,NULL,0,0,XNET_QOS_LISTEN_RELEASE | XNET_QOS_LISTEN_SET_DATA) returned 0x%08X"),
			Return);
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
	DWORD CreateLanGame(BYTE HostingPlayerNum);

	/**
	 * Builds a Live game query and submits it to Live for processing
	 *
	 * @param SearchingPlayerNum the player searching for games
	 * @param SearchSettings the settings to search with
	 *
	 * @return The result from the Live APIs
	 */
	DWORD FindLiveGames(BYTE SearchingPlayerNum,UOnlineGameSearch* SearchSettings);

	/**
	 * Builds a LAN query to broadcast
	 *
	 * @param SearchSettings the settings to search for games matching
	 *
	 * @return an error/success code
	 */
	DWORD FindLanGames(UOnlineGameSearch* SearchSettings);

	/**
	 * Joins a Live game by creating the session without hosting it
	 *
	 * @param PlayerNum the player joining the game
	 * @param DesiredGame the full search results information for joinging a game
	 * @param bIsFromInvite whether this join is from a search or from an invite
	 *
	 * @return The result from the Live APIs
	 */
	DWORD JoinLiveGame(BYTE PlayerNum,const FOnlineGameSearchResult& DesiredGame,UBOOL bIsFromInvite = FALSE);

	/**
	 * Terminates a Live session
	 *
	 * @return The result from the Live APIs
	 */
	DWORD DestroyLiveGame(void);

	/**
	 * Terminates a LAN session
	 *
	 * @return an error/success code
	 */
	DWORD DestroyLanGame(void);

	/** Stops the system link beacon from accepting broadcasts */
	inline void StopSystemLinkBeacon(void)
	{
		// Don't poll anymore since we are shutting it down
		SystemLinkState = SLS_NotUsingSystemLink;
		// Unbind the system link discovery socket
		delete SysLinkSocket;
		SysLinkSocket = NULL;
	}	

	/**
	 * Processes a system link packet. For a host, responds to discovery
	 * requests. For a client, parses the discovery response and places
	 * the resultant data in the current search's search results array
	 *
	 * @param PacketData the packet data to parse
	 * @param PacketLength the amount of data that was received
	 */
	void ParseSystemLinkPacket(BYTE* PacketData,INT PacketLength);

	/**
	 * Adds the game settings data to the packet that is sent by the host
	 * in reponse to a server query
	 *
	 * @param Packet the writer object that will encode the data
	 * @param GameSettings the game settings to add to the packet
	 */
	void AppendGameSettings(FSystemLinkPacketWriter& Packet,UOnlineGameSettings* GameSettings);

	/**
	 * Reads the game settings data from the packet and applies it to the
	 * specified object
	 *
	 * @param Packet the reader object that will read the data
	 * @param GameSettings the game settings to copy the data to
	 */
	void ReadGameSettings(FSystemLinkPacketReader& Packet,UOnlineGameSettings* NewServer);

	/**
	 * Ticks any system link background tasks
	 *
	 * @param DeltaTime the time since the last tick
	 */
	void TickLanTasks(FLOAT DeltaTime);

	/**
	 * Ticks voice subsystem for reading/submitting any voice data
	 *
	 * @param DeltaTime the time since the last tick
	 */
	void TickVoice(FLOAT DeltaTime);

	/**
	 * Reads any data that is currently queued in XHV
	 */
	void ProcessLocalVoicePackets(void);

	/**
	 * Submits network packets to XHV for playback
	 */
	void ProcessRemoteVoicePackets(void);

	/**
	 * Processes any talking delegates that need to be fired off
	 */
	void ProcessTalkingDelegates(void);

	/**
	 * Processes any speech recognition delegates that need to be fired off
	 */
	void ProcessSpeechRecognitionDelegates(void);

	/**
	 * Finds a remote talker in the cached list
	 *
	 * @param UniqueId the XUID of the player to search for
	 *
	 * @return pointer to the remote talker or NULL if not found
	 */
	inline FRemoteTalker* FindRemoteTalker(FUniqueNetId UniqueId)
	{
		for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
		{
			FRemoteTalker& Talker = RemoteTalkers(Index);
			// Compare XUIDs to see if they match
			if ((XUID&)Talker.TalkerId == (XUID&)UniqueId)
			{
				return &RemoteTalkers(Index);
			}
		}
		return NULL;
	}

	/**
	 * Re-evaluates the muting list for all local talkers
	 */
	void ProcessMuteChangeNotification(void);

	/**
	 * Registers/unregisters local talkers based upon login changes
	 */
	void UpdateVoiceFromLoginChange(void);

	/**
	 * Iterates the current remote talker list unregistering them with XHV
	 * and our internal state
	 */
	void RemoveAllRemoteTalkers(void);

	/**
	 * Registers all signed in local talkers
	 */
	void RegisterLocalTalkers(void);

	/**
	 * Unregisters all signed in local talkers
	 */
	void UnregisterLocalTalkers(void);

	/**
	 * Checks to see if a XUID is a local player or not. This is to avoid
	 * the extra processing that happens for remote players
	 *
	 * @param Player the player to check as being local
	 *
	 * @return TRUE if the player is local, FALSE otherwise
	 */
	inline UBOOL IsLocalPlayer(FUniqueNetId Player)
	{
		XUID Xuid;
		// Loop through the signins checking
		for (INT Index = 0; Index < 4; Index++)
		{
			// If the index has a XUID and it matches the talker, then they
			// are local
			if (XUserGetXUID(Index,&Xuid) == ERROR_SUCCESS &&
				Xuid == (XUID&)Player)
			{
				return TRUE;
			}
		}
		// Remote talker
		return FALSE;
	}

	/**
	 * Handles accepting a game invite for the specified user
	 *
	 * @param UserNum the user that accepted the game invite
	 */
	void ProcessGameInvite(DWORD UserNum);

	/**
	 * Handles notifying interested parties when a signin is cancelled
	 */
	void ProcessSignInCancelledNotification(void);

	/**
	 * Handles external UI change notifications
	 *
	 * @param bIsOpening whether the UI is opening or closing
	 */
	void ProcessExternalUINotification(UBOOL bIsOpening);

	/**
	 * Handles controller connection state changes
	 */
	void ProcessControllerNotification(void);

	/**
	 * Handles notifying interested parties when the Live connection status
	 * has changed
	 *
	 * @param Status the type of change that has happened
	 */
	void ProcessConnectionStatusNotification(HRESULT Status);

	/**
	 * Handles notifying interested parties when the link state changes
	 *
	 * @param bIsConnected whether the link has a connection or not
	 */
	void ProcessLinkStateNotification(UBOOL bIsConnected);

	/**
	 * Figures out which remote talkers need to be muted for a given local talker
	 *
	 * @param TalkerIndex the talker that needs the mute list checked for
	 * @param PlayerController the player controller associated with this talker
	 */
	void UpdateMuteListForLocalTalker(INT TalkerIndex,APlayerController* PlayerController);

	/**
	 * Handles notifying interested parties when the player changes profile data
	 *
	 * @param ChangeStatus bit flags indicating which user just changed status
	 */
	void ProcessProfileDataNotification(DWORD ChangeStatus);

public:

	/**
	 * Parses the search results into something the game play code can handle
	 *
	 * @param Search the Unreal search object to fill in
	 * @param SearchResults the buffer filled by Live
	 */
	void ParseSearchResults(UOnlineGameSearch* Search,PXSESSION_SEARCHRESULT_HEADER SearchResults);

	/**
	 * Parses the read profile results into something the game play code can handle
	 *
	 * @param PlayerNum the number of the user being processed
	 * @param ReadResults the buffer filled by Live
	 */
	void ParseReadProfileResults(BYTE PlayerNum,PXUSER_READ_PROFILE_SETTING_RESULT ReadResults);

	/**
	 * Parses the arbitration results into something the game play code can handle
	 *
	 * @param ArbitrationResults the buffer filled by Live
	 */
	void ParseArbitrationResults(PXSESSION_REGISTRATION_RESULTS ArbitrationResults);

	/**
	 * Tells the QoS thread to start its listening process. Builds the packet
	 * of custom data to send back to clients in the query.
	 *
	 * @return The success/error code of the operation
	 */
	DWORD RegisterQoS(void);

	/**
	 * Kicks off the list of returned servers' QoS queries
	 *
	 * @param AsyncData the object that holds the async QoS data
	 *
	 * @return TRUE if the call worked and the results should be polled for,
	 *		   FALSE otherwise
	 */
	UBOOL CheckServersQoS(FLiveAsyncTaskDataSearch* AsyncData);

	/**
	 * Parses the results from the QoS queries and places those results in the
	 * corresponding search results info
	 *
	 * @param QosData the data to parse the results of
	 */
	void ParseQoSResults(XNQOS* QosData);

	/**
	 * Registers all local players with the current session
	 *
	 * @param bIsFromInvite whether we are from an invite or not
	 */
	void RegisterLocalPlayers(UBOOL bIsFromInvite = FALSE);

	/**
	 * Parses the friends results into something the game play code can handle
	 *
	 * @param PlayerIndex the index of the player that this read was for
	 * @param LiveFriend the buffer filled by Live
	 */
	void ParseFriendsResults(DWORD PlayerIndex,PXONLINE_FRIEND LiveFriend);

	/**
	 * Adds one setting to the users profile results
	 *
	 * @param Profile the profile object to copy the data from
	 * @param LiveData the Live data structures to copy the data to
	 */
	void AppendProfileSetting(BYTE PlayerNum,const FOnlineProfileSetting& Setting);

	/**
	 * Determines whether the specified settings should come from the game
	 * default settings. If so, the defaults are copied into the players
	 * profile results and removed from the settings list
	 *
	 * @param PlayerNum the id of the player
	 * @param SettingsIds the set of ids to filter against the game defaults
	 */
	void ProcessProfileDefaults(BYTE PlayerNum,TArray<DWORD>& SettingsId);

	/**
	 * Parses the read results and copies them to the stats read object
	 *
	 * @param ReadResults the data to add to the stats object
	 */
	void ParseStatsReadResults(XUSER_STATS_READ_RESULTS* ReadResults);

	/**
	 * Changes the number of public/private slots for the session
	 *
	 * @param PublicSlots the number of public slots to use
	 * @param PrivateSlots the number of private slots to use
	 */
	void ModifySessionSize(DWORD PublicSlots,DWORD PrivateSlots);

	/**
	 * Shrinks the session to the number of arbitrated registrants
	 */
	void ShrinkToArbitratedRegistrantSize(void);

	/**
	 * Kicks off an async test to the Live service to test the upstream
	 * capabilities of the console's net connection
	 */
	void PerformQosToLiveTest(void);
