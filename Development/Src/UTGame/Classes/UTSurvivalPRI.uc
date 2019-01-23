/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSurvivalPRI extends UTPlayerReplicationInfo;

var		int ConsecutiveKills;	// How many kills in a row has this player made
var		int	QueuePosition;		// This player's current position in the queue
var		int CombatLevel;  		// This value describes how well the player is doing
								// 0 = Mano e' Mano
								// 1 = Challenger spawns with a weapon
								// 2 = 2x Challengers (

replication
{
	if ( Role==ROLE_Authority )
		QueuePosition, CombatLevel;
}

defaultproperties
{
}
