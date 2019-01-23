/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleSimTank extends SVehicleSimTank
	native(Vehicle);

/** Since Tanks osscilate a bit once we hit on target we don't steer as much in the same after we close. */
var bool bForceOnTarget;

/** When driving into something, reduce friction on the wheels. */
var()	float	FrontalCollisionGripFactor;

cpptext
{
	virtual void UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime);
}

DefaultProperties
{
	FrontalCollisionGripFactor=1.0
}
