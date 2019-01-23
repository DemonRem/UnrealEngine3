/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Avril extends UTWeapon
	abstract;

/** A pointer to the currently controlled rocket */
var repnotify UTProj_AvrilRocketBase MyRocket;

/** If you shortcircut the reload, the time will get deferred until you bring the weapon back up */
var float DeferredReloadTime;

/** How long the reload sequence takes*/
var float ReloadTime;
/** How long the fire sequence takes*/
var float FireTime;
/** Animation to play when there is no ammo*/
var name NoAmmoWeaponPutDownAnim;
/** Anim to play while reloading */
var name WeaponReloadAnim;
var AudioComponent ReloadSound;
var SoundCue ReloadCue;
/** set when holding altfire for targeting laser (can't use normal state system because it can fire simultaneously with primary) */
var bool bTargetingLaserActive;
/** component for the laser effect */
var ParticleSystemComponent LaserEffect;
/** list of known mines/rockets to affect */
var array<UTProjectile> TargetedProjectiles;
/** last time TargetedProjectiles list was updated */
var float LastTargetedProjectilesCheckTime;
/** the maximum number of projectiles we're allowed to control simultaneously */
var int MaxControlledProjectiles;
/** The speed of a deferred reload */
var float ReloadAnimSpeed;
replication
{
	if (bNetDirty)
		MyRocket, DeferredReloadTime;
}

simulated function AttachWeaponTo(SkeletalMeshComponent MeshCpnt, optional Name SocketName)
{
	local SkeletalMeshComponent SkelMesh;

	SkelMesh = SkeletalMeshComponent(Mesh);
	if (LaserEffect != None && SkelMesh != None)
	{
		SkelMesh.AttachComponentToSocket(LaserEffect, 'BeamStart');
	}

	Super.AttachWeaponTo(MeshCpnt, SocketName);
}

simulated function ReplicatedEvent(name VarName)
{
	if (VarName == 'MyRocket' && MyRocket == None && !HasAnyAmmo() )
	{
		WeaponEmpty();
	}
	Super.ReplicatedEvent(VarName);
}

simulated function Projectile ProjectileFire()
{
	if (Role == ROLE_Authority && MyRocket != None && MyRocket.LockingWeapon == self)
	{
		MyRocket.SetTarget(None, self);
		MyRocket = None;
   	}

	MyRocket = UTProj_AvrilRocketBase(Super.ProjectileFire());
	if (MyRocket != None )
	{
		MyRocket.MyWeapon = self;
		MyRocket.SetTarget(LockedTarget, self);
	}

	return MyRocket;
}

/**
 * We override TryPutDown so that we can store the deferred amount of time.
 */
simulated function bool TryPutDown()
{
	local float MinTimerTarget;
	local float TimerRate;
	local float TimerCount;

	bWeaponPutDown = true;

	TimerRate = GetTimerRate('RefireCheckTimer');
	if ( TimerRate > 0 )
	{
		MinTimerTarget = TimerRate * MinReloadPct[CurrentFireMode];
		TimerCount = GetTimerCount('RefireCheckTimer');

		if (TimerCount > MinTimerTarget)
		{
			DeferredReloadTime = TimerRate - TimerCount;
			PutDownWeapon();
			return true;
		}
		else
		{
			// Shorten the wait time
			SetTimer( MinTimerTarget - TimerCount , false, 'RefireCheckTimer');
			DeferredReloadTime = TimerRate - MinTimerTarget - (MinTimerTarget - TimerCount);
			return true;
		}
	}
	else
	{
		DeferredReloadTime = 0;
		return true;
	}

	return false;
}

simulated function float GetEquipTime()
{
	local float NewEquipTime;
	NewEquipTime = Super.GetEquipTime();

	if (DeferredReloadTime > NewEquipTime)
	{
		NewEquipTime = DeferredReloadTime;
	}

	DeferredReloadTime = 0;

	return NewEquipTime;
}

simulated function PlayWeaponEquip()
{
	ReloadAnimSpeed =GetTimerRate('WeaponEquipped') - GetTimerCount('WeaponEquipped') - super.GetEquipTime();
	super.PlayWeaponEquip();
	if(ReloadAnimSpeed > 0)
	{
		setTimer(EquipTime,false,'FastReload');
	}
}
simulated function FastReload()
{
	PlayReloadAnim(ReloadAnimSpeed);
	ReloadAnimSpeed=0.0f;
}
function AdjustLockTarget(actor NewLockTarget)
{
	local EZoomState ZoomState;

	Super.AdjustLockTarget(NewLockTarget);

	// check if we should start or stop the zooming due to the target change
	if (Instigator != None)
	{
		ZoomState = GetZoomedState();
		if (NewLockTarget != None)
		{
			if (ZoomState == ZST_NotZoomed && PendingFire(1))
			{
				CheckZoom(1);
			}
		}
		else if (ZoomState != ZST_NotZoomed && UTPlayerController(Instigator.Controller) != None)
		{
			EndZoom(UTPlayerController(Instigator.Controller));
		}
	}

	if (MyRocket != none)
	{
		MyRocket.SetTarget(NewLockTarget, self);
	}
}

function bool CanLockOnTo(Actor TA)
{
	return (TA.IsA('UTVehicle_Hoverboard') ? false : Super.CanLockOnTo(TA));
}

simulated function bool PassThroughDamage(Actor HitActor)
{
	return ((HitActor == MyRocket && MyRocket != None) || Super.PassThroughDamage(HitActor));
}

simulated function WeaponCalcCamera(float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot)
{
	local EZoomState ZoomState;

	if (LockedTarget != None)
	{
		ZoomState = GetZoomedState();
		if (ZoomState == ZST_ZoomingIn || ZoomState == ZST_Zoomed)
		{
			out_CamRot = rotator(LockedTarget.GetTargetLocation() - Instigator.GetPawnViewLocation());
			Instigator.Controller.SetRotation(out_CamRot);
		}
	}
}

simulated event SetPosition(UTPawn Holder)
{
	local EZoomState ZoomState;

	// if locked, make sure rotation is up to date for weapon position calculation
	if (Holder.Controller != None && LockedTarget != None)
	{
		ZoomState = GetZoomedState();
		if (ZoomState == ZST_ZoomingIn || ZoomState == ZST_Zoomed)
		{
			Holder.Controller.SetRotation(rotator(LockedTarget.GetTargetLocation() - Instigator.GetPawnViewLocation()));
		}
	}

	Super.SetPosition(Holder);
}

function RocketDestroyed(UTProj_AvrilRocketBase Rocket)
{
	if (Rocket == MyRocket)
	{
		MyRocket = none;
	}

	if (!HasAnyAmmo() )
	{
		WeaponEmpty();
	}
}

/**
 * Draw the Avril hud
 */
simulated function ActiveRenderOverlays( HUD H )
{
	local vector TargetXY;

	super.ActiveRenderOverlays(H);

	if ( LockedTarget!=none && GetZoomedState() != ZST_NotZoomed )
	{
		TargetXY = H.Canvas.Project(LockedTarget.GetTargetLocation());
		H.Canvas.SetPos(TargetXY.X,TargetXY.Y);
		H.Canvas.DrawTile(UTHud(H).HudTexture,22,58,343,212,22,58);
	}
}

// AI Interface
function float SuggestAttackStyle()
{
	return -0.4;
}

function float SuggestDefenseStyle()
{
	return 0.5;
}

function byte BestMode()
{
	return 0;
}

function float GetAIRating()
{
	local UTBot B;
	local float ZDiff, Dist, Result;

	B = UTBot(Instigator.Controller);
	if (B == None || B.Enemy == None)
	{
		return AIRating;
	}
	if (B.Focus != None && B.Focus.IsA('UTProj_SPMACamera'))
	{
		return 2;
	}

	if (Vehicle(B.Enemy) == None)
	{
		return 0.0;
	}

	Result = AIRating;
	ZDiff = Instigator.Location.Z - B.Enemy.Location.Z;
	if (ZDiff < -200.0)
	{
		Result += 0.1;
	}
	Dist = VSize(B.Enemy.Location - Instigator.Location);
	if (Dist > 2000.0)
	{
		return FMin(2.0, Result + (Dist - 2000.0) * 0.0002);
	}

	return Result;
}

function bool RecommendRangedAttack()
{
	local UTBot B;

	B = UTBot(Instigator.Controller);
	if (B == None || B.Enemy == None)
	{
		return true;
	}

	return (VSize(B.Enemy.Location - Instigator.Location) > 2000.0 * (1.0 + FRand()));
}
// end AI Interface

simulated function WeaponEmpty()
{
	// If we were firing, stop
	if (IsFiring())
	{
		GotoState('Active');
	}

	if ( Instigator != none && Instigator.IsLocallyControlled() && MyRocket == None)
	{
		Instigator.InvManager.SwitchToBestWeapon( true );
	}
}

/** updates the location for the targeting laser and notifies any spider mines or AVRiL rockets in the vacinity */
simulated function UpdateTargetingLaser()
{
	local vector TargetLocation, StartTrace, EndTrace;
	local ImpactInfo Impact;
	local UTProj_AvrilRocketBase Rocket;
	local UTProj_SpiderMineBase Mine;
	local int i, NumControlled;
	local array<UTSpiderMineTrap> Traps;

	// if we have a locked target, just go to that
	if (LockedTarget != None)
	{
		TargetLocation = LockedTarget.GetTargetLocation(Instigator);
	}
	else
	{
		// we have to trace to find out what we're hitting
		StartTrace = Instigator.GetWeaponStartTraceLocation();
		EndTrace = StartTrace + vector(GetAdjustedAim(StartTrace)) * LockRange;
		Impact = CalcWeaponFire(StartTrace, EndTrace);
		TargetLocation = Impact.HitLocation;
	}

	// update beam effect
	LaserEffect.SetVectorParameter('BeamEnd', TargetLocation);

	// notify mines and rockets
	if (Role == ROLE_Authority)
	{
		SetFlashLocation(TargetLocation);

		if (WorldInfo.TimeSeconds - LastTargetedProjectilesCheckTime >= 0.5)
		{
			FindTargetedProjectiles(TargetLocation, Traps);
		}

		for (i = 0; i < TargetedProjectiles.length && NumControlled < MaxControlledProjectiles; i++)
		{
			Rocket = UTProj_AvrilRocketBase(TargetedProjectiles[i]);
			if (Rocket != None)
			{
				// rockets only get notification of target locks
				if (Rocket.MyWeapon != self && LockedTarget != None && Rocket.SetTarget(LockedTarget, self))
				{
					NumControlled++;
				}
			}
			else
			{
				Mine = UTProj_SpiderMineBase(TargetedProjectiles[i]);
				if (Mine != None && Mine.SetScurryTarget(TargetLocation, Instigator))
				{
					NumControlled++;
				}
			}
		}
		// if we still have room to control more projectiles, ask traps to spawn mines
		for (i = 0; i < Traps.length && NumControlled < MaxControlledProjectiles; i++)
		{
			Mine = Traps[i].SpawnMine(None, Normal(TargetLocation - Instigator.Location));
			if (Mine != None && Mine.SetScurryTarget(TargetLocation, Instigator))
			{
				NumControlled++;
			}
		}
	}
}

/** called on a timer to fill the list of projectiles affected by the targeting laser */
function FindTargetedProjectiles(vector TargetLocation, out array<UTSpiderMineTrap> Traps)
{
	local Actor A;
	local UTSpiderMineTrap Trap;

	TargetedProjectiles.length = 0;
	foreach DynamicActors(class'Actor', A)
	{
		if (A.IsA('UTProj_SpiderMineBase') || A.IsA('UTProj_AvrilRocketBase'))
		{
			TargetedProjectiles[TargetedProjectiles.length] = UTProjectile(A);
		}
		else
		{
			Trap = UTSpiderMineTrap(A);
			if ( Trap != None && (Instigator.Controller == Trap.InstigatorController || WorldInfo.GRI.OnSameTeam(Trap, Instigator)) &&
				FastTrace(TargetLocation, A.Location + vect(0,0,32)) )
			{
				// spider mine trap spawns mines to follow target if they can see it
				Traps[Traps.length] = Trap;
			}
		}
	}

	LastTargetedProjectilesCheckTime = WorldInfo.TimeSeconds;
}

simulated function Tick(float DeltaTime)
{
	Super.Tick(DeltaTime);

	if (bTargetingLaserActive)
	{
		UpdateTargetingLaser();
	}
}

simulated function BeginFire(byte FireModeNum)
{
	if (FireModeNum == 1 && !bTargetingLaserActive)
	{
		// targeting laser altfire
		bTargetingLaserActive = true;
		UpdateTargetingLaser();
		LaserEffect.ActivateSystem();
	}
	Super.BeginFire(FireModeNum);
}

simulated function EndFire(byte FireModeNum)
{
	local UTPlayerController PC;
	local int i;
	local UTProj_AvrilRocketBase Rocket;

	if (FireModeNum == 1 && bTargetingLaserActive)
	{
		// targeting laser altfire
		bTargetingLaserActive = false;
		LaserEffect.DeactivateSystem();
		ClearFlashLocation();
		// lose target lock on any rockets we were controlling
		for (i = 0; i < TargetedProjectiles.length; i++)
		{
			Rocket = UTProj_AvrilRocketBase(TargetedProjectiles[i]);
			if (Rocket != None && Rocket.LockingWeapon == self && Rocket.MyWeapon != self)
			{
				Rocket.SetTarget(None, self);
			}
		}
		TargetedProjectiles.length = 0;
	}

	PC = UTPlayerController(Instigator.Controller);
	if (PC != None && LocalPlayer(PC.Player) != none && FireModeNum < bZoomedFireMode.Length && bZoomedFireMode[FireModeNum] != 0)
	{
		if ( GetZoomedState() != ZST_NotZoomed )
		{
			PC.EndZoom();
		}
	}
	Super(Weapon).EndFire(FireModeNum);
}

simulated function bool CheckZoom(byte FireModeNum)
{
	return ((bLockedOnTarget && GetZoomedState() == ZST_NotZoomed) ? Super.CheckZoom(FireModeNum) : false);
}

simulated function PlayFireEffects( byte FireModeNum, optional vector HitLocation )
{
	local float TotalFireTime;

	if (IsZero(HitLocation))
	{
		// rocket fired

		// Play Weapon fire animation
		if ( FireModeNum < WeaponFireAnim.Length && WeaponFireAnim[FireModeNum] != '' )
		{
			TotalFireTime = FireTime*((UTPawn(Owner)!= None) ? UTPawn(Owner).FireRateMultiplier : 1.0);
			PlayWeaponAnimation( WeaponFireAnim[FireModeNum], TotalFireTime);
			PlayArmAnimation(WeaponFireAnim[FireModeNum], TotalFireTime, false);
			SetTimer(TotalFireTime,false,'PlayReloadAnim');
		}

		// Start muzzle flash effect
		CauseMuzzleFlash();

		ShakeView();
	}
}

simulated function PlayReloadAnim(optional float ReloadOverrideTime)
{
	local float ReloadFinal;
	if(AmmoCount > 0)
	{
		ReloadFinal = ReloadOverrideTime != 0?ReloadOverrideTime:ReloadTime;
		PlayWeaponAnimation(WeaponReloadAnim,ReloadFinal);
		PlayArmAnimation(WeaponReloadAnim,ReloadFinal);
		if(ReloadSound == none)
		{
			ReloadSound = CreateAudioComponent(ReloadCue, false, true);
		}
		if(ReloadSound != none)
		{
			ReloadSound.PitchMultiplier = ReloadTime/ReloadFinal;
			ReloadSound.Play();
		}
	}
}

simulated function PlayWeaponPutDown()
{
	if(ReloadSound != none)
	{
		ReloadSound.FadeOut(0.1,0.0);
		ReloadSound = none;
	}
	if(AmmoCount > 0)
		WeaponPutDownAnim = default.WeaponPutDownAnim;
	else
		WeaponPutDownAnim = NoAmmoWeaponPutDownAnim;
	super.PlayWeaponPutDown();
}

simulated function PutDownWeapon()
{
	EndFire(1); // Stop the laser beam
	super.PutDownWeapon();
}

state Active
{
	simulated function bool ShouldLagRot()
	{
		return !bTargetingLaserActive;
	}
}

simulated state WeaponFiring
{
	simulated event EndState( Name NextStateName )
	{
		// do not call ClearFlashLocation() here as that is for the beam which may still be firing

		// Set weapon as not firing
		ClearFlashCount();
		ClearTimer('RefireCheckTimer');

		if (Instigator != none && AIController(Instigator.Controller) != None)
		{
			AIController(Instigator.Controller).NotifyWeaponFinishedFiring(self,CurrentFireMode);
		}
	}
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
