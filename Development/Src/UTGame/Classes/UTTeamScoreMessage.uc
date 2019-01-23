/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTeamScoreMessage extends UTLocalMessage;

var SoundNodeWave TeamScoreSounds[7];

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	local int i;

	for (i = 0; i < 7; i++)
	{
		Announcer.PrecacheSound(Default.TeamScoreSounds[i]);
	}
}

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
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

	if ( Switch < 2 )
		UTPlayerController(P).ClientMusicEvent(13);
	else if ( Switch < 4 )
		UTPlayerController(P).ClientMusicEvent(14);
	else
		UTPlayerController(P).ClientMusicEvent(15);
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return Default.TeamScoreSounds[MessageIndex];
}
defaultproperties
{
	TeamScoreSounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedTeamScores'
	TeamScoreSounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueTeamScores'
	TeamScoreSounds(2)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedTeamIncreasesTheirLead'
	TeamScoreSounds(3)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueTeamIncreasesTheirLead'
	TeamScoreSounds(4)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedTeamTakesTheLead'
	TeamScoreSounds(5)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueTeamTakesTheLead'
	TeamScoreSounds(6)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_HatTrick'
}
