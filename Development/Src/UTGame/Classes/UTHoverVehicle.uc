/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTHoverVehicle extends UTVehicle
	native(Vehicle)
	abstract;

/** indicies into VehicleEffects array of ground effects that have their 'DistToGround' parameter set via C++ */
var array<int> GroundEffectIndices;
/** maximum distance vehicle must be from the ground for ground effects to be displayed */
var float MaxGroundEffectDist;
/** particle parameter for the ground effect, set to the ground distance divided by MaxGroundEffectDist (so 0.0 to 1.0) */
var name GroundEffectDistParameterName;

/** scaling factor for this hover vehicle's gravity - used in GetGravityZ() */
var(Movement) float CustomGravityScaling;

/** scaling factor for this hover vehicle's gravity when above StallZ */
var(Movement) float StallZGravityScaling;

/** max speed */
var(Movement) float FullAirSpeed;

cpptext
{
	virtual void TickSpecial(FLOAT DeltaTime);
	virtual FLOAT GetGravityZ();
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		CustomGravityScaling;
}

/**
 * Call this function to blow up the vehicle
 */
simulated function BlowupVehicle()
{
	Super.BlowupVehicle();
	CustomGravityScaling= 1.0;
	if ( UTVehicleSimHover(SimObj) != None )
	{
		UTVehicleSimHover(SimObj).bDisableWheelsWhenOff = true;
	}
}

defaultproperties
{
	CustomGravityScaling=1.0
	MaxGroundEffectDist=256.0
	GroundEffectDistParameterName=DistToGround
	bNoZSmoothing=false
	CollisionDamageMult=0.0008
	StallZGravityScaling=1.0
}
