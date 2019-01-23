/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ActorFactoryTrigger extends ActorFactory
	config(Editor)
	collapsecategories
	hidecategories(Object)
	native;

defaultproperties
{
	MenuName="Add Trigger"
	NewActorClass=class'Engine.Trigger'
}
