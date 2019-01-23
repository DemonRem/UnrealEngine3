/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTLeviathanShield extends UTPaladinShield;

simulated event BaseChange();

defaultproperties
{
	Begin Object Name=ShieldMesh
		Scale=0.75
	End Object

	Begin Object Name=ShieldEffect
		Template=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShieldEffect'
		Scale=0.75
		Translation=(x=15,Y=40,Z=25)
	End Object
}
