/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ActorFactoryRigidBody extends ActorFactoryDynamicSM
	config(Editor)
	collapsecategories
	hidecategories(Object)
	native;

/** Should spawned Actor start simulating as soon as its created, or be 'asleep' until hit. */
var()	bool	bStartAwake;

/** Sets the bDamageAppliesImpulse flag on the new Actor. */
var()	bool	bDamageAppliesImpulse;

/** Indicates if the initial velocity settings below should be considered in the world space or local space of the spawn target actor. */
var()	bool	bLocalSpaceInitialVelocity;

/** Velocity that new rigid bodies will have when created. In the ref frame of the spawn target actor.  */
var()	vector	InitialVelocity;

/**
 *	If valid, Velocity added to InitialVelocity when creating actor.
 *	This is here in addition to InitialVelocity to maintain backwards compatibility.
 */
var()	DistributionVector	AdditionalVelocity;

/**
 *	If valid, Angular Velocity given to newly spawned Actor.
 */
var()	DistributionVector	InitialAngularVelocity;

var deprecated	bool bCollideActors;

/** Allows setting the RBChannel flag on the spawned rigid body's StaticMeshComponent. */
var()	ERBCollisionChannel		RBChannel;

var deprecated bool bRigidBodyIgnorePawns;

cpptext
{
	// UObject interface
	virtual void PostLoad();

	// ActorFactoryRigidBody
	virtual UBOOL CanCreateActor(FString& OutErrorMsg);
	virtual AActor* CreateActor( const FVector* const Location, const FRotator* const Rotation, const class USeqAct_ActorFactory* const ActorFactoryData );
}

defaultproperties
{
	MenuName="Add RigidBody"

	NewActorClass=class'Engine.KActor'
	GameplayActorClass=class'Engine.KActorSpawnable'

	bNoEncroachCheck=true
	bStartAwake=true
	bDamageAppliesImpulse=true
	CollisionType=COLLIDE_BlockAll
	RBChannel=RBCC_GameplayPhysics
	bRigidBodyIgnorePawns=true
}
