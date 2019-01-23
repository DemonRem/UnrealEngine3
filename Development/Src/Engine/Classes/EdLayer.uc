/* epic ===============================================
* class EdLayer
*
* A layer is a grouping of actors in the level.
* The actors contain pointers back to the layer that
* they belong to.
*
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class EdLayer extends Object
	deprecated
	hidecategories(Object)
	editinlinenew
	native;

/** A human readable description to use in the browser window  */
var() string Desc;

/** If TRUE, the actors in this layer are rendered */
var() bool bVisible;

/** If TRUE, the actors in this layer are readonly */
var() bool bLocked;

/** If TRUE, the actors in this layer are rendered using a user defined color */
var() bool bUseColor;

/** The color to use when drawing the actors in this layer if bUseColor is TRUE */
var() color DrawColor;

/** The coordinate system to use for this layer.  Affects how the widget works with the actors in this layer. */
var EdCoordSystem CoordSystem;

defaultproperties
{
	Desc="Layer"
	bVisible=True
	bUseColor=False
	DrawColor=(R=255,G=255,B=255)
	CoordSystem=None
}
