/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Turret extends UTVehicle_TrackTurretBase
	dependson(UTSkelControl_Turretconstrained);

/** The Template of the Beam to use */

var particlesystem BeamTemplate;

var color EffectColor;
var bool bFireRight;


simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'FlashLocation' && !IsZero(FlashLocation))
	{
		bFireRight = !bFireRight;
	}

	super.ReplicatedEvent(VarName);
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local UTEmitter Beam;
	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	// Handle Beam Effects for the shock beam

	if (!IsZero(HitLocation))
	{
		Beam = Spawn( class'UTEmitter',self,,GetEffectLocation(SeatIndex) );
		if (Beam != none)
		{
			Beam.SetTemplate(BeamTemplate);
			Beam.ParticleSystemComponent.SetVectorParameter('ShockBeamEnd', HitLocation );
		}
	}
}

function SetFlashLocation( Weapon Who, byte FireModeNum, vector NewLoc )
{
	Super.SetFlashLocation(Who,FireModeNum,NewLoc);
	bFireRight = !bFireRight;
}


simulated function SetVehicleEffectParms(name TriggerName, ParticleSystemComponent PSC)
{
	if (TriggerName == 'FireRight' || TriggerName == 'FireLeft')
	{
		PSC.SetColorParameter('MFlashColor',EffectColor);
	}
	else
	{
		Super.SetVehicleEffectParms(TriggerName, PSC);
	}
}


/**
 * We override GetCameraStart for the Turret so that it just uses the Socket Location
 */
simulated function vector GetCameraStart(int SeatIndex)
{
	local vector CamStart;

	if (Mesh.GetSocketWorldLocationAndRotation(Seats[SeatIndex].CameraTag, CamStart) )
	{
		return CamStart + (Seats[SeatIndex].CameraBaseOffset >> Rotation);
	}
	else
	{
		return Super.GetCameraStart(SeatIndex);
	}
}

defaultproperties
{
	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Turret.Mesh.SK_VH_Turret'
		AnimTreeTemplate=AnimTree'VH_Turret.Anims.AT_VH_Turret'
		PhysicsAsset=PhysicsAsset'VH_Turret.Mesh.SK_VH_Turret_Physics'
		AnimSets.Add(AnimSet'VH_Turret.Anims.VH_Turret_anims')
		Translation=(Z=-68.0)
	End Object

	Begin Object Name=CollisionCylinder
		CollisionHeight=80.0
		CollisionRadius=100.0
	End Object

	// turret sounds.
	Begin Object Class=AudioComponent Name=TurretTwistSound
		SoundCue=SoundCue'A_Vehicle_Turret.Cue.A_Turret_Rotate'
	End Object
	Components.Add(TurretTwistSound);

	Begin Object Class=AudioComponent Name=ACTurretMoveStart
		SoundCue=SoundCue'A_Vehicle_Turret.Cue.A_Turret_TrackStart01Cue'
	End Object
	Components.Add(ACTurretMoveStart);
	TurretMoveStart=ACTurretMoveStart

	Begin Object Class=AudioComponent Name=ACTurretMoveLoop
		SoundCue=SoundCue'A_Vehicle_Turret.Cue.A_Turret_TrackLoop01Cue'
	End Object
	Components.Add(ACTurretMoveLoop);
	TurretMoveLoop=ACTurretMoveLoop

	Begin Object Class=AudioComponent Name=ACTurretMoveStop
		SoundCue=SoundCue'A_Vehicle_Turret.Cue.A_Turret_TrackStop01Cue'
	End Object
	Components.Add(ACTurretMoveStop);
	TurretMoveStop=ACTurretMoveStop

	Begin Object Class=AudioComponent Name=PowerSound
		SoundCue=SoundCue'A_Vehicle_Turret.Cue.AxonTurret_PowerLoopCue'
	End Object
	components.add(PowerSound);
	EngineSound = PowerSound;

	// Collision sound.
	Begin Object Class=AudioComponent Name=MantaCollideSound
		SoundCue=SoundCue'A_Vehicle_Manta.SoundCues.A_Vehicle_Manta_Collide'
	End Object
	CollideSound=MantaCollideSound
	Components.Add(MantaCollideSound);


	Seats(0)={(	GunClass=class'UTVWeap_TurretPrimary',
				GunSocket=(LU_Barrel,RU_Barrel,LL_Barrel,RL_Barrel),
				TurretVarPrefix="",
				TurretControls=(MegaTurret,TurretBase),
				CameraTag=CameraViewSocket,
				bSeatVisible=true,
				GunPivotPoints=(Seat),
				SeatMotionAudio=TurretTwistSound,
				CameraEyeHeight=5,
				SeatBone=Seat,
				SeatOffset=(X=43,Z=-7),
				CameraOffset=-120)}

	VehicleEffects(0)=(EffectStartTag=FireUL,EffectTemplate=ParticleSystem'VH_Turret.Effects.P_VH_Turret_MuzzleFlash',EffectSocket=LU_Barrel)
	VehicleEffects(1)=(EffectStartTag=FireUR,EffectTemplate=ParticleSystem'VH_Turret.Effects.P_VH_Turret_MuzzleFlash',EffectSocket=RU_Barrel)
	VehicleEffects(2)=(EffectStartTag=FireLL,EffectTemplate=ParticleSystem'VH_Turret.Effects.P_VH_Turret_MuzzleFlash',EffectSocket=LL_Barrel)
	VehicleEffects(3)=(EffectStartTag=FireLR,EffectTemplate=ParticleSystem'VH_Turret.Effects.P_VH_Turret_MuzzleFlash',EffectSocket=RL_Barrel)

	VehicleAnims(0)=(AnimTag=FireUL,AnimSeqs=(Shoot1),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=LU_Player)
	VehicleAnims(1)=(AnimTag=FireUR,AnimSeqs=(Shoot2),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=RU_Player)
	VehicleAnims(2)=(AnimTag=FireLL,AnimSeqs=(Shoot3),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=LL_Player)
	VehicleAnims(3)=(AnimTag=FireLR,AnimSeqs=(Shoot4),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=RL_Player)

	VehicleAnims(4)=(AnimTag=EngineStart,AnimSeqs=(GetIn),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=TurretPlayer)
	VehicleAnims(5)=(AnimTag=EngineStop,AnimSeqs=(GetOut),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=TurretPlayer)


 	EffectColor=(R=35,G=26,B=151,A=255)
	BeamTemplate=VH_Turret.Effects.P_VH_Turret_TurretBeam

	FlagOffset=(X=-25.0,Y=0.0,Z=-25.0)
	FlagBone=Seat


}
