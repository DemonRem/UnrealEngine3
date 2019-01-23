/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqEvent_TurretSpawn extends SequenceEvent
	native(Sequence)
	hidecategories(SequenceEvent);

var name TurretGroupName;
cpptext
{
	virtual void OnCreated();
	virtual UBOOL RegisterEvent();
}


/** notification that the mover has completed all opening actions and is now ready to close */
simulated event TurretSpawned()
{
	local array<int> ActivateIndices;

	ActivateIndices[0] = 0;
	CheckActivate(Originator, Instigator, false, ActivateIndices);
}

defaultproperties
{
	ObjName="Turret Spawned"
	ObjCategory="Turrets"
	bPlayerOnly=false
	OutputLinks(0)=(LinkDesc="Spawned")
	TurretGroupName=TurretGroup
}
