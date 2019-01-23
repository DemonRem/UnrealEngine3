/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFHUD extends UTTeamHUD;

var UTCTFBase FlagBases[2];
var EFlagState FlagStates[2];

simulated function PostBeginPlay()
{
	local UTCTFBase CTFBase;
	Super.PostBeginPlay();
	ForEach WorldInfo.AllNavigationPoints(class'UTCTFBase', CTFBase)
	{
		if (CTFBase.DefenderTeamIndex < 2)
		{
			FlagBases[CTFBase.DefenderTeamIndex] = CTFBase;
		}
	}

	SetTimer(1.0, True);

}

simulated function Timer()
{
	local UTPlayerReplicationInfo PawnOwnerPRI;

	Super.Timer();

	if ( Pawn(PlayerOwner.ViewTarget) == None )
		return;

	PawnOwnerPRI = UTPlayerReplicationInfo(Pawn(PlayerOwner.ViewTarget).PlayerReplicationInfo);

    if ( (PawnOwnerPRI == None)
		|| (PlayerOwner.IsSpectating() && UTPlayerController(PlayerOwner).bBehindView) )
        return;

	if ( (UTGameReplicationInfo(WorldInfo.GRI) != None) && (PawnOwnerPRI.Team != None)
		&& UTGameReplicationInfo(WorldInfo.GRI).FlagIsHeldEnemy(PawnOwnerPRI.Team.TeamIndex) )
	{
		if ( PawnOwnerPRI.bHasFlag )
		{
			PlayerOwner.ReceiveLocalizedMessage( class'UTCTFHUDMessage', 2 );
		}
		else
		{
			PlayerOwner.ReceiveLocalizedMessage( class'UTCTFHUDMessage', 1 );
		}
	}
	else if ( PawnOwnerPRI.bHasFlag )
	{
		PlayerOwner.ReceiveLocalizedMessage( class'UTCTFHUDMessage', 0 );
	}


}

function DisplayTeamLogos(byte TeamIndex, vector2d POS, optional float DestScale=1.0)
{
	local LinearColor Alpha;
	Super.DisplayTeamLogos(TeamIndex, Pos, DestScale);

	Alpha = LC_White;
	Alpha.A = 0.25 + ( 0.75 * Abs(cos(WorldInfo.TimeSeconds * 3)));


    Canvas.SetPos(POS.X + 28 * ResolutionScale * DestScale, POS.Y + 26 * ResolutionScale * DestScale);
	switch ( UTGRI.FlagState[TeamIndex] )
	{
		case FLAG_Home:
			break;			// Display the flag icon when I have it

		case FLAG_Down:
			DrawTileCentered(HudTexture, 16 * DestScale, 27 * DestScale, 80,271,34,39,Alpha);
			break;

		case FLAG_HeldEnemy:
			DrawTileCentered(HudTexture, 16 * DestScale, 27 * DestScale, 344,214,21,55,Alpha);
			break;
	}
}


function Actor GetDirectionalDest(byte TeamIndex)
{
	return FlagBases[TeamIndex];
}

defaultproperties
{
	ScoreboardSceneTemplate=Scoreboard_CTF'UI_Scenes_Scoreboards.sbCTF'
}

