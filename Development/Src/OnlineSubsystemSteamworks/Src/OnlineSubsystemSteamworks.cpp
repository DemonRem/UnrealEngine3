/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemSteamworks.h"

#if WITH_UE3_NETWORKING && WITH_STEAMWORKS

// Unreal delegates that we never trigger:
// (invites happen in the Steam client or overlay. If the user accepts, we'll catch the callback to join game.)
//   ReceivedGameInviteDelegates
//   FriendInviteReceivedDelegates
//   FriendMessageReceivedDelegates
// (login/logout happens outside game process. We do send a fake LoginChangedDelegate on startup, though.)
//   LoginFailedDelegates
//   LogoutCompletedDelegates
// (steam handles account creation before you can start at all.)
//   AccountCreateDelegates
// (steam handles buddly list management.)
//   AddFriendByNameCompleteDelegates
// (appears to be a GameSpy-only thing.)
//   RegisterHostStatGuidCompleteDelegates
// (this is for the PS3 on-screen keyboard, gamepads, etc)
//   KeyboardInputDelegates
//   ControllerChangeDelegates

// need to handle:
// (!!! FIXME: handle callback notifications.)
//   JoinFriendGameCompleteDelegates (maybe)
//   GameInviteAcceptedDelegates (maybe)

// Cached Steamworks SDK interface pointers...
ISteamUtils *GSteamUtils = NULL;
ISteamUser *GSteamUser = NULL;
ISteamFriends *GSteamFriends = NULL;
ISteamRemoteStorage *GSteamRemoteStorage = NULL;
ISteamUserStats *GSteamUserStats = NULL;
ISteamMatchmakingServers *GSteamMatchmakingServers = NULL;
ISteamGameServer *GSteamGameServer = NULL;
ISteamMasterServerUpdater *GSteamMasterServerUpdater = NULL;
ISteamApps *GSteamApps = NULL;
uint32 GSteamAppID = 0;

#if HAVE_STEAM_GAMESERVER_STATS
ISteamGameServerStats *GSteamGameServerStats = NULL;
#endif

static inline const TCHAR *BoolToString(const bool Value)
{
	return Value ? TEXT("TRUE") : TEXT("FALSE");
}

static inline UBOOL IsServer()
{
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	if (WorldInfo)
	{
		return ((WorldInfo->NetMode == NM_ListenServer) || ( WorldInfo->NetMode == NM_DedicatedServer));
	}
	return FALSE;
}

// !!! FIXME: The current steam_api.lib in the SDK doesn't have CSteamID::Render(void), even though it's in the headers.
static inline FString RenderCSteamID(const CSteamID &Id)
{
	const EAccountType AccountType = Id.GetEAccountType();
	const DWORD Universe = (DWORD) Id.GetEUniverse();
	const uint32 AccountID = Id.GetAccountID();
	const uint32 AccountInstance = Id.GetUnAccountInstance();

	switch (AccountType)
	{
		case k_EAccountTypeAnonGameServer:
			return FString::Printf(TEXT("[A-%u:%u(%u)]"), Universe, AccountID, AccountInstance);
		
		case k_EAccountTypeGameServer:
			return FString::Printf(TEXT("[G-%u:%u]"), Universe, AccountID);

		case k_EAccountTypeMultiseat:
			return FString::Printf(TEXT("[%u:%u(%u%)]"), Universe, AccountID, AccountInstance);

		case k_EAccountTypePending:
			return FString::Printf(TEXT("[%u:%u(pending)]"), Universe, AccountID);

		default:
			return FString::Printf(TEXT("[%u:%u]"), Universe, AccountID);
	}
}

/**
 * Searches the WorldInfo's PRI array for the specified player so that the
 * player's APlayerController can be found
 *
 * @param Player the player that is being searched for
 *
 * @return the controller for the player, NULL if not found
 */
static APlayerController* FindPlayerControllerForUniqueId(FUniqueNetId Player)
{
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	if (WorldInfo)
	{
		AGameReplicationInfo* GRI = WorldInfo->GRI;
		if (GRI != NULL)
		{
			// Find the PRI that matches the net id
			for (INT Index = 0; Index < GRI->PRIArray.Num(); Index++)
			{
				APlayerReplicationInfo* PRI = GRI->PRIArray(Index);
				// If this PRI matches, get the owning actor
				if (PRI && PRI->UniqueId == Player)
				{
					return Cast<APlayerController>(PRI->Owner);
				}
			}
		}
	}
	// Couldn't be found
	return NULL;
}

/**
 * Searches the WorldInfo's PRI array for the specified player so that the
 * player's net connection can be found
 *
 * @param Player the player that is being searched for
 *
 * @return the net connection for the player, NULL if not found
 */
static UNetConnection* FindConnectionForUniqueId(FUniqueNetId Player)
{
	// Get the player controller so we can get the player (which is the net connection)
	APlayerController* PC = FindPlayerControllerForUniqueId(Player);
	if (PC)
	{
		return Cast<UNetConnection>(PC->Player);
	}
	// Couldn't be found
	return NULL;
}

// some glue code to keep all the API calls in here.
UBOOL appSteamAuthenticate(DWORD IP, BYTE* AuthBlob, DWORD BlobLen, QWORD* Uid)
{
	UBOOL Return = FALSE;

	if (GSteamGameServer != NULL)
	{
		CSteamID SteamId;
		Return = GSteamGameServer->SendUserConnectAndAuthenticate(IP, AuthBlob, BlobLen, &SteamId);
		*Uid = Return ? ((QWORD) SteamId.ConvertToUint64()) : ((QWORD) 0);
	}

	return Return;
}

QWORD appSteamGameServer_GetSteamID()
{
	return (QWORD) SteamGameServer_GetSteamID();
}

INT appSteamGameServer_BSecure()
{
	return SteamGameServer_BSecure() ? 1 : 0;
}

INT appSteamInitiateGameConnection(void *AuthBlob, INT BlobSize, QWORD ServerSteamID, DWORD IPAddr, INT Port, UBOOL ServerVACEnabled)
{
	if (GSteamworksInitialized)
	{
		return GSteamUser->InitiateGameConnection(AuthBlob, BlobSize, CSteamID(ServerSteamID), IPAddr, Port, ServerVACEnabled != FALSE);
	}
	return 0;
}

void appSteamworksTerminateGameConnection(DWORD Addr, INT Port)
{
	if (GSteamworksInitialized)
	{
		GSteamUser->TerminateGameConnection(Addr, Port);
	}
}

void appSteamworksSendUserDisconnect(FUniqueNetId UID)
{
	if (GSteamworksInitialized && GSteamGameServer != NULL)
	{
		const CSteamID SteamPlayerID((uint64)UID.Uid);
		GSteamGameServer->SendUserDisconnect(SteamPlayerID);
	}
}

static inline EOnlineServerConnectionStatus ConnectionResult(const EResult Result)
{
	switch (Result)
	{
		case k_EResultAdministratorOK:
		case k_EResultOK: 
			return OSCS_Connected;
			
		case k_EResultNoConnection:
			return OSCS_NoNetworkConnection;

		case k_EResultInvalidPassword:
		case k_EResultNotLoggedOn:
		case k_EResultAccessDenied:
		case k_EResultBanned:
		case k_EResultAccountNotFound:
		case k_EResultInvalidSteamID:
		case k_EResultRevoked:
		case k_EResultExpired:
		case k_EResultAlreadyRedeemed:
		case k_EResultBlocked:
		case k_EResultIgnored:
		case k_EResultAccountDisabled:
		case k_EResultAccountNotFeatured:
		case k_EResultInsufficientPrivilege:
			return OSCS_InvalidUser;

		case k_EResultLogonSessionReplaced:
		case k_EResultRemoteDisconnect:
		case k_EResultLoggedInElsewhere:
			return OSCS_DuplicateLoginDetected;

		case k_EResultInvalidProtocolVer:
		case k_EResultContentVersion:
			return OSCS_UpdateRequired;

		case k_EResultBusy:
			return OSCS_ServersTooBusy;

		default:
			return OSCS_ServiceUnavailable;
	}
}

void SteamCallbackBridge::OnGSClientAchievementStatus(GSClientAchievementStatus_t *CallbackData)
{
	// This is just for informational purposes at the moment. We never actually make the API call that would generate this.
	FUniqueNetId PlayerNetId;
	PlayerNetId.Uid = (QWORD) CallbackData->m_SteamID;
	APlayerController *Controller = FindPlayerControllerForUniqueId(PlayerNetId);
	FString Persona(Controller ? *Controller->PlayerReplicationInfo->PlayerName : TEXT("[???]"));
	debugf(NAME_DevOnline, TEXT("OnGSClientAchievementStatus: '%s' (%s), '%s', %s"),
		*Persona, *RenderCSteamID(CSteamID(CallbackData->m_SteamID)), UTF8_TO_TCHAR(CallbackData->m_pchAchievement), BoolToString(CallbackData->m_bUnlocked != 0)
	);
}

void SteamCallbackBridge::OnClientGameServerDeny(ClientGameServerDeny_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("OnClientGameServerDeny"));
	if (!GIsClient)
	{
		debugf(NAME_DevOnline, TEXT("OnClientGameServerDeny, but we're not a client. Ignoring."));
		return;
	}

	if (CallbackData->m_uAppID != GSteamAppID)
	{
		debugf(NAME_DevOnline, TEXT("OnClientGameServerDeny for wrong appid. Ignoring."));
		return;
	}

	UNetDriver* NetDriver = GWorld->GetNetDriver();
	if ((NetDriver == NULL) && (Cast<UGameEngine>(GEngine) != NULL && ((UGameEngine*)GEngine)->GPendingLevel != NULL))
	{
		NetDriver = ((UGameEngine*)GEngine)->GPendingLevel->NetDriver;
	}

	if (NetDriver && NetDriver->ServerConnection)
	{
		if ( (NetDriver->ServerConnection->GetAddrAsInt() == ((INT) CallbackData->m_unGameServerIP)) &&
		     (NetDriver->ServerConnection->GetAddrPort() == ((INT) CallbackData->m_usGameServerPort)) )
		{
			debugf(NAME_DevOnline, TEXT("OnClientGameServerDeny telling us to disconnect (secure=%d, reason=%d)"), (INT) CallbackData->m_bSecure, (INT) CallbackData->m_uReason);
			GEngine->Exec(TEXT("DISCONNECT"));
			return;
		}
	}

	debugf(NAME_DevOnline, TEXT("OnClientGameServerDeny for server we don't appear to be on. Ignoring."));
}

void SteamCallbackBridge::OnGameServerChangeRequested(GameServerChangeRequested_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("Steam callback: OnGameServerChangeRequested"));
	if (!GIsClient)
	{
		debugf(NAME_DevOnline, TEXT("We're not a client, not changing servers!"));
		return;
	}

	FString Url(FString::Printf(TEXT("%s://%s"), *FURL::DefaultProtocol, UTF8_TO_TCHAR(CallbackData->m_rgchServer)));
	if (CallbackData->m_rgchPassword[0])  // password specified?
	{
		Url += FString::Printf(TEXT("?Password=%s"), UTF8_TO_TCHAR(CallbackData->m_rgchPassword));
	}

	GEngine->SetClientTravel(*Url, TRAVEL_Absolute);
}


static inline const TCHAR *DenyReasonString(const EDenyReason Reason)
{
	switch (Reason)
	{
		case k_EDenyInvalid: return TEXT("Invalid");
		case k_EDenyInvalidVersion: return TEXT("Invalid version");
		case k_EDenyGeneric: return TEXT("Generic denial");
		case k_EDenyNotLoggedOn: return TEXT("Not logged on");
		case k_EDenyNoLicense: return TEXT("No license");
		case k_EDenyCheater: return TEXT("VAC banned");
		case k_EDenyLoggedInElseWhere: return TEXT("Logged in elsewhere");
		case k_EDenyUnknownText: return TEXT("Unknown text");
		case k_EDenyIncompatibleAnticheat: return TEXT("Incompatible anti-cheat");
		case k_EDenyMemoryCorruption: return TEXT("Memory corruption detected on client");
		case k_EDenyIncompatibleSoftware: return TEXT("Incompatible software");
		case k_EDenySteamConnectionLost: return TEXT("Steam connection lost");
		case k_EDenySteamConnectionError: return TEXT("Steam connection error");
		case k_EDenySteamResponseTimedOut: return TEXT("Steam response timed out");
		case k_EDenySteamValidationStalled: return TEXT("Steam validation stalled");
		case k_EDenySteamOwnerLeftGuestUser: return TEXT("Steam owner left guest user");
	}

	return TEXT("???");
}

void SteamCallbackBridge::OnGSClientApprove(GSClientApprove_t *CallbackData)
{
	Subsystem->OnGSClientApprove(CallbackData);
}

void UOnlineSubsystemSteamworks::OnGSClientApprove(GSClientApprove_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("OnGSClientApprove"));

	if (!IsServer())
	{
		debugf(NAME_DevOnline, TEXT("We're not the server, not approving!"));
		return;
	}

	FUniqueNetId UniqueId;
	UniqueId.Uid = (QWORD) CallbackData->m_SteamID.ConvertToUint64();

	if (GWorld && !GWorld->HandleOnlineSubsystemAuth(UniqueId, TRUE))
	{
		debugf(NAME_DevOnline, TEXT("No player matching id %s, not approving!"), *RenderCSteamID(CallbackData->m_SteamID));
		return;
	}

	debugf(NAME_DevOnline, TEXT("Aproving player at Steam's request. id: %s"), *RenderCSteamID(CallbackData->m_SteamID));


	// If the listen host has just been authenticated, let script know through the online subsystem
	AWorldInfo* WI = GWorld->GetWorldInfo();

	if (WI != NULL && WI->NetMode == NM_ListenServer && UniqueId == LoggedInPlayerId)
		bListenHostAuthenticated = TRUE;


	// Refresh the game settings.
	if (CachedGameInt)
	{
		CachedGameInt->RefreshPublishedGameSettings();
	}
}

void SteamCallbackBridge::OnGSClientDeny(GSClientDeny_t *CallbackData)
{
	Subsystem->OnGSClientDeny(CallbackData);
}

void UOnlineSubsystemSteamworks::OnGSClientDeny(GSClientDeny_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("OnGSClientDeny"));

	if (!IsServer())
	{
		debugf(NAME_DevOnline, TEXT("We're not the server, not denying!"));
		return;
	}

	FUniqueNetId UniqueId;
	UniqueId.Uid = (QWORD) CallbackData->m_SteamID.ConvertToUint64();

	if ((GIsClient) && (UniqueId.Uid == LoggedInPlayerId.Uid))
	{
		// uhoh! 
		// No NAME_DevOnline on this, always want it in the server logfile for the admins.
		debugf(TEXT("We're a listen server and we failed our own auth!"));
		GEngine->Exec(TEXT("DISCONNECT"));
		return;
	}

	if (GWorld && !GWorld->HandleOnlineSubsystemAuth(UniqueId, FALSE))
	{
		debugf(NAME_DevOnline, TEXT("No player matching id %s, not denying!"), *RenderCSteamID(CallbackData->m_SteamID));
		return;
	}

	// No NAME_DevOnline on this, always want it in the server logfile for the admins.
	const FString DenyReason(DenyReasonString(CallbackData->m_eDenyReason));
	debugf(TEXT("Denying player at Steam's request. id: %s, reason: %s, opttext: '%s'"),
		*RenderCSteamID(CallbackData->m_SteamID), *DenyReason, UTF8_TO_TCHAR(CallbackData->m_rgchOptionalText));


	// If the listen host failed authentication, log this event
	AWorldInfo* WI = GWorld->GetWorldInfo();

	if (WI != NULL && WI->NetMode == NM_ListenServer && UniqueId == LoggedInPlayerId)
	{
		FString FailExtra = UTF8_TO_TCHAR(CallbackData->m_rgchOptionalText);

		debugf(TEXT("Failed Steam auth for listen host (%s : %s), stats updates may fail for this player"), *DenyReason, *FailExtra);

		bListenHostPendingAuth = FALSE;
	}




	// Refresh the game settings.
	if (CachedGameInt)
	{
		CachedGameInt->RefreshPublishedGameSettings();
	}
}

void SteamCallbackBridge::OnGSClientKick(GSClientKick_t *CallbackData)
{
	Subsystem->OnGSClientKick(CallbackData);
}

void UOnlineSubsystemSteamworks::OnGSClientKick(GSClientKick_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("OnGSClientKick"));

	if (!IsServer())
	{
		debugf(NAME_DevOnline, TEXT("We're not the server, not kicking!"));
		return;
	}

	AGameInfo *GameInfo = NULL;
	if (GWorld && GWorld->GetWorldInfo())
	{
		GameInfo = GWorld->GetWorldInfo()->Game;
	}

	if (!GameInfo)
	{
		debugf(NAME_DevOnline, TEXT("No GameInfo, not kicking!"));
		return;
	}

	FUniqueNetId UniqueId;
	UniqueId.Uid = (QWORD) CallbackData->m_SteamID.ConvertToUint64();

	APlayerController *Controller = FindPlayerControllerForUniqueId(UniqueId);
	if (!Controller)
	{
		// Try kicking players that might still be going through initial auth handshake, just in case.
		if (!GWorld->HandleOnlineSubsystemAuth(UniqueId, FALSE))
		{
			debugf(NAME_DevOnline, TEXT("No player matching id %s, not kicking!"), *RenderCSteamID(CallbackData->m_SteamID));
			return;
		}
	}

	// No NAME_DevOnline on this, always want it in the server logfile for the admins.
	const FString KickReason(DenyReasonString(CallbackData->m_eDenyReason));
	debugf(TEXT("Kicking player at Steam's request. id: %s ('%s'), reason: %s"), *RenderCSteamID(CallbackData->m_SteamID), *Controller->PlayerReplicationInfo->PlayerName, *KickReason);
	GameInfo->eventForceKickPlayer(Controller, KickReason);

	if ((GIsClient) && (UniqueId.Uid == LoggedInPlayerId.Uid))
	{
		// uhoh! 
		// No NAME_DevOnline on this, always want it in the server logfile for the admins.
		debugf(TEXT("We're a listen server and we were kicked from our own game!"));
		GEngine->Exec(TEXT("DISCONNECT"));
		return;
	}

	// Refresh the game settings.
	if (CachedGameInt)
	{
		CachedGameInt->RefreshPublishedGameSettings();
	}
}

void SteamCallbackBridge::OnSteamShutdown(SteamShutdown_t *CallbackData)
{
	// No NAME_DevOnline on this debugf, so this logging isn't ever suppressed.
	debugf(TEXT("Steam client is shutting down; exiting, too."));

	// Steam client is shutting down, so go with it...
	appRequestExit(FALSE);  // ...but don't force immediate process kill.
}

void SteamCallbackBridge::OnGameOverlayActivated(GameOverlayActivated_t *CallbackData)
{
	const bool Active = (CallbackData->m_bActive != 0);
	debugf(NAME_DevOnline, TEXT("Steam GameOverlayActivated: %s"), BoolToString(Active));
	if (GEngine)
	{
		// try to pause the game, if allowed, as if we just lost the window manager's focus.
		GEngine->OnLostFocusPause(Active);
	}
}

void SteamCallbackBridge::OnGSPolicyResponse(GSPolicyResponse_t *CallbackData)
{
	Subsystem->OnGSPolicyResponse(CallbackData);
};

void SteamCallbackBridge::OnSteamServersConnected(SteamServersConnected_t *CallbackData)
{
	Subsystem->OnSteamServersConnected(CallbackData);
}

void SteamCallbackBridge::OnSteamServersDisconnected(SteamServersDisconnected_t *CallbackData)
{
	Subsystem->OnSteamServersDisconnected(CallbackData);
}

void SteamCallbackBridge::OnUserStatsReceived(UserStatsReceived_t *CallbackData)
{
	Subsystem->OnUserStatsReceived(CallbackData);
}

void SteamCallbackBridge::OnUserStatsStored(UserStatsStored_t *CallbackData)
{
	Subsystem->OnUserStatsStored(CallbackData);
}

void SteamCallbackBridge::OnSpecificUserStatsReceived(UserStatsReceived_t *CallbackData, bool bIOFailure)
{
	Subsystem->OnSpecificUserStatsReceived(CallbackData, bIOFailure);
}

#if HAVE_STEAM_GAMESERVER_STATS
void SteamCallbackBridge::OnSpecificGSStatsReceived(GSStatsReceived_t *CallbackData, bool bIOFailure)
{
	Subsystem->OnSpecificGSStatsReceived(CallbackData, bIOFailure);
}

void SteamCallbackBridge::OnSpecificGSStatsStored(GSStatsStored_t *CallbackData, bool bIOFailure)
{
	Subsystem->OnSpecificGSStatsStored(CallbackData, bIOFailure);
}
#endif

void SteamCallbackBridge::OnNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *CallbackData, bool bIOFailure)
{
	Subsystem->OnNumberOfCurrentPlayers(CallbackData, bIOFailure);
}

void SteamCallbackBridge::OnUserFindLeaderboard(LeaderboardFindResult_t* CallbackData, bool bIOFailure)
{
	Subsystem->OnUserFindLeaderboard(CallbackData, bIOFailure);
}

void SteamCallbackBridge::OnUserDownloadedLeaderboardEntries(LeaderboardScoresDownloaded_t* CallbackData, bool bIOFailure)
{
	Subsystem->OnUserDownloadedLeaderboardEntries(CallbackData, bIOFailure);
}

void SteamCallbackBridge::OnUserUploadedLeaderboardScore(LeaderboardScoreUploaded_t* CallbackData, bool bIOFailure)
{
	Subsystem->OnUserUploadedLeaderboardScore(CallbackData, bIOFailure);
}

void UOnlineSubsystemSteamworks::OnGSPolicyResponse(GSPolicyResponse_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("OnGSPolicyResponse: VAC Secured==%s"), BoolToString(CallbackData->m_bSecure != 0));
	if (CachedGameInt != NULL)
	{
		CachedGameInt->OnGSPolicyResponse(CallbackData->m_bSecure != 0);
	}
};

void UOnlineSubsystemSteamworks::OnSteamServersConnected(SteamServersConnected_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("Steam Servers Connected"));
	OnlineSubsystemSteamworks_eventOnConnectionStatusChange_Parms ConnectionParms(EC_EventParm);
	ConnectionParms.ConnectionStatus = OSCS_Connected;
	TriggerOnlineDelegates(this,ConnectionStatusChangeDelegates,&ConnectionParms);
}

void UOnlineSubsystemSteamworks::OnSteamServersDisconnected(SteamServersDisconnected_t *CallbackData)
{
	debugf(NAME_DevOnline, TEXT("Steam Servers Disconnected: %d"), (INT) CallbackData->m_eResult);
	OnlineSubsystemSteamworks_eventOnConnectionStatusChange_Parms ConnectionParms(EC_EventParm);
	ConnectionParms.ConnectionStatus = ConnectionResult(CallbackData->m_eResult);
	TriggerOnlineDelegates(this,ConnectionStatusChangeDelegates,&ConnectionParms);
}

void UOnlineSubsystemSteamworks::OnUserStatsReceived(UserStatsReceived_t *CallbackData)
{
	const CGameID GameID(GSteamAppID);
	if (GameID.ToUint64() != CallbackData->m_nGameID)
	{
		debugf(NAME_DevOnline, TEXT("Obtained steam user stats, but for wrong game! Ignoring."));
		return;
	}

	if (CallbackData->m_eResult != k_EResultOK)
	{
		debugf(NAME_DevOnline, TEXT("Failed to obtained steam user stats, error: %i."), (int)CallbackData->m_eResult);
		UserStatsReceivedState = OERS_Failed;
		return;
	}

	debugf(NAME_DevOnline, TEXT("Obtained steam user stats"));
	UserStatsReceivedState = OERS_Done;

	OnlineSubsystemSteamworks_eventOnReadAchievementsComplete_Parms Parms(EC_EventParm);
	Parms.TitleId = 0;  // !!! FIXME: ?
	TriggerOnlineDelegates(this,AchievementReadDelegates,&Parms);
}

void UOnlineSubsystemSteamworks::OnUserStatsStored(UserStatsStored_t *CallbackData)
{
	const CGameID GameID(GSteamAppID);
	if (GameID.ToUint64() != CallbackData->m_nGameID)
	{
		debugf(NAME_DevOnline, TEXT("Stored steam user stats, but for wrong game! Ignoring."));
		return;
	}

	DWORD Result = ERROR_SUCCESS;
	if (CallbackData->m_eResult == k_EResultOK)
	{
		debugf(NAME_DevOnline, TEXT("Stored steam user stats."));
	}
	else
	{
		Result = E_FAIL;
		debugf(NAME_DevOnline, TEXT("Failed to store steam user stats, error: %i."), (int)CallbackData->m_eResult);
	}

	if (bStoringAchievement)
	{
		FAsyncTaskDelegateResults Results(Result);
		TriggerOnlineDelegates(this,AchievementDelegates,&Results);
		bStoringAchievement = FALSE;
	}

#if HAVE_STEAM_CLIENT_STATS
	// If this is the client, and a stats store was pending, it is now complete; trigger the callbacks
	if (!IsServer() && TotalGSStatsStoresPending != 0)
	{
		TotalGSStatsStoresPending = 0;

		// NOTE: bGSStatsStoresSuccess is used by both client and game server stats code (no need to duplicate)
		if (CallbackData->m_eResult != k_EResultOK)
			bGSStatsStoresSuccess = FALSE;

		FAsyncTaskDelegateResultsNamedSession Params(FName(TEXT("Game")), (bGSStatsStoresSuccess ? S_OK : E_FAIL));
		TriggerOnlineDelegates(this, FlushOnlineStatsDelegates, &Params);
	}
#endif
}

void UOnlineSubsystemSteamworks::OnNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *CallbackData, bool bIOFailure)
{
	INT Count = -1;
	if (!bIOFailure && CallbackData->m_bSuccess && CallbackData->m_cPlayers >= 0)
	{
		Count = CallbackData->m_cPlayers;
	}

	debugf(NAME_DevOnline, TEXT("Steam OnNumberOfCurrentPlayers: %d"), Count);

	OnlineSubsystemSteamworks_eventOnGetNumberOfCurrentPlayersComplete_Parms NumberParms(EC_EventParm);
	NumberParms.TotalPlayers = Count;
	TriggerOnlineDelegates(this,GetNumberOfCurrentPlayersCompleteDelegates,&NumberParms);
}

UBOOL UOnlineSubsystemSteamworks::GetNumberOfCurrentPlayers()
{
	CallbackBridge->GetNumberOfCurrentPlayers();
	return TRUE;
}


void UOnlineSubsystemSteamworks::OnUserFindLeaderboard(LeaderboardFindResult_t* CallbackData, bool bIOFailure)
{
	PendingLeaderboardInits--;

	// If all pending finds requests have returned, clear the callback list
	if (PendingLeaderboardInits <= 0)
	{
		PendingLeaderboardInits = 0;
		CallbackBridge->EmptyUserFindLeaderboard();
	}

	// Initiate the 'LeaderboardList' entry, call deferred reads/writes, and handle failures
	if (!bIOFailure && CallbackData->m_bLeaderboardFound != 0 && CallbackData->m_hSteamLeaderboard != NULL)
	{
		// Get the leaderboard name, for looking it up in 'LeaderboardList'
		FString LeaderboardName(UTF8_TO_TCHAR(GSteamUserStats->GetLeaderboardName(CallbackData->m_hSteamLeaderboard)));

		INT ListIndex = INDEX_NONE;

		for (INT i=0; i<LeaderboardList.Num(); i++)
		{
			if (LeaderboardList(i).LeaderboardName == LeaderboardName)
			{
				// Copy over the OnlineSubsystem version of the name, to keep the case consistent in UScript
				LeaderboardName = LeaderboardList(i).LeaderboardName;

				ListIndex = i;
				break;
			}
		}

		// If the returned leaderboard is in 'LeaderboardList', store leaderboard info, and kickoff deferred read/write requests
		if (ListIndex != INDEX_NONE)
		{
			LeaderboardList(ListIndex).LeaderboardRef = CallbackData->m_hSteamLeaderboard;


			// Store extra leaderboard info
			LeaderboardList(ListIndex).LeaderboardSize = GSteamUserStats->GetLeaderboardEntryCount(CallbackData->m_hSteamLeaderboard);

			// SortType
			ELeaderboardSortMethod APISortMethod = GSteamUserStats->GetLeaderboardSortMethod(CallbackData->m_hSteamLeaderboard);

			if (APISortMethod == k_ELeaderboardSortMethodNone || APISortMethod == k_ELeaderboardSortMethodAscending)
				LeaderboardList(ListIndex).SortType = LST_Ascending;
			else // if (APISortMethod == k_ELeaderboardSortMethodDescending)
				LeaderboardList(ListIndex).SortType = LST_Descending;

			// DisplayFormat
			ELeaderboardDisplayType APIDisplayType = GSteamUserStats->GetLeaderboardDisplayType(CallbackData->m_hSteamLeaderboard);

			if (APIDisplayType == k_ELeaderboardDisplayTypeNone || APIDisplayType == k_ELeaderboardDisplayTypeNumeric)
				LeaderboardList(ListIndex).DisplayFormat = LF_Number;
			else if (APIDisplayType == k_ELeaderboardDisplayTypeTimeSeconds)
				LeaderboardList(ListIndex).DisplayFormat = LF_Seconds;
			else // if (APIDisplayType == k_ELeaderboardDisplayTypeTimeMilliSeconds)
				LeaderboardList(ListIndex).DisplayFormat = LF_Milliseconds;


			LeaderboardList(ListIndex).bLeaderboardInitiated = TRUE;
			LeaderboardList(ListIndex).bLeaderboardInitializing = FALSE;


			// Kickoff deferred read/write requests
			for (INT i=0; i<DeferredLeaderboardReads.Num(); i++)
			{
				if (DeferredLeaderboardReads(i).LeaderboardName == LeaderboardName)
				{
					if (!ReadLeaderboardEntries(LeaderboardName, DeferredLeaderboardReads(i).RequestType,
									DeferredLeaderboardReads(i).Start, DeferredLeaderboardReads(i).End))
					{
						debugf(TEXT("OnUserFindLeaderboard: Deferred leaderboard read failed, leaderboard name: %s"),
								*LeaderboardName);
					}

					DeferredLeaderboardReads.Remove(i, 1);
					i--;
				}
			}

			for (INT i=0; i<DeferredLeaderboardWrites.Num(); i++)
			{
				if (DeferredLeaderboardWrites(i).LeaderboardName == LeaderboardName)
				{
					if (!WriteLeaderboardScore(LeaderboardName, DeferredLeaderboardWrites(i).Score))
					{
						debugf(TEXT("OnUserFindLeaderboard: Deferred leaderboard write failed, leaderboard name: %s"),
								*LeaderboardName);
					}

					DeferredLeaderboardWrites.Remove(i, 1);
					i--;
				}
			}
		}
		else
		{
			FString LogMsg = FString::Printf(TEXT("%s '%s' %s (%s)"), TEXT("OnUserFindLeaderboard: Returned leaderboard name"),
								*LeaderboardName, TEXT("was not in 'LeaderboardList' array."),
								TEXT("ignore if triggered by CreateLeaderboard"));

			debugf(TEXT("%s"), *LogMsg);
		}
	}
	else
	{
		debugf(TEXT("OnUserFindLeaderboard: IOError, or leaderboard was not found"));
	}
}

void UOnlineSubsystemSteamworks::OnUserDownloadedLeaderboardEntries(LeaderboardScoresDownloaded_t* CallbackData, bool bIOFailure)
{
	PendingLeaderboardReads--;

	// If all pending read requests have returned, clear the callback list
	if (PendingLeaderboardReads <= 0)
	{
		PendingLeaderboardReads = 0;
		CallbackBridge->EmptyUserDownloadedLeaderboardEntries();
	}


	UBOOL bResult = FALSE;

	if (!bIOFailure && CallbackData->m_hSteamLeaderboard != NULL)
	{
		INT ListIndex = INDEX_NONE;

		// Find the leaderboard in 'LeaderboardList', based on the leaderboard handle
		for (INT i=0; i<LeaderboardList.Num(); i++)
		{
			if (LeaderboardList(i).LeaderboardRef == CallbackData->m_hSteamLeaderboard)
			{
				ListIndex = i;
				break;
			}
		}

		// Verify that the returned leaderboard entries are from an entry in 'LeaderboardList'
		if (ListIndex != INDEX_NONE)
		{
			// Update the leaderboard entry count
			LeaderboardList(ListIndex).LeaderboardSize = GSteamUserStats->GetLeaderboardEntryCount(CallbackData->m_hSteamLeaderboard);

			// Fill in the player list
			TArray<FUniqueNetId> Players;
			TArray<FLeaderboardEntry> LeaderboardEntries;

			if (CallbackData->m_cEntryCount == 0)
				bResult = TRUE;


			for (INT i=0; i<CallbackData->m_cEntryCount; i++)
			{
				LeaderboardEntry_t CurEntry;

				if (GSteamUserStats->GetDownloadedLeaderboardEntry(CallbackData->m_hSteamLeaderboardEntries, i, &CurEntry, NULL, 0))
				{
					bResult = TRUE;

					// Add to 'Players' list
					FUniqueNetId CurPlayer;
					CurPlayer.Uid = CurEntry.m_steamIDUser.ConvertToUint64();

					Players.AddItem(CurPlayer);


					// Add to 'LeaderboardEntries' list
					INT j = LeaderboardEntries.AddZeroed(1);

					LeaderboardEntries(j).PlayerUID = CurPlayer;
					LeaderboardEntries(j).Rank = CurEntry.m_nGlobalRank;
					LeaderboardEntries(j).Score = CurEntry.m_nScore;
				}
			}


			// If successful, pass the list on to ReadOnlineStats
			if (bResult)
			{
				if (CurrentStatsRead != NULL)
				{
					// Verify that the returned leaderboard data, is for this StatsRead
					FString StatsReadLeaderboard = LeaderboardNameLookup(CurrentStatsRead->ViewId);

					if (!StatsReadLeaderboard.IsEmpty() && StatsReadLeaderboard == LeaderboardList(ListIndex).LeaderboardName)
					{
						// Setup the ProcessedLeaderboardReads list
						ProcessedLeaderboardReads = LeaderboardEntries;

						UOnlineStatsRead* CurStatsRead = CurrentStatsRead;
						CurrentStatsRead = NULL;

						ReadOnlineStats(Players, CurStatsRead);
					}
					else
					{
						// NOTE: This is not a failure (since the correct results may be on the way), just unexpected
						debugf(TEXT("Got leaderboard results which do not match CurrentStatsRead; results leaderboard: %s"),
							*LeaderboardList(ListIndex).LeaderboardName);

						bResult = TRUE;
					}
				}
				else
				{
					debugf(TEXT("Got leaderboard results, when CurrentStatsRead is NULL"));
				}
			}
		}
		else
		{
			TCHAR* LName = UTF8_TO_TCHAR(GSteamUserStats->GetLeaderboardName(CallbackData->m_hSteamLeaderboard));

			debugf(TEXT("Got leaderboard read results for a leaderboard not in 'LeaderboardList', name: %s"), LName);
		}
	}
	else
	{
		debugf(TEXT("IOFailure when reading leaderboard entries"));
	}

	if (!bResult)
	{
		debugf(TEXT("OnUserDownloadedLeaderboardEntries: ReadOnlineStats (ranked) has failed"));

		CurrentStatsRead = NULL;

		FAsyncTaskDelegateResults Param((bResult ? S_OK : E_FAIL));
		TriggerOnlineDelegates(this, ReadOnlineStatsCompleteDelegates, &Param);
	}
}

void UOnlineSubsystemSteamworks::OnUserUploadedLeaderboardScore(LeaderboardScoreUploaded_t* CallbackData, bool bIOFailure)
{
	PendingLeaderboardWrites--;

	// If all pending write requests have returned, clear the callback list
	if (PendingLeaderboardWrites <= 0)
	{
		PendingLeaderboardWrites = 0;
		CallbackBridge->EmptyUserUploadedLeaderboardScore();
	}

	if (!bIOFailure && CallbackData->m_hSteamLeaderboard != NULL)
	{
		INT ListIndex = INDEX_NONE;

		// Find the leaderboard in 'LeaderboardList', based on the leaderboard handle
		for (INT i=0; i<LeaderboardList.Num(); i++)
		{
			if (LeaderboardList(i).LeaderboardRef == CallbackData->m_hSteamLeaderboard)
			{
				ListIndex = i;
				break;
			}
		}

		// Update the leaderboard entry count
		if (ListIndex != INDEX_NONE)
		{
			debugf(NAME_DevOnline, TEXT("Successfully updated score for leaderboard '%s'"), *LeaderboardList(ListIndex).LeaderboardName);

			LeaderboardList(ListIndex).LeaderboardSize = GSteamUserStats->GetLeaderboardEntryCount(CallbackData->m_hSteamLeaderboard);
		}
	}
	else
	{
		debugf(TEXT("IOFailure when writing leaderboard score"));
	}

	// NOTE: There is no script notification of failure to write leaderboard data
}


/**
 * Generates the field name used by Steam stats, based on a view id and a column id
 *
 * @param StatsRead holds the definitions of the tables to read the data from
 * @param ViewId the view to read from
 * @param ColumnId the column to read.  If 0, then this is just the view's column (flag indicating if the view has been written to)
 * @return the name of the field
 */
FString UOnlineSubsystemSteamworks::GetStatsFieldName(INT ViewId, INT ColumnId)
{
	return FString::Printf(TEXT("%d_%d"), ViewId, ColumnId);
}

void UOnlineSubsystemSteamworks::OnSpecificUserStatsReceived(UserStatsReceived_t *CallbackData, bool bIOFailure)
{
	if (CurrentStatsRead != NULL)
	{
		UBOOL bOk = TRUE;

		const CGameID GameID(GSteamAppID);
		check(GameID.ToUint64() == CallbackData->m_nGameID);  // This should never happen here!

		// !!! FIXME: this only provides valid information if the user has a relationship with the player
		//		(in buddy list, in same lobby, playing on same server, etc).
		const FString Persona(UTF8_TO_TCHAR(GSteamFriends->GetFriendPersonaName(CallbackData->m_steamIDUser)));
		const QWORD ProfileId = (QWORD) CallbackData->m_steamIDUser.ConvertToUint64();

		if (CallbackData->m_eResult == k_EResultFail)   // no stats for this user.
		{
			debugf(NAME_DevOnline, TEXT("No steam user stats for '%s'."), *RenderCSteamID(CallbackData->m_steamIDUser));

			// Add blank data
			const INT NewIndex = CurrentStatsRead->Rows.AddZeroed();
			FOnlineStatsRow& Row = CurrentStatsRead->Rows(NewIndex);

			Row.PlayerID.Uid = ProfileId;
			Row.Rank.SetData(TEXT("--"));

			// Fill the columns
			const INT NumColumns = CurrentStatsRead->ColumnIds.Num();

			Row.Columns.AddZeroed(NumColumns);

			// And copy the fields into the columns
			for (INT FieldIndex = 0; FieldIndex < NumColumns; FieldIndex++)
			{
				FOnlineStatsColumn& Col = Row.Columns(FieldIndex);

				Col.ColumnNo = CurrentStatsRead->ColumnIds(FieldIndex);
				Col.StatValue.SetData(TEXT("--"));
			}

			Row.NickName = Persona;


			bOk = FALSE;
		}

		else if ((bIOFailure) || CallbackData->m_eResult != k_EResultOK)   // generic failure.
		{
			debugf(NAME_DevOnline, TEXT("Failed to obtained specific steam user stats for '%s', error: %i."),
						*RenderCSteamID(CallbackData->m_steamIDUser), (int)CallbackData->m_eResult);

			CurrentStatsRead = NULL;
#if STEAM_CRASH_FIX
			StatsReadList.Empty();
#endif

			FAsyncTaskDelegateResults Results(E_FAIL);
			TriggerOnlineDelegates(this, ReadOnlineStatsCompleteDelegates, &Results);

			return;
		}

		else  // got stats!
		{
			debugf(NAME_DevOnline, TEXT("Obtained specific steam user stats for '%s' ('%s')."), *RenderCSteamID(CallbackData->m_steamIDUser), *Persona);


			// Add blank data
			INT NewIndex = CurrentStatsRead->Rows.AddZeroed();
			FOnlineStatsRow& Row = CurrentStatsRead->Rows(NewIndex);

			Row.PlayerID.Uid = ProfileId;


			// Search the leaderboard results for this players rank
			INT LeaderboardIndex = INDEX_NONE;

			for (INT i=0; i<ProcessedLeaderboardReads.Num(); i++)
			{
				if (ProcessedLeaderboardReads(i).PlayerUID == Row.PlayerID)
				{
					LeaderboardIndex = i;
					break;
				}
			}

			if (LeaderboardIndex != INDEX_NONE)
				Row.Rank.SetData((INT)ProcessedLeaderboardReads(LeaderboardIndex).Rank);
			else
				Row.Rank.SetData(TEXT("--"));


			// Fill the columns
			const INT NumColumns = CurrentStatsRead->ColumnIds.Num();

			Row.Columns.AddZeroed(NumColumns);

			// And copy the fields into the columns
			for (INT FieldIndex = 0; FieldIndex < NumColumns; FieldIndex++)
			{
				FOnlineStatsColumn& Col = Row.Columns(FieldIndex);
				Col.ColumnNo = CurrentStatsRead->ColumnIds(FieldIndex);

				const FString Key(GetStatsFieldName(CurrentStatsRead->ViewId, Col.ColumnNo));

				int32 IntData = 0;
				float FloatData = 0.0f;

				if (GSteamUserStats->GetUserStat(CallbackData->m_steamIDUser, TCHAR_TO_UTF8(*Key), &IntData))
				{
					Col.StatValue.SetData((INT)IntData);
				}
				else if (GSteamUserStats->GetUserStat(CallbackData->m_steamIDUser, TCHAR_TO_UTF8(*Key), &FloatData))
				{
					Col.StatValue.SetData((FLOAT)FloatData);
				}
				else
				{
					debugf(NAME_DevOnline, TEXT("Failed to get steam user stats value for key '%s'"), *Key);
					bOk = FALSE;
				}
			}

			Row.NickName = Persona;
		}

		// see if we're done waiting on requests...
		if (CurrentStatsRead->Rows.Num() == CurrentStatsRead->TotalRowsInView)
		{
			debugf(NAME_DevOnline, TEXT("Obtained all the steam user stats we wanted."));

			CurrentStatsRead = NULL;
#if STEAM_CRASH_FIX
			StatsReadList.Empty();
#endif

			FAsyncTaskDelegateResults Results((bOk ? S_OK : E_FAIL));
			TriggerOnlineDelegates(this,ReadOnlineStatsCompleteDelegates,&Results);
		}
		// @todo: Remove this code, when the Steam SDK crash caused by ReadOnlineStats has been fixed
#if STEAM_CRASH_FIX
		else if (StatsReadList.Num() > 0)
		{
			// Pass the next entry on to ReadOnlineStats
			UOnlineStatsRead* CurStatsRead = CurrentStatsRead;
			CurrentStatsRead = NULL;

			if (!ReadOnlineStats(StatsReadList, CurStatsRead))
			{
				CurrentStatsRead = NULL;
				StatsReadList.Empty();

				FAsyncTaskDelegateResults Results(E_FAIL);
				TriggerOnlineDelegates(this,ReadOnlineStatsCompleteDelegates,&Results);
			}
		}
#endif
	}
}

void UOnlineSubsystemSteamworks::OnSpecificGSStatsReceived(GSStatsReceived_t *CallbackData, bool bIOFailure)
{
	UserStatsReceived_t UserStatsReceived;
	UserStatsReceived.m_nGameID = CGameID(GSteamAppID).ToUint64();
	UserStatsReceived.m_eResult = CallbackData->m_eResult;
	UserStatsReceived.m_steamIDUser = CallbackData->m_steamIDUser;
	OnSpecificUserStatsReceived(&UserStatsReceived, bIOFailure);  // pass it through to the other version.
}

void UOnlineSubsystemSteamworks::OnSpecificGSStatsStored(GSStatsStored_t *CallbackData, bool bIOFailure)
{
	if ((bIOFailure) || CallbackData->m_eResult != k_EResultOK)   // generic failure.
	{
		debugf(NAME_DevOnline, TEXT("Failed to store specific steam user stats for '%s', error: %i."),
			*RenderCSteamID(CallbackData->m_steamIDUser), (int)CallbackData->m_eResult);
		bGSStatsStoresSuccess = FALSE;
	}
	else
	{
		debugf(NAME_DevOnline, TEXT("Stored specific steam user stats for '%s'."), *RenderCSteamID(CallbackData->m_steamIDUser));
	}
	TotalGSStatsStoresPending--;
}

void UOnlineSubsystemSteamworks::FinishDestroy()
{
	delete CallbackBridge;
	CallbackBridge = NULL;
	Super::FinishDestroy();
}

/**
 * Clears the various data that is associated with a player to prevent the
 * data being used across logins
 */
void UOnlineSubsystemSteamworks::ClearPlayerInfo()
{
	CachedProfile = NULL;
	LoggedInPlayerId = FUniqueNetId(0);
	LoggedInStatus = LS_NotLoggedIn;
	LoggedInPlayerName.Empty();
	CachedFriendMessages.Empty();
	CachedGameInt->SetInviteInfo(NULL);
//@todo joeg -- add more as they come up
}

/**
 * Finds the player's name based off of their net id
 *
 * @param NetId the net id we are finding the player name for
 *
 * @return the name of the player that matches the net id
 */
static FString GetPlayerName(FUniqueNetId NetId)
{
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	AGameReplicationInfo* GRI = WorldInfo->GRI;
	if (GRI != NULL)
	{
		for (INT Index = 0; Index < GRI->PRIArray.Num(); Index++)
		{
			APlayerReplicationInfo* PRI = GRI->PRIArray(Index);
			if ((QWORD&)PRI->UniqueId == (QWORD&)NetId)
			{
				return PRI->PlayerName;
			}
		}
		for (INT Index = 0; Index < GRI->InactivePRIArray.Num(); Index++)
		{
			APlayerReplicationInfo* PRI = GRI->InactivePRIArray(Index);
			if ((QWORD&)PRI->UniqueId == (QWORD&)NetId)
			{
				return PRI->PlayerName;
			}
		}
	}
	// We don't have a nick for this player
	return FString();
}

/**
 * Steamworks specific implementation. Sets the supported interface pointers and
 * initilizes the voice engine
 *
 * @return always returns TRUE
 */
UBOOL UOnlineSubsystemSteamworks::Init(void)
{
	Super::Init();

	// Set the initial state so we don't trigger an event unnecessarily
	bLastHasConnection = GSocketSubsystem->HasNetworkDevice();

	// Set to the localized default
	LoggedInPlayerName = LocalProfileName;
	LoggedInPlayerNum = -1;
	LoggedInStatus = LS_NotLoggedIn;
	LoggedInPlayerId = FUniqueNetId(0);

	// do the Steamworks initialization
	InitSteamworks();

	// Set the interface used for account creation
	eventSetAccountInterface(this);
	// Set the player interface to be the same as the object
	eventSetPlayerInterface(this);
	// Set the extended player interface to be the same as the object
	eventSetPlayerInterfaceEx(this);

	// Construct the object that handles the game interface for us
	CachedGameInt = ConstructObject<UOnlineGameInterfaceSteamworks>(UOnlineGameInterfaceSteamworks::StaticClass(),this);
	GameInterfaceImpl = CachedGameInt;
	if (GameInterfaceImpl)
	{
		GameInterfaceImpl->OwningSubsystem = this;
		// Set the game interface to be the same as the object
		eventSetGameInterface(GameInterfaceImpl);
	}
	// Set the stats reading/writing interface
	eventSetStatsInterface(this);
	eventSetSystemInterface(this);
	// Create the voice engine and if successful register the interface
	VoiceEngine = appCreateVoiceInterface(MaxLocalTalkers,MaxRemoteTalkers,	bIsUsingSpeechRecognition);
	if (VoiceEngine != NULL)
	{
		// Set the voice interface to this object
		eventSetVoiceInterface(this);
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Failed to create the voice interface"));
	}
	// Use MCP services for news or not
	if (bShouldUseMcp)
	{
		UOnlineNewsInterfaceMcp* NewsObject = ConstructObject<UOnlineNewsInterfaceMcp>(UOnlineNewsInterfaceMcp::StaticClass(),this);
		eventSetNewsInterface(NewsObject);
	}
	// Handle a missing profile data directory specification
	if (ProfileDataDirectory.Len() == 0)
	{
		ProfileDataDirectory = TEXT(".\\");
	}

	// Set the default toast location
	SetNetworkNotificationPosition(CurrentNotificationPosition);

	// Sign in locally to the default account if necessary.
	SignInLocally();

	return GameInterfaceImpl != NULL;
}

extern "C" { static void __cdecl SteamworksWarningMessageHook(int Severity, const char *Message); }
extern "C" { static void __cdecl SteamworksWarningMessageHookNoOp(int Severity, const char *Message); }

static void __cdecl SteamworksWarningMessageHook(int Severity, const char *Message)
{
	const TCHAR *MessageType;
	switch (Severity)
	{
		case 0: MessageType = TEXT("message"); break;
		case 1: MessageType = TEXT("warning"); break;
		default: MessageType = TEXT("notification"); break;  // Unknown severity; new SDK?
	}
	debugf(NAME_DevOnline,TEXT("Steamworks SDK %s: %s"),MessageType,UTF8_TO_TCHAR(Message));
}

static void __cdecl SteamworksWarningMessageHookNoOp(int Severity, const char *Message)
{
	// no-op.
}

static inline const TCHAR *GetSteamUniverseName(const EUniverse SteamUniverse)
{
	switch (SteamUniverse)
	{
		case k_EUniverseInvalid: return TEXT("INVALID");
		case k_EUniversePublic: return TEXT("PUBLIC");
		case k_EUniverseBeta: return TEXT("BETA");
		case k_EUniverseInternal: return TEXT("INTERNAL");
		case k_EUniverseDev: return TEXT("DEV");
		case k_EUniverseRC: return TEXT("RC");
	}
	return TEXT("???");
}

/** Initializes Steamworks */
UBOOL UOnlineSubsystemSteamworks::InitSteamworks()
{
	// We had to do the initial Steamworks init (SteamAPI_Init()) in UnMisc.cpp, near the start of the process. Check if it worked out here.

	if ( GSteamworksInitialized )
	{
		debugf(TEXT("Initializing Steamworks"));

		#define GET_STEAMWORKS_INTERFACE(Interface) \
			if ((G##Interface = Interface()) == NULL) \
			{ \
				debugf(NAME_DevNet, TEXT("Steamworks: %s() failed!"), TEXT(#Interface)); \
				GSteamworksInitialized = FALSE; \
			} \

		GET_STEAMWORKS_INTERFACE(SteamUtils);
		GET_STEAMWORKS_INTERFACE(SteamUser);
		GET_STEAMWORKS_INTERFACE(SteamFriends);
		GET_STEAMWORKS_INTERFACE(SteamRemoteStorage);
		GET_STEAMWORKS_INTERFACE(SteamUserStats);
		GET_STEAMWORKS_INTERFACE(SteamMatchmakingServers);
		GET_STEAMWORKS_INTERFACE(SteamApps);

		#undef GET_STEAMWORKS_INTERFACE
	}

	if (!GSteamworksInitialized)
	{
		debugf(TEXT("Steamworks is unavailable"));
		debugf(NAME_DevNet, TEXT("This usually means you're not running the Steam client, your Steam client is incompatible, or you don't have a proper steam_appid.txt"));
		debugf(NAME_DevNet, TEXT("We will continue without OnlineSubsystem support."));
		OnlineSubsystemSteamworks_eventOnConnectionStatusChange_Parms ConnectionParms(EC_EventParm);
		ConnectionParms.ConnectionStatus = OSCS_ServiceUnavailable;
		TriggerOnlineDelegates(this,ConnectionStatusChangeDelegates,&ConnectionParms);
		return TRUE;
	}

	// Please note that you don't get GSteamMasterServerUpdater, GSteamGameServer, and GSteamGameServerStats here, as they aren't available unless we
	//  call SteamGameServer_Init(), which we won't do until we're actually starting a game. So if you aren't sure you're called from an initialized
	//  game server, don't use these interfaces without checking if they are NULL!

	GSteamAppID = GSteamUtils->GetAppID();
	GSteamUtils->SetWarningMessageHook(SteamworksWarningMessageHook);
	UserStatsReceivedState = OERS_NotStarted;
	CallbackBridge = new SteamCallbackBridge(this);

	int32 SteamCloudTotal = 0;
	int32 SteamCloudAvailable = 0;
	if (!GSteamRemoteStorage->GetQuota(&SteamCloudTotal, &SteamCloudAvailable))
	{
		SteamCloudTotal = SteamCloudAvailable = -1;
	}

#if !FORCELOWGORE
	GForceLowGore = GSteamApps->BIsLowViolence();
#endif

	debugf(NAME_DevOnline, TEXT("Steam universe: %s"), GetSteamUniverseName(GSteamUtils->GetConnectedUniverse()));
	debugf(NAME_DevOnline, TEXT("Steam appid: %u"), GSteamAppID);
	debugf(NAME_DevOnline, TEXT("Steam IsSubscribed: %s"), BoolToString(GSteamApps->BIsSubscribed()));
	debugf(NAME_DevOnline, TEXT("Steam IsLowViolence: %s"), BoolToString(GSteamApps->BIsLowViolence()));
	debugf(NAME_DevOnline, TEXT("Steam IsCybercafe: %s"), BoolToString(GSteamApps->BIsCybercafe()));
	debugf(NAME_DevOnline, TEXT("Steam IsVACBanned: %s"), BoolToString(GSteamApps->BIsVACBanned()));
	debugf(NAME_DevOnline, TEXT("Steam IP country: %s"), ANSI_TO_TCHAR(GSteamUtils->GetIPCountry()));
	debugf(NAME_DevOnline, TEXT("Steam official server time: %u"), (DWORD) GSteamUtils->GetServerRealTime());
	debugf(NAME_DevOnline, TEXT("Steam Cloud quota: %d / %d"), (INT) SteamCloudAvailable, (INT) SteamCloudTotal);

	if (GSteamUser->BLoggedOn())
	{
		const char *PersonaName = GSteamFriends->GetPersonaName();
		LoggedInPlayerName = FString(UTF8_TO_TCHAR(PersonaName));
		LoggedInPlayerNum = 0;
		LoggedInPlayerId.Uid = (QWORD) GSteamUser->GetSteamID().ConvertToUint64();
		LoggedInStatus = LS_LoggedIn;
		debugf(TEXT("Logged in as '%s'"), *LoggedInPlayerName);

#if !FORBID_STEAM_RESET_STATS
		INT ResetStats = 0;
		GConfig->GetInt(TEXT("OnlineSubsystemSteamworks.OnlineSubsystemSteamworks"), TEXT("ResetStats"), ResetStats, GEngineIni);

		// we treat 1 as reset stats, 2 as reset stats AND achievements.
		if ((ResetStats == 1) || (ResetStats == 2))
		{
			const UBOOL bAchievementsToo = (ResetStats == 2);
			debugf(TEXT("RESETTING YOUR STEAM STATS!%s"), bAchievementsToo ? TEXT(" (and achievements, too!)") : TEXT(""));
			GSteamUserStats->ResetAllStats(bAchievementsToo == TRUE);  // it's too late now.
			GSteamUserStats->StoreStats();
		}
#endif

		OnlineSubsystemSteamworks_eventOnConnectionStatusChange_Parms ConnectionParms(EC_EventParm);
		ConnectionParms.ConnectionStatus = OSCS_Connected;
		TriggerOnlineDelegates(this,ConnectionStatusChangeDelegates,&ConnectionParms);

		// We don't have a login screen, so just tell the game's delegates we logged in here.
		OnlineSubsystemSteamworks_eventOnLoginChange_Parms LoginParms(EC_EventParm);
		LoginParms.LocalUserNum = LoggedInPlayerNum;
		TriggerOnlineDelegates(this,LoginChangeDelegates,&LoginParms);
	}
	return TRUE;
}


/**
 * Handles updating of any async tasks that need to be performed
 *
 * @param DeltaTime the amount of time that has passed since the last tick
 */
void UOnlineSubsystemSteamworks::TickVoice(FLOAT DeltaTime)
{
	if (VoiceEngine)
	{
		// Process VoIP data
		ProcessLocalVoicePackets();
		ProcessRemoteVoicePackets();
		// Let it do any async processing
		VoiceEngine->Tick(DeltaTime);
		// Fire off the events that script cares about
		ProcessTalkingDelegates();
		ProcessSpeechRecognitionDelegates();
	}
}


/**
 * Handles updating of any async tasks that need to be performed
 *
 * @param DeltaTime the amount of time that has passed since the last tick
 */
void UOnlineSubsystemSteamworks::Tick(FLOAT DeltaTime)
{
	// Check for there not being a logged in user, since we require at least a default user
	if (LoggedInStatus == LS_NotLoggedIn)
	{
		SignInLocally();
	}

	// Tick any tasks needed for LAN/networked game support
	TickGameInterfaceTasks(DeltaTime);
	// Let voice do any processing
	TickVoice(DeltaTime);
	TickConnectionStatusChange(DeltaTime);
	// Tick the Steamworks Core (may run callbacks!)
	TickSteamworksTasks(DeltaTime);
}


/**
 * Allows the Steamworks code to perform their Think operations
 */
void UOnlineSubsystemSteamworks::TickSteamworksTasks(FLOAT DeltaTime)
{
	if (!GSteamworksInitialized)
	{
		return;
	}

	SteamAPI_RunCallbacks();

	if (GSteamGameServer != NULL)
	{
		SteamGameServer_RunCallbacks();

		static FLOAT RefreshPublishedGameSettingsTime = 0.0f;
		RefreshPublishedGameSettingsTime += DeltaTime;
		if (RefreshPublishedGameSettingsTime >= 60.0f)   // make sure we push this every minute, just in case.
		{
			CachedGameInt->RefreshPublishedGameSettings();
			RefreshPublishedGameSettingsTime = 0.0f;
		}
	}

#if HAVE_STEAM_GAMESERVER_STATS
	if ((TotalGSStatsStoresPending == 0) && (CallbackBridge->GetGSStatsStoreCount() > 0))
	{
		CallbackBridge->EmptySpecificGSStatsStore();
		FAsyncTaskDelegateResultsNamedSession Params(FName(TEXT("Game")), bGSStatsStoresSuccess ? S_OK : E_FAIL);
		TriggerOnlineDelegates(this,FlushOnlineStatsDelegates,&Params);
	}
#endif

	// See if there are any avatar request retries pending.
	for (INT Index = 0; Index < QueuedAvatarRequests.Num(); Index++)
	{
		FQueuedAvatarRequest &Request = QueuedAvatarRequests(Index);
		Request.CheckTime += (FLOAT) DeltaTime;
		if (Request.CheckTime >= 1.0f)  // make sure Steam had time to recognize this guy.
		{
			Request.CheckTime = 0.0f;  // reset timer for next try.
			Request.NumberOfAttempts++;
			const UBOOL bLastTry = (Request.NumberOfAttempts >= 60);  // 60 seconds is probably more than enough.
			const UBOOL bGotIt = GetOnlineAvatar(Request.PlayerNetId, Request.ReadOnlineAvatarCompleteDelegate, bLastTry);
			if ((bGotIt) || (bLastTry))
			{
				QueuedAvatarRequests.Remove(Index);  // done either way.
				Index--;  // so we don't skip items...
			}
		}
	}


	// If this is a listen server, and the host is pending authentication, do reauthentication if necessary
	AWorldInfo* WI = (GWorld != NULL ? GWorld->GetWorldInfo() : NULL);

	if (bListenHostPendingAuth && !bListenHostAuthenticated && WI != NULL && WI->NetMode == NM_ListenServer)
	{
		// Limit checks to once every 10 seconds, so we're not adding load to each tick (do the first check immediately though)
		if (((WI->RealTimeSeconds - ListenAuthTimestamp) >= 10.0 || ListenAuthTimestamp > WI->RealTimeSeconds) ||
			ListenAuthRetryCount == 0)
		{
			FString UIDText = UOnlineSubsystem::UniqueNetIdToString(LoggedInPlayerId);

			UBOOL bAuthFailure = FALSE;
			UBOOL bAuthTimedOut = FALSE;
			TCHAR* FailReason = TEXT("");
			TCHAR* FailExtra = TEXT("");

			// Limit to 10 retries
			if (ListenAuthRetryCount < 10)
			{
				ListenAuthTimestamp = WI->RealTimeSeconds;
				ListenAuthRetryCount++;

				// Kick off a new auth request
				if (GSteamGameServer != NULL && GameInterfaceImpl != NULL && GameInterfaceImpl->SessionInfo != NULL)
				{
					debugf(NAME_DevOnline, TEXT("Attempting reauthentication of listen host with Steam"));

					// If this is a listen host, you need to re-initiate the game connection before attempting auth
					UBOOL bProtected = (GameInterfaceImpl != NULL ?
								GameInterfaceImpl->GameSettings->bAntiCheatProtected : FALSE);

					BYTE AuthBlob[2048];
					CSteamID AuthID((uint64)LoggedInPlayerId.Uid);
					const uint16 AuthPort = (uint16)GameInterfaceImpl->SessionInfo->HostAddr.GetPort();
					DWORD AuthIP = 0;

					GameInterfaceImpl->SessionInfo->HostAddr.GetIp(AuthIP);


					// First disconnect the user, if connected at all
					GSteamGameServer->SendUserDisconnect(AuthID);

					// Now setup the new connection (NOTE: AuthExtra for listen servers, is the listen port)
					int BlobLen = GSteamUser->InitiateGameConnection(AuthBlob, sizeof(AuthBlob), AuthID,
							AuthIP, AuthPort, bProtected != 0);


					// Now that the game connection has been reinitiated, attempt auth
					if (!GSteamGameServer->SendUserConnectAndAuthenticate(AuthIP, AuthBlob, BlobLen, &AuthID))
					{
						bAuthFailure = TRUE;
						FailReason = TEXT("API Failure");
						FailExtra = (BlobLen == 0 ? TEXT("InitiateGameConnection") :
								TEXT("SendUserConnectAndAuthenticate"));
					}
				}
				else
				{
					bAuthFailure = TRUE;
					FailReason = TEXT("Internal Failure");

					if (GSteamGameServer == NULL)
						FailExtra = TEXT("GSteamGameServer == NULL");
					else if (GameInterfaceImpl == NULL)
						FailExtra = TEXT("GameInterfaceImpl == NULL");
					else if (GameInterfaceImpl->SessionInfo == NULL)
						FailExtra = TEXT("GameInterfaceImpl->SessionInfo == NULL");
				}
			}
			else
			{
				bAuthFailure = TRUE;
				bAuthTimedOut = TRUE;
				FailReason = TEXT("Timed out after 10 tries");
			}

			// If the authentication fails, trigger a log entry
			if (bAuthFailure)
			{
				debugf(TEXT("Failed Steam auth retry for listen host (%s : %s), stats updates may fail for this player"),
						FailReason, FailExtra);
			}
		}
	}


	static FLOAT CheckIPCCallsTime = 0.0f;
	CheckIPCCallsTime += DeltaTime;
	if (CheckIPCCallsTime >= 60.0f)
	{
		debugf(NAME_DevOnline, TEXT("Steam IPC calls in the last minute: %u"), (DWORD) GSteamUtils->GetIPCCallCount());
		CheckIPCCallsTime = 0.0f;
	}
}

/**
 * Ticks the connection checking code
 *
 * @param DeltaTime the amount of time that has elapsed since the list check
 */
void UOnlineSubsystemSteamworks::TickConnectionStatusChange(FLOAT DeltaTime)
{
	ConnectionPresenceElapsedTime += DeltaTime;
	// See if the connection needs to be checked
	if (ConnectionPresenceElapsedTime > ConnectionPresenceTimeInterval)
	{
		// Compare the connection to the last check
		UBOOL bHasConnection = GSocketSubsystem->HasNetworkDevice();
		if (bHasConnection != bLastHasConnection)
		{
			// They differ so notify the game code
			OnlineSubsystemSteamworks_eventOnLinkStatusChange_Parms Parms(EC_EventParm);
			Parms.bIsConnected = bHasConnection ? FIRST_BITFIELD : 0;
			TriggerOnlineDelegates(this,LinkStatusDelegates,&Parms);
		}
		bLastHasConnection = bHasConnection;
		ConnectionPresenceElapsedTime = 0.f;
	}
}

/** @return Builds the Steamworks location string from the game the player is connected to */
FString UOnlineSubsystemSteamworks::GetServerLocation(void) const
{
	if (CachedGameInt->GameSettings &&
		CachedGameInt->SessionInfo)
	{
		FInternetIpAddr Addr(CachedGameInt->SessionInfo->HostAddr);
		DWORD OutAddr = 0;
		// This will be zero if we are the server
		Addr.GetIp(OutAddr);
		if (OutAddr == 0)
		{
			// Get the real IP address of the machine
			GSocketSubsystem->GetLocalHostAddr(*GLog,Addr);
		}
		// Format the string using the address and our connection count
		FString Location = FString::Printf(TEXT("%s://%s/?MaxPub=%d?MaxPri=%d"),
			*LocationUrl,
			*Addr.ToString(TRUE),
			CachedGameInt->GameSettings->NumPublicConnections,
			CachedGameInt->GameSettings->NumPrivateConnections);
		// Lan matches aren't joinable, so flag them as such
		if (CachedGameInt->GameSettings->bIsLanMatch)
		{
			Location += TEXT("?bIsLanMatch");
		}
		INT IsLocked = 0;
		//hack: add passworded server info
		if (CachedGameInt->GameSettings->GetStringSettingValue(7,IsLocked))
		{
			if (IsLocked)
			{
				Location += TEXT("?bRequiresPassword");
			}
		}
		return Location;
	}
	return FString::Printf(TEXT("%s:///Standalone"),*LocationUrl);
}

/**
 * Logs the player into the online service. If this fails, it generates a
 * OnLoginFailed notification
 *
 * @param LocalUserNum the controller number of the associated user
 * @param LoginName the unique identifier for the player
 * @param Password the password for this account
 * @param bWantsLocalOnly whether the player wants to sign in locally only or not
 *
 * @return true if the async call started ok, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::Login(BYTE LocalUserNum,const FString& LoginName,const FString& Password,UBOOL bWantsLocalOnly)
{
	UBOOL bSignedInOk = FALSE;

	if (!GSteamworksInitialized)
	{
		bWantsLocalOnly = TRUE;
	}

	if (bWantsLocalOnly == FALSE)
	{
		if (GSteamUser->BLoggedOn())
		{
			const char *PersonaName = GSteamFriends->GetPersonaName();
			LoggedInPlayerName = FString(UTF8_TO_TCHAR(PersonaName));
			LoggedInPlayerNum = 0;
			LoggedInPlayerId.Uid = (QWORD) GSteamUser->GetSteamID().ConvertToUint64();
			LoggedInStatus = LS_LoggedIn;
			debugf(NAME_DevOnline, TEXT("Logged in as '%s'"), *LoggedInPlayerName);

			// Stash which player is the active one
			LoggedInPlayerNum = LocalUserNum;
			bSignedInOk = TRUE;

			// Trigger the delegates so the UI can process
			OnlineSubsystemSteamworks_eventOnLoginChange_Parms Parms(EC_EventParm);
			Parms.LocalUserNum = 0;
			TriggerOnlineDelegates(this,LoginChangeDelegates,&Parms);
		}
		else
		{
			ClearPlayerInfo();
			FLoginFailedParms Params(LocalUserNum);
			TriggerOnlineDelegates(this,LoginFailedDelegates,&Params);
		}
	}
	else
	{
		FString BackupLogin = LoggedInPlayerName;
		// Temporarily swap the names to see if the profile exists
		LoggedInPlayerName = LoginName;
		if (DoesProfileExist())
		{
			ClearPlayerInfo();
			// Yay. Login worked
			LoggedInPlayerNum = LocalUserNum;
			LoggedInStatus = LS_UsingLocalProfile;
				LoggedInPlayerName = LoginName;
			bSignedInOk = TRUE;
			debugf(NAME_DevOnline,TEXT("Signing into profile %s locally"),*LoggedInPlayerName);
			// Trigger the delegates so the UI can process
			OnlineSubsystemSteamworks_eventOnLoginChange_Parms Parms2(EC_EventParm);
			Parms2.LocalUserNum = 0;
			TriggerOnlineDelegates(this,LoginChangeDelegates,&Parms2);
		}
		else
		{
			// Restore the previous log in name
			LoggedInPlayerName = BackupLogin;
			FLoginFailedParms Params(LocalUserNum);
			TriggerOnlineDelegates(this,LoginFailedDelegates,&Params);
		}
	}
	return bSignedInOk;
}

/**
 * Logs the player into the online service using parameters passed on the
 * command line. Expects -Login=<UserName> -Password=<password>. If either
 * are missing, the function returns false and doesn't start the login
 * process
 *
 * @return true if the async call started ok, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::AutoLogin(void)
{
	return FALSE;
}

/**
 * Logs the player into the default account
 */
void UOnlineSubsystemSteamworks::SignInLocally()
{
	if ((!GSteamworksInitialized) || (GSteamUser->BLoggedOn() == FALSE))
	{
		LoggedInPlayerName = GetClass()->GetDefaultObject<UOnlineSubsystemSteamworks>()->LocalProfileName;
		debugf(NAME_DevOnline,TEXT("Signing into the local profile %s"),*LoggedInPlayerName);
		LoggedInPlayerNum = 0;
		LoggedInStatus = LS_UsingLocalProfile;
		// Trigger the delegates so the UI can process
		OnlineSubsystemSteamworks_eventOnLoginChange_Parms Parms2(EC_EventParm);
		Parms2.LocalUserNum = 0;
		TriggerOnlineDelegates(this,LoginChangeDelegates,&Parms2);
	}
}

/**
 * Signs the player out of the online service
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::Logout(BYTE LocalUserNum)
{
	return FALSE;   // no log out on Steam.
}

/**
 * Fetches the login status for a given player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the enum value of their status
 */
BYTE UOnlineSubsystemSteamworks::GetLoginStatus(BYTE LocalUserNum)
{
	if ((!GSteamworksInitialized) || (LocalUserNum != LoggedInPlayerNum))
	{
		return LS_NotLoggedIn;
	}
	return ((GSteamUser->BLoggedOn()) ? LS_LoggedIn : LS_NotLoggedIn);
}

/**
 * Checks that a unique player id is part of the specified user's friends list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player being checked
 *
 * @return TRUE if a member of their friends list, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsFriend(BYTE LocalUserNum,FUniqueNetId PlayerID)
{
	if (GSteamworksInitialized && LocalUserNum == LoggedInPlayerNum && GetLoginStatus(LoggedInPlayerNum) > LS_NotLoggedIn)
	{
		// Ask Steam if they are on the buddy list
		const CSteamID SteamPlayerID((uint64) PlayerID.Uid);
		return (GSteamFriends->GetFriendRelationship(SteamPlayerID) == k_EFriendRelationshipFriend);
	}
	return FALSE;
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

UBOOL UOnlineSubsystemSteamworks::AreAnyFriends(BYTE LocalUserNum,TArray<FFriendsQuery>& Query)
{
	UBOOL bReturn = FALSE;
	// Steamworks doesn't have a bulk check so check one at a time
	for (INT Index = 0; Index < Query.Num(); Index++)
	{
		FFriendsQuery& FriendQuery = Query(Index);
		if (IsFriend(LocalUserNum,FriendQuery.UniqueId))
		{
			FriendQuery.bIsFriend = TRUE;
			bReturn = TRUE;
		}
	}
	return bReturn;
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
UBOOL UOnlineSubsystemSteamworks::ReadOnlineStats(const TArray<FUniqueNetId>& Players,
	UOnlineStatsRead* StatsRead)
{
	UBOOL Result = FALSE;
	UBOOL bSkipCallback = FALSE;

	if (CurrentStatsRead == NULL)
	{
		// Validate that players were specified
		if (GSteamworksInitialized && (Players.Num() > 0))
		{
			if (StatsRead != NULL)
			{
				CurrentStatsRead = StatsRead;

				// @todo: Revert this code once the Steamworks SDK crash (caused by too many stats requests) has been fixed
#if STEAM_CRASH_FIX
				if (StatsReadList.Num() == 0)
				{
					StatsReadList = Players;

					// Clear previous results
					CurrentStatsRead->Rows.Empty();
					CurrentStatsRead->TotalRowsInView = Players.Num();
				}

				const CSteamID SteamPlayerId((uint64) StatsReadList(0).Uid);

				StatsReadList.Remove(0, 1);

#if HAVE_STEAM_GAMESERVER_STATS
				if (GSteamGameServerStats != NULL)  // we're a server, use that interface...
				{
					CallbackBridge->AddSpecificGSStatsRequest(GSteamGameServerStats->RequestUserStats(SteamPlayerId));
				}
				else
#endif
				{
					CallbackBridge->AddSpecificUserStatsRequest(GSteamUserStats->RequestUserStats(SteamPlayerId));
				}
#else
				// Clear previous results
				CurrentStatsRead->Rows.Empty();

				// Setup the request
				for (INT i = 0; i < Players.Num(); i++)
				{
					const CSteamID SteamPlayerId((uint64) Players(i).Uid);

#if HAVE_STEAM_GAMESERVER_STATS
					if (GSteamGameServerStats != NULL)  // we're a server, use that interface...
					{
						CallbackBridge->AddSpecificGSStatsRequest(GSteamGameServerStats->RequestUserStats(SteamPlayerId));
					}
					else
#endif
					{
						CallbackBridge->AddSpecificUserStatsRequest(GSteamUserStats->RequestUserStats(SteamPlayerId));
					}
				}


				CurrentStatsRead->TotalRowsInView = Players.Num();
#endif

				Result = TRUE;
			}
			else
			{
				debugf(TEXT("ReadOnlineStats: Input StatsRead is NULL"));
			}
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't perform a stats read when one is in progress"));

		// Don't kill the current stats read by issuing callbacks for this
		bSkipCallback = TRUE;
	}

	if (Result != TRUE && !bSkipCallback)
	{
		debugf(NAME_DevOnline,TEXT("ReadOnlineStats() failed"));

		CurrentStatsRead = NULL;
#if STEAM_CRASH_FIX
		StatsReadList.Empty();
#endif

		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Param((Result ? S_OK : E_FAIL));
		TriggerOnlineDelegates(this, ReadOnlineStatsCompleteDelegates, &Param);
	}

	return Result;
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
UBOOL UOnlineSubsystemSteamworks::ReadOnlineStatsForFriends(BYTE LocalUserNum,
	UOnlineStatsRead* StatsRead)
{
	if (GSteamworksInitialized && (CurrentStatsRead == NULL))
	{
		const INT NumBuddies = GSteamFriends->GetFriendCount(k_EFriendFlagImmediate);
		TArray<FUniqueNetId> Players;
		Players.AddItem(LoggedInPlayerId);
		for (INT Index = 0; Index < NumBuddies; Index++)
		{
			const CSteamID SteamPlayerId(GSteamFriends->GetFriendByIndex(Index, k_EFriendFlagImmediate));
			FUniqueNetId Player;
			Player.Uid = (QWORD) SteamPlayerId.ConvertToUint64();
			if (Player.Uid != LoggedInPlayerId.Uid)
			{
				Players.AddItem(Player);
			}
		}

		// Now use the common method to read the stats
		return ReadOnlineStats(Players,StatsRead);
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
UBOOL UOnlineSubsystemSteamworks::ReadOnlineStatsByRank(UOnlineStatsRead* StatsRead,
	INT StartIndex,INT NumToRead)
{
	DWORD Result = E_FAIL;
	UBOOL bSkipCallback = FALSE;

	if (CurrentStatsRead == NULL)
	{
		if (StatsRead != NULL)
		{
			// Kickoff a leaderboard read request for this stats entry, and if there is no designated leaderboard, failure
			FString LeaderboardName = LeaderboardNameLookup(StatsRead->ViewId);

			if (!LeaderboardName.IsEmpty())
			{
				CurrentStatsRead = StatsRead;

				// Kickoff the read; ReadOnlineStats will be called with a list of GUIDs from OnUserDownloadedLeaderboardEntries
				if (ReadLeaderboardEntries(LeaderboardName, LRT_Global, StartIndex, StartIndex + Max(NumToRead, 0) - 1))
					Result = ERROR_IO_PENDING;
				else
					debugf(TEXT("ReadOnlineStatsByRank: Call to ReadLeaderboardEntries has failed"));
			}
			else
			{
				debugf(TEXT("ReadOnlineStatsByRank: StatsRead->ViewId does not have a LeaderboardNameMappings entry; failure"));
			}
		}
		else
		{
			debugf(TEXT("ReadOnlineStatsByRank: Input StatsRead is NULL"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Can't perform a stats read while one is in progress"));

		// Don't kill the current stats read by issuing callbacks for this
		bSkipCallback = TRUE;
	}

	if (Result != ERROR_IO_PENDING && !bSkipCallback)
	{
		debugf(NAME_Error,TEXT("ReadOnlineStatsByRank() failed"));

		CurrentStatsRead = NULL;

		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Param(Result);
		TriggerOnlineDelegates(this, ReadOnlineStatsCompleteDelegates, &Param);
	}

	return Result == ERROR_IO_PENDING;
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
UBOOL UOnlineSubsystemSteamworks::ReadOnlineStatsByRankAroundPlayer(BYTE LocalUserNum,
	UOnlineStatsRead* StatsRead,INT NumRows)
{
	DWORD Result = E_FAIL;
	UBOOL bSkipCallback = FALSE;

	if (CurrentStatsRead == NULL)
	{
		if (StatsRead != NULL)
		{
			// Kickoff a leaderboard read request for this stats entry, and if there is no designated leaderboard, failure
			FString LeaderboardName = LeaderboardNameLookup(StatsRead->ViewId);

			if (!LeaderboardName.IsEmpty())
			{
				CurrentStatsRead = StatsRead;

				// Kickoff the read; ReadOnlineStats will be called with a list of GUIDs from OnUserDownloadedLeaderboardEntries
				if (ReadLeaderboardEntries(LeaderboardName, LRT_Player, -Abs(NumRows), Abs(NumRows)))
					Result = ERROR_IO_PENDING;
				else
					debugf(TEXT("ReadOnlineStatsByRank: Call to ReadLeaderboardEntries has failed"));
			}
			else
			{
				debugf(TEXT("ReadOnlineStatsByRank: StatsRead->ViewId does not have a LeaderboardNameMappings entry; failure"));
			}
		}
		else
		{
			debugf(TEXT("ReadOnlineStatsByRankAroundPlayer: Input StatsRead is NULL"));
		}
	}
	else
	{
		debugf(NAME_Error, TEXT("Can't perform a stats read while one is in progress"));

		// Don't kill the current stats read by issuing callbacks for this
		bSkipCallback = TRUE;
	}

	if (Result != ERROR_IO_PENDING && !bSkipCallback)
	{
		debugf(NAME_Error, TEXT("ReadOnlineStatsByRankAroundPlayer() failed"));

		CurrentStatsRead = NULL;

		// Just trigger the delegate as having failed
		FAsyncTaskDelegateResults Param(Result);
		TriggerOnlineDelegates(this, ReadOnlineStatsCompleteDelegates, &Param);
	}

	return Result == ERROR_IO_PENDING;
}

/**
 * Cleans up any platform specific allocated data contained in the stats data
 *
 * @param StatsRead the object to handle per platform clean up on
 */
void UOnlineSubsystemSteamworks::FreeStats(UOnlineStatsRead* StatsRead)
{
	CallbackBridge->EmptySpecificUserStatsRequests();
#if HAVE_STEAM_GAMESERVER_STATS
	CallbackBridge->EmptySpecificGSStatsRequests();
#endif
}

/**
 * Writes out the stats contained within the stats write object to the online
 * subsystem's cache of stats data. Note the new data replaces the old. It does
 * not write the data to the permanent storage until a FlushOnlineStats() call
 * or a session ends. Stats cannot be written without a session or the write
 * request is ignored. No more than 5 stats views can be written to at a time
 * or the write request is ignored.
 *
 * @param SessionName the name of the session stats are being written for
 * @param Player the player to write stats for
 * @param StatsWrite the object containing the information to write
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::WriteOnlineStats(FName SessionName,FUniqueNetId Player,
	UOnlineStatsWrite* StatsWrite)
{
	// Skip processing if the server isn't logged in or if the game type is wrong
	if (SessionHasStats())
	{
		if (!IsServer() && Player.Uid != LoggedInPlayerId.Uid)
		{
			debugf(NAME_Error, TEXT("Attempted to write stats for UID which is not the clients"));
			return FALSE;
		}

		// Ignore unknown players
		if (Player.Uid != 0)
		{
			AWorldInfo* WI = GWorld->GetWorldInfo();
			UBOOL bHasClientStats = WI->NetMode != NM_DedicatedServer;

			FPendingPlayerStats& PendingPlayerStats = FindOrAddPendingPlayerStats(Player);

			// Get a ref to the view ids
			const TArrayNoInit<INT>& ViewIds = StatsWrite->ViewIds;
			INT NumIndexes = ViewIds.Num();
			INT NumProperties = StatsWrite->Properties.Num();
			TArray<INT> MonitoredAchMappings;
			TArray<INT> AchViewIds;

			// Check if achievement progress needs monitoring (recalculated each time WriteOnlineStats is called, in case list changes)
			if (bHasClientStats && NumIndexes > 0)
			{
				INT LastViewId = INDEX_NONE;

				for (INT i=0; i<AchievementMappings.Num(); i++)
				{
					const FAchievementMappingInfo& Ach = AchievementMappings(i);

					if (((Ach.bAutoUnlock || Ach.ProgressCount > 0) && Ach.MaxProgress > 0) && ViewIds.ContainsItem(Ach.ViewId))
					{
						if (Ach.ViewId != LastViewId)
						{
							LastViewId = Ach.ViewId;
							AchViewIds.AddUniqueItem(LastViewId);
						}

						MonitoredAchMappings.AddItem(i);
					}
				}
			}


			for (INT ViewIndex = 0; ViewIndex < NumIndexes; ViewIndex++)
			{
				INT ViewId = ViewIds(ViewIndex);
				UBOOL bMonitorAchievements = bHasClientStats && AchViewIds.ContainsItem(ViewId);

				for (INT PropertyIndex = 0; PropertyIndex < NumProperties; PropertyIndex++)
				{
					const FSettingsProperty& Property = StatsWrite->Properties(PropertyIndex);

					AddOrUpdatePlayerStat(PendingPlayerStats.Stats, ViewId, Property.PropertyId, Property.Data);

					if (bHasClientStats)
					{
						// Achievement updating
						if (bMonitorAchievements)
						{
							for (INT i=0; i<MonitoredAchMappings.Num(); i++)
							{
								const FAchievementMappingInfo& Ach = AchievementMappings(MonitoredAchMappings(i));

								if (Ach.ViewId == ViewId && Ach.AchievementId == Property.PropertyId)
								{
									AddOrUpdateAchievementStat(Ach, Property.Data);

									MonitoredAchMappings.Remove(i, 1);
									break;
								}
							}
						}

						// Leaderboard updating
						if (Property.PropertyId == StatsWrite->RatingId)
							AddOrUpdateLeaderboardStat(ViewId, Property.Data);
					}
				}

				// Removed code for setting the first stat column to 1, as it was clobbering data
			}
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("WriteOnlineStats: Ignoring unknown player"));
		}
	}
	return TRUE;
}

/**
 * Writes the specified set of scores to the skill tables
 *
 * @param SessionName the name of the session stats are being written for
 * @param LeaderboardId the leaderboard to write the score information to
 * @param PlayerScores the list of scores to write out
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::WriteOnlinePlayerScores(FName SessionName,INT LeaderboardId,const TArray<FOnlinePlayerScore>& PlayerScores)
{
	// Skip processing if the server isn't logged in
	if (SessionHasStats())
	{
		INT NumScores = PlayerScores.Num();
		for (INT Index = 0; Index < NumScores; Index++)
		{
			const FOnlinePlayerScore& Score = PlayerScores(Index);
			// Don't record scores for bots
			if (Score.PlayerID.Uid != 0)
			{
				FPendingPlayerStats& PendingPlayerStats = FindOrAddPendingPlayerStats(Score.PlayerID);
				PendingPlayerStats.Score = Score;
			}
		}
	}

	return TRUE;
}


/**
 * Called when ready to submit all collected stats
 *
 * @param Players the players for which to submit stats
 * @param StatsWrites the stats to submit for the players
 * @param PlayerScores the scores for the players
 */
UBOOL UOnlineSubsystemSteamworks::CreateAndSubmitStatsReport(void)
{
#if !HAVE_STEAM_GAMESERVER_STATS && !HAVE_STEAM_CLIENT_STATS
	return FALSE;
#else
	DWORD Return = S_OK;
	if (SessionHasStats())
	{
		// Handle automated achievements updates (must be done before stats updates, because Steam auto-unlocks achievements from the
		//	backend, if they are linked up with backend stats; there is a bug with Steam listen hosts, where the achievement is
		//	marked as unlocked clientside when it is in fact not, due to the pending stats update)
		for (INT i=0; i<PendingAchievementProgress.Num(); i++)
		{
			const FAchievementProgressStat& CurAch = PendingAchievementProgress(i);

			if (CurAch.bUnlock)
			{
				if (!UnlockAchievement(LoggedInPlayerNum, CurAch.AchievementId))
					debugf(TEXT("Achievement unlock failed for AchievementId: %i"), CurAch.AchievementId);
			}
			else if (CurAch.Progress > 0 && CurAch.Progress < CurAch.MaxProgress)
			{
				if (!DisplayAchievementProgress(CurAch.AchievementId, CurAch.Progress, CurAch.MaxProgress))
				{
					debugf(TEXT("Achievement progress display failed; AchievementId: %i, Progress: %i, MaxProgress: %i"),
							CurAch.AchievementId, CurAch.Progress, CurAch.MaxProgress);
				}
			}
			else
			{
				debugf(TEXT("Bad PendingAchievementProgress entry: AchievementId: %i, Progress: %i, MaxProgress: %i, bUnlock: %i"),
						CurAch.AchievementId, CurAch.Progress, CurAch.MaxProgress, CurAch.bUnlock);
			}
		}


		// Sort and filter player stats
		//PreprocessPlayersByScore();

		// Get the player count remaining after filtering
		const INT PlayerCount = PendingStats.Num();

		// Skip processing if there is no data to report
		if (PlayerCount > 0)
		{
			Return = ERROR_IO_PENDING;

			bGSStatsStoresSuccess = TRUE;   // callbacks may change this.

			for (INT Index = 0; Index < PlayerCount; Index++)
			{
				const FPendingPlayerStats &Stats = PendingStats(Index);
				const INT StatCount = Stats.Stats.Num();
				const CSteamID SteamPlayerId((uint64)Stats.Player.Uid);

				for (INT StatIndex = 0; StatIndex < StatCount; StatIndex++)
				{
					const FPlayerStat &Stat = Stats.Stats(StatIndex);
					const FString Key(GetStatsFieldName(Stat.ViewId, Stat.ColumnId));
					UBOOL bOk = TRUE;

					if (Stat.Data.Type == SDT_Int32)
					{
						INT Int32Value = 0;
						Stat.Data.GetData(Int32Value);

						// If stat values should be added together incrementally with the current stat values, do so
						if (bIncrementStatValues)
						{
							int32 CurVal = 0;
							bOk = FALSE;

							if (GSteamUserStats != NULL)
							{
								if (GSteamUserStats->GetUserStat(SteamPlayerId, TCHAR_TO_UTF8(*Key), &CurVal))
								{
									Int32Value += (INT)CurVal;
									bOk = TRUE;
								}
							}
							else if (GSteamGameServerStats != NULL)
							{
								if (GSteamGameServerStats->GetUserStat(SteamPlayerId, TCHAR_TO_UTF8(*Key),
													&CurVal))
								{
									Int32Value += (INT)CurVal;
									bOk = TRUE;
								}
							}
						}

#if HAVE_STEAM_GAMESERVER_STATS
						if (bOk && IsServer())
						{
							bOk = GSteamGameServerStats != NULL && GSteamGameServerStats->SetUserStat(SteamPlayerId,
													TCHAR_TO_UTF8(*Key),
													Int32Value);

#if HAVE_STEAM_CLIENT_STATS
							// If a listen server tries to set client-only stats for the host, it will fail, and
							//	needs to use GSteamUserStats instead (affects achievements in particular)
							if (!bOk && Stats.Player == LoggedInPlayerId)
							{
								bOk = GSteamUserStats != NULL && GSteamUserStats->SetStat(TCHAR_TO_UTF8(*Key),
																Int32Value);
							}
#endif
						}
#endif

#if HAVE_STEAM_CLIENT_STATS
						if (bOk && !IsServer())
						{
							bOk = GSteamUserStats != NULL && GSteamUserStats->SetStat(TCHAR_TO_UTF8(*Key), Int32Value);
						}
#endif
					}
					else if (Stat.Data.Type == SDT_Float)
					{
						FLOAT FloatValue = 0.0f;
						Stat.Data.GetData(FloatValue);


						// If stat values should be added together incrementally with the current stat values, do so
						if (bIncrementStatValues)
						{
							float CurVal = 0.0;
							bOk = FALSE;

							if (GSteamUserStats != NULL)
							{
								if (GSteamUserStats->GetUserStat(SteamPlayerId, TCHAR_TO_UTF8(*Key), &CurVal))
								{
									FloatValue += (FLOAT)CurVal;
									bOk = TRUE;
								}
							}
							else if (GSteamGameServerStats != NULL)
							{
								if (GSteamGameServerStats->GetUserStat(SteamPlayerId, TCHAR_TO_UTF8(*Key),
													&CurVal))
								{
									FloatValue += (FLOAT)CurVal;
									bOk = TRUE;
								}
							}
						}

#if HAVE_STEAM_GAMESERVER_STATS
						if (bOk && IsServer())
						{
							bOk = GSteamGameServerStats != NULL && GSteamGameServerStats->SetUserStat(SteamPlayerId,
								TCHAR_TO_UTF8(*Key), FloatValue);

#if HAVE_STEAM_CLIENT_STATS
							// If a listen server tries to set client-only stats for the host, it will fail, and
							//	needs to use GSteamUserStats instead (affects achievements in particular)
							if (!bOk && Stats.Player == LoggedInPlayerId)
							{
								bOk = GSteamUserStats != NULL && GSteamUserStats->SetStat(TCHAR_TO_UTF8(*Key),
																FloatValue);
							}
#endif
						}
#endif

#if HAVE_STEAM_CLIENT_STATS
						if (bOk && !IsServer())
						{
							bOk = GSteamUserStats != NULL && GSteamUserStats->SetStat(TCHAR_TO_UTF8(*Key), FloatValue);
						}
#endif
					}
					else
					{
						debugf(NAME_DevOnline, TEXT("Unexpected stats type!"));
						bOk = FALSE;
					}

					if (!bOk)
					{
						// We'll keep going though; maybe we'll get SOME of the stats through...
						bGSStatsStoresSuccess = FALSE;
					}
				}

#if HAVE_STEAM_GAMESERVER_STATS
				if (IsServer())
				{
					TotalGSStatsStoresPending++;
					CallbackBridge->AddSpecificGSStatsStore(GSteamGameServerStats->StoreUserStats(SteamPlayerId));
				}
#endif
			}

#if HAVE_STEAM_CLIENT_STATS
			// Since client stats can only update for the owning client, you only need to attempt stats storage once
			if (!IsServer())
			{
				if (GSteamUserStats != NULL && GSteamUserStats->StoreStats())
				{
					// Reuse this variable, to notify code that client stats are pending
					TotalGSStatsStoresPending = 1;
				}
				else
				{
					Return = E_FAIL;
				}
			}
#endif
		}
		else
		{
			Return = S_OK;
			debugf(NAME_DevOnline,TEXT("No players present to report data for"));
		}


		// Now store leaderboard stats
		for (INT i=0; i<PendingLeaderboardStats.Num(); i++)
		{
			if (!WriteLeaderboardScore(PendingLeaderboardStats(i).LeaderboardName, PendingLeaderboardStats(i).Score))
			{
				debugf(NAME_DevOnline, TEXT("Failed to write to leaderboard '%s'"), *PendingLeaderboardStats(i).LeaderboardName);
				Return = E_FAIL;
			}
		}
	}
	if (Return != ERROR_IO_PENDING)
	{
		FAsyncTaskDelegateResultsNamedSession Params(FName(TEXT("Game")),Return);
		TriggerOnlineDelegates(this,FlushOnlineStatsDelegates,&Params);
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
#endif
}

/**
 * Searches for a player's pending stats, returning them if they exist, or adding them if they don't
 *
 * @param Player the player to find/add stats for
 *
 * @return the existing/new stats for the player
 */
FPendingPlayerStats& UOnlineSubsystemSteamworks::FindOrAddPendingPlayerStats(const FUniqueNetId& Player)
{
	// First check if this player already has pending stats
	for(INT Index = 0; Index < PendingStats.Num(); Index++)
	{
		if (PendingStats(Index).Player.Uid == Player.Uid)
		{
			return PendingStats(Index);
		}
	}
	// This player doesn't have any stats, add them to the array
	INT Index = PendingStats.AddZeroed();
	FPendingPlayerStats& PlayerStats = PendingStats(Index);
	PlayerStats.Player = Player;

	// Use the stat guid from the player struct if a client, otherwise use the host's id
	if (PlayerStats.Player.Uid == LoggedInPlayerId.Uid)
	{
		// !!! FIXME: not used at the moment.
		//PlayerStats.StatGuid = ANSI_TO_TCHAR((const gsi_u8*)scGetConnectionId(SCHandle));
		PlayerStats.StatGuid = RenderCSteamID(LoggedInPlayerId.Uid);
	}
	return PlayerStats;
}

void UOnlineSubsystemSteamworks::AddOrUpdatePlayerStat(TArray<FPlayerStat>& PlayerStats, INT ViewId, INT ColumnId, const FSettingsData& Data)
{
	for (INT StatIndex = 0; StatIndex < PlayerStats.Num(); StatIndex++)
	{
		FPlayerStat &Stats = PlayerStats(StatIndex);
		if ((Stats.ViewId == ViewId) && (Stats.ColumnId == ColumnId))
		{
			Stats.Data = Data;
			return;
		}
	}
	INT AddIndex = PlayerStats.AddZeroed();
	FPlayerStat& NewPlayer = PlayerStats(AddIndex);
	NewPlayer.ViewId = ViewId;
	NewPlayer.ColumnId = ColumnId;
	NewPlayer.Data = Data;
}

void UOnlineSubsystemSteamworks::AddOrUpdateAchievementStat(const FAchievementMappingInfo& Ach, const FSettingsData& Data)
{
	INT CurProgress = 0;
	Data.GetData(CurProgress);

	UBOOL bUnlock = Ach.bAutoUnlock && CurProgress >= Ach.MaxProgress;

	// No monitoring needed if neither an unlock or progress toast is due
	if (!bUnlock && (Ach.ProgressCount == 0 || CurProgress == 0 || (CurProgress % Ach.ProgressCount) != 0))
		return;


	// Make sure the achievement hasn't already been achieved
	bool bAchieved = false;

	if (GSteamUserStats == NULL || !GSteamUserStats->GetAchievement(TCHAR_TO_ANSI(*Ach.AchievementName.GetNameString()), &bAchieved))
	{
		debugf(TEXT("AddOrUpdateAchievementStat: Failed to retrieve achievement status for achievement '%s'"),
			*Ach.AchievementName.GetNameString());

		return;
	}

	if (bAchieved)
		return;


	// Make sure the achievement stat value is actually changing value (otherwise you display the progress toast on every stats update)
	const CSteamID SteamPlayerId((uint64)LoggedInPlayerId.Uid);
	const FString Key(GetStatsFieldName(Ach.ViewId, Ach.AchievementId));
	INT OldProgress = 0;

	if (GSteamUserStats != NULL && GSteamUserStats->GetUserStat(SteamPlayerId, TCHAR_TO_UTF8(*Key), &OldProgress))
	{
		if (CurProgress == OldProgress)
			return;
	}
	else
	{
		debugf(TEXT("AddOrUpdateAchievementStat: Failed to retrieve current value for AchievementId: %i"), Ach.AchievementId);
		return;
	}


	// Store/update the progress
	for (INT i=0; i<PendingAchievementProgress.Num(); i++)
	{
		if (PendingAchievementProgress(i).AchievementId == Ach.AchievementId)
		{
			PendingAchievementProgress(i).Progress = CurProgress;
			PendingAchievementProgress(i).MaxProgress = Ach.MaxProgress;
			PendingAchievementProgress(i).bUnlock = bUnlock;

			return;
		}
	}

	INT CurIndex = PendingAchievementProgress.Add(1);

	PendingAchievementProgress(CurIndex).AchievementId = Ach.AchievementId;
	PendingAchievementProgress(CurIndex).Progress = CurProgress;
	PendingAchievementProgress(CurIndex).MaxProgress = Ach.MaxProgress;
	PendingAchievementProgress(CurIndex).bUnlock = bUnlock;
}

FString UOnlineSubsystemSteamworks::LeaderboardNameLookup(INT ViewId)
{
	for (INT i=0; i<LeaderboardNameMappings.Num(); i++)
	{
		if (LeaderboardNameMappings(i).ViewId == ViewId)
			return LeaderboardNameMappings(i).LeaderboardName;
	}

	return FString();
}

void UOnlineSubsystemSteamworks::AddOrUpdateLeaderboardStat(INT ViewId, const FSettingsData& Data)
{
	FString LeaderboardName = LeaderboardNameLookup(ViewId);

	if (LeaderboardName.IsEmpty())
	{
		// NOTE: Ignore this if the specified ViewId will not use leaderboards
		debugf(NAME_DevOnline, TEXT("AddOrUpdateLeaderboardStat: Missing LeaderboardNameMappings entry for ViewId: %i"), ViewId);

		return;
	}

	if (Data.Type != SDT_Int32)
	{
		debugf(TEXT("AddOrUpdateLeaderboardStat: Data type for leaderboard '%s' (ViewId '%i%') must be int32"), *LeaderboardName, ViewId);
		return;
	}


	// Store/update the stats
	INT CurVal = 0;
	Data.GetData(CurVal);

	for (INT i=0; i<PendingLeaderboardStats.Num(); i++)
	{
		if (PendingLeaderboardStats(i).LeaderboardName == LeaderboardName)
		{
			PendingLeaderboardStats(i).Score = CurVal;
			return;
		}
	}

	INT CurIndex = PendingLeaderboardStats.AddZeroed(1);

	PendingLeaderboardStats(CurIndex).LeaderboardName = LeaderboardName;
	PendingLeaderboardStats(CurIndex).Score = CurVal;
}



/**
 * Commits any changes in the online stats cache to the permanent storage
 *
 * @param SessionName the name of the session flushing stats
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::FlushOnlineStats(FName SessionName)
{
	// Skip processing if the server isn't logged in
	if (SessionHasStats())
	{
		if (PendingStats.Num() > 0)
		{
			CreateAndSubmitStatsReport();
			PendingStats.Empty();
			PendingLeaderboardStats.Empty();
			PendingAchievementProgress.Empty();
		}
	}
	return TRUE;
}

/**
 * Reads the host's stat guid for synching up stats. Only valid on the host.
 *
 * @return the host's stat guid
 */
FString UOnlineSubsystemSteamworks::GetHostStatGuid()
{
	// not used at the moment.
	return FString();
}

/**
 * Registers the host's stat guid with the client for verification they are part of
 * the stat. Note this is an async task for any backend communication that needs to
 * happen before the registration is deemed complete
 *
 * @param HostStatGuid the host's stat guid
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::RegisterHostStatGuid(const FString& HostStatGuid)
{
	// Not used at the moment.
	FAsyncTaskDelegateResults Params(S_OK);
	TriggerOnlineDelegates(this,RegisterHostStatGuidCompleteDelegates,&Params);
	return TRUE;
}

/**
 * Reads the client's stat guid that was generated by registering the host's guid
 * Used for synching up stats. Only valid on the client. Only callable after the
 * host registration has completed
 *
 * @return the client's stat guid
 */
FString UOnlineSubsystemSteamworks::GetClientStatGuid()
{
	// not used at the moment.
	return FString();
}

/**
 * Registers the client's stat guid on the host to validate that the client was in the stat.
 * Used for synching up stats. Only valid on the host.
 *
 * @param PlayerId the client's unique net id
 * @param ClientStatGuid the client's stat guid
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::RegisterStatGuid(FUniqueNetId PlayerID,const FString& ClientStatGuid)
{
	// Not really used at the moment.
	FPendingPlayerStats& PendingPlayerStats = FindOrAddPendingPlayerStats(PlayerID);
	PendingPlayerStats.StatGuid = ClientStatGuid;
	return TRUE;
}


/** Return steamcloud filename for player profile. */
FString UOnlineSubsystemSteamworks::CreateProfileName()
{
	return TEXT("profile.bin");  // this is what the GameSpy/PS3 stuff does. Don't split it per-user, since Steam handles that.
}

/**
 * Determines whether the user's profile file exists or not
 */
UBOOL UOnlineSubsystemSteamworks::DoesProfileExist()
{
	if (GSteamworksInitialized && LoggedInPlayerName.Len())
	{
		// There is a proper FileExists API, but if it doesn't exist it'll return zero for the size, and we need to treat zero-len profiles as missing anyhow.
		//return GSteamRemoteStorage->FileExists(TCHAR_TO_UTF8(*CreateProfileName()));
		return GSteamRemoteStorage->GetFileSize(TCHAR_TO_UTF8(*CreateProfileName())) > 0;
	}
	return FALSE;
}

/**
 * Reads the online profile settings for a given user from disk. If the file
 * exists, an async task is used to verify the file wasn't hacked and to
 * decompress the contents into a buffer. Once the task
 *
 * @param LocalUserNum the user that we are reading the data for
 * @param ProfileSettings the object to copy the results to and contains the list of items to read
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::ReadProfileSettings(BYTE LocalUserNum,
	UOnlineProfileSettings* ProfileSettings)
{
	DWORD Return = E_FAIL;
	// Only read the data for the logged in player
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// Only read if we don't have a profile for this player
		if (CachedProfile == NULL)
		{
			if (ProfileSettings != NULL)
			{
				CachedProfile = ProfileSettings;
				CachedProfile->AsyncState = OPAS_Read;
				// Clear the previous set of results
				CachedProfile->ProfileSettings.Empty();
				// Don't try to read without being logged in
				if (LoggedInStatus > LS_NotLoggedIn)
				{
					// Don't bother reading the local file if they haven't saved it before
					if (DoesProfileExist())
					{
						const int32	Size = GSteamRemoteStorage->GetFileSize(TCHAR_TO_UTF8(*CreateProfileName()));
						TArray<BYTE> Buffer(Size);
						if (GSteamRemoteStorage->FileRead(TCHAR_TO_UTF8(*CreateProfileName()), &Buffer(0), Size) == Size)
						{
							FProfileSettingsReader Reader(3000,TRUE,Buffer.GetTypedData(),Buffer.Num());
							// Serialize the profile from that array
							if (Reader.SerializeFromBuffer(CachedProfile->ProfileSettings))
							{
								INT ReadVersion = CachedProfile->GetVersionNumber();
								// Check the version number and reset to defaults if they don't match
								if (CachedProfile->VersionNumber != ReadVersion)
								{
									debugf(NAME_DevOnline,
										TEXT("Detected profile version mismatch (%d != %d), setting to defaults"),
										CachedProfile->VersionNumber,
										ReadVersion);
									CachedProfile->eventSetToDefaults();
								}
								Return = S_OK;
							}
							else
							{
								debugf(NAME_DevOnline,
									TEXT("Profile data for %s was corrupt, using defaults"),
									*LoggedInPlayerName);
								CachedProfile->eventSetToDefaults();
								Return = S_OK;
							}
						}
						else
						{
							debugf(NAME_DevOnline, TEXT("Failed to read local profile"));
							CachedProfile->eventSetToDefaults();
							if (LoggedInStatus == LS_UsingLocalProfile)
							{
								CachedProfile->AsyncState = OPAS_Finished;
								// Immediately save to that so the profile will be there in the future
								WriteProfileSettings(LocalUserNum,ProfileSettings);
							}
							Return = S_OK;
						}
					}
					else
					{
						// First time read so use defaults
						CachedProfile->eventSetToDefaults();
						CachedProfile->AsyncState = OPAS_Finished;
						// Immediately save to that so the profile will be there in the future
						WriteProfileSettings(LocalUserNum,ProfileSettings);
						Return = S_OK;
					}
				}
				else
				{
					debugf(NAME_DevOnline,
						TEXT("Player is not logged in using defaults for profile data"),
						*LoggedInPlayerName);
					CachedProfile->eventSetToDefaults();
					Return = S_OK;
				}
			}
			else
			{
				debugf(NAME_Error,TEXT("Can't specify a null profile settings object"));
			}
		}
		// Make sure the profile isn't already being read, since this is going to
		// complete immediately
		else if (CachedProfile->AsyncState != OPAS_Read)
		{
			debugf(NAME_DevOnline,TEXT("Using cached profile data instead of reading"));
			// If the specified read isn't the same as the cached object, copy the
			// data from the cache
			if (CachedProfile != ProfileSettings)
			{
				ProfileSettings->ProfileSettings = CachedProfile->ProfileSettings;
				CachedProfile = ProfileSettings;
			}
			Return = S_OK;
		}
		else
		{
			debugf(NAME_Error,TEXT("Profile read for player (%d) is already in progress"),LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Specified user is not logged in, setting to the defaults"));
		ProfileSettings->eventSetToDefaults();
		Return = S_OK;
	}
	// Trigger the delegates if there are any registered
	if (Return != ERROR_IO_PENDING)
	{
		// Mark the read as complete
		if (CachedProfile && LocalUserNum == LoggedInPlayerNum)
		{
			CachedProfile->AsyncState = OPAS_Finished;
		}
		check(LocalUserNum < 4);
		OnlineSubsystemSteamworks_eventOnReadProfileSettingsComplete_Parms Parms(EC_EventParm);
		Parms.bWasSuccessful = (Return == 0) ? FIRST_BITFIELD : 0;
		Parms.LocalUserNum = LocalUserNum;
		// Use the common method to do the work
		TriggerOnlineDelegates(this,ProfileCache.ReadDelegates,&Parms);
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
}

/**
 * Writes the online profile settings for a given user Live using an async task
 *
 * @param LocalUserNum the user that we are writing the data for
 * @param ProfileSettings the list of settings to write out
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::WriteProfileSettings(BYTE LocalUserNum,
	UOnlineProfileSettings* ProfileSettings)
{
	DWORD Return = E_FAIL;
	// Only the logged in user can write their profile data
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// Don't allow a write if there is a task already in progress
		if (CachedProfile == NULL ||
			(CachedProfile->AsyncState != OPAS_Read && CachedProfile->AsyncState != OPAS_Write))
		{
			if (ProfileSettings != NULL)
			{
				// Cache to make sure GC doesn't collect this while we are waiting
				// for the task to complete
				CachedProfile = ProfileSettings;
				// Mark this as a write in progress
				CachedProfile->AsyncState = OPAS_Write;
				// Make sure the profile settings have a version number
				CachedProfile->AppendVersionToSettings();
				// Update the save count for roaming profile support
				UOnlinePlayerStorage::SetProfileSaveCount(UOnlinePlayerStorage::GetProfileSaveCount(CachedProfile->ProfileSettings, PSI_ProfileSaveCount) + 1,ProfileSettings->ProfileSettings, PSI_ProfileSaveCount);

				// Write the data to Steam Cloud. This actually just goes to a local disk cache, and the Steam client pushes it
				//  to the Cloud when the game terminates, so this is a "fast" call that doesn't need async processing.

				// Serialize them to a blob and then write to disk
				FProfileSettingsWriter Writer(3000,TRUE);
				if (Writer.SerializeToBuffer(CachedProfile->ProfileSettings))
				{
					const INT Size = Writer.GetFinalBufferLength();
					if (GSteamRemoteStorage->FileWrite(TCHAR_TO_UTF8(*CreateProfileName()), (void*)Writer.GetFinalBuffer(), Size))
					{
						debugf(NAME_DevOnline, TEXT("Wrote profile (%d bytes) to Steam Cloud."), Size);
						Return = S_OK;
					}
					else
					{
						debugf(NAME_DevOnline, TEXT("Failed to write profile (%d bytes) to Steam Cloud!"), Size);
						Return = E_FAIL;
					}
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
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Ignoring write profile request for non-logged in player"));
		Return = S_OK;
	}
	if (Return != ERROR_IO_PENDING)
	{
		if (CachedProfile)
		{
			// Remove the write state so that subsequent writes work
			CachedProfile->AsyncState = OPAS_Finished;
		}
		OnlineSubsystemSteamworks_eventOnWriteProfileSettingsComplete_Parms Parms(EC_EventParm);
		Parms.bWasSuccessful = (Return == 0) ? FIRST_BITFIELD : 0;
		Parms.LocalUserNum = LoggedInPlayerNum;
		TriggerOnlineDelegates(this,WriteProfileSettingsDelegates,&Parms);
	}
	return Return == S_OK || Return == ERROR_IO_PENDING;
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
UBOOL UOnlineSubsystemSteamworks::ReadFriendsList(BYTE LocalUserNum,INT Count,INT StartingAt)
{
	DWORD Return = E_FAIL;
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		Return = S_OK;
	}
	// Always trigger the delegate immediately and again as friends are added
	FAsyncTaskDelegateResults Params(Return);
	TriggerOnlineDelegates(this,ReadFriendsDelegates,&Params);
	return Return == S_OK;
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
BYTE UOnlineSubsystemSteamworks::GetFriendsList(BYTE LocalUserNum,TArray<FOnlineFriend>& Friends,INT Count,INT StartingAt)
{
	// Empty the existing list so dupes don't happen
	Friends.Empty(Friends.Num());

	if (!GSteamworksInitialized)
	{
		return OERS_Done;
	}

	const INT NumBuddies = GSteamFriends->GetFriendCount(k_EFriendFlagImmediate);
	if (Count == 0)
	{
		Count = NumBuddies;
	}
	for (INT Index = 0; Index < Count; Index++)
	{
		const INT SteamFriendIndex = StartingAt + Index;
		if (SteamFriendIndex >= NumBuddies)
		{
			break;
		}

		const CSteamID SteamPlayerID(GSteamFriends->GetFriendByIndex(SteamFriendIndex, k_EFriendFlagImmediate));
		const char *NickName = GSteamFriends->GetFriendPersonaName(SteamPlayerID);
		const EPersonaState PersonaState = GSteamFriends->GetFriendPersonaState(SteamPlayerID);
		FriendGameInfo_t FriendGameInfo;
		const bool bInGame = GSteamFriends->GetFriendGamePlayed(SteamPlayerID, &FriendGameInfo);

		if (NickName[0] == 0)
		{
			// this user doesn't have a uniquenick
			// the regular nick could be used, but it not being unique could be a problem
			//TODO: should this use the regular nick?
			continue;
		}
		// !!! FIXME: we could generate the locationString properly by pinging the server directly, but that's a mess for several reasons.
		// !!! FIXME:  it would be nice if we could publish this information per-player through SteamFriends.
		//Friend.PresenceInfo =  // !!! FIXME: (wchar_t*)BuddyStatus.locationString;
		servernetadr_t Addr;
		Addr.Init(FriendGameInfo.m_unGameIP, FriendGameInfo.m_usQueryPort, FriendGameInfo.m_usGamePort);

		const INT FriendIndex = Friends.AddZeroed();
		FOnlineFriend& Friend = Friends(FriendIndex);
		Friend.UniqueId.Uid = (QWORD) SteamPlayerID.ConvertToUint64();
		Friend.NickName = UTF8_TO_TCHAR(NickName);
		Friend.PresenceInfo = ANSI_TO_TCHAR(Addr.GetConnectionAddressString());
 		Friend.bIsOnline = (PersonaState > k_EPersonaStateOffline);
		Friend.bIsPlaying = bInGame;
		Friend.bIsPlayingThisGame = (FriendGameInfo.m_gameID.AppID() == GSteamAppID);  // !!! FIXME: check mod id?
		if (Friend.bIsPlayingThisGame)
		{
			// Build an URL so we can parse options
			FURL Url(NULL,*Friend.PresenceInfo,TRAVEL_Absolute);
			// Get our current location to prevent joining the game we are already in
			const FString CurrentLocation = CachedGameInt && CachedGameInt->SessionInfo ?
				CachedGameInt->SessionInfo->HostAddr.ToString(FALSE) :
				TEXT("");
			// Parse the host address to see if they are joinable
			if (Url.Host.Len() > 0 &&
				// !!! FIXME
				//appStrstr((const TCHAR*)BuddyStatus.locationString,TEXT("Standalone")) == NULL &&
				//appStrstr((const TCHAR*)BuddyStatus.locationString,TEXT("bIsLanMatch")) == NULL &&
				(CurrentLocation.Len() == 0 || appStrstr(*Friend.PresenceInfo,*CurrentLocation) == NULL))
			{
				UBOOL bIsValid;
				FInternetIpAddr HostAddr;
				// Set the IP address listed and see if it's valid
				HostAddr.SetIp(*Url.Host,bIsValid);
				Friend.bIsJoinable = bIsValid;
			}
			Friend.bHasVoiceSupport = FALSE; // !!! FIXME: Url.HasOption(TEXT("bHasVoice"));
		}
	}
	return OERS_Done;
}

/**
 * Creates a network enabled account on the online service
 *
 * @param UserName the unique nickname of the account
 * @param Password the password securing the account
 * @param EmailAddress the address used to send password hints to
 * @param ProductKey the unique id for this installed product
 *
 * @return true if the account was created, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::CreateOnlineAccount(const FString& UserName,const FString& Password,const FString& EmailAddress,const FString& ProductKey)
{
	return FALSE;  // This is handled in the Steam client.
}

/**
 * Processes any talking delegates that need to be fired off
 */
void UOnlineSubsystemSteamworks::ProcessTalkingDelegates(void)
{
	// Skip all delegate handling if none are registered
	if (TalkingDelegates.Num() > 0)
	{
		// Only check players with voice
		if (CurrentLocalTalker.bHasVoice &&
			(CurrentLocalTalker.bWasTalking != CurrentLocalTalker.bIsTalking))
		{
			OnlineSubsystemSteamworks_eventOnPlayerTalkingStateChange_Parms Parms(EC_EventParm);
			// Use the cached id for this
			Parms.Player = LoggedInPlayerId;
			Parms.bIsTalking = CurrentLocalTalker.bIsTalking;
			TriggerOnlineDelegates(this,TalkingDelegates,&Parms);
			// Clear the flag so it only activates when needed
			CurrentLocalTalker.bWasTalking = CurrentLocalTalker.bIsTalking;
		}
		// Now check all remote talkers
		for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
		{
			FRemoteTalker& Talker = RemoteTalkers(Index);
			if (Talker.bWasTalking != Talker.bIsTalking)
			{
				OnlineSubsystemSteamworks_eventOnPlayerTalkingStateChange_Parms Parms(EC_EventParm);
				Parms.Player = Talker.TalkerId;
				Parms.bIsTalking = Talker.bIsTalking;
				TriggerOnlineDelegates(this,TalkingDelegates,&Parms);
				// Clear the flag so it only activates when needed
				Talker.bWasTalking = Talker.bIsTalking;
			}
		}
	}
}


/**
 * Processes any speech recognition delegates that need to be fired off
 */
void UOnlineSubsystemSteamworks::ProcessSpeechRecognitionDelegates(void)
{
	// Skip all delegate handling if we aren't using speech recognition
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		if (VoiceEngine->HasRecognitionCompleted(0))
		{
			TriggerOnlineDelegates(this,SpeechRecognitionCompleteDelegates,NULL);
		}
	}
}

/**
 * Registers the user as a talker
 *
 * @param LocalUserNum the local player index that is a talker
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::RegisterLocalTalker(BYTE LocalUserNum)
{
	DWORD Return = E_FAIL;
	if (VoiceEngine != NULL)
	{
		// Register the talker locally
		Return = VoiceEngine->RegisterLocalTalker(LocalUserNum);
		debugf(NAME_DevOnline,TEXT("RegisterLocalTalker(%d) returned 0x%08X"),
			LocalUserNum,Return);
		if (Return == S_OK)
		{
			CurrentLocalTalker.bHasVoice = TRUE;
		}
	}
	else
	{
		// Not properly logged in, so skip voice for them
		CurrentLocalTalker.bHasVoice = FALSE;
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
UBOOL UOnlineSubsystemSteamworks::UnregisterLocalTalker(BYTE LocalUserNum)
{
	DWORD Return = S_OK;
	// Skip the unregistration if not registered
	if (CurrentLocalTalker.bHasVoice == TRUE &&
		// Or when voice is disabled
		VoiceEngine != NULL)
	{
		Return = VoiceEngine->UnregisterLocalTalker(LocalUserNum);
		debugf(NAME_DevOnline,TEXT("UnregisterLocalTalker(%d) returned 0x%08X"),
			LocalUserNum,Return);
		CurrentLocalTalker.bHasVoice = FALSE;
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
UBOOL UOnlineSubsystemSteamworks::RegisterRemoteTalker(FUniqueNetId UniqueId)
{
	DWORD Return = E_FAIL;
	if (VoiceEngine != NULL)
	{
		// See if this talker has already been registered or not
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker == NULL)
		{
			// Add a new talker to our list
			INT AddIndex = RemoteTalkers.AddZeroed();
			Talker = &RemoteTalkers(AddIndex);
			// Copy the net id
			(QWORD&)Talker->TalkerId = (QWORD&)UniqueId;
			// Register the remote talker locally
			Return = VoiceEngine->RegisterRemoteTalker(UniqueId);
			debugf(NAME_DevOnline,TEXT("RegisterRemoteTalker(0x%016I64X) returned 0x%08X"),
				(QWORD&)UniqueId,Return);
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Remote talker is being re-registered"));
			Return = S_OK;
		}
		// Now start processing the remote voices
		Return = VoiceEngine->StartRemoteVoiceProcessing(UniqueId);
		debugf(NAME_DevOnline,TEXT("StartRemoteVoiceProcessing(0x%016I64X) returned 0x%08X"),
			(QWORD&)UniqueId,Return);
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
UBOOL UOnlineSubsystemSteamworks::UnregisterRemoteTalker(FUniqueNetId UniqueId)
{
	DWORD Return = E_FAIL;
	if (VoiceEngine != NULL)
	{
		// Make sure the talker is valid
		if (FindRemoteTalker(UniqueId) != NULL)
		{
			// Find them in the talkers array and remove them
			for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
			{
				const FRemoteTalker& Talker = RemoteTalkers(Index);
				// Is this the remote talker?
				if ((QWORD&)Talker.TalkerId == (QWORD&)UniqueId)
				{
					RemoteTalkers.Remove(Index);
					break;
				}
			}
			// Make sure to remove them from the mute list so that if they
			// rejoin they aren't already muted
			MuteList.RemoveItem(UniqueId);
			// Remove them from voice too
			Return = VoiceEngine->UnregisterRemoteTalker(UniqueId);
			debugf(NAME_DevOnline,TEXT("UnregisterRemoteTalker(0x%016I64X) returned 0x%08X"),
				(QWORD&)UniqueId,Return);
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Unknown remote talker specified to UnregisterRemoteTalker()"));
		}
	}
	return Return == S_OK;
}

/**
 * Finds a remote talker in the cached list
 *
 * @param UniqueId the net id of the player to search for
 *
 * @return pointer to the remote talker or NULL if not found
 */
FRemoteTalker* UOnlineSubsystemSteamworks::FindRemoteTalker(FUniqueNetId UniqueId)
{
	for (INT Index = 0; Index < RemoteTalkers.Num(); Index++)
	{
		FRemoteTalker& Talker = RemoteTalkers(Index);
		// Compare net ids to see if they match
		if (Talker.TalkerId.Uid == UniqueId.Uid)
		{
			return &RemoteTalkers(Index);
		}
	}
	return NULL;
}

/**
 * Determines if the specified player is actively talking into the mic
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsLocalPlayerTalking(BYTE LocalUserNum)
{
	return LocalUserNum == LoggedInPlayerNum && VoiceEngine != NULL && VoiceEngine->IsLocalPlayerTalking(LocalUserNum);
}

/**
 * Determines if the specified remote player is actively talking into the mic
 * NOTE: Network latencies will make this not 100% accurate
 *
 * @param UniqueId the unique id of the remote player being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsRemotePlayerTalking(FUniqueNetId UniqueId)
{
	return VoiceEngine != NULL && VoiceEngine->IsRemotePlayerTalking(UniqueId);
}

/**
 * Determines if the specified player has a headset connected
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player has a headset plugged in, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsHeadsetPresent(BYTE LocalUserNum)
{
	return LocalUserNum == LoggedInPlayerNum && VoiceEngine != NULL && VoiceEngine->IsHeadsetPresent(LocalUserNum);
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
UBOOL UOnlineSubsystemSteamworks::SetRemoteTalkerPriority(BYTE LocalUserNum,FUniqueNetId UniqueId,INT Priority)
{
	DWORD Return = E_FAIL;
	if (VoiceEngine != NULL)
	{
		// Find the remote talker to modify
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker != NULL)
		{
			Return = VoiceEngine->SetPlaybackPriority(LocalUserNum,UniqueId,Priority);
			debugf(NAME_DevOnline,TEXT("SetPlaybackPriority(%d,0x%016I64X,%d) return 0x%08X"),
				LocalUserNum,(QWORD&)UniqueId,Priority,Return);
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Unknown remote talker specified to SetRemoteTalkerPriority()"));
		}
	}
	return Return == S_OK;
}

/**
 * Mutes a remote talker for the specified local player. NOTE: This only mutes them in the
 * game unless the bIsSystemWide flag is true, which attempts to mute them globally
 *
 * @param LocalUserNum the user that is muting the remote talker
 * @param PlayerId the remote talker that is being muted
 * @param bIsSystemWide whether to try to mute them globally or not
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::MuteRemoteTalker(BYTE LocalUserNum,FUniqueNetId UniqueId,UBOOL bIsSystemWide)
{
	DWORD Return = E_FAIL;
	if (VoiceEngine != NULL)
	{
		// Find the specified talker
		FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
		if (Talker != NULL)
		{
			// Add them to the mute list
			MuteList.AddUniqueItem(UniqueId);
			Return = S_OK;
			debugf(NAME_DevOnline,TEXT("Muted talker 0x%016I64X"),(QWORD&)UniqueId);
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Unknown remote talker specified to MuteRemoteTalker()"));
		}
	}
	return Return == S_OK;
}

/**
 * Allows a remote talker to talk to the specified local player. NOTE: This only unmutes them in the
 * game unless the bIsSystemWide flag is true, which attempts to unmute them globally
 *
 * @param LocalUserNum the user that is allowing the remote talker to talk
 * @param PlayerId the remote talker that is being restored to talking
 * @param bIsSystemWide whether to try to unmute them globally or not
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::UnmuteRemoteTalker(BYTE LocalUserNum,FUniqueNetId UniqueId,UBOOL bIsSystemWide)
{
	DWORD Return = E_FAIL;
	if (LocalUserNum == LoggedInPlayerNum)
	{
		if (VoiceEngine != NULL)
		{
			// Find the specified talker
			FRemoteTalker* Talker = FindRemoteTalker(UniqueId);
			if (Talker != NULL)
			{
				// Remove them from the mute list
				MuteList.RemoveItem(UniqueId);
				Return = S_OK;
				debugf(NAME_DevOnline,TEXT("Muted talker 0x%016I64X"),(QWORD&)UniqueId);
			}
			else
			{
				debugf(NAME_DevOnline,TEXT("Unknown remote talker specified to UnmuteRemoteTalker()"));
			}
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("UnmuteRemoteTalker: Invalid LocalUserNum(%d) specified"),LocalUserNum);
	}
	return Return == S_OK;
}

/**
 * Tells the voice layer that networked processing of the voice data is allowed
 * for the specified player. This allows for push-to-talk style voice communication
 *
 * @param LocalUserNum the local user to allow network transimission for
 */
void UOnlineSubsystemSteamworks::StartNetworkedVoice(BYTE LocalUserNum)
{
	if (LocalUserNum == LoggedInPlayerNum)
	{
		CurrentLocalTalker.bHasNetworkedVoice = TRUE;
		// Since we don't leave the capturing on all the time to match a GameSpy bug, enable it now
		if (VoiceEngine)
		{
			VoiceEngine->StartLocalVoiceProcessing(LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid user specified in StartNetworkedVoice(%d)"),
			(DWORD)LocalUserNum);
	}
}

/**
 * Tells the voice layer to stop processing networked voice support for the
 * specified player. This allows for push-to-talk style voice communication
 *
 * @param LocalUserNum the local user to disallow network transimission for
 */
void UOnlineSubsystemSteamworks::StopNetworkedVoice(BYTE LocalUserNum)
{
	// Validate the range of the entry
	if (LocalUserNum == LoggedInPlayerNum)
	{
		CurrentLocalTalker.bHasNetworkedVoice = FALSE;
		// Since we don't leave the capturing on all the time due to a GameSpy bug, disable it now
		if (VoiceEngine)
		{
			VoiceEngine->StopLocalVoiceProcessing(LocalUserNum);
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid user specified in StopNetworkedVoice(%d)"),
			(DWORD)LocalUserNum);
	}
}

/**
 * Tells the voice system to start tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::StartSpeechRecognition(BYTE LocalUserNum)
{
	DWORD Return = E_FAIL;
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		Return = VoiceEngine->StartSpeechRecognition(LocalUserNum);
		debugf(NAME_DevOnline,TEXT("StartSpeechRecognition(%d) returned 0x%08X"),
			LocalUserNum,Return);
		if (Return == S_OK)
		{
			CurrentLocalTalker.bIsRecognizingSpeech = TRUE;
		}
	}
	return Return == S_OK;
}

/**
 * Tells the voice system to stop tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::StopSpeechRecognition(BYTE LocalUserNum)
{
	DWORD Return = E_FAIL;
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		Return = VoiceEngine->StopSpeechRecognition(LocalUserNum);
		debugf(NAME_DevOnline,TEXT("StopSpeechRecognition(%d) returned 0x%08X"),
			LocalUserNum,Return);
		CurrentLocalTalker.bIsRecognizingSpeech = FALSE;
	}
	return Return == S_OK;
}

/**
 * Gets the results of the voice recognition
 *
 * @param LocalUserNum the local user to read the results of
 * @param Words the set of words that were recognized by the voice analyzer
 *
 * @return true upon success, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::GetRecognitionResults(BYTE LocalUserNum,TArray<FSpeechRecognizedWord>& Words)
{
	DWORD Return = E_FAIL;
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		Return = VoiceEngine->GetRecognitionResults(LocalUserNum,Words);
		debugf(NAME_DevOnline,TEXT("GetRecognitionResults(%d,Array) returned 0x%08X"),
			LocalUserNum,Return);
	}
	return Return == S_OK;
}

/**
 * Changes the vocabulary id that is currently being used
 *
 * @param LocalUserNum the local user that is making the change
 * @param VocabularyId the new id to use
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::SelectVocabulary(BYTE LocalUserNum,INT VocabularyId)
{
	DWORD Return = E_FAIL;
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		Return = VoiceEngine->SelectVocabulary(LocalUserNum,VocabularyId);
		debugf(NAME_DevOnline,TEXT("SelectVocabulary(%d,%d) returned 0x%08X"),
			LocalUserNum,VocabularyId,Return);
	}
	return Return == S_OK;
}

/**
 * Changes the object that is in use to the one specified
 *
 * @param LocalUserNum the local user that is making the change
 * @param SpeechRecogObj the new object use
 *
 * @param true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::SetSpeechRecognitionObject(BYTE LocalUserNum,USpeechRecognition* SpeechRecogObj)
{
	DWORD Return = E_FAIL;
	if (bIsUsingSpeechRecognition && VoiceEngine != NULL)
	{
		Return = VoiceEngine->SetRecognitionObject(LocalUserNum,SpeechRecogObj);
		debugf(NAME_DevOnline,TEXT("SetRecognitionObject(%d,%s) returned 0x%08X"),
			LocalUserNum,SpeechRecogObj ? *SpeechRecogObj->GetName() : TEXT("NULL"),Return);
	}
	return Return == S_OK;
}

/**
 * Sets the online status information to use for the specified player. Used to
 * tell other players what the player is doing (playing, menus, away, etc.)
 *
 * @param LocalUserNum the controller number of the associated user
 * @param StatusId the status id to use (maps to strings where possible)
 * @param LocalizedStringSettings the list of localized string settings to set
 * @param Properties the list of properties to set
 */
void UOnlineSubsystemSteamworks::SetOnlineStatus(BYTE LocalUserNum,INT StatusId,
	const TArray<FLocalizedStringSetting>& LocalizedStringSettings,
	const TArray<FSettingsProperty>& Properties)
{
    // !!! FIXME: can't explicitly set this in Steamworks (but it handles user status, location elsewhere).
}

/**
 * Sends a friend invite to the specified player
 *
 * @param LocalUserNum the user that is sending the invite
 * @param NewFriend the player to send the friend request to
 * @param Message the message to display to the recipient
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::AddFriend(BYTE LocalUserNum,FUniqueNetId NewFriend,const FString& Message)
{
	if ((GSteamworksInitialized) && (LocalUserNum == LoggedInPlayerNum) && (GetLoginStatus(LoggedInPlayerNum) > LS_NotLoggedIn))
	{
		// Only send the invite if they aren't on the friend's list already
		const CSteamID SteamPlayerID((uint64) NewFriend.Uid);
		if (GSteamFriends->GetFriendRelationship(SteamPlayerID) == k_EFriendRelationshipNone)
		{
			// !!! FIXME: this isn't right.
			GSteamFriends->ActivateGameOverlayToUser("steamid", SteamPlayerID);
		}
		return TRUE;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid or not logged in player specified (%d)"),(DWORD)LocalUserNum);
	}
	return FALSE;
}

/**
 * Sends a friend invite to the specified player nick
 *
 * This is done in two steps:
 *		1. Search for the player by unique nick
 *		2. If found, issue the request. If not, return an error
 *
 * @param LocalUserNum the user that is sending the invite
 * @param FriendName the name of the player to send the invite to
 * @param Message the message to display to the recipient
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::AddFriendByName(BYTE LocalUserNum,const FString& FriendName,const FString& Message)
{
	// !!! FIXME: can't search for arbitrary users through Steamworks.
	return FALSE;
}

/**
 * Removes a friend from the player's friend list
 *
 * @param LocalUserNum the user that is removing the friend
 * @param FormerFriend the player to remove from the friend list
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::RemoveFriend(BYTE LocalUserNum,FUniqueNetId FormerFriend)
{
	if ((GSteamworksInitialized) && (LocalUserNum == LoggedInPlayerNum) && (GetLoginStatus(LoggedInPlayerNum) > LS_UsingLocalProfile))
	{
		// Only remove if they are on the friend's list already
		const CSteamID SteamPlayerID((uint64) FormerFriend.Uid);
		if (GSteamFriends->GetFriendRelationship(SteamPlayerID) == k_EFriendRelationshipFriend)
		{
			// !!! FIXME: this isn't right.
			GSteamFriends->ActivateGameOverlayToUser("steamid", SteamPlayerID);
		}
		return TRUE;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid or not logged in player specified (%d)"),(DWORD)LocalUserNum);
	}
	return FALSE;
}

/**
 * Used to accept a friend invite sent to this player
 *
 * @param LocalUserNum the user the invite is for
 * @param RequestingPlayer the player the invite is from
 *
 * @param true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::AcceptFriendInvite(BYTE LocalUserNum,FUniqueNetId NewFriend)
{
	// shouldn't hit this; Steam handles all this outside of the game.
	return FALSE;
}

/**
 * Used to deny a friend request sent to this player
 *
 * @param LocalUserNum the user the invite is for
 * @param RequestingPlayer the player the invite is from
 *
 * @param true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::DenyFriendInvite(BYTE LocalUserNum,FUniqueNetId RequestingPlayer)
{
	// shouldn't hit this; Steam handles all this outside of the game.
	return FALSE;
}

/**
 * Sends a message to a friend
 *
 * @param LocalUserNum the user that is sending the message
 * @param Friend the player to send the message to
 * @param Message the message to display to the recipient
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::SendMessageToFriend(BYTE LocalUserNum,FUniqueNetId Friend,const FString& Message)
{
	if ((GSteamworksInitialized) && (LocalUserNum == LoggedInPlayerNum) && (GetLoginStatus(LoggedInPlayerNum) > LS_UsingLocalProfile))
	{
		// !!! FIXME: this isn't right
		const CSteamID SteamPlayerID((uint64) Friend.Uid);
		GSteamFriends->ActivateGameOverlayToUser("chat", SteamPlayerID);
		return TRUE;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid or not logged in player specified (%d)"),(DWORD)LocalUserNum);
	}
	return FALSE;
}

/**
 * Sends an invitation to play in the player's current session
 *
 * @param LocalUserNum the user that is sending the invite
 * @param Friend the player to send the invite to
 * @param Text ignored in Steamworks
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::SendGameInviteToFriend(BYTE LocalUserNum,FUniqueNetId Friend,const FString&)
{
	// This is handled by Steam.
	return FALSE;
}

/**
 * Sends invitations to play in the player's current session
 *
 * @param LocalUserNum the user that is sending the invite
 * @param Friends the player to send the invite to
 * @param Text ignored in Steamworks
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::SendGameInviteToFriends(BYTE LocalUserNum,const TArray<FUniqueNetId>& Friends,const FString& Text)
{
	// This is handled by Steam.
	return FALSE;
}

/**
 * Attempts to join a friend's game session (join in progress)
 *
 * @param LocalUserNum the user that is sending the invite
 * @param Friends the player to send the invite to
 *
 * @return true if successful, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::JoinFriendGame(BYTE LocalUserNum,FUniqueNetId Friend)
{
	// Steam handles this.
	return FALSE;
}

/**
 * Reads any data that is currently queued in the voice interface
 */
void UOnlineSubsystemSteamworks::ProcessLocalVoicePackets(void)
{
	UNetDriver* NetDriver = GWorld->GetNetDriver();
	// Skip if the netdriver isn't present, as there's no network to route data on
	if (VoiceEngine != NULL)
	{
		// Only process the voice data if either network voice is enabled or
		// if the player is trying to have their voice issue commands
		if (CurrentLocalTalker.bHasNetworkedVoice ||
			CurrentLocalTalker.bIsRecognizingSpeech)
		{
			// Read the data from any local talkers
			DWORD DataReadyFlags = VoiceEngine->GetVoiceDataReadyFlags();
			// See if the logged in player has data
			if (DataReadyFlags & (1 << LoggedInPlayerNum))
			{
				// Mark the person as talking
				DWORD SpaceAvail = MAX_VOICE_DATA_SIZE - GVoiceData.LocalPackets[LoggedInPlayerNum].Length;
				// Figure out if there is space for this packet
				if (SpaceAvail > 0)
				{
					DWORD NumPacketsCopied = 0;
					// Figure out where to append the data
					BYTE* BufferStart = GVoiceData.LocalPackets[LoggedInPlayerNum].Buffer;
					BufferStart += GVoiceData.LocalPackets[LoggedInPlayerNum].Length;
					// Copy the sender info
					GVoiceData.LocalPackets[LoggedInPlayerNum].Sender = LoggedInPlayerId;
					// Process this user
					DWORD hr = VoiceEngine->ReadLocalVoiceData(LoggedInPlayerNum,
						BufferStart,
						&SpaceAvail);
					if (hr == S_OK)
					{
						// If there is no net connection or they aren't allowed to transmit, skip processing
						if (NetDriver &&
							LoggedInStatus == LS_LoggedIn &&
							CurrentLocalTalker.bHasNetworkedVoice)
						{
							// Update the length based on what it copied
							GVoiceData.LocalPackets[LoggedInPlayerNum].Length += SpaceAvail;
							if (SpaceAvail > 0)
							{
								CurrentLocalTalker.bIsTalking = TRUE;
							}
						}
						else
						{
							// Zero out the data since it isn't to be sent via the network
							GVoiceData.LocalPackets[LoggedInPlayerNum].Length = 0;
						}
					}
				}
				else
				{
					// Buffer overflow, so drop previous data
					GVoiceData.LocalPackets[LoggedInPlayerNum].Length = 0;
				}
			}
		}
	}
}

/**
 * Submits network packets to the voice interface for playback
 */
void UOnlineSubsystemSteamworks::ProcessRemoteVoicePackets(void)
{
	// Skip if we aren't networked
	if (GWorld->GetNetDriver())
	{
		// Now process all pending packets from the server
		for (INT Index = 0; Index < GVoiceData.RemotePackets.Num(); Index++)
		{
			FVoicePacket* VoicePacket = GVoiceData.RemotePackets(Index);
			if (VoicePacket != NULL)
			{
				// If the player has muted every one skip the processing
				if (CurrentLocalTalker.MuteType < MUTE_All)
				{
					UBOOL bIsMuted = FALSE;
					// Check for friends only muting
					if (CurrentLocalTalker.MuteType == MUTE_AllButFriends)
					{
						bIsMuted = IsFriend(LoggedInPlayerNum,VoicePacket->Sender) == FALSE;
					}
					// Now check the mute list
					if (bIsMuted == FALSE)
					{
						bIsMuted = MuteList.FindItemIndex(VoicePacket->Sender) != -1;
					}
					// Skip if they are muted
					if (bIsMuted == FALSE)
					{
						// Get the size since it is an in/out param
						DWORD PacketSize = VoicePacket->Length;
						// Submit this packet to the voice engine
						DWORD hr = VoiceEngine->SubmitRemoteVoiceData(VoicePacket->Sender,
							VoicePacket->Buffer,
							&PacketSize);
#if _DEBUG
						if (hr != S_OK)
						{
							debugf(NAME_DevOnline,TEXT("SubmitRemoteVoiceData() failed with 0x%08X"),hr);
						}
#endif
					}
					// Skip all delegate handling if none are registered
					if (bIsMuted == FALSE && TalkingDelegates.Num() > 0)
					{
						// Find the remote talker and mark them as talking
						for (INT Index2 = 0; Index2 < RemoteTalkers.Num(); Index2++)
						{
							FRemoteTalker& Talker = RemoteTalkers(Index2);
							// Compare the xuids
							if (Talker.TalkerId == VoicePacket->Sender)
							{
								Talker.bIsTalking = TRUE;
							}
						}
					}
				}
				VoicePacket->DecRef();
			}
		}
		// Zero the list without causing a free/realloc
		GVoiceData.RemotePackets.Reset();
	}
}

/** Registers all of the local talkers with the voice engine */
void UOnlineSubsystemSteamworks::RegisterLocalTalkers(void)
{
	RegisterLocalTalker(LoggedInPlayerNum);
}

/** Unregisters all of the local talkers from the voice engine */
void UOnlineSubsystemSteamworks::UnregisterLocalTalkers(void)
{
	UnregisterLocalTalker(LoggedInPlayerNum);
}

/**
 * Determines whether the player is allowed to use voice or text chat online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemSteamworks::CanCommunicate(BYTE LocalUserNum)
{
	return FPL_Enabled;
}

/**
 * Determines whether the player is allowed to play matches online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
BYTE UOnlineSubsystemSteamworks::CanPlayOnline(BYTE LocalUserNum)
{
	// GameSpy/PC always says "yes",
	// GameSpy/PS3 says "no" if you have parental controls blocking it, but never blocks for cheating.
	// Live doesn't implement it.
	return FPL_Enabled;
}

/**
 * Determines if the specified controller is connected or not
 *
 * @param ControllerId the controller to query
 *
 * @return true if connected, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsControllerConnected(INT ControllerId)
{
	return TRUE;
}

/**
 * Determines if the ethernet link is connected or not
 */
UBOOL UOnlineSubsystemSteamworks::HasLinkConnection(void)
{
	return TRUE;
}

/**
 * Displays the UI that prompts the user for their login credentials. Each
 * platform handles the authentication of the user's data.
 *
 * @param bShowOnlineOnly whether to only display online enabled profiles or not
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowLoginUI(UBOOL bShowOnlineOnly)
{
	return FALSE;
}

/**
 * Decrypts the product key and places it in the specified buffer
 *
 * @param Buffer the output buffer that holds the key
 * @param BufferLen the size of the buffer to fill
 *
 * @return TRUE if the decyption was successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::DecryptProductKey(BYTE* Buffer,DWORD BufferLen)
{
	if (EncryptedProductKey.Len())
	{
		BYTE EncryptedBuffer[256];
		DWORD Size = EncryptedProductKey.Len() / 3;
		// We need to convert to a buffer from the string form
		if (appStringToBlob(EncryptedProductKey,EncryptedBuffer,Size))
		{
			// Now we need to decrypt that buffer so we can submit it
			if (appDecryptBuffer(EncryptedBuffer,Size,Buffer,BufferLen))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**
 * Determines the NAT type the player is using
 */
BYTE UOnlineSubsystemSteamworks::GetNATType(void)
{
	return NAT_Open;   // This matches the PC GameSpy code...STUN negotiation happens elsewhere.
}

/**
 * Starts an asynchronous read of the specified file from the network platform's
 * title specific file store
 *
 * @param FileToRead the name of the file to read
 *
 * @return true if the calls starts successfully, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::ReadTitleFile(const FString& FileToRead)
{
	debugf(NAME_DevOnline, TEXT("ReadTitleFile('%s')"), *FileToRead);
	// !!! FIXME: the "title file" and Steam Cloud are different things (one is global to the game, the other is local to the user).
#if 0
	OnlineSubsystemSteamworks_eventOnReadTitleFileComplete_Parms Parms(EC_EventParm);
	Parms.bWasSuccessful = GSteamRemoteStorage->FileExists(TCHAR_TO_UTF8(*FileToRead)) ? FIRST_BITFIELD : 0;
	Parms.Filename = FileToRead;
	// Use the common method to do the work
	TriggerOnlineDelegates(this,ReadTitleFileCompleteDelegates,&Parms);
	return TRUE;   // Steam Cloud syncs before the game launches.
#endif
	return FALSE;
}

/**
 * Copies the file data into the specified buffer for the specified file
 *
 * @param FileToRead the name of the file to read
 * @param FileContents the out buffer to copy the data into
 *
 * @return true if the data was copied, false otherwise
 */
UBOOL UOnlineSubsystemSteamworks::GetTitleFileContents(const FString& FileName,TArray<BYTE>& FileContents)
{
	// !!! FIXME: the "title file" and Steam Cloud are different things (one is global to the game, the other is local to the user).
#if 0
	const int32 Size = GSteamRemoteStorage->GetFileSize(TCHAR_TO_UTF8(*FileName));
	if (Size < 0)
	{
		return FALSE;
	}
	FileContents.Reserve((INT) Size);
	if (GSteamRemoteStorage->FileRead(TCHAR_TO_UTF8(*FileName), &FileContents(0), Size) != Size)
	{
		FileContents.Empty();
		return FALSE;
	}
	return TRUE;
#endif
	return FALSE;
}

/**
 * Sets a new position for the network notification icons/images
 *
 * @param NewPos the new location to use
 */
void UOnlineSubsystemSteamworks::SetNetworkNotificationPosition(BYTE NewPos)
{
	CurrentNotificationPosition = NewPos;

	if (!GSteamworksInitialized)
	{
		return;
	}

	// Map our enum to Steamworks
	switch (CurrentNotificationPosition)
	{
        // !!! FIXME: There is no "center" in Steamworks, so I arbitrarily chose corners.
		case NNP_TopLeft:
		{
			GSteamUtils->SetOverlayNotificationPosition(k_EPositionTopLeft);
			break;
		}
		case NNP_TopRight:
		case NNP_TopCenter:
		{
			GSteamUtils->SetOverlayNotificationPosition(k_EPositionTopRight);
			break;
		}
		case NNP_CenterLeft:
		case NNP_BottomLeft:
		{
			GSteamUtils->SetOverlayNotificationPosition(k_EPositionBottomLeft);
			break;
		}
		case NNP_CenterRight:
		case NNP_Center:
		case NNP_BottomCenter:
		case NNP_BottomRight:
		{
			GSteamUtils->SetOverlayNotificationPosition(k_EPositionBottomRight);
			break;
		}
	}
}

/**
 * Starts an async read for the achievement list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param TitleId the title id of the game the achievements are to be read for
 * @param bShouldReadText whether to fetch the text strings or not
 * @param bShouldReadImages whether to fetch the image data or not
 *
 * @return TRUE if the task starts, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ReadAchievements(BYTE LocalUserNum,INT TitleId,UBOOL bShouldReadText,UBOOL bShouldReadImages)
{
	check(TitleId == 0);  // !!! FIXME: is this for checking different games?

	// @todo: Implement bShouldReadImages and bShouldReadText; the problem is, these are only relevant when calling GetAchievements

	if ((!GSteamworksInitialized) || (LocalUserNum != LoggedInPlayerNum))
	{
		return FALSE;
	}

	if ((UserStatsReceivedState != OERS_InProgress) && (UserStatsReceivedState != OERS_Done))
	{
		UserStatsReceivedState = OERS_InProgress;
		GSteamUserStats->RequestCurrentStats();
		return TRUE;  // success, pending.
	}

	OnlineSubsystemSteamworks_eventOnReadAchievementsComplete_Parms Parms(EC_EventParm);
	Parms.TitleId = TitleId;
	TriggerOnlineDelegates(this,AchievementReadDelegates,&Parms);
	return TRUE;
}


static UTexture2D *LoadSteamImageToTexture2D(const int Icon, const TCHAR *ImageName)
{
	uint32 Width, Height;
	if (!GSteamUtils->GetImageSize(Icon, &Width, &Height))
	{
		debugf(NAME_DevOnline, TEXT("Unexpected GetImageSize failure for Steam image %d ('%s')"), (INT) Icon, ImageName);
		return NULL;
	}

	TArray<BYTE> Buffer(Width * Height * 4);
	if (!GSteamUtils->GetImageRGBA(Icon, &Buffer(0), Buffer.Num()))
	{
		debugf(NAME_DevOnline, TEXT("Unexpected GetImageRGBA failure for Steam image %d ('%s')"), (INT) Icon, ImageName);
		return NULL;
	}

	UTexture2D* NewTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), INVALID_OBJECT, FName(ImageName));
	// disable compression, tiling, sRGB, and streaming for the texture
	NewTexture->CompressionNone			= TRUE;
	NewTexture->CompressionSettings		= TC_Default;
	NewTexture->MipGenSettings			= TMGS_NoMipmaps;
	NewTexture->CompressionNoAlpha		= TRUE;
	NewTexture->DeferCompression		= FALSE;
	NewTexture->bNoTiling				= TRUE;
	NewTexture->SRGB					= FALSE;
	NewTexture->NeverStream				= TRUE;
	NewTexture->LODGroup				= TEXTUREGROUP_UI;
	NewTexture->Init(Width,Height,PF_A8R8G8B8);
	// Only the first mip level is used
	check(NewTexture->Mips.Num() > 0);
	// Mip 0 is locked for the duration of the read request
	BYTE* MipData = (BYTE*)NewTexture->Mips(0).Data.Lock(LOCK_READ_WRITE);
	if (MipData)
	{
		// Calculate the row stride for the texture
		const INT TexStride = (Height / GPixelFormats[PF_A8R8G8B8].BlockSizeY) * GPixelFormats[PF_A8R8G8B8].BlockBytes;
		appMemzero(MipData, Height * TexStride);
		const BYTE *Src = (const BYTE *) &Buffer(0);
		for (uint32 Y = 0; Y < Height; Y++)
		{
			BYTE *Dest = MipData + (Y * TexStride);
			for (uint32 X = 0; X < Width; X++)
			{
				Dest[0] = Src[2];  // rgba to bgra
				Dest[1] = Src[1];
				Dest[2] = Src[0];
				Dest[3] = Src[3];
				Src += 4;
				Dest += 4;
			}
		}
		NewTexture->Mips(0).Data.Unlock();
		NewTexture->UpdateResource();   // Update the render resource for it
	}
	else
	{
		// Couldn't lock the mip
		debugf(NAME_DevOnline, TEXT("Failed to lock texture for Steam image %d ('%s')"), (INT) Icon, ImageName);
		NewTexture = NULL;   // let the garbage collector get it.
	}

	return NewTexture;
}

UBOOL UOnlineSubsystemSteamworks::GetOnlineAvatar(const struct FUniqueNetId PlayerNetId,FScriptDelegate &ReadOnlineAvatarCompleteDelegate,const UBOOL bTriggerOnFailure)
{
	if (!GSteamworksInitialized)
	{
		return FALSE;
	}

	UTexture2D *Avatar = NULL;
	const QWORD ProfileId = PlayerNetId.Uid;
	const CSteamID SteamPlayerId((uint64) ProfileId);

#if STEAMWORKS_112
	const int Icon = GSteamFriends->GetMediumFriendAvatar(SteamPlayerId);
#else
	const int Icon = GSteamFriends->GetFriendAvatar(SteamPlayerId, k_EAvatarSize64x64);
#endif

	if (Icon != 0)  // data for this user.
	{
		Avatar = LoadSteamImageToTexture2D(Icon, *FString::Printf(TEXT("Avatar_%uI64"), ProfileId));
		if (Avatar == NULL)
		{
			debugf(NAME_DevOnline,TEXT("GetOnlineAvatar() failed"));
		}

		if ((Avatar != NULL) || (bTriggerOnFailure))
		{
			OnlineSubsystemSteamworks_eventOnReadOnlineAvatarComplete_Parms Parms(EC_EventParm);
			Parms.PlayerNetId = PlayerNetId;
			Parms.Avatar = Avatar;
			ProcessDelegate(NAME_None,&ReadOnlineAvatarCompleteDelegate,&Parms);
		}
	}

	return (Avatar != NULL);
}

/**
 * Reads an avatar images for the specified player. Results are delivered via OnReadOnlineAvatarComplete delegates.
 *
 * @param PlayerNetId the unique id to read avatar for
 * @param ReadOnlineAvatarCompleteDelegate The delegate to call with results.
 */
void UOnlineSubsystemSteamworks::ReadOnlineAvatar(const struct FUniqueNetId PlayerNetId,FScriptDelegate ReadOnlineAvatarCompleteDelegate)
{
	debugf(NAME_DevOnline, TEXT("ReadOnlineAvatar for %s"), *RenderCSteamID(CSteamID((uint64) PlayerNetId.Uid)));

	// this isn't async for Steam. The Steam client either has the information or it doesn't (it caches people in your buddy list, on the same server, etc).
	if (!GetOnlineAvatar(PlayerNetId, ReadOnlineAvatarCompleteDelegate, FALSE))
	{
		// no data? Wait awhile, then re-request the buddy list, in case it shows up later (server's players' list catches up, etc).
		FQueuedAvatarRequest Request(EC_EventParm);
		Request.CheckTime = 0.0f;
		Request.NumberOfAttempts = 0;
		Request.PlayerNetId = PlayerNetId;
		Request.ReadOnlineAvatarCompleteDelegate = ReadOnlineAvatarCompleteDelegate;
		QueuedAvatarRequests.AddItem(Request);
	}
}

static void LoadAchievementDetails(TArray<FAchievementDetails>& Achievements, TArray<FAchievementMappingInfo>& AchievementLoadList,
					UBOOL bLoadImages=TRUE)
{
	unsigned int Index = 0;
	unsigned int ListLen = AchievementLoadList.Num();
	FString AchievementIdString;
	int Icon = 0;
	bool bAchieved;

	while (ListLen == 0 || Index < ListLen)  // If ListLen == 0, we'll break out when we run out of achievements
	{
		if (ListLen == 0)
			AchievementIdString = FString::Printf(TEXT("Achievement_%u"), Index);
		else
			AchievementIdString = AchievementLoadList(Index).AchievementName.GetNameString();


		FString AchievementName(UTF8_TO_TCHAR(GSteamUserStats->GetAchievementDisplayAttribute(TCHAR_TO_ANSI(*AchievementIdString), "name")));

		if (AchievementName.Len() == 0)
			break;


		if (bLoadImages)
		{
			// Disable warnings from the SDK for just this call, as we run until we fail.
			GSteamUtils->SetWarningMessageHook(SteamworksWarningMessageHookNoOp);
			Icon = GSteamUserStats->GetAchievementIcon(TCHAR_TO_ANSI(*AchievementIdString));
			GSteamUtils->SetWarningMessageHook(SteamworksWarningMessageHook);
		}


		FString AchievementDesc(UTF8_TO_TCHAR(GSteamUserStats->GetAchievementDisplayAttribute(TCHAR_TO_ANSI(*AchievementIdString), "desc")));
		FString ImageName = FString("AchievementImage_") + FString(AchievementName).Replace(TEXT(" "),TEXT("_"));

		UTexture2D* NewTexture = (Icon != 0 ? LoadSteamImageToTexture2D(Icon, *ImageName) : NULL);

		debugf(NAME_DevOnline, TEXT("Achievement #%d: '%s', '%s', Icon '%i'"), Index, *AchievementName, *AchievementDesc, Icon);

		const INT Position = Achievements.AddZeroed();
		FAchievementDetails &Achievement = Achievements(Position);
		Achievement.Id = Index;
		Achievement.AchievementName = AchievementName;
		Achievement.Description = AchievementDesc;
		Achievement.Image = NewTexture;

		if (GSteamUserStats->GetAchievement(TCHAR_TO_ANSI(*AchievementIdString), &bAchieved) && bAchieved)
		{
			Achievement.bWasAchievedOnline = TRUE;
			Achievement.bWasAchievedOffline = TRUE;
		}

		Index++;
	}
}


BYTE UOnlineSubsystemSteamworks::GetAchievements(BYTE LocalUserNum,TArray<FAchievementDetails>& Achievements,INT TitleId)
{
	check(TitleId == 0);  // !!! FIXME: is this for checking different games?

	Achievements.Reset();

	if ((!GSteamworksInitialized) || (LocalUserNum != LoggedInPlayerNum))
	{
		return OERS_NotStarted;
	}

	if (UserStatsReceivedState == OERS_Done)
	{
		LoadAchievementDetails(Achievements, AchievementMappings);
	}

	return UserStatsReceivedState;
}

/**
 * Unlocks the specified achievement for the specified user
 *
 * @param LocalUserNum the controller number of the associated user
 * @param AchievementId the id of the achievement to unlock
 *
 * @return TRUE if the call worked, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::UnlockAchievement(BYTE LocalUserNum,INT AchievementId)
{
	// Validate the user index
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		FString AchievementIdString;

		if (AchievementMappings.Num() == 0)
		{
			AchievementIdString = FString::Printf(TEXT("Achievement_%u"), AchievementId);
		}
		else
		{
			UBOOL bFound = FALSE;

			for (INT i=0; i<AchievementMappings.Num(); i++)
			{
				if (AchievementMappings(i).AchievementId == AchievementId)
				{
					AchievementIdString = AchievementMappings(i).AchievementName.GetNameString();

					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
			{
				debugf(TEXT("UnlockAchievement: AchievementId '%i' not found in AchievementMappings list"), AchievementId);
				return FALSE;
			}
		}

		bool bAchieved = false;
		if ((GSteamUserStats->GetAchievement(TCHAR_TO_ANSI(*AchievementIdString), &bAchieved)) && (bAchieved))
		{
			// we already have this achievement unlocked; just trigger and say we unlocked it.
			FAsyncTaskDelegateResults Results(ERROR_SUCCESS);
			TriggerOnlineDelegates(this,AchievementDelegates,&Results);
			return TRUE;
		}

		if (!GSteamUserStats->SetAchievement(TCHAR_TO_ANSI(*AchievementIdString)))
		{
			debugf(NAME_DevOnline,TEXT("SetAchievement() failed!"));
			FAsyncTaskDelegateResults Results(E_FAIL);
			TriggerOnlineDelegates(this,AchievementDelegates,&Results);
			return FALSE;
		}

		bStoringAchievement = TRUE;
		if (!GSteamUserStats->StoreStats())
		{
			debugf(NAME_DevOnline,TEXT("StoreStats() failed!"));
			bStoringAchievement = FALSE;
			FAsyncTaskDelegateResults Results(E_FAIL);
			TriggerOnlineDelegates(this,AchievementDelegates,&Results);
			return FALSE;
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to UnlockAchievement()"),
			(DWORD)LocalUserNum);
		return FALSE;
	}
	return TRUE;
}


/**
 * Sets up the specified leaderboard, so that read/write calls can be performed on it
 *
 * @param LeaderboardName	The name of the leaderboard to initiate
 * @return			Returns TRUE if the leaderboard is being setup, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::InitiateLeaderboard(const FString& LeaderboardName)
{
	UBOOL bResult = FALSE;
	INT ListIndex = INDEX_NONE;

	// Make sure the leaderboard isn't in 'LeaderboardList'
	for (INT i=0; i<LeaderboardList.Num(); i++)
	{
		if (LeaderboardList(i).LeaderboardName == LeaderboardName)
		{
			ListIndex = i;
			break;
		}
	}

	if (ListIndex == INDEX_NONE || (!LeaderboardList(ListIndex).bLeaderboardInitiated && !LeaderboardList(ListIndex).bLeaderboardInitializing))
	{
		if (GSteamworksInitialized)
		{
			// Increase the counter for pending leaderboard initiations, so callbacks can be reset later when it returns to 0
			PendingLeaderboardInits++;
			bResult = TRUE;

			// Add the LeaderboardList entry (if not already added)
			if (ListIndex == INDEX_NONE)
			{
				ListIndex = LeaderboardList.AddZeroed(1);
			}

			LeaderboardList(ListIndex).LeaderboardName = LeaderboardName;
			LeaderboardList(ListIndex).bLeaderboardInitializing = TRUE;

			// Kickoff the find request, and setup the returned callback
			CallbackBridge->AddUserFindLeaderboard(GSteamUserStats->FindLeaderboard(TCHAR_TO_UTF8(*LeaderboardName)));
		}
	}
	else
	{
		debugf(TEXT("Leaderboard name '%s' already exists in LeaderboardList; check LeaderboardList before calling"), *LeaderboardName);
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Reads entries from the specified leaderboard
 *
 * NOTE: The Start/End parameters determine the range of entries to return, and function differently depending upon RequestType:
 *	LRT_Global: Start/End start from the top of the leaderboard (which is 0)
 *	LRT_Player: Start/End start at the players location in the leaderboard, and can be + or - values (with End always greater than Start)
 *	LRT_Friends: Only people in the players Steam friends list are returned from the leaderboard
 *
 * @param LeaderboardName	The name of the leaderboard to access
 * @param RequestType		The type of entries to return from the leaderboard
 * @param Start			Dependant on RequestType, the start location for entries that should be returned from in the leaderboard
 * @param End			As above, but with respect to the end location, where entries should stop being returned
 * @return			Whether or not leaderboard entry reading failed/succeeded
 */
UBOOL UOnlineSubsystemSteamworks::ReadLeaderboardEntries(const FString& LeaderboardName, BYTE RequestType/*=LRT_Global*/, INT Start/*=0*/, INT End/*=0*/)
{
	UBOOL bResult = FALSE;
	INT ListIndex = INDEX_NONE;

	// Find the leaderboard in 'LeaderboardList'
	for (INT i=0; i<LeaderboardList.Num(); i++)
	{
		if (LeaderboardList(i).LeaderboardName == LeaderboardName)
		{
			ListIndex = i;
			break;
		}
	}

	if (GSteamworksInitialized)
	{
		// Make sure the leaderboard is initiated, and if not, defer the read request and automatically initiate it
		if (ListIndex != INDEX_NONE && LeaderboardList(ListIndex).bLeaderboardInitiated)
		{
			// If the leaderboard has a valid Steam leaderboard handle set, kick off the read request
			if (LeaderboardList(ListIndex).LeaderboardRef != NULL)
			{
				// Increase 'PendingLeaderboardReads', for resetting callbacks later
				PendingLeaderboardReads++;
				bResult = TRUE;

				ELeaderboardDataRequest SubRequestType = k_ELeaderboardDataRequestGlobal;

				if (RequestType == LRT_Player)
					SubRequestType = k_ELeaderboardDataRequestGlobalAroundUser;
				else if (RequestType == LRT_Friends)
					SubRequestType = k_ELeaderboardDataRequestFriends;

				CallbackBridge->AddUserDownloadedLeaderboardEntries(
					GSteamUserStats->DownloadLeaderboardEntries(LeaderboardList(ListIndex).LeaderboardRef, SubRequestType,
											Start, End));
			}
			else
			{
				debugf(TEXT("Invalid leaderboard handle for leaderboard '%s'"), *LeaderboardList(ListIndex).LeaderboardName);
			}
		}
		else
		{
			// Initialize the leaderboard, if it hasn't been already, and store the read request for execution later
			AWorldInfo* WI = GWorld != NULL ? GWorld->GetWorldInfo() : NULL;

			INT DefIndex = INDEX_NONE;

			// Check for a matching entry first (using 'AddUniqueItem' doesn't compile for some reason)
			for (INT i=0; i<DeferredLeaderboardReads.Num(); i++)
			{
				if (DeferredLeaderboardReads(i).LeaderboardName == LeaderboardName &&
					DeferredLeaderboardReads(i).RequestType == RequestType && DeferredLeaderboardReads(i).Start == Start &&
					DeferredLeaderboardReads(i).End == End)
				{
					DefIndex = i;
					break;
				}
			}

			// Add the entry if it's not already in the list
			if (DefIndex == INDEX_NONE)
			{
				DefIndex = DeferredLeaderboardReads.AddZeroed(1);

				DeferredLeaderboardReads(DefIndex).LeaderboardName = LeaderboardName;
				DeferredLeaderboardReads(DefIndex).RequestType = RequestType;
				DeferredLeaderboardReads(DefIndex).Start = Start;
				DeferredLeaderboardReads(DefIndex).End = End;
			}


			// Now kickoff initialization (if not already under way)
			if (ListIndex == INDEX_NONE || !LeaderboardList(ListIndex).bLeaderboardInitiated)
			{
				bResult = InitiateLeaderboard(LeaderboardName);

				// If initialization fails, remove the deferred entry
				if (!bResult)
					DeferredLeaderboardReads.Remove(DefIndex, 1);
			}
			else
			{
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Writes out the current players score, for the specified leaderboard
 *
 * @param LeaderboardName	The name of the leaderboard to write to
 * @param Score			The score value to submit to the leaderboard
 * @return			Whether or not the leaderboard score wrting failed/succeeded
 */
UBOOL UOnlineSubsystemSteamworks::WriteLeaderboardScore(const FString& LeaderboardName, INT Score)
{
	UBOOL bResult = FALSE;
	INT ListIndex = INDEX_NONE;

	if (GSteamworksInitialized)
	{
		// Find the leaderboard in 'LeaderboardList'
		for (INT i=0; i<LeaderboardList.Num(); i++)
		{
			if (LeaderboardList(i).LeaderboardName == LeaderboardName)
			{
				ListIndex = i;
				break;
			}
		}

		// Make sure the leaderboard is initiated, and if not, defer the write request and automatically initiate it
		if (ListIndex != INDEX_NONE && LeaderboardList(ListIndex).bLeaderboardInitiated)
		{

			// If the leaderboard has a valid Steam leaderboard handle set, kick off the upload request
			if (LeaderboardList(ListIndex).LeaderboardRef != NULL)
			{
				// Increase 'PendingLeaderboardWrites', for resetting callbacks later
				PendingLeaderboardWrites++;
				bResult = TRUE;

				ELeaderboardUploadScoreMethod UpdateType = (LeaderboardList(ListIndex).UpdateType == LUT_KeepBest ?
										k_ELeaderboardUploadScoreMethodKeepBest :
										k_ELeaderboardUploadScoreMethodForceUpdate);

				CallbackBridge->AddUserUploadedLeaderboardScore(GSteamUserStats->UploadLeaderboardScore(
									LeaderboardList(ListIndex).LeaderboardRef, UpdateType, Score, NULL, 0));
			}
			else
			{
				debugf(TEXT("Invalid leaderboard handle for leaderboard '%s'"), *LeaderboardList(ListIndex).LeaderboardName);
			}
		}
		else
		{
			// Initialize the leaderboard, if it hasn't been already, and store the write request for execution later
			AWorldInfo* WI = GWorld != NULL ? GWorld->GetWorldInfo() : NULL;

			INT DefIndex = INDEX_NONE;

			// Check for a matching entry first (using 'AddUniqueItem' doesn't compile for some reason)
			for (INT i=0; i<DeferredLeaderboardWrites.Num(); i++)
			{
				if (DeferredLeaderboardWrites(i).LeaderboardName == LeaderboardName && DeferredLeaderboardWrites(i).Score == Score)
				{
					DefIndex = i;
					break;
				}
			}

			// Add the entry if it's not already in the list
			if (DefIndex == INDEX_NONE)
			{
				DefIndex = DeferredLeaderboardWrites.AddZeroed(1);

				DeferredLeaderboardWrites(DefIndex).LeaderboardName = LeaderboardName;
				DeferredLeaderboardWrites(DefIndex).Score = Score;
			}


			// Now kickoff initialization (if not already under way)
			if (ListIndex == INDEX_NONE || !LeaderboardList(ListIndex).bLeaderboardInitiated)
			{
				bResult = InitiateLeaderboard(LeaderboardName);

				// If initialization fails, remove the deferred entry
				if (!bResult)
					DeferredLeaderboardWrites.Remove(DefIndex, 1);
			}
			else
			{
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Creates the specified leaderboard on the Steamworks backend
 * NOTE: It's best to use this for game/mod development purposes only, not for release usage
 *
 * @param LeaderboardName	The name to give the leaderboard (NOTE: This will be the human-readable name displayed on the backend and stats page)
 * @param SortType		The sorting to use for the leaderboard
 * @param DisplayFormat		The way to display leaderboard data
 * @return			Returns True if the leaderboard is being created, False otherwise
 */
UBOOL UOnlineSubsystemSteamworks::CreateLeaderboard(const FString& LeaderboardName, BYTE SortType, BYTE DisplayFormat)
{
	UBOOL bResult = FALSE;
	INT Index = INDEX_NONE;

	// If the leaderboard is already in 'LeaderboardList', it already exists on the backend
	for (INT i=0; i<LeaderboardList.Num(); i++)
	{
		if (LeaderboardList(i).LeaderboardName == LeaderboardName)
		{
			Index = i;
			break;
		}
	}

	if (Index == INDEX_NONE)
	{
		if (GSteamworksInitialized)
		{
			// Increase the counter for pending leaderboard initiations, so callbacks can be reset later when it returns to 0
			PendingLeaderboardInits++;
			bResult = TRUE;

			// Kickoff the find/create request, and setup the returned callback
			ELeaderboardSortMethod SortMethod = ((SortType == LST_Ascending) ?
								k_ELeaderboardSortMethodAscending :
								k_ELeaderboardSortMethodDescending);

			ELeaderboardDisplayType DisplayType = k_ELeaderboardDisplayTypeNumeric;

			if (DisplayFormat == LF_Seconds)
				DisplayType = k_ELeaderboardDisplayTypeTimeSeconds;
			else if (DisplayFormat == LF_Milliseconds)
				DisplayType = k_ELeaderboardDisplayTypeTimeMilliSeconds;


			// NOTE: Leaderboard creation is asynchronous, so this function may return True, yet still fail to create the leaderboard
			CallbackBridge->AddUserFindLeaderboard(GSteamUserStats->FindOrCreateLeaderboard(TCHAR_TO_UTF8(*LeaderboardName),
								SortMethod, DisplayType));
		}
	}
	else
	{
		debugf(TEXT("CreateLeaderboard: Leaderboard '%s' has already been created"), *LeaderboardName);
	}

	return bResult;
}


/**
 * Unlocks a gamer picture for the local user
 *
 * @param LocalUserNum the user to unlock the picture for
 * @param PictureId the id of the picture to unlock
 *
 * @return TRUE if the call worked, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::UnlockGamerPicture(BYTE LocalUserNum,INT PictureId)
{
	return FALSE;    // no gamer pictures on Steam.
}

/**
 * Displays the Steam Friends UI
 *
 * @param LocalUserNum the controller number of the user where are showing the friends for
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowFriendsUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// Show the friends UI for the specified controller num
		GSteamFriends->ActivateGameOverlay("Friends");
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowFriendsUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the Steam Friends Invite Request UI
 *
 * @param LocalUserNum the controller number of the user where are showing the friends for
 * @param UniqueId the id of the player being invited (null or 0 to have UI pick)
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowFriendsInviteUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// Show the friends UI for the specified controller num
		const CSteamID SteamPlayerID((uint64) UniqueId.Uid);
		GSteamFriends->ActivateGameOverlayToUser("steamid", SteamPlayerID);  // !!! FIXME: not exactly right.
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowFriendsInviteUI()"),
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
UBOOL UOnlineSubsystemSteamworks::ShowFeedbackUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// There's a "comment" field on the profile page. I'm not sure if this qualifies as "feedback" though.
		const CSteamID SteamPlayerID((uint64) UniqueId.Uid);
		GSteamFriends->ActivateGameOverlayToUser("steamid", SteamPlayerID);
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowFeedbackUI()"),
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
UBOOL UOnlineSubsystemSteamworks::ShowGamerCardUI(BYTE LocalUserNum,FUniqueNetId UniqueId)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// No "gamer card" but we can bring up the player's profile in the overlay.
		const CSteamID SteamPlayerID((uint64) UniqueId.Uid);
		GSteamFriends->ActivateGameOverlayToUser("steamid", SteamPlayerID);
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowGamerCardUI()"),
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
UBOOL UOnlineSubsystemSteamworks::ShowMessagesUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		// !!! FIXME: Does the community blotter count as "messages"?
		GSteamFriends->ActivateGameOverlay("Community");
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowMessagesUI()"),
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
UBOOL UOnlineSubsystemSteamworks::ShowAchievementsUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
#if 0  // This apparently is for the toast when you unlock an achievement, but Steam handles this itself.
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		GSteamFriends->ActivateGameOverlay("Achievements");
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowAchievementsUI()"),
			(DWORD)LocalUserNum);
	}
#endif
	return Result == ERROR_SUCCESS;
}
	

/**
 * Displays the players UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowPlayersUI(BYTE LocalUserNum)
{
	DWORD Result = E_FAIL;
	// Validate the user index passed in
	if (GSteamworksInitialized && (LocalUserNum == LoggedInPlayerNum))
	{
		GSteamFriends->ActivateGameOverlay("Players");
		Result = ERROR_SUCCESS;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Invalid player index (%d) specified to ShowPlayersUI()"),
			(DWORD)LocalUserNum);
	}
	return Result == ERROR_SUCCESS;
}

/**
 * Displays the invite ui
 *
 * @param LocalUserNum the local user sending the invite
 * @param InviteText the string to prefill the UI with
 */
UBOOL UOnlineSubsystemSteamworks::ShowInviteUI(BYTE LocalUserNum,const FString& InviteText)
{
	return FALSE;  // !!! FIXME: what to do with this?
}

/**
 * Displays the UI that shows the keyboard for inputing text
 *
 * @param LocalUserNum the controller number of the associated user
 * @param TitleText the title to display to the user
 * @param DescriptionText the text telling the user what to input
 * @param bIsPassword whether the item being entered is a password or not
 * @param bShouldValidate whether to apply the string validation API after input or not
 * @param DefaultText the default string to display
 * @param MaxResultLength the maximum length string expected to be filled in
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowKeyboardUI(BYTE LocalUserNum,
	const FString& TitleText,const FString& DescriptionText,UBOOL bIsPassword,
	UBOOL bShouldValidate,const FString& DefaultText,INT MaxResultLength)
{
	return FALSE;  // no keyboard UI on Steam. You already have a keyboard.   :)
}

/**
 * Shows a custom players UI for the specified list of players
 *
 * @param LocalUserNum the controller number of the associated user
 * @param Players the list of players to show in the custom UI
 * @param Title the title to use for the UI
 * @param Description the text to show at the top of the UI
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowCustomPlayersUI(BYTE LocalUserNum,const TArray<FUniqueNetId>& Players,const FString& Title,const FString& Description)
{
	return FALSE;  // !!! FIXME: nothing like this in Steam at the moment.
}

/**
 * Displays the marketplace UI for content
 *
 * @param LocalUserNum the local user viewing available content
 * @param CategoryMask the bitmask to use to filter content by type
 * @param OfferId a specific offer that you want shown
 */
UBOOL UOnlineSubsystemSteamworks::ShowContentMarketplaceUI(BYTE LocalUserNum,INT CategoryMask,INT OfferId)
{
	return FALSE;  // probably doesn't make sense on Steam (and Live for PC returns FALSE here, too!)
}
	
/**
 * Displays the marketplace UI for memberships
 *
 * @param LocalUserNum the local user viewing available memberships
 */
UBOOL UOnlineSubsystemSteamworks::ShowMembershipMarketplaceUI(BYTE LocalUserNum)
{
	return FALSE;  // probably doesn't make sense on Steam (and Live for PC returns FALSE here, too!)
}

/**
 * Displays the UI that allows the user to choose which device to save content to
 *
 * @param LocalUserNum the controller number of the associated user
 * @param SizeNeeded the size of the data to be saved in bytes
 * @param bForceShowUI true to always show the UI, false to only show the
 *		  UI if there are multiple valid choices
 * @param bManageStorage whether to allow the user to manage their storage or not
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
UBOOL UOnlineSubsystemSteamworks::ShowDeviceSelectionUI(BYTE LocalUserNum,INT SizeNeeded,UBOOL bManageStorage)
{
	return FALSE;   // this is what Live subsystem does for PC builds.
}

/**
 * Fetches the results of the device selection
 *
 * @param LocalUserNum the controller number of the associated user
 * @param DeviceName out param that gets a copy of the string
 *
 * @return the ID of the device that was selected
 */
INT UOnlineSubsystemSteamworks::GetDeviceSelectionResults(BYTE LocalUserNum,FString& DeviceName)
{
	// This is what Live subsystem does for non-console builds.
	DeviceName.Empty();
	return 0;
}

/**
 * Checks the device id to determine if it is still valid (could be removed)
 *
 * @param DeviceId the device to check
 *
 * @return TRUE if valid, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::IsDeviceValid(INT DeviceId,INT SizeNeeded)
{
	return FALSE;  // Live subsystem uses this for consoles, PC always returns FALSE.
}


/**
 * Resets the players stats (and achievements, if specified)
 *
 * @param bResetAchievements	If true, also resets player achievements
 * @return			TRUE if successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::ResetStats(UBOOL bResetAchievements)
{
	UBOOL bResult = FALSE;

#if FORBID_STEAM_RESET_STATS
	debugf(TEXT("Resetting of Steam user stats is disabled"));
#else
	debugf(TEXT("Resetting Steam user stats%s"), (bResetAchievements ? TEXT(" and achievements") : TEXT("")));

	if (GSteamUserStats->ResetAllStats(bResetAchievements == TRUE) && GSteamUserStats->StoreStats())
		bResult = TRUE;
#endif

	return bResult;
}


/**
 * Pops up the Steam toast dialog, notifying the player of their progress with an achievement (does not unlock achievements)
 *
 * @param AchievementId		The id of the achievment which will have its progress displayed
 * @param ProgressCount		The number of completed steps for this achievement
 * @param MaxProgress		The total number of required steps for this achievement, before it will be unlocked
 * @return			TRUE if successful, FALSE otherwise
 */
UBOOL UOnlineSubsystemSteamworks::DisplayAchievementProgress(INT AchievementId, INT ProgressCount, INT MaxProgress)
{
	UBOOL bResult = FALSE;

	if (GSteamworksInitialized)
	{
		FString AchievementIdString;

		if (AchievementMappings.Num() == 0)
		{
			AchievementIdString = FString::Printf(TEXT("Achievement_%u"), AchievementId);
		}
		else
		{
			UBOOL bFound = FALSE;

			for (INT i=0; i<AchievementMappings.Num(); i++)
			{
				if (AchievementMappings(i).AchievementId == AchievementId)
				{
					AchievementIdString = AchievementMappings(i).AchievementName.GetNameString();

					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
			{
				debugf(TEXT("DisplayAchievementProgress: AchievementId '%i' not found in AchievementMappings list"), AchievementId);
				return FALSE;
			}
		}

		if (GSteamUserStats->IndicateAchievementProgress(TCHAR_TO_ANSI(*AchievementIdString), ProgressCount, MaxProgress))
			bResult = TRUE;
	}

	return bResult;
}


/**
 * Converts the specified UID, into the players Steam Community name
 *
 * @param UID		The players UID
 * @result		The username of the player, as stored on the Steam backend
 */
FString UOnlineSubsystemSteamworks::UniqueNetIdToPlayerName(const FUniqueNetId& UID)
{
	FString Result = TEXT("");

	if (GSteamFriends != NULL)
	{
		const char* PlayerName = GSteamFriends->GetFriendPersonaName(CSteamID((uint64)UID.Uid));
		Result = FString(UTF8_TO_TCHAR(PlayerName));
	}

	return Result;
}


#endif	//#if WITH_UE3_NETWORKING

