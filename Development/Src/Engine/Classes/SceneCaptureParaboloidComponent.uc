/**
 * SceneCaptureParaboloidComponent
 *
 * Allows a scene capture for the 2 sides of a paraboloid map
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SceneCaptureParaboloidComponent extends SceneCaptureComponent
	deprecated
	native;

/** texture targets for the 2 paraboloid faces */
var(Capture) TextureRenderTarget TextureTargets[2];
/** near plane clip distance */
var(Capture) float NearPlane;
/** far plane clip distance */ 
var(Capture) float FarPlane;

defaultproperties
{
	NearPlane=20
	FarPlane=500
}
