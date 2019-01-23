/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDeployableSlowVolume extends UTDeployable;


static function class<Actor> GetTeamDeployable(int TeamNum)
{
	return class'UTSlowVolume';
}
function bool Deploy()
{
	local UTSlowVolume Volume;
	local vector SpawnLocation;
	local rotator Aim;

	Aim.Yaw = Instigator.GetViewRotation().Yaw;
	SpawnLocation = Instigator.Location + (FireOffset >> Aim);
	Volume = Spawn(class'UTSlowVolume',,, SpawnLocation, Aim);
	if (Volume != None)
	{
		Volume.OnDeployableUsedUp = Factory.DeployableUsed;
		return true;
	}
	else
	{
		return false;
	}
}

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_NR_Walls_SM_Wall02'
		Materials[0]=Material'VH_Paladin.Materials.M_VH_Paladin_Shield01'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		BlockRigidBody=false
		Translation=(X=20,Y=10,Z=-20)
		bUseAsOccluder=FALSE
	End Object
	Mesh=DeployableMesh
	Components.Add(DeployableMesh)
	Components.Remove(FirstPersonMesh)

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=ThirdPersonMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_NR_Walls_SM_Wall02'
		Materials[0]=Material'VH_Paladin.Materials.M_VH_Paladin_Shield01'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Translation=(Z=-30)
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=ThirdPersonMesh
	PickupFactoryMesh=ThirdPersonMesh

	FireOffset=(X=500.0,Y=200.0)
}
