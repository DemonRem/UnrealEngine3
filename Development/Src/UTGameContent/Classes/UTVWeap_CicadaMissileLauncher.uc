/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_CicadaMissileLauncher extends UTVehicleWeapon
	HideDropDown;

/** current number of rockets loaded */
var int RocketsLoaded;

/** maximum number of rockets that can be loaded */
var(CicadaWeapon) int MaxRockets;

/** sound played when loading a rocket */
var SoundCue WeaponLoadSnd;

/** if bLocked, all missiles will home to this location */
var vector LockedTargetVect;

/** position vehicle was at when lock was acquired */
var vector LockPosition;

/** How fast should the rockets accelerate towards it's cross or away */
var(CicadaWeapon) float AccelRate;

/* How long between each shot when launching a load of rockets */
var(CicadaWeapon) float LoadedFireTime;

replication
{
	if (bNetOwner && Role == ROLE_Authority)
		LockedTargetVect;
}

simulated function DrawAmmoCount( Hud H )
{
	local float X, Y, XL, YL;
	local vector pos;
	local UTHUD UTH;

	Super.DrawAmmoCount(H);

	UTH = UTHUD(H);
	if (UTH != None)
	{
		X = UTH.Canvas.ClipX - (5 * (UTH.Canvas.ClipX / 1024));
		Y = UTH.Canvas.ClipY - (5 * (UTH.Canvas.ClipX / 1024));

		UTH.DrawNumericWidget(X, Y, IconX, IconY, IconWidth, IconHeight, RocketsLoaded, 99, true);
	}

	if (!IsZero(LockedTargetVect))
	{
		Pos = H.Canvas.Project(LockedTargetVect);
		H.Canvas.StrLen("X", XL, YL);
		H.Canvas.SetPos(Pos.X - XL / 2.0, Pos.Y - YL / 2.0);
		H.Canvas.DrawText("X");
	}
}

simulated function bool IsAimCorrect()
{
	// this weapon uses homing rockets so it can always hit where it's being aimed
	return true;
}

simulated function int GetAmmoCount()
{
	return RocketsLoaded;
}

function vector FindInitialTarget(vector Start, rotator Dir)
{
	local vector HitLocation,HitNormal, End, out_CamStart;

	MyVehicle.VehicleCalcCamera(0, 0, Start, Dir, out_CamStart);
	End = Start + WeaponRange * vector(Dir);
	return (Trace(HitLocation, HitNormal, End, Start, true) != None) ? HitLocation : End;
}

simulated function CustomFire()
{
	// fire two projectiles - ProjectileFire() will handle switching the side the missile is spawned on
	ProjectileFire();
	ProjectileFire();
}

simulated function Projectile ProjectileFire()
{
	local UTProj_CicadaRocket P;
	local Vector Aim, AccelAdjustment, StartLocation, CrossVec;
	local rotator UpRot;


	P = UTProj_CicadaRocket( Super.ProjectileFire() );
	if (P!=none)
	{
		// Set their initial velocity

		Aim = Vector( MyVehicle.GetWeaponAim(self) );

		CrossVec = Vect(0,0,1) * ( MyVehicle.GetBarrelIndex(SeatIndex) > 0 ? 1 : -1);
		CrossVec *= (CurrentFireMode > 0 ? 1 : -1);

		AccelAdjustment = (Aim Cross CrossVec) * AccelRate;

		if (CurrentFireMode == 1)
		{
		   	AccelAdjustment.Z += ( (400.0 * FRand()) - 100.0 ) * ( FRand() * 2.f);
		}

		UpRot = MyVehicle.GetWeaponAim(self);
		if (CurrentFireMode == 1 && (UTBot(Instigator.Controller) != None) && !FastTrace(LockedTargetVect, P.Location))
		{
			UpRot.Pitch = FastTrace(P.Location + 3000.0 * vector(UpRot), P.Location) ? 12000 : 16000;
		}

		StartLocation = MyVehicle.GetPhysicalFireStartLoc(self);
		P.Target = FindInitialTarget(StartLocation, UpRot);

		if (CurrentFireMode == 1)
		{
			P.KillRange = 1024;
			P.bFinalTarget = false;
			P.SecondTarget = LockedTargetVect;
			P.SwitchTargetTime = 0.5;
		}
		else
		{
			P.DesiredDistanceToAxis = 64;
			P.bFinalTarget = true;
		}

		P.ArmMissile(AccelAdjustment, Vector(P.Rotation) * ( 700 + VSize(MyVehicle.Velocity)) );

	}
	return p;
}

function byte BestMode()
{
	if (Instigator.Controller == None || Instigator.Controller.Focus == None)
	{
		return 1;
	}
	if ( Pawn(Instigator.Controller.Focus) != None && !Pawn(Instigator.Controller.Focus).bStationary )
	{
		return 0;
	}

	return 1;
}

function FireLoadedRocket();


simulated state WeaponLoadAmmo
{
	// if we're locked on to a target the rockets should be able to hit it, so if Other is near the lock location, we can always attack it
	function bool CanAttack(Actor Other)
	{
		local float Radius, Height;

		Other.GetBoundingCylinder(Radius, Height);
		if (VSize(LockedTargetVect - Other.Location) < 100.f + Radius + Height)
		{
			return true;
		}
		return Global.CanAttack(Other);
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

	/**
	 * Adds a rocket to the count and uses up some ammo.  In addition, it plays
	 * a sound so that other pawns in the world can hear it.
	 */
	simulated function LoadRocket()
	{
		if (RocketsLoaded < MaxRockets && HasAmmo(CurrentFireMode))
		{
			// Add the rocket
			RocketsLoaded++;
			ConsumeAmmo(CurrentFireMode);
			if (RocketsLoaded > 1)
			{
				MakeNoise(0.5);
				WeaponPlaySound(WeaponLoadSnd);
			}
		}
	}

	/**
	 * Fire off a shot w/ effects
	 */
	simulated function WeaponFireLoad()
	{
		if (RocketsLoaded > 0)
		{
			GotoState('WeaponFiringLoad');
		}
		else
		{
			GotoState('Active');
		}
	}

	/**
	 * This is the timer event for each shot
	 */
	simulated event RefireCheckTimer()
	{
		if (!StillFiring(1))
		{
			WeaponFireLoad();
		}
		else
		{
			LoadRocket();
		}
	}

	simulated function SendToFiringState(byte FireModeNum)
	{
	}

	function SpawnTargetBeam()
	{
		/* FIXME:
		local PainterBeamEffect BP;

		BP = Spawn(class'CicadaLockBeamEffect',self);
		BP.StartEffect = WeaponFireLocation;
		BP.EndEffect = LockedTarget;
		BP.EffectOffset = vect(0,0,-20);
		BP.SetTargetState(PTS_Aquired);
		*/
	}

	simulated function BeginState(Name PreviousStateName)
	{
		if (Instigator != None && Instigator.IsHumanControlled() && Instigator.IsLocallyControlled())
		{
			PlayerController(Instigator.Controller).ClientPlaySound(LockAcquiredSound);
		}

		RocketsLoaded = 0;
		if ( (UTBot(Instigator.Controller) != None) && (Instigator == MyVehicle) && !UTBot(Instigator.Controller).bScriptedFrozen )
		{
			if (MyVehicle.Rise <= 0.f && FastTrace(Instigator.Location - vect(0,0,500), Instigator.Location))
			{
				MyVehicle.Rise = -0.5;
			}
			else
			{
				MyVehicle.Rise = 1.0;
			}
		}
		LockedTargetVect = FindInitialTarget(MyVehicle.GetPhysicalFireStartLoc(self), MyVehicle.GetWeaponAim(self));
		SpawnTargetBeam();

		Super.BeginState(PreviousStateName);
	}

	simulated function EndState(Name NextStateName)
	{
		ClearTimer('RefireCheckTimer');
		ClearTimer('FireLoadedRocket');
		Super.EndState(NextStateName);
	}

	simulated function bool IsFiring()
	{
		return true;
	}


Begin:
	LoadRocket();
	TimeWeaponFiring(CurrentFireMode);
}

simulated state WeaponFiringLoad
{
	simulated function SendToFiringState(byte FireModeNum);

	simulated function FireLoadedRocket()
	{
		PlayFiringSound();
		ProjectileFire();
		RocketsLoaded--;
		if (RocketsLoaded <= 0)
		{
			if (RocketsLoaded < 0)
			{
				`Warn("Extra rockets fired! RocketsLoaded:" @ RocketsLoaded);
				ScriptTrace();
			}

			GotoState('Active');
		}
	}

	simulated function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);

		ClearTimer('FireLoadedRocket');
		ClearFlashCount();
		ClearFlashLocation();
		ClearPendingFire(1);

		RocketsLoaded = 0;
		LockedTargetVect = vect(0,0,0);

	}

	simulated function TimeLoadedFiring()
	{
		SetTimer(LoadedFireTime,true,'FireLoadedRocket');
	}

Begin:
	FireLoadedRocket();
	TimeLoadedFiring();
}

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Custom
	WeaponProjectiles(0)=class'UTProj_CicadaRocket'
	WeaponProjectiles(1)=class'UTProj_CicadaRocket'

	WeaponFireSnd[0]=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_Fire'
	WeaponFireSnd[1]=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_MissileEject'

	LockAcquiredSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_TargetLock'
	WeaponLoadSnd=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_MissileLoad'

	FireInterval(0)=0.5
	FireInterval(1)=0.5
	FiringStatesArray(1)=WeaponLoadAmmo

	ShotCost(0)=0
	ShotCost(1)=0

		BotRefireRate=0.99
		bInstantHit=false
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0

	FireTriggerTags=(CicadaWeapon01,CicadaWeapon02)
	AltFireTriggerTags=(CicadaWeapon01,CicadaWeapon02)

	MaxRockets=16
	AccelRate=1500
	LoadedFireTime=0.1
}
