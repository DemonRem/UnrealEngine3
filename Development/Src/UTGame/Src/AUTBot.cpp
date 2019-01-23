//=============================================================================
//	AUTBot.cpp: UT AI implementation
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//
//=============================================================================

#include "UTGame.h"
#include "UTGameAIClasses.h"
#include "UTGameVehicleClasses.h"
#include "UnPath.h"

IMPLEMENT_CLASS(AUTBot);
IMPLEMENT_CLASS(AUTAvoidMarker);
IMPLEMENT_CLASS(UUTTranslocatorReachSpec);
IMPLEMENT_CLASS(AUTSquadAI);
IMPLEMENT_CLASS(UUTBotDecisionComponent);

enum EBotAIFunctions
{
	BotAI_PollWaitToSeeEnemy = 511,
	BotAI_PollLatentWhatToDoNext = 513,
};

void AUTBot::UpdatePawnRotation()
{
	if ( Focus )
	{
		FocalPoint = Focus->GetTargetLocation();
		TrackedVelocity = Focus->Velocity;
		if ( Focus == CurrentlyTrackedEnemy )
		{
			if ( !bEnemyIsVisible ) 
			{
				if ( bEnemyInfoValid )
					FocalPoint = LastSeenPos;
			}
			else
			{
				// focus on point where we think enemy is going, if we've built up a model
				if ( (SavedPositions.Num() > 0) && (SavedPositions(0).Time <= WorldInfo->TimeSeconds - TrackingReactionTime) )
				{
					// determine his position and velocity at the appropriate point in the past
					for ( INT i=0; i<SavedPositions.Num(); i++ )
					{
						if ( SavedPositions(i).Time > WorldInfo->TimeSeconds - TrackingReactionTime )
						{
							FocalPoint = SavedPositions(i-1).Position + (SavedPositions(i).Position - SavedPositions(i-1).Position)*(WorldInfo->TimeSeconds - TrackingReactionTime - SavedPositions(i-1).Time)/(SavedPositions(i).Time - SavedPositions(i-1).Time);
							TrackedVelocity = SavedPositions(i-1).Velocity + (SavedPositions(i).Velocity - SavedPositions(i-1).Velocity)*(WorldInfo->TimeSeconds - TrackingReactionTime - SavedPositions(i-1).Time)/(SavedPositions(i).Time - SavedPositions(i-1).Time);
							FocalPoint = FocalPoint + TrackedVelocity*TrackingReactionTime;
							break;
						}
					}
				}
			}

			// draw where bot thinks enemy is
			// @todo steve - remove (temp debug code)
			if ( CurrentlyTrackedEnemy->Controller && CurrentlyTrackedEnemy->Controller->GetAPlayerController() )
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(CurrentlyTrackedEnemy->Controller);
				if ( PC && PC->bBehindView && PC->myHUD && PC->myHUD->bShowDebugInfo )
					DrawDebugSphere(FocalPoint,24.f, 8, 255, 255, 0, false);
			}
		}
		else
		{
			ANavigationPoint *NavFocus = Cast<ANavigationPoint>(Focus);
			if ( NavFocus )
			{
				if ( CurrentPath && (MoveTarget == NavFocus) && !Pawn->Velocity.IsZero() )
				{
					// gliding pawns must focus on where they are going
					if ( Pawn->IsGlider() )
					{
						FocalPoint = bAdjusting ? AdjustLoc : Focus->Location;
					}
					else
					{
						FocalPoint = Focus->Location - CurrentPath->Start->Location + Pawn->Location;
					}
				}
			}
		}
	}

	// rotate pawn toward focus
	Pawn->rotateToward(FocalPoint);

	// face same direction as pawn
	Rotation = Pawn->Rotation;

	// draw line showing which direction bot is currently looking
	//DrawDebugLine(Pawn->Location, Pawn->Location + Rotation.Vector()*(Pawn->Location - FocalPoint).Size(), 255, 255,0, FALSE);
}

/*
LineOfSightTo()
returns true if controller's pawn can see Other actor.
Checks line to center of other actor, and possibly to head or box edges depending on distance
*/
DWORD AUTBot::LineOfSightTo(AActor *Other, INT bUseLOSFlag, FVector *chkLocation)
{
	if (Other == NULL)
	{
		return 0;
	}
	else if (Other != Enemy)
	{
		return Super::LineOfSightTo(Other, bUseLOSFlag, chkLocation);
	}
	else
	{
	// cache enemy visibility for one frame
		if (EnemyVisibilityTime == WorldInfo->TimeSeconds && VisibleEnemy == Enemy)
		{
			return bEnemyIsVisible;
		}
	}
	VisibleEnemy = Enemy;
	EnemyVisibilityTime = WorldInfo->TimeSeconds;
	bEnemyIsVisible = Super::LineOfSightTo(Enemy);
	return bEnemyIsVisible;
}

void AUTBot::UpdateEnemyInfo(APawn* AcquiredEnemy)
{
		LastSeenTime = GWorld->GetTimeSeconds();
		LastSeeingPos = GetViewTarget()->Location;
		LastSeenPos = Enemy->Location;
		bEnemyInfoValid = true;
}

AActor* AUTBot::FindBestSuperPickup(FLOAT MaxDist)
{
	if ( !Pawn )
	{
		return NULL; 
	}
	AActor * bestPath = NULL;
	PendingMover = NULL;
	bPreparingMove = false;

	// Set super pickups as endpoints
	APickupFactory *GOAL = NULL;
	for ( ANavigationPoint *N=GWorld->GetFirstNavigationPoint(); N!=NULL; N=N->nextNavigationPoint )
	{
		APickupFactory *IS = N->GetAPickupFactory();
		if ( IS && IS->bIsSuperItem
			&& IS->IsProbing(NAME_Touch) 
			&& !IS->BlockedByVehicle()
			&& (eventSuperDesireability(IS) > 0.f) )
		{
			IS->bTransientEndPoint = true;
			GOAL = IS;
		}
	}
	bestPath = FindPath(FVector(0.f,0.f,0.f), GOAL, TRUE, UCONST_BLOCKEDPATHCOST, FALSE );
	if ( RouteDist > MaxDist )
		bestPath = NULL;

	return bestPath; 
}

void AUTBot::WaitToSeeEnemy()
{
	if ( !Pawn || !Enemy )
		return;

	Focus = Enemy;
	GetStateFrame()->LatentAction = BotAI_PollWaitToSeeEnemy;
}

void AUTBot::execPollWaitToSeeEnemy( FFrame& Stack, RESULT_DECL )
{
	if( !Pawn || !Enemy )
	{
		GetStateFrame()->LatentAction = 0; 
		return;
	}
	if ( GWorld->GetTimeSeconds() - LastSeenTime > 0.1f )
		return;

	//check if facing enemy 
	if ( Pawn->ReachedDesiredRotation() )
		GetStateFrame()->LatentAction = 0; 
}
IMPLEMENT_FUNCTION( AUTBot, BotAI_PollWaitToSeeEnemy, execPollWaitToSeeEnemy);

void AUTBot::LatentWhatToDoNext()
{
	//@warning: need to set LatentAction before event in case it changes states
	GetStateFrame()->LatentAction = BotAI_PollLatentWhatToDoNext;
	eventWhatToDoNext();
}

void AUTBot::execPollLatentWhatToDoNext(FFrame& Stack, RESULT_DECL)
{
	if (Pawn == NULL || DecisionComponent == NULL || !DecisionComponent->bTriggered)
	{
		GetStateFrame()->LatentAction = 0;
	}
}
IMPLEMENT_FUNCTION(AUTBot, BotAI_PollLatentWhatToDoNext, execPollLatentWhatToDoNext);

/* CanMakePathTo()
// assumes valid CurrentPath, tries to see if CurrentPath can be combined with a path to N
*/
UBOOL AUTBot::CanMakePathTo(class AActor* A)
{
	ANavigationPoint *N = Cast<ANavigationPoint>(A);
	INT Success = 0;

	if ( N && Pawn->ValidAnchor() && CurrentPath 
		&& ((CurrentPath->reachFlags & R_WALK) == CurrentPath->reachFlags) )
	{
		UReachSpec *NextPath = 	CurrentPath->End->GetReachSpecTo(N);
		if ( NextPath &&  ((NextPath->reachFlags & R_WALK) == NextPath->reachFlags) 
			&& NextPath->supports(appTrunc(Pawn->CylinderComponent->CollisionRadius),appTrunc(Pawn->CylinderComponent->CollisionHeight),Pawn->calcMoveFlags(),appTrunc(Pawn->GetAIMaxFallSpeed()))
			&& !NextPath->IsA(UAdvancedReachSpec::StaticClass())
			&& (NextPath->CostFor(Pawn) < UCONST_BLOCKEDPATHCOST) )
		{
			FCheckResult Hit(1.f);
			GWorld->SingleLineCheck( Hit, this, N->Location, Pawn->Location + FVector(0,0,Pawn->EyeHeight), TRACE_World|TRACE_StopAtAnyHit );
			if ( !Hit.Actor )
			{
				// check in relatively straight line ( within path radii)
				FLOAT MaxDist = ::Min<FLOAT>(CurrentPath->CollisionRadius,NextPath->CollisionRadius);
				FVector Dir = (N->Location - Pawn->Location).SafeNormal();
				FVector LineDist = CurrentPath->End->Location - (Pawn->Location + (Dir | (CurrentPath->End->Location - Pawn->Location)) * Dir);
				//debugf(TEXT("Path dist is %f max %f"),LineDist.Size(),MaxDist);
				Success = ( LineDist.SizeSquared() < MaxDist * MaxDist );
			}
		}
	}

	return Success;
}

FLOAT FindBestInventory( ANavigationPoint* CurrentNode, APawn* seeker, FLOAT bestWeight )
{
	FLOAT CacheWeight = 0.f;
	if ( CurrentNode->InventoryCache && (CurrentNode->visitedWeight < CurrentNode->InventoryCache->LifeSpan * seeker->GroundSpeed) )
	{
		FLOAT BaseWeight = 0.f;
		FLOAT CacheDist = ::Max(1.f,CurrentNode->InventoryDist + CurrentNode->visitedWeight);
		if ( CurrentNode->InventoryCache->bDeleteMe || !CurrentNode->InventoryCache->Inventory || CurrentNode->InventoryCache->Inventory->bDeleteMe )
			CurrentNode->InventoryCache = NULL;
		else if ( CurrentNode->InventoryCache->Inventory->MaxDesireability/CacheDist > bestWeight )
			BaseWeight = seeker->Controller->eventRatePickup(CurrentNode->InventoryCache, CurrentNode->InventoryCache->Inventory->GetClass());
		CacheWeight = BaseWeight/CacheDist;
		if ( (CacheWeight > bestWeight) && !CurrentNode->InventoryCache->BlockedByVehicle() )
		{
			if ( BaseWeight >= 1.f )
				return 2.f;
			bestWeight = CacheWeight;
		}
	}

	APickupFactory* item = CurrentNode->GetAPickupFactory();
	if (item == NULL)
	{
		return CacheWeight;
	}

	while (item->ReplacementFactory != NULL && !item->IsProbing(NAME_Touch))
	{
		item = item->ReplacementFactory;
	}

	FLOAT AdjustedWeight = ::Max(1,CurrentNode->visitedWeight);
	if ( item && !item->bDeleteMe && (item->IsProbing(NAME_Touch) || (item->bPredictRespawns && (item->LatentFloat < seeker->Controller->RespawnPredictionTime))) 
			&& (item->MaxDesireability/AdjustedWeight > bestWeight) )
	{
		FLOAT BaseWeight = seeker->Controller->eventRatePickup(item, item->InventoryType);
		if ( !item->IsProbing(NAME_Touch) )
			AdjustedWeight += seeker->GroundSpeed * item->LatentFloat;
		if ( (CacheWeight * AdjustedWeight > BaseWeight) || (bestWeight * AdjustedWeight > BaseWeight) || item->BlockedByVehicle() )
			return CacheWeight;

		if ( (BaseWeight >= 1.f) && (BaseWeight > AdjustedWeight * bestWeight) )
			return 2.f;

		return BaseWeight/AdjustedWeight;
	}
	return CacheWeight;
}

AActor* AUTBot::FindBestInventoryPath(FLOAT& Weight)
{
	if ( !Pawn )
	{
		return NULL;
	}
	AActor * bestPath = NULL;
	bPreparingMove = false;

	// first, look for nearby dropped inventory
	if ( Pawn->ValidAnchor() )
	{
		if (Pawn->Anchor->InventoryCache != NULL && Pawn->Anchor->InventoryCache->InventoryClass != NULL)
		{
			if ( Pawn->Anchor->InventoryCache->bDeleteMe )
			{
				Pawn->Anchor->InventoryCache = NULL;
			}
			else if (eventRatePickup(Pawn->Anchor->InventoryCache, Pawn->Anchor->InventoryCache->InventoryClass) > 0.f)
			{
				if (Pawn->actorReachable(Pawn->Anchor->InventoryCache))
				{
					return Pawn->Anchor->InventoryCache;
				}
				else
				{
					Pawn->Anchor->InventoryCache = NULL;
				}
			}
		}
	}

	Weight = Pawn->findPathToward(NULL,FVector(0,0,0),&FindBestInventory, Weight,false);
	if ( Weight > 0.f )
		bestPath = SetPath();

	return bestPath;
}

/** JumpOverWall()
Make pawn jump over an obstruction
*/
void AUTBot::JumpOverWall(FVector WallNormal)
{
	Super::JumpOverWall(WallNormal);
	bJumpOverWall = true;
	bNotifyApex = true;

	AUTPawn* MyUTPawn = Cast<AUTPawn>(Pawn);
	if ( MyUTPawn )
	{
		MyUTPawn->bNoJumpAdjust = true; // don't let script adjust this jump again
		MyUTPawn->bReadyToDoubleJump = true;
	}
}

void AUTBot::NotifyJumpApex()
{
	eventNotifyJumpApex();
	bJumpOverWall = false;
}

void AUTBot::PreAirSteering(float DeltaTime)
{ 
	if ( !Pawn || ImpactVelocity.IsZero() )
		return;

	if ( !bPlannedJump || (Skill < 2.f) )
	{
		ImpactVelocity = FVector(0.f,0.f,0.f);
		return;
	}

	// no steering here if already doing low grav steering
	if ( (Pawn->Velocity.Z < 0.f) && (Pawn->GetGravityZ() > 0.9f * GWorld->GetDefaultGravityZ()) )
		return;

	Pawn->Acceleration = -1.f * ImpactVelocity * Pawn->AccelRate;
	Pawn->Acceleration.Z = 0.f;
}

void AUTBot::PostAirSteering(float DeltaTime)
{
	if ( ImpactVelocity.IsZero() )
		return;

	FVector OldVel = ImpactVelocity;
	ImpactVelocity = Pawn->NewFallVelocity(OldVel, Pawn->Acceleration, DeltaTime);
	if ( (ImpactVelocity | OldVel) < 0.f )
		ImpactVelocity = FVector(0.f,0.f,0.f);
}

void AUTBot::PostPhysFalling(float DeltaTime)
{
	if ( bInDodgeMove && (Pawn->Velocity.Z < 0.f) && (Pawn->Location.Z < DodgeLandZ + 10.f) )
	{
		bInDodgeMove = false;
		FVector Start = Pawn->Location - FVector(0.f,0.f,Pawn->CylinderComponent->CollisionHeight);
		FVector Dir = Pawn->Velocity.SafeNormal();
		if ( Dir.Z != 0.f )
		{
			Dir = Dir * (2.f * Pawn->MaxStepHeight + 20.f)/Abs(Dir.Z);
			FCheckResult Hit(1.f);
			GWorld->SingleLineCheck( Hit, Pawn, Start+Dir, Start, TRACE_World|TRACE_StopAtAnyHit );
			if ( Hit.Time == 1.f )
			{
				GWorld->SingleLineCheck( Hit, Pawn, Pawn->Location+Dir, Location, TRACE_World|TRACE_StopAtAnyHit, GetCylinderExtent() );
			}

			if ( Hit.Time == 1.f )
			{
				eventMissedDodge();
			}
		}
	}
}

/* UTBot Tick
*/
UBOOL AUTBot::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	// leave vehicle now, if requested
	if (bNeedDelayedLeaveVehicle)
	{
		bNeedDelayedLeaveVehicle = FALSE;
		eventDelayedLeaveVehicle();
	}

	UBOOL Ticked = Super::Tick(DeltaSeconds, TickType);

	if ( Ticked )
	{
		if( TickType==LEVELTICK_All )
		{
			if ( WarningProjectile && !WarningProjectile->bDeleteMe && (GWorld->GetTimeSeconds() > WarningDelay) )
			{
				eventDelayedWarning();
				WarningProjectile = NULL;
			}
			if ( MonitoredPawn )
			{
				if ( !Pawn || MonitoredPawn->bDeleteMe || !MonitoredPawn->Controller )
					eventMonitoredPawnAlert();
				else if ( !Pawn->SharingVehicleWith(MonitoredPawn) )
				{
					// quit if further than max dist, moving away fast, or has moved far enough
					if ( ((MonitoredPawn->Location - Pawn->Location).SizeSquared() > MonitorMaxDistSq)
						|| ((MonitoredPawn->Location - MonitorStartLoc).SizeSquared() > 0.25f * MonitorMaxDistSq) )
						eventMonitoredPawnAlert();
					else if ( (MonitoredPawn->Velocity.SizeSquared() > 0.6f * MonitoredPawn->GroundSpeed)
						&& ((MonitoredPawn->Velocity | (MonitorStartLoc - Pawn->Location)) > 0.f)
						&& ((MonitoredPawn->Location - Pawn->Location).SizeSquared() > 0.25f * MonitorMaxDistSq) )
						eventMonitoredPawnAlert();
				}
			}

			if ( CurrentlyTrackedEnemy != Enemy )
			{
				// clear current tracking
				SavedPositions.Empty();
				CurrentlyTrackedEnemy = Enemy;
				if ( CurrentlyTrackedEnemy )
				{
					AUTPawn *UTP = Cast<AUTPawn>(CurrentlyTrackedEnemy);
					if ( UTP )
					{
						UTP->RequestTrackingFor(this);
					}
					else
					{
						AUTVehicle *UTV = Cast<AUTVehicle>(CurrentlyTrackedEnemy);
						if ( UTV )
						{
							UTV->RequestTrackingFor(this);
						}
					}
				}
			}
			if ( CurrentlyTrackedEnemy )
			{
				// clear out obsolete position data
				// keep one position earlier than TrackingReactionTime to allow interpolation
				for ( INT i=0; i<SavedPositions.Num(); i++ )
				{
					if ( SavedPositions(i).Time > WorldInfo->TimeSeconds - TrackingReactionTime )
					{
						if ( i > 1 )
							SavedPositions.Remove(0, i-1);
						break;
					}
				}
			}
		}		
	}

	return Ticked;
}

void AUTPawn::RequestTrackingFor(AUTBot *Bot)
{
	Trackers.AddItem(Bot);
}

void AUTVehicle::RequestTrackingFor(AUTBot *Bot)
{
	Trackers.AddItem(Bot);
}

/* PostPollMove()
Called after a latent move update is polled, unless the movement completed.
Adjust pawn movement to avoid active FearSpots

*/
void AUTBot::PostPollMove()
{
	if ( Pawn->Acceleration.IsZero() )
		return;

	FVector FearAdjust(0.f,0.f,0.f);
	for ( INT i=0; i<2; i++ )
	{
		if ( FearSpots[i] )
		{
			if (FearSpots[i]->bDeleteMe || FearSpots[i]->CollisionCylinder == NULL || !Pawn->IsOverlapping(FearSpots[i]))
			{
				FearSpots[i] = NULL;
			}
			else 
			{
				FearAdjust += (Pawn->Location - FearSpots[i]->Location) / FearSpots[i]->CollisionCylinder->CollisionRadius;
			}
		}
	}

	if ( FearAdjust.IsZero() )
		return;

	FearAdjust.Normalize();
	FLOAT PawnAccelRate = Pawn->Acceleration.Size();
	FVector PawnDir = Pawn->Acceleration/PawnAccelRate;

	if ( (FearAdjust | PawnDir) > 0.7f )
		return;

	if ( (FearAdjust | PawnDir) < -0.7f )
	{
		FVector LeftDir = PawnDir ^ FVector(0.f,0.f,1.f);	
		LeftDir = LeftDir.SafeNormal();
		FearAdjust = 2.f * LeftDir;
		if ( (LeftDir | FearAdjust) < 0.f )
			FearAdjust *= -1.f;
	}

	Pawn->Acceleration = (PawnDir + FearAdjust).SafeNormal();
	Pawn->Acceleration *= PawnAccelRate;
}

UBOOL AUTBot::AirControlFromWall(float DeltaTime, FVector& RealAcceleration)
{
	if (!bNotifyFallingHitWall )
	{
		// try aircontrol push
		Pawn->Acceleration = Pawn->Velocity;
		Pawn->Acceleration.Z = 0.f;
		Pawn->Acceleration = Pawn->Acceleration.SafeNormal();
		Pawn->Acceleration *= Pawn->AccelRate;
		RealAcceleration = Pawn->Acceleration;
		return true;
	}
	return false;
}

UBOOL AUTScout::SetHighJumpFlag()
{
	if ( bRequiresDoubleJump )
	{
		bRequiresDoubleJump = false;
		return true;
	}
	return false;
}

UBOOL AUTPawn::SetHighJumpFlag()
{
	if ( bRequiresDoubleJump )
	{
		bRequiresDoubleJump = false;
		return true;
	}
	return false;
}

UReachSpec* AUTBot::PrepareForMove(ANavigationPoint *NavGoal, UReachSpec* CurrentPath)
{
	if ( CurrentPath->PrepareForMove(this) )
	{
		return CurrentPath;
	}
	else if ( (Pawn->Physics == PHYS_Walking)
			&& (Pawn->Location.Z + Pawn->MaxStepHeight >= CurrentPath->End->Location.Z)
			&& !CurrentPath->IsA(UAdvancedReachSpec::StaticClass())
			&& ((CurrentPath->reachFlags & R_WALK) == CurrentPath->reachFlags) 
			&& (appFrand() < DodgeToGoalPct) ) 
	{
		eventMayDodgeToMoveTarget();
		if ( NavGoal != MoveTarget )
		{
			CurrentPath = NULL;
			NextRoutePath = NULL;
			NavGoal = Cast<ANavigationPoint>(MoveTarget);
		}
	}
	return CurrentPath;
}


AActor* AUTBot::FindPathToSquadRoute(UBOOL bWeightDetours/* =0 */)
{
	if ( !Squad || !Squad->RouteObjective )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route for FindPathToSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}

	const TArray<ANavigationPoint*>& AlternateRoute = bUsePreviousSquadRoute ? Squad->PreviousObjectiveRouteCache : Squad->ObjectiveRouteCache;

	// if no route cache or bot doesn't want to use the squad route, just go straight to objective
	INT NumNodes = AlternateRoute.Num();
	if (NumNodes == 0 || !bUsingSquadRoute)
	{
		return FindPath(FVector(0,0,0), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
	}

	Squad->RouteObjective->bTransientEndPoint = true;
	INT AnchorIndex = Pawn->ValidAnchor() ? AlternateRoute.FindItemIndex(Pawn->Anchor) : INDEX_NONE;

	if (SquadRouteGoal != NULL)
	{
		INT SquadRouteGoalIndex = AlternateRoute.FindItemIndex(SquadRouteGoal);
		// if we're already pathfinding toward squad route, but haven't reached or passed it yet
		if (SquadRouteGoal == RouteGoal && SquadRouteGoalIndex != INDEX_NONE && SquadRouteGoalIndex > AnchorIndex)
		{
			// continue towards it
			AActor* Result = FindPath(FVector(0,0,0), SquadRouteGoal, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
			// if we found a path towards it, we're done
			if (Result != NULL)
			{
				//	fill out bot's route cache with squad route cache
				if (SquadRouteGoal != Squad->RouteObjective)
				{
					for (INT i = SquadRouteGoalIndex + 1; i < NumNodes; i++)
					{
						RouteCache.AddItem(AlternateRoute(i));
					}
				}

				return Result;
			}
		}
	}

	// if already on route, follow it
	if (AnchorIndex != INDEX_NONE)
	{
		SquadRouteGoal = (AnchorIndex < NumNodes - 4) ? AlternateRoute(AnchorIndex + 3) : Squad->RouteObjective;
		AActor* Result = FindPath(FVector(0,0,0), SquadRouteGoal, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
		if (Result != NULL)
		{
			// if pathfinding found something other than the SquadRouteGoal, temporarily disable squad routes so that the bot doesn't get confused about where to go
			if (RouteGoal != SquadRouteGoal)
			{
				SquadRouteGoal = Squad->RouteObjective;
				bUsingSquadRoute = FALSE;
			}
			//	fill out bot's route cache with squad route cache
			else if (SquadRouteGoal != Squad->RouteObjective)
			{
				for (INT j = AnchorIndex + 4; j < NumNodes; j++)
				{
					RouteCache.AddItem(AlternateRoute(j));
				}
			}
			
			return Result;
		}
	}
	SquadRouteGoal = NULL;
	bUsePreviousSquadRoute = FALSE;

	for (INT i = 0; i < Squad->ObjectiveRouteCache.Num() && Squad->ObjectiveRouteCache(i) != NULL; i++)
	{
		Squad->ObjectiveRouteCache(i)->bTransientEndPoint = true;
	}
	return FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
}

void AUTBot::BuildSquadRoute(INT Iterations)
{
	// clear current squad route
	Squad->ObjectiveRouteCache.Empty();
	Squad->PendingSquadRouteMaker = this;

	if ( !Squad || !Squad->RouteObjective || !Pawn )
	{
		//debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route objective for BuildSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return;
	}

	// if no valid anchor, fail!
	if ( !Pawn->ValidAnchor() )
	{
		//debugfSuppressed(NAME_DevPath,TEXT("Warning: No valid anchor for BuildSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return;
	}

	if (Iterations <= 0)
	{
		debugf(NAME_Warning, TEXT("Invalid Iterations of %i passed to UTBot::BuildSquadRoute()"), Iterations);
		Squad->PendingSquadRouteMaker = NULL;
		return;
	}

	do
	{
		Iterations--;
		AActor* Result = FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, FALSE, UCONST_BLOCKEDPATHCOST, FALSE );

		if ( !Result )
		{
			debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route found in BuildSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
			Squad->PendingSquadRouteMaker = NULL;
			return;
		}

		// add transient cost to nodes on this route and adjacent nodes
		FCheckResult Hit(1.f);
		INT RouteLength = RouteCache.Num();
		for (INT i=0; i<RouteLength; i++ )
		{
			ANavigationPoint *Nav = RouteCache(i);
			if ( Nav )
			{
				FLOAT CostFactor = (i<=0.5*RouteLength) ? i : RouteLength-i;
				CostFactor *= 2000/RouteLength;
				Nav->PathCost += appTrunc(CostFactor);
				for ( INT j=0; j<Nav->PathList.Num(); j++ )
				{
					if (Nav->PathList(j)->End.Nav != NULL)
					{
						Nav->PathList(j)->End->PathCost += appTrunc(CostFactor);
					}
				}
			}
		}
	} while (Iterations > 0);

	// clear all PathCosts
	for ( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav; Nav=Nav->nextNavigationPoint )
	{
		Nav->PathCost = 0;
	}

	// fill Squad ObjectiveRouteCache with route
	for (INT i=0; i<RouteCache.Num(); i++ )
	{
		Squad->ObjectiveRouteCache.AddItem(RouteCache(i));
	}

	Squad->PendingSquadRouteMaker = NULL;
}

FLOAT AUTBot::SpecialJumpCost(FLOAT RequiredJumpZ)
{
	// check for script override
	if (bScriptSpecialJumpCost)
	{
		FLOAT Cost = 0.f;
		if (eventSpecialJumpCost(RequiredJumpZ, Cost))
		{
			return Cost;
		}
	}
	// see if normal jumping will get us there (we have jump boots, low grav, or other automatic effect that improves jumping)
	if (Pawn->JumpZ + MultiJumpZ > RequiredJumpZ)
	{
		// if lowgrav, no special cost
		return (Pawn->GetGravityZ() < WorldInfo->DefaultGravityZ) ? 0.f : 1000.f;
	}
	else
	{
		// impact hammer required, so increase cost so bot won't waste the time/health unless it's a significant shortcut
		return 3000.f;
	}
}

/* CostFor()
Adjusted "length" of this path.
Values >= BLOCKEDPATHCOST indicate this path is blocked to the pawn
*/
INT UUTTranslocatorReachSpec::CostFor(APawn *P)
{
	if ( P->Physics == PHYS_Flying )
	{
		return Distance;  
	}
	if ( !P->Controller || !P->Controller->bCanDoSpecial || Cast<AVehicle>(P) )
	{
		return UCONST_BLOCKEDPATHCOST;
	}

	AUTBot *B = Cast<AUTBot>(P->Controller);
	if ( !B  )
	{
		return UCONST_BLOCKEDPATHCOST;
	}
	// check if it is possible to jump across this path via special jumping abilities the bot has or because of gravity change
	if (RequiredJumpZ > 0.f)
	{
		// adjust for any gravity changes
		FLOAT GravityZ = Start->GetGravityZ();
		if (GravityZ != OriginalGravityZ)
		{
			RequiredJumpZ -= (0.6f * RequiredJumpZ * (1.0 - GravityZ / OriginalGravityZ));
			OriginalGravityZ = GravityZ;
		}
		if (B->MaxSpecialJumpZ > RequiredJumpZ)
		{
			return appTrunc(Distance + B->SpecialJumpCost(RequiredJumpZ));
		}
	}
	if ( B->bAllowedToTranslocate )
	{
		return 200 + Distance/2;
	}

	return UCONST_BLOCKEDPATHCOST;
}


UBOOL UUTTranslocatorReachSpec::PrepareForMove(AController *C)
{
	if ( !C || !C->Pawn )
	{
		return false;
	}
	AUTBot *B = Cast<AUTBot>(C);
	if ( !B  )
	{
		return false;
	}
	// check if it is possible to jump across this path via special jumping abilities the bot has or because of gravity change
	if (RequiredJumpZ > 0.f && B->MaxSpecialJumpZ > RequiredJumpZ)
	{
		B->eventSpecialJumpTo(End, RequiredJumpZ);
		return true;
	}
	if ( B->bAllowedToTranslocate )
	{
		B->Pawn->Acceleration = FVector(0.f,0.f,0.f);
		B->eventTranslocateTo(End);
		return true;
	}
	return false;
}

FRotator AUTBot::SetRotationRate(FLOAT deltaTime)
{
	if ( bForceDesiredRotation )
	{
		Pawn->DesiredRotation = DesiredRotation;
	}

	INT YawDiff = Abs((Rotation.Yaw & 65535) - Pawn->DesiredRotation.Yaw);
	INT deltaYaw = 0;
	if ( Enemy && (Focus == Enemy) )
	{
		deltaYaw = (bEnemyAcquired && !Enemy->IsInvisible()) ? ::Max(Pawn->RotationRate.Yaw, RotationRate.Yaw) : AcquisitionYawRate;
		if ( (YawDiff < 2048) || (YawDiff > 63287) )
			bEnemyAcquired = true;
	}
	else
	{
		// adjust turn speed based on how far NPC needs to turn (if further, turn faster)
		if ( YawDiff > 32768 )
			YawDiff = 65536 - YawDiff;
		deltaYaw = Clamp(2*YawDiff, Pawn->RotationRate.Yaw, 2*Pawn->RotationRate.Yaw);
	}
	deltaYaw = appRound(deltaYaw * deltaTime);
	return FRotator(deltaYaw, deltaYaw, deltaYaw);
}

/* ForceReached()
* Controller can override Pawn NavigationPoint reach test
* return true if want to force successful reach by pawn of Nav
*/
UBOOL AUTBot::ForceReached(ANavigationPoint *Nav, const FVector& TestPosition)
{
	// if my pawn is blocked by a vehicle that is near the navigationpoint, call destination reached
	if ( Pawn && LastBlockingVehicle && !LastBlockingVehicle->bDeleteMe && (LastBlockingVehicle != Pawn)
		&& (Abs(LastBlockingVehicle->Location.Z - Nav->Location.Z) < Nav->CylinderComponent->CollisionHeight + LastBlockingVehicle->CylinderComponent->CollisionHeight)
		&& ((LastBlockingVehicle->Location - Nav->Location).Size2D() < Nav->CylinderComponent->CollisionRadius + LastBlockingVehicle->CylinderComponent->CollisionRadius)
		&& LastBlockingVehicle->ReachedBy(Pawn, TestPosition, Nav->Location) )
	{
		return TRUE;
	}

	// reset LastBlockingVehicle if we know it's not blocking our movement
	if (Nav == MoveTarget)
	{
		LastBlockingVehicle = NULL;
	}
	return FALSE;
}

void AUTBot::MarkEndPoints(ANavigationPoint* EndAnchor, AActor* Goal, const FVector& GoalLocation)
{
	// if we're building or following a squad alternate route, we must only mark the end anchor, otherwise FindPathToSquadRoute() will get confused about the bot's goal
	if (Squad != NULL && (Goal == SquadRouteGoal || Squad->PendingSquadRouteMaker == this))
	{
		EndAnchor->bEndPoint = TRUE;
	}
	else
	{
		Super::MarkEndPoints(EndAnchor, Goal, GoalLocation);
	}
}

void UUTBotDecisionComponent::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	checkSlow(GWorld->TickGroup == TG_DuringAsyncWork);
	if (bTriggered)
	{
		AUTBot* Bot = Cast<AUTBot>(GetOwner());
		if (Bot != NULL)
		{
			Bot->bExecutingWhatToDoNext = TRUE;
			Bot->eventExecuteWhatToDoNext();
			Bot->bExecutingWhatToDoNext = FALSE;
			bTriggered = FALSE;
		}
	}
}

// //@todo steve
//To be merged or discarded from UT2004

/* if navigationpoint is spawned, paths are invalid
NOTE THAT NavigationPoints should not be destroyed during gameplay!!!

void ANavigationPoint::FinishDestroy()
{
	if( GIsEditor && !GWorld->HasBegunPlay() )
	{
		GWorld->GetWorldInfo()->bPathsRebuilt = 0;

		// clean up reachspecs referring to this NavigationPoint
		for ( INT i=0; i<PathList.Num(); i++ )
		{
			if ( PathList(i) )
			{
				PathList(i)->Start = NULL;
			}
		}

		for( FActorIterator It; It; ++ It )
		{
			ANavigationPoint *Nav = Cast<ANavigationPoint>(*It);
			if ( Nav )
			{
				for ( INT j=0; j<Nav->PathList.Num(); j++ )
				{			
					if ( Nav->PathList(j) && Nav->PathList(j)->End == this )
					{
						Nav->PathList(j)->bPruned = true;
						Nav->PathList(j)->End = NULL;
					}
				}
				Nav->CleanUpPruned();
			}
		}
	}
	Super::FinishDestroy();
}

void ANavigationPoint::execSetBaseDistance( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(BaseNum);
	P_FINISH;

	SetBaseDistance(BaseNum, 0.f);
}

void ANavigationPoint::SetBaseDistance(INT BaseNum, FLOAT CurrentDist)
{
	if ( BaseDist[BaseNum] <= CurrentDist )
		return;
    BaseDist[BaseNum] = CurrentDist;

	for ( INT i=0; i<PathList.Num(); i++ )
		if ( ((PathList(i)->reachFlags & R_PROSCRIBED) == 0) && (PathList(i)->Distance >= 0.f) )
			PathList(i)->End->SetBaseDistance(BaseNum, CurrentDist + PathList(i)->Distance); 

}
*/

