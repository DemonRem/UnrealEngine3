/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_LeviathanTurretBase extends UTVehicleWeapon
	abstract;

/** how long the shield lasts */
var float ShieldDuration;
/** how long after using the shield before it can be used again */
var float ShieldRecharge;
/** next time shield can be used */
var float ShieldAvailableTime;
/** AI flag */
var bool bPutShieldUp;

/** whether shield is currently up (can't state scope because can fire both modes simultaneously) */
var bool bShieldActive;

simulated function float GetPowerPerc()
{
	if (bShieldActive)
	{
		return 1.0 - GetTimerCount('DeactivateShield') / ShieldDuration;
	}
	else
	{
		return 1.0 - (ShieldAvailableTime - WorldInfo.TimeSeconds) / ShieldRecharge;
	}
}

//@FIXME: needs to be rewritten for new shielding setup
/*
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

//@FIXME: needs to be verified with new shielding setup
function ShieldAgainstIncoming(optional Projectile P)
{
	local AIController Bot;

	Bot = AIController(Instigator.Controller);
	if (Bot != None)
	{
		if (P != None)
		{
			if (GetTimerRate('RefireCheckTimer') - GetTimerCount('RefireCheckTimer') <= VSize(P.Location - MyVehicle.Location) - 1100.f / VSize(P.Velocity))
			{
				// put shield up if pointed in right direction
				if (Bot.Skill < 5.f || (Normal(P.Location - MyVehicle.GetPhysicalFireStartLoc(self)) dot vector(MyVehicle.GetWeaponAim(self))) >= 0.7)
				{
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
*/

function NotifyShieldHit(int Damage);

simulated function bool HasAmmo(byte FireModeNum, optional int Amount)
{
	return (FireModeNum == 1) ? (WorldInfo.TimeSeconds >= ShieldAvailableTime) : Super.HasAmmo(FireModeNum, Amount);
}

simulated function BeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		if (WorldInfo.TimeSeconds >= ShieldAvailableTime)
		{
			bShieldActive = true;
			MyVehicle.SetShieldActive(SeatIndex, true);
			ShieldAvailableTime = WorldInfo.TimeSeconds + ShieldDuration + ShieldRecharge;
			SetTimer(ShieldDuration, false, 'DeactivateShield');
		}
	}
	else
	{
		Super.BeginFire(FireModeNum);
	}
}

simulated function DeactivateShield()
{
	bShieldActive = false;
	MyVehicle.SetShieldActive(SeatIndex, false);
}

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit
 	WeaponFireTypes(1)=EWFT_None

	AmmoDisplayType=EAWDS_BarGraph

	ShotCost(0)=0
	bInstantHit=true
	ShotCost(1)=1

	BotRefireRate=0.99

	ShieldDuration=4.0
	ShieldRecharge=5.0
}
