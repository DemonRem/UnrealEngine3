/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class KActor extends DynamicSMActor
	native(Physics)
	nativereplication
	placeable
	showcategories(Navigation);

cpptext
{
	// UObject interface
	virtual void PostLoad();

	// AActor interface
	virtual void physRigidBody(FLOAT DeltaTime);
	virtual INT* GetOptimizedRepList(BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
	virtual void OnRigidBodyCollision(const FRigidBodyCollisionInfo& Info0, const FRigidBodyCollisionInfo& Info1, const FCollisionImpactData& RigidCollisionData);
	UBOOL ShouldTrace(UPrimitiveComponent* Primitive, AActor *SourceActor, DWORD TraceFlags);

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
}

var()	bool	bDamageAppliesImpulse;
var() repnotify bool bWakeOnLevelStart;

// Impact effects
var				ParticleSystemComponent		ImpactEffectComponent;
var				AudioComponent				ImpactSoundComponent;
var				AudioComponent				ImpactSoundComponent2; // @TODO: This could be turned into a dynamic array; but for the moment just 2 will do.
var				float						LastImpactTime;

// Slide effects
var				ParticleSystemComponent		SlideEffectComponent;
var				AudioComponent				SlideSoundComponent;
var				bool						bCurrentSlide;
var				bool						bSlideActive;
var				float						LastSlideTime;


var native const RigidBodyState RBState;
var	native const float			AngErrorAccumulator;
/** replicated components of DrawScale3D */
var repnotify float DrawScaleX, DrawScaleY, DrawScaleZ;

var vector InitialLocation;
var rotator InitialRotation;

replication
{
	if ( Role == ROLE_Authority )
		RBState;
	if (bNetInitial && Role == ROLE_Authority)
		bWakeOnLevelStart, DrawScaleX, DrawScaleY, DrawScaleZ;
}

/** Util for getting the PhysicalMaterial applied to this KActor's StaticMesh. */
native final function PhysicalMaterial GetKActorPhysMaterial();

/** Forces the resolve the RBState regardless of wether the actor is sleeping */
native final function ResolveRBState();

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if (bWakeOnLevelStart)
	{
		StaticMeshComponent.WakeRigidBody();
	}
	DrawScaleX = DrawScale3D.X;
	DrawScaleY = DrawScale3D.Y;
	DrawScaleZ = DrawScale3D.Z;

	// Initialise impact/slide components (if we are being notified of physics events, and have sounds/effects set up
	// in PhysicalMaterial applied to our static mesh.
	if(StaticMeshComponent.bNotifyRigidBodyCollision)
	{
		SetPhysicalCollisionProperties();
	}

	InitialLocation = Location;
	InitialRotation = Rotation;
}

/** called when the actor falls out of the world 'safely' (below KillZ and such) */
simulated event FellOutOfWorld(class<DamageType> dmgType)
{
	ShutDown();
	Super.FellOutOfWorld(dmgType);
}

simulated function SetPhysicalCollisionProperties()
{
	local PhysicalMaterial PhysMat;
	PhysMat = GetKActorPhysMaterial();
	if(PhysMat.ImpactEffect != None)
	{
		ImpactEffectComponent = new(Outer) class'ParticleSystemComponent';
		AttachComponent(ImpactEffectComponent);
		ImpactEffectComponent.bAutoActivate = FALSE;
		ImpactEffectComponent.SetTemplate(PhysMat.ImpactEffect);
	}

	if(PhysMat.ImpactSound != None)
	{
		ImpactSoundComponent = new(Outer) class'AudioComponent';
		ImpactSoundComponent.bAutoDestroy = TRUE;
		AttachComponent(ImpactSoundComponent);
		ImpactSoundComponent.SoundCue = PhysMat.ImpactSound;

		ImpactSoundComponent2 = new(Outer) class'AudioComponent';
		ImpactSoundComponent2.bAutoDestroy = TRUE;
		AttachComponent(ImpactSoundComponent2);
		ImpactSoundComponent2.SoundCue = PhysMat.ImpactSound;
	}

	if(PhysMat.SlideEffect != None)
	{
		SlideEffectComponent = new(Outer) class'ParticleSystemComponent';
		AttachComponent(SlideEffectComponent);
		SlideEffectComponent.bAutoActivate = FALSE;
		SlideEffectComponent.SetTemplate(PhysMat.SlideEffect);
	}

	if(PhysMat.SlideSound != None)
	{
		SlideSoundComponent = new(Outer) class'AudioComponent';
		SlideSoundComponent.bAutoDestroy = TRUE;
		AttachComponent(SlideSoundComponent);
		SlideSoundComponent.SoundCue = PhysMat.SlideSound;
	}
}
simulated event ReplicatedEvent(name VarName)
{
	local vector NewDrawScale3D;

	if (VarName == 'bWakeOnLevelStart')
	{
		if (bWakeOnLevelStart)
		{
			StaticMeshComponent.WakeRigidBody();
		}
	}
	else if (VarName == 'DrawScaleX' || VarName == 'DrawScaleY' || VarName == 'DrawScaleZ')
	{
		NewDrawScale3D.X = DrawScaleX;
		NewDrawScale3D.Y = DrawScaleY;
		NewDrawScale3D.Z = DrawScaleZ;
		SetDrawScale3D(NewDrawScale3D);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function ApplyImpulse( Vector ImpulseDir, float ImpulseMag, Vector HitLocation, optional TraceHitInfo HitInfo )
{
	local vector ApplyImpulse;

	ImpulseDir = Normal(ImpulseDir);
	ApplyImpulse = ImpulseDir * ImpulseMag;

	if( HitInfo.HitComponent != None )
	{
		HitInfo.HitComponent.AddImpulse( ApplyImpulse, HitLocation, HitInfo.BoneName );
	}
	else
	{	// if no HitComponent is passed, default to our CollisionComponent
		CollisionComponent.AddImpulse( ApplyImpulse, HitLocation );
	}
}

/**
 * Default behaviour when shot is to apply an impulse and kick the KActor.
 */
event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local vector ApplyImpulse;

	// call Actor's version to handle any SeqEvent_TakeDamage for scripting
	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

	if ( bDamageAppliesImpulse && damageType.default.KDamageImpulse > 0 )
	{
		if ( VSize(momentum) < 0.001 )
		{
			`Log("Zero momentum to KActor.TakeDamage");
			return;
		}

		ApplyImpulse = Normal(momentum) * damageType.default.KDamageImpulse;
		if ( HitInfo.HitComponent != None )
		{
			HitInfo.HitComponent.AddImpulse(ApplyImpulse, HitLocation, HitInfo.BoneName);
		}
		else
		{	// if no HitComponent is passed, default to our CollisionComponent
			CollisionComponent.AddImpulse(ApplyImpulse, HitLocation);
		}
	}
}

/**
 * Respond to radial damage as well.
 */
simulated function TakeRadiusDamage
(
	Controller			InstigatedBy,
	float				BaseDamage,
	float				DamageRadius,
	class<DamageType>	DamageType,
	float				Momentum,
	vector				HurtOrigin,
	bool				bFullDamage,
	Actor DamageCauser
)
{
	local int Idx;
	local SeqEvent_TakeDamage DmgEvt;
	// search for any damage events
	for (Idx = 0; Idx < GeneratedEvents.Length; Idx++)
	{
		DmgEvt = SeqEvent_TakeDamage(GeneratedEvents[Idx]);
		if (DmgEvt != None)
		{
			// notify the event of the damage received
			DmgEvt.HandleDamage(self, InstigatedBy, DamageType, BaseDamage);
		}
	}
	if ( bDamageAppliesImpulse && damageType.default.RadialDamageImpulse > 0 && (Role == ROLE_Authority) )
	{
		CollisionComponent.AddRadialImpulse(HurtOrigin, DamageRadius, damageType.default.RadialDamageImpulse, RIF_Linear, damageType.default.bRadialDamageVelChange);
	}
}

/** If this KActor receives a Toggle ON event from Kismet, wake the physics up. */
simulated function OnToggle(SeqAct_Toggle action)
{
	if(action.InputLinks[0].bHasImpulse)
	{
		StaticMeshComponent.WakeRigidBody();
	}
}

/**
 * Called upon receiving a SeqAct_Teleport action.  Grabs
 * the first destination available and attempts to teleport
 * this actor.
 *
 * @param	inAction - teleport action that was activated
 */
simulated function OnTeleport(SeqAct_Teleport inAction)
{
	local array<Object> objVars;
	local int idx;
	local Actor destActor;

	// find the first supplied actor
	inAction.GetObjectVars(objVars,"Destination");
	for (idx = 0; idx < objVars.Length && destActor == None; idx++)
	{
		destActor = Actor(objVars[idx]);
	}

	// and set to that actor's location
	if (destActor != None)
	{
		StaticMeshComponent.SetRBPosition(destActor.Location);
		StaticMeshComponent.SetRBRotation(destActor.Rotation);
		PlayTeleportEffect(false, true);
	}
}

simulated function Reset()
{
	StaticMeshComponent.SetRBLinearVelocity( Vect(0,0,0) );
	StaticMeshComponent.SetRBAngularVelocity( Vect(0,0,0) );
	StaticMeshComponent.SetRBPosition( InitialLocation );
	StaticMeshComponent.SetRBRotation( InitialRotation );

	if (!bWakeOnLevelStart)
	{
		StaticMeshComponent.PutRigidBodyToSleep();
	}
	else
	{
		StaticMeshComponent.WakeRigidBody();
	}

	// Resolve the RBState and get all of the needed flags set
	ResolveRBState();

	// Force replication
	NetUpdateTime = WorldInfo.TimeSeconds - 1;

	super.Reset();
}

defaultproperties
{
	TickGroup=TG_PostAsyncWork

	SupportedEvents.Add(class'SeqEvent_RigidBodyCollision')

	Begin Object Name=StaticMeshComponent0
		WireframeColor=(R=0,G=255,B=128,A=255)
		BlockRigidBody=true
		RBChannel=RBCC_GameplayPhysics
		RBCollideWithChannels=(Default=TRUE,GameplayPhysics=TRUE,EffectPhysics=TRUE)
	End Object

	bDamageAppliesImpulse=true
	bNetInitialRotation=true
	Physics=PHYS_RigidBody
	bStatic=false
	bCollideWorld=false
	bProjTarget=true
	bBlockActors=true
	bWorldGeometry=false

	bNoDelete=true
	bAlwaysRelevant=true
	bSkipActorPropertyReplication=false
	bUpdateSimulatedPosition=true
	bReplicateMovement=true
	RemoteRole=ROLE_SimulatedProxy

	bCollideActors=true
	bNoEncroachCheck=true
	bBlocksTeleport=true
	bBlocksNavigation=true
	bPawnCanBaseOn=false
	bSafeBaseIfAsleep=TRUE
}
