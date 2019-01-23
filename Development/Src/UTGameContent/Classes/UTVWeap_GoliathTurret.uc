/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_GoliathTurret extends UTVehicleWeapon
	HideDropDown;

simulated function Projectile ProjectileFire()
{
	local Projectile P;
	local vector ForceLoc;

	P = Super.ProjectileFire();
	if ( (Role==ROLE_Authority) && (P != None) )
	{
		// apply force to vehicle
		ForceLoc = MyVehicle.GetTargetLocation();
		ForceLoc.Z += 90;
		MyVehicle.Mesh.AddImpulse(-0.5*P.Velocity, ForceLoc);
	}
	return P;
}

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_TankShell'
 	WeaponFireTypes(1)=EWFT_None

	WeaponFireSnd(0)=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Fire'

	FireInterval(0)=+2.5
	FireInterval(1)=+2.5
	ShotCost(0)=0
	ShotCost(1)=0

	FireTriggerTags=(GoliathTurret)

	FireCameraAnim[0]=CameraAnim'VH_Goliath.PrimaryFireViewShake'

	Spread[0]=0.015
	Spread[1]=0.015

	bZoomedFireMode(1)=1

	ZoomedTargetFOV=40.0
	ZoomedRate=60.0
}
