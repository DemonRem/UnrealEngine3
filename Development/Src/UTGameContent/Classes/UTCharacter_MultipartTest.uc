/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** Note: Class has been temporarily hijacked to be test character for new skeleton */
class UTCharacter_MultipartTest extends UTCharacter;



defaultproperties
{
	DrawScale=1.0
	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_AnimHuman.Mesh.ImportMesh_Human_Male'		// temporary
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
		BlockRigidBody=true
		CastShadow=true
		//PhysicsAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseMale_Physics'
	End Object

	BodySkins(Skin_RedTeam)=Material'CH_RTeam.Materials.M_CH_RTeam_MBody01_VRed'
	BodySkins(Skin_BlueTeam)=Material'CH_RTeam.Materials.M_CH_RTeam_MBody01_VBlue'
	BodySkins(Skin_Normal)=Material'CH_RTeam.Materials.M_CH_RTeam_MBody01_V01'

	// @fixme, some of this can remain specified in utcharacter.uc once all chars are using
	// the new skeleton
	//LeftToeBone=b_LeftToe
	//LeftFootBone=b_LeftAnkle
	//RightFootBone=b_RightAnkle
	//WeaponSocket=WeaponPoint
	//HeadBone=b_Head
	//TwistFireBone=b_Spine2
	//Gibs[0]=(BoneName=b_LeftForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	//Gibs[1]=(BoneName=b_RightForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=true)
	//Gibs[2]=(BoneName=b_LeftLeg,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	//Gibs[3]=(BoneName=b_RightLeg,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	//Gibs[4]=(BoneName=b_Spine,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	//Gibs[5]=(BoneName=b_Spine1,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	//Gibs[6]=(BoneName=b_Spine2,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	//Gibs[7]=(BoneName=b_RightClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	//Gibs[8]=(BoneName=b_LeftClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	//Gibs[9]=(BoneName=b_RearLoin,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
}
