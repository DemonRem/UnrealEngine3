/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class ExampleGameInfo extends GameInfo;

auto State PendingMatch
{
Begin:
	StartMatch();
}

defaultproperties
{
	HUDType=class'GameFramework.MobileHUD'
	PlayerControllerClass=class'ExampleGame.ExamplePlayerController'
	DefaultPawnClass=class'ExampleGame.ExamplePawn'
	bDelayedStart=false
}


