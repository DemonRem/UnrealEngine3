/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class CoverSlipReachSpec extends ForcedReachSpec
	native;

cpptext
{
	virtual INT CostFor(APawn* P);
}

defaultproperties
{
	ForcedPathSizeName=Common
	bSkipPrune=TRUE
}
