/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Extended version of console that only allows the quick console to be open if there are no UI scenes open, this is to allow UI scenes to process the TAB key.
 */
class UTConsole extends Console;

/**
 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
 *
 * @param	ControllerId	the controller that generated this input key event
 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
 * @param	EventType		the type of event which occured (pressed, released, etc.)
 * @param	AmountDepressed	for analog keys, the depression percent.
 *
 * @return	true to consume the key event, false to pass it on.
 */
event bool InputKey( int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE )
{
	local bool bResult;
	local UTGameUISceneClient UTSceneClient;

	bResult = false;

	if(Key == ConsoleKey && Event == IE_Pressed)
	{
		GotoState('Open');
		bResult = true;
	}
	else if(Key == TypeKey && Event == IE_Pressed )	// Only show the quick console if there are no UI scenes open that are accepting input.
	{
		UTSceneClient = UTGameUISceneClient(class'UIRoot'.static.GetSceneClient());

		if(UTSceneClient==None || UTSceneClient.IsUIAcceptingInput()==false)
		{
			GotoState('Typing');
			bResult = true;
		}
		else
		{
			return false;
		}
	}

	if(bResult==false)
	{
		bResult = Super.InputKey(ControllerId, Key, Event, AmountDepressed, bGamepad );
	}

	return bResult;
}
