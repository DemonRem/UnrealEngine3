/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "LevelUtils.h"

/**
 * Assembles the set of all referenced worlds.
 *
 * @param	OutWorlds			[out] The set of referenced worlds.
 * @param	bIncludeGWorld		If TRUE, include GWorld in the output list.
 * @param	bOnlyEditorVisible	If TRUE, only sub-levels that should be visible in-editor are included
 */
void FLevelUtils::GetWorlds(TArray<UWorld*>& OutWorlds, UBOOL bIncludeGWorld, UBOOL bOnlyEditorVisible)
{
	OutWorlds.Empty();
	if ( bIncludeGWorld )
	{
		OutWorlds.AddUniqueItem( GWorld );
	}

	// Iterate over the worldinfo's level array to find referenced levels ("worlds"). We don't 
	// iterate over the GWorld->Levels array as that only contains currently associated levels.
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();

	for ( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
		if ( StreamingLevel )
		{
			// If we asked for only sub-levels that are editor-visible, then limit our results appropriately
			UBOOL bShouldAlwaysBeLoaded = FALSE; // Cast< ULevelStreamingAlwaysLoaded >( StreamingLevel ) != NULL;
			if( !bOnlyEditorVisible || bShouldAlwaysBeLoaded || StreamingLevel->bShouldBeVisibleInEditor )
			{
				// This should always be the case for valid level names as the Editor preloads all packages.
				if ( StreamingLevel->LoadedLevel )
				{
					// Newer levels have their packages' world as the outer.
					UWorld* World = Cast<UWorld>( StreamingLevel->LoadedLevel->GetOuter() );
					if ( World )
					{
						OutWorlds.AddUniqueItem( World );
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	FindStreamingLevel methods.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns the streaming level corresponding to the specified ULevel, or NULL if none exists.
 *
 * @param		Level		The level to query.
 * @return					The level's streaming level, or NULL if none exists.
 */
ULevelStreaming* FLevelUtils::FindStreamingLevel(ULevel* Level)
{
	ULevelStreaming* MatchingLevel = NULL;

	AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
	for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* CurStreamingLevel = WorldInfo->StreamingLevels( LevelIndex );
		if( CurStreamingLevel && CurStreamingLevel->LoadedLevel == Level )
		{
			MatchingLevel = CurStreamingLevel;
			break;
		}
	}

	return MatchingLevel;
}

/**
 * Returns the streaming level by package name, or NULL if none exists.
 *
 * @param		PackageName		Name of the package containing the ULevel to query
 * @return						The level's streaming level, or NULL if none exists.
 */
ULevelStreaming* FLevelUtils::FindStreamingLevel(const TCHAR* InPackageName)
{
	const FName PackageName( InPackageName );

	ULevelStreaming* MatchingLevel = NULL;

	AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
	for( INT LevelIndex = 0 ; LevelIndex< WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* CurStreamingLevel = WorldInfo->StreamingLevels( LevelIndex );
		if( CurStreamingLevel && CurStreamingLevel->PackageName == PackageName )
		{
			MatchingLevel = CurStreamingLevel;
			break;
		}
	}

	return MatchingLevel;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Level bounding box methods.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Queries for the bounding box visibility a level's UStreamLevel 
 *
 * @param	Level		The level to query.
 * @return				TRUE if the level's bounding box is visible, FALSE otherwise.
 */
UBOOL FLevelUtils::IsLevelBoundingBoxVisible(ULevel* Level)
{
	if ( Level == GWorld->PersistentLevel )
	{
		return TRUE;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	checkMsg( StreamingLevel, "Couldn't find streaming level" );

	const UBOOL bBoundingBoxVisible = StreamingLevel->bBoundingBoxVisible;
	return bBoundingBoxVisible;
}

/**
 * Toggles whether or not a level's bounding box is rendered in the editor in place
 * of the level itself.
 *
 * @param	Level		The level to query.
 */
void FLevelUtils::ToggleLevelBoundingBox(ULevel* Level)
{
	if ( !Level || Level == GWorld->PersistentLevel )
	{
		return;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	checkMsg( StreamingLevel, "Couldn't find streaming level" );

	StreamingLevel->bBoundingBoxVisible = !StreamingLevel->bBoundingBoxVisible;

	GWorld->UpdateLevelStreaming();
	GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	RemoveLevelFromWorld
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Removes a level from the world.  Returns true if the level was removed successfully.
 *
 * @param	Level		The level to remove from the world.
 * @return				TRUE if the level was removed successfully, FALSE otherwise.
 */
UBOOL FLevelUtils::RemoveLevelFromWorld(ULevel* Level)
{
	if ( !Level || Level == GWorld->PersistentLevel )
	{
		return FALSE;
	}

	if ( IsLevelLocked(Level) )
	{
		appMsgf(AMT_OK, TEXT("RemoveLevelFromWorld: %s"), *LocalizeUnrealEd(TEXT("Error_OperationDisallowedOnLockedLevel")));
		return FALSE;
	}

	INT StreamingLevelIndex = INDEX_NONE;

	AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
	for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels( LevelIndex );
		if( StreamingLevel && StreamingLevel->LoadedLevel == Level )
		{
			StreamingLevelIndex = LevelIndex;
			break;
		}
	}

	const UBOOL bSuccess = StreamingLevelIndex != INDEX_NONE;
	if ( bSuccess )
	{
		WorldInfo->StreamingLevels.Remove( StreamingLevelIndex );
		WorldInfo->PostEditChange();
		
		GWorld->EditorDestroyLevel(Level);
	}

	return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Level locking/unlocking.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns TRUE if the specified level is locked for edit, FALSE otherwise.
 *
 * @param	Level		The level to query.
 * @return				TRUE if the level is locked, FALSE otherwise.
 */
UBOOL FLevelUtils::IsLevelLocked(ULevel* Level)
{
	if ( Level->IsInPIEPackage() || Level == GWorld->PersistentLevel || Level->GetName() == TEXT("TransLevelMoveBuffer") )
	{
		// PIE levels, the persistent level, and transient move levels are never locked.
		return FALSE;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	if( StreamingLevel != NULL )
	{
		return StreamingLevel->bLocked;
	}
	else
	{
		warnf( TEXT( "Couldn't find streaming level: %s" ), *Level->GetName() );
	}

	return FALSE;
}
UBOOL FLevelUtils::IsLevelLocked( AActor* Actor )
{
	return Actor != NULL && !Actor->IsTemplate() && Actor->GetLevel() != NULL && IsLevelLocked(Actor->GetLevel());
}

/**
 * Sets a level's edit lock.
 *
 * @param	Level		The level to modify.
 */
void FLevelUtils::ToggleLevelLock(ULevel* Level)
{
	if ( !Level || Level == GWorld->PersistentLevel )
	{
		return;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	checkMsg( StreamingLevel, "Couldn't find streaming level" );

	StreamingLevel->bLocked = !StreamingLevel->bLocked;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Level loading/unloading.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns TRUE if the level is currently loaded in the editor, FALSE otherwise.
 *
 * @param	Level		The level to query.
 * @return				TRUE if the level is loaded, FALSE otherwise.
 */
UBOOL FLevelUtils::IsLevelLoaded(ULevel* Level)
{
	if ( Level == GWorld->PersistentLevel )
	{
		// The persistent level is always loaded.
		return TRUE;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	checkMsg( StreamingLevel, "Couldn't find streaming level" );

	// @todo: Dave, please come talk to me before implementing anything like this.
	return TRUE;
}

/**
 * Flags an unloaded level for loading.
 *
 * @param	Level		The level to modify.
 */
void FLevelUtils::MarkLevelForLoading(ULevel* Level)
{
	// If the level is valid and not the persistent level (which is always loaded) . . .
	if ( Level && Level != GWorld->PersistentLevel )
	{
		// Mark the level's stream for load.
		ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
		checkMsg( StreamingLevel, "Couldn't find streaming level" );
		// @todo: Dave, please come talk to me before implementing anything like this.
	}
}

/**
 * Flags a loaded level for unloading.
 *
 * @param	Level		The level to modify.
 */
void FLevelUtils::MarkLevelForUnloading(ULevel* Level)
{
	// If the level is valid and not the persistent level (which is always loaded) . . .
	if ( Level && Level != GWorld->PersistentLevel )
	{
		ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
		checkMsg( StreamingLevel, "Couldn't find streaming level" );
		// @todo: Dave, please come talk to me before implementing anything like this.
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Level lighting.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns whether the passed in level is set to use self contained lighting.
 *
 * @param	Level	The level to query
 *
 * @return TRUE if level is set up to have self contained lighting, FALSE otherwise
 */
UBOOL FLevelUtils::HasSelfContainedLighting( ULevel* Level )
{
	UBOOL bHasSelfContainedLighting = Level && ((Level->GetOutermost()->PackageFlags & PKG_SelfContainedLighting) == PKG_SelfContainedLighting);
	return bHasSelfContainedLighting;
}

/**
 * Sets the passed in level to use self contained lighting or not, depending on
 * passed in argument.
 *
 * @param	Level	The level to set whether self contained lighting is used or not
 * @param	bUseSelfContainedLighting Whether to use self contained lighting or not
 */
void FLevelUtils::SetSelfContainedLighting( ULevel* Level, UBOOL bUseSelfContainedLighting )
{
	if( Level )
	{
		if( bUseSelfContainedLighting )
		{
			Level->GetOutermost()->PackageFlags |= PKG_SelfContainedLighting;
		}
		else
		{
			Level->GetOutermost()->PackageFlags &= ~PKG_SelfContainedLighting;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Level visibility.
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns TRUE if the specified level is visible in the editor, FALSE otherwise.
 *
 * @param	StreamingLevel		The level to query.
 */
UBOOL FLevelUtils::IsLevelVisible(ULevelStreaming* StreamingLevel)
{
	const UBOOL bVisible = StreamingLevel->bShouldBeVisibleInEditor;
	return bVisible;
}

/**
 * Returns TRUE if the specified level is visible in the editor, FALSE otherwise.
 *
 * @param	Level		The level to query.
 */
UBOOL FLevelUtils::IsLevelVisible(ULevel* Level)
{
	// P-level is specially handled
	if ( Level == GWorld->PersistentLevel )
	{
		return !( GWorld->PersistentLevel->GetWorldInfo()->bHiddenEdLevel );
	}

	// Handle streaming level
	ULevelStreaming* StreamingLevel = FindStreamingLevel( Level );
	if( StreamingLevel )
	{
		return IsLevelVisible( StreamingLevel );
	}
	else
	{
		warnf( TEXT( "Couldn't find streaming level: %s" ), *Level->GetName() );
	}

	return TRUE;
}

/**
* @return		TRUE if the actor should be considered for group visibility, FALSE otherwise.
*/
static inline UBOOL IsValidForGroupVisibility(const AActor* Actor)
{
	return ( Actor && Actor != GWorld->GetBrush() && Actor->GetClass()->GetDefaultActor()->bHiddenEd == FALSE );
}

/**
 * Sets a level's visibility in the editor.
 *
 * @param	StreamingLevel			The level to modify.
 * @param	bShouldBeVisible		The level's new visibility state.
 * @param	bForceGroupsVisible		If TRUE and the level is visible, force the level's groups to be visible.
 */
void FLevelUtils::SetLevelVisibility(ULevelStreaming* StreamingLevel, ULevel* Level, UBOOL bShouldBeVisible, UBOOL bForceGroupsVisible)
{
	// Handle the case of a streaming level
	if ( StreamingLevel )
	{
		// Set the visibility state for this streaming level.  Note that this will dirty the persistent level.
		StreamingLevel->Modify();
		StreamingLevel->bShouldBeVisibleInEditor = bShouldBeVisible;

		GWorld->UpdateLevelStreaming();

		// Force the level's groups to be visible, if desired
		if ( Level )
		{
			GCallbackEvent->Send( CALLBACK_RedrawAllViewports );

			if ( bShouldBeVisible && bForceGroupsVisible )
			{
				// Iterate over the level's actors, making a list of their groups and unhiding the groups.
				TArray<FString> VisibleGroups;
				GWorld->GetWorldInfo()->VisibleGroups.ParseIntoArray( &VisibleGroups, TEXT(","), 0 );

				TArray<FString> NewGroups;
				TTransArray<AActor*>& Actors = Level->Actors;
				UBOOL bVisibleGroupsNeedUpdate = FALSE;
				for ( INT ActorIndex = 0 ; ActorIndex < Actors.Num() ; ++ActorIndex )
				{
					AActor* Actor = Actors( ActorIndex );
					if ( IsValidForGroupVisibility( Actor ) )
					{
						// Make the actor group visible, if it's not already.
						if ( Actor->bHiddenEdGroup )
						{
							// While this action "dirties" the actor, intentionally do not call Modify() in order to prevent
							// the level from dirtying over a visibility change
							Actor->bHiddenEdGroup = FALSE;
						}

						// Add the actor's groups to the list of visible groups.
						const FString GroupName = *Actor->Group.ToString();
						GroupName.ParseIntoArray( &NewGroups, TEXT(","), FALSE );

						for( INT x = 0 ; x < NewGroups.Num() ; ++x )
						{
							const FString& NewGroup = NewGroups( x );
							if ( !VisibleGroups.ContainsItem( NewGroup ) )
							{
								VisibleGroups.AddItem( NewGroup );
								bVisibleGroupsNeedUpdate = TRUE;
							}
						}
					}
				}

				// Only update the visible groups if required
				if ( bVisibleGroupsNeedUpdate )
				{
					// Copy the visible groups list back over to worldinfo.
					GWorld->GetWorldInfo()->VisibleGroups = TEXT("");
					for( INT x = 0 ; x < VisibleGroups.Num() ; ++x )
					{
						if( GWorld->GetWorldInfo()->VisibleGroups.Len() > 0 )
						{
							GWorld->GetWorldInfo()->VisibleGroups += TEXT(",");
						}
						GWorld->GetWorldInfo()->VisibleGroups += VisibleGroups(x);
					}
				}
			}
		}
	}

	// Handle the case of the p-level
	// The p-level can't be unloaded, so its actors/BSP should just be temporarily hidden/unhidden
	// Also, intentionally do not force groups visible for the p-level
	else if ( Level && Level == GWorld->PersistentLevel )
	{
		// Set the visibility of each actor in the p-level
		for ( TArray<AActor*>::TIterator PLevelActorIter( Level->Actors ); PLevelActorIter; ++PLevelActorIter )
		{
			AActor* CurActor = *PLevelActorIter;
			if ( CurActor && !CurActor->IsABuilderBrush() )
			{
				CurActor->bHiddenEdLevel = !bShouldBeVisible;
				CurActor->ForceUpdateComponents( FALSE, FALSE );
			}
		}

		// Set the visibility of each BSP surface in the p-level
		UModel* CurLevelModel = Level->Model;
		if ( CurLevelModel )
		{
			for ( TArray<FBspSurf>::TIterator SurfaceIterator( CurLevelModel->Surfs ); SurfaceIterator; ++SurfaceIterator )
			{
				FBspSurf& CurSurf = *SurfaceIterator;
				CurSurf.bHiddenEdLevel = !bShouldBeVisible;
			}
		}
		GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
	}

	GCallbackEvent->Send( CALLBACK_RefreshEditor_GroupBrowser );
}
