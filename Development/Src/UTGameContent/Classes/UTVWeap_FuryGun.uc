/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_FuryGun extends UTVehicleWeapon;

var float RechargeTime;
var float RechargePct;

/** how far off crosshair vehicle turret can be and still have beam adjusted to crosshair */
var float BeamMaxAimAdjustment;

var MaterialImpactEffect VehicleHitEffect;

/** stored up fractional damage since we check every tick but health/damage are integers */
var float PartialDamage;
/** last pawn beam successfully hit */
var Pawn LastBeamTarget;

function byte BestMode()
{
	//@FIXME AI support

	return Super.BestMode();
}

function float RefireRate()
{
	// reduce refire rate with the beam altfire
	//@FIXME re-eval
	return (CurrentFireMode == 0) ? BotRefireRate : (BotRefireRate * 0.5);
}

/**********************************************************************
  Secondary Fire
 **********************************************************************/

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

/*********************************************************************************************
 * State WeaponFiring
 * See UTWeapon.WeaponFiring
 *********************************************************************************************/

simulated state WeaponBeamFiring
{
	simulated event bool IsFiring()
	{
		return true;
	}

	simulated event float GetPowerPerc()
	{
		return 1.0 - GetTimerCount('FiringDone') / GetTimerRate('FiringDone');
	}

	/** deals damage/momentum to the target we hit this tick */
	simulated function DealDamageTo(Actor Victim, int Damage, float DeltaTime, vector StartTrace)
	{
		local SVehicle V;

		//@hack: vehicle damage impulse is constant in the damagetype, so have to use this to scale by deltatime
		V = SVehicle(Victim);
		if (V != None)
		{
			V.RadialImpulseScaling = DeltaTime;
		}
		Victim.TakeDamage(Damage, Instigator.Controller, Victim.Location,
					InstantHitMomentum[CurrentFireMode] * DeltaTime * Normal(StartTrace - Victim.Location),
					InstantHitDamageTypes[CurrentFireMode],, self );
	}

	simulated function Tick(float DeltaTime)
	{
		local Vector StartTrace, EndTrace;
		local ImpactInfo IInfo;
		local Array<ImpactInfo>	ImpactList;
		local UTVehicle_Fury UTVFury;

		PartialDamage += InstantHitDamage[CurrentFireMode] * DeltaTime;

		StartTrace = InstantFireStartTrace();
		EndTrace = InstantFireEndTrace(StartTrace);

		// lock onto last hit target if still close to aim
		if (LastBeamTarget != None && Normal(EndTrace - StartTrace) dot Normal(LastBeamTarget.Location - StartTrace) >= LockAim)
		{
			// cause damage
			DealDamageTo(LastBeamTarget, int(PartialDamage), DeltaTime, StartTrace);
		}
		else
		{
			IInfo = CalcWeaponFire(StartTrace, EndTrace, ImpactList);

			// Check for an actual hit.  If there is one, flag it.
			UTVFury = UTVehicle_Fury(MyVehicle);
			if (IInfo.HitActor != None && !IInfo.HitActor.bWorldGeometry)
			{
				if (Role == ROLE_Authority)
				{
					// setup beam effect
					if (StaticMeshComponent(IInfo.HitInfo.HitComponent) != None)
					{
						// if we hit a static mesh, the trace was per poly so we can target the exact location
						UTVFury.BeamLockedInfo.LockedTarget = None;
						Global.SetFlashLocation(IInfo.HitLocation);
					}
					else
					{
						UTVFury.BeamLockedInfo.LockedTarget = IInfo.HitActor;
						UTVFury.BeamLockedInfo.LockedBone = IInfo.HitInfo.BoneName;
						UTVFury.BeamLockOn();
					}
				}

				// cause damage
				DealDamageTo(IInfo.HitActor, PartialDamage, DeltaTime, StartTrace);
				LastBeamTarget = Pawn(IInfo.HitActor);
			}
			else
			{
				LastBeamTarget = None;
				if (Role == ROLE_Authority)
				{
					UTVFury.BeamLockedInfo.LockedTarget = None;
					Global.SetFlashLocation((IInfo.HitActor == None) ? EndTrace : IInfo.HitLocation);
				}
			}
		}

		PartialDamage -= int(PartialDamage);
	}

	/** stubbed out so that CalcWeaponFire() in beam trace doesn't set FlashLocation when we might not want it */
	function SetFlashLocation(vector HitLocation);

	/** called when fire time is up or player lets go of the button */
	simulated function FiringDone()
	{
		GotoState('WeaponBeamRecharging');
	}

	simulated function RefireCheckTimer()
	{
		FiringDone();
	}

	simulated function EndFire(byte FireModeNum)
	{
		RechargePct = GetTimerCount('RefireCheckTimer') / GetTimerRate('RefireCheckTimer');
		Global.EndFire(FireModeNum);
		FiringDone();
	}

	simulated function float GetMaxFinalAimAdjustment()
	{
		return BeamMaxAimAdjustment;
	}

	simulated function BeginState( Name PreviousStateName )
	{
		RechargePct = 1.0;
		TimeWeaponFiring(CurrentFireMode);
		Tick(0.0);
	}

	/**
	 * When leaving the state, shut everything down
	 */
	simulated function EndState(Name NextStateName)
	{
		ClearTimer('RefireCheckTimer');
		UTVehicle_Fury(MyVehicle).BeamLockedInfo.LockedTarget = None;
		ClearFlashLocation();
		LastBeamTarget = None;
		PartialDamage = 0.0;

		Super.EndState(NextStateName);
	}
}

simulated state WeaponBeamRecharging
{
	simulated event float GetPowerPerc()
	{
		return GetTimerCount('RechargeDone') / GetTimerRate('RechargeDone');
	}

	simulated event BeginState(name PreviousStateName)
	{
		if (RechargePct ~= 0.0)
		{
			RechargeDone();
		}
		else
		{
			SetTimer(RechargeTime * RechargePct, false, 'RechargeDone');
		}
	}

	simulated event EndState(name NextStateName)
	{
		ClearTimer('RechargeDone');
	}

	simulated function RechargeDone()
	{
		Gotostate('Active');
	}
}

defaultproperties
{

	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_FuryBolt'

	WeaponFireSnd[0]=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_PrimaryFireCue'
	FireInterval(0)=0.2
	ShotCost(0)=0

	FireTriggerTags=(RaptorWeapon01,RaptorWeapon02)

	LockAim=0.996 // ~5 degrees

	BotRefireRate=0.95

	WeaponFireTypes(1)=EWFT_InstantHit
	FiringStatesArray(1)=WeaponBeamFiring

	FireInterval(1)=1.5
	// these values are per second
	InstantHitMomentum(1)=500000.0
	InstantHitDamage(1)=75

	bInstantHit=true

	WeaponFireSnd[1]=none
	InstantHitDamageTypes(1)=class'UTDmgType_FuryBeam'
	ShotCost(1)=0

	RechargeTime=0.5

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Fury.Effects.P_VH_Fury_Beam_Impact')
	VehicleHitEffect=(ParticleTemplate=ParticleSystem'VH_Fury.Effects.P_VH_Fury_Beam_Impact_Damage')

	AmmoDisplayType=EAWDS_BarGraph

	WeaponRange=3072

	BeamMaxAimAdjustment=0.71 // ~45 degrees
}
