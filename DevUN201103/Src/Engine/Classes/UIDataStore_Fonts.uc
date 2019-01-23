/**
 * This data store class is responsible for parsing and applying inline font changes.
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class UIDataStore_Fonts extends UIDataStore
	native(inherit);

cpptext
{
	/* === UIDataProvider interface === */
	/**
	 * Gets the list of font names available through this data store.
	 *
	 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
	 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
	 */
	virtual void GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields );

	/**
	 * This data store cannot generate string nodes.
	 */
	virtual UBOOL GetDataStoreValue( const FString& MarkupString, struct FUIProviderFieldValue& out_FieldValue ) { return FALSE; }
}

DefaultProperties
{
	Tag=Fonts
}
