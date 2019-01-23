/**
* Provides data for a UT3 weapon.
*
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
*/
class UTUIDataProvider_Weapon extends UTUIResourceDataProvider
	PerObjectConfig
	Config(Game)
	native(UI);

/** The weapon class path */
var config string ClassName;
/** class path to this weapon's preferred ammo class
 * this is primarily used by weapon replacement mutators to get the ammo types to replace from/to
 */
var config string AmmoClassPath;

/** Friendly name for the weapon */
var config localized string FriendlyName;

/** Description for the weapon */
var config localized string Description;

defaultproperties
{
	bSearchAllInis=true
}
