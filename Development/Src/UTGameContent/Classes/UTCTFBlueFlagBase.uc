/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFBlueFlagBase extends UTCTFBase
	placeable;

defaultproperties
{
	FlagType=class'UTGameContent.UTCTFBlueFlag'
    DefenderTeamIndex=1

	Begin Object Class=ParticleSystemComponent Name=EmptyParticles
		Template=ParticleSystem'Pickups.flag.effects.P_Flagbase_Empty_Idle_Blue'
		bAutoActivate=false
	End Object
	FlagEmptyParticles=EmptyParticles
	Components.Add(EmptyParticles)

	FlagBaseMaterial=MaterialInstanceConstant'Pickups.Base_Flag.Materials.M_Pickups_Base_Flag_Blue'
}
