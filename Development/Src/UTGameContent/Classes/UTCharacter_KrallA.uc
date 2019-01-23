/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCharacter_KrallA extends UTCharacter;

defaultproperties
{
	LeftToeBone=b_LeftFoot
	LeftFootBone=b_LeftFoot
	RightFootBone=b_RightFoot
	TakeHitPhysicsFixedBones[0]=b_LeftFoot
	TakeHitPhysicsFixedBones[1]=b_RightFoot

	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_Krall.Mesh.SK_CH_Krall01'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		AnimSets(1)=AnimSet'CH_AnimKrall.Anims.K_AnimKrall_Base'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
		PhysicsAsset=PhysicsAsset'CH_Krall.Mesh.SK_CH_Krall01_Physics'
	End Object

	Begin Object Name=FirstPersonArms
		//SkeletalMesh=SkeletalMesh'WP_FlakCannon.Mesh.SK_WP_FlakCannon_1P_Arms'
		SkeletalMesh=SkeletalMesh'WP_Arms.Mesh.SK_WP_Krall_MaleA_1P_Arms'
		PhysicsAsset=None
		FOV=55
		Animations=MeshSequenceA
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
		AbsoluteTranslation=true
		AbsoluteRotation=true
		AbsoluteScale=true
		bSyncActorLocationToRootRigidBody=false
		CastShadow=false
	End Object

	Begin Object Name=FirstPersonArms2
		//SkeletalMesh=SkeletalMesh'WP_FlakCannon.Mesh.SK_WP_FlakCannon_1P_Arms'
		SkeletalMesh=SkeletalMesh'WP_Arms.Mesh.SK_WP_Krall_MaleA_1P_Arms'
		PhysicsAsset=None
		FOV=55
		Scale3D=(Y=-1.0)
		Animations=MeshSequenceB
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		bOverrideAttachmentOwnerVisibility=true
		HiddenGame=true
		bAcceptsDecals=false
		AbsoluteTranslation=true
		AbsoluteRotation=true
		AbsoluteScale=true
		bSyncActorLocationToRootRigidBody=false
		CastShadow=false
	End Object

	BodySkinMaterialInstanceIndex=0
	BodySkins(Skin_RedTeam)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MBody02_VRed'
	BodySkins(Skin_BlueTeam)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MBody02_VBlue'
	BodySkins(Skin_Normal)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MBody02_V01'

	ArmSkins(FPArmSkin_BlueTeam)=none		// @fixme, override in utcharacter subclass files when content is ready
	ArmSkins(FPArmSkin_RedTeam)=none

	HeadSkinMaterialInstanceIndex=1
	HeadSkins(Skin_RedTeam)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MHead01_VRed'
	HeadSkins(Skin_BlueTeam)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MHead01_VBlue'
	HeadSkins(Skin_Normal)=MaterialInstance'CH_Krall.Materials.M_CH_Krall_MHead01_V01'
}
