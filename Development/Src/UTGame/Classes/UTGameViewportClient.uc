/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTGameViewportClient extends GameViewportClient
	config(Game);

/** The background to use for the menu and transition screens. */
var Texture2D LoadBackground;

var localized string LevelActionMessages[6];


function DrawTransition(Canvas Canvas)
{
	local int i;
	local float xl,yl;

	if(Outer.TransitionType != TT_None)
	{
		i = Outer.TransitionType;

		if(Outer.TransitionType != TT_Paused)
		{
			RenderHeader(Canvas);
		}

		Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);
		Canvas.Strlen(LevelActionMessages[i],xl,yl);
		Canvas.SetDrawColor(255,255,255,255);
		Canvas.SetPos(5,25);
		Canvas.DrawText(LevelActionMessages[i]);
		Canvas.SetDrawColor(255,255,0,255);
		if(Outer.TransitionType == TT_Loading)
		{
			Canvas.SetPos(5,25+YL);
			Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(3);
			Canvas.DrawText(Outer.TransitionDescription);
		}
	}
}

function RenderHeader(Canvas Canvas)
{
	local float xl,yl;
	// Draw the background

    Canvas.SetDrawColor(255,255,255,255);
	Canvas.SetPos(0,0);
	Canvas.DrawTile( LoadBackground, Canvas.ClipX, Canvas.ClipY,0,0,1024,768);


	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(1);
	Canvas.Strlen("Q",xl,yl);
    Canvas.SetDrawColor(255,255,255,255);
    Canvas.SetPos(5,5);
    Canvas.DrawText("Unreal Tournament 1998-2007 Epic Games, Inc.");
    Canvas.SetDrawColor(255,255,0,255);
}

/** This version prevents splitting in the main menu. */
function SetSplitscreenConfiguration( ESplitScreenType SplitType )
{
	local int Idx;

	if (GamePlayers[0].Actor.IsA('UTEntryPlayerController'))
	{
		// when in the menus, remove split and just use top player's viewport
		GamePlayers[0].Size.X = 1.0;
		GamePlayers[0].Size.Y = 1.0;
		GamePlayers[0].Origin.X = 0.0;
		GamePlayers[0].Origin.Y = 0.0;
		for (Idx = 1; Idx < GamePlayers.length; Idx++)
		{
			GamePlayers[Idx].Size.X = 0.0;
			GamePlayers[Idx].Size.Y = 0.0;
		}
	}
	else
	{
		Super.SetSplitscreenConfiguration(SplitType);
	}
}


defaultproperties
{
	LoadBackground=texture2d'T_UTHudGraphics.Textures.T_Malcolm_bk'
	UIControllerClass=class'UTGame.UTGameInteraction'
}
