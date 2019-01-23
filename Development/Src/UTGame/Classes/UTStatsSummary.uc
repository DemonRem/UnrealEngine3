/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 *
 * This is the base subclass for all stats summaries.  It's job is it take the current crop of stats data
 * and generate human readable summaries from it.
*/

class UTStatsSummary extends Object within UTGameStats
	abstract;


/**
 * Create a string from a time.  Copied form the hud
 */


function string TimeStr(float TimeToConvert)
{
	local int Minutes, Hours, Seconds;
	local string Result;

	Seconds =  TimeToConvert;

	if( Seconds > 3600 )
    {
        Hours = Seconds / 3600;
        Seconds -= Hours * 3600;
		Result = string(Hours)$":";
	}
	Minutes = Seconds / 60;
    Seconds -= Minutes * 60;

	if ( Minutes < 10 )
	{
		if ( Hours == 0 )
			Result = Result$" ";
		else
			Result = Result$"0";
	}
	Result = Result$Minutes$":";

	if ( Seconds < 10 )
	{
		Result = Result$"0";
	}
	Result = Result$Seconds;

	return Result;
}


function GenerateSummary();

defaultproperties
{
}
