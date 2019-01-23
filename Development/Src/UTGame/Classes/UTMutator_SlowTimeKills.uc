// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class UTMutator_SlowTimeKills extends UTMutator;

var class<GameRules> GRClass;

function bool MutatorIsAllowed()
{
	return (WorldInfo.NetMode == NM_Standalone);
}

function InitMutator(string Options, out string ErrorMessage)
{
	WorldInfo.Game.AddGameRules(GRClass);

	Super.InitMutator(Options, ErrorMessage);
}

defaultproperties
{
	GRClass=class'UTGame.UTGameRules_SlowTimeKills'
	GroupName="GAMESPEED"
}