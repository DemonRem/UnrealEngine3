/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class PhysicsAssetInstance extends Object
	hidecategories(Object)
	native(Physics);

/**
 *	Per-instance asset variables
 *	You only need one of these when you are actually running physics for the asset. 
 *	One is created for a skeletal mesh inside AActor::InitArticulated.
 *	You can perform line-checks etc. with just a PhysicsAsset.
 */


/**
 *	Actor that owns this PhysicsAsset instance.
 *	Filled in by InitInstance, so we don't need to save it.
 */
var 	const transient Actor							Owner; 

/** 
 *	Index of the 'Root Body', or top body in the asset heirarchy. Used by PHYS_RigidBody to get new location for Actor.
 *	Filled in by InitInstance, so we don't need to save it.
 */
var		const transient int								RootBodyIndex;

/** Array of RB_BodyInstance objects, storing per-instance state about about each body. */
var		const instanced array<RB_BodyInstance>			Bodies;

/** Array of RB_ConstraintInstance structs, storing per-instance state about each constraint. */
var		const instanced array<RB_ConstraintInstance>	Constraints;

/** Table indicating which pairs of bodies have collision disabled between them. Used internally. */
var		const native Map_Mirror							CollisionDisableTable{TMap<FRigidBodyIndexPair,UBOOL>};


/** Scaling factor to LinearDriveSpring in all RB_ConstraintInstances within this instance. */
var		const float										LinearSpringScale;

/** Scaling factor to LinearDriveDamping in all RB_ConstraintInstances within this instance. */
var		const float										LinearDampingScale;

/** Scaling factor to LinearDriveForceLimit in all RB_ConstraintInstances within this instance. */
var		const float										LinearForceLimitScale;


/** Scaling factor to AngularDriveSpring in all RB_ConstraintInstances within this instance. */
var		const float										AngularSpringScale;

/** Scaling factor to AngularDriveDamping in all RB_ConstraintInstances within this instance. */
var		const float										AngularDampingScale;

/** Scaling factor to AngularDriveForceLimit in all RB_ConstraintInstances within this instance. */
var		const float										AngularForceLimitScale;



cpptext
{
	// UObject interface
	virtual void			Serialize(FArchive& Ar);

	// UPhysicsAssetInstance interface
	void					InitInstance(class USkeletalMeshComponent* SkelComp, class UPhysicsAsset* PhysAsset, UBOOL bFixed, FRBPhysScene* InRBScene);
	UBOOL					TermInstance(FRBPhysScene* Scene);

	void					DisableCollision(class URB_BodyInstance* BodyA, class URB_BodyInstance* BodyB);
	void					EnableCollision(class URB_BodyInstance* BodyA, class URB_BodyInstance* BodyB);
}

final native function	SetLinearDriveScale(float InLinearSpringScale, float InLinearDampingScale, float InLinearForceLimitScale);
final native function	SetAngularDriveScale(float InAngularSpringScale, float InAngularDampingScale, float InAngularForceLimitScale);

/** Set angular motors strength */
final native function	SetAllMotorsAngularDriveStrength(float InAngularSpringStrength, float InAngularDampingStrength, float InAngularForceLimitStrength, SkeletalMeshComponent SkelMeshComp);

/** Utility which returns total mass of all bones below the supplied one in the hierarchy (including this one). */
final native function	float	GetTotalMassBelowBone(name InBoneName, PhysicsAsset InAsset, SkeletalMesh InSkelMesh);

/** Fix or unfix all bodies */
final native function	SetAllBodiesFixed(bool bNewFixed);

/** Fix or unfix a list of bodies, by name */
final native function	SetNamedBodiesFixed(bool bNewFixed, Array<Name> BoneNames, SkeletalMeshComponent SkelMesh, optional bool bSetOtherBodiesToComplement);

/** Enable or Disable AngularPositionDrive */
final native function	SetAllMotorsAngularPositionDrive(bool bEnableSwingDrive, bool bEnableTwistDrive);

/** Enable or Disable AngularPositionDrive based on a list of bone names */
final native function	SetNamedMotorsAngularPositionDrive(bool bEnableSwingDrive, bool bEnableTwistDrive, Array<Name> BoneNames, SkeletalMeshComponent SkelMeshComp, optional bool bSetOtherBodiesToComplement);

/** Set Angular Drive motors params for all constraint instance */
final native function	SetAllMotorsAngularDriveParams(float InSpring, float InDamping, float InForceLimit);

/** Use to toggle and set RigidBody angular and linear bone springs (see RB_BodyInstance). */
final native function	SetNamedRBBoneSprings(bool bEnable, Array<Name> BoneNames, float InBoneLinearSpring, float InBoneAngularSpring, SkeletalMeshComponent SkelMeshComp);

/** Use to toggle collision on particular bodies in the asset. */
final native function	SetNamedBodiesBlockRigidBody(bool bNewBlockRigidBody, Array<Name> BoneNames, SkeletalMeshComponent SkelMesh);

/** Allows you to fix/unfix bodies where bAlwaysFullAnimWeight is set to TRUE in the BodySetup. */
final native function	SetFullAnimWeightBonesFixed(bool bNewFixed, SkeletalMeshComponent SkelMesh);

/** Find instance of the body that matches the name supplied. */
final native function RB_BodyInstance FindBodyInstance(name BodyName, PhysicsAsset InAsset);

/** Find instance of the constraint that matches the name supplied. */
final native function RB_ConstraintInstance FindConstraintInstance(name ConName, PhysicsAsset InAsset);

defaultproperties
{
	LinearSpringScale=1.0
	LinearDampingScale=1.0
	LinearForceLimitScale=1.0

	AngularSpringScale=1.0
	AngularDampingScale=1.0
	AngularForceLimitScale=1.0
}