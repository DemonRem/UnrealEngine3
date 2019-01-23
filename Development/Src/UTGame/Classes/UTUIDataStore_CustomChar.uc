/**
 * Inherited version of the game resource datastore that exposes the various customizeable character parts to the UI.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataStore_CustomChar extends UIDataStore
	native(UI)
	transient
	implements(UIListElementProvider)
	Config(CustomChar)
	dependson(UTCustomChar_Data);

cpptext
{
	/** @return Returns the enum value for a part type given its tag. */
	ECharPart ResolvePartEnumFromTag(const FName &FieldTag);

	/** @return Returns the tag value for a part type given its enum. */
	FName ResolveTagFromPartEnum(ECharPart PartType);

	/**
	 * Initializes the dataproviders for all of the various character parts.
	 */
	virtual void InitializeDataStore();

	/** 
	 * Gets the list of element providers for a fieldname with filtered elements removed.
	 *
	 * @param FieldName				Fieldname to use to search for providers.
	 * @param OutParts				Array to store part indices in.
	 */
	void GetFilteredElementProviders(FName FieldName, TArray<class UUIDataProvider*> &OutParts);

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

	/**
	 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	FieldValue		the value to store for the property specified.
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex=INDEX_NONE );

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

	/* === UObject interface === */
	/** Required since maps are not yet supported by script serialization */
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize( FArchive& Ar );

	/**
	 * Called from ReloadConfig after the object has reloaded its configuration data.  Reinitializes the collection of list element providers.
	 */
	virtual void PostReloadConfig( UProperty* PropertyThatWasLoaded );
}

/** collection of providers per part type. */
var	const	private	native	transient	MultiMap_Mirror		PartProviders{TMultiMap<FName, class UUIDataProvider*>};

/** Array of FNames for the parts map, this should map directly to the ECharPart enum. */
var array<name>	PartTags;

/** Set of currently active bots that the player is going to play against. */
var array<int>	ActiveBots;

/** The family of parts we are going to show the player. */
var string	CurrentFamily;

/** The currently selected character. */
var string	CurrentCharacter;

/** The currently selected faction. */
var string	CurrentFaction;

/**
 * Attempts to find the index of a provider given a provider field name, a search tag, and a value to match.
 *
 * @return	Returns the index of the provider or INDEX_NONE if the provider wasn't found.
 */
native function int FindValueInProviderSet(name ProviderFieldName, name SearchTag, string SearchValue);

/**
 * Attempts to find the value of a provider given a provider cell field.
 *
 * @return	Returns true if the value was found, false otherwise.
 */
native function bool GetValueFromProviderSet(name ProviderFieldName, name SearchTag, int ListIndex, out string OutValue);

DefaultProperties
{
	Tag=UTCustomChar
	WriteAccessType=ACCESS_WriteAll
	PartTags=("Heads","Helmets","Facemasks","Goggles","Torsos","ShoulderPads","Arms","Thighs","Boots") 
}


