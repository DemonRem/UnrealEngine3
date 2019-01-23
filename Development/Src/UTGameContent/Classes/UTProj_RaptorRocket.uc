/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_RaptorRocket extends UTProj_SeekingRocket;

/** The last time a lock message was sent */
var float	LastLockWarningTime;

/** How long before re-sending the next Lock On message update */
var float	LockWarningInterval;

simulated function float TrackingStrength(vector ForceDir)
{
	// track air vehicles better, especially if from behind
	if ( UTAirVehicle(Seeking) != None )
	{
		return ((ForceDir Dot vector(Seeking.Rotation)) > 0.7) ? 2.0 : 1.2;
	}
	return Super.TrackingStrength(ForceDir);
}

simulated function Timer()
{
	local UTAirVehicle VehicleTarget;

	// possibly warn target
	VehicleTarget = UTAirVehicle(Seeking);
	if ( VehicleTarget !=None )
	{
		if ( WorldInfo.TimeSeconds - LastLockWarningTime > LockWarningInterval )
		{
			LastLockWarningTime = WorldInfo.TimeSeconds;

			// warning message for players
			VehicleTarget.LockOnWarning(self);
			
			// update LockWarningInterval based on target proximity
			LockWarningInterval = FClamp(0.25*VSize(Location - VehicleTarget.Location)/VSize(Velocity), 0.1, 1.5);
		}
	}

	super.timer();
}

defaultproperties
{
    MyDamageType=class'UTDmgType_RaptorRocket'

	ProjFlightTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Rocket_trail'
    ProjExplosionTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_RocketExplosion'

    speed=2000.0
    MaxSpeed=4000.0
    AccelRate=16000.0

    Damage=100.0
    DamageRadius=150.0

    MomentumTransfer=50000
	LockWarningInterval=1.5
}
