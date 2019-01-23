// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_Instagib extends UTMutator;

function InitMutator(string Options, out string ErrorMessage)
{
	local UTPickupFactory F;

	if ( UTGame(WorldInfo.Game) != None )
	{
		UTGame(WorldInfo.Game).DefaultInventory.Length = 0;
		UTGame(WorldInfo.Game).DefaultInventory[0] = class'UTGame.UTWeap_InstagibRifle';
		if ( UTTeamGame(WorldInfo.Game) != None )
		{
			UTTeamGame(WorldInfo.Game).TeammateBoost = 0.6;
		}
	}

	ForEach AllActors(class'UTPickupFactory', F)
	{
		F.DisablePickup();
	}
	super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GroupName="WEAPONMOD"
}