/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleCantCarryFlagMessage extends UTLocalMessage;

var localized string Message;
var SoundNodeWave Announcement;

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	Announcer.PrecacheSound(default.Announcement);
}

static simulated function ClientReceive( PlayerController P, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1,
						optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	UTPlayerController(P).PlayAnnouncement(default.class, Switch);
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return default.Announcement;
}

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
}

static function string GetString( optional int Switch, optional bool bPRI1HUD, optional PlayerReplicationInfo RelatedPRI_1,
					optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	return default.Message;
}

defaultproperties
{
	Announcement=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_CantCarryFlagInVehicle'

	bIsUnique=false
	FontSize=2
	MessageArea=2
	bBeep=false
	DrawColor=(R=0,G=160,B=255,A=255)
}
