/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTeamGameMessage extends UTLocalMessage;

var localized string	      RequestTeamSwapPrefix;
var localized string	      RequestTeamSwapPostfix;

var localized string YouAreOnRedMessage;
var localized string YouAreOnBlueMessage;

var color RedDrawColor;
var color BlueDrawColor;

//@FIXME: needs precaching!
var SoundNodeWave AnnouncerSounds[2];


static simulated function ClientReceive(
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	if (Switch > 0 && default.AnnouncerSounds[Switch - 1] != None)
	{
		UTPlayerController(P).PlayAnnouncement(default.class, Switch);
		UTPlayerController(P).PulseTeamColor();
	}
}


//
// Messages common to GameInfo derivatives.
//
static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{

	switch (Switch)
	{
		case 0:
			return Default.RequestTeamSwapPrefix@RelatedPRI_1.PlayerName@Default.RequestTeamSwapPostfix;
			break;
		case 1:
			return Default.YouAreOnRedMessage;
			break;
		case 2:
			return Default.YouAreOnBlueMessage;
			break;
	}
	return "";
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	if (MessageIndex > 0)
	{
		return default.AnnouncerSounds[MessageIndex - 1];
	}
	else
	{
		return None;
	}
}

static function int GetFontSize( int Switch, PlayerReplicationInfo RelatedPRI1, PlayerReplicationInfo RelatedPRI2, PlayerReplicationInfo LocalPlayer )
{
	if (Switch > 0)
	{
		return 2;
	}
	else
	{
	    return default.FontSize;
	}
}

static function float GetPos( int Switch, HUD myHUD  )
{
	if ( Switch > 0 )
	{
		return UTHUD(myHUD).MessageOffset[Default.MessageArea];
	}
	else
	{
		return UTHUD(myHUD).MessageOffset[4];
	}
}

static function color GetColor(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	switch (Switch)
	{
		case 1:
			return default.RedDrawColor;
			break;
		case 2:
			return default.BlueDrawColor;
			break;
	}
	return Default.DrawColor;
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	Announcer.PrecacheSound(default.AnnouncerSounds[0]);
	Announcer.PrecacheSound(default.AnnouncerSounds[1]);
}

defaultproperties
{
	bIsConsoleMessage=true

	RedDrawColor=(R=255,G=32,B=10,A=255)
	BlueDrawColor=(R=10,G=32,B=255,A=255)

	AnnouncerSounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_YouAreOnRed'
	AnnouncerSounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_YouAreOnBlue'
	MessageArea=2
}
