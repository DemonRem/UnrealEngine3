/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_BioShot extends UTProjectile;

var Vector			SurfaceNormal;
var float			RestTime;
var float			DripTime;
var bool 			bCheckedSurface;
var bool			bOnMover;
var bool			bLanding;
var bool			bExploded;
var repnotify float RemainingRestTime;

var enum HitType
{
	HIT_None,
	HIT_Wall,
	HIT_Ceiling,
	HIT_Floor
} HitMode;

var float WallThreshold;
var ParticleSystem WallHit;
var ParticleSystem FloorHit;
var ParticleSystem CeilingHit;
var ParticleSystem HitPawnTemplate;
var ParticleSystem HitBioTemplate;
var Emitter HitEmitter;
var MaterialInterface GooDecalTemplate;

/** sound to play on explosion due to being stepped in */
var SoundCue SteppedInSound;
var MaterialInterface GooDecalChoices[2];
var DecalComponent GooDecal;
var MaterialInstanceConstant GooDecalMaterial;
var MeshComponent GooMesh;

/** used to notify AI of bio on the ground */
var UTAvoidMarker FearSpot;

replication
{
	if (bNetDirty)
		RemainingRestTime;
}

function InitBio(UTWeap_BioRifle FiringWeapon, int InGlobStrength)
{
	// adjust speed
	Velocity.Z += TossZ;

	InitStats(FiringWeapon);
}

simulated event PhysicsVolumeChange(PhysicsVolume NewVolume)
{
	if ( PhysicsVolume.bWaterVolume && !NewVolume.bWaterVolume )
		Buoyancy = 0.5 * (Buoyancy + 1.08);
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (FearSpot != None)
	{
		FearSpot.Destroy();
	}
}

/**
 * Explode this glob
 */
simulated function Explode(Vector HitLocation, vector HitNormal)
{
	if ( bExploded )
		return;

	bExploded = true;
	SpawnExplosionEffects(HitLocation, HitNormal );
	Shutdown();
}

simulated function SpawnExplosionEffects(vector HitLocation, vector HitNormal)
{

	if(GooDecal != none && GooDecalMaterial != none)
	{
		GooDecalMaterial.SetScalarParameterValue('Blend',1);
	}

	if(HitEmitter != none) // shut down
	{
		HitEmitter.bCurrentlyActive=false;
		HitEmitter.Lifespan = 0.0001f; // seriously, goo, go away. :)
	}
	super.SpawnExplosionEffects(HitLocation, HitNormal);
}

simulated function GrowCollision()
{
	CollisionComponent.SetTranslation(vect(16,0,0));
	SetCollisionSize(16,16);
}

auto state Flying
{
	simulated event Landed(vector HitNormal, Actor FloorActor)
	{
		local Rotator NewRot;
		local float nDecalExtent;
		local DecalLifetimeData LifetimeData;
		local ParticleSystem CurrentHit;
		local int GooChoice;

		if ( bLanding )
			return;
		bLanding = true;
		if (WorldInfo.NetMode != NM_DedicatedServer && FloorActor != None)
		{
			GooChoice = rand(ArrayCount(GooDecalChoices));
			GooDecalTemplate = GooDecalChoices[GooChoice];

			nDecalExtent=70+Rand(15);
			nDecalExtent *= DrawScale/default.DrawScale;

			GooDecal = new(self) class'DecalComponent';

			GooDecal.DecalMaterial = GooDecalTemplate;

			GooDecalMaterial = new(Outer) class'MaterialInstanceConstant';
			GooDecalMaterial.SetParent(GooDecal.DecalMaterial);
			GooDecal.DecalMaterial = GooDecalMaterial;
			GooDecalMaterial.SetScalarParameterValue('Blend',0);

			LifetimeData = new(FloorActor) class'DecalLifetimeDataAge';
			// FIXME: decal lifespan based on detail level
			DecalLifetimeDataAge(LifetimeData).LifeSpan += LifeSpan;
			/*GooDecal = FloorActor.AddDecal( GooDecal.DecalMaterial, Location, rotator(-HitNormal),
							FRand() * 360, nDecalExtent, nDecalExtent, 1.0,
							true, None, LifetimeData, true, false, '', -1, -1,,0.1 );
							*/
			GooDecal.Location = Location;
			GooDecal.Orientation = rotator(-HitNormal);
			GooDecal.DecalRotation = FRand() * 360;
			GooDecal.Width = nDecalExtent;
			GooDecal.Height = nDecalExtent;
			GooDecal.Thickness = 10.0;
			GooDecal.FarPlane = GooDecal.Thickness * 0.5;
			GooDecal.NearPlane = -GooDecal.FarPlane;
			GooDecal.BackfaceAngle = 0.3;
			GooDecal.bNoClip = false;
			GooDecal.HitComponent = none;
			GooDecal.HitBone = '';
			GooDecal.HitNodeIndex = -1;
			GooDecal.HitLevelIndex = -1;
			GooDecal.LifetimeData = LifetimeData;
			GooDecal.bProjectOnTerrain = true;
			GooDecal.bProjectOnSkeletalMeshes = false;
			GooDecal.DepthBias = -0.001;
			FloorActor.AttachComponent(GooDecal);
			PlaySound(ImpactSound, true);
		}

		SurfaceNormal = HitNormal;
		if(Abs(HitNormal.Z) > WallThreshold) // A wall will have a low Z in the HitNormal since it's a unit vector
		{
			// is normal pointing up (floor) or down (ceiling)?
			CurrentHit = (HitNormal.Z >= 0) ? FloorHit : CeilingHit;
			HitMode = (HitNormal.Z >=0) ? HIT_Floor : HIT_Ceiling;
		}
		else
		{
			CurrentHit = WallHit;
			HitMode = HIT_Wall;
		}
		if (WorldInfo.NetMode != NM_DedicatedServer && CurrentHit != none)
		{
			HitEmitter = Spawn(class'UTEmitter', self,, location, rotator(HitNormal));
			if (HitEmitter != None)
			{
				HitEmitter.SetTemplate(CurrentHit, true);
			}
		}

		bBlockedByInstigator = true;
		bCollideWorld = false;
		bProjTarget = true;
		NewRot = Rotator(HitNormal);
		NewRot.Roll += 32768;
		SetRotation(NewRot);
		GrowCollision();
		SetPhysics(PHYS_None);
		if(GooMesh != none)
		{
			GooMesh.SetHidden(true);
		}
		bCheckedsurface = false;

		// spawn marker so AI can avoid
		if (FearSpot == None && WorldInfo.Game != None && WorldInfo.Game.NumBots > 0)
		{
			FearSpot = Spawn(class'UTAvoidMarker');
		}

		GotoState('OnGround');
		bLanding = false;
	}

	simulated function HitWall( Vector HitNormal, Actor Wall, PrimitiveComponent WallComp )
	{
		Landed(HitNormal, Wall);
		if ( !Wall.bStatic && !Wall.bWorldGeometry && (Physics == Phys_None) )
		{
			bOnMover = true;
			SetBase(Wall);
			if (Base == None)
			{
				Explode(Location, HitNormal);
			}
		}
	}

	simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
	{
		if ( (UTProj_BioShot(Other) != None) || Other.bProjTarget )
		{
			if ( !bExploded )
			{
				if(UTPawn(Other) != none)
				{
					ProjExplosionTemplate=HitPawnTemplate;
				}
				else
				{
					if(UTProj_BioGlob(Other) != none || UTProj_BioGlobling(Other) != none || UTProj_Bioshot(Other) != none)
					{
						ProjExplosionTemplate=HitBioTemplate;
					}
				}
				Other.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
				Explode( HitLocation, HitNormal );
			}
		}
   	}
}

state OnGround
{
	simulated function BeginState(Name PreviousStateName)
	{
		RemainingRestTime = RestTime;
		SetTimer(0.5, true);
	}

	simulated function Timer()
	{
		RemainingRestTime -= 0.5;
		if ( RemainingRestTime <= 0.0 )
		{
			Explode(Location, SurfaceNormal );
		}
	}

	simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
	{
		if ( (Other.bProjTarget && (Other != Base)) || (UTProj_BioShot(Other) != None) )
		{
			if ( !bExploded )
			{
				if(UTProj_BioGlob(Other) != none || UTProj_BioGlobling(Other) != none || UTProj_Bioshot(Other) != none)
				{
					ProjExplosionTemplate=HitBioTemplate;
				}
				Other.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
				ExplosionSound=SteppedInSound;
				Explode(Location, SurfaceNormal );
			}
		}
	}

	simulated event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		//`log(self$" take damage!");
		RemainingRestTime = -1.0;
		Timer();
	}

	simulated function HitWall( Vector HitNormal, Actor Wall, PrimitiveComponent WallComp );
}


simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	return;
}

defaultproperties
{
	Speed=2000.0
	Damage=21.0
	MomentumTransfer=40000
	MyDamageType=class'UTDmgType_BioGoo'
	LifeSpan=12.0
	RotationRate=(Pitch=50000)
	DesiredRotation=(Pitch=30000)
	bCollideWorld=true
	TossZ=0.0
	MaxEffectDistance=7000.0
	Buoyancy=1.5

	RestTime=3.0
	DripTime=1.8
	CheckRadius=40.0


	//	ProjFlightTemplate=ParticleSystem'Envy_Effects.FX.Bio_Splat'
	ProjExplosionTemplate=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Primary_PoP'
	HitPawnTemplate=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Player_Hit'
	HitBioTemplate=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Blob_hits_Blob_Burst'
	ExplosionLightClass=class'UTGame.UTBioExplosionLight'

	Explosionsound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_FireImpactExplode_Cue'
	ImpactSound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_FireImpactExplode_Cue'
	Physics=PHYS_Falling

	Begin Object Name=CollisionCylinder
		CollisionRadius=0
		CollisionHeight=0
		CollideActors=true
	End Object

	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_BioRifle.Mesh.S_Bio_Ball'
		CullDistance=12000
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)
	GooMesh=ProjectileMesh

	WallHit=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Impact_Primary_Wall';
	FloorHit=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Impact_Primary_Floor';
	CeilingHit=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Impact_Primary_Ceiling';

	WallThreshold = 0.3f;
	GooDecalTemplate=MaterialInterface'WP_BioRifle.Materials.Bio_Splat_Decal'
	bWaitForEffects=false

	GooDecalChoices[0]=MaterialInterface'WP_BioRifle.Materials.Bio_Splat_Decal'
	GooDecalChoices[1]=MaterialInterface'WP_BioRifle.Materials.Bio_Splat_Decal_001'
	SteppedInSound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_FireImpactFizzle_Cue'
	HitMode=HIT_None
}
