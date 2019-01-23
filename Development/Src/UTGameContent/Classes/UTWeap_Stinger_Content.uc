/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Stinger_Content extends UTWeap_Stinger;


defaultproperties
{
	WeaponColor=(R=255,G=255,B=0,A=255)
	PlayerViewOffset=(X=0,Y=2.0,Z=0.0)

	// Muzzle Flashes

	Begin Object Class=ParticleSystemComponent Name=MuzzleFlashComponent
		bAutoActivate=FALSE
		Template=particleSystem'WP_Stinger.Particles.P_Stinger_MF_Primary'
		DepthPriorityGroup=SDPG_Foreground
	End Object
	PrimaryMuzzleFlashPSC=MuzzleFlashComponent

	// Weapon SkeletalMesh / Anims

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_Stinger.Mesh.SK_WP_Stinger_1P'
		AnimSets(0)=AnimSet'WP_Stinger.Anims.K_WP_Stinger_1P_Base'
		Animations=MeshSequenceA
		Translation=(X=0.0,Y=0.0,Z=0.0)
		Scale=1.0
		FOV=65
	End Object
	AttachmentClass=class'UTGameContent.UTAttachment_Stinger'

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		StaticMesh=StaticMesh'WP_Stinger.Mesh.S_WP_Stinger_3P'
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Translation=(X=0.0,Y=0.0,Z=-10.0)
		Rotation=(Yaw=32768)
		Scale=0.75
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=StaticMeshComponent1
	PickupFactoryMesh=StaticMeshComponent1

	EffectSockets=(MuzzleFlashSocket,MuzzleFlashSocket)

	FiringStatesArray(0)=WeaponWindUp
	WeaponFireTypes(0)=EWFT_InstantHit
	FireInterval(0)=+0.1
	WindUpTime(0)=1.0
	WindDownTime(0)=0.27
	Spread(0)=0.0675
	InstantHitDamage(0)=14
	InstantHitDamageTypes(0)=class'UTDmgType_StingerBullet'

	FiringStatesArray(1)=WeaponFiring
	WeaponFireTypes(1)=EWFT_Projectile
	WeaponProjectiles(1)=class'UTProj_StingerShard'
	FireInterval(1)=+0.28
	WindUpTime(1)=0.33
	WindDownTime(1)=0.33

	FireOffset=(X=19,Y=10,Z=-10)

	WeaponSpinUpSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStartCue'
	WeaponSpinUpSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStartCue'
	WeaponSpinDownSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStopCue'
	WeaponSpinDownSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStopCue'
	WeaponWarmUpShotSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireCue'
	WeaponFireSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireLoopCue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireAltCue'
	WeaponPutDownSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_LowerCue'
	WeaponEquipSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_RaiseCue'

			

	Begin Object Class=AudioComponent Name=SpinSound
		bAutoPlay=false
		bStopWhenOwnerDestroyed=true
		bShouldRemainActiveIfDropped=true
		SoundCue=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindLoopCue'
	End Object

	SpinningBarrelSound=SpinSound
	Components.Add(SpinSound)

	MaxDesireability=0.73
	AIRating=+0.71
	CurrentRating=+0.71
	BotRefireRate=0.99
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=6
	GroupWeight=0.52
	AimError=850

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Stinger_Cue'

	AmmoCount=100
	LockerAmmoCount=150
	MaxAmmoCount=300

	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=-4.0,Y=0.0,Z=-4.0),OffsetRate=(X=1000.0,Y=0.0,Z=1000.0),OffsetTime=2,RotMag=(X=100.0,Y=0.0,Z=0.0),RotRate=(X=1000.0,Y=0.0,Z=0.0),RotTime=2)

	IconX=273
	IconY=413
	IconWidth=84
	IconHeight=50

	EquipTime=+0.6
	AimingHelpModifier=0.7

	MuzzleFlashLightClass=class'UTGameContent.UTStingerMuzzleFlashLight'
	CrossHairCoordinates=(U=448,V=0,UL=64,VL=64)
	LockerRotation=(Pitch=0,Roll=-16384)
	LockerOffset=(X=0.0,Z=20.0)

	IconCoordinates=(U=273,V=413,UL=84,VL=50)

	ArmsAnimSet=AnimSet'WP_Stinger.Anims.K_WP_Stinger_1P_Arms'

	WeaponFireAnim(0)=none
	WeaponFireAnim(1)=none
	ArmFireAnim(0)=none
	ArmFireAnim(1)=none

	WeaponSpinUpAnims(0)=WeaponRampUp
	WeaponSpinUpAnims(1)=WeaponRampUp
	WeaponSpinDownAnims(0)=WeaponRampDown
	WeaponSpinDownAnims(1)=WeaponRampDown
	WeaponFireAnims(0)=WeaponFire
	WeaponFireAnims(1)=WeaponFire-Secondary

	ArmSpinUpAnims(0)=WeaponRampUp
	ArmSpinUpAnims(1)=WeaponRampUp
	ArmSpinDownAnims(0)=WeaponRampDown
	ArmSpinDownAnims(1)=WeaponRampDown
	ArmFireAnims(0)=WeaponFire
	ArmFireAnims(1)=WeaponFire-Secondary

	MuzzleFlashPSCTemplate=WP_Stinger.Particles.P_Stinger_MF_Primary_WarmUP
	MuzzleFlashAltPSCTemplate=WP_Stinger.Particles.P_Stinger_MF_Alt_Fire

	QuickPickGroup=4
	QuickPickWeight=0.9
}

