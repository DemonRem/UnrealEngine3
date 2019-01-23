/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtHUDMessage extends UTLocalMessage;

// Switch 0: You have the flag message.

var(Message) localized string OnslaughtHUDString[3];

var(Message) color RedColor, YellowColor;

static function color GetColor(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	return (Switch == 1) ? default.RedColor : default.YellowColor;
}

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	Return Default.OnslaughtHUDString[Switch];
}

static function AddAnnouncement(UTAnnouncer Announcer, byte MessageIndex, optional Object OptionalObject) {}

defaultproperties
{
	bIsUnique=false
	bIsPartiallyUnique=true
	bIsConsoleMessage=False
	Lifetime=1

	YellowColor=(R=255,G=255,B=0,A=255)
	RedColor=(R=255,G=0,B=0,A=255)
	DrawColor=(R=0,G=160,B=255,A=255)
	FontSize=1

	MessageArea=0
}
