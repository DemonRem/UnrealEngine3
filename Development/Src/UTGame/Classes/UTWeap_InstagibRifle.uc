/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_InstagibRifle extends UTWeapon
	HideDropDown;

var array<MaterialInterface> TeamSkins;
var array<ParticleSystem> TeamMuzzleFlashes;

//-----------------------------------------------------------------
// AI Interface

function float GetAIRating()
{
	return AIRating;
}

function float RangedAttackTime()
{
	return 0;
}

simulated function SetSkin(Material NewMaterial)
{
	local int TeamIndex;

	if ( NewMaterial == None ) 	// Clear the materials
	{
		if ( Instigator != None )
		{
			TeamIndex = Instigator.GetTeamNum();
		}
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

simulated function AttachWeaponTo(SkeletalMeshComponent MeshCpnt, optional name SocketName)
{
	local int TeamIndex;

	TeamIndex = Instigator.GetTeamNum();
	if (TeamIndex > TeamMuzzleFlashes.length)
	{
		TeamIndex = 0;
	}
	MuzzleFlashPSCTemplate = TeamMuzzleFlashes[TeamIndex];

	Super.AttachWeaponTo(MeshCpnt, SocketName);
}

defaultproperties
{
	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashDuration=0.33;

	WeaponColor=(R=160,G=0,B=255,A=255)
	FireInterval(0)=+1.1
	FireInterval(1)=+1.1
	InstantHitMomentum(0)=+100000.0
	FireOffset=(X=20,Y=5)
	PlayerViewOffset=(X=17,Y=10.0,Z=-8.0)
	bCanThrow=false

	// Weapon SkeletalMesh
	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_ShockRifle.Mesh.SK_WP_ShockRifle_1P'
		AnimSets(0)=AnimSet'WP_ShockRifle.Anim.K_WP_ShockRifle_1P_Base'
		Animations=MeshSequenceA
		Rotation=(Yaw=-16384)
		FOV=60.0
	End Object
	AttachmentClass=class'UTGame.UTAttachment_InstagibRifle'

	InstantHitDamage(0)=1000
	InstantHitDamage(1)=1000

	InstantHitDamageTypes(0)=class'UTDmgType_Instagib'
	InstantHitDamageTypes(1)=class'UTDmgType_Instagib'

	WeaponFireAnim(0)=WeaponFireInstigib
	WeaponFireAnim(1)=WeaponFireInstigib

	WeaponFireSnd[0]=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_InstagibFireCue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_InstagibFireCue'
	WeaponEquipSnd=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_RaiseCue'
	WeaponPutDownSnd=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_LowerCue'

	AIRating=+1.0
	CurrentRating=+1.0
	BotRefireRate=0.7
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=true
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=4
	GroupWeight=0.5

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Shock_Cue'

	ShotCost(0)=0
	ShotCost(1)=0

	FireShake(0)=(OffsetMag=(X=-8.0,Y=0.00,Z=0.00),OffsetRate=(X=-600.0,Y=0.0,Z=0.0),OffsetTime=3.2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=-8.0,Y=0.00,Z=0.00),OffsetRate=(X=-600.0,Y=0.0,Z=0.0),OffsetTime=3.2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)

	IconX=400
	IconY=129
	IconWidth=22
	IconHeight=48
	IconCoordinates=(U=357,V=157,UL=84,VL=50)

	TeamSkins[0]=MaterialInterface'WP_ShockRifle.Materials.M_WP_ShockRifle_Instagib_Red'
	TeamSkins[1]=MaterialInterface'WP_ShockRifle.Materials.M_WP_ShockRifle_Instagib_Blue'
	TeamMuzzleFlashes[0]=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_MF_Red'
	TeamMuzzleFlashes[1]=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_MF_Blue'
	CrossHairCoordinates=(U=320,V=0,UL=64,VL=64)
}
