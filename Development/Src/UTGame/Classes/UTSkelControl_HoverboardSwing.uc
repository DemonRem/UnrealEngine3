/**
 *	Controller used by hoverboard for moving lower part in response to wheel movements.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_HoverboardSwing extends SkelControlSingleBone
	hidecategories(Translation,Rotation)
	native(Animation);

cpptext
{
	// SkelControlWheel interface
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
}

var()	float	SwingScale;
var()	float	MaxSwing;
var()	float	MaxSwingPerSec;
var()	float	MaxUseVel;
var		float	CurrentSwing;

defaultproperties
{
	bApplyRotation=true
	bAddRotation=true
	BoneRotationSpace=BCS_BoneSpace
	MaxSwingPerSec=1.0
}