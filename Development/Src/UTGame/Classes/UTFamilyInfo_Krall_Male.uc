/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*/

class UTFamilyInfo_Krall_Male extends UTFamilyInfo
	abstract;

defaultproperties
{
	FamilyID="KRAM"
	Faction="Krall"

	ArmMeshPackageName="WP_Arms"
	ArmMeshName="WP_Arms.Mesh.SK_WP_Krall_MaleA_1P_Arms"
	ArmSkinPackageName="CH_Krall_Arms"
	RedArmSkinName="CH_Krall_Arms.Materials.MI_CH_Krall_Arms_MFirstPersonArm_VRed"
	BlueArmSkinName="CH_Krall_Arms.Materials.MI_CH_Krall_Arms_MFirstPersonArm_VBlue"

	PhysAsset=PhysicsAsset'CH_Krall_Male.Mesh.SK_CH_Krall_Male01_Physics'
	AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
	AnimSets(1)=AnimSet'CH_AnimKrall.Anims.K_AnimKrall_Base'
	LeftFootBone=b_LeftFoot
	RightFootBone=b_RightFoot
	TakeHitPhysicsFixedBones[0]=b_LeftFoot
	TakeHitPhysicsFixedBones[1]=b_RightFoot

	BaseMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_Krall_Base'
	BioDeathMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_Krall_BioDeath'

	MasterSkeleton=SkeletalMesh'CH_All.Mesh.SK_Master_Skeleton_Krall'

	HeadGib=(BoneName=b_Head,GibClass=class'UTGib_HumanHead',bHighDetailOnly=false)

	Gibs[0]=(BoneName=b_LeftForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	Gibs[1]=(BoneName=b_RightForeArm,GibClass=class'UTGib_HumanHand',bHighDetailOnly=true)
	Gibs[2]=(BoneName=b_LeftLeg,GibClass=class'UTGib_HumanBone',bHighDetailOnly=false)
	Gibs[3]=(BoneName=b_RightLeg,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[4]=(BoneName=b_Spine,GibClass=class'UTGib_HumanTorso',bHighDetailOnly=false)
	Gibs[5]=(BoneName=b_Spine1,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	Gibs[6]=(BoneName=b_Spine2,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=true)
	Gibs[7]=(BoneName=b_LeftClav,GibClass=class'UTGib_HumanArm',bHighDetailOnly=true)
	Gibs[8]=(BoneName=b_RightClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[9]=(BoneName=b_LeftLegUpper,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[10]=(BoneName=b_RightLegUpper,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
}