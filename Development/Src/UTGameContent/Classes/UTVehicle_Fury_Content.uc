/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Fury_Content extends UTVehicle_Fury;

/** dynamic light which moves around following primary fire beam impact point */
var UTFuryBeamLight BeamLight;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	AnimPlayer = UTAnimNodeSequence(Mesh.FindAnimNode('AnimPlayer'));
	BlendNode = AnimNodeBlend(Mesh.FindAnimNode('LandBlend'));

	ArmBlendNodes[0] = UTAnimBlendByCollision( Mesh.FindAnimNode('UpRtBlend') );
	ArmBlendNodes[1] = UTAnimBlendByCollision( Mesh.FindAnimNode('UpLtBlend') );
	ArmBlendNodes[2] = UTAnimBlendByCollision( Mesh.FindAnimNode('LwRtBlend') );
	ArmBlendNodes[3] = UTAnimBlendByCollision( Mesh.FindAnimNode('LwLtBlend') );
}

simulated event PlayLanding()
{
 	Mesh.bForceDiscardRootMotion = false;
	AnimPlayer.PlayAnimation('Land',1.0,false);
	BlendNode.SetBlendTarget(1.0, 0.25);

	TargetTranslationTime = TakeOffRate;
}

simulated event PlayTakeOff()
{
	AnimPlayer.PlayAnimation('TakeOff',1.0,false);
	BlendNode.SetBlendTarget(0.0,TakeOffRate);
	TargetTranslationTime = TakeOffRate;
}

event OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
{
	Super.OnAnimEnd(SeqNode, PlayedTime, ExcessTime);
	if (SeqNode.AnimSeqName == 'TakeOff')
	{
		Mesh.bForceDiscardRootMotion = true;
	}
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (BeamLight != None)
	{
		BeamLight.Destroy();
	}
}

simulated function KillBeamEmitter()
{
	Super.KillBeamEmitter();
	if (BeamLight != None)
	{
		BeamLight.Destroy();
	}
}

simulated function SetBeamEmitterHidden(bool bHide)
{
	Super.SetBeamEmitterHidden(bHide);

	if (bHide && BeamLight != None)
	{
		BeamLight.AmbientSound.Stop();
		BeamLight.Destroy();
	}
}

simulated function VehicleWeaponFired( bool bViaReplication, vector HitLocation, int SeatIndex )
{
	Super.VehicleWeaponFired(bViaReplication,HitLocation,SeatIndex);

	if (WorldInfo.NetMode != NM_DedicatedServer && !IsZero(HitLocation))
	{
		if (BeamLight == None || BeamLight.bDeleteMe)
		{
			BeamLight = spawn(class'UTFuryBeamLight');
			BeamLight.AmbientSound.Play();
		}
		BeamLight.SetLocation(HitLocation + vect(0,0,128));
	}
}

defaultproperties
{
	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Fury.Mesh.SK_VH_Fury'
		PhysicsAsset=PhysicsAsset'VH_Fury.Mesh.SK_VH_Fury_Physics'
		AnimSets(0)=AnimSet'VH_Fury.Anims.K_VH_Fury'
		AnimTreeTemplate=AnimTree'VH_Fury.Anims.AT_VH_Fury'
	End Object

	Seats(0)={(	GunClass=class'UTVWeap_FuryGun',
				GunSocket=(LeftCannonA,RightCannonA,LeftCannonB,RightCannonB,LeftCannonC,RightCannonC),
				TurretControls=(LeftTurretConstraint,RightTurretConstraint,LeftTentacleTurretConstraint,RightTentacleTurretConstraint),
				GunPivotPoints=(LeftCannon, RightCannon),
				CameraTag=None,
				CameraOffset=-584)}

	// Engine sound.
	Begin Object Class=AudioComponent Name=FuryEngineSound
		SoundCue=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_EngineLoopCue'
	End Object
	EngineSound=FuryEngineSound
	Components.Add(FuryEngineSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_StartCue'
	ExitVehicleSound=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_StopCue'

	// Collision sound.
	Begin Object Class=AudioComponent Name=FuryCollideSound
		SoundCue=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_Collide'
	End Object
	CollideSound=FuryCollideSound
	Components.Add(FuryCollideSound);

	// Scrape sound.
	Begin Object Class=AudioComponent Name=BaseScrapeSound
		SoundCue=SoundCue'A_Gameplay.A_Gameplay_Onslaught_MetalScrape01Cue'
	End Object
	ScrapeSound=BaseScrapeSound
	Components.Add(BaseScrapeSound);

	DrawScale=1.3

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	RagdollMesh=none
	RagdollAsset=none

	IconXStart=0.5
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	GroundEffectIndices.Empty()

	BeamTemplate=particlesystem'VH_Fury.Effects.P_VH_Fury_AltBeam'
	BeamSockets(0)=ArmSocket0
	BeamSockets(1)=ArmSocket1
	BeamSockets(2)=ArmSocket2
	BeamSockets(3)=ArmSocket3
	EndPointParamName=LinkBeamEnd

	Begin Object Class=AudioComponent name=BeamAmbientSoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
	End Object
	BeamAmbientSound=BeamAmbientSoundComponent
	Components.Add(BeamAmbientSoundComponent)

	BeamFireSound=SoundCue'A_Vehicle_Fury.Cue.A_Vehicle_Fury_AltBeamCue'

	SpawnInSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'

	JetSFX(0)=(ExhaustTag=MainThrusters)
	JetSFX(1)=(ExhaustTag=LeftThrusters)
	JetSFX(2)=(ExhaustTag=RightThrusters)

	VehicleEffects(0)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Fury.Effects.P_VH_Fury_Exhaust',EffectSocket=ExhaustSocket)

	BoosterNames(0)=MainThrustersBoost
	BoosterNames(1)=LeftThrustersBoost
	BoosterNames(2)=RightThrustersBoost

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	PawnHudScene=VHud_Fury'UI_Scenes_HUD.VehicleHUDs.VH_Fury'

	DrivingPhysicalMaterial=PhysicalMaterial'VH_Fury.materials.physmat_furydriving'
	DefaultPhysicalMaterial=PhysicalMaterial'VH_Fury.materials.physmat_fury'
}
