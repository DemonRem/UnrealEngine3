/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemSteamworks.h"

#if WITH_UE3_NETWORKING && WITH_STEAMWORKS

#ifndef MAX_QUERY_MSEC
#define MAX_QUERY_MSEC 9999
#endif

// Glue to bridge between Steamworks callback class and Unreal class, since multiple inheritance from arbitrary C++ classes in unrealscript is complicated.
class SteamServerListResponse : public ISteamMatchmakingServerListResponse
{
public:
	SteamServerListResponse(UOnlineGameInterfaceSteamworks *_GameInterface) : GameInterface(_GameInterface) {}
	virtual void ServerResponded(HServerListRequest Request, int iServer) { GameInterface->ServerResponded(iServer); }
	virtual void ServerFailedToRespond(HServerListRequest Request, int iServer) { GameInterface->ServerFailedToRespond(iServer); }
	virtual void RefreshComplete(HServerListRequest Request, EMatchMakingServerResponse Response) { GameInterface->RefreshComplete(Response); }
private:
	UOnlineGameInterfaceSteamworks *GameInterface;
};

class SteamRulesResponse : public ISteamMatchmakingRulesResponse
{
public:
	SteamRulesResponse(UOnlineGameInterfaceSteamworks *_GameInterface, UOnlineGameSettings *_Settings, const DWORD _IpAddr, const INT _Port) : GameInterface(_GameInterface), Settings(_Settings), IpAddr(_IpAddr), Port(_Port), bDeleteMe(FALSE) {}
	virtual void RulesResponded( const char *pchRule, const char *pchValue ) { GameInterface->RulesResponded(this, pchRule, pchValue); }
	virtual void RulesFailedToRespond() { GameInterface->RulesFailedToRespond(this); }
	virtual void RulesRefreshComplete() { GameInterface->RulesRefreshComplete(this); }
	UOnlineGameSettings *GetOnlineGameSettings() const { return Settings; }
	SteamRulesMap &GetRules() { return Rules; }
	DWORD GetIpAddr() const { return IpAddr; }
	DWORD GetPort() const { return Port; }
	UBOOL bDeleteMe;  // flag for deletion, since we can't delete it during the callback.

private:
	UOnlineGameInterfaceSteamworks *GameInterface;
	UOnlineGameSettings *Settings;
	SteamRulesMap Rules;
	const DWORD IpAddr;
	const INT Port;
};

class SteamPingResponse : public ISteamMatchmakingPingResponse
{
public:
	SteamPingResponse(UOnlineGameInterfaceSteamworks *_GameInterface, UOnlineGameSettings *_Settings) : GameInterface(_GameInterface), Settings(_Settings), bDeleteMe(FALSE) {}
	virtual void ServerResponded(gameserveritem_t &server) { GameInterface->PingServerResponded(this, server); }
	virtual void ServerFailedToRespond() { GameInterface->PingServerFailedToRespond(this); }
	UOnlineGameSettings *GetOnlineGameSettings() const { return Settings; }
	UBOOL bDeleteMe;  // flag for deletion, since we can't delete it during the callback.

private:
	UOnlineGameInterfaceSteamworks *GameInterface;
	UOnlineGameSettings *Settings;
};


UBOOL UOnlineGameInterfaceSteamworks::StartSteamLanBeacon()
{
	UBOOL Return = FALSE;

	if ( GSteamGameServer == NULL)
	{
		const EServerMode ServerMode = eServerModeNoAuthentication;  // No auth == LAN game, to Steam.
		if (!PublishSteamServer(ServerMode))
		{
			return FALSE;
		}
	}

	return TRUE;
}

void UOnlineGameInterfaceSteamworks::StopSteamLanBeacon()
{
	if ( GSteamMasterServerUpdater != NULL)
	{
		GSteamMasterServerUpdater->SetActive(false);
		GSteamMasterServerUpdater->NotifyShutdown();
		GSteamMasterServerUpdater = NULL;
	}

	if ( GSteamGameServer != NULL)
	{
		SteamGameServer_Shutdown();
		GSteamGameServer = NULL;
	}

#if HAVE_STEAM_GAMESERVER_STATS
	GSteamGameServerStats = NULL;
#endif
}

/**
 * Marks an online game as in progress (as opposed to being in lobby)
 *
 * @param SessionName the name of the session that is being started
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::StartOnlineGame(FName SessionName)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
	{
		DWORD Return = E_FAIL;
		if (GameSettings != NULL && SessionInfo != NULL)
		{
			UBOOL bMustAuthListenServerClient = FALSE;
			AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
			if (WorldInfo)
			{
				bMustAuthListenServerClient = (WorldInfo->NetMode == NM_ListenServer);
			}

			// Lan matches don't report starting to external services
			if (GameSettings->bIsLanMatch == FALSE)
			{
				// Can't start a match multiple times
				if (CurrentGameState == OGS_Pending || CurrentGameState == OGS_Ended)
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
					bMustAuthListenServerClient = FALSE;
					StopSteamLanBeacon();
				}
				Return = S_OK;
			}

			if (bMustAuthListenServerClient)
			{
				UOnlineSubsystemSteamworks* OnlineSub = CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem);
				BYTE AuthBlob[2048];
				const uint16 Port = (uint16) SessionInfo->HostAddr.GetPort();
				DWORD Int32Addr = 0;
				SessionInfo->HostAddr.GetIp(Int32Addr);
				const int BlobLen = GSteamUser->InitiateGameConnection(AuthBlob, sizeof (AuthBlob),
							CSteamID((uint64)OnlineSub->LoggedInPlayerId.Uid), Int32Addr, Port,
							GameSettings->bAntiCheatProtected != 0);

				OnlineSub->bListenHostAuthenticated = FALSE;


				// Kick off the auth request
				UBOOL bAuthFailure = FALSE;
				TCHAR* FailReason = TEXT("");
				TCHAR* FailExtra = TEXT("");

				// BlobLen of 0 signifies failure
				if (BlobLen != 0)
				{
					if (GSteamGameServer != NULL)
					{
						CSteamID SteamId;

						// Mark the listen host as pending authentication, regardless of whether the call succeeedsm
						// so that authentication can be retried later if it fails
						GSteamGameServer->SendUserConnectAndAuthenticate(Int32Addr, AuthBlob, BlobLen, &SteamId);

						AWorldInfo* WI = GWorld->GetWorldInfo();

						OnlineSub->bListenHostPendingAuth = TRUE;

						if (WI != NULL)
							OnlineSub->ListenAuthTimestamp = WI->RealTimeSeconds;

						OnlineSub->ListenAuthRetryCount = 0;
					}
					else
					{
						bAuthFailure = TRUE;
						FailReason = TEXT("Internal Failure");
						FailExtra = TEXT("GSteamGameServer == NULL");
					}
				}
				else
				{
					bAuthFailure = TRUE;
					FailReason = TEXT("API Failure");
					FailExtra = TEXT("InitiateGameConnection");
				}


				// If authentication failed, log the fail reason
				if (bAuthFailure)
				{
					debugf(TEXT("Failed Steam auth for listen host (%s : %s), stats updates may fail for this player"),
						FailReason, FailExtra);
				}
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't start an online game that hasn't been created"));
		}
		// Update the game state if successful
		if (Return == S_OK || Return == ERROR_IO_PENDING)
		{
			CurrentGameState = OGS_InProgress;
		}
		if (Return != ERROR_IO_PENDING)
		{
			// Indicate that the start completed
			FAsyncTaskDelegateResultsNamedSession Params(SessionName,Return);
			TriggerOnlineDelegates(this,StartOnlineGameCompleteDelegates,&Params);
		}
		Result = Return == S_OK || Return == ERROR_IO_PENDING;
	}
	else
	{
		Result = Super::StartOnlineGame( SessionName );
	}

	return( Result );
}

/**
 * Marks an online game as having been ended
 *
 * @param SessionName the name of the session being ended
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::EndOnlineGame(FName SessionName)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
	{
		DWORD Return = E_FAIL;
		if (GameSettings != NULL && SessionInfo != NULL)
		{
			// JohnB: Removed code for terminating listen host client connection, as clients usually stay connected on server travel

			if (GameSettings->bIsLanMatch == FALSE)
			{
				// Can't end a match that isn't in progress
				if (CurrentGameState == OGS_InProgress)
				{
					Return = EndInternetGame();
				}
				else
				{
					debugf(NAME_DevOnline,TEXT("Can't end an online game in state %d"),CurrentGameState);
				}
			}
			else
			{
				Return = S_OK;
				// If the session should be advertised and the lan beacon was destroyed, recreate
				if ((GameSettings->bShouldAdvertise) && (GameSettings->bAllowJoinInProgress == FALSE))
				{
					// Recreate the beacon
					Return = StartSteamLanBeacon() ? S_OK : E_FAIL;
				}
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't end an online game that hasn't been created"));
		}
		if (Return == ERROR_IO_PENDING)
		{
			CurrentGameState = OGS_Ending;
		}
		// Fire delegates off if not async or in error
		if (Return != ERROR_IO_PENDING)
		{
			// Just trigger the delegate as having failed
			FAsyncTaskDelegateResultsNamedSession Params(SessionName,Return);
			TriggerOnlineDelegates(this,EndOnlineGameCompleteDelegates,&Params);
			CurrentGameState = OGS_Ended;
		}
		Result = ( Return == S_OK || Return == ERROR_IO_PENDING );
	}
	else
	{
		Result = Super::EndOnlineGame( SessionName );
	}

	return( Result );
}

/**
 * Determines if a settings object's UProperty should be advertised via Steamworks.
 *
 * @param PropertyName the property to check
 */
static inline UBOOL ShouldAdvertiseUProperty(FName PropertyName)
{
	if (FName(TEXT("NumPublicConnections"),FNAME_Find) == PropertyName)
	{
		return TRUE;
	}
	if (FName(TEXT("bUsesStats"),FNAME_Find) == PropertyName)
	{
		return TRUE;
	}
	if (FName(TEXT("bIsDedicated"),FNAME_Find) == PropertyName)
	{
		return TRUE;
	}
	if (FName(TEXT("OwningPlayerName"),FNAME_Find) == PropertyName)
	{
		return TRUE;
	}
	return FALSE;
}

void UOnlineGameInterfaceSteamworks::CancelAllQueries()
{
	if (GSteamMatchmakingServers == NULL)
	{
		return;  // we never initialized, so return immediately.
	}

	// stop any pending queries, so we don't callback into a destroyed object.
	if (CurrentMatchmakingQuery)
	{
		GSteamMatchmakingServers->CancelQuery(CurrentMatchmakingQuery);
		GSteamMatchmakingServers->ReleaseRequest(CurrentMatchmakingQuery);
		CurrentMatchmakingType = SMT_Invalid;
		CurrentMatchmakingQuery = 0;
	}

	delete ServerListResponse;
	ServerListResponse = NULL;

	for (INT Index = 0; Index < QueryToRulesResponseMap.Num(); Index++)
	{
		HServerQuery Query = (HServerQuery) QueryToRulesResponseMap(Index).Query;
		SteamRulesResponse *Response = QueryToRulesResponseMap(Index).Response;
		GSteamMatchmakingServers->CancelServerQuery(Query);
		delete Response;
	}
	QueryToRulesResponseMap.Empty();

	for (INT Index = 0; Index < QueryToPingResponseMap.Num(); Index++)
	{
		HServerQuery Query = (HServerQuery) QueryToPingResponseMap(Index).Query;
		SteamPingResponse *Response = QueryToPingResponseMap(Index).Response;
		GSteamMatchmakingServers->CancelServerQuery(Query);
		delete Response;
	}
	QueryToPingResponseMap.Empty();

	if (GameSearch)
	{
		GameSearch->bIsSearchInProgress = FALSE;
	}
}

void UOnlineGameInterfaceSteamworks::FinishDestroy()
{
	CancelAllQueries();
	Super::FinishDestroy();
}

/**
 * Starts process to add a Server to the search results.
 *
 * The server isn't actually added here, since we still need to obtain metadata about it, but
 * this method kicks that off.
 *
 * @param Server the server object to convert to UE3's format
 */
void UOnlineGameInterfaceSteamworks::AddServerToSearchResults(const gameserveritem_t *Server, const INT SteamIndex)
{
	check(Server);

	if (GameSearch->Results.Num() >= GameSearch->MaxSearchResults)
	{
		debugf(NAME_DevOnline, TEXT("Got enough search results, stopping query."));
		if (CurrentMatchmakingQuery)
		{
			GSteamMatchmakingServers->CancelQuery(CurrentMatchmakingQuery);   // no more results.
			GSteamMatchmakingServers->ReleaseRequest(CurrentMatchmakingQuery);   // no more results.
			CurrentMatchmakingQuery = 0;
		}
		return;
	}

	if (Server->m_bDoNotRefresh)
	{
		return;
	}

	if (Server->m_nAppID != GSteamAppID)
	{
		return;  // not this game?
	}

	const uint32 IpAddr = Server->m_NetAdr.GetIP();
	const uint16 ConnectionPort = Server->m_NetAdr.GetConnectionPort();
	const uint16 QueryPort = Server->m_NetAdr.GetQueryPort();

	// Make sure we haven't processed it
	// !!! FIXME: this probably isn't necessary.
	for (INT i = 0; i < GameSearch->Results.Num(); i++)
	{
		const FOnlineGameSearchResult& Result = GameSearch->Results(i);
		const FInternetIpAddr &HostAddr = ((FSessionInfo *) Result.PlatformData)->HostAddr;
		DWORD Addr;
		HostAddr.GetIp(Addr);
		if ((Addr == IpAddr) && (HostAddr.GetPort() == ConnectionPort))
		{
			return;
		}
	}

	// Create an object that we'll copy the data to
	UOnlineGameSettings* NewServer = ConstructObject<UOnlineGameSettings>(GameSearch->GameSettingsClass);
	if (NewServer != NULL)
	{
		NewServer->PingInMs = Clamp(Server->m_nPing,0,MAX_QUERY_MSEC);
		NewServer->bAntiCheatProtected = Server->m_bSecure ? 1 : 0;
		NewServer->NumPublicConnections = Server->m_nMaxPlayers;
		NewServer->NumOpenPublicConnections = Server->m_nMaxPlayers - Server->m_nPlayers;
		NewServer->bIsLanMatch = (CurrentMatchmakingType == SMT_LAN);
		NewServer->OwningPlayerId.Uid = (QWORD) Server->m_steamID.ConvertToUint64();
		NewServer->OwningPlayerName = UTF8_TO_TCHAR(Server->GetName());

		const TCHAR *ServerTypeString = (CurrentMatchmakingType == SMT_LAN) ? TEXT("LAN") : TEXT("Internet");
		debugf(NAME_DevOnline,TEXT("Querying rules data for %s server %s"), ServerTypeString, ANSI_TO_TCHAR(Server->m_NetAdr.GetQueryAddressString()));

		SteamRulesResponse *RulesResponse = new SteamRulesResponse(this, NewServer, IpAddr, ConnectionPort);
		const HServerQuery RulesQuery = GSteamMatchmakingServers->ServerRules(IpAddr, QueryPort, RulesResponse);
		const INT MapPosition = QueryToRulesResponseMap.AddZeroed();
		FServerQueryToRulesResponseMapping &RulesMapping = QueryToRulesResponseMap(MapPosition);
		RulesMapping.Query = (INT) RulesQuery;
		RulesMapping.Response = RulesResponse;
		// we now wait for RulesResponse to fire callbacks to continue processing this server.
	}
	else
	{
		debugf(NAME_Error,TEXT("Failed to create new online game settings object"));
	}
}

/**
 * Updates the server details with the new data
 *
 * @param GameSettings the game settings to update
 * @param Server the Steamworks data to update with
 */
void UOnlineGameInterfaceSteamworks::UpdateGameSettingsData(UOnlineGameSettings* GameSettings, const SteamRulesMap &Rules)
{
	if (GameSettings)
	{
		// Read the owning player id
		const FString *PlayerIdValue = Rules.Find(TEXT("OwningPlayerId"));
		if (PlayerIdValue != NULL)
		{
			GameSettings->OwningPlayerId.Uid = appAtoi64(*(*PlayerIdValue));
		}
		// Read the data bindable properties
		for (UProperty* Property = GameSettings->GetClass()->PropertyLink;
			Property != NULL;
			Property = Property->PropertyLinkNext)
		{
			// If the property is databindable and is not an object, we'll check for it
			if ((Property->PropertyFlags & CPF_DataBinding) != 0 &&
				Cast<UObjectProperty>(Property,CLASS_IsAUObjectProperty) == NULL)
			{
				BYTE* ValueAddress = (BYTE*)GameSettings + Property->Offset;
				const FString &DataBindableName = Property->GetName();
				const FString *DataBindableValue = Rules.Find(DataBindableName);
				if (DataBindableValue != NULL)
				{
					Property->ImportText(*(*DataBindableValue),ValueAddress,PPF_Localized,GameSettings);
				}
			}
		}
		// Read the settings
		for (INT Index = 0; Index < GameSettings->LocalizedSettings.Num(); Index++)
		{
			INT SettingId = GameSettings->LocalizedSettings(Index).Id;
			FString SettingName(TEXT("s"));
			SettingName += appItoa(SettingId);
			const FString *SettingValue = Rules.Find(SettingName);
			if (SettingValue != NULL)
			{
				GameSettings->LocalizedSettings(Index).ValueIndex = appAtoi(*(*SettingValue));
			}
		}
		// Read the properties
		for (INT Index = 0; Index < GameSettings->Properties.Num(); Index++)
		{
			INT PropertyId = GameSettings->Properties(Index).PropertyId;
			FString PropertyName(TEXT("p"));
			PropertyName += appItoa(PropertyId);
			const FString *PropertyValue = Rules.Find(PropertyName);
			if (PropertyValue != NULL)
			{
				GameSettings->Properties(Index).Data.FromString(*PropertyValue);
			}
		}
	}
}

/**
 * Marks a server in the server list as unreachable
 *
 * @param Addr the IP addr of the server to update
 */
void UOnlineGameInterfaceSteamworks::MarkServerAsUnreachable(const FInternetIpAddr& Addr)
{
	if (GameSearch)
	{
		// Search the results for the server that needs updating
		for (INT Index = 0; Index < GameSearch->Results.Num(); Index++)
		{
			FOnlineGameSearchResult& Result = GameSearch->Results(Index);
			if (Result.PlatformData)
			{
				FSessionInfo* SessInfo = (FSessionInfo*)Result.PlatformData;
				// If the IPs match, then this is the right one
				if (SessInfo->HostAddr == Addr)
				{
					Result.GameSettings->PingInMs = MAX_QUERY_MSEC;
					break;
				}
			}
		}
	}
}

/**
 * Builds a filter string based on the current GameSearch
 *
 * @param Filter the filter string will be placed here
 */
#if 0
void UOnlineGameInterfaceGameSpy::BuildFilter(FString& Filter)
{
	// Cache the number of properties and settings
	INT NumProperties = GameSearch->Properties.Num();
	INT NumSettings = GameSearch->LocalizedSettings.Num();

	// Loop through the clauses
	INT NumClauses = GameSearch->FilterQuery.OrClauses.Num();
	UBOOL bIsFirstClause = TRUE;
	for(INT ClauseIndex = 0 ; ClauseIndex < NumClauses ; ClauseIndex++)
	{
		FOnlineGameSearchORClause* Clause = &GameSearch->FilterQuery.OrClauses(ClauseIndex);
		INT NumSearchParameters = Clause->OrParams.Num();

		// Loop through the params
		FString ClauseFilter;
		UBOOL bIsFirstParameter = TRUE;
		for(INT ParameterIndex = 0 ; ParameterIndex < NumSearchParameters ; ParameterIndex++)
		{
			FOnlineGameSearchParameter* Parameter = &Clause->OrParams(ParameterIndex);

			FString ParameterName;
			FString ParameterValue;
			// Determine the parameter type so we can get it from the right place
			switch (Parameter->EntryType)
			{
				case OGSET_Property:
				{
					ParameterName = TEXT("p");
					ParameterName += appItoa(Parameter->EntryId);

					FSettingsProperty* Property = NULL;
					for(INT Index = 0 ; Index < NumProperties ; Index++)
					{
						if(GameSearch->Properties(Index).PropertyId == Parameter->EntryId)
						{
							Property = &GameSearch->Properties(Index);
							break;
						}
					}
					if(Property == NULL)
					{
						continue;
					}
					UBOOL bNeedsQuotes = (Property->Data.Type == SDT_String);
					if(bNeedsQuotes)
					{
						ParameterValue += TEXT("'");
					}
					ParameterValue += Property->Data.ToString();
					if(bNeedsQuotes)
					{
						ParameterValue += TEXT("'");
					}
					break;
				}
				case OGSET_LocalizedSetting:
				{
					if(GameSearch->IsWildcardStringSetting(Parameter->EntryId))
					{
						continue;
					}

					ParameterName = TEXT("s");
					ParameterName += appItoa(Parameter->EntryId);

					FLocalizedStringSetting* Setting = NULL;
					for(INT Index = 0 ; Index < NumSettings ; Index++)
					{
						if(GameSearch->LocalizedSettings(Index).Id == Parameter->EntryId)
						{
							Setting = &GameSearch->LocalizedSettings(Index);
							break;
						}
					}
					if(Setting == NULL)
					{
		"map"		- map the server is running, as set in the dedicated server api
		"dedicated" - reports bDedicated from the API
		"secure"	- VAC-enabled
		"full"		- not full
		"empty"		- not empty
		"noplayers" - is empty
		"proxy"		- a relay server
						continue;
					}
					ParameterValue = appItoa(Setting->ValueIndex);
					break;
				}
				case OGSET_ObjectProperty:
				{
					ParameterName = Parameter->ObjectPropertyName.ToString();
					// Search through the properties so we can find the corresponding value
					for (INT Index = 0; Index < GameSearch->NamedProperties.Num(); Index++)
					{
						if (GameSearch->NamedProperties(Index).ObjectPropertyName == Parameter->ObjectPropertyName)
						{
							ParameterValue = GameSearch->NamedProperties(Index).ObjectPropertyValue;
							break;
						}
					}
					break;
				}
			}

			FString ParameterOperator;
			switch(Parameter->ComparisonType)
			{
			case OGSCT_Equals:
				ParameterOperator = TEXT("=");
				break;
			case OGSCT_NotEquals:
				ParameterOperator = TEXT("!=");
				break;
			case OGSCT_GreaterThan:
				ParameterOperator = TEXT(">");
				break;
			case OGSCT_GreaterThanEquals:
				ParameterOperator = TEXT(">=");
				break;
			case OGSCT_LessThan:
				ParameterOperator = TEXT("<");
				break;
			case OGSCT_LessThanEquals:
				ParameterOperator = TEXT("<=");
				break;
			default:
				//todo: error
				break;
			}

			if(bIsFirstParameter)
			{
				bIsFirstParameter = FALSE;
			}
			else
			{
				ClauseFilter += TEXT("OR");
			}
			ClauseFilter += TEXT("(");
			ClauseFilter += ParameterName;
			ClauseFilter += ParameterOperator;
			ClauseFilter += ParameterValue;
			ClauseFilter += TEXT(")");
		}

		if(ClauseFilter.Len() == 0)
		{
			continue;
		}

		if(bIsFirstClause)
		{
			bIsFirstClause = FALSE;
		}
		else
		{
			Filter += TEXT("AND");
		}
		Filter += TEXT("(");
		Filter += ClauseFilter;
		Filter += TEXT(")");
	}
	// Wrap with the additional if present
	if (GameSearch->AdditionalSearchCriteria.Len())
	{
		if ( Filter.Len() > 0 )
		{
			FString AdditionalSearch(TEXT("("));
			AdditionalSearch += Filter;
			AdditionalSearch += TEXT("AND");
			AdditionalSearch += TEXT("(");
			AdditionalSearch += GameSearch->AdditionalSearchCriteria;
			AdditionalSearch += TEXT(")");
			AdditionalSearch += TEXT(")");

			Filter = AdditionalSearch;
		}
		else
		{
			Filter = GameSearch->AdditionalSearchCriteria;
		}
	}
	gsdebugf(NAME_DevOnline,TEXT("Query filter is '%s'"),*Filter);
}
#endif


void UOnlineGameInterfaceSteamworks::ServerResponded(int iServer)
{
	if ((CurrentMatchmakingType == SMT_Invalid) || (CurrentMatchmakingQuery == 0))
	{
		debugf(NAME_DevOnline, TEXT("ServerResponded() callback without current matchmaking type!"));
		return;
	}

	debugf(NAME_DevOnline,TEXT("Got ServerResponded(%d) callback"), iServer);
	const gameserveritem_t *Server = GSteamMatchmakingServers->GetServerDetails(CurrentMatchmakingQuery, iServer);
	AddServerToSearchResults(Server, iServer);
}

void UOnlineGameInterfaceSteamworks::ServerFailedToRespond(int iServer)
{
	debugf(NAME_DevOnline,TEXT("Got ServerFailedToRespond(%d) callback"), iServer);
	// just dump the server by doing nothing.
}

void UOnlineGameInterfaceSteamworks::RefreshComplete(EMatchMakingServerResponse Response)
{
	if ((CurrentMatchmakingType == SMT_Invalid) || (CurrentMatchmakingQuery == 0))
	{
		debugf(NAME_DevOnline, TEXT("RefreshComplete() callback without current matchmaking type!"));
		return;
	}

	debugf(NAME_DevOnline,TEXT("Got RefreshComplete() callback"));
	CleanupServerBrowserQuery(FALSE);

	// we'll signal the app about being complete in Tick(), since this callback only means we've got the
	//  whole server list. We may still be waiting on metadata queries for some of them.
}

void UOnlineGameInterfaceSteamworks::CancelSteamRulesQuery(SteamRulesResponse *RulesResponse, const UBOOL bCancel)
{
	for (INT Index = 0; Index < QueryToRulesResponseMap.Num(); Index++)
	{
		if (QueryToRulesResponseMap(Index).Response == RulesResponse)
		{
			if (bCancel)
			{
				HServerQuery Query = (HServerQuery) QueryToRulesResponseMap(Index).Query;
				GSteamMatchmakingServers->CancelServerQuery(Query);
			}
			QueryToRulesResponseMap.Remove(Index, 1);
			return;
		}
	}
}

void UOnlineGameInterfaceSteamworks::CancelSteamPingQuery(SteamPingResponse *PingResponse, const UBOOL bCancel)
{
	for (INT Index = 0; Index < QueryToPingResponseMap.Num(); Index++)
	{
		if (QueryToPingResponseMap(Index).Response == PingResponse)
		{
			if (bCancel)
			{
				HServerQuery Query = (HServerQuery) QueryToPingResponseMap(Index).Query;
				GSteamMatchmakingServers->CancelServerQuery(Query);
			}
			QueryToPingResponseMap.Remove(Index, 1);
			return;
		}
	}
}

void UOnlineGameInterfaceSteamworks::RulesResponded(SteamRulesResponse *RulesResponse, const char *pchRule, const char *pchValue)
{
	const FString Key(UTF8_TO_TCHAR(pchRule));
	const FString Value(UTF8_TO_TCHAR(pchValue));
	debugf(NAME_DevOnline,TEXT("Got RulesResponded('%s', '%s') callback"), *Key, *Value);

	if (!RulesResponse->bDeleteMe)
	{
		// Don't list this server if it's for a different version of the engine.
		// Otherwise, UDK servers for different releases will not work. Licensees
		// that are conforming their packages between releases may want to change this.
		if (Key == TEXT("SteamEngineVersion"))
		{
			if (appAtoi64(*Value) != GEngineVersion)
			{
				debugf(NAME_DevOnline,TEXT("This server is for a different engine version (%s), rejecting."), *Value);
				RulesResponse->bDeleteMe = TRUE;
			}
			return;  // don't add this rule, ever. It's only meant to eliminate incompatible servers at this level.
		}
		RulesResponse->GetRules().Set(Key, Value);
	}
}

void UOnlineGameInterfaceSteamworks::RulesFailedToRespond(SteamRulesResponse *RulesResponse)
{
	debugf(NAME_DevOnline,TEXT("Got RulesFailedToRespond() callback"));
	RulesResponse->bDeleteMe = TRUE;  // ...and throw this server away.
}

void UOnlineGameInterfaceSteamworks::RulesRefreshComplete(SteamRulesResponse *RulesResponse)
{
	debugf(NAME_DevOnline,TEXT("Got RulesRefreshComplete() callback"));

	if (!RulesResponse->bDeleteMe)
	{
		UOnlineGameSettings* GameSettings = RulesResponse->GetOnlineGameSettings();
		if (GameSettings)
		{
			// Finish filling in the game settings...
			UpdateGameSettingsData(GameSettings, RulesResponse->GetRules());

			// ...and finally add it to the search results.
			if (GameSearch)
			{
				FSessionInfo* SessInfo = CreateSessionInfo();
				SessInfo->HostAddr.SetIp(RulesResponse->GetIpAddr());
				SessInfo->HostAddr.SetPort(RulesResponse->GetPort());
				const INT Position = GameSearch->Results.AddZeroed();
				FOnlineGameSearchResult& Result = GameSearch->Results(Position);
				Result.PlatformData = SessInfo;
				Result.GameSettings = GameSettings;
			}

			// GameSpy calls this for every server update (and everything ignores the result).
			FAsyncTaskDelegateResults Param(S_OK);
			TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Param);
		}

		RulesResponse->bDeleteMe = TRUE;  // done with this!
	}
}

void UOnlineGameInterfaceSteamworks::UpdatePing(SteamPingResponse *PingResponse, const INT Ping)
{
	CancelSteamPingQuery(PingResponse, FALSE);  // Take this out of the array of running queries.

	UOnlineGameSettings* GameSettings = PingResponse->GetOnlineGameSettings();
	if (GameSettings)
	{
		GameSettings->PingInMs = Clamp(Ping, 0, MAX_QUERY_MSEC);
		// GameSpy calls this for every server update (and everything ignores the result).
		FAsyncTaskDelegateResults Param(S_OK);
		TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Param);
	}

	PingResponse->bDeleteMe = TRUE;  // done with this!
}

void UOnlineGameInterfaceSteamworks::PingServerResponded(SteamPingResponse *PingResponse, const gameserveritem_t &Server)
{
	debugf(NAME_DevOnline,TEXT("Got PingServerResponded() callback"));
	UpdatePing(PingResponse, Server.m_nPing);
}

void UOnlineGameInterfaceSteamworks::PingServerFailedToRespond(SteamPingResponse *PingResponse)
{
	debugf(NAME_DevOnline,TEXT("Got PingServerFailedToRespond() callback"));
	UpdatePing(PingResponse, MAX_QUERY_MSEC);
}

// !!! FIXME: we should probably just make CreateInternetGame and CreateLanGame virtual in the superclass.
/**
 * Searches for games matching the settings specified
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired settings that the returned sessions will have
 *
 * @return true if successful searching for sessions, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::FindOnlineGames(BYTE SearchingPlayerNum,UOnlineGameSearch* SearchSettings)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
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
				// Free up previous results, if present
				if (SearchSettings->Results.Num())
				{
					FreeSearchResults(SearchSettings);
				}
				GameSearch = SearchSettings;
				// Check for master server or lan query
				if (SearchSettings->bIsLanQuery == FALSE)
				{
					Return = FindInternetGames();
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
		if (Return != ERROR_IO_PENDING)
		{
			// Fire the delegate off immediately
			FAsyncTaskDelegateResults Params(Return);
			TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Params);
		}
		Result = ( Return == S_OK || Return == ERROR_IO_PENDING );
	}
	else
	{
		Result = Super::FindOnlineGames( SearchingPlayerNum, SearchSettings );
	}

	return( Result );
}


/**
 * Builds a Steamworks query and submits it to the Steamworks backend
 *
 * @return an Error/success code
 */
DWORD UOnlineGameInterfaceSteamworks::FindInternetGames()
{
	DWORD Return = E_FAIL;
	// Don't try to search if the network device is broken
	if (GSocketSubsystem->HasNetworkDevice())
	{
		// Make sure they are logged in to play online
		if (CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem)->LoggedInStatus == LS_LoggedIn)
		{
			debugf(NAME_DevOnline,TEXT("Starting search for Internet games..."));
			MatchMakingKeyValuePair_t Pairs[16];
			MatchMakingKeyValuePair_t *Filters[16];
			uint32 FilterCount = 0;

			for (INT i = 0; i < ARRAY_COUNT(Filters); i++)
			{
				Filters[i] = &Pairs[i];
			}

			CancelAllQueries();
			ServerListResponse = new SteamServerListResponse(this);
			CurrentMatchmakingType = SMT_Internet;

			//!!! FIXME Filters
			GameSearch->bIsSearchInProgress = TRUE;
			CurrentMatchmakingQuery = GSteamMatchmakingServers->RequestInternetServerList(GSteamAppID, Filters, FilterCount, ServerListResponse);
			Return = ERROR_IO_PENDING;
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("You must be logged in to an online profile to search for internet games"));
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't search for an internet game without a network connection"));
	}
	return Return;
}

DWORD UOnlineGameInterfaceSteamworks::FindLanGames()
{
	DWORD Return = E_FAIL;

	if( GSteamworksInitialized )
	{
		// Don't try to search if the network device is broken
		if (GSocketSubsystem->HasNetworkDevice())
		{
			// Make sure they are logged in to play online
			if (CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem)->LoggedInStatus == LS_LoggedIn)
			{
				debugf(NAME_DevOnline,TEXT("Starting search for LAN games..."));

				CancelAllQueries();
				ServerListResponse = new SteamServerListResponse(this);
				CurrentMatchmakingType = SMT_LAN;

				//!!! FIXME Filters
				GameSearch->bIsSearchInProgress = TRUE;
				CurrentMatchmakingQuery = GSteamMatchmakingServers->RequestLANServerList(GSteamAppID, ServerListResponse);
				Return = ERROR_IO_PENDING;
			}
			else
			{
				debugf(NAME_DevOnline,TEXT("You must be logged in to an online profile to search for LAN games"));
			}
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Can't search for a LAN game without a network connection"));
		}
	}
	else
	{
		Return = Super::FindLanGames();
	}

	return Return;
}


// !!! FIXME: should probably just make CancelFind*Games() virtual in the superclass.
/**
 * Cancels the current search in progress if possible for that search type
 *
 * @return true if successful searching for sessions, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::CancelFindOnlineGames()
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
	{
		DWORD Return = E_FAIL;
		if (GameSearch != NULL &&
			GameSearch->bIsSearchInProgress)
		{
			// Lan and internet are handled differently
			if (GameSearch->bIsLanQuery)
			{
				Return = CancelFindLanGames();
			}
			else
			{
				Return = CancelFindInternetGames();
			}
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Can't cancel a search that isn't in progress"));
		}
		if (Return != ERROR_IO_PENDING)
		{
			FAsyncTaskDelegateResults Results(Return);
			TriggerOnlineDelegates(this,CancelFindOnlineGamesCompleteDelegates,&Results);
		}
		Result = Return == S_OK || Return == ERROR_IO_PENDING;
	}
	else
	{
		Result =  Super::CancelFindOnlineGames();
	}

	return( Result );
}

/**
 * Attempts to cancel an internet game search
 *
 * @return an error/success code
 */
DWORD UOnlineGameInterfaceSteamworks::CancelFindInternetGames(void)
{
	debugf(NAME_DevOnline,TEXT("Canceling internet game search"));
	CleanupServerBrowserQuery(TRUE);
	return S_OK;
}

/**
 * Attempts to cancel a LAN game search
 *
 * @return an error/success code
 */
DWORD UOnlineGameInterfaceSteamworks::CancelFindLanGames(void)
{
	debugf(NAME_DevOnline,TEXT("Canceling LAN game search"));
	CleanupServerBrowserQuery(TRUE);
	return S_OK;
}

/** Frees the current server browser query and marks the search as done */
void UOnlineGameInterfaceSteamworks::CleanupServerBrowserQuery(const UBOOL bCancel)
{
	if (bCancel && (CurrentMatchmakingQuery != 0))
	{
		GSteamMatchmakingServers->CancelQuery(CurrentMatchmakingQuery);
		GSteamMatchmakingServers->ReleaseRequest(CurrentMatchmakingQuery);
		CurrentMatchmakingQuery = 0;
	}

	delete ServerListResponse;
	ServerListResponse = NULL;

	if (bCancel)
	{
		if (GameSearch)
		{
			GameSearch->bIsSearchInProgress = FALSE;
		}
		for (INT Index = 0; Index < QueryToRulesResponseMap.Num(); Index++)
		{
			SteamRulesResponse *Response = QueryToRulesResponseMap(Index).Response;
			Response->bDeleteMe = TRUE;
		}
	}
}

// !!! FIXME: there's one of these in OnlineSubsystemSteamworks.cpp, too.
static inline FString QwordToString(const QWORD Value)
{
#ifdef _MSC_VER
	return FString::Printf(TEXT("%uI64"), Value);
#else
	return FString::Printf(TEXT("%llu"), Value);
#endif
}

static inline void SetRule(const TCHAR *Key, const TCHAR *Value)
{
	debugf(NAME_DevOnline,TEXT("Advertising: %s=%s"), Key, Value);
	GSteamMasterServerUpdater->SetKeyValue(TCHAR_TO_UTF8(Key), TCHAR_TO_UTF8(Value));
}

// !!! FIXME: there's one of these in OnlineSubsystemSteamworks.cpp, too.
static inline const TCHAR *TrueOrFalseString(const bool Value)
{
	return (Value) ? TEXT("TRUE") : TEXT("FALSE");
}

void UOnlineGameInterfaceSteamworks::RefreshPublishedGameSettings()
{
	if ((GSteamGameServer == NULL) || (GSteamMasterServerUpdater == NULL))
	{
		return;  // nothing to do (perhaps not advertised?)
	}

	if (!GSteamGameServer->BLoggedOn())
	{
		return;  // still waiting for Steam chatter.
	}

	debugf(NAME_DevOnline,TEXT("Refreshing published game settings..."));
	FString GameName, Region;
	GConfig->GetString(TEXT("URL"), TEXT("GameName"), GameName, GEngineIni);
	GConfig->GetString(TEXT("OnlineSubsystemSteamworks.OnlineSubsystemSteamworks"), TEXT("Region"), Region, GEngineIni);
	if (Region.Len() == 0)
	{
		Region = TEXT("255");
	}

	INT IsLocked = 0;
	const bool bPassworded = ((GameSettings->GetStringSettingValue(7,IsLocked)) && (IsLocked));  // hack from GameSpy code.
	const int Version = GEngineMinNetVersion;
	const bool bDedicated = GameSettings->bIsDedicated;
	const int Slots = GameSettings->NumPublicConnections + GameSettings->NumPrivateConnections;
	const int NumPlayers = Slots - (GameSettings->NumOpenPublicConnections + GameSettings->NumOpenPrivateConnections);
	const FString MapName = GWorld->CurrentLevel->GetOutermost()->GetName();
	const INT NumBots = GWorld->GetGameInfo()->NumBots;
	const FString GameType(GWorld->GetGameInfo()->GetClass()->GetName());
	FString ServerName(GameSettings->OwningPlayerName);

	static UBOOL bMustFindServerDescId = TRUE;
	static INT ServerDescId = -1;
	if (bMustFindServerDescId)
	{
		bMustFindServerDescId = FALSE;
		const FName DescName(TEXT("ServerDescription"),FNAME_Find);
		for(INT Index = 0; Index < GameSettings->PropertyMappings.Num(); Index++)
		{
			if (GameSettings->PropertyMappings(Index).Name == DescName)
			{
				ServerDescId = GameSettings->PropertyMappings(Index).Id;
				break;  // all done!
			}
		}
	}

	if (ServerDescId != -1)
	{
		// Try to use the ServerDescription here instead of the OwningPlayerName.
		for(INT Index = 0; Index < GameSettings->Properties.Num(); Index++)
		{
			const INT PropertyId = GameSettings->Properties(Index).PropertyId;
			if (PropertyId == ServerDescId)
			{
				const FString ServerDesc = GameSettings->Properties(Index).Data.ToString();
				if (ServerDesc.Len() > 0)
				{
					ServerName = GameSettings->Properties(Index).Data.ToString();
				}
				break;
			}
		}
	}

	debugf(NAME_DevOnline, TEXT("Basic server data: %d, %s, %s, %s, %d, %s, %s, %s, %d, %d, %s"), Version, TrueOrFalseString(bDedicated), *Region, *GameName, Slots, TrueOrFalseString(bPassworded), *ServerName, *MapName, NumPlayers, NumBots, *GameType);
	GSteamGameServer->UpdateServerStatus(NumPlayers, Slots, NumBots, TCHAR_TO_UTF8(*ServerName), TCHAR_TO_UTF8(*ServerName), TCHAR_TO_UTF8(*MapName));
	GSteamGameServer->SetGameTags(TCHAR_TO_UTF8(*GameType));

	// Report player info.
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	AGameReplicationInfo* GRI = WorldInfo->GRI;
	for (INT Index = 0; Index < GRI->PRIArray.Num(); Index++)
	{
		APlayerReplicationInfo* PRI = GRI->PRIArray(Index);
		if (PRI)
		{
			GSteamGameServer->BUpdateUserData(CSteamID((uint64) PRI->UniqueId.Uid), TCHAR_TO_UTF8(*PRI->PlayerName), (uint32) PRI->Score);
		}
	}

	// In theory, the first *GameName should be the base game ("Unreal Tournament 2004") and the second should be
	//  the current game, perhaps a complete mod of the base game ("Red Orchestra"). But Unreal generally expects the mod
	//  to change URL.GameName in its .ini file, so we use the same value for both.
	const INT STEAM_MASTER_PROTOCOL_VERSION = 7;   // magic number.
	GSteamMasterServerUpdater->SetBasicServerData(STEAM_MASTER_PROTOCOL_VERSION, bDedicated, TCHAR_TO_UTF8(*Region), TCHAR_TO_UTF8(*GameName), Slots, bPassworded, TCHAR_TO_UTF8(*GameName));

	// set all our auxiliary data.
	GSteamMasterServerUpdater->ClearAllKeyValues();

	// First, some basic keys...
	SetRule(TEXT("SteamEngineVersion"), *QwordToString(GEngineVersion));
	SetRule(TEXT("OwningPlayerId"), *QwordToString(GameSettings->OwningPlayerId.Uid));

	// Then, the databindable things we want to publish...
	for (UProperty* Property = GameSettings->GetClass()->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
	{
		// If the property is databindable and is not an object, we want to report it
		if ((Property->PropertyFlags & CPF_DataBinding) != 0 &&
				Cast<UObjectProperty>(Property,CLASS_IsAUObjectProperty) == NULL &&
				ShouldAdvertiseUProperty(Property->GetFName()))
		{
			const BYTE* ValueAddress = (BYTE*)GameSettings + Property->Offset;
			FString StringValue;
			Property->ExportTextItem(StringValue,ValueAddress,NULL,GameSettings, Property->PropertyFlags & PPF_Localized);
			SetRule(*Property->GetName(), *StringValue);
		}
	}

	// Next, settings...
	for(INT Index = 0; Index < GameSettings->LocalizedSettings.Num(); Index++)
	{
		const INT SettingId = GameSettings->LocalizedSettings(Index).Id;
		const FString SettingName(FString::Printf(TEXT("s%d"), SettingId));
		const INT SettingValueInt = GameSettings->LocalizedSettings(Index).ValueIndex;
		const FString SettingValue(FString::Printf(TEXT("%d"), SettingValueInt));
		SetRule(*SettingName, *SettingValue);
	}

	// Finally, properties...
	for(INT Index = 0; Index < GameSettings->Properties.Num(); Index++)
	{
		const INT PropertyId = GameSettings->Properties(Index).PropertyId;
		const FString PropertyName(FString::Printf(TEXT("p%d"), PropertyId));
		const FString PropertyValue(GameSettings->Properties(Index).Data.ToString());
		SetRule(*PropertyName, *PropertyValue);
	}
}

void UOnlineGameInterfaceSteamworks::OnGSPolicyResponse(const UBOOL bIsVACSecured)
{
	if (GameSettings != NULL)
	{
		GameSettings->bAntiCheatProtected = bIsVACSecured;
	}

	RefreshPublishedGameSettings();
}

UBOOL UOnlineGameInterfaceSteamworks::PublishSteamServer(const EServerMode ServerMode)
{
	check(SessionInfo);
	UBOOL Return = FALSE;
	const uint16 Port = (uint16) SessionInfo->HostAddr.GetPort();
	DWORD Int32Addr = 0;
	SessionInfo->HostAddr.GetIp(Int32Addr);

	INT BasePort = 0;
	GConfig->GetInt(TEXT("URL"), TEXT("Port"), BasePort, GEngineIni);

	FString GameDir, GameVersion;
	GConfig->GetString(TEXT("OnlineSubsystemSteamworks.OnlineSubsystemSteamworks"), TEXT("GameDir"), GameDir, GEngineIni);
	GConfig->GetString(TEXT("OnlineSubsystemSteamworks.OnlineSubsystemSteamworks"), TEXT("GameVersion"), GameVersion, GEngineIni);

	if (GameDir.Len() == 0)
	{
		debugf(TEXT("You don't have [OnlineSubsystemSteamworks.OnlineSubsystemSteamworks].GameDir set in your .ini! Can't publish server on Steam!"));
		return FALSE;
	}

	if (GameVersion.Len() == 0)
	{
		debugf(TEXT("You don't have [OnlineSubsystemSteamworks.OnlineSubsystemSteamworks].GameVersion set in your .ini! Can't publish server on Steam!"));
		return FALSE;
	}

	if (BasePort == 0)
	{
		BasePort = 7777;  // oh well.
	}

	if (Port < ((uint16)BasePort))
	{
		BasePort = Port;  // oh well.
	}

	if (SteamGameServer_Init((uint32) Int32Addr, Port+1, Port, 0, 27015 + (Port - BasePort), ServerMode, TCHAR_TO_UTF8(*GameDir), TCHAR_TO_UTF8(*GameVersion)))
	{
		if ((GSteamGameServer = SteamGameServer()) == NULL)
		{
			debugf(NAME_DevOnline, TEXT("Steamworks: SteamGameServer() failed!"));
			SteamGameServer_Shutdown();
		}
#if HAVE_STEAM_GAMESERVER_STATS
		else if ((GSteamGameServerStats = SteamGameServerStats()) == NULL)
		{
			debugf(NAME_DevOnline, TEXT("Steamworks: SteamGameServerStats() failed!"));
			GSteamGameServer = NULL;
			SteamGameServer_Shutdown();
		}
#endif
		else if ((GSteamMasterServerUpdater = SteamMasterServerUpdater()) == NULL)
		{
			debugf(NAME_DevOnline, TEXT("Steamworks: SteamMasterServerUpdater() failed!"));
			GSteamGameServer = NULL;
#if HAVE_STEAM_GAMESERVER_STATS
			GSteamGameServerStats = NULL;
#endif
			SteamGameServer_Shutdown();
		}
		else
		{
			RefreshPublishedGameSettings();
			Return = TRUE;
		}
		GSteamMasterServerUpdater->SetActive(true);
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("SteamGameServer_Init() failed!"));
	}

	return Return;
}

// !!! FIXME: we should probably just make CreateInternetGame and CreateLanGame virtual in the superclass.
/**
 * Creates an online game based upon the settings object specified.
 * NOTE: online game registration is an async process and does not complete
 * until the OnCreateOnlineGameComplete delegate is called.
 *
 * @param HostingPlayerNum the index of the player hosting the match
 * @param SessionName the name of the session being created
 * @param NewGameSettings the settings to use for the new game session
 *
 * @return true if successful creating the session, false otherwsie
 */
UBOOL UOnlineGameInterfaceSteamworks::CreateOnlineGame(BYTE HostingPlayerNum,FName SessionName,UOnlineGameSettings* NewGameSettings)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
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
				// Allow for per platform override of the session info
				SessionInfo = CreateSessionInfo();
				// Init the game settings counts so the host can use them later
				GameSettings->NumOpenPrivateConnections = GameSettings->NumPrivateConnections;
				GameSettings->NumOpenPublicConnections = GameSettings->NumPublicConnections;
				// Copy the unique id of the owning player
				GameSettings->OwningPlayerId = OwningSubsystem->eventGetPlayerUniqueNetIdFromIndex(HostingPlayerNum);
				// Copy the name of the owning player
				GameSettings->OwningPlayerName = AGameReplicationInfo::StaticClass()->GetDefaultObject<AGameReplicationInfo>()->ServerName;
				if ( GameSettings->OwningPlayerName.Len() == 0 )
				{
					GameSettings->OwningPlayerName = OwningSubsystem->eventGetPlayerNicknameFromIndex(HostingPlayerNum);
				}
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
			debugf(NAME_Error,TEXT("Can't create a new online session when one is in progress: %s"), *(GameSettings->GetPathName()));
		}
		// Update the game state if successful
		if (Return == S_OK || Return == ERROR_IO_PENDING)
		{
			CurrentGameState = OGS_Pending;
		}
		if (Return != ERROR_IO_PENDING)
		{
			FAsyncTaskDelegateResultsNamedSession Params(SessionName,Return);
			TriggerOnlineDelegates(this,CreateOnlineGameCompleteDelegates,&Params);
		}
		Result = ( Return == S_OK || Return == ERROR_IO_PENDING );
	}
	else
	{
		Result = Super::CreateOnlineGame( HostingPlayerNum, SessionName, NewGameSettings );
	}

	return( Result );
}

/**
 * Creates a new Steamworks enabled game and registers it with the backend
 *
 * @param HostingPlayerNum the player hosting the game
 *
 * @return S_OK if it succeeded, otherwise an Error code
 */
DWORD UOnlineGameInterfaceSteamworks::CreateInternetGame(BYTE HostingPlayerNum)
{
	check(SessionInfo);
	DWORD Return = E_FAIL;
	// Don't try to create the session if the network device is broken
	if (GSocketSubsystem->HasNetworkDevice())
	{
		// Skip advertising if it is not required
		if (GameSettings->bShouldAdvertise &&
			GameSettings->NumPublicConnections > 0)
		{
			UBOOL bUseVAC = FALSE;
			GConfig->GetBool(TEXT("OnlineSubsystemSteamworks.OnlineSubsystemSteamworks"), TEXT("bUseVAC"), bUseVAC, GEngineIni);
			if (!bUseVAC)  // GConfig takes precedence if it enabled it, in case game doesn't have UI for enabling anti-cheat.
			{
				bUseVAC = GameSettings->bAntiCheatProtected;
			}
			debugf(NAME_DevOnline, TEXT("Steam server wants VAC: %s"), TrueOrFalseString(bUseVAC != FALSE));
			const EServerMode ServerMode = (bUseVAC) ? eServerModeAuthenticationAndSecure : eServerModeAuthentication;
			if (PublishSteamServer(ServerMode))
			{
				// Now that the internet game has been successfully created, all clients need to reconnect/reauth with Steam
				UNetDriver* NetDriver = GWorld->GetNetDriver();

				if (NetDriver != NULL && NetDriver->ClientConnections.Num() > 0)
				{
					QWORD ServerID = (QWORD)SteamGameServer_GetSteamID();
					BYTE bServerAntiCheatSecured = (BYTE)(SteamGameServer_BSecure() ? 1 : 0);

					for (INT i=0; i<NetDriver->ClientConnections.Num(); i++)
					{
						FNetControlMessage<NMT_AuthServer>::Send(NetDriver->ClientConnections(i), ServerID,
											bServerAntiCheatSecured);
						NetDriver->ClientConnections(i)->FlushNet();
					}
				}

				Return = S_OK;
			}
		}
		else
		{
			// Creating a private match so indicate ok
			debugf(NAME_DevOnline,TEXT("Creating a private match (not registered with Steam)"));
			Return = S_OK;
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't create an Internet game without a network connection"));
	}

	if (Return == S_OK)
	{
		// Register all local talkers
		RegisterLocalTalkers();
		CurrentGameState = OGS_Pending;
	}
	return Return;
}

/**
 * Creates a new lan enabled game
 *
 * @param HostingPlayerNum the player hosting the game
 *
 * @return S_OK if it succeeded, otherwise an error code
 */
DWORD UOnlineGameInterfaceSteamworks::CreateLanGame(BYTE HostingPlayerNum)
{
	check(SessionInfo);
	DWORD Return = S_OK;

	if( GSteamworksInitialized )
	{
		// Don't try to create the session if the network device is broken
		if (!GSocketSubsystem->HasNetworkDevice())
		{
			debugf(NAME_DevOnline, TEXT("Can't create a LAN game without a network connection"));
			Return = E_FAIL;
		}

		// Don't create a lan beacon if advertising is off
		if ((Return == S_OK) && (GameSettings->bShouldAdvertise == TRUE))
		{
			if (GameSettings->bAntiCheatProtected)
			{
				// no NAME_DevOnline for this one.
				debugf(TEXT("Requested LAN game with anti-cheat, but VAC is for Internet games. Disabling anti-cheat."));  // "play with your friends."
				GameSettings->bAntiCheatProtected = FALSE;
			}

			if (!StartSteamLanBeacon())
			{
				Return = E_FAIL;
			}
		}
		if (Return == S_OK)
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
	}
	else
	{
		Return = Super::CreateLanGame( HostingPlayerNum );
	}

	return Return;
}

/**
 * Updates the localized settings/properties for the game in question
 *
 * @param SessionName the name of the session to update
 * @param UpdatedGameSettings the object to update the game settings with
 * @param ignored Steam always needs to be updated
 *
 * @return true if successful creating the session, false otherwsie
 */
UBOOL UOnlineGameInterfaceSteamworks::UpdateOnlineGame(FName SessionName,UOnlineGameSettings* UpdatedGameSettings,UBOOL Ignored)
{
	// Don't try to without a network device
	if (GSocketSubsystem->HasNetworkDevice())
	{
		if (UpdatedGameSettings != NULL)
		{
			GameSettings = UpdatedGameSettings;
			RefreshPublishedGameSettings();
		}
		else
		{
			debugf(NAME_Error,TEXT("Can't update game settings with a NULL object"));
		}
	}
	FAsyncTaskDelegateResultsNamedSession Results(SessionName,0);
	TriggerOnlineDelegates(this,UpdateOnlineGameCompleteDelegates,&Results);
	return TRUE;
}

/** Returns TRUE if the game wants stats, FALSE if not */
UBOOL UOnlineGameInterfaceSteamworks::GameWantsStats()
{
	return GameSettings != NULL &&
		GameSettings->bIsLanMatch == FALSE &&
		GameSettings->bUsesStats == TRUE;
}

// !!! FIXME: just make the superclass Join*Game() virtual.
/**
 * Joins the game specified
 *
 * @param PlayerNum the index of the player searching for a match
 * @param SessionName the name of the session being joined
 * @param DesiredGame the desired game to join
 *
 * @return true if the call completed successfully, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::JoinOnlineGame(BYTE PlayerNum,FName SessionName,const FOnlineGameSearchResult& DesiredGame)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
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
				// Register all local talkers for voice
				RegisterLocalTalkers();
				FAsyncTaskDelegateResultsNamedSession Params(SessionName,S_OK);
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
		// Update the game state if successful
		if (Return == S_OK || Return == ERROR_IO_PENDING)
		{
			// Set the state so that StartOnlineGame() can work
			CurrentGameState = OGS_Pending;
		}
		if (Return != ERROR_IO_PENDING)
		{
			FAsyncTaskDelegateResultsNamedSession Params(SessionName,Return);
			TriggerOnlineDelegates(this,JoinOnlineGameCompleteDelegates,&Params);
		}
		debugf(NAME_DevOnline, TEXT("JoinOnlineGame  Return:0x%08X"), Return);
		Result = ( Return == S_OK || Return == ERROR_IO_PENDING );
	}
	else
	{
		Result = Super::JoinOnlineGame( PlayerNum, SessionName, DesiredGame );
	}

	return( Result );
}

/**
 * Joins the specified internet enabled game
 *
 * @param PlayerNum the player joining the game
 *
 * @return S_OK if it succeeded, otherwise an Error code
 */
DWORD UOnlineGameInterfaceSteamworks::JoinInternetGame(BYTE PlayerNum)
{
	DWORD Return = E_FAIL;
	// Don't try to without a network device
	if (GSocketSubsystem->HasNetworkDevice())
	{
		UOnlineSubsystemSteamworks* OnlineSub = CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem);
		// Make sure they are logged in to play online
		if (OnlineSub->LoggedInStatus == LS_LoggedIn)
		{
			// Register all local talkers
			RegisterLocalTalkers();
			// Check if we need to NAT Negotiation
			CurrentGameState = OGS_Pending;
			// Update the stats flag if present
			OnlineSub->bIsStatsSessionOk = GameWantsStats();
			Return = S_OK;
		}
		else
		{
			debugf(NAME_DevOnline,TEXT("Can't join an internet game without being logged into an online profile"));
		}
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't join an internet game without a network connection"));
	}
	return Return;
}

/**
 * Starts the specified internet enabled game
 *
 * @return S_OK if it succeeded, otherwise an Error code
 */
DWORD UOnlineGameInterfaceSteamworks::StartInternetGame()
{
	DWORD Return = E_FAIL;
	// Don't try to search if the network device is broken
	if (GSocketSubsystem->HasNetworkDevice())
	{
		// Enable the stats session
		UOnlineSubsystemSteamworks* OnlineSub = CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem);
		OnlineSub->bIsStatsSessionOk = TRUE;

		CurrentGameState = OGS_InProgress;
		FAsyncTaskDelegateResults Params(Return);
		TriggerOnlineDelegates(this,StartOnlineGameCompleteDelegates,&Params);
		Return = S_OK;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't start an internet game without a network connection"));
	}
	return Return;
}

/**
 * Ends the specified internet enabled game
 *
 * @return S_OK if it succeeded, otherwise an Error code
 */
DWORD UOnlineGameInterfaceSteamworks::EndInternetGame()
{
	DWORD Return = E_FAIL;
	// Don't try to search if the network device is broken
	if (GSocketSubsystem->HasNetworkDevice())
	{
		CurrentGameState = OGS_Ended;
		CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem)->FlushOnlineStats(FName(TEXT("Game")));
		Return = S_OK;
	}
	else
	{
		debugf(NAME_DevOnline,TEXT("Can't end an internet game without a network connection"));
	}
	return Return;
}

// !!! FIXME: Just make Destroy*Game() virtual in the superclass.
/**
 * Destroys the current online game
 * NOTE: online game de-registration is an async process and does not complete
 * until the OnDestroyOnlineGameComplete delegate is called.
 *
 * @param SessionName the name of the session being destroyed
 *
 * @return true if successful destroying the session, false otherwsie
 */
UBOOL UOnlineGameInterfaceSteamworks::DestroyOnlineGame(FName SessionName)
{
	UBOOL Result = FALSE;

	if( GSteamworksInitialized )
	{
		DWORD Return = E_FAIL;
		// Don't shut down if it isn't valid
		if (GameSettings != NULL && SessionInfo != NULL)
		{
			// Stop all local talkers (avoids a debug runtime warning)
			UnregisterLocalTalkers();
			// Stop all remote voice before ending the session
			RemoveAllRemoteTalkers();
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


		// At this stage the listen host is no longer authenticated (NM_ListenServer checks not necessary)
		UOnlineSubsystemSteamworks* OnlineSub = CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem);

		if (OnlineSub != NULL)
			OnlineSub->bListenHostAuthenticated = FALSE;


		// Update the game state if successful
		if (Return == S_OK || Return == ERROR_IO_PENDING)
		{
			CurrentGameState = OGS_NoSession;
		}
		if (Return != ERROR_IO_PENDING)
		{
			// Fire the delegate off immediately
			FAsyncTaskDelegateResultsNamedSession Params(SessionName,Return);
			TriggerOnlineDelegates(this,DestroyOnlineGameCompleteDelegates,&Params);
		}
		Result = ( Return == S_OK || Return == ERROR_IO_PENDING );
	}
	else
	{
		Result = Super::DestroyOnlineGame( SessionName );
	}

	return( Result );
}

/**
 * Terminates a Steamworks session, removing it from the Steamworks backend
 *
 * @return an Error/success code
 */
DWORD UOnlineGameInterfaceSteamworks::DestroyInternetGame()
{
	check(SessionInfo);
	if( GSteamworksInitialized )
	{
		UOnlineSubsystemSteamworks* OnlineSub = CastChecked<UOnlineSubsystemSteamworks>(OwningSubsystem);
		// Update the stats flag if present
		OnlineSub->bIsStatsSessionOk = FALSE;
		OnlineSub->UnregisterLocalTalkers();
		OnlineSub->UnregisterRemoteTalkers();

		if (GSteamMasterServerUpdater != NULL)
		{
			GSteamMasterServerUpdater->SetActive(false);
			GSteamMasterServerUpdater->NotifyShutdown();
		}

		// Clean up before firing the delegate
		delete SessionInfo;
		SessionInfo = NULL;
		// Save this off so we can trigger the right delegates
		UBOOL bIsDedicated = GameSettings->bIsDedicated;
		// Null out the no longer valid game settings
		GameSettings = NULL;

		SteamGameServer_Shutdown();
		GSteamGameServer = NULL;
		GSteamMasterServerUpdater = NULL;
#if HAVE_STEAM_GAMESERVER_STATS
		GSteamGameServerStats = NULL;
#endif
	}
	else
	{
		Super::DestroyInternetGame();
	}

	return S_OK;
}

/**
 * Terminates a LAN session
 *
 * @return an error/success code
 */
DWORD UOnlineGameInterfaceSteamworks::DestroyLanGame()
{
	if( GSteamworksInitialized )
	{
		check(SessionInfo);
		// Tear down the lan beacon
		StopSteamLanBeacon();
		// Clean up before firing the delegate
		delete SessionInfo;
		SessionInfo = NULL;
		// Save this off so we can trigger the right delegates
		UBOOL bIsDedicated = GameSettings->bIsDedicated;
		// Null out the no longer valid game settings
		GameSettings = NULL;
	}
	else
	{
		Super::DestroyLanGame();
	}

	return S_OK;
}


void UOnlineGameInterfaceSteamworks::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	// delete any response objects we're done with. They can't be deleted sooner, since
	//  the Steamworks SDK touches them after the callback runs.
	for (INT Index = 0; Index < QueryToRulesResponseMap.Num(); Index++)
	{
		SteamRulesResponse *Response = QueryToRulesResponseMap(Index).Response;
		if (Response->bDeleteMe)
		{
			QueryToRulesResponseMap.Remove(Index, 1);
			Index--;			
			delete Response;
		}
	}

	for (INT Index = 0; Index < QueryToPingResponseMap.Num(); Index++)
	{
		SteamPingResponse *Response = QueryToPingResponseMap(Index).Response;
		if (Response->bDeleteMe)
		{
			QueryToPingResponseMap.Remove(Index, 1);
			Index--;			
			delete Response;
		}
	}

	// Server list request is done, but we're still waiting for rules responses for some servers...
	if ((!ServerListResponse) && (GameSearch) && (GameSearch->bIsSearchInProgress))
	{
		if (QueryToRulesResponseMap.Num() == 0)
		{
			// all the pending rules responses came in. We're good.
			GameSearch->bIsSearchInProgress = FALSE;
			FAsyncTaskDelegateResults Param(S_OK);
			TriggerOnlineDelegates(this,FindOnlineGamesCompleteDelegates,&Param);
		}
	}
}


/**
 * Updates any pending lan tasks and fires event notifications as needed
 *
 * @param DeltaTime the amount of time that has elapsed since the last call
 */
void UOnlineGameInterfaceSteamworks::TickLanTasks(FLOAT DeltaTime)
{
	Super::TickLanTasks(DeltaTime);  // ...and recheck...
}

/**
 * Updates any pending internet tasks and fires event notifications as needed
 *
 * @param DeltaTime the amount of time that has elapsed since the last call
 */
void UOnlineGameInterfaceSteamworks::TickInternetTasks(FLOAT DeltaTime)
{
	// no-op: we run pending Steam callbacks elsewhere.
	Super::TickInternetTasks(DeltaTime);
}

/**
 * Tells the online subsystem to accept the game invite that is currently pending
 *
 * @param LocalUserNum the local user accepting the invite
 * @param SessionName the name of the session this invite is to be known as
 */
UBOOL UOnlineGameInterfaceSteamworks::AcceptGameInvite(BYTE LocalUserNum,FName SessionName)
{
	if (InviteGameSearch && InviteGameSearch->Results.Num() > 0)
	{
		// If there is an invite pending, make it our game
		if (JoinOnlineGame(LocalUserNum,FName(TEXT("Game")),InviteGameSearch->Results(0)) == FALSE)
		{
			debugf(NAME_Error,TEXT("Failed to join the invite game, aborting"));
		}
		// Clean up the invite data
		delete (FSessionInfo*)InviteGameSearch->Results(0).PlatformData;
		InviteGameSearch->Results(0).PlatformData = NULL;
		InviteGameSearch = NULL;
		return TRUE;
	}
	return FALSE;
}

/**
 * Creates a game settings object populated with the information from the location string
 *
 * @param LocationString the string to parse
 */
void UOnlineGameInterfaceSteamworks::SetInviteInfo(const TCHAR* LocationString)
{
	// Get rid of the old stuff if applicable
	if (InviteGameSearch && InviteGameSearch->Results.Num() > 0)
	{
		FOnlineGameSearchResult& Result = InviteGameSearch->Results(0);
		FSessionInfo* InviteSessionInfo = (FSessionInfo*)Result.PlatformData;
		// Free the old first
		delete InviteSessionInfo;
		Result.PlatformData = NULL;
		InviteGameSearch->Results.Empty();
	}
	InviteGameSearch = NULL;
	InviteLocationUrl.Empty();
	if (LocationString != NULL)
	{
		InviteLocationUrl = LocationString;
		InviteGameSearch = ConstructObject<UOnlineGameSearch>(UOnlineGameSearch::StaticClass());
		INT AddIndex = InviteGameSearch->Results.AddZeroed();
		FOnlineGameSearchResult& Result = InviteGameSearch->Results(AddIndex);
		Result.GameSettings = ConstructObject<UOnlineGameSettings>(UOnlineGameSettings::StaticClass());
		// Parse the URL to get the server and options
		FURL Url(NULL,LocationString,TRAVEL_Absolute);
		// Get the numbers from the location string
		INT MaxPub = appAtoi(Url.GetOption(TEXT("MaxPub="),TEXT("4")));
		INT MaxPri = appAtoi(Url.GetOption(TEXT("MaxPri="),TEXT("4")));
		// Set the number of slots and the hardcoded values
		Result.GameSettings->NumOpenPublicConnections = MaxPub;
		Result.GameSettings->NumOpenPrivateConnections = MaxPri;
		Result.GameSettings->NumPublicConnections = MaxPub;
		Result.GameSettings->NumPrivateConnections = MaxPri;
		Result.GameSettings->bIsLanMatch = FALSE;
		Result.GameSettings->bWasFromInvite = TRUE;
		// Create the session info and stash on the object
		FSessionInfo* SessInfo = CreateSessionInfo();
		UBOOL bIsValid;
		SessInfo->HostAddr.SetIp(*Url.Host,bIsValid);
		SessInfo->HostAddr.SetPort(Url.Port);
		// Store the session data
		Result.PlatformData = SessInfo;
	}
}

/** Registers all of the local talkers with the voice engine */
void UOnlineGameInterfaceSteamworks::RegisterLocalTalkers()
{
	UOnlineSubsystemSteamworks* Subsystem = Cast<UOnlineSubsystemSteamworks>(OwningSubsystem);
	if (Subsystem != NULL)
	{
		Subsystem->RegisterLocalTalkers();
	}
}

/** Unregisters all of the local talkers from the voice engine */
void UOnlineGameInterfaceSteamworks::UnregisterLocalTalkers()
{
	UOnlineSubsystemSteamworks* Subsystem = Cast<UOnlineSubsystemSteamworks>(OwningSubsystem);
	if (Subsystem != NULL)
	{
		Subsystem->UnregisterLocalTalkers();
	}
}

/**
 * Passes the new player to the subsystem so that voice is registered for this player
 *
 * @param SessionName the name of the session that the player is being added to
 * @param UniquePlayerId the player to register with the online service
 * @param bWasInvited whether the player was invited to the game or searched for it
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::RegisterPlayer(FName SessionName,FUniqueNetId PlayerID,UBOOL bWasInvited)
{
	UOnlineSubsystemSteamworks* Subsystem = Cast<UOnlineSubsystemSteamworks>(OwningSubsystem);
	if (Subsystem != NULL)
	{
		const CSteamID SteamPlayerId((uint64)PlayerID.Uid);

		Subsystem->RegisterRemoteTalker(PlayerID);

#if HAVE_STEAM_GAMESERVER_STATS && WITH_UE3_NETWORKING && WITH_STEAMWORKS
		// Retrieve the players stats (otherwise you can't update them later)
		if (GSteamGameServerStats != NULL)
		{
			Subsystem->CallbackBridge->AddSpecificGSStatsRequest(GSteamGameServerStats->RequestUserStats(SteamPlayerId));
		}
		else if (GSteamUserStats != NULL)
		{
			Subsystem->CallbackBridge->AddSpecificUserStatsRequest(GSteamUserStats->RequestUserStats(SteamPlayerId));
		}
#endif
	}

#if 1  // !!! FIXME: this is what GameSpy does at the moment, but it's not correct. The Live code (in the #else) is, but this requires reworking the uscript to get the Results struct to generate.
	FAsyncTaskDelegateResultsNamedSession Params(FName(TEXT("Game")),S_OK);
	TriggerOnlineDelegates(this,RegisterPlayerCompleteDelegates,&Params);
#else
	OnlineSubsystemSteamworks_eventOnRegisterPlayerComplete_Parms Results(EC_EventParm);
	Results.SessionName = SessionName;
	Results.PlayerID = PlayerID;
	Results.bWasSuccessful = FIRST_BITFIELD;
	TriggerOnlineDelegates(Subsystem,RegisterPlayerCompleteDelegates,&Results);
#endif
	return TRUE;
}

/**
 * Passes the removed player to the subsystem so that voice is unregistered for this player
 *
 * @param SessionName the name of the session to remove the player from
 * @param PlayerId the player to unregister with the online service
 *
 * @return true if the call succeeds, false otherwise
 */
UBOOL UOnlineGameInterfaceSteamworks::UnregisterPlayer(FName SessionName,FUniqueNetId PlayerID)
{
	UOnlineSubsystemSteamworks* Subsystem = Cast<UOnlineSubsystemSteamworks>(OwningSubsystem);
	if (Subsystem != NULL)
	{
		Subsystem->UnregisterRemoteTalker(PlayerID);
	}

	// Skip if we aren't playing a networked match
	if ( GSteamGameServer && GWorld && GWorld->GetNetDriver())
	{
		// Mark this player as disconnected so Steam can update their status.
		const CSteamID SteamPlayerID((uint64) PlayerID.Uid);
		GSteamGameServer->SendUserDisconnect(SteamPlayerID);
	}

#if 1  // !!! FIXME: this is what GameSpy does at the moment, but it's not correct. The Live code (in the #else) is, but this requires reworking the uscript to get the Results struct to generate.
	FAsyncTaskDelegateResultsNamedSession Params(FName(TEXT("Game")),S_OK);
	TriggerOnlineDelegates(this,UnregisterPlayerCompleteDelegates,&Params);
#else
	OnlineSubsystemSteamworks_eventOnUnregisterPlayerComplete_Parms Results(EC_EventParm);
	Results.SessionName = SessionName;
	Results.PlayerID = PlayerID;
	Results.bWasSuccessful = FIRST_BITFIELD;
	TriggerOnlineDelegates(Subsystem,UnregisterPlayerCompleteDelegates,&Results);
#endif

	return TRUE;
}

/**
 * Serializes the platform specific data into the provided buffer for the specified search result
 *
 * @param DesiredGame the game to copy the platform specific data for
 * @param PlatformSpecificInfo the buffer to fill with the platform specific information
 *
 * @return true if successful serializing the data, false otherwise
 */
DWORD UOnlineGameInterfaceSteamworks::ReadPlatformSpecificInternetSessionInfo(const FOnlineGameSearchResult& DesiredGame,BYTE PlatformSpecificInfo[64])
{
	DWORD Return = E_FAIL;

	FNboSerializeToBuffer Buffer(64);
	FSessionInfo* SessionInfo = (FSessionInfo*)DesiredGame.PlatformData;
	// Write the connection data
	Buffer << SessionInfo->HostAddr;
	if (Buffer.GetByteCount() <= 64)
	{
		// Copy the built up data
		appMemcpy(PlatformSpecificInfo,Buffer.GetRawBuffer(0),Buffer.GetByteCount());
		Return = S_OK;
	}
	else
	{
		debugf(NAME_DevOnline,
			TEXT("Platform data is larger (%d) than the supplied buffer (64)"),
			Buffer.GetByteCount());
	}

	return Return == S_OK;
}

/**
 * Creates a search result out of the platform specific data and adds that to the specified search object
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired search to bind the session to
 * @param PlatformSpecificInfo the platform specific information to convert to a server object
 *
 * @return the result code for the operation
 */
DWORD UOnlineGameInterfaceSteamworks::BindPlatformSpecificSessionToInternetSearch(BYTE SearchingPlayerNum,UOnlineGameSearch* SearchSettings,BYTE* PlatformSpecificInfo)
{
	DWORD Return = E_FAIL;

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
		// Allocate and read the session data
		FSessionInfo* SessInfo = new FSessionInfo(E_NoInit);
		// Read the serialized data from the buffer
		FNboSerializeFromBuffer Packet(PlatformSpecificInfo,80);
		// Read the connection data
		Packet >> SessInfo->HostAddr;
		// Store this in the results
		Result.PlatformData = SessInfo;
		Return = S_OK;
	}

	return Return;
}

#endif

