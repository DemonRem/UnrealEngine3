/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This class should have 2 inputs.  One that defines the wheel and one that defines
 * the tread.
*/

class UTSkelControl_TankTread extends SkelControlBase
	native(Animation);

/**
 * How much play should this portion of the tread have
 */
var(TankTread)	float	SpaceAbove;
var(TankTread)	float	SpaceBelow;

/**
 * This control will use this bone as it's tread
 */

var(TankTread) 	name	TreadBone;
var				int 	TreadIndex;

var(TankTread)  float			CenterOffset;
var(TankTread)  array<float> 	AlternateScanOffsets;

var(TankTread) bool	bAlwaysScan;


/**
 * This holds the current adjustment.  If the vehicle isn't moving then this is never updated
 */

var transient float	Adjustment;
var transient float TargetAdjustment;
var transient bool	bInitialized;

var transient bool	bDormant;
var transient bool	bLastDirWasBackwards;

cpptext
{
	FLOAT CalcAdjustment(FVector TraceStart, FVector TraceEnd, FVector Offsets, AActor* Owner);
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
	virtual void GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices);
}


defaultproperties
{
	SpaceAbove=24.0
	SpaceBelow=8.0
	bDormant=false
	bIgnoreWhenNotRendered=true
}
