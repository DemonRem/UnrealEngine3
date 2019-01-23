/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_PlayerDoll extends HudWidget_PawnDoll;

var instanced UIImage HelmetArmorImg;
var instanced UIImage ChestArmorImg;
var instanced UIImage ThighArmorImg;
var instanced UIImage ShieldBeltImg;
var instanced UIImage JumpBootsImg;
var instanced UILabel JumpBootsChargeText;

event WidgetTick(FLOAT DeltaTime)
{
	local UTPawn PawnOwner;
	local bool bHasShield, bHasChest, bHasThigh, bHasHelmet;

	PawnOwner = UTPawn( UTHudSceneOwner.GetPawnOwner() );
	if ( PawnOwner != none && PawnOwner.InvManager != none )
	{
		bHasShield = PawnOwner.ShieldBeltArmor > 0.0f;
		bHasChest = PawnOwner.VestArmor > 0.0f;
		bHasThigh = PawnOwner.ThighpadArmor > 0.0f;
		bHasHelmet = PawnOwner.HelmetArmor > 0.0f;

		ShieldBeltImg.SetVisibility(bHasShield);
		ChestArmorImg.SetVisibility(bHasChest);
		ThighArmorImg.SetVisibility(bHasThigh);
		HelmetArmorImg.SetVisibility(bHasHelmet);
	}
}

defaultproperties
{

	// Helmet Armor Image

	Begin Object Class=UIImage Name=iHelmetArmorImg
		WidgetTag=HelmetArmorImg
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	HelmetArmorImg=iHelmetArmorImg


	// Chest Armor Image

	Begin Object Class=UIImage Name=iChestArmorImg
		WidgetTag=ChestArmorImg
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	ChestArmorImg=iChestArmorImg

	// Chest Armor Image

	Begin Object Class=UIImage Name=iThighArmorImg
		WidgetTag=ThighArmorImg
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	ThighArmorImg=iThighArmorImg

	// Chest Armor Image

	Begin Object Class=UIImage Name=iShieldBeltImg
		WidgetTag=ShieldBeltImg
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	ShieldBeltImg=iShieldBeltImg

	// Chest Armor Image

	Begin Object Class=UIImage Name=iJumpBootsImg
		WidgetTag=JumpBootsImg
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	JumpBootsImg=iJumpBootsImg

	Begin Object Class=UILabel Name=lblJumpBootsChargeText
		DataSource=(MarkupString="0",RequiredFieldType=DATATYPE_Property)
		WidgetTag=JumpBootsChargeText

		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	JumpBootsChargeText=lblJumpBootsChargeText


	bRequiresTick=true
}
