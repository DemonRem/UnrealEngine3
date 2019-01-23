/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCharacter_IronguardMaleA extends UTCharacter;

defaultproperties
{
	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_IronGuard.Mesh.SK_CH_IronGuard_MaleA'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
	End Object

	BodySkinMaterialInstanceIndex=1
	BodySkins(Skin_RedTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_Mbody_VRed'
	BodySkins(Skin_BlueTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_Mbody_VBlue'
	BodySkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_Mbody_V'

	HeadSkinMaterialInstanceIndex=0
	HeadSkins(Skin_RedTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_MHead_VRed'
	HeadSkins(Skin_BlueTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_MHead_VBlue'
	HeadSkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_MHead'
}