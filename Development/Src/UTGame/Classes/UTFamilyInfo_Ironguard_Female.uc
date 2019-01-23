/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTFamilyInfo_Ironguard_Female extends UTFamilyInfo_Human
	abstract;

defaultproperties
{
	FamilyID="IRNF"
	Faction="Ironguard"

	ArmMeshPackageName="WP_Arms"
	ArmMeshName="WP_Arms.Mesh.SK_WP_IronGuard_MaleB_1P_Arms"
	ArmSkinPackageName="CH_IronGuard_Arms"
	RedArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VRed"
	BlueArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VBlue"

	PhysAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseFemale_Physics'
	AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
	SoundGroupClass=class'UTPawnSoundGroup_HumanFemale'

	BaseMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_IronG_Base'
	BioDeathMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_IronG_BioDeath'

	MasterSkeleton=SkeletalMesh'CH_All.Mesh.SK_Master_Skeleton_Human_Female'
}
