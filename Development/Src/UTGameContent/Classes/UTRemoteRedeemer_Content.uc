/**
* Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
class UTRemoteRedeemer_Content extends UTRemoteRedeemer
	notplaceable;


defaultproperties
{
	Begin Object Class=AudioComponent Name=AmbientSoundComponent
		bStopWhenOwnerDestroyed=true
		bShouldRemainActiveIfDropped=true
	End Object
	PawnAmbientSound=AmbientSoundComponent
	Components.Add(AmbientSoundComponent)

	Begin Object Class=StaticMeshComponent Name=WRocketMesh
		StaticMesh=StaticMesh'WP_AVRiL.Mesh.S_WP_AVRiL_Missile'
		CullDistance=20000
		Scale=1.5
		CollideActors=false
		BlockRigidBody=false
		BlockActors=false
		bOwnerNoSee=true
		bUseAsOccluder=FALSE
	End Object
	Components.Add(WRocketMesh)

	Begin Object Name=CollisionCylinder
		CollisionRadius=+024.000000
		CollisionHeight=+012.000000
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=false
		CollideActors=true
	End Object
	CylinderComponent=CollisionCylinder

	Begin Object Class=UTParticleSystemComponent Name=TrailComponent
		bOwnerNoSee=true
	End Object
	Trail=TrailComponent
	Components.Add(TrailComponent)

	RemoteRole=ROLE_SimulatedProxy
	NetPriority=3
	bNetTemporary=false
	bUpdateSimulatedPosition=true
	bSimulateGravity=false
	Physics=PHYS_Flying
	AirSpeed=1000.0
	AccelRate=2000.0
	bCanTeleport=false
	bDirectHitWall=true
	bCollideActors=false
	bCollideWorld=true
	bBlockActors=false
	BaseEyeHeight=0.0
	EyeHeight=0.0
	bStasis=false
	bCanCrouch=false
	bCanClimbLadders=false
	bCanPickupInventory=false
	bNetInitialRotation=true
	bSpecialHUD=true
	bCanUse=false
	bAttachDriver=false
	DriverDamageMult=1.0
	RedeemerProjClass=class'UTProj_Redeemer'
	LandMovementState=PlayerFlying
	/* FIXME:
	//YawToBankingRatio=60.0
	//BankingResetRate=15.0
	//BankingDamping=10.0
	//MaxBanking=20000
	//BankingToScopeRotationRatio=8.0
	//VelocityToAltitudePanRate=0.00175
	//MaxAltitudePanRate=10.0
	*/
}
