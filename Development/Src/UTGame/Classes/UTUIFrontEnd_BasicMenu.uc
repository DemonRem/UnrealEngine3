/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Basic Menu Scene for UT3, contains a menu list and a description label.
 */
class UTUIFrontEnd_BasicMenu extends UTUIFrontEnd;

/** Reference to the menu list for this scene. */
var transient UTUIMenuList	MenuList;

/** Reference to the description label for this scene. */
var transient UILabel		DescriptionLabel;

event PostInitialize()
{
	Super.PostInitialize();

	// Get Widget References
	DescriptionLabel = UILabel(FindChild('lblDescription', true));
	MenuList = UTUIMenuList(FindChild('lstMenu', true));
	if(MenuList != none)
	{
		MenuList.OnSubmitSelection = OnMenu_SubmitSelection;
		MenuList.OnValueChanged = OnMenu_ValueChanged;
	}
}

/** Setup the scene's buttonbar. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Select>", OnButtonBar_Select);
}

/**
* Executes a action based on the currently selected menu item.
*/
function OnSelectItem(int PlayerIndex=0)
{
	// Must be implemented in subclass
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_Select(UIScreenObject InButton, int InPlayerIndex)
{
	OnSelectItem();

	return true;
}


/**
* Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
*
* @param	Sender	the list that is submitting the selection
*/
function OnMenu_SubmitSelection( UIList Sender, optional int PlayerIndex=0 )
{
	OnSelectItem(PlayerIndex);
}


/**
 * Called when the user changes the currently selected list item.
 */
function OnMenu_ValueChanged( UIObject Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local string StringValue;

	SelectedItem = MenuList.GetCurrentItem();

	if(MenuList.GetCellFieldString(MenuList, 'Description', SelectedItem, StringValue))
	{
		DescriptionLabel.SetDatastoreBinding(StringValue);
	}
}

