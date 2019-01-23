/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//
// Switch is the note.
// RelatedPRI_1 is the player on the spree.
//
class UTKillingSpreeMessage extends UTLocalMessage;

var	localized string EndSpreeNote, EndSelfSpree, EndFemaleSpree, MultiKillString;
var	localized string SpreeNote[10];
var	localized string SelfSpreeNote[10];
var SoundNodeWave SpreeSound[10];
var	localized string EndSpreeNoteTrailer;

static function int GetFontSize( int Switch, PlayerReplicationInfo RelatedPRI1, PlayerReplicationInfo RelatedPRI2, PlayerReplicationInfo LocalPlayer )
{
	local Pawn ViewPawn;
	local int Size;

	if ( RelatedPRI2 == None )
	{

		Size = Default.FontSize;

		// If it's GODLIKE, use a bigger font

		if ( Switch == 4 )
		{
			Size = 2;
		}

		// If this is regarding the local player, then increase the size to make it more visible
		if ( LocalPlayer == RelatedPRI1 )
		{
			Size += 2;
		}
		else if ( LocalPlayer.bOnlySpectator )
		{
			ViewPawn = Pawn(PlayerController(LocalPlayer.Owner).ViewTarget);
			if ( (ViewPawn != None) && (ViewPawn.PlayerReplicationInfo == RelatedPRI1) )
			{
				Size++;
			}
		}
		return size;
	}
	return Default.FontSize;
}

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (RelatedPRI_2 == None)
	{
		if ( bPRI1HUD )
			return Default.SelfSpreeNote[Switch];
		if (RelatedPRI_1 == None)
			return "";

		if (RelatedPRI_1.PlayerName != "")
			return RelatedPRI_1.PlayerName@Default.SpreeNote[Switch];
	}
	else
	{
		if (RelatedPRI_1 == None)
		{
			if (RelatedPRI_2.PlayerName != "")
			{
				if ( RelatedPRI_2.bIsFemale )
					return RelatedPRI_2.PlayerName@Default.EndFemaleSpree;
				else
					return RelatedPRI_2.PlayerName@Default.EndSelfSpree;
			}
		}
		else
		{
			return RelatedPRI_1.PlayerName$Default.EndSpreeNote@RelatedPRI_2.PlayerName@Default.EndSpreeNoteTrailer;
		}
	}
	return "";
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

	if (RelatedPRI_2 != None)
		return;

	if ( (RelatedPRI_1 == P.PlayerReplicationInfo)
		|| (P.PlayerReplicationInfo.bOnlySpectator && (Pawn(P.ViewTarget) != None) && (Pawn(P.ViewTarget).PlayerReplicationInfo == RelatedPRI_1)) )
	{
		UTPlayerController(P).PlayAnnouncement(default.class,Switch );
		if ( Switch == 0 )
			UTPlayerController(P).ClientMusicEvent(8);
		else
			UTPlayerController(P).ClientMusicEvent(10);
	}
	else
		P.PlayBeepSound();
}

static function SoundNodeWave AnnouncementSound(byte MessageIndex, Object OptionalObject, PlayerController PC)
{
	return Default.SpreeSound[MessageIndex];
}

defaultproperties
{
	 FontSize=1
	 bBeep=False
	 SpreeSound(0)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_KillingSpree'
	 SpreeSound(1)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Rampage'
	 SpreeSound(2)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Dominating'
	 SpreeSound(3)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Unstoppable'
	 SpreeSound(4)=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_GodLike'
	MessageArea=3
}
