/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTSurvivalHUD extends UTHUD;
/*
function DrawScore()
{
    local float		xl,yl, LeftPos, TopPos;
    local int		i,j;
	local UTSurvivalPRI PRI;
	local UTSurvivalPRI Survivor;
	local array<UTSurvivalPRI> Challengers, Queue;


	if ( WorldInfo.GRI == None )
	{
		return;
	}

	Canvas.DrawColor = WhiteColor;
	Canvas.Font = class'Engine'.Static.GetSmallFont();
	Canvas.Strlen("Score", xl, yl);
	LeftPos	= 0.01 * Canvas.ClipX;
    TopPos		= 0.01 * Canvas.ClipY;

	Canvas.SetPos(LeftPos, TopPos);

	Canvas.DrawColor = WhiteColor;
	Canvas.DrawTile(HudTexture,34,34,150,356,34,34);
	Canvas.SetPos(LeftPos+40, TopPos+17-(YL/2));
	Canvas.DrawColor.R = 0;
	Canvas.DrawText(FormatTime());
	TopPos += 64;
	Canvas.DrawColor = WhiteColor;

	for ( i=0; i<WorldInfo.GRI.PRIArray.Length; i++ )
	{
		PRI = UTSurvivalPRI(WorldInfo.GRI.PriArray[i]);

		if ( !PRI.bOnlySpectator )
		{
			if (PRI.Team!=None)
			{
				if (PRI.Team.TeamIndex==0)
					Survivor = PRI;
				else if (PRI.Team.TeamIndex==1)
				{
					Challengers.Insert(0,1);
					Challengers[0]=PRI;
				}
			}
			else
			{
				for (j=0;j<Queue.Length;j++)
					if (PRI.QueuePosition < Queue[j].QueuePosition)
						break;

				if (j==Queue.Length)
					Queue.Length = j+1;
				else
					Queue.Insert(j,1);

				Queue[j]=PRI;
			}
		}
	}

	if (Survivor!=None)
	{
		Canvas.SetPos(LeftPos,TopPos);
		Canvas.DrawText(Survivor.PlayerName$": "$int(Survivor.Score));
		TopPos+=YL;

		if (Challengers.Length>0)
		{
			Canvas.SetPos(LeftPos,TopPos);
			Canvas.DrawText("-VS-");
			TopPos+=YL;

			for (i=0;i<Challengers.Length;i++)
			{
				Canvas.SetPos(LeftPos,TopPos);
				Canvas.DrawText(Challengers[i].PlayerName$": "$int(Challengers[i].Score));
				TopPos+=YL;
			}
		}
		else
		{
			Canvas.SetPos(LeftPos,TopPos);
			Canvas.DrawText("** Waiting **");
			TopPos+=YL;
		}
	}
	else if (Challengers.Length>0)
	{
		// Debug Log

        `log("ERROR: Challengers without a Survivor"@Challengers.Length);
    }

	TopPos+=YL;

	if (Queue.Length==0)
		return;

	Canvas.SetPos(LeftPos,TopPos);
	Canvas.DrawText("Waiting in Que");
	TopPos+=YL;

	Canvas.SetPos(LeftPos,TopPos);
	Canvas.DrawText("===============");
	TopPos+=YL;

	Canvas.SetDrawColor(200,200,200,255);
	for (i=0;i<Queue.Length;i++)
	{
		Canvas.SetPos(LeftPos,TopPos);
		Canvas.DrawText(Queue[i].PlayerName$": "$int(Queue[i].Score));
		TopPos+=YL;
	}
}

exec function showallpri()
{
	local UTSurvivalPRI PRI;
	local int i;
	for ( i=0; i<WorldInfo.GRI.PRIArray.Length; i++ )
	{
		PRI = UTSurvivalPRI(WorldInfo.GRI.PriArray[i]);
		if (PRI.Team!=none)
			`log("#### "@PRI.PlayerName@PRI.Team.TeamIndex);
		else
			`log("#### "@PRI.PlayerName@"No Team");
	}
}

defaultproperties
{
}
*/
