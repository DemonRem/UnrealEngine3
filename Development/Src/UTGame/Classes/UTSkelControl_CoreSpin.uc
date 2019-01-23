/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_CoreSpin extends SkelControlSingleBone
	native(Animation)
	hidecategories(Adjustments,Translation,Rotation);

/** How fast is the core to spin at max health */

var(Spin)	float 	DegreesPerSecondMax;	
var(Spin)	float	DegreesPerSecondMin;

/** Spin the other way */
var(Spin)	bool	bCounterClockwise;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
}

defaultproperties
{
	bApplyTranslation=false
	bAddTranslation=false
	bApplyRotation=true
	bAddRotation=true
	BoneRotationSpace=BCS_ActorSpace
	DegreesPerSecondMax=180
	DegreesPerSecondMin=40
}
