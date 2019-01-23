/**
 * Inherited version of the game resource datastore that has UT specific dataproviders.
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class UTUIDataStore_MenuItems extends UDKUIDataStore_MenuItems
	Config(Game);

DefaultProperties
{
	Tag=UTMenuItems
	WriteAccessType=ACCESS_WriteAll
	MapInfoDataProviderClass=class'UTUIDataProvider_MapInfo'
}


