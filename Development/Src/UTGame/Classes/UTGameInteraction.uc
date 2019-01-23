/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTGameInteraction extends UIInteraction;

/**
 * Merge 2 existing scenes together in to one
 *
 *@param	SourceScene	The Scene to merge
 *@param	SceneTarget		This optional param is the scene to merge in to.  If it's none, the currently active scene will
 *							be used.
 */
function bool MergeScene(UIScene SourceScene, optional UIScene SceneTarget)
{
	local UIScene TempScene;
	local array<UIObject> MergeChildren;
	local int i;
	local bool bResults;

	bResults = false;
	if ( SourceScene != none )
	{
		// Attempt to resolve the active scene

		if ( SceneTarget == none )
		{
			SceneTarget = SceneClient.GetActiveScene();
		}

		if ( SceneTarget != none )
		{
			if ( SceneClient.OpenScene(SourceScene,,TempScene) && TempScene != none )
			{
				// Get a list of the root children in the scene

				MergeChildren = TempScene.GetChildren(false);

				// Remove them from the temp scene and insert them in to the target scene

				for (i=0;i<MergeChildren.Length;i++)
				{
					TempScene.RemoveChild(MergeChildren[i]);
					SceneTarget.InsertChild(MergeChildren[i],,false);
				}

				// Close out the temp scene

				CloseScene(TempScene);
				bResults = true;
			}
		}
	}
	else
	{
		`log("Error: Attempting to Merge a null scene in to"@SceneTarget);
	}
	return bResults;
}




defaultproperties
{
	SceneClientClass=class'UTGameUISceneClient'
}
