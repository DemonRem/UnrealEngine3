/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef INCLUDED_ONLINESUBSYSTEMSTEAMWORKS_H
#define INCLUDED_ONLINESUBSYSTEMSTEAMWORKS_H 1

// Enable steam gameserver stats
#define HAVE_STEAM_GAMESERVER_STATS 1

// Enable steam client stats
#define HAVE_STEAM_CLIENT_STATS 1

// Temporary define, for encasing code which works around a Steamworks SDK crash
#define STEAM_CRASH_FIX 1


#include "UnIpDrv.h"

#if WITH_UE3_NETWORKING && WITH_STEAMWORKS

// !!! FIXME: Steam headers trigger secure-C-runtime warnings in Visual C++.
// !!! FIXME:  Rather than mess with _CRT_SECURE_NO_WARNINGS, we'll just
// !!! FIXME:  disable the warnings locally. This is a FIXME because this
// !!! FIXME:  should be removed when this is corrected in the Steamworks SDK.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#pragma pack(push,8)
#endif

// Steamworks SDK headers
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

// !!! FIXME: (See FIXME, above.)
#ifdef _MSC_VER
#pragma pack(pop)
#pragma warning(pop)
#endif

// Cached interface pointers so we don't use the getters for each API call...
extern ISteamUtils *GSteamUtils;
extern ISteamUser *GSteamUser;
extern ISteamFriends *GSteamFriends;
extern ISteamRemoteStorage *GSteamRemoteStorage;
extern ISteamUserStats *GSteamUserStats;
extern ISteamMatchmakingServers *GSteamMatchmakingServers;
extern ISteamGameServer *GSteamGameServer;
extern ISteamMasterServerUpdater *GSteamMasterServerUpdater;
extern ISteamApps *GSteamApps;
extern uint32 GSteamAppID;

#if HAVE_STEAM_GAMESERVER_STATS
extern ISteamGameServerStats *GSteamGameServerStats;
#endif

// predeclare some classes we'll reference in UnrealScript...
class SteamServerListResponse;
class SteamRulesResponse;
class SteamPingResponse;
class SteamCallbackBridge;
typedef TMap<FString, FString> SteamRulesMap;

/** Version of the delegate parms that takes a status code from the profile login callback and exposes it */
struct FLoginFailedParms
{
	/** The index of the player that the error is for */
	BYTE PlayerNum;
	/** Whether the login worked or not */
    BYTE ErrorStatus;

	FLoginFailedParms(BYTE InPlayerNum) : PlayerNum(InPlayerNum), ErrorStatus(OSCS_NotConnected) {}
	FLoginFailedParms() : PlayerNum(0), ErrorStatus(OSCS_NotConnected) {}
};

// Unreal integration script layer
#include "OnlineSubsystemSteamworksClasses.h"

// Glue to bridge between Steamworks callback class and Unreal class, since embedding a Steam CCallback in unrealscript is complicated.
class SteamCallbackBridge
{
public:
	SteamCallbackBridge(UOnlineSubsystemSteamworks *_Subsystem)
		: Subsystem(_Subsystem)
		, UserStatsReceivedCallback(this, &SteamCallbackBridge::OnUserStatsReceived)
		, UserStatsStoredCallback(this, &SteamCallbackBridge::OnUserStatsStored)
		, GameOverlayActivatedCallback(this, &SteamCallbackBridge::OnGameOverlayActivated)
		, SteamShutdownCallback(this, &SteamCallbackBridge::OnSteamShutdown)
		, GameServerChangeRequestedCallback(this, &SteamCallbackBridge::OnGameServerChangeRequested)
		, SteamServersConnectedCallback(this, &SteamCallbackBridge::OnSteamServersConnected)
		, SteamServersDisconnectedCallback(this, &SteamCallbackBridge::OnSteamServersDisconnected)
		, ClientGameServerDenyCallback(this, &SteamCallbackBridge::OnClientGameServerDeny)
		, GSClientApproveCallback(this, &SteamCallbackBridge::OnGSClientApprove)
		, GSClientDenyCallback(this, &SteamCallbackBridge::OnGSClientDeny)
		, GSClientKickCallback(this, &SteamCallbackBridge::OnGSClientKick)
		, GSPolicyResponseCallback(this, &SteamCallbackBridge::OnGSPolicyResponse)
		, GSClientAchievementStatusCallback(this, &SteamCallbackBridge::OnGSClientAchievementStatus)
	{
	}

	typedef CCallResult<SteamCallbackBridge, UserStatsReceived_t> SpecificUserStatsReceived;
	void AddSpecificUserStatsRequest(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = SpecificUserStatsCallbacks.Add();
			new(&SpecificUserStatsCallbacks(Position)) SpecificUserStatsReceived;
			SpecificUserStatsCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnSpecificUserStatsReceived);
		}
	}

	void EmptySpecificUserStatsRequests()
	{
		for (INT i=0; i<SpecificUserStatsCallbacks.Num(); i++)
			SpecificUserStatsCallbacks(i).Cancel();

		SpecificUserStatsCallbacks.Empty();
	}

#if HAVE_STEAM_GAMESERVER_STATS
	typedef CCallResult<SteamCallbackBridge, GSStatsReceived_t> SpecificGSStatsReceived;
	void AddSpecificGSStatsRequest(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = SpecificGSStatsCallbacks.Add();
			new(&SpecificGSStatsCallbacks(Position)) SpecificGSStatsReceived;
			SpecificGSStatsCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnSpecificGSStatsReceived);
		}
	}

	void EmptySpecificGSStatsRequests()
	{
		for (INT i=0; i<SpecificGSStatsCallbacks.Num(); i++)
			SpecificGSStatsCallbacks(i).Cancel();

		SpecificGSStatsCallbacks.Empty();
	}

	typedef CCallResult<SteamCallbackBridge, GSStatsStored_t> SpecificGSStatsStored;
	void AddSpecificGSStatsStore(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = SpecificGSStatsStoreCallbacks.Add();
			new(&SpecificGSStatsStoreCallbacks(Position)) SpecificGSStatsStored;
			SpecificGSStatsStoreCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnSpecificGSStatsStored);
		}
	}

	void EmptySpecificGSStatsStore()
	{
		for (INT i=0; i<SpecificGSStatsStoreCallbacks.Num(); i++)
			SpecificGSStatsStoreCallbacks(i).Cancel();

		SpecificGSStatsStoreCallbacks.Empty();
	}

	INT GetGSStatsStoreCount()
	{
		return SpecificGSStatsStoreCallbacks.Num();
	}
#endif

	void GetNumberOfCurrentPlayers()
	{
		if (!NumberOfCurrentPlayersCallback.IsActive())
		{
			SteamAPICall_t ApiCall = GSteamUserStats->GetNumberOfCurrentPlayers();

			if (ApiCall != k_uAPICallInvalid)
				NumberOfCurrentPlayersCallback.Set(ApiCall, this, &SteamCallbackBridge::OnNumberOfCurrentPlayers);
		}
	}

	// Leaderboards functions may potentially need to implement multiple callbacks, if called multiple times before the previous
	// calls return, which is why this code is implemented
	// IMPORTANT: Make sure to clean up the callbacks at level change or when done with them, to avoid mem-leaking
	typedef CCallResult<SteamCallbackBridge, LeaderboardFindResult_t> UserFindLeaderboard;

	void AddUserFindLeaderboard(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = UserFindLeaderboardCallbacks.Add();
			new(&UserFindLeaderboardCallbacks(Position)) UserFindLeaderboard;
			UserFindLeaderboardCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnUserFindLeaderboard);
		}
	}

	void EmptyUserFindLeaderboard()
	{
		for (INT i=0; i<UserFindLeaderboardCallbacks.Num(); i++)
			UserFindLeaderboardCallbacks(i).Cancel();

		UserFindLeaderboardCallbacks.Empty();
	}

	typedef CCallResult<SteamCallbackBridge, LeaderboardScoresDownloaded_t> UserDownloadedLeaderboardEntries;

	void AddUserDownloadedLeaderboardEntries(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = UserDownloadedLeaderboardEntriesCallbacks.Add();
			new(&UserDownloadedLeaderboardEntriesCallbacks(Position)) UserDownloadedLeaderboardEntries;
			UserDownloadedLeaderboardEntriesCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnUserDownloadedLeaderboardEntries);
		}
	}

	void EmptyUserDownloadedLeaderboardEntries()
	{
		for (INT i=0; i<UserDownloadedLeaderboardEntriesCallbacks.Num(); i++)
			UserDownloadedLeaderboardEntriesCallbacks(i).Cancel();

		UserDownloadedLeaderboardEntriesCallbacks.Empty();
	}

	typedef CCallResult<SteamCallbackBridge, LeaderboardScoreUploaded_t> UserUploadedLeaderboardScore;

	void AddUserUploadedLeaderboardScore(SteamAPICall_t ApiCall)
	{
		if (ApiCall != k_uAPICallInvalid)
		{
			const INT Position = UserUploadedLeaderboardScoreCallbacks.Add();
			new(&UserUploadedLeaderboardScoreCallbacks(Position)) UserUploadedLeaderboardScore;
			UserUploadedLeaderboardScoreCallbacks(Position).Set(ApiCall, this, &SteamCallbackBridge::OnUserUploadedLeaderboardScore);
		}
	}

	void EmptyUserUploadedLeaderboardScore()
	{
		for (INT i=0; i<UserUploadedLeaderboardScoreCallbacks.Num(); i++)
			UserUploadedLeaderboardScoreCallbacks(i).Cancel();

		UserUploadedLeaderboardScoreCallbacks.Empty();
	}

private:
	UOnlineSubsystemSteamworks *Subsystem;
	TArray<SpecificUserStatsReceived> SpecificUserStatsCallbacks;
#if HAVE_STEAM_GAMESERVER_STATS
	TArray<SpecificGSStatsReceived> SpecificGSStatsCallbacks;
	TArray<SpecificGSStatsStored> SpecificGSStatsStoreCallbacks;
#endif
	TArray<UserFindLeaderboard> UserFindLeaderboardCallbacks;
	TArray<UserDownloadedLeaderboardEntries> UserDownloadedLeaderboardEntriesCallbacks;
	TArray<UserUploadedLeaderboardScore> UserUploadedLeaderboardScoreCallbacks;

	STEAM_CALLBACK(SteamCallbackBridge, OnUserStatsReceived, UserStatsReceived_t, UserStatsReceivedCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnUserStatsStored, UserStatsStored_t, UserStatsStoredCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnGameOverlayActivated, GameOverlayActivated_t, GameOverlayActivatedCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnSteamShutdown, SteamShutdown_t, SteamShutdownCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnGameServerChangeRequested, GameServerChangeRequested_t, GameServerChangeRequestedCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnSteamServersConnected, SteamServersConnected_t, SteamServersConnectedCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnSteamServersDisconnected, SteamServersDisconnected_t, SteamServersDisconnectedCallback);
	STEAM_CALLBACK(SteamCallbackBridge, OnClientGameServerDeny, ClientGameServerDeny_t, ClientGameServerDenyCallback);
	STEAM_GAMESERVER_CALLBACK(SteamCallbackBridge, OnGSClientApprove, GSClientApprove_t, GSClientApproveCallback);
	STEAM_GAMESERVER_CALLBACK(SteamCallbackBridge, OnGSClientDeny, GSClientDeny_t, GSClientDenyCallback);
	STEAM_GAMESERVER_CALLBACK(SteamCallbackBridge, OnGSClientKick, GSClientKick_t, GSClientKickCallback);
	STEAM_GAMESERVER_CALLBACK(SteamCallbackBridge, OnGSPolicyResponse, GSPolicyResponse_t, GSPolicyResponseCallback);
	STEAM_GAMESERVER_CALLBACK(SteamCallbackBridge, OnGSClientAchievementStatus, GSClientAchievementStatus_t, GSClientAchievementStatusCallback);
	void OnSpecificUserStatsReceived(UserStatsReceived_t *CallbackData, bool bIOFailure);
#if HAVE_STEAM_GAMESERVER_STATS
	void OnSpecificGSStatsReceived(GSStatsReceived_t *CallbackData, bool bIOFailure);
	void OnSpecificGSStatsStored(GSStatsStored_t *CallbackData, bool bIOFailure);
#endif
	CCallResult<SteamCallbackBridge, NumberOfCurrentPlayers_t> NumberOfCurrentPlayersCallback;
	void OnNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *CallbackData, bool bIOFailure);

	void OnUserFindLeaderboard(LeaderboardFindResult_t* CallbackData, bool bIOFailure);
	void OnUserDownloadedLeaderboardEntries(LeaderboardScoresDownloaded_t* CallbackData, bool bIOFailure);
	void OnUserUploadedLeaderboardScore(LeaderboardScoreUploaded_t* CallbackData, bool bIOFailure);
};

#endif	//#if WITH_UE3_NETWORKING
#endif  // !INCLUDED_ONLINESUBSYSTEMSTEAMWORKS_H

