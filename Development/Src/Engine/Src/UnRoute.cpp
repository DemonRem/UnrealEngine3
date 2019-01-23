/*=============================================================================
	UnRoute.cpp: Unreal AI routing code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
 
#include "EnginePrivate.h"
#include "UnPath.h"
#include "EngineAIClasses.h"

IMPLEMENT_CLASS(ADynamicAnchor);

//pathdebug
// path debugging and logging
#define PATH_LOOP_TESTING 0
#if 0 && !PS3 && !FINAL_RELEASE
	#define DEBUGPATHONLY(x)    { ##x }	
//	#define DEBUGPATHLOG(x)		{ debugf(*##x); }
	#define DEBUGPATHLOG(x)
	#define DEBUGEMPTYPATHLOG		
	#define PRINTDEBUGPATHLOG(x)
	#define PRINTPATHLIST(x)
	
#elif 0 && !PS3 && !FINAL_RELEASE
	static TArray<FString> DebugPathLogCache;
	#define DEBUGPATHONLY(x)	//    { ##x }	
	#define DEBUGPATHLOG(x)			{ DebugPathLogCache.AddItem(##x); }
	#define DEBUGEMPTYPATHLOG		{ DebugPathLogCache.Empty(); }
	#define PRINTDEBUGPATHLOG(x)	{ PrintDebugPathLogCache(##x); }
	#define PRINTPATHLIST(x)		{ PrintPathList( ##x ); }
	
	static void PrintDebugPathLogCache( UBOOL bBreak )
	{
		//turn off for now
		if( !bBreak )
			return;

		for( INT Idx = 0; Idx < DebugPathLogCache.Num(); Idx++ )
		{
			debugf( *(DebugPathLogCache(Idx)) );
		}

		if( bBreak )
		{
			appDebugBreak();
		}
	}

	static void PrintPathList( ANavigationPoint* Goal )
	{
		DEBUGPATHLOG(FString::Printf(TEXT("Print Path List...")))

		INT Cnt = 0;
		ANavigationPoint* TestNav = Goal->nextOrdered;
		while( TestNav != NULL )
		{
			Cnt++;

			DEBUGPATHLOG(FString::Printf(TEXT("Path %d"), Cnt))
			ANavigationPoint* Nav = TestNav;
			while( Nav != NULL )
			{
				DEBUGPATHLOG(FString::Printf(TEXT("     %s"), *Nav->GetName()))

				Nav = Nav->previousPath;
			}

			TestNav = TestNav->nextOrdered;
		}
		
		DEBUGPATHLOG(FString::Printf(TEXT("End Print Path List... Total Count %d"),Cnt))
	}
	
#else
	#define DEBUGPATHONLY(x)
	#define DEBUGPATHLOG(x)	
	#define DEBUGEMPTYPATHLOG		
	#define PRINTDEBUGPATHLOG(x)
	#define PRINTPATHLIST(x)
#endif



/**
 * Attempt at a more optimized path finding algorithm.
 * @note - use at your own risk!  this is built around assumptions that may or may not be valid/optimal for your particular path networks.
 */
static UBOOL NewBestPathTo(APawn *Pawn, ANavigationPoint *Start, ANavigationPoint *Goal, UBOOL bReturnPartial)
{
	//debug
	DEBUGPATHONLY(Pawn->FlushPersistentDebugLines();)
	
	if (Goal == NULL)
	{
		return FALSE;
	}
	// early out if the start/goal anchors are the same
	if (Start == Goal)
	{
		//debug
		DEBUGPATHLOG(FString::Printf(TEXT("Start == Goal?")))

		Pawn->Controller->RouteCache_InsertItem(Start,0);
		return TRUE;
	}
	DEBUGPATHONLY(UWorld::VerifyNavList(*FString::Printf(TEXT("NEW!!! BESTPATHTO %s %s to %s"), *Pawn->GetName(), *Start->GetFullName(), *Goal->GetFullName()));)
	const INT MaxPathVisits = 500;
	INT NumPathVisits = 0;
	// grab initial flags needed for spec testing
	INT iRadius	= Pawn->bCanCrouch ? appTrunc(Pawn->CrouchRadius) : appTrunc(Pawn->CylinderComponent->CollisionRadius);
	INT iHeight = Pawn->bCanCrouch ? appTrunc(Pawn->CrouchHeight) : appTrunc(Pawn->CylinderComponent->CollisionHeight);
	INT iMaxFallSpeed = appTrunc(Pawn->GetAIMaxFallSpeed());
	INT moveFlags = Pawn->calcMoveFlags();

	// add the start nav to the head of the list
	Goal->nextOrdered = Start;
	Start->bAlreadyVisited = TRUE;
	// no cost to visit the start node
	Start->visitedWeight = 0;
	Start->bestPathWeight = 0;
	// while there are nodes left to search and a path hasn't been found
	UBOOL bFoundPath = FALSE;
	UBOOL bPartialPath = TRUE;
	while (Goal->nextOrdered != NULL && !bFoundPath)
	{
		// take the cheapest one
		ANavigationPoint *Nav = Goal->nextOrdered;

		//debug
//		PRINTPATHLIST( Goal )
		DEBUGPATHLOG(FString::Printf(TEXT("- evaluating %s w/ %d paths"),*Nav->GetFullName(),Nav->PathList.Num()))

		Goal->nextOrdered = Nav->nextOrdered;
		if (++NumPathVisits >= MaxPathVisits)
		{
			debugf(NAME_Warning,TEXT("PATH ERROR!!!!  %s exceeded max path visits looking for path from %s to %s, returning best guess"),*Pawn->GetName(),*Start->GetFullName(), *Goal->GetFullName());
#if !FINAL_RELEASE
//			Pawn->eventMessagePlayer( FString::Printf(TEXT("PATH ERROR!!!!  %s exceeded max path visits looking for path from %s to %s, returning best guess"),*Pawn->GetName(),*Start->GetFullName(), *Goal->GetFullName()) );
#endif

			Goal->previousPath = Nav;
			bFoundPath = TRUE;

//			PRINTDEBUGPATHLOG(FALSE);
			break;
		}
		// look at each node this one connects to
		for (INT PathIdx = 0; PathIdx < Nav->PathList.Num() && !bFoundPath; PathIdx++)
		{
			// if it is a valid connection that the pawn can walk
			UReachSpec *Spec = Nav->PathList(PathIdx);
			if (Spec != NULL && *Spec->End != NULL && !Spec->End->IsPendingKill() && Spec->supports(iRadius,iHeight,moveFlags,iMaxFallSpeed))
			{
				ANavigationPoint *PossibleNav = Spec->End.Nav;

				//debug
				DEBUGPATHLOG(FString::Printf(TEXT("--- check path %s (%d/%d)"), *PossibleNav->GetName(), PathIdx, PossibleNav->bAlreadyVisited))

				// Don't move through nodes that have no paths going away from it 
				// big enough to support the pawn
				if( !PossibleNav->ANavigationPoint::IsUsableAnchorFor(Pawn) )
				{
					//debug
					DEBUGPATHLOG(FString::Printf(TEXT("---== node is invalid anchor, no way to move through it ")))

					continue;
				}

				// Cache the cost of this nav
				INT InitialCost = Spec->CostFor(Pawn);
				if( InitialCost >= UCONST_BLOCKEDPATHCOST )
				{
					//debug
					DEBUGPATHLOG(FString::Printf(TEXT("---== node blocked")))
					DEBUGPATHONLY(Pawn->DrawDebugLine(Nav->Location,PossibleNav->Location,64,0,0,TRUE);)

					// don't bother with blocked paths
					continue;
				}
				// Make sure cost is valid
				if( InitialCost <= 0 )
				{
					debugf( TEXT("Path Warning!!! Cost from %s to %s is zero/neg %i"), *Nav->GetFullName(), *PossibleNav->GetFullName(), InitialCost );
					DEBUGPATHLOG(FString::Printf(TEXT("Path Warning!!! Cost from %s to %s is zero/neg %i"), *Nav->GetFullName(), *PossibleNav->GetFullName(), InitialCost))
				}

				// if it hasn't ready been visited
				if( !PossibleNav->bAlreadyVisited )
				{
					//debug
					DEBUGPATHONLY(Pawn->DrawDebugLine(Nav->Location,PossibleNav->Location,0,64,0,TRUE);)

					// mark the node that led to this node
					PossibleNav->previousPath = Nav;

					//debug
					DEBUGPATHLOG(FString::Printf(TEXT("NEW BPT set %s prev path = %s"), *PossibleNav->GetName(), *Nav->GetName()))

					// Check to see if this is the path
					if( PossibleNav == Goal )
					{
						//debug
						DEBUGPATHLOG(FString::Printf(TEXT("-! found path to goal from %s"),*Nav->GetName()))

						bFoundPath = TRUE;
						bPartialPath = FALSE;
						break;
					}
					else
					{			
						// Mark it as visited
						PossibleNav->bAlreadyVisited = TRUE;

						// Get weighted cost for this node
						PossibleNav->visitedWeight = InitialCost + Nav->visitedWeight;

						// Don't allow zero or negative costs
						if( PossibleNav->visitedWeight <= 0 )
						{
							PossibleNav = NULL;
						}
					}
				}
				else
				if( PossibleNav != Start &&
					PossibleNav->previousPath != Start )
				{
					//debug
					DEBUGPATHLOG(FString::Printf(TEXT("-> Attempting to find a shorter path to %s using %s [prev: %s] [%d/%d]"),
							*PossibleNav->GetName(),*Nav->GetName(),*PossibleNav->previousPath->GetName(),PossibleNav->previousPath->bestPathWeight,Nav->bestPathWeight ))

					// If potential path weight from nav to evaluated node is shorter
					// previous computed full path weight to evaluated node
					INT newVisitedWeight = PossibleNav->visitedWeight + Nav->visitedWeight;
//					INT newVisitedWeight = InitialCost + Nav->visitedWeight;
					if( newVisitedWeight < PossibleNav->visitedWeight )
					{
						//debug
						DEBUGPATHLOG(FString::Printf(TEXT("--> found shortcut")))

						// Search Goal->nextOrdered chain for anything containing PossibleNav
						// If found, remove that chain
						// Starting with goal
						ANavigationPoint* TestNav = Goal;
#if PATH_LOOP_TESTING
						TArray<ANavigationPoint*> TestNavList;
#endif
						while (TestNav != NULL)
						{
#if PATH_LOOP_TESTING
							if (TestNavList.ContainsItem(TestNav))
							{
								//debug
								debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s [OUTER SHORTCUT LOOP] Encountered loop [%s to %s] trying to remove %s from the path chain of %s"),
										*Pawn->GetName(),*Start->GetName(),*Goal->GetName(),*PossibleNav->GetName(), *Goal->GetName());
								PRINTDEBUGPATHLOG(TRUE);
								return FALSE;
							}
							else
							{
								TestNavList.AddItem(TestNav);
							}
#endif
							// Check next chain of built paths
							ANavigationPoint* ChkNav = TestNav->nextOrdered;
							// Iterate until we reach end of previous path list OR
							// possible nav is found in the list
#if PATH_LOOP_TESTING
							TArray<ANavigationPoint*> InnerNavList;
#endif
							while (ChkNav != NULL && ChkNav != PossibleNav)
							{
#if PATH_LOOP_TESTING
								if (InnerNavList.ContainsItem(ChkNav))
								{
									//debug
									debugf(NAME_Warning,TEXT("PATH ERROR!!!! [INNER SHORTCUT LOOP] %s Encountered loop [%s to %s] trying to remove %s from the path chain of %s"),
											*Pawn->GetName(),*Start->GetName(),*Goal->GetName(),*PossibleNav->GetName(), *TestNav->nextOrdered->GetName());
									(TRUE);
									return FALSE;
								}
								else
								{
									InnerNavList.AddItem(ChkNav);
								}
#endif	
								ChkNav = ChkNav->previousPath;
							}

							// If we found possible nav
							if( ChkNav == PossibleNav )
							{
								// Cull out this path from the list
								// Break, b/c we will only have a single path containing 
								// the possible nav
								TestNav->nextOrdered = TestNav->nextOrdered->nextOrdered;
								break;
							}

							// Move to next path list
							TestNav = TestNav->nextOrdered;
						}

						// Now that path list is clear of possible nav
						// Update previous path list
						PossibleNav->previousPath = Nav;

						//debug
						DEBUGPATHLOG(FString::Printf(TEXT("NEW BPT set shortcut %s prev path = %s"), *PossibleNav->GetName(), *Nav->GetName()))
					}
					else
					{
						// Otherwise, better path already exists
						// Just cull this whole path from the list
						PossibleNav = NULL;
					}
				}
				else
				{
					// Otherwise, already visited and too close to start
					// just ignore this path
					PossibleNav = NULL;
				}

				// If node is still being evaluated
				if( PossibleNav != NULL )
				{
					// Update the estimated weight for this path
					PossibleNav->bestPathWeight = Spec->AdjustedCostFor( Pawn, Nav, Goal, PossibleNav->visitedWeight );

					// Don't allow zero or negative weights
					if( PossibleNav->bestPathWeight <= 0 )
					{
						PossibleNav = NULL;
					}
				}

				// If node is still being evaluated
				if( PossibleNav != NULL )
				{
					// insert into the list for later searching
					ANavigationPoint *TestNav = Goal;
#if PATH_LOOP_TESTING
					TArray<ANavigationPoint*> NavList;
#endif
					while (TestNav != NULL)
					{
#if PATH_LOOP_TESTING
						if (NavList.ContainsItem(TestNav))
						{
							debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s [INSERT BY WEIGHT LOOP] Encountered loop [%s to %s] trying to insert %s by weight"),
									*Pawn->GetName(),*Start->GetName(),*Goal->GetName(),*PossibleNav->GetName());

							PRINTDEBUGPATHLOG(TRUE);
							return FALSE;
						}
						else
						{
							NavList.AddItem(TestNav);
						}
#endif

						// if reached the end of the list
						if (TestNav->nextOrdered == NULL)
						{
							//debug
							DEBUGPATHLOG(FString::Printf(TEXT("->> placed %s at the end of the list [from: %s]"),*PossibleNav->GetName(),*Nav->GetName()))

							// append to the end
							TestNav->nextOrdered = PossibleNav;
							break;
						}
						else
						// if this nav is cheaper than the test one
						if( PossibleNav->bestPathWeight <= TestNav->nextOrdered->bestPathWeight )
						{
							//debug
							DEBUGPATHLOG(FString::Printf(TEXT("->> inserted %s (%d) in before %s (%d) [from: %s]"),*PossibleNav->GetName(),PossibleNav->visitedWeight,*TestNav->nextOrdered->GetName(),TestNav->nextOrdered->visitedWeight,*Nav->GetName()))

							// insert into the list
							PossibleNav->nextOrdered = TestNav->nextOrdered;
							TestNav->nextOrdered = PossibleNav;
							break;
						}
						else
						{
							// move on to the next
							if (TestNav == TestNav->nextOrdered)
							{
								//@fixme - this shouldn't ever happen, so in the meantime gracefully handle the problem
								debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s [DUPLICATE INSERT] Encountered path duplicate [%s to %s] trying to insert %s into nextOrdered list"),
									*Pawn->GetName(),*Start->GetName(),*Goal->GetName(),*TestNav->GetName());

								PRINTDEBUGPATHLOG(TRUE);
								break;
							}
							TestNav = TestNav->nextOrdered;
						}
					}
				}
			}
		}
	}

	// if a path was found
	if (bFoundPath)
	{
		// fill in the route cache with the new path
		ANavigationPoint *Nav = Goal;
		// if we only have a partial path
		if (bPartialPath)
		{
			// don't include the goal as the final route node
			Nav = Goal->previousPath;
		}
#if PATH_LOOP_TESTING
		TArray<ANavigationPoint*> NavList;
#endif
		while (Nav != Start && Nav != NULL)
		{
#if PATH_LOOP_TESTING
			if (NavList.ContainsItem(Nav))
			{
				debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s [BUILD PATH LOOP] Encountered a loop [%s to %s] trying to insert %s into route cache"),
									*Pawn->GetName(),*Start->GetName(),*Goal->GetName(),*Nav->GetName());

				PRINTDEBUGPATHLOG(TRUE);
				return FALSE;
			}
			else
			{
				NavList.AddItem(Nav);
			}
#endif

			//debug
			DEBUGPATHONLY(Pawn->DrawDebugLine(Nav->Location,Nav->previousPath->Location,0,255,0,TRUE);)

			//@todo - wouldn't this step be unnecessary if we searched for the path backwards?  might have some nasty side effects though in the case of one-way paths
			Pawn->Controller->RouteCache_InsertItem( Nav, 0 );
			Nav = Nav->previousPath;
		}
		// add the start if necessary
		if( !Pawn->ReachedDestination( Start ) )
		{
			Pawn->Controller->RouteCache_InsertItem( Start, 0 );
		}
		return TRUE;
	}

	debugf(NAME_Warning,TEXT("PATH ERROR!!!! No path found to %s from %s for %s"),*Goal->GetName(),*Start->GetName(),*Pawn->GetName());
	return FALSE;
}

ANavigationPoint* FSortedPathList::FindStartAnchor(APawn *Searcher)
{
	// see which nodes are visible and reachable
	FCheckResult Hit(1.f);
	for ( INT i=0; i<numPoints; i++ )
	{
		GWorld->SingleLineCheck( Hit, Searcher, Path[i]->Location, Searcher->Location, TRACE_World|TRACE_StopAtAnyHit );
		if ( Hit.Actor )
			GWorld->SingleLineCheck( Hit, Searcher, Path[i]->Location + FVector(0.f,0.f, Path[i]->CylinderComponent->CollisionHeight), Searcher->Location + FVector(0.f,0.f, Searcher->CylinderComponent->CollisionHeight), TRACE_World|TRACE_StopAtAnyHit );
		if ( !Hit.Actor && Searcher->actorReachable(Path[i], 1, 0) )
			return Path[i];
	}
	return NULL;
}

ANavigationPoint* FSortedPathList::FindEndAnchor(APawn *Searcher, AActor *GoalActor, FVector EndLocation, UBOOL bAnyVisible, UBOOL bOnlyCheckVisible )
{
	if ( bOnlyCheckVisible && !bAnyVisible )
		return NULL;

	ANavigationPoint* NearestVisible = NULL;
	FVector RealLoc = Searcher->Location;

	// now see from which nodes EndLocation is visible and reachable
	FCheckResult Hit(1.f);
	for ( INT i=0; i<numPoints; i++ )
	{
		GWorld->SingleLineCheck( Hit, Searcher, EndLocation, Path[i]->Location, TRACE_World|TRACE_StopAtAnyHit );
		if ( Hit.Actor )
		{
			if ( GoalActor )
			{
				FLOAT GoalRadius, GoalHeight;
				GoalActor->GetBoundingCylinder(GoalRadius, GoalHeight);

				GWorld->SingleLineCheck( Hit, Searcher, EndLocation + FVector(0.f,0.f,GoalHeight), Path[i]->Location  + FVector(0.f,0.f, Path[i]->CylinderComponent->CollisionHeight), TRACE_World|TRACE_StopAtAnyHit );
			}
			else
				GWorld->SingleLineCheck( Hit, Searcher, EndLocation, Path[i]->Location  + FVector(0.f,0.f, Path[i]->CylinderComponent->CollisionHeight), TRACE_World|TRACE_StopAtAnyHit );
		}
			if ( !Hit.Actor )
		{
			if ( bOnlyCheckVisible )
				return Path[i];
		FVector AdjustedDest = Path[i]->Location;
		AdjustedDest.Z = AdjustedDest.Z + Searcher->CylinderComponent->CollisionHeight - Path[i]->CylinderComponent->CollisionHeight;
			if ( GWorld->FarMoveActor(Searcher,AdjustedDest,1,1) )
		{
			if ( GoalActor ? Searcher->actorReachable(GoalActor,1,0) : Searcher->pointReachable(EndLocation, 1) )
			{
				GWorld->FarMoveActor(Searcher, RealLoc, 1, 1);
				return Path[i];
			}
			else if ( bAnyVisible && !NearestVisible )
				NearestVisible = Path[i];
		}
	}
	}

	if ( Searcher->Location != RealLoc )
	{
		GWorld->FarMoveActor(Searcher, RealLoc, 1, 1);
	}

	return NearestVisible;
}

UBOOL APawn::ValidAnchor()
{
	if( bForceKeepAnchor || 
		(Anchor && !Anchor->bBlocked 
		&& (bCanCrouch ? (Anchor->MaxPathSize.Radius >= CrouchRadius) && (Anchor->MaxPathSize.Height >= CrouchHeight)
						: (Anchor->MaxPathSize.Radius >= CylinderComponent->CollisionRadius) && (Anchor->MaxPathSize.Height >= CylinderComponent->CollisionHeight))
		&& ReachedDestination(Location, Anchor->GetDestination(Controller), Anchor)) )
	{
		LastValidAnchorTime = GWorld->GetTimeSeconds();
		LastAnchor = Anchor;
		return true;
	}
	return false;
}

typedef FLOAT ( *NodeEvaluator ) (ANavigationPoint*, APawn*, FLOAT);

static FLOAT FindEndPoint( ANavigationPoint* CurrentNode, APawn* seeker, FLOAT bestWeight )
{
	if ( CurrentNode->bEndPoint )
	{
//		debugf(TEXT("Found endpoint %s"),*CurrentNode->GetName());
		return 2.f;
	}
	else
		return 0.f;
}

/** utility for FindAnchor() that returns whether Start has a ReachSpec to End that is acceptable for considering Start as an anchor */
static FORCEINLINE UBOOL HasReachSpecForAnchoring(ANavigationPoint* Start, ANavigationPoint* End)
{
	UReachSpec* Spec = Start->GetReachSpecTo(End);
	// only allow octree-accessible specs
	return (Spec != NULL && Spec->bAddToNavigationOctree);
}

/** finds the closest NavigationPoint within MAXPATHDIST that is usable by this pawn and directly reachable to/from TestLocation
 * @param TestActor the Actor to find an anchor for
 * @param TestLocation the location to find an anchor for
 * @param bStartPoint true if we're finding the start point for a path search, false if we're finding the end point
 * @param bOnlyCheckVisible if true, only check visibility - skip reachability test
 * @param Dist (out) if an anchor is found, set to the distance TestLocation is from it. Set to 0.f if the anchor overlaps TestLocation
 * @return a suitable anchor on the navigation network for reaching TestLocation, or NULL if no such point exists
 */
ANavigationPoint* APawn::FindAnchor(AActor* TestActor, const FVector& TestLocation, UBOOL bStartPoint, UBOOL bOnlyCheckVisible, FLOAT& Dist)
{
	INT Radius = appTrunc(CylinderComponent->CollisionRadius);
	INT Height = appTrunc(CylinderComponent->CollisionHeight);
	INT iMaxFallSpeed = appTrunc(GetAIMaxFallSpeed());
	INT MoveFlags = calcMoveFlags();

	// first try fast point check
	// return the first usable NavigationPoint found, otherwise use closest ReachSpec endpoint
	TArray<FNavigationOctreeObject*> Objects;
	GWorld->NavigationOctree->PointCheck(TestLocation, FVector(CylinderComponent->CollisionRadius, CylinderComponent->CollisionRadius, CylinderComponent->CollisionHeight), Objects);
	ANavigationPoint* BestReachSpecPoint = NULL;
	FLOAT BestReachSpecPointDistSquared = BIG_NUMBER;
	for (INT i = 0; i < Objects.Num(); i++)
	{
		ANavigationPoint* Nav = Objects(i)->GetOwner<ANavigationPoint>();
		if (Nav != NULL && Nav->IsUsableAnchorFor(this) && (!bStartPoint || !Nav->bDestinationOnly))
		{
			Dist = 0.f;
			return Nav;
		}
		else
		{
			UReachSpec* Spec = Objects(i)->GetOwner<UReachSpec>();
			if( Spec != NULL && 
				Spec->Start != NULL && 
				*Spec->End != NULL && 
				!Spec->End->bSpecialMove &&	// Don't do for special move paths, it breaks special movement code
				!Spec->End->bDestinationOnly && 
				Spec->supports(Radius, Height, MoveFlags, iMaxFallSpeed))
			{
				FLOAT StartDistSquared = (Spec->Start->Location - TestLocation).SizeSquared();
				FLOAT EndDistSquared = (Spec->End->Location - TestLocation).SizeSquared();
				// choose closer endpoint
				if (StartDistSquared < EndDistSquared && Spec->Start->IsUsableAnchorFor(this) && HasReachSpecForAnchoring(Spec->End, Spec->Start)) // only consider Start if there's a reverse ReachSpec
				{
					if (StartDistSquared < BestReachSpecPointDistSquared)
					{
						if (bStartPoint && Spec->Start->PathList.Num() == 0)
						{
							continue;
						}
						BestReachSpecPoint = Spec->Start;
						BestReachSpecPointDistSquared = StartDistSquared;
					}
				}
				else if (EndDistSquared < BestReachSpecPointDistSquared && Spec->End->IsUsableAnchorFor(this))
				{
					if (bStartPoint && Spec->End->PathList.Num() == 0)
					{
						continue;
					}
					BestReachSpecPoint = Spec->End;
					BestReachSpecPointDistSquared = EndDistSquared;
				}
			}
		}
	}
	// not directly touching NavigationPoint, use closest ReachSpec endpoint, if any
	if (BestReachSpecPoint != NULL)
	{
		Dist = appSqrt(BestReachSpecPointDistSquared);
		return BestReachSpecPoint;
	}

	// point check failed, try MAXPATHDIST radius check
	// we'll need to trace and check reachability, so create a distance sorted list of suitable points and then check them until we find one
	Objects.Empty(Objects.Num());
	FSortedPathList TestPoints;
	GWorld->NavigationOctree->RadiusCheck(TestLocation, MAXPATHDIST, Objects);
	for (INT i = 0; i < Objects.Num(); i++)
	{
		ANavigationPoint* Nav = Objects(i)->GetOwner<ANavigationPoint>();
		if (Nav != NULL && Nav->IsUsableAnchorFor(this) && (!bStartPoint || !Nav->bDestinationOnly))
		{
			TestPoints.AddPath(Nav, appTrunc((TestLocation - Nav->Location).SizeSquared()));
		}
	}

	// find the closest usable anchor among those found
	if (TestPoints.numPoints == 0)
	{
		return NULL;
	}
	else
	{
		ANavigationPoint* Result;
		if (bStartPoint)
		{
			Result = TestPoints.FindStartAnchor(this);
		}
		else
		{
			Result = TestPoints.FindEndAnchor(this, TestActor, TestLocation, (TestActor != NULL && Controller->AcceptNearbyPath(TestActor)), bOnlyCheckVisible);
		}
		if (Result == NULL)
		{
			FVector RealLoc = Location;
			if (GWorld->FarMoveActor(this, TestLocation, 1, 1))
			{
				// try finding reachable ReachSpec to move towards
				for (INT i = 0; i < Objects.Num() && Result == NULL; i++)
				{
					UReachSpec* Spec = Objects(i)->GetOwner<UReachSpec>();
					if( Spec != NULL && 
						Spec->Start != NULL && 
						*Spec->End != NULL &&
						!Spec->End->bSpecialMove &&	// Don't do for special move paths, it breaks special movement code
						Spec->supports( Radius, Height, MoveFlags, iMaxFallSpeed ) )
					{
						// make sure we're between the path's endpoints
						const FVector PathDir = (Spec->End->Location - Spec->Start->Location).SafeNormal();
						if (((Spec->Start->Location - Location).SafeNormal() | PathDir) < 0.f && ((Spec->End->Location - Location).SafeNormal() | PathDir) > 0.f)
						{
							// try point on path that's closest to us
							const FVector ClosestPoint = (Spec->Start->Location + (PathDir | (Location - Spec->Start->Location)) * PathDir);
							// see if it's reachable
							if (pointReachable(ClosestPoint, 0))
							{
								// we need StaticLoadClass() here to verify that the native class's defaults have been loaded, since it probably isn't referenced anywhere
								StaticLoadClass(ADynamicAnchor::StaticClass(), NULL, TEXT("Engine.DynamicAnchor"), NULL, LOAD_None, NULL);
								ADynamicAnchor* DynamicAnchor = (ADynamicAnchor*)GWorld->SpawnActor(ADynamicAnchor::StaticClass(), NAME_None, ClosestPoint);
								if (DynamicAnchor != NULL)
								{
									DynamicAnchor->Initialize(Controller, Spec->Start, Spec->End, Spec);
									Result = DynamicAnchor;
								}
							}
							else
							{
								// if that fails, try endpoints if they are more than MAXPATHDIST away (since they wouldn't have been checked yet)
								FCheckResult Hit(1.0f);
								if ( (Spec->Start->Location - Location).SizeSquared() > MAXPATHDISTSQ
										&& GWorld->SingleLineCheck(Hit, this, Spec->Start->Location, Location, TRACE_World | TRACE_StopAtAnyHit)
										&& Reachable(Spec->Start->Location, Spec->Start) )
								{
									Result = Spec->Start;
								}
								else if ( (Spec->End->Location - Location).SizeSquared() > MAXPATHDISTSQ
										&& GWorld->SingleLineCheck(Hit, this, Spec->End->Location, Location, TRACE_World | TRACE_StopAtAnyHit)
										&& Reachable(Spec->End->Location, Spec->End) )
								{
									// don't allow start anchors if they don't have any paths away
									if (bStartPoint &&
										Spec->End->PathList.Num() == 0)
									{
										continue;
									}
									Result = Spec->End;
								}
							}
						}
					}
				}
			}
			GWorld->FarMoveActor(this, RealLoc, 1, 1);
		}
		if (Result != NULL)
		{
			Dist = (Result->Location - TestLocation).Size();
		}
		// FIXME: if no reachspecs in list, try larger radius?
		return Result;
	}
}

//@fixme - temporary guarantee that only one pathfind occurs at a time
static UBOOL bIsPathFinding = FALSE;
struct FPathFindingGuard
{
	FPathFindingGuard()
	{
		bIsPathFinding = TRUE;
	}

	~FPathFindingGuard()
	{
		bIsPathFinding = FALSE;
	}
};

FLOAT APawn::findPathToward(AActor *goal, FVector GoalLocation, NodeEvaluator NodeEval, FLOAT BestWeight, UBOOL bWeightDetours, FLOAT MaxPathLength, UBOOL bReturnPartial )
{
	SCOPE_CYCLE_COUNTER(STAT_PathFinding_FindPathToward);

	//debug
	DEBUGEMPTYPATHLOG;
	DEBUGPATHLOG(FString::Printf(TEXT("%s FindPathToward: %s"),*GetName(), goal != NULL ? *goal->GetPathName() : *GoalLocation.ToString()))

	//@fixme - remove this once the problem has been identified
	// make sure only one path find occurs at once
	if (bIsPathFinding)
	{
		debugf(NAME_Warning,TEXT("findPathToward() called during an existing path find! %s"),*GetName());
		return 0.f;
	}
	FPathFindingGuard PathFindingGuard;

	NextPathRadius = 0.f;

	// If world has no navigation points OR
	// We already tried (and failed) to find an anchor this tick OR 
	// we have no controller
	if( !GWorld->GetFirstNavigationPoint() || (FindAnchorFailedTime == GWorld->GetTimeSeconds()) || !Controller )
	{
		//debug
		DEBUGPATHLOG(FString::Printf(TEXT("- initial abort, %2.1f"),FindAnchorFailedTime))

		// Early fail
		return 0.f;
	}

	UBOOL	bSpecifiedEnd		= (NodeEval == NULL);
	FVector RealLocation		= Location;

	ANavigationPoint *EndAnchor = goal ? goal->SpecifyEndAnchor(this) : NULL;
	FLOAT StartDist	= 0.f;
	UBOOL	bOnlyCheckVisible = (Physics == PHYS_RigidBody) || (goal && !EndAnchor && goal->AnchorNeedNotBeReachable());

	// Grab goal location from actor
	if( goal != NULL )
	{
		GoalLocation = goal->GetDestination( Controller );
	}
	FLOAT EndDist = EndAnchor ? (EndAnchor->Location - GoalLocation).Size() : 0.f;

	// check if my anchor is still valid
	if ( !ValidAnchor() )
	{
		SetAnchor( NULL );
	}
	
	if ( !Anchor || (!EndAnchor && bSpecifiedEnd) )
	{
		if (Anchor == NULL)
		{
			//debug
			DEBUGPATHLOG(FString::Printf(TEXT("- looking for new anchor")))

			SetAnchor( FindAnchor(this, Location, true, false, StartDist) );
			if (Anchor == NULL)
			{
				FindAnchorFailedTime = WorldInfo->TimeSeconds;
				return 0.f;
			}

			//debug
			DEBUGPATHLOG(FString::Printf(TEXT("-+ found new anchor %s"),*Anchor->GetPathName()))

			LastValidAnchorTime = GWorld->GetTimeSeconds();
			LastAnchor = Anchor;
		}
		else
		{
			//debug
			DEBUGPATHLOG(FString::Printf(TEXT("- using existing anchor %s"),*Anchor->GetPathName()))
		}
		if (EndAnchor == NULL && bSpecifiedEnd)
		{
			EndAnchor = FindAnchor(goal, GoalLocation, false, bOnlyCheckVisible, EndDist);
			if ( goal )
			{
				goal->NotifyAnchorFindingResult(EndAnchor, this);
			}
			if ( !EndAnchor )
			{
				return 0.f;
			}
		}
		
		if ( EndAnchor == Anchor )
		{
			// no way to get closer on the navigation network
			Controller->RouteCache_Empty();

			INT PassedAnchor = 0;
			if ( ReachedDestination(Location, Anchor->Location, goal) )
			{
				PassedAnchor = 1;
				if ( !goal )
				{
					return 0.f;
				}
			}
			else
			{
				// if on route (through or past anchor), keep going
				FVector GoalAnchor = GoalLocation - Anchor->Location;
				GoalAnchor = GoalAnchor.SafeNormal();
				FVector ThisAnchor = Location - Anchor->Location;
				ThisAnchor = ThisAnchor.SafeNormal();
				if ( (ThisAnchor | GoalAnchor) > 0.9 )
					PassedAnchor = 1;
			}

			if ( PassedAnchor )
			{
				ANavigationPoint* N = Cast<ANavigationPoint>(goal);
				if (N != NULL)
				{
					Controller->RouteCache_AddItem(N);
				}
			}
			else
			{
				Controller->RouteCache_AddItem(Anchor);
			}
			return (GoalLocation - Location).Size();
		}
	}
	else
	{
		//debug
		DEBUGPATHLOG(FString::Printf(TEXT("- using existing anchors %s and %s"),*Anchor->GetPathName(),*EndAnchor->GetPathName()))
	}

	for (ANavigationPoint *Nav = GWorld->GetFirstNavigationPoint(); Nav != NULL; Nav = Nav->nextNavigationPoint)
	{
		if (bCanFly && !Nav->bFlyingPreferred)
		{
			Nav->TransientCost += 4000;
		}

		Nav->ClearForPathFinding();
	}

	if ( EndAnchor )
	{
		Controller->MarkEndPoints(EndAnchor, goal, GoalLocation);
	}

	GWorld->FarMoveActor(this, RealLocation, 1, 1);
	Anchor->visitedWeight = appRound(StartDist);
	if ( bSpecifiedEnd )
	{
		NodeEval = &FindEndPoint;
	}

	// verify the anchors are on the same network
	if( Anchor != NULL && 
		EndAnchor != NULL && 
		Anchor->NetworkID != -1 &&
		EndAnchor->NetworkID != -1 &&
		Anchor->NetworkID != EndAnchor->NetworkID )
	{
		//debug
		DEBUGPATHLOG(FString::Printf(TEXT("Attempt to find path from %s to %s failed because they are on unconnected networks (%i/%i)"),*Anchor->GetFullName(),*EndAnchor->GetFullName(),Anchor->NetworkID,EndAnchor->NetworkID))
		return 0.f;
	}

	//debug
	DEBUGPATHLOG(FString::Printf(TEXT("%s - searching for path from %s to %s"), *GetName(), *Anchor->GetPathName(),*EndAnchor->GetPathName()))

	Controller->eventSetupSpecialPathAbilities();

	if( MaxPathLength == 0.f )
	{
		MaxPathLength = UCONST_BLOCKEDPATHCOST;
	}

	// allow Controller a chance to override with a cached path
	if (bSpecifiedEnd && Controller->OverridePathTo(EndAnchor, goal, GoalLocation, bWeightDetours, BestWeight))
	{
		return BestWeight;
	}

	Controller->RouteCache_Empty();

	FLOAT Result = 0.f;
	switch( PathSearchType )
	{
		case PST_NewBestPathTo:
			{
				Result = NewBestPathTo(this,Anchor,EndAnchor,bReturnPartial);
			}
			break;
		case PST_Default:   // FALL THRU
		case PST_Breadth:	// FALL THRU
		default:
			ANavigationPoint* BestDest = BestPathTo( NodeEval, Anchor, &BestWeight, bWeightDetours );
			if ( BestDest )
			{
				//debug
				DEBUGPATHLOG(FString::Printf(TEXT("- found path!")))

				Controller->SetRouteCache( BestDest, StartDist, EndDist );
				Result = BestWeight;
			}
			else
			{
				//debug
				DEBUGPATHLOG(FString::Printf(TEXT("NO PATH!")))

				Result = SecondRouteAttempt(Anchor, EndAnchor, NodeEval, BestWeight, goal, GoalLocation, StartDist, EndDist);
			}
	}

	return Result;
}

ANavigationPoint* ANavigationPoint::SpecifyEndAnchor(APawn* RouteFinder)
{
	return this;
}

UBOOL AActor::AnchorNeedNotBeReachable()
{
	return (Physics == PHYS_Falling) || (Physics == PHYS_Projectile);
}

/** SecondRouteAttempt()
Allows a second try at finding a route.  Not used by base pawn implementation.  See AVehicle implementation, where it attempts to find a route for the dismounted driver
*/
FLOAT APawn::SecondRouteAttempt(ANavigationPoint* Anchor, ANavigationPoint* EndAnchor, NodeEvaluator NodeEval, FLOAT BestWeight, AActor *goal, FVector GoalLocation, FLOAT StartDist, FLOAT EndDist)
{
	return 0.f;
}

void AController::MarkEndPoints(ANavigationPoint* EndAnchor, AActor* Goal, const FVector& GoalLocation)
{
	if (Pawn != NULL)
	{
		Pawn->MarkEndPoints(EndAnchor, Goal, GoalLocation);
	}
}

void APawn::MarkEndPoints(ANavigationPoint* EndAnchor, AActor* Goal, const FVector& GoalLocation)
{	
	if (EndAnchor != NULL)
	{
		EndAnchor->bEndPoint = 1;
	}
}

/* AddPath()
add a path to a sorted path list - sorted by distance
*/
void FSortedPathList::AddPath(ANavigationPoint *node, INT dist)
{
	int n=0;
	if ( numPoints > 8 )
	{
		if ( dist > Dist[numPoints/2] )
		{
			n = numPoints/2;
			if ( (numPoints > 16) && (dist > Dist[n + numPoints/4]) )
				n += numPoints/4;
		}
		else if ( (numPoints > 16) && (dist > Dist[numPoints/4]) )
			n = numPoints/4;
	}

	while ((n < numPoints) && (dist > Dist[n]))
		n++;

	if (n < MAXSORTED)
	{
		if (n == numPoints)
		{
			Path[n] = node;
			Dist[n] = dist;
			numPoints++;
		}
		else
		{
		ANavigationPoint *nextPath = Path[n];
			INT nextDist = Dist[n];
		Path[n] = node;
		Dist[n] = dist;
		if (numPoints < MAXSORTED)
			numPoints++;
		n++;
		while (n < numPoints)
		{
			ANavigationPoint *afterPath = Path[n];
				INT afterDist = Dist[n];
			Path[n] = nextPath;
				Dist[n] = nextDist;
			nextPath = afterPath;
			nextDist = afterDist;
			n++;
		}
	}
}
}

//-------------------------------------------------------------------------------------------------
/** BestPathTo()
* Search for best (or satisfactory) destination in NavigationPoint network, as defined by NodeEval function.  Nodes are visited in the order of least cost.
* A sorted list of nodes is maintained - the first node on the list is visited, and all reachable nodes attached to it (which haven't already been visited
* at a lower cost) are inserted into the list. Returns best next node when NodeEval function returns 1.
* @param NodeEval: function pointer to function used to evaluate nodes
* @param start:  NavigationPoint which is the starting point for the traversal of the navigation network.  
* @param Weight:  starting value defines minimum acceptable evaluated value for destination node.
* @returns recommended next node.
*/
ANavigationPoint* APawn::BestPathTo(NodeEvaluator NodeEval, ANavigationPoint *start, FLOAT *Weight, UBOOL bWeightDetours)
{
	SCOPE_CYCLE_COUNTER(STAT_PathFinding_BestPathTo);

	//debug
	DEBUGPATHONLY(FlushPersistentDebugLines();)
	DEBUGPATHONLY(UWorld::VerifyNavList(*FString::Printf(TEXT("BESTPATHTO %s %s"), *GetName(), *start->GetFullName() ));)

	ANavigationPoint* currentnode = start;
	ANavigationPoint* LastAdd = currentnode;
	ANavigationPoint* BestDest = NULL;

	INT iRadius = appTrunc(CylinderComponent->CollisionRadius);
	INT iHeight = appTrunc(CylinderComponent->CollisionHeight);
	INT iMaxFallSpeed = appTrunc(GetAIMaxFallSpeed());
	INT moveFlags = calcMoveFlags();
	
	if ( bCanCrouch )
	{
		iHeight = appTrunc(CrouchHeight);
		iRadius = appTrunc(CrouchRadius);
	}
	INT n = 0;

	// While still evaluating a node
	while ( currentnode )
	{
		// Mark as visited
		currentnode->bAlreadyVisited = true;

		//debug
		DEBUGPATHLOG(FString::Printf(TEXT("-> Distance to %s is %d"), *currentnode->GetName(), currentnode->visitedWeight))

		// Evaluate the node
		FLOAT thisWeight = (*NodeEval)(currentnode, this, *Weight);
		// If the weight is better than our last best weight
		if ( thisWeight > *Weight )
		{
			// Keep current node as our dest
			*Weight = thisWeight;
			BestDest = currentnode;
		}
		// If we found a "perfect" node
		if ( *Weight >= 1.f )
		{
			// Return detour check
			return CheckDetour(BestDest, start, bWeightDetours);
		}

		// Otherwise, if we have exceeded the max number of searches
		if ( n++ > 200 )
		{
			// If we have found something worth anything
			if ( *Weight > 0 )
			{
				// Return detour check
				return CheckDetour(BestDest, start, bWeightDetours);
			}
			// Otherwise, allow another 50 checks (??)
			else
			{
				n = 150;
			}
		}

		// Search through each path away from this node
		INT nextweight = 0;
		for ( INT i=0; i<currentnode->PathList.Num(); i++ )
		{
			UReachSpec *spec = currentnode->PathList(i);
			if (spec != NULL && *spec->End != NULL)
			{
				//debug
				DEBUGPATHLOG(FString::Printf(TEXT("-> check path from %s to %s with %d, %d, supports? %s, visited? %s"),*spec->Start->GetName(), *spec->End != NULL ? *spec->End->GetName() : TEXT("NULL"), spec->CollisionRadius, spec->CollisionHeight, spec->supports(iRadius, iHeight, moveFlags, iMaxFallSpeed) ? TEXT("True") : TEXT("False"), spec->End->bAlreadyVisited ? TEXT("TRUE") : TEXT("FALSE")))
			}

			// If path hasn't already been visited and it supports the pawn
			if ( spec && *spec->End != NULL && !spec->End->bAlreadyVisited && spec->supports(iRadius, iHeight, moveFlags, iMaxFallSpeed) )
			{
				//debug
				DEBUGPATHONLY(DrawDebugLine(currentnode->Location,spec->End->Location,0,64,0,TRUE);)

				// Get the cost for this path
				nextweight = spec->CostFor(this);

				// If path is not blocked
				if ( nextweight < UCONST_BLOCKEDPATHCOST )
				{
					ANavigationPoint* endnode = spec->End;

					// Don't allow zero or negative weight - could create a loop
					if ( nextweight <= 0 )
					{
						//debug
						debugf(TEXT("WARNING!!! - negative weight %d from %s to %s (%s)"), nextweight, *currentnode->GetName(), *endnode->GetName(), *spec->GetName() );

						nextweight = 1;
					}

					// Get total path weight for the next node
					INT newVisit = nextweight + currentnode->visitedWeight; 

					//debug
					DEBUGPATHLOG(FString::Printf(TEXT("--> Path from %s to %s costs %d total %d"), *currentnode->GetName(), *endnode->GetName(), nextweight, newVisit))

					if ( endnode->visitedWeight > newVisit )
					{
						//debug
						DEBUGPATHLOG(FString::Printf(TEXT("--> found better path to %s through %s"), *endnode->GetName(), *currentnode->GetName()))
//						PRINTPATHLIST(endnode);

						//debug
						DEBUGPATHLOG(FString::Printf(TEXT("BPT set %s prev path = %s"), *endnode->GetName(), *currentnode->GetName()))

						// found a better path to endnode
						endnode->previousPath = currentnode;
						if ( endnode->prevOrdered ) //remove from old position
						{
							endnode->prevOrdered->nextOrdered = endnode->nextOrdered;
							if (endnode->nextOrdered)
							{
								endnode->nextOrdered->prevOrdered = endnode->prevOrdered;
							}
							if ( (LastAdd == endnode) || (LastAdd->visitedWeight > endnode->visitedWeight) )
							{
								LastAdd = endnode->prevOrdered;
							}
							endnode->prevOrdered = NULL;
							endnode->nextOrdered = NULL;
						}
						endnode->visitedWeight = newVisit;

						// LastAdd is a good starting point for searching the list and inserting this node
						ANavigationPoint* nextnode = LastAdd;
#if PATH_LOOP_TESTING
						TArray<ANavigationPoint*> NavList;
#endif
						if ( nextnode->visitedWeight <= newVisit )
						{
							while (nextnode->nextOrdered && (nextnode->nextOrdered->visitedWeight < newVisit))
							{
								nextnode = nextnode->nextOrdered;
#if PATH_LOOP_TESTING
								if (NavList.ContainsItem(nextnode))
								{
									//debug
									debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s BPT [NEXTORDERED LOOP] Encountered a loop trying to insert %s into nextordered"),
										*GetName(), *endnode->GetFullName() );

									PRINTDEBUGPATHLOG(TRUE);
									break;
								}
								else
								{
									NavList.AddItem(nextnode);
								}
#endif
							}
						}
						else
						{
							while ( nextnode->prevOrdered && (nextnode->visitedWeight > newVisit) )
							{
								nextnode = nextnode->prevOrdered;
#if PATH_LOOP_TESTING
								if (NavList.ContainsItem(nextnode))
								{
									//debug
									debugf(NAME_Warning,TEXT("PATH ERROR!!!! %s BPT [PREVORDERED LOOP] Encountered a loop trying to insert %s into prevordered"),
										*GetName(), *endnode->GetFullName() );

									PRINTDEBUGPATHLOG(TRUE);
									break;
								}
								else
								{
									NavList.AddItem(nextnode);
								}
#endif
							}
						}

						if (nextnode->nextOrdered != endnode)
						{
							if (nextnode->nextOrdered)
								nextnode->nextOrdered->prevOrdered = endnode;
							endnode->nextOrdered = nextnode->nextOrdered;
							nextnode->nextOrdered = endnode;
							endnode->prevOrdered = nextnode;
						}
						LastAdd = endnode;
					}
				}
				else
				{
					//debug
					DEBUGPATHONLY(DrawDebugLine(currentnode->Location,spec->End->Location,64,0,0,TRUE);)
				}
			}
		}
		currentnode = currentnode->nextOrdered;
	}
/* UNCOMMENT TO HAVE FAILED ROUTE FINDING ATTEMPTS DISPLAYED
	if ( !BestDest && (NodeEval == &FindEndPoint) )
	{
		currentnode = start;
		GWorld->PersistentLineBatcher->BatchedLines.Empty();
		if ( currentnode )
		{
			for ( INT i=0; i<currentnode->PathList.Num(); i++ )
			{
				if ( currentnode->PathList(i)->End && currentnode->PathList(i)->End->bAlreadyVisited )
					GWorld->PersistentLineBatcher->DrawLine(currentnode->PathList(i)->End->Location, currentnode->Location, FColor(0, 255, 0));
			}
		}
		while ( currentnode )
		{
			for ( INT i=0; i<currentnode->PathList.Num(); i++ )
			{
				if ( currentnode->PathList(i)->End )
				{
					if ( currentnode->PathList(i)->End->bAlreadyVisited )
						GWorld->PersistentLineBatcher->DrawLine(currentnode->PathList(i)->End->Location, currentnode->Location, FColor(0, 0, 255));
					else
						GWorld->PersistentLineBatcher->DrawLine(currentnode->PathList(i)->End->Location, currentnode->Location, FColor(255, 0, 0));
				}
			}
			currentnode = currentnode->nextOrdered;
		}
		WorldInfo->bPlayersOnly = 1;
		return NULL;
	}
*/
	return CheckDetour(BestDest, start, bWeightDetours);
}

ANavigationPoint* APawn::CheckDetour(ANavigationPoint* BestDest, ANavigationPoint* Start, UBOOL bWeightDetours)
{
	if ( !bWeightDetours || !Start || !BestDest || (Start == BestDest) || !Anchor )
	{
		return BestDest;
	}

	ANavigationPoint* DetourDest = NULL;
	FLOAT DetourWeight = 0.f;

	// FIXME - mark list to ignore (if already in route)
	for ( INT i=0; i<Anchor->PathList.Num(); i++ )
	{
		UReachSpec *spec = Anchor->PathList(i);
		if (*spec->End != NULL && spec->End->visitedWeight < 2.f * MAXPATHDIST)
		{
			UReachSpec *Return = spec->End->GetReachSpecTo(Anchor);
			if ( Return && !Return->IsForced() )
			{
				spec->End->LastDetourWeight = spec->End->eventDetourWeight(this,spec->End->visitedWeight);
				if ( spec->End->LastDetourWeight > DetourWeight )
					DetourDest = spec->End;
			}
		}
	}
	if ( !DetourDest )
	{
		return BestDest;
	}

	ANavigationPoint *FirstPath = BestDest;
	// check that detourdest doesn't occur in route
	for ( ANavigationPoint *Path=BestDest; Path!=NULL; Path=Path->previousPath )
	{
		FirstPath = Path;
		if ( Path == DetourDest )
			return BestDest;
	}

	// check that AI really wants to detour
	if ( !Controller )
		return BestDest;
	Controller->RouteGoal = BestDest;
	Controller->RouteDist = BestDest->visitedWeight;
	if ( !Controller->eventAllowDetourTo(DetourDest) )
		return BestDest;

	// add detourdest to start of route
	if ( FirstPath != Anchor )
	{
		FirstPath->previousPath = Anchor;
		Anchor->previousPath = DetourDest;
		DetourDest->previousPath = NULL;
	}
	else
	{
		for ( ANavigationPoint *Path=BestDest; Path!=NULL; Path=Path->previousPath )
			if ( Path->previousPath == Anchor )
			{
				Path->previousPath = DetourDest;
				DetourDest->previousPath = Anchor;
				break;
			}
	}

	return BestDest;
}

/* SetRouteCache() puts the first 16 navigationpoints in the best route found in the
Controller's RouteCache[].
*/
void AController::SetRouteCache(ANavigationPoint *EndPath, FLOAT StartDist, FLOAT EndDist)
{
	RouteGoal = EndPath;
	if( !EndPath )
	{
		return;
	}

	RouteDist = EndPath->visitedWeight + EndDist;

	// reverse order of linked node list
	EndPath->nextOrdered = NULL;
#if PATH_LOOP_TESTING
	TArray<ANavigationPoint*> NavList;
#endif
	while ( EndPath->previousPath)
	{
		//debug
		DEBUGPATHONLY(DrawDebugLine(EndPath->Location,EndPath->previousPath->Location,0,255,0,TRUE);)

		EndPath->previousPath->nextOrdered = EndPath;
		EndPath = EndPath->previousPath;

#if PATH_LOOP_TESTING
		if (NavList.ContainsItem(EndPath))
		{
			//debug
			debugf(NAME_Warning,TEXT("PATH ERROR!!!! SetRouteCache [PREVPATH LOOP] %s Encountered loop [%s to %s] "),
					*Pawn->GetName(),*Pawn->Anchor->GetName(),*EndPath->GetName());
			PRINTDEBUGPATHLOG(TRUE);
		}
		else
		{
			NavList.AddItem(EndPath);
		}
#endif
	}

	// if the pawn is on the start node, then the first node in the path should be the next one
	if ( Pawn && (StartDist > 0.f) )
	{
		// if pawn not on the start node, check if second node on path is a better destination
		if ( EndPath->nextOrdered )
		{
			TArray<FNavigationOctreeObject*> Objects;
			GWorld->NavigationOctree->PointCheck(Pawn->Location, Pawn->GetCylinderExtent(), Objects);
			UBOOL bAlreadyOnPath = false;
			// if already on a reachspec to a further node on the path, then keep going
			for (INT i = 0; i < Objects.Num() && !bAlreadyOnPath; i++)
			{
				UReachSpec* Spec = Objects(i)->GetOwner<UReachSpec>();
				if (Spec != NULL)
				{
					for (ANavigationPoint* NextPath = EndPath->nextOrdered; NextPath != NULL && !bAlreadyOnPath; NextPath = NextPath->nextOrdered)
					{
						if (Spec->End == NextPath)
						{
							bAlreadyOnPath = true;
							EndPath = NextPath;
						}
					}
				}
			}
			if (!bAlreadyOnPath)
			{
				// check if pawn is closer to second node than first node is
				FLOAT TwoDist = (Pawn->Location - EndPath->nextOrdered->Location).Size();
				FLOAT PathDist = (EndPath->Location - EndPath->nextOrdered->Location).Size();
				FLOAT MaxDist = 0.75f * MAXPATHDIST;
				if ( EndPath->nextOrdered->IsA(AVolumePathNode::StaticClass()) )
				{
					MaxDist = ::Max(MaxDist,EndPath->nextOrdered->CylinderComponent->CollisionRadius);
				}
				if (TwoDist < MaxDist && TwoDist < PathDist)
				{
					// make sure second node is reachable
					FCheckResult Hit(1.f);
					GWorld->SingleLineCheck( Hit, this, EndPath->nextOrdered->Location, Pawn->Location, TRACE_World|TRACE_StopAtAnyHit );
					if ( !Hit.Actor	&& Pawn->actorReachable(EndPath->nextOrdered, 1, 1) )
					{
						//debugf(TEXT("Skipping Anchor"));
						EndPath = EndPath->nextOrdered;
					}
				}
			}
		}
	}
	else if( EndPath->nextOrdered )
	{
		EndPath = EndPath->nextOrdered;
	}

	// place all of the path into the controller route cache
#if PATH_LOOP_TESTING
	ANavigationPoint* EP = EndPath;
#endif
	while (EndPath != NULL)
	{
#if PATH_LOOP_TESTING
		if (RouteCache.ContainsItem(EndPath))
		{
			debugf(NAME_Warning,TEXT("PATH ERROR!!!! SetRouteCache [NEXTORDERED LOOP] %s Encountered loop [%s to %s] "),
				*Pawn->GetName(),*Pawn->Anchor->GetName(),*EP->GetName());
			PRINTDEBUGPATHLOG(TRUE);
			break;
		}
#endif
		RouteCache_AddItem( EndPath );
		EndPath = EndPath->nextOrdered;
	}

	if( Pawn && RouteCache.Num() > 1 )
	{
		ANavigationPoint *FirstPath = RouteCache(0);
		UReachSpec* NextSpec = NULL;
		if( FirstPath )
		{
			ANavigationPoint *SecondPath = RouteCache(1);
			if( SecondPath )
			{
				NextSpec = FirstPath->GetReachSpecTo(SecondPath);
			}
		}
		if ( NextSpec )
		{
			Pawn->NextPathRadius = NextSpec->CollisionRadius;
		}
		else
		{
			Pawn->NextPathRadius = 0.f;
		}
	}
}

/** initializes us with the given user and creates ReachSpecs to connect ourselves to the given endpoints,
 * using the given ReachSpec as a template
 * @param InUser the Controller that will be using us for navigation
 * @param Point1 the first NavigationPoint to connect to
 * @param Point2 the second NavigationPoint to connect to
 * @param SpecTemplate the ReachSpec to use as a template for the ReachSpecs we create
 */
void ADynamicAnchor::Initialize(AController* InUser, ANavigationPoint* Point1, ANavigationPoint* Point2, UReachSpec* SpecTemplate)
{
	checkSlow(InUser != NULL && InUser->Pawn != NULL && Point1 != NULL && Point2 != NULL && SpecTemplate != NULL);

	CurrentUser = InUser;
	INT NewRadius = appTrunc(InUser->Pawn->CylinderComponent->CollisionRadius);
	INT NewHeight = appTrunc(InUser->Pawn->CylinderComponent->CollisionHeight);
	
	// construct reachspecs between ourselves and the two endpoints of the given reachspec
	// copy its path properties (collision size, reachflags, etc)

	//@FIXME: need to be able to copy the other reachspec but passing it as a template causes it to be set as the ObjectArchetype, which screws up GCing streaming levels
	//UReachSpec* NewSpec = ConstructObject<UReachSpec>(SpecTemplate->GetClass(), GetOuter(), NAME_None, 0, SpecTemplate, INVALID_OBJECT);
	UReachSpec* NewSpec = ConstructObject<UReachSpec>(SpecTemplate->GetClass(), GetOuter(), NAME_None, 0);
	NewSpec->reachFlags = SpecTemplate->reachFlags;
	NewSpec->MaxLandingVelocity = SpecTemplate->MaxLandingVelocity;
	NewSpec->PathColorIndex = SpecTemplate->PathColorIndex;
	NewSpec->bCanCutCorners = SpecTemplate->bCanCutCorners;

	NewSpec->Start = this;
	NewSpec->End = Point1;
	NewSpec->End.Guid = Point1->NavGuid;
	NewSpec->Distance = appTrunc((NewSpec->End->Location - NewSpec->Start->Location).Size());
	NewSpec->bAddToNavigationOctree = FALSE; // unnecessary, since encompassed by ConnectTo
	NewSpec->bCanCutCorners = FALSE;
	NewSpec->CollisionRadius = NewRadius;
	NewSpec->CollisionHeight = NewHeight;
	PathList.AddItem(NewSpec);

	//@FIXME: need to be able to copy the other reachspec but passing it as a template causes it to be set as the ObjectArchetype, which screws up GCing streaming levels
	//NewSpec = ConstructObject<UReachSpec>(SpecTemplate->GetClass(), GetOuter(), NAME_None, 0, SpecTemplate, INVALID_OBJECT);
	NewSpec = ConstructObject<UReachSpec>(SpecTemplate->GetClass(), GetOuter(), NAME_None, 0);
	NewSpec->reachFlags = SpecTemplate->reachFlags;
	NewSpec->MaxLandingVelocity = SpecTemplate->MaxLandingVelocity;
	NewSpec->PathColorIndex = SpecTemplate->PathColorIndex;
	NewSpec->bCanCutCorners = SpecTemplate->bCanCutCorners;

	NewSpec->Start = this;
	NewSpec->End = Point2;
	NewSpec->End.Guid = Point2->NavGuid;
	NewSpec->Distance = appTrunc((NewSpec->End->Location - NewSpec->Start->Location).Size());
	NewSpec->bAddToNavigationOctree = FALSE; // unnecessary, since encompassed by ConnectTo
	NewSpec->bCanCutCorners = FALSE;
	NewSpec->CollisionRadius = NewRadius;
	NewSpec->CollisionHeight = NewHeight;
	PathList.AddItem(NewSpec);

	MaxPathSize.Height = InUser->Pawn->CylinderComponent->CollisionHeight;
	MaxPathSize.Radius = InUser->Pawn->CylinderComponent->CollisionRadius;

	// set our collision size in between that of the endpoints to make sure the AI doesn't get stuck beneath us in the case of one or both of the endpoints have a large size
	SetCollisionSize( Max<FLOAT>(Point1->CylinderComponent->CollisionRadius, Point2->CylinderComponent->CollisionRadius),
						Max<FLOAT>(Point1->CylinderComponent->CollisionHeight, Point2->CylinderComponent->CollisionHeight) );
	// add ourselves to the navigation octree so any other Pawns that need an Anchor here can find us instead of creating another
	AddToNavigationOctree();


}

void ADynamicAnchor::TickSpecial(FLOAT DeltaSeconds)
{
	if ( CurrentUser == NULL || CurrentUser->bDeleteMe || CurrentUser->Pawn == NULL || ( CurrentUser->Pawn->Anchor != this && CurrentUser->MoveTarget != this &&
																						PathList.FindItemIndex(CurrentUser->CurrentPath) == INDEX_NONE ) )
	{
		// try to find another user
		CurrentUser = NULL;
		for (AController* C = GWorld->GetFirstController(); C != NULL && CurrentUser == NULL; C = C->NextController)
		{
			if (C->Pawn != NULL && (C->Pawn->Anchor == this || C->MoveTarget == this && PathList.FindItemIndex(C->CurrentPath) == INDEX_NONE))
			{
				CurrentUser = C;
			}
		}
		// destroy if not in use
		if (CurrentUser == NULL)
		{
			GWorld->DestroyActor(this);
		}
	}
}
