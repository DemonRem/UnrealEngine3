/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqVar_Player extends SeqVar_Object
	native(Sequence);

cpptext
{
	UObject** GetObjectRef( INT Idx );

	virtual FString GetValueStr()
	{
		if (!bAllPlayers)
		{
			return FString::Printf(TEXT("Player %d"),PlayerIdx);
		}
		else
		{
			return FString(TEXT("Player"));
		}
	}

	virtual UBOOL SupportsProperty(UProperty *Property)
	{
		return FALSE;
	}
};

/** Local list of players in the game */
var transient array<Object> Players;

/** Return all player references? */
var() bool bAllPlayers;

/** Individual player selection for multiplayer scripting */
var() int PlayerIdx;

function Object GetObjectValue()
{
	local PlayerController PC;

	PC = PlayerController(ObjValue);
	if (PC == None)
	{
		foreach GetWorldInfo().AllControllers(class'PlayerController', PC)
		{
			ObjValue = PC;
			break;
		}
	}

	// we usually want the pawn, so return that if possible
	return (PC.Pawn != None) ? PC.Pawn : PC;
}

defaultproperties
{
	ObjName="Player"
	ObjCategory="Object"
	bAllPlayers=TRUE
}
