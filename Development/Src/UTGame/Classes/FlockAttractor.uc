/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class FlockAttractor extends Actor
	placeable
	native
	hidecategories(Advanced)
	hidecategories(Collision)
	hidecategories(Display)
	hidecategories(Actor);

var()	interp float	Attraction;
var()	bool			bAttractorEnabled;
var()	bool			bAttractionFalloff;
var()	bool			bActionAtThisAttractor;

var		CylinderComponent	CylinderComponent;

cpptext
{
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
}

simulated function OnToggle(SeqAct_Toggle action)
{
	if (action.InputLinks[0].bHasImpulse)
	{
		// turn on
		bAttractorEnabled = TRUE;
	}
	else if (action.InputLinks[1].bHasImpulse)
	{
		// turn off
		bAttractorEnabled = FALSE;
	}
	else if (action.InputLinks[2].bHasImpulse)
	{
		// toggle
		bAttractorEnabled = !bAttractorEnabled;
	}
}

defaultproperties
{
	Attraction=1.0
	bAttractorEnabled=true
	bAttractionFalloff=true

	bCollideActors=true

	Begin Object Class=CylinderComponent NAME=CollisionCylinder
		CollideActors=true
		CollisionRadius=+0200.000000
		CollisionHeight=+0040.000000
	End Object
	CylinderComponent=CollisionCylinder
	CollisionComponent=CollisionCylinder
	Components.Add(CollisionCylinder)

	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EnvyEditorResources.RedDefense'
		HiddenGame=true
		HiddenEditor=false
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)
}