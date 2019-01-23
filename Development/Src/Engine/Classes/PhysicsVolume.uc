//=============================================================================
// PhysicsVolume:  a bounding volume which affects actor physics
// Each Actor is affected at any time by one PhysicsVolume
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class PhysicsVolume extends Volume
	native
	nativereplication
	placeable;

var()		interp vector	ZoneVelocity;

var()		float		GroundFriction;
var()		float		TerminalVelocity;
var()		float		DamagePerSec;
var() class<DamageType>	DamageType;
var()		int			Priority;	// determines which PhysicsVolume takes precedence if they overlap
var() float  FluidFriction;

var()		bool	bPainCausing;			// Zone causes pain.
var			bool	BACKUP_bPainCausing;
var()		bool	bDestructive;			// Destroys most actors which enter it.
var()		bool	bNoInventory;
var()		bool	bMoveProjectiles;		// this velocity zone should impart velocity to projectiles and effects
var()		bool	bBounceVelocity;		// this velocity zone should bounce actors that land in it
var()		bool	bNeutralZone;			// Players can't take damage in this zone.

/**
 *	By default, the origin of an Actor must be inside a PhysicsVolume for it to affect it.
 *	If this flag is true though, if this Actor touches the volume at all, it will affect it.
 */
var()		bool	bPhysicsOnContact;

/**
 *	This controls the force that will be applied to PHYS_RigidBody objects in this volume to get them
 *	to match the ZoneVelocity.
 */
var()		float	RigidBodyDamping;

/** Applies a cap on the maximum damping force that is applied to objects. */
var()		float	MaxDampingForce;

var			bool	bWaterVolume;
var	Info PainTimer;

var PhysicsVolume NextPhysicsVolume;

cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	void SetZone( UBOOL bTest, UBOOL bForceRefresh );
	virtual UBOOL ShouldTrace(UPrimitiveComponent* Primitive,AActor *SourceActor, DWORD TraceFlags);
	virtual UBOOL WillHurt(APawn *P);
	virtual void CheckForErrors();

	virtual FLOAT GetVolumeRBGravityZ() { return GetGravityZ(); }
}

native function float GetGravityZ();

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	BACKUP_bPainCausing	= bPainCausing;

	if ( Role < ROLE_Authority )
		return;
	if ( bPainCausing )
		PainTimer = Spawn(class'VolumeTimer', self);
}

/* Reset() - reset actor to initial state - used when restarting level without reloading. */
function Reset()
{
	bPainCausing	= BACKUP_bPainCausing;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
}

/* Called when an actor in this PhysicsVolume changes its physics mode
*/
event PhysicsChangedFor(Actor Other);

event ActorEnteredVolume(Actor Other);
event ActorLeavingVolume(Actor Other);

event PawnEnteredVolume(Pawn Other);
event PawnLeavingVolume(Pawn Other);

simulated function OnToggle( SeqAct_Toggle inAction )
{
	Super.OnToggle( inAction );

	if( inAction.InputLinks[0].bHasImpulse )
	{
		// Turn on pain if that was it's original state
		bPainCausing = BACKUP_bPainCausing;
	}
	else
	if( inAction.InputLinks[1].bHasImpulse )
	{
		// Turn off pain
		bPainCausing = FALSE;
	}
	else
	if( inAction.InputLinks[2].bHasImpulse )
	{
		// Toggle pain off, or on if original state caused pain
		bPainCausing = !bPainCausing && BACKUP_bPainCausing;
	}
}

/*
TimerPop
damage touched actors if pain causing.
since PhysicsVolume is static, this function is actually called by a volumetimer
*/
function TimerPop(VolumeTimer T)
{
	local actor A;

	if ( T == PainTimer )
	{
		if ( !bPainCausing )
			return;

		ForEach TouchingActors(class'Actor', A)
			if ( A.bCanBeDamaged && !A.bStatic )
			{
				CausePainTo(A);
			}
	}
}

simulated event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
	Super.Touch(Other, OtherComp, HitLocation, HitNormal);
	if ( (Other == None) || Other.bStatic )
		return;
	if ( bNoInventory && (DroppedPickup(Other) != None) && (Other.Owner == None) )
	{
		Other.LifeSpan = 1.5;
		return;
	}
	if ( bMoveProjectiles && (ZoneVelocity != vect(0,0,0)) )
	{
		if ( Other.Physics == PHYS_Projectile )
			Other.Velocity += ZoneVelocity;
			else if ( (Other.Base == None) && Other.IsA('Emitter') && (Other.Physics == PHYS_None) )
		{
			Other.SetPhysics(PHYS_Projectile);
			Other.Velocity += ZoneVelocity;
		}
	}
	if ( bPainCausing )
	{
		if ( Other.bDestroyInPainVolume )
		{
			Other.Destroy();
			return;
		}
			if ( Other.bCanBeDamaged )
			{
				CausePainTo(Other);
			}
	}
}

function CausePainTo(Actor Other)
{
	local float depth;
	local Pawn P;

	// FIXMEZONE figure out depth of actor, and base pain on that!!!
	depth = 1;
	P = Pawn(Other);

	if ( DamagePerSec > 0 )
	{
		if ( WorldInfo.bSoftKillZ && (Other.Physics != PHYS_Walking) )
			return;
		if ( (DamageType == None) || (DamageType == class'DamageType') )
			`log("No valid damagetype specified for "$self);
		Other.TakeDamage(int(DamagePerSec * depth), None, Location, vect(0,0,0), DamageType);
	}
	else
	{
		if ( (P != None) && (P.Health < P.Default.Health) )
			P.Health = Min(P.Default.Health, P.Health - depth * DamagePerSec);
	}
}

/** called from GameInfo::SetPlayerDefaults() on the Pawn's PhysicsVolume after the its default movement properties have been restored
 * allows the volume to reapply any movement modifiers on the Pawn
 */
function ModifyPlayer(Pawn PlayerPawn);

defaultproperties
{
	Begin Object Name=BrushComponent0
		CollideActors=true
		BlockActors=false
		BlockZeroExtent=true
		BlockNonZeroExtent=true
		BlockRigidBody=false
	End Object

	MaxDampingForce=1000000.0
	FluidFriction=+0.3
    TerminalVelocity=+03500.000000
	bAlwaysRelevant=true
	bOnlyDirtyReplication=true
    GroundFriction=+00008.000000
	NetUpdateFrequency=0.1
	bSkipActorPropertyReplication=true
	DamageType=class'Engine.DamageType'
}
