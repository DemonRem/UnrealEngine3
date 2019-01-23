/**
 * Provides data for a UT3 mutator.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_Mutator extends UTUIResourceDataProvider
	PerObjectConfig
	native(UI)
	Config(Game);

/** The mutator class name. */
var config string ClassName;

/** Friendly name for the mutator. */
var config localized string FriendlyName;

/** Description for the mutator. */
var config localized string Description;

/** Name of the group the mutator belongs to. */
var config string GroupName;

/** Path to a UIScene to use for configuring this mutator. */
var config string UIConfigScene;

/** @return Returns whether or not this provider should be filtered, by default it returns FALSE. */
function native virtual bool IsFiltered();

/** @return Returns whether or not this mutator supports the currently set gamemode in the frontend. */
event bool SupportsCurrentGameMode()
{
	local string GameMode;
	local class<UTGame> GameModeClass;
	local bool bResult;

	bResult = true;

	GetDataStoreStringValue("<Registry:SelectedGameMode>", GameMode);

	GameModeClass = class<UTGame>(DynamicLoadObject(GameMode, class'class'));
	if(GameModeClass != none)
	{
		bResult = GameModeClass.static.AllowMutator(ClassName);
	}

	return bResult;
}

defaultproperties
{
	bSearchAllInis=true
}
