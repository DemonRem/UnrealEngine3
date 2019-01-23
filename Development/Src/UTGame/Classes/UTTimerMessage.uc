/** this plays the "X minutes/seconds remaining" announcements */
class UTTimerMessage extends UTLocalMessage
	abstract;

var array<ObjectiveAnnouncementInfo> Announcements;

static simulated function ClientReceive( PlayerController P, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1,
					optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	if (default.Announcements[Switch].AnnouncementSound != None)
	{
		UTPlayerController(P).PlayAnnouncement(default.class, Switch);
	}
}

static function string GetString( optional int Switch, optional bool bPRI1HUD, optional PlayerReplicationInfo RelatedPRI_1,
					optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	return default.Announcements[Switch].AnnouncementText;
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return default.Announcements[MessageIndex].AnnouncementSound;
}

static function bool ShouldBeRemoved(int MyMessageIndex, class<UTLocalMessage> NewAnnouncementClass, int NewMessageIndex)
{
	return (NewAnnouncementClass == default.Class);
}

static function int GetFontSize( int Switch, PlayerReplicationInfo RelatedPRI1, PlayerReplicationInfo RelatedPRI2, PlayerReplicationInfo LocalPlayer )
{
	if ( Switch > 10 )
	{
		return default.FontSize;
	}
	return 2;
}

defaultproperties
{
	FontSize=1
	bIsConsoleMessage=false
	bIsUnique=true
	bBeep=false
	DrawColor=(R=32,G=64,B=255)

	Announcements[1]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count01')
	Announcements[2]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count02')
	Announcements[3]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count03')
	Announcements[4]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count04')
	Announcements[5]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count05')
	Announcements[6]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count06')
	Announcements[7]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count07')
	Announcements[8]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count08')
	Announcements[9]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count09')
	Announcements[10]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count10')
	Announcements[12]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_30SecondsRemain')
	Announcements[13]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_1MinuteRemains')
	Announcements[14]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_2MinutesRemain')
	Announcements[15]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_3MinutesRemain')
	Announcements[16]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_5MinutesRemain')
}
