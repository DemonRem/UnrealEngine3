/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFRedFlagBase extends UTCTFBase
	placeable;


defaultproperties
{
	FlagType=class'UTGameContent.UTCTFRedFlag'
    DefenderTeamIndex=0

	Begin Object Class=ParticleSystemComponent Name=EmptyParticles
		Template=ParticleSystem'Pickups.flag.effects.P_Flagbase_Empty_Idle_Red'
		bAutoActivate=false
	End Object
	FlagEmptyParticles=EmptyParticles
	Components.Add(EmptyParticles)

	FlagBaseMaterial=MaterialInstanceConstant'Pickups.Base_Flag.Materials.M_Pickups_Base_Flag_Red'
}
