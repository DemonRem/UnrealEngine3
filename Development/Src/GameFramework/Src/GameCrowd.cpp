/**
* Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
*/
#include "GameFramework.h"

#include "EngineSequenceClasses.h"
#include "EngineAnimClasses.h"
#include "DebugRenderSceneProxy.h"

IMPLEMENT_CLASS(AGameCrowdAgent);
IMPLEMENT_CLASS(AGameCrowdAgentSkeletal);
IMPLEMENT_CLASS(AGameCrowdForcePoint);
IMPLEMENT_CLASS(AGameCrowdAttractor);
IMPLEMENT_CLASS(AGameCrowdRepulsor);
IMPLEMENT_CLASS(AGameCrowdInteractionPoint);
IMPLEMENT_CLASS(AGameCrowdDestination);
IMPLEMENT_CLASS(USeqAct_GameCrowdSpawner);
IMPLEMENT_CLASS(AGameCrowdReplicationActor);
IMPLEMENT_CLASS(USeqEvent_CrowdAgentReachedDestination);
IMPLEMENT_CLASS(USeqAct_PlayAgentAnimation);
IMPLEMENT_CLASS(UGameDestinationConnRenderingComponent);
IMPLEMENT_CLASS(UGameCrowdAgentBehavior);
IMPLEMENT_CLASS(AGameCrowdDestinationQueuePoint);
IMPLEMENT_CLASS(UGameCrowdBehavior_PlayAnimation);
IMPLEMENT_CLASS(UGameCrowdBehavior_WaitInQueue);
IMPLEMENT_CLASS(UGameCrowdBehavior_RunFromPanic);
IMPLEMENT_CLASS(UGameCrowdBehavior_WaitForGroup);
IMPLEMENT_CLASS(UGameCrowdGroup);
IMPLEMENT_CLASS(USeqAct_GameCrowdPopulationManagerToggle);
IMPLEMENT_CLASS(AGameCrowdPopulationManager);

// Crowd stats
DECLARE_STATS_GROUP(TEXT("Crowd"), STATGROUP_Crowd);

// Nav mesh stat enums. This could be in UnPath.h if we need stats in 
// UnNavigationMesh.cpp, etc (but then more rebuilds needed when changing)
enum ECrowdStats
{
	STAT_CrowdTotal = STAT_CrowdFirstStat,
	STAT_AgentTick,
	STAT_AgentPhysics,
	STAT_CrowdPopMgr,
	STAT_CrowdForce,
};
DECLARE_CYCLE_STAT(TEXT("Crowd Total"), STAT_CrowdTotal, STATGROUP_Crowd);
DECLARE_CYCLE_STAT(TEXT("Force Points"), STAT_CrowdForce, STATGROUP_Crowd);
DECLARE_CYCLE_STAT(TEXT("Pop Manager"), STAT_CrowdPopMgr, STATGROUP_Crowd);
DECLARE_CYCLE_STAT(TEXT("..Agent Physics"), STAT_AgentPhysics, STATGROUP_Crowd);
DECLARE_CYCLE_STAT(TEXT("Agent Full Tick"), STAT_AgentTick, STATGROUP_Crowd);

void AGameCrowdAgent::PreBeginPlay()
{
	GetLevel()->CrossLevelActors.AddItem(this);

	Super::PreBeginPlay();
}

void AGameCrowdAgent::PostScriptDestroyed()
{
	//@note: this won't be called if the agent is simply GC'ed due to level change/removal,
	// but in that case the level must be being GC'ed as well, making this unnecessary
	GetLevel()->CrossLevelActors.RemoveItem(this);

	Super::PostScriptDestroyed();
}

void AGameCrowdAgent::GetActorReferences(TArray<FActorReference*>& ActorRefs, UBOOL bIsRemovingLevel)
{
	if (bIsRemovingLevel)
	{
		// simply clear our references regardless as we can easily regenerate them from the remaining levels
		// FIXMESTEVE - clear navigation handle?
	}
	Super::GetActorReferences(ActorRefs, bIsRemovingLevel);
}

INT CalcDeltaYaw(const FRotator& A, const FRotator& B)
{
	INT DeltaYaw = (A.Yaw & 65535) - (B.Yaw & 65535);
	if(DeltaYaw > 32768)
		DeltaYaw -= 65536;
	else if(DeltaYaw < -32768)
		DeltaYaw += 65536;

	return DeltaYaw;
}

void AGameCrowdAgent::ClampVelocity(FLOAT DeltaTime, const FVector& OldVelocity, const FVector& ObstacleForce, const FVector& TotalForce)
{
	if ( bClampMovementSpeed && (Velocity.SizeSquared() > MaxSpeed*MaxSpeed) )
	{
		Velocity = Velocity.SafeNormal() * MaxSpeed;
	}
}

void AGameCrowdAgentSkeletal::ClampVelocity(FLOAT DeltaTime, const FVector& OldVelocity, const FVector& ObstacleForce, const FVector& TotalForce)
{
		// note: if not recently rendered, can't use root motion for velocity if skeleton doesn't update when not rendered
		if ( bUseRootMotionVelocity && !SkeletalMeshComponent->bNoSkeletonUpdate && (SkeletalMeshComponent->bUpdateSkelWhenNotRendered || (WorldInfo->TimeSeconds - SkeletalMeshComponent->LastRenderTime < 1.f)) )
		{
			// clamp velocity to root motion magnitude
			float RootMotionTranslationSizeSquared = SkeletalMeshComponent->RootMotionDelta.GetTranslation().SizeSquared();
			if ( Velocity.SizeSquared() > RootMotionTranslationSizeSquared/(DeltaTime*DeltaTime) )
			{
				Velocity = Velocity.SafeNormal() * appSqrt(RootMotionTranslationSizeSquared)/DeltaTime;
			}
			else 
			{
				// if velocity is too slow for root motion, but there's no reason not to sync up, do it
				// Make sure not increasing velocity into obstacles
				if ( (ObstacleForce | Velocity) >= 0.f )
				{
					FVector Vel2D = OldVelocity;
					Vel2D.Z = 0.f;
					if ( (Vel2D.SafeNormal() | TotalForce.SafeNormal()) > 0.999f )
					{
						Velocity = Velocity.SafeNormal() * appSqrt(RootMotionTranslationSizeSquared)/DeltaTime;
					}
				}
			}

			// if we're using root motion, then clear accumulated root motion
			// See USkeletalMeshComponent::UpdateSkelPose() for details.
			SkeletalMeshComponent->RootMotionDelta.SetTranslation(FVector::ZeroVector);
		}
		else
		{
			Super::ClampVelocity(DeltaTime, OldVelocity, ObstacleForce, TotalForce);
		}
}

/** 
  * Clamp velocity to reach destination exactly
  *  Assumes valid CurrentDestination
  */
void AGameCrowdAgent::ExactVelocity(FLOAT DeltaTime)
{
	if ( (CurrentDestination->Location - Location).SizeSquared() <= Velocity.SizeSquared() * DeltaTime*DeltaTime )
	{
		Velocity = (CurrentDestination->Location - Location)/DeltaTime;
	}
	else
	{
		FVector ExactDir = CurrentDestination->Location - Location;
		ExactDir.Z = 0.f;
		Velocity = Velocity.Size() * ExactDir.SafeNormal();
	}
}

/** Override physics update to do our own movement stuff */
void AGameCrowdAgent::performPhysics(FLOAT DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AgentPhysics);
	// Don't update if crowds disabled or this guy disabled
	if( !bSimulateThisTick )
	{
		return;
	}

	if( Location.Z < WorldInfo->KillZ )
	{
		eventFellOutOfWorld(WorldInfo->KillZDamageType);
		return;
	}

	FVector TotalForce = ExternalForce;
	ExternalForce = FVector(0.f);

	// Force velocity to zero while idling.
	if( IsIdle() )
	{
		Velocity = FVector(0.f);
		TotalForce = FVector(0.f);
	}
	else
	{
		FVector OldVelocity = Velocity;

		TotalForce += CheckRelevantAttractors();

		// Update forces between crowd members
		TotalForce += CalcIntraCrowdForce();


		FVector ObstacleForce = FVector::ZeroVector;
		FVector PathForce = FVector::ZeroVector;


		// so we sometimes want CrowdForcePoints to have a really large influence on crowd agents
		// the problem is that as soon as you start adding path forces 
		// you end up not getting your guys to be affected by the ForcePoints
		// so if you have a force that is over some amount; don't do any of that.
		if( TotalForce.Size() <= ForceAmountToIgnorePathForces )
		{
			ObstacleForce = TotalForce;

			// Update force to/along path
			PathForce = CalcPathForce();
			TotalForce += PathForce;
		}

		//warnf( TEXT( "TotalForce: %f"), TotalForce.Size() );


		// Velocity damping
		FLOAT VMag = Velocity.Size();
		TotalForce -= (Velocity * VMag * VelocityDamping);

		// Ensure forces only in Z plane.
		TotalForce.Z = 0.f;

		// Integrate force to get new velocity
		Velocity += TotalForce * DeltaTime;

		// Clamp velocity based on max movement speed or root motion
		ClampVelocity(DeltaTime, OldVelocity, ObstacleForce, TotalForce);

		// Clamp velocity to not overshoot destination
		if( CurrentDestination && (CurrentDestination->Location == IntermediatePoint) && (DeltaTime > 0.f) )
		{
			FVector ClosestPoint(0.f);
			FVector EndPoint = Location+Velocity*DeltaTime;
			FLOAT NearestDist = PointDistToSegment(IntermediatePoint, Location+Velocity*DeltaTime, Location, ClosestPoint);
			if ( CurrentDestination->ReachedByAgent(this, EndPoint, FALSE) )
			{
				if ( CurrentDestination->bMustReachExactly )
				{
					ExactVelocity(DeltaTime);
				}
			}
			else if ( CurrentDestination->ReachedByAgent(this, ClosestPoint, FALSE) )
			{
				if ( CurrentDestination->bMustReachExactly )
				{
					ExactVelocity(DeltaTime);
				}
				else
				{
					// FIXMESTEVE - to smooth more, could just clamp to a velocity that keeps from overshooting too much
					Velocity = (ClosestPoint - Location)/DeltaTime;
				}
			}
			else if ( ((Velocity | (CurrentDestination->Location - EndPoint)) < 0.f) )
			{
				// Force toward
				if ( NearestDist < AwareRadius )
				{
					ExactVelocity(DeltaTime);
				}
				else
				{
					Velocity += PathForce;
				}
			}
		}

		// Integrate velocity to get new position.
		FVector NewLocation = Location + (Velocity * DeltaTime);

		// Adjust velocity to avoid obstacles (as defined by NavMesh)
		if ( bCheckForObstacles && CurrentDestination )
		{
			bHitObstacle = FALSE;
			bBadHitNormal = FALSE;
			ObstacleCheckCount++;
			if ( !NavigationHandle )
			{
				eventInitNavigationHandle();
			}

			if ( NavigationHandle )
			{
				FCheckResult Hit(1.f);
				FVector NewPosition = NewLocation;
				NewPosition.Z += EyeZOffset;
				FVector TestVelocity = 0.5f * MaxWalkingSpeed * Velocity.SafeNormal();
				TestVelocity.Z = ::Max(0.f, TestVelocity.Z); // Not a good assumption if using crowd system for flying/swimming creatures, but neither is navmesh

				if ( !NavigationHandle->StaticObstacleLineCheck( GetOuter(), Hit, LastKnownGoodPosition, NewPosition + TestVelocity, FVector(0.f)) )
				{
					bHitObstacle = TRUE;
					//DrawDebugLine(NewPosition + TestVelocity, LastKnownGoodPosition,255,0,0,0);

					if ( (Velocity.SizeSquared() < 50.f) && (WorldInfo->TimeSeconds - LastPathingAttempt > 1.f) && (WorldInfo->TimeSeconds - LastRenderTime < 5.f) )
					{
						eventUpdateIntermediatePoint();
					}

					// FIXMESTEVE - maybe just kill if not visible to player
					// slide along wall
					FLOAT AgentSpeed = Velocity.Size();
					if ( Hit.Normal.IsZero() )
					{
						bBadHitNormal = TRUE;
						// FIXMESTEVE - need to figure out why this happens!
						//debugf(TEXT("%s NO VALID HITNORMAL"), *GetName());
						Velocity *= -1.f;
					}
					else if ( Hit.Normal.Z < WalkableFloorZ )
					{
						//debugf(TEXT("%s hit %f"), *GetName(), Hit.Normal.Z);
						Velocity = (Velocity - Hit.Normal * (Velocity | Hit.Normal)) * (1.f - Hit.Time);
						Velocity = AgentSpeed * Velocity.SafeNormal();
					}
					NewLocation = Location + (Velocity * DeltaTime);
 				}
				else
				{
					//DrawDebugLine(NewPosition + TestVelocity, LastKnownGoodPosition ,255,255,255,0);

					LastKnownGoodPosition = NewPosition;
				}
			}
		}

		// If desired, use ZELC to follow ground
		ConformTraceFrameCount++;
		if( ConformType != CFM_None )
		{
			// See if enough frames have passed to update target Z again
			if(ConformTraceFrameCount >= CurrentConformTraceInterval)
			{
				ConformTraceFrameCount = 0;

				UBOOL bSuccess = UpdateInterpZTranslation(NewLocation);
				if(!bSuccess)
				{
					return;
				}
			}

			// Move location towards Z target, at ConformZVel speed
			NewLocation.Z += InterpZTranslation;
		}

		// Point in direction of travel
		FRotator NewRotation = Velocity.Rotation();

		// Cap the maximum yaw rate
		INT DeltaYaw = CalcDeltaYaw(NewRotation, Rotation);
		INT MaxYaw = appRound(DeltaTime * MaxYawRate);
		DeltaYaw = ::Clamp(DeltaYaw, -MaxYaw, MaxYaw);
		NewRotation.Yaw = Rotation.Yaw + DeltaYaw;

		if ( !bAllowPitching )
		{
			NewRotation.Pitch = 0;
			NewRotation.Roll = 0;
		}

		// Actually move the Actor
		FCheckResult Hit(1.f);
		GWorld->MoveActor(this, NewLocation - Location, NewRotation, 0, Hit);
	}
}

UBOOL AGameCrowdAgent::IsIdle()
{
	return CurrentBehavior && CurrentBehavior->bIdleBehavior;
}

UBOOL AGameCrowdAgent::IsPanicked()
{
	return CurrentBehavior && CurrentBehavior->bIsPanicked;
}

/** Stop agent moving and play death anim */
void AGameCrowdAgent::PlayDeath(FVector KillMomentum)
{
	if ( CurrentBehavior )
	{
		eventStopBehavior();
	}
	SetCollision(FALSE, FALSE, FALSE); // Turn off all collision when dead.
	LifeSpan = DeadBodyDuration;
	eventFireDeathEvent();
}

/** Stop agent moving and play death anim */
void AGameCrowdAgentSkeletal::PlayDeath(FVector KillMomentum)
{
	Super::PlayDeath(KillMomentum);
	if( (DeathAnimNames.Num() > 0) && FullBodySlot )
	{
		const INT AnimIndex = appRand() % DeathAnimNames.Num();
		FullBodySlot->PlayCustomAnim(DeathAnimNames(AnimIndex), 1.f, 0.2f, -1.f, FALSE, TRUE);
		FullBodySlot->SetActorAnimEndNotification(TRUE);
		bIsPlayingDeathAnimation = TRUE;
	}
}

UBOOL AGameCrowdAgent::ShouldTrace(UPrimitiveComponent* Primitive, AActor *SourceActor, DWORD TraceFlags)
{
	if( SourceActor && !SourceActor->Instigator && SourceActor->IsA(AWeapon::StaticClass()) )
	{
		return FALSE;
	}

	return Super::ShouldTrace(Primitive, SourceActor, TraceFlags);
}

void AGameCrowdAgentSkeletal::SetRootMotion(UBOOL bRootMotionEnabled)
{
	if ( ActionSeqNode && SkeletalMeshComponent )
	{
		if ( bRootMotionEnabled )
		{
			ActionSeqNode->SetRootBoneAxisOption(RBA_Translate, RBA_Translate, RBA_Translate);
			SkeletalMeshComponent->RootMotionMode = RMM_Translate;
		}
		else
		{
			ActionSeqNode->SetRootBoneAxisOption(RBA_Default, RBA_Default, RBA_Default);
			SkeletalMeshComponent->RootMotionMode = bUseRootMotionVelocity? RMM_Accel : RMM_Ignore;
		}
	}
}

////////////////////////////////////////////////////
// USEQACT_GAMECROWDSPAWNER
////////////////////////////////////////////////////

void USeqAct_GameCrowdSpawner::KillAgents()
{
	// Iterate over list of dudes
	for(INT i=0; i<SpawnedList.Num(); i++)
	{
		AGameCrowdAgent* Agent = SpawnedList(i);
		if(Agent)
		{
			GWorld->DestroyActor(Agent);
		}
	}
	SpawnedList.Empty();

	// Tell clients if necessary
	if(GWorld->GetNetMode() != NM_Client && RepActor)
	{
		RepActor->DestroyAllCount++;
		RepActor->bNetDirty = TRUE;
	}
}

void USeqAct_GameCrowdSpawner::UpdateSpawning(FLOAT DeltaSeconds)
{
	UpdateOp(DeltaSeconds);
}

void USeqAct_GameCrowdSpawner::SpawnedAgent(AGameCrowdAgent *NewSpawn)
{
	// assign to any Linked variables
	TArray<UObject**> ObjVars;
	GetObjectVars(ObjVars,TEXT("Spawned Agent"));
	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		*(ObjVars(Idx)) = NewSpawn;
	}
	// activate the spawned output
	OutputLinks(0).bHasImpulse = TRUE;
}

/** Cache SpawnLocs from attached Kismet vars. */
void USeqAct_GameCrowdSpawner::CacheSpawnerVars()
{
	// Cache spawn locations.
	SpawnLocs.Empty();
	TArray<UObject**> Objs;
	GetObjectVars(Objs,TEXT("Spawn Points"));
	for(INT Idx = 0; Idx < Objs.Num(); Idx++)
	{
		AActor* TestActor = Cast<AActor>( *(Objs(Idx)) );
		if (TestActor != NULL)
		{
			// use the pawn instead of the controller
			if ( TestActor->GetAController() && TestActor->GetAController()->Pawn )
			{
				TestActor = TestActor->GetAController()->Pawn;
			}

			SpawnLocs.AddItem(TestActor);
		}
	}
}

void USeqAct_GameCrowdSpawner::Activated()
{
	// START
	if( InputLinks(0).bHasImpulse )
	{
		bSpawningActive = TRUE;

		CacheSpawnerVars();

		if( GEngine->IsSplitScreen() && !bHasReducedNumberDueToSplitScreen )
		{
			SpawnNum = appCeil(SpawnNum * SplitScreenNumReduction);
			bHasReducedNumberDueToSplitScreen = TRUE;
		}
	}
	// STOP
	else if( InputLinks(1).bHasImpulse )
	{
		bSpawningActive = FALSE;
	}
	// DESTROY ALL
	else if (InputLinks(2).bHasImpulse)
	{
		KillAgents();

		// Stop spawning as well
		bSpawningActive = FALSE;
	}
}

UBOOL USeqAct_GameCrowdSpawner::UpdateOp(FLOAT DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_CrowdTotal);

	// START
	if( InputLinks(0).bHasImpulse )
	{
		bSpawningActive = TRUE;
	}
	// STOP
	else if( InputLinks(1).bHasImpulse )
	{
		bSpawningActive = FALSE;
	}
	// DESTROY ALL
	else if (InputLinks(2).bHasImpulse)
	{
		KillAgents();

		// Stop spawning as well
		bSpawningActive = FALSE;
	}

	InputLinks(0).bHasImpulse = 0;
	InputLinks(1).bHasImpulse = 0;
	InputLinks(2).bHasImpulse = 0;

	// Create actor to replicate crowd control to clients
	if(GWorld->GetNetMode() != NM_Client)
	{
		if(!RepActor)
		{
			RepActor = (AGameCrowdReplicationActor*)GWorld->SpawnActor(AGameCrowdReplicationActor::StaticClass());
			RepActor->Spawner = this;
		}

		if(RepActor)
		{
			if(RepActor->bSpawningActive != bSpawningActive)
			{
				RepActor->bSpawningActive = bSpawningActive;
				RepActor->bNetDirty = TRUE;
			}
		}
	}

	INT Idx = 0;
	while(Idx < SpawnedList.Num())
	{
		if(!SpawnedList(Idx) || SpawnedList(Idx)->bDeleteMe)
		{
			SpawnedList.Remove(Idx--);

			// If we don't want to spawn more 
			if(!bRespawnDeadAgents)
			{
				SpawnNum--;
			}
		}
		Idx++;
	}

	// If crowds disabled - keep active but don't spawn any crowd members
	// also if active but too many agents out - keep active
	if( GWorld->bDisableCrowds || (SpawnedList.Num() >= SpawnNum) )
	{
		return FALSE;
	}

	// No longer spawning - stop updating
	// Also if no locations found - don't spawn - stop being active
	if( !bSpawningActive || (SpawnLocs.Num() == 0) )
	{
		return TRUE;
	}

	Remainder += (DeltaSeconds * SpawnRate);

	while(Remainder > 1.f && SpawnedList.Num() < SpawnNum)
	{
		INT SpawnIndex;
		
		if( !bCycleSpawnLocs )
		{
			// pick a random spawn point
			SpawnIndex = appRand() % SpawnLocs.Num();
		}
		else
		{
			// use the next spawn point in the array
			LastSpawnLocIndex++;
			if( LastSpawnLocIndex >= SpawnLocs.Num() )
			{
				LastSpawnLocIndex = 0;
			}
			SpawnIndex = LastSpawnLocIndex;
		}
		
		UBOOL bHaveValidSpawnLoc = !bOnlySpawnHidden;

		if ( bOnlySpawnHidden )
		{
			// get local player

			// pick a spawn location that can't be seen 
			for ( INT i=SpawnIndex; i<SpawnLocs.Num(); i++ )
			{
				if ( SpawnIsHidden(SpawnLocs(i)) )
				{
					SpawnIndex = i;
					bHaveValidSpawnLoc = TRUE;
					break;
				}
			}
			if ( !bHaveValidSpawnLoc )
			{
				for ( INT i=0; i<SpawnIndex; i++ )
				{
					if ( SpawnIsHidden(SpawnLocs(i)) )
					{
						SpawnIndex = i;
						bHaveValidSpawnLoc = TRUE;
						break;
					}
				}
			}
		}
		if ( bHaveValidSpawnLoc )
		{
			eventSpawnAgent(SpawnLocs(SpawnIndex));
			Remainder -= 1.f;
		}
		else
		{
			Remainder = 0.f;
		}
	}

	// Keep active
	return FALSE;
}

// FIXMESTEVE - this will need to be more sophisticated
UBOOL USeqAct_GameCrowdSpawner::SpawnIsHidden(AActor *SpawnLoc)
{
	for( INT Idx=0; Idx<GEngine->GamePlayers.Num(); Idx++ )   
	{  
		if (GEngine->GamePlayers(Idx) &&  
			GEngine->GamePlayers(Idx)->Actor &&  
			GEngine->GamePlayers(Idx)->Actor->IsLocalPlayerController())  
		{  
			APlayerController* PC = GEngine->GamePlayers(Idx)->Actor;
			FRotator CameraRotation(0,0,0);
			FVector CameraLocation(0.f);
			PC->eventGetPlayerViewPoint(CameraLocation, CameraRotation);

			// if behind player, then works
			if ( ((SpawnLoc->Location - CameraLocation) | CameraRotation.Vector()) < 0.f )
			{
				return TRUE;
			}

			// if facing player, do a trace to verify line of sight
			FCheckResult Hit(1.f);
			GWorld->SingleLineCheck( Hit, SpawnLoc, CameraLocation, SpawnLoc->Location, TRACE_World|TRACE_StopAtAnyHit|TRACE_ComplexCollision );
			return( Hit.Time < 1.f );
		}
	}
	return TRUE;
}

/** Add to WorldInfo's crowd spawner list */
void USeqAct_GameCrowdSpawner::Initialize()
{
	Super::Initialize();
	GetWorldInfo()->CrowdSpawnerActions.AddUniqueItem(this);
}

/** Called when level is unstreamed */
void USeqAct_GameCrowdSpawner::CleanUp()
{
	// Destroy actor used for replication when action is cleaned up
	if(RepActor)
	{
		GWorld->DestroyActor(RepActor);
		RepActor = NULL;
	}
	GetWorldInfo()->CrowdSpawnerActions.RemoveItemSwap(this);
}

UBOOL AGameCrowdAgent::WantsOverlapCheckWith(AActor* TestActor)
{
	// FIXMESTEVE - GetACrowdAgent() or actor bool?
	return TestActor && (TestActor->GetAPawn() || Cast<AGameCrowdAgent>(TestActor) || (TestActor->GetAVolume() && Cast<APhysicsVolume>(TestActor)));
}

/** 
  * Update NearbyDynamics lists used for avoidance purposes. 
  */
void AGameCrowdAgent::UpdateProximityInfo()
{
	SCOPE_CYCLE_COUNTER(STAT_AgentPhysics);

	NearbyDynamics.Reset();

	// skip update if idle, or if not recently rendered and far from player
	if ( IsIdle() )
	{
		return;
	}

	FLOAT CheckDistSq = (WorldInfo->TimeSeconds - LastRenderTime > 2.0) ? ProximityLODDist*ProximityLODDist : VisibleProximityLODDist*VisibleProximityLODDist;
	UBOOL bFoundNearbyPlayer = FALSE;
	for( INT Idx = 0; Idx < GEngine->GamePlayers.Num(); ++Idx )   
	{  
		if (GEngine->GamePlayers(Idx) &&  
			GEngine->GamePlayers(Idx)->Actor &&  
			GEngine->GamePlayers(Idx)->Actor->IsLocalPlayerController())  
		{  
			APlayerController* PC = GEngine->GamePlayers(Idx)->Actor;  
			// FIXMESTEVE - base on camera position, not viewtarget position
			PC->GetViewTarget();
			if ( PC->ViewTarget && ((PC->ViewTarget->Location - Location).SizeSquared() < CheckDistSq) )
			{
				bFoundNearbyPlayer = TRUE;
				break;
			}
		}
	}

	if ( !bFoundNearbyPlayer )
	{
		return;
	}

	// Agent-killing stuff
	UBOOL bKillAgent = FALSE;
	UBOOL bPlayDeathAnim = FALSE;

	// Query main collision octree for other agents or pawns nearby
	FMemMark Mark(GMainThreadMemStack);

	// Set up to check for possible encounter behaviors
	UBOOL bCheckForAgentEncounters = (WorldInfo->TimeSeconds - LastRenderTime < 0.1f) && !CurrentBehavior && (EncounterAgentBehaviors.Num() > 0);
	UBOOL bHavePotentialEncounter = FALSE;

	// Test location is out in front of agent
	FCheckResult* Link = GWorld->Hash->RestrictedOverlapCheck(GMainThreadMemStack, this, Location+0.5f*AwareRadius*Velocity.SafeNormal(), AwareRadius);
	for( FCheckResult* result=Link; result; result=result->GetNext())
	{
		checkSlow(result->Actor);

		// Look for nearby agents, find average velocity, and repel from them
		AGameCrowdAgent* FlockActor = Cast<AGameCrowdAgent>(result->Actor);
		if(FlockActor)
		{
			NearbyDynamics.AddItem(FlockActor);

			// add to the other agent's queue as well
			FlockActor->NearbyDynamics.AddUniqueItem(this);

			// trade viral behaviors
			if ( CurrentBehavior && CurrentBehavior->bIsViralBehavior )
			{
				if ( !FlockActor->CurrentBehavior || !FlockActor->CurrentBehavior->bIsViralBehavior )
				{
					CurrentBehavior->eventPropagateViralBehaviorTo(FlockActor);
				}
			}
			else if ( FlockActor->CurrentBehavior && FlockActor->CurrentBehavior->bIsViralBehavior )
			{
				FlockActor->CurrentBehavior->eventPropagateViralBehaviorTo(FlockActor);
			}

			// if don't have current behavior, and have EncounterAgentBehaviors, and other agent in front of me and facing me, then try initiating behavior
			FlockActor->bPotentialEncounter = bCheckForAgentEncounters && ((Rotation.Vector() | (FlockActor->Location - Location)) > 0.f) && ((FlockActor->Rotation.Vector() | (Location - FlockActor->Location)) > 0.f);
			bHavePotentialEncounter = bHavePotentialEncounter || FlockActor->bPotentialEncounter;
		}
		else if ( result->Actor->GetAPawn() )
		{
			NearbyDynamics.AddItem(result->Actor);
		}
		else if( result->Actor->GetAVolume() )
		{
			// If volume, look for a pain volume that would kill agent
			APhysicsVolume* PhysVol = Cast<APhysicsVolume>(result->Actor);
			if(PhysVol && PhysVol->bPainCausing)
			{
				// verify that am currently overlapping this pain volume (real radius + actual location)
				FRadiusOverlapCheck CheckInfo(Location,AvoidOtherRadius);
				if (CheckInfo.SphereBoundsTest(PhysVol->BrushComponent->Bounds))
				{
					// See if we want to destroy actor, or play the death anim
					if(PhysVol->bCrowdAgentsPlayDeathAnim)
					{
						bPlayDeathAnim = TRUE;
					}
					else
					{
						bKillAgent = TRUE;
					}
					break;
				}
			}
		}
	}
	Mark.Pop();

	// First handle dying cases
	if(bKillAgent)
	{
		eventKillAgent();
	}
	else if(bPlayDeathAnim)
	{
		PlayDeath(FVector(0.f));
	}
	else if ( bHavePotentialEncounter )
	{
		eventHandlePotentialAgentEncounter();
	}

	// FIXMESTEVE Now inform neighbors about each other, and delay their updates
}

/** 
 *	Add forces from relevant attractors/repulsors
 */
FVector AGameCrowdAgent::CheckRelevantAttractors()
{
	FVector AttractionForce(0.f);

	// Iterate over relevant attractors to update attraction/repulsion
	for(INT i=0; i<RelevantAttractors.Num(); i++)
	{
		AGameCrowdForcePoint* Attractor = RelevantAttractors(i);
		if( Attractor && Attractor->bIsEnabled )
		{
			// FIXMESTEVE - nativize for default implementation
			AttractionForce += Attractor->eventAppliedForce(this);
		}
	}

	return AttractionForce;
}

/** 
  * Calculate forces acting between crowd members, that is repulsion between crowd members, and force to match flock velocity 
  */
FVector AGameCrowdAgent::CalcIntraCrowdForce()
{
	FVector ResultForce(0.f);

	// Iterate over nearby agents and pawns to update velocity, repulsive forces etc
	FVector FlockVel(0,0,0);
	INT FlockCount = 0;

	for(INT i=0; i<NearbyDynamics.Num(); i++)
	{
		AActor* FlockActor = NearbyDynamics(i);
		if( FlockActor && !FlockActor->bDeleteMe && (FlockActor != this) )
		{
			//AGameCrowdAgent* OtherAgent = Cast<AGameCrowdAgent>(FlockActor);
		//FLOAT NetAvoidanceShare = OtherAgent ? AvoidanceShare/(AvoidanceShare + OtherAgent->AvoidanceShare) : 1.f;
			if ( FlockActor->GetAPawn() )
			{
				// if it isn't an agent, have to be more aggressive about avoidance, since other guy isn't avoiding me
				FVector ToFlockActor = FlockActor->Location - Location;
				FLOAT ToFlockActorMag = ToFlockActor.Size();
				FLOAT Overlap = AvoidOtherRadius - ToFlockActorMag;
			
				if( Square(2.f*AvoidOtherRadius) > ToFlockActor.SizeSquared() )
				{
					FLOAT ToFlockActorMag = ToFlockActor.Size();
					FLOAT Overlap = 2.f*AvoidOtherRadius - ToFlockActorMag;
					// normalize
					ToFlockActor /= ::Max(0.001f, ToFlockActorMag);

					ResultForce += ((Overlap/AvoidOtherRadius) * -ToFlockActor * (FlockActor->GetAPawn()->IsHumanControlled() ? AvoidPlayerStrength : AvoidOtherStrength));

					// FIXMESTEVE - also reduce based on velocity component that's toward this guy
				}
			}
			else
			{
				// USE RVO (http://gamma.cs.unc.edu/RVO/icra2008.pdf) for avoidance between agents,
				FlockVel += FlockActor->Velocity;

				FVector ToFlockActor = FlockActor->Location - Location;
				if( Square(AvoidOtherRadius) > ToFlockActor.SizeSquared() )
				{
					FLOAT ToFlockActorMag = ToFlockActor.Size();
					FLOAT Overlap = AvoidOtherRadius - ToFlockActorMag;
					// normalize
					ToFlockActor /= ::Max(0.001f, ToFlockActorMag);

					ResultForce += ((Overlap/AvoidOtherRadius) * -ToFlockActor * AvoidOtherStrength);
					// FIXMESTEVE - also reduce based on velocity component that's toward this guy
				}

				FlockCount++;
			}
		}
	}

	// keep crowd groups close together
	if ( MyGroup )
	{
		bWantsGroupIdle = FALSE;
		for ( INT i=0; i<MyGroup->Members.Num(); i++ )
		{
			if ( MyGroup->Members(i) && !MyGroup->Members(i)->bDeleteMe && ((MyGroup->Members(i)->Location - Location).SizeSquared() > DesiredGroupRadiusSq) )
			{
				ResultForce += GroupAttractionStrength * (MyGroup->Members(i)->Location - Location).SafeNormal();

				if ( ((MyGroup->Members(i)->Location - Location).SizeSquared() > 4.f*DesiredGroupRadiusSq) )
				{
					bWantsGroupIdle = ((Velocity | (MyGroup->Members(i)->Location - Location)) < 0.f)
										&& ((MyGroup->Members(i)->Velocity | (Location - MyGroup->Members(i)->Location)) > 0.f);
					if ( bWantsGroupIdle )
					{
						break;
					}
				}
			}
		}
	}

	// Average location and velocity for nearby agents.
	if(FlockCount > 0)
	{
		FlockVel /= (FLOAT)FlockCount;

		// Match velocity
		ResultForce += (FlockVel - Velocity) * MatchVelStrength;
	}

	return ResultForce;
}

/** 
  * If desired, take the current location, and using a line check, update its Z to match the ground better. 
  */
UBOOL AGameCrowdAgent::UpdateInterpZTranslation(const FVector& NewLocation)
{
	FVector LineStart = NewLocation + FVector(0.f,0.f,ConformTraceDist);
	FVector LineEnd = NewLocation - FVector(0.f,0.f,GroundOffset+ConformTraceDist);

	// Ground-conforming stuff
	UBOOL bHitGround = FALSE;
	FLOAT GroundHitTime = 1.f;
	FVector GroundHitLocation(0.f), GroundHitNormal(0.f);

	FMemMark LineMark(GMainThreadMemStack);
	if ( (ConformType == CFM_NavMesh) && NavigationHandle )
	{
		// trace against NavMesh
		FCheckResult Hit(1.f);
		NavigationHandle->StaticLineCheck(Hit,LineStart,LineEnd, FVector(0.f));
		if(Hit.Time < GroundHitTime)
		{
			bHitGround = TRUE;
			GroundHitTime = Hit.Time;
			GroundHitLocation = Hit.Location;
			GroundHitNormal = Hit.Normal;
		}
		else
		{
			//FIXMESTEVE - temp workaround for nav mesh trace issues when get too close to obstacle edge
			FCheckResult Result(1.f);
			GWorld->SingleLineCheck(Result, this, LineEnd, LineStart, TRACE_World, FVector(0.f));
			// If we hit something
			if(Result.Time < GroundHitTime)
			{
/*				if ( GroundHitTime == 1.f ) 
				{
					debugf(TEXT("SingleLineCheck hit when conform didn't"));
					DrawDebugLine(LineStart,LineEnd,255,0,0,TRUE);
				}
*/				bHitGround = TRUE;
				GroundHitTime = Result.Time;
				GroundHitLocation = Result.Location;
				GroundHitNormal = Hit.Normal;
			}
		}
		//DrawDebugLine(LineStart,LineEnd,255,0,0,0);
	}
	else
	{
		// Line trace down and look through results
		FCheckResult Result(1.f);
		GWorld->SingleLineCheck(Result, this, LineEnd, LineStart, TRACE_World, FVector(0.f));
		// If we hit something
		if(Result.Time < GroundHitTime)
		{
			bHitGround = TRUE;
			GroundHitTime = Result.Time;
			GroundHitLocation = Result.Location;
			GroundHitNormal = Result.Normal;
		}
	}
	LineMark.Pop();

	FLOAT TargetZ = NewLocation.Z;
	// If we hit something - move to that point
	if(bHitGround)
	{
		// If you end up embedded in the world - kill the crowd member
		if(GroundHitTime < KINDA_SMALL_NUMBER)
		{
			LifeSpan = -0.1f;
		}
		// Otherwise just position at end of line trace.
		else
		{
			TargetZ = GroundHitLocation.Z + GroundOffset;
		}
	}
	// If we didn't move to bottom of line check
	else
	{
		TargetZ = LineEnd.Z + GroundOffset;
	}

//	debugf(TEXT("%s Translate from %f to %f"), *GetName(), NewLocation.Z, TargetZ);
	InterpZTranslation = (TargetZ - NewLocation.Z)/((FLOAT)CurrentConformTraceInterval);
	
	// FIXMESTEVE TEMP DEBUG
	LastGroundZ = GroundHitNormal.Z;

	if ( Square(GroundHitNormal.Z) < 0.9f )
	{
		// Predict slope will continue, so interpolate ahead  FIXMESTEVE IMPROVE
		InterpZTranslation *= 1.5f;

		// update CurrentConformTraceInterval based on distance to player and whether currently on slope
		// FIXMESTEVE = use distance as well
		CurrentConformTraceInterval = ConformTraceInterval/3;
	}
	else
	{
		CurrentConformTraceInterval = ConformTraceInterval;
	}

	return TRUE;
}

/** 
  * Update force based on closest path - will push towards path if too far away, or push along path if close enough 
  */
FVector AGameCrowdAgent::CalcPathForce()
{
	// To target attraction
	FVector ToDestination(0.f); 
	FLOAT CurrentPathStrength = FollowPathStrength;
	if( CurrentDestination )
	{
		FLOAT DestinationDist = (IntermediatePoint - Location).Size2D();
		if ( DestinationDist > 0.f )
		{
			ToDestination = (IntermediatePoint - Location)/DestinationDist;
			ToDestination.Z = 0.f;

			if ( DestinationDist < AwareRadius )
			{
				// stronger force when near destination and not moving toward it
				CurrentPathStrength *= (2.f - (Velocity.SafeNormal() | ToDestination));
			}
		}
	}

	return ToDestination * CurrentPathStrength;
}

void UGameCrowdAgentBehavior::Tick(FLOAT DeltaTime) 
{
	if( ActionTarget )
	{
		// If desired, rotate pawn to look at target when performing action.
		FRotator ToTargetRot = (ActionTarget->Location - MyAgent->Location).Rotation();
		ToTargetRot.Pitch = 0;
		INT DeltaYaw = CalcDeltaYaw(ToTargetRot, MyAgent->Rotation);
		FRotator NewRotation = MyAgent->Rotation;
		FLOAT MaxTurn = DeltaTime * MyAgent->RotateToTargetSpeed;
		if ( Abs(DeltaYaw) < MaxTurn )
		{
			NewRotation.Yaw = ToTargetRot.Yaw;
		}
		else
		{
			if ( DeltaYaw < 0 )
			{
				MaxTurn *= -1.f;
			}		
			NewRotation.Yaw += appRound(MaxTurn);
		}
		MyAgent->SetRotation(NewRotation);

		if ( bFaceActionTargetFirst && (Abs(DeltaYaw) < 400) )
		{
			eventFinishedTargetRotation();
		}
	}

	// check to see if we are going to turn off the "viralness" of the current behavior
	if( bIsViralBehavior
		&& ( DurationOfViralBehaviorPropagation > 0.0f )
		&& ( TimeToStopPropagatingViralBehavior < GWorld->GetWorldInfo()->TimeSeconds) 
		)
	{
		//warnf( TEXT( "Stopping Viral Behavior %s" ), *MyAgent->GetFullName() );
		bIsViralBehavior = FALSE;
	}


}


void UGameCrowdBehavior_RunFromPanic::Tick(FLOAT DeltaTime) 
{
	Super::Tick( DeltaTime );

	// check to see if we are going to stop our panic behavior due to Duration running out
	if( ( MyAgent->bIsPanicked == TRUE ) 
		&& ( DurationOfPanic > 0.0f )
		&& ( TimeToStopPanic < GWorld->GetWorldInfo()->TimeSeconds)
		)
	{
		MyAgent->eventStopBehavior();
	}
}


/**
  * Set the "Out Agent" output of the current sequence to be MyAgent.
  */
void UGameCrowdBehavior_PlayAnimation::SetSequenceOutput()
{
	TArray<UObject**> AgentVars;
	AnimSequence->GetObjectVars(AgentVars,TEXT("Out Agent"));
	for (INT Idx = 0; Idx < AgentVars.Num(); Idx++)
	{
		*(AgentVars(Idx)) = MyAgent;
	}
}

/** 
  * Update position of an agent in a crowd, using properties in the action. 
  */
void AGameCrowdAgentSkeletal::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if ( bDeleteMe || !bAnimateThisTick )
	{
		return;
	}

	if ( !bUseRootMotionVelocity )
	{
		FLOAT VMag = Velocity.Size();

		// Blend between running and walking anim
		if(SpeedBlendNode)
		{
			FLOAT CurrentWeight = SpeedBlendNode->Child2Weight;

			FLOAT TargetWeight = ((VMag - SpeedBlendStart)/(SpeedBlendEnd - SpeedBlendStart));
			TargetWeight = ::Clamp<FLOAT>(TargetWeight, 0.f, 1.f);

			// limit how quickly anim rate can change
			FLOAT DeltaWeight = (TargetWeight - CurrentWeight);
			FLOAT MaxScaleChange = MaxSpeedBlendChangeSpeed * DeltaTime;
			DeltaWeight = Clamp(DeltaWeight, -MaxScaleChange, MaxScaleChange);

			SpeedBlendNode->SetBlendTarget(CurrentWeight + DeltaWeight, 0.f);
		}

		// Change anim rate based on speed
		if( AgentTree )
		{
			AgentTree->SetGroupRateScale(MoveSyncGroupName, VMag * AnimVelRate);
		}
	}
}

UBOOL AGameCrowdDestination::ReachedByAgent(AGameCrowdAgent *Agent, FVector TestPosition, UBOOL bTestExactly)
{
	return ((Location - TestPosition).SizeSquared2D() < Square((bTestExactly && bMustReachExactly) ? ExactReachTolerance : Agent->ReachThreshold*CylinderComponent->CollisionRadius)) && Abs(Location.Z - TestPosition.Z) < CylinderComponent->CollisionHeight + Agent->GroundOffset/2.f;
}

UBOOL AGameCrowdDestinationQueuePoint::QueueReachedBy(AGameCrowdAgent *Agent, FVector TestPosition)
{
	return ((Location - TestPosition).SizeSquared2D() < Square(CylinderComponent->CollisionRadius)) && Abs(Location.Z - TestPosition.Z) < CylinderComponent->CollisionHeight + Agent->GroundOffset/2.f;
}

UBOOL AGameCrowdAgent::ShouldEndIdle()
{
	return !CurrentBehavior || CurrentBehavior->ShouldEndIdle();
}

UBOOL UGameCrowdAgentBehavior::ShouldEndIdle()
{
	return !bIdleBehavior;
}

UBOOL UGameCrowdBehavior_WaitInQueue::ShouldEndIdle()
{
	if ( QueuePosition )
	{
		// if waiting in line and have reached queue position, wait till line moves
		bIdleBehavior = QueuePosition->QueueReachedBy(MyAgent, MyAgent->Location);
		return !bIdleBehavior;
	}
	else
	{
		debugf(TEXT("Queue behavior with no queue position!"));
		bIdleBehavior = FALSE;
		return TRUE;
	}
}

UBOOL UGameCrowdBehavior_WaitForGroup::ShouldEndIdle()
{
	UBOOL bShouldEndIdle = TRUE;

	// see if laggards have caught up or are no longer closing in.
	if ( MyAgent->MyGroup )
	{
		for ( INT i=0; i<MyAgent->MyGroup->Members.Num(); i++ )
		{
			if ( MyAgent->MyGroup->Members(i) && !MyAgent->MyGroup->Members(i)->bDeleteMe && ((MyAgent->MyGroup->Members(i)->Location - MyAgent->Location).SizeSquared() > MyAgent->DesiredGroupRadiusSq) 
				&& ((MyAgent->MyGroup->Members(i)->Velocity | (MyAgent->Location - MyAgent->MyGroup->Members(i)->Location)) > 0.f))
			{
				bShouldEndIdle = FALSE;
				break;
			}
		}
	}
	if ( bShouldEndIdle )
	{
		MyAgent->eventStopBehavior();
	}
	return bShouldEndIdle;
}

/** 
  *  Creates a new behavior object based on the passed in archetype ,
  *  and assigns it to be the agent's CurrentBehavior
  *
  * @PARAM BehaviorArchetype is the archetype for the new behavior
  */
void AGameCrowdAgent::SetCurrentBehavior(UGameCrowdAgentBehavior* BehaviorArchetype)
{
	CurrentBehavior = ConstructObject<UGameCrowdAgentBehavior>( BehaviorArchetype->GetClass(), this, NAME_None, 0, BehaviorArchetype );
}

/**
  * Check if reached intermediate point in route to destination
  */
UBOOL AGameCrowdAgent::ReachedIntermediatePoint()
{
	// check if close enough
	FVector Dist = Location - IntermediatePoint;
	if ( Abs(Dist.Z) < 2.f*SearchExtent.Z )
	{
		Dist.Z = 0.f;
		if ( Dist.SizeSquared() < Square(2.f*SearchExtent.X) )
		{
			return TRUE;
		}
	}

	if ( NavigationHandle && NavigationHandle->CurrentEdge )
	{
		// if following navmesh path, see if on destination poly
		// FIXMESTEVE - move to navigationhandle after discussin with MattT
		if ( NavigationHandle->AnchorPoly )
		{
			FNavMeshPolyBase* OtherPoly = NavigationHandle->CurrentEdge->GetOtherPoly(NavigationHandle->AnchorPoly); 
			if ( OtherPoly->ContainsPoint(Location,TRUE) )
			{
				//debugf(TEXT("%s on destination poly at %f"), *GetName(), WorldInfo->TimeSeconds);
				return TRUE;
			}
		}
/* FIXMESTEVE - maybe to play with later, or remove
		// see if somewhere on current edge
		FVector Closest(0.f);
		FLOAT DistFromEdge = PointDistToSegment(Location,NavigationHandle->CurrentEdge->GetVertLocation(0),NavigationHandle->CurrentEdge->GetVertLocation(1),Closest );
		if ( DistFromEdge < 2.f*SearchExtent.X + 2.f*SearchExtent.Z )
		{
			FVector Dir = Closest - Location;
			Dir.Z = 0.f;
			if ( Dir.SizeSquared() < Square(SearchExtent.X) )
			{
				debugf(TEXT("%s on destination EDGE at %f"), *GetName(), WorldInfo->TimeSeconds);
				return TRUE;
			}
		}
*/
	}
	return FALSE;
}

/**
  * Handles movement destination updating for agent.
  *
  * @RETURNS true if destination updating was handled
  */ 
UBOOL UGameCrowdAgentBehavior::HandleMovement()
{
	return FALSE;
}

/**
  * Handles movement destination updating for agent.
  *
  * @RETURNS true if destination updating was handled
  */ 
UBOOL UGameCrowdBehavior_WaitInQueue::HandleMovement()
{
	if (QueuePosition )
	{
		// moving toward QueuePosition
		if ( QueuePosition->QueueReachedBy(MyAgent, MyAgent->Location) )
		{
			QueuePosition->eventReachedDestination(MyAgent);
		}
		else if ( (MyAgent->IntermediatePoint != QueuePosition->Location) && MyAgent->ReachedIntermediatePoint() )
		{
			MyAgent->eventUpdateIntermediatePoint(QueuePosition);
		}
	}
	return TRUE;
}

/** Update position of an 'agent' in a crowd, using properties in the action. */
void AGameCrowdAgent::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if ( bDeleteMe || !bSimulateThisTick )
	{
		return;
	}

	if ( CurrentDestination )
	{
		// FIXMESTEVE - merge IsIdle() and HandleMovement() here?
		if ( IsIdle() )
		{
			if( ShouldEndIdle() )
			{
				if ( CurrentDestination->ReachedByAgent(this, Location, FALSE) )
				{
					CurrentDestination->eventReachedDestination(this);
				}
			}
		}
		else if ( !CurrentBehavior || !CurrentBehavior->HandleMovement() )
		{
			if ( bWantsGroupIdle && !CurrentBehavior && (GroupWaitingBehaviors.Num() > 0) )
			{
				// Too far ahead of group, wait if necessary
				eventWaitForGroupMembers();
			}
			// check if have reached destination, if so perform appropriate action
			else if ( CurrentDestination->ReachedByAgent(this, Location, TRUE) )
			{
				CurrentDestination->eventReachedDestination(this);
			}
			else if ( (IntermediatePoint != CurrentDestination->Location) && ReachedIntermediatePoint() )
			{
				eventUpdateIntermediatePoint(CurrentDestination);
			}
		}
	}

	// Update aware check frame count and see if its time to update proximity info.
	AwareUpdateFrameCount++;
	if(AwareUpdateFrameCount >= AwareUpdateInterval)
	{
		AwareUpdateFrameCount = 0;
		UpdateProximityInfo();

		// check if can see local player
		if ( bWantsSeePlayerNotification && (WorldInfo->TimeSeconds - LastRenderTime < 0.1f) )
		{
			for( INT Idx = 0; Idx < GEngine->GamePlayers.Num(); ++Idx )   
			{  
				if (GEngine->GamePlayers(Idx) &&  
					GEngine->GamePlayers(Idx)->Actor &&  
					GEngine->GamePlayers(Idx)->Actor->IsLocalPlayerController())  
				{  
					APlayerController* PC = GEngine->GamePlayers(Idx)->Actor;  

					// if facing player, do a trace to verify line of sight
					if( PC->Pawn && ((PC->Pawn->Location - Location).SizeSquared() < MaxSeePlayerDistSq) && (((PC->Pawn->Location - Location) | Rotation.Vector()) > 0.f) )  
					{  
						FCheckResult Hit(1.f);
						GWorld->SingleLineCheck( Hit, this, PC->Pawn->Location + FVector(0.f, 0.f, PC->Pawn->BaseEyeHeight), Location + FVector(0.f,0.f,EyeZOffset), TRACE_World|TRACE_StopAtAnyHit|TRACE_ComplexCollision );
						if ( Hit.Time == 1.f )
						{
							eventNotifySeePlayer(PC);
							break;
						}
					}  
				}
			}
		}
	}

	// tick behavior
	if ( CurrentBehavior )
	{
		CurrentBehavior->Tick(DeltaTime);
	}
}

#if WITH_EDITOR
void AGameCrowdInteractionPoint::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * 500.0f;

	if ( bCtrlDown )
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		CylinderComponent->CollisionHeight += ModifiedScale.X;
		CylinderComponent->CollisionHeight = Max( 0.0f, CylinderComponent->CollisionHeight );
	}
	else
	{
		CylinderComponent->CollisionRadius += ModifiedScale.X;
		CylinderComponent->CollisionRadius = Max( 0.0f, CylinderComponent->CollisionRadius );

		// If non-uniformly scaling, Z scale affects height and Y can affect radius too.
		if ( !ModifiedScale.AllComponentsEqual() )
		{
			CylinderComponent->CollisionHeight += -ModifiedScale.Z;
			CylinderComponent->CollisionHeight = Max( 0.0f, CylinderComponent->CollisionHeight );

			CylinderComponent->CollisionRadius += ModifiedScale.Y;
			CylinderComponent->CollisionRadius = Max( 0.0f, CylinderComponent->CollisionRadius );
		}
	}
}
#endif

void AGameCrowdForcePoint::TickSpecial( FLOAT DeltaSeconds )
{
	SCOPE_CYCLE_COUNTER(STAT_CrowdTotal);
	SCOPE_CYCLE_COUNTER(STAT_CrowdForce);

	Super::TickSpecial(DeltaSeconds);

	// check for overlapping crowd agents
	if ( bIsEnabled )
	{
		//FindTouchingActors();
		FMemMark Mark(GMainThreadMemStack);
		
		TLookupMap<AActor*> NewTouching;
		
		FLOAT ColRadius, ColHeight;
		GetBoundingCylinder(ColRadius, ColHeight);
		UBOOL bIsZeroExtent = (ColRadius == 0.f) && (ColHeight == 0.f);
		FCheckResult* FirstHit = GWorld->Hash ? GWorld->Hash->ActorEncroachmentCheck( GMainThreadMemStack, this, Location, Rotation, TRACE_AllColliding ) : NULL;	
		for( FCheckResult* Test = FirstHit; Test; Test=Test->GetNext() )
			if(	Test->Actor!=this && !Test->Actor->IsBasedOn(this) && Test->Actor != GWorld->GetWorldInfo() )
			{
				if( !IsBlockedBy(Test->Actor,Test->Component)
					&& (!Test->Component || (bIsZeroExtent ? Test->Component->BlockZeroExtent : Test->Component->BlockNonZeroExtent)) )
				{
					// Make sure Test->Location is not Zero, if that's the case, use Location
					FVector	HitLocation = Test->Location.IsZero() ? Location : Test->Location;

					// Make sure we have a valid Normal
					FVector NormalDir = Test->Normal.IsZero() ? (Location - HitLocation) : Test->Normal;
					if( !NormalDir.IsZero() )
					{
						NormalDir.Normalize();
					}
					else
					{
						NormalDir = FVector(0,0,1.f);
					}

					NewTouching.AddItem(Test->Actor);
					BeginTouch( Test->Actor, Test->Component, HitLocation, NormalDir, Test->SourceComponent );
				}
			}						

		for(INT Idx=0;Idx<Touching.Num();)
		{
			if(Touching(Idx) && !NewTouching.Find(Touching(Idx)))		
			{
				EndTouch(Touching(Idx),0);
			}
			else
			{
				Idx++;
			}
		}

		Mark.Pop();	
	}
}

/** 
  * Special version of IsOverlapping to handle touching crowd agents
  */
UBOOL AGameCrowdForcePoint::IsOverlapping( AActor* Other, FCheckResult* Hit, UPrimitiveComponent* OtherPrimitiveComponent, UPrimitiveComponent* MyPrimitiveComponent )
{
	checkSlow(Other!=NULL);

	SCOPE_CYCLE_COUNTER(STAT_CrowdForce);
	if ( !bCollideActors || !CollisionComponent || !CollisionComponent->CollideActors )
	{
		return FALSE;
	}

	// FIXMESTEVE - not just for skeletal
	AGameCrowdAgentSkeletal* Agent = Cast<AGameCrowdAgentSkeletal>(Other);

	if ( !Agent || !Agent->SkeletalMeshComponent )
	{
		return Super::IsOverlapping(Other, Hit, OtherPrimitiveComponent, MyPrimitiveComponent);
	}

	// Can't overlap actors with collision turned off.
	if( !Agent->bCollideActors )
	{
		return FALSE;
	}

	// Now have 2 actors we want to overlap, so we have to pick which one to treat as a box and test against the PrimitiveComponents of the other.
	AActor* PrimitiveActor = this;
	AActor* BoxActor = Other;

	// If we were not supplied an FCheckResult, use a temp one.
	FCheckResult TestHit;
	if(Hit==NULL)
	{
		Hit = &TestHit;
	}

	// Check bounding box of collision component against each colliding PrimitiveComponent.
	FBox BoxActorBox = Agent->SkeletalMeshComponent->Bounds.GetBox();
	if(BoxActorBox.IsValid)
	{
		// adjust box position since overlap check is for different location than actor's current location
		BoxActorBox.Min -= OverlapAdjust;
		BoxActorBox.Max -= OverlapAdjust;
	}

	// If we failed to get a valid bounding box from the Box actor, we can't get an overlap.
	if(!BoxActorBox.IsValid)
	{
		return FALSE;
	}

	FVector BoxCenter, BoxExtent;
	BoxActorBox.GetCenterAndExtents(BoxCenter, BoxExtent);

	// DEBUGGING: Time how long the point checks take, and print if its more than PERF_SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT.
#if defined(PERF_SHOW_SLOW_OVERLAPS) || LOOKING_FOR_PERF_ISSUES
	DWORD Time=0;
	CLOCK_CYCLES(Time);
#endif

	// Check agent box against my collisioncomponent.
	if( CollisionComponent->PointCheck(*Hit, BoxCenter, BoxExtent, 0) == 0 )
	{
		Hit->Component = CollisionComponent;
	}

#if defined(PERF_SHOW_SLOW_OVERLAPS) || LOOKING_FOR_PERF_ISSUES
	const static FLOAT PERF_SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT = 0.05f; // modify this value to look at larger or smaller sets of "bad" actors
	UNCLOCK_CYCLES(Time);
	FLOAT MSec = Time * GSecondsPerCycle * 1000.f;
	if( MSec > PERF_SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT )
	{
		debugf( NAME_PerfWarning, TEXT("IsOverLapping: Testing: P:%s - B:%s Time: %f"), *(PrimitiveActor->GetPathName()), *(BoxActor->GetPathName()), MSec );
	}
#endif


	return Hit->Component != NULL;
}


UBOOL USeqEvent_CrowdAgentReachedDestination::CheckActivate(AActor *InOriginator, AActor *InInstigator, UBOOL bTest, TArray<INT>* ActivateIndices, UBOOL bPushTop)
{
	UBOOL bResult = Super::CheckActivate(InOriginator, InInstigator, bTest, ActivateIndices, bPushTop);

	if( bResult && !bTest )
	{
		AGameCrowdAgent* Agent = Cast<AGameCrowdAgent>(InInstigator);
		if( Agent )
		{
			// see if any Agent variables are attached
			TArray<UObject**> AgentVars;
			GetObjectVars(AgentVars,TEXT("Agent"));
			for (INT Idx = 0; Idx < AgentVars.Num(); Idx++)
			{
				*(AgentVars(Idx)) = Agent;
			}
		}
	}

	return bResult;
}

void USeqAct_PlayAgentAnimation::Activated()
{
	// START
	if( InputLinks(0).bHasImpulse )
	{
		// cache ActionTarget
		TArray<UObject**> Objs;
		GetObjectVars(Objs,TEXT("Action Focus"));
		for(INT Idx = 0; Idx < Objs.Num(); Idx++)
		{
			AActor* TestActor = Cast<AActor>( *(Objs(Idx)) );
			if (TestActor != NULL)
			{
				// use the pawn instead of the controller
				if ( TestActor->GetAController() && TestActor->GetAController()->Pawn )
				{
					TestActor = TestActor->GetAController()->Pawn;
				}

				ActionTarget = TestActor;
				break;
			}
		}
	}

	Super::Activated();
}

UBOOL USeqAct_PlayAgentAnimation::UpdateOp(FLOAT DeltaTime)
{
	// call the script handler if we catch a stop while active
	if (InputLinks.Num() > 1 && InputLinks(1).bHasImpulse)
	{
		USequenceAction::Activated();
	}
	return Super::UpdateOp(DeltaTime);
}

/** AGameCrowdPopulationManager IMPLEMENT Interface_NavigationHandle */
void AGameCrowdPopulationManager::SetupPathfindingParams( FNavMeshPathParams& out_ParamCache )
{
	VERIFY_NAVMESH_PARAMS(9)
	if(QueryingAgent != NULL)
	{
		out_ParamCache.bAbleToSearch = TRUE;
		out_ParamCache.SearchExtent = QueryingAgent->SearchExtent;
		out_ParamCache.SearchStart = QueryingAgent->Location;
	}
	else
	{
		out_ParamCache.bAbleToSearch = FALSE;
		out_ParamCache.SearchExtent = FVector(0.f);
		out_ParamCache.SearchStart = Location;
	}
	out_ParamCache.SearchLaneMultiplier = 0.f;
	out_ParamCache.bCanMantle = FALSE;
	out_ParamCache.bNeedsMantleValidityTest = FALSE;
	out_ParamCache.MaxDropHeight = 0.f;
	out_ParamCache.MinWalkableZ = 0.7f;
	out_ParamCache.MaxHoverDistance = 0.f;


}

FVector AGameCrowdPopulationManager::GetEdgeZAdjust(FNavMeshEdgeBase* Edge)
{
	if(QueryingAgent != NULL)
	{
		return FVector(0.f,0.f,QueryingAgent->SearchExtent.Z);
	}
	else
	{
		return FVector(0.f);
	}
}

void AGameCrowdAgent::SetupPathfindingParams( FNavMeshPathParams& out_ParamCache )
{
	VERIFY_NAVMESH_PARAMS(9)
	out_ParamCache.bAbleToSearch = TRUE;
	out_ParamCache.SearchExtent = SearchExtent;
	out_ParamCache.SearchLaneMultiplier = 0.f;
	out_ParamCache.SearchStart = Location;
	out_ParamCache.bCanMantle = FALSE;
	out_ParamCache.bNeedsMantleValidityTest = FALSE;
	out_ParamCache.MaxDropHeight = 0.f;
	out_ParamCache.MinWalkableZ = 0.7f;
	out_ParamCache.MaxHoverDistance = 0.f;
}

FVector AGameCrowdAgent::GetEdgeZAdjust(FNavMeshEdgeBase* Edge)
{
	return FVector(0.f,0.f,SearchExtent.Z);
}

UBOOL AGameCrowdAgent::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	SCOPE_CYCLE_COUNTER(STAT_CrowdTotal);
	SCOPE_CYCLE_COUNTER(STAT_AgentTick);
	bSimulateThisTick = ShouldPerformCrowdSimulation(DeltaSeconds);
	return Super::Tick(DeltaSeconds, TickType);
}

UBOOL AGameCrowdAgentSkeletal::ShouldPerformCrowdSimulation(FLOAT DeltaTime) 
{
	UBOOL bResult = Super::ShouldPerformCrowdSimulation(DeltaTime);
	bAnimateThisTick = bResult && (WorldInfo->TimeSeconds - LastRenderTime < 0.2f);
	if ( bAnimateThisTick )
	{
		FVector PlayerLocation(0.f);
		FRotator PlayerRotation(0,0,0);
		for( INT Idx = 0; Idx < GEngine->GamePlayers.Num(); ++Idx )   
		{
			if (GEngine->GamePlayers(Idx) &&  
				GEngine->GamePlayers(Idx)->Actor &&  
				GEngine->GamePlayers(Idx)->Actor->IsLocalPlayerController())  
			{
				GEngine->GamePlayers(Idx)->Actor->eventGetPlayerViewPoint(PlayerLocation, PlayerRotation);
				break;
			}
		}

		// Don't animate if far away from viewer(s)
		bAnimateThisTick = ((Location - PlayerLocation).SizeSquared() < MaxAnimationDistanceSq);
	}

	// turn off skeleton as well, unless ragdoll or playing death anim
	if ( SkeletalMeshComponent && (Physics != PHYS_RigidBody) && !bIsPlayingDeathAnimation )
	{
		SkeletalMeshComponent->bNoSkeletonUpdate = !bAnimateThisTick;
	}
	return bResult;
}

/** 
 * This will allow subclasses to implement specialized behavior for whether or not to actually simulate. 
 * Base behavior is to reduce update frequency based on how long since an agent has been visible.
 **/
UBOOL AGameCrowdAgent::ShouldPerformCrowdSimulation(FLOAT DeltaTime) 
{
	if ( !GWorld->bDisableCrowds )
	{
		// don't perform simulation if dead crowd member
		if ( Health < 0 )
		{
			return FALSE;
		}
		ForceUpdateTime = ::Max(LastRenderTime, ForceUpdateTime);

		// reduce update rate if agent is not visible
		if ( WorldInfo->TimeSeconds - LastUpdateTime > NotVisibleTickScalingFactor * (WorldInfo->TimeSeconds - ForceUpdateTime) * 30.f * DeltaTime )
		{
			LastUpdateTime = WorldInfo->TimeSeconds;
			return TRUE;
		}
		if ( WorldInfo->TimeSeconds - ForceUpdateTime > NotVisibleLifeSpan )
		{
			// don't destroy if still in line of sight and not too far away
			FVector PlayerLocation(0.f);
			FRotator PlayerRotation(0,0,0);
			UBOOL bLineOfSight = FALSE;
			for( INT Idx = 0; Idx < GEngine->GamePlayers.Num(); ++Idx )   
			{
				if (GEngine->GamePlayers(Idx) &&  
					GEngine->GamePlayers(Idx)->Actor &&  
					GEngine->GamePlayers(Idx)->Actor->IsLocalPlayerController())  
				{
					GEngine->GamePlayers(Idx)->Actor->eventGetPlayerViewPoint(PlayerLocation, PlayerRotation);
					if ( (Location - PlayerLocation).SizeSquared() < MaxLOSLifeDistanceSq )
					{
						// if heading for visible or soon to be visible destination, and not too far away, keep alive
						if ( CurrentDestination && (CurrentDestination->bIsVisible || CurrentDestination->bWillBeVisible) )
						{
							ForceUpdateTime = WorldInfo->TimeSeconds;
							bLineOfSight = TRUE;
							break;
						}
						else 
						{
							// check if line of sight
							FCheckResult Hit(1.f);
							GWorld->SingleLineCheck(Hit, this, Location, PlayerLocation, TRACE_World | TRACE_StopAtAnyHit);
							if ( !Hit.Actor )
							{
								bLineOfSight = TRUE;
								ForceUpdateTime = WorldInfo->TimeSeconds + 3.f;
								break;
							}
						}
					}
				}
			}

			// if not in L.O.S. and not visible for a while, kill agent
			if ( !bLineOfSight )
			{
				eventKillAgent();
			}
		}
	}
	return FALSE;
}

//=============================================================================
// FConnectionRenderingSceneProxy

/** Represents a GameDestinationConnRenderingComponent to the scene manager. */
class FConnectionRenderingSceneProxy : public FDebugRenderSceneProxy
{
public:

	FConnectionRenderingSceneProxy(const UGameDestinationConnRenderingComponent* InComponent):
	FDebugRenderSceneProxy(InComponent)
	{
		// only on selected GameCrowdDestination
			// draw destination connections
			AGameCrowdDestination* Dest = Cast<AGameCrowdDestination>(InComponent->GetOwner());

			if( Dest )
			{
				if( Dest->NextDestinations.Num() > 0 )
				{
					for (INT Idx = 0; Idx < Dest->NextDestinations.Num(); Idx++)
					{
						AGameCrowdDestination* NextDest = Dest->NextDestinations(Idx);
						if( NextDest )
						{
							FLinearColor Color = FLinearColor(1.f,1.f,0.f,1.f);
							new(Lines) FDebugRenderSceneProxy::FDebugLine(Dest->Location, NextDest->Location,Color);
						}
					}
				}
				AActor* Prev = Dest;
				for ( AGameCrowdDestinationQueuePoint* QueuePoint=Dest->QueueHead; QueuePoint!=NULL; QueuePoint=QueuePoint->NextQueuePosition )
				{
					FLinearColor Color = FLinearColor(1.f,0.3f,1.f,1.f);
					new(Lines) FDebugRenderSceneProxy::FDebugLine(Prev->Location, QueuePoint->Location,Color);
					Prev = QueuePoint;
				}
			}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDynamicRelevance = IsShown(View) && bSelected;
		Result.SetDPG(SDPG_World,TRUE);
		if (IsShadowCast(View))
		{
			Result.bShadowRelevance = TRUE;
		}
		return Result;
	}

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const { return( FDebugRenderSceneProxy::GetAllocatedSize() ); }
};


//=============================================================================
// UGameDestinationConnRenderingComponent

/**
 * Creates a new scene proxy for the path rendering component.
 * @return	Pointer to the FPathRenderingSceneProxy
 */
FPrimitiveSceneProxy* UGameDestinationConnRenderingComponent::CreateSceneProxy()
{
	return new FConnectionRenderingSceneProxy(this);
}

void UGameDestinationConnRenderingComponent::UpdateBounds()
{
	FBox BoundingBox(0);

	AGameCrowdDestination* Dest = Cast<AGameCrowdDestination>(Owner);
	if ( Dest )
	{
		if ( Dest->NextDestinations.Num() > 0)
		{
			for (INT Idx = 0; Idx < Dest->NextDestinations.Num(); Idx++)
			{
				AGameCrowdDestination* End = Dest->NextDestinations(Idx);
				if( End )
				{
					BoundingBox += Dest->Location;
					BoundingBox += End->Location;
				}
			}
		}
		for ( AGameCrowdDestinationQueuePoint* QueuePoint=Dest->QueueHead; QueuePoint!=NULL; QueuePoint=QueuePoint->NextQueuePosition )
		{
			BoundingBox += Dest->Location;
			BoundingBox += QueuePoint->Location;
		}
	}
	Bounds = FBoxSphereBounds(BoundingBox);
}

/**
Hook to allow actors to render HUD overlays for themselves.
Assumes that appropriate font has already been set
*/
void AGameCrowdAgent::NativePostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir)
{
	ULocalPlayer *MyLocalPlayer = PC ? Cast<ULocalPlayer>(PC->Player) : NULL;
	if ( MyLocalPlayer 
		&& MyLocalPlayer->GetActorVisibility(this)
		&& ((CameraDir | (Location - CameraPosition)) > 0.f) 
		&& ((CameraPosition - Location).SizeSquared() * Square(PC->LODDistanceFactor) < Square(BeaconMaxDist)) )
	{
		eventPostRenderFor(PC, Canvas, CameraPosition, CameraDir);
	}
}

/** 
  * EditorLinkSelectionInterface 
  */
void AGameCrowdDestination::LinkSelection(USelection* SelectedActors)
{
	UBOOL bFoundOtherDest = FALSE;
	for( INT SelectedIdx=0; SelectedIdx<SelectedActors->Num(); SelectedIdx++ )
	{
		AGameCrowdDestination* CurDest = Cast<AGameCrowdDestination>((*SelectedActors)(SelectedIdx));
		if( CurDest )
		{
			if ( CurDest != this )
			{
				// if not in the list yet, add it
				NextDestinations.AddUniqueItem(CurDest);
				bFoundOtherDest = TRUE;
			}
		}
	}

	if ( !bFoundOtherDest )
	{
		// connect any queue points to this destination
		for( INT SelectedIdx=0; SelectedIdx<SelectedActors->Num(); SelectedIdx++ )
		{
			AGameCrowdDestinationQueuePoint* NewQueuePt = Cast<AGameCrowdDestinationQueuePoint>((*SelectedActors)(SelectedIdx));
			if( NewQueuePt )
			{
				// check if already in the list
				UBOOL bFoundInList = FALSE;

				if ( NewQueuePt->NextQueuePosition )
				{
					//already part of a list
					bFoundInList = TRUE;
				}
				else
				{
					for ( AGameCrowdDestinationQueuePoint* QueuePt=QueueHead; QueuePt!=NULL; QueuePt=QueuePt->NextQueuePosition )
					{
						if ( QueuePt == NewQueuePt )
						{
							bFoundInList = TRUE;
							break;
						}
					}
				}
				// NOTE: if if QueuePt is in another list as tail  (this could also be done by hand changing NextQueuePosition)
				// This is checked for in Check For Errors and when actually using a Queue

				// not in list, so insert it at appropriate position, based on distance
				if ( !bFoundInList )
				{
					if ( !QueueHead )
					{
						QueueHead = NewQueuePt;
					}
					else if ( (Location - NewQueuePt->Location).SizeSquared() < (Location - QueueHead->Location).SizeSquared() )
					{
						NewQueuePt->NextQueuePosition = QueueHead;
						QueueHead = NewQueuePt;
					}
					else
					{
						for ( AGameCrowdDestinationQueuePoint* QueuePt=QueueHead; QueuePt!=NULL; QueuePt=QueuePt->NextQueuePosition )
						{
							if ( QueuePt->NextQueuePosition )
							{
								if ( (Location - NewQueuePt->Location).SizeSquared() < (Location - QueuePt->NextQueuePosition->Location).SizeSquared() )
								{
									NewQueuePt->NextQueuePosition = QueuePt->NextQueuePosition;
									QueuePt->NextQueuePosition = NewQueuePt;
								}											
							}
							else
							{
								QueuePt->NextQueuePosition = NewQueuePt;
								break;
							}
						}
					}
				}
			}
		}
	}

	// update drawn connections
	UGameDestinationConnRenderingComponent *Comp = NULL;
	if ( Components.FindItemByClass<UGameDestinationConnRenderingComponent>(&Comp) )
	{
		FComponentReattachContext Context(Comp);
	}	
}

/** 
  * EditorLinkSelectionInterface 
  */
void AGameCrowdDestination::UnLinkSelection(USelection* SelectedActors)
{
	UBOOL bFoundOtherDest = FALSE;
	for( INT SelectedIdx=0; SelectedIdx<SelectedActors->Num(); SelectedIdx++ )
	{
		AGameCrowdDestination* CurDest = Cast<AGameCrowdDestination>((*SelectedActors)(SelectedIdx));
		if( CurDest )
		{
			if ( CurDest != this )
			{
				// if not in the list yet, add it
				NextDestinations.RemoveItem(CurDest);
				bFoundOtherDest = TRUE;
			}
		}
	}

	// update drawn connections
	UGameDestinationConnRenderingComponent *Comp = NULL;
	if ( Components.FindItemByClass<UGameDestinationConnRenderingComponent>(&Comp) )
	{
		FComponentReattachContext Context(Comp);
	}	
}

#if WITH_EDITOR
void AGameCrowdDestination::CheckForErrors()
{
	// make sure this destination doesn't have a queue with shared queue points
	for ( AGameCrowdDestinationQueuePoint* QueuePt=QueueHead; QueuePt!=NULL; QueuePt=QueuePt->NextQueuePosition )
	{
		if ( QueuePt->QueueDestination && (QueuePt->QueueDestination != this) )
		{
			// maybe shared - check if still attached to other destination
			UBOOL bFoundShared = FALSE;
			for ( AGameCrowdDestinationQueuePoint* SharedQueuePt=QueuePt->QueueDestination->QueueHead; SharedQueuePt!=NULL; SharedQueuePt=SharedQueuePt->NextQueuePosition )
			{
				if ( SharedQueuePt == QueuePt )
				{
					bFoundShared = TRUE;
					break;
				}
			}
			if ( bFoundShared )
			{
				GWarn->MapCheck_Add( MCTYPE_ERROR, this, *FString::Printf(TEXT("Shares a QueuePoint %s with %s"), *QueuePt->GetName(), *QueuePt->QueueDestination->GetName()) );
			}
		}
		QueuePt->QueueDestination = this;
	}
}
#endif


FString AGameCrowdAgent::GetDetailedInfoInternal() const
{
	FString Result;  

	if( MyArchetype != NULL )
	{
		Result = MyArchetype->GetName();
	}
	else
	{
		Result = TEXT("No_MyArchetype");
	}

	return Result;  
}

/**
  *  Find pop mgr to target before activating
  */
void USeqAct_GameCrowdPopulationManagerToggle::Activated()
{
	// find target, if not already found
	if ( Targets.Num() == 0 )
	{
		eventFindPopMgrTarget();
	}

	Super::Activated();
}

/**
  *  Implementation just for stats gathering
  */
UBOOL AGameCrowdPopulationManager::Tick( FLOAT DeltaTime, enum ELevelTick TickType )
{
	SCOPE_CYCLE_COUNTER(STAT_CrowdTotal);
	SCOPE_CYCLE_COUNTER(STAT_CrowdPopMgr);
	return Super::Tick(DeltaTime, TickType);
}
