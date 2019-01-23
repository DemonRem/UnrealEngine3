/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class RB_BodyInstance extends Object
	hidecategories(Object)
	native(Physics);

cpptext
{
	// Object interface
	virtual void FinishDestroy();

	// RB_BodyInstance interface
	void InitBody(class URB_BodySetup* setup, const FMatrix& transform, const FVector& Scale3D, UBOOL bFixed, UPrimitiveComponent* PrimComp, FRBPhysScene* InRBScene);
	UBOOL TermBody(FRBPhysScene* Scene);

	UBOOL IsValidBodyInstance();
	FMatrix GetUnrealWorldTM();

	FVector GetCOMPosition();
	FLOAT GetBodyMass();
	void DrawCOMPosition( FPrimitiveDrawInterface* PDI, FLOAT COMRenderSize, const FColor& COMRenderColor );

	/** Utility for copying properties from one BodyInstance to another. */
	void CopyBodyInstancePropertiesFrom(URB_BodyInstance* FromInst);

	UPhysicalMaterial* GetPhysicalMaterial();

#if WITH_NOVODEX
	class NxActor* GetNxActor();
	void UpdatePhysMaterialOverride();
#endif
}

/** PrimitiveComponent containing this body. */
var const transient PrimitiveComponent		OwnerComponent;

/** Index of this BodyInstance within the PhysicsAssetInstance/PhysicsAsset. */
var const int								BodyIndex;

/** Current linear velocity of this BodyInstance. Only updated if this is the root body of the CollisionComponent of an Actor in PHYS_RigidBody */
var vector									Velocity;

/** 
 *	Previous frame Velocity of this BodyInstance. Only updated if this is the root body of the CollisionComponent of an Actor in PHYS_RigidBody
 *	used for collision events since by the time the event is sent, Velocity would have been updated with the post-collision velocity
 */
var vector									PreviousVelocity;

/** Physics scene index. */
var	native const int						SceneIndex;

/** Internal use. Physics-engine representation of this body. */
var native const pointer					BodyData;

/** Internal use. Physics-engine representation of the spring on this body. */
var native const pointer					BoneSpring;

/** If bUseKinActorForBoneSpring is true, this is the Physics-engine representation of the kinematic actor the spring is attached to. */
var native const pointer					BoneSpringKinActor;

/** Enable linear 'spring' between the physics body for this bone, and the world-space location of the animation bone. */
var(BoneSpring)	bool						bEnableBoneSpringLinear;

/** Enable angular 'spring' between the physics body for this bone, and the world-space location of the animation bone. */
var(BoneSpring)	bool						bEnableBoneSpringAngular;

/** If true, bone spring will automatically disable if it ever gets longer than the OverextensionThreshold. */
var(BoneSpring)	bool						bDisableOnOverextension;

/** 
 *	This will teleport the whole PhysicsAssetInstance if this spring gets too long, to reduce to error to zero.
 *	Note - having this set on more than one body in a PhysicsAssetInstance will have unpredictable results.
 */
var(BoneSpring) bool						bTeleportOnOverextension;

/** 
 *	When using bone springs, connect them to a kinematic body and animate that, rather than animating the attachment location on the 'world'.
 *	This adds some overhead, but tracks rapidly moving targets better.
 */
var(BoneSpring)	bool						bUseKinActorForBoneSpring;

/** 
 *	When using bone springs, connect them to the physics body of the Base's CollisionComponent.
 *	When using this option, SetBoneSpringTarget must be given a matrix for this bone relative to the other bone,
 *	rather than relative to the world.
 */
var(BoneSpring) bool						bMakeSpringToBaseCollisionComponent;

/** Strength of linear spring to animated bone. */
var(BoneSpring) float						BoneLinearSpring;

/** Damping on linear spring to animated bone. */
var(BoneSpring) float						BoneLinearDamping;

/** Strength of angular spring to animated bone. */
var(BoneSpring) float						BoneAngularSpring;

/** Damping on angular spring to animated bone. */
var(BoneSpring) float						BoneAngularDamping;

/** If bDisableOnOverextension is on, the bone spring will be disabled if it stretches more than this amount. */
var(BoneSpring)	float						OverextensionThreshold;

/** 
 *	The force applied to the body to address custom gravity for the actor is multiplied CustomGravityFactor, allowing bodies to individually control how 
 *	custom gravity settings affectthem
 */
var()			float						CustomGravityFactor;

/** 
 *	This body should only collide with other bodies with their components marked bConsiderPawnForRBCollision.
 *	Useful for flappy bits you do not want to collide with the world.
 */
var()			const bool					bOnlyCollideWithPawns;

/** For per-bodyinstance effects this keeps track of the last time one played. Could be used for items like gib effects. */
var				transient	float			LastEffectPlayedTime;

/** Allows you to override the PhysicalMaterial to use for this body. */
var(Physics)	const PhysicalMaterial		PhysMaterialOverride;

/** Force this body to be fixed (kinematic) or not. Overrides the BodySetup for this body. */
final native function				SetFixed(bool bNewFixed);
/** Returns TRUE if this body is fixed */
final native function		bool	IsFixed();

/** Get current velocity in world space from physics body. */
final native function vector		GetUnrealWorldVelocity();

/** Get current angular velocity in world space from physics body. */
final native function vector		GetUnrealWorldAngularVelocity();

/** 
 *	Used to turn the angular/linear bone spring on and off. 
 *	InBoneTarget is in world space, unless bMakeSpringToBaseCollisionComponent is TRUE, in which case it is relative to that body.
 */
final native function				EnableBoneSpring(bool bInEnableLinear, bool bInEnableAngular, const out matrix InBoneTarget);

/** Used to set the spring stiffness and  damping parameters for the bone spring. */
final native function				SetBoneSpringParams(float InLinearSpring, float InLinearDamping, float InAngularSpring, float InAngularDamping);

/** 
 *	Used to set desired target location of this spring. Usually called by UpdateRBBonesFromSpaceBases. 
 *	InBoneTarget is in world space, unless bMakeSpringToBaseCollisionComponent is TRUE, in which case it is relative to that body.
 */
final native function				SetBoneSpringTarget(const out matrix InBoneTarget, bool bTeleport);

/** Used to disable rigid body collisions for this body. Overrides the bNoCollision flag in the BodySetup for this body. */
final native function				SetBlockRigidBody(bool bNewBlockRigidBody);

/** Set physical material override for this body */
final native function				SetPhysMaterialOverride( PhysicalMaterial NewPhysMaterial );

defaultproperties
{
	BoneLinearSpring=10.0
	BoneLinearDamping=0.1
	BoneAngularSpring=1.0
	BoneAngularDamping=0.1
	CustomGravityFactor=1.0
	LastEffectPlayedTime = 0.0
}
