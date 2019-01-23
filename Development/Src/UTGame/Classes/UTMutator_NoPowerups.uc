// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_NoPowerups extends UTMutator;

function InitMutator(string Options, out string ErrorMessage)
{
	local UTPickupFactory F;

	ForEach AllActors(class'UTPickupFactory', F)
	{
		if ( F.bIsSuperItem || F.IsA('UTPickupFactory_JumpBoots') )
		{
			F.DisablePickup();
		}
	}
	super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GroupName="WEAPONMOD"
}