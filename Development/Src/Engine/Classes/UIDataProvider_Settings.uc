/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This class is responsible for mapping properties in an Settings
 * object to something that the UI system can consume.
 */
class UIDataProvider_Settings extends UIDynamicDataProvider
	native(inherit)
	transient;

/** Holds the settings object that will be exposed to the UI */
var Settings Settings;

/** Keeps a list of providers for each settings id */
struct native SettingsArrayProvider
{
	/** The settings id that this provider is for */
	var int SettingsId;
	/** Cached to avoid extra look ups */
	var name SettingsName;
	/** The provider object to expose the data with */
	var UIDataProvider_SettingsArray Provider;
};

/** The list of mappings from settings id to their provider */
var array<SettingsArrayProvider> SettingsArrayProviders;

/** Whether this provider is a row in a list (removes array handling) */
var bool bIsAListRow;

cpptext
{
	/**
	 * Resolves the value of the data field specified and stores it in the output parameter.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see GetDataStoreValue for additional notes
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL GetFieldValue(const FString& FieldName,FUIProviderFieldValue& out_FieldValue,INT ArrayIndex = INDEX_NONE);

	/**
	 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	FieldValue		the value to store for the property specified.
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL SetFieldValue(const FString& FieldName,const FUIProviderScriptFieldValue& FieldValue,INT ArrayIndex = INDEX_NONE);

	/**
	 * Generates filler data for a given tag. Uses the OnlineDataType to determine
	 * what the hardcoded filler data will look like
	 *
 	 * @param DataTag the tag to generate filler data for
 	 *
	 * @return a string containing example data
	 */
	virtual FString GenerateFillerData(const FString& DataTag);

	/**
	 * Builds a list of available fields from the array of properties in the
	 * game settings object
	 *
	 * @param OutFields	out value that receives the list of exposed properties
	 */
	virtual void GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields);

	/**
	 * Resolves PropertyName into a list element provider that provides list elements for the property specified.
	 *
	 * @param	PropertyName	the name of the property that corresponds to a list element provider supported by this data store
	 *
	 * @return	a pointer to an interface for retrieving list elements associated with the data specified, or NULL if
	 *			there is no list element provider associated with the specified property.
	 */
	virtual TScriptInterface<class IUIListElementProvider> ResolveListElementProvider(const FString& PropertyName);

	/**
	 * Binds the new settings object to this provider. Sets the type to instance
	 *
	 * @param NewSettings the new object to bind
	 * @param bIsInList whether to use list handling or not
	 *
	 * @return TRUE if bound ok, FALSE otherwise
	 */
	UBOOL BindSettings(USettings* NewSettings,UBOOL bIsInList = FALSE);
}

defaultproperties
{
	WriteAccessType=ACCESS_WriteAll
}