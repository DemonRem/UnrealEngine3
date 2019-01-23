/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_Pawns extends UTUIScene_Hud
	abstract
	native(UI);

var transient bool bForceInitialAnimation;
var transient bool bDeactivating;

/**
 * Called when a scene has finished it's transitions.  This is typically called when
 * whatever animations have occured are completed.
 */

delegate OnSceneTransitionFinished(UTUIScene_Hud SourceScene, bool bOut);

/**
 * This function is called directly after becomes active.  It should being/handle any activation
 * animations/etc and then call OnSceneTransitionFinished.
 *
 * @Param 	NotificationDelegate	The function to call when the transition is finished
 */
function ActivatePawnsScene( delegate<OnSceneTransitionFinished> NotificationDelegate)
{
	OnSceneTransitionFinished = NotificationDelegate;
	bDeactivating = false;
	AnimateScene('Transition','Activate');
}

/**
 * This function is called when you wish to close a scene a scene.  It should being/handle any deactivation
 * animations/etc and then call OnSceneTransitionFinished.  NOTE it should not close the scene.
 *
 * @Param 	NotificationDelegate	The function to call when the transition is finished
 */
function DeactivatePawnsScene( delegate<OnSceneTransitionFinished> NotificationDelegate)
{
	OnSceneTransitionFinished = NotificationDelegate;
	bDeactivating = true;
	AnimateScene('Transition','Deactivate');
}

function AnimateScene(name AnimName, name SeqName)
{
	local array<UIObject> Kids;
	local UTUI_HudWidget Kid;
	local int i;
	local bool bIsAnimating;

	Kids = GetChildren(true);
	for (i=0;i<Kids.Length;i++)
	{
		Kid = UTUI_HudWidget(Kids[i]);
		if ( Kid != none && Kid.Animations.Length>0 )
		{
			if ( Kid.PlayAnimation(AnimName, SeqName,bForceInitialAnimation) )
			{
				bIsAnimating = true;
			}

		}
	}
	bForceInitialAnimation = false;
	if (!bIsAnimating)
	{
		OnSceneTransitionFinished(self, bDeactivating);
	}
}

event OnAnimationFinished(UIObject AnimTarget, name AnimName, name SeqName)
{
	OnSceneTransitionFinished(self, bDeactivating);
}


defaultproperties
{
	bForceInitialAnimation=true
}
