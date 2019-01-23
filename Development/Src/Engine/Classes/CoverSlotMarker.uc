/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class CoverSlotMarker extends NavigationPoint
	native;

cpptext
{
	FVector GetMarkerLocation( AScout* Scout );
	virtual void addReachSpecs(AScout* Scout, UBOOL bOnlyChanged);
	virtual void AddForcedSpecs( AScout *Scout );
	virtual UBOOL CanConnectTo(ANavigationPoint* Nav, UBOOL bCheckDistance);
	virtual UBOOL ShouldBeBased()
	{
		// don't base since we're set directly to the slot location
		return FALSE;
	}

	UBOOL PlaceScout(AScout *Scout);
	virtual UBOOL CanPrunePath(INT index);
	virtual UClass* GetReachSpecClass( ANavigationPoint* Nav, UClass* ReachSpecClass );
}

var() editconst CoverInfo OwningSlot;

event PostBeginPlay()
{
	super.PostBeginPlay();

	if (OwningSlot.Link != None)
	{
		bBlocked = OwningSlot.Link.bBlocked;
	}
}

simulated function int ExtraPathCost( Controller AI )
{
	if( !OwningSlot.Link.IsValidClaim( AI, OwningSlot.SlotIdx, TRUE ) )
	{
		return class'ReachSpec'.const.BLOCKEDPATHCOST;
	}
	return 0;
}

simulated function Vector GetSlotLocation()
{
	if( OwningSlot.Link != None )
	{
		return OwningSlot.Link.GetSlotLocation(OwningSlot.SlotIdx);
	}

	return vect(0,0,0);
}

simulated function Rotator GetSlotRotation()
{
	if( OwningSlot.Link != None )
	{
		return OwningSlot.Link.GetSlotRotation(OwningSlot.SlotIdx);
	}

	return rot(0,0,0);
}

defaultproperties
{
	bCollideWhenPlacing=FALSE
	bSpecialMove=TRUE

	// Jump up cost so AI tends to pathfind through open areas
	// instead of along walls
	Cost=300

//test
	Components.Remove(Sprite)
	Components.Remove(Sprite2)
	Components.Remove(Arrow)

	Begin Object Name=CollisionCylinder
		CollisionRadius=40.f
		CollisionHeight=40.f
	End Object
}
