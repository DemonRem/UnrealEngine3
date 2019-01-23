/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_Clock extends HudWidget_SingleValue;

var transient string TimeString;

function bool GetText(out string ValueStr, float DeltaTime)
{
	local bool bResult;
	local WorldInfo WI;
	local GameReplicationInfo GRI;
	local int Seconds, Hours, Mins;
	local string NewTimeString;

	WI = UTSceneOwner.GetWorldInfo();

	if ( WI != none )
	{
		GRI = WI.GRI;
		if (GRI != none )
		{
			Seconds = GRI.TimeLimit != 0 ? GRI.RemainingTime : GRI.ElapsedTime;
			Hours = Seconds / 3600;
			Seconds -= Hours * 3600;
			Mins = Seconds / 60;
			Seconds -= Mins * 60;

			NewTimeString = "" $ ( Hours > 9 ? String(Hours) : "0"$String(Hours)) $ ":";
			NewTimeString = NewTimeString $ ( Mins > 9 ? String(Mins) : "0"$String(Mins)) $ ":";
			NewTimeString = NewTimeString $ ( Seconds > 9 ? String(Seconds) : "0"$String(Seconds));
		}
		else
		{
			NewTimeString = "00:00";
		}
	}
	else
	{
		NewTimeString = "00:00";
	}

	bResult = ( TimeString != NewTimeString );
	TimeString = NewTimeString;
	ValueStr = TimeString;
	return bResult;
}

defaultproperties
{
	WidgetTag=ClockWidget
}
