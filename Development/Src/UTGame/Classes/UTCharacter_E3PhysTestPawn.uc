/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCharacter_E3PhysTestPawn extends UTCharacter_IronguardMaleA;

State Dying
{
	event Timer();
	simulated function BeginState(name PrevStateName)
	{
		super.BeginState(PrevStateName);
		LifeSpan=0.0;
	}
}

defaultproperties
{
}
