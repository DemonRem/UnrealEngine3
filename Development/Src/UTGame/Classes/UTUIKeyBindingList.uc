/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Keybinding List, exposes a set of UTUIDataProvider_KeyBinding objects to the user so they can bind keys.
 */

class UTUIKeyBindingList extends UTUIOptionList
	placeable
	native(UIFrontEnd);

cpptext
{
protected:
	/**
	 * Handles input events for this widget.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );
}

/** Number of buttons to display for each key binding. */
var transient int NumButtons;

/** Whether or not we should try to bind the next key. */
var transient bool bCurrentlyBindingKey;

/** Which button we are currently rebinding. */
var transient UIObject CurrentlyBindingObject;

/** The previous binding of the object we are rebinding. */
var transient name PreviousBinding;

/** Reference to the message box scene. */
var transient UTUIScene_MessageBox MessageBoxReference;

/** Returns the player input object for the player that owns this widget. */
event PlayerInput GetPlayerInput()
{
	local LocalPlayer LP;
	local PlayerInput Result;

	Result = none;
	LP = GetPlayerOwner();

	if(LP != none && LP.Actor != none)
	{
		Result = LP.Actor.PlayerInput;
	}

	return Result;
}

/** Generates widgets for all of the options. */
native function RegenerateOptions();

/** Repositions all of the visible options. */
native function RepositionOptions();

/** Refreshes the binding labels for all of the buttons. */
native function RefreshBindingLabels();

/** Post initialize, binds callbacks for all of the generated options. */
event PostInitialize()
{
	local int ObjectIdx;

	Super.PostInitialize();
	
	// Go through all of the generated object and set the OnValueChanged delegate.
	for(ObjectIdx=0; ObjectIdx < GeneratedObjects.length; ObjectIdx++)
	{
		GeneratedObjects[ObjectIdx].OptionObj.OnClicked = OnClicked;
	}
}

/** Unbinds the specified key. */
function UnbindKey(name BindName)
{
	local PlayerInput PInput;
	local int BindingIdx;

	PInput = GetPlayerInput();

	for(BindingIdx = 0;BindingIdx < PInput.Bindings.Length;BindingIdx++)
	{
		if(PInput.Bindings[BindingIdx].Name == BindName)
		{
			PInput.Bindings.Remove(BindingIdx, 1);
			PInput.SaveConfig();
			break;
		}
	}
}

/** Callback for all of the options we generated. */
function bool OnClicked( UIScreenObject Sender, int PlayerIndex )
{
	local UILabelButton BindingButton;

	// Cancel the object we were previously binding.
	if(CurrentlyBindingObject==None && MessageBoxReference==None)
	{
		// Rebind the key
		bCurrentlyBindingKey = true;
		CurrentlyBindingObject = UIObject(Sender);

		// Unbind the key that was clicked on.
		BindingButton = UILabelButton(CurrentlyBindingObject);

		if(BindingButton != none)
		{
			PreviousBinding = name(BindingButton.GetDataStoreBinding());
			BindingButton.SetDataStoreBinding("<Strings:UTGameUI.Settings.PressKeyToBind>");

			// Display the bind key dialog.
			MessageBoxReference = UTUIScene(GetScene()).GetMessageBoxScene();
			MessageBoxReference.OnMBInputKey = OnBindKey_InputKey;
			MessageBoxReference.FadeDuration = 0.125f;
			MessageBoxReference.DisplayModalBox("<Strings:UTGameUI.MessageBox.BindKey_Message>","<Strings:UTGameUI.MessageBox.BindKey_Title>",0.0f);
		}
	}

	return true;
}

/** Finishes the binding process by clearing variables and closing the bind key dialog. */
function FinishBinding()
{
	bCurrentlyBindingKey = FALSE;
	CurrentlyBindingObject = None;

	// Close the modal dialog.
	MessageBoxReference.OnClosed = OnBindKey_Closed;
	MessageBoxReference.Close();
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
function bool OnBindKey_InputKey( const out InputEventParameters EventParms )
{
	return HandleInputKey(EventParms);
}

/** Callback for when the bindkey dialog has finished closing. */
function OnBindKey_Closed()
{
	MessageBoxReference.OnClosed = None;
	MessageBoxReference.OnMBInputKey = None;
	MessageBoxReference=None;
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
	local PlayerInput PInput;
	local int ObjectIdx;
	local UTUIDataProvider_KeyBinding KeyBindingProvider;

	bResult=false;

	// If we are currently binding a key, then 
	if(bCurrentlyBindingKey)
	{
		if(EventParms.bShiftPressed && EventParms.InputKeyName=='Escape')
		{
			FinishBinding();
		}
		else if(EventParms.EventType==IE_Released)
		{
			// Reoslve the option name
			ObjectIdx = GetObjectInfoIndexFromObject(CurrentlyBindingObject);

			if(ObjectIdx != INDEX_NONE)
			{
				KeyBindingProvider = UTUIDataProvider_KeyBinding(GeneratedObjects[ObjectIdx].OptionProvider);

				if(KeyBindingProvider != none)
				{
					PInput = GetPlayerInput();

					if(PInput != none)
					{
						`Log("UTUIKeyBindingList::HandleInputKey() - Binding key '" $ EventParms.InputKeyName $ "' to command '" $ KeyBindingProvider.Command $ "'");
						UnbindKey(PreviousBinding);
						PInput.SetBind(EventParms.InputKeyName, KeyBindingProvider.Command);
					}

					RefreshBindingLabels();
				}
			}

			FinishBinding();
		}

		bResult = TRUE;
	}

	return bResult;
}

defaultproperties
{
	NumButtons=2
}

