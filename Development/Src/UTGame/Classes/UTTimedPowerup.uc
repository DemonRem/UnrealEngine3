/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTTimedPowerup extends UTInventory
	native
	abstract;

/** the amount of time remaining before the powerup expires (automatically decremented in C++ tick)
 * @note: only counts down while the item is owned by someone (not when on a dropped pickup)
 */
var databinding	float TimeRemaining;

/** Used to determine which symbol represents this object on the paperdoll */
var int HudIndex;

/** Sound played when powerup's time is up */
var SoundCue PowerupOverSound;
cpptext
{
	virtual void TickSpecial(FLOAT DeltaTime);
}

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	Super.GivenTo(NewOwner, bDoNotActivate);

	ClientSetTimeRemaining(TimeRemaining);
}

/** called by the server on the client to tell it how much time the UDamage has for the HUD timer */
reliable client function ClientSetTimeRemaining(float NewTimeRemaining)
{
	TimeRemaining = NewTimeRemaining;
}

function bool DenyPickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	local DroppedPickup Drop;

	if (ItemClass == Class)
	{
		Drop = DroppedPickup(Pickup);
		if (Drop != None && UTTimedPowerup(Drop.Inventory) != None)
		{
			TimeRemaining += UTTimedPowerup(Drop.Inventory).TimeRemaining;
		}
		else
		{
			TimeRemaining += default.TimeRemaining;
		}
		ClientSetTimeRemaining(TimeRemaining);
		Pickup.PickedUpBy(Instigator);
		AnnouncePickup(Instigator);
		return true;
	}

	return false;
}

/** called when TimeRemaining reaches zero */
event TimeExpired()
{
	if(PowerUpOverSound != none)
	{
		Instigator.PlaySound(PowerupOverSound);
	}
	Destroy();
}

static function float BotDesireability(Actor PickupHolder, Pawn P)
{
	return default.MaxDesireability;
}

static function float DetourWeight(Pawn Other, float PathWeight)
{
	return (default.MaxDesireability / PathWeight);
}

defaultproperties
{
	bPredictRespawns=true
	bDelayedSpawn=true
	bDropOnDeath=true
	RespawnTime=90.000000
	MaxDesireability=2.0

	TimeRemaining=30.0
}
