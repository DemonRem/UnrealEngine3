/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_ScavengerBoltBase extends UTProjectile
	native;

/** # of times they can bounce before blowing up */
var int Bounces;

/** Current target being tracked */
var Actor TargetActor;

/** How far away bolt can detect a target */
var float DetectionRange;

/** How fast to accelerate towards target being seeked */
var float SeekingAcceleration;

/** Range that target can be attacked (squared) */
var float AttackRangeSq;

/** Max range for attacking target.  If bolt further than this from instigator, it returns */
var float MaxAttackRangeSq;

/** Damage beam effect */
var ParticleSystem BeamEffect;
var ParticleSystemComponent BeamEmitter;

var float SpawnTime;

var float SeekingLifeSpan;

var float LastDamageTime;

var name BeamEndName;

var float DamageFrequency;

var float FastHomeAccel;
var float SlowHomeAccel;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual void ValidateTarget();
}
replication
{
	if ( bNetDirty )
		TargetActor;
}

simulated function PostBeginPlay()
{
    Super.PostBeginPlay();
	SpawnTime = WorldInfo.TimeSeconds;
}

simulated function Landed(vector HitNormal, Actor FloorActor)
{
	HitWall(HitNormal, FloorActor, None);
}

simulated function bool FullySpawned()
{
	return (WorldInfo.TimeSeconds - SpawnTime) > 1.0;
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
}

event KillBolt()
{
	Destroy();
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	bBlockedByInstigator = true;
	Acceleration = vect(0,0,0);

	if ( (Bounces > 0) || (TargetActor == None) )
	{
		// @todo play impact sound
		if ( Wall == Instigator )
		{
			Bounces = Default.Bounces;

			// bounce mostly along tangent
			Velocity = VSize(Velocity) * Normal(Velocity - 1.1 * HitNormal * (Velocity dot HitNormal));
		}
		else
		{
			if ( TargetActor != None )
			{
				PlaySound(ExplosionSound);
				Bounces --;
			}
			else
			{
				Bounces = Default.Bounces;
			}
			Velocity = Velocity - 2.0 * HitNormal * (Velocity dot HitNormal);
		}
	}
	else
	{
		bBounce = false;
		SetPhysics(PHYS_None);
		Explode(Location, HitNormal);
	}
}

simulated function SpawnExplosionEffects(vector HitLocation, vector HitNormal)
{
	local vector x;
	if ( WorldInfo.NetMode != NM_DedicatedServer && EffectIsRelevant(Location,false,MaxEffectDistance) )
	{
		x = normal(Velocity cross HitNormal);
		x = normal(HitNormal cross x);

		WorldInfo.MyEmitterPool.SpawnEmitter(ProjExplosionTemplate, HitLocation, rotator(x));
		bSuppressExplosionFX = true; // so we don't get called again
	}

	if (ExplosionSound!=None)
	{
		PlaySound(ExplosionSound);
	}
}

simulated function SetTargetActor( actor HitActor )
{
	local pawn TargetPawn;

	if ( UTOnslaughtObjective(HitActor) != None )
	{
		TargetActor = HitActor;
		return;
	}
	TargetPawn = Pawn(HitActor);
	if ( (TargetPawn != None) && (TargetPawn.Health > 0) && !WorldInfo.GRI.OnSameTeam(Instigator, TargetPawn) )
	{
		TargetActor = TargetPawn;
	}
	if ( LifeSpan == 0.0 )
	{
		LifeSpan = SeekingLifeSpan;
	}
}

simulated event SpawnBeam()
{
	if ( BeamEmitter == None )
	{
		BeamEmitter = new(Outer) class'UTParticleSystemComponent';
		BeamEmitter.SetTemplate(BeamEffect);
		BeamEmitter.SetTickGroup( TG_PostAsyncWork );
		AttachComponent(BeamEmitter);
	}
	BeamEmitter.SetHidden(false);
}

event DealDamage(vector HitLocation)
{
	local UTOnslaughtObjective TargetNode;

	if ( (Instigator == None) || !FastTrace(Location, Instigator.Location) )
	{
		TargetActor = None;
		return;
	}
	TargetNode = UTOnslaughtObjective(TargetActor);
	LastDamageTime = WorldInfo.TimeSeconds;
	if ( (TargetNode != None) && WorldInfo.GRI.OnSameTeam(Instigator, TargetNode) )
	{

		TargetNode.HealDamage( Damage, InstigatorController, MyDamageType);
		if ( TargetNode.Health >= TargetNode.DamageCapacity )
		{
			TargetActor = None;
			LifeSpan = FMax(LifeSpan, 7.0);
		}
	}
	else
	{
		// damage the target
		TargetActor.TakeDamage( Damage, InstigatorController, HitLocation, vect(0,0,0), MyDamageType,, self);
		if ( TargetNode == None )
		{
			LifeSpan = FMax(LifeSpan - 0.5*DamageFrequency, 0.0001);
		}
	}
}

event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	// just knock away

	Velocity += Momentum*0.1;
}

defaultproperties
{
	Bounces=8
	bBounce=true
	bRotationFollowsVelocity=true

    Speed=80
    MaxSpeed=900
    AccelRate=0.0

    Damage=10
    DamageRadius=0
    MomentumTransfer=0
	CheckRadius=0.0
    LifeSpan=0.0
	DetectionRange=1000
	SeekingAcceleration=2000
	AttackRangeSq=90000.0
	MaxAttackRangeSq=36000000.0

    bCollideWorld=true
	bNetTemporary=false
	SeekingLifeSpan=10.0

	Begin Object Name=CollisionCylinder
		CollisionRadius=16
		CollisionHeight=16
		AlwaysLoadOnClient=True
		AlwaysLoadOnServer=True
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object

	BeamEndName=LinkBeamEnd

	DamageFrequency=+0.2
	FastHomeAccel=2400.0
	SlowHomeAccel=2000.0
	bBlockedByInstigator=true
}
