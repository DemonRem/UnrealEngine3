class UTDeployableTeleportBeacon extends UTDeployable;

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'Pickups.Translocator.Mesh.S_Translocator'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		BlockRigidBody=false
		Scale=4.0
		Translation=(X=20,Y=10,Z=-20)
		bUseAsOccluder=false
	End Object
	Mesh=DeployableMesh
	Components.Add(DeployableMesh)
	Components.Remove(FirstPersonMesh)

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=ThirdPersonMesh
		StaticMesh=StaticMesh'Pickups.Translocator.Mesh.S_Translocator'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Scale=4.0
		Translation=(Z=-30)
		bUseAsOccluder=false
	End Object
	DroppedPickupMesh=ThirdPersonMesh
	PickupFactoryMesh=ThirdPersonMesh

	FireOffset=(X=25.0)

	DeployedActorClass=class'UTTeleportBeacon'
}
