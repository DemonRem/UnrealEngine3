/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCTFScoreboard extends UTTeamScoreboard;

// Draw Flag indicators

simulated function DrawCol(float x1,float x2,float y, int Index)
{
	local float Scale;
	local int OtherTeamIndex;

	Super.DrawCol(x1,x2,y,Index);

	OtherTeamIndex = Abs(WorldInfo.GRI.PRIArray[Index].Team.TeamIndex - 1);

	Scale = Canvas.ClipX / 1024;
	if ( WorldInfo.GRI.PRIArray[Index].bHasFlag )
	{
		Canvas.SetPos(x1-30*Scale,y);
		Canvas.DrawColor = WorldInfo.GRI.Teams[OtherTeamIndex].GetHudColor();
		Canvas.DrawTile(HudTexture,30*Scale, 30*Scale,336,129,61,48);			
	}
}


defaultproperties
{
}
