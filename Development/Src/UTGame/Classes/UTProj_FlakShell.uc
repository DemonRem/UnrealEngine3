/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_FlakShell extends UTProjectile;

simulated function Explode(vector HitLocation, vector HitNormal)
{
	local vector SpawnPos;
	local actor HitActor;
	local rotator rot;
	local int i;
	local UTProj_FlakShard NewChunk;

	Super.Explode(HitLocation, HitNormal);

	SpawnPos = Location + 10 * HitNormal;

	HitActor = Trace(HitLocation, HitNormal, SpawnPos, Location, false);
	if (HitActor != None)
	{
		SpawnPos = HitLocation;
	}
	if (Role == ROLE_Authority)
	{
		for (i = 0; i < 5; i++)
		{
			rot = Rotation;
			rot.yaw += FRand() * 32000 - 16000;
			rot.pitch += FRand() * 32000 - 16000;
			rot.roll += FRand() * 32000 - 16000;
			NewChunk = Spawn(class 'UTProj_FlakShard',, '', SpawnPos, rot);
			if (NewChunk != None)
			{
				NewChunk.Init(vector(rot));

				// Pass along stats data
				NewChunk.FiringOwnerStatsID = FiringOwnerStatsID;
				NewChunk.FiringWeaponStatsID = FiringWeaponStatsID;
				NewChunk.FiringWeaponMode = FiringWeaponMode;
			}
		}
	}
}

defaultproperties
{
	DamageRadius=+200.0
	speed=1200.000000
	Damage=100.000000
	MomentumTransfer=75000
	MyDamageType=class'UTDmgType_FlakShell'
	LifeSpan=6.0
	bCollideWorld=true
	TossZ=+305.0
	CheckRadius=40.0

	ProjFlightTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_Flak_Alt_Smoke_Trail'
	ProjExplosionTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_Flak_Alt_Explosion'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'
	ExplosionDecal=Material'WP_RocketLauncher.M_WP_RocketLauncher_ImpactDecal'
	DecalWidth=128.0
	DecalHeight=128.0

	AmbientSound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireAltInAirCue'
	ExplosionSound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireAltImpactExplodeCue'

	Physics=PHYS_Falling
	CustomGravityScaling=1.0

	DrawScale=1.5

	// mesh
	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_FlakCannon.Mesh.S_WP_Flak_Shell'
		scale=3.0
		Rotation=(Pitch=16384,Yaw=-16384)
		CullDistance=4000
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)

	bWaitForEffects=true
	bRotationFollowsVelocity=true
}
