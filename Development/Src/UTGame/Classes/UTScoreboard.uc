/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTScoreboard extends ScoreBoard
	config(Game);

var const	Texture2D	HudTexture;


var int ScrollTop;
	
function PostBeginPlay()
{
	Super.PostBeginPlay();
}	
	
function DrawHUD()
{
	local int i, idx, ItemsPerPage;
	local GameReplicationInfo GRI;
	local float LeftPos,TopPos,ColX,Width, Height, xl, yl, w,h,nw;
	local string str,scrollmsg;

	local int ShowCount;

	GRI = WorldInfo.GRI;
	if (GRI==none)
	{
		return;
	}
	
	WorldInfo.GRI.SortPRIArray();

	for (i=0;i<GRI.PRIArray.Length;i++)
	{
		if ( !GRI.PRIArray[i].bIsSpectator )
		{
			ShowCount++;
		}
	}
	
	LeftPos = Canvas.ClipX * 0.05;
	TopPos  = Canvas.ClipY * 0.15;
	Width = Canvas.ClipX * 0.9;
	Height = Canvas.ClipY * 0.7;
	
	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(3);
	
	Str = GRI.GameClass.default.GameName;
	Canvas.StrLen(Str,xl,yl);
	Canvas.SetPos(Canvas.ClipX*0.5 - xl * 0.5, 5);
	Canvas.DrawText(Str);
	
	Canvas.SetDrawColor(0,0,0,128);
	Canvas.SetPos(LeftPos, TopPos);
	Canvas.DrawTile(Canvas.DefaultTexture, Width, Height, 0,0,32,32);
	Canvas.SetPos(LeftPos+2, TopPos+2);
	Canvas.SetDrawColor(255,255,0,255);	
	Canvas.DrawBox(Width-4,Height-4);

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
	ScrollMsg = "Use the Scrollwheell to scroll the scoreboard";
	Canvas.StrLen(ScrollMsg,w,h);
	Canvas.SetPos(Canvas.ClipX * 0.5-w*0.5, TopPos+Height-h-3);
	Canvas.DrawText(ScrollMsg);

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);
	
	LeftPos += 16;
	TopPos += 16;
	Width -= 32;
	Height -= 64;
	
	Canvas.StrLen("00. ",xl,yl);	
	nw = xl;
	ItemsPerPage = 	int ( Height / (yl*1.2) );

	if ( ScrollTop < 0 || ShowCount <= ItemsPerPage )
	{
		ScrollTop = 0;
	}
	else
	{
		if (ScrollTop + ItemsPerPage > ShowCount)
		{
			ScrollTop = ShowCount - ItemsPerPage;
		}
	}

	ColX = LeftPos + Width * 0.85;
		
	for (i=0; i<ItemsPerPage; i++)
	{
	
		Idx = ScrollTop + i;
	
		// Skip out if nothing to draw
	
		if (Idx >= GRI.PRIArray.Length)
		{
			break;
		}
	
		Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);
		if (GRI.PRIArray[ Idx ] == PlayerOwner.PlayerReplicationInfo)
		{
			Canvas.SetDrawColor(0,255,255,255);
		}
		else
		{
			Canvas.SetDrawColor(255,255,255,255);
		}
	
		Canvas.SetPos(LeftPos, TopPos);
		Canvas.DrawText(""$(Idx+1)$".");
		Canvas.SetPos(LeftPos + nw + 5, TopPos);
		Canvas.DrawText(GRI.PRIArray[Idx].PlayerName);
		
		Str = ""$Int(GRI.PRIArray[Idx].Score);
		Canvas.StrLen(Str,w,h);
		
		Canvas.SetPos(ColX - 25 * (Canvas.ClipX/1024) - W, TopPos);
		Canvas.DrawText(Str);

		Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
		
		str = "Deaths:"@Int(GRI.PRIArray[Idx].Deaths)$" ";
		Canvas.Strlen(Str,w,h);
		Canvas.SetPos(ColX,TopPos);
		Canvas.DrawText(str);
		if (PlayerOwner.Role < ROLE_Authority)
		{
			str = "Ping:"@GRI.PRIArray[Idx].Ping$" ";
			Canvas.SetPos(ColX,TopPos+h);
			Canvas.DrawText(str);
		}
		
		TopPos += YL * 1.2;
	}
	
}

simulated function ChangeState(bool bIsVisible)
{
	local UTPlayerInput PInput;

	PInput = UTPlayerInput(PlayerOwner.PlayerInput);

	if (PInput != none)
	{
		if (bIsVisible)
		{
			PInput.OnInputKey = SBInputKey;
			ScrollTop = 0;
		}
		else
		{
			PInput.OnInputKey=none;
		}
	}
}

function bool SBInputKey(int ControllerID, name Key, EInputEvent Event, float AmountDepressed, bool Gamepad)
{
	if (Key == 'MouseScrollUp' || Key == 'MouseScrollDown')
	{
		if (Event == IE_Released)
		{
			if (Key == 'MouseScrollUp')
			{
				ScrollTop--;	
			}
			else
			{
				ScrollTop++;
			}
		}
		return true;
	}
	return false;
}

defaultproperties
{
	ConsoleMessagePosY=0.85
	HudTexture=Texture2D'T_UTHudGraphics.Textures.UTOldHUD'

}
