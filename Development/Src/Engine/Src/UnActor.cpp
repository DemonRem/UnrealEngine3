/*=============================================================================
	UnActor.cpp: AActor implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"
#include "EngineAIClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "UnStatChart.h"
#include "BSPOps.h"

/**
 * PERF_ISSUE_FINDER
 *
 * Point checks should not take an excessively long time as we do lots of them
 *
 * Turn his on to have the engine log when a specific actor is taking longer than 
 * SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT to do its IsOverlapping check.
 *
 **/
//#define SHOW_SLOW_OVERLAPS 1
const static FLOAT SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT = 1.0f; // modify this value to look at larger or smaller sets of "bad" actors



/*-----------------------------------------------------------------------------
	AActor object implementations.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(AActor);
IMPLEMENT_CLASS(ALight);
IMPLEMENT_CLASS(ADirectionalLight);
IMPLEMENT_CLASS(ADirectionalLightToggleable);
IMPLEMENT_CLASS(APointLight);
IMPLEMENT_CLASS(APointLightMovable);
IMPLEMENT_CLASS(APointLightToggleable);
IMPLEMENT_CLASS(ASpotLight);
IMPLEMENT_CLASS(ASpotLightMovable);
IMPLEMENT_CLASS(ASpotLightToggleable);
IMPLEMENT_CLASS(UCameraMode);
IMPLEMENT_CLASS(AClipMarker);
IMPLEMENT_CLASS(APolyMarker);
IMPLEMENT_CLASS(AWeapon);
IMPLEMENT_CLASS(ANote);
IMPLEMENT_CLASS(AWorldInfo);
IMPLEMENT_CLASS(AGameInfo);
IMPLEMENT_CLASS(AZoneInfo);
IMPLEMENT_CLASS(UReachSpec);
IMPLEMENT_CLASS(UForcedReachSpec);
IMPLEMENT_CLASS(UAdvancedReachSpec);
IMPLEMENT_CLASS(ULadderReachSpec);
IMPLEMENT_CLASS(UProscribedReachSpec);
IMPLEMENT_CLASS(USlotToSlotReachSpec);
IMPLEMENT_CLASS(UMantleReachSpec);
IMPLEMENT_CLASS(USwatTurnReachSpec);
IMPLEMENT_CLASS(UCoverSlipReachSpec);
IMPLEMENT_CLASS(UWallTransReachSpec);
IMPLEMENT_CLASS(UFloorToCeilingReachSpec);
IMPLEMENT_CLASS(UCeilingReachSpec);
IMPLEMENT_CLASS(APathNode);
IMPLEMENT_CLASS(ANavigationPoint);
IMPLEMENT_CLASS(AScout);
IMPLEMENT_CLASS(AProjectile);
IMPLEMENT_CLASS(ATeleporter);
IMPLEMENT_CLASS(APlayerStart);
IMPLEMENT_CLASS(AKeypoint);
IMPLEMENT_CLASS(AInventory);
IMPLEMENT_CLASS(AInventoryManager);
IMPLEMENT_CLASS(APickupFactory);
IMPLEMENT_CLASS(ATrigger);
IMPLEMENT_CLASS(AHUD);
IMPLEMENT_CLASS(USavedMove);
IMPLEMENT_CLASS(AInfo);
IMPLEMENT_CLASS(AReplicationInfo);
IMPLEMENT_CLASS(APlayerReplicationInfo);
IMPLEMENT_CLASS(AInternetInfo);
IMPLEMENT_CLASS(AGameReplicationInfo);
IMPLEMENT_CLASS(ADroppedPickup);
IMPLEMENT_CLASS(AController);
IMPLEMENT_CLASS(AAIController);
IMPLEMENT_CLASS(APlayerController);
IMPLEMENT_CLASS(AMutator);
IMPLEMENT_CLASS(AVehicle);
IMPLEMENT_CLASS(ALadder);
IMPLEMENT_CLASS(UDamageType);
IMPLEMENT_CLASS(UKillZDamageType);
IMPLEMENT_CLASS(ABrush);
IMPLEMENT_CLASS(AVolume);
IMPLEMENT_CLASS(APhysicsVolume);
IMPLEMENT_CLASS(AGravityVolume);
IMPLEMENT_CLASS(ADefaultPhysicsVolume);
IMPLEMENT_CLASS(ALadderVolume);
IMPLEMENT_CLASS(APotentialClimbWatcher);
IMPLEMENT_CLASS(ABlockingVolume);
IMPLEMENT_CLASS(AAutoLadder);
IMPLEMENT_CLASS(ATeamInfo);
IMPLEMENT_CLASS(AObjective);
IMPLEMENT_CLASS(UEdCoordSystem);
IMPLEMENT_CLASS(UEdLayer);
IMPLEMENT_CLASS(ARoute);
IMPLEMENT_CLASS(ALiftCenter);
IMPLEMENT_CLASS(ALiftExit);
IMPLEMENT_CLASS(UBookMark);
IMPLEMENT_CLASS(UClipPadEntry);
IMPLEMENT_CLASS(ATriggerVolume);
IMPLEMENT_CLASS(AVolumePathNode);
IMPLEMENT_CLASS(ADoorMarker);
IMPLEMENT_CLASS(ADynamicBlockingVolume);
IMPLEMENT_CLASS(APortalMarker);
IMPLEMENT_CLASS(UTeleportReachSpec);
IMPLEMENT_CLASS(ALevelStreamingVolume);
IMPLEMENT_CLASS(UMapInfo);
IMPLEMENT_CLASS(ACullDistanceVolume);
IMPLEMENT_CLASS(AEmitterPool);

void AActor::NetDirty(UProperty* property)
{
	if ( property && (property->PropertyFlags & CPF_Net) )
	{
		// test and make sure actor not getting dirtied too often!
		bNetDirty = true;
	}
}

INT* AActor::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	if ( bSkipActorPropertyReplication && !bNetInitial )
	{
		return Ptr;
	}

	if ( Role == ROLE_Authority )
	{
		DOREP(Actor,bHardAttach);
  		if ( bReplicateMovement )
		{
			UBOOL bRigidActor = (Physics == PHYS_RigidBody && ((AActor*)Recent)->Physics == PHYS_RigidBody && (!bNetInitial || bStatic || bNoDelete));
			if ( RemoteRole != ROLE_AutonomousProxy )
			{
				if ( !bRigidActor )
				{
					UBOOL bAlreadyLoc = false;

					// If the actor was based and is no longer, send the location!
					if ( !Base && ((AActor*)Recent)->Base )
					{
						static UProperty* spLocation = FindObjectChecked<UProperty>(AActor::StaticClass(),TEXT("Location"));
						*Ptr++ = spLocation->RepIndex;
						bAlreadyLoc = true;
					}

					DOREP(Actor,Base);
					if( Base && !Base->bWorldGeometry && Map->CanSerializeObject(Base) )
					{
						if (bUpdateSimulatedPosition)
						{
							DOREP(Actor,RelativeLocation);
							DOREP(Actor,RelativeRotation);
						}
						if (((AActor*)Recent)->Base == NULL)
						{
							if (!bNetInitial || bUpdateSimulatedPosition)
							{
								DOREP(Actor,Location);
							}
							if (!bNetInitial || bUpdateSimulatedPosition || (!bNetInitialRotation && !bStatic && !bNoDelete))
							{
								DOREP(Actor,Rotation);
							}
						}
					}
					else 
					{
						if (bUpdateSimulatedPosition)
						{
							if (!bAlreadyLoc)
							{
								// if velocity changed to zero, make sure location gets replicated again one last time
								if ( Velocity.IsZero() && NEQ(Velocity, ((AActor*)Recent)->Velocity, Map, Channel) )
								{
									static UProperty* spLocationb = FindObjectChecked<UProperty>(AActor::StaticClass(),TEXT("Location"));
									*Ptr++ = spLocationb->RepIndex;
								}
								else
								{
									DOREP(Actor,Location);
								}
							}

							if (!bNetInitial)
							{
								DOREP(Actor,Physics);
								DOREP(Actor,Rotation);
							}
							else if (!bNetInitialRotation)
							{
								DOREP(Actor,Rotation);
							}
						}
						else if (bNetInitial && !bNetInitialRotation && !bStatic && !bNoDelete)
						{
							DOREP(Actor,Rotation);
						}
					}
				}
				else if (bReplicateRigidBodyLocation)
				{
					DOREP(Actor,Location);
				}

				if ( RemoteRole == ROLE_SimulatedProxy )
				{
					if ( !bRigidActor && (bNetInitial || bUpdateSimulatedPosition) )
					{
						if ( bReplicatePredictedVelocity )
						{
							FVector RealVelocity = Velocity;
							ExtrapolateVelocity(Velocity);
							DOREP(Actor, Velocity);
							Velocity = RealVelocity;
						}
						else
							DOREP(Actor,Velocity);
					}

					if ( bNetInitial )
					{
						DOREP(Actor,Physics);
					}
				}
			}
			else if ( bNetInitial && !bNetInitialRotation )
			{
				DOREP(Actor,Rotation);
			}
		}
		if ( bNetDirty )
		{
			DOREP(Actor,DrawScale);
			//DOREP(Actor,DrawScale3D); // Doesn't work in networking, because of vector rounding
			DOREP(Actor,bCollideActors);
			DOREP(Actor,bCollideWorld);
			DOREP(Actor,bHidden);
			if( bCollideActors || bCollideWorld )
			{
				DOREP(Actor,bProjTarget);
				DOREP(Actor,bBlockActors);
			}
			if ( !bSkipActorPropertyReplication )
			{
				// skip these if bSkipActorPropertyReplication, because if they aren't relevant to the client, bNetInitial never gets cleared
				// which obviates bSkipActorPropertyReplication
 				if ( bNetOwner )
				{
					DOREP(Actor,Owner);
				}
				if ( bReplicateInstigator && (!bNetTemporary || (Instigator && Map->CanSerializeObject(Instigator))) )
				{
					DOREP(Actor,Instigator);
				}
			}
		}
		DOREP(Actor,Role);
		DOREP(Actor,RemoteRole);
		DOREP(Actor,bNetOwner);
		DOREP(Actor,bTearOff);
	}

	return Ptr;
}

/** 
 * Predict average velocity over next replication interval
 */
void AActor::ExtrapolateVelocity( FVector& ExtrapolatedVelocity)
{
	ExtrapolatedVelocity = Velocity;
}

/** 
 * Predict average velocity over next replication interval
 */
void APawn::ExtrapolateVelocity( FVector& ExtrapolatedVelocity)
{
	ExtrapolatedVelocity = Velocity;

	// only predict velocity if walking and not using root motion
	if ( (Physics != PHYS_Walking) || (Mesh && (Mesh->RootMotionMode != RMM_Ignore)) )
	{
		//@todo steve - extrapolate for flying and swimming as well
		return;
	}
	FLOAT PredictionDelta = 0.5f/GWorld->NetDriver->NetServerMaxTickRate;	// 1/60 of a second, assuming 30 updates per second
	FVector RealVelocity = Velocity;

	if ( Acceleration.IsZero() )
	{
		// don't drift to a stop, brake
		ExtrapolatedVelocity = ExtrapolatedVelocity - (2.f * ExtrapolatedVelocity) * PredictionDelta * PhysicsVolume->GroundFriction; 

		// brake to a stop, not backwards
		if( (RealVelocity | ExtrapolatedVelocity) < 0.f )
		{
			ExtrapolatedVelocity = FVector(0.f);
		}
	}
	else
	{
		ExtrapolatedVelocity = ExtrapolatedVelocity + Acceleration * PredictionDelta;
		if ( ExtrapolatedVelocity.SizeSquared() > GroundSpeed*GroundSpeed )
		{
			ExtrapolatedVelocity = ExtrapolatedVelocity.SafeNormal() * GroundSpeed;
		}
	}
}

INT* APawn::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	if ( !bTearOff && !bNetOwner && (GWorld->GetTimeSeconds() - Channel->LastFullUpdateTime < 0.09f) )
	{
		DOREP(Pawn,PlayerReplicationInfo);
		Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
		return Ptr;
	}
	Channel->LastFullUpdateTime = GWorld->GetTimeSeconds();

	if (bTearOff && Role == ROLE_Authority && bNetDirty)
	{
		DOREP(Pawn,TearOffMomentum);
	}

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	if ( Role==ROLE_Authority )
	{
		if ( !bNetOwner )
		{
			DOREP(Pawn,RemoteViewPitch);
		}

		if ( bNetDirty )
		{
			DOREP(Pawn,PlayerReplicationInfo); 
			DOREP(Pawn,Health);

			if (bNetOwner && (Controller == NULL || Map->CanSerializeObject(Controller)))
			{
				DOREP(Pawn,Controller);
			}
			else if (((APawn*)Recent)->Controller != NULL && ((APawn*)Recent)->Controller != Controller)
			{
				// we don't do DOREP() here because NEQ() will mark Pawn to stay dirty if Controller can't be serialized, which we don't want
				// but we do need it to replicate a NULL in that case
				static UProperty* spControllerb = FindObjectChecked<UProperty>(APawn::StaticClass(), TEXT("Controller"));
				*Ptr++ = spControllerb->RepIndex;
			}

			if( bNetOwner )
			{
				DOREP(Pawn,GroundSpeed);
				DOREP(Pawn,WaterSpeed);
				DOREP(Pawn,AirSpeed);
				DOREP(Pawn,AccelRate);
				DOREP(Pawn,JumpZ);
				DOREP(Pawn,AirControl);
				DOREP(Pawn,InvManager);
			}

			if( !bNetOwner )
			{
				DOREP(Pawn,bIsCrouched);
				DOREP(Pawn,FlashCount);
				DOREP(Pawn,FiringMode);
			}

			DOREP(Pawn,FlashLocation);
			DOREP(Pawn,HitDamageType);
			DOREP(Pawn,TakeHitLocation);
			DOREP(Pawn,bSimulateGravity);
			DOREP(Pawn,bIsWalking);

			if (DrivenVehicle == NULL || Map->CanSerializeObject(DrivenVehicle))
			{
				DOREP(Pawn,DrivenVehicle);
			}

			if( !bTearOff )
			{
				DOREP(Pawn,TearOffMomentum);
			}
		}
	}

	return Ptr;
}

INT* AVehicle::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if ( Role == ROLE_Authority )
	{
		if ( bNetDirty )
		{
			DOREP(Vehicle,bDriving);
			if ( bNetOwner || !Driver || !Driver->bHidden )
			{
				DOREP(Vehicle,Driver);
			}
		}
	}

	return Ptr;
}

INT* ASVehicle::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if (Physics == PHYS_RigidBody && (Controller != NULL || NEQ(VState.RBState, ((ASVehicle*)Recent)->VState.RBState, Map, Channel)))
	{
		DOREP(SVehicle,VState);
	}

	return Ptr;
}

INT* AController::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( bNetDirty && (Role==ROLE_Authority) )
	{
		DOREP(Controller,PlayerReplicationInfo);
		DOREP(Controller,Pawn);
	}

	return Ptr;
}

INT* APlayerController::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( bNetOwner && (Role==ROLE_Authority) )
	{
		if ( (ViewTarget != Pawn) && ViewTarget && ViewTarget->GetAPawn() )
		{
			DOREP(PlayerController,TargetViewRotation);
			DOREP(PlayerController,TargetEyeHeight);
		}
	}

	return Ptr;
}

INT* APhysicsVolume::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( (Role==ROLE_Authority) && bSkipActorPropertyReplication && !bNetInitial )
	{
		DOREP(Actor,Location);
		DOREP(Actor,Rotation);
		DOREP(Actor,Base);
		if( Base && !Base->bWorldGeometry )
		{
			DOREP(Actor,RelativeLocation);
			DOREP(Actor,RelativeRotation);
		}
	}

	return Ptr;
}

INT* AWorldInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	// only replicate needed actor properties
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if (bNetDirty)
	{
		DOREP(WorldInfo,Pauser);
		DOREP(WorldInfo,TimeDilation);
		DOREP(WorldInfo,WorldGravityZ);
		DOREP(WorldInfo,bHighPriorityLoading);
	}

	return Ptr;
}

INT* AReplicationInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{

	if ( !bSkipActorPropertyReplication )
		Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	return Ptr;
}

INT* APlayerReplicationInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( (Role==ROLE_Authority) && bNetDirty )
	{
		DOREP(PlayerReplicationInfo,Score);
		DOREP(PlayerReplicationInfo,Deaths);
		DOREP(PlayerReplicationInfo,bHasFlag);

		if ( !bNetOwner )
		{
			DOREP(PlayerReplicationInfo,Ping);
			DOREP(PlayerReplicationInfo,PacketLoss);
		}
 		DOREP(PlayerReplicationInfo,PlayerLocationHint);
		DOREP(PlayerReplicationInfo,PlayerName);
		DOREP(PlayerReplicationInfo,Team);
		DOREP(PlayerReplicationInfo,TeamID);
		DOREP(PlayerReplicationInfo,bAdmin);
		DOREP(PlayerReplicationInfo,bIsFemale);
		DOREP(PlayerReplicationInfo,bIsSpectator);
		DOREP(PlayerReplicationInfo,bOnlySpectator);
		DOREP(PlayerReplicationInfo,bWaitingPlayer);
		DOREP(PlayerReplicationInfo,bReadyToPlay);
		DOREP(PlayerReplicationInfo,bOutOfLives);
		DOREP(PlayerReplicationInfo,bControllerVibrationAllowed);
		DOREP(PlayerReplicationInfo,StartTime);
		DOREP(PlayerReplicationInfo,UniqueId);

		if ( bNetInitial )
		{
			DOREP(PlayerReplicationInfo,PlayerID);
			DOREP(PlayerReplicationInfo,bBot);
			DOREP(PlayerReplicationInfo,StartTime);
			DOREP(PlayerReplicationInfo,bIsInactive);
		}
	}

	return Ptr;
}

INT* AGameReplicationInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( (Role==ROLE_Authority) && bNetDirty )
	{
		DOREP(GameReplicationInfo,bStopCountDown);
		DOREP(GameReplicationInfo,bMatchHasBegun);
		DOREP(GameReplicationInfo,bMatchIsOver);
		DOREP(GameReplicationInfo,Winner);
		DOREP(GameReplicationInfo,MatchID);
		DOREPARRAY(GameReplicationInfo,Teams);
		if ( bNetInitial )
		{
			DOREP(GameReplicationInfo,GameClass);
			DOREP(GameReplicationInfo,RemainingTime);
			DOREP(GameReplicationInfo,ElapsedTime);
			DOREP(GameReplicationInfo,ServerName);
			DOREP(GameReplicationInfo,ShortName);
			DOREP(GameReplicationInfo,AdminName);
			DOREP(GameReplicationInfo,AdminEmail);
			DOREP(GameReplicationInfo,ServerRegion);
			DOREP(GameReplicationInfo,MessageOfTheDay);
			DOREP(GameReplicationInfo,GoalScore);
			DOREP(GameReplicationInfo,TimeLimit);
			DOREP(GameReplicationInfo,MaxLives);
			DOREP(GameReplicationInfo,bTrackStats);
			DOREP(GameReplicationInfo,bIsArbitrated);
			// prevent RemainingMinute from getting replicated until it changes as we already send the more up to date RemainingTime in this case
			((AGameReplicationInfo*)Recent)->RemainingMinute = RemainingMinute;
		}
		else
		{
			DOREP(GameReplicationInfo,RemainingMinute);
		}
	}

	return Ptr;
}

INT* ATeamInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	if( Role==ROLE_Authority )
	{
		if ( bNetDirty )
		{
			DOREP(TeamInfo,Score);
		}
		if ( bNetInitial )
		{
			DOREP(TeamInfo,TeamName);
			DOREP(TeamInfo,TeamIndex);
		}
	}

	return Ptr;
}

INT* APickupFactory::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	DOREP(PickupFactory,bPickupHidden);
	if ( bOnlyReplicateHidden )
	{
		DOREP(Actor,bHidden);
		if ( bNetInitial )
		{
			DOREP(Actor,Rotation);
		}
	}
	else
	{
		Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	}
	
	return Ptr;
}

INT* AInventory::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	if( bNetOwner && (Role==ROLE_Authority) && bNetDirty )
	{
		DOREP(Inventory,InvManager);
		DOREP(Inventory,Inventory);
	}

	return Ptr;
}

INT* AMatineeActor::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	if (!bSkipActorPropertyReplication)
	{
		Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);
	}

	if (bNetDirty)
	{
		if (bNetInitial)
		{
			DOREP(MatineeActor,InterpAction);
		}
		DOREP(MatineeActor,bIsPlaying);
		DOREP(MatineeActor,bReversePlayback);
		DOREP(MatineeActor,bPaused);
		DOREP(MatineeActor,PlayRate);
		DOREP(MatineeActor,Position);
	}

	return Ptr;
}

INT* AKActor::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);

	DOREP(KActor,RBState);
	if (bNetInitial)
	{
		DOREP(KActor,bWakeOnLevelStart);
		DOREP(KActor,DrawScaleX);
		DOREP(KActor,DrawScaleY);
		DOREP(KActor,DrawScaleZ);
	}

	return Ptr;
}

INT* AKAsset::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);

	if ( bNetDirty )
	{
		DOREP(KAsset,RepHitImpulse);
		DOREP(KAsset,ReplicatedMesh);
		DOREP(KAsset,bTempRep);
	}
	return Ptr;
}
 
INT* AVolume::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);

	// LDs can change collision of dynamic volumes at runtime via Kismet, so we need to make sure we replicate that even if bSkipActorPropertyReplication is true
	if (bSkipActorPropertyReplication && !bNetInitial)
	{
		DOREP(Actor,bCollideActors);
	}

	return Ptr;
}

/*-----------------------------------------------------------------------------
	AActor networking implementation.
-----------------------------------------------------------------------------*/

//
// Static variables for networking.
//
static FVector				SavedLocation;
static FVector				SavedRelativeLocation;
static FRotator				SavedRotation;
static FRotator				SavedRelativeRotation;
static AActor*				SavedBase;
static DWORD				SavedCollision;
static FLOAT				SavedRadius;
static FLOAT				SavedHeight;
static FVector				SavedSimInterpolate;
static FLOAT				SavedDrawScale;
static AVehicle*			SavedDrivenVehicle;
static UBOOL				SavedHardAttach;
static UBOOL				SavedbIsCrouched;
static USeqAct_Interp*		SavedInterpAction;
static UBOOL				SavedbIsPlaying;
static FLOAT				SavedPosition;
static BYTE					SavedPhysics;
static UBOOL				SavedbHidden;
static AActor*				SavedOwner;

/** GetNetPriority()
@param Viewer		PlayerController owned by the client for whom net priority is being determined
@param InChannel	Channel on which this actor is being replicated.
@param Time			Time since actor was last replicated
@returns			Priority of this actor for replication
*/
FLOAT AActor::GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, FLOAT Time, UBOOL bLowBandwidth)
{
	if ( Instigator && (Instigator == Viewer->Pawn) )
		Time *= 4.f; 
	else if ( !bHidden )
	{
		FVector Dir = Location - ViewPos;
		FLOAT DistSq = Dir.SizeSquared();
		if ( bLowBandwidth )
		{
			if ( (ViewDir | Dir) < 0.f )
			{
				if ( DistSq > NEARSIGHTTHRESHOLDSQUARED )
					Time *= 0.2f;
				else if ( DistSq > CLOSEPROXIMITYSQUARED )
					Time *= 0.5f;
			}
			if ( DistSq > MEDSIGHTTHRESHOLDSQUARED )
				Time *= 0.35f;
			else if ( Base && (Base == Viewer->Pawn) )
				Time *= 3.f;
		}
		else if ( (ViewDir | Dir) < 0.f )
		{
			if ( DistSq > NEARSIGHTTHRESHOLDSQUARED )
				Time *= 0.3f;
			else if ( DistSq > CLOSEPROXIMITYSQUARED )
				Time *= 0.5f;
		}
	}

	return NetPriority * Time;
}

//
// Always called immediately before properties are received from the remote.
//
void AActor::PreNetReceive()
{
	SavedLocation   = Location;
	SavedRotation   = Rotation;
	SavedRelativeLocation = RelativeLocation;
	SavedRelativeRotation = RelativeRotation;
	SavedBase       = Base;
	SavedHardAttach = bHardAttach;
	SavedCollision  = bCollideActors;
	SavedDrawScale	= DrawScale;
	SavedPhysics = Physics;
	SavedbHidden = bHidden;
	SavedOwner = Owner;
}

void APawn::PreNetReceive()
{
	SavedbIsCrouched = bIsCrouched;
	SavedDrivenVehicle = DrivenVehicle;
	AActor::PreNetReceive();
}

//
// Always called immediately after properties are received from the remote.
//
void AActor::PostNetReceive()
{
	Exchange ( Location,        SavedLocation  );
	Exchange ( Rotation,        SavedRotation  );
	Exchange ( RelativeLocation,        SavedRelativeLocation  );
	Exchange ( RelativeRotation,        SavedRelativeRotation  );
	Exchange ( Base,            SavedBase      );
	ExchangeB( bCollideActors,  SavedCollision );
	Exchange ( DrawScale, SavedDrawScale       );
	ExchangeB( bHardAttach, SavedHardAttach );
	ExchangeB( bHidden, SavedbHidden );
	Exchange ( Owner, SavedOwner );

	if (bHidden != SavedbHidden)
	{
		SetHidden(SavedbHidden);
	}
	if (Owner != SavedOwner)
	{
		SetOwner(SavedOwner);
	}

	if( bCollideActors!=SavedCollision )
	{
		SetCollision( SavedCollision, bBlockActors, bIgnoreEncroachers );
	}
	PostNetReceiveLocation();
	if( Rotation!=SavedRotation )
	{
		FCheckResult Hit;
		GWorld->MoveActor( this, FVector(0,0,0), SavedRotation, MOVE_NoFail, Hit );
	}
	if ( DrawScale!=SavedDrawScale )
		SetDrawScale(SavedDrawScale);

	if (Physics != SavedPhysics)
	{
		Exchange(Physics, SavedPhysics);
		setPhysics(SavedPhysics);
	}

	PostNetReceiveBase(SavedBase);
}

void AActor::PostNetReceiveBase(AActor* NewBase)
{	
	UBOOL bBaseChanged = ( Base!=NewBase );
	if( bBaseChanged )
	{
		bHardAttach = SavedHardAttach;

		// Base changed.
		UBOOL bRelativeLocationChanged = (SavedRelativeLocation != RelativeLocation);
		UBOOL bRelativeRotationChanged = (SavedRelativeRotation != RelativeRotation);
		SetBase( NewBase );
		if ( !bRelativeLocationChanged )
			SavedRelativeLocation = RelativeLocation;
		if ( !bRelativeRotationChanged )
			SavedRelativeRotation = RelativeRotation;
	}
	else
	{
		// If the base didn't change, but the 'hard attach' flag did, re-base this actor.
		if(SavedHardAttach != bHardAttach)
		{
			bHardAttach = SavedHardAttach;

			SetBase( NULL );
			SetBase( NewBase );
		}
	}

	if ( Base && !Base->bWorldGeometry )
	{
		if ( bBaseChanged || (RelativeLocation != SavedRelativeLocation) )
		{
			GWorld->FarMoveActor( this, Base->Location + SavedRelativeLocation, 0, 1, 1 );
			RelativeLocation = SavedRelativeLocation;
		}
		if ( bBaseChanged || (RelativeRotation != SavedRelativeRotation) )
		{
			FCheckResult Hit;
			FRotator NewRotation = (FRotationMatrix(SavedRelativeRotation) * FRotationMatrix(Base->Rotation)).Rotator();
			GWorld->MoveActor( this, FVector(0,0,0), NewRotation, MOVE_NoFail, Hit);
		}
	}
	bJustTeleported = 0;
}

void AActor::PostNetReceiveLocation()
{
	if (Location != SavedLocation)
	{
		GWorld->FarMoveActor(this, SavedLocation, 0, 1, 1);
		if (Physics == PHYS_RigidBody)
		{
			// move rigid body components in physics system
			for (INT i = 0; i < Components.Num(); i++)
			{
				UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Components(i));
				if (Primitive != NULL)
				{
					USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Primitive);
					if(!SkelComp || SkelComp->bSyncActorLocationToRootRigidBody)
					{
						Primitive->SetRBPosition(Primitive->LocalToWorld.GetOrigin());
					}
				}
			}
		}
	}
}

void APawn::PostNetReceiveLocation()
{
	if ( Physics == PHYS_RigidBody )
	{
		AActor::PostNetReceiveLocation();
		return;
	}

	if ( DrivenVehicle != SavedDrivenVehicle )
	{
		if ( DrivenVehicle )
		{
			if (SavedDrivenVehicle != NULL)
			{
				AVehicle* NewDrivenVehicle = DrivenVehicle;
				eventStopDriving(SavedDrivenVehicle);
				DrivenVehicle = NewDrivenVehicle;
			}

			eventStartDriving(DrivenVehicle);
			SavedBase = Base;
			SavedPhysics = Physics;
			SavedHardAttach = bHardAttach;
			SavedRotation = Rotation;
			SavedRelativeLocation = RelativeLocation;
			SavedRelativeRotation = RelativeRotation;
			return;
		}
		else
		{
			FVector NewLoc = SavedLocation;
			SavedLocation = Location;
			eventStopDriving(SavedDrivenVehicle);
			if ( Location == SavedLocation )
				SavedLocation = NewLoc;
		}
	}

	// Fire event when Pawn starts or stops crouching, to update collision and mesh offset.
	if( bIsCrouched != SavedbIsCrouched )
	{
		if( bIsCrouched )
		{
			if (Role == ROLE_SimulatedProxy)
			{
				// restore collision size if it was reduced by below code so it doesn't affect crouch height change
				APawn* DefaultPawn = (APawn*)(GetClass()->GetDefaultActor());
				if (DefaultPawn->CylinderComponent->CollisionRadius - CylinderComponent->CollisionRadius - 1.f < KINDA_SMALL_NUMBER)
				{
					SetCollisionSize(CylinderComponent->CollisionRadius + 1.f, CylinderComponent->CollisionHeight + 1.f);
				}
			}

			Crouch(1);
		}
		else
		{
			UnCrouch(1);
		}
	}

	// always consider Location as changed if we were spawned this tick as in that case our replicated Location was set as part of spawning, before PreNetReceive()
	if (Location == SavedLocation && CreationTime != WorldInfo->TimeSeconds)
		return;

	if( Role == ROLE_SimulatedProxy )
	{
		FCheckResult Hit(1.f);
		if ( GWorld->EncroachingWorldGeometry(Hit,SavedLocation+CollisionComponent->Translation,GetCylinderExtent()) )
		{
			APawn* DefaultPawn = (APawn*)(GetClass()->GetDefaultActor());

			if ( CylinderComponent->CollisionRadius == DefaultPawn->CylinderComponent->CollisionRadius )
			{
				// slightly reduce cylinder size to compensate for replicated vector rounding errors
				SetCollisionSize(CylinderComponent->CollisionRadius - 1.f, CylinderComponent->CollisionHeight - 1.f);
			}
			bSimGravityDisabled = TRUE;
		}
		else if ( bIsCrouched || Velocity.IsZero() )
			bSimGravityDisabled = TRUE;
		else
		{
			SavedLocation.Z += 2.f;
			bSimGravityDisabled = FALSE;
		}
		FVector OldLocation = Location;
		GWorld->FarMoveActor( this, SavedLocation, 0, 1, 1 );
		if ( !bSimGravityDisabled )
		{
			// smooth out movement of other players to account for frame rate induced jitter
			// look at whether location is a reasonable approximation already
			// if so only partially correct
			FVector Dir = OldLocation - Location;
			FLOAT StartError = Dir.Size();
			if ( StartError > 4.f )
			{
				moveSmooth(::Min(0.75f * StartError,CylinderComponent->CollisionRadius) * Dir.SafeNormal());
			}
		}
	}
	else
		Super::PostNetReceiveLocation();
}

void AMatineeActor::PreNetReceive()
{
	Super::PreNetReceive();

	SavedInterpAction = InterpAction;
	SavedbIsPlaying = bIsPlaying;
	SavedPosition = Position;
}

void AMatineeActor::PostNetReceive()
{
	Super::PostNetReceive();

	if (InterpAction != NULL)
	{
		// if we just received the matinee action, set 'saved' values to default so we make sure to apply previously received values
		if (SavedInterpAction == NULL)
		{
			AMatineeActor* Default = GetClass()->GetDefaultObject<AMatineeActor>();
			SavedbIsPlaying = Default->bIsPlaying;
			SavedPosition = Default->Position;
		}
		// apply PlayRate
		InterpAction->PlayRate = PlayRate;
		// apply bReversePlayback
		if (InterpAction->bReversePlayback != bReversePlayback)
		{
			InterpAction->bReversePlayback = bReversePlayback;
			if (SavedbIsPlaying && bIsPlaying)
			{
				// notify actors that something has changed
				for (INT i = 0; i < InterpAction->LatentActors.Num(); i++)
				{
					if (InterpAction->LatentActors(i) != NULL)
					{
						InterpAction->LatentActors(i)->eventInterpolationChanged(InterpAction);
					}
				}
			}
		}
		// start up interpolation, if necessary
		if (!SavedbIsPlaying && (bIsPlaying || Position != SavedPosition))
		{
			InterpAction->InitInterp();
			// if we're playing forward, call Play() to process any special properties on InterpAction that may affect the meaning of 'Position' (bNoResetOnRewind, etc)
			if (!bReversePlayback)
			{
				InterpAction->Play();
			}
			// find affected actors and add InterpAction to their LatentActions list so they can find it in physInterpolating()
			// @warning: this code requires the linked actors to be static object references (i.e., some other Kismet action can't be assigning them)
			TArray<UObject**> ObjectVars;
			InterpAction->GetObjectVars(ObjectVars);
			for (INT i = 0; i < ObjectVars.Num(); i++)
			{
				AActor* Actor = Cast<AActor>(*(ObjectVars(i)));
				if (Actor != NULL && !Actor->bDeleteMe)
				{
					Actor->LatentActions.AddItem(InterpAction);
					InterpAction->LatentActors.AddItem(Actor);
					// fire an event if we're really playing (and not just starting it up to do a position update)
					if (bIsPlaying)
					{
						// if actor has already been ticked, reupdate physics with updated position from track.
						// Fixes Matinee viewing through a camera at old position for 1 frame.
						Actor->performPhysics(1.f);
						Actor->eventInterpolationStarted(InterpAction);
					}
				}
			}
		}
		// if we received a different current position
		if (Position != SavedPosition)
		{
			// set to position replicated from server
			InterpAction->UpdateInterp(Position, false, false);
			// update interpolating actors for the new position
			for (INT i = 0; i < InterpAction->LatentActors.Num(); i++)
			{
				AActor *InterpActor = InterpAction->LatentActors(i);
				if (InterpActor != NULL && !InterpActor->IsPendingKill() && InterpActor->Physics == PHYS_Interpolating)
				{
					InterpAction->LatentActors(i)->physInterpolating(0.f);
				}
			}
		}
		// terminate interpolation, if necessary
		if ((SavedbIsPlaying || Position != SavedPosition) && !bIsPlaying)
		{
			InterpAction->TermInterp();
			// find affected actors and remove InterpAction from their LatentActions list
			while (InterpAction->LatentActors.Num() > 0)
			{
				AActor* Actor = InterpAction->LatentActors.Pop();
				if (Actor != NULL)
				{
					Actor->LatentActions.RemoveItem(InterpAction);
					// fire an event if we were really playing (and not just starting it up to do a position update)
					if (SavedbIsPlaying)
					{
						Actor->eventInterpolationFinished(InterpAction);
					}
				}
			}
		}
		InterpAction->bIsPlaying = bIsPlaying;
		InterpAction->bPaused = bPaused;
	}
}

/*-----------------------------------------------------------------------------
	APlayerController implementation.
-----------------------------------------------------------------------------*/

//
// Set the player.
//
void APlayerController::SetPlayer( UPlayer* InPlayer )
{
	check(InPlayer!=NULL);

	// Detach old player.
	if( InPlayer->Actor )
		InPlayer->Actor->Player = NULL;

	// Set the viewport.
	Player = InPlayer;
	InPlayer->Actor = this;

	// cap outgoing rate to max set by server
	UNetDriver* Driver = GWorld->NetDriver;
	if( (ClientCap>=2600) && Driver && Driver->ServerConnection )
		Player->CurrentNetSpeed = Driver->ServerConnection->CurrentNetSpeed = Clamp( ClientCap, 1800, Driver->MaxClientRate );

	// initialize the input system only if local player
	if ( Cast<ULocalPlayer>(InPlayer) )
	{
		eventInitInputSystem();
	}

	// This is done in controller replicated event for clients
	if (GWorld->GetWorldInfo()->NetMode != NM_Client)
	{
		eventInitUniquePlayerId();
	}
	eventSpawnPlayerCamera();

	// notify script that we've been assigned a valid player
	eventReceivedPlayer();
}

/*-----------------------------------------------------------------------------
	AActor.
-----------------------------------------------------------------------------*/

void AActor::BeginDestroy()
{
	ClearComponents();
	Super::BeginDestroy();
}

UBOOL AActor::IsReadyForFinishDestroy()
{
	return Super::IsReadyForFinishDestroy() && DetachFence.GetNumPendingFences() == 0;
}

void AActor::PostLoad()
{
	Super::PostLoad();

	// check for empty Attached entries
	for ( INT i=0; i<Attached.Num(); i++ )
	{
		if ( (Attached(i) == NULL) || (Attached(i)->Base != this) || Attached(i)->bDeleteMe )
		{
			Attached.Remove(i--);
		}
	}

	// add ourselves to our Owner's Children array
	if (Owner != NULL)
	{
		checkSlow(!Owner->Children.ContainsItem(this));
		Owner->Children.AddItem(this);
	}

	// Set our components' owners so InitRBPhys works.
	for(UINT ComponentIndex = 0;ComponentIndex < (UINT)Components.Num();ComponentIndex++)
	{
		UActorComponent* ActorComponent = Components(ComponentIndex);
		if( ActorComponent )
		{
			ActorComponent->SetOwner(this);
		}
	}

	// Convert UseCharacterLights to lighting channels. This needs to happen in AActor::PostLoad instead of
	// UPrimitiveComponent::PostLoad as the latter doesn't have access to the owner at this point.
	if( GetLinkerVersion() < VER_LIGHTING_CHANNEL_SUPPORT )
	{
		for(UINT ComponentIndex = 0;ComponentIndex < (UINT)Components.Num();ComponentIndex++)
		{
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Components(ComponentIndex));
			if( PrimitiveComponent )
			{
				PrimitiveComponent->LightingChannels.Dynamic		= UseCharacterLights || !HasStaticShadowing();
				PrimitiveComponent->LightingChannels.Static			= !UseCharacterLights && HasStaticShadowing();
				PrimitiveComponent->LightingChannels.bInitialized	= TRUE;
			}
		}
	}

	SetDefaultCollisionType();

	// Check/warning when loading actors in editor. Should never load bDeleteMe Actors!
	if (GIsEditor && bDeleteMe)
	{
		debugf( TEXT("Loaded Actor (%s) with bDeleteMe == true"), *GetName() );
	}
}

void AActor::ProcessEvent( UFunction* Function, void* Parms, void* Result )
{
	if( (GWorld->HasBegunPlay() || HasAnyFlags(RF_ClassDefaultObject)) && !GIsGarbageCollecting )
	{
		Super::ProcessEvent( Function, Parms, Result );
	}
}

/** information for handling Actors having their Base changed between PreEditChange() and PostEditChange() */
struct FBaseInfo
{
	AActor* Actor;
	AActor* Base;
	USkeletalMeshComponent* BaseSkelComponent;
	FName BaseBoneName;

	FBaseInfo(AActor* InActor)
	: Actor(InActor), Base(InActor->Base), BaseSkelComponent(InActor->BaseSkelComponent), BaseBoneName(InActor->BaseBoneName)
	{}
};
static TArray<FBaseInfo> BaseInfos;

/** hackish way to prevent LDs getting spammed with world geometry bBlockActors warnings when changing many actors at once */
static UBOOL bDisplayedWorldGeometryWarning = FALSE;

void AActor::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	ClearComponents();

	new(BaseInfos) FBaseInfo(this);

	bDisplayedWorldGeometryWarning = FALSE;
}

void AActor::PostEditChange(UProperty* PropertyThatChanged)
{
	for ( INT i=0; i<Attached.Num(); i++ )
	{
		if ( Attached(i) == NULL )
		{
			Attached.Remove(i);
			i--;
		}
	}

	// we only want CollisionType to modify the flags if the LD explicitly modified it
	if (PropertyThatChanged != NULL && PropertyThatChanged->GetFName() == FName(TEXT("CollisionType")))
	{
		SetCollisionFromCollisionType();
		GCallbackEvent->Send( CALLBACK_RefreshPropertyWindows );
	}

	if (CollisionComponent != NULL && (PropertyThatChanged == NULL || PropertyThatChanged->GetFName() == FName(TEXT("BlockRigidBody"))))
	{
		CollisionComponent->BlockRigidBody = BlockRigidBody;
		GCallbackEvent->Send( CALLBACK_RefreshPropertyWindows );
	}

	ForceUpdateComponents(FALSE,FALSE);

	// Now, because we need to correctly remove this Actor from its old Base's Attachments array, we have to pretend that nothing has changed yet
	// and call SetBase(NULL). We backed up the old base information in PreEditChange...
	for (INT i = 0; i < BaseInfos.Num(); i++)
	{
		// If this is the actor you are looking for
		if( BaseInfos(i).Actor == this )
		{
			// If this actor can't move AND new base can move - invalid base
			if( Base != NULL && (bStatic || !bMovable) && (!Base->bStatic && Base->bMovable) )
			{
				debugf(TEXT("PostEditChange 'Base' failed! Cannot base static actor %s on moveable actor %s - restoring previous base %s"), *GetName(), *Base->GetName(), BaseInfos(i).Base ? *BaseInfos(i).Base->GetName() : TEXT("NULL") );

				// Restore the old base
				SetBase( BaseInfos(i).Base );
				EditorUpdateBase();
			}
			else
			// Otherwise, if something has changed
			if( Base != BaseInfos(i).Base || BaseSkelComponent != BaseInfos(i).BaseSkelComponent || BaseBoneName != BaseInfos(i).BaseBoneName )
			{
				// Back up 'new data'
				FBaseInfo NewBaseInfo(this);

				// Restore 'old' base settings, and call SetBase(NULL) to cleanly detach and broadly attach to new Actor
				Base = BaseInfos(i).Base;
				BaseSkelComponent = BaseInfos(i).BaseSkelComponent;
				BaseBoneName = BaseInfos(i).BaseBoneName;
				SetBase(NewBaseInfo.Base);

				// Put in 'new' settings, and call EditorUpdateBase (which will find a SkeletalMeshComponent if necessary)
				// and update the Base fully.
				BaseSkelComponent = NewBaseInfo.BaseSkelComponent;
				BaseBoneName = NewBaseInfo.BaseBoneName;
				EditorUpdateBase();
			}
			BaseInfos.Remove(i);
			break;
		}
	}

	GCallbackEvent->Send( CALLBACK_UpdateUI );
	Super::PostEditChange(PropertyThatChanged);
}

// There might be a constraint to this actor, so we iterate over all actors to check. If there is, update ref frames.
void AActor::PostEditMove(UBOOL bFinished)
{
	if ( bFinished )
	{
		// propagate our movement
		GObjectPropagator->OnActorMove(this);

		// Look for a component on this actor that accepts decals.
		// Issue a decal update request if one is found.
		if ( GIsEditor && !GIsPlayInEditorWorld )
		{
			for ( INT Idx = 0 ; Idx < Components.Num() ; ++Idx )
			{
				const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>( Components(Idx) );
				if ( PrimitiveComponent && PrimitiveComponent->bAcceptsDecals )
				{
					GEngine->IssueDecalUpdateRequest();
					break;
				}
			}
		}

		// Update any constraint actors joined to this actor.
		for( FActorIterator It; It; ++It )
		{
			ARB_ConstraintActor* ConActor = Cast<ARB_ConstraintActor>( *It );
			if(ConActor)
			{
				if( ConActor->ConstraintActor1 == this || 
					ConActor->ConstraintActor2 == this ||
					ConActor->PulleyPivotActor1 == this || 
					ConActor->PulleyPivotActor2 == this )
				{
					ConActor->UpdateConstraintFramesFromActor();
				}
			}
		}

		GCallbackEvent->Send( CALLBACK_UpdateUI );
	}
	
	// Mark components as dirty so their rendering gets updated.
	MarkComponentsAsDirty();
}

/**
 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
 * The default implementation is simply to translate the actor's location.
 */
void AActor::EditorApplyTranslation(const FVector& DeltaTranslation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	Location += DeltaTranslation;
}

/**
 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
 * The default implementation is simply to modify the actor's rotation.
 */
void AActor::EditorApplyRotation(const FRotator& DeltaRotation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	FRotator ActorRotWind, ActorRotRem;
	this->Rotation.GetWindingAndRemainder(ActorRotWind, ActorRotRem);

	const FQuat ActorQ = ActorRotRem.Quaternion();
	const FQuat DeltaQ = DeltaRotation.Quaternion();
	const FQuat ResultQ = DeltaQ * ActorQ;

	const FRotator NewActorRotRem = FRotator( ResultQ );
	FRotator DeltaRot = NewActorRotRem - ActorRotRem;
	DeltaRot.MakeShortestRoute();

	this->Rotation += DeltaRot;
}

/**
 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
 * The default implementation is simply to modify the actor's draw scale.
 */
void AActor::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	DrawScale3D += ScaleMatrix.TransformFVector( DrawScale3D );

	if( PivotLocation )
	{
		Location -= *PivotLocation;
		Location += ScaleMatrix.TransformFVector( Location );
		Location += *PivotLocation;
	}

	GCallbackEvent->Send( CALLBACK_UpdateUI );
}

void AActor::SetDefaultCollisionType()
{
	// default to 'custom' (programmer set nonstandard settings)
	CollisionType = COLLIDE_CustomDefault;

	if (bCollideActors && CollisionComponent != NULL && CollisionComponent->CollideActors)
	{
		if (!bBlockActors || CollisionComponent->BlockActors)
		{
			if (CollisionComponent->BlockZeroExtent)
			{
				if (CollisionComponent->BlockNonZeroExtent)
				{
					CollisionType = (bBlockActors && CollisionComponent->BlockActors) ? COLLIDE_BlockAll : COLLIDE_TouchAll;
				}
				else
				{
					CollisionType = (bBlockActors && CollisionComponent->BlockActors) ? COLLIDE_BlockWeapons : COLLIDE_TouchWeapons;
				}
			}
			else if (CollisionComponent->BlockNonZeroExtent)
			{
				CollisionType = (bBlockActors && CollisionComponent->BlockActors) ? COLLIDE_BlockAllButWeapons : COLLIDE_TouchAllButWeapons;
			}
		}
		// else (bBlockActors && !CollisionComponent->BlockActors), we're using some custom collision (e.g. only secondary collision component blocks)
	}
	else if (!bCollideActors && (!CollisionComponent || !CollisionComponent->BlockRigidBody))
	{
		CollisionType = COLLIDE_NoCollision;
	}

	// match mirrored BlockRigidBody flag
	if (CollisionComponent != NULL)
	{
		BlockRigidBody = CollisionComponent->BlockRigidBody;
	}

	// also make sure archetype CollisionType is set so that it only shows up bold in the property window if it has actually been changed
	AActor* TemplateActor = GetArchetype<AActor>();
	if (TemplateActor != NULL)
	{
		TemplateActor->SetDefaultCollisionType();
	}
}

/** sets collision flags based on the current CollisionType */
void AActor::SetCollisionFromCollisionType()
{
	if (CollisionComponent != NULL)
	{
		// this is called from PostEditChange(), actor factories, etc that are calling Clear/UpdateComponents() themselves so we only reattach stuff that is currently attached
		TArray<UActorComponent*> PreviouslyAttachedComponents;
		for (INT i = 0; i < Components.Num(); i++)
		{
			if (Components(i) != NULL && Components(i)->IsAttached())
			{
				PreviouslyAttachedComponents.AddItem(Components(i));
				Components(i)->ConditionalDetach();
			}
		}

		switch (CollisionType)
		{
			case COLLIDE_CustomDefault:
			{
					// restore to default programmer-defined settings
					AActor* DefaultActor = GetClass()->GetDefaultActor();
					bCollideActors = DefaultActor->bCollideActors;
					bBlockActors = DefaultActor->bBlockActors;
					if (DefaultActor->CollisionComponent != NULL)
					{
						CollisionComponent->CollideActors = DefaultActor->CollisionComponent->CollideActors;
						CollisionComponent->BlockActors = DefaultActor->CollisionComponent->BlockActors;
						CollisionComponent->BlockNonZeroExtent = DefaultActor->CollisionComponent->BlockNonZeroExtent;
						CollisionComponent->BlockZeroExtent = DefaultActor->CollisionComponent->BlockZeroExtent;
						CollisionComponent->BlockRigidBody = DefaultActor->CollisionComponent->BlockRigidBody;
					}
					else
					{
						debugf(NAME_Warning, TEXT("SetCollisionFromCollisionType(): class %s has no default CollisionComponent so %s's CollisionComponent cannot be reset to defaults."), *GetClass()->GetName(), *GetName());
					}
					break;
			}
			case COLLIDE_NoCollision:
				bCollideActors = FALSE;
				CollisionComponent->CollideActors = FALSE;
				CollisionComponent->BlockRigidBody = FALSE;
				break;
			case COLLIDE_BlockAll:
			case COLLIDE_BlockWeapons:
			case COLLIDE_BlockAllButWeapons:
				bCollideActors = TRUE;
				bBlockActors = TRUE;
				CollisionComponent->CollideActors = TRUE;
				CollisionComponent->BlockActors = TRUE;
				CollisionComponent->BlockNonZeroExtent = (CollisionType == COLLIDE_BlockAll || CollisionType == COLLIDE_BlockAllButWeapons);
				CollisionComponent->BlockRigidBody = CollisionComponent->BlockNonZeroExtent;
				CollisionComponent->BlockZeroExtent = (CollisionType == COLLIDE_BlockAll || CollisionType == COLLIDE_BlockWeapons);
				break;
			case COLLIDE_TouchAll:
			case COLLIDE_TouchWeapons:
			case COLLIDE_TouchAllButWeapons:
				// bWorldGeometry actors must block if they collide at all, so force the flags to respect that even if the LD tries to change them
				if (bWorldGeometry)
				{
					if (!bDisplayedWorldGeometryWarning)
					{
						appMsgf(AMT_OK, *LocalizeUnrealEd("Error_WorldGeometryMustBlock"), *GetName());
						bDisplayedWorldGeometryWarning = TRUE;
					}
					SetDefaultCollisionType();
				}
				else
				{
					bCollideActors = TRUE;
					bBlockActors = FALSE;
					CollisionComponent->CollideActors = TRUE;
					CollisionComponent->BlockActors = FALSE;
					CollisionComponent->BlockRigidBody = FALSE;
					CollisionComponent->BlockNonZeroExtent = (CollisionType == COLLIDE_TouchAll || CollisionType == COLLIDE_TouchAllButWeapons);
					CollisionComponent->BlockZeroExtent = (CollisionType == COLLIDE_TouchAll || CollisionType == COLLIDE_TouchWeapons);
				}
				break;
			default:
				debugf(NAME_Error, TEXT("%s set CollisionType to unknown value %i"), *GetFullName(), INT(CollisionType));
				bCollideActors = FALSE;
				break;
		}

		// mirror BlockRigidBody flag
		BlockRigidBody = CollisionComponent->BlockRigidBody;

		// reattach components that were previously attached
		const FMatrix& ActorToWorld = LocalToWorld();
		for (INT i = 0; i < PreviouslyAttachedComponents.Num(); i++)
		{
			if (!PreviouslyAttachedComponents(i)->IsAttached()) // might have been attached by a previous component in the list
			{
				PreviouslyAttachedComponents(i)->ConditionalAttach(GWorld->Scene, this, ActorToWorld);
			}
		}
	}
}

void AActor::Spawned()
{
	SetDefaultCollisionType();
}

//
// Get the collision cylinder extent to use when moving this actor through the level (ie. just looking at CollisionComponent)
//
FVector AActor::GetCylinderExtent() const
{
	UCylinderComponent* CylComp = Cast<UCylinderComponent>(CollisionComponent);
	
	if(CylComp)
	{
		return FVector(CylComp->CollisionRadius, CylComp->CollisionRadius, CylComp->CollisionHeight);
	}
	else
	{
		// use bounding cylinder
		FLOAT CollisionRadius, CollisionHeight;
		((AActor *)this)->GetBoundingCylinder(CollisionRadius, CollisionHeight);
		return FVector(CollisionRadius,CollisionRadius,CollisionHeight);
	}
}

//
// Get height/radius of big cylinder around this actors colliding components.
//
void AActor::GetBoundingCylinder(FLOAT& CollisionRadius, FLOAT& CollisionHeight) const
{
	FBox Box = GetComponentsBoundingBox();
	FVector BoxExtent = Box.GetExtent();

	CollisionHeight = BoxExtent.Z;
	CollisionRadius = appSqrt( (BoxExtent.X * BoxExtent.X) + (BoxExtent.Y * BoxExtent.Y) );
}

//
// Set the actor's collision properties.
//
void AActor::SetCollision(UBOOL bNewCollideActors, UBOOL bNewBlockActors, UBOOL bNewIgnoreEncroachers )
{
	// Make sure we're calling this function to change something.
	if( ( bCollideActors == bNewCollideActors )
		&& ( bBlockActors == bNewBlockActors )
		&& ( bIgnoreEncroachers == bNewIgnoreEncroachers )
		)
	{
		return;
	}

#if !FINAL_RELEASE
	// Check to see if this move is illegal during this tick group
	if( GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork )
	{
		debugf(NAME_Error,TEXT("Can't change collision on actor (%s) during async work!"),*GetName());
	}
#endif

	const UBOOL bOldCollideActors = bCollideActors;

	// Untouch everything if we're turning collision off.
	if( bCollideActors && !bNewCollideActors )
	{
		for( int i=0; i<Touching.Num(); )
		{
			if( Touching(i) )
			{
				Touching(i)->EndTouch(this, 0);
			}
			else
			{
				i++;
			}
		}
	}

	// If the collide actors flag is changing, then all collidable components
	// need to be detached and then reattached
	UBOOL bClearAndUpdate = bCollideActors != bNewCollideActors;
	if (bClearAndUpdate)
	{
		// clear only primitive components so we don't needlessly reattach components that never collide
		for (INT ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
		{
			UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Components(ComponentIndex));
			if (Primitive != NULL && Primitive->CollideActors)
			{
				Primitive->ConditionalDetach();
			}
		}
	}
	// Set properties.
	bCollideActors = bNewCollideActors;
	bBlockActors   = bNewBlockActors;
	bIgnoreEncroachers = bNewIgnoreEncroachers;
	// Collision flags changed and collidable components need to be re-added
	if (bClearAndUpdate)
	{
		ConditionalUpdateComponents();
	}

	// Touch.
	if( bNewCollideActors && !bOldCollideActors )
	{
		FindTouchingActors();
	}
	// notify script
	eventCollisionChanged();
	// dirty this actor for replication
	bNetDirty = TRUE;
}

/** UnTouchActors()
UnTouch actors which are no longer overlapping this actor
*/
void AActor::UnTouchActors()
{
	for( int i=0; i<Touching.Num(); )
	{
		if( Touching(i) && !IsOverlapping(Touching(i)) )
		{
			EndTouch( Touching(i), 0 );
		}
		else
		{
			i++;
		}
	}
}

/** FindTouchingActors()
Touch all actors which are overlapping
*/
void AActor::FindTouchingActors()
{
	FMemMark Mark(GMem);
	FLOAT ColRadius, ColHeight;
	GetBoundingCylinder(ColRadius, ColHeight);
	UBOOL bIsZeroExtent = (ColRadius == 0.f) && (ColHeight == 0.f);
	FCheckResult* FirstHit = GWorld->Hash ? GWorld->Hash->ActorEncroachmentCheck( GMem, this, Location, Rotation, TRACE_AllColliding ) : NULL;	
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

				BeginTouch( Test->Actor, Test->Component, HitLocation, NormalDir );
			}
		}						
	Mark.Pop();
}

//
// Set the collision cylinder size (if there is one).
//
void AActor::SetCollisionSize( FLOAT NewRadius, FLOAT NewHeight )
{
	UCylinderComponent* CylComp = Cast<UCylinderComponent>(CollisionComponent);

	if(CylComp)
		CylComp->SetCylinderSize(NewRadius, NewHeight);

	UnTouchActors();
	FindTouchingActors();
	// notify script
	eventCollisionChanged();
	// dirty this actor for replication
	bNetDirty = true;	
}

void AActor::SetDrawScale( FLOAT NewScale )
{
	DrawScale = NewScale;
	// mark components for an update
	MarkComponentsAsDirty();

	bNetDirty = true;	// for network replication
}

void AActor::SetDrawScale3D( FVector NewScale3D )
{
	DrawScale3D = NewScale3D;
	// mark components for an update
	MarkComponentsAsDirty();

	// not replicated!
}

/* Update relative rotation - called by ULevel::MoveActor()
 don't update RelativeRotation if attached to a bone -
 if attached to a bone, only update RelativeRotation directly
*/
void AActor::UpdateRelativeRotation()
{
	if ( !Base || Base->bWorldGeometry || (BaseBoneName != NAME_None) )
		return;

	// update RelativeRotation which is the rotation relative to the base's rotation
	RelativeRotation = (FRotationMatrix(Rotation) * FRotationMatrix(Base->Rotation).Transpose()).Rotator();
}

// Adds up the bounding box from each primitive component to give an aggregate bounding box for this actor.
FBox AActor::GetComponentsBoundingBox(UBOOL bNonColliding) const
{
	FBox Box(0);

	for(UINT ComponentIndex = 0;ComponentIndex < (UINT)this->Components.Num();ComponentIndex++)
	{
		UPrimitiveComponent*	primComp = Cast<UPrimitiveComponent>(this->Components(ComponentIndex));

		// Only use collidable components to find collision bounding box.
		if( primComp && primComp->IsAttached() && (bNonColliding || primComp->CollideActors) )
		{
			Box += primComp->Bounds.GetBox();
		}
	}

	return Box;
}

void AActor::GetComponentsBoundingBox(FBox& ActorBox) const
{
	ActorBox = GetComponentsBoundingBox();
}


/**
 * This will check to see if the Actor is still in the world.  It will check things like
 * the KillZ, SoftKillZ, outside world bounds, etc. and handle the situation.
 **/
void AActor::CheckStillInWorld()
{
	// check the variations of KillZ
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	if( Location.Z < ((WorldInfo->bSoftKillZ && Physics == PHYS_Falling) ? (WorldInfo->KillZ - WorldInfo->SoftKill) : WorldInfo->KillZ) )
	{
		eventFellOutOfWorld(WorldInfo->KillZDamageType);
	}
	// Check if box has poked outside the world
	else if( ( CollisionComponent != NULL ) && ( CollisionComponent->IsAttached() == TRUE ) )
	{
		const FBox&	Box = CollisionComponent->Bounds.GetBox();
		if(	Box.Min.X < -HALF_WORLD_MAX || Box.Max.X > HALF_WORLD_MAX ||
			Box.Min.Y < -HALF_WORLD_MAX || Box.Max.Y > HALF_WORLD_MAX ||
			Box.Min.Z < -HALF_WORLD_MAX || Box.Max.Z > HALF_WORLD_MAX )
		{
			debugf(NAME_Warning, TEXT("%s is outside the world bounds!"), *GetName());
			eventOutsideWorldBounds();
			// not safe to use physics or collision at this point
			SetCollision(FALSE, FALSE, bIgnoreEncroachers);
			setPhysics(PHYS_None);
		}
	}
}

FVector AActor::OverlapAdjust;

//
// Return whether this actor overlaps another.
// Called normally from MoveActor, to see if we should 'untouch' things.
// Normally - the only things that can overlap an actor are volumes.
// However, we also use this test during ActorEncroachmentCheck, so we support
// Encroachers (ie. movers) overlapping actors.
//
UBOOL AActor::IsOverlapping( AActor* Other, FCheckResult* Hit )
{
	checkSlow(Other!=NULL);

	if ( (this->IsBrush() && Other->IsBrush()) || (Other == GWorld->GetWorldInfo()) )
	{
		// We cannot detect whether these actors are overlapping so we say they aren't.
		return 0;
	}

	// Things dont overlap themselves
	if(this == Other)
		return 0;

	// Things that do encroaching (movers, rigid body actors etc.) can't encroach each other!
	if(this->IsEncroacher() && Other->IsEncroacher())
		return 0;

	// Things that are joined together dont overlap.
	if( this->IsBasedOn(Other) || Other->IsBasedOn(this) )
	{
		return 0;
	}

	// Can't overlap actors with collision turned off.
	if( !this->bCollideActors || !Other->bCollideActors )
		return 0;

	// Now have 2 actors we want to overlap, so we have to pick which one to treat as a box and test against the PrimitiveComponents of the other.
	AActor* PrimitiveActor = NULL;
	AActor* BoxActor = NULL;

	// For volumes, test the bounding box against the volume primitive.
	// in the volume case, we cannot test per-poly
	UBOOL bForceSimpleCollision = FALSE;
	if(this->GetAVolume())
	{
		PrimitiveActor = this;
		BoxActor = Other;
		bForceSimpleCollision = TRUE;
	}
	else if(Other->GetAVolume())
	{
		PrimitiveActor = Other;
		BoxActor = this;
		bForceSimpleCollision = TRUE;
	}
	// For Encroachers, we test the complex primitive of the mover against the bounding box of the other thing.
	else if(this->IsEncroacher())
	{
		PrimitiveActor = this;
		BoxActor = Other;	
	}
	else if(Other->IsEncroacher())
	{
		PrimitiveActor = Other;	
		BoxActor = this;
	}
	// If none of these cases, try a cylinder/cylinder overlap.
	else
	{
		// Fallback - see if collision component cylinders are overlapping.
		// @fixme - perhaps use bounding box of any collisioncomponent for this test?
		UCylinderComponent* CylComp1 = Cast<UCylinderComponent>(this->CollisionComponent);
		UCylinderComponent* CylComp2 = Cast<UCylinderComponent>(Other->CollisionComponent);

		if(CylComp1 && CylComp2)
		{
			// use OverlapAdjust because actor may have been temporarily moved
			FVector CylOrigin1 = CylComp1->GetOrigin() + OverlapAdjust;
			FVector CylOrigin2 = CylComp2->GetOrigin();

			if ( (Square(CylOrigin1.Z - CylOrigin2.Z) < Square(CylComp1->CollisionHeight + CylComp2->CollisionHeight))
			&&	(Square(CylOrigin1.X - CylOrigin2.X) + Square(CylOrigin1.Y - CylOrigin2.Y)
				< Square(CylComp1->CollisionRadius + CylComp2->CollisionRadius)) )
			{
				if( Hit )
					Hit->Component = CylComp2;
				return 1;
			}
			else
				return 0;
		}
		else
		{
			return 0;
		}
	}

	check(BoxActor);
	check(PrimitiveActor);
	check(BoxActor != PrimitiveActor);

	if(!BoxActor->CollisionComponent)
		return 0;
		
	// Check bounding box of collision component against each colliding PrimitiveComponent.
	FBox BoxActorBox = BoxActor->CollisionComponent->Bounds.GetBox();
	if(BoxActorBox.IsValid)
	{
		// adjust box position since overlap check is for different location than actor's current location
		if ( BoxActor == this )
		{
			BoxActorBox.Min += OverlapAdjust;
			BoxActorBox.Max += OverlapAdjust;
		}
		else
		{
			BoxActorBox.Min -= OverlapAdjust;
			BoxActorBox.Max -= OverlapAdjust;
		}
	}

	//GWorld->LineBatcher->DrawWireBox( BoxActorBox, FColor(255,255,0) );

	// If we failed to get a valid bounding box from the Box actor, we can't get an overlap.
	if(!BoxActorBox.IsValid)
		return 0;

	FVector BoxCenter, BoxExtent;
	BoxActorBox.GetCenterAndExtents(BoxCenter, BoxExtent);

	// If we were not supplied an FCheckResult, use a temp one.
	FCheckResult TestHit;
	if(Hit==NULL)
		Hit = &TestHit;

	// DEBUGGING: Time how long the point checks take, and print if its more than SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT.
#if defined(SHOW_SLOW_OVERLAPS) || LOOKING_FOR_PERF_ISSUES
	DWORD Time=0;
	clock(Time);
#endif

	// Check box against each colliding primitive component.
	UBOOL HaveHit = 0;
	for(UINT ComponentIndex = 0; ComponentIndex < (UINT)PrimitiveActor->Components.Num(); ComponentIndex++)
	{
		UPrimitiveComponent* primComp = Cast<UPrimitiveComponent>(PrimitiveActor->Components(ComponentIndex));

		if(primComp && primComp->CollideActors && primComp->BlockNonZeroExtent)
		{
			HaveHit = (primComp->PointCheck(*Hit, BoxCenter, BoxExtent, (BoxActor->bCollideComplex && !bForceSimpleCollision) ? TRACE_ComplexCollision : 0) == 0);
			if(HaveHit)
			{
				Hit->Component = primComp;
				break;
			}
		}
	}

#if defined(SHOW_SLOW_OVERLAPS) || LOOKING_FOR_PERF_ISSUES
	unclock(Time);
	FLOAT MSec = Time * GSecondsPerCycle * 1000.f;
	if( MSec > SHOW_SLOW_OVERLAPS_TAKING_LONG_TIME_AMOUNT )
	{
		debugf( NAME_PerfWarning, TEXT("IsOverLapping: Testing: P:%s - B:%s Time: %f"), *(PrimitiveActor->GetPathName()), *(BoxActor->GetPathName()), MSec );
	}
#endif

	return HaveHit;

}

/**
 * Sets the ticking group for this actor.
 *
 * @param NewTickGroup the new value to assign
 */
void AActor::SetTickGroup(BYTE NewTickGroup)
{
	TickGroup = NewTickGroup;
}


/*-----------------------------------------------------------------------------
	Actor touch minions.
-----------------------------------------------------------------------------*/

static UBOOL TouchTo( AActor* Actor, AActor* Other, UPrimitiveComponent* OtherComp, const FVector &HitLocation, const FVector &HitNormal)
{
	check(Actor);
	check(Other);
	check(Actor!=Other);

	// if already touching, then don't bother with further checks
	if (Actor->Touching.ContainsItem(Other))
	{
		return 1;
	}

	// check for touch sequence events
	if (GIsGame)
	{
		for (INT Idx = 0; Idx < Actor->GeneratedEvents.Num(); Idx++)
		{
			USeqEvent_Touch *TouchEvent = Cast<USeqEvent_Touch>(Actor->GeneratedEvents(Idx));
			if (TouchEvent != NULL)
			{
				TouchEvent->CheckTouchActivate(Actor,Other);
			}
		}
	}

	// Make Actor touch TouchActor.
	Actor->Touching.AddItem(Other);
	Actor->eventTouch( Other, OtherComp, HitLocation, HitNormal );

	// See if first actor did something that caused an UnTouch.
	INT i = 0;
	return ( Actor->Touching.FindItem(Other,i) );

}

//
// Note that TouchActor has begun touching Actor.
//
// This routine is reflexive.
//
// Handles the case of the first-notified actor changing its touch status.
//
void AActor::BeginTouch( AActor* Other, UPrimitiveComponent* OtherComp, const FVector &HitLocation, const FVector &HitNormal)
{
	// Perform reflective touch.
	if ( TouchTo( this, Other, OtherComp, HitLocation, HitNormal ) )
		TouchTo( Other, this, this->CollisionComponent, HitLocation, HitNormal );

}

//
// Note that TouchActor is no longer touching Actor.
//
// If NoNotifyActor is specified, Actor is not notified but
// TouchActor is (this happens during actor destruction).
//
void AActor::EndTouch( AActor* Other, UBOOL bNoNotifySelf )
{
	check(Other!=this);

	// Notify Actor.
	INT i=0;
	if ( !bNoNotifySelf && Touching.FindItem(Other,i) )
	{
		eventUnTouch( Other );
	}
	Touching.RemoveItem(Other);

	// check for untouch sequence events on both actors
	if (GIsGame)
	{
		USeqEvent_Touch *TouchEvent = NULL;
		for (INT Idx = 0; Idx < GeneratedEvents.Num(); Idx++)
		{
			TouchEvent = Cast<USeqEvent_Touch>(GeneratedEvents(Idx));
			if (TouchEvent != NULL)
			{
				TouchEvent->CheckUnTouchActivate(this,Other);
			}
		}
		for (INT Idx = 0; Idx < Other->GeneratedEvents.Num(); Idx++)
		{
			TouchEvent = Cast<USeqEvent_Touch>(Other->GeneratedEvents(Idx));
			if (TouchEvent != NULL)
			{
				TouchEvent->CheckUnTouchActivate(Other,this);
			}
		}
	}

	if ( Other->Touching.FindItem(this,i) )
	{
		Other->eventUnTouch( this );
		Other->Touching.RemoveItem(this);
	}
}

/*-----------------------------------------------------------------------------
	Relations.
-----------------------------------------------------------------------------*/

/**
 * @return		TRUE if the actor is in the named group, FALSE otherwise.
 */
UBOOL AActor::IsInGroup(const TCHAR* GroupName) const
{
	// Take the actor's group string and break it up into an array.
	TArray<FString> GroupList;
	GetGroups( GroupList );

	// Iterate over the array of group names searching for the input group.
	for( INT GroupIndex = 0 ; GroupIndex < GroupList.Num() ; ++GroupIndex )
	{
		if( GroupList( GroupIndex ) == GroupName )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Parses the actor's group string into a list of group names (strings).
 * @param		OutGroups		[out] Receives the list of group names.
 */
void AActor::GetGroups(TArray<FString>& OutGroups) const
{
	OutGroups.Empty();
	Group.ToString().ParseIntoArray( &OutGroups, TEXT(","), FALSE );
}

/** marks all PrimitiveComponents for which their Owner is relevant for visibility as dirty because the Owner of some Actor in the chain has changed
 * @param TheActor the actor to mark components dirty for
 * @param bProcessChildren whether to recursively iterate over children
 */
static void MarkOwnerRelevantComponentsDirty(AActor* TheActor)
{
	for (INT i = 0; i < TheActor->AllComponents.Num(); i++)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(TheActor->AllComponents(i));
		if (Primitive != NULL && (TheActor->bOnlyOwnerSee || Primitive->bOnlyOwnerSee || Primitive->bOwnerNoSee))
		{
			Primitive->BeginDeferredReattach();
		}
	}

	// recurse over children of this Actor
	for (INT i = 0; i < TheActor->Children.Num(); i++)
	{
		AActor* Child = TheActor->Children(i);
		if (Child != NULL && !Child->ActorIsPendingKill())
		{
			MarkOwnerRelevantComponentsDirty(Child);
		}
	}
}

//
// Change the actor's owner.
//
void AActor::SetOwner( AActor *NewOwner )
{
	if (Owner != NewOwner && !ActorIsPendingKill())
	{
		if (NewOwner != NULL && NewOwner->IsOwnedBy(this))
		{
			debugf(NAME_Error, TEXT("SetOwner(): Failed to set '%s' owner of '%s' because it would cause an Owner loop"), *NewOwner->GetName(), *GetName());
			return;
		}

		// Sets this actor's parent to the specified actor.
		AActor* OldOwner = Owner;
		if( Owner != NULL )
		{
			Owner->eventLostChild( this );
			if (OldOwner != Owner)
			{
				// LostChild() resulted in another SetOwner()
				return;
			}
			// remove from old owner's Children array
			verifySlow(Owner->Children.RemoveItem(this) == 1);
		}

		Owner = NewOwner;

		if( Owner != NULL )
		{
			// add to new owner's Children array
			checkSlow(!Owner->Children.ContainsItem(this));
			Owner->Children.AddItem(this);

			Owner->eventGainedChild( this );
			if (Owner != NewOwner)
			{
				// GainedChild() resulted in another SetOwner()
				return;
			}
		}

		// mark all components for which Owner is relevant for visibility to be updated
		MarkOwnerRelevantComponentsDirty(this);

		bNetDirty = TRUE;
	}
}

/** changes the value of bOnlyOwnerSee
 * @param bNewOnlyOwnerSee the new value to assign to bOnlyOwnerSee
 */
void AActor::SetOnlyOwnerSee(UBOOL bNewOnlyOwnerSee)
{
	bOnlyOwnerSee = bNewOnlyOwnerSee;
	MarkComponentsAsDirty(FALSE);
}

/**
 *	Change the actor's base.
 *	If you are a attaching to a SkeletalMeshComponent that is using another SkeletalMeshComponent for its bone transforms (via the ParentAnimComponent pointer)
 *	you should base the Actor on that component instead.
 *	AttachName is checks for a socket w/ that name first otherwise uses it as a direct bone name
 */
void AActor::SetBase( AActor* NewBase, FVector NewFloor, int bNotifyActor, USkeletalMeshComponent* SkelComp, FName AttachName )
{
	// Verify no recursion.
	for( AActor* Loop=NewBase; Loop!=NULL; Loop=Loop->Base )
	{
		if( Loop == this )
		{
			debugf(TEXT(" SetBase failed! Recursion detected. Actor %s already based on %s."), *GetName(), *NewBase->GetName());
			return;
		}
	}

	// Don't allow static actors to be based on movable or deleted actors
	if( NewBase != NULL && (bStatic || !bMovable) && ((!NewBase->bStatic && NewBase->bMovable) || NewBase->bDeleteMe) )
	{
		debugf(TEXT("SetBase failed! Cannot base static actor %s on moveable actor %s"), *GetName(), *NewBase->GetName());
		return;
	}

	// If anything is different from current base, update the based information.
	if( (NewBase != Base) || (SkelComp != BaseSkelComponent) || (AttachName != BaseBoneName) )
	{
		//debugf(TEXT("%3.2f SetBase %s -> %s, SkelComp: %s, AttachName: %s"), GWorld->GetTimeSeconds(), *GetName(), NewBase ? *NewBase->GetName() : TEXT("NULL"), *SkelComp->GetName(), *AttachName.ToString());

		// Notify old base, unless it's the level or terrain (but not movers).
		if( Base && !Base->bWorldGeometry )
		{
			INT RemovalCount = Base->Attached.RemoveItem(this);
			//@fixme - disabled check for editor since it was being triggered during import, is this safe?
			checkf(!GIsGame || RemovalCount <= 1, TEXT("%s was in Attached array of %s multiple times!"), *GetFullName(), *Base->GetFullName()); // Verify that an actor wasn't attached multiple times. @todo: might want to also check > 0?
			Base->eventDetach( this );
		}

		// Set base.
		Base = NewBase;
		BaseSkelComponent = NULL;
		BaseBoneName = NAME_None;

		if ( Base && !Base->bWorldGeometry )
		{
			if ( !bHardAttach || (Role == ROLE_Authority) )
			{
				RelativeLocation = Location - Base->Location;
				UpdateRelativeRotation();
			}

			// If skeletal case, check bone is valid before we try and attach.
			INT BoneIndex = INDEX_NONE;

			// Check to see if it is a socket first
			USkeletalMeshSocket* Socket = (SkelComp && SkelComp->SkeletalMesh) ? SkelComp->SkeletalMesh->FindSocket( AttachName ) : NULL;
			if( Socket )
			{
				// Use socket bone name
				AttachName = Socket->BoneName;
			}

			if( SkelComp && (AttachName != NAME_None) )
			{
				// Check we are not trying to base on a bone of a SkeletalMeshComponent that is 
				// using another SkeletalMeshComponent for its bone transforms.
				if(SkelComp->ParentAnimComponent)
				{
					debugf( 
						TEXT("SkeletalMeshComponent %s in Actor %s has a ParentAnimComponent - should attach Actor %s to that instead."), 
						*SkelComp->GetPathName(),
						*Base->GetPathName(),
						*GetPathName()
						);
				}
				else
				{
					BoneIndex = SkelComp->MatchRefBone(AttachName);
					if(BoneIndex == INDEX_NONE)
					{
						debugf( TEXT("AActor::SetBase : Bone (%s) not found on %s for %s!"), *AttachName.ToString() , *Base->GetName(), *GetName());					
					}
				}
			}

			// Bone exists and component is successfully initialized, so remember offset from it.
			if(BoneIndex != INDEX_NONE && SkelComp->IsAttached() && SkelComp->GetOwner() != NULL)
			{				
				check(SkelComp);
				check(SkelComp->GetOwner() == Base); // SkelComp should always be owned by the Actor we are basing on.

				if( Socket )
				{
					RelativeLocation = Socket->RelativeLocation;
					RelativeRotation = Socket->RelativeRotation;
				}
				else
				{
					// Get transform of bone we wish to attach to.
					FMatrix BaseTM = SkelComp->GetBoneMatrix(BoneIndex);
					BaseTM.RemoveScaling();
					FMatrix BaseInvTM = BaseTM.Inverse();

					FRotationTranslationMatrix ChildTM(Rotation,Location);

					// Find relative transform of actor from its base bone, and store it.
					FMatrix HardRelMatrix =  ChildTM * BaseInvTM;
					RelativeLocation = HardRelMatrix.GetOrigin();
					RelativeRotation = HardRelMatrix.Rotator();
				}

				BaseSkelComponent = SkelComp;
				BaseBoneName = AttachName;
			}
			// Calculate the transform of this actor relative to its base.
			else if(bHardAttach)
			{
				FMatrix BaseInvTM = FTranslationMatrix(-Base->Location) * FInverseRotationMatrix(Base->Rotation);
				FRotationTranslationMatrix ChildTM(Rotation, Location);

				FMatrix HardRelMatrix =  ChildTM * BaseInvTM;
				RelativeLocation = HardRelMatrix.GetOrigin();
				RelativeRotation = HardRelMatrix.Rotator();
			}
		}

		// Notify new base, unless it's the level.
		if( Base && !Base->bWorldGeometry )
		{
			Base->Attached.AddItem(this);
			Base->eventAttach( this );
		}

		// Notify this actor of his new floor.
		if ( bNotifyActor )
		{
			eventBaseChange();
		}
	}
}

//
// Determine if BlockingActor should block actors of the given class.
// This routine needs to be reflexive or else it will create funky
// results, i.e. A->IsBlockedBy(B) <-> B->IsBlockedBy(A).
//
UBOOL AActor::IsBlockedBy( const AActor* Other, const UPrimitiveComponent* Primitive ) const
{
	checkSlow(this!=NULL);
	checkSlow(Other!=NULL);

	if(Primitive && !Primitive->BlockActors)
		return 0;

	if( Other->bWorldGeometry )
		return bCollideWorld && Other->bBlockActors;
	else if ( Other->IgnoreBlockingBy((AActor *)this) )
		return false;
	else if ( IgnoreBlockingBy((AActor*)Other) )
		return false;
	else if( Other->IsBrush() || Other->IsEncroacher() )
		return bCollideWorld && Other->bBlockActors;
	else if ( IsBrush() || IsEncroacher() )
		return Other->bCollideWorld && bBlockActors;
	else
		return Other->bBlockActors && bBlockActors;

}

UBOOL AActor::IgnoreBlockingBy( const AActor *Other ) const
{
	if ( bIgnoreEncroachers && Other->IsEncroacher() )
		return true;
	return false;
}

UBOOL ABlockingVolume::IgnoreBlockingBy( const AActor *Other ) const
{
	return ( Other->GetAProjectile() != NULL );
}

UBOOL APawn::IgnoreBlockingBy( const AActor *Other ) const
{
	return ((Physics == PHYS_RigidBody && Other->bIgnoreRigidBodyPawns) || (bIgnoreEncroachers && Other->IsEncroacher()));
}

UBOOL APlayerController::IgnoreBlockingBy( const AActor *Other ) const
{
	// allow playercontrollers (acting as cameras) to go through movers
	if ( (Other->Physics == PHYS_RigidBody) && !Other->IsA(AVehicle::StaticClass()) )
		return true;
	if ( bIgnoreEncroachers && Other->IsEncroacher() )
		return true;
	return false;
}

UBOOL AProjectile::IgnoreBlockingBy( const AActor *Other ) const
{
	if ( bIgnoreEncroachers && Other->IsEncroacher() )
		return true;
	if ( !bBlockedByInstigator && Other == Instigator )
		return true;

	return false;
}

void APawn::EditorApplyRotation(const FRotator& DeltaRotation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	Super::EditorApplyRotation(DeltaRotation, bAltDown, bShiftDown, bCtrlDown);

	// Forward new rotation on to the pawn's controller.
	if( Controller )
	{
		Controller->Rotation = Rotation;
	}
}

void APawn::SetBase( AActor* NewBase, FVector NewFloor, int bNotifyActor, USkeletalMeshComponent* SkelComp, FName AttachName )
{
	Floor = NewFloor;
	Super::SetBase(NewBase,NewFloor,bNotifyActor,SkelComp,AttachName);
}

/** Add a data point to a line on the global debug chart. */
void AActor::ChartData(const FString& DataName, FLOAT DataValue)
{
	if(GStatChart)
	{
		// Make graph line name by concatenating actor name with data name.
		FString LineName = FString::Printf(TEXT("%s_%s"), *GetName(), *DataName);

		GStatChart->AddDataPoint(LineName, DataValue);
	}
}

//
void AActor::SetHidden(UBOOL bNewHidden)
{
	if (bHidden != bNewHidden)
	{
		bHidden = bNewHidden;
		bNetDirty = TRUE;
		MarkComponentsAsDirty(FALSE);
	}
}

UBOOL AActor::IsPlayerOwned()
{
    AActor* TopActor = GetTopOwner();
	AController* C = TopActor ? TopActor->GetAController() : NULL;
	return C? C->IsPlayerOwner() : FALSE;
}

/**
 * Returns TRUE if this actor is contained by TestLevel.
 * @todo seamless: update once Actor->Outer != Level
 */
UBOOL AActor::IsInLevel(const ULevel *TestLevel) const
{
	return (GetOuter() == TestLevel);
}

/** Return the ULevel that this Actor is part of. */
ULevel* AActor::GetLevel() const
{
	return CastChecked<ULevel>( GetOuter() );
}

/**
 * Determine whether this actor is referenced by its level's GameSequence.
 *
 * @param	pReferencer		if specified, will be set to the SequenceObject that is referencing this actor.
 *
 * @return TRUE if this actor is referenced by kismet.
 */
UBOOL AActor::IsReferencedByKismet( USequenceObject** pReferencer/*=NULL*/ ) const
{
	USequence* Sequence = GIsGame ? GWorld->GetGameSequence() : GetLevel()->GetGameSequence();
	return ( Sequence && Sequence->ReferencesObject(this, pReferencer) );
}

/*-----------------------------------------------------------------------------
	Special editor support.
-----------------------------------------------------------------------------*/

AActor* AActor::GetHitActor()
{
	return this;
}

// Determines if this actor is hidden in the editor viewports or not.

UBOOL AActor::IsHiddenEd() const
{
	// If any of the standard hide flags are set, return 1

	if( bHiddenEd || bHiddenEdGroup || bHiddenEdCustom )
		return 1;

	// If this actor is part of a layer and that layer is hidden, return 1

	if( Layer && Layer->bVisible == 0 )
		return 1;

	// Otherwise, it's visible

	return 0;
}

// Get the name of the map from the last used URL
FString AActor::GetURLMap()
{
	if (!GIsEditor)
	{
		return CastChecked<UGameEngine>(GEngine)->LastURL.Map;
	}
	else
	{
		//@fixme - figure out map name from editor
		return FString(TEXT(""));
	}
}

/*---------------------------------------------------------------------------------------
	Brush class implementation.
---------------------------------------------------------------------------------------*/

/**
 * Serialize function
 *
 * @param Ar Archive to serialize with
 */
void ABrush::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if( Ar.Ver() < VER_FIXED_BRUSH_POLYFLAGS )
	{
		PolyFlags &= ~PF_ModelComponentMask;
	}
}

void ABrush::PostEditChange(UProperty* PropertyThatChanged)
{
	if(Brush)
	{
		Brush->BuildBound();
	}
	Super::PostEditChange(PropertyThatChanged);
}

void ABrush::CopyPosRotScaleFrom( ABrush* Other )
{
	check(BrushComponent);
	check(Brush);
	check(Other);
	check(Other->BrushComponent);
	check(Other->Brush);

	Location    = Other->Location;
	Rotation    = Other->Rotation;
	PrePivot	= Other->PrePivot;

	Brush->BuildBound();
	ClearComponents();
	ConditionalUpdateComponents();
}

/**
 * Return whether this actor is a builder brush or not.
 *
 * @return TRUE if this actor is a builder brush, FALSE otherwise
 */
UBOOL ABrush::IsABuilderBrush() const
{
	if(GIsGame)
	{
		return FALSE;
	}
	else
	{
		return GetLevel()->GetBrush() == this;
	}
}

/**
 * Return whether this actor is the current builder brush or not
 *
 * @return TRUE if this actor is the current builder brush, FALSE otherwise
 */
UBOOL ABrush::IsCurrentBuilderBrush() const
{
	if(GIsGame)
	{
		return FALSE;
	}
	else
	{
		return GWorld->GetBrush() == this;
	}
}

void ABrush::InitPosRotScale()
{
	check(BrushComponent);
	check(Brush);
	
	Location  = FVector(0,0,0);
	Rotation  = FRotator(0,0,0);
	PrePivot  = FVector(0,0,0);

}
void ABrush::PostLoad()
{
	Super::PostLoad();
#if !CONSOLE
	if ( GIsEditor && !Rotation.IsZero() )
	{
		// Only path up the rotation if this is a level-placed actor (ie not a prefab).
		if ( GetOuter() && GetOuter()->IsA(ULevel::StaticClass()) )
		{
			if ( IsVolumeBrush() )
			{
				const FRotator OldRotation = Rotation;
				Rotation = FRotator(0,0,0);
				FBSPOps::RotateBrushVerts( this, OldRotation, FALSE );
			}
			else
			{
				Rotation = FRotator(0,0,0);
			}

			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s: brush rotation fixed"),*GetName()) );
		}
	}

	// Assign the default material to brush polys with NULL material references.
	if ( Brush && Brush->Polys )
	{
		if ( IsStaticBrush() )
		{
			for( INT PolyIndex = 0 ; PolyIndex < Brush->Polys->Element.Num() ; ++PolyIndex )
			{
				FPoly& CurrentPoly = Brush->Polys->Element(PolyIndex);
				if ( !CurrentPoly.Material )
				{
					CurrentPoly.Material = GEngine->DefaultMaterial;
				}
			}
		}
	}
#endif
}

FColor ABrush::GetWireColor() const
{
	FColor Color = GEngine->C_BrushWire;

	if( IsStaticBrush() )
	{
		Color = bColored ?						BrushColor :
				CsgOper == CSG_Subtract ?		GEngine->C_SubtractWire :
				CsgOper != CSG_Add ?			GEngine->C_BrushWire :
				(PolyFlags & PF_Portal) ?		GEngine->C_SemiSolidWire :
				(PolyFlags & PF_NotSolid) ?		GEngine->C_NonSolidWire :
				(PolyFlags & PF_Semisolid) ?	GEngine->C_ScaleBoxHi :
												GEngine->C_AddWire;
	}
	else if( IsVolumeBrush() )
	{
		Color = bColored ? BrushColor : GEngine->C_Volume;
	}

	return Color;
}

/*---------------------------------------------------------------------------------------
	Tracing check implementation.
	ShouldTrace() returns true if actor should be checked for collision under the conditions
	specified by traceflags
---------------------------------------------------------------------------------------*/

UBOOL AActor::ShouldTrace(UPrimitiveComponent* Primitive,AActor *SourceActor, DWORD TraceFlags)
{
	return ( bWorldGeometry && (TraceFlags & TRACE_LevelGeometry) )
			|| ( !bWorldGeometry && (TraceFlags & TRACE_Others) 
				&& ((TraceFlags & TRACE_OnlyProjActor) 
					? (bProjTarget || (bBlockActors && Primitive->BlockActors)) 
					: (!(TraceFlags & TRACE_Blocking) || (SourceActor && SourceActor->IsBlockedBy(this,Primitive)))) );
}

UBOOL AInterpActor::ShouldTrace(UPrimitiveComponent *Primitive,AActor *SourceActor, DWORD TraceFlags)
{
	return (TraceFlags & TRACE_Movers) 
				&& ((TraceFlags & TRACE_OnlyProjActor) 
					? (bProjTarget || (bBlockActors && Primitive->BlockActors)) 
					: (!(TraceFlags & TRACE_Blocking) || (SourceActor && SourceActor->IsBlockedBy(this,Primitive))));
}

void AInterpActor::CheckForErrors()
{
	Super::CheckForErrors();
	if ( Base && Base->Physics == PHYS_RigidBody )
	{
		if ( StaticMeshComponent && StaticMeshComponent->BlockRigidBody ) // GWarn'd by ADynamicSMActor::CheckForErrors()
		{
			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : InterpActor's StaticMeshComponent has BlockRigidBody, but is based on an actor with PHYS_RigidBody"), *GetName() ), MCACTION_NONE, TEXT("BlockRigidBodyWithRigidBody") );
		}
	}
}

UBOOL AVolume::ShouldTrace(UPrimitiveComponent* Primitive,AActor *SourceActor, DWORD TraceFlags)
{
	return ( bWorldGeometry && (TraceFlags & TRACE_LevelGeometry) )
			|| ( !bWorldGeometry && (TraceFlags & TRACE_Volumes) 
				&& ((TraceFlags & TRACE_OnlyProjActor) 
					? (bProjTarget || (bBlockActors && Primitive->BlockActors)) 
					: (!(TraceFlags & TRACE_Blocking) || (SourceActor && SourceActor->IsBlockedBy(this,Primitive)))) );
}

void AVolume::PostEditImport()
{
	Super::PostEditImport();

#if !CONSOLE
	FBSPOps::csgPrepMovingBrush( this );
#endif // !CONSOLE
}

/**
 * Serialize function.
 *
 * @param	Ar	Archive to serialize with.
 */
void ALevelStreamingVolume::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	
	// Convert deprecated value to new usage type.
	if( Ar.Ver() < VER_ADDED_LEVELSTREAMINGVOLUME_USAGE )
	{
		if( bMakeVisibleAfterLoad )
		{
			Usage = SVB_LoadingAndVisibility;
			bMakeVisibleAfterLoad = FALSE;
		}
		else
		{
			Usage = SVB_Loading;
		}
	}
}

/**
 * Called after change has occured - used to force update of contained primitives.
 */
void ACullDistanceVolume::PostEditChange( UProperty* PropertyThatChanged )
{
	Super::PostEditChange( PropertyThatChanged );
	GWorld->UpdateCullDistanceVolumes();
}

/**
 * bFinished is FALSE while the actor is being continually moved, and becomes TRUE on the last call.
 * This can be used to defer computationally intensive calculations to the final PostEditMove call of
 * eg a drag operation.
 */
void ACullDistanceVolume::PostEditMove(UBOOL bFinished)
{
	Super::PostEditMove(bFinished);
	if( bFinished )
	{
		GWorld->UpdateCullDistanceVolumes();
	}
}

/**
 * Returns whether the passed in primitive can be affected by cull distance volumes.
 *
 * @param	PrimitiveComponent	Component to test
 * @return	TRUE if tested component can be affected, FALSE otherwise
 */
UBOOL ACullDistanceVolume::CanBeAffectedByVolumes( UPrimitiveComponent* PrimitiveComponent )
{
	AActor* Owner = PrimitiveComponent ? PrimitiveComponent->GetOwner() : NULL;

	// Require an owner so we can use its location
	if(	Owner
	// Disregard movable actors
	&& !Owner->bMovable 
	// Disregard prefabs.
	&& !PrimitiveComponent->IsTemplate()
	// Skip primitives that have bHiddenGame set as we don't want to cull out brush rendering or other helper objects.
	&&	PrimitiveComponent->HiddenGame == FALSE
	// Only operate on primitives attached to current world.				
	&&	PrimitiveComponent->GetScene() == GWorld->Scene 
	// Require cull distance volume support to be enabled.
	&&	PrimitiveComponent->bAllowCullDistanceVolume )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}	
}

/**
 * Updates primitives affected by this volume.
 */
void ACullDistanceVolume::UpdateVolume()
{
	// Nothing to do if there is no brush component or no cull distances are set
	if( BrushComponent && CullDistances.Num() > 0 && bEnabled )
	{
		// Test center of mesh bounds to see whether it is encompassed by volume
		// and propagate cull distance if it is.
		for( TObjectIterator<UPrimitiveComponent> It; It; ++It )
		{
			UPrimitiveComponent*	PrimitiveComponent	= *It;
			AActor*					Owner				= PrimitiveComponent->GetOwner();

			// Check whether primitive can be affected by cull distance volumes.
			if(	ACullDistanceVolume::CanBeAffectedByVolumes( PrimitiveComponent ) )
			{
				// Check whether primitive supports cull distance volumes and its center point is being encompassed by this volume.
				if( Encompasses( Owner->Location ) )
				{		
					// Find best match in CullDistances array.
					FLOAT PrimitiveSize			= PrimitiveComponent->Bounds.SphereRadius * 2;
					FLOAT CurrentError			= FLT_MAX;
					FLOAT CurrentCullDistance	= 0;
					for( INT CullDistanceIndex=0; CullDistanceIndex<CullDistances.Num(); CullDistanceIndex++ )
					{
						const FCullDistanceSizePair& CullDistancePair = CullDistances(CullDistanceIndex);
						if( Abs( PrimitiveSize - CullDistancePair.Size ) < CurrentError )
						{
							CurrentError		= Abs( PrimitiveSize - CullDistancePair.Size );
							CurrentCullDistance = CullDistancePair.CullDistance;
						}
					}

					// LD or other volume specified cull distance, use minimum of current and one used for this volume.
					if( PrimitiveComponent->CachedCullDistance > 0 )
					{
						PrimitiveComponent->CachedCullDistance = Min( PrimitiveComponent->CachedCullDistance, CurrentCullDistance );
					}
					// LD didn't specify cull distance, use current setting directly.
					else
					{
						PrimitiveComponent->CachedCullDistance = CurrentCullDistance;
					}
				}
			}
		}
	}
}

UBOOL APhysicsVolume::ShouldTrace(UPrimitiveComponent* Primitive,AActor *SourceActor, DWORD TraceFlags)
{
	return ((TraceFlags & TRACE_PhysicsVolumes) || Super::ShouldTrace(Primitive,SourceActor,TraceFlags));
}

static inline UBOOL AllComponentsEqual(const FVector& Vec, FLOAT Tolerance=KINDA_SMALL_NUMBER)
{
	return Abs( Vec.X - Vec.Y ) < Tolerance && Abs( Vec.X - Vec.Z ) < Tolerance && Abs( Vec.Y - Vec.Z ) < Tolerance;
}

void ATrigger::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
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
		if ( !AllComponentsEqual(ModifiedScale) )
		{
			CylinderComponent->CollisionHeight += -ModifiedScale.Z;
			CylinderComponent->CollisionHeight = Max( 0.0f, CylinderComponent->CollisionHeight );

			CylinderComponent->CollisionRadius += ModifiedScale.Y;
			CylinderComponent->CollisionRadius = Max( 0.0f, CylinderComponent->CollisionRadius );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void ApplyScaleToFloat(FLOAT& Dst, const FVector& DeltaScale, FLOAT Magnitude)
{
	const FLOAT Multiplier = ( DeltaScale.X > 0.0f || DeltaScale.Y > 0.0f || DeltaScale.Z > 0.0f ) ? Magnitude : -Magnitude;
	Dst += Multiplier * DeltaScale.Size();
	Dst = Max( 0.0f, Dst );
}

void APointLight::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>( LightComponent );
	check( PointLightComponent );

	const FVector ModifiedScale = DeltaScale * 500.0f;

	ApplyScaleToFloat( PointLightComponent->Radius, ModifiedScale, 1.0f );
	PostEditChange( NULL );
}

void ASpotLight::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>( LightComponent );
	check( SpotLightComponent );
	const FVector ModifiedScale = DeltaScale * 500.0f;

	if ( bCtrlDown )
	{
		ApplyScaleToFloat( SpotLightComponent->OuterConeAngle, ModifiedScale, 0.01f );
		SpotLightComponent->OuterConeAngle = Min( 89.0f, SpotLightComponent->OuterConeAngle );
		SpotLightComponent->InnerConeAngle = Min( SpotLightComponent->OuterConeAngle, SpotLightComponent->InnerConeAngle );
	}
	else if ( bAltDown )
	{
		ApplyScaleToFloat( SpotLightComponent->InnerConeAngle, ModifiedScale, 0.01f );
		SpotLightComponent->InnerConeAngle = Min( 89.0f, SpotLightComponent->InnerConeAngle );
		SpotLightComponent->OuterConeAngle = Max( SpotLightComponent->OuterConeAngle, SpotLightComponent->InnerConeAngle );
	}
	else
	{
		ApplyScaleToFloat( SpotLightComponent->Radius, ModifiedScale, 1.0f );
	}

	PostEditChange( NULL );
}

/**
 * Changes the bHardAttach value. First checks to see if this actor is based
 * on another actor. If so, the actor is "re-based" so that the hard attach
 * will take effect.
 *
 * @param bNewHardAttach the new hard attach setting
 */
void AActor::SetHardAttach(UBOOL bNewHardAttach)
{
	if (bNewHardAttach != bHardAttach)
	{
		AActor* OldBase = Base;
		// If this actor is already based, it needs to rebase itself so that
		// the matrix is properly updated
		if (OldBase != NULL)
		{
			// Cache the other settings
			USkeletalMeshComponent* OldBaseSkelComponent = BaseSkelComponent;
			FName OldBaseBoneName = BaseBoneName;
			// Clear the old base
			SetBase(NULL,FVector(0.f,0.f,1.f),0,NULL,NAME_None);
			bHardAttach = bNewHardAttach;
			// "Re-base" to the old base
			SetBase(OldBase,FVector(0.f,0.f,1.f),0,OldBaseSkelComponent,OldBaseBoneName);
		}
		else
		{
			bHardAttach = bNewHardAttach;
		}
	}
}

/**
 * Changes the bHardAttach value. First checks to see if this actor is based
 * on another actor. If so, the actor is "re-based" so that the hard attach
 * will take effect.
 *
 * @param bNewHardAttach the new hard attach setting
 */
void APawn::SetHardAttach(UBOOL bNewHardAttach)
{
	if (bNewHardAttach != bHardAttach)
	{
		AActor* OldBase = Base;
		// If this actor is already based, it needs to rebase itself so that
		// the matrix is properly updated
		if (OldBase != NULL)
		{
			// Cache the other settings
			USkeletalMeshComponent* OldBaseSkelComponent = BaseSkelComponent;
			FName OldBaseBoneName = BaseBoneName;
			// Pawns also use the floor setting
			FVector OldFloor = Floor;
			// Clear the old base
			SetBase(NULL,FVector(0.f,0.f,1.f),0,NULL,NAME_None);
			bHardAttach = bNewHardAttach;
			// "Re-base" to the old base
			SetBase(OldBase,OldFloor,0,OldBaseSkelComponent,OldBaseBoneName);
		}
		else
		{
			bHardAttach = bNewHardAttach;
		}
	}
}

/** @return the optimal location to fire weapons at this actor */
FVector AActor::GetTargetLocation(AActor* RequestedBy)
{
	return Location;
}

/** 
* @param ReplicatedActor
* @param ActorOwner - actor which should be considered the owner of ReplicatedActor (normally ReplicatedActor->Owner)
* @return true if this actor should be considered relevancy owner for ReplicatedActor, which has bOnlyRelevantToOwner=true
*/
UBOOL AActor::IsRelevancyOwnerFor(AActor* ReplicatedActor, AActor* ActorOwner)
{
	return (ActorOwner == this);
}

/** adds/removes a property from a list of properties that will always be replicated when this Actor is bNetInitial, even if the code thinks
 * the client has the same value the server already does
 * This is a workaround to the problem where an LD places an Actor in the level, changes a replicated variable away from the defaults,
 * then at runtime the variable is changed back to the default but it doesn't replicate because initial replication is based on class defaults
 * Only has an effect when called on bStatic or bNoDelete Actors
 * Only properties already in the owning class's replication block may be specified
 * @param PropToReplicate the property to add or remove to the list
 * @param bAdd true to add the property, false to remove the property
 */
void AActor::SetForcedInitialReplicatedProperty(UProperty* PropToReplicate, UBOOL bAdd)
{
	if (bStatic || bNoDelete)
	{
		if (PropToReplicate == NULL)
		{
			debugf(NAME_Warning, TEXT("None passed into SetForcedInitialReplicatedProperty()"));
		}
		else if (!GetClass()->IsChildOf(PropToReplicate->GetOwnerClass()))
		{
			debugf(NAME_Warning, TEXT("Property '%s' passed to SetForcedInitialReplicatedProperty() that is not a member of Actor '%s'"), *PropToReplicate->GetPathName(), *GetName());
		}
		else if (!(PropToReplicate->PropertyFlags & CPF_Net))
		{
			debugf(NAME_Warning, TEXT("Property '%s' passed to SetForcedInitialReplicatedProperty() is not in the replication block for its class"), *PropToReplicate->GetPathName());
		}
		else if (WorldInfo->NetMode != NM_Client && WorldInfo->NetMode != NM_Standalone && GWorld->NetDriver != NULL)
		{
			TArray<UProperty*>* PropArray = GWorld->NetDriver->ForcedInitialReplicationMap.Find(this);
			if (bAdd)
			{
				if (PropArray == NULL)
				{
					// add an entry for this Actor
					TArray<UProperty*> NewArray;
					NewArray.AddItem(PropToReplicate);
					GWorld->NetDriver->ForcedInitialReplicationMap.Set(this, NewArray);
				}
				else
				{
					// add to list for this Actor
					PropArray->AddUniqueItem(PropToReplicate);
				}
			}
			else if (PropArray != NULL)
			{
				PropArray->RemoveItem(PropToReplicate);
				if (PropArray->Num() == 0)
				{
					GWorld->NetDriver->ForcedInitialReplicationMap.Remove(this);
				}
			}
		}
	}
}

/**
 * Returns true if the id from the other PRI matches this PRI's id
 *
 * @param OtherPRI the PRI to compare IDs with
 */
UBOOL APlayerReplicationInfo::AreUniqueNetIdsEqual(APlayerReplicationInfo* OtherPRI)
{
	return OtherPRI ? (const QWORD&)UniqueId.Uid == (const QWORD&)OtherPRI->UniqueId.Uid : FALSE;
}

/*
 * Tries to position a box to avoid overlapping world geometry.
 * If no overlap, the box is placed at SpotLocation, otherwise the position is adjusted
 * @Parameter BoxExtent is the collision extent (X and Y=CollisionRadius, Z=CollisionHeight)
 * @Parameter SpotLocation is the position where the box should be placed.  Contains the adjusted location if it is adjusted.
 * @Return true if successful in finding a valid non-world geometry overlapping location
 */
UBOOL AActor::FindSpot(FVector BoxExtent, FVector& SpotLocation)
{
	return GWorld->FindSpot(BoxExtent, SpotLocation, bCollideComplex);
}


