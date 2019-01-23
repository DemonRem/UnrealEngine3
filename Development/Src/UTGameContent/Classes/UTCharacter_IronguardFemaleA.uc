/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCharacter_IronguardFemaleA extends UTCharacter_IronguardMaleA;

defaultproperties
{
	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_IronGuard.Mesh.SK_CH_IronGuard_FemaleA'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
		PhysicsAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseFemale_Physics'
	End Object

	BodySkinMaterialInstanceIndex=1
	BodySkins(Skin_RedTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_FBody_VRed'
	BodySkins(Skin_BlueTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_FBody_VBlue'
	BodySkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_FBody'

	HeadSkinMaterialInstanceIndex=0
	HeadSkins(Skin_RedTeam)=None
	HeadSkins(Skin_BlueTeam)=None
	HeadSkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_IronG01_FHead01_SK1'

	SoundGroupClass=class'UTPawnSoundGroup_HumanFemale'
}
