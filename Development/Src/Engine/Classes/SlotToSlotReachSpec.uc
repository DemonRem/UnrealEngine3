/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SlotToSlotReachSpec extends ForcedReachSpec
	native;

cpptext
{
	virtual INT defineFor( class ANavigationPoint *begin, class ANavigationPoint * dest, class APawn * Scout );
	virtual INT CostFor(APawn* P);
}

// Value CoverLink.ECoverDirection for movement direction along this spec
var() editconst Byte SpecDirection;

defaultproperties
{
	ForcedPathSizeName=Common
	bSkipPrune=FALSE
	PruneSpecList(0)=class'ReachSpec'
}
