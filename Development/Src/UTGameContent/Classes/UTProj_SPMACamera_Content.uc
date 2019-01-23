/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_SPMACamera_Content extends UTProj_SPMACamera;

/** This is the Particle System that will get created and attached to the light sockets */
var ParticleSystem LightEffectTemplate[2];

/** Holds access to the lights */
var ParticleSystemComponent TeamLights[4];

/**
 * Add Team Color Effects
 */
simulated function AddTeamHighlights()
{
	local int i, TeamIndex;
	local name n;

	TeamIndex = clamp( Instigator.GetTeamNum(), 0 ,1);

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		for (i=0;i<4;i++)
		{
			TeamLights[i] = new(Outer) class'UTParticleSystemComponent';
			n = Name("Light"$i);
			Mesh.AttachComponentToSocket(TeamLights[i],n);
			TeamLights[i].SetTemplate( LightEffectTemplate[TeamIndex]  );
		}
	}
}

simulated function RemoveTeamHighlights()
{
	local int i;
	for (i=0;i<4;i++)
	{
		if (TeamLights[i] != none)
		{
			TeamLights[i].DeactivateSystem();
			TeamLights[i] = none;
		}
	}
}

simulated function ShutDown()
{
	RemoveTeamHighlights();
	Super.ShutDown();
}

simulated event Destroyed()
{
	RemoveTeamHighlights();
	Super.Destroyed();
}

simulated function DeployCamera()
{
	Super.DeployCamera();

	AddTeamHighlights();

	if (DeploySound != none)
	{
		PlaySound(DeploySound, true);
	}

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		Mesh.PlayAnim('MissleOpen');
	}
}

simulated function bool CalcCamera(float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV)
{
	local bool bResult;

	bResult = Super.CalcCamera(fDeltaTime, out_CamLoc, out_CamRot, out_FOV);

	if (Mesh.bOwnerNoSee)
	{
		RemoveTeamHighlights();
	}

	return bResult;
}

defaultproperties
{
	Components.Remove(ProjectileMesh)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Class=SkeletalMeshComponent Name=ProjMesh
		SkeletalMesh=SkeletalMesh'VH_SPMA.Mesh.SK_VH_SPMA_Camera'
		AnimSets(0)=AnimSet'VH_SPMA.Anims.K_VH_SPMA_Camera'
		Animations=MeshSequenceA
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=true
		Scale=0.15
		bUseAsOccluder=FALSE
	End Object
	Mesh=ProjMesh
	Components.Add(ProjMesh)

	Begin Object Class=StaticMeshComponent Name=CursorMeshCross
		StaticMesh=StaticMesh'VH_SPMA.Mesh.TargetCross'
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		CullDistance=200000
		BlockRigidBody=false
		DepthPriorityGroup=SDPG_Foreground
		AbsoluteTranslation=true
		AbsoluteRotation=true
		HiddenGame=true
		bUseAsOccluder=FALSE
		bOnlyOwnerSee=true
	End Object
	CursorComp(0)=CursorMeshCross
	Components.Add(CursorMeshCross)

	Begin Object Class=StaticMeshComponent Name=CursorMeshNot
		StaticMesh=StaticMesh'VH_SPMA.Mesh.TargetNot'
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		CullDistance=200000
		BlockRigidBody=false
		DepthPriorityGroup=SDPG_Foreground
		AbsoluteTranslation=true
		AbsoluteRotation=true
		HiddenGame=true
		bUseAsOccluder=FALSE
		bOnlyOwnerSee=true
	End Object
	CursorComp(1)=CursorMeshNot
	Components.Add(CursorMeshNot)

	Begin Object Name=CollisionCylinder
		CollisionRadius=48
		CollisionHeight=32
		AlwaysLoadOnClient=True
		AlwaysLoadOnServer=True
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		CollideActors=true
	End Object

	AmbientSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_CameraAmbient'

	ProjFlightTemplate=WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketTrail
	ProjExplosionTemplate=Envy_Effects.Tests.Effects.P_SPMA_CamImpact

	MyDamageType=class'UTDmgType_SPMACameraCrush'

	LightEffectTemplate(0)=WP_Translocator.Particles.P_WP_Translocator_Trail_Red
	LightEffectTemplate(1)=WP_Translocator.Particles.P_WP_Translocator_Trail

	DeploySound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_CameraDeploy'
	ShotDownSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPAM_CameraShotDown'
	ExplosionSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_CameraCrash'

	PS_Trail[1]=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_Target_ArcTrail_Blue';
	PS_Trail[0]=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_Target_ArcTrail_Red';
	PS_StartPoint=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_Target_ArcStart'
	PS_EndPoint[1]=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_Target_ArcEnd_Blue'
	PS_EndPoint[0]=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_Target_ArcEnd_Red'
}
