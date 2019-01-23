/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_SPMA_Content extends UTVehicle_SPMA;

/** The Template of the Beam to use */
var ParticleSystem BeamTemplate;
/** sound of the moving turret while deployed*/
var SoundCue TurretMovementSound;

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local UTEmitter ShockBeam;
	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	// Handle Beam Effects for the shock beam

	if (SeatIndex==1 && !IsZero(HitLocation))
	{
		ShockBeam = Spawn( class'UTEmitter',self,,GetEffectLocation(SeatIndex) );
		if (ShockBeam != none)
		{
			ShockBeam.SetTemplate(BeamTemplate);
			ShockBeam.ParticleSystemComponent.SetVectorParameter('ShockBeamEnd', HitLocation );
		}
	}
}

simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	Super.CauseMuzzleFlashLight(SeatIndex);
	if (SeatIndex==0)
	{
		PlayAnim('Fire');
		VehicleEvent('DriverGun');
	}
	else if (SeatIndex==1)
	{
		VehicleEvent('PassengerGun');
	}
}

simulated function SitDriver( UTPawn UTP, int SeatIndex)
{
	Super.SitDriver(UTP,SeatIndex);
	if (SeatIndex == 0 && DeployedState == EDS_Undeployed )
	{
		PlayAnim( 'GetIn' );
	}
}

function PassengerLeave(int SeatIndex)
{
	Super.PassengerLeave(SeatIndex);
	if (SeatIndex == 0 && DeployedState == EDS_Undeployed)
	{
		PlayAnim( 'GetOut' );
	}
}

simulated function SetVehicleDeployed()
{
	Super.SetVehicleDeployed();

	// add turret motion sound
	if (WorldInfo.NetMode != NM_DedicatedServer && Seats[0].SeatMotionAudio == None && TurretMovementSound != None)
	{
		Seats[0].SeatMotionAudio = CreateAudioComponent(TurretMovementSound, false, true);
	}
}

simulated function SetVehicleUndeployed()
{
	Super.SetVehicleUndeployed();

	// remove turret motion sound/stop sound
	if (Seats[0].SeatMotionAudio != None)
	{
		Seats[0].SeatMotionAudio.Stop();
		Seats[0].SeatMotionAudio = None;
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=260.000000
		Translation=(X=0.0,Y=0.0,Z=100.0)
	End Object
	CylinderComponent=CollisionCylinder


	Begin Object Name=SVehicleMesh
		AnimSets(0)=AnimSet'VH_SPMA.Anims.VH_SPMA_Anims'
		SkeletalMesh=SkeletalMesh'VH_SPMA.Mesh.SK_VH_SPMA'
		PhysicsAsset=PhysicsAsset'VH_SPMA.Mesh.SK_VH_SPMA_Physics'
		AnimTreeTemplate=AnimTree'VH_SPMA.Anims.AT_VH_SPMA'
	End Object

	DrawScale=1.3

	// turret twist sound.
	TurretMovementSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_CannonRotate'

	Seats(0)={(	GunClass=class'UTVWeap_SPMACannon_Content',
				GunSocket=(TurretFireSocket),
				GunPivotPoints=(MainTurret_Yaw),
				TurretControls=(TurretConstraint,TurretYawConstraint),
				CameraTag=DriverViewSocket,
				CameraOffset=-320,
				MuzzleFlashLightClass=class'UTTankMuzzleFlash')}

	Seats(1)={(	GunClass=class'UTVWeap_SPMAPassengerGun',
				GunSocket=(GunnerFireSocket),
				GunPivotPoints=(SecondaryTurret_YawLift),
				TurretVarPrefix="Gunner",
				TurretControls=(GunnerConstraint,GunnerYawConstraint),
				CameraTag=GunnerViewSocket,
				CameraOffset=-16,
				CameraEyeHeight=5)}

	VehicleEffects(0)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=DamageSmoke01)

	VehicleEffects(1)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceA)
	VehicleEffects(2)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceB)
	VehicleEffects(3)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceC)
	VehicleEffects(4)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceD)
	VehicleEffects(5)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceE)
	VehicleEffects(6)=(EffectStartTag=Deployed,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Piston_Smoke',EffectSocket=BraceF)

	// Muzzle Flashes
	VehicleEffects(7)=(EffectStartTag=CannonFire,EffectTemplate=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_PrimaryMuzzleFlash',EffectSocket=TurretFireSocket)
	VehicleEffects(8)=(EffectStartTag=CameraFire,EffectTemplate=ParticleSystem'VH_SPMA.Effects.P_VH_SPMA_AltMuzzleFlash',EffectSocket=TurretFireSocket)
	VehicleEffects(9)=(EffectStartTag=ShockTurretFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverPrimMuzzleFlash',EffectSocket=GunnerFireSocket)
	VehicleEffects(10)=(EffectStartTag=ShockTurretAltFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverAltMuzzleFlash',EffectSocket=GunnerFireSocket)

	VehicleAnims(0)=(AnimTag=CannonFire,AnimSeqs=(Fire),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)
	VehicleAnims(1)=(AnimTag=CameraFire,AnimSeqs=(Fire),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)


	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=SPMAEngineSound
		SoundCue=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_EngineIdle'
	End Object
	EngineSound=SPMAEngineSound
	Components.Add(SPMAEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=SPMACollideSound
		SoundCue=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Collide'
	End Object
	CollideSound=SPMACollideSound
	Components.Add(SPMACollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_EngineRampUp'
	ExitVehicleSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_EngineRampDown'

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=250.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	BeamTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam'

	IdleAnim(0)=InActiveStill
	IdleAnim(1)=ActiveStill
	DeployAnim(0)=Deploying
	DeployAnim(1)=UnDeploying
	TreadSpeedParameterName=tread

	IconXStart=0
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	DeploySound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'
	UndeploySound=Soundcue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'

	ExplosionSound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Explode'
	HoverBoardAttachSockets=(HoverAttach00)

	PawnHudScene=VHud_SPMA'UI_Scenes_HUD.VehicleHUDs.VH_SPMA'
	PassengerTeamBeaconOffset=(X=100.0f,Y=15.0f,Z=50.0f);	
}
