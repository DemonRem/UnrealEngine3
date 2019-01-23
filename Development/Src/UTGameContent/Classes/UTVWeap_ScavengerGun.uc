/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVWeap_ScavengerGun extends UTVWeap_MantaGun;

defaultproperties
{
	WeaponProjectiles(0)=class'UTProj_ViperBolt'
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_PrimaryFireCue'

	// ~15 degrees - since this vehicle's gun doesn't move by itself, we give some more leeway
	MaxFinalAimAdjustment=0.966
}
