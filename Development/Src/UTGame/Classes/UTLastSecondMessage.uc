/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTLastSecondMessage extends UTLocalMessage;

var localized string LastSecondRed, LastSecondBlue;

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if ( TeamInfo(OptionalObject) == None )
		return "";
	if ( TeamInfo(OptionalObject).TeamIndex == 0 )
		return Default.LastSecondRed;
	else
		return Default.LastSecondBlue;
}

static simulated function ClientReceive(
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	UTPlayerController(P).PlayAnnouncement(default.class,Switch );

	if ( P.PlayerReplicationInfo == RelatedPRI_1 )
		UTPlayerController(P).ClientMusicEvent(2);
	else
		UTPlayerController(P).ClientMusicEvent(10);
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	if ( MessageIndex == 1 )
		return SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Denied';
	else
		return SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_LastSecondSave';
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	Announcer.PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_LastSecondSave');
	Announcer.PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Denied');
}

defaultproperties
{
	bBeep=false
	bIsUnique=True
	FontSize=2
	MessageArea=2
	DrawColor=(R=0,G=160,B=255,A=255)
}
