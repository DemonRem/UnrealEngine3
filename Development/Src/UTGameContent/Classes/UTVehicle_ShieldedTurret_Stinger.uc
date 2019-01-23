/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_ShieldedTurret_Stinger extends UTVehicle_ShieldedTurret;

defaultproperties
{
	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Stinger'
		AnimTreeTemplate=AnimTree'VH_TurretSmall.Anims.AT_VH_TurretSmall_Stinger'
		PhysicsAsset=PhysicsAsset'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Stinger_Physics'
		Translation=(Z=-40)
	End Object

	Seats(0)={(	GunClass=class'UTVWeap_LeviathanTurretStinger',
				GunSocket=(Gun),
				TurretVarPrefix="",
				TurretControls=(TurretBase),
				CameraTag=CameraViewSocket,
				bSeatVisible=true,
				GunPivotPoints=(SmallTurretStinger-arm),
				CameraEyeHeight=5,
				SeatBone=SmallTurretStinger-arm,
				SeatOffset=(X=18,Z=25),
				CameraOffset=-120,
				DriverDamageMult=0.4
		)}

	VehicleEffects(0)=(EffectStartTag=TurretStingerMF,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretStingerMF',EffectSocket=Gun)

	ShieldBone=SmallTurretStinger-arm
	FlagOffset=(X=-60.0,Y=0.0,Z=25.0)
	FlagBone=SmallTurretStinger-main
}
