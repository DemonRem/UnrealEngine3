/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAnnouncer extends Info
	config(Game);

/** information about an announcer package */
struct AnnouncerPackageInfo
{
	/** name of the package */
	var string PackageName;
	/** prefix for the names of all sounds in this package */
	var string SoundPrefix;
};
/** announcements that exist in this custom package will override the default hardcoded sounds */
var globalconfig AnnouncerPackageInfo CustomAnnouncerPackage;

var globalconfig byte AnnouncerLevel;				// 0=none, 1=no possession announcements, 2=all
// FIXMESTEVE var globalconfig	byte			AnnouncerVolume;			// 1 to 4

/** sounds loaded from custom package and the old sound they are replacing */
struct CachedSound
{
	var SoundNodeWave OldSound, NewSound;
};
var array<CachedSound> CachedCustomSounds;

/** sounds that were not replaced, so we can avoid wasting time trying to DLO them from the custom package */
var array<SoundNodeWave> CachedDefaultSounds;

var bool bPrecachedBaseSounds;
var bool bPrecachedGameSounds;

var byte PlayingAnnouncementIndex;
var class<UTLocalMessage> PlayingAnnouncementClass; // Announcer Sound

var UTQueuedAnnouncement Queue;

/** Time between playing 2 announcer sounds */
var float GapTime;

var UTPlayerController PlayerOwner;

/** the sound cue used for announcer sounds. We then use a wave parameter named Announcement to insert the actual sound we want to play.
 * (this allows us to avoid having to change a whole lot of cues together if we want to change SoundCue options for the announcements)
 */
var SoundCue AnnouncerSoundCue;
/** allows overriding AnnouncerSoundCue */
var globalconfig string CustomAnnouncerSoundCue;

function Destroyed()
{
	local UTQueuedAnnouncement A;

	Super.Destroyed();

	for ( A=Queue; A!=None; A=A.nextAnnouncement )
		A.Destroy();
}

function PostBeginPlay()
{
	Super.PostBeginPlay();

	PlayerOwner = UTPlayerController(Owner);

	if (CustomAnnouncerSoundCue != "")
	{
		AnnouncerSoundCue = SoundCue(DynamicLoadObject(CustomAnnouncerSoundCue, class'SoundCue'));
	}
}

function AnnouncementFinished(AudioComponent AC)
{
	if (GapTime > 0.0)
	{
		SetTimer(GapTime, false, 'PlayNextAnnouncement');
	}
	else
	{
		PlayNextAnnouncement();
	}
}

function PlayNextAnnouncement()
{
	local UTQueuedAnnouncement PlayedAnnouncement;

	PlayingAnnouncementClass = None;

	if ( Queue != None )
	{
		PlayedAnnouncement = Queue;
		Queue = PlayedAnnouncement.nextAnnouncement;
		PlayAnnouncementNow(PlayedAnnouncement.AnnouncementClass, PlayedAnnouncement.MessageIndex, PlayedAnnouncement.OptionalObject);
		PlayedAnnouncement.Destroy();
	}
}

function PlayAnnouncementNow(class<UTLocalMessage> InMessageClass, byte MessageIndex, optional Object OptionalObject)
{
	local SoundNodeWave ASound;
	local AudioComponent AC;

	ASound = GetSound(InMessageClass.Static.AnnouncementSound(MessageIndex, OptionalObject, PlayerOwner));

	if ( ASound != None )
	{
		//@FIXME: part of playsound pool?
		AC = PlayerOwner.CreateAudioComponent(AnnouncerSoundCue, false, false);

		// AC will be none if -nosound option used
		if ( AC != None )
		{
			AC.SetWaveParameter('Announcement', ASound);
			AC.bAutoDestroy = true;
			AC.bShouldRemainActiveIfDropped = true;
			AC.OnAudioFinished = AnnouncementFinished;
			AC.Play();
		}
		PlayingAnnouncementClass = InMessageClass;
		PlayingAnnouncementIndex = MessageIndex;
	}
	else 
	{
		`log("NO SOUND FOR "$InMessageClass@MessageIndex@OptionalObject);
		PlayNextAnnouncement();
	}
}

function PlayAnnouncement(class<UTLocalMessage> InMessageClass, byte MessageIndex, optional Object OptionalObject)
{
	if ( InMessageClass.Static.AnnouncementLevel(MessageIndex) > AnnouncerLevel )
	{
		/* FIXMESTEVE
		if ( AnnouncementLevel == 2 )
			PlayerOwner.ClientPlaySound(Soundcue'GameSounds.DDAverted');
		else if ( AnnouncementLevel == 1 )
			PlayerOwner.PlayBeepSound();
		*/
		return;
	}

	if ( PlayingAnnouncementClass == None )
	{
		// play immediately
		PlayAnnouncementNow(InMessageClass, MessageIndex, OptionalObject);
		return;
	}
	InMessageClass.static.AddAnnouncement(self, MessageIndex, OptionalObject);
}

/** constructs the fully qualified name of the sound given a package info and sound name */
function string GetFullSoundName(AnnouncerPackageInfo Package, string SoundName)
{
	return (Package.PackageName $ "." $ Package.SoundPrefix $ SoundName);
}

/** get the sound to use given the default sound (checks custom package) */
function SoundNodeWave GetSound(SoundNodeWave DefaultSound)
{
	local SoundNodeWave NewSound;
	local string SoundName;
	local string FullName;
	local int Pos;
	local CachedSound NewCacheEntry;

	// if we have no custom announcer package or we know this sound doesn't have a replacement in it, just return the requested sound
	if (DefaultSound == None || CustomAnnouncerPackage.PackageName == "" || CachedDefaultSounds.Find(DefaultSound) != INDEX_NONE)
	{
		return DefaultSound;
	}
	else
	{
		// try to find already cached entry
		Pos = CachedCustomSounds.Find('OldSound', DefaultSound);
		if (Pos != INDEX_NONE)
		{
			return CachedCustomSounds[Pos].NewSound;
		}
		else
		{
			// try searching for entire sound name
			SoundName = string(DefaultSound.Name);
			FullName = GetFullSoundName(CustomAnnouncerPackage, SoundName);
			NewSound = SoundNodeWave(DynamicLoadObject(FullName, class'SoundNodeWave', true));
			if (NewSound == None)
			{
				// try searching for only what's after the last underscore (cut out our crazy naming convention)
				Pos = InStr(SoundName, "_", true);
				if (Pos != INDEX_NONE)
				{
					FullName = GetFullSoundName(CustomAnnouncerPackage, Right(SoundName, Len(SoundName) - Pos - 1));
					NewSound = SoundNodeWave(DynamicLoadObject(FullName, class'SoundNodeWave', true));
				}
			}

			if (NewSound != None)
			{
				NewCacheEntry.OldSound = DefaultSound;
				NewCacheEntry.NewSound = NewSound;
				CachedCustomSounds[CachedCustomSounds.length] = NewCacheEntry;
				return NewSound;
			}
			else
			{
				// couldn't find it
				CachedDefaultSounds[CachedDefaultSounds.length] = DefaultSound;
				return DefaultSound;
			}
		}
	}
}

/** used to make sure all custom replacements are loaded at startup time */
function PrecacheSound(SoundNodeWave DefaultSound)
{
	GetSound(DefaultSound);
}

function PrecacheAnnouncements()
{
	local class<UTGame> GameClass;
	local UTGameObjective O;

	if ( !bPrecachedGameSounds && (WorldInfo.GRI != None) && (WorldInfo.GRI.GameClass != None) )
	{
		bPrecachedGameSounds = true;
		GameClass = class<UTGame>(WorldInfo.GRI.GameClass);
		if ( GameClass != None )
			GameClass.Static.PrecacheGameAnnouncements(self);
	}

	foreach DynamicActors(class'UTGameObjective', O)
	{
		O.PrecacheAnnouncer(self);
	}

	if ( !bPrecachedBaseSounds )
	{
		bPrecachedBaseSounds = true;

		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Headshot');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Headhunter');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_FlakMonkey');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Combowhore');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_FirstBlood');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_DoubleKill');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MultiKill');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MegaKill');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_UltraKill');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_MonsterKill');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_KillingSpree');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Rampage');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Dominating');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Unstoppable');
		PrecacheSound(SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_GodLike');

		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count01');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count02');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count03');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count04');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count05');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count06');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count07');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count08');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count09');
		PrecacheSound(SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Count10');
	}
}

defaultproperties
{
	AnnouncerSoundCue=SoundCue'A_Announcer_FemaleUT.Announcements.A_Gameplay_AnnouncerCue'
}
