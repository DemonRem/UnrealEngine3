/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
/** this is used to handle auto objective announcements (what the game thinks the player should do next) */
class UTObjectiveAnnouncement extends UTObjectiveSpecificMessage
	dependson(UTPlayerController);

static function ObjectiveAnnouncementInfo GetObjectiveAnnouncement(byte MessageIndex, Object Objective, PlayerController PC)
{
	local UTGameObjective GameObj;
	local UTCarriedObject Flag;
	local ObjectiveAnnouncementInfo EmptyAnnouncement;

	GameObj = UTGameObjective(Objective);
	if (GameObj != None)
	{
		return GameObj.WorldInfo.GRI.OnSameTeam(GameObj, PC) ? GameObj.DefendAnnouncement : GameObj.AttackAnnouncement;
	}
	else
	{
		Flag = UTCarriedObject(Objective);
		if (Flag != None && Flag.Team != None && Flag.Team.TeamIndex < Flag.NeedToPickUpAnnouncements.length)
		{
			return Flag.NeedToPickUpAnnouncements[Flag.Team.TeamIndex];
		}
	}

	return EmptyAnnouncement;
}

static function bool ShouldBeRemoved(int MyMessageIndex, class<UTLocalMessage> NewAnnouncementClass, int NewMessageIndex)
{
	// don't ever allow two objective messages to be in the queue simultaneously
	return (default.Class == NewAnnouncementClass);
}

static simulated function SetDisplayedOrders(string OrderText, HUD aHUD)
{
	if ( UTHUD(aHUD) != None )
	{
		UTHUD(aHUD).SetDisplayedOrders(OrderText);
	}
}

defaultproperties
{
	bIsUnique=True
	FontSize=1
	MessageArea=3
	bBeep=false
	DrawColor=(R=255,G=255,B=255,A=255)
}
