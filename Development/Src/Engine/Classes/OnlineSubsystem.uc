/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This class holds a set of online interfaces that game code uses to talk
 * with the platform layer's services. The set of services are implemented
 * as interface pointers so that we can mix & match services at run time.
 * This also allows licensees the ability to use part of our base services
 * and provide custom implmentations of others.
 */
class OnlineSubsystem extends Object
	native
	abstract
	inherits(FTickableObject);

/** The interface to use for creating and/or enumerating account information */
var OnlineAccountInterface AccountInterface;

/** The interface for accessing online player methods */
var OnlinePlayerInterface PlayerInterface;

/** The interface for accessing online player extension methods */
var OnlinePlayerInterfaceEx PlayerInterfaceEx;

/** The interface for accessing system wide network functions */
var OnlineSystemInterface SystemInterface;

/** The interface to use for creating, searching for, or destroying online games */
var OnlineGameInterface GameInterface;

/** The interface to use for online content */
var OnlineContentInterface ContentInterface;

/** The interface to use for voice communication */
var OnlineVoiceInterface VoiceInterface;

/** The interface to use for stats read/write operations */
var OnlineStatsInterface StatsInterface;

/** Struct that holds a transient, unique identifier for a player */
struct native UniqueNetId
{
	/** The id used by the network to uniquely identify a player */
	var byte Uid[8];

	structcpptext
	{
		/**
		 * Converts a DWORD into our type in network byte order
		 *
		 * @param Id the value to assign
		 */
		FORCEINLINE FUniqueNetId& operator=(const DWORD Id)
		{
#if WITH_PANORAMA || XBOX
			check(0 && "Bad conversion loses data");
#else
			Uid[0] = (Id >> 24) & 0xFF;
			Uid[1] = (Id >> 16) & 0xFF;
			Uid[2] = (Id >> 8) & 0xFF;
			Uid[3] = Id & 0xFF;
#endif
			return *this;
		}

		/**
		 * Converts the unique id to a DWORD
		 *
		 * @return unique id as a DWORD
		 */
		FORCEINLINE DWORD ToDWORD(void) const
		{
#if WITH_PANORAMA || XBOX
			check(0 && "Bad conversion loses data");
			return 0;
#else
			DWORD D1 = Uid[0];
			DWORD D2 = Uid[1];
			DWORD D3 = Uid[2];
			DWORD D4 = Uid[3];
			return D1 << 24 | D2 << 16 | D3 << 8 | D4;
#endif
		}
	}
};

/** The different login statuses for a player */
enum ELoginStatus
{
	/** Player has not logged in or chosen a local profile */
	LS_NotLoggedIn,
	/** Player is using a local profile but is not logged in */
	LS_UsingLocalProfile,
	/** Player has been validated by the platform specific authentication service */
	LS_LoggedIn
};

/** This enum indicates access to major features in the game (parent controls */
enum EFeaturePrivilegeLevel
{
	/** Parental controls have disabled this feature */
	FPL_Disabled,
	/** Parental controls allow this feature only with people on their friends list */
	FPL_EnabledFriendsOnly,
	/** Parental controls allow this feature everywhere */
	FPL_Enabled
};

/** Used to bulk query the friends list */
struct native FriendsQuery
{
	/** The unique player id to check friends status for */
	var UniqueNetId UniqueId;
	/** Out param indicating whether the player is a friend or not */
	var bool bIsFriend;
};

/** Indicates where network notifications should appear on the screen */
enum ENetworkNotificationPosition
{
	NNP_TopLeft,
	NNP_TopCenter,
	NNP_TopRight,
	NNP_CenterLeft,
	NNP_Center,
	NNP_CenterRight,
	NNP_BottomLeft,
	NNP_BottomCenter,
	NNP_BottomRight
};

/** Enum indicating the current state of the online game (in progress, ended, etc.) */
enum EOnlineGameState
{
	/** An online game has not been created yet */
	OGS_NoSession,
	/** Session has been created and the match hasn't started (pre match lobby) */
	OGS_Pending,
	/** The current session has started. Matches with join in progress disabled are no longer joinable */
	OGS_InProgress,
	/** The session is still valid, but the match is no longer being played (post match lobby) */
	OGS_Ending,
	/** The session is closed and any stats committed */
	OGS_Ended
};

/** The state of an async enumeration (friends, content, etc) read request */
enum EOnlineEnumerationReadState
{
	OERS_NotStarted,
	OERS_InProgress,
	OERS_Done,
	OERS_Failed
};

/** Holds information about a player in a friends list */
struct native OnlineFriend
{
	/** Unique identifier of the friend */
	var const UniqueNetId UniqueId;
	/** Player's nick as published to the online service */
	var const string NickName;
	/** String holding information about the player's game state (cap-ed flag, etc.) */
	var const string PresenceInfo;
	/** Whether the friend is online or not */
	var const bool bIsOnline;
	/** Whether the friend is playing a game or not */
	var const bool bIsPlaying;
	/** Whether the friend is playing the same game or not */
	var const bool bIsPlayingThisGame;
	/** Whether the game the friend is in is joinable or not */
	var const bool bIsJoinable;
	/** Whether the friend can chat via voice or not */
	var const bool bHasVoiceSupport;

	structcpptext
	{
		/**
		 * Zeroes members
		 */
		FOnlineFriend(void)
		{
			appMemzero(this,sizeof(FOnlineFriend));
		}

		/**
		 * Copy constructor. Copies the other into this object
		 *
		 * @param Other the other structure to copy
		 */
		FOnlineFriend(const FOnlineFriend& Other)
		{
			appMemcpy(&UniqueId,&Other.UniqueId,sizeof(FUniqueNetId));
			NickName = Other.NickName;
			PresenceInfo = Other.PresenceInfo;
			bIsOnline = Other.bIsOnline;
			bIsPlaying = Other.bIsPlaying;
			bIsPlayingThisGame = Other.bIsPlayingThisGame;
			bIsJoinable = Other.bIsJoinable;
			bHasVoiceSupport = Other.bHasVoiceSupport;
		}

		/**
		 * Assignment operator. Copies the other into this object
		 *
		 * @param Other the other structure to copy
		 */
		FOnlineFriend& operator=(const FOnlineFriend& Other)
		{
			if (this != &Other)
			{
				appMemcpy(&UniqueId,&Other.UniqueId,sizeof(FUniqueNetId));
				NickName = Other.NickName;
				PresenceInfo = Other.PresenceInfo;
				bIsOnline = Other.bIsOnline;
				bIsPlaying = Other.bIsPlaying;
				bIsPlayingThisGame = Other.bIsPlayingThisGame;
				bIsJoinable = Other.bIsJoinable;
				bHasVoiceSupport = Other.bHasVoiceSupport;
			}
			return *this;
		}
	}
};

/** Holds information about a single piece of downloaded content */
struct native OnlineContent
{
	/** Optional user index that content is downloaded for (-1 means it's not associated with any user) */
	var int UserIndex;
	/** Displayable name of the content */
	var string FriendlyName;
	/** File system usable reference to the content */
	var string ContentPath;
	/** List of packages in the content */
	var array<string> ContentPackages;
	/** List of all non-package files in the content */
	var array<string> ContentFiles;
};

/**
 * Indicates the connection status with the remote online servers
 */
enum EOnlineServerConnectionStatus
{
	/** Gracefully disconnected from the online servers */
	OSCS_NotConnected,
	/** Connected to the online servers just fine */
	OSCS_Connected,
	/** Connection was lost for some reason */
	OSCS_ConnectionDropped,
	/** Can't connect because of missing network connection */
	OSCS_NoNetworkConnection,
	/** Service is temporarily unavailable */
	OSCS_ServiceUnavailable,
	/** An update is required before connecting is possible */
	OSCS_UpdateRequired,
	/** Servers are too busy to handle the request right now */
	OSCS_ServersTooBusy,
	/** Disconnected due to duplicate login */
	OSCS_DuplicateLoginDetected,
	/** Can't connect because of an invalid/unknown user */
	OSCS_InvalidUser
};

/**
 * The various NAT types the player may have
 */
enum ENATType
{
	/** Unable to determine the NAT type */
	NAT_Unknown,
	/** Anyone can join without connectivity problems */
	NAT_Open,
	/** Most can join but might have problems with strict */
	NAT_Moderate,
	/** Will most likely have connectivity problems with strict/moderate */
	NAT_Strict
};

/** Struct holding the information about a single arbitration registrant */
struct native OnlineArbitrationRegistrant
{
	/** Unique id of the machine involved in the arbitrated session */
	var const qword MachineId;
	/** Unique id of the player involved in the arbitrated session */
	var const UniqueNetId PlayerId;
	/** Trust level of the machine/player for the arbitrated session */
	var const int Trustworthiness;
};

/**
 * Holds a word/phrase that was recognized by the speech analyzer
 *
 * @note See VoiceInterface.h to change the native layout of this struct
 */
struct SpeechRecognizedWord
{
	/** The id of the word in the vocabulary */
	var int WordId;
	/** the actual word */
	var string WordText;
	/** How confident the analyzer was in the recognition */
	var float Confidence;
};

/** Indicates the state the LAN beacon is in */
enum ELanBeaconState
{
	/** The lan beacon is disabled */
	LANB_NotUsingLanBeacon,
	/** The lan beacon is responding to client requests for information */
	LANB_Hosting,
	/** The lan beacon is querying servers for information */
	LANB_Searching
};

/**
 * Struct holding information used when writing scoring information that is used
 * to determine a player's skill rating
 */
struct native OnlinePlayerScore
{
	/** The player that this score is for */
	var UniqueNetId PlayerId;
	/** The team that the player is on */
	var int TeamId;
	/** The score for this player */
	var int Score;
};

/** The series of status codes that the account creation method can return */
enum EOnlineAccountCreateStatus
{
	/** Created the account successfully */
	OACS_CreateSuccessful,
	/** Failed but no indication why */
	OACS_UnknownError,
	/** The user name is invalid */
	OACS_InvalidUserName,
	/** The password is invalid */
	OACS_InvalidPassword,
	/** The unique user name is invalid */
	OACS_InvalidUniqueUserName,
	/** The user name is invalid */
	OACS_UniqueUserNameInUse
};

/** Information about a local talker */
struct native LocalTalker
{
	/** Whether this talker is currently registered */
	var bool bHasVoice;
	/** Whether the talker should send network data */
	var bool bHasNetworkedVoice;
	/** Whether the local talker was speaking last frame */
	var bool bWasTalking;
};

/** Information about a remote talker */
struct native RemoteTalker
{
	/** The unique id for this talker */
	var UniqueNetId TalkerId;
	/** Whether the remote talker was speaking last frame */
	var bool bWasTalking;
};

/** Holds the data used in a friend message */
struct native FriendMessage
{
	/** The player that is sending the message */
	var UniqueNetId SendingPlayerId;
	/** The nick name of the player that sent the message */
	var string SendingPlayerNick;
	/** The corresponding message that was sent */
	var string Message;
};

/** Holds any information pertaining to a friend invite */
struct native FriendInvite extends FriendMessage
{
	/** Whether the invite has been accepted or not */
	var bool bWasAccepted;
	/** Whether the invite has been denied or not */
	var bool bWasDenied;
};

cpptext
{
// FTickableObject interface

	/**
	 * Returns whether it is okay to tick this object. E.g. objects being loaded in the background shouldn't be ticked
	 * till they are finalized and unreachable objects cannot be ticked either.
	 *
	 * @return	TRUE if tickable, FALSE otherwise
	 */
	virtual UBOOL IsTickable()
	{
		// We cannot tick objects that are unreachable or are in the process of being loaded in the background.
		return !HasAnyFlags( RF_Unreachable | RF_AsyncLoading );
	}

	/**
	 * Used to determine if an object should be ticked when the game is paused.
	 *
	 * @return always TRUE as networking needs to be ticked even when paused
	 */
	virtual UBOOL IsTickableWhenPaused(void)
	{
		return TRUE;
	}

	/**
	 * Here to complete the interface but needs to be overriden
	 *
	 * @param ignored
	 */
	virtual void Tick(FLOAT)
	{
		check(0 && "Must be overridden");
	}

	/**
	 * Handle downloaded content in a platform-independent way
	 *
	 * @param Content The content descriptor that describes the downloaded content files
	 *
	 * @param return TRUE is successful
	 */
	virtual UBOOL ProcessDownloadedContent(const FOnlineContent& Content);

	/**
	 * Flush downloaded content for all users, making the engine stop using the content
	 *
	 * @param MaxNumUsers Platform specific max number of users to flush (this will iterate over all users from 0 to MaxNumUsers, as well as NO_USER
	 */
	virtual void FlushAllDownloadedContent(INT MaxNumUsers);
}

/**
 * Called from engine start up code to allow the subsystem to initialize
 *
 * @return TRUE if the initialization was successful, FALSE otherwise
 */
event bool Init();

/**
 * Called from native code to assign the account interface
 *
 * @param NewInterface the object to assign as providing the account interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetAccountInterface(Object NewInterface)
{
	AccountInterface = OnlineAccountInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return AccountInterface != None;
}

/**
 * Called from native code to assign the player interface
 *
 * @param NewInterface the object to assign as providing the player interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetPlayerInterface(Object NewInterface)
{
	PlayerInterface = OnlinePlayerInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return PlayerInterface != None;
}

/**
 * Called from native code to assign the extended player interface
 *
 * @param NewInterface the object to assign as providing the player interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetPlayerInterfaceEx(Object NewInterface)
{
	PlayerInterfaceEx = OnlinePlayerInterfaceEx(NewInterface);
	// This will return false, if the interface wasn't supported
	return PlayerInterfaceEx != None;
}

/**
 * Called from native code to assign the system interface
 *
 * @param NewInterface the object to assign as providing the system interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetSystemInterface(Object NewInterface)
{
	SystemInterface = OnlineSystemInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return SystemInterface != None;
}

/**
 * Called from native code to assign the system interface
 *
 * @param NewInterface the object to assign as providing the system interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetGameInterface(Object NewInterface)
{
	GameInterface = OnlineGameInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return GameInterface != None;
}

/**
 * Called from native code to assign the content interface
 *
 * @param NewInterface the object to assign as providing the content interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetContentInterface(Object NewInterface)
{
	ContentInterface = OnlineContentInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return ContentInterface != None;
}

/**
 * Called from native code to assign the system interface
 *
 * @param NewInterface the object to assign as providing the system interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetVoiceInterface(Object NewInterface)
{
	VoiceInterface = OnlineVoiceInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return VoiceInterface != None;
}

/**
 * Called from native code to assign the system interface
 *
 * @param NewInterface the object to assign as providing the system interface
 *
 * @return TRUE if the interface is valid, FALSE otherwise
 */
event bool SetStatsInterface(Object NewInterface)
{
	StatsInterface = OnlineStatsInterface(NewInterface);
	// This will return false, if the interface wasn't supported
	return StatsInterface != None;
}
