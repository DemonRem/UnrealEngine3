/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPaladinShield extends UTVehicleShield;

event BaseChange()
{
	local UTVehicle_Paladin Paladin;

	if (ShieldEffectComponent != None && !ShieldEffectComponent.bAttached)
	{
		Paladin = UTVehicle_Paladin(Base);
		if (Paladin != None)
		{
			Paladin.Mesh.AttachComponentToSocket(ShieldEffectComponent, 'ShieldGen');
		}
		else
		{
			AttachComponent(ShieldEffectComponent);
		}
	}
}

event Destroyed()
{
	local UTVehicle_Paladin Paladin;

	Super.Destroyed();

	if (ShieldEffectComponent != None && ShieldEffectComponent.bAttached)
	{
		Paladin = UTVehicle_Paladin(Base);
		if (Paladin != None)
		{
			Paladin.Mesh.DetachComponent(ShieldEffectComponent);
		}
	}
}

defaultproperties
{
	Begin Object Class=AudioComponent Name=AmbientSoundComponent
		bStopWhenOwnerDestroyed=true
		bShouldRemainActiveIfDropped=true
		SoundCue=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_ShieldAmbient'
		bAutoPlay=false
	End Object
	AmbientComponent=AmbientSoundComponent
	Components.Add(AmbientSoundComponent)

	Begin Object Class=StaticMeshComponent Name=ShieldMesh
		StaticMesh=StaticMesh'VH_Paladin.Mesh.S_VH_Paladin_Shield'
		Translation=(X=350,Z=150)
		Scale=2.0
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=true
		BlockNonZeroExtent=true
		HiddenGame=true
		CastShadow=false
		BlockRigidBody=false
		bAcceptsLights=false
		bUseAsOccluder=FALSE
	End Object
	CollisionComponent=ShieldMesh
	Components.Add(ShieldMesh)

	Begin Object Class=UTParticleSystemComponent Name=ShieldEffect
		Translation=(X=-50)
		Template=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_ShieldEffect'
		HiddenGame=true
		bAutoActivate=false
	End Object
	ShieldEffectComponent=ShieldEffect

	ActivatedSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_ShieldActivate'
	DeactivatedSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_ShieldOff'
}
