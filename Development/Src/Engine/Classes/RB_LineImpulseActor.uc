/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class RB_LineImpulseActor extends Actor
	native(Physics)
	placeable;

/**
 *	Similar to the RB_RadialImpulseActor, but does a line-check and applies an impulse to all Actors using rigid-bodies that it hits.
 */


cpptext
{
	// UObject interface
	virtual void PostEditChange(UProperty* PropertyThatChanged);
}

/** Strength of impulse to apply to actors hit by the line check. */
var()	float			ImpulseStrength;

/** Length of line to check along. */
var()	float			ImpulseRange;

/** If true, the Strength is taken as a change in velocity instead of an impulse (ie. mass will have no affect). */
var()	bool			bVelChange;

/** If an impulse should only be applied to the first thing the line hits, or all things in the lines path. */
var()	bool			bStopAtFirstHit;

var		ArrowComponent	Arrow;

var repnotify byte ImpulseCount;

replication
{
	if (bNetDirty)
		ImpulseCount;
}

/** Do the line check and apply impulse now. */
native final function FireLineImpulse();

/** Handling Toggle event from Kismet. */
simulated function OnToggle(SeqAct_Toggle inAction)
{
	if (inAction.InputLinks[0].bHasImpulse)
	{
		FireLineImpulse();
		ImpulseCount++;
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0f;
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'ImpulseCount')
	{
		FireLineImpulse();
	}
}

cpptext
{
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
}

defaultproperties
{
	Begin Object Class=ArrowComponent Name=ArrowComponent0
		ArrowSize=4.16667
		ArrowColor=(R=255,G=0,B=0)
	End Object
	Arrow=ArrowComponent0
	Components.Add(ArrowComponent0)

	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_LineImpulse'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	ImpulseStrength=900.0
	ImpulseRange=200.0

	bEdShouldSnap=true
	RemoteRole=ROLE_SimulatedProxy
	bNoDelete=true
	bAlwaysRelevant=true
	NetUpdateFrequency=0.1
	bOnlyDirtyReplication=true
}
