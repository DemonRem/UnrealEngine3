/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_ScorpionTurret extends UTVehicleWeapon
	HideDropDown;
var class<UTProj_ScorpionGlob> TeamProjectiles[2];
simulated function Projectile ProjectileFire()
{
	if(Role==ROLE_Authority)
	{
		WeaponProjectiles[0] = TeamProjectiles[(MyVehicle.GetTeamNum()==1)?1:0];
	}
	return super.ProjectileFire();
}
defaultproperties
{
	WeaponColor=(R=64,G=255,B=64,A=255)
	PlayerViewOffset=(X=11.0,Y=7.0,Z=-9.0)

	FireInterval(0)=+0.4
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_ScorpionGlob'
	TeamProjectiles[0]=class'UTProj_ScorpionGlob_Red'
	TeamProjectiles[1]=class'UTProj_ScorpionGlob'

	WeaponFireSnd[0]=SoundCue'A_Weapon.BioRifle.Cue.A_Weapon_Bio_Fire01_Cue'

	BotRefireRate=0.99
	bInstantHit=false
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShotCost(0)=0
	ShotCost(1)=0
	Spread[0]=0.15

	FireOffset=(X=19,Y=10,Z=-10)

	IconX=382
	IconY=82
	IconWidth=27
	IconHeight=42
}
