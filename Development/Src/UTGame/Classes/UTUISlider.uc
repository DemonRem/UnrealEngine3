/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Extended version of the slider for UT3.
 */
class UTUISlider extends UISlider
	native(UI);

cpptext
{
public:
	/**
	 * Render this slider.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas );
}

defaultproperties
{
	BarSize=(Value=16.0)
	MarkerWidth=(Value=32.0)
	MarkerHeight=(Value=32.0)
}