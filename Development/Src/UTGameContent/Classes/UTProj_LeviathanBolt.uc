/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_LeviathanBolt extends UTProjectile;

var vector TargetLoc;
var vector InitialDir;

replication
{
    if( bNetInitial && (Role==ROLE_Authority) )
        TargetLoc, InitialDir;
}

/**
 * Clean up
 */
simulated function Shutdown()
{
	Super.ShutDown();
	SetTimer(0.0,false);
}
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetTimer(0.1,true);
}

simulated function Timer()
{
    local vector ForceDir;
    local float VelMag;

    if ( InitialDir == vect(0,0,0) )
        InitialDir = Normal(Velocity);

	Acceleration = vect(0,0,0);

	ForceDir = Normal(TargetLoc - Location);

	if( (ForceDir Dot InitialDir) > 0 )
	{
		VelMag = VSize(Velocity);
		ForceDir = Normal(ForceDir * 0.9 * VelMag + Velocity);
		Velocity =  VelMag * ForceDir;
		Acceleration += 5 * ForceDir;
	}
	
	SetRotation( Rotator(Normal(Velocity)) );
	
}

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_Bolt'
	ProjExplosionTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_BoltImpact'

    Speed=1200
    MaxSpeed=3500
    AccelRate=20000.0

    Damage=20
    DamageRadius=100
    MomentumTransfer=4000

    MyDamageType=class'UTDmgType_LeviathanBolt'
    LifeSpan=1.6

    bCollideWorld=true
    DrawScale=1.2

	Components.Remove(ProjectileMesh)
}
