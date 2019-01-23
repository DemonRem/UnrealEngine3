/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Depth of Field post process effect
 *
 */
class DOFAndBloomEffect extends DOFEffect
	native;

/** A scale applied to blooming colors. */
var() float BloomScale;

cpptext
{
	// UPostProcessEffect interface

	/**
	 * Creates a proxy to represent the render info for a post process effect
	 * @param WorldSettings - The world's post process settings for the view.
	 * @return The proxy object.
	 */
	virtual class FPostProcessSceneProxy* CreateSceneProxy(const FPostProcessSettings* WorldSettings);

	/**
	 * @param View - current view
	 * @return TRUE if the effect should be rendered
	 */
	virtual UBOOL IsShown(const FSceneView* View) const;
}

defaultproperties
{
	BloomScale=1.0
	BlurKernelSize=16.0
}