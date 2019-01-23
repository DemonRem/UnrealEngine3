/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** condition that activates an output depending on whether one of the link setups in a list is currently active */
class UTSeqCond_IsLinkSetupActive extends SequenceCondition;

/** the list of link setups to check */
var() array<name> LinkSetupsToCheck;

event Activated()
{
	local int i;
	local UTOnslaughtGRI GRI;
	local bool bLinkSetupActive;

	GRI = UTOnslaughtGRI(GetWorldInfo().GRI);
	if (GRI != None)
	{
		for (i = 0; i < LinkSetupsToCheck.length; i++)
		{
			if (LinkSetupsToCheck[i] == GRI.LinkSetupName)
			{
				bLinkSetupActive = true;
				break;
			}
		}
	}

	OutputLinks[bLinkSetupActive ? 0 : 1].bHasImpulse = true;
}

defaultproperties
{
	ObjName="Is Link Setup Active"
	OutputLinks(0)=(LinkDesc="Active")
	OutputLinks(1)=(LinkDesc="Not Active")
}
