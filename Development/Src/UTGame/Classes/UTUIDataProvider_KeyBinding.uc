/**
 * Provides a possible key binding for the game.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_KeyBinding extends UIResourceDataProvider
	native(UI)
	PerObjectConfig
	Config(Game);

/** Friendly displayable name to the player. */
var config localized string FriendlyName;

/** Command to bind the key to. */
var config string Command;
