/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_UDamage extends UTPowerupPickupFactory;


simulated function SetPickupVisible()
{
	ParticleEffects.ActivateSystem();
	super.SetPickupVisible();
}

simulated function SetPickupHidden()
{
	ParticleEffects.DeactivateSystem();
	super.SetPickupHidden();
}


defaultproperties
{
	InventoryType=class'UTGame.UTUDamage'
	bRotatingPickup=true
	YawRotationRate=32768
	BaseBrightEmissive=(R=4.0,G=1.0,B=10.0)
	BaseDimEmissive=(R=2.0,G=0.5,B=5.0)

	Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.Udamage.Mesh.S_Pickups_UDamage'
		HiddenGame=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=true
		CollideActors=false
		BlockRigidBody=false
		AlwaysLoadOnServer=false
		AlwaysLoadOnClient=false
		Scale3D=(X=0.7,Y=0.7,Z=0.7)
		CullDistance=8000
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
 	Components.Add(MeshComponentA)
 	PickupMesh=MeshComponentA

	Begin Object Class=UTParticleSystemComponent Name=DamageParticles
		Template=ParticleSystem'Pickups.UDamage.Effects.P_Pickups_UDamage_Idle'
		bAutoActivate=false
		SecondsBeforeInactive=1.0
	End Object
	Components.Add(DamageParticles);
	ParticleEffects = DamageParticles;

	Begin Object Class=AudioComponent Name=DamageReady
		SoundCue=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_GroundLoopCue'
	End Object
	PickupReadySound=DamageReady
	Components.Add(DamageReady)
	RespawnSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_SpawnCue'
}

