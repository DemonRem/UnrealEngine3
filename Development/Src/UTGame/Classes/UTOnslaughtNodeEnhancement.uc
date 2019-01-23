/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtNodeEnhancement extends UTGameObjective
	abstract;

var() UTOnslaughtNodeObjective ControllingNode;	/** Level designer can set, or it is auto-assigned to nearest bunker */

replication
{
	if (bNetInitial)
		ControllingNode;
}

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	if ( (Role == ROLE_Authority) && (ControllingNode != None) )
	{
		SetControllingNode(ControllingNode);
	}
}

simulated function string GetLocationStringFor(PlayerReplicationInfo PRI)
{
	if ( ControllingNode != None )
	{
		return ControllingNode.GetLocationStringFor(PRI);
	}
	return LocationPrefix$GetHumanReadableName()$LocationPostfix;
}

function BetweenEndPointFor(Pawn P, out actor MainLocationHint, out actor SecondaryLocationHint)
{
	if ( ControllingNode != None )
	{
		MainLocationHint = ControllingNode;
		ControllingNode.BetweenEndPointFor(P, MainLocationHint, SecondaryLocationHint);
	}
}

function SetControllingNode(UTOnslaughtNodeObjective NewControllingNode)
{
	ControllingNode = NewControllingNode;
	ControllingNode.Enhancements[ControllingNode.Enhancements.Length] = self;
}

simulated function UpdateTeamEffects();

function Activate()
{
	DefenderTeamIndex = GetTeamNum();
	UpdateTeamEffects();
}

function Deactivate()
{
	DefenderTeamIndex = 2;
	UpdateTeamEffects();
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DefenderTeamIndex')
	{
		UpdateTeamEffects();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function TarydiumBoost(float Quantity);

simulated function byte GetTeamNum()
{
	return (ControllingNode != None) ? ControllingNode.GetTeamNum() : byte(255);
}

defaultproperties
{
	HUDMaterial=None
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=true
	bMovable=false

	bStatic=false
	bHidden=false
	bCollideActors=true
	bCollideWorld=true
	bBlockActors=true
}
