/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_SPMACannon_Content extends UTVWeap_SPMACannon
	hidedropdown;

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponFireSnd(0)=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Fire'
	WeaponProjectiles(0)=class'UTProj_SPMAShell_Content'
	FireInterval(0)=+4.0

	WeaponFireTypes(1)=EWFT_Projectile
	WeaponFireSnd(1)=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPAM_CameraLaunch'
	WeaponProjectiles(1)=class'UTProj_SPMACamera_Content'
	FireInterval(1)=+1.5

	FireTriggerTags=(CannonFire)
	AltFireTriggertags=(CameraFire)

	BoomSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_DistantSPMA'
}
