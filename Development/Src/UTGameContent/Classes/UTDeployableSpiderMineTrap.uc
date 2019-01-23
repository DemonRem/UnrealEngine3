/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeployableSpiderMineTrap extends UTDeployable;

var array< class<UTSpiderMineTrap> > TeamSpiderTrapClasses;

static function class<Actor> GetTeamDeployable(int TeamNum)
{
	if (TeamNum >= default.TeamSpiderTrapClasses.length)
	{
		TeamNum = 0;
	}
	return default.TeamSpiderTrapClasses[TeamNum];
}

function bool Deploy()
{
	DeployedActorClass = class<UTSpiderMineTrap>(GetTeamDeployable(Instigator.GetTeamNum()));
	return Super.Deploy();
}

defaultproperties
{
	TeamSpiderTrapClasses[0]=class'UTSpiderMineTrapRed'
	TeamSpiderTrapClasses[1]=class'UTSpiderMineTrapBlue'

	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_HU_Deco_SM_FanBox'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		BlockRigidBody=false
		Scale3D=(X=0.025,Y=0.025,Z=0.025)
		Translation=(X=20,Y=10,Z=-20)
		bUseAsOccluder=FALSE
	End Object
	Mesh=DeployableMesh
	Components.Add(DeployableMesh)
	Components.Remove(FirstPersonMesh)

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=ThirdPersonMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_HU_Deco_SM_FanBox'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Scale3D=(X=0.2,Y=0.2,Z=0.2)
		Translation=(Z=-30)
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=ThirdPersonMesh
	PickupFactoryMesh=ThirdPersonMesh

	FireOffset=(X=25.0)
}
