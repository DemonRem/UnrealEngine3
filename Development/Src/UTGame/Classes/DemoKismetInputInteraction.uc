/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** interaction that checks for input matching any SeqEvent_DemoInput and if so activates them */
class DemoKismetInputInteraction extends Interaction
	within DemoPlayerController;

var array<SeqEvent_DemoInput> InputEvents;

event bool InputKey(int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE)
{
	local int i;

	for (i = 0; i < InputEvents.length; i++)
	{
		if (InputEvents[i].CheckInput(Outer, ControllerId, Key, Event))
		{
			return true;
		}
	}

	return false;
}
