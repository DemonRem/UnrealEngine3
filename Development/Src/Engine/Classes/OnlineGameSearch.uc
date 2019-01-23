/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base settings for an online game search
 */
class OnlineGameSearch extends Settings
	native;

/** Max number of queries returned by the matchmaking service */
var int MaxSearchResults;

/** The query to use for finding matching servers */
var LocalizedStringSetting Query;

/** Whether the query is intended for LAN matches or not */
var databinding bool bIsLanQuery;

/** Whether to use arbitration or not */
var databinding bool bUsesArbitration;

/** Whether the search object in question is in progress or not. This is the union of the other flags */
var const bool bIsSearchInProgress;

/** Whether the search object in question has a listen server search in progress or not */
var const bool bIsListenServerSearchInProgress;

/** Whether the search object in question has a dedicated server search in progress or not */
var const bool bIsDedicatedServerSearchInProgress;

/** Whether the search object in question has a list play search in progress or not */
var const bool bIsListPlaySearchInProgress;

/** Whether the search should include dedicated servers or not */
var databinding bool bShouldIncludeDedicatedServers;

/** Whether the search should include listen servers or not */
var databinding bool bShouldIncludeListenServers;

/** Whether the search should include list play servers or not */
var databinding bool bShouldIncludeListPlayServers;

/** The total number of list play servers available if a list play search */
var databinding int NumListPlayServersAvailable;

/** Struct used to return matching servers */
struct native OnlineGameSearchResult
{
	/** The settings used by this particular server */
	var const OnlineGameSettings GameSettings;
	/**
	 * Platform/online provider specific data
	 * NOTE: It is imperative that the subsystem be called to clean this data
	 * up or the PlatformData will leak memory!
	 */
	var const native pointer PlatformData{void};

	structcpptext
	{
		/** Default constructor does nothing and is here for NoInit types */
		FOnlineGameSearchResult()
		{
		}

		/** Zeroing constructor */
		FOnlineGameSearchResult(EEventParm) :
			GameSettings(NULL),
			PlatformData(NULL)
		{
		}
	}
};

/** The class to create for each returned result from the search */
var class<OnlineGameSettings> GameSettingsClass;

/** The list of servers and their settings that match the search */
var const array<OnlineGameSearchResult> Results;

/** The type of data to use to fill out an online parameter */
enum EOnlineGameSearchEntryType
{
	/** A property is used to filter with */
	OGSET_Property,
	/** A localized setting is used to filter with */
	OGSET_LocalizedSetting
};

/** The type of comparison to perform on the search entry */
enum EOnlineGameSearchComparisonType
{
	OGSCT_Equals,
	OGSCT_NotEquals,
	OGSCT_GreaterThan,
	OGSCT_GreaterThanEquals,
	OGSCT_LessThan,
	OGSCT_LessThanEquals
};

/** Struct used to describe a search criteria */
struct native OnlineGameSearchParameter
{
	/** The Id of the property or localized string */
	var int EntryId;
	/** Whether this parameter to compare against comes from a property or a localized setting */
	var EOnlineGameSearchEntryType EntryType;
	/** The type of comparison to perform */
	var EOnlineGameSearchComparisonType ComparisonType;
};

/** Used to indicate which way to sort a result set */
enum EOnlineGameSearchSortType
{
	OGSSO_Ascending,
	OGSSO_Descending
};

/** Struct used to describe the sorting of items */
struct native OnlineGameSearchSortClause
{
	/** The Id of the property or localized string */
	var int EntryId;
	/** Whether this parameter to compare against comes from a property or a localized setting */
	var EOnlineGameSearchEntryType EntryType;
	/** The type of comparison to perform */
	var EOnlineGameSearchSortType SortType;
};

/** Matches parameters using a series of OR comparisons */
struct native OnlineGameSearchORClause
{
	/** The list of parameters to compare and use as an OR clause */
	var array<OnlineGameSearchParameter> OrParams;
};

/** Struct used to describe a query */
struct native OnlineGameSearchQuery
{
	/** A set of OR clauses that are ANDed together to filter potential servers */
	var array<OnlineGameSearchORClause> OrClauses;
	/** A list of sort operations used to order the servers that match the filtering */
	var array<OnlineGameSearchSortClause> SortClauses;
};

/** Holds the query to use when filtering servers and they require non-predefined queries */
var const OnlineGameSearchQuery FilterQuery;

defaultproperties
{
	// Override this with your game specific class so that metadata can properly
	// expose the game information to the UI
	GameSettingsClass=class'Engine.OnlineGameSettings'
	MaxSearchResults=25
	bShouldIncludeListenServers=true
}
