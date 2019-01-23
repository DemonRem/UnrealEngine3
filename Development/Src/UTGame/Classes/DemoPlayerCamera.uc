/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class DemoPlayerCamera extends AnimatedCamera
	config(Camera);

// set to editinline so we can access it through the "editactor" command in-game
var(Camera) editinline	DemoCamMod_ScreenShake	DemoCamMod_ScreenShake;


function PostBeginPlay()
{
	super.PostBeginPlay();

	// Add our screen shake modifier to the camera system
	if( DemoCamMod_ScreenShake == None )
	{
		DemoCamMod_ScreenShake = new(Outer) class'DemoCamMod_ScreenShake';
	}
	DemoCamMod_ScreenShake.Init();
	DemoCamMod_ScreenShake.AddCameraModifier( Self );
}
