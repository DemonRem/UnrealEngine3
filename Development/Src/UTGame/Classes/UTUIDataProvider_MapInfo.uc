/**
 * Provides data for a UT3 map.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_MapInfo extends UTUIResourceDataProvider
	native(UI)
	PerObjectConfig
	Config(Game);

/** Unique ID for maps. */
var config int	  MapID;

/** Actual map name to load */
var config string MapName;

/** Friendly displayable name to the player. */
var config localized string FriendlyName;

/** Localized description of the map */
var config localized string Description;

/** Markup text used to display the preview image for the map. */
var config string PreviewImageMarkup;

/** @return Returns whether or not this provider should be filtered, by default it returns FALSE. */
native virtual function bool IsFiltered();

/** @return Returns whether or not this provider is supported by the current game mode */
event bool SupportedByCurrentGameMode()
{
	local int Pos;
	local string ThisMapPrefix;
	local string GameModePrefix;
	local bool bResult;

	bResult = true;

	// Get our map prefix.
	Pos = InStr(MapName,"-");
	ThisMapPrefix = left(MapName,Pos);

	if(GetDataStoreStringValue("<Registry:SelectedGameModePrefix>", GameModePrefix))
	{
		bResult = (ThisMapPrefix~=GameModePrefix);
	}

	return bResult;
}

defaultproperties
{
	bSearchAllInis=true
}
