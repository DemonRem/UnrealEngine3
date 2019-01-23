/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This thumbnail renderer displays a given prefab
 */
class PrefabThumbnailRenderer extends IconThumbnailRenderer
	native
	config(Editor);

cpptext
{
	virtual void Draw(UObject* Object,EThumbnailPrimType,INT X,INT Y,
		DWORD Width,DWORD Height,FViewport*,FCanvas* Canvas,
		EThumbnailBackgroundType BackgroundType);
}