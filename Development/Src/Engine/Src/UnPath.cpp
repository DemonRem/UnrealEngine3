/*=============================================================================
	UnPath.cpp: Unreal pathnode placement
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 =============================================================================*/

#include "EnginePrivate.h"
#include "UnPath.h"
#include "UnTerrain.h"
#include "EngineAIClasses.h"
#include "EngineMaterialClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineSequenceClasses.h"
#include "DebugRenderSceneProxy.h"

const FOctreeNodeBounds FNavigationOctree::RootNodeBounds(FVector(0,0,0),HALF_WORLD_MAX);
/** maximum objects we can have in one node before we split it */
#define MAX_OBJECTS_PER_NODE 10

/**
 * Removes paths from all navigation points in the world, and removes all
 * path markers from actors in the world.
 */
void AScout::UndefinePaths()
{
	debugfSuppressed(NAME_DevPath,TEXT("Remove old reachspecs"));
	GWarn->BeginSlowTask( TEXT("Undefining Paths"), 1);
	//clear NavigationPoints
	const INT ProgressDenominator = FActorIteratorBase::GetProgressDenominator();
	const FString LocalizeUndefing( LocalizeUnrealEd(TEXT("Undefining")) );
	for( FActorIterator It; It; ++It )
	{
		GWarn->StatusUpdatef( It.GetProgressNumerator(), ProgressDenominator, *LocalizeUndefing );
		AActor*	Actor = *It;
		ANavigationPoint *Nav = Cast<ANavigationPoint>(Actor);
		if (Nav != NULL)
		{
			if (!(Nav->GetClass()->ClassFlags & CLASS_Placeable))
			{
				/* delete any nodes which aren't placeable, because they were automatically generated,
				  and will be automatically generated again. */
				GWorld->DestroyActor(Nav);
			}
			else
			{
				// reset the nav network id for this nav
				Nav->NetworkID = -1;
				// and clear any previous paths
				Nav->ClearPaths();
			}
		}
		else
		{
			Actor->ClearMarker();
		}
	}
	if ( GWorld->GetWorldInfo()->bPathsRebuilt )
	{
		debugf(TEXT("undefinepaths Clear paths rebuilt"));
	}
	GWorld->GetWorldInfo()->bPathsRebuilt = FALSE;
	GWarn->EndSlowTask();
}

void AScout::ReviewPaths()
{
	GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("ReviewingPathsE")), 1 );

	if ( !GWorld->GetFirstNavigationPoint() )
	{
		GWarn->MapCheck_Add( MCTYPE_ERROR, NULL, TEXT("No navigation point list. Paths define needed."), MCACTION_NONE, TEXT("NoNavigationPoints"));
	}
	else
	{
		INT NumDone = 0;
		INT NumPaths = 0;
		INT NumStarts = 0;
		for ( ANavigationPoint* N=GWorld->GetFirstNavigationPoint(); N!=NULL; N=N->nextNavigationPoint )
		{
			if ( Cast<APlayerStart>(N) )
				NumStarts++;
			NumPaths++;
		}
		
		if ( NumStarts < MinNumPlayerStarts )
			GWarn->MapCheck_Add( MCTYPE_ERROR, GWorld->GetFirstNavigationPoint(), *FString::Printf(TEXT("Only %d PlayerStarts in this level (need at least %d)"),NumStarts,MinNumPlayerStarts), MCACTION_NONE, TEXT("NeedMorePlayerstarts"));

		SetPathCollision(TRUE);
		const FString LocalizeReviewingPaths( LocalizeUnrealEd(TEXT("ReviewingPaths")) );
		for ( ANavigationPoint* N=GWorld->GetFirstNavigationPoint(); N!=NULL; N=N->nextNavigationPoint )
		{
			GWarn->StatusUpdatef( NumDone, NumPaths, *LocalizeReviewingPaths );
			N->ReviewPath(this);
			NumDone++;
		}
		SetPathCollision(FALSE);
	}
	GWarn->EndSlowTask();
}

void AScout::SetPathCollision(UBOOL bEnabled)
{
	if ( bEnabled )
	{
		for( FActorIterator It; It; ++It )
		{
			AActor *Actor = *It;
			// turn off collision - for non-static actors with bPathColliding false
			if( !Actor->bDeleteMe )
			{
				if (Actor->bBlockActors && Actor->bCollideActors && !Actor->bStatic && !Actor->bPathColliding )
				{
					Actor->bPathTemp = TRUE;
					Actor->SetCollision( FALSE, Actor->bBlockActors, Actor->bIgnoreEncroachers );
				}
				else
				{
					Actor->bPathTemp = FALSE;
				}
			}
		}
	}
	else
	{
		for( FActorIterator It; It; ++It )
		{
			AActor *Actor = *It;
			if( Actor->bPathTemp )
			{
				Actor->bPathTemp = FALSE;
				Actor->SetCollision( TRUE, Actor->bBlockActors, Actor->bIgnoreEncroachers );
			}
		}
	}
}

void AScout::PrunePaths(INT NumPaths)
{
	// first, check for unnecessary PathNodes
	INT NumDone = 0;
	const FString LocalizeCheckingForUnnecessaryPathNodes( LocalizeUnrealEd(TEXT("CheckingForUnnecessaryPathNodes")) );
	for (ANavigationPoint* Nav = GWorld->GetFirstNavigationPoint(); Nav != NULL; Nav = Nav->nextNavigationPoint)
	{
		GWarn->StatusUpdatef( NumDone, NumPaths, *LocalizeCheckingForUnnecessaryPathNodes );

		if (Nav->GetClass() == APathNode::StaticClass() && Nav->PathList.Num() > 0)
		{
			// if the PathNode is connected only to other nodes that have paths that encompass it, then it is unnecessary
			UBOOL bNecessary = TRUE;
			for (INT i = 0; i < Nav->PathList.Num(); i++)
			{	
				// if Nav is not designed for flying Pawns, ignore end nodes that are
				if( *Nav->PathList(i)->End != NULL && (Nav->bFlyingPreferred || !Nav->PathList(i)->End->bFlyingPreferred))
				{
					bNecessary = FALSE;
					UBOOL bOverlapsNav = FALSE;
					for (INT j = 0; j < Nav->PathList(i)->End->PathList.Num(); j++)
					{
						UReachSpec* Spec = Nav->PathList(i)->End->PathList(j);

						if (Spec != NULL && *Spec->End != NULL)
						{
							if (Spec->bAddToNavigationOctree && Spec->End != Nav && Spec->IsOnPath(Nav->Location, Nav->CylinderComponent->CollisionRadius))
							{
								bOverlapsNav = TRUE;
								break;
							}
						}
					}

					if (!bOverlapsNav)
					{
						bNecessary = TRUE;
						break;
					}
				}
			}
			if (!bNecessary)
			{
				GWarn->MapCheck_Add(MCTYPE_WARNING, Nav, *FString::Printf(TEXT("%s is unnecessary and should be removed"), *Nav->GetName()), MCACTION_NONE, TEXT("UselessPathNode"));
			}
		}
		NumDone++;
	}

	// prune excess reachspecs
	debugfSuppressed(NAME_DevPath,TEXT("Prune reachspecs"));
	INT NumPruned = 0;
	NumDone = 0;
	const FString LocalizePruningReachspecs( LocalizeUnrealEd(TEXT("PruningReachspecs")) );
	for (ANavigationPoint* Nav = GWorld->GetFirstNavigationPoint(); Nav != NULL; Nav = Nav->nextNavigationPoint)
	{
		GWarn->StatusUpdatef( NumDone, NumPaths, *LocalizePruningReachspecs );
		NumPruned += Nav->PrunePaths();
		NumDone++;
	}
	debugfSuppressed(NAME_DevPath,TEXT("Pruned %d reachspecs"), NumPruned);
}

void AScout::UpdateInterpActors(UBOOL &bProblemsMoving, TArray<USeqAct_Interp*> &InterpActs)
{
	bProblemsMoving = FALSE;
	// Keep track of which actors we are updating - so we can check for multiple actions updating the same actor.
	TArray<AActor*> MovedActors;
	for (TObjectIterator<USeqAct_Interp> InterpIt; InterpIt; ++InterpIt)
	{
		// For each SeqAct_Interp we flagged as having to be updated for pathing...
		USeqAct_Interp* Interp = *InterpIt;
		if(Interp->bInterpForPathBuilding)
		{
			InterpActs.AddItem(Interp);
			// Initialise it..
			Interp->InitInterp();
			for(INT j=0; j<Interp->GroupInst.Num(); j++)
			{
				// Check if any of the actors its working on have already been affected..
				AActor* InterpActor = Interp->GroupInst(j)->GroupActor;
				if(InterpActor)
				{
					if( MovedActors.ContainsItem(InterpActor) )
					{
						// This Actor is being manipulated by another SeqAct_Interp - so bail out.
						appMsgf(AMT_OK, *LocalizeUnrealEd("Error_MultipleInterpsReferToSameActor"), *InterpActor->GetName());
						bProblemsMoving = true;
					}
					else
					{
						// Things are ok - just add it to the list.
						MovedActors.AddItem(InterpActor);
					}
				}
			}

			if(!bProblemsMoving)
			{
				// If there were no problem initialising, first backup the current state of the actors..
				for(INT i=0; i<Interp->GroupInst.Num(); i++)
				{
					Interp->GroupInst(i)->SaveGroupActorState();
					AActor* GroupActor = Interp->GroupInst(i)->GetGroupActor();
					if( GroupActor )
					{
						Interp->SaveActorTransforms( GroupActor );
					}
				}

				// ..then interpolate them to the 'PathBuildTime'.
				Interp->UpdateInterp( Interp->InterpData->PathBuildTime, true );
			}
			else
			{
				// If there were problems, term this interp now (so we won't try and restore its state later on).
				Interp->TermInterp();
			}
		}
	}
}

void AScout::RestoreInterpActors(TArray<USeqAct_Interp*> &InterpActs)
{
	// Reset any interpolated actors.
	const FString LocalizeBuildPathsResettingActors( LocalizeUnrealEd(TEXT("BuildPathsResettingActors")) );
	for(INT i=0; i<InterpActs.Num(); i++)
	{
		GWarn->StatusUpdatef( i, InterpActs.Num(), *LocalizeBuildPathsResettingActors );

		USeqAct_Interp* Interp = CastChecked<USeqAct_Interp>( InterpActs(i) );

		if(Interp->bInterpForPathBuilding)
		{
			for(INT j=0; j<Interp->GroupInst.Num(); j++)
			{
				Interp->GroupInst(j)->RestoreGroupActorState();
			}

			Interp->RestoreActorTransforms();
			Interp->TermInterp();
			Interp->Position = 0.f;
		}
	}
}

void AScout::BuildNavLists()
{
	// build the per-level nav lists
	UBOOL bBuildCancelled = GEngine->GetMapBuildCancelled();
	if (!bBuildCancelled)
	{
		TArray<FGuid> NavGuids;
		// reset the world's nav list
		GWorld->ResetNavList();
		// for each level,
		const FString LocalizeBuildPathsNavigationPointsOnBases( LocalizeUnrealEd(TEXT("BuildPathsNavigationPointsOnBases")) );
		for ( INT LevelIdx = 0; LevelIdx < GWorld->Levels.Num(); LevelIdx++ )
		{
			ULevel *Level = GWorld->Levels(LevelIdx);
			if (Level != NULL)
			{
				Level->ResetNavList();
				UBOOL bHasPathNodes = FALSE;
				// look for any path nodes in this level
				for (INT ActorIdx = 0; ActorIdx < Level->Actors.Num() && !bBuildCancelled; ActorIdx++)
				{
					ANavigationPoint *Nav = Cast<ANavigationPoint>(Level->Actors(ActorIdx));
					if (Nav != NULL && !Nav->IsPendingKill())
					{
						// note that the level does have pathnodes
						bHasPathNodes = TRUE;
						// check for invalid cylinders
						if (Nav->CylinderComponent == NULL)
						{
							GWarn->MapCheck_Add(MCTYPE_WARNING, Nav, *FString::Printf(TEXT("%s doesn't have a cylinder component!"),*Nav->GetName()), MCACTION_NONE, TEXT("CylinderComponentNull"));
						}
						else
						{
							// add to the level path list
							Level->AddToNavList(Nav);
							// base the nav points as well
							Nav->FindBase();
							// init the nav for pathfinding
							Nav->InitForPathFinding();
							Nav->bHasCrossLevelPaths = FALSE;
							// and generate a guid for it as well
							if (!Nav->NavGuid.IsValid() || NavGuids.ContainsItem(Nav->NavGuid))
							{
								Nav->NavGuid = appCreateGuid();
							}
							else
							{
								// save the existing guid to check for duplicates
								NavGuids.AddItem(Nav->NavGuid);
							}
						}
					}
					bBuildCancelled = GEngine->GetMapBuildCancelled();
				}
				GWarn->StatusUpdatef( LevelIdx, GWorld->Levels.Num(), *LocalizeBuildPathsNavigationPointsOnBases );
				// if there are any nodes, 
				if (bHasPathNodes)
				{
					// mark the level as dirty so that it will be saved
					Level->MarkPackageDirty();
					// and add to the world nav list
					GWorld->AddLevelNavList(Level);
				}
			}
		}
	}
}

/**
 * Clears all the paths and rebuilds them.
 *
 * @param	bReviewPaths	If TRUE, review paths if any were created.
 * @param	bShowMapCheck	If TRUE, conditionally show the Map Check dialog.
 */
void AScout::DefinePaths(UBOOL bReviewPaths, UBOOL bShowMapCheck)
{
	GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("DefiningPaths")), TRUE );

	// Build Terrain Collision Data
	for (TObjectIterator<UTerrainComponent> TerrainIt; TerrainIt; ++TerrainIt)
	{
		TerrainIt->BuildCollisionData();
	}

	// remove old paths
	UndefinePaths();

	// Position interpolated actors in desired locations for path-building.
	TArray<USeqAct_Interp*> InterpActs;
	UBOOL bProblemsMoving = FALSE;
	UpdateInterpActors(bProblemsMoving,InterpActs);
	UBOOL bBuildCancelled = GEngine->GetMapBuildCancelled();
	if (!bProblemsMoving && !bBuildCancelled)
	{
		// Setup all actor collision for path building
		SetPathCollision(TRUE);
		// number of paths, used for progress bar
		INT NumPaths = 0; 
		// Add NavigationPoint markers to any actors which want to be marked
		INT ProgressDenominator = FActorIteratorBase::GetProgressDenominator();
		SetCollision( FALSE, FALSE, bIgnoreEncroachers );
		const FString LocalizeBuildPathsDefining( LocalizeUnrealEd(TEXT("BuildPathsDefining")) );
		for( FActorIterator It; It && !bBuildCancelled; ++It )
		{
			GWarn->StatusUpdatef( It.GetProgressNumerator(), ProgressDenominator, *LocalizeBuildPathsDefining );
			AActor *Actor = *It;
			NumPaths += Actor->AddMyMarker(this);

			bBuildCancelled = GEngine->GetMapBuildCancelled();
		}
		// build the navigation lists
		BuildNavLists();
		// setup the scout
		SetCollision(TRUE, TRUE, bIgnoreEncroachers);

		// Adjust cover
		if( !bBuildCancelled )
		{
			Exec( TEXT("ADJUSTCOVER FROMDEFINEPATHS=TRUE") );
		}

		// calculate and add reachspecs to pathnodes
		debugfSuppressed(NAME_DevPath,TEXT("Add reachspecs"));
		INT NumDone = 0;
		const FString LocalizeBuildPathsAddingReachspecs( LocalizeUnrealEd(TEXT("BuildPathsAddingReachspecs")) );
		for( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav && !bBuildCancelled; Nav=Nav->nextNavigationPoint )
		{
			GWarn->StatusUpdatef( NumDone, NumPaths, *LocalizeBuildPathsAddingReachspecs );
			Nav->addReachSpecs(this,FALSE);
			debugfSuppressed( NAME_DevPath, TEXT("Added reachspecs to %s"),*Nav->GetName() );
			NumDone++;
			bBuildCancelled = GEngine->GetMapBuildCancelled();
		}

		// Called to eliminate paths so computing long paths 
		// doesn't bother iterating over specs that will be deleted anyway
		if( !bBuildCancelled )
		{
			PrunePaths(NumPaths);
		}

		Exec( *FString::Printf(TEXT("ADDLONGREACHSPECS NUMPATHS=%d"), NumPaths) );

		// allow scout to add any game specific special reachspecs
		if (!bBuildCancelled)
		{
			debugf(TEXT("%s add special paths"),*GetName());
			AddSpecialPaths(NumPaths, FALSE);
		}
		// turn off collision and reset temporarily changed actors
		SetPathCollision( FALSE );

		// Add forced specs if needed
		GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("AddingForcedSpecs")), TRUE );
		{
			INT NavigationPointCount = 0;
			const FString LocalizeBuildPathsAddingForcedSpecsFE( LocalizeUnrealEd(TEXT("BuildPathsAddingForcedSpecsFE")) );
			for( ANavigationPoint* Nav = GWorld->GetFirstNavigationPoint(); Nav && !bBuildCancelled; Nav = Nav->nextNavigationPoint )
			{
				Nav->AddForcedSpecs( this );

				// Call prune paths so MaxPathSize can be updated
				Nav->PrunePaths();

				GWarn->StatusUpdatef(NavigationPointCount, NumPaths, *LocalizeBuildPathsAddingForcedSpecsFE, NavigationPointCount);

				bBuildCancelled = GEngine->GetMapBuildCancelled();
				NavigationPointCount++;
			}

		}
		GWarn->EndSlowTask();

		if( !bBuildCancelled )
		{
			INT NavigationPointCount = 0;

			ANavigationPoint::BuildNetworkIDs();

			// sort PathLists
			// clear pathschanged flags and remove bases if in other level
			// handle specs crossing levels
			const FString LocalizeBuildPathsCheckingCrossLevelReachspecsFE( LocalizeUnrealEd(TEXT("BuildPathsCheckingCrossLevelReachspecsFE")) );
			for( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav && !bBuildCancelled; Nav=Nav->nextNavigationPoint )
			{
				Nav->SortPathList();
				Nav->bPathsChanged = FALSE;
				Nav->PostEditChange(NULL);
				GWarn->StatusUpdatef( NavigationPointCount, NumPaths, *LocalizeBuildPathsCheckingCrossLevelReachspecsFE, NavigationPointCount );
				bBuildCancelled = GEngine->GetMapBuildCancelled();
				NavigationPointCount++;
			}

			if (!bBuildCancelled)
			{
				GWorld->GetWorldInfo()->bPathsRebuilt = TRUE;
				debugf(TEXT("SET paths rebuilt"));
			}
		}
	}
	// reset the interp actors moved for path building
	RestoreInterpActors(InterpActs);
	
	GWarn->EndSlowTask();
	// Don't check for errors if we didn't bother building paths!
	if( !bProblemsMoving && GWorld->GetFirstNavigationPoint() )
	{
		INT ProgressDenominator = FActorIteratorBase::GetProgressDenominator();
		const FString LocalizeBuildPathsCheckingActorsForErrors( LocalizeUnrealEd(TEXT("BuildPathsCheckingActorsForErrors")) );
		for( FActorIterator It; It && !bBuildCancelled; ++It )
		{
			GWarn->StatusUpdatef( It.GetProgressNumerator(), ProgressDenominator, *LocalizeBuildPathsCheckingActorsForErrors );
			AActor *Actor = *It;
			Actor->CheckForErrors();
			bBuildCancelled = GEngine->GetMapBuildCancelled();
		}
	}
	// if the build was cancelled then invalidate paths
	if( bBuildCancelled )
	{
		UndefinePaths();
	}
	else
	{
		debugfSuppressed(NAME_DevPath,TEXT("All done"));
		if ( bReviewPaths && GWorld->GetFirstNavigationPoint() )
		{
			ReviewPaths();
		}
		if ( bShowMapCheck )
		{
			GWarn->MapCheck_ShowConditionally();
		}
	}
}


//------------------------------------------------------------------------------------------------
// certain actors add paths to mark their position
INT ANavigationPoint::AddMyMarker(AActor *S)
{
	return 1;
}

INT APathNode::AddMyMarker(AActor *S)
{
	GWorld->GetWorldInfo()->bHasPathNodes = true;
	return 1;
}

FVector ALadderVolume::FindCenter()
{
	FVector Center(0.f,0.f,0.f);
	check(BrushComponent);
	check(Brush);
	for(INT PolygonIndex = 0;PolygonIndex < Brush->Polys->Element.Num();PolygonIndex++)
	{
		FPoly&	Poly = Brush->Polys->Element(PolygonIndex);
		FVector NewPart(0.f,0.f,0.f);
		for(INT VertexIndex = 0;VertexIndex < Poly.Vertices.Num();VertexIndex++)
			NewPart += Poly.Vertices(VertexIndex);
		NewPart /= Poly.Vertices.Num();
		Center += NewPart;
	}
	Center /= Brush->Polys->Element.Num();
	return Center;
}

INT ALadderVolume::AddMyMarker(AActor *S)
{
	check(BrushComponent);

	if ( !bAutoPath || !Brush )
		return 0;

	FVector Center = FindCenter();
	Center = LocalToWorld().TransformFVector(Center);

	AScout* Scout = Cast<AScout>(S);
	if ( !Scout )
		return 0 ;

	UClass *pathClass = AAutoLadder::StaticClass();
	AAutoLadder* DefaultLadder = (AAutoLadder*)( pathClass->GetDefaultActor() );

	// find ladder bottom
	FCheckResult Hit(1.f);
	GWorld->SingleLineCheck(Hit, this, Center - 10000.f * ClimbDir, Center, TRACE_World);
	if ( Hit.Time == 1.f )
		return 0;
	FVector Position = Hit.Location + DefaultLadder->CylinderComponent->CollisionHeight * ClimbDir;

	// place Ladder at bottom of volume
	GWorld->SpawnActor(pathClass, NAME_None, Position);

	// place Ladder at top of volume + 0.5 * CollisionHeight of Ladder
	Position = FindTop(Center + 500.f * ClimbDir);
	GWorld->SpawnActor(pathClass, NAME_None, Position - 5.f * ClimbDir);
	return 2;
}

// find the edge of the brush in the ClimbDir direction
FVector ALadderVolume::FindTop(FVector V)
{
	if ( Encompasses(V) )
		return FindTop(V + 500.f * ClimbDir);

	// trace back to brush edge from this outside point
	FCheckResult Hit(1.f);
	check(BrushComponent);
	BrushComponent->LineCheck( Hit, V - 10000.f * ClimbDir, V, FVector(0.f,0.f,0.f), 0 );
	return Hit.Location;

}

//------------------------------------------------------------------------------------------------
//Private methods

AScout* FPathBuilder::Scout = NULL;

void FPathBuilder::DestroyScout()
{
	for (FActorIterator It; It; ++It)
	{
		AScout* TheScout = Cast<AScout>(*It);
		if (TheScout != NULL)
		{
			if (TheScout->Controller != NULL)
			{
				GWorld->DestroyActor(TheScout->Controller);
			}
			GWorld->DestroyActor(TheScout);
		}
	}

	FPathBuilder::Scout = NULL;
}

/*getScout()
Find the scout actor in the level. If none exists, add one.
*/
AScout* FPathBuilder::GetScout()
{
	AScout* NewScout = FPathBuilder::Scout;
	if (NewScout == NULL || NewScout->IsPendingKill())
	{
		NewScout = NULL;
		FString ScoutClassName = GEngine->ScoutClassName;
		UClass *ScoutClass = FindObject<UClass>(ANY_PACKAGE, *ScoutClassName);
		if (ScoutClass == NULL)
		{
			appErrorf(TEXT("Failed to find scout class for path building!"));
		}
		// search for an existing scout
		for( FActorIterator It; It && NewScout == NULL; ++It )
		{
			if (It->IsA(ScoutClass))
			{
				NewScout = Cast<AScout>(*It);
			}
		}
		// if one wasn't found create a new one
		if (NewScout == NULL)
		{
			NewScout = (AScout*)GWorld->SpawnActor(ScoutClass);
			check(NewScout != NULL);
			// set the scout as transient so it can't get accidentally saved in the map
			NewScout->SetFlags(RF_Transient);
			// give it a default controller
			NewScout->Controller = (AController*)GWorld->SpawnActor(FindObjectChecked<UClass>(ANY_PACKAGE, TEXT("AIController")));
			check(NewScout->Controller != NULL);
			NewScout->Controller->SetFlags(RF_Transient);
		}
		if (NewScout != NULL)
		{
			// initialize scout collision properties
			NewScout->SetCollision( TRUE, TRUE, NewScout->bIgnoreEncroachers );
			NewScout->bCollideWorld = 1;
			NewScout->SetZone( 1,1 );
			NewScout->PhysicsVolume = GWorld->GetWorldInfo()->GetDefaultPhysicsVolume();
			NewScout->SetVolumes();
			NewScout->bHiddenEd = 1;
			NewScout->SetPrototype();

		}
	}
	return NewScout;
}

void FPathBuilder::Exec( const TCHAR* Str )
{
	Scout = GetScout();
	if( Scout )
	{
		Scout->Exec( Str );
	}
	DestroyScout();
}

void AScout::Exec( const TCHAR* Str )
{
	if( ParseCommand( &Str, TEXT("DEFINEPATHS") ) )
	{
		UBOOL bReviewPaths	= FALSE;
		UBOOL bShowMapCheck = FALSE;

		ParseUBOOL( Str, TEXT("REVIEWPATHS="),  bReviewPaths  );
		ParseUBOOL( Str, TEXT("SHOWMAPCHECK="), bShowMapCheck );

		DefinePaths( bReviewPaths, bShowMapCheck );
	}
	else
	if( ParseCommand( &Str, TEXT("SETPATHCOLLISION") ) )
	{
		UBOOL bEnabled = FALSE;

		ParseUBOOL( Str, TEXT("ENABLED="), bEnabled );

		SetPathCollision( bEnabled );
	}
	else
	if( ParseCommand( &Str, TEXT("ADDLONGREACHSPECS") ) )
	{
		INT NumPaths = 0;
		Parse( Str, TEXT("NUMPATHS="), NumPaths );
		AddLongReachSpecs( NumPaths );
	}	
}

/** Do a second pass to add long range reachspecs */
void AScout::AddLongReachSpecs( INT NumPaths )
{
	debugfSuppressed(NAME_DevPath, TEXT("Add long range reachspecs"));
	INT NumDone = 0;
	UBOOL bBuildCancelled = GEngine->GetMapBuildCancelled();
	const FString LocalizeBuildPathsAddingLongRangeReachspecs( LocalizeUnrealEd(TEXT("BuildPathsAddingLongRangeReachspecs")) );
	for( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav && !bBuildCancelled; Nav=Nav->nextNavigationPoint )
	{
		GWarn->StatusUpdatef( NumDone, NumPaths, *LocalizeBuildPathsAddingLongRangeReachspecs );
		bBuildCancelled = GEngine->GetMapBuildCancelled();
		Nav->AddLongPaths(this, FALSE);
		NumDone++;
	}

	// Called again to potentially prune new paths
	if( !bBuildCancelled )
	{
		PrunePaths(NumPaths);
	}
}

void AScout::PostBeginPlay()
{
	Super::PostBeginPlay();
	SetPrototype();
}

void AScout::SetPrototype()
{
}

FVector AScout::GetMaxSize()
{
	FLOAT MaxRadius = 0.f;
	FLOAT MaxHeight = 0.f;
	for (INT i = 0; i < PathSizes.Num(); i++)
	{
		MaxRadius = Max<FLOAT>(MaxRadius, PathSizes(i).Radius);
		MaxHeight = Max<FLOAT>(MaxHeight, PathSizes(i).Height);
	}
	return FVector(MaxRadius, MaxHeight, 0.f);
}

//=============================================================================
// UReachSpec

static void DrawLineArrow(FPrimitiveDrawInterface* PDI,const FVector &start,const FVector &end,const FColor &color,FLOAT mag)
{
	// draw a pretty arrow
	FVector dir = end - start;
	FLOAT dirMag = dir.Size();
	dir /= dirMag;
	FVector YAxis, ZAxis;
	dir.FindBestAxisVectors(YAxis,ZAxis);
	FMatrix arrowTM(dir,YAxis,ZAxis,start);
	DrawDirectionalArrow(PDI,arrowTM,color,dirMag,mag,SDPG_World);
}


/**
* Adds geometery for a UReachSpec object.
* @param ReachSpec		ReachSpec to add geometry for.
*/
void UReachSpec::AddToDebugRenderProxy(FDebugRenderSceneProxy* DRSP)
{
	if ( Start && *End && !End->IsPendingKill() )
	{
		FPlane PathColorValue = PathColor();
		new(DRSP->ArrowLines) FDebugRenderSceneProxy::FArrowLine(Start->Location, End->Location - FVector(0,0,-16), FLinearColor(PathColorValue.X,PathColorValue.Y,PathColorValue.Z,PathColorValue.W));
	}
	else
	{
		// check for connections across levels
		if ( GIsEditor && Start && End.Guid.IsValid() )
		{
			for (FActorIterator It; It; ++It)
			{
				ANavigationPoint *Nav = Cast<ANavigationPoint>(*It);
				if (Nav != NULL && Nav->NavGuid == End.Guid)
				{

					new(DRSP->ArrowLines) FDebugRenderSceneProxy::FArrowLine(Start->Location, Nav->Location - FVector(0,0,-16), FColor(128,128,128,255));

					if( GIsEditor &&
						Start->IsSelected() && 
						Nav->IsSelected() )
					{
						const INT Idx = DRSP->Cylinders.Add();
						FDebugRenderSceneProxy::FWireCylinder &Cylinder = DRSP->Cylinders(Idx);

						Cylinder.Base = Start->Location + FVector(0,0,(CollisionHeight - Start->CylinderComponent->CollisionHeight));
						Cylinder.Color = FColor(255,0,0);
						Cylinder.Radius = CollisionRadius;
						Cylinder.HalfHeight = CollisionHeight;
					}

					// no need to continue searching
					break;
				}
			}
		}
	}
}

void FNavigationOctreeObject::SetBox(const FBox& InBoundingBox)
{
	UBOOL bIsInOctree = (OctreeNode != NULL);
	if (bIsInOctree)
	{
		GWorld->NavigationOctree->RemoveObject(this);
	}
	BoundingBox = InBoundingBox;
	BoxCenter = BoundingBox.GetCenter();
	if (bIsInOctree)
	{
		GWorld->NavigationOctree->AddObject(this);
	}
}

UBOOL FNavigationOctreeObject::OverlapCheck(const FBox& TestBox)
{
	if (Owner != NULL && OwnerType & NAV_ReachSpec)
	{
		return ((UReachSpec*)Owner)->NavigationOverlapCheck(TestBox);
	}
	else
	{
		// the default is to only use the AABB check
		return false;
	}
}

/** destructor, removes us from the octree if we're still there */
FNavigationOctreeObject::~FNavigationOctreeObject()
{
	if (OctreeNode != NULL && GWorld != NULL && GWorld->NavigationOctree != NULL)
	{
		GWorld->NavigationOctree->RemoveObject(this);
	}
}

/** filters an object with the given bounding box through this node, either adding it or passing it to its children
 * if the object is added to this node, the node may also be split if it exceeds the maximum number of objects allowed for a node without children
 * assumes the bounding box fits in this node and always succeeds
 * @param Object the object to filter
 * @param NodeBounds the bounding box for this node
 */
void FNavigationOctreeNode::FilterObject(FNavigationOctreeObject* Object, const FOctreeNodeBounds& NodeBounds)
{
	INT ChildIndex = -1;
	if (Children != NULL)
	{
		ChildIndex = FindChild(NodeBounds, Object->BoundingBox);
	}
	if (ChildIndex != -1)
	{
		Children[ChildIndex].FilterObject(Object, FOctreeNodeBounds(NodeBounds, ChildIndex));
	}
	else
	{
		if (Children == NULL && Objects.Num() >= MAX_OBJECTS_PER_NODE)
		{
			Children = new FNavigationOctreeNode[8];

			// remove and reinsert primitives at this node, to see if they belong at a lower level
			TArray<FNavigationOctreeObject*> ReinsertObjects(Objects);
			Objects.Empty();
			ReinsertObjects.AddItem(Object);
			
			for (INT i = 0; i < ReinsertObjects.Num(); i++)
			{
				FilterObject(ReinsertObjects(i), NodeBounds);
			}
		}
		else
		{
			// store the object here
			Objects.AddItem(Object);
			Object->OctreeNode = this;
		}
	}
}

/** returns all objects in this node and all children whose bounding box intersects with the given sphere
 * @param Point the center of the sphere
 * @param RadiusSquared squared radius of the sphere
 * @param Extent bounding box for the sphere
 * @param OutObjects (out) all objects found in the radius
 * @param NodeBounds the bounding box for this node
 */
void FNavigationOctreeNode::RadiusCheck(const FVector& Point, FLOAT RadiusSquared, const FBox& Extent, TArray<FNavigationOctreeObject*>& OutObjects, const FOctreeNodeBounds& NodeBounds)
{
	// iterate through all the objects and add all the ones whose origin is within the radius
	for (INT i = 0; i < Objects.Num(); i++)
	{
		if (SphereAABBIntersectionTest(Point, RadiusSquared, Objects(i)->BoundingBox))
		{
			OutObjects.AddItem(Objects(i));
		}
	}

	// check children
	if (Children != NULL)
	{
		INT ChildIdx[8];
		INT NumChildren = FindChildren(NodeBounds, Extent, ChildIdx);
		for (INT i = 0; i < NumChildren; i++)
		{
			Children[ChildIdx[i]].RadiusCheck(Point, RadiusSquared, Extent, OutObjects, FOctreeNodeBounds(NodeBounds, ChildIdx[i]));
		}
	}
}

/** checks the given box against the objects in this node and returns the first object found that intersects with it
 * recurses down to children that intersect the box
 * @param Box the box to check
 * @param NodeBounds the bounding box for this node
 * @return the first object found that intersects, or NULL if none do
 */
void FNavigationOctreeNode::OverlapCheck(const FBox& Box, TArray<FNavigationOctreeObject*>& OutObjects, const FOctreeNodeBounds& NodeBounds)
{
	// iterate through all the objects; if we find one that intersects the given box, return it
	for (INT i = 0; i < Objects.Num(); i++)
	{
		if (Objects(i)->BoundingBox.Intersect(Box) && !Objects(i)->OverlapCheck(Box))
		{
			OutObjects.AddItem(Objects(i));
		}
	}

	// check children
	if (Children != NULL)
	{
		INT ChildIdx[8];
		INT NumChildren = FindChildren(NodeBounds, Box, ChildIdx);
		for (INT i = 0; i < NumChildren; i++)
		{
			Children[ChildIdx[i]].OverlapCheck(Box, OutObjects, FOctreeNodeBounds(NodeBounds, ChildIdx[i]));
		}
	}
}

/** adds an object with the given bounding box
 * @param Object the object to add
 * @note this method assumes the object is not already in the octree
 */
void FNavigationOctree::AddObject(FNavigationOctreeObject* Object)
{
	//debugf(TEXT("Adding object %s to octree"),*Object->GetOwner<UObject>()->GetPathName());
	const FBox& BoundingBox = Object->BoundingBox;
	if (	BoundingBox.Max.X < -HALF_WORLD_MAX || BoundingBox.Min.X > HALF_WORLD_MAX ||
			BoundingBox.Max.Y < -HALF_WORLD_MAX || BoundingBox.Min.Y > HALF_WORLD_MAX ||
			BoundingBox.Max.Z < -HALF_WORLD_MAX || BoundingBox.Min.Z > HALF_WORLD_MAX )
	{
		debugf(NAME_Warning, TEXT("%s is outside the world"), *Object->GetOwner<UObject>()->GetName());
	}
	else
	{
		// verify that the object wasn't already in the octree
		checkSlow(Object->OctreeNode == NULL);

		RootNode->FilterObject(Object, RootNodeBounds);
	}
}

/** removes the given object from the octree
 * @param Object the object to remove
 * @return true if the object was in the octree and was removed, false if it wasn't in the octree
 */
UBOOL FNavigationOctree::RemoveObject(FNavigationOctreeObject* Object)
{
	if (Object->OctreeNode == NULL)
	{
		return false;
	}
	else
	{
		//debugf(TEXT("Removing object %s from octree"),*Object->GetOwner<UObject>()->GetPathName());
		UBOOL bResult = Object->OctreeNode->RemoveObject(Object);
		if (!bResult)
		{
			debugf(NAME_Warning, TEXT("Attempt to remove %s from navigation octree but it isn't there"), *Object->GetOwner<UObject>()->GetName());
		}		
		Object->OctreeNode = NULL;

		return bResult;
	}
}

/** 
 * Removes all objects from octree.
 */
void FNavigationOctree::RemoveAllObjects()
{
	delete RootNode;
	RootNode = new FNavigationOctreeNode;
}

/** counts the number of nodes and objects there are in the octree
 * @param NumNodes (out) incremented by the number of nodes (this one plus all children)
 * @param NumObjects (out) incremented by the total number of objects in this node and its child nodes
 */
void FNavigationOctreeNode::CollectStats(INT& NumNodes, INT& NumObjects)
{
	NumNodes++;
	NumObjects += Objects.Num();
	if (Children != NULL)
	{
		for (INT i = 0; i < 8; i++)
		{
			Children[i].CollectStats(NumNodes, NumObjects);
		}
	}
}

/** 
* Find the given object in the octree node
* @param bRecurseChildren - recurse on this node's child nodes
* @return TRUE if object exists in the octree node 
*/
UBOOL FNavigationOctreeNode::FindObject( UObject* Owner, UBOOL bRecurseChildren )
{
	if( Owner )
	{
		for( INT ObjIdx=0; ObjIdx < Objects.Num(); ObjIdx++ )
		{
			FNavigationOctreeObject* CurObj = Objects(ObjIdx);
			if( CurObj->GetOwner<UObject>() == Owner )
			{
				return TRUE;
			}
		}
		if( Children != NULL &&
			bRecurseChildren )
		{
			for( INT i=0; i<8; i++ )
			{
				if( Children[i].FindObject(Owner,TRUE) )
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/** console command handler for implementing debugging commands */
UBOOL FNavigationOctree::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (ParseCommand(&Cmd, TEXT("NAVOCTREE")))
	{
		if (ParseCommand(&Cmd, TEXT("STATS")))
		{
			INT NumNodes = 0, NumObjects = 0;
			RootNode->CollectStats(NumNodes, NumObjects);
			Ar.Logf(TEXT("Number of objects: %i"), NumObjects);
			Ar.Logf(TEXT("Number of nodes: %i"), NumNodes);
			Ar.Logf(TEXT("Memory used by octree structures: %i bytes"), sizeof(FNavigationOctree) + NumNodes * sizeof(FNavigationOctreeNode) + NumObjects * sizeof(FNavigationOctreeObject*));
			Ar.Logf(TEXT("Memory used by objects in the octree: %i bytes"), NumObjects * sizeof(FNavigationOctreeObject));
		}
		else if( ParseCommand(&Cmd,TEXT("FIND")) )
		{
			UObject* Object;
			if(	ParseObject(Cmd,TEXT("NAME="), UObject::StaticClass(), Object, ANY_PACKAGE) )
			{
				debugf(TEXT("NAVOCTREE: FIND: %s = %s"), *Object->GetFullName(), RootNode->FindObject(Object,TRUE) ? TEXT("TRUE") : TEXT("FALSE") );
			}
			else
			{
				debugf(TEXT("NAVOCTREE: FIND: invalid object"));
			}
		}

		return 1;
	}	
	else
	{
		return 0;
	}
}
