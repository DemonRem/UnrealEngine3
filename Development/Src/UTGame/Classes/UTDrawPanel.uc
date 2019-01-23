/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDrawPanel extends UIContainer
	placeable
	native(UI);

/** Holds a reference to the canvas.  This is only valid during the DrawPanel() event */
var Canvas Canvas;

cpptext
{
	void PostRender_Widget(FCanvas* Canvas);
}

/** Draws a 2D Line */
native final function Draw2DLine(int X1, int Y1, int X2, int Y2, color LineColor);


/**
 * In both functions below, Canvas will to the bounds of the widget.  You should
 * use DrawTileClipped() in order to make sure left/top clipping occurs if not
 * against the left/top edges
 */

/**
 * If this delegate is set, native code will call here first for rendering.
 *
 * @Return true to stop native code from calling the event
 */

delegate bool DrawDelegate(Canvas C);


/**
 * Handle drawing in script
 */

event DrawPanel();


defaultproperties
{
}
