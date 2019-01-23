/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Raptor_Content extends UTVehicle_Raptor;

var color EffectColor[2];

simulated function SetVehicleEffectParms(name TriggerName, ParticleSystemComponent PSC)
{
	if (TriggerName == 'RaptorWeapon01' || TriggerName == 'RaptorWeapon02')
	{
		PSC.SetColorParameter('MFlashColor',EffectColor[GetTeamNum()]);
	}
	else
	{
		Super.SetVehicleEffectParms(TriggerName, PSC);
	}
}

simulated function vector GetPhysicalFireStartLoc(UTWeapon ForWeapon)
{
	local vector RocketSocketL;
	local rotator RocketSocketR;

	if (ForWeapon.CurrentFireMode == 1)
	{
		Mesh.GetSocketWorldLocationAndRotation('RocketSocket',RocketSocketL, RocketSocketR);
		return RocketSocketL;
	}
	else
		return Super.GetPhysicalFireStartLoc(ForWeapon);
}

simulated state DyingVehicle
{
	/** spawn an explosion effect and damage nearby actors */
	simulated function InitialVehicleExplosion()
	{
		super.InitialVehicleExplosion();
		if ( FRand() < 0.5 )
			MaybeBreak('leftwing');
		else
			MaybeBreak('rightwing');
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=70.0
		CollisionRadius=140.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Class=CylinderComponent Name=RagdollCollisionCylinder
		CollisionHeight=70.0
		CollisionRadius=140.0
	End Object
	RagdollCylinder=RagdollCollisionCylinder

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Raptor.Mesh.SK_VH_Raptor'
		AnimTreeTemplate=AnimTree'VH_Raptor.Anims.AT_VH_Raptor'
		PhysicsAsset=PhysicsAsset'VH_Raptor.Anims.SK_VH_Raptor_Physics'
	End Object

	DrawScale=1.3

	Seats(0)={(	GunClass=class'UTVWeap_RaptorGun',
				GunSocket=(Gun_Socket_01,Gun_Socket_02),
				GunPivotPoints=(left_gun,rt_gun),
				TurretControls=(gun_rotate_lt,gun_rotate_rt),
				CameraTag=ViewSocket,
				CameraOffset=-384)}

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=RaptorEngineSound
		SoundCue=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_EngineLoop'
	End Object
	EngineSound=RaptorEngineSound
	Components.Add(RaptorEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=RaptorCollideSound
		SoundCue=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_Collide'
	End Object
	CollideSound=RaptorCollideSound
	Components.Add(RaptorCollideSound);


	EnterVehicleSound=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_Start'
	ExitVehicleSound=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_Stop'

	// Scrape sound.
	Begin Object Class=AudioComponent Name=BaseScrapeSound
		SoundCue=SoundCue'A_Gameplay.A_Gameplay_Onslaught_MetalScrape01Cue'
	End Object
	ScrapeSound=BaseScrapeSound
	Components.Add(BaseScrapeSound);

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	VehicleEffects(0)=(EffectStartTag=RaptorWeapon01,EffectTemplate=ParticleSystem'VH_Raptor.Effects.PS_Raptor_MF',EffectSocket=Gun_Socket_02)
	VehicleEffects(1)=(EffectStartTag=RaptorWeapon02,EffectTemplate=ParticleSystem'VH_Raptor.Effects.PS_Raptor_MF',EffectSocket=Gun_Socket_01)
 	VehicleEffects(2)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=DamageSmoke01)

	VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Contrail',EffectSocket=LeftTip)
	VehicleEffects(4)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Contrail',EffectSocket=RightTip)
	VehicleEffects(5)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Contrail',EffectSocket=RearRtContrail)
	VehicleEffects(6)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Contrail',EffectSocket=RearLtContrail)

	VehicleEffects(7)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Exhaust',EffectSocket=ExhaustL)
	VehicleEffects(8)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_Raptor_Exhaust',EffectSocket=ExhaustR)
	VehicleEffects(9)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Raptor.EffectS.P_VH_Raptor_GroundEffect',EffectSocket=GroundEffectBase)

	ContrailEffectIndices=(3,4,5,6)
	GroundEffectIndices=(9)

	RagdollMesh=SkeletalMesh'VH_Raptor.Mesh.SK_VH_Raptor_Damaged'
	RagdollAsset=PhysicsAsset'VH_Raptor.Mesh.SK_VH_Raptor_Damaged_Physics'
	RagdollRotationOffset=(Yaw=-16384)

	SpawnInTemplates[0]=ParticleSystem'VH_Raptor.Effects.P_VH_Raptor_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Raptor.Effects.P_VH_Raptor_Spawn_Blue'
	SpawnInTime=5.00

 	EffectColor(0)=(R=151,G=26,B=35,A=255)
 	EffectColor(1)=(R=35,G=26,B=151,A=255)

	IconXStart=0.5
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	ExplosionSound=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_Explode'
	TurretPivotSocketName=TurretPiv
	HoverBoardAttachSockets=(HoverAttach00)

	PawnHudScene=VHud_Raptor'UI_Scenes_HUD.VehicleHUDs.VH_Raptor'

}
