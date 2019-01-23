/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This interface deals with the online games. It creates, destroys, performs
 * searches for online games. This interface is overloaded to provide custom
 * matchmaking services
 */
interface OnlineGameInterface
	dependson(OnlineSubsystem,OnlineGameSearch);

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
function bool CreateOnlineGame(byte HostingPlayerNum,OnlineGameSettings NewGameSettings);

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
function AddCreateOnlineGameCompleteDelegate(delegate<OnCreateOnlineGameComplete> CreateOnlineGameCompleteDelegate);

/**
 * Removes the delegate from the list of notifications
 *
 * @param CreateOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearCreateOnlineGameCompleteDelegate(delegate<OnCreateOnlineGameComplete> CreateOnlineGameCompleteDelegate);

/**
 * Updates the localized settings/properties for the game in question
 *
 * @param UpdatedGameSettings the object to update the game settings with
 *
 * @return true if successful creating the session, false otherwsie
 */
function bool UpdateOnlineGame(OnlineGameSettings UpdatedGameSettings);

/** Returns the currently set game settings */
function OnlineGameSettings GetGameSettings();

/**
 * Destroys the current online game
 * NOTE: online game de-registration is an async process and does not complete
 * until the OnDestroyOnlineGameComplete delegate is called.
 *
 * @return true if successful destroying the session, false otherwsie
 */
function bool DestroyOnlineGame();

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
function AddDestroyOnlineGameCompleteDelegate(delegate<OnDestroyOnlineGameComplete> DestroyOnlineGameCompleteDelegate);

/**
 * Removes the delegate from the list of notifications
 *
 * @param DestroyOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearDestroyOnlineGameCompleteDelegate(delegate<OnDestroyOnlineGameComplete> DestroyOnlineGameCompleteDelegate);

/**
 * Searches for games matching the settings specified
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired settings that the returned sessions will have
 *
 * @return true if successful searching for sessions, false otherwise
 */
function bool FindOnlineGames(byte SearchingPlayerNum,OnlineGameSearch SearchSettings);

/**
 * Delegate fired when the search for an online game has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnFindOnlineGamesComplete(bool bWasSuccessful);

/**
 * Adds the delegate used to notify the gameplay code that the search they
 * kicked off has completed
 *
 * @param FindOnlineGamesCompleteDelegate the delegate to use for notifications
 */
function AddFindOnlineGamesCompleteDelegate(delegate<OnFindOnlineGamesComplete> FindOnlineGamesCompleteDelegate);

/**
 * Removes the delegate from the notify list
 *
 * @param FindOnlineGamesCompleteDelegate the delegate to use for notifications
 */
function ClearFindOnlineGamesCompleteDelegate(delegate<OnFindOnlineGamesComplete> FindOnlineGamesCompleteDelegate);

/**
 * Returns the currently set game search object
 */
function OnlineGameSearch GetGameSearch();

/**
 * Cleans up any platform specific allocated data contained in the search results
 */
function FreeSearchResults();

/**
 * Joins the game specified
 *
 * @param PlayerNum the index of the player searching for a match
 * @param DesiredGame the desired game to join
 *
 * @return true if the call completed successfully, false otherwise
 */
function bool JoinOnlineGame(byte PlayerNum,const out OnlineGameSearchResult DesiredGame);

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
function AddJoinOnlineGameCompleteDelegate(delegate<OnJoinOnlineGameComplete> JoinOnlineGameCompleteDelegate);

/**
 * Removes the delegate from the list of notifications
 *
 * @param JoinOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearJoinOnlineGameCompleteDelegate(delegate<OnJoinOnlineGameComplete> JoinOnlineGameCompleteDelegate);

/**
 * Returns the platform specific connection information for joining the match.
 * Call this function from the delegate of join completion
 *
 * @param ConnectInfo the out var containing the platform specific connection information
 *
 * @return true if the call was successful, false otherwise
 */
function bool GetResolvedConnectString(out string ConnectInfo);

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
 * Removes the delegate from the list of notifications
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
 * Removes the delegate from the list of notifications
 *
 * @param UnregisterPlayerCompleteDelegate the delegate to use for notifications
 */
function ClearUnregisterPlayerCompleteDelegate(delegate<OnUnregisterPlayerComplete> UnregisterPlayerCompleteDelegate);

/**
 * Marks an online game as in progress (as opposed to being in lobby)
 *
 * @return true if the call succeeds, false otherwise
 */
function bool StartOnlineGame();

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
function AddStartOnlineGameCompleteDelegate(delegate<OnStartOnlineGameComplete> StartOnlineGameCompleteDelegate);

/**
 * Removes the delegate from the list of notifications
 *
 * @param StartOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearStartOnlineGameCompleteDelegate(delegate<OnStartOnlineGameComplete> StartOnlineGameCompleteDelegate);

/**
 * Marks an online game as having been ended
 *
 * @return true if the call succeeds, false otherwise
 */
function bool EndOnlineGame();

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
function AddEndOnlineGameCompleteDelegate(delegate<OnEndOnlineGameComplete> EndOnlineGameCompleteDelegate);

/**
 * Removes the delegate from the list of notifications
 *
 * @param EndOnlineGameCompleteDelegate the delegate to use for notifications
 */
function ClearEndOnlineGameCompleteDelegate(delegate<OnEndOnlineGameComplete> EndOnlineGameCompleteDelegate);

/**
 * Returns the current state of the online game
 */
function EOnlineGameState GetOnlineGameState();

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
 * Removes the delegate from the list of notifications
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
 * Removes the delegate from the list of notifications
 *
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
