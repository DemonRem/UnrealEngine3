/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTStationaryTurretShield extends UTLeviathanShield;

defaultproperties
{
	Begin Object Name=ShieldMesh
		Scale=0.75
		Translation=(X=100.0,Y=0.0,Z=40.0)
		Scale3D=(X=1.0,Y=0.7,Z=1.0)
	End Object

	Begin Object Name=ShieldEffect
		Template=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShieldEffect'
		Scale=0.75
		Translation=(X=5.0,Y=40.0,Z=60.0)
	End Object
}
