//=============================================================================
// TeamInfo.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class TeamInfo extends ReplicationInfo
	native
	nativereplication;

var databinding localized string TeamName;
var databinding int Size; //number of players on this team in the level
var databinding float Score;
var databinding int TeamIndex;
var databinding color TeamColor;

cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	// Variables the server should send to the client.
	if( bNetDirty && (Role==ROLE_Authority) )
		Score;
	if ( bNetInitial && (Role==ROLE_Authority) )
		TeamName, TeamIndex;
}

function bool AddToTeam( Controller Other )
{
	local Controller P;
	local bool bSuccess;

	// make sure loadout works for this team
	if ( Other == None )
	{
		`log("Added none to team!!!");
		return false;
	}

	Size++;
	Other.PlayerReplicationInfo.Team = self;
	Other.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;

	bSuccess = false;
	if ( Other.IsA('PlayerController') )
		Other.PlayerReplicationInfo.TeamID = 0;
	else
		Other.PlayerReplicationInfo.TeamID = 1;

	while ( !bSuccess )
	{
		bSuccess = true;
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( P.bIsPlayer && (P != Other)
				&& (P.PlayerReplicationInfo.Team == Other.PlayerReplicationInfo.Team)
				&& (P.PlayerReplicationInfo.TeamId == Other.PlayerReplicationInfo.TeamId) )
			{
				bSuccess = false;
				break;
			}
		}
		if ( !bSuccess )
		{
			Other.PlayerReplicationInfo.TeamID = Other.PlayerReplicationInfo.TeamID + 1;
		}
	}
	return true;
}

function RemoveFromTeam(Controller Other)
{
	Size--;
}

simulated function string GetHumanReadableName()
{
	return TeamName;
}

/* GetHUDColor()
returns HUD color associated with this team
*/
simulated function color GetHUDColor()
{
	return TeamColor;
}

/* GetTextColor()
returns text color associated with this team
*/
function color GetTextColor()
{
	return TeamColor;
}

simulated event byte GetTeamNum()
{
	return BYTE(TeamIndex);
}

defaultproperties
{
	TickGroup=TG_DuringAsyncWork

	NetUpdateFrequency=2
	TeamColor=(r=255,g=64,b=64,a=255)
}
