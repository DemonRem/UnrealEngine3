/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPlayerNameMessage extends UTLocalMessage;

static function string GetString(
    optional int Switch,
	optional bool bPRI1HUD,
    optional PlayerReplicationInfo RelatedPRI_1, 
    optional PlayerReplicationInfo RelatedPRI_2,
    optional Object OptionalObject
    )
{
	if ( RelatedPRI_1 == None )
		return "";

    return RelatedPRI_1.PlayerName;
}

defaultproperties
{
	bIsUnique=True
	FontSize=2
	bBeep=False
    Lifetime=1.5
	bIsConsoleMessage=false

    DrawColor=(R=0,G=200,B=0,A=200)
    PosY=0.58
}
