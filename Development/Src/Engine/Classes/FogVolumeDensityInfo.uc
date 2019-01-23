/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeDensityInfo extends Info
	showcategories(Movement)
	native(FogVolume)
	abstract;

/** The fog component which stores data specific to each density function. */
var() const FogVolumeDensityComponent	DensityComponent;

/** replicated copy of HeightFogComponent's bEnabled property */
var repnotify bool bEnabled;

replication
{
	if (Role == ROLE_Authority)
		bEnabled;
}

event PostBeginPlay()
{
	Super.PostBeginPlay();

	bEnabled = DensityComponent.bEnabled;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bEnabled')
	{
		DensityComponent.SetEnabled(bEnabled);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/* epic ===============================================
* ::OnToggle
*
* Scripted support for toggling height fog, checks which
* operation to perform by looking at the action input.
*
* Input 1: turn on
* Input 2: turn off
* Input 3: toggle
*
* =====================================================
*/
simulated function OnToggle(SeqAct_Toggle action)
{
	if (action.InputLinks[0].bHasImpulse)
	{
		// turn on
		DensityComponent.SetEnabled(TRUE);
	}
	else if (action.InputLinks[1].bHasImpulse)
	{
		// turn off
		DensityComponent.SetEnabled(FALSE);
	}
	else if (action.InputLinks[2].bHasImpulse)
	{
		// toggle
		DensityComponent.SetEnabled(!DensityComponent.bEnabled);
	}
	bEnabled = DensityComponent.bEnabled;
	ForceNetRelevant();
	SetForcedInitialReplicatedProperty(Property'Engine.FogVolumeDensityInfo.bEnabled', (bEnabled == default.bEnabled));
}

defaultproperties
{
	bStatic=FALSE
	bNoDelete=true
}
