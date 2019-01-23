/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTTDMScoreboardPanel extends UTScoreboardPanel;

function string GetLeftMisc(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		if (LeftMiscStr != "")
		{
			return UserString(LeftMiscStr, PRI);
		}
		else
		{
			if ( PRI.WorldInfo.GRI.OnSameTeam(PRI, PlayerOwner) )
			{
				return "Location:"@PRI.GetLocationName();
			}
			else
			{
				return "";
			}
		}
	}
	return "LMisc";
}



defaultproperties
{
	bDrawPlayerNum=false
}
