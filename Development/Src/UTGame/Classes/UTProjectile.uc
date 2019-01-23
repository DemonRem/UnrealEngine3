/**
 * UTProjectile
 *
 * This is our base projectile class.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProjectile extends Projectile
	abstract
	native
	nativereplication;

/** Additional Sounds */
var bool bSuppressSounds;
var SoundCue	AmbientSound;		// The sound that is played looped.
var SoundCue	ExplosionSound;		// The sound that is played when it explodes

/** If true, never cut out ambient sound for perf reasons */
var	bool		bImportantAmbientSound;

/** Effects */

/** This is the effect that is played while in flight */
var ParticleSystemComponent	ProjEffects;

/** Effects Template */
var ParticleSystem ProjFlightTemplate;
var ParticleSystem ProjExplosionTemplate;

/** If true, this explosion effect expects to be orientated differently and have extra data
    passed in via parameters */
var bool bAdvanceExplosionEffect;

/** decal for explosion */
var MaterialInterface ExplosionDecal;
var float DecalWidth, DecalHeight;

/** This value sets the cap how far away the explosion effect of this projectile can be seen */
var float MaxEffectDistance;

/** used to prevent effects when projectiles are destroyed (see LimitationVolume) */
var bool bSuppressExplosionFX;
/** if True, this projectile will remain alive (but hidden) until the flight effect is done */
var bool bWaitForEffects;
/** if true, the shutdown function has been called and 'new' effects shouldn't happen */
var bool bShuttingDown;

/** Acceleration magnitude. By default, acceleration is in the same direction as velocity */
var float AccelRate;

var float TossZ;

/** This holds the stat's ID of the firing weapon */
var int FiringOwnerStatsID;
var int FiringWeaponStatsID;
var int FiringWeaponMode;

/** for console games (wider collision w/ enemy players) */
var bool bWideCheck;
var float CheckRadius;

/** Make true if want to spawn ProjectileLight.  Set false in TickSpecial() once it's been determined whether Instigator is local player.  Done there to make sure instigator has been replicated */
var bool bCheckProjectileLight;
/** Class of ProjectileLight */
var class<PointLightComponent> ProjectileLightClass;
/** LightComponent for this projectile (spawned only if projectile fired by the local player) */
var PointLightComponent ProjectileLight;

/** Class of ExplosionLight */
var class<UTExplosionLight> ExplosionLightClass;
/** Max distance to create ExplosionLight */
var float MaxExplosionLightDistance;

/** TerminalVelocity for this projectile when falling */
var float TerminalVelocity;
/** Water buoyancy. A ratio (1.0 = neutral buoyancy, 0.0 = no buoyancy) */
var float Buoyancy;

/** custom gravity multiplier */
var float CustomGravityScaling;

/** if this projectile is fired by a vehicle passenger gun, this is the base vehicle
 * considered the same as Instigator for purposes of bBlockedByInstigator
 */
var Vehicle InstigatorBaseVehicle;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual void GetNetBuoyancy(FLOAT &NetBuoyancy, FLOAT &NetFluidFriction);
	virtual FLOAT GetGravityZ();
	virtual UBOOL IgnoreBlockingBy(const AActor* Other) const;
	virtual INT* GetOptimizedRepList(BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
}

replication
{
	if (bNetInitial)
		bWideCheck;
	if (bNetDirty && bReplicateInstigator)
		InstigatorBaseVehicle;
}

/** returns terminal velocity (max speed while falling) for this actor.  Unless overridden, it returns the TerminalVelocity of the PhysicsVolume in which this actor is located.
*/
native function float GetTerminalVelocity();

/** CreateProjectileLight() called from TickSpecial() once if Instigator is local player
*/
simulated event CreateProjectileLight()
{
	if ( WorldInfo.bDropDetail )
		return;

	ProjectileLight = new(self) ProjectileLightClass;
	AttachComponent(ProjectileLight);
}

/**
 * Explode when the projectile comes to rest on the floor.  It's called from the native physics processing functions.  By default,
 * when we hit the floor, we just explode.
 */
simulated event Landed( vector HitNormal, actor FloorActor )
{
	HitWall(HitNormal, FloorActor, None);
}

/**
 * When this actor begins its life, play any ambient sounds attached to it
 */
simulated function PostBeginPlay()
{
	local AudioComponent AmbientComponent;
	local UTWeaponPawn WeaponPawn;

	if (Role == ROLE_Authority)
	{
		if (UTDummyPawn(Instigator) != None)
		{
			// dummy cinematic projectile, turn off net relevance
			RemoteRole = ROLE_None;
			bSuppressSounds = UTDummyPawn(Instigator).FireAction.bSuppressSounds;
		}

		WeaponPawn = UTWeaponPawn(Instigator);
		if (WeaponPawn != None)
		{
			InstigatorBaseVehicle = WeaponPawn.MyVehicle;
		}
		else if (Instigator != None && Instigator.DrivenVehicle != None)
		{
			InstigatorBaseVehicle = Instigator.DrivenVehicle;
		}

		// If on console, init wide check
		bWideCheck = bWideCheck ||  (Instigator != None && UTConsolePlayerController(Instigator.Controller) != None && CheckRadius > 0);
	}

	Super.PostBeginPlay();

	if ( bDeleteMe || bShuttingDown)
		return;

	// Set its Ambient Sound
	if (AmbientSound != None && WorldInfo.NetMode != NM_DedicatedServer && !bSuppressSounds)
	{
		if ( bImportantAmbientSound || !WorldInfo.bDropDetail )
		{
			AmbientComponent = CreateAudioComponent(AmbientSound, true, true);
			if ( AmbientComponent != None )
			{
				AmbientComponent.bShouldRemainActiveIfDropped = true;
			}
		}
	}

	// Spawn any effects needed for flight
	SpawnFlightEffects();
}

simulated event SetInitialState()
{
	bScriptInitialized = true;
	if (Role < ROLE_Authority && AccelRate != 0.f)
	{
		GotoState('WaitingForVelocity');
	}
	else
	{
		GotoState((InitialState != 'None') ? InitialState : 'Auto');
	}
}

/**
 * Initalize the Projectile
 */
function Init(vector Direction)
{
	SetRotation(rotator(Direction));

	Velocity = Speed * Direction;
	Velocity.Z += TossZ;
	Acceleration = AccelRate * Normal(Velocity);
}

/**
 *
 */
simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	if (DamageRadius > 0.0)
	{
		Explode( HitLocation, HitNormal );
	}
	else
	{
		Other.TakeDamage(Damage,InstigatorController,HitLocation,MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		Shutdown();
	}
}


/**
 * Explode this Projectile
 */
simulated function Explode(vector HitLocation, vector HitNormal)
{
	if (Damage>0 && DamageRadius>0)
	{
		ProjectileHurtRadius(Damage,DamageRadius, MyDamageType, MomentumTransfer, HitLocation, HitNormal );
	}
	SpawnExplosionEffects(HitLocation, HitNormal);

	if ( Role == ROLE_Authority )
		MakeNoise(1.0);

	ShutDown();
}

/**
 * Hurt locally authoritative actors within the radius.
 * Projectile version if needed offsets the start of the radius check to prevent hits embedded in walls from failing to cause damage
 */
simulated function bool ProjectileHurtRadius( float DamageAmount,
								    float InDamageRadius,
									class<DamageType> DamageType,
									float Momentum,
									vector HurtOrigin,
									vector HitNormal
									)
{
	local bool bCausedDamage, bInitializedAltOrigin, bFailedAltOrigin;
	local Actor	Victim;
	local vector AltOrigin;

	// Prevent HurtRadius() from being reentrant.
	if ( bHurtEntry )
		return false;

	bHurtEntry = true;
	bCausedDamage = false;

	// if ImpactedActor is set, we actually want to give it full damage, and then let him be ignored by super.HurtRadius()
	if ( (ImpactedActor != None) && (ImpactedActor != self)  )
	{
		ImpactedActor.TakeRadiusDamage(InstigatorController, DamageAmount, InDamageRadius, DamageType, Momentum, HurtOrigin, true, self);
		bCausedDamage = ImpactedActor.bProjTarget;
	}

	foreach CollidingActors( class'Actor', Victim, DamageRadius, HurtOrigin )
	{
		if ( !Victim.bWorldGeometry && (Victim != self) && (Victim != ImpactedActor) && (Victim.bProjTarget || (NavigationPoint(Victim) == None)) )
		{
			if ( !FastTrace(HurtOrigin, Victim.Location,, TRUE) )
			{
				// try out from wall, in case trace start was embedded
				if ( !bInitializedAltOrigin )
				{
					// initialize alternate trace start
					bInitializedAltOrigin = true;
					AltOrigin = HurtOrigin + class'UTPawn'.Default.MaxStepHeight * HitNormal;
					if ( !FastTrace(HurtOrigin, AltOrigin,, TRUE) )
					{
						if ( Velocity == vect(0,0,0) )
						{
							bFailedAltOrigin = true;
						}
						else
						{
							AltOrigin = HurtOrigin - class'UTPawn'.Default.MaxStepHeight * normal(Velocity);
							bFailedAltOrigin = !FastTrace(HurtOrigin, AltOrigin,, TRUE);
						}
					}
				}
				if ( bFailedAltOrigin || !FastTrace(AltOrigin, Victim.Location,, TRUE) )
				{
					continue;
				}
			}
			Victim.TakeRadiusDamage(InstigatorController, DamageAmount, DamageRadius, DamageType, Momentum, HurtOrigin, false, self);
			bCausedDamage = bCausedDamage || Victim.bProjTarget;
		}
	}
	bHurtEntry = false;

	if ( bCausedDamage )
	{
		UpdateHitStats(bCausedDamage);
		return true;
	}
	return false;
}

/**
 * Spawns any effects needed for the flight of this projectile
 */
simulated function SpawnFlightEffects()
{
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		ProjEffects = new(Outer) class'UTParticleSystemComponent';
		ProjEffects.bOverrideLODMethod = true;
		ProjEffects.LODMethod = PARTICLESYSTEMLODMETHOD_DirectSet;
		ProjEffects.SetTemplate(ProjFlightTemplate);
		ProjEffects.SetLODLevel(WorldInfo.bDropDetail ? 1 : 0);
		ProjEffects.OnSystemFinished = MyOnParticleSystemFinished;
		ProjEffects.bUpdateComponentInTick = true;
		AttachComponent(ProjEffects);
	}
}

simulated function bool CheckMaxEffectDistance(PlayerController P, vector SpawnLocation, optional float CullDistance)
{
	local float Dist;

	if ( P.ViewTarget == None )
		return true;

	if ( (Vector(P.Rotation) Dot (SpawnLocation - P.ViewTarget.Location)) < 0.0 )
	{
		return (VSize(P.ViewTarget.Location - SpawnLocation) < 1000);
	}

	Dist = VSize(SpawnLocation - P.ViewTarget.Location);

	if (CullDistance > 0 && CullDistance < Dist * P.LODDistanceFactor)
	{
		return false;
	}

	return true;
}

simulated function bool EffectIsRelevant(vector SpawnLocation, bool bForceDedicated, optional float CullDistance)
{
	local PlayerController PC;

	// always spawn effect if local player is close, even if not rendered recently
	if (WorldInfo.NetMode != NM_DedicatedServer && SpawnLocation == Location)
	{
		foreach LocalPlayerControllers(class'PlayerController', PC)
		{
			if (PC.ViewTarget != None && VSize(PC.ViewTarget.Location - Location) < 256.0)
			{
				return true;
			}
		}
	}

	return Super.EffectIsRelevant(SpawnLocation, bForceDedicated, CullDistance);
}

/** sets any additional particle parameters on the explosion effect required by subclasses */
simulated function SetExplosionEffectParameters(ParticleSystemComponent ProjExplosion);

/**
 * Spawn Explosion Effects
 */
simulated function SpawnExplosionEffects(vector HitLocation, vector HitNormal)
{
	local DecalLifetimeData LifetimeData;
	local vector LightLoc, LightHitLocation, LightHitNormal;
	local vector Direction;
	local ParticleSystemComponent ProjExplosion;

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (ProjectileLight != None)
		{
			DetachComponent(ProjectileLight);
			ProjectileLight = None;
		}
		if (ProjExplosionTemplate != None && EffectIsRelevant(Location, false, MaxEffectDistance))
		{
			if (!bAdvanceExplosionEffect)
			{
				ProjExplosion = WorldInfo.MyEmitterPool.SpawnEmitter(ProjExplosionTemplate, HitLocation, rotator(HitNormal), ImpactedActor);
			}
			else
			{
				Direction = normal(Velocity - 2.0 * HitNormal * (Velocity dot HitNormal)) * Vect(1,1,0);
				ProjExplosion = WorldInfo.MyEmitterPool.SpawnEmitter(ProjExplosionTemplate, HitLocation, rotator(Direction), ImpactedActor);
				ProjExplosion.SetVectorParameter('Velocity',Direction);
				ProjExplosion.SetVectorParameter('HitNormal',HitNormal);
			}
			SetExplosionEffectParameters(ProjExplosion);

			if (ExplosionLightClass != None && !WorldInfo.bDropDetail && ShouldSpawnExplosionLight(HitLocation, HitNormal) )
			{
				if (Trace(LightHitLocation, LightHitNormal, HitLocation + (0.25 * ExplosionLightClass.default.TimeShift[0].Radius * HitNormal), HitLocation, false) == None)
				{
					LightLoc = HitLocation + (0.25 * ExplosionLightClass.default.TimeShift[0].Radius * (vect(1,0,0) >> ProjExplosion.Rotation));
				}
				else
				{
					LightLoc = HitLocation + (0.5 * VSize(HitLocation - LightHitLocation) * (vect(1,0,0) >> ProjExplosion.Rotation));
				}

				UTEmitterPool(WorldInfo.MyEmitterPool).SpawnExplosionLight(ExplosionLightClass, LightLoc, ImpactedActor);
			}
		}

		if (ExplosionSound != None && !bSuppressSounds)
		{
			PlaySound(ExplosionSound, true);
		}

		if (ExplosionDecal != None && ImpactedActor != None)
		{
			// FIXME: decal lifespan based on detail level
			LifetimeData = new(ImpactedActor) class'DecalLifetimeDataAge';
			ImpactedActor.AddDecal(ExplosionDecal, HitLocation, rotator(-HitNormal), 0, DecalWidth, DecalHeight, 10.0, true, None, LifetimeData, true, false, '', -1, -1 );
		}

		bSuppressExplosionFX = true; // so we don't get called again
	}
}

/** ShouldSpawnExplosionLight()
Decide whether or not to create an explosion light for this explosion
*/
simulated function bool ShouldSpawnExplosionLight(vector HitLocation, vector HitNormal)
{
	local PlayerController P;
	local float Dist;

	// decide whether to spawn explosion light
	ForEach LocalPlayerControllers(class'PlayerController', P)
	{
		Dist = VSize(P.ViewTarget.Location - Location);
		if ( (P.Pawn == Instigator) || (Dist < ExplosionLightClass.Default.Radius) || ((Dist < MaxExplosionLightDistance) && ((vector(P.Rotation) dot (Location - P.ViewTarget.Location)) > 0)) )
		{
			return true;
		}
	}
	return false;
}

/**
 * Clean up
 */
simulated function Shutdown()
{
	local vector HitLocation, HitNormal;

	bShuttingDown=true;
	HitNormal = normal(Velocity * -1);
	Trace(HitLocation,HitNormal,(Location + (HitNormal*-32)), Location + (HitNormal*32),true,vect(0,0,0));

	SetPhysics(PHYS_None);

	if (ProjEffects!=None)
	{
		ProjEffects.DeactivateSystem();
	}

	if (WorldInfo.NetMode != NM_DedicatedServer && !bSuppressExplosionFX)
	{
		SpawnExplosionEffects(Location, HitNormal);
	}

	HideProjectile();
	SetCollision(false,false);

	// If we have to wait for effects, tweak the death conditions

	if (bWaitForEffects)
	{
		if (bNetTemporary)
		{
			if ( WorldInfo.NetMode == NM_DedicatedServer )
			{
				// We are on a dedicated server and not replicating anything nor do we have effects so destroy right away
				Destroy();
			}
			else
			{
				// We can't die right away but make sure we don't replicate to anyone
				RemoteRole = ROLE_None;
				// make sure we leave enough lifetime for the effect to play
				LifeSpan = FMax(LifeSpan, 2.0);
			}
		}
		else
		{
			bTearOff = true;
			if (WorldInfo.NetMode == NM_DedicatedServer)
			{
				LifeSpan = 2.0;
			}
			else
			{
				// make sure we leave enough lifetime for the effect to play
				LifeSpan = FMax(LifeSpan, 2.0);
			}
		}
	}
	else
	{
		Destroy();
	}
}

// If this actor

event TornOff()
{
	ShutDown();
	Super.TornOff();
}

/**
 * Hide any meshes/etc.
 */
simulated function HideProjectile()
{
	local MeshComponent ComponentIt;
	foreach ComponentList(class'MeshComponent',ComponentIt)
	{
		ComponentIt.SetHidden(true);
	}
}

simulated function Destroyed()
{
	// Final Failsafe check for explosion effect
	if (WorldInfo.NetMode != NM_DedicatedServer && !bSuppressExplosionFX)
	{
		SpawnExplosionEffects(Location, vector(Rotation) * -1);
	}

	super.Destroyed();
}

simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	if (bWaitForEffects && PSC == ProjEffects)
	{
		// it is not safe to destroy the actor here because other threads are doing stuff, so do it next tick
		LifeSpan = 0.01;
	}
}

/** state used only on the client for projectiles with AccelRate > 0 to wait for Velocity to be replicated so we can use it to set Acceleration
 *	the alternative would be to make Velocity repnotify in Actor.uc, but since many Actors (such as Pawns) change their
 *	velocity very frequently, that would have a greater performance hit
 */
state WaitingForVelocity
{
	simulated function Tick(float DeltaTime)
	{
		if (!IsZero(Velocity))
		{
			Acceleration = AccelRate * Normal(Velocity);
			GotoState((InitialState != 'None') ? InitialState : 'Auto');
		}
	}
}

/**
 * Track Stats
 */
function UpdateHitStats(bool bDirectHit)
{
	local UTGame GI;

	if (Instigator != None && FiringWeaponStatsID >= 0)
	{
		GI = UTGame(WorldInfo.Game);
		if (GI != none && GI.GameStats != None)
		{
			if (bDirectHit)
			{
				GI.GameStats.WeaponEvent(FiringOwnerStatsID,FiringWeaponStatsID, FiringWeaponMode, Instigator.PlayerReplicationInfo,'directhit');
			}
			else
			{
				GI.GameStats.WeaponEvent(FiringOwnerStatsID,FiringWeaponStatsID, FiringWeaponMode, Instigator.PlayerReplicationInfo,'hit');
			}
		}
	}
}

function InitStats(UTWeapon InstigatorWeapon)
{
	FiringOwnerStatsID = InstigatorWeapon.OwnerStatsID;
	FiringWeaponStatsID = InstigatorWeapon.WeaponStatsID;
	FiringWeaponMode = InstigatorWeapon.CurrentFireMode;
}

simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	out_CamLoc = Location + (CylinderComponent.CollisionHeight * Vect(0,0,1));
	return true;
}

function Actor GetHomingTarget(UTProjectile Seeker, Controller InstigatedBy) { return self; }

/** called when this Projectile is the ViewTarget of a local player
 * @return the Pawn to use for rendering HUD displays
 */
simulated function Pawn GetPawnOwner();

simulated function float GetTimeToLocation(vector TargetLoc)
{
	local float Dist, ProjTime, AccelTime, AccelDist;

	if (AccelRate == 0.0)
	{
		return (VSize(TargetLoc - Location) / Speed);
	}
	else
	{
		// figure out how long it would take if we accelerated the whole way
		Dist = VSize(TargetLoc - Location);
		ProjTime = (-Speed + Sqrt(Square(Speed) - (2.0 * AccelRate * -Dist))) / (1.0 * AccelRate);
		// figure out how long it will actually take to accelerate to max speed
		AccelTime = (MaxSpeed - Speed) / AccelRate;
		if (ProjTime > AccelTime)
		{
			// figure out distance traveled while accelerating to max speed
			AccelDist = (Speed * AccelTime) + (0.5 * AccelRate * Square(AccelTime));
			// add time to accelerate to max speed plus time to travel remaining dist at max speed
			ProjTime = AccelTime + ((Dist - AccelDist) / MaxSpeed);
		}
		return ProjTime;
	}
}

simulated static function float GetRange()
{
	local float AccelTime;

	if (default.LifeSpan == 0.0)
	{
		return 15000.0;
	}
	else if (default.AccelRate == 0.0)
	{
		return (default.Speed * default.LifeSpan);
	}
	else
	{
		AccelTime = (default.MaxSpeed - default.Speed) / default.AccelRate;
		if (AccelTime < default.LifeSpan)
		{
			return ((0.5 * default.AccelRate * AccelTime * AccelTime) + (default.Speed * AccelTime) + (default.MaxSpeed * (default.LifeSpan - AccelTime)));
		}
		else
		{
			return (0.5 * default.AccelRate * default.LifeSpan * default.LifeSpan) + (default.Speed * default.LifeSpan);
		}
	}
}

defaultproperties
{
	DamageRadius=+0.0
	TossZ=0.0
	bWaitForEffects=false
	MaxEffectDistance=+10000.0
	MaxExplosionLightDistance=+4000.0
	CheckRadius=0.0
	bBlockedByInstigator=false
	TerminalVelocity=3500.0
	bCollideComplex=true
	bSwitchToZeroCollision=true
	CustomGravityScaling=1.0

	Components.Remove(Sprite)
	bShuttingDown=false
}
