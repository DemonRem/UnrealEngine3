/** this class is used for status announcements related to UTOnslaughtSpecialObjectives */
class UTSpecialObjectiveStatusMessage extends UTObjectiveSpecificMessage
	dependson(UTPlayerController);

static function ObjectiveAnnouncementInfo GetObjectiveAnnouncement(byte MessageIndex, Object Objective, PlayerController PC)
{
	local UTOnslaughtSpecialObjective SpecialObjective;
	local ObjectiveAnnouncementInfo EmptyAnnouncement;

	SpecialObjective = UTOnslaughtSpecialObjective(Objective);
	if (SpecialObjective != None)
	{
		switch (MessageIndex)
		{
			case 0:
				return SpecialObjective.UnderAttackAnnouncement;
			case 1:
				return SpecialObjective.DisabledAnnouncement;
			default:
				`Warn("Invalid MessageIndex" @ MessageIndex);
				break;
		}
	}

	return EmptyAnnouncement;
}

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
}

defaultproperties
{
	DrawColor=(R=0,G=160,B=255,A=255)
	FontSize=2
	Lifetime=2.5
	bIsConsoleMessage=true
	MessageArea=6
}
