/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCharacter_TwinSoulsMaleA extends UTCharacter;

defaultproperties
{
	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_RTeam.Mesh.SK_CH_TwinSouls_MaleA'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
	End Object

	BodySkinMaterialInstanceIndex=1
	BodySkins(Skin_RedTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody01_VRed'
	BodySkins(Skin_BlueTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody01_VBlue'
	BodySkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody01_V01'

	HeadSkinMaterialInstanceIndex=0
	HeadSkins(Skin_RedTeam)=None
	HeadSkins(Skin_BlueTeam)=None
	HeadSkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MHead01_V01'
}
