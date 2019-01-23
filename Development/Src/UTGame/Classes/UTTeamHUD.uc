/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTeamHUD extends UTHUD;


var config bool bShowTeamScore;
var config bool bShowDirectional;
var int LastScores[2];
var int ScoreTransitionTime[2];
var vector2D TeamIconCenterPoints[2];

/** The scaling modifier will be applied to the widget that coorsponds to the player's team */
var() float TeamScaleModifier;



function DisplayScoring()
{
	Super.DisplayScoring();

	if ( bShowTeamScore )
	{
		DisplayTeamScore();
	}
}

function DisplayTeamScore()
{
	local float HudCenterX, VsSize, DestScale, W, H, PulseScale;
	local vector2d POS, Logo;
	local byte TeamIndex;

	local LinearColor TeamLC;
	local color TextC;

	// Display the VS to get out positioning

	HudCenterX = Canvas.ClipX * 0.5;
	VsSize = 9.5 * ResolutionScale;
	POS.Y = 0;

	// Draw the VS.

	Canvas.SetPos(HudCenterX - VsSize, 0);
	Canvas.SetDrawColor(255,255,255,255);
	Canvas.DrawTile(AltHudTexture,19 * ResolutionScale, 17 * ResolutionScale, 109, 9, 19, 17);


	// Draw the Left Team Indicator

		DestScale = TeamScaleModifier;


		PulseScale = 1.0;
		TeamIndex = UTPlayerOwner.GetTeamNum();
		GetTeamColor(TeamIndex, TeamLC, TextC);

        W = 146 * DestScale * ResolutionScale;
        H = 63 * DestScale * ResolutionScale;

		POS.X = HudCenterX - VsSize - W;

		Canvas.SetPos(POS.X,POS.Y);
		Canvas.DrawColorizedTile(AltHudTexture, W, H, 0, 0, 109, 52, TeamLC);



		DrawGlowText(""$GetTeamScore(TeamIndex), POS.X + 90 * DestScale * ResolutionScale, POS.Y , 42 * DestScale * ResolutionScale, PulseScale,true);

        Logo.X = POS.X + (TeamIconCenterPoints[0].X * DestScale);
        Logo.Y = POS.Y + (TeamIconCenterPoints[0].Y * DestScale);

       	DisplayTeamLogos(TeamIndex,Logo, DestScale);


	// Draw the Right Team Indicator

		PulseScale = 1.0;
		TeamIndex = Abs(1.0 - TeamIndex);

		GetTeamColor(TeamIndex, TeamLC, TextC);


        W = 146 * ResolutionScale;
        H = 63 * ResolutionScale;

		POS.X = HudCenterX + VsSize;

		Canvas.SetPos(POS.X,POS.Y);
		Canvas.DrawColorizedTile(AltHudTexture, W, H, 128, 0, 109, 52, TeamLC);

		Canvas.DrawColor = WhiteColor;
		DrawGlowText(""$GetTeamScore(TeamIndex), POS.X + 57 * ResolutionScale, POS.Y, 42*ResolutionScale, PulseScale);

        Logo.X = POS.X + TeamIconCenterPoints[1].X;
        Logo.Y = POS.Y + TeamIconCenterPoints[1].Y;
       	DisplayTeamLogos(TeamIndex,Logo);

}

function int GetTeamScore(byte TeamIndex)
{
	return INT(UTGRI.Teams[TeamIndex].Score);

}

function Actor GetDirectionalDest(byte TeamIndex)
{
	return none;
}


function DisplayTeamLogos(byte TeamIndex, vector2d POS, optional float DestScale=1.0)
{
	if ( bShowDirectional )
	{
		DisplayDirectionIndicator(POS, GetDirectionalDest(TeamIndex), DestScale );
	}
}

function DisplayDirectionIndicator(vector2D POS, Actor DestActor, float DestScale)
{
	local rotator Dir,Angle;
	local vector start;

	if ( DestActor != none )
	{
		Start = (PawnOwner != none) ? PawnOwner.Location : UTPlayerOwner.Location;
		Dir  = Rotator(DestActor.Location - Start);
		Angle.Yaw = (Dir.Yaw - PlayerOwner.Rotation.Yaw) & 65535;

        Canvas.SetDrawColor(64,255,64,255);
		Canvas.SetPos(POS.X, POS.Y);
		Canvas.DrawRotatedTile( AltHudTexture, Angle, 57 * DestScale * ResolutionScale, 52 * DestScale * ResolutionScale, 897, 452, 43, 43);
	}
}




defaultproperties
{
	ScoreboardSceneTemplate=UTUIScene_TeamScoreboard'UI_Scenes_Scoreboards.sbTeamDM'
	TeamScaleModifier=1.5

	TeamIconCenterPoints(0)=(x=89.0,y=15.0)
	TeamIconCenterPoints(1)=(x=0,y=15)

}

