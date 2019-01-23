/**
 * Version of KActor that can be dynamically spawned and destroyed during gameplay
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 **/
class KActorSpawnable extends KActor
	native(Physics)
	notplaceable;

simulated function Initialize()
{
	SetHidden(FALSE);
	StaticMeshComponent.SetHidden(FALSE);
	SetPhysics(PHYS_RigidBody);
	SetCollision(true, false);
}

simulated function Recycle()
{
	SetHidden(TRUE);
	StaticMeshComponent.SetHidden(TRUE);
	SetPhysics(PHYS_None);
	SetCollision(false, false);
	ClearTimer('Recycle');
}

/** Used when the actor is pulled from a cache for use. */
native function ResetComponents();

defaultproperties
{
	bNoDelete=FALSE
}
