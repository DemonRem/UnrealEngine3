/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_BioGlob extends UTProj_BioShot;

var() int			MaxRestingGlobStrength;
var repnotify int	GlobStrength;
var() float			GloblingSpeed;

var SkeletalMeshComponent GooSkel;
var ParticleSystemComponent HitWallEffect;

replication
{
	if (bNetInitial)
		GlobStrength;
}

function InitBio(UTWeap_BioRifle FiringWeapon, int InGlobStrength)
{
	// adjust speed
	InGlobStrength = Max(InGlobStrength, 1);
	Velocity = Normal(Velocity) * Speed * (0.4 + InGlobStrength)/(1.35*InGlobStrength);
	Velocity.Z += TossZ;

	InitStats(FiringWeapon);
	SetGlobStrength(InGlobStrength);
	RestTime = Default.RestTime + InGlobStrength;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (SkeletalMeshComponent(GooMesh) != None)
	{
		SkeletalMeshComponent(GooMesh).PlayAnim('Shake2', SkeletalMeshComponent(GooMesh).GetAnimLength('Shake2') * 0.5, true);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'RemainingRestTime')
	{
		if (!IsInState('OnGround'))
		{
			Landed(vector(Rotation), None);
		}
	}
	else if (VarName == 'GlobStrength')
	{
		SetGlobStrength(GlobStrength);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/**
 * Sets the strength of this bio goo actor
 */
simulated function SetGlobStrength( int NewStrength )
{
	GlobStrength = Max(NewStrength,1);
	SetDrawScale(Sqrt(GlobStrength) * default.DrawScale);

	// set different damagetype for charged shots
	if (GlobStrength > 1)
	{
		MyDamageType = default.MyDamageType;
	}
	else
	{
		MyDamageType = class'UTProj_BioShot'.default.MyDamageType;
	}
}

simulated function GrowCollision()
{
	local float CollRadius, CollHeight;

	CollRadius = 10 + 10 * Sqrt(GlobStrength);
	CollHeight = FMin(32, CollRadius);
	CollisionComponent.SetTranslation((CollHeight/DrawScale)*vect(1.0,0,0));
	SetCollisionSize(CollRadius,CollHeight);
}

/**
 * Explode this glob
 */
simulated function Explode(Vector HitLocation, vector HitNormal)
{
	local float GlobVolume;

	if ( bExploded )
		return;

	bExploded = true;
	GlobVolume = Sqrt(GlobStrength);
	Damage = Default.Damage * GlobStrength;
	MomentumTransfer = Default.MomentumTransfer * GlobVolume;

	if ( ImpactedActor == None )
	{
		ImpactedActor = Base;
	}
	ProjectileHurtRadius(Damage, DamageRadius, MyDamageType, MomentumTransfer, HitLocation, HitNormal);
	SpawnExplosionEffects(HitLocation, HitNormal );
	Shutdown();
}

/**
 * Spawns several children globs
 */
simulated function SplashGloblings()
{
	local int g;
	local UTProj_BioShot NewGlob;
	local float GlobVolume;
	local Vector VNorm;

	if ( (GlobStrength > MaxRestingGlobStrength) && (UTPawn(Base) == None) )
	{
		if (Role == ROLE_Authority)
		{
			GlobVolume = Sqrt(GlobStrength);

			for (g=0; g<GlobStrength-MaxRestingGlobStrength; g++)
			{
				NewGlob = Spawn(class'UTProj_BioGlobling', self,, Location+GlobVolume*6*SurfaceNormal);
				if (NewGlob != None)
				{
					// init newglob
					NewGlob.Velocity = (GloblingSpeed + FRand()*150.0) * (SurfaceNormal + VRand()*0.8);
					if (Physics == PHYS_Falling)
					{
						VNorm = (Velocity dot SurfaceNormal) * SurfaceNormal;
						NewGlob.Velocity += (-VNorm + (Velocity - VNorm)) * 0.1;
					}
					NewGlob.InstigatorController = InstigatorController;

					NewGlob.FiringOwnerStatsID = FiringOwnerStatsID;
					NewGlob.FiringWeaponStatsID = FiringWeaponStatsID;
					NewGlob.FiringWeaponMode = FiringWeaponMode;
				}
			}
		}
		SetGlobStrength(MaxRestingGlobStrength);
	}
}


auto state Flying
{
	simulated event Landed(vector HitNormal, Actor FloorActor)
	{
		local SkelControlLookAt SCLA;

		SurfaceNormal = HitNormal;
		SplashGloblings();
		// align and unhide the skeletal mesh version:
		if (WorldInfo.Netmode != NM_DedicatedServer && GooSkel != none)
		{
			DetachComponent(GooMesh);
			AttachComponent(GooSkel);
			HitWallEffect.ActivateSystem();
			SCLA = SkelControlLookAt(GooSkel.FindSkelControl('LookControl'));
			if(SCLA != none)
			{
				SCLA.TargetLocation = HitNormal+Location;
			}
		}
		Super.Landed(HitNormal, FloorActor);
	}

	simulated function HitWall( Vector HitNormal, Actor HitWall, PrimitiveComponent WallComp )
	{
		SurfaceNormal = HitNormal;
		SplashGloblings();
		Super.HitWall(HitNormal, HitWall, WallComp);
	}

	simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
	{
		if ( !Other.bWorldGeometry )
		{
			RestTime = 1.0;
		}
		if ( UTPawn(Other) != None )
		{
			RestTime = 1.0;
			SurfaceNormal = HitNormal;
			Super.HitWall(HitNormal, Other, None);
		}
		else if ( !bExploded && (Other.bProjTarget || ((UTProj_BioShot(Other) != None) && (UTProj_BioGlob(Other) == None))) )
		{
			SplashGloblings();
			Explode( HitLocation, HitNormal );
		}
   	}
}

simulated function MergeWithGlob(int AdditionalGlobStrength)
{
	SetGlobStrength(GlobStrength + AdditionalGlobStrength);
	if ( UTPawn(Base) == None )
	{
		RestTime += AdditionalGlobStrength;
		if (RemainingRestTime > 0.0)
		{
			RemainingRestTime += AdditionalGlobStrength;
		}
	}
	SplashGloblings();
}

state OnGround
{
	simulated function Drip()
	{
		// @todo drop a globling
	}

	simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
	{
		local UTProj_BioGlob Glob;

		Glob = UTProj_BioGlob(Other);
		if ( Glob != None )
		{
			if (  (Glob.Owner != self) && (Owner != Glob) )
			{
				MergeWithGlob(GlobStrength);
				Glob.Destroy();
			}
		}
		else if ( !bExploded && (UTProj_BioGlobling(Other) == None)
					&& ((Other.bProjTarget && (Other != Base)) || (UTProj_BioShot(Other) != None)) )
		{
			Explode(Location, SurfaceNormal );
		}
   	}

	simulated function MergeWithGlob(int AdditionalGlobStrength)
	{
		SetGlobStrength(GlobStrength+AdditionalGlobStrength);
		SplashGloblings();
		GrowCollision();
		PlaySound(ImpactSound);
		bCheckedSurface = false;
		SetTimer(GetTimerCount() + AdditionalGlobStrength, false);
	}


	simulated function AnimEnd(int Channel)
	{
		local float DotProduct;

		if (!bCheckedSurface)
		{
			DotProduct = SurfaceNormal dot Vect(0,0,-1);
			if (DotProduct > 0.7)
			{
				//                PlayAnim('Drip', 0.66);
				SetTimer(DripTime, false, 'Drip');
				if (bOnMover)
				{
					Explode(Location, SurfaceNormal);
				}
			}
			else if (DotProduct > -0.5)
			{
				//                PlayAnim('Slide', 1.0);
				if (bOnMover)
				{
					Explode(Location, SurfaceNormal);
				}
			}
			bCheckedSurface = true;
		}
	}
}

defaultproperties
{
	Components.Empty();
	MaxRestingGlobStrength=6
	GlobStrength=1
	GloblingSpeed=200.0
	DamageRadius=120.0
	bProjTarget=true
	bNetTemporary=false
	DrawScale=0.5
	LifeSpan=20.0
	MyDamageType=class'UTDmgType_BioGoo_Charged'

	Begin Object Name=CollisionCylinder
		CollisionRadius=20
		CollisionHeight=4
	End Object
	components.Add(CollisionCylinder)
	Explosionsound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_FireAltImpactExplode_Cue'
	ImpactSound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_FireAltImpactExplode_Cue'
	AmbientSound=SoundCue'A_Weapon_BioRifle.Weapon.A_BioRifle_Inair_Cue'

	Begin Object Class=SkeletalMeshComponent Name=ProjectileSkelMeshFloor
		SkeletalMesh=SkeletalMesh'WP_BioRifle.Mesh.WP_Bio_Alt_Blob_Base'
		AnimTreeTemplate=AnimTree'WP_BioRifle.WP_Bio_Alt_Blob_Morph_AnimTree'
		CullDistance=12000
		Scale=0.9
		bAcceptsDecals=false
	End Object
	GooSkel=ProjectileSkelMeshFloor
	// attached when projectile lands

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object
	Begin Object Class=SkeletalMeshComponent Name=ProjectileSkelMeshAir
		SkeletalMesh=SkeletalMesh'WP_BioRifle.Mesh.SK_WP_Bio_Alt_Projectile_Blob'
		AnimSets(0)=AnimSet'WP_BioRifle.Anims.Bio_Alt_Blob_Projectile_Shake'
		Animations=MeshSequenceA
		bAcceptsDecals=false
	End Object
	goomesh=ProjectileSkelMeshAir
	Components.Add(ProjectileSkelMeshAir)

	FloorHit=none
	CeilingHit=none
	WallHit=none

	Begin Object Class=ParticleSystemComponent Name=HitWallFX
		Template=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Alt_Blob_Impact'
		bAutoActivate=false
		SecondsBeforeInactive=1.0f
	End Object
	HitWallEffect=HitWallFX
	ProjExplosionTemplate=ParticleSystem'WP_BioRifle.Particles.P_WP_Bio_Alt_Blob_POP'
}
