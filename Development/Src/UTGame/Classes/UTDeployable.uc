/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeployable extends UTWeapon
	abstract
	native;

/** the factory that spawned this deployable */
var UTDeployablePickupFactory Factory;

/** class of deployable actor to spawn */
var class<UTDeployedActor> DeployedActorClass;

/** message given to player when deploying fails */
var localized string FailedDeployMessage;

/** Toss strength when throwing out deployable */
var float TossMag;

/** Scale for deployable when being scaled for a 3p preview, such as in a stealth vehicle*/
var float PreviewScale3p;

/** if true, factory delays respawn countdown until this deployable is used */
var bool bDelayRespawn;

function bool DenyPickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	// players can only carry one deployable at a time
	return ClassIsChildOf(ItemClass, class'UTDeployable');
}

static function InitPickupMesh(PrimitiveComponent InMesh);

static function float BotDesireability(Actor PickupHolder, Pawn P)
{
	return (P.FindInventoryType(class'UTDeployable') != None) ? -1.0 : Super.BotDesireability(PickupHolder, P);
}

static function class<Actor> GetTeamDeployable(int TeamNum)
{
	return default.DeployedActorClass;
}

/** Recommend an objective for player carrying this deployable */
function UTGameObjective RecommendObjective(Controller C)
{
	return None;
}

/** attempts to deploy the item
 * @return whether or not deploying was successful
 */
function bool Deploy()
{
	local UTDeployedActor DeployedActor;
	local vector SpawnLocation;
	local rotator Aim, FlatAim;

	SpawnLocation = GetPhysicalFireStartLoc();
	Aim = GetAdjustedAim(SpawnLocation);
	FlatAim.Yaw = Aim.Yaw;

	DeployedActor = Spawn(DeployedActorClass, self,, SpawnLocation, FlatAim);
	if (DeployedActor != None)
	{
		if ( AmmoCount <= 0 )
		{
			if ( Factory != None )
			{
				DeployedActor.OnDeployableUsedUp = Factory.DeployableUsed;
			}
			Mesh.SetHidden(true);
		}
		DeployedActor.Velocity = TossMag * vector(Aim);
		return true;
	}
	else
	{
		return false;
	}
}

/** called when User tries to deploy us and fails for some reason */
function DeployFailed()
{
	// refund ammo
	AddAmmo(ShotCost[CurrentFireMode]);
	// call client version
	ClientDeployFailed();
}

/** called to notify client of deploy failure */
reliable client function ClientDeployFailed()
{
	local PlayerController PC;

	PC = PlayerController(Instigator.Controller);
	if (PC != None)
	{
		PC.ClientMessage(FailedDeployMessage);
	}
}

simulated function CustomFire()
{
	if ( (Role == ROLE_Authority) && !Deploy() )
	{
		DeployFailed();
	}
}

simulated event Destroyed()
{
	// make sure client finishes any in-progress switch away from this weapon
	if (Instigator != None && Instigator.Weapon == self && InvManager != None && InvManager.PendingWeapon != None)
	{
		Instigator.InvManager.ChangedWeapon();
	}

	if (Role == ROLE_Authority && AmmoCount > 0 && Factory != None)
	{
		Factory.DeployableUsed(self);
	}

	Super.Destroyed();
}

simulated function bool AllowSwitchTo(Weapon NewWeapon)
{
	return ( (AmmoCount <= 0 || (UTInventoryManager(InvManager) != None && UTInventoryManager(InvManager).bInfiniteAmmo))
		&& Super.AllowSwitchTo(NewWeapon) );
}

auto state Inactive
{
	simulated function BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		// destroy if we're out of ammo, so the player can pick up a different deployable
		if (Role == ROLE_Authority && !HasAnyAmmo())
		{
			Destroy();
		}
	}
}

/**
 * State WeaponEquipping
 * The Weapon is in this state while transitioning from Inactive to Active state.
 * Typically, the weapon will remain in this state while its selection animation is being played.
 * While in this state, the weapon cannot be fired.
 */
simulated state WeaponEquipping
{
	simulated function WeaponEquipped()
	{
		local UTPlayerController PC;

		super.WeaponEquipped();

		if ( Role == ROLE_Authority )
		{
			PC = UTPlayerController(Instigator.Controller);
			if ( PC != None )
			{
				 PC.CheckAutoObjective(true);
			}
		}
	}
}

defaultproperties
{
	WeaponFireTypes[0]=EWFT_Custom
	WeaponFireTypes[1]=EWFT_None
	ShotCost[0]=1
	AmmoCount=1
	MaxAmmoCount=1
	InventoryGroup=11
	RespawnTime=30.0
	TossMag=500.0
	bDelayRespawn=true

	bExportMenuData=false
	PreviewScale3p=1.0f
}
