/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_SuperHealth extends UTHealthPickupFactory;


var ParticleSystemComponent Crackle; // the crackling lightning effect that surrounds the keg on spawn


simulated event ReplicatedEvent(name VarName)
{
	if(VarName == 'bIsRespawning' )
	{
		if(bIsRespawning)
		{
			respawneffect();
		}
	}
	if(VarName == 'bPickupHidden' && bPickupHidden)
	{
		setResOut();
	}
	super.ReplicatedEvent(VarName);
}

state pickup
{
	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
	}
	simulated function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		bIsRespawning=false;
		SetResOut();


	}
}
simulated function RespawnEffect()
{
	super.RespawnEffect();
	bIsRespawning=true;
	Crackle.ActivateSystem();
}

simulated function SetResOut()
{
	MIC_Visibility.setScalarParameterValue(VisibilityParamName, 1.f );
}
simulated function SetPickupHidden()
{
	// NO SUPER CALL, the material takes care of visuals and will break if this does as intended
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
	bPickupHidden = true;
	Glow.SetFloatParameter('LightStrength',0.0f);
}

simulated function SetPickupVisible()
{
	Glow.SetFloatParameter('LightStrength', 1.0f);
	// NO SUPER CALL, the material takes care of visuals and will break if this does as intended
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
	bPickupHidden = false;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	// Create a material instance for the keg:
	MIC_Visibility = MeshComponent(PickupMesh).CreateMaterialInstance(0);
	MIC_Visibility.setScalarParameterValue(VisibilityParamName, 1.f );

	Glow.SetFloatParameter('LightStrength',0.0f);
	Glow.ActivateSystem();
}


defaultproperties
{
	RespawnEffectTime = 1.5f;
	bSuperHeal=true
	bPredictRespawns=true
	bIsSuperItem=true
	RespawnTime=60.000000
	MaxDesireability=2.000000
	HealingAmount=100
	PickupSound=SoundCue'A_Pickups.Health.Cue.A_Pickups_Health_Super_Cue'

	Begin Object Class=StaticMeshComponent Name=BaseMeshComp
		StaticMesh=StaticMesh'Pickups.Health_Large.Mesh.S_Pickups_Base_Health_Large'
		CollideActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Translation=(Z=-44)
		Rotation=(Yaw=16384)
		Scale=0.8
		CullDistance=7000
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
	Components.Add(BaseMeshComp)
	BaseMesh=BaseMeshComp

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		StaticMesh=StaticMesh'Pickups.Health_Large.Mesh.S_Pickups_Health_Large_Keg'
		CollideActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CullDistance=7000
		Materials(0)=Material'Pickups.Health_Large.Materials.M_Pickups_Health_Large_Keg'
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
	Components.Add(StaticMeshComponent1)
	PickupMesh=StaticMeshComponent1
	

	Begin Object Class=UTParticleSystemComponent Name=ParticleGlow
		Template=ParticleSystem'Pickups.Health_Large.Effects.P_Pickups_Base_Health_Glow'
		Translation=(Z=-50.0)
		SecondsBeforeInactive=1.0
	End Object
	Components.Add(ParticleGlow)
	Glow=ParticleGlow

	Begin Object Class=UTParticleSystemComponent Name=ParticleCrackle
		Template=ParticleSystem'Pickups.Health_Large.Effects.P_Pickups_Base_Health_Spawn'
		Translation=(Z=-50.0)
	End Object
	Components.Add(ParticleCrackle)
	Crackle=ParticleCrackle

	YawRotationRate=16384
	bRotatingPickup=true

	bDoVisibilityFadeIn=TRUE
	VisibilityParamName=ResIn
}
