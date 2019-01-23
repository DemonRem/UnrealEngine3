/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * A loaded weapon fires a single shot for normal fire, but allows
 * the player to load up additional shots in alt-fire
 */

class UTLoadedWeapon extends UTWeapon
	abstract;

/** This holds the current number of shots queued up */
var int LoadedShotCount;

/** This holds the maximum number of shots that can be queued */
var int MaxLoadCount;

/*** Remove me.. we shouldn't be needed */
var SoundCue WeaponLoadSnd;
var Soundcue WeaponLoadedSnd;

// LoadedWeapons supported multiple animation paths depending on
// how many rockets have been loaded.  These next few variables
// are used to time those paths.

/** Holds how long it takes to que a given shot */
var array<float> AltFireQueueTimes;

/** Holds how long it takes to fire a given shot */
var array<float> AltFireLaunchTimes;

/** How long does it takes after a shot is fired for the weapon to reset and return to active */
var array<float> AltFireEndTimes;

/** Holds a collection of sound cues that define what sound to play at each loaded level */
var array<SoundCue> AltFireSndQue;

/** Allow for multiple muzzle flash sockets **FIXME: will become offsets */
var array<name> MuzzleFlashSocketList;

/** Holds a list of emitters that make up the muzzle flash */
var array<ParticleSystemComponent> MuzzleFlashPSCList;

/** How much distance should be between each shot */
var int	SpreadDist;

/** How much grace at the end of loading should be given before the weapon auto-fires */
var float GracePeriod;

/** How much of the load-up timer needs to pass before the weapon will wait to fire on release */
var float WaitToFirePct;


/**
  * Adjust weapon equip and fire timings so they match between PC and console
  * This is important so the sounds match up.
  */
simulated function AdjustWeaponTimingForConsole()
{
	local int i;

	Super.AdjustWeaponTimingForConsole();

	For ( i=0; i<AltFireQueueTimes.Length; i++ )
	{
		AltFireQueueTimes[i] = AltFireQueueTimes[i]/1.1;
	}
	For ( i=0; i<AltFireEndTimes.Length; i++ )
	{
		AltFireEndTimes[i] = AltFireEndTimes[i]/1.1;
	}
	For ( i=0; i<AltFireLaunchTimes.Length; i++ )
	{
		AltFireLaunchTimes[i] = AltFireLaunchTimes[i]/1.1;
	}
}

/**
 * @Returns 	The amount of spread
 */
simulated function int GetSpreadDist()
{
	return SpreadDist;
}

/**
 * Fire off a load of projectiles.
 *
 * Network: Server Only
 */
function FireLoad()
{
	local int i;
	local vector SpreadVector;
	local rotator Aim;
	local float theta;
   	local vector RealStartLoc, X,Y,Z;
	local Projectile	SpawnedProjectile;

	SetCurrentFireMode(1);
	IncrementFlashCount();

	// this is the location where the projectile is spawned
	RealStartLoc = GetPhysicalFireStartLoc();

	Aim = GetAdjustedAim( RealStartLoc );			// get fire aim direction

	GetViewAxes(X,Y,Z);

	UpdateFiredStats(LoadedShotCount);

	for (i = 0; i < LoadedShotCount; i++)
	{
		// Give them some gradual spread.
		theta = GetSpreadDist() * PI / 32768.0 * (i - float(LoadedShotCount - 1) / 2.0);
		SpreadVector.X = Cos(theta);
		SpreadVector.Y = Sin(theta);
		SpreadVector.Z = 0.0;
		SpawnedProjectile = Spawn(GetProjectileClass(),,, RealStartLoc, Rotator(SpreadVector >> Aim));
		if ( SpawnedProjectile != None )
		{
			SpawnedProjectile.Init(SpreadVector >> Aim);
			UTProjectile(SpawnedProjectile).InitStats(self);
		}
	}
}

/*********************************************************************************************
 * AI Interface
 *********************************************************************************************/

function float SuggestAttackStyle()
{
	local float EnemyDist;

	// recommend backing off if target is too close
	EnemyDist = VSize(Instigator.Controller.Enemy.Location - Owner.Location);
	if ( EnemyDist < 750 )
	{
		if ( EnemyDist < 500 )
			return -1.5;
		else
			return -0.7;
	}
	else if ( EnemyDist > 1600 )
		return 0.5;
	else
		return -0.1;
}

// tell bot how valuable this weapon would be to use, based on the bot's combat situation
// also suggest whether to use regular or alternate fire mode
function float GetAIRating()
{
	local UTBot B;
	local float EnemyDist, Rating, ZDiff;
	local vector EnemyDir;

	B = UTBot(Instigator.Controller);
	if ( (B == None) || (B.Enemy == None) )
		return AIRating;

	// if standing on a lift, make sure not about to go around a corner and lose sight of target
	// (don't want to blow up a rocket in bot's face)
	if ( (Instigator.Base != None) && (Instigator.Base.Velocity != vect(0,0,0))
		&& !B.CheckFutureSight(0.1) )
		return 0.1;

	EnemyDir = B.Enemy.Location - Instigator.Location;
	EnemyDist = VSize(EnemyDir);
	Rating = AIRating;

	// don't pick rocket launcher if enemy is too close
	if ( EnemyDist < 360 )
	{
		if ( Instigator.Weapon == self )
		{
			// don't switch away from rocket launcher unless really bad tactical situation
			if ( (EnemyDist > 250) || ((Instigator.Health < 50) && (Instigator.Health < B.Enemy.Health - 30)) )
				return Rating;
		}
		return 0.05 + EnemyDist * 0.001;
	}

	// rockets are good if higher than target, bad if lower than target
	ZDiff = Instigator.Location.Z - B.Enemy.Location.Z;
	if ( ZDiff > 120 )
		Rating += 0.25;
	else if ( ZDiff < -160 )
		Rating -= 0.35;
	else if ( ZDiff < -80 )
		Rating -= 0.05;
	if ( (B.Enemy.Weapon != None) && B.Enemy.Weapon.bMeleeWeapon && (EnemyDist < 2500) )
		Rating += 0.25;

	return Rating;
}

/* BestMode()
choose between regular or alt-fire
*/
function byte BestMode()
{
	local UTBot B;

	B = UTBot(Instigator.Controller);
	if ( (B == None) || (B.Enemy == None) )
		return 0;

	if ( (FRand() < 0.3) && !B.IsStrafing() && (Instigator.Physics != PHYS_Falling) )
		return 1;
	return 0;
}

/*********************************************************************************************
 * Utility Functions
 *********************************************************************************************/


/**
 * Returns interval in seconds between each shot, for the firing state of FireModeNum firing mode.
 * We override it here to support the queue!
 *
 * @param	FireModeNum	fire mode
 * @return	Period in seconds of firing mode
 */

simulated function float GetFireInterval( byte FireModeNum )
{
	if (FireModeNum != 1 || UTPawn(Owner) == None || LoadedShotCount == MaxLoadCount)
	{
		return super.GetFireInterval(FireModeNum);
	}
	else
	{
		return AltFireQueueTimes[LoadedShotCount] * UTPawn(Owner).FireRateMultiplier;
	}
}

/**
 * When we attach the weapon, attach any additional muzzle flash spots
 */

simulated function AttachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;
	local int I;

	Super.AttachMuzzleFlash();

	SKMesh = SkeletalMeshComponent(Mesh);

	if (MuzzleFlashPSCTemplate != none)
	{
		for (i=0;i<3;i++)
		{
			MuzzleFlashPSCList[i] = new(Outer) class'UTParticleSystemComponent';
			MuzzleFlashPSCList[i].SetTemplate(MuzzleFlashPSCTemplate);
			MuzzleFlashPSCList[i].DeactivateSystem();
			MuzzleFlashPSCList[i].SetDepthPriorityGroup(SDPG_Foreground);
			MuzzleFlashPSCList[i].SetColorParameter('MuzzleFlashColor',MuzzleFlashColor);
			SKMesh.AttachComponentToSocket(MuzzleFlashPSCList[i], MuzzleFlashSocketList[i]);
		}
	}
}

/**
 * Kill any muzzle flash PCSs
 */

simulated function DetachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;
	local int i;

	if ( MuzzleFlashPSCList.Length == 0 )
		return;

	SKMesh = SkeletalMeshComponent(Mesh);

	for (i=0;i<3;i++)
	{
		if (MuzzleFlashPSCList[i] != none)
			SKMesh.DetachComponent( MuzzleFlashPSCList[i] );
	}
}

/**
 * Turns the MuzzleFlashlight off
 */
simulated event MuzzleFlashTimer()
{
	local int i, length;
	if (CurrentFireMode != 1)
	{
		Super.MuzzleFlashTimer();
	}
	else
	{
		length = MuzzleFlashPSCList.length;
		for (i=0;i<length;i++)
		{
			MuzzleFlashPSCList[i].DeactivateSystem();
		}
	}
}

/**
 * Causes the muzzle flashlight to turn on and setup a time to
 * turn it back off again.
 */
simulated event CauseMuzzleFlash()
{
	local int i;

	if (CurrentFireMode != 1)
	{
		Super.CauseMuzzleFlash();
	}
	else if (MuzzleFlashLight != None)
	{
		MuzzleFlashLight.SetEnabled(true);
	}
	else
	{
		for (i = 0; i < LoadedShotCount; i++)
		{
			if (MuzzleFlashPSCList[i] != None)
			{
				MuzzleFlashPSCList[i].SetVectorParameter('MFlashScale',Vect(0.5,0.5,0.5));
				MuzzleFlashPSCList[i].ActivateSystem();
			}
		}
		// Set when to turn it off.
		SetTimer(MuzzleFlashDuration,false,'MuzzleFlashTimer');
	}
}

/**
 * Quickly turns off an active muzzle flash
 */

simulated event StopMuzzleFlash()
{
	ClearTimer('MuzzleFlashTimer');
	MuzzleFlashTimer();

	if ( MuzzleFlashPSC != none )
	{
		MuzzleFlashPSC.DeactivateSystem();
	}
}

/**
 * Fire off the current load and play any needed sounds/animations
 */

simulated function WeaponFireLoad()
{
	if (Role == Role_Authority)
	{
		FireLoad();
	}

	PlayFiringSound();
	InvManager.OwnerEvent('FiredWeapon');
	GotoState('WeaponPlayingFire');
}

simulated function PlayLoadedSound();

/*********************************************************************************************
 * States
 *********************************************************************************************/


/*********************************************************************************************
 * State WeaponLoadAmmo
 * In this state, ammo will continue to load up until MaxLoadCount has been reached.  It's
 * similar to the firing state
 *********************************************************************************************/

simulated state WeaponLoadAmmo
{
	simulated function TimeWeaponFiring( byte FireModeNum )
	{
		SetTimer( GetFireInterval(1) , false, 'RefireCheckTimer' );
	}

	simulated function WeaponEmpty()
	{
		if ( Instigator.IsLocallyControlled() )
		{
			StopWeaponAnimation();
			GotoState('WeaponWaitingForFire');
		}
		else
		{
			Global.WeaponEmpty();
		}
	}

	/**
	 * Adds a projectile to the queue and uses up some ammo.  In Addition, it plays
	 * a sound so that other pawns in the world can here it.
	 */
	simulated function AddProjectile()
	{
		local string AnimName;

		WeaponPlaySound(AltFireSndQue[LoadedShotCount]);

		// Play the que animation
		if ( Instigator.IsFirstPerson() )
		{
			AnimName = "WeaponAltFireQueue"$LoadedShotCount+1;
			PlayWeaponAnimation( name(AnimName), AltFireQueueTimes[LoadedShotCount]);

		}
		TimeWeaponFiring(1);
	}

	simulated function RefireCheckTimer()
	{
		// If we have ammo, load a shot

		if ( HasAmmo(CurrentFireMode) )
		{
			// Play Loading Sound
			MakeNoise(0.5);
			WeaponPlaySound(WeaponLoadedSnd);

			// Add the Rocket
			PlayLoadedSound();
			LoadedShotCount++;
			ConsumeAmmo(CurrentFireMode);
		}

		// Figure out what to do now

		// If we have released AltFire - Just fire the load
		if ( !StillFiring(1) )
		{
			if ( LoadedShotCount > 0 )
			{
				// We have to check to insure we are in the proper state here.  StillFiring() may cause
				// bots to end their firing sequence.  This will cause and EndFire() call to be made which will
				// also fire off the load and switch to the animation state prematurely.

				if ( IsInState('WeaponLoadAmmo') )
				{
					WeaponFireLoad();
				}
			}
			else
			{
				GotoState('Active');
			}
		}
		else if ( !HasAmmo(CurrentFireMode) || LoadedShotCount >= MaxLoadCount )
		{
			GotoState('WeaponWaitingForFire');
		}
		else
		{
			AddProjectile();
		}
	}

	/**
	 * We need to override EndFire so that we can correctly fire off the
	 * current load if we have any.
	 */


	simulated function EndFire(byte FireModeNum)
	{
		local float MinTimerTarget, TimerCount;
		// Pass along to the global to handle everything
		Global.EndFire(FireModeNum);

		if (FireModeNum == 1 && LoadedShotCount>0)
		{
			MinTimerTarget = GetTimerRate('RefireCheckTimer') * WaitToFirePct;
			TimerCount = GetTimerCount('RefireCheckTimer');
			if (TimerCount < MinTimerTarget)
			{
				WeaponFireLoad();
			}
		}
	}


	/**
	 * Insure that the LoadedShotCount is 0 when we leave this state
	 */
	simulated function EndState(Name NextStateName)
	{
		ClearTimer('RefireCheckTimer');
		super.EndState(NextStateName);
	}


	simulated function bool IsFiring()
	{
		return true;
	}

	/**
	 * When we are in the firing state, don't allow for a pickup to switch the weapon
	 */
	simulated function bool DenyClientWeaponSet()
	{
		return true;
	}

	simulated function bool TryPutdown()
	{
		bWeaponPutDown = true;
		return true;
	}

Begin:
	AddProjectile();
}

/*********************************************************************************************
 * State WeaponWaitingForFire
 * After the weapon has fully loaded, it will enter this state and wait for a short period of
 * time before auto-firing
 *********************************************************************************************/


simulated state WeaponWaitingForFire
{
	/**
	 * Insure that the LoadedShotCount is 0 when we leave this state
	 */
	simulated function EndState(Name NextStateName)
	{
		ClearTimer('ForceFireTimer');
		super.EndState(NextStateName);
	}

	/**
	 * Set the Grace Period
	 */
	simulated function BeginState(Name PrevStateName)
	{
		SetTimer(GracePeriod, false, 'ForceFireTimer');
		Super.BeginState(PrevStateName);
	}

	/**
	 * If we get an EndFire - Exit Immediately
	 */
	simulated function EndFire(byte FireModeNum)
	{
		if (FireModeNum == 1 && LoadedShotCount>0)
		{
			WeaponFireLoad();
		}
		Global.EndFire(FireModeNum);
	}

	/**
	 * Fire the load
	 */

	simulated function ForceFireTimer()
	{
		WeaponFireLoad();
	}

}

/*********************************************************************************************
 * State WeaponPlayingFire
 * In this state, the weapon will have already spawned the projectiles and is just playing out
 * the firing animations.  When finished, it returns to the Active state
 *
 * We use 2 animations, one for firing and one of spin down in order to allow better tweaking of
 * the timing.
 *********************************************************************************************/

simulated State WeaponPlayingFire
{
	/**
	 * Choose the proper "Firing" animation to play depending on the number of shots loaded.
	 */

	simulated function PlayFiringAnim()
	{
		local string AnimName;
		local int index;

		index = max(LoadedShotCount,1);

		AnimName = "WeaponAltFireLaunch"$Index;
		PlayWeaponAnimation( name(AnimName), AltFireLaunchTimes[Index-1]);
		SetTimer(AltFireLaunchTimes[Index-1],false,'FireAnimDone');
	}

	/**
	 * Choose the proper "We are done firing, reset" animation to play depending on the number of shots loaded
	 */

	simulated function PlayFiringEndAnim()
	{
		local string AnimName;
		local int index;

		index = max(LoadedShotCount,1);

		AnimName = "WeaponAltFireLaunch"$Index$"End";
		PlayWeaponAnimation( name(AnimName), AltFireEndTimes[Index-1]);
		SetTimer(AltFireEndTimes[Index-1],false,'FireAnimEnded');
	}

	/**
	 * When the Fire animation is done chain the ending animation.
	 */

	simulated function FireAnimDone()
	{
		PlayFiringEndAnim();

	}
	/**
	 * When the End-Fire animation is done return to the active state
	 */

	simulated function FireAnimEnded()
	{
		// if switching to another weapon, abort firing and put down right away
		if( bWeaponPutDown )
		{
			PutDownWeapon();
			return;
		}

		// if out of ammo, then call weapon empty notification
		if( !HasAnyAmmo()  )
		{
			WeaponEmpty();
		}
		Gotostate('Active');
	}

	/**
	 * Clean up the weapon.  Reset the shot count, etc
	 */

	simulated function EndState(Name NextStateName)
	{
		LoadedShotCount = 0;
		ClearFlashCount();
		ClearFlashLocation();

		// Clear out the other 2 timers

		ClearTimer('FireAnimDone');
		ClearTimer('FireAnimEnded');
	}

Begin:
	WeaponPlaySound(WeaponLoadSnd);
	PlayFiringAnim();
}



defaultproperties
{
	MaxLoadCount=3
	SpreadDist=1000
	FiringStatesArray(1)=WeaponLoadAmmo
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponFireTypes(1)=EWFT_Projectile
	WaitToFirePct=0.85
	GracePeriod=0.5
}
