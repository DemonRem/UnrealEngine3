/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// Keypoint, the base class of invisible actors which mark things.
//=============================================================================
class Keypoint extends Actor
	abstract
	placeable
	native;

defaultproperties
{
	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Keypoint'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	bStatic=True
	bHidden=True
}
