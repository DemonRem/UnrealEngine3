/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_TurretMultiBone extends UTSkelControl_TurretConstrained
	native(Animation);

/** The name of the bone that controls the left turret barrels */
var(Turret)	name LeftTurretArmBone;

/** Index of the above Bone */
var transient int LeftTurretArmIndex;

/** The name of the bone that controls the right turret barrels */
var(Turret)	name RightTurretArmBone;

/** Index of the above Bone */
var transient int RightTurretArmIndex;

/** The name of the bone that controls pivoting up */
var(Turret)	name PivotUpBone;

/** Index of the above Bone */
var transient int PivotUpIndex;

/** Begin to pivot up when the turret's pitch has reached this % of the constraint */
var(Turret) float PivotUpPerc;

/** Begin to pivot down when the turret's pitch has reached this % of the constraint */
var(Turret) float PivotDownPerc;

/** This is set to true when the the control has precached all of the indexes */
var transient bool bIndexCacheIsValid;

/** How much of the constraint has been met.  If the value is negative, we are heading towards
    the min angle */
var transient float ConstraintPct;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
}


defaultproperties
{
	bIndexCacheIsValid=false;
}
