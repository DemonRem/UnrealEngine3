/**
 * Base for classes which provide data to the UI subsystem.
 * A data store is how the UI references data in the game.  Data stores allow the UI to reference game data in a safe
 * manner, since they encapsulate lifetime management.  A data store can be either persistent, in which case it is
 * attached directly to the UIInteraction object and is available to all widgets, or it can be temporary, in which case
 * it is attached to the current scene and is only accessible to the widgets contained by that scene.
 *
 * Persistent data stores might track information such as UI data for all gametypes or characters.  Temporary data
 * stores might track stuff like the name that was entered into some UI value widget.  Data stores can provide static
 * information, such as the names of all gametypes, or dynamic information, such as the name of the current gametype.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIDataStore extends UIDataProvider
      native(inherit)
      abstract;

cpptext
{
	/**
	 * Allows each data store the chance to load any dependent classes
	 */
	virtual void LoadDependentClasses(void)
	{
	}

	/**
	 * Hook for performing any initialization required for this data store
	 */
	virtual void InitializeDataStore();

	/**
	 * Called when this data store is added to the data store manager's list of active data stores.
	 *
	 * @param	PlayerOwner		the player that will be associated with this DataStore.  Only relevant if this data store is
	 *							associated with a particular player; NULL if this is a global data store.
	 */
	virtual void OnRegister( class ULocalPlayer* PlayerOwner );

	/**
	 * Called when this data store is removed from the data store manager's list of active data stores.
	 *
	 * @param	PlayerOwner		the player that will be associated with this DataStore.  Only relevant if this data store is
	 *							associated with a particular player; NULL if this is a global data store.
	 */
	virtual void OnUnregister( class ULocalPlayer* PlayerOwner );

	/**
	 * Resolves PropertyName into a list element provider that provides list elements for the property specified.
	 *
	 * @param	PropertyName	the name of the property that corresponds to a list element provider supported by this data store
	 *
	 * @return	a pointer to an interface for retrieving list elements associated with the data specified, or NULL if
	 *			there is no list element provider associated with the specified property.
	 */
	virtual TScriptInterface<class IUIListElementProvider> ResolveListElementProvider( const FString& PropertyName ) { return TScriptInterface<IUIListElementProvider>(); }

	/**
	 * Retrieves the tag used for referencing this data store.  Normally corresponds to Tag, but may be different for some special
	 * data stores.
	 */
	virtual FName GetDataStoreID() const { return Tag; }
}

`include(Core/Globals.uci)

/** The name used to access this datastore */
var		name				Tag;

/** the list of delegates to call when data exposed by this data store has been updated */
var		array<delegate<OnRefreshDataStore> >		RefreshSubscriberNotifies;

/**
 * This delegate is called whenever the values exposed by this data store have been updated.  Provides data stores with a way to
 * notify subscribers when they should refresh their values from this data store.
 *
 * @return	unused
 *
 * @todo - currently, the data stores resolved via markup text in UIStrings aren't registered/unregistered....
 */
delegate bool OnRefreshDataStore( optional int Unused=INDEX_NONE );

/**
 * Called when this data store is added to the data store manager's list of active data stores.
 *
 * @param	PlayerOwner		the player that will be associated with this DataStore.  Only relevant if this data store is
 *							associated with a particular player; NULL if this is a global data store.
 */
event Registered( LocalPlayer PlayerOwner );

/**
 * Called when this data store is removed from the data store manager's list of active data stores.
 *
 * @param	PlayerOwner		the player that will be associated with this DataStore.  Only relevant if this data store is
 *							associated with a particular player; NULL if this is a global data store.
 */
event Unregistered( LocalPlayer PlayerOwner );

/**
 * Notification that a subscriber is using a value from this data store.  Adds the subscriber's RefreshSubscriberValue method
 * to this data store's list of refresh notifies so that the subscriber can refresh its value when the data store's value changes.
 *
 * @param	Subscriber	the subscriber that attached to the data store.
 */
event SubscriberAttached( UIDataStoreSubscriber Subscriber )
{
//	local delegate<OnRefreshDataStore> SubscriberNotify;
	local int SubscriberNotifyIndex;
//	local string DebugString;

//	DebugString = `location @ `showvar(Subscriber);
	if ( Subscriber != None )
	{
		SubscriberNotifyIndex = RefreshSubscriberNotifies.Find(Subscriber.RefreshSubscriberValue);

		if ( SubscriberNotifyIndex == INDEX_NONE )
		{
			SubscriberNotifyIndex = RefreshSubscriberNotifies.Length;

//			DebugString @= "successfully added to notify list at index" @ `showvar(SubscriberNotifyIndex);
			RefreshSubscriberNotifies[SubscriberNotifyIndex] = Subscriber.RefreshSubscriberValue;
		}
		else
		{
//			DebugString @= "not added to notify list - already in list at index" @ `showvar(SubscriberNotifyIndex);
		}
	}

//	`log(DebugString,,'DataStore');
}

/**
 * Notification that a subscriber is no longer using any values from this data store.  Removes the subscriber's RefreshSubscriberValue method
 * from this data store's list of refresh notifies so that the subscriber no longer refreshes its value when the data store's value changes.
 *
 * @param	Subscriber	the subscriber that detached from the data store.
 */
event SubscriberDetached( UIDataStoreSubscriber Subscriber )
{
	local int SubscriberNotifyIndex;
//	local string DebugString;

//	DebugString = `location @ `showvar(Subscriber);
	if ( Subscriber != None )
	{
		SubscriberNotifyIndex = RefreshSubscriberNotifies.Find(Subscriber.RefreshSubscriberValue);
		if ( SubscriberNotifyIndex != INDEX_NONE )
		{
//			DebugString @= "successfully removed from notify list at index" @ SubscriberNotifyIndex;
			RefreshSubscriberNotifies.Remove(SubscriberNotifyIndex,1);
		}
		else
		{
//			DebugString @= "failed to remove from notify list - not found in list";
		}
	}

//	`log(DebugString,,'DataStore');
}

/**
 * Called when the current map is being unloaded.  Cleans up any references which would prevent garbage collection.
 *
 * @return	TRUE indicates that this data store should be automatically unregistered when this game session ends.
 */
function bool NotifyGameSessionEnded();

/**
 * Loops through the subscriber notify list and calls the delegate letting the subscriber know to refresh their value.
 */
event RefreshSubscribers()
{
	local int Idx;
	local delegate<OnRefreshDataStore> Subscriber;

	//@todo: Right now we make a copy of the array because the refresh notify for datastore subscribers is unregistering the notify
	// when resolving markup.
	local array<delegate<OnRefreshDataStore> > SubscriberArrayCopy;
	SubscriberArrayCopy.Length = RefreshSubscriberNotifies.Length;

	for(Idx = 0; Idx < SubscriberArrayCopy.Length; Idx++)
	{
		SubscriberArrayCopy[Idx] = RefreshSubscriberNotifies[Idx];
	}

	for (Idx = 0; Idx < SubscriberArrayCopy.Length; Idx++)
	{
		Subscriber = SubscriberArrayCopy[Idx];
		Subscriber();
	}
}


/**
 * Notifies the data store that all values bound to this data store in the current scene have been saved.  Provides data stores which
 * perform buffered or batched data transactions with a way to determine when the UI system has finished writing data to the data store.
 *
 * @note: for now, this lives in UIDataStore, but it might make sense to move it up to UIDataProvider later on.
 */
native function OnCommit();

DefaultProperties
{

}
