/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTHoldSpot extends UTDefensePoint
	notplaceable;

var UTVehicle HoldVehicle;

function PreBeginPlay()
{
	Super(NavigationPoint).PreBeginPlay();
}

function Actor GetMoveTarget()
{
	if ( HoldVehicle != None )
	{
		if ( HoldVehicle.Health <= 0 )
			HoldVehicle = None;
		if ( HoldVehicle != None )
			return HoldVehicle.GetMoveTargetFor(None);
	}

	return self;
}

function FreePoint()
{
	Destroy();
}

defaultproperties
{
	bCollideWhenPlacing=false
	bStatic=false
	bNoDelete=false
}
