/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_LeviathanPrimary extends UTVehicleWeapon;

/** the amount of time the same target must be kept for the shot to go off */
var float PaintTime;

/** How long after the shot does it take to rechage (should be the time of the effect) */
var float RechargeTime;

/** units/sec player can move their aim and not reset the paint timer */
var float TargetSlack;
/** current target location */
var vector TargetLocation;
/** Ambient sound played while acquiring the target */
var SoundCue AcquireSound;

simulated function SendToFiringState( byte FireModeNum )
{
	local UTVehicle_Leviathan MyLevi;

	// make sure fire mode is valid
	if( FireModeNum >= FiringStatesArray.Length )
	{
		WeaponLog("Invalid FireModeNum", "Weapon::SendToFiringState");
		return;
	}

	// Ignore a none fire type
	if( WeaponFireTypes[FireModeNum] == EWFT_None )
	{
		return;
	}


	MyLevi = UTVehicle_Leviathan(MyVehicle);
	if (MyLevi != none)
	{
		// set current fire mode
		SetCurrentFireMode(FireModeNum);

		if ( MyLevi.DeployedState == EDS_Undeployed )
		{
			GotoState('WeaponFiring');
		}
		else if (MyLevi.DeployedState == EDS_Deployed )
		{
			GotoState('WeaponBeamFiring');
		}
	}
}

reliable client function ClientHasFired()
{
	GotoState('WeaponRecharge');
}

simulated state WeaponBeamFiring
{
	simulated function BeginState( Name PreviousStateName )
	{
		Super.BeginState(PreviousStateName);

		if (Role == ROLE_Authority)
		{
			SetTimer(PaintTime,false,'FireWeapon');
		}

		InstantFire();
	}

	function FireWeapon()
	{
		local UTEmit_LeviathanExplosion Blast;

		if (ROLE==ROLE_Authority)
		{
			Blast = spawn(class'UTEmit_LeviathanExplosion',instigator,,TargetLocation + Vect(0,0,32));
			Blast.Instigator = Instigator;
			Blast.Explode();
		}

		if ( !Instigator.IsLocallyControlled() )
		{
			ClientHasFired();
		}
		GotoState('WeaponRecharge');
	}

	/**
	 * When leaving the state, shut everything down
	 */
	simulated function EndState(Name NextStateName)
	{
		ClearTimer('FireWeapon');
		Super.EndState(NextStateName);
	}

	simulated function bool IsFiring()
	{
		return true;
	}

	simulated function SetFlashLocation(vector HitLocation)
	{
		if (ROLE == ROLE_Authority)
		{
			Super.SetFlashLocation(HitLocation);
			TargetLocation = HitLocation;
		}
	}
}

simulated state WeaponRecharge
{
	simulated function BeginState( Name PreviousStateName )
	{
		Super.BeginState(PreviousStateName);
		SetTimer(RechargeTime,false,'Charged');
		TimeWeaponFiring(0);
	}

	simulated function EndState(Name NextStateName)
	{
		ClearTimer('RefireCheckTimer');
		ClearFlashLocation();
		Super.EndState(NextStateName);
	}

	simulated function bool IsFiring()
	{
		return true;
	}

	simulated function Charged()
	{
		GotoState('Active');
	}
}

simulated function Projectile ProjectileFire()
{
	local vector ActualStart,ActualEnd, HitLocation, HitNormal;
	local UTProj_LeviathanBolt Bolt;

	Bolt = UTProj_LeviathanBolt( super.ProjectileFire() );

	if (Bolt != none)
	{
		ActualStart = Instigator.GetWeaponStartTraceLocation();
		ActualEnd = ActualStart + vector(GetAdjustedAim(ActualStart)) * GetTraceRange();

		if ( Trace(HitLocation,HitNormal, ActualEnd, ActualStart) != none )
		{
			ActualEnd = HitLocation;
		}

		Bolt.TargetLoc = ActualEnd;
	}
	return Bolt;
}
simulated function float GetMaxFinalAimAdjustment()
{
	local UTVehicle_Leviathan MyLevi;

	MyLevi = UTVehicle_Leviathan(MyVehicle);
	if ( MyLevi != none && MyLevi.DeployedState == EDS_Deployed )
	{
		return 0.998;
	}
	else
	{
		return Super.GetMaxFinalAimAdjustment();
	}
}

simulated function NotifyVehicleDeployed()
{
	bInstantHit = true;
	AimTraceRange = MaxRange();
}

simulated function NotifyVehicleUndeployed()
{
	bInstantHit = false;
	AimTraceRange = MaxRange();
}

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_Projectile
 	WeaponFireTypes(1)=EWFT_None
	WeaponProjectiles(0)=class'UTProj_LeviathanBolt'
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_TurretFire'
	FireInterval(0)=+0.3
	RechargeTime=3.0
	ShotCost(0)=0
	ShotCost(1)=0
	Spread[0]=0.015

//	FireShake(0)=(RotMag=(Z=250),RotRate=(Z=2500),RotTime=6,OffsetMag=(Z=10),OffsetRate=(Z=200),OffsetTime=10)

	PaintTime=2.0
	TargetSlack=50.0

	FireTriggerTags=(DriverMF_L,DriverMF_R)


}
