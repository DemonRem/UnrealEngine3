/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class DemoGame extends GameInfo;

auto State PendingMatch
{
Begin:
	StartMatch();
}

defaultproperties
{
	PlayerControllerClass=class'UTGame.DemoPlayerController'
	DefaultPawnClass=class'UTGame.DemoPawn'
}
