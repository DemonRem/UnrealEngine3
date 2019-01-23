/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

//@FIXME: delete - replace with UTOnslaughtPowernode_Content (needs map resave)
class UTOnslaughtConduit_Content extends UTOnslaughtPowernode_Content
	placeable;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	// fixup code for old maps (temp)
	if (FastTrace(Location - ((CylinderComponent.CollisionHeight + 55.0) * vect(0,0,1))))
	{
		SetLocation(Location - vect(0,0,55));
	}
}
