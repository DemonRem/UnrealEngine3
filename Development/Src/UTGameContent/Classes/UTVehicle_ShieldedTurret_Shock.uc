/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_ShieldedTurret_Shock extends UTVehicle_ShieldedTurret;

defaultproperties
{
	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Balls'
		AnimTreeTemplate=AnimTree'VH_TurretSmall.Anims.AT_VH_TurretSmall_Balls'
		PhysicsAsset=PhysicsAsset'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Balls_Physics'
		Translation=(Z=-40)
	End Object

	Seats(0)={(	GunClass=class'UTVWeap_LeviathanTurretShock',
				GunSocket=(Gun),
				TurretVarPrefix="",
				TurretControls=(TurretBase),
				CameraTag=CameraViewSocket,
				bSeatVisible=true,
				GunPivotPoints=(SmallTurretBeam-arm),
				CameraEyeHeight=5,
				SeatBone=SmallTurretBeam-arm,
				SeatOffset=(X=18,Z=25),
				CameraOffset=-120,
				DriverDamageMult=0.4
		)}

	VehicleEffects(0)=(EffectStartTag=TurretShockMF,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretShockMF',EffectSocket=Gun)

	ShieldBone=SmallTurretBeam-arm
	FlagOffset=(X=-60.0,Y=0.0,Z=25.0)
	FlagBone=SmallTurretBeam-main
}
