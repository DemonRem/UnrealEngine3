/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnPath.h"
#include "FileHelpers.h"
#include "ScopedTransaction.h"
#include "UnFaceFXSupport.h"

#if WITH_FACEFX
using namespace OC3Ent;
using namespace Face;
#endif

//@hack: this needs to be cleaned up!
static TCHAR TempStr[MAX_EDCMD], TempName[MAX_EDCMD], Temp[MAX_EDCMD];
static WORD Word1, Word2, Word4;

/**
 * Dumps a set of selected objects to debugf.
 */
static void PrivateDumpSelection(USelection* Selection)
{
	for ( USelection::TObjectConstIterator Itor = Selection->ObjectConstItor() ; Itor ; ++Itor )
	{
		UObject *CurObject = *Itor;
		if ( CurObject )
		{
			debugf(TEXT("    %s"), *CurObject->GetClass()->GetName() );
		}
		else
		{
			debugf(TEXT("    NULL object"));
		}
	}
}

UBOOL UUnrealEdEngine::Exec( const TCHAR* Stream, FOutputDevice& Ar )
{
	if( UEditorEngine::Exec( Stream, Ar ) )
	{
		return TRUE;
	}

	const TCHAR* Str = Stream;

	if( ParseCommand(&Str, TEXT("DUMPSELECTION")) )
	{
		debugf(TEXT("Selected Actors:"));
		PrivateDumpSelection( GetSelectedActors() );
		debugf(TEXT("Selected Non-Actors:"));
		PrivateDumpSelection( GetSelectedObjects() );
	}
	//----------------------------------------------------------------------------------
	// EDIT
	//
	if( ParseCommand(&Str,TEXT("EDIT")) )
	{
		if( Exec_Edit( Str, Ar ) )
		{
			return TRUE;
		}
	}
	//------------------------------------------------------------------------------------
	// ACTOR: Actor-related functions
	//
	else if (ParseCommand(&Str,TEXT("ACTOR")))
	{
		if( Exec_Actor( Str, Ar ) )
		{
			return TRUE;
		}
	}
	//------------------------------------------------------------------------------------
	// MODE management (Global EDITOR mode):
	//
	else if( ParseCommand(&Str,TEXT("MODE")) )
	{
		if( Exec_Mode( Str, Ar ) )
		{
			return TRUE;
		}
	}
	//----------------------------------------------------------------------------------
	// PIVOT
	//
	else if( ParseCommand(&Str,TEXT("PIVOT")) )
	{
		if(		Exec_Pivot( Str, Ar ) )
		{
			return TRUE;
		}
	}
	//----------------------------------------------------------------------------------
	// QUERY VALUE
	//
	else if (ParseCommand(&Str, TEXT("QUERYVALUE")))
	{
		FString Key;
		// get required key value
		if (!ParseToken(Str, Key, FALSE))
		{
			return FALSE;
		}

		FString Label;
		// get required prompt
		if (!ParseToken(Str, Label, FALSE))
		{
			return FALSE;
		}

		FString Default;
		// default is optional
		ParseToken(Str, Default, FALSE);

		wxTextEntryDialog Dlg(NULL, *Label, *Key, *Default);

		if(Dlg.ShowModal() == wxID_OK)
		{
			// if the user hit OK, pass back the result in the OutputDevice
			Ar.Log(Dlg.GetValue());
		}

		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("UNMOUNTALLFACEFX")) )
	{
#if WITH_FACEFX
		for( TObjectIterator<UFaceFXAsset> It; It; ++It )
		{
			UFaceFXAsset* Asset = *It;
			FxActor* fActor = Asset->GetFxActor();
			if(fActor)
			{
				// If its open in studio - do not modify it (warn instead).
				if(fActor->IsOpenInStudio())
				{
					appMsgf(AMT_OK, *LocalizeUnrealEd("CannotUnmountFaceFXOpenInStudio"), *Asset->GetPathName());
				}
				else
				{
					// Copy array, as we will be unmounting things and changing the one on the asset!
					TArray<UFaceFXAnimSet*> MountedSets = Asset->MountedFaceFXAnimSets;
					for(INT i=0; i<MountedSets.Num(); i++)
					{
						UFaceFXAnimSet* Set = MountedSets(i);
						Asset->UnmountFaceFXAnimSet(Set);
						debugf( TEXT("Unmounting: %s From %s"), *Set->GetName(), *Asset->GetName() );
					}
				}
			}
		}
#endif // WITH_FACEFX

		return TRUE;
	}
	else
	{
		UUISceneManager* SceneManager = GetBrowserManager()->UISceneManager;
		if ( SceneManager != NULL && SceneManager->Exec(Stream, Ar) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

/** @return Returns whether or not the user is able to autosave. **/
UBOOL UUnrealEdEngine::CanAutosave() const
{
	return ( AutoSave && !GIsSlowTask && GEditorModeTools().GetCurrentModeID() != EM_InterpEdit && !PlayWorld );
}

/** @return Returns whether or not autosave is going to occur within the next minute. */
UBOOL UUnrealEdEngine::AutoSaveSoon() const
{
	UBOOL bResult = FALSE;
	if(CanAutosave())
	{
		FLOAT TimeTillAutoSave = (FLOAT)AutosaveTimeMinutes - AutosaveCount/60.0f;
		bResult = TimeTillAutoSave < 1.0f && TimeTillAutoSave > 0.0f;
	}

	return bResult;
}

/** @return Returns the amount of time until the next autosave in seconds. */
INT UUnrealEdEngine::GetTimeTillAutosave() const
{
	INT Result = -1;

	if(CanAutosave())
	{
		 Result = appTrunc(60*(AutosaveTimeMinutes - AutosaveCount/60.0f));
	}

	return Result;
}

void UUnrealEdEngine::AttemptLevelAutosave()
{
	// Don't autosave if disabled or if it is not yet time to autosave.
	const UBOOL bTimeToAutosave = ( AutoSave && (AutosaveCount/60.0f >= (FLOAT)AutosaveTimeMinutes) );
	if ( bTimeToAutosave )
	{
		// Don't autosave during interpolation editing, if there's another slow task
		// already in progress, or while a PIE world is playing.
		const UBOOL bCanAutosave = CanAutosave();

		if( bCanAutosave )
		{
			// Check to see if the user is in the middle of a drag operation.
			UBOOL bUserIsInteracting = FALSE;
			for( INT ClientIndex = 0 ; ClientIndex < ViewportClients.Num() ; ++ClientIndex )
			{
				if ( ViewportClients(ClientIndex)->bIsTracking )
				{
					bUserIsInteracting = TRUE;
					break;
				}
			}

			// Don't interrupt the user with an autosave.
			if ( !bUserIsInteracting )
			{
				SaveConfig();

				// Make sure the autosave directory exists before attempting to write the file.
				const FString AbsoluteAutoSaveDir( FString(appBaseDir()) * AutoSaveDir );
				GFileManager->MakeDirectory( *AbsoluteAutoSaveDir, 1 );

				// Autosave all levels.
				const INT NewAutoSaveIndex = (AutoSaveIndex+1)%10;
				const UBOOL bLevelSaved = FEditorFileUtils::AutosaveMap( AbsoluteAutoSaveDir, NewAutoSaveIndex );

				if ( bLevelSaved )
				{
					// If a level was actually saved, update the autosave index.
					AutoSaveIndex = NewAutoSaveIndex;
				}

				ResetAutosaveTimer();
			}
		}
	}
}

UBOOL UUnrealEdEngine::Exec_Edit( const TCHAR* Str, FOutputDevice& Ar )
{
	if( ParseCommand(&Str,TEXT("CUT")) )
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("Cut")) );
		edactCopySelected( TRUE, TRUE );
		edactDeleteSelected();
	}
	else if( ParseCommand(&Str,TEXT("COPY")) )
	{
		edactCopySelected( TRUE, TRUE );
	}
	else if( ParseCommand(&Str,TEXT("PASTE")) )
	{
		const FVector SaveClickLocation = GEditor->ClickLocation;

		enum EPasteTo
		{
			PT_OriginalLocation	= 0,
			PT_Here				= 1,
			PT_WorldOrigin		= 2
		} PasteTo = PT_OriginalLocation;

		FString TransName = *LocalizeUnrealEd("Paste");
		if( Parse( Str, TEXT("TO="), TempStr, 15 ) )
		{
			if( !appStrcmp( TempStr, TEXT("HERE") ) )
			{
				PasteTo = PT_Here;
				TransName = *LocalizeUnrealEd("PasteHere");
			}
			else
			{
				if( !appStrcmp( TempStr, TEXT("ORIGIN") ) )
				{
					PasteTo = PT_WorldOrigin;
					TransName = *LocalizeUnrealEd("PasteToWorldOrigin");
				}
			}
		}

		const FScopedTransaction Transaction( *TransName );

		GEditor->SelectNone( TRUE, FALSE );
		edactPasteSelected( FALSE, FALSE, TRUE );

		if( PasteTo != PT_OriginalLocation )
		{
			// Get a bounding box for all the selected actors locations.
			FBox bbox(0);
			INT NumActorsToMove = 0;

			for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
			{
				AActor* Actor = static_cast<AActor*>( *It );
				checkSlow( Actor->IsA(AActor::StaticClass()) );

				bbox += Actor->Location;
				++NumActorsToMove;
			}

			if ( NumActorsToMove > 0 )
			{
				// Figure out which location to center the actors around.
				const FVector Origin( PasteTo == PT_Here ? SaveClickLocation : FVector(0,0,0) );

				// Compute how far the actors have to move.
				const FVector Location = bbox.GetCenter();
				const FVector Adjust = Origin - Location;

				// Move the actors.
				AActor* SingleActor = NULL;
				for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
				{
					AActor* Actor = static_cast<AActor*>( *It );
					checkSlow( Actor->IsA(AActor::StaticClass()) );

					SingleActor = Actor;
					Actor->Location += Adjust;
					Actor->ForceUpdateComponents();
				}

				// Update the pivot location.
				check(SingleActor);
				SetPivot( SingleActor->Location, 0, 0, 1 );
			}
		}

		RedrawLevelEditingViewports();
	}

	return FALSE;
}

UBOOL UUnrealEdEngine::Exec_Pivot( const TCHAR* Str, FOutputDevice& Ar )
{
	if( ParseCommand(&Str,TEXT("HERE")) )
	{
		NoteActorMovement();
		SetPivot( ClickLocation, 0, 0, 0 );
		FinishAllSnaps();
		RedrawLevelEditingViewports();
	}
	else if( ParseCommand(&Str,TEXT("SNAPPED")) )
	{
		NoteActorMovement();
		SetPivot( ClickLocation, 1, 0, 0 );
		FinishAllSnaps();
		RedrawLevelEditingViewports();
	}
	else if( ParseCommand(&Str,TEXT("CENTERSELECTION")) )
	{
		NoteActorMovement();

		// Figure out the center location of all selections

		int count = 0;
		FVector center(0,0,0);

		for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			center += Actor->Location;
			count++;
		}

		if( count > 0 )
		{
			ClickLocation = center / count;

			SetPivot( ClickLocation, 0, 0, 0 );
			FinishAllSnaps();
		}

		RedrawLevelEditingViewports();
	}

	return FALSE;
}

static void MirrorActors(const FVector& MirrorScale)
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd("MirroringActors") );

	// Fires CALLBACK_LevelDirtied when falling out of scope.
	FScopedLevelDirtied		LevelDirtyCallback;

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		if( Actor->IsABrush() )
		{
			const FVector LocalToWorldOffset = -(Actor->Location - GEditorModeTools().PivotLocation + Actor->PrePivot);

			ABrush* Brush = (ABrush*)Actor;
			Brush->Brush->Modify();

			for( INT poly = 0 ; poly < Brush->Brush->Polys->Element.Num() ; poly++ )
			{
				FPoly* Poly = &(Brush->Brush->Polys->Element(poly));
				Brush->Brush->Polys->Element.ModifyAllItems();

				Poly->TextureU *= MirrorScale;
				Poly->TextureV *= MirrorScale;

				Poly->Base += LocalToWorldOffset;
				Poly->Base *= MirrorScale;
				Poly->Base -= LocalToWorldOffset;

				for( INT vtx = 0 ; vtx < Poly->Vertices.Num(); vtx++ )
				{
					Poly->Vertices(vtx) += LocalToWorldOffset;
					Poly->Vertices(vtx) *= MirrorScale;
					Poly->Vertices(vtx) -= LocalToWorldOffset;
				}

				Poly->Reverse();
				Poly->CalcNormal();
			}

			Brush->ClearComponents();
		}
		else
		{
			const FRotationMatrix TempRot( Actor->Rotation );
			const FVector New0( TempRot.GetAxis(0) * MirrorScale );
			const FVector New1( TempRot.GetAxis(1) * MirrorScale );
			const FVector New2( TempRot.GetAxis(2) * MirrorScale );
			// Revert the handedness of the rotation, but make up for it in the scaling.
			// Arbitrarily choose the X axis to remain fixed.
			const FMatrix NewRot( -New0, New1, New2, FVector(0,0,0) );

			Actor->Modify();
			Actor->DrawScale3D.X = -Actor->DrawScale3D.X;
			Actor->Rotation = NewRot.Rotator();
			Actor->Location -= GEditorModeTools().PivotLocation - Actor->PrePivot;
			Actor->Location *= MirrorScale;
			Actor->Location += GEditorModeTools().PivotLocation - Actor->PrePivot;
		}

		Actor->InvalidateLightingCache();
		Actor->PostEditMove( TRUE );

		Actor->MarkPackageDirty();
		LevelDirtyCallback.Request();
	}

	GEditor->RedrawLevelEditingViewports();
}

UBOOL UUnrealEdEngine::Exec_Actor( const TCHAR* Str, FOutputDevice& Ar )
{
	if( ParseCommand(&Str,TEXT("ADD")) )
	{
		UClass* Class;
		if( ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE ) )
		{
			AActor* Default   = Class->GetDefaultActor();
			FVector Collision = Default->GetCylinderExtent();
			INT bSnap = 1;
			Parse(Str,TEXT("SNAP="),bSnap);
			if( bSnap )
			{
				Constraints.Snap( ClickLocation, FVector(0, 0, 0) );
			}
			FVector Location = ClickLocation + ClickPlane * (FBoxPushOut(ClickPlane,Collision) + 0.1);
			if( bSnap )
			{
				Constraints.Snap( Location, FVector(0, 0, 0) );
			}
			AddActor( Class, Location );
			RedrawLevelEditingViewports();
			return TRUE;
		}
	}
	else if( ParseCommand(&Str,TEXT("MIRROR")) )
	{
		FVector MirrorScale( 1, 1, 1 );
		GetFVECTOR( Str, MirrorScale );
		// We can't have zeroes in the vector
		if( !MirrorScale.X )		MirrorScale.X = 1;
		if( !MirrorScale.Y )		MirrorScale.Y = 1;
		if( !MirrorScale.Z )		MirrorScale.Z = 1;
		MirrorActors( MirrorScale );
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("HIDE")) )
	{
		if( ParseCommand(&Str,TEXT("SELECTED")) ) // ACTOR HIDE SELECTED
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("HideSelectedActors") );
			edactHideSelected();
			SelectNone( TRUE, TRUE );
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("UNSELECTED")) ) // ACTOR HIDE UNSELECTEED
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("HideUnselected") );
			edactHideUnselected();
			SelectNone( TRUE, TRUE );
			return TRUE;
		}
	}
	else if( ParseCommand(&Str,TEXT("UNHIDE")) ) // ACTOR UNHIDE ALL
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd("UnHideAll") );
		edactUnHideAll();
		return TRUE;
	}
	else if( ParseCommand(&Str, TEXT("APPLYTRANSFORM")) )
	{
		appErrorf(*LocalizeUnrealEd("Error_TriedToExecDeprecatedCmd"), Str);
	}
	else if( ParseCommand(&Str, TEXT("REPLACE")) )
	{
		UClass* Class;
		if( ParseCommand(&Str, TEXT("BRUSH")) ) // ACTOR REPLACE BRUSH
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("ReplaceSelectedBrushActors") );
			edactReplaceSelectedBrush();
			return TRUE;
		}
		else if( ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE ) ) // ACTOR REPLACE CLASS=<class>
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("ReplaceSelectedNonBrushActors") );
			edactReplaceSelectedNonBrushWithClass( Class );
			return TRUE;
		}
	}
	else if( ParseCommand(&Str,TEXT("SELECT")) )
	{
		if( ParseCommand(&Str,TEXT("NONE")) ) // ACTOR SELECT NONE
		{
			return Exec( TEXT("SELECT NONE") );
		}
		else if( ParseCommand(&Str,TEXT("ALL")) ) // ACTOR SELECT ALL
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectAll") );
			edactSelectAll();
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("INSIDE") ) ) // ACTOR SELECT INSIDE
		{
			appErrorf(*LocalizeUnrealEd("Error_TriedToExecDeprecatedCmd"), Str);
		}
		else if( ParseCommand(&Str,TEXT("INVERT") ) ) // ACTOR SELECT INVERT
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectInvert") );
			edactSelectInvert();
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("OFCLASS")) ) // ACTOR SELECT OFCLASS CLASS=<class>
		{
			UClass* Class;
			if( ParseObject<UClass>(Str,TEXT("CLASS="),Class,ANY_PACKAGE) )
			{
				const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectOfClass") );
				edactSelectOfClass( Class );
			}
			else
			{
				Ar.Log( NAME_ExecWarning, TEXT("Missing class") );
			}
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("OFSUBCLASS")) ) // ACTOR SELECT OFSUBCLASS CLASS=<class>
		{
			UClass* Class;
			if( ParseObject<UClass>(Str,TEXT("CLASS="),Class,ANY_PACKAGE) )
			{
				const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectSubclassOfClass") );
				edactSelectSubclassOf( Class );
			}
			else
			{
				Ar.Log( NAME_ExecWarning, TEXT("Missing class") );
			}
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("BYPROPERTY")) ) // ACTOR SELECT BYPROPERTY
		{
			GEditor->SelectByPropertyColoration();
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("DELETED")) ) // ACTOR SELECT DELETED
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectDeleted") );
			edactSelectDeleted();
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("MATCHINGSTATICMESH")) ) // ACTOR SELECT MATCHINGSTATICMESH
		{
			const UBOOL bAllClasses = ParseCommand( &Str, TEXT("ALLCLASSES") );
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectMatchingStaticMesh") );
			edactSelectMatchingStaticMesh( bAllClasses );
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("KISMETREF")) ) // ACTOR SELECT KISMETREF
		{
			const UBOOL bReferenced = ParseCommand( &Str, TEXT("1") );
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectKismetReferencedActors") );
			edactSelectKismetReferencedActors( bReferenced );
			return TRUE;
		}
		else
		{
			// Get actor name.
			FName ActorName(NAME_None);
			if ( Parse( Str, TEXT("NAME="), ActorName ) )
			{
				AActor* Actor = FindObject<AActor>( GWorld->CurrentLevel, *ActorName.ToString() );
				const FScopedTransaction Transaction( *LocalizeUnrealEd("SelectToggleSingleActor") );
				SelectActor( Actor, !(Actor && Actor->IsSelected()), FALSE, TRUE );
			}
			return TRUE;
		}
	}
	else if( ParseCommand(&Str,TEXT("DELETE")) )		// ACTOR SELECT DELETE
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd("DeleteActors") );
		edactDeleteSelected();
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("UPDATE")) )		// ACTOR SELECT UPDATE
	{
		for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			Actor->PreEditChange(NULL);
			Actor->PostEditChange(NULL);
		}
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("SET")) )
	{
		// @todo DB: deprecate the ACTOR SET exec.
		RedrawLevelEditingViewports();
		GCallbackEvent->Send( CALLBACK_RefreshEditor_LevelBrowser );
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("RESET")) )
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd("ResetActors") );

		UBOOL Location=0;
		UBOOL Pivot=0;
		UBOOL Rotation=0;
		UBOOL Scale=0;
		if( ParseCommand(&Str,TEXT("LOCATION")) )
		{
			Location=1;
			ResetPivot();
		}
		else if( ParseCommand(&Str, TEXT("PIVOT")) )
		{
			Pivot=1;
			ResetPivot();
		}
		else if( ParseCommand(&Str,TEXT("ROTATION")) )
		{
			Rotation=1;
		}
		else if( ParseCommand(&Str,TEXT("SCALE")) )
		{
			Scale=1;
		}
		else if( ParseCommand(&Str,TEXT("ALL")) )
		{
			Location=Rotation=Scale=1;
			ResetPivot();
		}

		// Fires CALLBACK_LevelDirtied when falling out of scope.
		FScopedLevelDirtied		LevelDirtyCallback;

		for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			Actor->PreEditChange(NULL);
			Actor->Modify();

			if( Location ) 
			{
				Actor->Location  = FVector(0.f,0.f,0.f);
			}
			if( Location ) 
			{
				Actor->PrePivot  = FVector(0.f,0.f,0.f);
			}
			if( Pivot && Actor->IsABrush() )
			{
				ABrush* Brush = (ABrush*)(Actor);
				Brush->Location -= Brush->PrePivot;
				Brush->PrePivot = FVector(0.f,0.f,0.f);
				Brush->PostEditChange(NULL);
			}
			if( Scale ) 
			{
				Actor->DrawScale = 1.0f;
			}

			Actor->MarkPackageDirty();
			LevelDirtyCallback.Request();
		}

		RedrawLevelEditingViewports();
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("DUPLICATE")) )
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd("DuplicateActors") );

		// if not specially handled by the current editing mode,
		if (!GEditorModeTools().GetCurrentMode()->HandleDuplicate())
		{
			// duplicate selected
			edactDuplicateSelected(TRUE);
		}
		RedrawLevelEditingViewports();
		return TRUE;
	}
	else if( ParseCommand(&Str, TEXT("ALIGN")) )
	{
		if( ParseCommand(&Str,TEXT("SNAPTOFLOOR")) )
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("SnapActorsToFloor") );

			UBOOL bAlign=0;
			ParseUBOOL( Str, TEXT("ALIGN="), bAlign );

			// Fires CALLBACK_LevelDirtied when falling out of scope.
			FScopedLevelDirtied		LevelDirtyCallback;

			for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
			{
				AActor* Actor = static_cast<AActor*>( *It );
				checkSlow( Actor->IsA(AActor::StaticClass()) );

				Actor->Modify();
				MoveActorToFloor(Actor,bAlign);
				Actor->InvalidateLightingCache();
				Actor->ForceUpdateComponents();

				Actor->MarkPackageDirty();
				LevelDirtyCallback.Request();
			}

			AActor* Actor = GetSelectedActors()->GetTop<AActor>();
			if( Actor )
			{
				SetPivot( Actor->Location, 0, 0, 1 );
			}

			RedrawLevelEditingViewports();
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("MOVETOGRID")) )
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("MoveActorToGrid") );

			// Update the pivot location.
			const FVector OldPivot = GetPivotLocation();
			const FVector NewPivot = OldPivot.GridSnap(Constraints.GetGridSize());
			const FVector Delta = NewPivot - OldPivot;

			SetPivot( NewPivot, 0, 0, 1 );


			// Fires CALLBACK_LevelDirtied when falling out of scope.
			FScopedLevelDirtied		LevelDirtyCallback;

			for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
			{
				AActor* Actor = static_cast<AActor*>( *It );
				checkSlow( Actor->IsA(AActor::StaticClass()) );

				Actor->Modify();

				const FVector OldLocation = Actor->Location;

				GWorld->FarMoveActor( Actor, OldLocation+Delta, FALSE, FALSE, TRUE );
				Actor->InvalidateLightingCache();
				Actor->ForceUpdateComponents();

				Actor->MarkPackageDirty();
				LevelDirtyCallback.Request();
			}

			RedrawLevelEditingViewports();
			return TRUE;
		}
		else
		{
			const FScopedTransaction Transaction( *LocalizeUnrealEd("AlignBrushVertices") );
			edactAlignVertices();
			RedrawLevelEditingViewports();
			return TRUE;
		}
	}
	else if( ParseCommand(&Str,TEXT("TOGGLE")) )
	{
		if( ParseCommand(&Str,TEXT("LOCKMOVEMENT")) )			// ACTOR TOGGLE LOCKMOVEMENT
		{
			// Fires CALLBACK_LevelDirtied when falling out of scope.
			FScopedLevelDirtied		LevelDirtyCallback;

			for ( FSelectionIterator It( GetSelectedActorIterator() ) ; It ; ++It )
			{
				AActor* Actor = static_cast<AActor*>( *It );
				checkSlow( Actor->IsA(AActor::StaticClass()) );

				Actor->Modify();
				Actor->bLockLocation = !Actor->bLockLocation;

				Actor->MarkPackageDirty();
				LevelDirtyCallback.Request();
			}
		}

		RedrawLevelEditingViewports();
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("LEVELCURRENT")) )
	{
		MakeSelectedActorsLevelCurrent();
		return TRUE;
	}
	else if( ParseCommand(&Str,TEXT("MOVETOCURRENT")) )
	{
		MoveSelectedActorsToCurrentLevel();
		return TRUE;
	}

	return FALSE;
}


UBOOL UUnrealEdEngine::Exec_Mode( const TCHAR* Str, FOutputDevice& Ar )
{
	Word1 = GEditorModeTools().GetCurrentModeID();  // To see if we should redraw
	Word2 = GEditorModeTools().GetCurrentModeID();  // Destination mode to set

	UBOOL DWord1;
	if( ParseCommand(&Str,TEXT("WIDGETCOORDSYSTEMCYCLE")) )
	{
		INT Wk = GEditorModeTools().CoordSystem;
		Wk++;

		if( Wk == COORD_Max )
		{
			Wk -= COORD_Max;
		}
		GEditorModeTools().CoordSystem = (ECoordSystem)Wk;
		GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
		GCallbackEvent->Send( CALLBACK_UpdateUI );
	}
	if( ParseCommand(&Str,TEXT("WIDGETMODECYCLE")) )
	{
		if( GEditorModeTools().GetShowWidget() )
		{
			const INT CurrentWk = GEditorModeTools().GetWidgetMode();
			INT Wk = CurrentWk;
			// don't allow cycling through non uniform scaling
			const INT MaxWidget = (INT)FWidget::WM_ScaleNonUniform;
			do
			{
				Wk++;
				// Roll back to the start if we go past FWidget::WM_Scale
				if( Wk >= MaxWidget )
				{
					Wk -= MaxWidget;
				}
			}
			while (!GEditorModeTools().GetCurrentMode()->UsesWidget((FWidget::EWidgetMode)Wk) && Wk != CurrentWk);
			GEditorModeTools().SetWidgetMode( (FWidget::EWidgetMode)Wk );
			GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
		}
	}
	if( ParseUBOOL(Str,TEXT("GRID="), DWord1) )
	{
		FinishAllSnaps();
		Constraints.GridEnabled = DWord1;
		Word1=MAXWORD;
		GCallbackEvent->Send( CALLBACK_UpdateUI );
	}
	if( ParseUBOOL(Str,TEXT("ROTGRID="), DWord1) )
	{
		FinishAllSnaps();
		Constraints.RotGridEnabled=DWord1;
		Word1=MAXWORD;
		GCallbackEvent->Send( CALLBACK_UpdateUI );
	}
	if( ParseUBOOL(Str,TEXT("SNAPVERTEX="), DWord1) )
	{
		FinishAllSnaps();
		Constraints.SnapVertices=DWord1;
		Word1=MAXWORD;
		GCallbackEvent->Send( CALLBACK_UpdateUI );
	}
	if( ParseUBOOL(Str,TEXT("ALWAYSSHOWTERRAIN="), DWord1) )
	{
		FinishAllSnaps();
		AlwaysShowTerrain=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("USEACTORROTATIONGIZMO="), DWord1) )
	{
		FinishAllSnaps();
		UseActorRotationGizmo=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("SHOWBRUSHMARKERPOLYS="), DWord1) )
	{
		FinishAllSnaps();
		bShowBrushMarkerPolys=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("SELECTIONLOCK="), DWord1) )
	{
		FinishAllSnaps();
		// If -1 is passed in, treat it as a toggle.  Otherwise, use the value as a literal assignment.
		if( DWord1 == -1 )
			GEdSelectionLock=(GEdSelectionLock == 0) ? 1 : 0;
		else
			GEdSelectionLock=DWord1;
		Word1=MAXWORD;
	}
	Parse(Str,TEXT("MAPEXT="), GApp->MapExt);
	if( ParseUBOOL(Str,TEXT("USESIZINGBOX="), DWord1) )
	{
		FinishAllSnaps();
		// If -1 is passed in, treat it as a toggle.  Otherwise, use the value as a literal assignment.
		if( DWord1 == -1 )
			UseSizingBox=(UseSizingBox == 0) ? 1 : 0;
		else
			UseSizingBox=DWord1;
		Word1=MAXWORD;
	}
	Parse( Str, TEXT("SPEED="), MovementSpeed );
	Parse( Str, TEXT("SNAPDIST="), Constraints.SnapDistance );
	//
	// Major modes:
	//
	if 		(ParseCommand(&Str,TEXT("CAMERAMOVE")))		Word2 = EM_Default;
	else if (ParseCommand(&Str,TEXT("TERRAINEDIT")))	Word2 = EM_TerrainEdit;
	else if	(ParseCommand(&Str,TEXT("GEOMETRY"))) 		Word2 = EM_Geometry;
	else if	(ParseCommand(&Str,TEXT("TEXTURE"))) 		Word2 = EM_Texture;
	else if (ParseCommand(&Str,TEXT("COVEREDIT")))		Word2 = EM_CoverEdit;

	if( Word2 != Word1 )
		GCallbackEvent->Send( CALLBACK_ChangeEditorMode, Word2 );

	// Reset the roll on all viewport cameras
	for(UINT ViewportIndex = 0;ViewportIndex < (UINT)ViewportClients.Num();ViewportIndex++)
	{
		if(ViewportClients(ViewportIndex)->ViewportType == LVT_Perspective)
			ViewportClients(ViewportIndex)->ViewRotation.Roll = 0;
	}

	GCallbackEvent->Send( CALLBACK_RedrawAllViewports );

	return TRUE;
}
