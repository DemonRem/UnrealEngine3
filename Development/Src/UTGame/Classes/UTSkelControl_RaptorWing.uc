/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_RaptorWing extends SkelControlSingleBone
	hidecategories(Translation, Rotation, Adjustment)
	native(Animation);

/** The tip Sockets are used to trace from the tip of the wing */
var(Wing)	name	TipSockets[2];

/** JoinSockets are used when tracing from the joint + velocity */

var(Wing)	name	JointSockets[2];

/** TipBone defines which bone is left and which is right */

var(Wing)	name	TipBones[2];

/** WingLength is the left of the wing */

var(Wing)	int		WingLength;

/** These holds the various runtime values 0=Left, 1=Right */

var			int		Bones[2];
var			int		bForcedDesired[2];
var			int		ActualPitch[2];
var			int		DesiredPitch[2];
var			float	PitchRate[2];

var			int		DesiredAngle;

/**  WorldInfo->TimeSeconds < UpTimeSeconds then the wings will not fold down */

var			float	UpTimeSeconds;
var			float	MinUpTime;

/** If true, this component has been initialized and the BoneIndex array is valid */
var	transient	bool	bInitialized;

/** true if was already folding up wings because of descent */
var bool bAlreadyDescending;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
}

/**
 * This function will force a single wing in to the up position
 */

native function	ForceSingleWingUp(bool bRight, float Rate);

/**
 * This function will force both wings in to the up position
 */

native function ForceWingsUp(float Rate);

defaultproperties
{
	bApplyRotation=true
	BoneRotationSpace=BCS_ActorSpace
	ControlStrength=1.0
	WingLength=165;
	TipSocket
	ActualPitch(0)=-16384
	ActualPitch(1)=-16384
	DesiredPitch=-16384
	bInitialize
	MinUpTime=0.5
	bIgnoreWhenNotRendered=true
}
