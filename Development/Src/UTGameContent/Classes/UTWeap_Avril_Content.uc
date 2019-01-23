/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Avril_Content extends UTWeap_Avril;


defaultproperties
{
	WeaponColor=(R=255,G=0,B=0,A=255)
	FireInterval(0)=+4.0
	FireInterval(1)=+0.0
	PlayerViewOffset=(X=19.0,Y=7.0,Z=-18.0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_AVRiL.Mesh.SK_WP_Avril_1P'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_AVRiL.Anims.K_WP_Avril_1P_Base'
		Animations=MeshSequenceA
		Scale=0.05
	End Object
	AttachmentClass=class'UTGameContent.UTAttachment_Avril'

	// Pickup staticmesh

	// Pickup mesh Transform
	Begin Object Class=SkeletalMeshComponent Name=StaticMeshComponent1
		SkeletalMesh=SkeletalMesh'WP_AVRiL.Mesh.SK_WP_Avril_3P'
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		scale=0.05
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=StaticMeshComponent1
	PickupFactoryMesh=StaticMeshComponent1

	WeaponFireSnd[0]=SoundCue'A_Weapon.AVRiL.Cue.A_Weapon_AVRiL_Fire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon.AVRiL.Cue.A_Weapon_AVRiL_Fire_Cue'
	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_AVRiL_Cue'
	LockAcquiredSound=SoundCue'A_Weapon.AVRiL.Cue.A_Weapon_AVRiL_Lock_Cue'

 	WeaponFireTypes(0)=EWFT_Projectile
	WeaponFireTypes(1)=EWFT_None

	ShotCost(1)=0
	WeaponProjectiles(0)=class'UTProj_AvrilRocket'

	MaxDesireability=0.7
	AIRating=+0.55
	CurrentRating=+0.55
	BotRefireRate=0.5
	bInstantHit=false
	bSplashJump=true
	bSplashDamage=true
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=1
	InventoryGroup=10
	GroupWeight=0.6

	AmmoCount=5
	LockerAmmoCount=5
	MaxAmmoCount=25
	FireOffset=(X=22,Y=10)

	MinReloadPct(0)=0.15
	MinReloadPct(1)=0.15

	bZoomedFireMode(0)=0
	bZoomedFireMode(1)=1

	ZoomedTargetFOV=40.0
	ZoomedRate=200

	FireShake(0)=(OffsetMag=(X=-20.0,Y=0.00,Z=0.00),OffsetRate=(X=-1000.0,Y=0.0,Z=0.0),OffsetTime=2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)

	WeaponLockType=WEAPLOCK_Constant

	LockAim=0.996
	LockChecktime=0.05
	LockRange=15000
	LockAcquireTime=0.05
	LockTolerance=0.4

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_AVRiL.Particles.P_WP_Avril_MuzzleFlash'
	MuzzleFlashDuration=0.33

	IconX=460
	IconY=343
	IconWidth=29
	IconHeight=47

	EquipTime=0.75
	PutDownTime=0.45
	CrossHairCoordinates=(U=384,V=64,UL=64,VL=64)

	QuickPickGroup=7
	QuickPickWeight=0.9

}
