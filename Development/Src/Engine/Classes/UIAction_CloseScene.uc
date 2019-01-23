/**
 * Closes a scene.  If no scene is specified and bAutoTargetOwner is true for this action, closes the owner scene.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIAction_CloseScene extends UIAction_Scene
	native(inherit);

cpptext
{
	virtual void Activated();
}

DefaultProperties
{
	ObjName="Close Scene"

	bAutoTargetOwner=true
}
