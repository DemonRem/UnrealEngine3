/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class KAsset extends Actor
	native(Physics)
	nativereplication
	placeable;

var() const editconst SkeletalMeshComponent	SkeletalMeshComponent;

var()		bool		bDamageAppliesImpulse;
var()		bool		bWakeOnLevelStart;

/** Whether this KAsset should block Pawns. */
var()		bool		bBlockPawns;

/** Used to replicate mesh to clients */
var repnotify SkeletalMesh ReplicatedMesh;

/** Replicated hit impulse, for client side effects */
var RepNotify ReplicatedHitImpulse RepHitImpulse;

/** Previously received impulse count */
var byte OldImpulseCount;

/**
 *	Data read/written by the GetPhysicsState/SetPhysicsState functions.
 *	Static array because we can't replicate dynamic arrays.
 */
var RigidBodyState RigidBodyStates[16];

var bool bTempRep; //@TODO FIXMESTEVE remove (make sure rephitimpulse replication still works)

cpptext
{
	virtual INT* GetOptimizedRepList(BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
}

replication
{
	if ( Role == ROLE_Authority)
		ReplicatedMesh, RepHitImpulse, bTempRep;
}

/**
 *	Reads from any physics going on into the RigidBodyStates array.
 *	Will set the bNewData flag on any structs that read data in.
 */
native function GetAssetPhysicsState();

/**
 *	Takes any entries in the RigidBodyStates array which have bNewData set to true and
 *	pushes that data into the physics.
 *	@param bWakeBodies	Ensure bodies that are having new state applied are awake.
 */
native function SetAssetPhysicsState(bool bWakeBodies);

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if (bWakeOnLevelStart)
	{
		SkeletalMeshComponent.WakeRigidBody();
	}
	ReplicatedMesh = SkeletalMeshComponent.SkeletalMesh;
}

simulated event ReplicatedEvent( name VarName )
{
	if (VarName == 'ReplicatedMesh')
	{
		SkeletalMeshComponent.SetSkeletalMesh(ReplicatedMesh);
	}
	else if ( VarName == 'RepHitImpulse')
	{
		ClientApplyImpulse();
	}
}

/**
 * Apply hit impulse on client side.  Note that client may miss some impulses
 */
simulated function ClientApplyImpulse()
{
	local int i;
	
	if ( RepHitImpulse.bRadialImpulse )
	{
		CollisionComponent.AddRadialImpulse(RepHitImpulse.HitLocation, RepHitImpulse.AppliedImpulse.X, RepHitImpulse.AppliedImpulse.Y, RIF_Linear);
	}
	else
	{
		if ( OldImpulseCount < RepHitImpulse.ImpulseCount )
		{
			for ( i=OldImpulseCount; i<RepHitImpulse.ImpulseCount; i++ )
			{
				CollisionComponent.AddImpulse(RepHitImpulse.AppliedImpulse, RepHitImpulse.HitLocation, RepHitImpulse.BoneName);
			}	
		}
		else
			CollisionComponent.AddImpulse(RepHitImpulse.AppliedImpulse, RepHitImpulse.HitLocation, RepHitImpulse.BoneName);
	}
	OldImpulseCount = RepHitImpulse.ImpulseCount;
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

		// Make sure we have a valid TraceHitInfo with our SkeletalMesh
		// we need a bone to apply proper impulse
		CheckHitInfo( HitInfo, SkeletalMeshComponent, Normal(Momentum), hitlocation );

		ApplyImpulse = Normal(momentum) * damageType.default.KDamageImpulse;
		if ( HitInfo.HitComponent != None )
		{
			HitInfo.HitComponent.AddImpulse(ApplyImpulse, HitLocation, HitInfo.BoneName);
			RepHitImpulse.AppliedImpulse = ApplyImpulse;
			RepHitImpulse.HitLocation = ( RepHitImpulse.HitLocation == HitLocation ) ? HitLocation + vect(0,0,1) : HitLocation;
			RepHitImpulse.BoneName = HitInfo.BoneName;
			RepHitImpulse.bRadialImpulse = false;
			RepHitImpulse.ImpulseCount++;
			bNetDirty = true;  // struct update doesn't properly update bNetDirty
			bTempRep = !bTempRep;
		}
	}
}

/**
 * Take Radius Damage
 *
 * @param	InstigatedBy, instigator of the damage
 * @param	Base Damage
 * @param	Damage Radius (from Origin)
 * @param	DamageType class
 * @param	Momentum (float)
 * @param	HurtOrigin, origin of the damage radius.
 * @param DamageCauser the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
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
	if ( bDamageAppliesImpulse && damageType.default.RadialDamageImpulse > 0 && (Role == ROLE_Authority) )
	{
		CollisionComponent.AddRadialImpulse(HurtOrigin, DamageRadius, damageType.default.RadialDamageImpulse, RIF_Linear, damageType.default.bRadialDamageVelChange);
		RepHitImpulse.AppliedImpulse.X = DamageRadius;
		RepHitImpulse.AppliedImpulse.Y = damageType.default.KDamageImpulse;
		RepHitImpulse.AppliedImpulse.Z = 0;
		RepHitImpulse.HitLocation = ( RepHitImpulse.HitLocation == HurtOrigin ) ? HurtOrigin + vect(0,0,1) : HurtOrigin;
		RepHitImpulse.BoneName = '';
		RepHitImpulse.bRadialImpulse = true;
		bNetDirty = true;  // struct update doesn't properly update bNetDirty
		bTempRep = !bTempRep;
	}
}

/** If this KAsset receives a Toggle ON event from Kismet, wake the physics up. */
simulated function OnToggle(SeqAct_Toggle action)
{
	if(action.InputLinks[0].bHasImpulse)
	{
		SkeletalMeshComponent.WakeRigidBody();
	}
}

simulated function OnTeleport(SeqAct_Teleport InAction)
{
	local Actor DestActor;

	DestActor = Actor(SeqVar_Object(InAction.VariableLinks[1].LinkedVariables[0]).GetObjectValue());
	if (DestActor != None)
	{
		SkeletalMeshComponent.SetRBPosition(DestActor.Location);
	}
	else
	{
		InAction.ScriptLog("No Destination for" @ InAction @ "on" @ self);
	}
}


/** Performs actual attachment. Can be subclassed for class specific behaviors. */
function DoKismetAttachment( Actor Attachment, SeqAct_AttachToActor Action )
{
	Attachment.SetBase( Self,, SkeletalMeshComponent, Action.BoneName );
}

cpptext
{
public:
	// UObject interface
	virtual void PostLoad();

	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();

	UBOOL ShouldTrace(UPrimitiveComponent* Primitive, AActor *SourceActor, DWORD TraceFlags);
	UBOOL IgnoreBlockingBy(const AActor* Other);
}

defaultproperties
{
	TickGroup=TG_PostAsyncWork

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
	End Object
	Components.Add(MyLightEnvironment)

	Begin Object Class=SkeletalMeshComponent Name=KAssetSkelMeshComponent
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=true
		BlockNonZeroExtent=false
		BlockRigidBody=true
		bHasPhysicsAssetInstance=true
		bUpdateKinematicBonesFromAnimation=false
		PhysicsWeight=1.0
		RBChannel=RBCC_GameplayPhysics
		RBCollideWithChannels=(Default=TRUE,GameplayPhysics=TRUE,EffectPhysics=TRUE)
		LightEnvironment=MyLightEnvironment
	End Object
	CollisionComponent=KAssetSkelMeshComponent
	SkeletalMeshComponent=KAssetSkelMeshComponent
	Components.Add(KAssetSkelMeshComponent)

	SupportedEvents.Add(class'SeqEvent_ConstraintBroken')
	SupportedEvents.Add(class'SeqEvent_RigidBodyCollision')

	bDamageAppliesImpulse=true
	bNetInitialRotation=true
	Physics=PHYS_RigidBody
	bEdShouldSnap=true
	bStatic=false
	bCollideActors=true
	bBlockActors=true
	bWorldGeometry=false
	bCollideWorld=false
	bProjTarget=true

	bNoDelete=true
	bAlwaysRelevant=true
	bSkipActorPropertyReplication=false
	bUpdateSimulatedPosition=true
	bReplicateMovement=true
	RemoteRole=ROLE_SimulatedProxy
	bReplicateRigidBodyLocation=true
}
