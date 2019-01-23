/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCarriedObjectMessage extends UTLocalMessage
	abstract;


var(Message) localized string ReturnBlue, ReturnRed;
var(Message) localized string ReturnedBlue, ReturnedRed;
var(Message) localized string CaptureBlue, CaptureRed;
var(Message) localized string DroppedBlue, DroppedRed;
var(Message) localized string HasBlue, HasRed;

var SoundNodeWave ReturnSounds[2];
var SoundNodeWave DroppedSounds[2];
var SoundNodeWave TakenSounds[2];





static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	local int i;

	for (i = 0; i < 2; i++)
	{
		Announcer.PrecacheSound(default.ReturnSounds[i]);
		Announcer.PrecacheSound(default.DroppedSounds[i]);
		Announcer.PrecacheSound(default.TakenSounds[i]);
	}
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
	if ( RelatedPRI_1 == P.PlayerReplicationInfo )
	{
		if ( (Switch == 1) || (Switch == 8) )
		{
			UTPlayerController(P).ClientMusicEvent(3);
		}
		else if ( (Switch == 4) || (Switch == 6) || (Switch == 11) || (Switch == 13) )
		{
			UTPlayerController(P).ClientMusicEvent(7);
		}
	}
	else if ( (Switch == 4) || (Switch == 11) )
	{
		if ( (RelatedPRI_1 != None) && !P.WorldInfo.GRI.OnSameTeam(P,RelatedPRI_1) )
		{
			UTPlayerController(P).ClientMusicEvent(4);
		}
	}
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	switch (MessageIndex)
	{
		// red team
		// Returned the flag.
	case 0:
	case 1:
	case 3: // because it fell out of the world
	case 5:
		return default.ReturnSounds[0];
		break;

		// Dropped the flag.
	case 2:
		return default.DroppedSounds[0];
		break;

		// taken the flag
	case 4: // taken from dropped position
	case 6: // taken from base
		return default.TakenSounds[0];
		break;

		// blue team
		// Returned the flag.
	case 7:
	case 8:
	case 10: // because it fell out of the world
	case 12:
		return default.ReturnSounds[1];
		break;

		// Dropped the flag.
	case 9:
		return default.DroppedSounds[1];
		break;

		// taken the flag
	case 11: // taken from dropped position
	case 13: // taken from base
		return default.TakenSounds[1];
		break;

	}
}

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
}

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
		// RED TEAM
		// Captured the flag.
	case 0:
		if (RelatedPRI_1 == None)
			return "";

		return RelatedPRI_1.PlayerName@Default.CaptureRed;
		break;

		// Returned the flag.
	case 1:
		if (RelatedPRI_1 == None)
		{
			return Default.ReturnedRed;
		}
		return RelatedPRI_1.PlayerName@Default.ReturnRed;
		break;

		// Dropped the flag.
	case 2:
		if (RelatedPRI_1 == None)
			return "";

		return RelatedPRI_1.playername@Default.DroppedRed;
		break;

		// Was returned.
	case 3:
		return Default.ReturnedRed;
		break;

		// Has the flag.
	case 4:
		if (RelatedPRI_1 == None)
			return "";
		return RelatedPRI_1.playername@Default.HasRed;
		break;

		// Auto send home.
	case 5:
		return Default.ReturnedRed;
		break;

		// Pickup
	case 6:
		if (RelatedPRI_1 == None)
			return "";
		return RelatedPRI_1.playername@Default.HasRed;
		break;

		// BLUE TEAM
		// Captured the flag.
	case 7:
		if (RelatedPRI_1 == None)
			return "";

		return RelatedPRI_1.PlayerName@Default.CaptureBlue;
		break;

		// Returned the flag.
	case 8:
		if (RelatedPRI_1 == None)
		{
			return Default.ReturnedBlue;
		}
		return RelatedPRI_1.playername@Default.ReturnBlue;
		break;

		// Dropped the flag.
	case 9:
		if (RelatedPRI_1 == None)
			return "";

		return RelatedPRI_1.playername@Default.DroppedBlue;
		break;

		// Was returned.
	case 10:
		return Default.ReturnedBlue;
		break;

		// Has the flag.
	case 11:
		if (RelatedPRI_1 == None)
			return "";
		return RelatedPRI_1.playername@Default.HasBlue;
		break;

		// Auto send home.
	case 12:
		return Default.ReturnedBlue;
		break;

		// Pickup
	case 13:
		if (RelatedPRI_1 == None)
			return "";
		return RelatedPRI_1.playername@Default.HasBlue;
		break;
	}
	return "";
}

/**
* Don't let multiple messages for same flag stack up
*/
static function bool ShouldBeRemoved(int MyMessageIndex, class<UTLocalMessage> NewAnnouncementClass, int NewMessageIndex)
{
	// check if message is not a flag status announcement
	if (default.Class != NewAnnouncementClass)
	{
		return false;
	}

	// check if messages are for same flag
	if ( ((MyMessageIndex < 7) != (NewMessageIndex < 7)) || (MyMessageIndex == 0) )
	{
		return false;
	}

	if ( MyMessageIndex > 6 )
		MyMessageIndex -= 7;
	if ( NewMessageIndex > 6 )
		NewMessageIndex -= 7;

	if ( (NewMessageIndex == 1) || (NewMessageIndex == 3) || (NewMessageIndex == 5) || (NewMessageIndex == 0) )
		return true;

	return ( (MyMessageIndex == 2) || (MyMessageIndex == 4) || (MyMessageIndex == 6) );
}


defaultproperties
{
	bIsUnique=True
	FontSize=1
	MessageArea=1
	bBeep=false
	DrawColor=(R=0,G=160,B=255,A=255)
}