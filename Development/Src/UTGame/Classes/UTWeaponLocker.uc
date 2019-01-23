/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeaponLocker extends UTPickupFactory
	native;

struct native WeaponEntry
{
	var() class<UTWeapon> WeaponClass;
	var PrimitiveComponent PickupMesh;
};
var() array<WeaponEntry> Weapons;

/** when received on the client, replaces Weapons array classes with this array instead */
struct native ReplacementWeaponEntry
{
	/** indicates whether this entry in the Weapons array was actually replaced
	 * (so we can distinguish between WeaponClass == None because it wasn't touched vs. because it was removed
	 */
	var bool bReplaced;
	/** the class of weapon to replace with */
	var class<UTWeapon> WeaponClass;
};
var repnotify ReplacementWeaponEntry ReplacementWeapons[6];

/** offsets from locker location where we can place weapon meshes */
var array<vector> LockerPositions;

var localized string LockerString;

struct native PawnToucher
{
	var Pawn P;
	var float NextTouchTime;
};
var array<PawnToucher> Customers;

/** clientside flag - whether the locker should be displayed as active and having weapons available */
var bool bIsActive;
/** clientside flag - whether or not a local player is near this locker */
var bool bPlayerNearby;
/** how close a player needs to be to be considered nearby */
var float ProximityDistance;

/** component for the active/inactive effect, depending on state */
var ParticleSystemComponent AmbientEffect;
/** effect that's visible when active and the player gets nearby */
var ParticleSystemComponent ProximityEffect;
/** effect for when the weapon locker cannot be used by the player right now */
var ParticleSystem InactiveEffectTemplate;
/** effect for when the weapon locker is usable by the player right now */
var ParticleSystem ActiveEffectTemplate;
/** effect played over weapons being scaled in when the player is nearby */
var ParticleSystem WeaponSpawnEffectTemplate;

/** the rate to scale the weapons's Scale3D.X when spawning them in (set to 0.0 to disable the scaling) */
var float ScaleRate;

cpptext
{
	virtual void TickSpecial(FLOAT DeltaTime);
}

replication
{
	if (bNetInitial)
		ReplacementWeapons;
}

// Called after PostBeginPlay.
simulated event SetInitialState()
{
	if ( bIsDisabled )
	{
		GotoState('Disabled');
	}
	else
	{
		Super(Actor).SetInitialState();
	}
}

/* ShouldCamp()
Returns true if Bot should wait for me
*/
function bool ShouldCamp(UTBot B, float MaxWait)
{
	return false;
}

function bool AddCustomer(Pawn P)
{
	local int			i;
	local PawnToucher	PT;

	if ( UTInventoryManager(P.InvManager) == None )
		return false;

	if ( Customers.Length > 0 )
		for ( i=0; i<Customers.Length; i++ )
		{
			if ( Customers[i].NextTouchTime < WorldInfo.TimeSeconds )
			{
				if ( Customers[i].P == P )
				{
					Customers[i].NextTouchTime = WorldInfo.TimeSeconds + 30;
					return true;
				}
				Customers.Remove(i,1);
				i--;
			}
			else if ( Customers[i].P == P )
				return false;
		}

	PT.P = P;
	PT.NextTouchTime = WorldInfo.TimeSeconds + 30;
	Customers[Customers.Length] = PT;
	return true;
}

function bool HasCustomer(Pawn P)
{
	local int i;

	if ( Customers.Length > 0 )
		for ( i=0; i<Customers.Length; i++ )
		{
			if ( Customers[i].NextTouchTime < WorldInfo.TimeSeconds )
			{
				if ( Customers[i].P == P )
					return false;
				Customers.Remove(i,1);
				i--;
			}
			else if ( Customers[i].P == P )
				return true;
		}

	return false;
}

simulated function PostBeginPlay()
{
	if ( bIsDisabled )
	{
		return;
	}
	Super.PostBeginPlay();

	InitializeWeapons();
}

/** initialize properties/display for the weapons that are listed in the array */
simulated function InitializeWeapons()
{
	local int i;

	// clear out null entries
	for (i = 0; i < Weapons.length && i < LockerPositions.length; i++)
	{
		if (Weapons[i].WeaponClass == None)
		{
			Weapons.Remove(i, 1);
			i--;
		}
	}

	// initialize weapons
	MaxDesireability = 0;
	for (i = 0; i < Weapons.Length; i++)
	{
		MaxDesireability += Weapons[i].WeaponClass.Default.AIRating;
	}
}

simulated event ReplicatedEvent(name VarName)
{
	local int i;

	if (VarName == 'ReplacementWeapons')
	{
		for (i = 0; i < ArrayCount(ReplacementWeapons); i++)
		{
			if (ReplacementWeapons[i].bReplaced)
			{
				if (i >= Weapons.length)
				{
					Weapons.length = i + 1;
				}
				Weapons[i].WeaponClass = ReplacementWeapons[i].WeaponClass;
				if (Weapons[i].PickupMesh != None)
				{
					DetachComponent(Weapons[i].PickupMesh);
					Weapons[i].PickupMesh = None;
				}
			}
		}
		InitializeWeapons();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** replaces an entry in the Weapons array (generally used by mutators) */
function ReplaceWeapon(int Index, class<UTWeapon> NewWeaponClass)
{
	if (Index >= 0)
	{
		if (Index >= Weapons.length)
		{
			Weapons.length = Index + 1;
		}
		Weapons[Index].WeaponClass = NewWeaponClass;
		if (Index < ArrayCount(ReplacementWeapons))
		{
			ReplacementWeapons[Index].bReplaced = true;
			ReplacementWeapons[Index].WeaponClass = NewWeaponClass;
		}
	}
}

simulated static function UpdateHUD(UTHUD H)
{
	Super.UpdateHUD(H);
	H.LastWeaponPickupTime = H.LastPickupTime;
}

function Reset()
{
	Super(NavigationPoint).Reset();
}

simulated function String GetHumanReadableName()
{
	return LockerString;
}

// tell the bot how much it wants this weapon pickup
// called when the bot is trying to decide which inventory pickup to go after next
function float BotDesireability(Pawn Bot)
{
	local UTWeapon AlreadyHas;
	local float desire;
	local int i;

	if ( bHidden || HasCustomer(Bot) )
		return 0;

	// see if bot already has a weapon of this type
	for ( i=0; i<Weapons.Length; i++ )
		if ( Weapons[i].WeaponClass != None )
		{
			AlreadyHas = UTWeapon(Bot.FindInventoryType(Weapons[i].WeaponClass));
			if ( AlreadyHas == None )
				desire += Weapons[i].WeaponClass.Default.AIRating;
			else if ( AlreadyHas.NeedAmmo() )
				desire += 0.15;
		}
	if ( UTBot(Bot.Controller).bHuntPlayer && (desire * 0.833 < Bot.Weapon.AIRating - 0.1) )
		return 0;

	// incentivize bot to get this weapon if it doesn't have a good weapon already
	if ( (Bot.Weapon == None) || (Bot.Weapon.AIRating < 0.5) )
		return 2*desire;

	return desire;
}

/* DetourWeight()
value of this path to take a quick detour (usually 0, used when on route to distant objective, but want to grab inventory for example)
*/
function float DetourWeight(Pawn Other,float PathWeight)
{
	local UTWeapon AlreadyHas;
	local float desire;
	local int i;

	if ( bHidden || HasCustomer(Other) )
		return 0;

	// see if bot already has a weapon of this type
	for ( i=0; i<Weapons.Length; i++ )
	{
		AlreadyHas = UTWeapon(Other.FindInventoryType(Weapons[i].WeaponClass));
		if ( AlreadyHas == None )
			desire += Weapons[i].WeaponClass.Default.AIRating;
		else if ( AlreadyHas.NeedAmmo() )
			desire += 0.15;
	}
	if ( AIController(Other.Controller).PriorityObjective()
		&& ((Other.Weapon.AIRating > 0.5) || (PathWeight > 400)) )
		return 0.2/PathWeight;
	return desire/PathWeight;
}

simulated function InitializePickup() {}
simulated function ShowActive();
simulated function NotifyLocalPlayerDead(PlayerController PC);

simulated function SetPlayerNearby(bool bNewPlayerNearby, bool bPlayEffects)
{
	local int i;
	local vector NewScale;
	local PrimitiveComponent DefaultMesh;

	if (bNewPlayerNearby != bPlayerNearby)
	{
		bPlayerNearby = bNewPlayerNearby;
		if ( WorldInfo.NetMode == NM_DedicatedServer )
		{
			return;
		}
		if (bPlayerNearby)
		{
			if ( !EffectIsRelevant(Location, false) )
			{
				return;
			}
			LightEnvironment.bDynamic = true;
			for (i = 0; i < Weapons.length; i++)
			{
				if (Weapons[i].WeaponClass.default.PickupFactoryMesh != None)
				{
					if ( Weapons[i].PickupMesh == None )
					{
						DefaultMesh = Weapons[i].WeaponClass.default.PickupFactoryMesh;
						Weapons[i].PickupMesh = new(self) DefaultMesh.Class(DefaultMesh);
						Weapons[i].PickupMesh.SetTranslation(LockerPositions[i] + Weapons[i].WeaponClass.default.LockerOffset);
						Weapons[i].PickupMesh.SetRotation(Weapons[i].WeaponClass.default.LockerRotation);
						Weapons[i].PickupMesh.SetLightEnvironment(LightEnvironment);
						NewScale = Weapons[i].PickupMesh.Scale3D;
						NewScale.X = 0.1;
						Weapons[i].PickupMesh.SetScale3D(NewScale);
					}
					if (Weapons[i].PickupMesh != None)
					{
						Weapons[i].PickupMesh.SetHidden(false);
						AttachComponent(Weapons[i].PickupMesh);

						if (bPlayEffects)
						{
							WorldInfo.MyEmitterPool.SpawnEmitter(WeaponSpawnEffectTemplate, Weapons[i].PickupMesh.GetPosition());
						}
					}
				}
			}
			ProximityEffect.ActivateSystem();
			ClearTimer('DestroyWeapons');
		}
		else
		{
			LightEnvironment.bDynamic = false;
			bPlayEffects = bPlayEffects && EffectIsRelevant(Location, false);
			for (i = 0; i < Weapons.length; i++)
			{
				if (Weapons[i].PickupMesh != None)
				{
					Weapons[i].PickupMesh.SetHidden(true);
					if (bPlayEffects)
					{
						WorldInfo.MyEmitterPool.SpawnEmitter(WeaponSpawnEffectTemplate, Weapons[i].PickupMesh.GetPosition());
					}
				}
			}
			ProximityEffect.DeactivateSystem();
			SetTimer(5.0, false, 'DestroyWeapons');
		}
	}
}

simulated function DestroyWeapons()
{
	local int i;

	for (i = 0; i < Weapons.length; i++)
	{
		if (Weapons[i].PickupMesh != None)
		{
			DetachComponent(Weapons[i].PickupMesh);
			Weapons[i].PickupMesh = None;
		}
	}
}

simulated function ShowHidden()
{
	bIsActive = false;
	AmbientEffect.SetTemplate(InactiveEffectTemplate);
	SetPlayerNearby(false, false);
}

auto state LockerPickup
{
	function bool ReadyToPickup(float MaxWait)
	{
		return true;
	}

	/*
	 * Validate touch (if valid return true to let other pick me up and trigger event).
	*/
	simulated function bool ValidTouch( actor Other )
	{
		// make sure its a live player
		if ( (Pawn(Other) == None) || !Pawn(Other).bCanPickupInventory || (Pawn(Other).DrivenVehicle == None && Pawn(Other).Controller == None) )
			return false;

		// make sure not touching through wall
		if ( !FastTrace(Other.Location, Location) )
		{
			SetTimer(0.5, false, 'RecheckValidTouch');
			return false;
		}

		return true;
	}

	/**
	Pickup was touched through a wall.  Check to see if touching pawn is no longer obstructed
	*/
	event RecheckValidTouch()
	{
		CheckTouching();
	}

	/**
	 * Make sure no pawn already touching (while touch was disabled in sleep).
	*/
	function CheckTouching()
	{
		local Pawn P;

		ForEach TouchingActors(class'Pawn', P)
			Touch(P, None, Location, Normal(Location-P.Location) );
	}

	simulated function ShowActive()
	{
		bIsActive = true;
		AmbientEffect.SetTemplate(ActiveEffectTemplate);
		CheckProximity();
	}

	simulated function NotifyLocalPlayerDead(PlayerController PC)
	{
		ShowActive();
	}

	simulated function CheckProximity()
	{
		local PlayerController PC;
		local bool bNewPlayerNearby;

		if (bIsActive)
		{
			foreach LocalPlayerControllers(class'PlayerController', PC)
			{
				if (Pawn(PC.ViewTarget) != None && VSize(PC.ViewTarget.Location - Location) < ProximityDistance)
				{
					bNewPlayerNearby = true;
					break;
				}
			}
		}

		SetPlayerNearby(bNewPlayerNearby, true);
	}

	// When touched by an actor.
	simulated event Touch( actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
	{
		local int		i;
		local UTWeapon Copy;
		local Pawn Recipient;

		// If touched by a player pawn, let him pick this up.
		if( ValidTouch(Other) )
		{
			Recipient = Pawn(Other);
			if ( (PlayerController(Recipient.Controller) != None) && (LocalPlayer(PlayerController(Recipient.Controller).Player) != None) )
			{
				if ( bIsActive )
				{
					ShowHidden();
					SetTimer(30,false,'ShowActive');
				}
			}
			if ( Role < ROLE_Authority )
				return;
			if ( !AddCustomer(Recipient) )
				return;

			for ( i=0; i<Weapons.Length; i++ )
			{
				InventoryType = Weapons[i].WeaponClass;
				Copy = UTWeapon(UTInventoryManager(Recipient.InvManager).HasInventoryOfClass(InventoryType));
				if ( Copy != None )
				{
					Copy.FillToInitialAmmo();
					Copy.AnnouncePickup(Recipient);
				}
				else if (WorldInfo.Game.PickupQuery(Recipient, InventoryType, self))
				{
					Copy = UTWeapon(spawn(InventoryType));
					if ( Copy != None )
					{
						Copy.GiveTo(Recipient);
						Copy.AnnouncePickup(Recipient);
						if ( Copy.LockerAmmoCount - Copy.Default.AmmoCount > 0 )
							Copy.AddAmmo(Copy.LockerAmmoCount - Copy.Default.AmmoCount);
					}
					else
						`log(self$" failed to spawn "$inventorytype);
				}
			}
		}
	}

	simulated event BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		ShowActive();
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			SetTimer(0.1, true, 'CheckProximity');
		}
	}
}

State Disabled
{
	simulated function BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		ShowHidden();
	}
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}

