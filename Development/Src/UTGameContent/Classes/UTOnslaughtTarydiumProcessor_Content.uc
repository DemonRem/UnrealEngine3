/**
* Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
class UTOnslaughtTarydiumProcessor_Content extends UTOnslaughtTarydiumProcessor;


defaultproperties
{
	bDestinationOnly=true
	bMustTouchToReach=false
	bPathColliding=true
	OreEventThreshold=100.0

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent0
		StaticMesh=StaticMesh'GP_Conquest.SM.Mesh.S_GP_Conq_SM_ProcessingPlant01'
		CollideActors=true
		BlockActors=true
		CastShadow=true
		HiddenGame=false
		Translation=(X=0.0,Y=0.0,Z=-50.0)
		Scale3D=(X=1.0,Y=1.0,Z=1.0)
		bUseAsOccluder=FALSE
	End Object
	CollisionComponent=StaticMeshComponent0
	Components.Add(StaticMeshComponent0)

	Begin Object Name=CollisionCylinder
		CollisionHeight=+50.0
		CollisionRadius=+300.0
	End Object

	SupportedEvents.Add(class'UTSeqEvent_MinedOre')

	MiningBotClass=class'UTOnslaughtMiningRobot_Content'
}
