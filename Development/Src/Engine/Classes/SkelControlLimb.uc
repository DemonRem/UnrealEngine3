class SkelControlLimb extends SkelControlBase
	native(Anim);
	
/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 *	Controller Specifically designed for 2-bone limbs with an elbow-like joint in the middle.
 *	Should be set up to act on the bone at the end of the limb - ie the hand in the case of an arm.
 */
 
cpptext
{
	// USkelControlBase interface
	virtual void GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);	

	virtual INT GetWidgetCount();
	virtual FMatrix GetWidgetTM(INT WidgetIndex, USkeletalMeshComponent* SkelComp, INT BoneIndex);
	virtual void HandleWidgetDrag(INT WidgetIndex, const FVector& DragVec);
	virtual void DrawSkelControl3D(const FSceneView* View, FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, INT BoneIndex);
}

/** Where you want the controlled bone to be. Will be placed as close as possible within the constraints of the limb. */
var(Effector)	vector					EffectorLocation;

/** Reference frame that the DesiredLocation is defined in. */
var(Effector)	EBoneControlSpace		EffectorLocationSpace;

/** Name of bone used if DesiredLocationSpace is BCS_OtherBoneSpace. */
var(Effector)	name					EffectorSpaceBoneName;

/** Point in space where the joint should move towards as it bends. */
var(Joint)		vector					JointTargetLocation;

/** Reference frame in which JointTargetLocation is defined. */
var(Joint)		EBoneControlSpace		JointTargetLocationSpace;

/** Name of bone used if JointTargetLocationSpace is BCS_OtherBoneSpace. */
var(Joint)		name					JointTargetSpaceBoneName;

/** Axis of graphical bone to align along the length of the bone. */
var(Limb)		EAxis					BoneAxis;

/** Axis of graphical bone to align along the hinge axis of the joint. */
var(Limb)		EAxis					JointAxis;

/** If we want to invert BoneAxis when constructing the transform for the bones. */
var(Limb)		bool					bInvertBoneAxis;

/** If we want to invert JointAxis when constructing the transform for the bones. */
var(Limb)		bool					bInvertJointAxis;

/** 
 *	If true, modify the relative rotation between the end 'effector' bone and its parent bone. If false,
 *	the rotation of the end bone will not be modified by this controller.
 */
var(Limb)		bool					bMaintainEffectorRelRot;

/** If true, rotation of effector bone is copied from the bone specified by EffectorSpaceBoneName. */
var(Limb)		bool					bTakeRotationFromEffectorSpace;

defaultproperties
{
	EffectorLocationSpace=BCS_WorldSpace
	JointTargetLocationSpace=BCS_WorldSpace
	
	BoneAxis=AXIS_X
	JointAxis=AXIS_Y
}
