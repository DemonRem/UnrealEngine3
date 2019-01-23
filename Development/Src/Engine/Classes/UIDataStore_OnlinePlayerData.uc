/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This class is responsible for retrieving the friends list from the online
 * subsystem and populating the UI with that data.
 */
class UIDataStore_OnlinePlayerData extends UIDataStore_Remote
	native(inherit)
	implements(UIListElementProvider)
	config(Engine)
	transient;

/** Provides access to the player's online friends list */
var UIDataProvider_OnlineFriends FriendsProvider;

/** Provides access to the player's recent online players list */
var UIDataProvider_OnlinePlayers PlayersProvider;

/** Provides access to the player's clan members list */
var UIDataProvider_OnlineClanMates ClanMatesProvider;

/** Holds the player that this provider is getting friends for */
var LocalPlayer Player;

/** The online nick name for the player */
var string PlayerNick;

/** The number of new downloads for this player */
var int NumNewDownloads;

/** The total number of downloads for this player */
var int NumTotalDownloads;

/** The name of the OnlineProfileSettings class to use as the default */
var config string ProfileSettingsClassName;

/** The class that should be created when a player is bound to this data store */
var class<OnlineProfileSettings> ProfileSettingsClass;

/** Provides access to the player's profile data */
var UIDataProvider_OnlineProfileSettings ProfileProvider;

cpptext
{
/* === UIDataStore interface === */

	/**
	 * Loads the game specific OnlineProfileSettings class
	 */
	virtual void LoadDependentClasses(void);

	/**
	 * Creates the data providers exposed by this data store
	 */
	virtual void InitializeDataStore(void);

	/**
	 * Forwards the calls to the data providers so they can do their start up
	 *
	 * @param Player the player that will be associated with this DataStore
	 */
	virtual void OnRegister(ULocalPlayer* Player);

	/**
	 * Tells all of the child providers to clear their player data
	 *
	 * @param Player ignored
	 */
	virtual void OnUnregister(ULocalPlayer*);

	/**
	 * Gets the list of data fields exposed by this data provider
	 *
	 * @param OutFields Filled in with the list of fields supported by its aggregated providers
	 */
	virtual void GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields);

	/**
	 * Gets the value for the specified field
	 *
	 * @param	FieldName		the field to look up the value for
	 * @param	OutFieldValue	out param getting the value
	 * @param	ArrayIndex		ignored
	 */
	virtual UBOOL GetFieldValue(const FString& FieldName,FUIProviderFieldValue& OutFieldValue,INT ArrayIndex=INDEX_NONE );

	/**
	 * Resolves PropertyName into a list element provider that provides list elements for the property specified.
	 *
	 * @param	PropertyName	the name of the property that corresponds to a list element provider supported by this data store
	 *
	 * @return	a pointer to an interface for retrieving list elements associated with the data specified, or NULL if
	 *			there is no list element provider associated with the specified property.
	 */
	virtual TScriptInterface<class IUIListElementProvider> ResolveListElementProvider( const FString& PropertyName );

/* === IUIListElementProvider interface === */

	/**
	 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
	 *
	 * @return	the list of tags supported by this element provider which correspond to list element data.
	 */
	virtual TArray<FName> GetElementProviderTags();

	/**
	 * Returns the number of list elements associated with the data tag specified.
	 *
	 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
	 *						from GetElementProviderTags.
	 *
	 * @return	the total number of elements that are required to fully represent the data specified.
	 */
	virtual INT GetElementCount( FName FieldName );

	/**
	 * Retrieves the list elements associated with the data tag specified.
	 *
	 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
	 *							from GetElementProviderTags.
	 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
	 *
	 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
	 */
	virtual UBOOL GetListElements( FName FieldName, TArray<INT>& out_Elements );

	/**
	 * Retrieves a list element for the specified data tag that can provide the list with the available cells for this list element.
	 * Used by the UI editor to know which cells are available for binding to individual list cells.
	 *
	 * @param	FieldName		the tag of the list element data provider that we want the schema for.
	 *
	 * @return	a pointer to some instance of the data provider for the tag specified.  only used for enumerating the available
	 *			cell bindings, so doesn't need to actually contain any data (i.e. can be the CDO for the data provider class, for example)
	 */
	virtual TScriptInterface<class IUIListElementCellProvider> GetElementCellSchemaProvider( FName FieldName );

	/**
	 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the values for the cells
	 * of the list element indicated by CellValueProvider.DataSourceIndex
	 *
	 * @param	FieldName		the tag of the list element data field that we want the values for
	 * @param	ListIndex		the list index for the element to get values for
	 *
	 * @return	a pointer to an instance of the data provider that contains the value for the data field and list index specified
	 */
	virtual TScriptInterface<class IUIListElementCellProvider> GetElementCellValueProvider( FName FieldName, INT ListIndex );
}

/**
 * Binds the player to this provider. Starts the async friends list gathering
 *
 * @param InPlayer the player that we are retrieving friends for
 */
event OnRegister(LocalPlayer InPlayer)
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	Player = InPlayer;
	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// We need to know when the player's login changes
			PlayerInterface.AddLoginChangeDelegate(OnLoginChange,Player.ControllerId);
		}
		if (OnlineSub.PlayerInterfaceEx != None)
		{
			// We need to know when the player changes data (change nick name, etc)
			OnlineSub.PlayerInterfaceEx.AddProfileDataChangedDelegate(Player.ControllerId,OnPlayerDataChange);
		}
		if (OnlineSub.ContentInterface != None)
		{
			// Set the delegate for updating the downloadable content info
			OnlineSub.ContentInterface.AddQueryAvailableDownloadsComplete(Player.ControllerId,OnDownloadableContentQueryDone);
		}
	}
	// Force a refresh
	OnLoginChange();
}

/**
 * Clears our delegate for getting login change notifications
 */
event OnUnregister()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// Clear our delegate
			PlayerInterface.ClearLoginChangeDelegate(OnLoginChange,Player.ControllerId);
		}
		if (OnlineSub.PlayerInterfaceEx != None)
		{
			// Clear for GC reasons
			OnlineSub.PlayerInterfaceEx.ClearProfileDataChangedDelegate(Player.ControllerId,OnPlayerDataChange);
		}
		if (OnlineSub.ContentInterface != None)
		{
			// Clear the delegate for updating the downloadable content info
			OnlineSub.ContentInterface.ClearQueryAvailableDownloadsComplete(Player.ControllerId,OnDownloadableContentQueryDone);
		}
	}
}

/**
 * Refetches the player's nick name from the online subsystem
 */
function OnLoginChange()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// Start a query for downloadable content...
			if(OnlineSub.ContentInterface != None)
			{
				OnlineSub.ContentInterface.QueryAvailableDownloads(Player.ControllerId);
			}
			// Get the name and force a refresh
			PlayerNick = PlayerInterface.GetPlayerNickname(Player.ControllerId);
			RefreshSubscribers();
		}
	}
}

/**
 * Refetches the player's nick name from the online subsystem
 */
function OnPlayerDataChange()
{
	local OnlineSubsystem OnlineSub;

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface != None)
		{
			// Get the name and force a refresh
			PlayerNick = OnlineSub.PlayerInterface.GetPlayerNickname(Player.ControllerId);
			RefreshSubscribers();
		}
	}
}

/**
 * Registers the delegates with the providers so we can know when async data changes
 */
event RegisterDelegates()
{
	FriendsProvider.AddPropertyNotificationChangeRequest(OnProviderChanged);
	PlayersProvider.AddPropertyNotificationChangeRequest(OnProviderChanged);
	ClanMatesProvider.AddPropertyNotificationChangeRequest(OnProviderChanged);
	ProfileProvider.AddPropertyNotificationChangeRequest(OnProviderChanged);
}

/**
 * Handles notification that one of our providers has changed and in turn
 * notifies the UI system
 *
 * @param PropTag the property that changed
 */
function OnProviderChanged(optional name PropTag)
{
	RefreshSubscribers();
}

/**
 * Caches the downloadable content info for the player we're bound to
 *
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
function OnDownloadableContentQueryDone(bool bWasSuccessful)
{
	local OnlineSubsystem OnlineSub;

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None && OnlineSub.ContentInterface != None)
	{
		if (bWasSuccessful == true)
		{
			// Read the data and tell the UI to refresh
			OnlineSub.ContentInterface.GetAvailableDownloadCounts(Player.ControllerId,
				NumNewDownloads,NumTotalDownloads);
			RefreshSubscribers();
		}
		else
		{
//@todo - Should we retry?
			`Log("Failed to query for downloaded content");
		}
	}
}

/** Forwards the call to the provider */
event bool SaveProfileData()
{
	if (ProfileProvider != None)
	{
		return ProfileProvider.SaveProfileData();
	}
	return false;
}

defaultproperties
{
	Tag=OnlinePlayerData
	// So something shows up in the editor
	PlayerNick="PlayerNickNameHere"
}
