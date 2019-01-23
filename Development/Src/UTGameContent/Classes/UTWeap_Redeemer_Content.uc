/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Redeemer_Content extends UTWeap_Redeemer;


defaultproperties
{
	WeaponColor=(R=255,G=0,B=0,A=255)
	FireInterval(0)=+0.9
	FireInterval(1)=+0.95
//	PlayerViewOffset=(X=5,Y=1,Z=-10.0)
	PlayerViewOffset=(X=60,Y=2,Z=-27.0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
//		SkeletalMesh=SkeletalMesh'WP_Redeemer.Mesh.SK_WP_Redeemer_1P'
		SkeletalMesh=SkeletalMesh'WP_Redeemer.Mesh.SK_WP_Redeemer_3P_Mk2'
		PhysicsAsset=None
		FOV=60
//		AnimSets(0)=AnimSet'WP_Redeemer.Anims.K_WP_Redeemer_1P_Base'
//		Animations=MeshSequenceA
	End Object
	AttachmentClass=class'UTGameContent.UTAttachment_Redeemer'

	// Pickup staticmesh
	Begin Object Class=SkeletalMeshComponent Name=ThirdPersonMesh
		SkeletalMesh=SkeletalMesh'WP_Redeemer.Mesh.S_WP_Redeemer_3P'
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Scale=0.5
		bUpdateSkelWhenNotRendered=false
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=ThirdPersonMesh
	PickupFactoryMesh=ThirdPersonMesh

	WeaponFireSnd[0]=SoundCue'A_Weapon.Redeemer.Cue.A_Weapon_Redeemer_Fire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon.Redeemer.Cue.A_Weapon_Redeemer_Fire_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon.Redeemer.Cue.A_Weapon_Redeemer_Raise_Cue'

	WeaponFireTypes(0)=EWFT_Projectile
	WeaponFireTypes(1)=EWFT_Projectile

	WeaponProjectiles(0)=class'UTProj_Redeemer'
	WeaponProjectiles(1)=class'UTProj_Redeemer'

	FireOffset=(X=15,Y=5)

	MaxDesireability=1.5
	AIRating=+0.78
	CurrentRating=+0.78
	BotRefireRate=0.99
	bInstantHit=false
	bSplashJump=true
	bSplashDamage=true
	bRecommendSplashDamage=true
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=1
	InventoryGroup=10
	GroupWeight=0.5

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Redeemer_Cue'

	AmmoCount=1
	LockerAmmoCount=1
	MaxAmmoCount=1
	RespawnTime=120.0
	bSuperWeapon=true
	bDelayedSpawn=true

	FireShake(0)=(OffsetMag=(X=-20.0,Y=0.00,Z=0.00),OffsetRate=(X=-1000.0,Y=0.0,Z=0.0),OffsetTime=2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=-20.0,Y=0.00,Z=0.00),OffsetRate=(X=-1000.0,Y=0.0,Z=0.0),OffsetTime=2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=WP_RocketLauncher.Effects.P_WP_RockerLauncher_Muzzle_Flash
	MuzzleFlashColor=(R=200,G=64,B=64,A=255)
	MuzzleFlashDuration=0.33;

	bNeverForwardPendingFire=true

	EquipTime=+0.9
	PutDownTime=+0.45
	CrossHairCoordinates=(U=320,V=64,UL=64,VL=64)

	WeaponPutDownAnim=none
	WeaponEquipAnim=none

	WarHeadClass=class'UTRemoteRedeemer_Content'

}
