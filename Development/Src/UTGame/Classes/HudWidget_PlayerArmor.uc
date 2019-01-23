/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_PlayerArmor extends HudWidget_SingleValue;

var transient int OldArmor;
var() LinearColor ArmorFlashColor;

function bool GetText(out string ValueStr, float DeltaTime)
{
	local bool bResult;
	local Pawn P;
	local UTPawn UTP;
	local int Armor;

	Armor = 0;

	P = UTHudSceneOwner.GetPawnOwner();
	if (P != None)
	{

		if (Vehicle(P) != none )

		{
			UTP = UTPawn( Vehicle(P).Driver );
		}
		else
		{
			UTP = UTPawn(P);
		}
		if (UTP != None)
		{
			Armor = UTP.GetShieldStrength();
		}
	}

	bResult = (OldArmor != Armor);

	if ( Armor > OldArmor )
	{
		Flash(0.6, ArmorFlashColor);
	}

	OldArmor = Armor;
	ValueStr = String(Armor);
	return bResult;
}


defaultproperties
{
	WidgetTag=PlayerArmor
	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false
	ArmorFlashColor=(R=1.0,G=0.208360,B=0.000638,A=1.0)
 }
