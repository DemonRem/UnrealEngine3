/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "GroupUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper functions.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @return		TRUE if the actor can be considered by the group browser, FALSE otherwise.
 */
static inline UBOOL IsValid(const AActor* Actor)
{
	const UBOOL bIsBuilderBrush	= ( Actor == GWorld->GetBrush() );
	const UBOOL bIsHidden		= ( Actor->GetClass()->GetDefaultActor()->bHiddenEd == TRUE );
	const UBOOL bIsValid		= !bIsHidden && !bIsBuilderBrush;

	return bIsValid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Operations on an individual actor.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Adds the actor to the named group.
 *
 * @return		TRUE if the actor was added.  FALSE is returned if the
 *              actor already belongs to the group.
 */
UBOOL FGroupUtils::AddActorToGroup(AActor* Actor, const FString& GroupName, UBOOL bModifyActor)
{
	if(	IsValid( Actor ) )
	{
		// Don't add the actor to a group it already belongs to.
		if( !Actor->IsInGroup( *GroupName ) )
		{
			if ( bModifyActor )
			{
				Actor->Modify();
			}

			const FString CurGroup = Actor->Group.ToString();
			Actor->Group = FName( *FString::Printf(TEXT("%s%s%s"), *CurGroup, (CurGroup.Len() ? TEXT(","):TEXT("")), *GroupName ) );

			// Remove the actor from "None.
			const FString NoneGroup( TEXT("None") );
			if ( Actor->IsInGroup( *NoneGroup ) )
			{
				RemoveActorFromGroup( Actor, NoneGroup, FALSE );
			}

			// update per-view visibility info
			UpdateActorAllViewsVisibility(Actor);

			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Removes an actor from the specified group.
 *
 * @return		TRUE if the actor was removed from the group.  FALSE is returned if the
 *              actor already belonged to the group.
 */
UBOOL FGroupUtils::RemoveActorFromGroup(AActor* Actor, const FString& GroupToRemove, UBOOL bModifyActor)
{
	if(	IsValid( Actor ) )
	{
		TArray<FString> GroupArray;
		FString GroupName = Actor->Group.ToString();
		GroupName.ParseIntoArray( &GroupArray, TEXT(","), FALSE );

		const UBOOL bRemovedFromGroup = GroupArray.RemoveItem( GroupToRemove ) > 0;
		if ( bRemovedFromGroup )
		{
			// Reconstruct a new group name list for the actor.
			GroupName = TEXT("");
			for( INT GroupIndex = 0 ; GroupIndex < GroupArray.Num() ; ++GroupIndex )
			{
				if( GroupName.Len() )
				{
					GroupName += TEXT(",");
				}
				GroupName += GroupArray(GroupIndex);
			}

			if ( bModifyActor )
			{
				Actor->Modify();
			}
			Actor->Group = FName( *GroupName );

			// update per-view visibility info
			UpdateActorAllViewsVisibility(Actor);

			return TRUE;
		}
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Operations on selected actors.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Adds selected actors to the named group.
 *
 * @param	GroupName	A group name.
 * @return				TRUE if at least one actor was added.  FALSE is returned if all selected
 *                      actors already belong to the named group.
 */
UBOOL FGroupUtils::AddSelectedActorsToGroups(const TArray<FString>& GroupNames)
{
	UBOOL bReturnVal = FALSE;

	const FString NoneString( TEXT("None") );
	const UBOOL bAddingToNoneGroup = GroupNames.ContainsItem( NoneString );

	TArray<FString> GroupArray;

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		if ( IsValid( Actor ) )
		{
			// Break the actors group names down and only add the ones back in that don't occur in GroupNames.
			FString GroupName = Actor->Group.ToString();
			GroupName.ParseIntoArray( &GroupArray, TEXT(","), FALSE );

			const INT OldGroupNum = GroupArray.Num();
			const UBOOL bWasInNoneGroup = GroupArray.ContainsItem( NoneString );

			// Add groups to the list.
			for( INT x = 0 ; x < GroupNames.Num() ; ++x )
			{
				GroupArray.AddUniqueItem( GroupNames(x) );
			}

			// Were any new groups added to this actor?
			if ( OldGroupNum != GroupArray.Num() )
			{
				// Reconstruct a new group name list for the actor.
				GroupName = TEXT("");

				// Skip over the "None" group if the actor used to belong to that group and we're not adding it.
				const UBOOL bSkipOverNoneGroup = bWasInNoneGroup && !bAddingToNoneGroup;

				for( INT GroupIndex = 0 ; GroupIndex < GroupArray.Num() ; ++GroupIndex )
				{
					// Skip over the "None" group if the actor used to belong to that group
					if ( bSkipOverNoneGroup && GroupArray(GroupIndex) == NoneString )
					{
						continue;
					}

					if( GroupName.Len() )
					{
						GroupName += TEXT(",");
					}
					GroupName += GroupArray(GroupIndex);
				}
				Actor->Modify();
				Actor->Group = FName( *GroupName );

				// update per-view visibility info
				UpdateActorAllViewsVisibility(Actor);

				bReturnVal = TRUE;
			}
		}
	}

	return bReturnVal;
}

/**
 * Adds selected actors to the named groups.
 *
 * @param	GroupNames	A valid list of group names.
 * @return				TRUE if at least one actor was added.  FALSE is returned if all selected
 *                      actors already belong to the named groups.
 */
UBOOL FGroupUtils::AddSelectedActorsToGroup(const FString& GroupName)
{
	UBOOL bReturnVal = FALSE;

	const FString NoneGroup( TEXT("None") );
	const UBOOL bAddingToNoneGroup = (GroupName == NoneGroup);

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		if ( IsValid( Actor ) )
		{
			// Don't add the actor to a group it already belongs to.
			if( !Actor->IsInGroup( *GroupName ) )
			{
				// Add the group to the actors group list.
				Actor->Modify();

				const FString CurGroup = Actor->Group.ToString();
				Actor->Group = FName( *FString::Printf(TEXT("%s%s%s"), *CurGroup, (CurGroup.Len() ? TEXT(","):TEXT("")), *GroupName ) );

				// If we're not adding to the "None" group and the actor has single membership in the "None" group, remove the actor from "None.
				if ( !bAddingToNoneGroup && Actor->IsInGroup( *NoneGroup ) )
				{
					RemoveActorFromGroup( Actor, NoneGroup, FALSE );
				}

				// update per-view visibility info
				UpdateActorAllViewsVisibility(Actor);

				bReturnVal = TRUE;
			}
		}
	}
	return bReturnVal;
}

/**
 * Removes selected actors from the named groups.
 *
 * @param	GroupNames	A valid list of group names.
 * @return				TRUE if at least one actor was removed.
 */
UBOOL FGroupUtils::RemoveSelectedActorsFromGroups(const TArray<FString>& GroupNames)
{
	UBOOL bReturnVal = FALSE;
	TArray<FString> GroupArray;

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		if ( IsValid( Actor ) )
		{
			// Break the actors group names down and only add the ones back in that don't occur in GroupNames.
			FString GroupName = Actor->Group.ToString();
			GroupName.ParseIntoArray( &GroupArray, TEXT(","), FALSE );

			UBOOL bRemovedFromGroup = FALSE;
			for( INT x = 0 ; x < GroupArray.Num() ; ++x )
			{
				if ( GroupNames.ContainsItem( GroupArray(x) ) )
				{
					GroupArray.Remove( x );
					--x;
					bRemovedFromGroup = TRUE;
				}
			}

			if ( bRemovedFromGroup )
			{
				// Reconstruct a new group name list for the actor.
				GroupName = TEXT("");
				for( INT GroupIndex = 0 ; GroupIndex < GroupArray.Num() ; ++GroupIndex )
				{
					if( GroupName.Len() )
					{
						GroupName += TEXT(",");
					}
					GroupName += GroupArray(GroupIndex);
				}
				Actor->Modify();
				Actor->Group = FName( *GroupName );

				// update per-view visibility info
				UpdateActorAllViewsVisibility(Actor);

				bReturnVal = TRUE;
			}
		}
	}

	return bReturnVal;
}

/**
 * Selects/deselects actors belonging to the named groups.
 *
 * @param	GroupNames	A list of group names.
 * @param	bSelect		If TRUE actors are selected; if FALSE, actors are deselected.
 * @param	Editor		The editor in which to select/deselect the actors.
 * @return				TRUE if at least one actor was selected/deselected.
 */
UBOOL FGroupUtils::SelectActorsInGroups(const TArray<FString>& GroupNames, UBOOL bSelect, UEditorEngine* Editor)
{
	UBOOL bSelectedActor = FALSE;

	if ( GroupNames.Num() > 0 )
	{
		// Iterate over all actors, looking for actors in the specified groups.
		for( FActorIterator It ; It ; ++It )
		{
			AActor* Actor = *It;
			if( IsValid( Actor ) )
			{
				for ( INT GroupIndex = 0 ; GroupIndex < GroupNames.Num() ; ++GroupIndex )
				{
					const FString& CurGroup = GroupNames(GroupIndex);
					if ( Actor->IsInGroup( *CurGroup ) )
					{
						// The actor was found to be in a specified group.
						// Set selection state and move on to the next actor.
						Editor->SelectActor( Actor, bSelect, NULL, FALSE, TRUE );
						bSelectedActor = TRUE;
						break;
					}
				}
			}
		}
	}

	return bSelectedActor;
}








/**
 * Updates the per-view visibility for all actors for the given view
 *
 * @param ViewIndex Index of the view (into GEditor->ViewportClients)
 * @param GroupThatChanged [optional] If one group was changed (toggled in view popup, etc), then we only need to modify actors that use that group
 */
void FGroupUtils::UpdatePerViewVisibility(INT ViewIndex, FName GroupThatChanged)
{
	// get the viewport client
	FEditorLevelViewportClient* Viewport = GEditor->ViewportClients(ViewIndex);

	// cache the FString representation of the fname
	FString GroupString = GroupThatChanged.ToString();

	// Iterate over all actors, looking for actors in the specified groups.
	for( FActorIterator It ; It ; ++It )
	{
		AActor* Actor = *It;
		if( IsValid( Actor ) )
		{
			// if the view has nothing hidden, just just quickly mark the actor as visible in this view 
			if (Viewport->ViewHiddenGroups.Num() == 0)
			{
				// if the actor had this view hidden, then unhide it
				if (Actor->HiddenEditorViews & ((QWORD)1 << ViewIndex))
				{
					// make sure this actor doesn't have the view set
					Actor->HiddenEditorViews &= ~((QWORD)1 << ViewIndex);
					Actor->ConditionalForceUpdateComponents(FALSE, FALSE);
				}
			}
			// else if we were given a name that was changed, only update actors with that name in their groups,
			// otherwise update all actors
			else if (GroupThatChanged == NAME_Skip || Actor->Group.ToString().InStr(*GroupString) != INDEX_NONE)
			{
				UpdateActorViewVisibility(ViewIndex, Actor);
			}
		}
	}

	// make sure we redraw the viewport
	Viewport->Invalidate();
}


/**
 * Updates per-view visibility for the given actor in the given view
 *
 * @param ViewIndex Index of the view (into GEditor->ViewportClients)
 * @param Actor Actor to update
 * @param bReattachIfDirty If TRUE, the actor will reattach itself to give the rendering thread updated information
 */
void FGroupUtils::UpdateActorViewVisibility(INT ViewIndex, AActor* Actor, UBOOL bReattachIfDirty)
{
	// get the viewport client
	FEditorLevelViewportClient* Viewport = GEditor->ViewportClients(ViewIndex);

	// get the groups the actor is in
	TArray<FString> GroupList;
	Actor->GetGroups( GroupList );

	INT NumHiddenGroups = 0;
	// look for which of the actor groups are hidden
	for (INT HiddenGroupIndex = 0; HiddenGroupIndex < GroupList.Num(); HiddenGroupIndex++)
	{
		FName HiddenGroup = FName(*GroupList(HiddenGroupIndex));
		// if its in the view hidden list, this group is hidden for this actor
		if (Viewport->ViewHiddenGroups.FindItemIndex(HiddenGroup) != -1)
		{
			NumHiddenGroups++;
			// right now, if one is hidden, the actor is hidden
			break;
		}
	}

	QWORD OriginalHiddenViews = Actor->HiddenEditorViews;

	// right now, if one is hidden, the actor is hidden
	if (NumHiddenGroups)
	{
		Actor->HiddenEditorViews |= ((QWORD)1 << ViewIndex);
	}
	else
	{
		Actor->HiddenEditorViews &= ~((QWORD)1 << ViewIndex);
	}

	// reattach if we changed the visibility bits, as the rnedering thread needs them
	if (bReattachIfDirty && OriginalHiddenViews != Actor->HiddenEditorViews)
	{
		Actor->ConditionalForceUpdateComponents(FALSE, FALSE);

		// make sure we redraw the viewport
		Viewport->Invalidate();
	}
}

/**
 * Updates per-view visibility for the given actor for all views
 *
 * @param Actor Actor to update
 */
void FGroupUtils::UpdateActorAllViewsVisibility(AActor* Actor)
{
	QWORD OriginalHiddenViews = Actor->HiddenEditorViews;

	for (INT ViewIndex = 0; ViewIndex < GEditor->ViewportClients.Num(); ViewIndex++)
	{
		// don't have this reattach, as we can do it once for all views
		UpdateActorViewVisibility(ViewIndex, Actor, FALSE);
	}

	// reattach if we changed the visibility bits, as the rnedering thread needs them
	if (OriginalHiddenViews != Actor->HiddenEditorViews)
	{
		Actor->ConditionalForceUpdateComponents(FALSE, FALSE);

		// redraw all viewports if the actor
		for (INT ViewIndex = 0; ViewIndex < GEditor->ViewportClients.Num(); ViewIndex++)
		{
			// make sure we redraw all viewports
			GEditor->ViewportClients(ViewIndex)->Invalidate();
		}
	}
}


/**
 * Removes the corresponding visibility bit from all actors (slides the later bits down 1)
 *
 * @param ViewIndex Index of the view (into GEditor->ViewportClients)
 */
void FGroupUtils::RemoveViewFromActorViewVisibility(INT ViewIndex)
{
	// get the bit for the view index
	QWORD ViewBit = ((QWORD)1 << ViewIndex);
	// get all bits under that that we want to keep
	QWORD KeepBits = ViewBit - 1;

	// Iterate over all actors, looking for actors in the specified groups.
	for( FActorIterator It ; It ; ++It )
	{
		AActor* Actor = *It;
		if( IsValid( Actor ) )
		{
			// remember original bits
			QWORD OriginalHiddenViews = Actor->HiddenEditorViews;

			QWORD Was = Actor->HiddenEditorViews;

			// slide all bits higher than ViewIndex down one since the view is being removed from GEditor
			QWORD LowBits = Actor->HiddenEditorViews & KeepBits;

			// now slide the top bits down by ViewIndex + 1 (chopping off ViewBit)
			QWORD HighBits = Actor->HiddenEditorViews >> (ViewIndex + 1);
			// then slide back up by ViewIndex, which will now have erased ViewBit, as well as leaving 0 in the low bits
			HighBits = HighBits << ViewIndex;

			// put it all back together
			Actor->HiddenEditorViews = LowBits | HighBits;

			// reattach if we changed the visibility bits, as the rnedering thread needs them
			if (OriginalHiddenViews != Actor->HiddenEditorViews)
			{
				// Find all attached primitive components and update the scene proxy with the actors updated visibility map
				for( INT ComponentIdx = 0; ComponentIdx < Actor->Components.Num(); ++ComponentIdx )
				{
					UActorComponent* Component = Actor->Components(ComponentIdx);
					if (Component && Component->IsAttached())
					{
						UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
						if (PrimitiveComponent)
						{
							// Push visibility to the render thread
							PrimitiveComponent->PushEditorVisibilityToProxy( Actor->HiddenEditorViews );
						}
					}
				}
			}
		}
	}
}

/**
 * Gets all known groups from the world
 *
 * @param AllGroups Output array to store all known groups
 */
void FGroupUtils::GetAllGroups(TArray<FName>& AllGroups)
{
	// clear anything in there
	AllGroups.Empty();

	// get a list of all unique groups currently in use by actors
	for (FActorIterator It; It; ++It)
	{
		AActor* Actor = *It;
		if(	IsValid(Actor) )
		{
			TArray<FString> GroupArray;
			Actor->GetGroups( GroupArray );

			// add unique groups to the list of all groups
			for( INT GroupIndex = 0 ; GroupIndex < GroupArray.Num() ; ++GroupIndex )
			{
				AllGroups.AddUniqueItem(FName(*GroupArray(GroupIndex)));
			}
		}
	}
}

/**
 * Gets all known groups from the world
 *
 * @param AllGroups Output array to store all known groups
 */
void FGroupUtils::GetAllGroups(TArray<FString>& AllGroups)
{
	// clear anything in there
	AllGroups.Empty();

	// get a list of all unique groups currently in use by actors
	for (FActorIterator It; It; ++It)
	{
		AActor* Actor = *It;
		if(	IsValid(Actor) )
		{
			TArray<FString> GroupArray;
			Actor->GetGroups( GroupArray );

			// add unique groups to the list of all groups
			for( INT GroupIndex = 0 ; GroupIndex < GroupArray.Num() ; ++GroupIndex )
			{
				AllGroups.AddUniqueItem(GroupArray(GroupIndex));
			}
		}
	}
}
