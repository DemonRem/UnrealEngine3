/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTStartupMessage extends UTLocalMessage;

var localized string Stage[8], NotReady, SinglePlayer;
var SoundCue	Riff;

static simulated function ClientReceive(
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	local UTPlayerController UTP;

	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	UTP = UTPlayerController(P);
	if ( UTP == None )
		return;

	// don't play sound if quickstart=true, so no 'play' voiceover at start of tutorials
	if ( Switch == 5 )
	{
		if ( (P.WorldInfo != none) && ((UTGame(P.WorldInfo.Game) == None) || !UTGame(P.WorldInfo.Game).SkipPlaySound()) )
			UTP.PlayAnnouncement(Default.Class, 1);
	}
	else if ( (Switch > 1) && (Switch < 5) )
		UTP.PlayBeepSound();
	else if ( Switch == 7 && Default.Riff != none)
		UTP.ClientPlaySound(Default.Riff);
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_Play';
}

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	local int i, PlayerCount;
	local GameReplicationInfo GRI;

	if ( (RelatedPRI_1 != None) && (RelatedPRI_1.WorldInfo.NetMode == NM_Standalone) )
	{
		if ( (UTGame(RelatedPRI_1.WorldInfo.Game) != None) && UTGame(RelatedPRI_1.WorldInfo.Game).bQuickstart )
			return "";
		if ( Switch < 2 )
			return Default.SinglePlayer;
	}
	else if ( Switch == 0 && RelatedPRI_1 != None )
	{
		GRI = RelatedPRI_1.WorldInfo.GRI;
		if (GRI == None)
			return Default.Stage[0];
		for (i = 0; i < GRI.PRIArray.Length; i++)
		{
			if ( GRI.PRIArray[i] != None && !GRI.PRIArray[i].bOnlySpectator
			     && (!GRI.PRIArray[i].bIsSpectator || GRI.PRIArray[i].bWaitingPlayer) )
				PlayerCount++;
		}
		if (UTGameReplicationInfo(GRI).MinNetPlayers - PlayerCount > 0)
			return Default.Stage[0]@"("$(UTGameReplicationInfo(GRI).MinNetPlayers - PlayerCount)$")";
	}
	else if ( switch == 1 )
	{
		if ( (RelatedPRI_1 == None) || !RelatedPRI_1.bWaitingPlayer )
			return Default.Stage[0];
		else if ( RelatedPRI_1.bReadyToPlay )
			return Default.Stage[1];
		else
			return Default.NotReady;
	}
	return Default.Stage[Switch];
}

defaultproperties
{
	FontSize=1
	bIsConsoleMessage=false
	bIsUnique=true
	bBeep=False
	DrawColor=(R=32,G=64,B=255)

 //   Riff=soundcue'GameSounds.UT2K3Fanfare11'
}
