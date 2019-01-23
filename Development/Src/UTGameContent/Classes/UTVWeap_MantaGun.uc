/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_MantaGun extends UTVehicleWeapon
	HideDropDown;

simulated function DrawAmmoCount( Hud H )
{
	local UTHud UTH;
	local float x,y;
	local float Power;

	Power = MyVehicle.GetChargePower();

	UTH = UTHud(H);

	if ( UTH != None )
	{
		x = UTH.Canvas.ClipX - (5 * (UTH.Canvas.ClipX / 1024));
		y = UTH.Canvas.ClipY - (5 * (UTH.Canvas.ClipX / 1024));
		UTH.DrawChargeWidget(X, Y, IconX, IconY, IconWidth, IconHeight, Power, true);
	}
}

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_MantaBolt'
	WeaponFireTypes(1)=EWFT_None

	WeaponFireSnd[0]=SoundCue'A_Weapon.LinkGun.Cue.A_Weapon_Link_Fire_Cue'

	FireInterval(0)=+0.2
	ShotCost(0)=0
	ShotCost(1)=0
	FireTriggerTags=(MantaWeapon01,MantaWeapon02)
}
