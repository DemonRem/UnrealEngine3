/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCTFScoreboardPanel extends UTTDMScoreboardPanel;

var() Texture2D FlagTexture;
var() TextureCoordinates FlagCoords;

/*
function DrawCriticalPlayer(Canvas C, UTPlayerReplicationInfo PRI, float YPos, float CellHeight, UTScoreboardPanel Panel, int PIndex)
{
	local Vector2D ViewportSize;
	local float X, Scale;

    if (FlagTexture != none )
    {
		Panel.GetViewportSize(ViewportSize);
		Scale = ViewportSize.X / 1024;

		X = (Panel.AssociatedTeamIndex == 0) ? C.ClipX + 5 : C.ClipX - 5 - FlagCoords.UL * Scale;
		C.SetPos(X,YPos + (CellHeight * 0.5) - (FlagCoords.VL*Scale*0.5));
		C.DrawColor = TeamColors[Panel.AssociatedTeamIndex];
		C.DrawTile(FlagTexture,FlagCoords.UL*Scale,FlagCoords.VL*Scale,FlagCoords.U,FlagCoords.V,FlagCoords.UL,FlagCoords.VL);
	}
}
*/

/**
 * Draw the Player's Score
 */

function float DrawScore(UTPlayerReplicationInfo PRI, float YPos, int FontIndex, float FontScale)
{
	local string Spot;
	local float x, xl, yl, bxl;

	// Draw the player's Caps

	Spot = GetPlayerScore(PRI);
	StrLen("0000",XL,YL, FontIndex, 1.0);
	BXL = XL;
	StrLen(Spot,XL,YL,FontIndex,FontScale);
	X = Canvas.ClipX - BXL - 5 * FontScale;
	DrawString( Spot, X+(BXL * 0.5) - (XL *0.5), YPos,FontIndex,FontScale);
	Strlen(" ",XL,YL,FontIndex, FontScale);
	return (X-XL);
}

simulated function DrawPlayerNum(UTPlayerReplicationInfo PRI, int PIndex, out float YPos, float FontIndex, float FontScale)
{
//	local float XPos;
	local float W,H,Y;
	local float XL, YL;
//	local string Spot;
	local color C;

	if ( FlagTexture != none && PRI.bHasFlag )
	{

		C = Canvas.DrawColor;

    	// Figure out how much space we have

		StrLen("00",XL,YL, FontIndex, FontScale);
		W = XL * 0.8;
		H = W * (FlagCoords.VL / FlagCoords.UL);

        Y = YPos + (YL * 0.5) - (H * 0.5);

		Canvas.SetPos(0, Y);
		Canvas.SetDrawColor(255,255,0,255);
		Canvas.DrawTile(FlagTexture, W, H, FlagCoords.U, FlagCoords.V, FlagCoords.UL, FlagCoords.VL);

		Canvas.DrawColor = C;

	}
}


function string GetRightMisc(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		if (RightMiscStr != "")
		{
			return UserString(RightMiscStr,PRI);
		}

		return "";

	}
	return "RMisc";
}


defaultproperties
{
	FlagTexture=Texture2D'UI_HUD.HUD.UI_HUD_BaseE'
	FlagCoords=(U=756,V=0,UL=67,VL=40)
	bDrawPlayerNum=true
}
