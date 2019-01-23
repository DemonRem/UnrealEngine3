/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtOrbMessage extends UTCTFMessage;

var SoundNodeWave EnemyIncoming;
var SoundNodeWave EnemyDropped;
var SoundNodeWave EnemyDestroyed;

var(Message) localized string EnemyIncomingString, EnemyDroppedString, EnemyDestroyedString;

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass)
{
	super.PrecacheGameAnnouncements(Announcer, GameClass);

	Announcer.PrecacheSound(Default.EnemyIncoming);
	Announcer.PrecacheSound(Default.EnemyDropped);
	Announcer.PrecacheSound(Default.EnemyDestroyed);
}


static simulated function ClientReceive(
	PlayerController P,
	optional int SwitchIndex,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	local UTTeamInfo OrbTeam;
	local UTOnslaughtFlag Orb;
	local int BaseSwitch;

	// if enemy orb, either switch to custom message or ignore, depending on whether in sensor range
	OrbTeam = UTTeamInfo(OptionalObject);
	if ( (OrbTeam != None) && (P.PlayerReplicationInfo.Team != OrbTeam) )
	{
		Orb = UTOnslaughtFlag(OrbTeam.TeamFlag);
		if ( Orb != None )
		{
			if ( (Orb.LastNearbyObjective == None)
				|| (VSize(Orb.Location - Orb.LastNearbyObjective.Location) > Orb.LastNearbyObjective.MaxSensorRange) )
			{
				return;
			}

			BaseSwitch = (SwitchIndex<7) ? SwitchIndex : SwitchIndex-7;
			// switch to custom message
			if ( BaseSwitch == 2 )
			{
				// enemy dropped
				SwitchIndex = 15;
			}
			else if ( (BaseSwitch == 1) || (BaseSwitch == 3) || (BaseSwitch == 5) )
			{
				// enemy destroyed (returned)
				SwitchIndex = 16;
			}
		}
	}
	Super.ClientReceive(P, SwitchIndex, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	if ( RelatedPRI_1 == P.PlayerReplicationInfo )
	{
		if ( (SwitchIndex == 16) )
		{
			UTPlayerController(P).ClientMusicEvent(3);
		}
	}
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, optional Object OptionalObject)
{
	if ( MessageIndex == 14 )
	{
		return Default.EnemyIncoming;
	}
	else if ( MessageIndex == 15 )
	{
		return Default.EnemyDropped;
	}
	else if ( MessageIndex == 16 )
	{
		return Default.EnemyDestroyed;
	}
	else
	{
		return super.AnnouncementSound(MessageIndex, OptionalObject);
	}
}

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if ( Switch == 14 )
	{
		return Default.EnemyIncomingString;
	}
	else if ( Switch == 15 )
	{
		return Default.EnemyDroppedString;
	}
	else if ( Switch == 16 )
	{
		return Default.EnemyDestroyedString;
	}
	else
	{
		return super.GetString(Switch, bPRI1HUD, RelatedPRI_1, RelatedPRI_2, OptionalObject);
	}
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
	
	if ( (MyMessageIndex>13) == (NewMessageIndex>13) )
	{
		return true;
	}
	return Super.ShouldBeRemoved(MyMessageIndex, NewAnnouncementClass, NewMessageIndex);
}

defaultproperties
{
	ReturnSounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedOrbDestroyed'
	ReturnSounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueOrbDestroyed'
	DroppedSounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedOrbDropped'
	DroppedSounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueOrbDropped'
	TakenSounds(0)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedOrbPickedUp'
	TakenSounds(1)=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueOrbPickedUp'

	EnemyIncoming=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_EnemyOrbCarrierIncoming'
	EnemyDropped=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_EnemyOrbDropped'
	EnemyDestroyed=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_EnemyOrbDestroyed'
}

