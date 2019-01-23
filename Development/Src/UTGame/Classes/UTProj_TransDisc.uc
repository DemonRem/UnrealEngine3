/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_TransDisc extends UTProjectile
	abstract;

/** How much "Integrity" does it have */
var float Integrity;

/** Who disrupted it */
var controller DisruptedController;

/** Effect when it's broken */
var ParticleSystemComponent DisruptedEffect;
var SoundCue DisruptedSound;
var AudioComponent DisruptedLoop;

/** Create a linked list of all beacons in the level */
var UTProj_TransDisc NextBeacon;

var Actor TranslocationTarget;	// for AI
var bool bNoAI;


/** particle system played when the disc bounces off something */
var ParticleSystem BounceTemplate;

var SoundCue BounceSound;

/** When the disk has landed this system starts running*/
var ParticleSystemComponent	LandEffects;

var UTWeap_Translocator MyTranslocator;
enum ETLState
{
	TLS_None,
	TLS_InAir,
	TLS_OnGround,
	TLS_Disrupted
};

var repnotify ETLState TLState;

replication
{
	if (bNetDirty)
		TLState;
	if ( bNetInitial && PlayerController(InstigatorController) != None &&
	WorldInfo.ReplicationViewers.Find('InViewer', PlayerController(InstigatorController)) != INDEX_NONE )
		MyTranslocator;
}

simulated function PostBeginPlay()
{
	local UTGame G;

	Super.PostBeginPlay();
	if ( !bDeleteMe )
	{
		// add to beacon list
		G = UTGame(WorldInfo.Game);
		if ( G == None )
			return;
		NextBeacon = G.BeaconList;
		G.BeaconList = self;
	}
}

simulated function Destroyed()
{
	local UTGame G;
	local UTProj_TransDisc T;

	Super.Destroyed();

	if (ROLE==ROLE_Authority)
	{
		G = UTGame(WorldInfo.Game);
		if ( G == None )
			return;

		// remove from beacon list
		if ( G.BeaconList == self )
			G.BeaconList = NextBeacon;
		else
		{
			for ( T=G.BeaconList; T!=None; T=T.NextBeacon )
			{
				if ( T.NextBeacon == self )
				{
					T.NextBeacon = NextBeacon;
					return;
				}
			}
		}
	}

	if (DisruptedEffect != none)
	{
		DisruptedEffect.SetHidden(true);
		DisruptedLoop.Stop();
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'TLState'  )
	{
		switch (TLState)
		{
			case TLS_InAir:

				SpawnTrail();
				break;

			case TLS_OnGround:

				SpawnBounceEffect(vector(Rotation));
				break;

			case TLS_Disrupted:
				DisruptedEffect.SetHidden(false);
				PlaySound(DisruptedSound);
				DisruptedLoop.FadeIn(0.2,1.0);
				break;
		}
	}
	else
		super.ReplicatedEvent(VarName);
}

function bool Disrupted()
{
	return Integrity <= 0 && !bDeleteMe;
}

/**
 * When this disc lands, give it some bounce.
 */
simulated event Landed( vector HitNormal, actor FloorActor )
{
	HitWall(HitNormal, FloorActor, None);
}

simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	return;
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	bBlockedByInstigator = true;
	Velocity = 0.3*(( Velocity dot HitNormal ) * HitNormal * (-2.0) + Velocity);   // Reflect off Wall w/damping
	Speed = VSize(Velocity);

	if ( Speed < 20 && Wall.bWorldGeometry && (HitNormal.Z >= 0.7) )
	{
		bBounce = false;
		bWaitForEffects = false;
		SetPhysics(PHYS_None);
		SetRotation(rotator(HitNormal) + rot(16384,0,0));

		TLState = TLS_OnGround;
		if (ProjEffects!=None)
		{
			ProjEffects.DeactivateSystem();
		}
		if(LandEffects != none && !LandEffects.bIsActive)
		{
			LandEffects.ActivateSystem();
		}
	}
	SpawnBounceEffect(HitNormal);
}

simulated function SpawnBounceEffect(vector HitNormal)
{
	if (EffectIsRelevant(Location, false, MaxEffectDistance))
	{
		WorldInfo.MyEmitterPool.SpawnEmitter(BounceTemplate, Location, rotator(HitNormal) + rot(16384,0,0));
		PlaySound(BounceSound, true);
	}
}

/**
 * Unlike most projectiles, when the TransDisc hits a pawn, have it bounce like it was a wall
 */
simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	local UTWeap_ImpactHammer Hammer;
	local vector X,Y,Z, Dir;
	local UTPawn P;

	if ( Other == Instigator && Physics == PHYS_None )
	{
		Shutdown();
	}
	else if ( Other != Instigator )
	{
		// If the player has a hammer up and it's charged, and they are looking at the player, kill them
		P = UTPawn(Other);
		if ( P != none && P.Controller != none && P.Weapon != none )
		{
			Hammer = UTWeap_ImpactHammer(P.Weapon);
			if ( Hammer != none && Hammer.IsInState('WeaponChargeUp') )
			{
				GetAxes( Hammer.GetAdjustedAim( P.GetWeaponStartTraceLocation() ),X,Y,Z);
				Dir = HitLocation - P.Location;

				if ( Normal(Dir) dot Normal(x) > 0.7 )
				{
					TakeDamage(Integrity, P.Controller, HitLocation, vect(0,0,0), Hammer.InstantHitDamageTypes[0],, Hammer);
					Hammer.ClientEndFire(0);
					Hammer.EndFire(0);
				}
			}
		}


		if (Pawn(Other) != None && Vehicle(Other) == None && (VSize2D(Velocity) < 200 || Pawn(Other).Health <= 0))
		{
			return;
		}
		HitWall( -Normal(Velocity), Other, None );
	}
}

/**
 * Called from the UTWeap_Translocator to recall this disc
 */
simulated function Recall()
{
	// FIXME: Handle any effects here
	Shutdown();
}

event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local UTBot B;

	// No Team Damage
	if ( InstigatorController == None ||
		(EventInstigator != InstigatorController && (!WorldInfo.Game.bTeamGame || !WorldInfo.GRI.OnSameTeam(InstigatorController, EventInstigator))) )
	{
		Integrity -= DamageAmount;

		DisruptedController = EventInstigator;

		if ( Disrupted() && Role==ROLE_Authority )
		{
			TLState = TLS_Disrupted;
			if (WorldInfo.NetMode != NM_DedicatedServer)
			{
				PlaySound(DisruptedSound);
				DisruptedLoop.FadeIn(0.2,1.0);
				DisruptedEffect.SetHidden(false);
			}
			B = UTBot(Instigator.Controller);
			if (B != None)
			{
				B.bHasTranslocator = false;
			}
		}
	}
}


// AI Interface
function SetTranslocationTarget(actor T)
{
	TranslocationTarget = T;
	GotoState('MonitoringThrow');
}

function bool IsMonitoring(actor A)
{
	return false;
}

function EndMonitoring();

State MonitoringThrow
{
	function bool IsMonitoring(actor A)
	{
		return ( A == TranslocationTarget );
	}

	simulated function Destroyed()
	{
		local UTBot B;

		B = UTBot(Instigator.Controller);
		if ( B != None )
		{
			B.TranslocationTarget = None;
			B.bPreparingMove = false;
			B.SwitchToBestWeapon();
		}
		Global.Destroyed();
	}

	function EndMonitoring()
	{
		if (!bDeleteMe && !bPendingDelete && Instigator != None && UTWeap_Translocator(Instigator.Weapon) != None)
		{
			Recall();
		}
		GotoState('');
	}

	function EndState(Name NextStateName)
	{
		local UTBot B;

		B = UTBot(Instigator.Controller);
		if ( (B != None) && !bNoAI )
		{
			B.TranslocationTarget = None;
			B.bPreparingMove = false;
			B.SwitchToBestWeapon();
		}
	}

	simulated function HitWall( vector HitNormal, actor Wall, PrimitiveComponent WallComp )
	{
		Global.HitWall(HitNormal,Wall, WallComp);
		if ( (UTCarriedObject(TranslocationTarget) != None) && (HitNormal.Z > 0.7)
				&& (VSize(Location - TranslocationTarget.Location) < FMin(400, VSize(Instigator.Location - TranslocationTarget.Location))) )
		{
			Instigator.Controller.MoveTarget = TranslocationTarget;
			BotTranslocate();
			return;
		}

		if ( Physics == PHYS_None )
		{
			if ( (UTBot(Instigator.Controller) != None) && UTBot(Instigator.Controller).bPreparingMove )
			{
				UTBot(Instigator.Controller).MoveTimer = -1;
			}
			EndMonitoring();
		}
	}

	function BotTranslocate()
	{
		if (MyTranslocator != None)
		{
			MyTranslocator.CustomFire();
		}
		EndMonitoring();
	}

	simulated event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
	{
		local Pawn P;
		local vector TTargetLoc, AdjustedLoc;
		local UTBot B;

		Global.Touch(Other, OtherComp, Location, HitNormal);

		if (Role == ROLE_Authority)
		{
			P = Pawn(Other);
			if (P != None && P != Instigator)
			{
				B = UTBot(Instigator.Controller);
				if (B == None)
				{
					EndMonitoring();
				}
				else if (WorldInfo.GRI.OnSameTeam(P.Controller, Instigator))
				{
					// if pawn we hit is covering destination, translocate anyway
					TTargetLoc = B.GetTranslocationDestination();
					if (VSize2D(P.Location - TTargetLoc) < P.GetCollisionRadius() + 50.0 && Location.Z - TTargetLoc.Z > -45.0)
					{
						// move out a bit to avoid telefragging teammate
						SetCollision(false, false);
						AdjustedLoc = HitLocation;
						AdjustedLoc.Z = P.Location.Z;
						SetLocation(P.Location + Normal(AdjustedLoc - P.Location) * (P.GetCollisionRadius() + Instigator.GetCollisionRadius() + 10.0));

						BotTranslocate();
					}
				}
				else if (UTBot(P.Controller).ProficientWithWeapon() && (2.0 + FRand() * 8.0 < UTBot(P.Controller).Skill))
				{
					BotTranslocate();
				}
			}
		}
	}

	// FIXME - consider making this a timer or projectile latent function instead of using tick?
	function Tick(float DeltaTime)
	{
		local vector Dist, Dir, HitLocation, HitNormal, TTargetLoc;
		local float ZDiff, Dist2D;
		local Actor HitActor;
		local UTBot B;

		B = UTBot(Instigator.Controller);
		if ( B == None || TranslocationTarget == None || !B.Squad.AllowTranslocationBy(B)
			|| ((UTCarriedObject(Instigator.Controller.MoveTarget) != None) && (Instigator.Controller.MoveTarget != TranslocationTarget))
			|| !B.ValidTranslocationTarget(TranslocationTarget) )
		{
			EndMonitoring();
			return;
		}

		TTargetLoc = B.GetTranslocationDestination();
		Dist = Location - TTargetLoc;
		ZDiff = Dist.Z;
		Dist.Z = 0;
		Dir = TTargetLoc - Instigator.Location;
		Dir.Z = 0;
		Dist2D = VSize2D(Dist);
		if ( Dist2D < 50 )
		{
			if ( ZDiff > -45 )
			{
				Instigator.Controller.MoveTarget = TranslocationTarget;
				BotTranslocate();
			}
			return;
		}
		if ( (Dist Dot Dir) > 0 )
		{
			if (B.bPreparingMove)
			{
				B.MoveTimer = -1;
			}
			else if ( (UTCarriedObject(TranslocationTarget) != None) && (ZDiff > 0) && (Dist2D < FMin(400, VSize(Instigator.Location - TTargetLoc) - 250)) )
			{
				// if safe underneath, then translocate
				HitActor = Trace(HitLocation, HitNormal, Location - vect(0,0,100), Location, false);
				if ( (HitActor != None) && (HitNormal.Z > 0.7) )
				{
					Instigator.Controller.MoveTarget = TranslocationTarget;
					BotTranslocate();
					return;
				}
			}
			EndMonitoring();
			return;
		}
	}
}
// END AI interface

simulated function SpawnFlightEffects()
{
	// on client use repnotify flag to spawn effect
	if (WorldInfo.Netmode != NM_Client)
	{
		// no effect on server
		TLState = TLS_InAir;
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			SpawnTrail();
		}
	}
}

simulated function SpawnTrail()
{
	ProjEffects = new(Outer) class'UTParticleSystemComponent';
	ProjEffects.SetTemplate(ProjFlightTemplate);
	AttachComponent(ProjEffects);
}

simulated function ShutDown()
{
	if(MyTranslocator != none)
	{
		MyTranslocator.WeaponIdleAnims[0] = MyTranslocator.default.WeaponIdleAnims[0];
		MyTranslocator.PlayWeaponAnimation(MyTranslocator.WeaponIdleAnims[0],0.001);
	}
	super.ShutDown();
}

defaultproperties
{
	speed=1330.000000
	Buoyancy=+0.6
	Damage=0
	MomentumTransfer=50000
	MyDamageType=class'UTDmgType_Telefrag'
	LifeSpan=0.0
	bCollideWorld=true
	bProjTarget=True
	TossZ=+155.0
	bBounce=true
	bNetTemporary=false
	Integrity=2
	Physics=PHYS_Falling
	bSwitchToZeroCollision=false

	Begin Object Name=CollisionCylinder
		CollisionRadius=10
		CollisionHeight=2
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		CollideActors=true
	End Object

	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		CastShadow=true
		bAcceptsLights=true
		Translation=(Z=2)
		CollideActors=false
		CullDistance=7500
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)

}
