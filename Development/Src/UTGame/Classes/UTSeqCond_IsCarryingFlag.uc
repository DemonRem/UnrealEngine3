class UTSeqCond_IsCarryingFlag extends SequenceCondition;

var Actor Target;
var() UTGameObjective FlagBase;

event Activated()
{
	local UTPlayerReplicationInfo PRI;
	local Pawn P;
	local Controller C;
	local UTCarriedObject Flag;
	local UTOnslaughtFlagBase ONSBase;
	local UTTeamGame Game;
	local byte TeamNum;

	P = Pawn(Target);
	if (P != None)
	{
		PRI = UTPlayerReplicationInfo(P.PlayerReplicationInfo);
	}
	else
	{
		C = Controller(Target);
		if (C != None)
		{
			PRI = UTPlayerReplicationInfo(C.PlayerReplicationInfo);
		}
	}
	if (PRI == None)
	{
		ScriptLog("Invalid target specified for" @ self);
	}
	else
	{
		if (FlagBase != None)
		{
			Flag = FlagBase.GetFlag();
			if (Flag == None)
			{
				ONSBase = UTOnslaughtFlagBase(FlagBase);
				Game = UTTeamGame(GetWorldInfo().Game);
				if (ONSBase != None && Game != None)
				{
					// since ONS orbs move around, try looking up team number and get that flag
					TeamNum = ONSBase.GetTeamNum();
					if (TeamNum < ArrayCount(Game.Teams) && Game.Teams[TeamNum] != None)
					{
						Flag = Game.Teams[TeamNum].TeamFlag;
					}
				}
			}
		}

		OutputLinks[(PRI.bHasFlag && (Flag == None || PRI.GetFlag() == Flag)) ? 0 : 1].bHasImpulse = true;
	}
}

defaultproperties
{
	ObjName="Is Carrying Flag"
	OutputLinks(0)=(LinkDesc="Has Flag")
	OutputLinks(1)=(LinkDesc="No Flag")

	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Target",PropertyName=Target,MinVars=1,MaxVars=1)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Flag Base (Optional)",PropertyName=FlagBase,MinVars=1,MaxVars=1)
}
