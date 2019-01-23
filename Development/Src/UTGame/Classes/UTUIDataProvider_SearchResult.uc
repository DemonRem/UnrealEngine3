/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UT specific search result that exposes some extra fields to the server browser.
 */
class UTUIDataProvider_SearchResult extends UIDataProvider_Settings
	native(UI);

/** Some information used to display images in the search results list. */
struct native ResultImageInfo
{
	var transient Surface	ImageReference;
	var string				ImageName;
	var float				ImageX;
	var float				ImageY;
	var float				ImageXL;
	var float				ImageYL;
};

var transient ResultImageInfo	LockedImage;
var transient ResultImageInfo	PureImage;
var transient ResultImageInfo	FriendOnlineImage;

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
	 * Builds a list of available fields from the array of properties in the
	 * game settings object
	 *
	 * @param OutFields	out value that receives the list of exposed properties
	 */
	virtual void GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields);
}

defaultproperties
{
	PureImage=(ImageName="UI_HUD.HUD.UI_HUD_BaseC", ImageX=820, ImageXL=31, ImageY=179, ImageYL=31);
	LockedImage=(ImageName="UI_HUD.HUD.UI_HUD_BaseC", ImageX=851, ImageXL=31, ImageY=179, ImageYL=31);
	FriendOnlineImage=(ImageName="UI_HUD.HUD.UI_HUD_BaseC", ImageX=882, ImageXL=31, ImageY=179, ImageYL=31);
}