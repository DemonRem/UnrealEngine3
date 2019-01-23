/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_DeployableGroup extends HudWidget_WeaponGroup
	native(UI);

cpptext
{
	/**
	 * We manage the weapon groups natively for speed.
	 */
	virtual void Tick_Widget(FLOAT DeltaTime);
}

defaultproperties
{
}
