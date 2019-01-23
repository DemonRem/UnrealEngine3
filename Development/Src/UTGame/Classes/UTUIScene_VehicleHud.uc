/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_VehicleHud extends UTUIScene_Pawns
	abstract
	native(UI);

/** Holds a reference to it's vehicle */
var UTVehicle MyVehicle;

function NotifyGameSessionEnded()
{
	Super.NotifyGameSessionEnded();
	MyVehicle = none;
}

event SceneDeactivated()
{
	Super.SceneDeactivated();
	MyVehicle = none;
}

function AssociateWithVehicle(UTVehicle NewVehicle)
{
	MyVehicle = NewVehicle;
}


defaultproperties
{
}
