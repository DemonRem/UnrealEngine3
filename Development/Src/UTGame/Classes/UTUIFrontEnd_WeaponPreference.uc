/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UI scene that allows the user to set their weapon preferences.
 */
class UTUIFrontEnd_WeaponPreference extends UTUIFrontEnd;

/** List of weapons. */
var transient UIList WeaponList;

/** Description of the currently selected weapon. */
var transient UILabel DescriptionLabel;

/** Reference to the menu datastore */
var transient UTUIDataStore_MenuItems MenuDataStore;

/** Post initialize callback. */
event PostInitialize()
{
	Super.PostInitialize();

	// Find widget references
	WeaponList = UIList(FindChild('lstWeapons', true));
	WeaponList.OnValueChanged = OnWeaponList_ValueChanged;

	DescriptionLabel = UILabel(FindChild('lblDescription', true));

	// Get reference to the menu datastore
	MenuDataStore = UTUIDataStore_MenuItems(GetCurrentUIController().DataStoreManager.FindDataStore('UTMenuItems'));

	OnResetToDefaults();
}

/** Sets up the button bar for the parent scene. */
function SetupButtonBar()
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ShiftUp>", OnButtonBar_ShiftUp);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ShiftDown>", OnButtonBar_ShiftDown);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ResetToDefaults>", OnButtonBar_ResetToDefaults);
}

/** Updates the description and preview image for the currently selected weapon when the user changes the currently selected weapon. */
function OnWeaponList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	local string NewDescription;
	local int SelectedItem;

	SelectedItem = WeaponList.GetCurrentItem();

	if(class'UTUIMenuList'.static.GetCellFieldString(WeaponList, 'Description', SelectedItem, NewDescription))
	{
		DescriptionLabel.SetDataStoreBinding(NewDescription);
	}
}

/** Callback for when the user wants to back out of the scene. */
function OnBack()
{
	CloseScene(self);
}

/** Shifts the currently selected weapon up or down in the weapon preference order. */
function OnShiftWeapon(bool bShiftUp)
{
	local int SelectedItem;
	local int SwapItem;
	local int NewIndex;

	SelectedItem = WeaponList.Index;

	if(bShiftUp)
	{
		NewIndex = SelectedItem-1;
	}
	else
	{
		NewIndex = SelectedItem+1;
	}

	if(NewIndex >= 0 && NewIndex < MenuDataStore.WeaponPriority.length)
	{
		SwapItem = MenuDataStore.WeaponPriority[NewIndex];
		MenuDataStore.WeaponPriority[NewIndex] = MenuDataStore.WeaponPriority[SelectedItem];
		MenuDataStore.WeaponPriority[SelectedItem] = SwapItem;

		WeaponList.RefreshSubscriberValue();
		WeaponList.SetIndex(NewIndex);
	}
}

/** Resets the weapon order to its defaults. */
function OnResetToDefaults()
{
	local int WeaponIdx;

	// Initialize the weapon priority array
	MenuDataStore.WeaponPriority.length = 0;
	for(WeaponIdx=0; WeaponIdx<MenuDataStore.GetProviderCount('Weapons'); WeaponIdx++)
	{
		MenuDataStore.WeaponPriority.AddItem(WeaponIdx);
	}

	WeaponIdx = WeaponList.Index;
	WeaponList.RefreshSubscriberValue();
	WeaponList.SetIndex(WeaponIdx);
}


/** Button bar callbacks. */
function bool OnButtonBar_ShiftUp(UIScreenObject InButton, int PlayerIndex)
{
	OnShiftWeapon(true);

	return true;
}

function bool OnButtonBar_ShiftDown(UIScreenObject InButton, int PlayerIndex)
{
	OnShiftWeapon(false);

	return true;
}

function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	OnBack();

	return true;
}

function bool OnButtonBar_ResetToDefaults(UIScreenObject InButton, int PlayerIndex)
{
	OnResetToDefaults();

	return true;
}



/**
 * Provides a hook for unrealscript to respond to input using actual input key names (i.e. Left, Tab, etc.)
 *
 * Called when an input key event is received which this widget responds to and is in the correct state to process.  The
 * keys and states widgets receive input for is managed through the UI editor's key binding dialog (F8).
 *
 * This delegate is called BEFORE kismet is given a chance to process the input.
 *
 * @param	EventParms	information about the input event.
 *
 * @return	TRUE to indicate that this input key was processed; no further processing will occur on this input key event.
 */
function bool HandleInputKey( const out InputEventParameters EventParms )
{
	local bool bResult;

	bResult = false;

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_X')
		{
			OnResetToDefaults();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_LeftShoulder')
		{
			OnShiftWeapon(true);
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_RightShoulder')
		{
			OnShiftWeapon(false);
			bResult=true;
		}
	}

	return bResult;
}


defaultproperties
{

}

