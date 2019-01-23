/*=============================================================================
	UnReach.cpp: Reachspec creation and management

	These methods are members of the UReachSpec class, 

	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "UnPath.h"

INT APawn::calcMoveFlags()
{
	return ( bCanWalk * R_WALK + bCanFly * R_FLY + bCanSwim * R_SWIM + bJumpCapable * R_JUMP ); 
}

// DO!
// Handle paths that are blocked by our ultimate goal causing path finding to fail/flood network

/** returns whether this path is currently blocked and unusable to the given pawn */
UBOOL UReachSpec::IsBlockedFor(APawn* P)
{
	if (End->bBlocked || (End->bBlockedForVehicles && P != NULL && P->IsA(AVehicle::StaticClass())))
	{
		return TRUE;
	}
	else if( !P->CanUseReachSpec( this ) )
	{
		return TRUE;
	}
	else if (BlockedBy != NULL)
	{
		// verify that BlockedBy is still in the way
		//@warning: for consistency, this check should match the one in UReachSpec::PrepareForMove()
		FCheckResult Hit(1.0f);

		if (BlockedBy != P && BlockedBy->bBlocksNavigation)
		{
			// offset the trace by the Pawn's MaxStepHeight so that objects the Pawn could walk over are not detected
			FLOAT StepAdjustment = P->MaxStepHeight * 0.5f;
			if ( !BlockedBy->ActorLineCheck( Hit, End->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - End->CylinderComponent->CollisionHeight + StepAdjustment),
												Start->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - Start->CylinderComponent->CollisionHeight + StepAdjustment),
												FVector(CollisionRadius, CollisionRadius, CollisionHeight - StepAdjustment), TRACE_Pawns | TRACE_Others | TRACE_Blocking ) )
			{
				return TRUE;
			}
		}
		
		BlockedBy = NULL;
		return FALSE;
	}
	else
	{
		return FALSE;
	}
}

/* CostFor()
Adjusted "length" of this path.
Values >= BLOCKEDPATHCOST indicate this path is blocked to the pawn
*/
INT UReachSpec::CostFor(APawn *P)
{
	INT Cost = 0;

	if (IsBlockedFor(P) || (End->bMayCausePain && End->PhysicsVolume && End->PhysicsVolume->WillHurt(P)))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	else if( CollisionHeight >= P->FullHeight )
	{
// FIXMESTEVE	if ( reachFlags & R_SWIM )
//		return appTrunc(Distance * SWIMCOSTMULTIPLIER) + End->Cost;
		Cost = Distance + End->Cost;
	}
	else
	{
		Cost = appTrunc((CROUCHCOSTMULTIPLIER * 1.f/P->CrouchedPct) * Distance + End->Cost);
	}
	if (P->Controller->InUseNodeCostMultiplier > 0.f)
	{
		// Scale cost if there is someone sitting on the end of this spec
		ANavigationPoint* EndNav = *End;
		if (EndNav->AnchoredPawn != NULL)
		{
			//debugf(TEXT("-! %s has anchored pawn %s"),*EndNav->GetName(),*EndNav->AnchoredPawn->GetName());
			Cost = appTrunc(Cost * P->Controller->InUseNodeCostMultiplier);
		}
		else if (EndNav->LastAnchoredPawnTime > 0.f)
		{
			// scale cost over time for a ghost effect
			FLOAT Delta = GWorld->GetTimeSeconds() - EndNav->LastAnchoredPawnTime;
			//debugf(TEXT("-! %s had anchored pawn %2.3f seconds ago"),*EndNav->GetName(),Delta);
			// if it has been less than 5 seconds since something anchored here
			if (Delta <= 5.f)
			{
				// scale based on that duration
				FLOAT AdjustedMultiplier = P->Controller->InUseNodeCostMultiplier * 0.5;
				AdjustedMultiplier -= (AdjustedMultiplier - 1.0f) * (Delta / 5.0f);
				Cost = appTrunc(Cost * AdjustedMultiplier);
			}
			else
			{
				// otherwise clear the time and don't scale
				EndNav->LastAnchoredPawnTime = 0.f;
			}
		}
	}

	return Cost;
}

void UReachSpec::execCostFor(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(APawn, P);
	P_FINISH;

	*(INT*)Result = (End.Nav != NULL) ? CostFor(P) : UCONST_BLOCKEDPATHCOST;
}

INT UReachSpec::AdjustedCostFor( APawn* P, ANavigationPoint* Anchor, ANavigationPoint* Goal, INT Cost )
{
	// scale cost to make paths away from the goal direction more expensive
	const FLOAT DotToGoal = Clamp<FLOAT>(1.f - ((End->Location - Start->Location).SafeNormal2D() | (Goal->Location - Anchor->Location).SafeNormal2D()),0.1f,2.f);

	//debugf(TEXT("-- %s to %s dot to goal %f extra cost %f"), *Start->GetName(), *End->GetName(), DotToGoal, appTrunc(((End->Location - Goal->Location).SizeSquared() * DotToGoal) + ((Distance * Distance) * DotToGoal)) );

	// Additional cost based on the distance to goal, and based on the distance travelled
	Cost += appTrunc(((End->Location - Goal->Location).SizeSquared() * DotToGoal) + ((Distance * Distance) * DotToGoal));

	// Allow pawn to adjust the cost
	Cost = P->AdjustCostForReachSpec( this, Cost );

	return Cost;
}

/* CostFor()
Adjusted "length" of this path.
Values >= BLOCKEDPATHCOST indicate this path is blocked to the pawn
*/
INT UAdvancedReachSpec::CostFor(APawn *P)
{
	if ( !P->Controller || !P->Controller->bCanDoSpecial )
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	return Super::CostFor(P);
}

/* CostFor()
Adjusted "length" of this path.
Values >= BLOCKEDPATHCOST indicate this path is blocked to the pawn
*/
INT ULadderReachSpec::CostFor(APawn *P)
{
	if ( !P->bCanClimbLadders )
	{
		return UCONST_BLOCKEDPATHCOST;
	}

	INT Cost = Distance + End->Cost;

	return Cost;
}

INT UProscribedReachSpec::CostFor(APawn *P)
{
	return UCONST_BLOCKEDPATHCOST;
}

INT UForcedReachSpec::CostFor(APawn *P)
{
	INT Cost = 0;
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	else if ( End->bSpecialForced )
	{
		Cost = Distance + End->eventSpecialCost(P,this);
	}
	else
	{
		Cost = Distance + End->Cost;
	}

	return Cost;
}

INT USlotToSlotReachSpec::CostFor(APawn* P)
{
	return Super::CostFor(P);
}

INT UMantleReachSpec::CostFor(APawn *P)
{
	if (Start->AnchoredPawn != NULL && Start->AnchoredPawn != P)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (*End != NULL && End->AnchoredPawn != NULL && End->AnchoredPawn != P)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (!P->bCanMantle)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}

	return Distance + End->Cost;
}

INT USwatTurnReachSpec::CostFor(APawn *P)
{
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}

	return Distance + End->Cost;
}

INT UCoverSlipReachSpec::CostFor(APawn *P)
{
	INT Cost = 0;
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	
	Cost = Distance + End->eventSpecialCost(P,this);

	return Cost;
}

INT UWallTransReachSpec::CostFor(APawn *P)
{
	if (!P->bCanClimbCeilings)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	return Distance;
}

INT UFloorToCeilingReachSpec::CostFor(APawn *P)
{
	if (!P->bCanClimbCeilings)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	// Don't allow wretch to drop down onto or jump up onto a pawn
	// already occupying the end node
	if( End->AnchoredPawn && End->AnchoredPawn != P )
	{
		return UCONST_BLOCKEDPATHCOST;
	}

	// F/C transitions ignore distance
	return 100 + End->Cost;
}

INT UFloorToCeilingReachSpec::AdjustedCostFor( APawn* P, ANavigationPoint* Anchor, ANavigationPoint* Goal, INT Cost )
{
	// Always make transition cheapest
	return Cost;
}

INT UCeilingReachSpec::CostFor(APawn *P)
{
	if (!P->bCanClimbCeilings)
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	if (IsBlockedFor(P))
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	return Distance + End->Cost;
}

INT UCeilingReachSpec::AdjustedCostFor( APawn* P, ANavigationPoint* Anchor, ANavigationPoint* Goal, INT Cost )
{
	// Favor nodes in line towards the goal
	FLOAT DotToGoal		= (End->Location - Anchor->Location).SafeNormal2D() | (Goal->Location - Anchor->Location).SafeNormal2D();
	FLOAT AdjustedCost	= (End->Location - Goal->Location).SizeSquared2D();

	// Scale the cost to bias our search order
	Cost += appTrunc(AdjustedCost * Clamp<FLOAT>(1.f - DotToGoal,0.01f,2.f));

	return Cost;
}

INT UTeleportReachSpec::CostFor(APawn* P)
{
	return (Start == NULL || !Start->CanTeleport(P)) ? UCONST_BLOCKEDPATHCOST : Super::CostFor(P);
}

UBOOL UReachSpec::PrepareForMove(AController* C)
{
	UBOOL bResult = FALSE;
	if (C != NULL && C->Pawn != NULL && C->Pawn->CylinderComponent != NULL)
	{
		// check for obstructions
		//@warning: for consistency, this check should match the one in UReachSpec::IsBlockedFor()
		FVector Dir = (End->Location - Start->Location).SafeNormal();
		FMemMark Mark(GMem);
		// offset the trace by the Pawn's MaxStepHeight so that objects the Pawn could walk over are not detected
		FLOAT StepAdjustment = C->Pawn->MaxStepHeight * 0.5f;
		FCheckResult* Hits = GWorld->MultiLineCheck( GMem, End->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - End->CylinderComponent->CollisionHeight + StepAdjustment),
													Start->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - Start->CylinderComponent->CollisionHeight + StepAdjustment),
													FVector(CollisionRadius, CollisionRadius, CollisionHeight - StepAdjustment), TRACE_Pawns | TRACE_Others | TRACE_Blocking, C->Pawn );
		for (FCheckResult* CheckHit = Hits; CheckHit != NULL && !bResult; CheckHit = CheckHit->GetNext())
		{
			if ( CheckHit->Actor != NULL && CheckHit->Actor->bBlocksNavigation && CheckHit->Actor != Start && CheckHit->Actor != End &&
				(Dir | (CheckHit->Actor->Location - Start->Location).SafeNormal()) > KINDA_SMALL_NUMBER )
			{
				ANavigationPoint* WorkaroundEnd = End;
				// if the obstruction overlaps our endpoint
				if (CheckHit->Actor->GetComponentsBoundingBox(FALSE).IsInside(End->Location))
				{
					// if End isn't the AI's goal, try to skip End and go around it
					if (C->RouteGoal != End && C->RouteCache.Num() > 1 && C->RouteCache(1) != End && Cast<ANavigationPoint>(C->RouteCache(1)) != NULL)
					{
						WorkaroundEnd = (ANavigationPoint*)C->RouteCache(1);
					}
					else
					{
						if (!C->eventHandlePathObstruction(CheckHit->Actor))
						{
							debugfSuppressed(NAME_DevPath, TEXT("AI goal %s obstructed by bBlocksNavigation Actor %s"), *End->GetName(), *CheckHit->Actor->GetName());
							C->MoveTimer = -1.f;
							BlockedBy = CheckHit->Actor;
						}
						bResult = TRUE;
					}
				}
				if (!bResult)
				{
					// try to find a way around; check perpendicular to path direction
					FLOAT BlockerRadius, BlockerHeight;
					CheckHit->Actor->GetBoundingCylinder(BlockerRadius, BlockerHeight);
					FVector Side = (Dir ^ FRotationMatrix(Dir.Rotation()).TransformNormal(FVector(0.f, 0.f, 1.f))) * (BlockerRadius + 2.0f * C->Pawn->CylinderComponent->CollisionRadius);
					FVector CheckPoints[2] = {CheckHit->Actor->Location + Side, CheckHit->Actor->Location - Side};
					for (INT i = 0; i < ARRAY_COUNT(CheckPoints) && !bResult; i++)
					{
						// since the ReachSpec covers the majority of the area we're trying to move through,
						// we'll just do some simple forward and down line traces to verify reachability rather than using the expensive PointReachable()
						FCheckResult Hit(1.0f);
						FVector Extent = C->Pawn->bCanCrouch ? C->Pawn->GetCrouchSize() : C->Pawn->GetDefaultCollisionSize();
						Extent.Z = Max<FLOAT>(Extent.Z - C->Pawn->MaxStepHeight, 1.f);
						// if trace from Start to CheckPoints[i] to WorkaroundEnd doesn't hit anything
						// if (WorkaroundEnd != End) also make sure that CheckPoints[i] to WorkaroundEnd doesn't hit the obstruction
						if ( GWorld->SingleLineCheck(Hit, C->Pawn, CheckPoints[i], Start->Location, TRACE_World | TRACE_StopAtAnyHit, Extent) &&
							GWorld->SingleLineCheck(Hit, C->Pawn, WorkaroundEnd->Location, CheckPoints[i], TRACE_World | TRACE_StopAtAnyHit, Extent) &&
							(WorkaroundEnd == End || CheckHit->Actor->ActorLineCheck(Hit, CheckPoints[i], WorkaroundEnd->Location, FVector(0.f, 0.f, 0.f), TRACE_World | TRACE_StopAtAnyHit)) )
						{
							// if Pawn can't fly, verify valid landing
							bResult = C->Pawn->bCanFly;
							if (!bResult)
							{
								FVector Down(0.f, 0.f, C->Pawn->CylinderComponent->CollisionHeight + C->Pawn->MaxStepHeight + (C->Pawn->bCanJump ? C->Pawn->MaxJumpHeight : 0));
								bResult = (!GWorld->SingleLineCheck(Hit, C->Pawn, CheckPoints[i] - Down, CheckPoints[i], TRACE_World) && Hit.Normal.Z >= C->Pawn->WalkableFloorZ);
							}
							if (bResult)
							{
								// spawn a dynamic anchor at this location for the pawn to use to get around the obstacle
								// we need StaticLoadClass() here to verify that the native class's defaults have been loaded, since it probably isn't referenced anywhere
								StaticLoadClass(ADynamicAnchor::StaticClass(), NULL, TEXT("Engine.DynamicAnchor"), NULL, LOAD_None, NULL);
								ADynamicAnchor* DynamicAnchor = (ADynamicAnchor*)GWorld->SpawnActor(ADynamicAnchor::StaticClass(), NAME_None, CheckPoints[i]);
								if (DynamicAnchor != NULL)
								{
									DynamicAnchor->Initialize(C, Start, WorkaroundEnd, this);
									if (C->Focus == C->MoveTarget)
									{
										C->Focus = DynamicAnchor;
									}
									C->MoveTarget = DynamicAnchor;
									if (End == WorkaroundEnd || C->RouteCache.Num() == 0)
									{
										// we are inserting an additional node on the bot's path
										C->RouteCache.InsertItem(DynamicAnchor, 0);
									}
									else
									{
										// we're skipping the old End, so replace it in the bot's path
										C->RouteCache(0) = DynamicAnchor;
									}
									C->Pawn->setMoveTimer(DynamicAnchor->Location - C->Pawn->Location);
								}
							}
						}
					}
					if (!bResult)
					{
						// we failed to find a way around
						if (!C->eventHandlePathObstruction(CheckHit->Actor))
						{
							C->MoveTimer = -1.f;
							BlockedBy = CheckHit->Actor;
						}
						bResult = TRUE;
					}
				}
			}
		}
		Mark.Pop();
	}

	return bResult;
}

UBOOL UForcedReachSpec::PrepareForMove(AController* C)
{
	if (Super::PrepareForMove(C))
	{
		return TRUE;
	}
	else if (C == NULL || C->Pawn == NULL)
	{
		return FALSE;
	}
	else if (End->bSpecialForced)
	{
		End->eventSuggestMovePreparation(C->Pawn);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/* Path Color
returns the color to use when displaying this path in the editor
 */
FPlane UReachSpec::PathColor()
{
	if ( (reachFlags & R_FLY) && !(reachFlags & R_WALK) )
	{
		if ( PathColorIndex < 1 )
			return  FPlane(1.f,0.5f,0.f,0.f);	// orange = narrow flying
		else
			return FPlane(1.f,0.5f,0.3f,0.f);		// light orange = flying
	}

	if ( reachFlags & R_HIGHJUMP )
		return FPlane(1.f,0.5f, 1.f,0.f);		// light purple = requires high jump

	switch ( PathColorIndex )
	{
		case 0:		return FPlane(0.f,0.f,1.f,0.f); // blue path = narrow
		case 1:		return FPlane(0.f,1.f,0.f,0.f);  // green path = normal
		case 2:		return FPlane(1.f,1.f,1.f,0.f);  // white path = very wide
		case 3:		return FPlane(1.f,0.5f,1.f,0.f); // pink path = wide
		case 4:		return FPlane(0.f,1.f,1.f,0.f);	 // teal path = enourmous
	}

	return FPlane(0.f,0.f,1.f,0.f); // blue path = narrow
}

/* operator <=
Used for comparing reachspecs reach requirements
less than means that this has easier reach requirements (equal or easier in all categories,
does not compare distance, start, and end
*/
int UReachSpec::operator<= (const UReachSpec &Spec)
{
	if (IsProscribed())
	{
		return 1;
	}

	if( GetClass() != Spec.GetClass() )
	{
		return 0;
	}

	int result =  
		(CollisionRadius >= Spec.CollisionRadius) &&
		(CollisionHeight >= Spec.CollisionHeight) &&
		((reachFlags | Spec.reachFlags) == Spec.reachFlags) &&
		(MaxLandingVelocity <= Spec.MaxLandingVelocity);

	return result; 
}

UBOOL UReachSpec::ShouldPruneAgainst( UReachSpec* Spec )
{
	if( !bPruned && !bSkipPrune && *End != NULL )
	{
		if( PruneSpecList.FindItemIndex(Spec->GetClass())  >= 0 ||
			Spec->PruneSpecList.FindItemIndex(GetClass())  >= 0 )
		{
			return TRUE;
		}

		return (*this <= *Spec);
	}

	return FALSE;
}

/* defineFor()
initialize the reachspec for a  traversal from start actor to end actor.
Note - this must be a direct traversal (no routing).
Returns 1 if the definition was successful (there is such a reachspec), and zero
if no definition was possible
*/

INT UReachSpec::defineFor( ANavigationPoint *begin, ANavigationPoint *dest, APawn *ScoutPawn )
{
	Start = begin;
	End = dest;
	AScout *Scout = Cast<AScout>(ScoutPawn);
	Scout->InitForPathing( begin, dest );
	Start->PrePath();
	End->PrePath();
	// debugf(TEXT("Define path from %s to %s"),*begin->GetName(), *dest->GetName());
	INT result = findBestReachable((AScout *)Scout);
	Start->PostPath();
	End->PostPath();
	Scout->SetPathColor(this);
	return result;
}

/* findBestReachable()

  This is the function that determines what radii and heights of pawns are checked for setting up reachspecs

  Modify this function if you want a different set of parameters checked

  Note that reachable regions are also determined by this routine, so allow a wide max radius to be tested

  What is checked currently (in order):

	Crouched human (HUMANRADIUS, CROUCHEDHUMANHEIGHT)
	Standing human (HUMANRADIUS, HUMANHEIGHT)
	Small non-human
	Common non-human (COMMONRADIUS, COMMONHEIGHT)
	Large non-human (MAXCOMMONRADIUS, MAXCOMMONHEIGHT)

	Then, with the largest height that was successful, determine the reachable region

  TODO: it might be a good idea to look at what pawns are referenced by the level, and check for all their radii/height combos.
  This won't work for a UT type game where pawns are dynamically loaded

*/

INT UReachSpec::findBestReachable(AScout *Scout)
{
	// start with the smallest collision size
	FLOAT bestRadius = Scout->PathSizes(0).Radius;
	FLOAT bestHeight = Scout->PathSizes(0).Height;
	Scout->SetCollisionSize( bestRadius, bestHeight );
	INT success = 0;
	// attempt to place the scout at the start node
	if( Start->PlaceScout( Scout ) )
	{
		// Set floor of path for the scount
		// (wall path nodes will have floor facing out of wall)
		FVector Up(0,0,1);
		Start->GetUpDir( Up );
		Scout->Floor = Up;

		FCheckResult Hit(1.f);
		// initialize scout for movement
		Scout->MaxLandingVelocity = 0.f;
		FVector ViewPoint = Start->Location;
		ViewPoint.Z += Start->CylinderComponent->CollisionHeight * Up.Z;

		// check visibility to end node, first from eye level,
		// and then directly if that failed
		if( GWorld->SingleLineCheck(Hit, Scout, End->Location, ViewPoint, TRACE_World | TRACE_StopAtAnyHit) ||
			GWorld->SingleLineCheck(Hit, Scout, End->Location, Scout->Location, TRACE_World | TRACE_StopAtAnyHit) )
		{
			// if we successfully walked with smallest collision,
			success = Scout->actorReachable( End, TRUE, TRUE );
			if( success )
			{
				// save the movement flags and landing velocity
				MaxLandingVelocity = appTrunc(Scout->MaxLandingVelocity);
				reachFlags = success;

				// and find the largest supported size
				INT svdSuccess = success;
				for( INT idx = 1; idx < Scout->PathSizes.Num(); idx++ )
				{
					 Scout->SetCollisionSize( Scout->PathSizes(idx).Radius, Scout->PathSizes(idx).Height );
					 if( Start->PlaceScout( Scout ) )
					 {
						 success = Scout->actorReachable( End, TRUE, TRUE );
						 if( success )
						 {
							 // save the size if still reachable
							 // Save the LARGEST size
							 bestRadius = ::Max( bestRadius, Scout->PathSizes(idx).Radius );
							 bestHeight = ::Max( bestHeight, Scout->PathSizes(idx).Height );
							 svdSuccess = success;
						 }
						 else
						 {
							 // Otherwise, couldn't reach - exit
							 break;
						 }

						 //TODO: handle testing/flagging crouch specs
					 }
					 else
					 {
						 // Otherwise, couldn't fit - exit
						 break;
					 }
				}

				success = svdSuccess;
			}
		}
	}

	if( success )
	{
		// init reach spec based on results
		CollisionRadius = appTrunc(bestRadius);
		CollisionHeight = appTrunc(bestHeight);
		Distance = appTrunc((End->Location - Start->Location).Size()); 
	}

	return success;
}

	
FVector UReachSpec::GetForcedPathSize( ANavigationPoint* Nav, AScout* Scout )
{
	if( Nav->bVehicleDestination )
	{
		return Scout->GetSize(FName(TEXT("Vehicle"),FNAME_Find));
	}

	if( appStricmp( appGetGameName(), TEXT("War") ) == 0 )
	{
		return Scout->GetSize(ForcedPathSizeName);
	}
    
	return Scout->GetSize(FName(TEXT("Common"),FNAME_Find));
}

/** If bAddToNavigationOctree is true, adds the ReachSpec to the navigation octree */
void UReachSpec::AddToNavigationOctree()
{
	if ( bAddToNavigationOctree && Start != NULL && *End != NULL && Start->CylinderComponent != NULL && End->CylinderComponent != NULL && Start->Location != End->Location
		&& (NavOctreeObject == NULL || NavOctreeObject->OctreeNode == NULL) )
	{
		// if the collision of the start and end points encompasses the path, don't add an object for it
		if ((End->Location - Start->Location).Size() <= Start->CylinderComponent->CollisionRadius + End->CylinderComponent->CollisionRadius)
		{
			bAddToNavigationOctree = FALSE;
			return;
		}
		// check for an opposite spec that is already in the tree
		UReachSpec* Opposite = End->GetReachSpecTo(Start);
		if (Opposite != NULL && Opposite->NavOctreeObject != NULL && Opposite->NavOctreeObject->OctreeNode != NULL)
		{
			return;
		}
		// create the octree object
		if (NavOctreeObject == NULL)
		{
			NavOctreeObject = new FNavigationOctreeObject;
			NavOctreeObject->SetOwner(this);
		}
		// find a vector perpendicular to the path direction and the Z axis and extend it CollisionRadius units
		FVector Side = (End->Location - Start->Location).UnsafeNormal() ^ FVector(0.0f, 0.0f, 1.0f) * CollisionRadius;
		// construct the bounding box representing this path and assign it to the octree object
		FBox BoundingBox(0);
		FVector Height = FVector(0.f, 0.f, CollisionHeight);
		FVector StartHeight = FVector(0.f, 0.f, Start->CylinderComponent->CollisionHeight);
		FVector EndHeight = FVector(0.f, 0.f, End->CylinderComponent->CollisionHeight);
		BoundingBox += Start->Location + Side + (Height * 2.0f) - StartHeight;
		BoundingBox += Start->Location + Side - StartHeight;
		BoundingBox += Start->Location - Side + (Height * 2.0f) - StartHeight;
		BoundingBox += Start->Location - Side - StartHeight;
		BoundingBox += End->Location + Side + (Height * 2.0f) - EndHeight;
		BoundingBox += End->Location + Side - EndHeight;
		BoundingBox += End->Location - Side + (Height * 2.0f) - EndHeight;
		BoundingBox += End->Location - Side - EndHeight;
		NavOctreeObject->SetBox(BoundingBox);

		if (NavOctreeObject->OctreeNode == NULL)
		{
			// add the object to the octree
			GWorld->NavigationOctree->AddObject(NavOctreeObject);
		}
	}
}

/** returns whether TestBox overlaps the path this ReachSpec represents
 * @note this function assumes that TestBox has already passed a bounding box overlap check
 * @param TestBox the box to check
 * @return true if the box doesn't overlap this path, false if it does
 */
UBOOL UReachSpec::NavigationOverlapCheck(const FBox& TestBox)
{
	// make sure the end spec is valid, as in the streaming level case the destination may not be streamed in yet
	if (*End == NULL)
	{
		return TRUE;
	}
	FVector TestCenter, TestExtent;
	TestBox.GetCenterAndExtents(TestCenter, TestExtent);
	if (Square(TestExtent.X - TestExtent.Y) < KINDA_SMALL_NUMBER)
	{
		// approximate the box as a cylinder (which it probably started as anyway) and do fast point to line from box center to path
		const FVector PathDir = (End->Location - Start->Location).SafeNormal();
		const FVector ClosestPoint = (Start->Location + (PathDir | (TestCenter - Start->Location)) * PathDir);
		// test Z axis difference
		FLOAT DownDist = Lerp<FLOAT>(Start->CylinderComponent->CollisionHeight, End->CylinderComponent->CollisionHeight, (ClosestPoint - Start->Location).SizeSquared() / Square(FLOAT(Distance)));
		if (TestCenter.Z + TestExtent.Z < ClosestPoint.Z - DownDist || TestCenter.Z - TestExtent.Z > ClosestPoint.Z + FLOAT(CollisionHeight * 2) - DownDist)
		{
			return TRUE;
		}
		// test XY axis difference
		const FLOAT TestLength = TestExtent.X + CollisionRadius;
		return ((TestCenter - ClosestPoint).SizeSquared2D() > TestLength * TestLength);
	}
	else
	{
		// do slower box intersection check
		FVector HitLocation, HitNormal;
		FLOAT HitTime;
		return !FLineExtentBoxIntersection( TestBox, Start->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - Start->CylinderComponent->CollisionHeight),
											End->Location + FVector(0.f, 0.f, FLOAT(CollisionHeight) - End->CylinderComponent->CollisionHeight), FVector(CollisionRadius, CollisionRadius, CollisionHeight),
											HitLocation, HitNormal, HitTime );
	}
}

/** returns whether Point is within MaxDist units of the path this ReachSpec represents
 * @param Point the point to check
 * @param MaxDist the maximum distance the point can be from the path
 * @return true if the point is within MaxDist units, false otherwise
 */
UBOOL UReachSpec::IsOnPath(const FVector& Point, FLOAT MaxDist)
{
	// if Point is between the endpoints and the point to line distance from Point to the path line is less than the our CollisionRadius + MaxDist, then it is on the path
	const FVector PathDir = (End->Location - Start->Location).SafeNormal();
	if (((Start->Location - Point).SafeNormal() | PathDir) < 0.f && ((End->Location - Point).SafeNormal() | PathDir) > 0.f)
	{
		const FVector LineDir = Point - (Start->Location + (PathDir | (Point - Start->Location)) * PathDir);
		const FLOAT TestLength = MaxDist + CollisionRadius;
		return (LineDir.SizeSquared() <= TestLength * TestLength);
	}
	else
	{
		return false;
	}
}

void UReachSpec::FinishDestroy()
{
	if (NavOctreeObject != NULL)
	{
		delete NavOctreeObject;
		NavOctreeObject = NULL;
	}

	Super::FinishDestroy();
}

void UReachSpec::RemoveFromNavigationOctree()
{
	if (NavOctreeObject != NULL)
	{
		GWorld->NavigationOctree->RemoveObject(NavOctreeObject);
		bAddToNavigationOctree = FALSE;
	}
}

INT USlotToSlotReachSpec::defineFor( ANavigationPoint* begin, ANavigationPoint* dest, APawn* Scout )
{
	INT Result = UReachSpec::defineFor( begin, dest, Scout );
	
	ACoverSlotMarker* BeginMarker = Cast<ACoverSlotMarker>(begin);
	ACoverSlotMarker* DestMarker  = Cast<ACoverSlotMarker>(dest);

	INT BeginIdx	= BeginMarker->OwningSlot.SlotIdx;
	INT DestIdx		= DestMarker->OwningSlot.SlotIdx;
	INT LastIdx		= BeginMarker->OwningSlot.Link->Slots.Num() - 1;

	// If spec is looping from first to last slot
	if( BeginIdx == 0 && DestIdx == LastIdx )
	{
		SpecDirection = CD_Left;
	}
	else
	// If spec is looping from last to first slot
	if( BeginIdx == LastIdx && DestIdx == 0 )
	{
		SpecDirection = CD_Right;
	}
	else
	if( DestIdx < BeginIdx )
	{
		SpecDirection = CD_Left;
	}
	else
	{
		SpecDirection = CD_Right;
	}

	return Result;
}
