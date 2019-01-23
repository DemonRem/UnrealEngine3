/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_Berserk extends UTPowerupPickupFactory;


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
	InventoryType=class'UTGame.UTBerserk'
	bRotatingPickup=true
	YawRotationRate=32768

	BaseBrightEmissive=(R=50.0,G=1.0)
	BaseDimEmissive=(R=10.0,G=0.25)

	Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.Berserk.Mesh.S_Pickups_Berserk'
		HiddenGame=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=true
		CollideActors=false
		AlwaysLoadOnServer=false
		AlwaysLoadOnClient=false
		Scale3D=(X=0.7,Y=0.7,Z=0.7)
		CullDistance=8000
		Materials(0)=Material'Pickups.Berserk.Materials.M_Pickups_Berserk'
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
 	Components.Add(MeshComponentA)
 	PickupMesh=MeshComponentA

	Begin Object Class=UTParticleSystemComponent Name=BerserkParticles
		Template=ParticleSystem'Pickups.Berserk.Effects.P_Pickups_Berserk_Idle'
		bAutoActivate=false
		SecondsBeforeInactive=1.0
	End Object
	Components.Add(BerserkParticles);
	ParticleEffects = BerserkParticles;

	Begin Object Class=AudioComponent Name=BerserkReady
		SoundCue=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_GroundLoopCue'
	End Object
	PickupReadySound=BerserkReady
	Components.Add(BerserkReady)
	RespawnSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_SpawnCue'

}

