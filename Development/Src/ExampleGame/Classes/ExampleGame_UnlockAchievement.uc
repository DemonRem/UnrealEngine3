/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** Used to allow level designers to issue achievements */
class ExampleGame_UnlockAchievement extends UIAction
	dependson(ExamplePlayerController);

/** The achievement to unlock */
var() EExampleGameAchievements AchievementId;

defaultproperties
{
	ObjName="Unlock Achievement"
	ObjCategory="Misc"
}
