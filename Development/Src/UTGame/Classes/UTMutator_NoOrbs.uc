// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_NoOrbs extends UTMutator;

function InitMutator(string Options, out string ErrorMessage)
{
	local UTOnslaughtFlagBase F;

	ForEach AllActors(class'UTOnslaughtFlagBase', F)
	{
		F.DisableOrbs();
	}
	super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GroupName="WARFARE"
}