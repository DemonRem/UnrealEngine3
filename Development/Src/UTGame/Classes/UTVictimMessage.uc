/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVictimMessage extends UTWeaponKillMessage;

var(Message) localized string YouWereKilledBy, KilledByTrailer, OrbSuicideString;

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject 
	)
{
	if (RelatedPRI_1 == None)
	{
		if ( class<UTDmgType_OrbReturn>(OptionalObject) != None )
			return default.OrbSuicideString;
		return "";
	}

	if (RelatedPRI_1.PlayerName != "")
		return Default.YouWereKilledBy@RelatedPRI_1.PlayerName$Default.KilledByTrailer;
}

defaultproperties
{
	bIsUnique=True
	Lifetime=6
	DrawColor=(R=255,G=0,B=0,A=255)
	FontSize=2
	MessageArea=1
}
