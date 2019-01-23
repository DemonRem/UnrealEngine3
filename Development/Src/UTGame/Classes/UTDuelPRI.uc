class UTDuelPRI extends UTPlayerReplicationInfo;

/** position in the queue. < 0 means this player is currently in combat */
var int QueuePosition;
/** number of consecutive games won */
var int ConsecutiveWins;

replication
{
	if (bNetDirty)
		QueuePosition, ConsecutiveWins;
}

defaultproperties
{
	QueuePosition=-1
}
