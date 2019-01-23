/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Translocator_Content extends UTWeap_Translocator
	HideDropDown;



simulated function WeaponEmpty()
{
	local AnimNodeSequence ANS;
	super.WeaponEmpty();
	WeaponIdleAnims[0] = EmptyIdleAnim;
	ANS = GetWeaponAnimNodeSeq();
	if(ANS != none && ANS.AnimSeq != none && ANS.AnimSeq.SequenceName == EmptyIdleAnim)
	{
		PlayWeaponAnimation(WeaponIdleAnims[0],0.001);
	}
}

simulated function ReAddAmmo()
{
	local AnimNodeSequence ANS;
	super.ReAddAmmo();
	if(AmmoCount > 0 && WeaponIdleAnims[0] == EmptyIdleAnim && (TransDisc == None || TransDisc.bDeleteMe))
	{
		WeaponIdleAnims[0] = default.WeaponIdleAnims[0];
		ANS = GetWeaponAnimNodeSeq();
		if(ANS != none && ANS.AnimSeq != none && ANS.AnimSeq.SequenceName == EmptyIdleAnim)
		{
			PlayWeaponAnimation(WeaponIdleAnims[0],0.001);
		}
	}
}

defaultproperties
{
	WeaponColor=(R=255,G=255,B=128,A=255)
	PlayerViewOffset=(X=10.0,Y=7.0,Z=-10.0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_Translocator.Mesh.SK_WP_Translocator_1P'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_Translocator.Anims.K_WP_Translocator_1P_Base'
		Animations=MeshSequenceA
		Scale=1
		FOV=65.0
	End Object
	AttachmentClass=class'UTGameContent.UTAttachment_Translocator'

	TeamSkins(0)=none
	TeamSkins(1)=MaterialInstance'WP_Translocator.Materials.M_WP_Translocator_1PBlue'
	TeamSkins(2)=none
	TeamSkins(3)=none

	// Pickup mesh Transform
	Begin Object Class=SkeletalMeshComponent Name=StaticMeshComponent1
		SkeletalMesh=SkeletalMesh'WP_Translocator.Mesh.SK_WP_Translocator_3P'
		bOnlyOwnerSee=false
	    CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Translation=(X=0.0,Y=0.0,Z=-10.0)
		Rotation=(Yaw=32768)
		Scale3D=(X=0.3,Y=0.3,Z=0.3)
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=StaticMeshComponent1
	PickupFactoryMesh=StaticMeshComponent1

	WeaponFireSnd[0]=SoundCue'A_Weapon.Translocator.Cue.A_Weapon_Trans_Fire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon.Translocator.Cue.A_Weapon_Trans_Recall_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon.Translocator.Cue.A_Weapon_Trans_Raise_Cue'

	TransRecalledSound=SoundCue'A_Weapon.Translocator.Cue.A_Weapon_Trans_Recall_Cue'

	WeaponFireTypes(0)=EWFT_Projectile
 	WeaponFireTypes(1)=EWFT_Custom
	WeaponProjectiles(0)=class'UTProj_TransDisc_Content'
	WeaponProjectiles(1)=class'UTProj_TransDisc_Content'

	ArmsAnimSet=AnimSet'WP_Translocator.Anims.K_WP_Translocator_1P_Arms'
	
	WeaponIdleAnims(0)=WeaponIdle
	EmptyIdleAnim=WeaponIdleEmpty
	ArmIdleAnims(0)=WeaponIdle
	WeaponFireAnim(0)=WeaponFire
	WeaponFireAnim(1)=none
	ArmFireAnim(0)=WeaponFire
	ArmFireAnim(1)=none

	FailedTranslocationDamageClass=class'UTDmgType_FailedTranslocation'

	FireInterval(0)=+0.25
	FireInterval(1)=+0.25

	FireOffset=(X=15,Y=8,Z=-10)

	AIRating=-1.0
	CurrentRating=-1.0
	BotRefireRate=0.7
	bInstantHit=false
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0

	InventoryGroup=0
	GroupWeight=0.6

	AmmoCount=6
	MaxAmmoCount=6

	RechargeRate=1.25

	ShotCost(0)=0
	ShotCost(1)=1

	bCanThrow=false

	CrossHairCoordinates=(U=0,V=0,UL=64,VL=64)

	WeaponPutDownAnim=none
	WeaponEquipAnim=none

	AmmoDisplayType=EAWDS_Both
}
