/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's input settings.
 */

class UTUITabPage_InputSettings extends UTUITabPage_Options
	placeable;

/** Reference to the PC keybinding scene. */
var string	KeyBindingScenePC;

/** Reference to the 360 keybinding scene. */
var string	KeyBindingScene360;

/** Reference to the PS3 keybinding scene. */
var string	KeyBindingScenePS3;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.Settings.Input>");
}

/** Callback allowing the tabpage to setup the button bar for the current scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ConfigureControls>", OnButtonBar_ConfigureControls);
}


/** Displays the key binding scene. */
function OnShowKeyBindingScene()
{
	local UTUIScene OwnerUTScene;
	OwnerUTScene = UTUIScene(GetScene());

	switch(class'UTUIScene'.static.GetPlatform())
	{
	case UTPlatform_PC:
		OwnerUTScene.OpenSceneByName(KeyBindingScenePC);
		break;
	case UTPlatform_360:
		OwnerUTScene.OpenSceneByName(KeyBindingScene360);
		break;
	case UTPlatform_PS3:
		OwnerUTScene.OpenSceneByName(KeyBindingScenePS3);
		break;
	}
}


function bool OnButtonBar_ConfigureControls(UIScreenObject InButton, int PlayerIndex)
{
	OnShowKeyBindingScene();

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
			OnShowKeyBindingScene();
			bResult=true;
		}
	}

	return bResult;
}

defaultproperties
{
	KeyBindingScenePC="UI_Scenes_FrontEnd.Scenes.BindKeysPC"	
	KeyBindingScene360="UI_Scenes_FrontEnd.Scenes.BindKeys360"	
	KeyBindingScenePS3="UI_Scenes_FrontEnd.Scenes.BindKeysPS3"
}