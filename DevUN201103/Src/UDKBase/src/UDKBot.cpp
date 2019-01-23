//=============================================================================
//	UDKBot.cpp: UT AI implementation
// Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
// Confidential.
//
//=============================================================================

#include "UDKBase.h"
#include "UnPath.h"

IMPLEMENT_CLASS(AUDKBot);
IMPLEMENT_CLASS(UUDKAIDecisionComponent);
IMPLEMENT_CLASS(AUDKSquadAI);

#if PS3
// used to skip bot updates if they've taken too long this tick
static FLOAT GLastBotDecisionTime = -1.0f;
static FLOAT GTotalBotTimeThisTick = 0.0f; // in milliseconds
#endif

void UUDKAIDecisionComponent::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	checkSlow(GWorld->TickGroup == TG_DuringAsyncWork);
	if (bTriggered)
	{
#if PS3
		// if bots are taking too long, hold off for a frame
		FLOAT CurrentTime = GWorld->GetWorldInfo()->TimeSeconds;
		if (GLastBotDecisionTime != CurrentTime)
		{
			GLastBotDecisionTime = CurrentTime;
			GTotalBotTimeThisTick = 0.0f;
		}
		else if (GTotalBotTimeThisTick > 4.0f)
		{
			return;
		}
		DWORD Cycles = 0 - appCycles();
#endif

		AUDKBot* Bot = Cast<AUDKBot>(GetOwner());
		if (Bot != NULL)
		{
			Bot->bExecutingWhatToDoNext = TRUE;
			Bot->eventExecuteWhatToDoNext();
			Bot->bExecutingWhatToDoNext = FALSE;
			bTriggered = FALSE;
		}
#if PS3
		Cycles += appCycles();
		GTotalBotTimeThisTick += Cycles * GSecondsPerCycle * 1000.f;
#endif
	}
}


enum EBotAIFunctions
{
	BotAI_PollWaitToSeeEnemy = 511,
	BotAI_PollLatentWhatToDoNext = 513,
};

/** Called to force controller to abort current move
*/
void AUDKBot::FailMove()
{
	MoveTimer = -1.f;
	bForceRefreshRoute = TRUE;
}

/** temporarily set by some pathfinding functions to force MarkEndPoints() to only mark EndAnchor */
static UBOOL GOnlyMarkEndAnchor = FALSE;

void AUDKBot::UpdatePawnRotation()
{
	if (TemporaryFocus != NULL && !TemporaryFocus->bDeleteMe && Focus != NULL && Focus == Enemy)
	{
		SetFocalPoint( TemporaryFocus->GetTargetLocation(Pawn, bTargetAlternateLoc) );
	}
	else if (Focus != NULL)
	{
		SetFocalPoint( ( Focus != Pawn )
						? Focus->GetTargetLocation(Pawn, bTargetAlternateLoc)
						: Pawn->Location + 1000.f*Pawn->Rotation.Vector() );
		TrackedVelocity = Focus->Velocity;
		if ( Pawn && CurrentlyTrackedEnemy && (Focus == CurrentlyTrackedEnemy) )
		{
			FVector TargetLoc = GetFocalPoint();
			if ( !bEnemyIsVisible ) 
			{
				if ( bEnemyInfoValid )
					TargetLoc = LastSeenPos;
			}
			else
			{
				// focus on point where we think enemy is going, if we've built up a model
				if ( (SavedPositions.Num() > 0) && (SavedPositions(0).Time <= WorldInfo->TimeSeconds - TrackingReactionTime) )
				{
					// determine his position and velocity at the appropriate point in the past
					for (INT i = 1; i < SavedPositions.Num(); i++)
					{
						if (SavedPositions(i).Time > WorldInfo->TimeSeconds - TrackingReactionTime)
						{
							TargetLoc = SavedPositions(i-1).Position + (SavedPositions(i).Position - SavedPositions(i-1).Position)*(WorldInfo->TimeSeconds - TrackingReactionTime - SavedPositions(i-1).Time)/(SavedPositions(i).Time - SavedPositions(i-1).Time);
							TrackedVelocity = SavedPositions(i-1).Velocity + (SavedPositions(i).Velocity - SavedPositions(i-1).Velocity)*(WorldInfo->TimeSeconds - TrackingReactionTime - SavedPositions(i-1).Time)/(SavedPositions(i).Time - SavedPositions(i-1).Time);

							// model of where target is currently
							TargetLoc = TargetLoc + TrackedVelocity*TrackingReactionTime;
							SetFocalPoint( TargetLoc );
							UBOOL bHitScan = TRUE;
							FLOAT TargetDist = (GetFocalPoint() - Pawn->Location).Size();;

							// lead target if appropriate
							// @TODO FIXMESTEVE - what about shock rifle - need to decide before firing whether to lead or not
							if ( bLeadTarget && !TrackedVelocity.IsNearlyZero() )
							{
								AUDKWeapon* MyWeapon = Pawn ? Cast<AUDKWeapon>(Pawn->Weapon): NULL;
								UClass* ProjClass = MyWeapon && (MyWeapon->CurrentFireMode <  MyWeapon->WeaponProjectiles.Num())
														? MyWeapon->WeaponProjectiles(MyWeapon->CurrentFireMode) 
														: NULL;
								if ( ProjClass && MyWeapon->bLeadTarget )
								{
									AUDKProjectile* DefaultProjectile = Cast<AUDKProjectile>(ProjClass->GetDefaultActor());
									if ( DefaultProjectile )
									{
										FLOAT ProjSpeed = DefaultProjectile->Speed;
										if ( ProjSpeed > 0.f )
										{
											// accurately calculate speed for accelerating projectiles
											FLOAT TravelTime = TargetDist/ProjSpeed;

											if ( MyWeapon->bConsiderProjectileAcceleration && (DefaultProjectile->AccelRate > 0.f) )
											{
												// first calculate time to maxspeed
												FLOAT TimeToMaxSpeed = (DefaultProjectile->MaxSpeed - ProjSpeed)/DefaultProjectile->AccelRate;
												FLOAT DistToMaxSpeed = TimeToMaxSpeed * (DefaultProjectile->Speed + 0.5f * DefaultProjectile->AccelRate);

												if ( DistToMaxSpeed > TargetDist )
												{
													FLOAT AvgSpeedToTarget = DefaultProjectile->Speed + 0.5f * DefaultProjectile->AccelRate * TargetDist/DistToMaxSpeed;
													TravelTime = TargetDist/AvgSpeedToTarget;
												}
												else
												{
													TravelTime = TimeToMaxSpeed + (TargetDist - DistToMaxSpeed)/DefaultProjectile->MaxSpeed;
												}
											}
											FVector FireLocation = Pawn->Location;
											FireLocation.Z += Pawn->BaseEyeHeight;
											bHitScan = FALSE;

											FCheckResult Hit(1.f);
											if ( CurrentlyTrackedEnemy->Physics == PHYS_Falling )
											{
												// take gravity and landing into account
												TrackedVelocity.Z = TrackedVelocity.Z + 0.5f*TravelTime*CurrentlyTrackedEnemy->GetGravityZ();
											}
											SetFocalPoint( GetFocalPoint() + TrackedVelocity*TravelTime );

											if ( (BlockedAimTarget == CurrentlyTrackedEnemy) || (WorldInfo->TimeSeconds - LastAimUpdateTime > AimUpdateFrequency) )
											{
												LastAimUpdateTime = WorldInfo->TimeSeconds;
												BlockedAimTarget = NULL;

												// make sure enemy is not hemmed by wall (or landing)
												GWorld->SingleLineCheck(Hit, CurrentlyTrackedEnemy, GetFocalPoint(), CurrentlyTrackedEnemy->Location, TRACE_World | TRACE_ComplexCollision);
												if ( Hit.Actor )
												{
													BlockedAimTarget = CurrentlyTrackedEnemy;
													SetFocalPoint( Hit.Location - 24.f*TrackedVelocity.SafeNormal() );
												}

												// make sure have a clean shot at where I think he's going
												FVector ProjStart = Pawn->Location;
												ProjStart.Z += Pawn->BaseEyeHeight;
												ProjStart += (GetFocalPoint() - ProjStart).SafeNormal() * MyWeapon->FireOffset.X;
												GWorld->SingleLineCheck(Hit, CurrentlyTrackedEnemy, GetFocalPoint(), ProjStart, TRACE_World | TRACE_ComplexCollision);
												if ( Hit.Actor )
												{
													// first see if head would work
													SetFocalPoint( GetFocalPoint() + FVector(0,0,CurrentlyTrackedEnemy->BaseEyeHeight) );
													GWorld->SingleLineCheck(Hit, CurrentlyTrackedEnemy, GetFocalPoint(), ProjStart, TRACE_World | TRACE_ComplexCollision);
													BlockedAimTarget = CurrentlyTrackedEnemy;

													if ( Hit.Actor )
													{
														// iteratively track down correct aim spot over multiple ticks
														LastIterativeCheck *= 0.5f;
													}
													else 
													{
														LastIterativeCheck = ::Clamp(LastIterativeCheck + 0.5f*(1.f-LastIterativeCheck), 0.f, 1.f);
													}
													SetFocalPoint( TargetLoc + LastIterativeCheck*(GetFocalPoint() - TargetLoc) );
												}
												else
												{
													LastIterativeCheck = 1.f;
												}
											}
										}
									}							
								}
							}

							/*
							if ( CurrentlyTrackedEnemy->Controller && CurrentlyTrackedEnemy->Controller->GetAPlayerController() )
							{
								AUDKPlayerController* PC = Cast<AUDKPlayerController>(CurrentlyTrackedEnemy->Controller);
								if ( PC && PC->bBehindView && PC->myHUD && PC->myHUD->bShowDebugInfo )
								{
									DrawDebugSphere(GetFocalPoint(),24.f, 8, 255, 255, 0, FALSE);
								}
							}
							*/
							if ( WorldInfo->TimeSeconds - LastErrorUpdateTime > ErrorUpdateFrequency )
							{
								LastErrorUpdateTime = WorldInfo->TimeSeconds + appFrand() * 0.1f;
								CurrentAimError = eventAdjustAimError(TargetDist, bHitScan);
							}

							FVector DirY = ((GetFocalPoint() - Pawn->Location) ^ FVector(0.f,0.f,1.f)).SafeNormal();
							SetFocalPoint( GetFocalPoint() + DirY * appTan(CurrentAimError* PI/32768.f) * (GetFocalPoint() - Pawn->Location).Size() );
							break;
						}
					}
				}
			}

			/*
			// draw where bot thinks enemy will go
			if ( CurrentlyTrackedEnemy->Controller && CurrentlyTrackedEnemy->Controller->GetAPlayerController() )
			{
				AUDKPlayerController* PC = Cast<AUDKPlayerController>(CurrentlyTrackedEnemy->Controller);
				if ( PC && PC->bBehindView && PC->myHUD && PC->myHUD->bShowDebugInfo )
				{
					DrawDebugSphere(TargetLoc,24.f, 8, 0, 255, 255, FALSE);
					DrawDebugSphere(GetFocalPoint(),24.f, 8, 255, 0, 0, FALSE);
				}
			}
			*/
		}
		else
		{
			CurrentAimError = 0.f;
			ANavigationPoint *NavFocus = Cast<ANavigationPoint>(Focus);
			if ( NavFocus )
			{
				if ( CurrentPath && CurrentPath->Start && (MoveTarget == NavFocus) && !Pawn->Velocity.IsZero() )
				{
					// gliding pawns must focus on where they are going
					if ( Pawn->IsGlider() )
					{
						SetFocalPoint( bAdjusting ? GetAdjustLocation() : Focus->Location );
					}
					else
					{
						SetFocalPoint( Focus->Location - CurrentPath->Start->Location + Pawn->Location );
					}
				}
			}
		}
	}

	if ( WorldInfo->NetMode != NM_Standalone )
	{
		Pawn->SetRemoteViewPitch((GetFocalPoint() - Pawn->Location - FVector(0.f,0.f, Pawn->BaseEyeHeight)).Rotation().Pitch);
	}

	// rotate pawn toward focus
	Pawn->rotateToward(GetFocalPoint());

	// face same direction as pawn
	Rotation = Pawn->Rotation;

	// set pitch @TODO FIXMESTEVE smoothly rotate there
	FVector Dir = (GetFocalPoint() - (Pawn->Location+FVector(0.f,0.f,Pawn->BaseEyeHeight)));
	if (Focus != NULL && Focus->GetAPawn() && Cast<AVehicle>(Focus) == NULL)
	{
		Dir.Z += ((APawn*)Focus)->BaseEyeHeight;
	}
	Rotation.Pitch = Dir.Rotation().Pitch;

	// draw line showing which direction bot is currently looking
	//DrawDebugLine(Pawn->Location, Pawn->Location + Rotation.Vector()*(Pawn->Location - GetFocalPoint()).Size(), 255, 255,0, FALSE);
}

/*
LineOfSightTo()
returns true if controller's pawn can see Other actor.
Checks line to center of other actor, and possibly to head or box edges depending on distance
*/
DWORD AUDKBot::LineOfSightTo(const AActor* Other, INT bUseLOSFlag, const FVector* chkLocation, UBOOL bTryAlternateTargetLoc)
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

void AUDKBot::UpdateEnemyInfo(APawn* AcquiredEnemy)
{
		LastSeenTime = GWorld->GetTimeSeconds();
		LastSeeingPos = GetViewTarget()->Location;
		LastSeenPos = Enemy->Location;
		bEnemyInfoValid = TRUE;
}

AActor* AUDKBot::FindBestSuperPickup(FLOAT MaxDist)
{
	if ( !Pawn )
	{
		return NULL; 
	}
	AActor * bestPath = NULL;
	PendingMover = NULL;
	bPreparingMove = FALSE;

	// Set super pickups as endpoints
	ANavigationPoint* GOAL = NULL;
	for ( ANavigationPoint *N=GWorld->GetFirstNavigationPoint(); N!=NULL; N=N->nextNavigationPoint )
	{
		APickupFactory *IS = N->GetAPickupFactory();
		if ( IS && IS->bIsSuperItem
			&& (IS->IsProbing(NAME_Touch) || (RespawnPredictionTime > 0.f && IS->LatentFloat > 0.f && IS->LatentFloat <= RespawnPredictionTime))
			&& !IS->BlockedByVehicle()
			&& (eventSuperDesireability(IS) > 0.f) )
		{
			IS->bTransientEndPoint = TRUE;
		}
		// outside above in case caller set additional endpoints to consider
		if (N->bTransientEndPoint && (GOAL == NULL || Pawn->Anchor == NULL || N->NetworkID == Pawn->Anchor->NetworkID))
		{
			GOAL = N;
		}
	}
	if (GOAL == NULL)
	{
		return NULL;
	}
	else
	{
		GOnlyMarkEndAnchor = TRUE;
		bestPath = FindPath(FVector(0.f,0.f,0.f), GOAL, TRUE, appTrunc(MaxDist), FALSE);
		GOnlyMarkEndAnchor = FALSE;
		return bestPath;
	}
}

void AUDKBot::WaitToSeeEnemy()
{
	if ( !Pawn || !Enemy )
		return;

	Focus = Enemy;
	GetStateFrame()->LatentAction = BotAI_PollWaitToSeeEnemy;
}

void AUDKBot::execPollWaitToSeeEnemy( FFrame& Stack, RESULT_DECL )
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
IMPLEMENT_FUNCTION( AUDKBot, BotAI_PollWaitToSeeEnemy, execPollWaitToSeeEnemy);

void AUDKBot::LatentWhatToDoNext()
{
	//@warning: need to set LatentAction before event in case it changes states
	GetStateFrame()->LatentAction = BotAI_PollLatentWhatToDoNext;
	eventWhatToDoNext();
}

void AUDKBot::execPollLatentWhatToDoNext(FFrame& Stack, RESULT_DECL)
{
	if (Pawn == NULL || DecisionComponent == NULL || !DecisionComponent->bTriggered)
	{
		GetStateFrame()->LatentAction = 0;
	}
}
IMPLEMENT_FUNCTION(AUDKBot, BotAI_PollLatentWhatToDoNext, execPollLatentWhatToDoNext);

/* CanMakePathTo()
// assumes valid CurrentPath, tries to see if CurrentPath can be combined with a path to Actor A
*/
UBOOL AUDKBot::CanMakePathTo(class AActor* A)
{
	ANavigationPoint *N = Cast<ANavigationPoint>(A);
	INT Success = 0;

	if ( N && Pawn->ValidAnchor() && CurrentPath 
		&& ((CurrentPath->reachFlags & R_WALK) == CurrentPath->reachFlags) )
	{
		UReachSpec *NextPath = 	CurrentPath->End.Nav()->GetReachSpecTo(N);
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

	if ( item && !item->bDeleteMe && (item->IsProbing(NAME_Touch) || (item->bPredictRespawns && item->LatentFloat > 0.f && item->LatentFloat < (Cast<AUDKBot>(seeker->Controller) ? Cast<AUDKBot>(seeker->Controller)->RespawnPredictionTime : 0.f))) 
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

AActor* AUDKBot::FindBestInventoryPath(FLOAT& Weight)
{
	if ( !Pawn )
	{
		return NULL;
	}
	AActor * bestPath = NULL;
	bPreparingMove = FALSE;

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

	// if we have a min Weight, also cap max path length at a distance where no reasonably rated pickup would exceed Weight after that distance was factored in
	INT MaxPathLength = (Weight > 0.f) ? appTrunc(10.f / Weight) : 0;
	Weight = Pawn->findPathToward(NULL, FVector(0,0,0), &FindBestInventory, Weight, FALSE, MaxPathLength, FALSE, 100);
	if ( Weight > 0.f )
		bestPath = SetPath();

	return bestPath;
}

void AUDKBot::AdjustFromWall(FVector HitNormal, AActor* HitActor)
{
	FLOAT OldMoveTimer = MoveTimer;

	Super::AdjustFromWall(HitNormal, HitActor);

	// try to move to center of path
	if ( MoveTimer < 0.f && OldMoveTimer >= 0.f && !bAdjusting && GetStateFrame()->LatentAction == AI_PollMoveToward &&
		CurrentPath != NULL && CurrentPath->Start != NULL )
	{
		AUDKPawn* UDKPawn = Cast<AUDKPawn>(Pawn);
		if ( UDKPawn != NULL )
		{
			FVector ClosestPoint = CurrentPath->Start->Location + (CurrentPathDir | (Pawn->Location - CurrentPath->Start->Location)) * CurrentPathDir;
			FVector Dir = ClosestPoint - Pawn->Location;
			if (Dir.SizeSquared2D() > Square(Pawn->CylinderComponent->CollisionRadius * 0.5f))
			{
				SetAdjustLocation(ClosestPoint, TRUE);
				MoveTimer = OldMoveTimer;
				// see if need to start by jumping
				//@FIXME: processHitWall() should try jump even to adjust location - then we don't need this
				FCheckResult Hit(1.0f);
				FVector JumpTestPoint = ClosestPoint;
				JumpTestPoint.Z = Pawn->Location.Z;
				if (!GWorld->SingleLineCheck(Hit, Pawn, JumpTestPoint, Pawn->Location, TRACE_AllBlocking, Pawn->GetCylinderExtent()))
				{
					FVector JumpDir = FVector(0.f, 0.f, (UDKPawn->bCanDoubleJump | (UDKPawn->MaxMultiJump > 0)) ? UDKPawn->MaxDoubleJumpHeight : Pawn->MaxJumpHeight);
					FCheckResult Hit2(1.0f);
					if (GWorld->SingleLineCheck(Hit2, Pawn, JumpTestPoint + JumpDir, Pawn->Location + JumpDir, TRACE_AllBlocking | TRACE_StopAtAnyHit, Pawn->GetCylinderExtent()))
					{
						MoveFailureCount++;
						if ( MoveFailureCount > 3 )
						{
							if ( CurrentPath )
							{ 
								CurrentPath->reachFlags |= R_HIGHJUMP;
								CurrentPath->Distance += 500;
								eventTimeDJReset();
								UDKPawn->bCanDoubleJump = FALSE;

							}
							FailMove();
							return;
						}

						//@FIXME: should use PickWallAdjust(), but that uses DesiredDirection() to figure out dest which doesn't take adjust location into account
						JumpOverWall(Hit.Normal);
					}
					else
					{
					  MoveFailureCount++;
					  if ( MoveFailureCount > 2 )
					  {
						  if ( CurrentPath )
						  { 
							  CurrentPath->Distance += 500;
						  }
					  }
						  FailMove();
					}
				}
				else 
				{
					MoveFailureCount++;
					if ( MoveFailureCount > 3 )
					{
						if ( CurrentPath )
						{ 
							CurrentPath->Distance += 500;
						}
					}
					
					if ( CurrentPath )
					{
						// Move toward center of path
						FVector ClosestPoint = Pawn->Location;
						PointDistToLine(Pawn->Location, CurrentPath->End->Location - CurrentPath->Start->Location, CurrentPath->Start->Location, ClosestPoint);
						SetAdjustLocation( ClosestPoint + Pawn->CylinderComponent->CollisionRadius * (ClosestPoint - Pawn->Location).SafeNormal(), TRUE );
					}
				}
			}
		}
	}
}

/** JumpOverWall()
Make pawn jump over an obstruction
*/
void AUDKBot::JumpOverWall(FVector WallNormal)
{
	Super::JumpOverWall(WallNormal);
	bJumpOverWall = TRUE;
	bNotifyApex = TRUE;

	AUDKPawn* MyUTPawn = Cast<AUDKPawn>(Pawn);
	if ( MyUTPawn )
	{
		MyUTPawn->bNoJumpAdjust = TRUE; // don't let script adjust this jump again
		MyUTPawn->bReadyToDoubleJump = TRUE;
	}
}

void AUDKBot::NotifyJumpApex()
{
	eventNotifyJumpApex();
	bJumpOverWall = FALSE;
}

void AUDKBot::PreAirSteering(FLOAT DeltaTime)
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

void AUDKBot::PostAirSteering(FLOAT DeltaTime)
{
	if ( ImpactVelocity.IsZero() )
		return;

	FVector OldVel = ImpactVelocity;
	ImpactVelocity = Pawn->NewFallVelocity(OldVel, Pawn->Acceleration, DeltaTime);
	if ( (ImpactVelocity | OldVel) < 0.f )
		ImpactVelocity = FVector(0.f,0.f,0.f);
}

void AUDKBot::PostPhysFalling(FLOAT DeltaTime)
{
	if ( bInDodgeMove && (Pawn->Velocity.Z < 0.f) && (Pawn->Location.Z < DodgeLandZ + 10.f) )
	{
		bInDodgeMove = FALSE;
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
UBOOL AUDKBot::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
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
			if ( WarningProjectile && !WarningProjectile->bDeleteMe && (WorldInfo->TimeSeconds > WarningDelay) )
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
					AUDKPawn *UTP = Cast<AUDKPawn>(CurrentlyTrackedEnemy);
					if ( UTP )
					{
						UTP->RequestTrackingFor(this);
					}
					else
					{
						AUDKVehicle *UTV = Cast<AUDKVehicle>(CurrentlyTrackedEnemy);
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

void AUDKBot::PrePollMove()
{
	// if we're moving to our CurrentlyTrackedEnemy, move to FocalPoint instead of actual Pawn dest so bot charge is to location bot thinks player is headed
	if ( GetStateFrame()->LatentAction == UCONST_LATENT_MOVETOWARD && MoveTarget != NULL &&
		MoveTarget == Focus && MoveTarget == Enemy && MoveTarget == CurrentlyTrackedEnemy )
	{
		SetDestinationPosition( GetFocalPoint() );
	}
}

/* PostPollMove()
Called after a latent move update is polled, unless the movement completed.
Adjust pawn movement to avoid active FearSpots

*/
void AUDKBot::PostPollMove()
{
	if ( Pawn->Acceleration.IsZero() )
		return;

	FVector FearAdjust(0.f,0.f,0.f);
	for ( INT i=0; i<2; i++ )
	{
		if ( FearSpots[i] )
		{
			AActor* FearSpot = FearSpots[i];
			if (FearSpot->bDeleteMe || !Pawn->IsOverlapping(FearSpot))
			{
				FearSpots[i] = NULL;
			}
			else 
			{
				FearAdjust += (Pawn->Location - FearSpot->Location) / 100.f;
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

UBOOL AUDKBot::AirControlFromWall(FLOAT DeltaTime, FVector& RealAcceleration)
{
	if (!bNotifyFallingHitWall )
	{
		// try aircontrol push
		Pawn->Acceleration = Pawn->Velocity;
		Pawn->Acceleration.Z = 0.f;
		Pawn->Acceleration = Pawn->Acceleration.SafeNormal();
		Pawn->Acceleration *= Pawn->AccelRate;
		RealAcceleration = Pawn->Acceleration;
		return TRUE;
	}
	return FALSE;
}


UReachSpec* AUDKBot::PrepareForMove(ANavigationPoint *NavGoal, UReachSpec* CurrentPath)
{
	// check if cutting a corner on this path would cause the bot to fall
	//@FIXME: @hack: path builder should do this by performing a similiar check for all possible adjusted destinations
	if (CurrentPath->bCanCutCorners && !(CurrentPath->reachFlags & R_JUMP) && CurrentPath->CollisionRadius > appTrunc(Pawn->CylinderComponent->CollisionRadius))
	{
		// temporarily set NextRoutePath for GetDestination()
		NextRoutePath = GetNextRoutePath(NavGoal);

		FVector Dest = NavGoal->GetDestination(this);
		FVector Extent = Pawn->GetCylinderExtent();
		FVector TraceStart = Dest - FVector(0.f, 0.f, NavGoal->CylinderComponent->CollisionHeight - (Extent.Z * 2.f));
		FCheckResult Hit(1.0f);
		if (GWorld->SingleLineCheck(Hit, Pawn, TraceStart - FVector(0.f, 0.f, Pawn->MaxJumpHeight + Extent.Z), TraceStart, TRACE_AllBlocking | TRACE_StopAtAnyHit, Extent))
		{
			CurrentPath->bCanCutCorners = FALSE;
		}

		NextRoutePath = NULL;
	}

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
			ANavigationPoint* NewNavGoal = Cast<ANavigationPoint>(MoveTarget);
			CurrentPath = (NewNavGoal != NULL) ? NavGoal->GetReachSpecTo(NewNavGoal) : NULL;
			NextRoutePath = NULL;
		}
	}
	return CurrentPath;
}

UBOOL AUDKBot::OverridePathTo(ANavigationPoint* EndAnchor, AActor* Goal, const FVector& GoalLocation, UBOOL bWeightDetours, FLOAT& BestWeight)
{
	if (bAllowRouteReuse && !bForceRefreshRoute && (Squad == NULL || Squad->PendingSquadRouteMaker != this) && RouteCache.Num() > 1 && !Pawn->Anchor->bEndPoint)
	{
		INT StartIndex = RouteCache.FindItemIndex(Pawn->Anchor);
		if (StartIndex != INDEX_NONE)
		{
			// iterate along path, making sure it's all still usable
			INT Radius = appTrunc(Pawn->CylinderComponent->CollisionRadius);
			INT Height = appTrunc(Pawn->CylinderComponent->CollisionHeight);
			INT MaxFallSpeed = appTrunc(Pawn->GetAIMaxFallSpeed());
			INT MoveFlags = Pawn->calcMoveFlags();
			// also check Driver, if we're in a vehicle
			UBOOL bInVehicle = FALSE;
			INT DriverRadius = 0, DriverHeight = 0, DriverMaxFallSpeed = 0, DriverMoveFlags = 0;
			AVehicle* V = Cast<AVehicle>(Pawn);
			if (V != NULL && V->Driver != NULL)
			{
				bInVehicle = TRUE;
				DriverRadius = appTrunc(V->Driver->CylinderComponent->CollisionRadius);
				DriverHeight = appTrunc(V->Driver->CylinderComponent->CollisionHeight);
				DriverMaxFallSpeed = appTrunc(V->Driver->GetAIMaxFallSpeed());
				DriverMoveFlags = V->Driver->calcMoveFlags();
			}

			UBOOL bValidPath = TRUE;
			INT TotalCost = 0;
			INT EndIndex = INDEX_NONE;
			for (INT i = StartIndex; bValidPath && EndIndex == INDEX_NONE; i++)
			{
				if (RouteCache(i)->bEndPoint)
				{
					EndIndex = i;
				}
				else if (i >= RouteCache.Num() - 1 || RouteCache(i + 1) == NULL)
				{
					bValidPath = FALSE;
				}
				else
				{
					UReachSpec* Spec = RouteCache(i)->GetReachSpecTo(RouteCache(i + 1));
					if ( Spec == NULL || *Spec->End == NULL ||
						(!Spec->supports(Radius, Height, MoveFlags, MaxFallSpeed) && (!bInVehicle || !Spec->supports(DriverRadius, DriverHeight, DriverMoveFlags, DriverMaxFallSpeed))) )
					{
						bValidPath = FALSE;
					}
					else
					{
						INT Cost = Spec->CostFor(Pawn);
						if (Cost >= UCONST_BLOCKEDPATHCOST && bInVehicle)
						{
							// temporarily swap Controller/Pawn so we can check if the path is usable for the driver
							AController* OldController = V->Driver->Controller;
							V->Driver->Controller = this;
							Pawn = V->Driver;

							Cost = Spec->CostFor(V->Driver);

							V->Driver->Controller = OldController;
							Pawn = V;
						}
						if (Cost >= UCONST_BLOCKEDPATHCOST)
						{
							bValidPath = FALSE;
						}
						else
						{
							TotalCost += Cost;
						}
					}
				}
			}

			if (bValidPath)
			{
				RouteDist = FLOAT(TotalCost) + (EndAnchor->Location - GoalLocation).Size();
				// remove old RouteCache entries
				EndIndex -= StartIndex + 1;
				RouteCache.Remove(0, StartIndex + 1); //@warning: after this, StartIndex is no longer valid
				// remove extra RouteCache entries (e.g. FindPathToSquadRoute() does this)
				if (RouteCache.Num() > EndIndex + 1)
				{
					RouteCache.Remove(EndIndex + 1, RouteCache.Num() - EndIndex - 1);
				}
				// check detour
				if (bWeightDetours && !Pawn->IsA(AVehicle::StaticClass()))
				{
					ANavigationPoint* DetourDest = NULL;
					FLOAT DetourWeight = 0.f;
					for (INT i = 0; i < Pawn->Anchor->PathList.Num(); i++ )
					{
						UReachSpec* Spec = Pawn->Anchor->PathList(i);
						if (*Spec->End != NULL && Spec->supports(Radius, Height, MoveFlags, MaxFallSpeed))
						{
							FLOAT Weight = Spec->CostFor(Pawn);
							if (Weight < 2.f * MAXPATHDIST)
							{
								UReachSpec* Return = Spec->End.Nav()->GetReachSpecTo(Pawn->Anchor);
								if (Return != NULL && !Return->IsForced())
								{
									if ( Weight == 0.f ) debugf(TEXT("ZERO WEIGHT for spec %s from %s to %s"), *Spec->GetName(), *Spec->Start->GetName(), *(Spec->End.Nav())->GetName());
									Spec->End.Nav()->LastDetourWeight = Spec->End.Nav()->eventDetourWeight(Pawn, Weight);
									if (Spec->End.Nav()->LastDetourWeight > DetourWeight)
									{
										DetourDest = Spec->End.Nav();
									}
								}
							}
						}
					}
					if (DetourDest != NULL && !RouteCache.ContainsItem(DetourDest) && eventAllowDetourTo(DetourDest))
					{
						// if possible, go from detour right onto path without going back to current Anchor
						// @todo: possibly try more than one?
						UReachSpec* Spec = DetourDest->GetReachSpecTo(RouteCache(0));
						if (Spec != NULL && Spec->supports(Radius, Height, MoveFlags, MaxFallSpeed) && Spec->CostFor(Pawn) < UCONST_BLOCKEDPATHCOST)
						{
							RouteCache.Insert(0, 1);
							RouteCache(0) = DetourDest;
						}
						else
						{
							RouteCache.Insert(0, 2);
							RouteCache(0) = DetourDest;
							RouteCache(1) = Pawn->Anchor;
						}
					}
				}
				if (RouteCache.Num() > 1)
				{
					UReachSpec* NextSpec = RouteCache(0)->GetReachSpecTo(RouteCache(1));
					Pawn->NextPathRadius = (NextSpec != NULL) ? NextSpec->CollisionRadius : 0.f;
				}
				else
				{
					Pawn->NextPathRadius = 0.f;
				}

				// check if need to leave vehicle now
				BestWeight = 1.f;
				if (V != NULL)
				{
					UReachSpec* CurrentSpec = Pawn->Anchor->GetReachSpecTo(RouteCache(0));
					if (CurrentSpec != NULL && (!CurrentSpec->supports(Radius, Height, MoveFlags, MaxFallSpeed) || CurrentSpec->CostFor(Pawn) >= UCONST_BLOCKEDPATHCOST) && !V->eventContinueOnFoot())
					{
						// should leave vehicle, but can't, so return zero weight to fail pathfinding
						BestWeight = 0.f;
					}
				}

				return TRUE;
			}
		}
	}

	bForceRefreshRoute = FALSE;
	return FALSE;
}

/** disables a vehicle's bRetryPathfindingWithDriver temporarily for squad route finding
 * as in that case we'd rather disable using alternate paths instead of bot abandoning vehicle early to follow it
 */
struct FScopedDisableVehiclePathRetry
{
public:
	FScopedDisableVehiclePathRetry(AVehicle* V)
		: TheVehicle(V)
	{
		if (V != NULL)
		{
			bOldRetryPathfindingWithDriver = V->bRetryPathfindingWithDriver;
			V->bRetryPathfindingWithDriver = FALSE;
		}
	}
	~FScopedDisableVehiclePathRetry()
	{
		if (TheVehicle != NULL)
		{
			TheVehicle->bRetryPathfindingWithDriver = bOldRetryPathfindingWithDriver;
		}
	}
private:
	AVehicle* TheVehicle;
	UBOOL bOldRetryPathfindingWithDriver;
};

AActor* AUDKBot::FindPathToSquadRoute(UBOOL bWeightDetours/* =0 */)
{
	if (!Squad || !Squad->RouteObjective || !Pawn)
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route for FindPathToSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}

	const TArray<ANavigationPoint*>& AlternateRoute = bUsePreviousSquadRoute ? Squad->PreviousObjectiveRouteCache : Squad->ObjectiveRouteCache;

	// if no route cache or bot doesn't want to use the squad route, just go straight to objective
	INT NumNodes = AlternateRoute.Num();
	if (NumNodes == 0 || !bUsingSquadRoute || (Cast<AUDKVehicle>(Pawn) != NULL && !((AUDKVehicle*)Pawn)->bUseAlternatePaths))
	{
		return FindPath(FVector(0,0,0), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
	}

	FScopedDisableVehiclePathRetry(Cast<AVehicle>(Pawn));

	Squad->RouteObjective->bTransientEndPoint = TRUE;
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
		if (AnchorIndex < NumNodes - 4)
		{
			SquadRouteGoal = AlternateRoute(AnchorIndex + 3);
			// also mark points further along, in case they're closer for this bot
			for (INT i = AnchorIndex + 4; i < NumNodes; i++)
			{
				if (AlternateRoute(i) != NULL)
				{
					AlternateRoute(i)->bTransientEndPoint = TRUE;
				}
			}
		}
		else
		{
			SquadRouteGoal = Squad->RouteObjective;
		}
		AActor* Result = FindPath(FVector(0,0,0), SquadRouteGoal, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE );
		if (Result != NULL)
		{
			if (RouteGoal != SquadRouteGoal)
			{
				// check if found later point on path
				for (INT i = AnchorIndex + 3; i < NumNodes; i++)
				{
					if (AlternateRoute(i) == RouteGoal)
					{
						SquadRouteGoal = (ANavigationPoint*)RouteGoal;
						break;
					}
				}
				// if pathfinding found something not on squad route, temporarily disable squad routes so that the bot doesn't get confused about where to go
				if (RouteGoal != SquadRouteGoal)
				{
					SquadRouteGoal = Squad->RouteObjective;
					bUsingSquadRoute = FALSE;
				}
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
		// couldn't find path to rest of route, so give up
		else if (!bUsePreviousSquadRoute)
		{
			bUsingSquadRoute = FALSE;
			return FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE);
		}
	}
	SquadRouteGoal = NULL;
	bUsePreviousSquadRoute = FALSE;

	for (INT i = 0; i < Squad->ObjectiveRouteCache.Num() && Squad->ObjectiveRouteCache(i) != NULL; i++)
	{
		Squad->ObjectiveRouteCache(i)->bTransientEndPoint = TRUE;
	}
	AActor* Result = FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE);
	if (Result != NULL)
	{
		return Result;
	}
	else
	{
		// disable for now if no path
		bUsingSquadRoute = FALSE;
		// this one only checks objective
		return FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, bWeightDetours, UCONST_BLOCKEDPATHCOST, FALSE);
	}
}

void AUDKBot::BuildSquadRoute()
{
	// clear current squad route
	Squad->ObjectiveRouteCache.Empty();
	Squad->PendingSquadRouteMaker = this;

	if ( !Squad || !Squad->RouteObjective || !Pawn )
	{
		//debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route objective for BuildSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return;
	}

	if (Squad->MaxSquadRoutes <= 0)
	{
		debugf(NAME_DevPath, TEXT("BuildSquadRoute(): MaxSquadRoutes is <= 0"));
		Squad->PendingSquadRouteMaker = NULL;
		return;
	}

	if (Squad->SquadRouteIteration < 0)
	{
		Squad->SquadRouteIteration = 0;
	}

	// first, add transient cost to all previous routes in the list and all nodes adjacent to them
	for (INT i = 0; i < Squad->SquadRouteIteration && i < Squad->SquadRoutes.Num(); i++)
	{
		FAlternateRoute& Route = Squad->SquadRoutes(i);
		INT RouteLength = Route.RouteCache.Num();
		for (INT j = 0; j < Route.RouteCache.Num(); j++)
		{
			ANavigationPoint* Nav = Route.RouteCache(j);
			if (Nav != NULL)
			{
				FLOAT CostFactor = (j <= RouteLength / 2) ? j : (RouteLength - j);
				INT FinalCost = appTrunc(CostFactor * 3000.f / RouteLength);
				Nav->TransientCost += FinalCost;
				for (INT k = 0; k < Nav->PathList.Num(); k++)
				{
					if (Nav->PathList(k)->End.Nav() != NULL)
					{
						Nav->PathList(k)->End.Nav()->TransientCost += FinalCost;
					}
				}
			}
		}
	}

	AActor* Result = FindPath(FVector(0.f,0.f,0.f), Squad->RouteObjective, FALSE, UCONST_BLOCKEDPATHCOST, FALSE );

	if ( !Result )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No squad route found in BuildSquadRoute by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		Squad->PendingSquadRouteMaker = NULL;
		return;
	}

	// fill Squad ObjectiveRouteCache with route
	for (INT i=0; i<RouteCache.Num(); i++ )
	{
		Squad->ObjectiveRouteCache.AddItem(RouteCache(i));
	}
	// add to list so we can build upon it later
	if (Squad->SquadRouteIteration >= Squad->SquadRoutes.Num())
	{
		Squad->SquadRoutes.AddZeroed(Squad->SquadRouteIteration - Squad->SquadRoutes.Num() + 1);
	}
	Squad->SquadRoutes(Squad->SquadRouteIteration).RouteCache = Squad->ObjectiveRouteCache;
	Squad->SquadRouteIteration = (Squad->SquadRouteIteration + 1) % Squad->MaxSquadRoutes;

	Squad->PendingSquadRouteMaker = NULL;
}

FLOAT AUDKBot::SpecialJumpCost(FLOAT RequiredJumpZ)
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

FRotator AUDKBot::SetRotationRate(FLOAT deltaTime)
{
	INT YawDiff = Abs((Rotation.Yaw & 65535) - Pawn->DesiredRotation.Yaw);
	INT deltaYaw = 0;
	if ( Enemy && (Focus == Enemy) )
	{
		deltaYaw = (bEnemyAcquired && !Enemy->IsInvisible()) ? ::Max(Pawn->RotationRate.Yaw, RotationRate.Yaw) : AcquisitionYawRate;
		if ( (YawDiff < 2048) || (YawDiff > 63287) )
			bEnemyAcquired = TRUE;
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
UBOOL AUDKBot::ForceReached(ANavigationPoint *Nav, const FVector& TestPosition)
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

void AUDKBot::MarkEndPoints(ANavigationPoint* EndAnchor, AActor* Goal, const FVector& GoalLocation)
{
	// if we're building or following a squad alternate route, we must only mark the end anchor, otherwise FindPathToSquadRoute() will get confused about the bot's goal
	// also check if specifically requested only marking end
	if (GOnlyMarkEndAnchor || (Squad != NULL && (Goal == SquadRouteGoal || Squad->PendingSquadRouteMaker == this)))
	{
		EndAnchor->bEndPoint = TRUE;
	}
	else
	{
		Super::MarkEndPoints(EndAnchor, Goal, GoalLocation);
	}
}

/**
  *  Some early outs in C++ for performance
  */
void AUDKBot::HearNoise(AActor* NoiseMaker, FLOAT Loudness, FName NoiseType)
{
	if ( !NoiseMaker->Instigator || !NoiseMaker->Instigator->Controller || !WorldInfo->GRI || WorldInfo->GRI->OnSameTeam(this, NoiseMaker->Instigator) )
	{
		return;
	}
	if ( (!Enemy || !LineOfSightTo(Enemy))
		&& IsProbing(NAME_HearNoise) 
		&& CanHear(NoiseMaker->Location, Loudness, NoiseMaker) )
	{
		eventHearNoise(Loudness, NoiseMaker, NoiseType);
	}
}

/* 
Returns 1 if controller can hear this noise
Several types of hearing are supported

Noises must be perceptible (based on distance, loudness, and the alerntess of the controller

  Options for hearing are: (assuming the noise is perceptible

  bLOSHearing = Hear any perceptible noise which is not blocked by geometry
  bMuffledHearing = Hear occluded noises if closer
*/
UBOOL AUDKBot::CanHear(const FVector& NoiseLoc, FLOAT Loudness, AActor *Other)
{
	if ( !Other->Instigator || !Other->Instigator->Controller || !Pawn )
		return FALSE; //ignore sounds from uncontrolled (dead) pawns, or if don't have a pawn to control

	FLOAT DistSq = (Pawn->Location - NoiseLoc).SizeSquared();
	FLOAT Perceived = Loudness * Square(HearingThreshold);

	// take pawn alertness into account (it ranges from -1 to 1 normally)
	Perceived *= ::Max(0.f,(Pawn->Alertness + 1.f));

	// check if sound is too quiet to hear
	if ( Perceived < DistSq )
		return FALSE;

	// if can hear muffled sounds, see if they are within half range
	if ( Perceived > 4.f * DistSq )
	{
		return TRUE;
	}

	// check if sound is occluded
	FVector ViewLoc = Pawn->Location + FVector(0.f,0.f,Pawn->BaseEyeHeight);
	FCheckResult Hit(1.f);
	GWorld->SingleLineCheck(Hit, this, NoiseLoc, ViewLoc, TRACE_World);
	return ( Hit.Time == 1.f );
		return TRUE;

	return FALSE;
}

DWORD AUDKBot::SeePawn(APawn* Other, UBOOL bMaySkipChecks)
{
	if (Pawn == NULL || Other == NULL)
	{
		return 0;
	}
	else
	{
		UUDKMapInfo* MapInfo = Cast<UUDKMapInfo>(WorldInfo->GetMapInfo());
		if ( MapInfo != NULL && MapInfo->VisibilityModifier < 1.0f && 
			(Other->Location - Pawn->Location).SizeSquared() > Square(Pawn->SightRadius * MapInfo->VisibilityModifier) )
		{
			return 0;
		}
		else
		{
			return Super::SeePawn(Other, bMaySkipChecks);
		}
	}
}


/** ShouldCheckVisibilityOf()
returns true if should check whether pawn controlled by controller C is visible
*/
UBOOL AUDKBot::ShouldCheckVisibilityOf(AController *C)
{
	if ( Super::ShouldCheckVisibilityOf(C) && C->Pawn )
	{
		AUDKPawn* CheckPawn = Cast<AUDKPawn>(C->Pawn);
		if ( CheckPawn && CheckPawn->bFeigningDeath && (CheckPawn != Enemy) && (Skill < 4.5f) && (!WorldInfo->Game || (WorldInfo->Game->NumBots > 1) || (Skill < 2.f)) )
		{
			return FALSE;
		}

		return TRUE;
	}
	return FALSE;
}

