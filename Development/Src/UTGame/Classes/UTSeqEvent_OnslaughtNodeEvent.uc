/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqEvent_OnslaughtNodeEvent extends SequenceEvent;

function NotifyNodeChanged(Controller EventInstigator)
{
	local UTOnslaughtNodeObjective Node;
	local array<int> ActivateIndices;
	local SeqVar_Object ObjVar;
	local UTOnslaughtPowerCore Core;

	Node = UTOnslaughtNodeObjective(Originator);
	if (Node == None)
	{
		`Warn("No Onslaught Node connected to" @ self);
	}
	else
	{
		// figure out which output to activate
		if (Node.IsConstructing())
		{
			ActivateIndices[0] = 0;
		}
		else if (Node.IsActive())
		{
			ActivateIndices[0] = 1;
		}
		else if (Node.IsCurrentlyDestroyed() || Node.IsNeutral())
		{
			ActivateIndices[0] = 2;
		}
		// activate the event
		if (CheckActivate(Originator, EventInstigator, false, ActivateIndices))
		{
			// set variables linked to "Node Team" to the main base of the team that owns the node
			// (can't use team number because when switching sides after a round that'll be backwards)
			if (Node.IsConstructing() || Node.IsActive())
			{
				Core = UTOnslaughtGame(Node.WorldInfo.Game).PowerCore[Node.GetTeamNum()];
			}
			else
			{
				Core = None;
			}
			foreach LinkedVariables(class'SeqVar_Object', ObjVar, "Node Team's PowerCore")
			{
				ObjVar.SetObjectValue(Core);
			}
		}
	}
}

defaultproperties
{
	ObjName="Onslaught Node Event"
	ObjCategory="Objective"
	bPlayerOnly=false
	MaxTriggerCount=0
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Node Team's PowerCore",bWriteable=true)
	OutputLinks[0]=(LinkDesc="Started Building")
	OutputLinks[1]=(LinkDesc="Finished Building")
	OutputLinks[2]=(LinkDesc="Destroyed")
	ObjClassVersion=2
}
