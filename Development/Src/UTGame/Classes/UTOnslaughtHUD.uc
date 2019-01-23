/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtHUD extends UTTeamHUD;

var transient HudWidget_Map MapWidget;

var UTOnslaughtPowerCore PowerCore[2];
var float LastMouseMoveTime;
var array<UTOnslaughtPowerNode> PowerNodes;
var bool bPowerNodesInitialized;
var EFlagState FlagStates[2];

simulated function PostBeginPlay()
{

	GetPowerCores();

	Super.PostBeginPlay();
	SetTimer(1.0, True);
}

simulated function Timer()
{
	local UTPlayerReplicationInfo PawnOwnerPRI;
	local int i, NumNodes;
	local UTOnslaughtFlag EnemyFlag, CarriedFlag;
	local UTOnslaughtPowerNode Node;
	local UTTeamInfo EnemyTeam;

	Super.Timer();

	if ( PawnOwner == None )
		return;

	PawnOwnerPRI = UTPlayerReplicationInfo(PawnOwner.PlayerReplicationInfo);

	if ( PawnOwnerPRI == None || PawnOwnerPRI.Team == None
		|| (PlayerOwner.IsSpectating() && UTPlayerController(PlayerOwner).bBehindView) )
	{
		return;
	}

	if (!bPowerNodesInitialized)
	{
		bPowerNodesInitialized = true;
		foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtPowerNode',Node)
		{
			PowerNodes[PowerNodes.Length] = Node;
		}
	}

	if (WorldInfo.GRI != None)
	{
		EnemyTeam = UTTeamInfo(WorldInfo.GRI.Teams[1 - PawnOwnerPRI.Team.TeamIndex]);
		if (EnemyTeam != None)
		{
			NumNodes = PowerNodes.length;
			EnemyFlag = UTOnslaughtFlag(EnemyTeam.TeamFlag);
			if (EnemyFlag != None)
			{
				for (i = 0; i < NumNodes; i++)
				{
					if (PowerNodes[i].DefenderTeamIndex == PawnOwnerPRI.Team.TeamIndex)
					{
						if ( VSize(PowerNodes[i].Location - EnemyFlag.Location) < PowerNodes[i].MaxSensorRange
							 && VSize(PowerNodes[i].Location - PawnOwner.Location) < PowerNodes[i].MaxSensorRange
							 && PowerNodes[i].PoweredBy(1 - PawnOwnerPRI.Team.TeamIndex) )
						{
							PlayerOwner.ReceiveLocalizedMessage(class'UTOnslaughtHUDMessage', 1);
							break;
						}
					}
				}
			}
		}
	}

	if ( PawnOwnerPRI.bHasFlag )
	{
		CarriedFlag = UTOnslaughtFlag(UTTeamInfo(PawnOwnerPRI.Team).TeamFlag);
		if ( (CarriedFlag != None) && (CarriedFlag.LockedNode != None) )
		{
			CarriedFlag.LockedNode.VerifyOrbLock(CarriedFlag);
			if ( CarriedFlag.LockedNode != None )
			{
				PlayerOwner.ReceiveLocalizedMessage( class'UTOnslaughtHUDMessage', 2 );
			}
			else
			{
				PlayerOwner.ReceiveLocalizedMessage( class'UTOnslaughtHUDMessage', 0 );
			}
		}
		else
		{
			PlayerOwner.ReceiveLocalizedMessage( class'UTOnslaughtHUDMessage', 0 );
		}
	}
}

function AddLocalizedMessage
(
	int						Index,
	class<LocalMessage>		InMessageClass,
	string					CriticalString,
	int						Switch,
	float					Position,
	float					LifeTime,
	int						FontSize,
	color					DrawColor,
	optional int			MessageCount,
	optional object			OptionalObject
)
{
	Super.AddLocalizedMessage(Index, InMessageClass, CriticalString, Switch, Position, LifeTime, FontSize, DrawColor, MessageCount, OptionalObject);

	// highlight minimap node
	if ( InMessageClass.static.IsKeyObjectiveMessage(Switch) )
	{
		if ( UTGameObjective(OptionalObject) != None )
		{
			UTGameObjective(OptionalObject).HighlightOnMinimap(Switch);
		}
		else if ( (UTTeamInfo(OptionalObject) != None) && (UTTeamInfo(OptionalObject).TeamFlag != None) )
		{
			UTTeamInfo(OptionalObject).TeamFlag.HighlightOnMinimap(Switch);
		}
	}
}

function bool FindGround(out vector KnownVect)
{
	local vector HitLocation, HitNormal;
	local actor a;

	A = Trace(HitLocation, HitNormal, KnownVect + Vect(0,0,-50000), KnownVect + Vect(0,0,+50000),false);
	if (A!= none)
	{
		KnownVect = HitLocation;
		return true;
	}
	return false;
}

function GetPowerCores()
{
	local UTOnslaughtPowerCore Core;

	if (PowerCore[0] == None || PowerCore[1] == None || PowerCore[0].GetTeamNum() != 0 || PowerCore[1].GetTeamNum() != 1)
	{
		foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtPowerCore',Core)
		{
			if ( Core.DefenderTeamIndex < 2 )
			{
				PowerCore[Core.DefenderTeamIndex] = Core;
			}
		}
	}
}

exec function SetMapExtent(float value)
{
`if(`notdefined(FINAL_RELEASE))
	local float OldValue;
	OldValue = UTOnslaughtMapInfo(WorldInfo.GetMapInfo()).MapExtent;
`endif

    if (Value>0)
	{
		UTOnslaughtMapInfo(WorldInfo.GetMapInfo()).MapExtent = Value;
	}

	`log("### MapExtent:"@OldValue@Value);
}


function DisplayTeamScore()
{
	GetPowerCores();
	Super.DisplayTeamScore();
}

function DisplayTeamLogos(byte TeamIndex, vector2d POS, optional float DestScale=1.0)
{
	local LinearColor Alpha;
	local UTTeamInfo Team;
	local float XL,YL;
	local UTOnslaughtFlag Orb;

	Super.DisplayTeamLogos(TeamIndex, Pos, DestScale);

	Alpha = LC_White;
	Alpha.A = 0.25 + ( 0.75 * Abs(cos(WorldInfo.TimeSeconds * 3)));

	POS.X += 28 * ResolutionScale * DestScale;
	POS.Y += 26 * ResolutionScale * DestScale;

    Canvas.SetPos(POS.X, POS.Y);
	switch ( UTGRI.FlagState[TeamIndex] )
	{
		case FLAG_Home:
			DrawTileCentered(HudTexture, 16 * DestScale, 27 * DestScale, 344,214,21,55,Alpha);
			break;			// Display the flag icon when I have it

		case FLAG_Down:
			DrawTileCentered(HudTexture, 16 * DestScale, 27 * DestScale, 80,271,34,39,Alpha);

			Team = UTTeamInfo(WorldInfo.GRI.Teams[TeamIndex]);
			if (Team != None)
			{
				Orb = UTOnslaughtFlag(Team.TeamFlag);
				if (Orb != None && Orb.RemainingDropTime > 0)
				{
					Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);
					Canvas.StrLen(string(Orb.RemainingDropTime), XL, YL);

					Canvas.DrawColor = BlackColor;
					Canvas.SetPos(POS.X - (XL*0.5)+1, POS.Y - (YL * 0.5)+1);
					Canvas.DrawText(string(Orb.RemainingDropTime));

					Canvas.DrawColor = BlackColor;
					Canvas.SetPos(POS.X - (XL*0.5)-1, POS.Y - (YL * 0.5)-1);
					Canvas.DrawText(string(Orb.RemainingDropTime));


					Canvas.DrawColor = WhiteColor;
					Canvas.SetPos(POS.X - (XL*0.5), POS.Y - (YL * 0.5));
					Canvas.DrawText(string(Orb.RemainingDropTime));
				}
			}


			break;
	}
}

function int GetTeamScore(byte TeamIndex)
{
	local int Health;

	Health = PowerCore[TeamIndex].Health;
	Health = Health > 0 ? Max(1, 100 * Health / PowerCore[TeamIndex].DamageCapacity) : 0;
	return Health;
}


defaultproperties
{
	ScoreboardSceneTemplate=Scoreboard_ONS'UI_Scenes_Scoreboards.sbONS'
	MOTDSceneTemplate=UTUIScene_MOTD'UI_Scenes_HUD.Menus.MOTDONSMenu'
	FlagStates(0)=FLAG_Down
	FlagStates(1)=FLAG_Down
}

