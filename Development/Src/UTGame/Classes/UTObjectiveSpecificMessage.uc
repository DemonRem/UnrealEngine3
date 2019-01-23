/** base class for messages that get their text/sound from the objective actor passed to them */
class UTObjectiveSpecificMessage extends UTLocalMessage
	abstract;

/** should be implemented to return the announcement to use based on the given objective and index */
static function ObjectiveAnnouncementInfo GetObjectiveAnnouncement(byte MessageIndex, Object Objective, PlayerController PC);

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	local ObjectiveAnnouncementInfo Announcement;

	Announcement = GetObjectiveAnnouncement(MessageIndex, OptionalObject, PC);
	return Announcement.AnnouncementSound;
}

static simulated function ClientReceive( PlayerController P, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1,
						optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	local UTPlayerController UTP;
	local ObjectiveAnnouncementInfo Announcement;
	local UTWarfareBarricade B;

	UTP = UTPlayerController(P);
	if (UTP != None)
	{
		Announcement = GetObjectiveAnnouncement(Switch, OptionalObject, P);

		if (Announcement.AnnouncementSound != None)
		{
			UTP.PlayAnnouncement(default.Class, Switch, OptionalObject);
		}

		if (Announcement.AnnouncementText != "")
		{
			if (P.myHud != None)
			{
				P.myHUD.LocalizedMessage( default.Class, RelatedPRI_1, Announcement.AnnouncementText,
								Switch, static.GetPos(Switch, P.MyHUD), static.GetLifeTime(Switch),
								static.GetFontSize(Switch, RelatedPRI_1, RelatedPRI_2, P.PlayerReplicationInfo),
								static.GetColor(Switch, RelatedPRI_1, RelatedPRI_2), OptionalObject );
				SetDisplayedOrders(Announcement.AnnouncementText, P.MyHUD);
			}
			if (IsConsoleMessage(Switch) && LocalPlayer(P.Player).ViewportClient != None)
			{
				LocalPlayer(P.Player).ViewportClient.ViewportConsole.OutputText(Announcement.AnnouncementText);
			}
		}
		if ( Switch == 1 )
		{
			// check if barricade disabled
			B = UTWarfareBarricade(OptionalObject);
			if ( B != None )
			{
				B.DestroyedTime = B.WorldInfo.TimeSeconds;
			}
		}
	}
	
}

static simulated function SetDisplayedOrders(string OrderText, HUD aHUD);

defaultproperties
{
	MessageArea=6
}