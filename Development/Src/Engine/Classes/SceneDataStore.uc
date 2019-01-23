/**
 * This data store class is used for providing access to data that should only have the lifetime of the current scene.
 * Each scene has its own SceneDataStore, which is capable of containing an arbitrary number of data elements, configurable
 * by the designer using the UI editor.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class SceneDataStore extends UIDataStore
	native(UIPrivate)
	implements(UIListElementProvider,UIListElementCellProvider)
	nontransient;

cpptext
{
	/* === UIDataStore interface === */
	/**
	 * Creates the data provider for this scene data store.
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
protected:
	/**
	 * Resolves the value of the data field specified and stores it in the output parameter.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see ParseDataStoreReference for additional notes
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );

	/**
	 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	FieldValue		the value to store for the property specified.
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex=INDEX_NONE );

public:
	/**
	 * Gets the list of data fields exposed by this data provider.
	 *
	 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
	 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
	 */
	virtual void GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields );

	/**
	 * Returns a pointer to the data provider which provides the tags for this data provider.  Normally, that will be this data provider,
	 * but for some data stores such as the Scene data store, data is pulled from an internal provider but the data fields are presented as
	 * though they are fields of the data store itself.
	 */
	virtual UUIDataProvider* GetDefaultDataProvider();


	/** === IUIListElementProviderInterface === */
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
	 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the available cells for this list element.
	 * Used by the UI editor to know which cells are available for binding to individual list cells.
	 *
	 * @param	FieldName		the tag of the list element data field that we want the schema for.
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


	/* === IUIListElementCellProvider === */
	/**
	 * Retrieves the list of tags that can be bound to individual cells in a single list element, along with the human-readable,
	 * localized string that should be used in the header for each cell tag (in lists which have column headers enabled).
	 *
	 * @param	out_CellTags	receives the list of tag/column headers that can be bound to element cells for the specified property.
	 */
	virtual void GetElementCellTags( TMap<FName,FString>& out_CellTags );

	/**
	 * Retrieves the field type for the specified cell.
	 *
	 * @param	CellTag				the tag for the element cell to get the field type for
	 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
	 *
	 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
	 */
	virtual UBOOL GetCellFieldType( const FName& CellTag, BYTE& out_CellFieldType );

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
	virtual UBOOL GetCellFieldValue( const FName& CellTag, INT ListIndex, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );
}

var	const	transient	UIScene				OwnerScene;

/**
 * The data provider that contains the data fields supported by this scene data store
 */
var	protected	UIDynamicFieldProvider		SceneDataProvider;

/* == Delegates == */

/* == Events == */

/* == Natives == */

/* == UnrealScript == */
/**
 * Gets the data value source array for the specified data field.
 *
 * @param	FieldName			the name of the data field the source data should be associated with.
 * @param	out_DataValueArray	receives the array of data values available for FieldName.
 * @param	bPersistent			specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *								wouldn't be.
 *
 * @return	TRUE if the array containing possible values for the FieldName data field was successfully located and copied
 *			into the out_DataValueArray variable.
 */
final function bool GetCollectionValueArray( name FieldName, out array<string> out_DataValueArray, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.GetCollectionValueArray(FieldName, out_DataValueArray, bPersistent);
	}

	return false;
}

/**
 * Sets the source data for a collection data field to the values specified.  It is not necessary to add the field first
 * (via AddField) in order to set the collection values.
 *
 * @param	FieldName			the name of the data field the source data should be associated with.
 * @param	CollectionValues	the actual values that will be associated with FieldName.
 * @param	bClearExisting		specify TRUE to clear the existing collection data before adding the new values
 * @param	InsertIndex			the position to insert the new values (only relevant if bClearExisting is FALSE)
 * @param	bPersistent			specify TRUE to ensure that the values will be added to PersistentCollectionData, even
 *								if they otherwise wouldn't be.
 *
 * @return	TRUE if the collection data was applied successfully; FALSE if there was also already data for this collection
 *			data field [and bOverwriteExisting was FALSE] or the data couldn't otherwise
 */
final function bool SetCollectionValueArray( name FieldName, out const array<string> CollectionValues, optional bool bClearExisting=true, optional int InsertIndex=INDEX_NONE, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.SetCollectionValueArray(FieldName, CollectionValues, bClearExisting, InsertIndex, bPersistent);
	}

	return false;
}

/**
 * Inserts a new string into the list of values for the specified collection data field.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	NewValue		the value to insert
 * @param	InsertIndex		the index [into the array of values for FieldName] to insert the new value, or INDEX_NONE to
 *							append the value to the end of the list.
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 * @param	bAllowDuplicateValues
 *							controls whether multiple elements containing the same value should be allowed in the data source
 *							collection.  If FALSE is specified, and NewValue already exists in the collection source array, method
 *							return TRUE but it does not modify the array.  If TRUE is specified, NewValue will be added anyway,
 *							resulting in multiple copies of NewValue existing in the array.
 *
 * @return	TRUE if the new value was successfully inserted into the collection data source for the specified field.
 */
final function bool InsertCollectionValue( name FieldName, out const string NewValue, optional int InsertIndex=INDEX_NONE, optional bool bPersistent, optional bool bAllowDuplicateValues )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.InsertCollectionValue(FieldName, NewValue, InsertIndex, bPersistent, bAllowDuplicateValues);
	}

	return false;
}

/**
 * Removes a value from the collection data source specified by FieldName.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	ValueToRemove	the value that should be removed
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the value was successfully removed or didn't exist in the first place.
 */
final function bool RemoveCollectionValue( name FieldName, out const string ValueToRemove, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.RemoveCollectionValue(FieldName,ValueToRemove,bPersistent);
	}

	return false;
}

/**
 * Removes the value from the collection data source specified by FieldName located at ValueIndex.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	ValueIndex		the index [into the array of values for FieldName] of the value that should be removed.
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the value was successfully removed; FALSE if ValueIndex wasn't valid or the value couldn't be removed.
 */
final function bool RemoveCollectionValueByIndex( name FieldName, int ValueIndex, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.RemoveCollectionValueByIndex(FieldName,ValueIndex,bPersistent);
	}

	return false;
}

/**
 * Replaces the value in a collection data source with a different value.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	CurrentValue	the value that will be replaced.
 * @param	NewValue		the value that will replace CurrentValue
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the old value was successfully replaced with the new value.
 */
final function bool ReplaceCollectionValue( name FieldName, out const string CurrentValue, out const string NewValue, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.ReplaceCollectionValue(FieldName,CurrentValue,NewValue,bPersistent);
	}

	return false;
}

/**
 * Replaces the value located at ValueIndex in a collection data source with a different value
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	ValueIndex		the index [into the array of values for FieldName] of the value that should be replaced.
 * @param	NewValue		the value that should replace the old value.
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the value was successfully replaced; FALSE if ValueIndex wasn't valid or the value couldn't be removed.
 */
final function bool ReplaceCollectionValueByIndex( name FieldName, int ValueIndex, out const string NewValue, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.ReplaceCollectionValueByIndex(FieldName,ValueIndex,NewValue,bPersistent);
	}

	return false;
}

/**
 * Removes all data values for a single collection data field.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the data values were successfully cleared or didn't exist in the first place; FALSE if they couldn't be removed.
 */
final function bool ClearCollectionValueArray( name FieldName, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.ClearCollectionValueArray(FieldName,bPersistent);
	}

	return false;
}

/**
 * Retrieves the value of an element in a collection data source array.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	ValueIndex		the index [into the array of values for FieldName] of the value that should be retrieved.
 * @param	out_Value		receives the value of the collection data source element
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	TRUE if the value was successfully retrieved and copied to out_Value.
 */
final function bool GetCollectionValue( name FieldName, int ValueIndex, out string out_Value, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.GetCollectionValue(FieldName,ValueIndex,out_Value,bPersistent);
	}

	return false;
}

/**
 * Finds the index [into the array of values for FieldName] for a specific value.
 *
 * @param	FieldName		the name of the data field associated with the data source collection being manipulated.
 * @param	ValueToFind		the value that should be found.
 * @param	bPersistent		specify TRUE to ensure that the PersistentCollectionData is used, even if it otherwise
 *							wouldn't be.
 *
 * @return	the index for the specified value, or INDEX_NONE if it couldn't be found.
 */
final function int FindCollectionValueIndex( name FieldName, out const string ValueToFind, optional bool bPersistent )
{
	if ( SceneDataProvider != None )
	{
		return SceneDataProvider.FindCollectionValueIndex(FieldName,ValueToFind,bPersistent);
	}

	return INDEX_NONE;
}

/* == SequenceAction handlers == */


DefaultProperties
{
	Tag=SCENE_DATASTORE_TAG
}
