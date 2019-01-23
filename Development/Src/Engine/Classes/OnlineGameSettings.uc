/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base configuration settings for an online game
 */
class OnlineGameSettings extends Settings
	native;

/** The number of publicly available connections advertised */
var databinding int NumPublicConnections;

/** The number of connections that are private (invite/password) only */
var databinding int NumPrivateConnections;

/** The number of publicly available connections that are available (read only) */
var databinding int NumOpenPublicConnections;

/** The number of private connections that are available (read only) */
var databinding int NumOpenPrivateConnections;

/** The server's nonce for this session */
var const byte ServerNonce[8];

/** Max number of queries returned by the match finding service */
var int MaxSearchResults;

/** Whether this match is publicly advertised on the online service */
var databinding bool bShouldAdvertise;

/** This game will be lan only and not be visible to external players */
var databinding bool bIsLanMatch;

/** Whether the match should gather stats or not */
var databinding bool bUsesStats;

/** Whether joining in progress is allowed or not */
var databinding bool bAllowJoinInProgress;

/** Whether the match allows invitations for this session or not */
var databinding bool bAllowInvites;

/** Whether to display user presence information or not */
var databinding bool bUsesPresence;

/** Whether joining via player presence is allowed or not */
var databinding bool bAllowJoinViaPresence;

/** Whether the session should use arbitration or not */
var databinding bool bUsesArbitration;

/** Whether the game is an invitation or searched for game */
var const bool bWasFromInvite;

/** The owner of the game */
var databinding string OwningPlayerName;

/** The unique net id of the player that owns this game */
var UniqueNetId OwningPlayerId;

/** The ping of the server in milliseconds */
var databinding int PingInMs;

/** Whether this server is a dedicated server or not */
var databinding bool bIsDedicated;

/** Whether this server is a list play server or not */
var databinding bool bIsListPlay;

/** The type of dedicated server (everyone, premium, etc) */
enum EDedicatedServerType
{
	/** Anyone that can play MP can join */
	DST_Standard,
	/** Anyone with a premium play account can join */
	DST_Premium1,
	/** Anyone with the next level of premium play can join */
	DST_Premium2
};

/** The type of server based upon player service restrictions */
var EDedicatedServerType DedicatedServerType;

defaultproperties
{
	bAllowJoinInProgress=true
	bUsesStats=true
	bShouldAdvertise=true
	bAllowInvites=true
	bUsesPresence=true
	bAllowJoinViaPresence=true
}