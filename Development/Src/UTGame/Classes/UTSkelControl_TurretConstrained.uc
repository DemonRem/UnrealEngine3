/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_TurretConstrained extends SkelControlSingleBone
	native(Animation);

var(Constraint)	bool bConstrainPitch;
var(Constraint)	bool bConstrainYaw;
var(Constraint)	bool bConstrainRoll;

var(Constraint)	bool bInvertPitch;
var(Constraint)	bool bInvertYaw;
var(Constraint)	bool bInvertRoll;


struct native TurretConstraintData
{
	var() int	PitchConstraint;
	var() int	YawConstraint;
	var() int	RollConstraint;
};

var(Constraint)	TurretConstraintData MaxAngle;    // Max. angle in Degrees
var(Constraint) TurretConstraintData MinAngle;    // Min. angle in Degrees

/**
 * Allow each turret to have various steps in which to contrain the data.
 */

struct native TurretStepData
{
	var() int StepStartAngle;
	var() int StepEndAngle;
	var() TurretConstraintData MaxAngle;
	var() TurretConstraintData MinAngle;
};

var(Constraints) array<TurretStepData> Steps;

var(Turret)	float 	LagDegreesPerSecond;
var(Turret) rotator	DesiredBoneRotation;

/** If true, this turret won't update if the seat it is associated with is firing */
var(Turret) bool bFixedWhenFiring;

/** The Seat Index this control is associated with */
var(Turret) int AssociatedSeatIndex;

/** If true, this turret will reset to 0,0,0 when there isn't a driver */
var(Turret) bool bResetWhenUnattended;

var bool bIsInMotion;

/** This is the world space rotation after constraints have been applied
 * We set Bone rotation to this value by default in GetAffectedBones
 */
var transient Rotator ConstrainedBoneRotation;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
}

delegate OnTurretStatusChange(bool bIsMoving);

defaultproperties
{
	bConstrainPitch=false;
	bConstrainYaw=false;
	bConstrainRoll=false;

	LagDegreesPerSecond=360
	bApplyRotation=true
	BoneRotationSpace=BCS_ActorSpace
	bIsInMotion=false

}

