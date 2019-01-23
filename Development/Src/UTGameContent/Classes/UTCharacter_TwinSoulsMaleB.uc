/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCharacter_TwinSoulsMaleB extends UTCharacter_TwinSoulsMaleA;

defaultproperties
{
	Begin Object Name=WPawnSkeletalMeshComponent
		SkeletalMesh=SkeletalMesh'CH_RTeam.Mesh.SK_CH_TwinSouls_MaleB'
	End Object

	BodySkins(Skin_RedTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody02_VRed'
	BodySkins(Skin_BlueTeam)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody02_VBlue'
	BodySkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MBody02_V01'

	HeadSkins(Skin_RedTeam)=None
	HeadSkins(Skin_BlueTeam)=None
	HeadSkins(Skin_Normal)=MaterialInstance'CH_All_Axon.Materials.MI_CH_RTeam_MHead02_V01'
}
