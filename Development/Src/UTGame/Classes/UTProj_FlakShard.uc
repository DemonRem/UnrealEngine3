/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_FlakShard extends UTProjectile;

/** # of times they can bounce */
var int Bounces;

/** How fast damage is redusced per second from when the chunk is fired */
var float DamageAttenuation;

var float ShrinkTimer;

var ParticleSystem BounceTemplate;
var ParticleSystem RockSmokeTemplate;

/** reference to impact effect we created; we wait for this before being destroyed */
var array<ParticleSystemComponent> ImpactEffects;

var array<MaterialSoundEffect> HitSoundsList;
var MaterialSoundEffect DefaultHitSound;
var SoundCue HitPawnSound;
replication
{
	if (bNetInitial && Role == ROLE_Authority)
		Bounces;
}

function Init(vector Direction)
{
	local float rnd;

	Super.Init(Direction);

	if (PhysicsVolume.bWaterVolume)
	{
		Velocity *= 0.65;
	}

	rnd = frand();
	if (rnd > 0.63)
	{
		Bounces = (rnd > 0.88) ? 1 : 3;
	}
	else
	{
		Bounces=2;
	}

	SetRotation(RotRand());
}

/** 
  * Attenuate shard damage at long range
  */
simulated function float GetDamage(Actor Other, vector HitLocation)
{
	return Max(5, Damage - DamageAttenuation * FMax(0.f, (default.Lifespan - LifeSpan - 1)));
}

simulated function float GetMomentumTransfer()
{
	return MomentumTransfer;
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	local Vehicle V;

	if ( (UTProj_FlakShard(Other) == none) && (Physics == PHYS_Falling || Other != Instigator) )
	{
		speed = VSize(Velocity);
		if (Speed>400)
		{
			Other.TakeDamage(GetDamage(Other, HitLocation), InstigatorController, HitLocation, GetMomentumTransfer() * Normal(Velocity), MyDamageType,, self);
			if ( Role == ROLE_Authority )
			{
				V = Vehicle(Other);
				if (V != None && PlayerController(V.Controller) != None)
				{
					SetImpactSound(HitNormal, none);
					PlayerController(V.Controller).ClientPlaySound(ImpactSound);
				}
				else if(Pawn(Other) != none)
				{
					PlaySound(HitPawnSound);
				}
			}
		}
		Shutdown();
	}
}

simulated function Landed(vector HitNormal, Actor FloorActor)
{
	HitWall(HitNormal, FloorActor, None);
}

simulated function SetImpactSound(vector HitNormal, PrimitiveComponent Component)
{
	local TraceHitInfo HitInfo;
	local vector HitLoc, HitNorm;
	local int i;
	local UTPhysicalMaterialProperty PhysicalProperty;
	if(Component != none)
	{
		TraceComponent(HitLoc, HitNorm, Component, location-HitNormal, location+HitNormal,,HitInfo);
	}
	else
	{
		Trace(HitLoc, HitNorm, location-HitNormal, location+HitNormal, true,,HitInfo);
		//`log("Traced"@HitInfo.HitComponent);
	}
	if(HitInfo.PhysMaterial != none)
	{
		PhysicalProperty = UTPhysicalMaterialProperty(HitInfo.PhysMaterial.GetPhysicalMaterialProperty(class'UTPhysicalMaterialProperty'));
		//`log("Searching for"@PhysicalProperty.MaterialType);
		if( PhysicalProperty != none )
		{
			i = HitSoundsList.Find('MaterialType',PhysicalProperty.MaterialType);
			if (i != -1)
			{
				ImpactSound = HitSoundsList[i].Sound;
			}
			else
			{
			ImpactSound = DefaultHitSound.Sound;
			}
		}
	}
	else
	{
		ImpactSound = DefaultHitSound.Sound;
	}
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	local ParticleSystemComponent ImpactEffect;
	local audioComponent HitSoundComp;

	bBlockedByInstigator = true;

	if ( !Wall.bStatic && !Wall.bWorldGeometry && Wall.bProjTarget )
	{
		Wall.TakeDamage(GetDamage(Wall, Location), InstigatorController, Location, GetMomentumTransfer() * Normal(Velocity), MyDamageType,, self);
		Shutdown();
	}
	else
	{
		// spawn impact effect
		if (EffectIsRelevant(Location, false, MaxEffectDistance))
		{
			ImpactEffect = new(Outer) class'UTParticleSystemComponent';
			ImpactEffect.SetAbsolute(true, true, true);
			ImpactEffect.SetTranslation(Location);
			ImpactEffect.SetRotation(rotator(HitNormal));
			ImpactEffect.SetTemplate((Bounces > 0) ? BounceTemplate : RockSmokeTemplate);
			ImpactEffect.OnSystemFinished = MyOnParticleSystemFinished;
			AttachComponent(ImpactEffect);
			ImpactEffects[ImpactEffects.length] = ImpactEffect;
		}
		SetImpactSound(HitNormal,WallComp);

		
		SetPhysics(PHYS_Falling);
		if (Bounces > 0)
		{
			if ((!WorldInfo.bDropDetail && FRand() < 0.4) || (UTProj_FlakShardMain(self) != none))
			{
				
				HitSoundComp = CreateAudioComponent( ImpactSound, false, false, true, location );
				if(HitSoundComp != none)
				{
					if(Bounces ==2)
					{
						HitSoundComp.VolumeMultiplier = 1.0;
					}
					else if(Bounces ==1)
					{
						HitSoundComp.VolumeMultiplier = 0.5;
					}
					else if (Bounces ==0)
					{
						HitSoundComp.VolumeMultiplier = 0.2;
					}
					HitSoundComp.Play();
				}
				
			}

			Velocity = 0.8 * (Velocity - 2.0 * HitNormal * (Velocity dot HitNormal));
			Bounces = Bounces - 1;

			if (Bounces == 0)
			{
				if (ProjEffects != None)
				{
					ProjEffects.DeactivateSystem();
				}
			}
		}
		else
		{
			bBounce = false;
			SetPhysics(PHYS_None);
			SetTimer(0.5 * FRand(), false, 'StartToShrink');
		}
	}
}

simulated function StartToShrink()
{
	GotoState('Shrink');
}

simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	local int i;

	if (ImpactEffects.length == 1)
	{
		Destroy();
	}
	else
	{
		i = ImpactEffects.Find(PSC);
		if (i != -1)
		{
			ImpactEffects.Remove(i, 1);
		}
		DetachComponent(PSC);
	}
}

simulated state Shrink
{
	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		ShrinkTimer = 0.7;
		if (WorldInfo.NetMode == NM_DedicatedServer || ImpactEffects.length == 0)
		{
			LifeSpan = 0.7;
		}
	}

	simulated function Tick(float DeltaTime)
	{
		ShrinkTimer -= DeltaTime;
		SetDrawScale(FMax(0.01f, (ShrinkTimer / 0.7) * 0.5));
	}
}


defaultproperties
{
	speed=2500.000000
	MaxSpeed=2700.000000
	Damage=18
	DamageRadius=+0.0
	DamageAttenuation=5.0
	MaxEffectDistance=5000.0

	MomentumTransfer=14000
	MyDamageType=class'UTDmgType_FlakShard'
	LifeSpan=3
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	bCollideWorld=true
	CheckRadius=20.0

	bBounce=true
	Bounces=2
	NetCullDistanceSquared=+49000000.0

	ProjFlightTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_Flak_trails'

	// Add the Mesh
	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_FlakCannon.Mesh.S_WP_Flak_Rock'
		CullDistance=3000
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)

	ImpactSound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactDirtCue'
	DefaultHitSound=(Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactDirtCue')
	HitSoundsList(0)=(MaterialType=Dirt,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactDirtCue')
	HitSoundsList(1)=(MaterialType=Flesh,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactFleshCue')
	HitSoundsList(2)=(MaterialType=Glass,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactGlassCue')
	HitSoundsList(3)=(MaterialType=Metal,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactMetalCue')
	HitSoundsList(4)=(MaterialType=Snow,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactSnowCue')
	HitSoundsList(5)=(MaterialType=Stone,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactStoneCue')
	HitSoundsList(6)=(MaterialType=Water,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactWaterCue')
	HitSoundsList(7)=(MaterialType=Wood,Sound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactWoodCue')
	HitPawnSound=SoundCue'A_Weapon_FlakCannon.Weapons.A_FlakCannon_FireImpactFleshCue'

	bWaitForEffects=false

	BounceTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_Flak_Rock_bounce'
	RockSmokeTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_Flak_rocksmoke'
	
	
}
