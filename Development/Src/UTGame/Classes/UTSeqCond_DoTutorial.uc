/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqCond_DoTutorial extends SequenceCondition;

event Activated()
{
	local bool bTutorialOK;
	local WorldInfo WI;
	local int PlayerCnt;
	local PlayerController PC;

	WI = GetWorldInfo();

	bTutorialOK = false;
	if ( WI != none && WI.GRI != none && UTGameReplicationInfo(WI.GRI) != none )
	{

		// If we are in story mode the tutorial is ok.

		bTutorialOK = UTGameReplicationInfo(WI.GRI).bStoryMode;

		// Count the # of players.

		foreach WI.AllControllers(class'PlayerController', PC)
		{
			PlayerCnt++;
		}

		// If we are > 1 then it's not ok

		if (PlayerCnt>1)
		{
			bTutorialOk = false;
		}
	}


	OutputLinks[ bTutorialOK ? 0 : 1].bHasImpulse = true;
}


defaultproperties
{
	ObjName="Do Tutorial"
	OutputLinks(0)=(LinkDesc="Play Tutorial")
	OutputLinks(1)=(LinkDesc="Abort Tutorial")
}


