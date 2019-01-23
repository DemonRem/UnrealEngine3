/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Key binding screen for the PC.
 */
class UTUIFrontEnd_BindKeysPC extends UTUIFrontEnd;

/** Binding list object, we need to route key presses to this first. */
var transient UTUIKeyBindingList BindingList;

/** Post initialize callback. */
event PostInitialize()
{
	Super.PostInitialize();

	// Find widget references
	BindingList = UTUIKeyBindingList(FindChild('lstKeys', true));
}

/** Callback to setup the buttonbar for this scene. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Accept>", OnButtonBar_Accept);
}

/** Callback for when the user wants to exit this screen. */
function OnBack()
{
	CloseScene(self);
}

/** Callback for when the user wants to save their options. */
function OnAccept()
{
	// @todo: Save Options
	CloseScene(self);
}

/** Callback for when the user wants to reset to the default set of option values. */
function OnResetToDefaults()
{
	
}

/** Button bar callbacks. */
function bool OnButtonBar_Accept(UIScreenObject InButton, int PlayerIndex)
{
	OnAccept();

	return true;
}

function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
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

	// Let the binding list get first chance at the input because the user may be binding a key.
	bResult=BindingList.HandleInputKey(EventParms);

	if(bResult == false)
	{
		if(EventParms.EventType==IE_Released)
		{
			if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
			{
				OnBack();
				bResult=true;
			}
		}
	}

	return bResult;
}
