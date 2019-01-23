/**
 * Copyright © 2006 Epic Games, Inc. All Rights Reserved.
 */
class OnlineSubsystemLive extends OnlineSubsystem
	native
	implements(OnlinePlayerInterface,OnlinePlayerInterfaceEx,OnlineSystemInterface,OnlineGameInterface,OnlineContentInterface,OnlineVoiceInterface,OnlineStatsInterface)
	config(Engine);

/**
 * The notification handle to use for polling events
 */
var const native transient pointer NotificationHandle{void};

/**
 * The session information of the current session
 */
var const native transient pointer SessionInfo{FSecureSessionInfo};

/**
 * The number of simultaneous logins allowed (1, 2, or 4)
 */
var const config int NumLogins;

/**
 * Where Live notifications will be displayed on the screen
 */
var ENetworkNotificationPosition CurrentNotificationPosition;

/**
 * This is the array of pending async tasks. Each tick these tasks are checked
 * completion. If complete, the delegate associated with them is called
 */
var native const array<pointer> AsyncTasks{FLiveAsyncTask};

/**
 * The current game settings object in use
 */
var const OnlineGameSettings GameSettings;

/**
 * The current game search object in use
 */
var const OnlineGameSearch GameSearch;

/** The current game state as the Live layer understands it */
var const EOnlineGameState CurrentGameState;

/** Holds the cached state of the friends list for a single player */
struct native FriendsListCache
{
	/** The list of returned friends */
	var array<OnlineFriend> Friends;
	/** Indicates the state of the async read */
	var EOnlineEnumerationReadState ReadState;
	/** The delegate to call when the read is complete */
	var delegate<OnReadFriendsComplete> ReadCompleteDelegate;
};

/** Cache of friends data per player */
var FriendsListCache FriendsCache[4];

/** Holds an array of login delegates */
struct native LoginDelegates
{
	/** This is the list of requested delegates to fire */
	var array<delegate<OnLoginChange> > Delegates;
};

/** Used for per player index notification of sign in changes */
var LoginDelegates PlayerLoginDelegates[4];

/** Holds the list of delegates to fire when any login changes */
var LoginDelegates AllLoginDelegates;

/** Used to compare previous sign in masks to handle players signing out */
var const int LastSignInMask;

/** The set of last known xuids */
var const UniqueNetId LastXuids[4];

/** Holds the cached state of the friends list for a single player */
struct native ContentListCache
{
	/** The list of returned friends */
	var array<OnlineContent> Content;
	/** Indicates the state of the async read */
	var EOnlineEnumerationReadState ReadState;
	/** The delegate to call when the content has changed (user logged in, etc) */
	var delegate<OnContentChange> ContentChangeDelegate;
	/** The delegate to call when the read is complete */
	var delegate<OnReadContentComplete> ReadCompleteDelegate;
	/** The number of new downloadable content packages available */
	var int NewDownloadCount;
	/** The total number of downloadable content packages available */
	var int TotalDownloadCount;
	/** The delegate to call when the read is complete */
	var delegate<OnQueryAvailableDownloadsComplete> QueryDownloadsDelegate;
};

/** Cache of content list per player */
var ContentListCache ContentCache[4];

/** The current system link state */
enum ESystemLinkState
{
	SLS_NotUsingSystemLink,
	SLS_Hosting,
	SLS_Searching
};

/** The current state we are in */
var ESystemLinkState SystemLinkState;

/** Port to listen on for system link queries/responses */
var const config int SystemLinkAnnouncePort;

/** System link announcement socket used to send/receive discovery packets */
var const native transient pointer SysLinkSocket{FSystemLinkBeacon};

/** Used by a client to uniquely identify itself during system link match discovery */
var const byte SystemLinkNonce[8];

/** The time the system link query started */
var float SystemLinkQueryTimeLeft;

/** The amount of time to wait before timing out a system link query request */
var config float SystemLinkQueryTimeout;

/** Holds the last keyboard input results */
var string KeyboardInputResults;

/** Per user cache of device id information */
struct native DeviceIdCache
{
	/** The last selected device id for this user */
	var int DeviceId;
	/** Delegate used to fire the array of events off */
	var delegate<OnDeviceSelectionComplete> DeviceSelectionMulticast;
	/** List of subscribers interested in device selection notification */
	var array<delegate<OnDeviceSelectionComplete> > DeviceSelectionDelegates;
};

/** Holds the last results of device selection */
var DeviceIdCache DeviceCache[4];

/** Holds the cached state of the profile for a single player */
struct native ProfileSettingsCache
{
	/** The profile for the player */
	var OnlineProfileSettings Profile;
	/** Used for per player index notification of profile reads completing */
	var delegate<OnReadProfileSettingsComplete> ReadDelegate;
	/** Used for per player index notification of profile reads completing */
	var array<delegate<OnReadProfileSettingsComplete> > ReadDelegates;
	/** Used for per player index notification of profile writes completing */
	var delegate<OnWriteProfileSettingsComplete> WriteDelegate;
};

/** Holds the per player profile data */
var ProfileSettingsCache ProfileCache[4];

/**
 * Holds the pointer to the platform specific FVoiceInterface implementation
 * used for voice communication
 */
var const native transient pointer VoiceEngine{FVoiceInterface};

/** Holds the maximum number of local talkers allowed */
var config int MaxLocalTalkers;

/** Holds the maximum number of remote talkers allowed (clamped to 30 which is XHV max) */
var config int MaxRemoteTalkers;

/**
 * Information about a remote talker's priority
 *
 * Zero means highest priority, < zero means muted
 */
struct native TalkerPriority
{
	/** Holds the current priority for this talker */
	var int CurrentPriority;
	/** Holds the last priority for this talker */
	var int LastPriority;
};

/** Information about a local talker */
struct native LocalTalker
{
	/** Whether this talker is currently registered */
	var bool bHasVoice;
	/** Whether loopback is enabled for this talker */
	var bool bUseLoopback;
	/** Whether the local talker was speaking last frame */
	var bool bWasTalking;
};

/** Information about a remote talker */
struct native RemoteTalker
{
	/** Holds the priorities for each of the local players */
	var TalkerPriority LocalPriorities[4];
	/** The unique id for this talker */
	var UniqueNetId TalkerId;
	/** The number of local talkers that have this remote talker muted */
	var int NumberOfMutes;
	/** Whether the remote talker was speaking last frame */
	var bool bWasTalking;
};

/** Holds information about each of the local talkers */
var LocalTalker LocalTalkers[4];

/** Array of registered remote talkers */
var array<RemoteTalker> RemoteTalkers;

/** Holds the list of delegates that are interested in receiving talking notifications */
var array<delegate<OnPlayerTalking> > TalkingDelegates;

/** The array of delegates for notifying when speech recognition has completed for a player */
var delegate<OnRecognitionComplete> SpeechRecognitionDelegates[4];

/** Whether speech recognition is enabled */
var config bool bIsUsingSpeechRecognition;

/** The array of delegates for notifying when an achievement write has completed */
var delegate<OnUnlockAchievementComplete> AchievementDelegates[4];

/** QoS packet with extra data to send to client */
var const byte QoSPacket[512];

/** The currently outstanding stats read request */
var const OnlineStatsRead CurrentStatsRead;

/** Used to track when a user cancels a sign-in request so that code waiting for a result can continue */
var const bool bIsInSignInUI;

/** This is the list of requested delegates to fire when a login is cancelled */
var array<delegate<OnLoginCancelled> > LoginCancelledDelegates;

/** This is the list of delegates requesting notification when a game search finishes */
var array<delegate<OnFindOnlineGamesComplete> > FindOnlineGamesCompleteDelegates;

/** This is the list of delegates requesting notification when a stats read finishes */
var array<delegate<OnReadOnlineStatsComplete> > ReadOnlineStatsCompleteDelegates;

/** This is the list of delegates requesting notification when a Live UI opens/closes */
var array<delegate<OnExternalUIChange> > ExternalUIChangeDelegates;

/** This is the list of delegates requesting notification when a controller's state changes */
var array<delegate<OnControllerChange> > ControllerChangeDelegates;

/** Holds a true/false connection state for each of the possible 4 controllers */
var int LastInputDeviceConnectedMask;

/** This is the list of delegates requesting notification Live's connection state changes */
var array<delegate<OnConnectionStatusChange> > ConnectionStatusChangeDelegates;

/** This is the list of delegates requesting notification of link status changes */
var array<delegate<OnConnectionStatusChange> > LinkStatusChangeDelegates;

/** Last set registrants in an arbitration registration */
var array<OnlineArbitrationRegistrant> ArbitrationList;

/** Holds the delegate and the last accepted invite for a player */
struct native InviteData
{
	/** The per user delegates for game invites */
	var delegate<OnGameInviteAccepted> InviteDelegate;
	/** Cached invite data for the player */
	var const native transient pointer InviteData{XINVITE_INFO};
	/** Game search results associated with this invite */
	var const OnlineGameSearch InviteSearch;
};

/** The cached data for the players */
var InviteData InviteCache[4];

/** Used to notify subscribers when the player changes their (non-game) profile */
var delegate<OnProfileDataChanged> ProfileDataChangedDelegates[4];

/** Whether to log arbitration data or not */
var config bool bShouldLogArbitrationData;

/** Whether to log stats (including true skill) data or not */
var config bool bShouldLogStatsData;

/**
 * Delegate used in login notifications
 */
delegate OnLoginChange();

/**
 * Delegate used to notify when a login request was cancelled by the user
 */
delegate OnLoginCancelled();

/**
 * Delegate used in mute list change notifications
 */
delegate OnMutingChange();

/**
 * Delegate used in friends list change notifications
 */
delegate OnFriendsChange();

/**
 * Called from engine start up code to allow the subsystem to initialize
 *
 * @return TRUE if the initialization was successful, FALSE otherwise
 */
native event bool Init();

/**
 * Displays the UI that prompts the user for their login credentials. Each
 * platform handles the authentication of the user's data.
 *
 * @param bShowOnlineOnly whether to only display online enabled profiles or not
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowLoginUI(optional bool bShowOnlineOnly = false);

/**
 * Fetches the login status for a given player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the enum value of their status
 */
native function ELoginStatus GetLoginStatus(byte LocalUserNum);

/**
 * Gets the platform specific unique id for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the byte array that will receive the id
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool GetUniquePlayerId(byte LocalUserNum,out UniqueNetId PlayerId);

/**
 * Reads the player's nick name from the online service
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return a string containing the players nick name
 */
native function string GetPlayerNickname(byte LocalUserNum);

/**
 * Determines whether the player is allowed to play online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanPlayOnline(byte LocalUserNum);

/**
 * Determines whether the player is allowed to use voice or text chat online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanCommunicate(byte LocalUserNum);

/**
 * Determines whether the player is allowed to download user created content
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanDownloadUserContent(byte LocalUserNum);

/**
 * Determines whether the player is allowed to buy content online
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanPurchaseContent(byte LocalUserNum);

/**
 * Determines whether the player is allowed to view other people's player profile
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanViewPlayerProfiles(byte LocalUserNum);

/**
 * Determines whether the player is allowed to have their online presence
 * information shown to remote clients
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return the Privilege level that is enabled
 */
native function EFeaturePrivilegeLevel CanShowPresenceInformation(byte LocalUserNum);

/**
 * Checks that a unique player id is part of the specified user's friends list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player being checked
 *
 * @return TRUE if a member of their friends list, FALSE otherwise
 */
native function bool IsFriend(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Checks that whether a group of player ids are among the specified player's
 * friends
 *
 * @param LocalUserNum the controller number of the associated user
 * @param Query an array of players to check for being included on the friends list
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool AreAnyFriends(byte LocalUserNum,out array<FriendsQuery> Query);

/**
 * Checks that a unique player id is on the specified user's mute list
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player being checked
 *
 * @return TRUE if the player should be muted, FALSE otherwise
 */
native function bool IsMuted(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Displays the UI that shows a user's list of friends
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowFriendsUI(byte LocalUserNum);

/**
 * Displays the UI that shows a user's list of friends
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player being invited
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowFriendsInviteUI(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Sets the delegate used to notify the gameplay code that a login changed
 *
 * @param LoginDelegate the delegate to use for notifications
 * @param LocalUserNum whether to watch for changes on a specific slot or all slots
 */
function SetLoginChangeDelegate(delegate<OnLoginChange> LoginDelegate,optional byte LocalUserNum = 255)
{
	local int AddIndex;

	// If this is for any login change
	if (LocalUserNum == 255)
	{
		// Add this delegate to the array if not already present
		if (AllLoginDelegates.Delegates.Find(LoginDelegate) == INDEX_NONE)
		{
			AddIndex = AllLoginDelegates.Delegates.Length;
			AllLoginDelegates.Delegates.Length = AllLoginDelegates.Delegates.Length + 1;
			AllLoginDelegates.Delegates[AddIndex] = LoginDelegate;
		}
	}
	// Make sure it's within range
	else if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Add this delegate to the array if not already present
		if (PlayerLoginDelegates[LocalUserNum].Delegates.Find(LoginDelegate) == INDEX_NONE)
		{
			AddIndex = PlayerLoginDelegates[LocalUserNum].Delegates.Length;
			PlayerLoginDelegates[LocalUserNum].Delegates.Length = PlayerLoginDelegates[LocalUserNum].Delegates.Length + 1;
			PlayerLoginDelegates[LocalUserNum].Delegates[AddIndex] = LoginDelegate;
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetLoginChangeDelegate()");
	}
}

/**
 * Removes the specified delegate from the notification list
 *
 * @param LoginDelegate the delegate to use for notifications
 * @param LocalUserNum whether to watch for changes on a specific slot or all slots
 */
function ClearLoginChangeDelegate(delegate<OnLoginChange> LoginDelegate,optional byte LocalUserNum = 255)
{
	local int RemoveIndex;

	// If this is for any login change
	if (LocalUserNum == 255)
	{
		// Remove this delegate from the array if found
		RemoveIndex = AllLoginDelegates.Delegates.Find(LoginDelegate);
		if (RemoveIndex != INDEX_NONE)
		{
			AllLoginDelegates.Delegates.Remove(RemoveIndex,1);
		}
	}
	// Make sure it's within range and remove the per player login index
	else if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Remove this delegate from the array if found
		RemoveIndex = PlayerLoginDelegates[LocalUserNum].Delegates.Find(LoginDelegate);
		if (RemoveIndex != INDEX_NONE)
		{
			PlayerLoginDelegates[LocalUserNum].Delegates.Remove(RemoveIndex,1);
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to ClearLoginChangeDelegate()");
	}
}

/**
 * Adds a delegate to the list of delegates that are fired when a login is cancelled
 *
 * @param CancelledDelegate the delegate to add to the list
 */
function SetLoginCancelledDelegate(delegate<OnLoginCancelled> CancelledDelegate)
{
	local int AddIndex;

	// Add this delegate to the array if not already present
	if (LoginCancelledDelegates.Find(CancelledDelegate) == INDEX_NONE)
	{
		AddIndex = LoginCancelledDelegates.Length;
		LoginCancelledDelegates.Length = LoginCancelledDelegates.Length + 1;
		LoginCancelledDelegates[AddIndex] = CancelledDelegate;
	}
}

/**
 * Removes the specified delegate from the notification list
 *
 * @param CancelledDelegate the delegate to remove fromt he list
 */
function ClearLoginCancelledDelegate(delegate<OnLoginCancelled> CancelledDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = LoginCancelledDelegates.Find(CancelledDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		LoginCancelledDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Sets the delegate used to notify the gameplay code that a muting list changed
 *
 * @param MutingDelegate the delegate to use for notifications
 */
function SetMutingChangeDelegate(delegate<OnMutingChange> MutingDelegate)
{
	OnMutingChange = MutingDelegate;
}

/**
 * Sets the delegate used to notify the gameplay code that a friends list changed
 *
 * @param FriendsDelegate the delegate to use for notifications
 */
function SetFriendsChangeDelegate(delegate<OnFriendsChange> FriendsDelegate)
{
	OnFriendsChange = FriendsDelegate;
}

/**
 * Displays the UI that allows a player to give feedback on another player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player having feedback given for
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowFeedbackUI(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Displays the gamer card UI for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PlayerId the id of the player to show the gamer card of
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowGamerCardUI(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Displays the messages UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowMessagesUI(byte LocalUserNum);

/**
 * Displays the achievements UI for a player
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowAchievementsUI(byte LocalUserNum);

/**
 * Displays the UI that shows the player list
 *
 * @param LocalUserNum the controller number of the associated user
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowPlayersUI(byte LocalUserNum);

/**
 * Displays the UI that shows the keyboard for inputing text
 *
 * @param LocalUserNum the controller number of the associated user
 * @param TitleText the title to display to the user
 * @param DescriptionText the text telling the user what to input
 * @param bShouldValidate whether to apply the string validation API after input or not
 * @param DefaultText the default string to display
 * @param MaxResultLength the maximum length string expected to be filled in
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowKeyboardUI(byte LocalUserNum,string TitleText,
	string DescriptionText,optional bool bShouldValidate = true,
	optional string DefaultText,optional int MaxResultLength = 256);

/**
 * Sets the delegate used to notify the gameplay code that the user has completed
 * their keyboard input
 *
 * @param InputDelegate the delegate to use for notifications
 */
function SetKeyboardInputDoneDelegate(delegate<OnKeyboardInputComplete> InputDelegate)
{
	OnKeyboardInputComplete = InputDelegate;
}

/**
 * Delegate used when the keyboard input request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnKeyboardInputComplete(bool bWasSuccessful);

/**
 * Fetches the results of the input
 *
 * @return the string entered by the user
 */
function string GetKeyboardInputResults()
{
	return KeyboardInputResults;
}

/**
 * Determines if the ethernet link is connected or not
 */
native function bool HasLinkConnection();

/**
 * Delegate fired when the network link status changes
 *
 * @param bIsConnected whether the link is currently connected or not
 */
delegate OnLinkStatusChange(bool bIsConnected);

/**
 * Sets the delegate used to notify the gameplay code that link status changed
 *
 * @param LinkStatusDelegate the delegate to use for notifications
 */
function SetLinkStatusChangeDelegate(delegate<OnLinkStatusChange> LinkStatusDelegate)
{
	local int AddIndex;
	// Only add to the list once
	if (LinkStatusChangeDelegates.Find(LinkStatusDelegate) == INDEX_NONE)
	{
		AddIndex = LinkStatusChangeDelegates.Length;
		LinkStatusChangeDelegates.Length = ConnectionStatusChangeDelegates.Length + 1;
		LinkStatusChangeDelegates[AddIndex] = LinkStatusDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param LinkStatusDelegate the delegate to remove
 */
function ClearLinkStatusChangeDelegate(delegate<OnLinkStatusChange> LinkStatusDelegate)
{
	local int RemoveIndex;
	// See if the specified delegate is in the list
	RemoveIndex = LinkStatusChangeDelegates.Find(LinkStatusDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		LinkStatusChangeDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Delegate fired when an external UI display state changes (opening/closing)
 *
 * @param bIsOpening whether the external UI is opening or closing
 */
delegate OnExternalUIChange(bool bIsOpening);

/**
 * Sets the delegate used to notify the gameplay code that external UI state
 * changed (opened/closed)
 *
 * @param ExternalUIDelegate the delegate to use for notifications
 */
function SetExternalUIChangeDelegate(delegate<OnExternalUIChange> ExternalUIDelegate)
{
	local int AddIndex;
	// Add this delegate to the array if not already present
	if (ExternalUIChangeDelegates.Find(ExternalUIDelegate) == INDEX_NONE)
	{
		AddIndex = ExternalUIChangeDelegates.Length;
		ExternalUIChangeDelegates.Length = ExternalUIChangeDelegates.Length + 1;
		ExternalUIChangeDelegates[AddIndex] = ExternalUIDelegate;
	}
}

/**
 * Removes the delegate from the notification list
 *
 * @param ExternalUIDelegate the delegate to remove
 */
function ClearExternalUIChangeDelegate(delegate<OnExternalUIChange> ExternalUIDelegate)
{
	local int RemoveIndex;
	RemoveIndex = ExternalUIChangeDelegates.Find(ExternalUIDelegate);
	// Verify that it is in the array
	if (RemoveIndex != INDEX_NONE)
	{
		ExternalUIChangeDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Determines the current notification position setting
 */
function ENetworkNotificationPosition GetNetworkNotificationPosition()
{
	return CurrentNotificationPosition;
}

/**
 * Sets a new position for the network notification icons/images
 *
 * @param NewPos the new location to use
 */
native function SetNetworkNotificationPosition(ENetworkNotificationPosition NewPos);

/**
 * Delegate fired when the controller becomes dis/connected
 *
 * @param ControllerId the id of the controller that changed connection state
 * @param bIsConnected whether the controller connected (true) or disconnected (false)
 */
delegate OnControllerChange(int ControllerId,bool bIsConnected);

/**
 * Sets the delegate used to notify the gameplay code that the controller state changed
 *
 * @param ControllerChangeDelegate the delegate to use for notifications
 */
function SetControllerChangeDelegate(delegate<OnControllerChange> ControllerChangeDelegate)
{
	local int AddIndex;
	// Add this delegate to the array if not already present
	if (ControllerChangeDelegates.Find(ControllerChangeDelegate) == INDEX_NONE)
	{
		AddIndex = ControllerChangeDelegates.Length;
		ControllerChangeDelegates.Length = ControllerChangeDelegates.Length + 1;
		ControllerChangeDelegates[AddIndex] = ControllerChangeDelegate;
	}
}

/**
 * Removes the delegate used to notify the gameplay code that the controller state changed
 *
 * @param ControllerChangeDelegate the delegate to remove
 */
function ClearControllerChangeDelegate(delegate<OnControllerChange> ControllerChangeDelegate)
{
	local int RemoveIndex;
	RemoveIndex = ControllerChangeDelegates.Find(ControllerChangeDelegate);
	// Verify that it is in the array
	if (RemoveIndex != INDEX_NONE)
	{
		ControllerChangeDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Determines if the specified controller is connected or not
 *
 * @param ControllerId the controller to query
 *
 * @return true if connected, false otherwise
 */
native function bool IsControllerConnected(int ControllerId);

/**
 * Delegate fire when the online server connection state changes
 *
 * @param ConnectionStatus the new connection status
 */
delegate OnConnectionStatusChange(EOnlineServerConnectionStatus ConnectionStatus);

/**
 * Adds the delegate to the list to be notified when the connection status changes
 *
 * @param ConnectionStatusDelegate the delegate to add
 */
function SetConnectionStatusChangeDelegate(delegate<OnConnectionStatusChange> ConnectionStatusDelegate)
{
	local int AddIndex;
	// Only add to the list once
	if (ConnectionStatusChangeDelegates.Find(ConnectionStatusDelegate) == INDEX_NONE)
	{
		AddIndex = ConnectionStatusChangeDelegates.Length;
		ConnectionStatusChangeDelegates.Length = ConnectionStatusChangeDelegates.Length + 1;
		ConnectionStatusChangeDelegates[AddIndex] = ConnectionStatusDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param ConnectionStatusDelegate the delegate to remove
 */
function ClearConnectionStatusChangeDelegate(delegate<OnConnectionStatusChange> ConnectionStatusDelegate)
{
	local int RemoveIndex;
	// See if the specified delegate is in the list
	RemoveIndex = ConnectionStatusChangeDelegates.Find(ConnectionStatusDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		ConnectionStatusChangeDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Determines the NAT type the player is using
 */
native function ENATType GetNATType();

/**
 * Creates an online game based upon the settings object specified
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
function SetCreateOnlineGameCompleteDelegate(delegate<OnCreateOnlineGameComplete> CreateOnlineGameCompleteDelegate)
{
	OnCreateOnlineGameComplete = CreateOnlineGameCompleteDelegate;
}

/** Returns the currently set game settings */
function OnlineGameSettings GetGameSettings()
{
	return GameSettings;
}

/**
 * Destroys the current online game
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
function SetDestroyOnlineGameCompleteDelegate(delegate<OnDestroyOnlineGameComplete> DestroyOnlineGameCompleteDelegate)
{
	OnDestroyOnlineGameComplete = DestroyOnlineGameCompleteDelegate;
}

/**
 * Searches for games matching the settings specified
 *
 * @param SearchingPlayerNum the index of the player searching for a match
 * @param SearchSettings the desired settings that the returned sessions will have
 *
 * @return true if successful destroying the session, false otherwsie
 */
native function bool FindOnlineGames(byte SearchingPlayerNum,OnlineGameSearch SearchSettings);

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
function SetFindOnlineGamesCompleteDelegate(delegate<OnFindOnlineGamesComplete> FindOnlineGamesCompleteDelegate)
{
	local int AddIndex;
	// Used to forward the event to the list
	OnFindOnlineGamesComplete = MulticastFindOnlineGamesComplete;
	// Only add to the list once
	if (FindOnlineGamesCompleteDelegates.Find(FindOnlineGamesCompleteDelegate) == INDEX_NONE)
	{
		AddIndex = FindOnlineGamesCompleteDelegates.Length;
		FindOnlineGamesCompleteDelegates.Length = FindOnlineGamesCompleteDelegates.Length + 1;
		FindOnlineGamesCompleteDelegates[AddIndex] = FindOnlineGamesCompleteDelegate;
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
 * Local version of the delegate that sends to the array of subscribers
 *
 * @param bWasSuccessful whether the call completed or not
 */
function MulticastFindOnlineGamesComplete(bool bWasSuccessful)
{
	local int Index;
	local delegate<OnFindOnlineGamesComplete> Subscriber;

	// Loop through and notify all subscribed delegates
	for (Index = 0; Index < FindOnlineGamesCompleteDelegates.Length; Index++)
	{
		Subscriber = FindOnlineGamesCompleteDelegates[Index];
		Subscriber(bWasSuccessful);
	}
}

/** Returns the currently set game search object */
function OnlineGameSearch GetGameSearch()
{
	return GameSearch;
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
 * @return true if the call was successful, false otherwise
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
function SetJoinOnlineGameCompleteDelegate(delegate<OnJoinOnlineGameComplete> JoinOnlineGameCompleteDelegate)
{
	OnJoinOnlineGameComplete = JoinOnlineGameCompleteDelegate;
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
 * @param PlayerId the player to register with the online service
 * @param bWasInvited whether the player was invited or is coming via a search
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool RegisterPlayer(UniqueNetId PlayerId,bool bWasInvited);

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
function SetRegisterPlayerCompleteDelegate(delegate<OnRegisterPlayerComplete> RegisterPlayerCompleteDelegate)
{
	OnRegisterPlayerComplete = RegisterPlayerCompleteDelegate;
}

/**
 * Unregisters a player with the online service as being part of the online game
 *
 * @param PlayerId the player to unregister with the online service
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool UnregisterPlayer(UniqueNetId PlayerId);

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
function SetUnregisterPlayerCompleteDelegate(delegate<OnUnregisterPlayerComplete> UnregisterPlayerCompleteDelegate)
{
	OnUnregisterPlayerComplete = UnregisterPlayerCompleteDelegate;
}

/**
 * Reads the online profile settings for a given user
 *
 * @param LocalUserNum the user that we are reading the data for
 * @param ProfileSettings the object to copy the results to and contains the list of items to read
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool ReadProfileSettings(byte LocalUserNum,OnlineProfileSettings ProfileSettings);

/**
 * Delegate used when the last read profile settings request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnReadProfileSettingsComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the last read request has completed
 *
 * @param LocalUserNum which user to watch for read complete notifications
 * @param ReadProfileSettingsCompleteDelegate the delegate to use for notifications
 */
function SetReadProfileSettingsCompleteDelegate(byte LocalUserNum,delegate<OnReadProfileSettingsComplete> ReadProfileSettingsCompleteDelegate)
{
	local int AddIndex;

	// Make sure the user is valid
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Add this delegate to the array if not already present
		if (ProfileCache[LocalUserNum].ReadDelegates.Find(ReadProfileSettingsCompleteDelegate) == INDEX_NONE)
		{
			AddIndex = ProfileCache[LocalUserNum].ReadDelegates.Length;
			ProfileCache[LocalUserNum].ReadDelegates.Length = ProfileCache[LocalUserNum].ReadDelegates.Length + 1;
			ProfileCache[LocalUserNum].ReadDelegates[AddIndex] = ReadProfileSettingsCompleteDelegate;
		}
		// A bit of a hack, but set our per user one which will forward the calls
		switch (LocalUserNum)
		{
			case 0:
				ProfileCache[LocalUserNum].ReadDelegate = ProfileReadNotify1;
				break;
			case 1:
				ProfileCache[LocalUserNum].ReadDelegate = ProfileReadNotify2;
				break;
			case 2:
				ProfileCache[LocalUserNum].ReadDelegate = ProfileReadNotify3;
				break;
			case 3:
				ProfileCache[LocalUserNum].ReadDelegate = ProfileReadNotify4;
				break;
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetReadProfileSettingsCompleteDelegate()");
	}
}

/**
 * Searches the existing set of delegates for the one specified and removes it
 * from the list
 *
 * @param LocalUserNum which user to watch for read complete notifications
 * @param ReadProfileSettingsCompleteDelegate the delegate to find and clear
 */
function ClearReadProfileSettingsCompleteDelegate(byte LocalUserNum,delegate<OnReadProfileSettingsComplete> ReadProfileSettingsCompleteDelegate)
{
	local int RemoveIndex;

	// Make sure the user is valid
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		RemoveIndex = ProfileCache[LocalUserNum].ReadDelegates.Find(ReadProfileSettingsCompleteDelegate);
		// Remove this delegate from the array if found
		if (RemoveIndex != INDEX_NONE)
		{
			ProfileCache[LocalUserNum].ReadDelegates.Remove(RemoveIndex,1);
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to ClearReadProfileSettingsCompleteDelegate()");
	}
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param LocalUserNum the user that had the read
 * @param bWasSuccessful whether the read completed ok or not
 */
function ProfileReadNotify(int LocalUserNum,bool bWasSuccessful)
{
	local int Index;
	local delegate<OnReadProfileSettingsComplete> Subscriber;

	// Loop through and notify all subscribed delegates
	for (Index = 0; Index < ProfileCache[LocalUserNum].ReadDelegates.Length; Index++)
	{
		Subscriber = ProfileCache[LocalUserNum].ReadDelegates[Index];
		Subscriber(bWasSuccessful);
	}
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function ProfileReadNotify1(bool bWasSuccessful)
{
	ProfileReadNotify(0,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function ProfileReadNotify2(bool bWasSuccessful)
{
	ProfileReadNotify(1,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function ProfileReadNotify3(bool bWasSuccessful)
{
	ProfileReadNotify(2,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function ProfileReadNotify4(bool bWasSuccessful)
{
	ProfileReadNotify(3,bWasSuccessful);
}

/**
 * Writes the online profile settings for a given user to the online data store
 *
 * @param LocalUserNum the user that we are writing the data for
 * @param ProfileSettings the list of settings to write out
 *
 * @return true if the call succeeds, false otherwise
 */
native function bool WriteProfileSettings(byte LocalUserNum,OnlineProfileSettings ProfileSettings);

/**
 * Delegate used when the last write profile settings request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnWriteProfileSettingsComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the last write request has completed
 *
 * @param LocalUserNum which user to watch for write complete notifications
 * @param WriteProfileSettingsCompleteDelegate the delegate to use for notifications
 */
function SetWriteProfileSettingsCompleteDelegate(byte LocalUserNum,delegate<OnWriteProfileSettingsComplete> WriteProfileSettingsCompleteDelegate)
{
	// Make sure the user is valid
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		ProfileCache[LocalUserNum].WriteDelegate = WriteProfileSettingsCompleteDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetWriteProfileSettingsCompleteDelegate()");
	}
}

/**
 * Sets a rich presence information to use for the specified player
 *
 * @param LocalUserNum the controller number of the associated user
 * @param PresenceMode the rich presence mode to use
 * @param LocalizedStringSettings the list of localized string settings to set
 * @param Properties the list of properties to set
 */
native function SetRichPresence(byte LocalUserNum,int PresenceMode,
	const out array<LocalizedStringSetting> LocalizedStringSettings,
	const out array<SettingsProperty> Properties);

/**
 * Displays the invite ui
 *
 * @param LocalUserNum the local user sending the invite
 * @param InviteText the string to prefill the UI with
 */
native function bool ShowInviteUI(byte LocalUserNum,optional string InviteText);

/**
 * Displays the marketplace UI for content
 *
 * @param LocalUserNum the local user viewing available content
 */
native function bool ShowContentMarketplaceUI(byte LocalUserNum);

/**
 * Displays the marketplace UI for memberships
 *
 * @param LocalUserNum the local user viewing available memberships
 */
native function bool ShowMembershipMarketplaceUI(byte LocalUserNum);

/**
 * Displays the UI that allows the user to choose which device to save content to
 *
 * @param LocalUserNum the controller number of the associated user
 * @param SizeNeeded the size of the data to be saved in bytes
 * @param bForceShowUI true to always show the UI, false to only show the
 *		  UI if there are multiple valid choices
 *
 * @return TRUE if it was able to show the UI, FALSE if it failed
 */
native function bool ShowDeviceSelectionUI(byte LocalUserNum,int SizeNeeded,bool bForceShowUI = false);

/**
 * Sets the delegate used to notify the gameplay code that the user has completed
 * their device selection
 *
 * @param LocalUserNum the controller number of the associated user
 * @param DeviceDelegate the delegate to use for notifications
 */
function SetDeviceSelectionDoneDelegate(byte LocalUserNum,delegate<OnDeviceSelectionComplete> DeviceDelegate)
{
	local int AddIndex;

	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Add this delegate to the array if not already present
		if (DeviceCache[LocalUserNum].DeviceSelectionDelegates.Find(DeviceDelegate) == INDEX_NONE)
		{
			AddIndex = DeviceCache[LocalUserNum].DeviceSelectionDelegates.Length;
			DeviceCache[LocalUserNum].DeviceSelectionDelegates.Length = DeviceCache[LocalUserNum].DeviceSelectionDelegates.Length + 1;
			DeviceCache[LocalUserNum].DeviceSelectionDelegates[AddIndex] = DeviceDelegate;
		}
		// A bit of a hack, but set our per user one which will forward the calls
		switch (LocalUserNum)
		{
			case 0:
				DeviceCache[LocalUserNum].DeviceSelectionMulticast = DeviceSelectionMulticast1;
				break;
			case 1:
				DeviceCache[LocalUserNum].DeviceSelectionMulticast = DeviceSelectionMulticast2;
				break;
			case 2:
				DeviceCache[LocalUserNum].DeviceSelectionMulticast = DeviceSelectionMulticast3;
				break;
			case 3:
				DeviceCache[LocalUserNum].DeviceSelectionMulticast = DeviceSelectionMulticast4;
				break;
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetDeviceSelectionDoneDelegate()");
	}
}

/**
 * Removes the specified delegate from the list of callbacks
 *
 * @param LocalUserNum the controller number of the associated user
 * @param DeviceDelegate the delegate to use for notifications
 */
function ClearDeviceSelectionDoneDelegate(byte LocalUserNum,delegate<OnDeviceSelectionComplete> DeviceDelegate)
{
	local int RemoveIndex;

	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		// Find the delegate and remove it
		RemoveIndex = DeviceCache[LocalUserNum].DeviceSelectionDelegates.Find(DeviceDelegate);
		if (RemoveIndex != INDEX_NONE)
		{
			DeviceCache[LocalUserNum].DeviceSelectionDelegates.Remove(RemoveIndex,1);
		}
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to ClearDeviceSelectionDoneDelegate()");
	}
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function DeviceSelectionMulticast1(bool bWasSuccessful)
{
	DeviceSelectionMulticast(0,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function DeviceSelectionMulticast2(bool bWasSuccessful)
{
	DeviceSelectionMulticast(1,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function DeviceSelectionMulticast3(bool bWasSuccessful)
{
	DeviceSelectionMulticast(2,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param bWasSuccessful whether the call succeeded or not
 */
function DeviceSelectionMulticast4(bool bWasSuccessful)
{
	DeviceSelectionMulticast(3,bWasSuccessful);
}

/**
 * Iterates the list of delegates and fires them off
 *
 * @param LocalUserNum the user to fire the delegates for
 * @param bWasSuccessful whether the called succeeded or not
 */
function DeviceSelectionMulticast(byte LocalUserNum,bool bWasSuccessful)
{
	local int Index;
	local delegate<OnDeviceSelectionComplete> Subscriber;

	// Loop through and notify all subscribed delegates
	for (Index = 0; Index < DeviceCache[LocalUserNum].DeviceSelectionDelegates.Length; Index++)
	{
		Subscriber = DeviceCache[LocalUserNum].DeviceSelectionDelegates[Index];
		Subscriber(bWasSuccessful);
	}
}

/**
 * Fetches the results of the device selection
 *
 * @param LocalPlayerNum the player to check the results for
 * @param DeviceName out param that gets a copy of the string
 *
 * @return the ID of the device that was selected
 */
native function int GetDeviceSelectionResults(byte LocalPlayerNum,out string DeviceName);

/**
 * Delegate used when the device selection request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnDeviceSelectionComplete(bool bWasSuccessful);

/**
 * Checks the device id to determine if it is still valid (could be removed)
 *
 * @param DeviceId the device to check
 *
 * @return true if valid, false otherwise
 */
native function bool IsDeviceValid(int DeviceId);

/**
 * Unlocks the specified achievement for the specified user
 *
 * @param LocalUserNum the controller number of the associated user
 * @param AchievementId the id of the achievement to unlock
 *
 * @return TRUE if the call worked, FALSE otherwise
 */
native function bool UnlockAchievement(byte LocalUserNum,int AchievementId);

/**
 * Sets the delegate used to notify the gameplay code that the achievement unlocking has completed
 *
 * @param LocalUserNum which user to watch for read complete notifications
 * @param UnlockAchievementCompleteDelegate the delegate to use for notifications
 */
function SetUnlockAchievementCompleteDelegate(byte LocalUserNum,delegate<OnUnlockAchievementComplete> UnlockAchievementCompleteDelegate)
{
	// Make sure the user is valid
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		AchievementDelegates[LocalUserNum] = UnlockAchievementCompleteDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetUnlockAchievementCompleteDelegate()");
	}
}

/**
 * Delegate used when the achievement unlocking has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnUnlockAchievementComplete(bool bWasSuccessful);

/**
 * Unlocks a gamer picture for the local user
 *
 * @param LocalUserNum the user to unlock the picture for
 * @param PictureId the id of the picture to unlock
 */
native function bool UnlockGamerPicture(byte LocalUserNum,int PictureId);

/**
 * Called when an external change to player profile data has occured
 */
delegate OnProfileDataChanged();

/**
 * Sets the delegate used to notify the gameplay code that someone has changed their profile data externally
 *
 * @param LocalUserNum the user the delegate is interested in
 * @param ProfileDataChangedDelegate the delegate to use for notifications
 */
function SetProfileDataChangedDelegate(byte LocalUserNum,delegate<OnProfileDataChanged> ProfileDataChangedDelegate)
{
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		ProfileDataChangedDelegates[LocalUserNum] = ProfileDataChangedDelegate;
	}
	else
	{
		`Log("Invalid user id ("$LocalUserNum$") specified for SetProfileDataChangedDelegate()");
	}
}

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
function SetStartOnlineGameCompleteDelegate(delegate<OnStartOnlineGameComplete> StartOnlineGameCompleteDelegate)
{
	OnStartOnlineGameComplete = StartOnlineGameCompleteDelegate;
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
function SetEndOnlineGameCompleteDelegate(delegate<OnEndOnlineGameComplete> EndOnlineGameCompleteDelegate)
{
	OnEndOnlineGameComplete = EndOnlineGameCompleteDelegate;
}

/**
 * Returns the current state of the online game
 */
function EOnlineGameState GetOnlineGameState()
{
	return CurrentGameState;
}

/**
 * Tells the game to register with the underlying arbitration server if available
 */
native function bool RegisterForArbitration();

/**
 * Writes the final arbitration information for a match
 */
native function bool WriteArbitrationData();

/**
 * Tells the online subsystem to accept the game invite that is currently pending
 *
 * @param LocalUserNum the local user accepting the invite
 */
native function bool AcceptGameInvite(byte LocalUserNum);

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
function SetArbitrationRegistrationCompleteDelegate(delegate<OnArbitrationRegistrationComplete> ArbitrationRegistrationCompleteDelegate)
{
	OnArbitrationRegistrationComplete = ArbitrationRegistrationCompleteDelegate;
}

/**
 * Returns the list of arbitrated players for the arbitrated session
 */
function array<OnlineArbitrationRegistrant> GetArbitratedPlayers()
{
	return ArbitrationList;
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
native function bool ReadFriendsList(byte LocalUserNum,optional int Count,optional int StartingAt);

/**
 * Delegate used when the friends read request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnReadFriendsComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the friends read request has completed
 *
 * @param ReadFriendsCompleteDelegate the delegate to use for notifications
 */
function SetReadFriendsCompleteDelegate(byte LocalUserNum,delegate<OnReadFriendsComplete> ReadFriendsCompleteDelegate)
{
	FriendsCache[LocalUserNum].ReadCompleteDelegate = ReadFriendsCompleteDelegate;
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
 * @return ERS_Done if the read has completed, otherwise one of the other states
 */
native function EOnlineEnumerationReadState GetFriendsList(byte LocalUserNum,out array<OnlineFriend> Friends,optional int Count,optional int StartingAt);

/**
 * Sets the delegate used to notify the gameplay code when a game invite has been accepted
 *
 * @param LocalUserNum the user to request notification for
 * @param GameInviteAcceptedDelegate the delegate to use for notifications
 */
function SetGameInviteAcceptedDelegate(byte LocalUserNum,delegate<OnGameInviteAccepted> GameInviteAcceptedDelegate)
{
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		InviteCache[LocalUserNum].InviteDelegate = GameInviteAcceptedDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetGameInviteAcceptedDelegate()");
	}
}

/**
 * Called when a user accepts a game invitation. Allows the gameplay code a chance
 * to clean up any existing state before accepting the invite. The invite must be
 * accepted by calling AcceptGameInvite() on the OnlineGameInterface after clean up
 * has completed
 *
 * @param InviteSettings the settings for the game we're joining via invite
 */
delegate OnGameInviteAccepted(OnlineGameSettings InviteSettings);

/**
 * Delegate used in content change (add or deletion) notifications
 * for any user
 */
delegate OnContentChange();

/**
 * Sets the delegate used to notify the gameplay code that (downloaded) content changed
 *
 * @param Content Delegate the delegate to use for notifications
 * @param LocalUserNum whether to watch for changes on a specific slot or all slots
 */
function SetContentChangeDelegate(delegate<OnContentChange> ContentDelegate, optional byte LocalUserNum = 255)
{
	if (LocalUserNum == 255)
	{
		OnContentChange = ContentDelegate;
	}
	// Make sure it's within range
	else if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		ContentCache[LocalUserNum].ContentChangeDelegate = ContentDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetContentChangeDelegate()");
	}
}

/**
 * Delegate used when the content read request has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnReadContentComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the content read request has completed
 *
 * @param LocalUserNum the user to read the content list of
 * @param ReadContentCompleteDelegate the delegate to use for notifications
 */
function SetReadContentComplete(byte LocalUserNum, delegate<OnReadContentComplete> ReadContentCompleteDelegate)
{
	ContentCache[LocalUserNum].ReadCompleteDelegate = ReadContentCompleteDelegate;
}

/**
 * Starts an async task that retrieves the list of downloaded content for the player.
 *
 * @param LocalUserNum The user to read the content list of
 *
 * @return true if the read request was issued successfully, false otherwise
 */
native function bool ReadContentList(byte LocalUserNum);

/**
 * Retrieve the list of content the given user has downloaded or otherwise retrieved
 * to the local console.

 * @param LocalUserNum The user to read the content list of
 * @param ContentList The out array that receives the list of all content
 *
 * @return ERS_Done if the read has completed, otherwise one of the other states
 */
native function EOnlineEnumerationReadState GetContentList(byte LocalUserNum, out array<OnlineContent> ContentList);

/**
 * Asks the online system for the number of new and total content downloads
 *
 * @param LocalUserNum the user to check the content download availability for
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool QueryAvailableDownloads(byte LocalUserNum);

/**
 * Called once the download query completes
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnQueryAvailableDownloadsComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the content download query has completed
 *
 * @param LocalUserNum the user to check the content download availability for
 * @param ReadContentCompleteDelegate the delegate to use for notifications
 */
function SetQueryAvailableDownloadsComplete(byte LocalUserNum,delegate<OnQueryAvailableDownloadsComplete> QueryDownloadsDelegate)
{
	// Make sure it's within range
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		ContentCache[LocalUserNum].QueryDownloadsDelegate = QueryDownloadsDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetQueryAvailableDownloadsComplete()");
	}
}

/**
 * Returns the number of new and total downloads available for the user
 *
 * @param LocalUserNum the user to check the content download availability for
 * @param NewDownloads out value of the number of new downloads available
 * @param TotalDownloads out value of the number of total downloads available
 */
function GetAvailableDownloadCounts(byte LocalUserNum,out int NewDownloads,out int TotalDownloads)
{
	// Make sure it's within range
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		NewDownloads = ContentCache[LocalUserNum].NewDownloadCount;
		TotalDownloads = ContentCache[LocalUserNum].TotalDownloadCount;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to GetAvailableDownloadCounts()");
	}
}

/**
 * Registers the user as a talker
 *
 * @param LocalUserNum the local player index that is a talker
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool RegisterLocalTalker(byte LocalUserNum);

/**
 * Unregisters the user as a talker
 *
 * @param LocalUserNum the local player index to be removed
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool UnregisterLocalTalker(byte LocalUserNum);

/**
 * Registers a remote player as a talker
 *
 * @param PlayerId the unique id of the remote player that is a talker
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool RegisterRemoteTalker(UniqueNetId PlayerId);

/**
 * Unregisters a remote player as a talker
 *
 * @param PlayerId the unique id of the remote player to be removed
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
native function bool UnregisterRemoteTalker(UniqueNetId PlayerId);

/**
 * Determines if the specified player is actively talking into the mic
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
native function bool IsLocalPlayerTalking(byte LocalUserNum);

/**
 * Determines if the specified remote player is actively talking into the mic
 * NOTE: Network latencies will make this not 100% accurate
 *
 * @param PlayerId the unique id of the remote player being queried
 *
 * @return TRUE if the player is talking, FALSE otherwise
 */
native function bool IsRemotePlayerTalking(UniqueNetId PlayerId);

/**
 * Determines if the specified player has a headset connected
 *
 * @param LocalUserNum the local player index being queried
 *
 * @return TRUE if the player has a headset plugged in, FALSE otherwise
 */
native function bool IsHeadsetPresent(byte LocalUserNum);

/**
 * Sets the relative priority for a remote talker. 0 is highest
 *
 * @param LocalUserNum the user that controls the relative priority
 * @param PlayerId the remote talker that is having their priority changed for
 * @param Priority the relative priority to use (0 highest, < 0 is muted)
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
native function bool SetRemoteTalkerPriority(byte LocalUserNum,UniqueNetId PlayerId,int Priority);

/**
 * Mutes a remote talker for the specified local player. NOTE: This is separate
 * from the user's permanent online mute list
 *
 * @param LocalUserNum the user that is muting the remote talker
 * @param PlayerId the remote talker that is being muted
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
native function bool MuteRemoteTalker(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Allows a remote talker to talk to the specified local player. NOTE: This call
 * will fail for remote talkers on the user's permanent online mute list
 *
 * @param LocalUserNum the user that is allowing the remote talker to talk
 * @param PlayerId the remote talker that is being restored to talking
 *
 * @return TRUE if the function succeeds, FALSE otherwise
 */
native function bool UnmuteRemoteTalker(byte LocalUserNum,UniqueNetId PlayerId);

/**
 * Called when a player is talking either locally or remote. This will be called
 * once for each active talker each frame.
 *
 * @param Player the player that is talking
 */
delegate OnPlayerTalking(UniqueNetId Player);

/**
 * Adds a talker delegate to the list of notifications
 *
 * @param TalkerDelegate the delegate to call when a player is talking
 */
function AddPlayerTalkingDelegate(delegate<OnPlayerTalking> TalkerDelegate)
{
	local int AddIndex;
	// Add this delegate to the array if not already present
	if (TalkingDelegates.Find(TalkerDelegate) == INDEX_NONE)
	{
		AddIndex = TalkingDelegates.Length;
		TalkingDelegates.Length = TalkingDelegates.Length + 1;
		TalkingDelegates[AddIndex] = TalkerDelegate;
	}
}

/**
 * Removes a talker delegate to the list of notifications
 *
 * @param TalkerDelegate the delegate to remove from the notification list
 */
function ClearPlayerTalkingDelegate(delegate<OnPlayerTalking> TalkerDelegate)
{
	local int RemoveIndex;
	RemoveIndex = TalkingDelegates.Find(TalkerDelegate);
	// Only remove if found
	if (RemoveIndex != INDEX_NONE)
	{
		TalkingDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Tells the voice system to start tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
native function bool StartSpeechRecognition(byte LocalUserNum);

/**
 * Tells the voice system to stop tracking voice data for speech recognition
 *
 * @param LocalUserNum the local user to recognize voice data for
 *
 * @return true upon success, false otherwise
 */
native function bool StopSpeechRecognition(byte LocalUserNum);

/**
 * Gets the results of the voice recognition
 *
 * @param LocalUserNum the local user to read the results of
 * @param Words the set of words that were recognized by the voice analyzer
 *
 * @return true upon success, false otherwise
 */
native function bool GetRecognitionResults(byte LocalUserNum,out array<SpeechRecognizedWord> Words);

/**
 * Called when speech recognition for a given player has completed. The
 * consumer of the notification can call GetRecognitionResults() to get the
 * words that were recognized
 */
delegate OnRecognitionComplete();

/**
 * Sets the speech recognition notification callback to use for the specified user
 *
 * @param LocalUserNum the local user to receive notifications for
 * @param RecognitionDelegate the delegate to call when recognition is complete
 */
function SetRecognitionCompleteDelegate(byte LocalUserNum,delegate<OnRecognitionComplete> RecognitionDelegate)
{
	// Make sure it's within range
	if (LocalUserNum >= 0 && LocalUserNum < 4)
	{
		SpeechRecognitionDelegates[LocalUserNum] = RecognitionDelegate;
	}
	else
	{
		`warn("Invalid index ("$LocalUserNum$") passed to SetRecognitionCompleteDelegate()");
	}
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
native function bool ReadOnlineStats(const out array<UniqueNetId> Players,OnlineStatsRead StatsRead);

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
native function bool ReadOnlineStatsForFriends(byte LocalUserNum,OnlineStatsRead StatsRead);

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
native function bool ReadOnlineStatsByRank(OnlineStatsRead StatsRead,optional int StartIndex = 1,optional int NumToRead = 100);

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
native function bool ReadOnlineStatsByRankAroundPlayer(byte LocalUserNum,OnlineStatsRead StatsRead,optional int NumRows = 10);

/**
 * Sets the delegate used to notify the gameplay code that the stats read has completed
 *
 * @param ReadOnlineStatsCompleteDelegate the delegate to use for notifications
 */
function SetReadOnlineStatsCompleteDelegate(delegate<OnReadOnlineStatsComplete> ReadOnlineStatsCompleteDelegate)
{
	local int AddIndex;
	// Used to forward the event to the list
	OnReadOnlineStatsComplete = MulticastReadOnlineStatsComplete;
	// Only add to the list once
	if (ReadOnlineStatsCompleteDelegates.Find(ReadOnlineStatsCompleteDelegate) == INDEX_NONE)
	{
		AddIndex = ReadOnlineStatsCompleteDelegates.Length;
		ReadOnlineStatsCompleteDelegates.Length = ReadOnlineStatsCompleteDelegates.Length + 1;
		ReadOnlineStatsCompleteDelegates[AddIndex] = ReadOnlineStatsCompleteDelegate;
	}
}

/**
 * Removes the delegate from the notify list
 *
 * @param ReadOnlineStatsCompleteDelegate the delegate to use for notifications
 */
function ClearReadOnlineStatsCompleteDelegate(delegate<OnReadOnlineStatsComplete> ReadOnlineStatsCompleteDelegate)
{
	local int RemoveIndex;
	// Find it in the list
	RemoveIndex = ReadOnlineStatsCompleteDelegates.Find(ReadOnlineStatsCompleteDelegate);
	// Only remove if found
	if (RemoveIndex != INDEX_NONE)
	{
		ReadOnlineStatsCompleteDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Local version of the delegate that sends to the array of subscribers
 *
 * @param bWasSuccessful whether the call completed or not
 */
function MulticastReadOnlineStatsComplete(bool bWasSuccessful)
{
	local int Index;
	local delegate<OnReadOnlineStatsComplete> Subscriber;

	// Loop through and notify all subscribed delegates
	for (Index = 0; Index < ReadOnlineStatsCompleteDelegates.Length; Index++)
	{
		Subscriber = ReadOnlineStatsCompleteDelegates[Index];
		Subscriber(bWasSuccessful);
	}
}

/**
 * Notifies the interested party that the last stats read has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnReadOnlineStatsComplete(bool bWasSuccessful);

/**
 * Cleans up any platform specific allocated data contained in the stats data
 *
 * @param StatsRead the object to handle per platform clean up on
 */
native function FreeStats(OnlineStatsRead StatsRead);

/**
 * Writes out the stats contained within the stats write object to the online
 * subsystem's cache of stats data. Note the new data replaces the old. It does
 * not write the data to the permanent storage until a FlushOnlineStats() call
 * or a session ends. Stats cannot be written without a session or the write
 * request is ignored. No more than 5 stats views can be written to at a time
 * or the write request is ignored.
 *
 * @param Player the player to write stats for
 * @param StatsWrite the object containing the information to write
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
native function bool WriteOnlineStats(UniqueNetId Player,OnlineStatsWrite StatsWrite);

/**
 * Commits any changes in the online stats cache to the permanent storage
 *
 * @return TRUE if the call is successful, FALSE otherwise
 */
native function bool FlushOnlineStats();

/**
 * Delegate called when the stats flush operation has completed
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
delegate OnFlushOnlineStatsComplete(bool bWasSuccessful);

/**
 * Sets the delegate used to notify the gameplay code that the stats flush has completed
 *
 * @param FlushOnlineStatsCompleteDelegate the delegate to use for notifications
 */
function SetFlushOnlineStatsCompleteDelegate(delegate<OnFlushOnlineStatsComplete> FlushOnlineStatsCompleteDelegate)
{
	OnFlushOnlineStatsComplete = FlushOnlineStatsCompleteDelegate;
}

defaultproperties
{
}
