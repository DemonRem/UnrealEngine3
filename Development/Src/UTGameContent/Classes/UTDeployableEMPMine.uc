/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDeployableEMPMine extends UTDeployable;

defaultproperties
{
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

	DeployedActorClass=class'UTEMPMine'
}
