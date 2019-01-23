/**
 * Provides data for a UT3 game mode.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_GameModeInfo extends UTUIResourceDataProvider
	native(UI)
	PerObjectConfig
	Config(Game);

/** full path to the game class */
var config string GameMode;

/** The map that this game mode defaults to */
var config string DefaultMap;

/** Friendly displayable name to the player. */
var config localized string FriendlyName;

/** Localized description of the game mode */
var config localized string Description;

/** Markup text used to display the preview image for the game mode. */
var config string PreviewImageMarkup;

/** Prefix of the maps this gametype supports. */
var config string Prefix;

defaultproperties
{
	bSearchAllInis=true
}
