/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_NightshadeGun_Content extends UTVWeap_NightshadeGun;

defaultproperties
{
	FireInterval(0)=+0.16
	FireInterval(1)=+0.4
	FiringStatesArray(0)=WeaponBeamFiring
	WeaponFireTypes(0)=EWFT_InstantHit
	WeaponFireTypes(1)=EWFT_Custom
	InstantHitDamage(0)=85
	InstantHitDamageTypes(0)=class'UTDmgType_NightshadeBeam'

	WeaponFireSnd(0)=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_AltFireQueue1_Cue'

	AIRating=+0.71
	CurrentRating=+0.71
	BotRefireRate=0.99
	bInstantHit=true
	ShouldFireOnRelease(0)=0
	InventoryGroup=5
	GroupWeight=0.5

	WeaponRange=900
	MomentumTransfer=50000.0

	AltFireModeChangeSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_AltModeChange_Cue'

	DeployableList(0)=(DeployableClass=class'UTDeployableSpiderMineTrap',MaxCnt=3,DropOffset=(x=30))
	DeployableList(1)=(DeployableClass=class'UTDeployableSlowVolume',MaxCnt=1,DropOffset=(X=500.0,Y=200.0))
	DeployableList(2)=(DeployableClass=class'UTDeployableTeleportBeacon',MaxCnt=1,DropOffset=(x=30))
	DeployableList(3)=(DeployableClass=class'UTDeployableEMPMine',MaxCnt=2,DropOffset=(x=30))

	LinkFlexibility=0.5 // determines how easy it is to maintain a link.
				// 1=must aim directly at linkee, 0=linkee can be 90 degrees to either side of you
	LinkBreakDelay=0.5   // link will stay established for this long extra when blocked (so you don't have to worry about every last tree getting in the way)

	IconCoords(0)=(U=273,V=159,UL=84,VL=50)
	IconCoords(1)=(U=273,V=107,UL=84,VL=50)
	IconCoords(2)=(U=273,V=107,UL=84,VL=50)
	IconCoords(3)=(U=273,V=107,UL=84,VL=50)
}
