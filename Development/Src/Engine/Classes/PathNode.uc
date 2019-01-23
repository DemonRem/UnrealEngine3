//=============================================================================
// PathNode.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class PathNode extends NavigationPoint
	placeable
	native;

cpptext
{
	virtual INT AddMyMarker(AActor *S);
}

defaultproperties
{
	Begin Object NAME=Sprite
		Sprite=Texture2D'EngineResources.S_Pickup'
	End Object
}
