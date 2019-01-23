/**
 * This datastore allows games to map aliases to strings that may change based on the current platform or language setting.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 *
 */
class UTUIDataStore_StringAliasMap extends UIDataStore_StringAliasMap
	native(UI)
	Config(Game);

/**
 * Set MappedString to be the localized string using the FieldName as a key
 * Returns the index into the mapped string array of where it was found.
 */
native virtual function int GetStringWithFieldName( String FieldName, out String MappedString );