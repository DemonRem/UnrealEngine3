/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_CicadaRocket extends UTProjectile;

var float SpiralForceMag;
var float InwardForceMag;
var float ForwardForceMag;
var float DesiredDistanceToAxis;
var float DesiredDistanceDecayRate;
var float InwardForceMagGrowthRate;

var float CurSpiralForceMag;
var float CurInwardForceMag;
var float CurForwardForceMag;

var float DT;

var vector AxisOrigin;
var vector AxisDir;

var vector Target, SecondTarget, InitialDir;
var float KillRange;
var bool bFinalTarget;
var float SwitchTargetTime;

var SoundCue IgniteSound;
var ParticleSystem ProjIgnitedFlightTemplate;

var float IgniteTime;
var repnotify vector InitialAcceleration;


replication
{
	if ( bNetInitial && Role == ROLE_Authority )
		IgniteTime, InitialAcceleration, Target, SecondTarget, SwitchTargetTime, bFinalTarget;
}


simulated function ReplicatedEvent(name VarName)
{
	if ( VarName == 'InitialAcceleration' )
	{
		SetTimer(IgniteTime , false, 'Ignite');
		Acceleration = InitialAcceleration;
	}
}

function Init(vector Direction);

function ArmMissile(vector InitAccel, vector InitVelocity)
{
	Velocity = InitVelocity;
	InitialAcceleration = InitAccel;

	IgniteTime = (FRand() * 0.25) + 0.35;

	// Seed the acceleration/timer on a server

	ReplicatedEvent('InitialAcceleration');
}


simulated function ChangeTarget()
{
	Target = SecondTarget;
	bFinalTarget = true;
	SwitchTargetTime = 0;
}

simulated function Ignite()
{
	local float Dist, TravelTime;

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		PlaySound(IgniteSound, true);
		ProjEffects.SetTemplate(ProjIgnitedFlightTemplate);
	}

	SetCollision(true, true);

	if (!bFinalTarget)
	{
		// Look for a pending collision

		Dist = VSize(Target - Location);
		if ( Dist < KillRange )
		{
			ChangeTarget();
		}
		else	// No pending collision, look for a collision before the switch
		{
			TravelTime = (Dist / VSize(Velocity));
			if ( TravelTime < SwitchTargetTime  )
			{

				// 3/4 life the travel time and then switch

				SwitchTargetTime = TravelTime * 0.75;
			}
		}
	}

	if ( Vsize(Location - Target) <= KillRange )
	{
		GotoState('Homing');
	}
	else
	{
		GotoState('Spiraling');
	}

}


state Spiraling
{
	simulated function ChangeTarget()
	{
		Global.ChangeTarget();
		BeginState('none');
	}

	simulated function BeginState(name PreviousStateName)
	{
		CurSpiralForceMag = SpiralForceMag;
		CurInwardForceMag = InwardForceMag;
		CurForwardForceMag = ForwardForceMag;

		AxisOrigin = Location;
		AxisDir =  Normal(Target - AxisOrigin);

		if ( Vehicle(Instigator) != None )
		{
			Velocity = FMax(Instigator.Velocity dot AxisDir, 0.0) * AxisDir;
		}
		else
		{
			Velocity = AxisDir * Speed;
		}

		if (PhysicsVolume != None && PhysicsVolume.bWaterVolume)
		{
			Velocity = 0.6 * Velocity;
		}

		SetTimer(DT, true);
	}

	simulated function Timer()
	{
		local vector ParallelComponent, PerpendicularComponent, NormalizedPerpendicularComponent;
		local vector SpiralForce, InwardForce, ForwardForce;
		local float InwardForceScale;

		// Add code to switch directions

		// Update the inward force magnitude.
		CurInwardForceMag += InwardForceMagGrowthRate * DT;

		ParallelComponent = ((Location - AxisOrigin) dot AxisDir) * AxisDir;
		PerpendicularComponent = (Location - AxisOrigin) - ParallelComponent;
		NormalizedPerpendicularComponent = Normal(PerpendicularComponent);

		InwardForceScale = VSize(PerpendicularComponent) - DesiredDistanceToAxis;

		SpiralForce = CurSpiralForceMag * Normal(AxisDir cross NormalizedPerpendicularComponent);
		InwardForce = -CurInwardForceMag * InwardForceScale * NormalizedPerpendicularComponent;
		ForwardForce = CurForwardForceMag * AxisDir;

		Acceleration = SpiralForce + InwardForce + ForwardForce;

		DesiredDistanceToAxis -= DesiredDistanceDecayRate * DT;
		DesiredDistanceToAxis = FMax(DesiredDistanceToAxis, 0.0);

		// Update rocket so it faces in the direction its going.
		SetRotation(rotator(Velocity));

		// Check to see if we should switch to Home in Mode
		if (!bFinalTarget)
		{
			SwitchTargetTime -= DT;
			if (SwitchTargetTime <= 0)
			{
				ChangeTarget();
			}
		}

		if (VSize(Location - Target) <= KillRange)
		{
			GotoState('Homing');
		}
	}
}

state Homing
{
	simulated function Timer()
	{
		local vector ForceDir;
		local float VelMag;

		if (InitialDir == vect(0,0,0))
		{
			InitialDir = Normal(Velocity);
		}

		Acceleration = vect(0,0,0);
		Super.Timer();

		// do normal guidance to target.
		ForceDir = Normal(Target - Location);

		if ((ForceDir Dot InitialDir) > 0.f)
		{
			VelMag = VSize(Velocity);

			ForceDir = Normal(ForceDir * 0.9 * VelMag + Velocity);
			Velocity =  VelMag * ForceDir;
			Acceleration += 5 * ForceDir;
		}
		// Update rocket so it faces in the direction its going.
		SetRotation(rotator(Velocity));
	}

	simulated function BeginState(name PreviousStateName)
	{
		Velocity *= 0.6;
		SetTimer(0.1, true);
	}
}


simulated function Landed(vector HitNormal, Actor FloorActor)
{
	Explode(Location, HitNormal);
}

simulated function ProcessTouch(Actor Other, vector HitLocation, vector HitNormal)
{
	if (Other != Instigator && (!Other.IsA('Projectile') || Other.bProjTarget))
	{
		Explode(HitLocation, vect(0,0,1));
	}
}

simulated function Explode(vector HitLocation, vector HitNormal)
{
	ClearTimer('Ignite');
	Super.Explode(HitLocation, HitNormal);
}

defaultproperties
{
	Speed=350.0
	AccelRate=750
	MaxSpeed=4000.0
	SpiralForceMag=800.0
	InwardForceMag=25.0
	ForwardForceMag=15000.0
	DesiredDistanceToAxis=250.0
	DesiredDistanceDecayRate=500.0
	InwardForceMagGrowthRate=0.0
	DT=0.1
	MomentumTransfer=40000
	Damage=50
	DamageRadius=250.0
	MyDamageType=class'UTDmgType_CicadaRocket'
	RemoteRole=ROLE_SimulatedProxy
	LifeSpan=7.0
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=900000)
	DrawScale=0.5
	bCollideWorld=True
	bCollideActors=false
	bNetTemporary=False
	KillRange=1024
	IgniteSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_MissileIgnite'
	AmbientSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_MissileFlight'
	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'VH_Cicada.Mesh.S_VH_Cicada_Rocket'
		Scale3D=(X=1.0,Y=1.0,Z=0.5)
		Translation=(X=50.0)
		Rotation=(Roll=3276)
		Scale=6.0
	End Object
//	Components.Remove(ProjectileMesh)

	ExplosionLightClass=class'UTGame.UTCicadaRocketExplosionLight'
	ProjFlightTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_MissileTrail'
	ProjIgnitedFlightTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_MissileTrailIgnited'
	ProjExplosionTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion'
	bWaitForEffects=true
	bRotationFollowsVelocity=true

}
