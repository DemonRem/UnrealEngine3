/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*/

class UTFamilyInfo_Liandri_Male extends UTFamilyInfo_Human
	abstract;

defaultproperties
{
	FamilyID="LIAM"
	Faction="Liandri"

	ArmMeshPackageName="WP_Arms"
	ArmMeshName="WP_Arms.Mesh.SK_WP_IronGuard_MaleB_1P_Arms"
	ArmSkinPackageName="CH_IronGuard_Arms"
	RedArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VRed"
	BlueArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VBlue"

	PhysAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseMale_Physics'

	BaseMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_Corrupt_Base'
	BioDeathMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_Corrupt_BioDeath'

	MasterSkeleton=SkeletalMesh'CH_All.Mesh.SK_Master_Skeleton_Human_Male'
}