/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_PlayerHealth extends HudWidget_SingleValue;

var transient int OldHealth;
var() LinearColor	HealthFlashColor;

function bool GetText(out string ValueStr, float DeltaTime)
{
	local bool bResult;
	local Pawn P;
	local Vehicle V;
	local int Health;

	P = UTHudSceneOwner.GetPawnOwner();
	if ( P != none )
	{

		V = Vehicle(P);
		if (V != None)
		{
			Health = (V.Driver != None) ? Max(0, V.Driver.Health) : 0;
		}

		else
		{
			Health = Max(0, P.Health);
		}
	}
	else
	{
		Health = 0;
	}

	bResult = (OldHealth != Health);

	if ( Health > OldHealth )
	{
		Flash(0.6, HealthFlashColor);
	}

	OldHealth = Health;
	ValueStr = String(Health);
	return bResult;
}


defaultproperties
{
	WidgetTag=PlayerHealthWidget
	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false
	HealthFlashColor=(R=0.25,G=1.0,B=0.25,A=1.0)
}
