/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Options tab page, autocreates a set of options widgets using the datasource provided.
 */

class UTUITabPage_Options extends UTTabPage
	placeable;

/** Option list present on this tab page. */
var transient UTUIOptionList			OptionList;
var transient UTUIDataStore_StringList	StringListDataStore;
var transient UILabel					DescriptionLabel;

delegate OnAcceptOptions(UIScreenObject InObject, int PlayerIndex);

/** Called when one of our options changes. */
delegate OnOptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex);

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Store a reference to the string list datastore.
	StringListDataStore = UTUIDataStore_StringList(GetCurrentUIController().DataStoreManager.FindDataStore('UTStringList', GetPlayerOwner()));

	/** Store widget references. */
	DescriptionLabel = UILabel(FindChild('lblDescription', true));
	OptionList = UTUIOptionList(FindChild('lstOptions', true));
	OptionList.OnAcceptOptions = OnOptionList_AcceptOptions;
	OptionList.OnOptionChanged = OnOptionList_OptionChanged;
	OptionList.OnOptionFocused = OnOptionList_OptionFocused;

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_GameSettings>");
}

/** Pass through the accept callback. */
function OnOptionList_AcceptOptions(UIScreenObject InObject, int PlayerIndex)
{
	OnAcceptOptions(InObject, PlayerIndex);
}

/** Pass through the option callback. */
function OnOptionList_OptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex)
{
	OnOptionChanged(InObject, OptionName, PlayerIndex);
}

/** Callback for when an option is focused, by default tries to set the description label for this tab page. */
function OnOptionList_OptionFocused(UIScreenObject InObject, UIDataProvider OptionProvider)
{
	local UTUIDataProvider_MenuOption MenuOptionProvider;

	MenuOptionProvider = UTUIDataProvider_MenuOption(OptionProvider);

	if(DescriptionLabel != None && MenuOptionProvider != None)
	{
		DescriptionLabel.SetDataStoreBinding(MenuOptionProvider.Description);
	}
}