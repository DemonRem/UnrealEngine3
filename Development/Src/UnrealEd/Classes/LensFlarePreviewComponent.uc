/**
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class LensFlarePreviewComponent extends PrimitiveComponent
	native
	collapsecategories
	hidecategories(Object)
	editinlinenew;

var native transient const pointer	LensFlareEditorPtr{class WxLensFlareEditor};

cpptext
{
	virtual void Render(const FSceneView* View,FPrimitiveDrawInterface* PDI);
}

