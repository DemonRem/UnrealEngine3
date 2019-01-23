/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Game mode selection page for UT3.
 */

class UTUITabPage_GameModeSelection extends UTTabPage
	placeable;

/** Preview image for a game mode. */
var transient UIImage	GameModePreviewImage[3];

/** Description of the currently selected game mode. */
var transient UILabel	GameModeDescription;

/** Delegate for when the user selects a game mode on this page. */
delegate OnGameModeSelected(string InGameMode, string InDefaultMap, bool bSelectionSubmitted);

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	local UIList GameModeList;

	Super.PostInitialize();

	// Setup delegates
	GameModeList = UIList(FindChild('lstGameModes', true));
	if(GameModeList != none)
	{
		GameModeList.OnSubmitSelection = OnGameModeList_SubmitSelection;
		GameModeList.OnValueChanged = OnGameModeList_ValueChanged;
	}

	// Store widget references

	GameModePreviewImage[0] = UIImage(FindChild('imgGameModePreview1', true));
	GameModePreviewImage[1] = UIImage(FindChild('imgGameModePreview2', true));
	GameModePreviewImage[2] = UIImage(FindChild('imgGameModePreview3', true));

	GameModeDescription = UILabel(FindChild('lblDescription', true));

	// Setup tab caption
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_GameMode>");
}

/**
 * Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnGameModeList_SubmitSelection( UIList Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string GameMode;
	local string DefaultMap;
	local string Prefix;

	SelectedItem = Sender.GetCurrentItem();

	MenuList = UTUIMenuList(Sender);

	if(MenuList != none)
	{
		if(MenuList.GetCellFieldString(MenuList, 'GameMode', SelectedItem, GameMode) && 
		   MenuList.GetCellFieldString(MenuList, 'DefaultMap', SelectedItem, DefaultMap) &&
		   MenuList.GetCellFieldString(MenuList, 'Prefix', SelectedItem, Prefix))
		{
			`Log("Current Game Mode: " $ GameMode);
			`Log("Current Game Mode Default Map: " $ DefaultMap);
			`Log("Current Game Mode Prefix: " $ Prefix);

			SetDataStoreStringValue("<Registry:SelectedGameModePrefix>", Prefix);

			OnGameModeSelected(GameMode, DefaultMap, true);
		}
	}
}


/**
 * Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnGameModeList_ValueChanged( UIObject Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string StringValue;
	local string GameMode;
	local string DefaultMap;
	local string Prefix;

	// Get some data about the currently selected item and update widgets.
	MenuList = UTUIMenuList(Sender);
	
	if(MenuList != none)
	{
		SelectedItem = MenuList.GetCurrentItem();

		if(MenuList.GetCellFieldString(MenuList, 'PreviewImageMarkup', SelectedItem, StringValue))
		{
			GameModePreviewImage[0].SetDatastoreBinding(StringValue);
			GameModePreviewImage[1].SetDatastoreBinding(StringValue);
			GameModePreviewImage[2].SetDatastoreBinding(StringValue);
		}

		if(MenuList.GetCellFieldString(MenuList, 'Description', SelectedItem, StringValue))
		{
			GameModeDescription.SetDatastoreBinding(StringValue);
		}



		if(MenuList.GetCellFieldString(MenuList, 'GameMode', SelectedItem, GameMode) && 
			MenuList.GetCellFieldString(MenuList, 'DefaultMap', SelectedItem, DefaultMap) &&
			MenuList.GetCellFieldString(MenuList, 'Prefix', SelectedItem, Prefix))
		{
			`Log("Current Game Mode: " $ GameMode);
			`Log("Current Game Mode Default Map: " $ DefaultMap);
			`Log("Current Game Mode Prefix: " $ Prefix);

			SetDataStoreStringValue("<Registry:SelectedGameModePrefix>", Prefix);

			OnGameModeSelected(GameMode, DefaultMap, false);
		}
	}
}