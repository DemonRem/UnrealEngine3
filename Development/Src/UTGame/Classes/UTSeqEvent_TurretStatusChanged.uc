/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqEvent_TurretStatusChanged extends SequenceEvent;

defaultproperties
{
	ObjName="Turret Status Changed"
	OutputLinks[0]=(LinkDesc="Created")
	OutputLinks[1]=(LinkDesc="Destroyed")
	OutputLinks[2]=(LinkDesc="Player Enter")
	OutputLinks[3]=(LinkDesc="Player Exit")
	bPlayerOnly=false
	MaxTriggerCount=0
}
