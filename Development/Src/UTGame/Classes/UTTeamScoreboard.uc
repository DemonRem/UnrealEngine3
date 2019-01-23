/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTTeamScoreboard extends UTScoreboard;

var color PlayerColor;

function DrawHUD()
{
	local int i, ItemsPerPage, Count;
	local GameReplicationInfo GRI;
	local float LeftPos,TopPos,CenterXPos,Width, Height, xl, yl, w, h, Scale;
	local string str,scrollmsg;

	local array<int> RedList;
	local array<int> BlueList;
	
	GRI = WorldInfo.GRI;
	if (GRI==none)
	{
		return;
	}

	WorldInfo.GRI.SortPRIArray();
	Scale = Canvas.ClipX / 1024;

	for (i=0;i<GRI.PRIArray.Length;i++)
	{
		if ( !GRI.PRIArray[i].bIsSpectator )
		{
			switch (GRI.PRIArray[i].Team.TeamIndex)
			{
				case 0: RedList[RedList.Length] = i; break;
				case 1: BlueList[BlueList.Length] = i; break;
			}
		}
	}
	
	LeftPos = Canvas.ClipX * 0.05;
	TopPos  = Canvas.ClipY * 0.15;
	Width = Canvas.ClipX * 0.9;
	Height = Canvas.ClipY * 0.82;
	
	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(3);

	// Draw the Game Name
	
	Str = GRI.GameClass.default.GameName;

	Canvas.StrLen(Str,xl,yl);
	Canvas.SetPos(Canvas.ClipX*0.5 - xl * 0.5, 5);
	Canvas.DrawText(Str);

	// Draw the background
	
	Canvas.SetDrawColor(0,0,0,128);
	Canvas.SetPos(LeftPos, TopPos);
	Canvas.DrawTile(Canvas.DefaultTexture, Width, Height, 0,0,32,32);
	Canvas.SetPos(LeftPos+2, TopPos+2);
	Canvas.SetDrawColor(255,255,0,255);	
	Canvas.DrawBox(Width-4,Height-4);

	// Draw the Scroll MEssage

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
	ScrollMsg = "Use the Scrollwheell to scroll the scoreboard";
	Canvas.StrLen(ScrollMsg,w,h);
	Canvas.SetPos(Canvas.ClipX * 0.5-w*0.5, TopPos+Height-h-3);
	Canvas.DrawText(ScrollMsg);

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);
	
	LeftPos += 64  * Scale;
	TopPos  += 32  * Scale;
	Width   -= 128 * Scale;
	Height  -= 48 * Scale;

	CenterXPos = LeftPos + Width * 0.5;
	Canvas.StrLen('99',xl,yl);	

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
	Canvas.StrLen('99',w,h);	
	
	YL += H;
	ItemsPerPage = 	int ( Height / ( yl * 1.2) );

	Count = RedList.Length;
	if (Count < BlueList.Length)
		Count = BlueList.Length;

	if (ScrollTop < 0 || Count <= ItemsPerPage)
	{
		ScrollTop = 0;
	}
	else
	{
		if (ScrollTop + ItemsPerPage > Count)	
		{
			ScrollTop = Count - ItemsPerPage;
		}
	}
	
	for (i=0; i<ItemsPerPage; i++)
	{
		if (ScrollTop + i<RedList.Length)
		{
			DrawCol(LeftPos, CenterXPos - (32*Scale), TopPos + i * YL * 1.2,RedList[ScrollTop + i]);
		}
		if (ScrollTop + i<BlueList.Length)
		{
			DrawCol(CenterXPos + (32 * Scale), LeftPos+Width, TopPos + i * YL * 1.2, BlueList[ScrollTop + i]);
		}
	}
}

simulated function DrawCol(float x1,float x2,float y, int Index)
{
	local float xl,yl;
	local string s;
	local PlayerReplicationInfo PRI;

	PRI = WorldInfo.GRI.PRIArray[Index];
	if (PRI==none)
	{
		return;
	}

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);

	if (PRI == PlayerOwner.PlayerReplicationInfo)
	{
		Canvas.DrawColor = PlayerColor;
	}
	else
	{
		Canvas.DrawColor = PRI.Team.GetTextColor();
	}
	Canvas.SetPos(x1,y);
	Canvas.DrawText(PRI.PlayerName);

	s = ""$Int(PRI.Score);
	
	Canvas.Strlen(s,xl,yl);
	Canvas.SetPos(x2-xl,y);
	Canvas.DrawText(s);

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
	
	if ( PRI.Team == PlayerOwner.PlayerReplicationInfo.Team )
	{
		Canvas.SetPos(x1,y+yl);
		Canvas.SetDrawColor(180,180,180,255);		
		Canvas.DrawText("Location:"@PRI.GetLocationName());	
	}
}


defaultproperties
{
	PlayerColor=(R=0,G=255,B=255,A=255)
}
