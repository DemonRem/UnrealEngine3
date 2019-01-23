/**
 * Game resource data stores provide access to available game resources, such as the available gametypes, maps, or mutators
 * The data for each type of game resource is provided through a data provider and is specified in the .ini file for that
 * data provider class type using the PerObjectConfig paradigm.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIDataStore_GameResource extends UIDataStore
	native(inherit)
	implements(UIListElementProvider)
	Config(Game);

struct native GameResourceDataProvider
{
	/** the tag that is used to access this provider, i.e. Players, Teams, etc. */
	var	config		name							ProviderTag;

	/** the name of the class associated with this data provider */
	var	config		string							ProviderClassName;

	/** the UIDataProvider class that exposes the data for this data field tag */
	var	transient	class<UIResourceDataProvider>	ProviderClass;
};

/** the list of data providers supported by this data store that correspond to list element data */
var	config								array<GameResourceDataProvider>		ElementProviderTypes;

/** collection of list element provider instances that are associated with each ElementProviderType */
var	const	private	native	transient	MultiMap_Mirror						ListElementProviders{TMultiMap<FName,class UUIResourceDataProvider*>};

cpptext
{
	/* === UUIDataStore_GameResource interface === */
	/**
	 * Finds or creates the UIResourceDataProvider instances referenced by ElementProviderTypes, and stores the result
	 * into the ListElementProvider map.
	 */
	virtual void InitializeListElementProviders();

	/**
	 * Finds the index for the GameResourceDataProvider with a tag matching ProviderTag.
	 *
	 * @return	the index into the ElementProviderTypes array for the GameResourceDataProvider element that has the
	 *			tag specified, or INDEX_NONE if there are no elements of the ElementProviderTypes array that have that tag.
	 */
	INT FindProviderTypeIndex( FName ProviderTag );

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
	 * Determines whether a member of a collection should be considered "enabled" by subscribed lists.  Disabled elements will still be displayed in the list
	 * but will be drawn using the disabled state.
	 *
	 * @param	FieldName			the name of the collection data field that CollectionIndex indexes into.
	 * @param	CollectionIndex		the index into the data field collection indicated by FieldName to check
	 *
	 * @return	TRUE if FieldName doesn't correspond to a valid collection data field, CollectionIndex is an invalid index for that collection,
	 *			or the item is actually enabled; FALSE only if the item was successfully resolved into a data field value, but should be considered disabled.
	 */
	virtual UBOOL IsElementEnabled( FName FieldName, INT CollectionIndex );

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

	/* === UIDataStore interface === */
	/**
	 * Loads the classes referenced by the ElementProviderTypes array.
	 */
	virtual void LoadDependentClasses();

	/**
	 * Initializes the ListElementProviders map
	 */
	virtual void InitializeDataStore();

	/**
	 * Resolves PropertyName into a list element provider that provides list elements for the property specified.
	 *
	 * @param	PropertyName	the name of the property that corresponds to a list element provider supported by this data store
	 *
	 * @return	a pointer to an interface for retrieving list elements associated with the data specified, or NULL if
	 *			there is no list element provider associated with the specified property.
	 */
	virtual TScriptInterface<class IUIListElementProvider> ResolveListElementProvider( const FString& PropertyName );

	/* === UIDataProvider interface === */
	/**
	 * Resolves the value of the data field specified and stores it in the output parameter.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see GetDataStoreValue for additional notes
	 * @param	ArrayIndex		optional array index for use with data collections
	 *
	 * @todo - not yet implemented
	 */
	virtual UBOOL GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );

	/**
	 * Generates filler data for a given tag.  This is used by the editor to generate a preview that gives the
	 * user an idea as to what a bound datastore will look like in-game.
	 *
 	 * @param		DataTag		the tag corresponding to the data field that we want filler data for
 	 *
	 * @return		a string of made-up data which is indicative of the typical [resolved] value for the specified field.
	 */
	virtual FString GenerateFillerData( const FString& DataTag );

	/**
	 * Gets the list of data fields exposed by this data provider.
	 *
	 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
	 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
	 */
	virtual void GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields );

	/* === UObject interface === */
	/** Required since maps are not yet supported by script serialization */
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize( FArchive& Ar );

	/**
	 * Called from ReloadConfig after the object has reloaded its configuration data.  Reinitializes the collection of list element providers.
	 */
	virtual void PostReloadConfig( UProperty* PropertyThatWasLoaded );
}

DefaultProperties
{
	Tag=GameResources
}
