/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class CeilingReachSpec extends ReachSpec
	native;

cpptext
{
	virtual INT CostFor(APawn* P);
}

native function int AdjustedCostFor( Pawn P, NavigationPoint Anchor, NavigationPoint Goal, int Cost );

defaultproperties
{
}
