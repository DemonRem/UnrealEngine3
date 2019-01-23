/**
 * Event which is activated when the user a console command.
 * Originator: the PlayerController for the player that typed the console command
 * Instigator: the pawn of the player that typed the console command.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqEvent_Console extends SequenceEvent
	native(Sequence);

/** Name to match when entered from the console */
var() Name ConsoleEventName;

/** Description to display when listing summary of all console events */
var() string EventDesc;

cpptext
{
protected:
	FString GetCustomTitle() const;

public:
	virtual void PostLoad();
	virtual void PostEditChange(UProperty* PropertyThatChanged);
};

defaultproperties
{
	ObjName="Console Event"
	ObjCategory="Misc"
	ConsoleEventName=Default

	EventDesc="No description"

	MaxTriggerCount=0
}
