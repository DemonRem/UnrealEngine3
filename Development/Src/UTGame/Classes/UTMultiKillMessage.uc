/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTMultiKillMessage extends UTLocalMessage;

var	localized string 	KillString[6];
var SoundNodeWave KillSound[6];

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if ( class'UTPlayerController'.default.bNoMatureLanguage )
		return Default.KillString[Min(Switch,6)-1];

	else
		return Default.KillString[Min(Switch,7)-1];
}

static simulated function ClientReceive(
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (P.GamePlayEndedState())
	{
		`Warn("Possible incorrect multikill message" @ P @ Switch @ RelatedPRI_1 @ RelatedPRI_2 @ RelatedPRI_1.PlayerName @ RelatedPRI_2.PlayerName);
		ScriptTrace();
	}

	if ( Switch < 2 )
		UTPlayerController(P).ClientMusicEvent(9);
	else if ( Switch < 4 )
		UTPlayerController(P).ClientMusicEvent(11);
	else
		UTPlayerController(P).ClientMusicEvent(12);

	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);
	if ( class'UTPlayerController'.default.bNoMatureLanguage )
		UTPlayerController(P).PlayAnnouncement(default.class, Min(Switch-1,4));
	else
		UTPlayerController(P).PlayAnnouncement(default.class, Min(Switch-1,5));
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return Default.KillSound[MessageIndex];
}

static function int GetFontSize( int Switch, PlayerReplicationInfo RelatedPRI1, PlayerReplicationInfo RelatedPRI2, PlayerReplicationInfo LocalPlayer )
{
	if ( Switch <= 4 )
		return 2;
	if ( Switch == 7 )
		return 4;
	return 3;
}

defaultproperties
{
	KillSound(0)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_DoubleKill'
	KillSound(1)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MultiKill'
	KillSound(2)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MegaKill'
	KillSound(3)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_UltraKill'
	KillSound(4)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MonsterKill'
	KillSound(5)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_HolyShit'
	bIsSpecial=True
	bIsUnique=True
	Lifetime=3
	bBeep=False

	DrawColor=(R=255,G=0,B=0)
	FontSize=3

	MessageArea=2
}
