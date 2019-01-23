/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTFamilyInfo_Ironguard_Male extends UTFamilyInfo_Human
	abstract;

defaultproperties
{
	FamilyID="IRNM"
	Faction="Ironguard"

	ArmMeshPackageName="WP_Arms"
	ArmMeshName="WP_Arms.Mesh.SK_WP_IronGuard_MaleB_1P_Arms"
	ArmSkinPackageName="CH_IronGuard_Arms"
	RedArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VRed"
	BlueArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VBlue"

	PhysAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseMale_Physics'
	AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'

	BaseMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_IronG_Base'
	BioDeathMICParent=MaterialInstanceConstant'CH_All.Materials.MI_CH_ALL_IronG_BioDeath'

	MasterSkeleton=SkeletalMesh'CH_All.Mesh.SK_Master_Skeleton_Human_Male'
/* - Moved
	FamilyEmotes.Add((CategoryName="Taunt",Tag="MessYouUp",EmoteName="Mess You Up",EmoteAnim="Taunt_UB_Slit_Throat",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_ImAMessYouUpCue',bTopHalfEmote=true))
	FamilyEmotes.Add((CategoryName="Taunt",Tag="OnFire",EmoteName="I'm On Fire",EmoteAnim="Stumble_Bwd",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_ImOnFireCue'))
	FamilyEmotes.Add((CategoryName="Warning",Tag="HereTheyCome",EmoteName="Here They Come",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_HereTheyComeCue'))
	FamilyEmotes.Add((CategoryName="Victory",Tag="Cheer",EmoteName="Cheer",EmoteAnim="Taunt_FB_Victory"))
	FamilyEmotes.Add((CategoryName="Action",Tag="TakeFlag",EmoteName="Take Flag",EmoteAnim="Taunt_UB_Flag_Pickup",bTopHalfEmote=true))
*/
}
