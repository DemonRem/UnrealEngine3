/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * This is a movable blocking volume. It can be moved by matinee, being based on
 * dynamic objects, etc.
 */
class DynamicBlockingVolume extends BlockingVolume
	native
	placeable;

cpptext
{
	virtual void CheckForErrors();
}


defaultproperties
{
	//@todo - Change back to PHYS_None
	Physics=PHYS_Interpolating

	bStatic=false

	bAlwaysRelevant=true
	bReplicateMovement=true
	bOnlyDirtyReplication=true
	RemoteRole=ROLE_None

	BrushColor=(R=255,G=255,B=100,A=255)
}
