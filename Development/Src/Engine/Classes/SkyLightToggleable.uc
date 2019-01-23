/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SkyLightToggleable extends SkyLight
	placeable;

defaultproperties
{
	// Visual things should be ticked in parallel with physics
	TickGroup=TG_DuringAsyncWork

	bMovable=FALSE
	bStatic=FALSE
	bHardAttach=TRUE
}
