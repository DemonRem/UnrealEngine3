/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Settings scene for UT3.
 */
class UTUIFrontEnd_Settings extends UTUIFrontEnd_BasicMenu;

const SETTINGS_OPTION_PLAYER = 0;
const SETTINGS_OPTION_VIDEO = 1;
const SETTINGS_OPTION_AUDIO = 2;
const SETTINGS_OPTION_INPUT = 3;
const SETTINGS_OPTION_NETWORK = 4;
const SETTINGS_OPTION_WEAPONS = 5;
const SETTINGS_OPTION_CREDITS = 6;

/** @todo: Temp reference to the player config scene */
var string	PlayerConfigScene;
var string	NewCharacterScene;
var string	CreditsScene;

/** Reference to the actual scene that holds all of the settings panels. */
var string SettingsPanelsScene;

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.Clear();

	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Select>", OnButtonBar_Select);
}

/**
 * Executes a action based on the currently selected menu item. 
 */
function OnSelectItem(int PlayerIndex=0)
{
	local int SelectedItem;
	local UIScene OpenedScene;
	local UTUIFrontEnd_SettingsPanels SettingsPanelsInstance;
	SelectedItem = MenuList.GetCurrentItem();

	switch(SelectedItem)
	{
	case SETTINGS_OPTION_PLAYER:
		OnShowPlayerScene();
		break;
	case SETTINGS_OPTION_VIDEO:
		OpenedScene = OpenSceneByName(SettingsPanelsScene);
		SettingsPanelsInstance = UTUIFrontEnd_SettingsPanels(OpenedScene);
		SettingsPanelsInstance.TabControl.ActivatePage(SettingsPanelsInstance.VideoTab, PlayerIndex);
		break;
	case SETTINGS_OPTION_AUDIO:
		OpenedScene = OpenSceneByName(SettingsPanelsScene);
		SettingsPanelsInstance = UTUIFrontEnd_SettingsPanels(OpenedScene);
		SettingsPanelsInstance.TabControl.ActivatePage(SettingsPanelsInstance.AudioTab, PlayerIndex);
		break;
	case SETTINGS_OPTION_INPUT:
		OpenedScene = OpenSceneByName(SettingsPanelsScene);
		SettingsPanelsInstance = UTUIFrontEnd_SettingsPanels(OpenedScene);
		SettingsPanelsInstance.TabControl.ActivatePage(SettingsPanelsInstance.InputTab, PlayerIndex);
		break;
	case SETTINGS_OPTION_NETWORK:
		OpenedScene = OpenSceneByName(SettingsPanelsScene);
		SettingsPanelsInstance = UTUIFrontEnd_SettingsPanels(OpenedScene);
		SettingsPanelsInstance.TabControl.ActivatePage(SettingsPanelsInstance.NetworkTab, PlayerIndex);
		break;
	case SETTINGS_OPTION_WEAPONS:
		OpenedScene = OpenSceneByName(SettingsPanelsScene);
		SettingsPanelsInstance = UTUIFrontEnd_SettingsPanels(OpenedScene);
		SettingsPanelsInstance.TabControl.ActivatePage(SettingsPanelsInstance.WeaponsTab, PlayerIndex);
		break;
	case SETTINGS_OPTION_CREDITS:
		OpenSceneByName(CreditsScene);
		break;
	}
}

/** @return bool Returns whether or not this user has saved character data. */
function bool HasSavedCharacterData()
{
	local bool bHaveLoadedCharData;
	local string CharacterDataStr;

	bHaveLoadedCharData = false;

	
	if(GetDataStoreStringValue("<OnlinePlayerData:ProfileData.CustomCharData>", CharacterDataStr, none, GetPlayerOwner()))
	{
		if(Len(CharacterDataStr) > 0)
		{
			bHaveLoadedCharData = true;
		}
	}

	return bHaveLoadedCharData;
}

// @todo: Both of the following functions are temporary until the player settings panel is created.
function OnShowPlayerScene()
{
	local UTUIScene_MessageBox MessageBoxReference;
	local array<string> MessageBoxOptions;

	// Check to see if we have any saved char data
	if(HasSavedCharacterData())
	{
		// Let the user choose to edit their old character or create a new one.
		MessageBoxReference = GetMessageBoxScene();

		if(MessageBoxReference != none)
		{
			MessageBoxOptions.AddItem("<Strings:UTGameUI.CharacterCustomization.CreateNewCharacter>");
			MessageBoxOptions.AddItem("<Strings:UTGameUI.CharacterCustomization.EditExistingCharacter>");

			MessageBoxReference.SetPotentialOptions(MessageBoxOptions);
			MessageBoxReference.Display("<Strings:UTGameUI.CharacterCustomization.CreateNewChar_Message>", 
				"<Strings:UTGameUI.CharacterCustomization.CreateNewChar_Title>", OnStartNewCharacter_Confirm);
		}
	}
	else
	{
		OpenSceneByName(NewCharacterScene);
	}
}


function OnStartNewCharacter_Confirm(int SelectedOption, int PlayerIndex)
{
	if(SelectedOption==0)
	{
		OpenSceneByName(NewCharacterScene);
	}
	else
	{
		OpenSceneByName(PlayerConfigScene);
	}
}

/** Callback for when the user wants to back out of the scene. */
function OnBack()
{
	CloseScene(self);
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();

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

	bResult=false;

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}


defaultproperties
{
	PlayerConfigScene="UI_Scenes_FrontEnd.Scenes.CharacterCustomization"
	NewCharacterScene="UI_Scenes_FrontEnd.Scenes.CharacterFaction"
	CreditsScene="UI_Scenes_FrontEnd.Scenes.Credits"
	SettingsPanelsScene="UI_Scenes_ChrisBLayout.Scenes.SettingsPanels"
}