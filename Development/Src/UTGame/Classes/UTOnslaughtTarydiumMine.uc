/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtTarydiumMine extends UTGameObjective
	abstract;

var color MineColor;

simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected )
{
	Canvas.DrawColor = MineColor;
	Super.RenderLinks(MP, Canvas, PlayerOwner, ColorPercent, bFullScreenMap, bSelected);
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
