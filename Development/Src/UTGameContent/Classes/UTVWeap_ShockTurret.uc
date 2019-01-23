/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_ShockTurret extends UTVehicleWeapon
	HideDropDown;

/** This array holds the list of current balls available */
var array<UTProj_VehicleShockBall> ShockBalls;

/** This is the mininum aim variance for auto aiming */
var float MinAim;

/** AI flag - set by shock ball that reaches target to tell bot to do the combo */
var bool bDoCombo;
/** shock ball bot will shoot at when it wants to do the combo */
var UTProj_VehicleShockBall ComboTarget;

function byte BestMode()
{
	local UTBot B;

	if (bDoCombo && ComboTarget != None && !ComboTarget.bDeleteMe)
	{
		B = UTBot(Instigator.Controller);
		if (B == None || B.Enemy == None || B.Enemy != B.Focus || B.LineOfSightTo(B.Enemy))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

simulated function Rotator GetAdjustedAim(vector StartFireLoc)
{
	// if ready to combo, aim at shockball
	if (UTBot(Instigator.Controller) != None && CurrentFireMode == 1 && bDoCombo && ComboTarget != None && !ComboTarget.bDeleteMe)
	{
		return rotator(ComboTarget.Location - StartFireLoc);
	}
	else
	{
		return Super.GetAdjustedAim(StartFireLoc);
	}
}

function SetComboTarget(UTProj_VehicleShockBall ShockBall)
{
	if (UTBot(Instigator.Controller) != None && Instigator.Controller.Enemy != None)
	{
		ComboTarget = ShockBall;
		ShockBall.Monitor(UTBot(Instigator.Controller).Enemy);
	}
}

simulated function Projectile ProjectileFire()
{
	local UTProj_VehicleShockBall SB;
	local vector VelDir;

	// Add it to the array to track

	SB = UTProj_VehicleShockBall(Super.ProjectileFire());
	if (SB!=none)
	{
		ShockBalls[ShockBalls.Length] = SB;
		AimingTraceIgnoredActors[AimingTraceIgnoredActors.length] = SB;
		SB.InstigatorWeapon = self;
		SetComboTarget(SB);
		if ( (MyVehicle.Velocity dot SB.Velocity) > 0 )
		{
			// increase shock ball velocity based on momentum imparted by vehicle
			VelDir = Normal(SB.Velocity);
			SB.Velocity = VelDir * (SB.Speed + 0.5*(MyVehicle.Velocity dot VelDir));
		}
	}

	return SB;
}

/**
 * We override InstantFire (copied from UTVehicle) and adjust the endtrace if there is a good shockball
 * to combo against
 */

simulated function InstantFire()
{
	local vector					StartTrace, EndTrace;
	local array<ImpactInfo>			ImpactsList;
	local int						Idx, x;
	local ImpactInfo				RealImpact;
	local float						BestAim, CurAim;
	local vector					WeaponAim;
	local UTProj_VehicleShockBall	BestProj;

	// define range to use for CalcWeaponFire()
	StartTrace = MyVehicle.GetPhysicalFireStartLoc(self);
	WeaponAim = vector(MyVehicle.GetWeaponAim(self));
	EndTrace = StartTrace + WeaponAim * GetTraceRange();

	// Look to see if there is a ShockBall to auto-aim at
	BestAim = MinAim;
	for (x=0; x < ShockBalls.Length; x++)
	{
		// Remove old entries
		if (ShockBalls[x] == None)
		{
			ShockBalls.Remove(x,1);
			x--;
		}
		else
		{
			// Who is the best case
			CurAim = Normal(ShockBalls[x].Location - StartTrace) dot WeaponAim;
			if (CurAim > BestAim)
			{
				BestAim = CurAim;
				BestProj = ShockBalls[x];
			}
		}
	}

	// If we found one, set the trace
	if (BestProj != None)
	{
		EndTrace = BestProj.Location;
		bDoCombo = false;
	}

	// Perform shot
	RealImpact = CalcWeaponFire( StartTrace, EndTrace, ImpactsList );

	// Set flash location to trigger client side effects.
	// if HitActor == None, then HitLocation represents the end of the trace (maxrange)
	// Remote clients perform another trace to retrieve the remaining Hit Information (HitActor, HitNormal, HitInfo...)
	// Here, The final impact is replicated. More complex bullet physics (bounce, penetration...)
	// would probably have to run a full simulation on remote clients.
	SetFlashLocation( RealImpact.HitLocation );

	// Process all Instant Hits on local player and server (gives damage, spawns any effects).
	for( Idx=0; Idx<ImpactsList.Length; Idx++ )
	{
		ProcessInstantHit( CurrentFireMode, ImpactsList[Idx] );
	}
}


defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_BallFire'
	WeaponProjectiles(0)=class'UTProj_VehicleShockBall'
	FireInterval(0)=+0.40
	ShotCost(0)=0
	bInstantHit=true
	WeaponFireTypes(1)=EWFT_InstantHit
	WeaponFireSnd[1]=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_BeamFire'
	InstantHitDamageTypes(1)=class'UTDmgType_VehicleShockBeam'
	InstantHitDamage(1)=25
	FireInterval(1)=+0.75
	ShotCost(1)=0

	FireTriggerTags=(ShockTurretFire)
	AltFireTriggerTags=(ShockTurretAltFire)
	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_PrimeImpact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')

	MinAim=0.925
	AimingHelpModifier=0.0
}
