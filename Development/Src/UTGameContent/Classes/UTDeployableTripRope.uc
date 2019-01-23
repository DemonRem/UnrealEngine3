/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeployableTripRope extends UTDeployableTripwire;

defaultproperties
{
	Begin Object Name=DeployableMesh
		StaticMesh=StaticMesh'Pickups.Deployables.Mesh.S_LASERWIRE_BEAM' //hack
 		CollideActors=false
		BlockActors=false
		CastShadow=false	
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Scale3D=(X=3,Y=3,Z=3)
		DepthPriorityGroup=SDPG_Foreground
		Translation=(X=35,Y=10,Z=-35)
		bUseAsOccluder=FALSE
	End Object

	// Pickup staticmesh
	Begin Object Name=ThirdPersonMesh
		StaticMesh=StaticMesh'Pickups.Deployables.Mesh.S_LASERWIRE_BEAM' // hack
		CollideActors=false
		BlockActors=false
		CastShadow=false
		BlockRigidBody=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Scale3D=(X=1,Y=1,Z=1)
		Translation=(Z=-30)
		bUseAsOccluder=FALSE
	End Object

	TripwireClass=class'UTTriprope'
}
