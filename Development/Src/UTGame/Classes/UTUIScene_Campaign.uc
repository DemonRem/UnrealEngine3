/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_Campaign extends UTUIScene
	native(UI)
	abstract;

/** Holds reference to the button bar */

var transient UTUIButtonBar ButtonBar;

event PostInitialize( )
{
	Super.PostInitialize();

	OnRawInputKey=HandleInputKey;

	ButtonBar = UTUIButtonBar(FindChild('ButtonBar',true));
	if ( ButtonBar != none )
	{
		ButtonBar.SetButton(0, "<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	}
}


/**
 * Call when it's time to go back to the previous scene
 */
function bool Scene_Back()
{
	CloseScene(self);
	return true;
}


/**
 * Button bar callbacks - Back Button
 */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	return Scene_Back();
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
			Scene_Back();
			bResult=true;
		}
	}

	return bResult;
}


defaultproperties
{
	bPauseGameWhileActive=false;
}


