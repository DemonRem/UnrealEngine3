/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_ShieldedTurret extends UTVehicle_TrackTurretBase
	abstract;

/** actor used for the shield */
var UTStationaryTurretShield TurretShield;
/** bone to attach it to */
var name ShieldBone;
/** indicates whether or not the shield is currently active */
var repnotify bool bShieldActive;
/** used for replicating shield hits to make it flash on clients */
var repnotify byte ShieldHitCount;

replication
{
	if (bNetDirty)
		bShieldActive, ShieldHitCount;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	TurretShield = Spawn(class'UTStationaryTurretShield');
	TurretShield.SetBase(self,, Mesh, ShieldBone);
	Mesh.AttachComponent(TurretShield.ShieldEffectComponent, ShieldBone);
}

function IncomingMissile(Projectile P)
{
	local AIController C;

	C = AIController(Controller);
	if (C != None && C.Skill >= 2.0)
	{
		UTVWeap_LeviathanTurretBase(Weapon).ShieldAgainstIncoming(P);
	}
}

function bool Dodge(eDoubleClickDir DoubleClickMove)
{
	UTVWeap_LeviathanTurretBase(Weapon).ShieldAgainstIncoming();
	return false;
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local vector ShieldHitLocation, ShieldHitNormal;

	if ( Role < ROLE_Authority )
		return;

	// don't take damage if the shield is active and it hit the shield component or skipped it for some reason but should have hit it
	if ( !bShieldActive || TurretShield == None ||
		( HitInfo.HitComponent != TurretShield.CollisionComponent && ( IsZero(Momentum) || HitLocation == Location || DamageType == None
								|| !ClassIsChildOf(DamageType, class'UTDamageType')
								|| !TraceComponent(ShieldHitLocation, ShieldHitNormal, TurretShield.CollisionComponent, HitLocation, HitLocation - 2000.f * Normal(Momentum)) ) ) )
	{
		Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	}
	else if ( !WorldInfo.GRI.OnSameTeam(self, EventInstigator) )
	{
		UTVWeap_LeviathanTurretBase(Weapon).NotifyShieldHit(Damage);
		ShieldHit();
	}
}

simulated function SetShieldActive(int SeatIndex, bool bNowActive)
{
	if (SeatIndex == 0)
	{
		bShieldActive = bNowActive;
		if (TurretShield != None)
		{
			TurretShield.SetActive(bNowActive);
		}
	}
}

simulated function ShieldHit()
{
	// FIXME: play effects
	ShieldHitCount++;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bShieldActive')
	{
		SetShieldActive(0, bShieldActive);
	}
	else if (VarName == 'ShieldHitCount')
	{
		ShieldHit();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function BlowupVehicle()
{
	if (TurretShield != None)
	{
		TurretShield.Destroy();
	}

	Super.BlowupVehicle();
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (TurretShield != None)
	{
		TurretShield.Destroy();
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionRadius=80.000000
		CollisionHeight=40.0
	End Object

	ExitRadius=100.0
	TargetLocationAdjustment=(x=0.0,y=0.0,z=55.0)
	Seats(0)={(	GunClass=class'UTVWeap_TurretPrimary',
				GunSocket=(LU_Barrel,RU_Barrel,LL_Barrel,RL_Barrel),
				TurretVarPrefix="",
				TurretControls=(MegaTurret,TurretBase),
				CameraTag=CameraViewSocket,
				bSeatVisible=true,
				GunPivotPoints=(Seat),
				CameraEyeHeight=5,
				SeatBone=Seat,
				SeatOffset=(X=30,Z=-5),
				CameraOffset=-120)}
}
