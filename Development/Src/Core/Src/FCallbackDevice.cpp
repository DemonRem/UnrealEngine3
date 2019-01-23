/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Core includes.
#include "CorePrivate.h"

/**
 * Macro for wrapping implmentations of TScopedCallback'd ECallbackEventType
 */
#define IMPLEMENT_SCOPED_CALLBACK( CallbackName )						\
	void FScoped##CallbackName##Impl::FireCallback()					\
	{																	\
		GCallbackEvent->Send( CALLBACK_##CallbackName );				\
	}

IMPLEMENT_SCOPED_CALLBACK( None );

IMPLEMENT_SCOPED_CALLBACK( SelChange );
IMPLEMENT_SCOPED_CALLBACK( MapChange );
IMPLEMENT_SCOPED_CALLBACK( GroupChange );
IMPLEMENT_SCOPED_CALLBACK( WorldChange );
IMPLEMENT_SCOPED_CALLBACK( LevelDirtied );
IMPLEMENT_SCOPED_CALLBACK( SurfProps );
IMPLEMENT_SCOPED_CALLBACK( ActorProps );
IMPLEMENT_SCOPED_CALLBACK( ChangeEditorMode );
IMPLEMENT_SCOPED_CALLBACK( RedrawAllViewports );
IMPLEMENT_SCOPED_CALLBACK( ActorPropertiesChange );

IMPLEMENT_SCOPED_CALLBACK( RefreshEditor );

IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_AllBrowsers );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_LevelBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_GenericBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_GroupBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_ActorBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_TerrainBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_DynamicShadowStatsBrowser );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_SceneManager );
IMPLEMENT_SCOPED_CALLBACK( RefreshEditor_Kismet );

IMPLEMENT_SCOPED_CALLBACK( DisplayLoadErrors );
IMPLEMENT_SCOPED_CALLBACK( EditorModeEnter );
IMPLEMENT_SCOPED_CALLBACK( EditorModeExit );
IMPLEMENT_SCOPED_CALLBACK( UpdateUI );
IMPLEMENT_SCOPED_CALLBACK( SelectObject );
IMPLEMENT_SCOPED_CALLBACK( ShowDockableWindow );
IMPLEMENT_SCOPED_CALLBACK( Undo );
IMPLEMENT_SCOPED_CALLBACK( PreWindowsMessage );
IMPLEMENT_SCOPED_CALLBACK( PostWindowsMessage );
IMPLEMENT_SCOPED_CALLBACK( RedirectorFollowed );

IMPLEMENT_SCOPED_CALLBACK( ObjectPropertyChanged );
IMPLEMENT_SCOPED_CALLBACK( EndPIE );
IMPLEMENT_SCOPED_CALLBACK( RefreshPropertyWindows );

IMPLEMENT_SCOPED_CALLBACK( UIEditor_StyleModified );
IMPLEMENT_SCOPED_CALLBACK( UIEditor_ModifiedRenderOrder );
IMPLEMENT_SCOPED_CALLBACK( UIEditor_RefreshScenePositions );
