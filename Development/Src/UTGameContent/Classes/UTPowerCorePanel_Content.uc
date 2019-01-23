/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTPowerCorePanel_Content extends UTPowerCorePanel;


defaultproperties
{
	Begin Object Name=GibMesh Class=StaticMeshComponent
		StaticMesh=StaticMesh'GP_Onslaught.Mesh.S_GP_Ons_Power_Core_Panel'
		bCastDynamicShadow=false
		Scale=0.6
		BlockActors=false
		CollideActors=true
		BlockRigidBody=true
		CastShadow=false
		bNotifyRigidBodyCollision=true
		ScriptRigidBodyCollisionThreshold=10.0
		bUseHardwareScene=true
		RBCollideWithChannels=(Default=true,Pawn=true,Vehicle=true,GameplayPhysics=true,EffectPhysics=true)
		bUseAsOccluder=false
	End Object
	Mesh=GibMesh
	CollisionComponent=GibMesh
	Components.Add(GibMesh)

	TickGroup=TG_PostAsyncWork
	RemoteRole=ROLE_None
	Physics=PHYS_RigidBody
	bNoEncroachCheck=true
	bCollideActors=true
	bBlockActors=false
	bWorldGeometry=false
	bCollideWorld=true
	bProjTarget=true
	LifeSpan=8.0
	bGameRelevant=true

	BreakSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CorePanelBreakCue'
	HitSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CorePanelImpactCue'
}
