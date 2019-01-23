/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_SeekingRocket extends UTProj_Rocket;

var Actor Seeking;
var vector InitialDir;

replication
{
    if( bNetInitial && (Role==ROLE_Authority) )
        Seeking, InitialDir;
}

simulated function float TrackingStrength(vector ForceDir)
{
	// track vehicles better
	return (UTVehicle(Seeking) != None) ? 0.8 : 0.6;
}

/**
 * Clean up
 */
simulated function Shutdown()
{
	Super.ShutDown();

	SetTimer(0.0,false);
}

simulated function Timer()
{
    local vector ForceDir;
    local float VelMag;
    local vector SeekingVector;

    if ( InitialDir == vect(0,0,0) )
        InitialDir = Normal(Velocity);

	Acceleration = vect(0,0,0);
    if ( (Seeking != None) && (Seeking != Instigator) )
    {
		SeekingVector = (UTVehicle(Seeking) != None) ? UTVehicle(Seeking).GetTargetLocation(self) : Seeking.Location;
		ForceDir = Normal(SeekingVector - Location);

		if( (ForceDir Dot InitialDir) > 0 )
		{
			VelMag = VSize(Velocity);
			ForceDir = Normal(ForceDir * TrackingStrength(ForceDir) * VelMag + Velocity);
			Velocity =  VelMag * ForceDir;
			Acceleration += 5 * ForceDir;
		}
    }
	else if ( Role == ROLE_Authority )
		SetTimer(0.0, false);
}

simulated function PostBeginPlay()
{
    Super.PostBeginPlay();
    SetTimer(0.1, true);
}

defaultproperties
{
    MyDamageType=class'UTDmgType_SeekingRocket'
    LifeSpan=8.000000
    bRotationFollowsVelocity=true
	CheckRadius=20.0
}
