//=============================================================================
// Copyright © 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTEditor.h"
#include "UTGameUIClasses.h"

IMPLEMENT_CLASS(UUTEditorUISceneClient);

/*=========================================================================================
  UUTEditorUISceneClient - We have our own scene client so that we can provide Tick/Prerender
  passes.  We also use this object as a repository for all UTFontPool objects
  ========================================================================================= */

/**
 * We override the Render_Scene so that we can provide a PreRender pass
 * @See UGameUISceneClient for more details
 */
void UUTEditorUISceneClient::Render_Scene(FCanvas* Canvas, UUIScene *Scene)
{
	// UTUIScene's support a pre-render pass.  If this is a UTUIScene, start that pass and clock it

	UUTUIScene* UTScene = Cast<UUTUIScene>(Scene);
	if ( UTScene )
	{
		UTScene->PreRender(Canvas);
	}

	// Render the scene

	Super::Render_Scene(Canvas,Scene);
}
