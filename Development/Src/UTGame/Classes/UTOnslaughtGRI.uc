/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtGRI extends UTGameReplicationInfo;

/** name of link setup currently in use */
var repnotify name LinkSetupName;

/** cache of team powercores - use GetTeamCore() to access */
var protected array<UTOnslaughtPowerCore> CachedTeamCores;

replication
{
	if (bNetDirty)
		LinkSetupName;
}

simulated event ReplicatedEvent(name VarName)
{
	local UTOnslaughtMapInfo OnslaughtInfo;

	if (VarName == 'LinkSetupName')
	{
		OnslaughtInfo = UTOnslaughtMapInfo(WorldInfo.GetMapInfo());
		if (OnslaughtInfo != None)
		{
			OnslaughtInfo.ApplyLinkSetup(LinkSetupName);
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function UTOnslaughtPowerCore GetTeamCore(byte TeamNum)
{
	local UTOnslaughtPowerCore Core;

	if (TeamNum < CachedTeamCores.length && CachedTeamCores[TeamNum] != None && CachedTeamCores[TeamNum].DefenderTeamIndex == TeamNum)
	{
		return CachedTeamCores[TeamNum];
	}
	else
	{
		foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtPowerCore', Core)
		{
			if (Core.DefenderTeamIndex == TeamNum)
			{
				CachedTeamCores[TeamNum] = Core;
				return Core;
			}
		}
	}
}

simulated function bool IsNecrisTeam(byte TeamNum)
{
	local UTOnslaughtPowerCore Core;

	Core = GetTeamCore(TeamNum);
	return (Core != None && Core.bNecrisCore);
}

defaultproperties
{
	MapMenuTemplate=UTUIScene_OnsMapMenu'UI_Scenes_HUD.Menus.OnsMapTeleportMenu'
}
