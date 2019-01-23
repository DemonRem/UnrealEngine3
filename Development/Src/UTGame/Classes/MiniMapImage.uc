/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class MiniMapImage extends UIImage
	native(UI);

var transient UIState CurrentState;

native function SetStyle(name NewStyleTag);

cpptext
{
	virtual void Render_Widget( FCanvas* Canvas );
}

defaultproperties
{
	DefaultStates.Add(class'UIState_UTObjActive')
	DefaultStates.Add(class'UIState_UTObjBuilding')
	DefaultStates.Add(class'UIState_UTObjCritical')
	DefaultStates.Add(class'UIState_UTObjInactive')
	DefaultStates.Add(class'UIState_UTObjNeutral')
	DefaultStates.Add(class'UIState_UTObjProtected')
	DefaultStates.Add(class'UIState_UTObjUnderAttack')
}
