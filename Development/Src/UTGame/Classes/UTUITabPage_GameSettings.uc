/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's Game settings.
 */

class UTUITabPage_GameSettings extends UTUITabPage_Options
	placeable;

var transient string BotSelectionScene;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_GameSettings>");
}


/** Sets up the button bar for the parent scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	if(GetScene().SceneTag=='InstantAction')
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.BotSelection>", OnButtonBar_BotSelection);
	}
}

/** Shows the bot configuration scene. */
function OnShowBotSelection()
{
	local UTUIFrontEnd_BotSelection OpenedScene;

	OpenedScene = UTUIFrontEnd_BotSelection(UTUIScene(GetScene()).OpenSceneByName(BotSelectionScene));
	OpenedScene.OnAcceptedBots = OnBotsSelected;
}

/** Callback for when the user selects a set of bots. */
function OnBotsSelected()
{
	local int ObjectIndex;

	// Update the number of bots option.
	ObjectIndex = OptionList.GetObjectInfoIndexFromName('NumBots_Common');

	if(ObjectIndex != INDEX_NONE)
	{
		UIDataStoreSubscriber(OptionList.GeneratedObjects[ObjectIndex].OptionObj).RefreshSubscriberValue();
	}
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_BotSelection(UIScreenObject InButton, int PlayerIndex)
{
	OnShowBotSelection();

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
		if(EventParms.InputKeyName=='XboxTypeS_Y')		// Move mutator
		{
			OnShowBotSelection();

			bResult=true;
		}
	}

	return bResult;
}

defaultproperties
{
	BotSelectionScene="UI_Scenes_FrontEnd.Scenes.BotSelection"
}
