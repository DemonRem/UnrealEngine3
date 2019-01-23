/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqAct_ChangeOrbBase extends SequenceAction;

/** the PowerCore belonging to the team whose orb's base should be changed */
var() UTOnslaughtPowerCore FlagOwnerCore;
/** the new base to set it to */
var() UTOnslaughtFlagBase NewBase;

event Activated()
{
	local UTOnslaughtGame Game;
	local UTOnslaughtFlag Flag;

	if (FlagOwnerCore == None)
	{
		ScriptLog("Error: Flag Owner's PowerCore not specified");
	}
	else if (NewBase == None)
	{
		ScriptLog("Error: No valid base was connected to the action");
	}
	else if (NewBase.GetTeamNum() != FlagOwnerCore.DefenderTeamIndex)
	{
		ScriptLog("Error: Can't move to new base" @ NewBase @ "because it is on a different team than the orb being moved");
	}
	else if (FlagOwnerCore.DefenderTeamIndex < ArrayCount(Game.Teams))
	{
		Game = UTOnslaughtGame(FlagOwnerCore.WorldInfo.Game);
		Flag = UTOnslaughtFlag(Game.Teams[FlagOwnerCore.DefenderTeamIndex].TeamFlag);
		if (Flag != None)
		{
			UTOnslaughtFlagBase(Flag.HomeBase).myFlag = None;
			Flag.HomeBase = NewBase;
			NewBase.myFlag = Flag;
			if (Flag.bHome)
			{
				Flag.SetLocation(NewBase.Location);
				Flag.SetRotation(NewBase.Rotation);
				NewBase.ObjectiveChanged();
			}
			Flag.NetUpdateTime = Flag.WorldInfo.TimeSeconds - 1.0;
		}
	}
}

defaultproperties
{
	bCallHandler=false
	ObjCategory="Objective"
	ObjName="Change Orb Base"
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Flag Owner's PowerCore",MinVars=1,MaxVars=1,PropertyName=FlagOwnerCore)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="New Base",MinVars=1,MaxVars=1,PropertyName=NewBase)
}
