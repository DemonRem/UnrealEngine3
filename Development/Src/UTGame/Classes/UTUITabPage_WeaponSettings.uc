/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's weapon settings.
 */

class UTUITabPage_WeaponSettings extends UTUITabPage_Options
	placeable;


/** Reference to the weapon preference scene. */
var string	WeaponPreferenceScene;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.Settings.Weapons>");
}


/** Callback allowing the tabpage to setup the button bar for the current scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.WeaponPreference>", OnButtonBar_WeaponPreference);
}



/** Displays the key binding scene. */
function OnShowWeaponPreferenceScene()
{
	UTUIScene(GetScene()).OpenSceneByName(WeaponPreferenceScene);
}


function bool OnButtonBar_WeaponPreference(UIScreenObject InButton, int PlayerIndex)
{
	OnShowWeaponPreferenceScene();

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
		if(EventParms.InputKeyName=='XboxTypeS_Y')
		{
			OnShowWeaponPreferenceScene();
			bResult=true;
		}
	}

	return bResult;
}

defaultproperties
{
	WeaponPreferenceScene="UI_Scenes_FrontEnd.Scenes.WeaponPreference"
}

