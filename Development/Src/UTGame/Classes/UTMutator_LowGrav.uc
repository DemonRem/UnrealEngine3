// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_LowGrav extends UTMutator;

/** Gravity along Z axis applied to objects in physics volumes which had default gravity. */
var()	float	GravityZ;

function InitMutator(string Options, out string ErrorMessage)
{
	WorldInfo.WorldGravityZ = GravityZ;
	Super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GroupName="JUMPING"
	GravityZ=-100.0
}