/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_ShieldedTurret_Beam extends UTVehicle_ShieldedTurret;

/** beam effect variables */
var ParticleSystem BeamTemplate;
var name BeamEndpointVarName;

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
			Beam.ParticleSystemComponent.SetVectorParameter(BeamEndpointVarName, HitLocation );
		}
	}
}

defaultproperties
{
	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Beam'
		AnimTreeTemplate=AnimTree'VH_TurretSmall.Anims.AT_VH_TurretSmall_Beam'
		PhysicsAsset=PhysicsAsset'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Beam_Physics'
		Translation=(Z=-40)
	End Object

	Seats(0)={(	GunClass=class'UTVWeap_LeviathanTurretBeam',
				GunSocket=(GunBarrel2,GunBarrel1),
				TurretVarPrefix="",
				TurretControls=(TurretBase),
				CameraTag=CameraViewSocket,
				bSeatVisible=true,
				GunPivotPoints=(SmallTurretBalls-arm),
				CameraEyeHeight=5,
				SeatBone=SmallTurretBalls-arm,
				SeatOffset=(X=18,Z=25),
				CameraOffset=-120,
				DriverDamageMult=0.4
		)}

	VehicleEffects(0)=(EffectStartTag=TurretBeamMF_L,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretBeamMF',EffectSocket=GunBarrel2)
	VehicleEffects(1)=(EffectStartTag=TurretBeamMF_R,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretBeamMF',EffectSocket=GunBarrel1)

	BeamTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_LaserBeam'
	BeamEndpointVarName=ShockBeamEnd

	ShieldBone=SmallTurretBalls-arm
	FlagOffset=(X=-60.0,Y=0.0,Z=25.0)
	FlagBone=SmallTurretBeam-main
}
