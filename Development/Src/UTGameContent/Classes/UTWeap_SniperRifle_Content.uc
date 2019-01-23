/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_SniperRifle_Content extends UTWeap_SniperRifle;
var array<MaterialInstance> TeamSkins;

simulated function SetSkin(Material NewMaterial)
{
	local int TeamIndex;

	if ( NewMaterial == None ) 	// Clear the materials
	{
		TeamIndex = Instigator.GetTeamNum();
		if (TeamIndex > TeamSkins.length)
		{
			TeamIndex = 0;
		}
		Mesh.SetMaterial(0,TeamSkins[TeamIndex]);
	}
	else
	{
		Super.SetSkin(NewMaterial);
	}
}


defaultproperties
{
	WeaponColor=(R=255,G=0,B=64,A=255)
		PlayerViewOffset=(X=0,Y=0,Z=0)

		Begin Object class=AnimNodeSequence Name=MeshSequenceA
		End Object

		// Weapon SkeletalMesh
		Begin Object Name=FirstPersonMesh
		    SkeletalMesh=SkeletalMesh'WP_SniperRifle.Mesh.SK_WP_SniperRifle_1P'
			AnimSets(0)=AnimSet'WP_SniperRifle.Anims.K_WP_SniperRifle_1P_Base'
			Animations=MeshSequenceA
			Scale=1.0
			FOV=80
		End Object
		AttachmentClass=class'UTGameContent.UTAttachment_SniperRifle'

		ArmsAnimSet=AnimSet'WP_SniperRifle.Anims.K_WP_SniperRifle_1P_Arms'

		// Pickup staticmesh
		Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		    StaticMesh=StaticMesh'WP_SniperRifle.Mesh.S_WP_SniperRifle_3P'
			bOnlyOwnerSee=false
			CastShadow=false
			bForceDirectLightMap=true
			bCastDynamicShadow=false
			CollideActors=false
			bUseAsOccluder=FALSE
		End Object
		DroppedPickupMesh=StaticMeshComponent1
		PickupFactoryMesh=StaticMeshComponent1

		Begin Object Class=AudioComponent Name=ZoomingAudio
			SoundCue=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomLoop_Cue'
		End Object
		ZoomLoop=ZoomingAudio
		Components.Add(ZoomingAudio)

		InstantHitDamage(0)=70
		InstantHitDamage(1)=0
		InstantHitMomentum(0)=10000.0

		FireInterval(0)=+1.33
		FireInterval(1)=+1.33

		FiringStatesArray(1)=Active

		WeaponRange=17000

		InstantHitDamageTypes(0)=class'UTDmgType_SniperPrimary'
		InstantHitDamageTypes(1)=None

		WeaponFireSnd[0]=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Fire_Cue'
		//WeaponFireSnd[1]=SoundCue'A_Weapon.SniperRifle.Cue.A_Weapon_SN_Fire01_Cue'
		WeaponEquipSnd=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Raise_Cue'
		WeaponPutDownSnd=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Lower_Cue'
		ZoomOutSound=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomOut_Cue'
		ZoomInSound=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomIn_Cue'

		WeaponFireTypes(0)=EWFT_InstantHit

		MaxDesireability=0.63
		AIRating=+0.7
		CurrentRating=+0.7
		BotRefireRate=0.7
		bInstantHit=true
		bSplashJump=false
		bSplashDamage=false
		bRecommendSplashDamage=false
		bSniping=true
		ShouldFireOnRelease(0)=0
		ShouldFireOnRelease(1)=0
		InventoryGroup=9
		GroupWeight=0.5
		AimError=850

		PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Sniper_Cue'

		AmmoCount=10
		LockerAmmoCount=10
		MaxAmmoCount=40

		HeadShotDamageType=class'UTDmgType_SniperHeadShot'
		HeadShotDamageMult=2.0
		SlowHeadshotScale=1.75
		RunningHeadshotScale=0.8

		// Configure the zoom

		bZoomedFireMode(0)=0
		bZoomedFireMode(1)=1

		ZoomedTargetFOV=12.0
		ZoomedRate=60.0

		FadeTime=0.3

		FireShake(0)=(OffsetMag=(X=-15.0,Y=0.0,Z=10.0),OffsetRate=(X=-4000.0,Y=0.0,Z=4000.0),OffsetTime=1.6,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)
		HudMaterial=Texture2D'WP_SniperRifle.Textures.T_SniperCrosshair'

		MuzzleFlashSocket=MuzzleFlashSocket
		MuzzleFlashPSCTemplate=ParticleSystem'WP_SniperRifle.Effects.P_WP_SniperRifle_MuzzleFlash'
		MuzzleFlashDuration=0.33
		MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'

		IconX=451
		IconY=448
		IconWidth=54
		IconHeight=51

		EquipTime=+0.6
		PutDownTime=+0.45
		CrossHairCoordinates=(U=256,V=64,UL=64,VL=64)

		QuickPickGroup=5
		QuickPickWeight=0.8

		TeamSkins[0]=MaterialInstance'WP_SniperRifle.Materials.M_WP_SniperRifle'
		TeamSkins[1]=MaterialInstance'WP_SniperRifle.Materials.M_WP_SniperRifle_Blue'

}
