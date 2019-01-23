/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_WeaponAmmo extends UTUI_HudWidget
	dependson(UTWeapon);

var instanced UILabel ValueText;
var instanced UIProgressBar ValueBar;

var transient int OldAmmoCount;
var transient float OldPercValue;

var() LinearColor AmmoFlashColor;
var transient float FlashRate;
var transient LinearColor FlashColor;

event WidgetTick(float DeltaTime)
{
	local Pawn P;
	local UTWeapon Weap;
	local int AmmoCount;
	local float PercValue;
	local AmmoWidgetDisplayStyle DisplayStyle;

	DisplayStyle = EAWDS_Both;
	P = UTHudSceneOwner.GetPawnOwner();
	if ( P != none )
	{
		Weap = UTWeapon( P.Weapon );
		if ( Weap != none )
		{
			DisplayStyle = Weap.AmmoDisplayType;
			AmmoCount = Max(0,Weap.AmmoCount);
			PercValue = Weap.GetPowerPerc();
		}
		else
		{
			AmmoCount = 0;
			PercValue=1.0;
		}
	}
	else
	{
			AmmoCount = 0;
			PercValue=1.0;
	}

	if ( DisplayStyle != EAWDS_BarGraph )
	{
		if ( OldAmmoCount != AmmoCount )
		{
			ValueText.SetValue( String(AmmoCount) );
		}

		ValueText.SetVisibility(true);

		if ( AmmoCount > OldAmmoCount )
		{

			FlashRate=0.6;
			FlashColor= AmmoFlashColor;
		}

	}

	else
	{
		ValueText.SetVisibility(false);
	}

	if ( DisplayStyle != EAWDS_Numeric )
	{
		if ( OldPercValue != PercValue )
		{
			ValueBar.SetValue( PercValue, true );
		}
		ValueBar.SetVisibility(true);
	}
	else
	{
		ValueBar.SetVisibility(false);
	}

	OldAmmoCount = AmmoCount;
	OldPercValue = PercValue;


	if (FlashRate > 0.0f)
	{
		FlashColor.R += (1.0f - FlashColor.R) * (DeltaTime / FlashRate);
		FlashColor.G += (1.0f - FlashColor.G) * (DeltaTime / FlashRate);
		FlashColor.B += (1.0f - FlashColor.B) * (DeltaTime / FlashRate);
		FlashRate -= DeltaTime;

		if ( FlashRate < 0.0f )
		{
			FlashRate = 0.0f;
		}

		ValueText.StringRenderComponent.SetColor(FlashColor);
	}



}


defaultproperties
{
	Begin Object Class=UILabel Name=lblValueText
		DataSource=(MarkupString="000",RequiredFieldType=DATATYPE_Property)
		WidgetTag=ValueText

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
	ValueText=lblValueText

	Begin Object Class=UIProgressBar Name=pbValueBar

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
	ValueBar=pbValueBar

	WidgetTag=WeaponAmmoWidget

	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false

	bRequiresTick=true


	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageScene,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageScene,
				Value[UIFACE_Right]=0.119140625,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageScene,
				Value[UIFACE_Bottom]=0.069010416,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageScene)}

	AmmoFlashColor=(R=0.25,G=1.0,B=0.25,A=1.0)
}

