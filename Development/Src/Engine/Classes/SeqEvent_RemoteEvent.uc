/**
 * Activated by the ActivateRemoteEvent action.
 * Originator: current WorldInfo
 * Instigator: the actor that is assigned [in editor] as the ActivateRemoteEvent action's Instigator
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqEvent_RemoteEvent extends SequenceEvent
	native(Sequence);

/** Name of this event for remote activation */
var() Name EventName;

defaultproperties
{
	ObjName="Remote Event"

	EventName=DefaultEvent
	bPlayerOnly=FALSE
}
