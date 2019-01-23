/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeaponRewardMessage extends UTLocalMessage;

var	localized string 	RewardString[4];
var SoundNodeWave RewardSounds[4];

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	return Default.RewardString[Switch];
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
	UTPlayerController(P).PlayAnnouncement(default.class, Switch);
	UTPlayerController(P).ClientMusicEvent(6);
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return Default.RewardSounds[MessageIndex];
}

defaultproperties
{
	RewardSounds(0)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_HeadShot'
	RewardSounds(1)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_FlakMonkey'
	RewardSounds(2)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_HeadHunter'
	RewardSounds(3)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_ComboWhore'
	bIsSpecial=True
	bIsUnique=True
	Lifetime=3
	bBeep=False

	DrawColor=(R=255,G=255,B=0)
	FontSize=2

	MessageArea=3
}
