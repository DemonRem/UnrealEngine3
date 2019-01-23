/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This class is responsible for retrieving the friends list from the online
 * subsystem and populating the UI with that data.
 */
class UIDataProvider_OnlineFriends extends UIDataProvider_OnlinePlayerDataBase
	native(inherit)
	implements(UIListElementCellProvider)
	dependson(OnlineSubsystem)
	transient;

/** Gets a copy of the friends data from the online subsystem */
var array<OnlineFriend> FriendsList;

cpptext
{
/* === IUIListElement interface === */

	/**
	 * Returns the names of the exposed members in OnlineFriend
	 *
	 * @see OnlineFriend structure in OnlineSubsystem
	 */
	virtual void GetElementCellTags( TMap<FName,FString>& out_CellTags )
	{
		//@fixme - localize the column header strings
		out_CellTags.Set(FName(TEXT("NickName")),TEXT("NickName"));
		out_CellTags.Set(FName(TEXT("PresenceInfo")),TEXT("PresenceInfo"));
		out_CellTags.Set(FName(TEXT("bIsOnline")),TEXT("bIsOnline"));
		out_CellTags.Set(FName(TEXT("bIsPlaying")),TEXT("bIsPlaying"));
		out_CellTags.Set(FName(TEXT("bIsPlayingThisGame")),TEXT("bIsPlayingThisGame"));
		out_CellTags.Set(FName(TEXT("bIsJoinable")),TEXT("bIsJoinable"));
		out_CellTags.Set(FName(TEXT("bHasVoiceSupport")),TEXT("bHasVoiceSupport"));
	}

	/**
	 * Retrieves the field type for the specified cell.
	 *
	 * @param	CellTag				the tag for the element cell to get the field type for
	 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
	 *
	 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
	 */
	virtual UBOOL GetCellFieldType( const FName& CellTag, BYTE& out_CellFieldType )
	{
		//@fixme joeg - implement this
		out_CellFieldType = DATATYPE_Property;
		return TRUE;
	}

	/**
	 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
	 *
	 * @param	CellTag			the tag for the element cell to resolve the value for
	 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
	 *							do not provide unique UIListElement objects for each element.
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see GetDataStoreValue for additional notes
	 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
	 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
	 *							to a data collection.
	 */
	virtual UBOOL GetCellFieldValue( const FName& CellTag, INT ListIndex, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );

/* === UIDataProvider interface === */

	/**
	 * Gets the list of data fields exposed by this data provider.
	 *
	 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
	 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
	 */
	virtual void GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
	{
		new(out_Fields) FUIDataProviderField( FName(TEXT("Friends")), DATATYPE_Collection );
	}

	/**
	 * Resolves the value of the data field specified and stores it in the output parameter.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see GetDataStoreValue for additional notes
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );
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

	Super.OnRegister(InPlayer);
	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// Register that we are interested in any sign in change for this player
			PlayerInterface.AddLoginChangeDelegate(OnLoginChange,Player.ControllerId);
			// Set our callback function per player
			PlayerInterface.AddReadFriendsCompleteDelegate(Player.ControllerId,OnFriendsReadComplete);
			// Start the async task
			if (PlayerInterface.ReadFriendsList(Player.ControllerId) == false)
			{
				`warn("Can't retrieve friends for player ("$Player.ControllerId$")");
			}
		}
		else
		{
			`warn("OnlineSubsystem does not support the player interface. Can't retrieve friends for player ("$
				Player.ControllerId$")");
		}
	}
	else
	{
		`warn("No OnlineSubsystem present. Can't retrieve friends for player ("$
			Player.ControllerId$")");
	}
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
			// Set our callback function per player
			PlayerInterface.ClearReadFriendsCompleteDelegate(Player.ControllerId,OnFriendsReadComplete);
			// Clear our delegate
			PlayerInterface.ClearLoginChangeDelegate(OnLoginChange,Player.ControllerId);
		}
	}
	Super.OnUnregister();
}

/**
 * Handles the notification that the async read of the friends data is done
 *
 * @param bWasSuccessful whether the call completed ok or not
 */
function OnFriendsReadComplete(bool bWasSuccessful)
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	if (bWasSuccessful == true)
	{
		// Figure out if we have an online subsystem registered
		OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
		if (OnlineSub != None)
		{
			// Grab the player interface to verify the subsystem supports it
			PlayerInterface = OnlineSub.PlayerInterface;
			if (PlayerInterface != None)
			{
				// Make a copy of the friends data for the UI
				PlayerInterface.GetFriendsList(Player.ControllerId,FriendsList);
			}
		}
		// Notify any subscribers that we have new data
		NotifyPropertyChanged();
	}
	else
	{
		`Log("Failed to read friends list");
	}
}

/**
 * Executes a refetching of the friends data when the login for this player
 * changes
 */
function OnLoginChange()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface PlayerInterface;

	FriendsList.Length = 0;
	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the player interface to verify the subsystem supports it
		PlayerInterface = OnlineSub.PlayerInterface;
		if (PlayerInterface != None)
		{
			// Start the async task
			if (PlayerInterface.ReadFriendsList(Player.ControllerId) == false)
			{
				`warn("Can't retrieve friends for player ("$Player.ControllerId$")");
				// Notify any subscribers that we have changed data
				NotifyPropertyChanged();
			}
		}
	}
}
