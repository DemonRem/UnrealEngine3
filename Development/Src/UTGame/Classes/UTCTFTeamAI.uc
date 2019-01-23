/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFTeamAI extends UTTeamAI;

var UTCTFFlag FriendlyFlag, EnemyFlag;
var float LastGotFlag;

function UTSquadAI AddSquadWithLeader(Controller C, UTGameObjective O)
{
	local UTCTFSquadAI S;

	if ( O == None )
		O = EnemyFlag.HomeBase;
	S = UTCTFSquadAI(Super.AddSquadWithLeader(C,O));
	if ( S != None )
	{
		S.FriendlyFlag = FriendlyFlag;
		S.EnemyFlag = EnemyFlag;
	}
	return S;
}

defaultproperties
{
	OrderList(0)=ATTACK
	OrderList(1)=DEFEND
	OrderList(2)=ATTACK
	OrderList(3)=ATTACK
	OrderList(4)=ATTACK
	OrderList(5)=DEFEND
	OrderList(6)=FREELANCE
	OrderList(7)=ATTACK
	SquadType=class'UTGame.UTCTFSquadAI'
}
