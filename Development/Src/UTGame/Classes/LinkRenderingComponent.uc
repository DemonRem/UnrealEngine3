/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class LinkRenderingComponent extends PathRenderingComponent
	hidecategories(Object)
	native;

cpptext
{
	virtual void UpdateBounds();
	virtual void Render(const FSceneView* View,class FPrimitiveDrawInterface* PDI);
};
