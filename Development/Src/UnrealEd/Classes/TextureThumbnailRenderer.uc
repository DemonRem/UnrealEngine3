/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This thumbnail renderer displays the texture for the object in question
 */
class TextureThumbnailRenderer extends ThumbnailRenderer
	native;

cpptext
{
	/**
	 * Calculates the size the thumbnail would be at the specified zoom level
	 *
	 * @param Object the object the thumbnail is of
	 * @param PrimType ignored
	 * @param Zoom the current multiplier of size
	 * @param OutWidth the var that gets the width of the thumbnail
	 * @param OutHeight the var that gets the height
	 */
	virtual void GetThumbnailSize(UObject* Object,EThumbnailPrimType,
		FLOAT Zoom,DWORD& OutWidth,DWORD& OutHeight);

	/**
	 * Draws a thumbnail for the object that was specified
	 *
	 * @param Object the object to draw the thumbnail for
	 * @param PrimType ignored
	 * @param X the X coordinate to start drawing at
	 * @param Y the Y coordinate to start drawing at
	 * @param Width the width of the thumbnail to draw
	 * @param Height the height of the thumbnail to draw
	 * @param Viewport ignored
	 * @param RI the render interface to draw with
	 * @param BackgroundType type of background for the thumbnail
	 */
	virtual void Draw(UObject* Object,EThumbnailPrimType,INT X,INT Y,
		DWORD Width,DWORD Height,FViewport*,FCanvas* Canvas,
		EThumbnailBackgroundType BackgroundType);
}
