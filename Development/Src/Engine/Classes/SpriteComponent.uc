/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SpriteComponent extends PrimitiveComponent
	native
	noexport
	collapsecategories
	hidecategories(Object)
	editinlinenew;

var() Texture2D	Sprite;
var() bool		bIsScreenSizeScaled;
var() float		ScreenSize;

defaultproperties
{
	Sprite=Texture2D'EngineResources.S_Actor'
	bIsScreenSizeScaled=false
	ScreenSize=0.1
}
