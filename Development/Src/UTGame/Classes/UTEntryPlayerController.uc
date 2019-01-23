/**
 * UTEntryPlayerController
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTEntryPlayerController extends UTPlayerController
	config(Game)
	native;

event InitInputSystem()
{
	// Need to bypass the UTPlayerController since it initializes voice and we do not want to do that in the menus.
	Super(GamePlayerController).InitInputSystem();

	AddOnlineDelegates(false);

	// we do this here so that we only bother to create it for local players
	CameraAnimPlayer = new(self) class'CameraAnimInst';
}

function LoadCharacterFromProfile(UTProfileSettings Profile);	// Do nothing
function SetPawnConstructionScene(bool bShow);	// Do nothing
