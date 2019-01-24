/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

class AICommandNodeRoot extends AICommandNodeBase
	native(AI);

cpptext
{
#if WITH_EDITOR
	virtual FString GetDisplayName();
	virtual void CreateAutoConnectors();
#endif
};

var() Name RootName;

defaultproperties
{
	RootName=AITree
}

