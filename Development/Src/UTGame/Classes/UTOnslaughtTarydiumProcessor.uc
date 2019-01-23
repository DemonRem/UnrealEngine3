/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtTarydiumProcessor extends UTOnslaughtNodeEnhancement
	abstract;

var() UTOnslaughtTarydiumMine Mine;
var()	float	OreEventThreshold;
var		float	OreCount;
const MAXMINERS = 4;
var UTOnslaughtMiningRobot MiningBots[MAXMINERS];
var bool bBuildingBot;

var class<UTOnslaughtMiningRobot> MiningBotClass;

simulated function PostBeginPlay()
{
	local UTOnslaughtTarydiumMine M;
	local float BestDist, NewDist;

	super.PostBeginPlay();

	if ( Mine == None )
	{
		// find closest mine
		ForEach WorldInfo.AllNavigationPoints(class'UTOnslaughtTarydiumMine', M)
		{
			NewDist = VSize(M.Location - Location);
			if ( (Mine == None) || (NewDist < BestDist) )
			{
				Mine = M;
				BestDist = NewDist;
			}
		}
	}
}

function ReceiveOre(float Quantity)
{
	local int i;
	local float Count, OreShare;
	local UTOnslaughtObjective O;
	local UTOnslaughtGame TheGame;

	// count received ore for Kismet event
	OreCount += Quantity;
	if ( OreCount >= OreEventThreshold )
	{
		OreCount -= OreEventThreshold;
		TriggerEventClass(class'UTSeqEvent_MinedOre', None);
	}

	if (ControllingNode != None && ControllingNode.IsActive())
	{
		// count how many team controlled objectives accept tarydium boosts
		count = 0;
		TheGame = UTOnslaughtGame(WorldInfo.Game);
		for ( i=0; i<TheGame.PowerNodes.Length; i++ )
		{
			O = TheGame.PowerNodes[i];
			if ( O.IsActive() && WorldInfo.GRI.OnSameTeam(ControllingNode, O) )
			{
				Count += 1;
			}
		}

		// give a boost to each team controlled objective which accepts it
		OreShare = Quantity/Count;
		TheGame.PowerCore[ControllingNode.DefenderTeamIndex].ProcessedTarydium += Quantity;
		for ( i=0; i<TheGame.PowerNodes.Length; i++ )
		{
			TheGame.PowerNodes[i].TarydiumBoost(OreShare);
		}
	}
}

function MinerDestroyed()
{
	if ( bBuildingBot )
		return;

	CreateMiner(10);
}

function Activate()
{
	CreateMiner(3);
}

function CreateMiner(float MinerCreationTime)
{
	SetTimer(MinerCreationTime, false, 'SpawnMiner');
}

function SpawnMiner()
{
	local int i, Count, Slot;
	local vector X,Y,Z;

	Slot = -1;
	for ( i=0; i<MAXMINERS; i++ )
	{
		if ( (MiningBots[i] != None) && !MiningBots[i].bDeleteMe )
			Count++;
		else if ( Slot == -1 )
			Slot = i;
	}

	if ( Count == MAXMINERS )
		return;

	GetAxes(Rotation, X,Y,Z);
	MiningBots[Slot] = spawn(MiningBotClass,,, Location + 300*X);
	if ( MiningBots[Slot] != None && !MiningBots[Slot].bDeleteMe )
	{
		MiningBots[Slot].Home = self;
		Count++;
	}

	if ( Count < MAXMINERS )
		CreateMiner(5);
}


function Deactivate()
{
	local int i;

	for ( i=0; i<MAXMINERS; i++ )
	{
		if ( (MiningBots[i] != None) && !MiningBots[i].bDeleteMe )
			MiningBots[i].Destroy();
	}
}

simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected )
{
	if (ControllingNode != None)
	{
		Canvas.Draw2DLine(ControllingNode.HUDLocation.X, ControllingNode.HUDLocation.Y, Mine.HUDLocation.X, Mine.HUDLocation.Y, Mine.MineColor);
	}

	bAlreadyRendered = true;
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
