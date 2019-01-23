/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Class that implements a cross platform version of the game interface
 */
class OnlineGameInterfaceImpl extends Object within OnlineSubsystemCommonImpl
	native
	implements(OnlineGameInterface)
	config(Engine);

/** The owning subsystem that this object is providing an implementation for */
var OnlineSubsystemCommonImpl OwningSubsystem;

/** The current game settings object in use */
var const OnlineGameSettings GameSettings;

/** The current game search object in use */
var const OnlineGameSearch GameSearch;

/** The current game state as the Live layer understands it */
var const EOnlineGameState CurrentGameState;

/** Array of delegates to multicast with for game creation notification */
var array<delegate<OnCreateOnlineGameComplete> > CreateOnlineGameCompleteDelegates;

/** Array of delegates to multicast with for game destruction notification */
var array<delegate<OnDestroyOnlineGameComplete> > DestroyOnlineGameCompleteDelegates;

/** Array of delegates to multicast with for game join notification */
var array<delegate<OnJoinOnlineGameComplete> > JoinOnlineGameCompleteDelegates;

/** Array of delegates to multicast with for game starting notification */
var array<delegate<OnStartOnlineGameComplete> > StartOnlineGameCompleteDelegates;

/** Array of delegates to multicast with for game ending notification */
var array<delegate<OnEndOnlineGameComplete> > EndOnlineGameCompleteDelegates;

/** Array of delegates to multicast with for game search notification */
var array<delegate<OnFindOnlineGamesComplete> > FindOnlineGamesCompleteDelegates;

/** Array of delegates to multicast with for server registration completion */
var array<delegate<OnRegisterDedicatedServerComplete> > RegisterDedicatedServerCompleteDelegates;

/** Array of delegates to multicast with for server unregistration completion */
var array<delegate<OnUnregisterDedicatedServerComplete> > UnregisterDedicatedServerCompleteDelegates;

/** The current state the lan beacon is in */
var const ELanBeaconState LanBeaconState;

/** Port to listen on for LAN queries/responses */
var const config int LanAnnouncePort;

/** Unique id to keep UE3 games from seeing each others' lan packets */
var const config int LanGameUniqueId;

/** Used by a client to uniquely identify itself during lan match discovery */
var const byte LanNonce[8];

/** Mask containing which platforms can cross communicate */
var const config int LanPacketPlatformMask;

/** The amount of time before the lan query is considered done */
var float LanQueryTimeLeft;

/** The amount of time to wait before timing out a lan query request */
var config float LanQueryTimeout;

/** LAN announcement socket used to send/receive discovery packets */
var const native transient pointer LanBeacon{FLanBeacon};

/** The session information used to connect to a host */
var native const transient private pointer SessionInfo{FSessionInfo};

/**
 * Delegate fired when the search for an online game has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnFindOnlineGamesComplete(bool bWasSuccessful);

/**
 * Returns the current state of the online game
 */
function EOnlineGameState GetOnlineGameState()
{
	return CurrentGameState;
}

/** Returns the currently set game settings */
function OnlineGameSettings GetGameSettings()
{
	return GameSettings;
}

/** Returns the currently set game search object */
function OnlineGameSearch GetGameSearch()
{
	return GameSearch;
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
native function bool CreateOnlineGame(byte HostingPlayerNum,OnlineGameSettings NewGameSettings);

/**
 * Delegate fired when a create request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnCreateOnlineGameComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the online game they
 * created has completed the creation process
 *
 * @param CreateOnlineGameCompleteDelegate the delegate to use for notifications
 */
function AddCreateOnlineGameCompleteDelegate(delegate<OnCreateOnlineGameComplete> CreateOnlineGameCompleteDelegate)
{
	if (CreateOnlineGameCompleteDelegates.Find(CreateOnlineGameCompleteDelegate) == INDEX_NONE)
	{
		CreateOnlineGameCompleteDelegates[CreateOnlineGameCompleteDelegates.Length] = CreateOnlineGameCompleteDelegate;
	}
}

/**
 * Sets the delegate used to notify the gameplay code that the online game they
 * created has completed the creation process
 *
 * @param CreateOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearCreateOnlineGameCompleteDelegate(delegate<OnCreateOnlineGameComplete> CreateOnlineGameCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = CreateOnlineGameCompleteDelegates.Find(CreateOnlineGameCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		CreateOnlineGameCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Updates the localized settings/properties for the game in question
 *
 * @param UpdatedGameSettings the object to update the game settings with
 *
 * @return true if successful creating the session, false otherwsie
 */
function bool UpdateOnlineGame(OnlineGameSettings UpdatedGameSettings);

/**
 * Destroys the current online game
 * NOTE: online game de-registration is an async process and does not complete
 * until the OnDestroyOnlineGameComplete delegate is called.
 *
 * @return true if successful destroying the session, false otherwsie
 */
native function bool DestroyOnlineGame();

/**
 * Delegate fired when a destroying an online game has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnDestroyOnlineGameComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the online game they
 * destroyed has completed the destruction process
 *
 * @param DestroyOnlineGameCompleteDelegate the delegate to use for notifications
 */
function AddDestroyOnlineGameCompleteDelegate(delegate<OnDestroyOnlineGameComplete> DestroyOnlineGameCompleteDelegate)
{
	if (DestroyOnlineGameCompleteDelegates.Find(DestroyOnlineGameCompleteDelegate) == INDEX_NONE)
	{
		DestroyOnlineGameCompleteDelegates[DestroyOnlineGameCompleteDelegates.Length] = DestroyOnlineGameCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notification list
 *
 * @param DestroyOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearDestroyOnlineGameCompleteDelegate(delegate<OnDestroyOnlineGameComplete> DestroyOnlineGameCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = DestroyOnlineGameCompleteDelegates.Find(DestroyOnlineGameCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		DestroyOnlineGameCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Searches for games matching the settings specified
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired settings that the returned sessions will have
 *
 * @return true if successful searching for sessions, false otherwise
 */
native function bool FindOnlineGames(byte SearchingPlayerNum,OnlineGameSearch SearchSettings);

/**
 * Adds the delegate used to notify the gameplay code that the search they
 * kicked off has completed
 *
 * @param FindOnlineGamesCompleteDelegate the delegate to use for notifications
 */
function AddFindOnlineGamesCompleteDelegate(delegate<OnFindOnlineGamesComplete> FindOnlineGamesCompleteDelegate)
{
	// Only add to the list once
	if (FindOnlineGamesCompleteDelegates.Find(FindOnlineGamesCompleteDelegate) == INDEX_NONE)
	{
		FindOnlineGamesCompleteDelegates[FindOnlineGamesCompleteDelegates.Length] = FindOnlineGamesCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param FindOnlineGamesCompleteDelegate the delegate to use for notifications
 */
function ClearFindOnlineGamesCompleteDelegate(delegate<OnFindOnlineGamesComplete> FindOnlineGamesCompleteDelegate)
{
	local int RemoveIndex;
	// Find it in the list
	RemoveIndex = FindOnlineGamesCompleteDelegates.Find(FindOnlineGamesCompleteDelegate);
	// Only remove if found
	if (RemoveIndex != INDEX_NONE)
	{
		FindOnlineGamesCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Cleans up any platform specific allocated data contained in the search results
 */
native function FreeSearchResults();

/**
 * Joins the game specified
 *
 * @param PlayerNum the index of the player searching for a match
 * @param DesiredGame the desired game to join
 *
 * @return true if the call completed successfully, false otherwise
 */
native function bool JoinOnlineGame(byte PlayerNum,const out OnlineGameSearchResult DesiredGame);

/**
 * Delegate fired when the joing process for an online game has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnJoinOnlineGameComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the join request they
 * kicked off has completed
 *
 * @param JoinOnlineGameCompleteDelegate the delegate to use for notifications
 */
function AddJoinOnlineGameCompleteDelegate(delegate<OnJoinOnlineGameComplete> JoinOnlineGameCompleteDelegate)
{
	if (JoinOnlineGameCompleteDelegates.Find(JoinOnlineGameCompleteDelegate) == INDEX_NONE)
	{
		JoinOnlineGameCompleteDelegates[JoinOnlineGameCompleteDelegates.Length] = JoinOnlineGameCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param JoinOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearJoinOnlineGameCompleteDelegate(delegate<OnJoinOnlineGameComplete> JoinOnlineGameCompleteDelegate)
{
	local int RemoveIndex;
	// Find it in the list
	RemoveIndex = JoinOnlineGameCompleteDelegates.Find(JoinOnlineGameCompleteDelegate);
	// Only remove if found
	if (RemoveIndex != INDEX_NONE)
	{
		JoinOnlineGameCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Returns the platform specific connection information for joining the match.
 * Call this function from the delegate of join completion
 *
 * @param ConnectInfo the out var containing the platform specific connection information
 *
 * @return true if the call was successful, false otherwise
 */
native function bool GetResolvedConnectString(out string ConnectInfo);

/**
 * Registers a player with the online service as being part of the online game
 *
 * @param UniquePlayerId the player to register with the online service
 * @param bWasInvited whether the player was invited to the game or searched for it
 *
 * @return true if the call succeeds, false otherwise
 */
function bool RegisterPlayer(UniqueNetId PlayerId,bool bWasInvited);

/**
 * Delegate fired when the registration process has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnRegisterPlayerComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the player
 * registration request they submitted has completed
 *
 * @param RegisterPlayerCompleteDelegate the delegate to use for notifications
 */
function AddRegisterPlayerCompleteDelegate(delegate<OnRegisterPlayerComplete> RegisterPlayerCompleteDelegate);

/**
 * Removes the delegate from the notify list
 *
 * @param RegisterPlayerCompleteDelegate the delegate to use for notifications
 */
function ClearRegisterPlayerCompleteDelegate(delegate<OnRegisterPlayerComplete> RegisterPlayerCompleteDelegate);

/**
 * Unregisters a player with the online service as being part of the online game
 *
 * @param PlayerId the player to unregister with the online service
 *
 * @return true if the call succeeds, false otherwise
 */
function bool UnregisterPlayer(UniqueNetId PlayerId);

/**
 * Delegate fired when the unregistration process has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnUnregisterPlayerComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the player
 * unregistration request they submitted has completed
 *
 * @param UnregisterPlayerCompleteDelegate the delegate to use for notifications
 */
function AddUnregisterPlayerCompleteDelegate(delegate<OnUnregisterPlayerComplete> UnregisterPlayerCompleteDelegate);

/**
 * Removes the delegate from the notify list
 *
 * @param UnregisterPlayerCompleteDelegate the delegate to use for notifications
 */
function ClearUnregisterPlayerCompleteDelegate(delegate<OnUnregisterPlayerComplete> UnregisterPlayerCompleteDelegate);

/**
 * Marks an online game as in progress (as opposed to being in lobby)
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool StartOnlineGame();

/**
 * Delegate fired when the online game has transitioned to the started state
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnStartOnlineGameComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the online game has
 * transitioned to the started state.
 *
 * @param StartOnlineGameCompleteDelegate the delegate to use for notifications
 */
function AddStartOnlineGameCompleteDelegate(delegate<OnStartOnlineGameComplete> StartOnlineGameCompleteDelegate)
{
	if (StartOnlineGameCompleteDelegates.Find(StartOnlineGameCompleteDelegate) == INDEX_NONE)
	{
		StartOnlineGameCompleteDelegates[StartOnlineGameCompleteDelegates.Length] = StartOnlineGameCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param StartOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearStartOnlineGameCompleteDelegate(delegate<OnStartOnlineGameComplete> StartOnlineGameCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = StartOnlineGameCompleteDelegates.Find(StartOnlineGameCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		StartOnlineGameCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Marks an online game as having been ended
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool EndOnlineGame();

/**
 * Delegate fired when the online game has transitioned to the ending game state
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnEndOnlineGameComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the online game has
 * transitioned to the ending state.
 *
 * @param EndOnlineGameCompleteDelegate the delegate to use for notifications
 */
function AddEndOnlineGameCompleteDelegate(delegate<OnEndOnlineGameComplete> EndOnlineGameCompleteDelegate)
{
	if (EndOnlineGameCompleteDelegates.Find(EndOnlineGameCompleteDelegate) == INDEX_NONE)
	{
		EndOnlineGameCompleteDelegates[EndOnlineGameCompleteDelegates.Length] = EndOnlineGameCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param EndOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearEndOnlineGameCompleteDelegate(delegate<OnEndOnlineGameComplete> EndOnlineGameCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = EndOnlineGameCompleteDelegates.Find(EndOnlineGameCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		EndOnlineGameCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Tells the game to register with the underlying arbitration server if available
 */
function bool RegisterForArbitration();

/**
 * Delegate fired when the online game has completed registration for arbitration
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnArbitrationRegistrationComplete(bool bWasSuccessful);

/**
 * Sets the notification callback to use when arbitration registration has completed
 *
 * @param ArbitrationRegistrationCompleteDelegate the delegate to use for notifications
 */
function AddArbitrationRegistrationCompleteDelegate(delegate<OnArbitrationRegistrationComplete> ArbitrationRegistrationCompleteDelegate);

/**
 * Removes the specified delegate from the notification list
 *
 * @param ArbitrationRegistrationCompleteDelegate the delegate to use for notifications
 */
function ClearArbitrationRegistrationCompleteDelegate(delegate<OnArbitrationRegistrationComplete> ArbitrationRegistrationCompleteDelegate);

/**
 * Returns the list of arbitrated players for the arbitrated session
 */
function array<OnlineArbitrationRegistrant> GetArbitratedPlayers();

/**
 * Sets the delegate used to notify the gameplay code when a game invite has been accepted
 *
 * @param LocalUserNum the user to request notification for
 * @param GameInviteAcceptedDelegate the delegate to use for notifications
 */
function AddGameInviteAcceptedDelegate(byte LocalUserNum,delegate<OnGameInviteAccepted> GameInviteAcceptedDelegate);

/**
 * Removes the specified delegate from the notification list
 *
 * @param LocalUserNum the user to request notification for
 * @param GameInviteAcceptedDelegate the delegate to use for notifications
 */
function ClearGameInviteAcceptedDelegate(byte LocalUserNum,delegate<OnGameInviteAccepted> GameInviteAcceptedDelegate);

/**
 * Called when a user accepts a game invitation. Allows the gameplay code a chance
 * to clean up any existing state before accepting the invite. The invite must be
 * accepted by calling AcceptGameInvite() on the OnlineGameInterface after clean up
 * has completed
 *
 * @param GameInviteSettings all of the game information for the game they've
 *							 accepted the invite to
 */
delegate OnGameInviteAccepted(OnlineGameSettings GameInviteSettings);

/**
 * Tells the online subsystem to accept the game invite that is currently pending
 *
 * @param LocalUserNum the local user accepting the invite
 */
function bool AcceptGameInvite(byte LocalUserNum);

/**
 * Updates the current session's skill rating using the list of players' skills
 *
 * @param Players the set of players to use in the skill calculation
 *
 * @return true if the update succeeded, false otherwise
 */
function bool RecalculateSkillRating(const out array<UniqueNetId> Players);

/**
 * Registers the dedicated server with the online service. NOTE: This is async
 * so will not complete until the delegates indicate that it has
 *
 * @param HostingPlayerNum the player hosting the session
 * @param NewGameSettings the settings that are to be published to the service
 *
 * @return true if the server is registered properly, false otherwise
 */
function bool RegisterDedicatedServer(byte HostingPlayerNum,OnlineGameSettings NewGameSettings)
{
	return CreateOnlineGame(HostingPlayerNum,NewGameSettings);
}

/**
 * Delegate fired when a registration request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnRegisterDedicatedServerComplete(bool bWasSuccessful);

/**
 * Adds a delegate used to notify the gameplay code that the server registration is complete
 *
 * @param RegisterDedicatedServerCompleteDelegate the delegate to use for notifications
 */
function AddRegisterDedicatedServerCompleteDelegate(delegate<OnRegisterDedicatedServerComplete> RegisterDedicatedServerCompleteDelegate)
{
	if (RegisterDedicatedServerCompleteDelegates.Find(RegisterDedicatedServerCompleteDelegate) == INDEX_NONE)
	{
		RegisterDedicatedServerCompleteDelegates[RegisterDedicatedServerCompleteDelegates.Length] = RegisterDedicatedServerCompleteDelegate;
	}
}

/**
 * Removes the delegate from the list of notifications
 *
 * @param RegisterDedicatedServerCompleteDelegate the delegate to use for notifications
 */
function ClearRegisterDedicatedServerCompleteDelegate(delegate<OnRegisterDedicatedServerComplete> RegisterDedicatedServerCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = RegisterDedicatedServerCompleteDelegates.Find(RegisterDedicatedServerCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		RegisterDedicatedServerCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Unregisters the dedicated server with the online service. NOTE: This is async
 * so will not complete until the delegates indicate that it has
 *
 * @param HostingPlayerNum the player hosting the session
 *
 * @return true if the server is unregistered properly, false otherwise
 */
function bool UnregisterDedicatedServer(byte HostingPlayerNum)
{
	return DestroyOnlineGame();
}

/**
 * Delegate fired when the unregistration request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnUnregisterDedicatedServerComplete(bool bWasSuccessful);

/**
 * Adds a delegate used to notify the gameplay code that the server registration is complete
 *
 * @param UnregisterDedicatedServerCompleteDelegate the delegate to use for notifications
 */
function AddUnregisterDedicatedServerCompleteDelegate(delegate<OnUnregisterDedicatedServerComplete> UnregisterDedicatedServerCompleteDelegate)
{
	if (UnregisterDedicatedServerCompleteDelegates.Find(UnregisterDedicatedServerCompleteDelegate) == INDEX_NONE)
	{
		UnregisterDedicatedServerCompleteDelegates[UnregisterDedicatedServerCompleteDelegates.Length] = UnregisterDedicatedServerCompleteDelegate;
	}
}

/**
 * Removes the delegate from the list of notifications
 *
 * @param UnregisterDedicatedServerCompleteDelegate the delegate to use for notifications
 */
function ClearUnregisterDedicatedServerCompleteDelegate(delegate<OnUnregisterDedicatedServerComplete> UnregisterDedicatedServerCompleteDelegate)
{
	local int RemoveIndex;

	RemoveIndex = UnregisterDedicatedServerCompleteDelegates.Find(UnregisterDedicatedServerCompleteDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		UnregisterDedicatedServerCompleteDelegates.Remove(RemoveIndex,1);
	}
}
