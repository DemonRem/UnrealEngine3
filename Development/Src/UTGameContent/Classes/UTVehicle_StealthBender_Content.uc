/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_StealthBender_Content extends UTVehicle_StealthBender;

/** The Template of the Beam to use */

var particlesystem BeamTemplate;

/** The primary fire beam */
var ParticleSystemComponent BeamEmitter;

/** Sound that plays while primary fires*/
var protected AudioComponent BeamAmbientSound;

/** The sound used for the audio component BeamAmbientSound */
var soundcue BeamFireSound;

/** The name of the EndPoint parameter */
var name EndPointParamName;

/** team based colors for beam when targeting a teammate */
var color LinkBeamColors[3];

var MaterialInstance VisibleSecondarySkin;
var MaterialInstance CloakedSecondarySkin;
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	AddBeamEmitter();
	PlayVehicleAnimation('Inactive');
}

simulated event Destroyed()
{
	super.Destroyed();
	KillBeamEmitter();
}

function bool DriverEnter(Pawn P)
{
	UTVWeap_NightShadeGun(Seats[0].Gun).bShowDeployableName = false;
	return Super.DriverEnter(P);
}

simulated function AddBeamEmitter()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (BeamEmitter == None)
		{
			if (BeamTemplate != None)
			{
				BeamEmitter = new(Outer) class'UTParticleSystemComponent';
				BeamEmitter.SetTemplate(BeamTemplate);
				BeamEmitter.SetHidden(true);
				Mesh.AttachComponentToSocket( BeamEmitter,Seats[0].GunSocket[0] );
			}
		}
		else
		{
			BeamEmitter.ActivateSystem();
		}
	}
}

simulated function KillBeamEmitter()
{
	if (BeamEmitter != none)
	{
		SetBeamEmitterHidden(true);
		BeamEmitter.DeactivateSystem();
	}
}

simulated function SetBeamEmitterHidden(bool bHide)
{
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if ( BeamEmitter.HiddenGame != bHide )
		{
			if (BeamEmitter != none)
				BeamEmitter.SetHidden(bHide);

			BeamAmbientSound.Stop();
			if (!bHide)
			{
				BeamAmbientSound.SoundCue = BeamFireSound;
				BeamAmbientSound.Play();
				BeamEmitter.ActivateSystem();
			}
		}
	}
}

static function color GetTeamBeamColor(byte TeamNum)
{
	if (TeamNum >= ArrayCount(default.LinkBeamColors))
	{
		TeamNum = ArrayCount(default.LinkBeamColors) - 1;
	}

	return default.LinkBeamColors[TeamNum];
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local color BeamColor;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	if (SeatIndex == 0)
	{
//		HitEffectColor = (Team < 2) ? TeamFireColor[Team] : TeamFireColor[0];
		SetBeamEmitterHidden(false);
		BeamEmitter.SetVectorParameter(EndPointParamName, FlashLocation);

		if (FiringMode == 2 && WorldInfo.GRI.GameClass.Default.bTeamGame)
		{
			BeamColor = GetTeamBeamColor(Instigator.GetTeamNum());
		}
		else
		{
			BeamColor = GetTeamBeamColor(255);
		}
		BeamEmitter.SetColorParameter('Link_Beam_Color', BeamColor);

	}
}

simulated function VehicleWeaponStoppedFiring( bool bViaReplication, int SeatIndex )
{
	SetBeamEmitterHidden(true);
}

/**
 * Function cloaks or decloaks the vehicle.
 */
simulated function Cloak(bool bIsEnabled)
{
	if(bIsEnabled && Mesh.Materials[1] != CloakedSkin)
	{
		Mesh.SetMaterial(1,BodyMaterialInstance);
	}
	else if(!bIsEnabled && Mesh.Materials[1] != VisibleSecondarySkin)
	{
		Mesh.SetMaterial(1,VisibleSecondarySkin);
	}
	super.Cloak(bIsEnabled);
}
defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=+40.0
		CollisionRadius=+170.0
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_StealthBender.Mesh.SK_VH_StealthBender'
		PhysicsAsset=PhysicsAsset'VH_Hellbender.Mesh.SK_VH_Hellbender_Physics'
		AnimTreeTemplate=AnimTree'VH_StealthBender.Anims.AT_VH_StealthBender'
		AnimSets(0)=AnimSet'VH_StealthBender.Anims.K_VH_StealthBender'
	End Object

	DrawScale=1.2

	RagdollMesh=SkeletalMesh'VH_Hellbender.Mesh.SK_VH_HellbenderDestroyed'
	RagdollAsset=PhysicsAsset'VH_Hellbender.Mesh.SK_VH_HellbenderDestroyed_Physics'
	RagdollRotationOffset=(Yaw=-20480)
	RagdollOffset=(Y=-70.0,Z=20.0)
	BigExplosionTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DeathExplosion'

	Seats(0)={(	GunClass=class'UTVWeap_StealthbenderGun',
				GunSocket=(GunnerFireSocket),
				TurretControls=(GunnerConstraint,DeployYaw),
				CameraTag=DriverViewSocket,
				CameraOffset=-400,
				DriverDamageMult=0.0,
			 )}

	Begin Object Class=AudioComponent name=BeamAmbientSoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
	End Object
	BeamAmbientSound=BeamAmbientSoundComponent
	Components.Add(BeamAmbientSoundComponent)

	BeamFireSound=SoundCue'A_Weapon.LinkGun.Cue.A_Weapon_Link_AltFire_Cue'

	SkinTranslucencyName=skintranslucency
	HitEffectName=HitEffect


/*
	Seats(1)={(	GunClass=class'UTVWeap_HellBenderPrimary',
				GunSocket=(TurretFireSocket),
				GunPivotPoints=(MainTurretYaw),
				TurretVarPrefix="Turret",
				TurretControls=(TurretConstraintPitch,TurretConstraintYaw),
				CameraEyeHeight=20,
				CameraOffset=-256,
				CameraTag=TurretViewSocket,
				DriverDamageMult=0.2,
				ImpactFlashLightClass=class'UTGame.UTShockMuzzleFlashLight',
				MuzzleFlashLightClass=class'UTGame.UTTurretMuzzleFlashLight',
				bSeatVisible=true,
				SeatBone=MainTurretYaw,
				SeatOffset=(X=30,Y=0,Z=-15))}
*/

	// Sounds

	Begin Object Class=AudioComponent Name=ScorpionTireSound
		SoundCue=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireDirt01Cue'
	End Object
	TireAudioComp=ScorpionTireSound
	Components.Add(ScorpionTireSound);

	TireSoundList(0)=(MaterialType=Dirt,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireDirt01Cue')
	TireSoundList(1)=(MaterialType=Foliage,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireFoliage01Cue')
	TireSoundList(2)=(MaterialType=Grass,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireGrass01Cue')
	TireSoundList(3)=(MaterialType=Metal,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireMetal01Cue')
	TireSoundList(4)=(MaterialType=Mud,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireMud01Cue')
	TireSoundList(5)=(MaterialType=Snow,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireSnow01Cue')
	TireSoundList(6)=(MaterialType=Stone,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireStone01Cue')
	TireSoundList(7)=(MaterialType=Wood,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireWood01Cue')

	// Engine sound.
	Begin Object Class=AudioComponent Name=HellBenderEngineSound
		SoundCue=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineIdle'
	End Object
	EngineSound=HellBenderEngineSound
	Components.Add(HellBenderEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=HellBenderCollideSound
		SoundCue=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_Collide'
	End Object
	CollideSound=HellBenderCollideSound
	Components.Add(HellBenderCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineStart'
	ExitVehicleSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineStop'

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=250.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	VehicleEffects(0)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=DamageSmoke01)
	VehicleEffects(1)=(EffectStartTag=ShockTurretFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverPrimMuzzleFlash',EffectSocket=GunnerFireSocket)
	VehicleEffects(2)=(EffectStartTag=ShockTurretAltFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverAltMuzzleFlash',EffectSocket=GunnerFireSocket)

	//VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_GenericExhaust',EffectSocket=ExhaustLeft)
	//VehicleEffects(4)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_GenericExhaust',EffectSocket=ExhaustRight)

	VehicleAnims(0)=(AnimTag=EngineStart,AnimSeqs=(GetIn),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)
	VehicleAnims(2)=(AnimTag=EngineStop,AnimSeqs=(GetOut),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)
	//VehicleAnims(3)=(AnimTag=Inactive,AnimSeqs=(InactiveIdle),AnimRate=1.0,bAnimLoopLastSeq=true,AnimPlayerName=AnimPlayer)

	SpawnInTemplates[0]=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_Spawn_Blue'
	SpawnInTime=5.00

	BeamTemplate=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'

	ExplosionSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_Explode'
	ExhaustEffectName=ExhaustVel

	// deploy functionality
	IdleAnim(0)=InactiveIdle
	IdleAnim(1)=Idle
	DeployAnim(0)=ArmExtend
	DeployAnim(1)=ArmRetract

	DeploySound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'
	UndeploySound=Soundcue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'


	DeployTime = 1.3;
	UnDeployTime = 1.3;

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	EndPointParamName=LinkBeamEnd

	LinkBeamColors(0)=(R=255,G=64,B=64,A=255)
	LinkBeamColors(1)=(R=64,G=64,B=255,A=255)
	LinkBeamColors(2)=(R=32,G=255,B=32,A=255)
	TurretName=DeployYaw

	DrivingPhysicalMaterial=PhysicalMaterial'vh_hellbender.materials.physmat_hellbenderdriving'
	DefaultPhysicalMaterial=PhysicalMaterial'vh_hellbender.materials.physmat_hellbender'

	PawnHudScene=VHud_StealthBender'UI_Scenes_HUD.VehicleHUDs.VH_StealthBender'

	VisibleSkin=MaterialInstance'VH_Hellbender.Materials.MI_VH_Hellbender_Red'
	VisibleSecondarySkin=MaterialInstance'VH_StealthBender.Materials.M_VH_Stealthbender'
	CloakedSkin=MaterialInstance'VH_StealthBender.Materials.M_VH_StealthBender_Hellbender_Skin';
	CloakedSecondarySkin=MaterialInstance'VH_StealthBender.Materials.M_VH_StealthBender_Skin'
	DeployedViewSocket=DeployViewSocket
	TeamSkinParamName=SkinColor
}
