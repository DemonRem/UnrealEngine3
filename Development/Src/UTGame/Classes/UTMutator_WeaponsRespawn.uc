// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_WeaponsRespawn extends UTMutator;

function InitMutator(string Options, out string ErrorMessage)
{
	UTGame(WorldInfo.Game).bWeaponStay = false;
	super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GroupName="WEAPONSPAWN"
}