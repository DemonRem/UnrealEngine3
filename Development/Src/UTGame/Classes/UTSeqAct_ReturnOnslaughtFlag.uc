/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqAct_ReturnOnslaughtFlag extends SequenceAction;

/** the PowerCore belonging to the team whose flag should be returned */
var() UTOnslaughtPowerCore FlagOwnerCore;

event Activated()
{
	local UTOnslaughtGame Game;
	local UTCarriedObject Flag;

	if (FlagOwnerCore == None)
	{
		ScriptLog("Error: Flag Owner's PowerCore not specified");
	}
	else if (FlagOwnerCore.DefenderTeamIndex < ArrayCount(Game.Teams))
	{
		Game = UTOnslaughtGame(GetWorldInfo().Game);
		Flag = Game.Teams[FlagOwnerCore.DefenderTeamIndex].TeamFlag;
		if (Flag != None)
		{
			Flag.KismetSendHome();
		}
	}
}

defaultproperties
{
	bCallHandler=false
	ObjName="Return Onslaught Orb"
	ObjCategory="Objective"

	VariableLinks[0]=(ExpectedType=class'SeqVar_Object',LinkDesc="Flag Owner's PowerCore",PropertyName=FlagOwnerCore,MinVars=1,MaxVars=1)
}
