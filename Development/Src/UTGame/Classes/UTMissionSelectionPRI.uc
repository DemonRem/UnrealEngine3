/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMissionSelectionPRI extends UTPlayerReplicationInfo;


var bool bIsHost;


/**
 * Tell the game we have selected the mission
 */
reliable server function AcceptMission(int MissionIndex, string URL)
{
	if ( bIsHost )
	{
    	UTMissionSelectionGame(WorldInfo.Game).AcceptMission(MissionIndex, URL);
	}
}


/**
 * The Mission has changed, notify everyone
 */
reliable server function ChangeMission(int NewMissionIndex)
{
	local UTMissionGRI GRI;

	GRI = UTMissionGRI(Worldinfo.GRI);
	if (GRI != none && bIsHost)
	{
		GRI.ChangeMission(NewMissionIndex);
	}
}


/**
 * Let's the server know we are ready to play
 */

reliable server function ServerReadyToPlay()
{
	bReadyToPlay = true;
}

defaultproperties
{


}
