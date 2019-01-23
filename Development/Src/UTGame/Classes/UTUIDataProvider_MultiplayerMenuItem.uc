/**
 * Provides menu items for the multiplayer menu.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_MultiplayerMenuItem extends UTUIResourceDataProvider
	native(UI)
	PerObjectConfig
	Config(Game);

/** Friendly displayable name to the player. */
var config localized string FriendlyName;

/** Localized description of the map */
var config localized string Description;
