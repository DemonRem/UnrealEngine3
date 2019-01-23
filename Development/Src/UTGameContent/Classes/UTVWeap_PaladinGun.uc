/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVWeap_PaladinGun extends UTVehicleWeapon
	HideDropDown;

var() float MaxShieldHealth;
/** how long after the last shield activation before it starts recharging again */
var() float MaxDelayTime;
var() float ShieldRechargeRate;
var float LastShieldHitTime;
/** whether or not pressing fire while alt firing will trigger the combo proximity explosion */
var bool bComboFireReady;

var float CurrentShieldHealth;
var float CurrentDelayTime;
var bool bPutShieldUp;

replication
{
	if (bNetOwner && Role == ROLE_Authority)
		CurrentShieldHealth;
}

simulated function DrawAmmoCount( Hud H )
{
	local UTHud UTH;
	local float x,y;


	UTH = UTHud(H);

	if ( UTH != None )
	{
		x = UTH.Canvas.ClipX - (5 * (UTH.Canvas.ClipX / 1024));
		y = UTH.Canvas.ClipY - (5 * (UTH.Canvas.ClipX / 1024));
		UTH.DrawChargeWidget(X, Y, IconX, IconY, IconWidth, IconHeight, CurrentShieldHealth / MaxShieldHealth, true);
	}
}


function byte BestMode()
{
	local UTBot B;

	if (CurrentShieldHealth <= 0.f)
	{
		return 0;
	}
	if (Projectile(Instigator.Controller.Focus) != None)
	{
		return 1;
	}

	B = UTBot(Instigator.Controller);
	if (B == None || B.Enemy == None)
	{
		return 0;
	}

	if (bPutShieldUp || !B.LineOfSightTo(B.Enemy))
	{
		LastShieldHitTime = WorldInfo.TimeSeconds;
		bPutShieldUp = false;
		return 1;
	}

	if (VSize(B.Enemy.Location - Location) < 900.f)
	{
		return (IsInState('ShieldActive') ? 0 : 1);
	}
	if (IsInState('ShieldActive') && WorldInfo.TimeSeconds - LastShieldHitTime < 2.f)
	{
		return 1;
	}
	else if (B.Enemy != B.Focus)
	{
		return 0;
	}
	else
	{
		// check if near friendly node, and between it and enemy
		if ( B.Squad.SquadObjective != None && VSize(B.Pawn.Location - B.Squad.SquadObjective.Location) < 1000.f
			&& (Normal(B.Enemy.Location - B.Squad.SquadObjective.Location) dot Normal(B.Pawn.Location - B.Squad.SquadObjective.Location)) > 0.7 )
		{
			return 1;
		}

		// use shield if heavily damaged
		if (B.Pawn.Health < 0.3 * B.Pawn.Default.Health)
		{
			return 1;
		}

		// use shield against heavy vehicles
		if ( UTVehicle(B.Enemy) != None && UTVehicle(B.Enemy).ImportantVehicle() && B.Enemy.Controller != None
			&& (vector(B.Enemy.Controller.Rotation) dot Normal(Instigator.Location - B.Enemy.Location)) > 0.9 )
		{
			return 1;
		}

		return 0;
	}
}

function ShieldAgainstIncoming(optional Projectile P)
{
	local AIController Bot;

	Bot = AIController(Instigator.Controller);
	if (Bot != None)
	{
		if (P != None)
		{
			if (GetTimerRate('RefireCheckTimer') - GetTimerCount('RefireCheckTimer') <= VSize(P.Location - MyVehicle.Location) - 1100.f / FMax(1.0, VSize(P.Velocity)))
			{
				// put shield up if pointed in right direction
				if (Bot.Skill < 5.f || (Normal(P.Location - MyVehicle.GetPhysicalFireStartLoc(self)) dot vector(MyVehicle.GetWeaponAim(self))) >= 0.7)
				{
					LastShieldHitTime = WorldInfo.TimeSeconds;
					bPutShieldUp = true;
					Bot.FireWeaponAt(Bot.Focus);
				}
				else if (Bot.Enemy == None || Bot.Enemy == P.Instigator)
				{
					Bot.Focus = Bot.Enemy;
					LastShieldHitTime = WorldInfo.TimeSeconds;
					bPutShieldUp = true;
					Bot.FireWeaponAt(Bot.Focus);
				}
			}
		}
		else if (Bot.Enemy != None)
		{
			if (GetTimerRate('RefireCheckTimer') - GetTimerCount('RefireCheckTimer') <= 0.2 || FRand() >= 0.6)
			{
				LastShieldHitTime = WorldInfo.TimeSeconds;
				bPutShieldUp = true;
				Bot.FireWeaponAt(Bot.Focus);
			}
		}
	}
}

function NotifyShieldHit(int Damage)
{
	CurrentShieldHealth = FMax(CurrentShieldHealth - Damage, 0.f);
	LastShieldHitTime = WorldInfo.TimeSeconds;
}

simulated event Tick(float DeltaTime)
{
	Super.Tick(DeltaTime);

	if (CurrentShieldHealth < MaxShieldHealth)
	{
		if (CurrentDelayTime < MaxDelayTime)
		{
			CurrentDelayTime += DeltaTime;
		}
		else
		{
			CurrentShieldHealth = FMin(CurrentShieldHealth + ShieldRechargeRate * DeltaTime, MaxShieldHealth);
		}
	}
}

simulated function bool HasAmmo(byte FireModeNum, optional int Amount)
{
	return (FireModeNum == 1) ? (CurrentShieldHealth > float(Amount)) : Super.HasAmmo(FireModeNum, Amount);
}

simulated state ShieldActive extends WeaponFiring
{
	simulated event Tick(float DeltaTime)
	{
		if (Role == ROLE_Authority && CurrentShieldHealth <= 0.f)
		{
			// Ran out of shield energy so turn it off
			GotoState('Active');
		}
	}

	function float RefireRate()
	{
		// bots should treat this fire mode as a repeater weapon they need to continuously fire to use
		return 0.99;
	}

	simulated function RefireCheckTimer()
	{
		if (!bComboFireReady)
		{
			// combo fire has recharged
			bComboFireReady = true;
			// reset timer to the altfire's normal rate
			SetTimer(GetFireInterval(CurrentFireMode), true, 'RefireCheckTimer');
		}

		Super.RefireCheckTimer();
	}

	simulated function BeginFire(byte FireModeNum)
	{
		Super.BeginFire(FireModeNum);

		if (FireModeNum == 0)
		{
			if (bComboFireReady)
			{
				ProjectileFire();
				SetTimer(GetFireInterval(0), false, 'RefireCheckTimer');
				bComboFireReady = false;
			}
		}
		else if (FireModeNum == 1)
		{
			MyVehicle.SetShieldActive(SeatIndex, true);
		}
	}

	function class<Projectile> GetProjectileClass()
	{
		// this only gets called here when firing both fire modes simultaneously
		// firemode 1 (the shield) will be active first, so we need to override to spawn the primary projectile
		return WeaponProjectiles[0];
	}

	simulated function BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		CurrentDelayTime = 0.0;
		bComboFireReady = true;

		MyVehicle.SetShieldActive(SeatIndex, true);
	}

	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		MyVehicle.SetShieldActive(SeatIndex, false);
	}

	simulated function EndFire(byte FireModeNum)
	{
		Super.EndFire(FireModeNum);

		if (FireModeNum == 1)
		{
			MyVehicle.SetShieldActive(SeatIndex, false);
		}
	}
}

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_PaladinEnergyBolt'
	FiringStatesArray(1)=ShieldActive
	FireInterval(0)=2.35
	FireInterval(1)=0.1
	ShotCost(0)=0
	ShotCost(1)=0
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_Fire'

	bSplashDamage=true
	bRecommendSplashDamage=true
	BotRefireRate=0.8

	FireTriggerTags=(PrimanyFire)

	MaxShieldHealth=2000.0
	CurrentShieldHealth=2000.0
	MaxDelayTime=2.0
	ShieldRechargeRate=400.0
}
