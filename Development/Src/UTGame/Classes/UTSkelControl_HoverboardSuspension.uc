/**
 *	Controller used by hoverboard for moving lower part in response to wheel movements.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_HoverboardSuspension extends SkelControlSingleBone
	hidecategories(Translation,Rotation)
	native(Animation);



cpptext
{
	// SkelControlWheel interface
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
}

var()	float	TransIgnore;
var()	float	TransScale;
var()	float	TransOffset;
var()	float	MaxTrans;
var()	float	RotScale;
var()	float	MaxRot;

defaultproperties
{
	bApplyTranslation=true
	bAddTranslation=true
	BoneTranslationSpace=BCS_BoneSpace

	bApplyRotation=true
	bAddRotation=true
	BoneRotationSpace=BCS_BoneSpace
}