/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeployableTripwire extends UTDeployable;

var class<UTTripWire> TripwireClass;

simulated function bool Deploy()
{
	local UTTripwire S;
	local vector SpawnLocation;
	local float traceDist;
	local vector EndTrace;
	local rotator Aim;
	local actor HitWall;
	local vector HitLoc,HitNorm;

	traceDist = 2000.0f;

	SpawnLocation = GetPhysicalFireStartLoc();
	//Aim.Yaw = GetAdjustedAim(SpawnLocation).Yaw;
	Aim = GetAdjustedAim(SpawnLocation);
	EndTrace = SpawnLocation + vector(Aim)*traceDist;
	HitWall = Instigator.Trace(HitLoc,HitNorm,EndTrace,SpawnLocation,false);
	if (HitWall != None && Pawn(HitWall) == none)
	{
		S = Spawn( Tripwireclass,,,
				HitLoc,
				Aim );

		//S = Spawn(Tripwireclass, Instigator,, SpawnLocation + vector(Aim)*(1.1*Instigator.GetCollisionRadius()+Tripwireclass.default.CylinderComponent.CollisionRadius),Aim);
		if (S != None)
		{
			S.TeamNum = Instigator.GetTeamNum();
			S.OnDeployableUsedUp = Factory.DeployableUsed;
			EndTrace = S.Location + vector(Aim) * -1 * Tripwireclass.default.MaxLength;
			//DrawDebugLine(Spawnlocation,Endtrace,255,0,255,true);
			if(S.Trace(HitLoc,HitNorm,EndTrace,SpawnLocation,false) != none)
			{
				//DrawDebugLine(S.Location,HitLoc,0,255,0,true);
				S.StartLaser(HitLoc,HitNorm);
				S.MakeActiveInTime(0.75f);
				return true;
			}
			else
			{
				S.Destroy();
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'Pickups.Translocator.Mesh.S_Translocator'
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
	Mesh=DeployableMesh
	Components.Add(DeployableMesh)
	Components.Remove(FirstPersonMesh)

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=ThirdPersonMesh
		StaticMesh=StaticMesh'Pickups.Translocator.Mesh.S_Translocator'
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
	DroppedPickupMesh=ThirdPersonMesh
	PickupFactoryMesh=ThirdPersonMesh



	TripwireClass=class'UTTripWire'
}
