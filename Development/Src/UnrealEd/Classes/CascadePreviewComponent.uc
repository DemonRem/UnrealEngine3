/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class CascadePreviewComponent extends PrimitiveComponent
	native
	collapsecategories
	hidecategories(Object)
	editinlinenew;

var native transient const pointer	CascadePtr{class WxCascade};

cpptext
{
	virtual void Render(const FSceneView* View,FPrimitiveDrawInterface* PDI);
}

