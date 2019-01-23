/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVictoryMessage extends UTLocalMessage;

var SoundNodeWave VictorySounds[6];

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return Default.VictorySounds[MessageIndex];
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
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	Announcer.PrecacheSound(Default.VictorySounds[0]);
	Announcer.PrecacheSound(Default.VictorySounds[1]);
	if ( GameClass.Default.bTeamGame )
	{
		Announcer.PrecacheSound(Default.VictorySounds[4]);
		Announcer.PrecacheSound(Default.VictorySounds[5]);
	}
	else
	{
		Announcer.PrecacheSound(Default.VictorySounds[2]);
		Announcer.PrecacheSound(Default.VictorySounds[3]);
	}
}

defaultproperties
{
	VictorySounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_FlawlessVictory'
	VictorySounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_HumiliatingDefeat'
	VictorySounds(2)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_WonTheMatch'
	VictorySounds(3)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_LostTheMatch'
	VictorySounds(4)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedTeamWins'
	VictorySounds(5)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueTeamWinner'
	MessageArea=2
}
