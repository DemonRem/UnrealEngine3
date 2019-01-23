/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Stats widget, auto generates labels for all of the fields in the bound datasource.
 */

class UTUIStatsList extends UIScrollFrame
	placeable
	native(UIFrontEnd)
	implements(UIDataStoreSubscriber);

cpptext
{
	/**
	 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
	 * once the scene has been completely initialized.
	 * For widgets added at runtime, called after the widget has been inserted into its parent's
	 * list of children.
	 *
	 * @param	inOwnerScene	the scene to add this widget to.
	 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
	 *							is being added to the scene's list of children.
	 */
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );

	/* === UUIScreenObject interface === */
	/**
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();
}
/** Info about an option we have generated. */
struct native GeneratedStatisticInfo
{
	var name			DataTag;
	var UILabel			KeyObj;
	var UILabel			ValueObj;
};

/** List of auto-generated objects, anything in this array will be removed from the children's array before presave. */
var transient array<GeneratedStatisticInfo>	GeneratedObjects;

/** The data store that this list is bound to */
var(Data)						UIDataStoreBinding		DataSource;

/** the list element provider referenced by DataSource */
var	const	transient			UIListElementProvider	DataProvider;

/** Generates widgets for all of the options. */
native function RegenerateOptions();

/** Repositions all of the visible options. */
native function RepositionOptions();

/** Sets which result row to get stats values from and then retrieves the stats values. */
native function SetStatsIndex(int ResultIdx);

/** Post initialize, binds callbacks for all of the generated options. */
event PostInitialize()
{
	Super.PostInitialize();
	
	RegenerateOptions();
}

/** UIDataSourceSubscriber interface */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
native final virtual function SetDataStoreBinding( string MarkupText, optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
native final virtual function string GetDataStoreBinding( optional int BindingIndex=INDEX_NONE ) const;

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native final virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
native final virtual function GetBoundDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
native final virtual function ClearBoundDataStores();


defaultproperties
{
	DataSource=(RequiredFieldType=DATATYPE_Collection)
}

