/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_NecrisRaptorWings extends SkelControlSingleBone
	hidecategories(Translation, Rotation, Adjustment)
	native(Animation);

/** The tip Sockets are used to trace from the tip of the wing */
var(Wing)	name	TipSocket;

/** JoinSockets are used when tracing from the joint + velocity */

var(Wing)	name	JointSocket;

/** LowerWingBoneName is the bone that aligns the lower wing */

var(Wing)	name	LowerWingBoneName;

/** UpperWingBoneName is the bone that aligns the upper wing */

var(Wing)	name	UpperWingBoneName;

/** WingLength is the left of the wing */

var(Wing)	int		WingLength;

/** If true, this wing is the top wing and all traced will be inverted */

var(Wing)	bool	bInverted;

/** These holds the various runtime values 0=Left, 1=Right */

var			int		UpperWingIndex;
var			int		bForcedDesired;
var			int		ActualPitch;
var			int		DesiredPitch;
var			float	PitchRate;

var			int		DesiredAngle;

/**  WorldInfo->TimeSeconds < UpTimeSeconds then the wings will not fold down */

var			float	UpTimeSeconds;
var			float	MinUpTime;

/** If true, this component has been initialized and the BoneIndex array is valid */

var	transient	bool	bInitialized;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
}

defaultproperties
{
	bApplyRotation=true
	BoneRotationSpace=BCS_ActorSpace
	ControlStrength=1.0
	WingLength=165;
	MinUpTime=0.5
	bIgnoreWhenNotRendered=true
}
