/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Extended version of combo box for UT3.
 */
class UTUIComboBox extends UIComboBox
	native(UI)
	implements(UIDataStorePublisher);

var name ToggleButtonStyleName;
var name ToggleButtonCheckedStyleName;
var name EditboxBGStyleName;

cpptext
{
	/**
	 * Called whenever the selected item is modified.  Activates the SliderValueChanged kismet event and calls the OnValueChanged
	 * delegate.
	 *
	 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
	 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
	 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
	 */
	virtual void NotifyValueChanged( INT PlayerIndex=INDEX_NONE, INT NotifyFlags=0 );
}

/** Called after initialization. */
event PostInitialize()
{
	Super.PostInitialize();

	// Set subwidget styles.
	SetupChildStyles();
}

/** Initializes styles for child widgets. */
native function SetupChildStyles();


/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 */
native function SetDataStoreBinding( string MarkupText, optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
native function string GetDataStoreBinding( optional int BindingIndex=INDEX_NONE ) const;

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									-1:	all data sources
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
native function GetBoundDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
native function ClearBoundDataStores();

/** Saves the subscriber value for this combobox. */
native function bool SaveSubscriberValue( out array<UIDataStore> out_BoundDataStores, optional int BindingIndex=INDEX_NONE );

/**
 * @returns the Selection Index of the currently selected list item
 */
function Int GetSelectionIndex()
{
	return ( ComboList != none ) ? ComboList.Index : -1;
}

/**
 * Sets the current index for the list.
 *
 * @param	NewIndex		The new index to select
 */
function SetSelectionIndex(int NewIndex)
{
	if ( ComboList != none && NewIndex >= 0 && NewIndex < ComboList.GetItemCount() )
	{
		ComboList.SetIndex(NewIndex);
	}
}

defaultproperties
{
	ToggleButtonStyleName="DropdownMenuArrowClosed"
	ToggleButtonCheckedStyleName="DropdownMenuArrowOpen"
	EditboxBGStyleName="DropdownMenuClosed"
}
