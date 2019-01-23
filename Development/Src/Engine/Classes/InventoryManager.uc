//=============================================================================
// InventoryManager
//	Base class to manage Pawn's inventory
//	This provides a simple interface to control and interact with the Pawn's inventory,
//	such as weapons, items and ammunition.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class InventoryManager extends Actor
	native;

/** Logging pre-processor macros */
`include( Core\Globals.uci )


/** First inventory item in inventory linked list */
var Inventory InventoryChain;

/**
 * Player will switch to PendingWeapon, once the current weapon has been put down.
 * @fixme laurent -- PendingWeapon should be made protected, because too many bugs result by setting this variable directly.
 * It's only safe to read it, but to change it, SetCurrentWeapon() should be used.
 */
var			Weapon		PendingWeapon;

var			Weapon		LastAttemptedSwitchToWeapon;

/** if true, don't allow player to put down weapon without switching to another one */
var			bool		bMustHoldWeapon;

/** Holds the current "Fire" status for both firing modes */
var				Array<int>					PendingFire;

//
// Network replication.
//
replication
{

	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) && bNetDirty && bNetOwner )
		InventoryChain;
}

event PostBeginPlay()
{
	Super.PostBeginPlay();
	Instigator = Pawn(Owner);
}


/**
 * returns all Inventory Actors of class BaseClass
 *
 * @param	BaseClass	Inventory actors returned are of, or childs of, this base class.
 * @output	Inv			Inventory actors returned.
 * @note this iterator bails if it encounters more than 100 items, since temporary loops in linked list may sometimes be created
 *	on network clients while link pointers are being replicated. For performance reasons you shouldn't have that many inventory items anyway.
 */

native final iterator function InventoryActors( class<Inventory> BaseClass, out Inventory Inv );


/**
 * Dump debug stats in log of all weapons in inventory.
 */

exec simulated function DumpWeaponStats()
{
	local Weapon	Weap;

	`log( "+++" @ GetFuncName() @ "+++" );
	`log( "Pawn.Weapon:" $ Instigator.Weapon @ "PendingWeapon:" $ PendingWeapon );
	ForEach InventoryActors( class'Weapon', Weap )
	{
		Weap.DumpWeaponDebugToLog();
	}
}


/**
 * Setup Inventory for Pawn P.
 * Override this to change inventory assignment (from a pawn to another)
 * Network: Server only
 */
function SetupFor(Pawn P)
{
	Instigator = P;
	SetOwner(P);
}


/**	Event called when inventory manager is destroyed, called from Pawn.Destroyed() */
event Destroyed()
{
	DiscardInventory();
}


/**
 * Handle Pickup. Can Pawn pickup this item?
 *
 * @param	ItemClass Class of Inventory our Owner is trying to pick up
 * @param	Pickup the Actor containing that item (this may be a PickupFactory or it may be a DroppedPickup)
 *
 * @return	whether or not the Pickup actor should give its item to Other
 */
function bool HandlePickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	local Inventory	Inv;

	if( InventoryChain == None )
	{
		return TRUE;
	}

	// Give other Inventory Items a chance to deny this pickup
	ForEach InventoryActors(class'Inventory', Inv)
	{
		if( Inv.DenyPickupQuery(ItemClass, Pickup) )
		{
			return FALSE;
		}
	}
	return TRUE;
}


/**
 * returns the inventory item of the requested class if it exists in this inventory manager.
 * @param	DesiredClass	class of inventory item we're trying to find.
 * @param	bAllowSubclass whether subclasses of the desired class are acceptable
 * @return	Inventory actor if found, None otherwise.
 */
simulated event Inventory FindInventoryType(class<Inventory> DesiredClass, optional bool bAllowSubclass)
{
	local Inventory		Inv;

	ForEach InventoryActors(DesiredClass, Inv)
	{
		if (bAllowSubclass || Inv.Class == DesiredClass)
		{
			return Inv;
		}
	}
	return None;
}


/**
 * Spawns a new Inventory actor of NewInventoryItemClass type, and adds it to the Inventory Manager.
 * @param	NewInventoryItemClass		Class of inventory item to spawn and add.
 * @return	Inventory actor, None if couldn't be spawned.
 */
simulated function Inventory CreateInventory(class<Inventory> NewInventoryItemClass, optional bool bDoNotActivate)
{
	local Inventory	Inv;

	if( NewInventoryItemClass != None )
	{
		inv = Spawn(NewInventoryItemClass);
		if( inv != None )
		{
			if( !AddInventory(Inv, bDoNotActivate) )
			{
				`warn("InventoryManager::CreateInventory - Couldn't Add newly created inventory" @ Inv);
				Inv.Destroy();
				Inv = None;
			}
		}
		else
		{
			`warn("InventoryManager::CreateInventory - Couldn't spawn inventory" @ NewInventoryItemClass);
		}
	}

	return Inv;
}

/**
 * Adds an existing inventory item to the list.
 * Returns true to indicate it was added, false if it was already in the list.
 *
 * @param	NewItem		Item to add to inventory manager.
 * @return	true if item was added, false otherwise.
 */
simulated function bool AddInventory(Inventory NewItem, optional bool bDoNotActivate)
{
	local Inventory Item, LastItem;

	// The item should not have been destroyed if we get here.
	if( (NewItem != None) && !NewItem.bDeleteMe )
	{
		// if we don't have an inventory list, start here
		if( InventoryChain == None )
		{
			InventoryChain = newItem;
		}
		else
		{
			// Skip if already in the inventory.
			for (Item = InventoryChain; Item != None; Item = Item.Inventory)
			{
				if( Item == NewItem )
				{
					return FALSE;
				}
				LastItem = Item;
			}
			LastItem.Inventory = NewItem;
		}

		NewItem.SetOwner( Instigator );
		NewItem.Instigator = Instigator;
		NewItem.InvManager = Self;
		NewItem.GivenTo( Instigator, bDoNotActivate);

		// Trigger inventory event
		Instigator.TriggerEventClass(class'SeqEvent_GetInventory', NewItem);
		return TRUE;
	}

	return FALSE;
}


/**
 * Attempts to remove an item from the inventory list if it exists.
 *
 * @param	Item	Item to remove from inventory
 */
simulated function RemoveFromInventory(Inventory ItemToRemove)
{
	local Inventory Item;
	local bool		bFound;

	if( ItemToRemove != None )
	{
		// make sure we don't have other references to the item
		if( ItemToRemove == Instigator.Weapon )
		{
			Instigator.Weapon = None;
		}

		if( InventoryChain == ItemToRemove )
		{
			bFound = TRUE;
			InventoryChain = ItemToRemove.Inventory;
		}
		else
		{
			// If this item is in our inventory chain, unlink it.
			for(Item = InventoryChain; Item != None; Item = Item.Inventory)
			{
				if( Item.Inventory == ItemToRemove )
				{
					bFound = TRUE;
					Item.Inventory = ItemToRemove.Inventory;
					break;
				}
			}
		}

		if( bFound )
		{
			`LogInv("removed" @ ItemToRemove);
			ItemToRemove.ItemRemovedFromInvManager();
			ItemToRemove.SetOwner(None);
			ItemToRemove.Inventory = None;
		}

		if (Instigator.Health > 0 && Instigator.Weapon == None && Instigator.Controller != None)
		{
			Instigator.Controller.ClientSwitchToBestWeapon(true);
		}
	}
}


/**
 * Discard full inventory, generally because the owner died
 */
simulated event DiscardInventory()
{
	local Inventory	Inv;
	local vector	TossVelocity;
	local bool		bBelowKillZ;

	// don't drop any inventory if below KillZ or out of world
	bBelowKillZ = (Instigator == None) || (Instigator.Location.Z < WorldInfo.KillZ);

	ForEach InventoryActors(class'Inventory', Inv)
	{
		if( Inv.bDropOnDeath && !bBelowKillZ )
		{
			TossVelocity = vector(Instigator.GetViewRotation());
			TossVelocity = TossVelocity * ((Instigator.Velocity dot TossVelocity) + 500.f) + 250.f * VRand() + vect(0,0,250);
			Inv.DropFrom(Instigator.Location, TossVelocity);
		}
		else
		{
			Inv.Destroy();
		}
	}

	// Clear reference to Weapon
	Instigator.Weapon = None;

	// Clear reference to PendingWeapon
	PendingWeapon = None;
}


/**
 * Damage modifier. Is Pawn carrying items that can modify taken damage?
 * Called from GameInfo.ReduceDamage()
 */
function int ModifyDamage( int Damage, Controller instigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	return Damage;
}


/**
 * Used to inform inventory when owner event occurs (for example jumping or weapon change)
 *
 * @param	EventName	Name of event to forward to inventory items.
 */
simulated function OwnerEvent(name EventName)
{
	local Inventory	Inv;

	ForEach InventoryActors(class'Inventory', Inv)
	{
		if( Inv.bReceiveOwnerEvents )
		{
			Inv.OwnerEvent(EventName);
		}
	}
}


/**
 * Hook called from HUD actor. Gives access to HUD and Canvas
 *
 * @param	H	HUD
 */
simulated function DrawHud( HUD H )
{
	local Inventory Inv;

	// Send RenderOverlays event to Inv Items requesting it
	ForEach InventoryActors(class'Inventory', Inv)
	{
    	if( Inv.bRenderOverlays )
		{
	    	Inv.RenderOverlays(H);
		}
	}

	// Send ActiveRenderOverlays event to active weapon
	if( Instigator.Weapon != None )
	{
		Instigator.Weapon.ActiveRenderOverlays(H);
	}
}

//
// Weapon Interface
//

/**
 * Pawn desires to fire. By default it fires the Active Weapon if it exists.
 * Called from PlayerController::StartFire() -> Pawn::StartFire()
 * Network: Local Player
 *
 * @param	FireModeNum		Fire mode number.
 */
simulated function StartFire(byte FireModeNum)
{
    if( Instigator.Weapon != None )
	{
		Instigator.Weapon.StartFire(FireModeNum);
	}
}


/**
 * Pawn stops firing.
 * i.e. player releases fire button, this may not stop weapon firing right away. (for example press button once for a burst fire)
 * Network: Local Player
 *
 * @param	FireModeNum		Fire mode number.
 */
simulated function StopFire( byte FireModeNum )
{
    if( Instigator.Weapon != None )
	{
		Instigator.Weapon.StopFire( FireModeNum );
	}
}


/**
 * returns true if ThisWeapon is the Pawn's active weapon.
 *
 * @param	ThisWeapon	weapon to test if it's the Pawn's active weapon.
 *
 * @return	true if ThisWeapon is the Pawn's current weapon
 */
simulated function bool IsActiveWeapon( Weapon ThisWeapon )
{
	return (ThisWeapon == Instigator.Weapon);
}


/**
 * Returns a weight reflecting the desire to use the
 * given weapon, used for AI and player best weapon
 * selection.
 *
 * @param	Weapon W
 * @return	Weapon rating (range -1.f to 1.f)
 */
simulated function float GetWeaponRatingFor( Weapon W )
{
	local float Rating;

	if ( !W.HasAnyAmmo() )
		return -1;

	Rating = 1;
	// tend to stick with same weapon
	if ( !Instigator.IsHumanControlled() && IsActiveWeapon( W ) && (Instigator.Controller.Enemy != None) )
		Rating += 0.21;

	return Rating;
}


/**
 * returns the best weapon for this Pawn in loadout
 */
simulated function Weapon GetBestWeapon( optional bool bForceADifferentWeapon  )
{
	local Weapon	W, BestWeapon;
	local float		Rating, BestRating;

	ForEach InventoryActors( class'Weapon', W )
	{
		if( w.HasAnyAmmo() )
		{
			if( bForceADifferentWeapon &&
				IsActiveWeapon( W ) )
			{
				continue;
			}

			Rating = W.GetWeaponRating();
			if( BestWeapon == None ||
				Rating > BestRating )
			{
				BestWeapon = W;
				BestRating = Rating;
			}
		}
	}

	return BestWeapon;
}


/**
 * Switch to best weapon available in loadout
 * Network: LocalPlayer
 */
simulated function SwitchToBestWeapon( optional bool bForceADifferentWeapon )
{
	local Weapon BestWeapon;

	// if we don't already have a pending weapon,
	if( bForceADifferentWeapon ||
		PendingWeapon == None ||
		(AIController(Instigator.Controller) != None) )
	{
		// figure out the new weapon to bring up
		BestWeapon = GetBestWeapon( bForceADifferentWeapon );

		if( BestWeapon == None )
		{
			return;
		}

		// if it matches our current weapon then don't bother switching
		if( BestWeapon == Instigator.Weapon )
		{
			BestWeapon = None;
			PendingWeapon = None;
			Instigator.Weapon.Activate();
		}
	}

	// stop any current weapon fire
	Instigator.Controller.StopFiring();

	// and activate the new pending weapon
	SetCurrentWeapon(BestWeapon);
}


/**
 * Switches to Previous weapon
 * Network: Client
 */
simulated function PrevWeapon()
{
	local Weapon	CandidateWeapon, StartWeapon, W;

	StartWeapon = Instigator.Weapon;
	if ( PendingWeapon != None )
	{
		StartWeapon = PendingWeapon;
	}

	// Get previous
	ForEach InventoryActors( class'Weapon', W )
	{
		if ( W == StartWeapon )
		{
			break;
		}
		CandidateWeapon = W;
	}

	// if none found, get last
	if ( CandidateWeapon == None )
	{
		ForEach InventoryActors( class'Weapon', W )
		{
			CandidateWeapon = W;
		}
	}

	// If same weapon, do not change
	if ( CandidateWeapon == Instigator.Weapon )
	{
		return;
	}

	SetCurrentWeapon(CandidateWeapon);
}


/**
 * Switches to Next weapon
 * Network: Client
 */
simulated function NextWeapon()
{
	local Weapon	StartWeapon, CandidateWeapon, W;
	local bool		bBreakNext;

	StartWeapon = Instigator.Weapon;
	if( PendingWeapon != None )
	{
		StartWeapon = PendingWeapon;
	}

	ForEach InventoryActors( class'Weapon', W )
	{
		if( bBreakNext || (StartWeapon == None) )
		{
			CandidateWeapon = W;
			break;
		}
		if( W == StartWeapon )
		{
			bBreakNext = true;
		}
	}

	if( CandidateWeapon == None )
	{
		ForEach InventoryActors( class'Weapon', W )
		{
			CandidateWeapon = W;
			break;
		}
	}
	// If same weapon, do not change
	if( CandidateWeapon == Instigator.Weapon )
	{
		return;
	}

	SetCurrentWeapon(CandidateWeapon);
}


/**
 * Set DesiredWeapon as Current (Active) Weapon.
 * Network: LocalPlayer
 *
 * @param	DesiredWeapon, Desired weapon to assign to player
 */
reliable client function SetCurrentWeapon(Weapon DesiredWeapon)
{
	local Weapon PrevWeapon;

	PrevWeapon = Instigator.Weapon;

	`LogInv("PrevWeapon:" @ PrevWeapon @ "DesiredWeapon:" @ DesiredWeapon);

	// Make sure we are switching to a new weapon
	// Handle the case where we're selecting again a weapon we've just deselected
	if( PrevWeapon != None && DesiredWeapon == PrevWeapon && !PrevWeapon.IsInState('WeaponPuttingDown') )
	{
		`LogInv("DesiredWeapon == PrevWeapon - abort");
		return;
	}

	// Set the new weapon as pending
	SetPendingWeapon(DesiredWeapon);

	// if there is an old weapon handle it first.
	if( PrevWeapon != None && !PrevWeapon.bDeleteMe && !PrevWeapon.IsInState('Inactive') )
	{
		// Try to put the weapon down.
		`LogInv("OldWeapon != None - try to put down");
		PrevWeapon.TryPutdown();
	}
	else
	{
		// We don't have a weapon, force the call to ChangedWeapon
		ChangedWeapon();
	}

	// Tell the server we have changed the pending weapon
	if( Role < Role_Authority )
	{
		ServerSetCurrentWeapon(DesiredWeapon);
	}
}


/**
 * Set the pending weapon for switching.
 * This shouldn't be called outside of SetCurrentWeapon()
 */
simulated function SetPendingWeapon(Weapon DesiredWeapon)
{
	// set the new weapon as pending
	PendingWeapon = DesiredWeapon;
}


/**
 * ServerSetCurrentWeapon begins the Putdown sequence on the server.  This function makes
 * the assumption that if TryPutDown succeeded on the client, it will succeed on the server.
 * This function shouldn't be called from anywhere except SetCurrentWeapon
 *
 * Network: Dedicated Server
 */
reliable server function ServerSetCurrentWeapon(Weapon DesiredWeapon)
{
	local Weapon PrevWeapon;

	PrevWeapon = Instigator.Weapon;

	`LogInv("PrevWeapon:" @ PrevWeapon @ "DesiredWeapon:" @ DesiredWeapon);

	// Set the new weapon as pending
	SetPendingWeapon(DesiredWeapon);

	// Make sure we are switching to a new weapon
	// Handle the case where we're selecting again a weapon we've just deselected
	if( PrevWeapon != None && DesiredWeapon == PrevWeapon && !PrevWeapon.IsInState('WeaponPuttingDown') )
	{
		`LogInv("DesiredWeapon == PrevWeapon - abort");
		return;
	}

	// Make sure we are switching to a new weapon
	if( PrevWeapon != None && !PrevWeapon.bDeleteMe && !PrevWeapon.IsInState('Inactive') )
	{
		`LogInv("OldWeapon != None - try to put down");
		PrevWeapon.TryPutDown();
	}
	else
	{
		ChangedWeapon();
	}
}


/** Prevents player from being without a weapon. */
simulated function bool CancelWeaponChange()
{
	// if PendingWeapon is None, prevent instigator from having no weapon,
	// so re-activate current weapon.
	if( PendingWeapon == None )
	{
		PendingWeapon = Instigator.Weapon;
	}

	return FALSE;
}


/**
 * ChangedWeapon is called when the current weapon is finished being deactivated
 */
simulated function ChangedWeapon()
{
	local Weapon OldWeapon;

	// Save current weapon as old weapon
	OldWeapon = Instigator.Weapon;

	// Make sure we can switch to a null weapon, otherwise, reactivate the current weapon
	if( PendingWeapon == None && bMustHoldWeapon )
	{
		if( OldWeapon != None )
		{
			OldWeapon.Activate();
			PendingWeapon = OldWeapon;
		}
	}

	`LogInv("switch from" @ OldWeapon @ "to" @ PendingWeapon);

	// switch to Pending Weapon
	Instigator.Weapon = PendingWeapon;

	// tell inventory that weapon changed (in case any effect was being applied)
	OwnerEvent('ChangedWeapon');

	// Play any Weapon Switch Animations
	Instigator.PlayWeaponSwitch(OldWeapon, PendingWeapon);

	// If we are going to an actual weapon, activate it.
	if( PendingWeapon != None )
	{
		// Setup the Weapon
		PendingWeapon.Instigator = Instigator;

		// Make some noise
		if( WorldInfo.Game != None )
		{
			Instigator.MakeNoise( 0.1 * WorldInfo.Game.GameDifficulty, 'ChangedWeapon' );
		}

		// Activate the Weapon
		PendingWeapon.Activate();
		PendingWeapon = None;
	}

	// Notify of a weapon change
	if( Instigator.Controller != None )
	{
		Instigator.Controller.NotifyChangedWeapon(OldWeapon, Instigator.Weapon);
	}
}


/**
 * Weapon just given to a player, check if player should switch to this weapon
 * Network: LocalPlayer
 * Called from Weapon.ClientWeaponSet()
 */
simulated function ClientWeaponSet(Weapon NewWeapon, bool bOptionalSet)
{
	local Weapon OldWeapon;

	OldWeapon = Instigator.Weapon;

	// If no current weapon, then set this one
	if( OldWeapon == None || OldWeapon.bDeleteMe || OldWeapon.IsInState('Inactive') )
	{
		`LogInv("OldWeapon == None or Inactive - Set new weapon right away" @ NewWeapon);
		SetCurrentWeapon(NewWeapon);
		return;
	}

	if( OldWeapon == NewWeapon )
	{
		`LogInv("OldWeapon == NewWeapon - abort" @ NewWeapon);
		return;
	}

	if( bOptionalSet )
	{
		if( OldWeapon.DenyClientWeaponSet() ||
			(Instigator.IsHumanControlled() && PlayerController(Instigator.Controller).bNeverSwitchOnPickup) )
		{
			`LogInv("bOptionalSet && (DenyClientWeaponSet() || bNeverSwitchOnPickup) - abort" @ NewWeapon);

			LastAttemptedSwitchToWeapon = NewWeapon;
			return;
		}
	}

	if( PendingWeapon == None || !PendingWeapon.HasAnyAmmo() || PendingWeapon.GetWeaponRating() < NewWeapon.GetWeaponRating() )
	{
		// Compare switch priority and decide if we should switch to new weapon
		if( !Instigator.Weapon.HasAnyAmmo() || Instigator.Weapon.GetWeaponRating() < NewWeapon.GetWeaponRating() )
		{
			`LogInv("Switch to new weapon:" @ NewWeapon);
			SetCurrentWeapon(NewWeapon);
			return;
		}
	}

	`LogInv("Send to inactive state" @ NewWeapon);
	NewWeapon.GotoState('Inactive');
}

/**
 * If the server detects that the client's weapon is out of sync, it will use this function to realign them.
 * Network: LocalPlayer
 *
 * @Param	NewWeapon	The weapon the server wishes to force the client to
 */

reliable client function ClientSyncWeapon(Weapon NewWeapon)
{
	local Weapon OldWeapon;

	if ( NewWeapon == Instigator.Weapon )
	{
		// FIXME: Remove this log.  It's here only to see how often this occurs.

		`log(Self@"(Owned by"@Owner@") is trying to Sync to the currently active weapon ("$NewWeapon$")");
		return;
	}

	OldWeapon = Instigator.Weapon;

	// switch to the new Weapon
	Instigator.Weapon = NewWeapon;

	// tell inventory that weapon changed (in case any effect was being applied)
	OwnerEvent('ChangedWeapon');

	// Play any Weapon Switch Animations
	Instigator.PlayWeaponSwitch(OldWeapon, NewWeapon);

	// If we are going to an actual weapon, activate it.
	if( NewWeapon != None )
	{
		// Setup the Weapon
		Instigator.Weapon.Instigator = Instigator;

		// Make some noise
		if( WorldInfo.Game != None )
		{
			Instigator.MakeNoise(0.1 * WorldInfo.Game.GameDifficulty, 'ChangedWeapon' );
		}

		// Activate the Weapon
		Instigator.Weapon.Activate();
	}

	// Notify of a weapon change
	if( Instigator.Controller != None )
	{
		Instigator.Controller.NotifyChangedWeapon(OldWeapon, Instigator.Weapon);
	}
}

defaultproperties
{
	TickGroup=TG_DuringAsyncWork

	bReplicateInstigator=TRUE
	RemoteRole=ROLE_SimulatedProxy
	bOnlyDirtyReplication=TRUE
	bOnlyRelevantToOwner=TRUE
	NetPriority=1.4
	bHidden=TRUE
	Physics=PHYS_None
	bReplicateMovement=FALSE
	bStatic=FALSE
	bNoDelete=FALSE
}
