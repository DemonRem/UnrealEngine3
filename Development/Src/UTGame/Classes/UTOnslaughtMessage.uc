/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtMessage extends UTLocalMessage;

var(Message) localized string RedTeamDominatesString;
var(Message) localized string BlueTeamDominatesString;
var(Message) localized string RedTeamPowerCoreString;
var(Message) localized string BlueTeamPowerCoreString;
var(Message) localized string VehicleLockedString;
var(Message) localized string InvincibleCoreString;
var(Message) localized string UnattainableNodeString;
var(Message) localized string RedPowerCoreAttackedString;
var(Message) localized string BluePowerCoreAttackedString;
var(Message) localized string RedPowerNodeAttackedString;
var(Message) localized string BluePowerNodeAttackedString;
var(Message) localized string InWayOfVehicleSpawnString;
var(Message) localized string UnpoweredString;
var(Message) localized string RedPowerCoreDestroyedString;
var(Message) localized string BluePowerCoreDestroyedString;
var(Message) localized string RedPowerNodeDestroyedString;
var(Message) localized string BluePowerNodeDestroyedString;
var(Message) localized string RedPowerCoreCriticalString;
var(Message) localized string BluePowerCoreCriticalString;
var(Message) localized string PressUseToTeleportString;
var(Message) localized string RedPowerCoreVulnerableString;
var(Message) localized string BluePowerCoreVulnerableString;
var(Message) localized string RedPowerNodeUnderConstructionString;
var(Message) localized string BluePowerNodeUnderConstructionString;
var(Message) localized string RedPowerCoreDamagedString;
var(Message) localized string BluePowerCoreDamagedString;
var(Message) localized string RedPowerNodeSeveredString;
var(Message) localized string BluePowerNodeSeveredString;
var(Message) localized string PowerCoresAreDrainingString;
var(Message) localized string UnhealablePowerCoreString;
var(Message) localized string CameraDeploy;
var(Message) localized string PowerNodeShieldedByOrbString;
var(Message) localized string PowerNodeTemporarilyShieldedString;
var(Message) localized string OrbReturnsOrbString;

var SoundNodeWave MessageAnnouncements[45];
var float MessagePosY[45];
var SoundCue VictorySound;

var color RedColor;
var color YellowColor;

static simulated function ClientReceive(
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	Super.ClientReceive(P, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);

	if (UTOnslaughtObjective(OptionalObject) != None)
	{
		if (P.WorldInfo.TimeSeconds < UTOnslaughtObjective(OptionalObject).LastAttackAnnouncementTime + 10)
			return;
		else
			UTOnslaughtObjective(OptionalObject).LastAttackAnnouncementTime = P.WorldInfo.TimeSeconds;
	}
	if (default.MessageAnnouncements[Switch] != None)
	{
		UTPlayerController(P).PlayAnnouncement(default.class, Switch);
	}

	if (P.PlayerReplicationInfo != None && P.PlayerReplicationInfo.Team != None && P.PlayerReplicationInfo.Team.TeamIndex == Switch)
		P.ClientPlaySound(default.VictorySound);
}


static function byte AnnouncementLevel(byte MessageIndex)
{
	return 2;
}


static function SoundNodeWave AnnouncementSound(byte MessageIndex, optional Object OptionalObject)
{
	return default.MessageAnnouncements[MessageIndex];
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
		case 0:
			return Default.RedTeamDominatesString;
			break;

		case 1:
			return Default.BlueTeamDominatesString;
			break;

		case 2:
			return Default.RedTeamPowerCoreString;
			break;

		case 3:
			return Default.BlueTeamPowerCoreString;
			break;

		case 4:
	    return Default.VehicleLockedString;
	    break;

		case 5:
	    return Default.InvincibleCoreString;
	    break;

		case 6:
	    return Default.UnattainableNodeString;
	    break;

		case 7:
	    return Default.RedPowerCoreAttackedString;
	    break;

		case 8:
	    return Default.BluePowerCoreAttackedString;
	    break;

		case 9:
	    return Default.RedPowerNodeAttackedString;
	    break;

		case 10:
	    return Default.BluePowerNodeAttackedString;
	    break;

		case 11:
	    return Default.InWayOfVehicleSpawnString;
	    break;

		case 13:
			return Default.UnpoweredString;
			break;

		case 14:
			return Default.RedPowerCoreDestroyedString;
			break;

		case 15:
			return Default.BluePowerCoreDestroyedString;
			break;

		case 16:
			return Default.RedPowerNodeDestroyedString;
			break;

		case 17:
			return Default.BluePowerNodeDestroyedString;
			break;
		case 18:
			return Default.RedPowerCoreCriticalString;
			break;
		case 19:
			return Default.BluePowerCoreCriticalString;
			break;
		case 20:
			return Default.RedPowerCoreVulnerableString;
			break;
		case 21:
			return Default.BluePowerCoreVulnerableString;
			break;
		case 22:
			return Default.PressUseToTeleportString;
			break;
		case 23:
			return Default.RedPowerNodeUnderConstructionString;
			break;
		case 24:
			return Default.BluePowerNodeUnderConstructionString;
			break;
		case 25:
			return Default.RedPowerCoreDamagedString;
			break;
		case 26:
			return Default.BluePowerCoreDamagedString;
			break;
		case 27:
			return Default.RedPowerNodeSeveredString;
			break;
		case 28:
			return Default.BluePowerNodeSeveredString;
			break;
		case 29:
			return Default.PowerCoresAreDrainingString;
			break;
		case 30:
			return Default.UnhealablePowerCoreString;
			break;
		case 34:
			return default.CameraDeploy;
			break;
		case 42:
			return default.PowerNodeShieldedByOrbString;
			break;
		case 43:
			return default.PowerNodeTemporarilyShieldedString;
			break;
		case 44:
			return default.OrbReturnsOrbString;
			break;
	}
	return "";
}

static function color GetColor(
    optional int Switch,
    optional PlayerReplicationInfo RelatedPRI_1,
    optional PlayerReplicationInfo RelatedPRI_2
    )
{
	if ((Switch > 6 && Switch < 11) || (Switch > 17 && Switch < 22) || Switch > 24)
		return Default.RedColor;
	else if (Switch == 11)
		return Default.YellowColor;
	else
		return Default.DrawColor;
}

static function float GetPos( int Switch )
{
	return (default.MessagePosY[Switch] == 0.0) ? default.PosY : default.MessagePosY[Switch];
}

static function int GetFontSize( int Switch, PlayerReplicationInfo RelatedPRI1, PlayerReplicationInfo RelatedPRI2, PlayerReplicationInfo LocalPlayer )
{
 	return (default.MessagePosY[Switch] == 0.0) ? default.FontSize : default.FontSize+1;
}

static function float GetLifeTime(int Switch)
{
	if (Switch == 29)
		return 4.0;

	return default.LifeTime;
}

static function bool IsConsoleMessage(int Switch)
{
 	if (Switch < 5 || (Switch > 12 && Switch < 18) || (Switch > 19 && Switch < 22) || (Switch > 25 && Switch < 41))
 		return true;

 	return false;
}

DefaultProperties
{
	RedColor=(R=255,G=0,B=0,A=255)
	YellowColor=(R=255,G=255,B=0,A=255)
	Lifetime=2.5
	PosY=0.1
	MessageAnnouncements[7]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedCoreUnderAttack'
	MessageAnnouncements[8]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueCoreUnderAttack'
	MessageAnnouncements[14]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedCoreDestroyed'
	MessageAnnouncements[15]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueCoreDestroyed'
	MessageAnnouncements[16]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedNodeDestroyed'
	MessageAnnouncements[17]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueNodeDestroyed'
	MessageAnnouncements[18]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedCoreCritical'
	MessageAnnouncements[19]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueCoreCritical'
	MessageAnnouncements[20]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedCoreVulnerable'
	MessageAnnouncements[21]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueCoreVulnerable'
	MessageAnnouncements[23]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedNodeConstruction'
	MessageAnnouncements[24]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueNodeConstruction'
	MessageAnnouncements[25]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedCoreDamaged'
	MessageAnnouncements[26]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueCoreDamaged'
	MessageAnnouncements[27]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_RedNodeIsolated'
	MessageAnnouncements[28]=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_BlueNodeIsolated'
	MessageAnnouncements[35]=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Hijacked'
	MessageAnnouncements[36]=SoundNodeWave'A_Announcer_UTMale01.wav.A_Announcer_UTMale_Carjacked'
	//VictorySound=Soundcue'GameSounds.Fanfares.UT2K3Fanfare04'
	bIsUnique=false
	bIsPartiallyUnique=true
	bBeep=false
	DrawColor=(R=0,G=160,B=255,A=255)
	FontSize=1

	MessagePosY[4]=0.242
	MessagePosY[5]=0.242
	MessagePosY[6]=0.242
	MessagePosY[11]=0.242
	MessagePosY[13]=0.242
	MessagePosY[14]=0.242
	MessagePosY[15]=0.242
	MessagePosY[18]=0.242
	MessagePosY[19]=0.242
	MessagePosY[22]=0.242
	MessagePosY[29]=0.90
	MessagePosY[30]=0.242
	MessagePosY[34]=0.242
	MessagePosY[42]=0.242
	MessagePosY[43]=0.242
	MessagePosY[44]=0.242
}
