/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTInventory extends Inventory
	native
	abstract;
var bool bDropOnDisrupt;

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	local UTGame GI;

	Super.GivenTo(NewOwner, bDoNotActivate);

	GI = UTGame(WorldInfo.Game);
	if (GI != none && GI.GameStats != none)
	{
		GI.GameStats.PickupInventoryEvent(Class,NewOwner.PlayerReplicationInfo,'pickup');
	}
}

function DropFrom( vector StartLocation, vector StartVelocity )
{
	local UTGame GI;
	local pawn POwner;

	GI = UTGame(WorldInfo.Game);

	POwner = Pawn(Owner);

	if (GI != none && GI.GameStats != none && POwner != none)
	{
		GI.GameStats.PickupInventoryEvent(Class,POwner.PlayerReplicationInfo,'dropped');
	}

	Super.DropFrom(StartLocation, StartVelocity );
}

defaultproperties
{
	MessageClass=class'UTPickupMessage'

	DroppedPickupClass=class'UTRotatingDroppedPickup'
	bDropOnDisrupt=true
}
