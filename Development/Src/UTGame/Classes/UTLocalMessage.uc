/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTLocalMessage extends LocalMessage
	abstract;

/** Message area on HUD (index into UTHUD.MessageOffset[]) */
var int MessageArea;

static function byte AnnouncementLevel(byte MessageIndex)
{
	return 1;
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC);

/**
 * Allow messages to remove themselves if they are superfluous because of newly added message
 */
static function bool ShouldBeRemoved(int MyMessageIndex, class<UTLocalMessage> NewAnnouncementClass, int NewMessageIndex)
{
	return false;
}

static function AddAnnouncement(UTAnnouncer Announcer, byte MessageIndex, optional Object OptionalObject)
{
	local UTQueuedAnnouncement NewAnnouncement, A, RemovedAnnouncement;

	NewAnnouncement = Announcer.Spawn(class'UTQueuedAnnouncement');
	NewAnnouncement.AnnouncementClass = Default.Class;
	NewAnnouncement.MessageIndex = MessageIndex;
	NewAnnouncement.OptionalObject = OptionalObject;

	if (Announcer.Queue != None && Announcer.Queue.AnnouncementClass.static.ShouldBeRemoved(Announcer.Queue.MessageIndex, Default.Class, MessageIndex))
	{
		RemovedAnnouncement = Announcer.Queue;
		Announcer.Queue = Announcer.Queue.nextAnnouncement;
		RemovedAnnouncement.Destroy();
	}

	// default implementation is just add to end of queue
	if ( Announcer.Queue == None )
	{
		NewAnnouncement.nextAnnouncement = Announcer.Queue;
		Announcer.Queue = NewAnnouncement;
	}
	else
	{
		for ( A=Announcer.Queue; A!=None; A=A.nextAnnouncement )
		{
			if ( A.nextAnnouncement == None )
			{
				A.nextAnnouncement = NewAnnouncement;
				break;
			}
			else  if ( A.nextAnnouncement.AnnouncementClass.static.ShouldBeRemoved(A.nextAnnouncement.MessageIndex, Default.Class, MessageIndex) )
			{
				RemovedAnnouncement = A.nextAnnouncement;
				A.nextAnnouncement = A.nextAnnouncement.nextAnnouncement;
				RemovedAnnouncement.Destroy();
			}
		}
	}
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer, class<GameInfo> GameClass);

static function float GetPos( int Switch, HUD myHUD )
{
	return UTHUD(myHUD).MessageOffset[Default.MessageArea];
}

defaultproperties
{
	MessageArea=1
}