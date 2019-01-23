/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTFamilyInfo_Human extends UTFamilyInfo
	abstract;

defaultproperties
{
	HeadGib=(BoneName=b_Head,GibClass=class'UTGib_HumanHead',bHighDetailOnly=false)

	Gibs[0]=(BoneName=b_LeftForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
 	Gibs[1]=(BoneName=b_RightForeArm,GibClass=class'UTGib_HumanHand',bHighDetailOnly=true)
 	Gibs[2]=(BoneName=b_LeftLeg,GibClass=class'UTGib_HumanBone',bHighDetailOnly=false)
 	Gibs[3]=(BoneName=b_RightLeg,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
 	Gibs[4]=(BoneName=b_Spine,GibClass=class'UTGib_HumanTorso',bHighDetailOnly=false)
 	Gibs[5]=(BoneName=b_Spine1,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
 	Gibs[6]=(BoneName=b_Spine2,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=true)
 	Gibs[7]=(BoneName=b_LeftClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[8]=(BoneName=b_RightClav,GibClass=class'UTGib_HumanArm',bHighDetailOnly=true)
 	Gibs[9]=(BoneName=b_LeftLegUpper,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
 	Gibs[10]=(BoneName=b_RightLegUpper,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
 }
