/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_Grenade extends UTProjectile
	native;

cpptext
{
	virtual void physicsRotation(FLOAT deltaTime);
}

/**
 * Set the inital velocity and cook time
 */
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetTimer(2.5+FRand()*0.5,false);                  //Grenade begins unarmed
	RandSpin(100000);
}

function Init(vector Direction)
{
	SetRotation(Rotator(Direction));

	Velocity = Speed * Direction;
	TossZ = TossZ + (FRand() * TossZ / 2.0) - (TossZ / 4.0);
	Velocity.Z += TossZ;
	Acceleration = AccelRate * Normal(Velocity);
}

/**
 * Explode
 */
simulated function Timer()
{
	Explode(Location,Normal(Velocity));
}

/**
 * Give a little bounce
 */
simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	bBlockedByInstigator = true;

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		PlaySound(ImpactSound, true);
	}

	// check to make sure we didn't hit a pawn

	if ( Pawn(Wall) == none )
	{
		Velocity = 0.75*(( Velocity dot HitNormal ) * HitNormal * -2.0 + Velocity);   // Reflect off Wall w/damping
		Speed = VSize(Velocity);

		if (Velocity.Z > 400)
		{
			Velocity.Z = 0.5 * (400 + Velocity.Z);
		}
		// If we hit a pawn or we are moving too slowly, explod

		if ( Speed < 20 || Pawn(Wall) != none )
		{
			ImpactedActor = Wall;
			SetPhysics(PHYS_None);
			if (ProjEffects!=None)
			{
				ProjEffects.DeactivateSystem();
			}
		}
	}
	else if ( Wall != Instigator ) 	// Hit a different pawn, just explode
	{
		Explode(Location, HitNormal);
	}
}

/**
 * When a grenade enters the water, kill effects/velocity and let it sink
 */
simulated function PhysicsVolumeChange( PhysicsVolume NewVolume )
{
	if ( WaterVolume(NewVolume) != none )
	{
		Velocity = vect(0,0,0);
		if (ProjEffects!=None)
		{
			ProjEffects.DeactivateSystem();
		}

		// FIXME: Add water effects

	}

	Super.PhysicsVolumeChange(NewVolume);
}


defaultproperties
{
	ProjFlightTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_Smoke_Trail'

	ProjExplosionTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'

	speed=700
	MaxSpeed=1000.0
	Damage=100.0
	DamageRadius=200
	MomentumTransfer=50000
	MyDamageType=class'UTDmgType_Grenade'
	LifeSpan=0.0
	ExplosionSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Impact_Cue'
	bCollideWorld=true
	bBounce=true
	TossZ=+245.0
	Physics=PHYS_Falling
	CheckRadius=40.0

	ImpactSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_GrenadeFloor_Cue'

	bNetTemporary=False
	bWaitForEffects=false

	CustomGravityScaling=0.5

	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_RocketLauncher.Mesh.S_WP_Rocketlauncher_Rocket'
		Scale=1.0
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		CullDistance=7500
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)
}
