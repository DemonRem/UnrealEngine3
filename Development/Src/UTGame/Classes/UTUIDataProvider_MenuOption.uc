/**
 * Provides an option for a UI menu item.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UTUIDataProvider_MenuOption extends UTUIResourceDataProvider
	native(UI)
	PerObjectConfig
	Config(Game);

enum EUTOptionType
{
	UTOT_ComboReadOnly,
	UTOT_ComboNumeric,
	UTOT_CheckBox,
	UTOT_Slider,
	UTOT_Spinner,
	UTOT_EditBox
};

var config EUTOptionType OptionType;

/** Name of the option set that this option belongs to. */
var config name OptionSet;

/** Markup for the option */
var config string DataStoreMarkup;

/** Game mode required for this option to appear. */
var config name RequiredGameMode;

/** Friendly displayable name to the player. */
var config localized string FriendlyName;

/** Localized description of the option */
var config localized string Description;

/** Whether or not the options presented to the user are the only options they can choose from, used on PC only for setting whether combobox edit boxes are read only or not. */
// @todo: As of this change, we don't have support for editable comboboxes.
/*var config bool bReadOnlyCombo;*/

/** Range data for the option, only used if its a slider type. */
var config UIRangeData	RangeData;

/** @return Returns whether or not this provider should be filtered, by default it returns FALSE. */
function native virtual bool IsFiltered();

defaultproperties
{
	
}