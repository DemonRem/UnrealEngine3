/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_DarkWalkerTurret extends UTVehicleWeapon
	HideDropDown;

var float FireTime;
var float RechargeTime;

var bool bFromTimer;

var MaterialImpactEffect VehicleHitEffect;

/** interpolation speeds for zoom in/out when firing */
var() protected float ZoomInInterpSpeed;
var() protected float ZoomOutInterpSpeed;


simulated static function MaterialImpactEffect GetImpactEffect(Actor HitActor, PhysicalMaterial HitMaterial, byte FireModeNum)
{
	return (FireModeNum == 0 && Vehicle(HitActor) != None) ? default.VehicleHitEffect : Super.GetImpactEffect(HitActor, HitMaterial, FireModeNum);
}

simulated function DrawAmmoCount( Hud H )
{
	local UTHud UTH;
	local float x,y;

	UTH = UTHud(H);

	if ( UTH != None )
	{
		x = UTH.Canvas.ClipX - (5 * (UTH.Canvas.ClipX / 1024));
		y = UTH.Canvas.ClipY - (5 * (UTH.Canvas.ClipX / 1024));

		UTH.DrawChargeWidget(X, Y, IconX, IconY, IconWidth, IconHeight, GetPowerPerc(), true);
	}
}

simulated event float GetPowerPerc()
{
	return 1.0;
}

reliable client function ClientStopBeamFiring();

function StopBeamFiring();


/*********************************************************************************************
 * State WeaponFiring
 * See UTWeapon.WeaponFiring
 *********************************************************************************************/

simulated state WeaponBeamFiring
{
	simulated event float GetPowerPerc()
	{
		return 1.0 - GetTimerCount('FiringDone') / GetTimerRate('FiringDone');
	}

	simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
	{
		if ( (ROLE == ROLE_Authority) && (Impact.HitActor != None) )
		{
			if (bFromTimer || UTPawn(Impact.HitActor) != none)
			{
				Impact.HitActor.TakeDamage( InstantHitDamage[0], Instigator.Controller,	Impact.HitLocation,
								InstantHitMomentum[0] * Impact.RayDir,	InstantHitDamageTypes[0], Impact.HitInfo, self );
			}
		}
	}

	reliable client function ClientStopBeamFiring()
	{
		if ( Role < ROLE_Authority )
		{
			FiringDone();
		}
	}

	function StopBeamFiring()
	{
		ClientStopBeamFiring();
		FiringDone();
	}

	simulated function RefireCheckTimer()
	{
		bFromTimer=true;
		InstantFire();
		bFromTimer=false;
	}

	/**
	 * Update the beam and handle the effects
	 */
	simulated function Tick(float DeltaTime)
	{
		// Retrace everything and see if there is a new LinkedTo or if something has changed.
		InstantFire();
	}

	/**
	 * In this weapon, RefireCheckTimer consumes ammo and deals out health/damage.  It's not
	 * concerned with the effects.  They are handled in the tick()
	 */
	simulated function FiringDone()
	{
		Gotostate('WeaponBeamRecharging');
	}

	simulated function EndFire(byte FireModeNum)
	{
		Global.EndFire(FireModeNum);
	}

	simulated function BeginState( Name PreviousStateName )
	{
		local UTPlayerController PC;

		InstantFire();
		SetTimer(FireTime,false,'FiringDone');
		TimeWeaponFiring(0);

		PC = UTPlayerController(Instigator.Controller);
		if ( (PC != None) && (LocalPlayer(PC.Player) != None) )
		{
			PC.StartZoomNonlinear(ZoomedTargetFOV, ZoomInInterpSpeed);
		}

	}


	/**
	 * When leaving the state, shut everything down
	 */
	simulated function EndState(Name NextStateName)
	{
		local UTPlayerController PC;

		ClearTimer('FiringDone');
		ClearTimer('RefireCheckTimer');
		ClearFlashLocation();

		PC = UTPlayerController(Instigator.Controller);
		if ( (PC != None) && (LocalPlayer(PC.Player) != None) )
		{
			PC.EndZoomNonlinear(ZoomOutInterpSpeed);
		}

		super.EndState(NextStateName);
	}

	simulated function bool IsFiring()
	{
		return true;
	}
}

simulated state WeaponBeamRecharging
{
	simulated event float GetPowerPerc()
	{
		return GetTimerCount('RechargeDone') / GetTimerRate('RechargeDone');
	}

	simulated function BeginState( Name PreviousStateName )
	{
		SetTimer(FireTime,false,'RechargeDone');
	}

	/**
	 * In this weapon, RefireCheckTimer consumes ammo and deals out health/damage.  It's not
	 * concerned with the effects.  They are handled in the tick()
	 */
	simulated function RechargeDone()
	{
		Gotostate('Active');
	}
}

defaultproperties
{
	WeaponFireTypes(0)=EWFT_InstantHit
	WeaponFireTypes(1)=EWFT_None
	FiringStatesArray(0)=WeaponBeamFiring

	InstantHitMomentum(0)=+60000.0
	InstantHitDamage(0)=120
	FireInterval(0)=+0.2

	ZoomedTargetFOV=55			// hijacked from UTWeapon
	ZoomInInterpSpeed=6.f
	ZoomOutInterpSpeed=3.f

	bInstantHit=true

	WeaponFireSnd[0]=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FireBeamCue'
	InstantHitDamageTypes(0)=class'UTDmgType_DarkWalkerTurretBeam'

	ShotCost(0)=0
	ShotCost(1)=0

	FireTriggerTags=(MainTurretFire)

	FireTime=1.5
	RechargeTime=0.5

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Beam_Impact')
	VehicleHitEffect=(ParticleTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Beam_Impact_Damage')
	AmmoDisplayType=EAWDS_BarGraph
}
