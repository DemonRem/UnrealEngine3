/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UnIpDrv.h"

IMPLEMENT_CLASS(UOnlineGameInterfaceImpl);

/**
 * Ticks this object to update any async tasks
 *
 * @param DeltaTime the time since the last tick
 */
void UOnlineGameInterfaceImpl::Tick(FLOAT DeltaTime)
{
	// Tick lan tasks
	TickLanTasks(DeltaTime);
	// Tick any internet tasks
	TickInternetTasks(DeltaTime);
}

/**
 * Determines if the packet header is valid or not
 *
 * @param Packet the packet data to check
 * @param Length the size of the packet buffer
 *
 * @return true if the header is valid, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::IsValidLanQueryPacket(const BYTE* Packet,
	DWORD Length,QWORD& ClientNonce)
{
	ClientNonce = 0;
	UBOOL bIsValid = FALSE;
	// Serialize out the data if the packet is the right size
	if (Length == LAN_BEACON_PACKET_HEADER_SIZE)
	{
		FNboSerializeFromBuffer PacketReader(Packet,Length);
		BYTE Version = 0;
		PacketReader >> Version;
		// Do the versions match?
		if (Version == LAN_BEACON_PACKET_VERSION)
		{
			BYTE Platform = 255;
			PacketReader >> Platform;
			// Can we communicate with this platform?
			if (Platform & LanPacketPlatformMask)
			{
				INT GameId = -1;
				PacketReader >> GameId;
				// Is this our game?
				if (GameId == LanGameUniqueId)
				{
					BYTE SQ1 = 0;
					PacketReader >> SQ1;
					BYTE SQ2 = 0;
					PacketReader >> SQ2;
					// Is this a server query?
					bIsValid = (SQ1 == LAN_SERVER_QUERY1 && SQ2 == LAN_SERVER_QUERY2);
					// Read the client nonce as the outvalue
					PacketReader >> ClientNonce;
				}
			}
		}
	}
	return bIsValid;
}

/**
 * Determines if the packet header is valid or not
 *
 * @param Packet the packet data to check
 * @param Length the size of the packet buffer
 *
 * @return true if the header is valid, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::IsValidLanResponsePacket(const BYTE* Packet,DWORD Length)
{
	UBOOL bIsValid = FALSE;
	// Serialize out the data if the packet is the right size
	if (Length > LAN_BEACON_PACKET_HEADER_SIZE)
	{
		FNboSerializeFromBuffer PacketReader(Packet,Length);
		BYTE Version = 0;
		PacketReader >> Version;
		// Do the versions match?
		if (Version == LAN_BEACON_PACKET_VERSION)
		{
			BYTE Platform = 255;
			PacketReader >> Platform;
			// Can we communicate with this platform?
			if (Platform & LanPacketPlatformMask)
			{
				INT GameId = -1;
				PacketReader >> GameId;
				// Is this our game?
				if (GameId == LanGameUniqueId)
				{
					BYTE SQ1 = 0;
					PacketReader >> SQ1;
					BYTE SQ2 = 0;
					PacketReader >> SQ2;
					// Is this a server response?
					if (SQ1 == LAN_SERVER_RESPONSE1 && SQ2 == LAN_SERVER_RESPONSE2)
					{
						QWORD Nonce = 0;
						PacketReader >> Nonce;
						bIsValid = Nonce == (QWORD&)LanNonce;
					}
				}
			}
		}
	}
	return bIsValid;
}

/**
 * Ticks any lan beacon background tasks
 *
 * @param DeltaTime the time since the last tick
 */
void UOnlineGameInterfaceImpl::TickLanTasks(FLOAT DeltaTime)
{
	if (LanBeaconState > LANB_NotUsingLanBeacon && LanBeacon != NULL)
	{
		BYTE PacketData[512];
		UBOOL bShouldRead = TRUE;
		// Read each pending packet and pass it out for processing
		while (bShouldRead)
		{
			INT NumRead = LanBeacon->ReceivePacket(PacketData,512);
			if (NumRead > 0)
			{
				// Hand this packet off to child classes for processing
				ProcessLanPacket(PacketData,NumRead);
				// Reset the timeout since a packet came in
				LanQueryTimeLeft = LanQueryTimeout;
			}
			else
			{
				if (LanBeaconState == LANB_Searching)
				{
					// Decrement the amount of time remaining
					LanQueryTimeLeft -= DeltaTime;
					// Check for a timeout on the search packet
					if (LanQueryTimeLeft <= 0.f)
					{
						// Stop future timeouts since we aren't searching any more
						LanBeaconState = LANB_NotUsingLanBeacon;
						if (GameSearch != NULL)
						{
							GameSearch->bIsSearchInProgress = FALSE;
						}
						// Trigger the delegate so the UI knows we didn't find any
						FAsyncTaskDelegateResults Params(S_OK);
						TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Params);
					}
				}
				bShouldRead = FALSE;
			}
		}
	}
}

/**
 * Adds the game settings data to the packet that is sent by the host
 * in reponse to a server query
 *
 * @param Packet the writer object that will encode the data
 * @param InGameSettings the game settings to add to the packet
 */
void UOnlineGameInterfaceImpl::AppendGameSettingsToPacket(FNboSerializeToBuffer& Packet,
	UOnlineGameSettings* InGameSettings)
{
//@todo joeg -- consider using introspection (property iterator or databinding
// iterator) to handle subclasses

#if DEBUG_LAN_BEACON
	debugf(NAME_DevOnline,TEXT("Sending game settings to client"));
#endif
	// Members of the game settings class
	Packet << InGameSettings->NumOpenPublicConnections
		<< InGameSettings->NumOpenPrivateConnections
		<< InGameSettings->NumPublicConnections
		<< InGameSettings->NumPrivateConnections
		<< (BYTE)InGameSettings->bShouldAdvertise
		<< (BYTE)InGameSettings->bIsLanMatch
		<< (BYTE)InGameSettings->bUsesStats
		<< (BYTE)InGameSettings->bAllowJoinInProgress
		<< (BYTE)InGameSettings->bAllowInvites
		<< (BYTE)InGameSettings->bUsesPresence
		<< (BYTE)InGameSettings->bAllowJoinViaPresence
		<< (BYTE)InGameSettings->bUsesArbitration;
	// Write the player id so we can show gamercard
	Packet << InGameSettings->OwningPlayerId;
	Packet << InGameSettings->OwningPlayerName;
#if DEBUG_SYSLINK
	QWORD Uid = (QWORD&)InGameSettings->OwningPlayerId.Uid;
	debugf(NAME_DevOnline,TEXT("%s 0x%016I64X"),*InGameSettings->OwningPlayerName,Uid);
#endif
	// Now add the contexts and properties from the settings class
	// First, add the number contexts involved
	INT Num = InGameSettings->LocalizedSettings.Num();
	Packet << Num;
	// Now add each context individually
	for (INT Index = 0; Index < InGameSettings->LocalizedSettings.Num(); Index++)
	{
		Packet << InGameSettings->LocalizedSettings(Index);
#if DEBUG_LAN_BEACON
		debugf(NAME_DevOnline,*BuildContextString(InGameSettings,InGameSettings->LocalizedSettings(Index)));
#endif
	}
	// Next, add the number of properties involved
	Num = InGameSettings->Properties.Num();
	Packet << Num;
	// Now add each property
	for (INT Index = 0; Index < InGameSettings->Properties.Num(); Index++)
	{
		Packet << InGameSettings->Properties(Index);
#if DEBUG_LAN_BEACON
		debugf(NAME_DevOnline,*BuildPropertyString(InGameSettings,InGameSettings->Properties(Index)));
#endif
	}
}

/**
 * Reads the game settings data from the packet and applies it to the
 * specified object
 *
 * @param Packet the reader object that will read the data
 * @param InGameSettings the game settings to copy the data to
 */
void UOnlineGameInterfaceImpl::ReadGameSettingsFromPacket(FNboSerializeFromBuffer& Packet,
	UOnlineGameSettings* InGameSettings)
{
//@todo joeg -- consider using introspection (property iterator or databinding
// iterator) to handle subclasses

#if DEBUG_LAN_BEACON
	debugf(NAME_DevOnline,TEXT("Reading game settings from server"));
#endif
	// Members of the game settings class
	Packet >> InGameSettings->NumOpenPublicConnections
		>> InGameSettings->NumOpenPrivateConnections
		>> InGameSettings->NumPublicConnections
		>> InGameSettings->NumPrivateConnections;
	BYTE Read = FALSE;
	// Read all the bools as bytes
	Packet >> Read;
	InGameSettings->bShouldAdvertise = Read == TRUE;
	Packet >> Read;
	InGameSettings->bIsLanMatch = Read == TRUE;
	Packet >> Read;
	InGameSettings->bUsesStats = Read == TRUE;
	Packet >> Read;
	InGameSettings->bAllowJoinInProgress = Read == TRUE;
	Packet >> Read;
	InGameSettings->bAllowInvites = Read == TRUE;
	Packet >> Read;
	InGameSettings->bUsesPresence = Read == TRUE;
	Packet >> Read;
	InGameSettings->bAllowJoinViaPresence = Read == TRUE;
	Packet >> Read;
	InGameSettings->bUsesArbitration = Read == TRUE;
	// Read the owning player id
	Packet >> InGameSettings->OwningPlayerId;
	// Read the owning player name
	Packet >> InGameSettings->OwningPlayerName;
#if DEBUG_LAN_BEACON
	QWORD Uid = (QWORD&)InGameSettings->OwningPlayerId.Uid;
	debugf(NAME_DevOnline,TEXT("%s 0x%016I64X"),*InGameSettings->OwningPlayerName,Uid);
#endif
	// Now read the contexts and properties from the settings class
	INT NumContexts = 0;
	// First, read the number contexts involved, so we can presize the array
	Packet >> NumContexts;
	if (Packet.HasOverflow() == FALSE)
	{
		InGameSettings->LocalizedSettings.Empty(NumContexts);
		InGameSettings->LocalizedSettings.AddZeroed(NumContexts);
	}
	// Now read each context individually
	for (INT Index = 0;
		Index < InGameSettings->LocalizedSettings.Num() && Packet.HasOverflow() == FALSE;
		Index++)
	{
		Packet >> InGameSettings->LocalizedSettings(Index);
#if DEBUG_LAN_BEACON
		debugf(NAME_DevOnline,*BuildContextString(InGameSettings,InGameSettings->LocalizedSettings(Index)));
#endif
	}
	INT NumProps = 0;
	// Next, read the number of properties involved for array presizing
	Packet >> NumProps;
	if (Packet.HasOverflow() == FALSE)
	{
		InGameSettings->Properties.Empty(NumProps);
		InGameSettings->Properties.AddZeroed(NumProps);
	}
	// Now read each property from the packet
	for (INT Index = 0;
		Index < InGameSettings->Properties.Num() && Packet.HasOverflow() == FALSE;
		Index++)
	{
		Packet >> InGameSettings->Properties(Index);
#if DEBUG_LAN_BEACON
		debugf(NAME_DevOnline,*BuildPropertyString(InGameSettings,InGameSettings->Properties(Index)));
#endif
	}
	// If there was an overflow, treat the string settings/properties as broken
	if (Packet.HasOverflow())
	{
		InGameSettings->LocalizedSettings.Empty();
		InGameSettings->Properties.Empty();
		debugf(NAME_DevOnline,TEXT("Packet overflow detected in ReadGameSettingsFromPacket()"));
	}
}

/**
 * Builds a LAN query and broadcasts it
 *
 * @return an error/success code
 */
DWORD UOnlineGameInterfaceImpl::FindLanGames(void)
{
	DWORD Return = S_OK;
	// Recreate the unique identifier for this client
	GenerateNonce(LanNonce,8);
	// Create the lan beacon if we don't already have one
	if (LanBeacon == NULL)
	{
		LanBeacon = new FLanBeacon();
		if (LanBeacon->Init(LanAnnouncePort) == FALSE)
		{
			debugf(NAME_Error,TEXT("Failed to create socket for lan announce port %d"),
				GSocketSubsystem->GetSocketError());
			Return = E_FAIL;
		}
	}
	// If we have a socket and a nonce, broadcast a discovery packet
	if (LanBeacon && Return == S_OK)
	{
		QWORD Nonce = *(QWORD*)LanNonce;
		FNboSerializeToBuffer Packet(LAN_BEACON_MAX_PACKET_SIZE);
		// Build the discovery packet
		Packet << LAN_BEACON_PACKET_VERSION
			// Platform information
			<< (BYTE)appGetPlatformType()
			// Game id to prevent cross game lan packets
			<< LanGameUniqueId
			// Identify the packet type
			<< LAN_SERVER_QUERY1 << LAN_SERVER_QUERY2
			// Append the nonce as a QWORD
			<< Nonce;
		// Now kick off our broadcast which hosts will respond to
		if (LanBeacon->BroadcastPacket(Packet,Packet.GetByteCount()))
		{
#if !FINAL_RELEASE
			debugf(NAME_DevOnline,TEXT("Sent query packet..."));
#endif
			// We need to poll for the return packets
			LanBeaconState = LANB_Searching;
			// Set the timestamp for timing out a search
			LanQueryTimeLeft = LanQueryTimeout;
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to send discovery broadcast %d"),
				GSocketSubsystem->GetSocketError());
			Return = E_FAIL;
		}
	}
	if (Return != S_OK)
	{
		delete LanBeacon;
		LanBeacon = NULL;
		LanBeaconState = LANB_NotUsingLanBeacon;
	}
	return Return;
}

/**
 * Creates an online game based upon the settings object specified.
 * NOTE: online game registration is an async process and does not complete
 * until the OnCreateOnlineGameComplete delegate is called.
 *
 * @param HostingPlayerNum the index of the player hosting the match
 * @param NewGameSettings the settings to use for the new game session
 *
 * @return true if successful creating the session, false otherwsie
 */
UBOOL UOnlineGameInterfaceImpl::CreateOnlineGame(BYTE HostingPlayerNum,UOnlineGameSettings* NewGameSettings)
{
	check(OwningSubsystem && "Was this object created and initialized properly?");
	DWORD Return = E_FAIL;
	// Don't set if we already have a session going
	if (GameSettings == NULL)
	{
		GameSettings = NewGameSettings;
		if (GameSettings != NULL)
		{
			check(SessionInfo == NULL);
			SessionInfo = new FSessionInfo();
			// Init the game settings counts so the host can use them later
			GameSettings->NumOpenPrivateConnections = GameSettings->NumPrivateConnections;
			GameSettings->NumOpenPublicConnections = GameSettings->NumPublicConnections;
			// Copy the unique id of the owning player
			GameSettings->OwningPlayerId = OwningSubsystem->eventGetPlayerUniqueNetIdFromIndex(HostingPlayerNum);
			// Copy the name of the owning player
			GameSettings->OwningPlayerName = OwningSubsystem->eventGetPlayerNicknameFromIndex(HostingPlayerNum);
			// Determine if we are registering a session on our master server or
			// via lan
			if (GameSettings->bIsLanMatch == FALSE)
			{
				Return = CreateInternetGame(HostingPlayerNum);
			}
			else
			{
				// Lan match so do any beacon creation
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
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Creates a new lan enabled game
 *
 * @param HostingPlayerNum the player hosting the game
 *
 * @return S_OK if it succeeded, otherwise an error code
 */
DWORD UOnlineGameInterfaceImpl::CreateLanGame(BYTE HostingPlayerNum)
{
	check(SessionInfo);
	DWORD Return = S_OK;
	// Don't create a lan beacon if advertising is off
	if (GameSettings->bShouldAdvertise == TRUE)
	{
		// Bind a socket for lan beacon activity
		LanBeacon = new FLanBeacon();
		if (LanBeacon->Init(LanAnnouncePort))
		{
			// We successfully created everything so mark the socket as
			// needing polling
			LanBeaconState = LANB_Hosting;
			debugf(NAME_DevOnline,
				TEXT("Listening for lan beacon requestes on %d"),
				LanAnnouncePort);
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to init to lan beacon %d"),
				GSocketSubsystem->GetSocketError());
			Return = E_FAIL;
		}
	}
	if (Return == S_OK)
	{
//@todo joeg -- re-enable once voice support is added
		// Register all local talkers
//		RegisterLocalTalkers();
		CurrentGameState = OGS_Pending;
	}
	else
	{
		// Clean up the session info so we don't get into a confused state
		delete SessionInfo;
		SessionInfo = NULL;
		GameSettings = NULL;
	}
	FAsyncTaskDelegateResults Params(Return);
	TriggerOnlineDelegates(this,
		GameSettings->bIsDedicated ? RegisterDedicatedServerCompleteDelegates : CreateOnlineGameCompleteDelegates,
		&Params);
	return Return;
}

/**
 * Destroys the current online game
 * NOTE: online game de-registration is an async process and does not complete
 * until the OnDestroyOnlineGameComplete delegate is called.
 *
 * @return true if successful destroying the session, false otherwsie
 */
UBOOL UOnlineGameInterfaceImpl::DestroyOnlineGame(void)
{
	DWORD Return = E_FAIL;
	// Don't shut down if it isn't valid
	if (GameSettings != NULL && SessionInfo != NULL)
	{
//@todo joeg -- re-enable once voice support is added
		// Stop all local talkers (avoids a debug runtime warning)
//		UnregisterLocalTalkers();
		// Stop all remote voice before ending the session
//		RemoveAllRemoteTalkers();
		// Determine if this is a lan match or our master server
		if (GameSettings->bIsLanMatch == FALSE)
		{
			Return = DestroyInternetGame();
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
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Terminates a LAN session
 *
 * @return an error/success code
 */
DWORD UOnlineGameInterfaceImpl::DestroyLanGame(void)
{
	check(SessionInfo);
	// Only tear down the beacon if it was advertising
	if (GameSettings->bShouldAdvertise)
	{
		// Tear down the lan beacon
		StopLanBeacon();
	}
	// Clean up before firing the delegate
	delete SessionInfo;
	SessionInfo = NULL;
	// Null out the no longer valid game settings
	GameSettings = NULL;
	// Fire the delegate off immediately
	FAsyncTaskDelegateResults Params(S_OK);
	TriggerOnlineDelegates(this,DestroyOnlineGameCompleteDelegates,&Params);
	return S_OK;
}

/**
 * Searches for games matching the settings specified
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired settings that the returned sessions will have
 *
 * @return true if successful searching for sessions, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::FindOnlineGames(BYTE SearchingPlayerNum,UOnlineGameSearch* SearchSettings)
{
	DWORD Return = E_FAIL;
	// Verify that we have valid search settings
	if (SearchSettings != NULL)
	{
		// Don't start another while in progress or multiple entries for the
		// same server will show up in the server list
		if ((GameSearch && GameSearch->bIsSearchInProgress == FALSE) ||
			GameSearch == NULL)
		{
			// Free up previous results
			FreeSearchResults();
			GameSearch = SearchSettings;
			// Check for master server or lan query
			if (SearchSettings->bIsLanQuery == FALSE)
			{
				FindInternetGames();
			}
			else
			{
				Return = FindLanGames();
			}
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Ignoring game search request while one is pending"));
			Return = ERROR_IO_PENDING;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't search with null criteria"));
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Parses a LAN packet and handles responses/search population
 * as needed
 *
 * @param PacketData the packet data to parse
 * @param PacketLength the amount of data that was received
 */
void UOnlineGameInterfaceImpl::ProcessLanPacket(BYTE* PacketData,INT PacketLength)
{
	// Check our mode to determine the type of allowed packets
	if (LanBeaconState == LANB_Hosting)
	{
		// Don't respond to queries when the match is full
		if (GameSettings->NumOpenPublicConnections > 0)
		{
			QWORD ClientNonce;
			// We can only accept Server Query packets
			if (IsValidLanQueryPacket(PacketData,PacketLength,ClientNonce))
			{
				FNboSerializeToBuffer Packet(LAN_BEACON_MAX_PACKET_SIZE);
				// Add the supported version
				Packet << LAN_BEACON_PACKET_VERSION
					// Platform information
					<< (BYTE)appGetPlatformType()
					// Game id to prevent cross game lan packets
					<< LanGameUniqueId
					// Add the packet type
					<< LAN_SERVER_RESPONSE1 << LAN_SERVER_RESPONSE2
					// Append the client nonce as a QWORD
					<< ClientNonce;
				// Write host info (ip and port)
				Packet << SessionInfo->HostAddr;
				// Now append per game settings
				AppendGameSettingsToPacket(Packet,GameSettings);
				// Broadcast this response so the client can see us
				if (LanBeacon->BroadcastPacket(Packet,Packet.GetByteCount()) == FALSE)
				{
					debugf(NAME_Error,TEXT("Failed to send response packet %d"),
						GSocketSubsystem->GetLastErrorCode());
				}
			}
		}
	}
	else if (LanBeaconState == LANB_Searching)
	{
		// We can only accept Server Response packets
		if (IsValidLanResponsePacket(PacketData,PacketLength))
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
				// Strip off the header since it's been validated
				FNboSerializeFromBuffer Packet(&PacketData[LAN_BEACON_PACKET_HEADER_SIZE],
					PacketLength - LAN_BEACON_PACKET_HEADER_SIZE);
				// Allocate and read the session data
				FSessionInfo* SessInfo = new FSessionInfo(E_NoInit);
				// Read the connection data
				Packet >> SessInfo->HostAddr;
				// Store this in the results
				Result.PlatformData = SessInfo;
				// Read any per object data using the server object
				ReadGameSettingsFromPacket(Packet,NewServer);
				// Let any registered consumers know the data has changed
				FAsyncTaskDelegateResults Params(S_OK);
				TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Params);
			}
			else
			{
				debugf(NAME_Error,TEXT("Failed to create new online game settings object"));
			}
		}
	}
}

/**
 * Cleans up any platform specific allocated data contained in the search results
 */
void UOnlineGameInterfaceImpl::FreeSearchResults(void)
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
					// Free the data
					delete (FSessionInfo*)Result.PlatformData;
				}
			}
			GameSearch->Results.Empty();
			GameSearch = NULL;
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Can't free search results while the search is in progress"));
		}
	}
}

/**
 * Joins the game specified
 *
 * @param PlayerNum the index of the player searching for a match
 * @param DesiredGame the desired game to join
 *
 * @return true if the call completed successfully, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::JoinOnlineGame(BYTE PlayerNum,const FOnlineGameSearchResult& DesiredGame)
{
	DWORD Return = E_FAIL;
	// Don't join a session if already in one or hosting one
	if (SessionInfo == NULL)
	{
		Return = S_OK;
		// Make the selected game our current game settings
		GameSettings = DesiredGame.GameSettings;
		// Create an empty session and fill it based upon game type
		SessionInfo = new FSessionInfo(E_NoInit);
		// Copy the session info over
		appMemcpy(SessionInfo,DesiredGame.PlatformData,sizeof(FSessionInfo));
		// The session info is created/filled differently depending on type
		if (GameSettings->bIsLanMatch == FALSE)
		{
			Return = JoinInternetGame(PlayerNum);
		}
		else
		{
//@todo joeg -- re-enable once voice support is added
			// Register all local talkers for voice
//			RegisterLocalTalkers();
			// Set the state so that StartOnlineGame() can work
			CurrentGameState = OGS_Pending;
			FAsyncTaskDelegateResults Params(S_OK);
			TriggerOnlineDelegates(this,JoinOnlineGameCompleteDelegates,&Params);
		}
		// Handle clean up in one place
		if (Return != S_OK && Return != ERROR_IO_PENDING)
		{
			// Clean up the session info so we don't get into a confused state
			delete SessionInfo;
			SessionInfo = NULL;
			GameSettings = NULL;
		}
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Marks an online game as in progress (as opposed to being in lobby)
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::StartOnlineGame(void)
{
	DWORD Return = E_FAIL;
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Lan matches don't report starting to external services
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Can't start a match multiple times
			if (CurrentGameState == OGS_Pending)
			{
				Return = StartInternetGame();
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
				StopLanBeacon();
			}
			Return = S_OK;
			CurrentGameState = OGS_InProgress;
			// Indicate that the start completed
			FAsyncTaskDelegateResults Params(Return);
			TriggerOnlineDelegates(this,StartOnlineGameCompleteDelegates,&Params);
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't start an online game that hasn't been created"));
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Marks an online game as having been ended
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::EndOnlineGame(void)
{
	DWORD Return = E_FAIL;
	if (GameSettings != NULL && SessionInfo != NULL)
	{
		// Lan matches don't report ending to master server
		if (GameSettings->bIsLanMatch == FALSE)
		{
			// Can't end a match that isn't in progress
			if (CurrentGameState == OGS_InProgress)
			{
				Return = EndInternetGame();
				CurrentGameState = OGS_Ending;
			}
			else
			{
				debugf(NAME_DevOnline,TEXT("Can't end an online game in state %d"),CurrentGameState);
			}
		}
		else
		{
			Return = S_OK;
			FAsyncTaskDelegateResults Params(Return);
			TriggerOnlineDelegates(this,EndOnlineGameCompleteDelegates,&Params);
			CurrentGameState = OGS_Ended;
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't end an online game that hasn't been created"));
	}
	if (Return != S_OK && Return != ERROR_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Params(Return);
		TriggerOnlineDelegates(this,EndOnlineGameCompleteDelegates,&Params);
		CurrentGameState = OGS_Ended;
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Returns the platform specific connection information for joining the match.
 * Call this function from the delegate of join completion
 *
 * @param ConnectInfo the out var containing the platform specific connection information
 *
 * @return true if the call was successful, false otherwise
 */
UBOOL UOnlineGameInterfaceImpl::GetResolvedConnectString(FString& ConnectInfo)
{
	UBOOL bOk = FALSE;
	if (SessionInfo != NULL)
	{
		// Copy the destination IP and port information
		ConnectInfo = SessionInfo->HostAddr.ToString(TRUE);
		bOk = TRUE;
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't decrypt a NULL session's IP"));
	}
	return bOk;
}
