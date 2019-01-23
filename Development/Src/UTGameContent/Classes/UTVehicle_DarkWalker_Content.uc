/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_DarkWalker_Content extends UTVehicle_DarkWalker;

/** dynamic light which moves around following primary fire beam impact point */
var UTDarkWalkerBeamLight BeamLight;

var float HornImpulseMag;
var float VehicleHornModifier;
var ParticleSystemComponent DarkwalkerHornEffect;
var repnotify bool bSpeakerReady;
var() float SpeakerRadius;
var SoundCue HornAttackSound;

replication
{
	if (!bNetOwner)
		bSpeakerReady;
}

event MantaDuckEffect()
{
	if (bHoldingDuck)
	{
		VehicleEvent('CrushStart');
	}
	else
	{
		VehicleEvent('CrushStop');
	}
}

simulated function DriverLeft()
{
	Super.DriverLeft();

	if (Role == ROLE_Authority)
	{
		if (UTVWeap_DarkWalkerTurret(Seats[0].Gun) != none)
		{
			UTVWeap_DarkWalkerTurret(Seats[0].Gun).StopBeamFiring();
		}
	}
}

/** Overloaded so we can attach the muzzle flash light to a custom socket */
simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	Super.CauseMuzzleFlashLight(SeatIndex);

	if ( (SeatIndex == 0) && Seats[SeatIndex].MuzzleFlashLight != none )
	{
		Mesh.DetachComponent(Seats[SeatIndex].MuzzleFlashLight);
		Mesh.AttachComponentToSocket(Seats[SeatIndex].MuzzleFlashLight, 'PrimaryMuzzleFlash');
	}
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local vector HitNormal, NewHitLoc, ApproxHitDir, StartTrace, EndTrace;
	local TraceHitInfo HitInfo;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	if ( SeatIndex == 0 )
	{
		if (BeamLight == None || BeamLight.bDeleteMe)
		{
			BeamLight = Spawn(class'UTDarkWalkerBeamLight');
			BeamLight.AmbientSound.Play();
		}

		// @fixme: the super of this function also does traces via findweaponhitnormal
		// to get the same information, which means a redundant trace call.  should prolly
		// factor the overloaded call pass along relevant info, and have this information-
		// gathering step happen only once beforehand.
		ApproxHitDir = Normal(HitLocation - Location);
		StartTrace = HitLocation - ApproxHitDir * 32.f;
		EndTrace = HitLocation + ApproxHitDir * 32.f;
		FindWeaponHitNormal(NewHitLoc, HitNormal, EndTrace, StartTrace, HitInfo);

		BeamLight.SetLocation(HitLocation + HitNormal*128);
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

simulated event Destroyed()
{
	Super.Destroyed();

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

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		if (bSpeakerReady)
		{
			PlayHornAttack();
		}
		return true;
	}

	return false;
}

simulated function PlayHornAttack()
{
	local Pawn HitPawn;
	local vector HornImpulse, HitLocation, HitNormal;
	local Pawn BoardPawn;

	bSpeakerReady = false;

	if (Trace(HitLocation, HitNormal, Location - vect(0,0,600), Location) != None)
	{
		DarkwalkerHornEffect.SetTranslation(HitLocation - Location);
	}
	else
	{
		HitLocation = Location;
		HitLocation.Z -= 400;
		DarkwalkerHornEffect.SetTranslation(vect(0,0,-400));
	}
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		PlaySound(HornAttackSound, true);
		DarkwalkerHornEffect.ActivateSystem();
	}

	if (Role == ROLE_Authority)
	{
		MakeNoise(1.0);

		foreach OverlappingActors(class 'Pawn', HitPawn, SpeakerRadius,HitLocation)
		{
			if ( (HitPawn.Mesh != None) && !WorldInfo.GRI.OnSameTeam(HitPawn, self))
			{
				// throw him outwards also
				HornImpulse = HitPawn.Location - HitLocation;
				HornImpulse.Z = 0;
				HornImpulse = HornImpulseMag * Normal(HornImpulse);
				HornImpulse.Z = 250.0;

				if (UTPawn(HitPawn) != None)
				{
					UTPawn(HitPawn).ForceRagdoll();
					UTPawn(HitPawn).FeignDeathStartTime = WorldInfo.TimeSeconds + 1.5;
				}
				else if( UTVehicle_Hoverboard(HitPawn) != none)
				{
					BoardPawn = UTVehicle_Hoverboard(HitPawn).Driver; // just in case the board gets nuked from the ragdoll
					UTVehicle_Hoverboard(HitPawn).RagdollDriver();
					HitPawn = BoardPawn;
				}
				if ( HitPawn.Physics == PHYS_RigidBody )
				{
					if(UTVehicle(HitPawn) != none)
					{
						HitPawn.Mesh.AddImpulse(HornImpulse*VehicleHornModifier, HitLocation);
					}
					else
					{
						HitPawn.Mesh.AddImpulse(HornImpulse, HitLocation);
					}
				}
			}
		}
	}
	SetTimer(4.0f, false, 'ClearHornTimer');
}

simulated function ClearHornTimer()
{
	bSpeakerReady = true;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bSpeakerReady')
	{
		if (!bSpeakerReady)
		{
			PlayHornAttack();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=140.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_DarkWalker.Mesh.SK_VH_DarkWalker_Torso'
		AnimSets(0)=AnimSet'VH_DarkWalker.Anims.K_VH_DarkWalker'
		AnimTreeTemplate=AnimTree'VH_DarkWalker.Anims.AT_VH_DarkWalker'
		PhysicsAsset=PhysicsAsset'VH_DarkWalker.Mesh.SK_VH_DarkWalker_Torso_Physics'
	End Object

	RagdollMesh=SkeletalMesh'VH_DarkWalker.Mesh.SK_VH_DarkWalkerDestroyed'
	RagdollAsset=PhysicsAsset'VH_DarkWalker.Mesh.SK_VH_DarkWalkerDestroyed_Physics'

	// turret twist sound.
	Begin Object Class=AudioComponent Name=TurretTwistSound
		SoundCue=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_TurretRotateCue'
	End Object
	Components.Add(TurretTwistSound);

	CameraScaleMax=1.6

	Seats(0)={( GunClass=class'UTVWeap_DarkWalkerTurret',
				GunSocket=(MainGun_Fire),
				GunPivotPoints=(PrimaryTurretPitch),
				TurretVarPrefix="",
				CameraTag=DriverViewSocket,
				CameraOffset=-720,
				DriverDamageMult=0.0,
				TurretControls=(MainGunPitch),
				MuzzleFlashLightClass=class'UTDarkWalkerMuzzleFlashLight')}


	Seats(1)={( GunClass=class'UTVWeap_DarkWalkerPassGun',
				GunSocket=(TurretBarrel_00,TurretBarrel_01,TurretBarrel_02,TurretBarrel_03),
				GunPivotPoints=(SecondaryTurretYaw),
				TurretVarPrefix="Turret",
				TurretControls=(TurretYaw,TurretPitch),
				DriverDamageMult=0.0,
				CameraOffset=0,
				CameraTag=TurretViewSocket,
				DriverDamageMult=0.0,
				bSeatVisible=true,
				SeatBone=SecondaryTurretYaw,
				SeatOffset=(X=162,Y=0,Z=-81)
				)}

	// These muzzleflashes are the idle effects it seems, so start them with the engine.
	VehicleEffects(0)=(EffectStartTag=EngineStart,EffectEndTag=STOP_MainTurretFire,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_MuzzleFlash',EffectSocket=MainGun_00)
	VehicleEffects(1)=(EffectStartTag=EngineStart,EffectEndTag=STOP_MainTurretFire,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_MuzzleFlash',EffectSocket=MainGun_01)

	VehicleEffects(2)=(EffectStartTag=TurretWeapon03,EffectEndTag=STOP_TurretWeapon00,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Secondary_MuzzleFlash',EffectSocket=TurretBarrel_00)
	VehicleEffects(3)=(EffectStartTag=TurretWeapon00,EffectEndTag=STOP_TurretWeapon01,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Secondary_MuzzleFlash',EffectSocket=TurretBarrel_01)
	VehicleEffects(4)=(EffectStartTag=TurretWeapon01,EffectEndTag=STOP_TurretWeapon02,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Secondary_MuzzleFlash',EffectSocket=TurretBarrel_02)
	VehicleEffects(5)=(EffectStartTag=TurretWeapon02,EffectEndTag=STOP_TurretWeapon03,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_Secondary_MuzzleFlash',EffectSocket=TurretBarrel_03)

	VehicleEffects(6)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_AimBeam',EffectSocket=LT_AimBeamSocket)
	VehicleEffects(7)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_AimBeam',EffectSocket=RT_AimBeamSocket)
	VehicleEffects(8)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_PowerBall',EffectSocket=PowerBallSocket)
	VehicleEffects(9)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_DarkWalker',EffectSocket=DamageSmoke01)

	Begin Object Class=ParticleSystemComponent Name=HornEffect
		Template=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_HornEffect'
		bAutoActivate=false
		Translation=(x=0.0,y=0.0,z=-400.0)
	End Object
	Components.Add(HornEffect);
	DarkwalkerHornEffect=HornEffect;

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=MantaEngineSound
		SoundCue=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_EngineLoopCue'
	End Object
	EngineSound=MantaEngineSound
	Components.Add(MantaEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=MantaCollideSound
		SoundCue=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_CollideCue'
	End Object
	CollideSound=MantaCollideSound
	Components.Add(MantaCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_StartCue'
	ExitVehicleSound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_StopCue'

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

	BodyAttachSocketName=PowerBallSocket

	BeamTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_MainGun_Beam'
	BeamSockets(0)=MainGun_00
	BeamSockets(1)=MainGun_01
	EndPointParamName=LinkBeamEnd

	Begin Object Class=AudioComponent name=BeamAmbientSoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
	End Object
	BeamAmbientSound=BeamAmbientSoundComponent
	Components.Add(BeamAmbientSoundComponent)

	BeamFireSound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FireBeamCue'
	FlagBone=Head

	HornAttackSound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_HornCue'
	SpawnInSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'
	ExplosionSound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_ExplosionCue'

	PawnHudScene=VHud_DarkWalker'UI_Scenes_HUD.VehicleHUDs.VH_DarkWalker'

	BigExplosionTemplate=ParticleSystem'VH_Nemesis.Effects.P_VH_Nemesis_Explosion'
	BodyType=class'UTWalkerBody_DarkWalker'

	bSpeakerReady=true
	SpeakerRadius=750.0f
	HornImpulseMag=1250.0
	VehicleHornModifier=5.3f
	PassengerTeamBeaconOffset=(X=-150.0f,Y=0.0f,Z=0.0f);	
}
