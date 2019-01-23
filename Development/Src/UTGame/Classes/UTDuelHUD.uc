class UTDuelHUD extends UTTeamHUD;

event PostRender()
{
	local int i;
	local float XL, YL;

	Super.PostRender();

	if (bShowHUD && !bShowScores && WorldInfo.GRI != None)
	{
		for (i = 0; i < WorldInfo.GRI.PRIArray.length; i++)
		{
			if (WorldInfo.GRI.PRIArray[i].Team != None)
			{
				Canvas.Font = GetFontSizeIndex(1);
				Canvas.DrawColor = WorldInfo.GRI.PRIArray[i].Team.GetTextColor();
				Canvas.StrLen(WorldInfo.GRI.PRIArray[i].PlayerName, XL, YL);
				Canvas.SetPos( (WorldInfo.GRI.PRIArray[i].Team.TeamIndex == 0) ? (Canvas.ClipX * 0.45 - XL) : (Canvas.ClipX * 0.55),
						Canvas.ClipY * 0.14 );
				Canvas.DrawText(WorldInfo.GRI.PRIArray[i].PlayerName);
			}
		}
	}
}
