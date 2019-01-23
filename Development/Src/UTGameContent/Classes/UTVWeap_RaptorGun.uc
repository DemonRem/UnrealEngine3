/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_RaptorGun extends UTVehicleWeapon
	HideDropDown;

simulated function Projectile ProjectileFire()
{
	local UTProj_RaptorRocket Rocket;
	local float BestAim, BestDist;
	local UTVehicle AimTarget;
	local vector RocketSocketL;
	local rotator RocketSocketR;

	Rocket = UTProj_RaptorRocket(Super.ProjectileFire());

	if (Rocket != None && MyVehicle != None && MyVehicle.Mesh != None)
	{
		MyVehicle.Mesh.GetSocketWorldLocationAndRotation('RocketSocket',RocketSocketL, RocketSocketR);
		AimTarget = UTVehicle(Instigator.Controller.PickTarget(class'UTHoverVehicle', BestAim, BestDist, vector(RocketSocketR), RocketSocketL, LockRange));
		if (AimTarget != None && AimTarget.bHomingTarget)
		{
			Rocket.Seeking = AimTarget;
		}
	}

	return Rocket;
}

simulated function float MaxRange()
{
	local float Range;

	// ignore missles for the range calculation
	if (bInstantHit)
	{
		Range = GetTraceRange();
	}
	if (WeaponProjectiles[0] != None)
	{
		Range = FMax(Range, WeaponProjectiles[0].static.GetRange());
	}
	return Range;
}

function class<Projectile> GetProjectileClass()
{
	if ( CurrentFireMode == 0 && Instigator.GetTeamNum() == 0 )
	{
		return class'UTProj_RaptorBoltRed';
	}
	else
	{
		return Super.GetProjectileClass();
	}
}

function byte BestMode()
{
	local UTBot Bot;

	Bot = UTBot(Instigator.Controller);
	if (Bot != None && UTVehicle(Bot.Enemy) != None && UTVehicle(Bot.Enemy).bHomingTarget && (FRand() < 0.3 + 0.1 * Bot.Skill))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

function float RefireRate()
{
	// reduce refire rate with the missle altfire
	return (CurrentFireMode == 0) ? BotRefireRate : (BotRefireRate * 0.5);
}

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_RaptorBolt'
	WeaponFireTypes(1)=EWFT_Projectile
	WeaponProjectiles(1)=class'UTProj_RaptorRocket'

	WeaponFireSnd[0]=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_Fire'
	WeaponFireSnd[1]=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_AltFire'
	FireInterval(0)=+0.2
	FireInterval(1)=+1.2
	ShotCost(0)=0
	ShotCost(1)=0

	FireTriggerTags=(RaptorWeapon01,RaptorWeapon02)

	LockAim=0.996
	LockRange=15000.0

	BotRefireRate=0.95
}
