/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_NightShade_Content extends UTVehicle_NightShade;

/** The Link Beam */
var particleSystem BeamTemplate;

/** Holds the Emitter for the Beam */
var ParticleSystemComponent BeamEmitter;

/** Where to attach the Beam */
var name BeamSockets;

/** The name of the EndPoint parameter */
var name EndPointParamName;

var protected AudioComponent BeamAmbientSound;

var soundcue BeamFireSound;

/** team based colors for beam when targeting a teammate */
var color LinkBeamColors[3];

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	AddBeamEmitter();
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
				Mesh.AttachComponentToSocket( BeamEmitter,BeamSockets );
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
		BeamEmitter.SetHidden(true);
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
		HitEffectColor = (Team < 2) ? TeamFireColor[Team] : TeamFireColor[0];
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
 * @HACK - Until we have the actual model, hack the Hellbender turret
 */
simulated function InitializeTurrets()
{
	Super.InitializeTurrets();
	Seats[0].TurretControllers[0].MinAngle.PitchConstraint = -8192;
}

defaultproperties
{
	LinkBeamColors(0)=(R=255,G=64,B=64,A=255)
	LinkBeamColors(1)=(R=64,G=64,B=255,A=255)
	LinkBeamColors(2)=(R=32,G=255,B=32,A=255)

	SkinTranslucencyName=skintranslucency
	HitEffectName=HitEffect

	Begin Object Name=CollisionCylinder
		CollisionHeight=+40.0
		CollisionRadius=+170.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_NightShade.Mesh.SK_VH_NightShade'
		PhysicsAsset=PhysicsAsset'VH_NightShade.Mesh.SK_VH_NightShade_Physics'
		AnimTreeTemplate=AnimTree'VH_NightShade.Anims.AT_VH_NightShade'
		AnimSets(0)=AnimSet'VH_NightShade.Anims.K_VH_NightShade';
	End Object

	VisibleSkin=MaterialInstance'VH_NightShade.Materials.M_VH_NightShade';
	CloakedSkin=MaterialInstance'VH_NightShade.Materials.M_VH_NightShade_Skin';


	RagdollMesh=SkeletalMesh'VH_NightShade.Mesh.SK_VH_NightShade'
	RagdollAsset=PhysicsAsset'VH_NightShade.Mesh.SK_VH_NightShade_Physics'
	BigExplosionTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DeathExplosion'

	Seats(0)={(	GunClass=class'UTVWeap_NightshadeGun_Content',
				GunSocket=(TurretFireSocket),
				TurretVarPrefix="",
				TurretControls=(TurretConstraintPitch,TurretConstraintYaw,DeployYaw),
				GunPivotPoints=(Turret_Pitch),
				CameraTag=DriverViewSocket,
				CameraOffset=-400,
				DriverDamageMult=0.0)}

	// Sounds
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
	VehicleEffects(1)=(EffectStartTag=BackTurretFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_SecondMuzzleFlash',EffectSocket=TurretFireSocket)

	VehicleAnims(0)=(AnimTag=Deployed,AnimSeqs=(ArmExtendIdle),AnimRate=1.0,bAnimLoopLastSeq=true,AnimPlayerName=AnimPlayer);

	BeamTemplate=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'
	BeamSockets=TurretFireSocket
	EndPointParamName=LinkBeamEnd

	Begin Object Class=AudioComponent name=BeamAmbientSoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
	End Object
	BeamAmbientSound=BeamAmbientSoundComponent
	Components.Add(BeamAmbientSoundComponent)

	BeamFireSound=SoundCue'A_Weapon.LinkGun.Cue.A_Weapon_Link_AltFire_Cue'

	SpawnInSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'

	IdleAnim(0)=Idle
	IdleAnim(1)=Idle //ArmExtendIdle
	DeployAnim(0)=ArmExtend
	DeployAnim(1)=ArmRetract

	DeploySound=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'
	UndeploySound=Soundcue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_Deploy'

	DeployTime=3.2
	UnDeployTime=0.7

	HoverBoardAttachSockets=(HoverAttach00)

	PawnHudScene=VHud_Nightshade'UI_Scenes_HUD.VehicleHUDs.VH_NightShade'
	TurretName=DeployYaw
	DeployedViewSocket=DeployViewSocket
	TeamSkinParamName=SkinColor
}
