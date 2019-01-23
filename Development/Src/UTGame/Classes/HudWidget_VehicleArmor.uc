/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_VehicleArmor extends HudWidget_SingleValue;

var instanced UIProgressBar PowerPercBar;
var instanced UIProgressBar ArmorPercBar;
var transient int OldArmor;



function bool GetText(out string ValueStr, float DeltaTime)
{
	local bool bResult;
	local Pawn P;
	local int Armor;
	local UTVehicle V;
	local float ArmorPerc;

	local float PowerPerc;

	Armor = 0;
	ArmorPerc = 1.0;


	P = UTHudSceneOwner.GetPawnOwner();
	if (P != None)
	{
		if ( UTVehicleBase(P) != none )
		{
			if (UTVehicleBase(P).GetPowerLevel(PowerPerc))
			{
				PowerPercBar.SetVisibility(true);
				PowerPercBar.SetValue(100 * PowerPerc);
			}
			else
			{
				PowerPercBar.SetVisibility(false);
			}

			if (UTWeaponPawn(P) != none )
			{
				V = UTWeaponPawn(P).MyVehicle;
			}
			else
			{
				V = UTVehicle(P);
			}
		}

		if (V != None)
		{
			Armor = V.Health;
			ArmorPerc = float(Armor) / float(V.Default.Health);
		}
	}

	bResult = (OldArmor != Armor);
	OldArmor = Armor;
	ValueStr = String(Armor);

	if ( bResult )
	{
		ArmorPercBar.SetValue(ArmorPerc,true);
	}

	return bResult;
}


defaultproperties
{
	// Perc Bar

	Begin Object Class=UIProgressBar Name=pArmorPercBar
		WidgetTag=ArmorPercBar
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
	ArmorPercBar=pArmorPercBar

	// Power Bar

	Begin Object Class=UIProgressBar Name=pPowerPercBar
		WidgetTag=PowerPercBar
		Position={( Value[UIFACE_Left]=0.25,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=-0.5,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.7,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.5,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		bHidden=true
		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	PowerPercBar=pPowerPercBar

	WidgetTag=VehicleArmor
	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false
 }
