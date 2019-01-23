/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTJumpPadReachSpec extends UTTrajectoryReachSpec
	native;

cpptext
{
	virtual FVector GetInitialVelocity();
}

native function int CostFor(Pawn P);
