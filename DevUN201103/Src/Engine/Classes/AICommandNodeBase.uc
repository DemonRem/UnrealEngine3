/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

class AICommandNodeBase extends K2NodeBase
	native(AI);

cpptext
{
#if WITH_EDITOR
	virtual FString GetDisplayName();
	virtual void CreateAutoConnectors();
#endif
};

var() class<AICommandBase>      CommandClass;

/** DMC for utility function */
var() DMC_Prototype             UtilityDMC;


native function AICommandNodeBase   SelectBestChild( AIController InAI, out AITreeHandle Handle );


defaultproperties
{
}

