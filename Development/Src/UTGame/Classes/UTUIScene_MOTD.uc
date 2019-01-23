/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_MOTD extends UTUIScene_Hud
	native(UI);

var() float TimeRemainingOnScreen;

var transient UISafeRegionPanel Panel;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}


event PostInitialize()
{
	Super.PostInitialize();

	Panel = UISafeRegionPanel(FindChild('SafePanel',true));
	Panel.PlayUIAnimation('FadeIn',,0.5);
}

event Finish()
{
	Panel.PlayUIAnimation('FadeOut',,0.5);
}

function AnimEnd(UIObject AnimTarget, int AnimIndex, UIAnimationSeq AnimSeq)
{
	if (AnimSeq.SeqName == 'FadeOut')
	{
		SceneClient.CloseScene(Self);
	}
}

defaultproperties
{
	TimeRemainingOnScreen=6.0
}
