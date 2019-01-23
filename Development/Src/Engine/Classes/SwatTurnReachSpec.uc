/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class SwatTurnReachSpec extends ForcedReachSpec
	native;

cpptext
{
	virtual INT CostFor(APawn* P);
}

defaultproperties
{
	ForcedPathSizeName=Common
	bSkipPrune=FALSE
	PruneSpecList(0)=class'ReachSpec'
}
