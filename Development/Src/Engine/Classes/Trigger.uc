/**
 * An actor used to generate collision events (touch/untouch), and
 * interactions events (ue) as inputs into the scripting system.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class Trigger extends Actor
	placeable
	native;

cpptext
{
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
}

/** Base cylinder component for collision */
var() editconst const CylinderComponent	CylinderComponent;
/** for AI, true if we have been recently triggered (so AI doesn't try to trigger it again) */
var bool bRecentlyTriggered;
/** how long bRecentlyTriggered should stay set after each triggering */
var() float AITriggerDelay;

event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal)
{
	if (FindEventsOfClass(class'SeqEvent_Touch'))
	{
		bRecentlyTriggered = true;
		SetTimer(AITriggerDelay, false, 'UnTrigger');
	}
}

function UnTrigger()
{
	bRecentlyTriggered = false;
}

simulated function bool StopsProjectile(Projectile P)
{
	return false;
}

defaultproperties
{
	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Trigger'
		HiddenGame=False
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	Begin Object Class=CylinderComponent NAME=CollisionCylinder LegacyClassName=Trigger_TriggerCylinderComponent_Class
		CollideActors=true
		CollisionRadius=+0040.000000
		CollisionHeight=+0040.000000
	End Object
	CollisionComponent=CollisionCylinder
	CylinderComponent=CollisionCylinder
	Components.Add(CollisionCylinder)

	bHidden=true
	bCollideActors=true
	bProjTarget=true
	bStatic=false
	bNoDelete=true
	AITriggerDelay=2.0

	SupportedEvents.Add(class'SeqEvent_Used')
}
