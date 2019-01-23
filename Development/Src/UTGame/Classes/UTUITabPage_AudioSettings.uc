/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's Audio settings.
 */

class UTUITabPage_AudioSettings extends UTUITabPage_Options
	placeable;


/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.Settings.Audio>");
}

