/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_Frags extends HudWidget_SingleValue;

var transient int OldFragCount;


function bool GetText(out string ValueStr, float DeltaTime)
{
	local bool bResult;
	local PlayerReplicationInfo PRI;
	local int FragCount;

	PRI = UTHudSceneOwner.GetPRIOwner();
	if ( PRI != none )
	{
		FragCount = PRI.Score;
	}
	else
	{
		FragCount = 0;
	}

	bResult = (OldFragCount != FragCount);
	OldFragCount = FragCount;
	ValueStr = String(FragCount);
	return bResult;
}

defaultproperties
{
	WidgetTag=FragsWidget
	bVisibleBeforeMatch=false
	bVisibleDuringMatch=true

}
