/*=============================================================================
	UnController.cpp: AI implementation

  This contains both C++ methods (movement and reachability), as well as some
  AI related natives

	Copyright 1998-2007 Epic Games, Inc. This software is a trade secret.
=============================================================================*/
#include "EnginePrivate.h"
#include "UnNet.h"
#include "FConfigCacheIni.h"
#include "UnPath.h"
#include "EngineAIClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "EngineAudioDeviceClasses.h"

IMPLEMENT_CLASS(UCheatManager);

/** GetNetPriority()
@param Viewer		PlayerController owned by the client for whom net priority is being determined
@param InChannel	Channel on which this actor is being replicated.
@param Time			Time since actor was last replicated
@return				Priority of this actor for replication
*/
FLOAT APlayerController::GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, FLOAT Time, UBOOL bLowBandwidth)
{
	if ( Viewer == this )
		Time *= 4.f;
	return NetPriority * Time;
}

UBOOL APlayerController::CheckSpeedHack(FLOAT NewDeltaTime)
{
	UBOOL Result = 1;

	FLOAT DefaultMaxTimeMargin = ((AGameInfo *)(AGameInfo::StaticClass()->GetDefaultActor()))->MaxTimeMargin;
	if ( (ServerTimeStamp > 0.f) && (DefaultMaxTimeMargin > 0.f) )
    {
 		if ( GWorld->GetTimeSeconds() - ServerTimeStamp > 0.3f )
		{
			TimeMargin = 0.f;
			MaxTimeMargin = DefaultMaxTimeMargin;
		}
		else if ( (TimeMargin > MaxTimeMargin) && (MaxTimeMargin < 0.2f) )
        {
			if ( MaxTimeMargin == 0.f )
			{
				MaxTimeMargin = DefaultMaxTimeMargin;
			}
			else
			{
				// player is too far ahead - make them stand still for a while
				if ( Pawn )
					Pawn->Velocity = FVector(0.f,0.f,0.f);
				TimeMargin -= 0.5f * (GWorld->GetTimeSeconds() - ServerTimeStamp);
				if ( TimeMargin < MaxTimeMargin )
				{
					MaxTimeMargin = DefaultMaxTimeMargin;
				}
				else
				{
					MaxTimeMargin = 0.1f;
				}
				Result = 0;
			}
        }
		else
		{
			FLOAT DefaultMinTimeMargin = ((AGameInfo *)(AGameInfo::StaticClass()->GetDefaultActor()))->MinTimeMargin;
			if ( TimeMargin < DefaultMinTimeMargin )
			{			
				TimeMargin = 0.7f * DefaultMinTimeMargin;
			}
			else if ( TimeMargin < -0.3f )
			{
				TimeMargin = TimeMargin + NewDeltaTime - (GWorld->GetTimeSeconds() - ServerTimeStamp) - 0.002f;
			}
			else
			{
				TimeMargin = TimeMargin + NewDeltaTime - ((AGameInfo *)(AGameInfo::StaticClass()->GetDefaultActor()))->TimeMarginSlack * (GWorld->GetTimeSeconds() - ServerTimeStamp);
				if ( TimeMargin > 0.f )
				{
					TimeMargin -= 0.002f;
				}
			}
            
  			// if still same tick on server, don't trip detection
			if ( GWorld->GetTimeSeconds() != ServerTimeStamp )
			{
				if ( TimeMargin > MaxTimeMargin )
				{
					MaxTimeMargin = 0.1f;
					// *(DWORD*)Result = 0; // commented out to give him one tick of grace, in case it gets reset
				}
				else 
				{
					MaxTimeMargin = DefaultMaxTimeMargin;
				}
			}
		}
    }

	return Result;
}

/**
 * Called when a level with path nodes is being removed.  Clear any path refs so that the level can be garbage collected.
 */
void AController::ClearCrossLevelPaths(ULevel *Level)
{
	if (Pawn != NULL)
	{
		if (Pawn->Anchor != NULL && Pawn->Anchor->IsInLevel(Level))
		{
			Pawn->SetAnchor(NULL);
		}
		if (Pawn->LastAnchor != NULL && Pawn->LastAnchor->IsInLevel(Level))
		{
			Pawn->LastAnchor = NULL;
		}
	}
	for (INT Idx = 0; Idx < RouteCache.Num(); Idx++)
	{
		ANavigationPoint *Nav = RouteCache(Idx);
		if (Nav != NULL && Nav->IsInLevel(Level))
		{
			RouteCache_Empty();
			GetStateFrame()->LatentAction = 0;
			break;
		}
	}
	if (MoveTarget != NULL && MoveTarget->IsInLevel(Level))
	{
		MoveTarget = NULL;
		GetStateFrame()->LatentAction = 0;
	}
	if (CurrentPath != NULL && CurrentPath->Start != NULL && CurrentPath->Start->IsInLevel(Level))
	{
		CurrentPath = NULL;
	}
	if (NextRoutePath != NULL && NextRoutePath->Start != NULL && NextRoutePath->Start->IsInLevel(Level))
	{
		NextRoutePath = NULL;
	}
}

/**
 * Serialize function
 *
 * @param Ar Archive to serialize with
 */
void AController::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );
	// Work around a bug in previous engine versions where the controller was serialized to disk outside of save games.
	if( Ar.Ver() < VER_SAVE_PRECOOK_PHYS_VERSION )
	{
		// Marks the Actor as pending kill so garbage collection will delete references to it and PostBeginPlay won't 
		// attach the controller to the controller list.
		MarkPendingKill();
		bDeleteMe = TRUE;
	}
}

void AController::Spawned()
{
	Super::Spawned();
	PlayerNum = GWorld->PlayerNum++;
}

UBOOL AController::IsLocalPlayerController()
{
	return false;
}

UBOOL APlayerController::IsLocalPlayerController()
{
	return ( Player && Player->IsA(ULocalPlayer::StaticClass()) );
}

UBOOL AController::LocalPlayerController()
{
	return false;
}

UBOOL APlayerController::LocalPlayerController()
{
	return ( Player && Player->IsA(ULocalPlayer::StaticClass()) );
}

UBOOL AController::WantsLedgeCheck()
{
	if ( !Pawn )
		return false;
	if ( Pawn->bCanJump )
	{
		// check if moving toward a navigation point, and not messed with
		if ( MoveTarget && (GetStateFrame()->LatentAction == AI_PollMoveToward) )
		{
			// check if still on path
			if ( CurrentPath && (CurrentPath->End == MoveTarget) )
			{
				FVector LineDir = Pawn->Location - (CurrentPath->Start->Location + (CurrentPathDir | (Pawn->Location - CurrentPath->Start->Location)) * CurrentPathDir);
				if (LineDir.SizeSquared() < 0.5f * Pawn->CylinderComponent->CollisionRadius * Pawn->CylinderComponent->CollisionRadius)
				{
					//debugf(TEXT("%s skip ledge check because on path"), *Pawn->GetName());
					return false;
				}
			}
			// check if could reach by jumping
			if ( MoveTarget->Physics != PHYS_Falling )
			{
				FVector JumpVelocity(0.f,0.f,0.f);
				if ( Pawn->SuggestJumpVelocity(JumpVelocity, MoveTarget->Location, Pawn->Location) )
				{
					return false;
				}
			}
		}
	}
	//debugf(TEXT("%s do ledge check"), *Pawn->GetName());
	return ( !Pawn->bCanFly );
}

UBOOL APlayerController::WantsLedgeCheck()
{
	return ( Pawn && (Pawn->bIsCrouched || Pawn->bIsWalking) );
}

UBOOL APlayerController::StopAtLedge()
{
	return FALSE;
}

UBOOL AController::StopAtLedge()
{
	if( !Pawn || 
		!Pawn->bCanJump || 
		 Pawn->bStopAtLedges )
	{
		MoveTimer = -1.f;
		return TRUE;
	}

	return FALSE;
}
//-------------------------------------------------------------------------------------------------
/*
Node Evaluation functions, used with APawn::BestPathTo()
*/

// declare type for node evaluation functions
typedef FLOAT ( *NodeEvaluator ) (ANavigationPoint*, APawn*, FLOAT);


FLOAT FindRandomPath( ANavigationPoint* CurrentNode, APawn* seeker, FLOAT bestWeight )
{
	if ( CurrentNode->bEndPoint )
		return (1000.f + appFrand());
	return appFrand();
}
//----------------------------------------------------------------------------------

void APlayerController::ClientUpdateLevelStreamingStatus(FName PackageName, UBOOL bNewShouldBeLoaded, UBOOL bNewShouldBeVisible, UBOOL bNewShouldBlockOnLoad )
{
	// if we're about to commit a map change, we assume that the streaming update is based on the to be loaded map and so defer it until that is complete
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine != NULL && GameEngine->bShouldCommitPendingMapChange)
	{
		new(GameEngine->PendingLevelStreamingStatusUpdates) FLevelStreamingStatus(PackageName, bNewShouldBeLoaded, bNewShouldBeVisible);
	}
	else
	{
		// search for the level object by name
		ULevelStreaming* LevelStreamingObject = NULL;
		if (PackageName != NAME_None)
		{
			AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
			for (INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++)
			{
				ULevelStreaming* CurrentLevelStreamingObject = WorldInfo->StreamingLevels(LevelIndex);
				if (CurrentLevelStreamingObject != NULL && CurrentLevelStreamingObject->PackageName == PackageName)
				{
					LevelStreamingObject = CurrentLevelStreamingObject;
					if (LevelStreamingObject != NULL)
					{
						LevelStreamingObject->bShouldBeLoaded		= bNewShouldBeLoaded;
						LevelStreamingObject->bShouldBeVisible		= bNewShouldBeVisible;
						LevelStreamingObject->bShouldBlockOnLoad	= bNewShouldBlockOnLoad;
					}
					else
					{
						debugfSuppressed( NAME_DevStreaming, TEXT("Unable to handle streaming object %s"),*LevelStreamingObject->GetName() );
					}

					// break out of object iterator if we found a match
					break;
				}
			}
		}

		if (LevelStreamingObject == NULL)
		{
			debugfSuppressed( NAME_DevStreaming, TEXT("Unable to find streaming object %s"), *PackageName.ToString() );
		}
	}
}

void APlayerController::ClientFlushLevelStreaming()
{
	// if we're already doing a map change, requesting another blocking load is just wasting time
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine == NULL || !GameEngine->bShouldCommitPendingMapChange)
	{
		// request level streaming be flushed next frame
		GWorld->UpdateLevelStreaming(NULL);
		WorldInfo->bRequestedBlockOnAsyncLoading = TRUE;
		// request GC as soon as possible to remove any unloaded levels from memory
		WorldInfo->ForceGarbageCollection();
	}
}

/** called when the client adds/removes a streamed level
 * the server will only replicate references to Actors in visible levels so that it's impossible to send references to
 * Actors the client has not initialized
 * @param PackageName the name of the package for the level whose status changed
 */
void APlayerController::ServerUpdateLevelVisibility(FName PackageName, UBOOL bIsVisible)
{
	UNetConnection* Connection = Cast<UNetConnection>(Player);
	if (Connection != NULL)
	{
		// add or remove the level package name from the list, as requested
		if (bIsVisible)
		{
			Connection->ClientVisibleLevelNames.AddUniqueItem(PackageName);
		}
		else
		{
			Connection->ClientVisibleLevelNames.RemoveItem(PackageName);
		}
	}
}

/** Whether or not to allow mature language **/
void APlayerController::SetAllowMatureLanguage( UBOOL bAllowMatureLanguge )
{
	GEngine->bAllowMatureLanguage = bAllowMatureLanguge;
}


/** Sets the Audio Group to this the value passed in **/
void APlayerController::SetAudioGroupVolume( FName GroupName, FLOAT Volume )
{
	if( ( GEngine->Client != NULL )
		&& ( GEngine->Client->GetAudioDevice() != NULL )
		)
	{
		GEngine->Client->GetAudioDevice()->SetGroupVolume( GroupName, Volume );
	}
}


void APlayerController::SetNetSpeed(INT NewSpeed)
{
	UNetDriver* Driver = GWorld->NetDriver;
	if (Player != NULL && Driver != NULL)
	{
		Player->CurrentNetSpeed = Clamp(NewSpeed, 1800, Driver->MaxClientRate);
		if (Driver->ServerConnection != NULL)
		{
			Driver->ServerConnection->CurrentNetSpeed = Player->CurrentNetSpeed;
		}
	}
}

void APlayerController::UpdateURL(const FString& NewOption, const FString& NewValue, UBOOL bSaveDefault)
{
	UGameEngine* GameEngine = Cast<UGameEngine>( GEngine );
	if( GameEngine )
	{
		GameEngine->LastURL.AddOption( *(NewOption + TEXT("=") + NewValue) );
		if( bSaveDefault )
		{
			GameEngine->LastURL.SaveURLConfig( TEXT("DefaultPlayer"), *NewOption, GGameIni );
		}
	}
}

FString APlayerController::GetDefaultURL(const FString& o)
{
	FString Option = o;
	FURL URL;
	URL.LoadURLConfig( TEXT("DefaultPlayer"), GGameIni );

	return FString( URL.GetOption(*(Option + FString(TEXT("="))), TEXT("")) );
}

void ADroppedPickup::AddToNavigation()
{
	if ( !Inventory )
		return;

	if ( PickupCache )
	{
		if ( PickupCache->InventoryCache == this )
			PickupCache->InventoryCache = NULL;
		PickupCache = NULL;
	}

	// find searcher
	APawn *Searcher = NULL;
	for ( AController *C=GWorld->GetFirstController(); C!=NULL; C=C->NextController )
	{
		if ( C->bIsPlayer && C->Pawn )
		{
			Searcher = C->Pawn;
			break;
		}
	}

	if ( !Searcher )
	{
		return;
	}

	// find nearest path
	FSortedPathList EndPoints;
	TArray<FNavigationOctreeObject*> Objects;
	GWorld->NavigationOctree->RadiusCheck(Location, MAXPATHDIST, Objects);
	for (INT i = 0; i < Objects.Num(); i++)
	{
		ANavigationPoint* Nav = Objects(i)->GetOwner<ANavigationPoint>();
		if ( Nav != NULL && (Location.Z - Nav->Location.Z < Searcher->MaxStepHeight + Searcher->MaxJumpHeight) &&
			(Nav->InventoryCache == NULL || Nav->InventoryCache->bDeleteMe || Nav->InventoryCache->Inventory == NULL || Nav->InventoryCache->Inventory->MaxDesireability <= Inventory->MaxDesireability) )
		{
			EndPoints.AddPath(Nav, appTrunc((Location - Nav->Location).SizeSquared()));
		}
	}

	if ( EndPoints.numPoints > 0 )
	{
		PickupCache = EndPoints.FindEndAnchor(Searcher,this,Location,false,false);
	}

	if ( PickupCache )
	{
		PickupCache->InventoryCache = this;
		PickupCache->InventoryDist = (Location - PickupCache->Location).Size();
	}
}

void ADroppedPickup::RemoveFromNavigation()
{
	if( !PickupCache )
	{
		return;
	}

	if( PickupCache->InventoryCache == this )
	{
		PickupCache->InventoryCache = NULL;
	}
}


FString APlayerController::ConsoleCommand(const FString& Cmd,UBOOL bWriteToLog)
{
	if (Player != NULL)
	{
		UConsole* ViewportConsole = (GEngine->GameViewport != NULL) ? GEngine->GameViewport->ViewportConsole : NULL;
		FConsoleOutputDevice StrOut(ViewportConsole);

		const INT CmdLen = Cmd.Len();
		TCHAR* CommandBuffer = (TCHAR*)appMalloc((CmdLen+1)*sizeof(TCHAR));
		TCHAR* Line = (TCHAR*)appMalloc((CmdLen+1)*sizeof(TCHAR));

		const TCHAR* Command = CommandBuffer;
		// copy the command into a modifiable buffer
		appStrcpy(CommandBuffer, (CmdLen+1), *Cmd.Left(CmdLen)); 
		

		// iterate over the line, breaking up on |'s
		while (ParseLine(&Command, Line, CmdLen+1))	// The ParseLine function expects the full array size, including the NULL character.
		{
			Player->Exec(Line, StrOut);
		}

		// Free temp arrays
		appFree(CommandBuffer);
		CommandBuffer=NULL;

		appFree(Line);
		Line=NULL;

		if (!bWriteToLog)
		{
			return *StrOut;
		}
	}

	return TEXT("");
}

/* CanSee()
returns true if LineOfSightto object and it is within creature's
peripheral vision
*/

UBOOL AController::CanSee(class APawn* Other)
{
	return SeePawn(Other, false);
}

/** 
 * Similar to CanSee but uses points to check instead of actor locations 
 */
UBOOL AController::CanSeeByPoints( FVector ViewLocation, FVector TestLocation, FRotator ViewRotation )
{
	if( BeyondFogDistance( ViewLocation, TestLocation ) || Pawn == NULL )
	{
		return FALSE;
	}

	const FLOAT maxdist = Pawn->SightRadius;

	// fixed max sight distance
	if( (TestLocation - ViewLocation).SizeSquared() > maxdist * maxdist )
	{
		return FALSE;
	}

	const FLOAT dist = (TestLocation - ViewLocation).Size();

	// check field of view
	const FVector& SightDir		= (TestLocation - ViewLocation).SafeNormal();
	const FVector& LookDir		= ViewRotation.Vector();
	const FLOAT	SightDotLook	= (SightDir | LookDir);
	
	if( SightDotLook < Pawn->PeripheralVision )
	{
		return FALSE;
	}

	FCheckResult Hit;
	const UBOOL bClearLine = GWorld->SingleLineCheck( Hit, Pawn, TestLocation, ViewLocation, TRACE_World|TRACE_StopAtAnyHit|TRACE_ComplexCollision );

	return bClearLine;

//	return LineOfSightTo(Other, bMaySkipChecks);
}

AVehicle* APawn::GetVehicleBase()
{
	if( !Base || !Base->GetAPawn() )
	{
		return NULL;
	}

	return Cast<AVehicle>(Base);
}

/* PickTarget()
Find the best pawn target for this controller to aim at.  Used for autoaiming.
*/
APawn* AController::PickTarget(UClass* TargetClass, FLOAT& bestAim, FLOAT& bestDist, FVector FireDir, FVector projStart, FLOAT MaxRange)
{
	if (Role < ROLE_Authority)
	{
		debugf(NAME_Warning, TEXT("Can't call PickTarget() on client"));
		return NULL;
	}

	if( !TargetClass )
	{
		TargetClass = APawn::StaticClass();
	}

	if( bestAim >= 1.f )
	{
		return NULL;
	}

	APawn* BestTarget = NULL;
	const FLOAT VerticalAim = bestAim * 3.f - 2.f;
	FCheckResult Hit(1.f);
    const FLOAT MaxRangeSquared = MaxRange*MaxRange;

	for ( AController *next=GWorld->GetFirstController(); next!=NULL; next=next->NextController )
	{
		APawn* NewTarget = next->Pawn;
		if ( NewTarget && (NewTarget != Pawn) )
		{
			if ( !NewTarget->bProjTarget )
			{
				// perhaps target vehicle this pawn is based on instead
				NewTarget = NewTarget->GetVehicleBase();
				if( !NewTarget || NewTarget->Controller )
				{
					continue;
				}
			}
			// look for best controlled pawn target
			if ( NewTarget->GetClass()->IsChildOf(TargetClass) && NewTarget->IsValidEnemyTargetFor(PlayerReplicationInfo, TRUE) )
			{
				const FVector AimDir = NewTarget->Location - projStart;
				FLOAT newAim = FireDir | AimDir;
				if ( newAim > 0 )
				{
					FLOAT FireDist = AimDir.SizeSquared();
					// only find targets which are < MaxRange units away
					if ( FireDist < MaxRangeSquared )
					{
						FireDist = appSqrt(FireDist);
						newAim = newAim/FireDist;
						if ( newAim > bestAim )
						{
							// target is more in line than current best - see if target is visible
							GWorld->SingleLineCheck( Hit, this, NewTarget->Location + FVector(0,0,NewTarget->EyeHeight), projStart, TRACE_World|TRACE_StopAtAnyHit );
							if( Hit.Actor )
							{
								GWorld->SingleLineCheck( Hit, this, NewTarget->Location, projStart, TRACE_World|TRACE_StopAtAnyHit );
							}

							if ( !Hit.Actor )
							{
								BestTarget = NewTarget;
								bestAim = newAim;
								bestDist = FireDist;
							}
						}
						else if ( !BestTarget )
						{
							// no target yet, so be more liberal about up/down error (more vertical autoaim help)
							FVector FireDir2D = FireDir;
							FireDir2D.Z = 0;
							FireDir2D.Normalize();
							FLOAT newAim2D = FireDir2D | AimDir;
							newAim2D = newAim2D/FireDist;
							if ( (newAim2D > bestAim) && (newAim > VerticalAim) )
							{
								GWorld->SingleLineCheck( Hit, this, NewTarget->Location, projStart, TRACE_World|TRACE_StopAtAnyHit );
								if( Hit.Actor )
								{
									GWorld->SingleLineCheck( Hit, this, NewTarget->Location + FVector(0,0,NewTarget->EyeHeight), projStart, TRACE_World|TRACE_StopAtAnyHit );
								}

								if ( !Hit.Actor )
								{
									BestTarget = NewTarget;
									bestDist = FireDist;
								}
							}
						}
					}
				}
			}
		}
	}

	return BestTarget;
}

/**
  * obsolete - use IsValidEnemyTargetFor() instead
  */
UBOOL APawn::IsValidTargetFor(const AController* C) const
{
	return C && IsValidEnemyTargetFor(C->PlayerReplicationInfo, TRUE);
}

/** returns if we are a valid enemy for PRI
 * checks things like whether we're alive, teammates, etc
 * works on clients and servers
 */
UBOOL APawn::IsValidEnemyTargetFor(const APlayerReplicationInfo* OtherPRI, UBOOL bNoPRIIsEnemy) const
{
	// only am valid target if not dead, and not driving a vehicle
	if ( bDeleteMe || (Health <=0) || DrivenVehicle )
	{
		return FALSE;
	}
	if ( !PlayerReplicationInfo )
	{
		 return bNoPRIIsEnemy;
	}
	
	// and not on same team, or neither is on a team (which implies not a team game)
	return !OtherPRI || !PlayerReplicationInfo->Team || (PlayerReplicationInfo->Team != OtherPRI->Team);
}

/* execWaitForLanding()
wait until physics is not PHYS_Falling
*/
void AController::execWaitForLanding( FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT_OPTX(waitDuration,4.f);
	P_FINISH;

	LatentFloat = waitDuration;
	if ( Pawn && (Pawn->Physics == PHYS_Falling) )
		GetStateFrame()->LatentAction = AI_PollWaitForLanding;
}

void AController::execPollWaitForLanding( FFrame& Stack, RESULT_DECL )
{
	if( Pawn && (Pawn->Physics != PHYS_Falling) )
	{
		GetStateFrame()->LatentAction = 0;
	}
	else
	{
		FLOAT DeltaSeconds = *(FLOAT*)Result;
		LatentFloat -= DeltaSeconds;
		if (LatentFloat <= 0.f)
		{
			eventLongFall();
		}
	}
}
IMPLEMENT_FUNCTION( AController, AI_PollWaitForLanding, execPollWaitForLanding);

UBOOL AController::PickWallAdjust(FVector HitNormal)
{
	if ( !Pawn )
		return false;
	return Pawn->PickWallAdjust(HitNormal, NULL);
}

/* FindStairRotation()
returns an integer to use as a pitch to orient player view along current ground (flat, up, or down)
*/
INT APlayerController::FindStairRotation(FLOAT deltaTime)
{
	// only recommend pitch if controller has a pawn, and frame rate isn't ridiculously low

	if ( !Pawn || (deltaTime > 0.33) )
	{
		return Rotation.Pitch;
	}

	if (Rotation.Pitch > 32768)
		Rotation.Pitch = (Rotation.Pitch & 65535) - 65536;
	
	FCheckResult Hit(1.f);
	FRotator LookRot = Rotation;
	LookRot.Pitch = 0;
	FVector Dir = LookRot.Vector();
	FVector EyeSpot = Pawn->Location + FVector(0,0,Pawn->BaseEyeHeight);
	FLOAT height = Pawn->CylinderComponent->CollisionHeight + Pawn->BaseEyeHeight;
	FVector CollisionSlice(Pawn->CylinderComponent->CollisionRadius,Pawn->CylinderComponent->CollisionRadius,1.f);

	GWorld->SingleLineCheck(Hit, this, EyeSpot + 2 * height * Dir, EyeSpot, TRACE_World, CollisionSlice);
	FLOAT Dist = 2 * height * Hit.Time;
	INT stairRot = 0;
	if (Dist > 0.8 * height)
	{
		FVector Spot = EyeSpot + 0.5 * Dist * Dir;
		FLOAT Down = 3 * height;
		GWorld->SingleLineCheck(Hit, this, Spot - FVector(0,0,Down), Spot, TRACE_World, CollisionSlice);
		if (Hit.Time < 1.f)
		{
			FLOAT firstDown = Down * Hit.Time;
			if (firstDown < 0.7f * height - 6.f) // then up or level
			{
				Spot = EyeSpot + Dist * Dir;
				GWorld->SingleLineCheck(Hit, this, Spot - FVector(0,0,Down), Spot, TRACE_World, CollisionSlice);
				stairRot = ::Max(0, Rotation.Pitch);
				if ( Down * Hit.Time < firstDown - 10 )
					stairRot = 3600;
			}
			else if  (firstDown > 0.7f * height + 6.f) // then down or level
			{
				GWorld->SingleLineCheck(Hit, this, Pawn->Location + 0.9*Dist*Dir, Pawn->Location, TRACE_World|TRACE_StopAtAnyHit);
				if (Hit.Time == 1.f)
				{
					Spot = EyeSpot + Dist * Dir;
					GWorld->SingleLineCheck(Hit, this, Spot - FVector(0,0,Down), Spot, TRACE_World, CollisionSlice);
					if (Down * Hit.Time > firstDown + 10)
						stairRot = -4000;
				}
			}
		}
	}
	INT Diff = Abs(Rotation.Pitch - stairRot);
	if( (Diff > 0) && (GWorld->GetTimeSeconds() - GroundPitchTime > 0.25) )
	{
		FLOAT RotRate = 4;
		if( Diff < 1000 )
			RotRate = 4000/Diff;

		RotRate = ::Min(1.f, RotRate * deltaTime);
		stairRot = appRound(FLOAT(Rotation.Pitch) * (1 - RotRate) + FLOAT(stairRot) * RotRate);
	}
	else
	{
		if ( (Diff < 10) && (Abs(stairRot) < 10) )
			GroundPitchTime = GWorld->GetTimeSeconds();
		stairRot = Rotation.Pitch;
	}
	return stairRot;
}

void AActor::execSuggestTossVelocity( FFrame& Stack, RESULT_DECL )
{
	P_GET_VECTOR_REF(TossVelocity);
	P_GET_VECTOR(Destination);
	P_GET_VECTOR(Start);
	P_GET_FLOAT(TossSpeed);
	P_GET_FLOAT_OPTX(BaseTossZ, 0.f);
	P_GET_FLOAT_OPTX(DesiredZPct, 0.05f);
	P_GET_VECTOR_OPTX(CollisionSize, FVector(0.f,0.f,0.f));
	P_GET_FLOAT_OPTX(TerminalVelocity, 0.f);
	P_FINISH;

	*(DWORD*)Result = SuggestTossVelocity(&TossVelocity, Destination, Start, TossSpeed, BaseTossZ, DesiredZPct, CollisionSize, TerminalVelocity);
}

UBOOL AController::ActorReachable(class AActor* actor)
{
	if ( !actor || !Pawn )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No pawn or goal for ActorReachable by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return 0;
	}

	// check if cached failed reach
	if ( (LastFailedReach == actor) && (FailedReachTime == GWorld->GetTimeSeconds())
		&& (FailedReachLocation == Pawn->Location) )
	{
		return 0;
	}
	else
	{
		INT Reach = Pawn->actorReachable(actor);
		if ( !Reach )
		{
			LastFailedReach = actor;
			FailedReachTime = GWorld->GetTimeSeconds();
			FailedReachLocation = Pawn->Location;
		}
		return Reach;
	}
}

UBOOL AController::PointReachable(FVector point)
{
	if ( !Pawn )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No pawn for pointReachable by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return 0;
	}

	return Pawn->pointReachable(point);
}

/* FindPathTo() and FindPathToward()
returns the best pathnode toward a point or actor - even if it is directly reachable
If there is no path, returns None
By default clears paths.  If script wants to preset some path weighting, etc., then
it can explicitly clear paths using execClearPaths before presetting the values and
calling FindPathTo with clearpath = 0
*/
AActor* AController::FindPath(const FVector& Point, AActor* Goal, UBOOL bWeightDetours, FLOAT MaxPathLength, UBOOL bReturnPartial)
{
	if ( !Pawn )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No pawn for FindPath by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}

	LastRouteFind = GWorld->GetTimeSeconds();
	AActor * bestPath = NULL;
	bPreparingMove = FALSE;

	if (Pawn->findPathToward(Goal, Point, NULL, 0.f, bWeightDetours, MaxPathLength, bReturnPartial) > 0.f)
	{
		bestPath = SetPath();
	}

	return bestPath;
}

AActor* AController::FindPathTo( FVector Point, FLOAT MaxPathLength, UBOOL bReturnPartial )
{
	return FindPath( Point, NULL, FALSE, MaxPathLength, bReturnPartial );
}

AActor* AController::FindPathToward( class AActor* goal, UBOOL bWeightDetours, FLOAT MaxPathLength, UBOOL bReturnPartial )
{
	if ( !goal )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No goal for FindPathToward by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}
	return FindPath(FVector(0,0,0), goal, bWeightDetours, MaxPathLength, bReturnPartial );
}

AActor* AController::FindPathToIntercept(class APawn* goal, class AActor* OtherRouteGoal, UBOOL bWeightDetours, FLOAT MaxPathLength, UBOOL bReturnPartial )
{
    APawn *goalPawn = goal ? goal->GetAPawn() : NULL;
	if ( !goalPawn || !Pawn )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No goal for FindPathToIntercept by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}
	// debugf(TEXT("%s Find path to intercept %s going to %s"),*Pawn->GetName(),*goal->GetName(),*OtherRouteGoal->GetName());
	if ( !Pawn->ValidAnchor() || !goalPawn->Controller || !OtherRouteGoal )
	{
		AActor *ResultPath = FindPath(FVector(0,0,0), goalPawn, bWeightDetours, MaxPathLength, bReturnPartial );
		return ResultPath;
	}
	UBOOL bFindDirectPath = true;
	UBOOL bHumanPathed = false;
	if ( goalPawn->IsHumanControlled() )
	{
		APlayerController *GoalPC = Cast<APlayerController>(goalPawn->Controller);
		if ( GoalPC && (goalPawn->Location != GoalPC->FailedPathStart) )
		{	
			bHumanPathed = (goalPawn->Controller->FindPath(FVector(0.f,0.f,0.f), OtherRouteGoal, FALSE, MaxPathLength, bReturnPartial )!= NULL);
			if ( !bHumanPathed )
				GoalPC->FailedPathStart = goalPawn->Location;
		}
	}

	if ( ((goalPawn->Controller->GetStateFrame()->LatentAction == AI_PollMoveToward) || (GWorld->GetTimeSeconds() - goalPawn->Controller->LastRouteFind < 0.75f))
		|| bHumanPathed )
	{
		// if already on path, movetoward goalPawn
		for (INT i=0; i<goalPawn->Controller->RouteCache.Num(); i++ )
		{
			if ( !goalPawn->Controller->RouteCache(i) )
				break;
			else
			{	
				bFindDirectPath = FALSE;
				if ( goalPawn->Controller->RouteCache(i) == Pawn->Anchor )
				{
//						debugf(TEXT("Already on path"));
					bFindDirectPath = TRUE;
					break;
				}
			}
		}
	}
	AActor *ResultActor = NULL;

	if ( bFindDirectPath )
	{
		ResultActor = FindPath(FVector(0.f,0.f,0.f), goalPawn, bWeightDetours, MaxPathLength, bReturnPartial );
	}
	else
	{
		ANavigationPoint* Nav = Cast<ANavigationPoint>(goalPawn->Controller->MoveTarget);
		if ( Nav )
		{
			Nav->bTransientEndPoint = TRUE;
		}

		for (INT i=0; i<goalPawn->Controller->RouteCache.Num(); i++ )
		{
			Nav = goalPawn->Controller->RouteCache(i);
			if ( Nav )
			{
				Nav->bTransientEndPoint = TRUE;
//					debugf(TEXT("Mark %s"),*Nav->GetName());
			}
			else
				break;
		}
		ResultActor = FindPath( FVector(0.f,0.f,0.f), goalPawn, bWeightDetours, MaxPathLength, bReturnPartial );
	}
	return ResultActor;
}

AActor* AController::FindPathTowardNearest(class UClass* GoalClass, UBOOL bWeightDetours, FLOAT MaxPathLength, UBOOL bReturnPartial )
{
	if ( !GoalClass || !Pawn )
	{
		debugfSuppressed(NAME_DevPath,TEXT("Warning: No goal for FindPathTowardNearest by %s in %s"),*GetName(), *GetStateFrame()->Describe() );
		return NULL;
	}
	ANavigationPoint* Found = NULL;

	// mark appropriate Navigation points
	for ( ANavigationPoint* Nav=GWorld->GetFirstNavigationPoint(); Nav; Nav=Nav->nextNavigationPoint )
		if ( Nav->GetClass() == GoalClass )
		{
			Nav->bTransientEndPoint = true;
			Found = Nav;
		}
	if ( Found )
		return FindPath(FVector(0,0,0), Found, bWeightDetours, MaxPathLength, bReturnPartial );
	else
		return NULL;
}

/* FindRandomDest()
returns a random pathnode which is reachable from the creature's location.  Note that the path to
this destination is in the RouteCache.
*/
ANavigationPoint* AController::FindRandomDest()
{
	if ( !Pawn )
		return NULL;

	ANavigationPoint * bestPath = NULL;
	bPreparingMove = false;
	if( Pawn->findPathToward( NULL, FVector(0,0,0), &FindRandomPath, 0.f, FALSE, UCONST_BLOCKEDPATHCOST, FALSE ) > 0 )
	{
		bestPath = Cast<ANavigationPoint>(RouteGoal);
	}

	return bestPath;
}

void AController::execLineOfSightTo( FFrame& Stack, RESULT_DECL )
{
	P_GET_ACTOR(Other);
	P_GET_VECTOR_OPTX(chkLocation,FVector(0.f,0.f,0.f));
	P_FINISH;

	if (chkLocation.IsZero())
	{
		*(DWORD*)Result = LineOfSightTo(Other);
	}
	else
	{
		*(DWORD*)Result = LineOfSightTo(Other,0,&chkLocation);
	}
}

/* execMoveTo()
start moving to a point -does not use routing
Destination is set to a point
*/
void AController::execMoveTo(FFrame& Stack, RESULT_DECL)
{
	P_GET_VECTOR(dest);
	P_GET_ACTOR_OPTX(viewfocus, NULL);
	P_GET_UBOOL_OPTX(bShouldWalk, (Pawn != NULL) ? Pawn->bIsWalking : 0);
	P_FINISH;

	if ( !Pawn )
		return;

	if ( bShouldWalk != Pawn->bIsWalking )
		Pawn->eventSetWalking(bShouldWalk);
	FVector MoveDir = dest - Pawn->Location;

	MoveTarget = NULL;
	Pawn->bReducedSpeed = false;
	Pawn->DesiredSpeed = Pawn->MaxDesiredSpeed;
	Pawn->DestinationOffset = 0.f;
	Pawn->NextPathRadius = 0.f;
	Focus = viewfocus;
	Pawn->setMoveTimer(MoveDir); 
	GetStateFrame()->LatentAction = AI_PollMoveTo;
	Destination = dest;
	if ( !Focus )
		FocalPoint = Destination;
	bAdjusting = false;
	CurrentPath = NULL;
	NextRoutePath = NULL;
	Pawn->ClearSerpentine();
	AdjustLoc = Destination;
	bAdvancedTactics = false;
	Pawn->moveToward(Destination, NULL);
}

void AController::execPollMoveTo( FFrame& Stack, RESULT_DECL )
{
	if( !Pawn || ((MoveTimer < 0.f) && (Pawn->Physics != PHYS_Falling)) )
	{
		GetStateFrame()->LatentAction = 0;
		return;
	}
	if ( bAdjusting )
		bAdjusting = !Pawn->moveToward(AdjustLoc, NULL);
	if( !bAdjusting && (!Pawn || Pawn->moveToward(Destination, NULL)) )
		GetStateFrame()->LatentAction = 0;
	else
		PostPollMove();
}
IMPLEMENT_FUNCTION( AController, AI_PollMoveTo, execPollMoveTo);

void AController::EndClimbLadder()
{
	if ( (GetStateFrame()->LatentAction == AI_PollMoveToward)
		&& Pawn && MoveTarget && MoveTarget->IsA(ALadder::StaticClass()) )
	{
		if ( Pawn->IsOverlapping(MoveTarget) )
			Pawn->SetAnchor(Cast<ANavigationPoint>(MoveTarget));
		GetStateFrame()->LatentAction = 0;
	}
}

/* execInLatentExecution()
returns true if controller currently performing latent execution with
passed in LatentAction value
*/
UBOOL AController::InLatentExecution(INT LatentActionNumber)
{
	return ( GetStateFrame()->LatentAction == LatentActionNumber );
}

void AController::StopLatentExecution()
{
	GetStateFrame()->LatentAction = 0;
	LatentFloat = -1.f;
}

/** activates path lanes for this Controller's current movement and adjusts its destination accordingly
 * @param DesiredLaneOffset the offset from the center of the Controller's CurrentPath that is desired
 * 				the Controller sets its LaneOffset as close as it can get to it without
 *				allowing any part of the Pawn's cylinder outside of the CurrentPath
 */
void AController::SetPathLane(FLOAT DesiredLaneOffset)
{
	// only enable if we're currently moving along the navigation network
	if (GetStateFrame()->LatentAction == AI_PollMoveToward && CurrentPath != NULL)
	{
		bUsingPathLanes = TRUE;
		// clamp the desired offset to what we can actually fit in
		FLOAT MaxOffset = FLOAT(CurrentPath->CollisionRadius) - Pawn->CylinderComponent->CollisionRadius;
		LaneOffset = Clamp<FLOAT>(DesiredLaneOffset, -MaxOffset, MaxOffset);
		// adjust to get in the lane
		FLOAT AdjustDist = LaneOffset + Pawn->CylinderComponent->CollisionRadius;
		if (LaneOffset > 0.f && !bAdjusting && Square(AdjustDist) < (Pawn->Location - CurrentPath->End->Location).SizeSquared2D())
		{
			bAdjusting = TRUE;
			FVector ClosestPointOnPath = (CurrentPath->Start->Location + (CurrentPathDir | (Pawn->Location - CurrentPath->Start->Location)) * CurrentPathDir);
			AdjustLoc = ClosestPointOnPath + (CurrentPathDir * AdjustDist) - ((CurrentPathDir ^ FVector(0.f, 0.f, 1.f)) * LaneOffset);
		}
	}
}

/* MoveToward()
start moving toward a goal actor -does not use routing
MoveTarget is set to goal
*/
void AController::execMoveToward(FFrame& Stack, RESULT_DECL)
{
	P_GET_ACTOR(goal);
	P_GET_ACTOR_OPTX(viewfocus, NULL);
	P_GET_FLOAT_OPTX(DesiredOffset, 0.f);
	P_GET_UBOOL_OPTX(bStrafe, FALSE);
	P_GET_UBOOL_OPTX(bShouldWalk, (Pawn != NULL) ? Pawn->bIsWalking : 0);
	P_FINISH;

	if ( !goal || !Pawn )
	{
		//Stack.Log("MoveToward with no goal");
		return;
	}

	if( bShouldWalk != Pawn->bIsWalking )
	{
		Pawn->eventSetWalking( bShouldWalk );
	}

	FVector Move = goal->Location - Pawn->Location;	
	Pawn->bReducedSpeed = false;
	Pawn->DesiredSpeed = Pawn->MaxDesiredSpeed;

	MoveTarget = goal;
	Focus = viewfocus ? viewfocus : goal;
	if( goal->GetAPawn() )
	{
		MoveTimer = 1.2f; //max before re-assess movetoward
	}
	else
	{
		Pawn->setMoveTimer(Move);
	}

	Destination = MoveTarget->GetDestination( this );
	GetStateFrame()->LatentAction = AI_PollMoveToward;
	bAdjusting = false;
	AdjustLoc = Destination;
	Pawn->ClearSerpentine();
	bAdvancedTactics = bStrafe && ( (PlayerReplicationInfo && PlayerReplicationInfo->bHasFlag) || bForceStrafe || (GWorld->GetNetMode() != NM_Standalone) || (GWorld->GetTimeSeconds() - Pawn->LastRenderTime < 5.f) || bSoaking );
	bUsingPathLanes = FALSE;
	LaneOffset = 0.f;

	// if necessary, allow the pawn to prepare for this move
	// give pawn the opportunity if its a navigation network move,
	// based on the reachspec
	ANavigationPoint *NavGoal = Cast<ANavigationPoint>(MoveTarget);

	FLOAT NewDestinationOffset = 0.f;
	CurrentPath = NULL;
	NextRoutePath = NULL;
	if ( NavGoal )
	{
		// if the reachspec isn't currently supported by the pawn
		// then give the pawn an opportunity to perform some latent preparation
		// (Controller will set its bPreparingMove=true if it needs latent preparation)
		if (Pawn->Anchor != NULL)
		{
			UReachSpec* NewCurrentPath = Pawn->Anchor->GetReachSpecTo(NavGoal);
			if (NewCurrentPath != NULL && (Pawn->ValidAnchor() || NewCurrentPath->IsOnPath(Pawn->Location, Pawn->GetCylinderExtent().X)))
			{
				CurrentPath = NewCurrentPath;
			}
		}
		if (CurrentPath != NULL)
		{
			CurrentPath = PrepareForMove( NavGoal, CurrentPath );
			if ( CurrentPath )
			{
				NextRoutePath = GetNextRoutePath( NavGoal );
				CurrentPathDir = CurrentPath->End->Location - CurrentPath->Start->Location;
				CurrentPathDir = CurrentPathDir.SafeNormal();
				if( NavGoal->bSpecialMove )
				{
					NavGoal->eventSuggestMovePreparation( Pawn );
				}

				// If move target was cancelled... exit
				if( !MoveTarget )
				{
					return;
				}
				else
				{
					// Otherwise, reset destination in case move prep has changed MoveTarget
					Destination = MoveTarget->GetDestination( this );
					AdjustLoc = Destination;
				}

				if (CurrentPath != NULL)
				{
					// handle leaving AVolumePathNodes (since don't go all the way to the center)
					if ( (Pawn->Physics == PHYS_Flying) || (Pawn->Physics == PHYS_Swimming) )
					{
						AVolumePathNode *StartFPN = Cast<AVolumePathNode>(CurrentPath->Start);
						// need to alter direction to get out of current AVolumePathNode region safely, if still in it
						if ( StartFPN && ((Abs(StartFPN->Location.Z - Destination.Z) > StartFPN->CylinderComponent->CollisionHeight)
									|| ((StartFPN->Location - Destination).Size2D() > StartFPN->CylinderComponent->CollisionRadius)) )
						{
							FCheckResult Hit(1.f);
							FVector Start = StartFPN->Location;
							if ( !Cast<AVolumePathNode>(CurrentPath->End) && ((StartFPN->Location - Destination).Size2D() < StartFPN->CylinderComponent->CollisionRadius) 
								&& (Destination.Z < StartFPN->Location.Z) )
							{
								Start = Destination;
								Start.Z = StartFPN->Location.Z - StartFPN->CylinderComponent->CollisionHeight + 10.f;
							}
							if ( !StartFPN->CylinderComponent->LineCheck(Hit,Destination, Start,FVector(0.f,0.f,0.f),0) )
							{
								bAdjusting = true;
								AdjustLoc = Hit.Location;
							}
						}
					}

					// if other AI pawns are using this path, use path lanes
					UReachSpec* OppositePath = NavGoal->GetReachSpecTo(Pawn->Anchor);

					// construct a list of Controllers using the same path or the opposite path
					TArray<AController*> PathMates;
					for (AController* C = WorldInfo->ControllerList; C != NULL; C = C->NextController)
					{
						if (C != this && C->Pawn != NULL)
						{
							// if the other Controller is moving along the same path or will be soon
							if ( C->GetStateFrame()->LatentAction == AI_PollMoveToward &&
									( (C->CurrentPath != NULL && (C->CurrentPath == CurrentPath || C->CurrentPath == OppositePath)) ||
										(C->NextRoutePath != NULL && (C->NextRoutePath == CurrentPath || C->NextRoutePath == OppositePath)) ) )
							{
								PathMates.AddItem(C);
							}
							// or if it's sitting on top of our endpoint
							else if (C->Pawn->Velocity.IsZero() && C->Pawn->Acceleration.IsZero() && C->Pawn->Anchor == NavGoal && C->Pawn->ValidAnchor())
							{
								PathMates.AddItem(C);
							}
						}
					}

					if (PathMates.Num() > 0)
					{
						// allocate lanes for each Pawn on the path, starting from our far right
						FLOAT CurrentLaneOffset = FLOAT(CurrentPath->CollisionRadius);
						// set our lane first
						SetPathLane(CurrentLaneOffset);
						// take the offset we actually used and find out where the next pawn's lane should start
						CurrentLaneOffset = LaneOffset - Pawn->CylinderComponent->CollisionRadius;
						// now do the same for the other Pawns
						for (INT i = 0; i < PathMates.Num(); i++)
						{
							// if this is the last one, always force it all the way to the left
							if (i == PathMates.Num() - 1)
							{
								CurrentLaneOffset = -FLOAT(CurrentPath->CollisionRadius);
							}
							// if this one is going in the opposite direction, we have to take the negative of everything
							if (PathMates(i)->CurrentPath == OppositePath || PathMates(i)->NextRoutePath == OppositePath)
							{
                                PathMates(i)->SetPathLane(-CurrentLaneOffset + PathMates(i)->Pawn->CylinderComponent->CollisionRadius);
								CurrentLaneOffset = -PathMates(i)->LaneOffset - PathMates(i)->Pawn->CylinderComponent->CollisionRadius;
							}
							else
							{
								PathMates(i)->SetPathLane(CurrentLaneOffset - PathMates(i)->Pawn->CylinderComponent->CollisionRadius);
								CurrentLaneOffset = PathMates(i)->LaneOffset - PathMates(i)->Pawn->CylinderComponent->CollisionRadius;
							}
						}
					}
				}
			}
		}
		else if( NavGoal->bSpecialMove )
		{
			NavGoal->eventSuggestMovePreparation(Pawn);		
		}

		if ( !NavGoal->bNeverUseStrafing && !NavGoal->bForceNoStrafing )
		{
			if( CurrentPath )
			{
				Pawn->InitSerpentine();
			}
			if( NavGoal != RouteGoal )
			{
				NewDestinationOffset = (0.7f + 0.3f * appFrand()) * 
											::Max(0.f, Pawn->NextPathRadius - Pawn->CylinderComponent->CollisionRadius);
			}
		}
	}

	Pawn->DestinationOffset = (DesiredOffset == 0.f) ? NewDestinationOffset : DesiredOffset;
	Pawn->NextPathRadius = 0.f;

	if( !bPreparingMove )
	{
		Pawn->moveToward(Destination, MoveTarget);
	}
}

UReachSpec* AController::PrepareForMove( ANavigationPoint *NavGoal, UReachSpec* Path )
{
	Path->PrepareForMove( this );
	return Path;
}

UReachSpec* AController::GetNextRoutePath( ANavigationPoint *NavGoal )
{
	UReachSpec* Spec = NULL;
	if(  RouteGoal && 
		(NavGoal == CurrentPath->End) && 
		(NavGoal != RouteGoal) )
	{
		for( INT i = 0; i < RouteCache.Num() - 1; i++ )
		{
			if( !RouteCache(i) )
			{
				break;
			}
			else if( RouteCache(i) == CurrentPath->End )
			{
				ANavigationPoint *NextNav = RouteCache(i+1);
				if ( NextNav )
				{
					Spec = CurrentPath->End->GetReachSpecTo(NextNav);
				}
				break;
			}
		}
	}

	return Spec;
}

FVector AActor::GetDestination( AController* C )
{
	return Location;
}

void AController::execPollMoveToward( FFrame& Stack, RESULT_DECL )
{
	if( !MoveTarget || !Pawn || ((MoveTimer < 0.f) && (Pawn->Physics != PHYS_Falling)) )
	{	
		//debugf(TEXT("MoveTarget cleared during movetoward"));
		GetStateFrame()->LatentAction = 0;
		return;
	}
	// check that pawn is ready to go
	if ( bPreparingMove )
		return;
	// check if adjusting around an obstacle
	if ( bAdjusting )
	{
		bAdjusting = !Pawn->moveToward(AdjustLoc, MoveTarget);
	}
	if ( !MoveTarget || !Pawn )
	{
		GetStateFrame()->LatentAction = 0;
		return;
	}
	if ( bAdjusting && Cast<AVolumePathNode>(MoveTarget) )
	{
		if ( Pawn->ReachedDestination(Pawn->Location,MoveTarget->Location, MoveTarget) )
		{
			GetStateFrame()->LatentAction = 0;
			return;
		}
		else if ( (Pawn->Velocity | (AdjustLoc - Pawn->Location)) < 0.f )
			bAdjusting = false;
	}

	if ( !bAdjusting )
	{
		// set destination to current movetarget location
		FVector StartingDest = MoveTarget->GetDestination(this);
		Destination = StartingDest;
		FLOAT MoveTargetRadius, MoveTargetHeight;
		MoveTarget->GetBoundingCylinder(MoveTargetRadius, MoveTargetHeight);

		if( Pawn->Physics==PHYS_Flying )
		{
			if ( MoveTarget->GetAPawn() )
			{
				if ( MoveTarget->GetAPawn()->bStationary )
				{
					Destination.Z += 2.f * ::Max(MoveTargetHeight, 2.5f * Pawn->CylinderComponent->CollisionHeight);
				}
				else
				{
					Destination.Z += 0.7f * MoveTargetHeight;
				}
			}
			else if ( MoveTarget->IsA(ANavigationPoint::StaticClass()) && !MoveTarget->IsA(AVolumePathNode::StaticClass()) )
			{
				if ( MoveTarget->IsA(ALiftExit::StaticClass()) && CurrentPath && CurrentPath->Start->IsA(ALiftCenter::StaticClass()) )
				{
					Destination = Pawn->Location;
					Destination.Z = MoveTarget->Location.Z;
				}
				else if ( Pawn->Location.Z < Destination.Z )
				{
					if ( Pawn->Location.Z > Destination.Z - MoveTargetHeight )
					{
						Destination.Z += MoveTargetHeight;
					}
					else
					{
						Destination.Z += 500.f;
					}
				}
			}
		}
		else if( Pawn->Physics == PHYS_Spider )
		{
			Destination = Destination - MoveTargetRadius * Pawn->Floor;
		}

		FLOAT oldDesiredSpeed = Pawn->DesiredSpeed;

		// move to movetarget
		if (Pawn->moveToward(Destination, MoveTarget))
		{
			GetStateFrame()->LatentAction = 0;
		}
		else if ( MoveTarget && Pawn && (Pawn->Physics == PHYS_Walking) )
		{
			FVector Diff = Pawn->Location - Destination;
			FLOAT DiffZ = Diff.Z;
			Diff.Z = 0.f;
			// reduce timer if seem to be stuck above or below
			if ( Diff.SizeSquared() < Pawn->CylinderComponent->CollisionRadius * Pawn->CylinderComponent->CollisionRadius )
			{
				MoveTimer -= Pawn->AvgPhysicsTime;
				if ( DiffZ > Pawn->CylinderComponent->CollisionRadius + 2 * Pawn->MaxStepHeight )
				{
					// check if visible below
					FCheckResult Hit(1.f);
					GWorld->SingleLineCheck(Hit, Pawn, Destination, Pawn->Location, TRACE_World|TRACE_StopAtAnyHit);
					if ( (Hit.Time < 1.f) && (Hit.Actor != MoveTarget) )
					{
						GetStateFrame()->LatentAction = 0;
					}
				}
			}
		}
		if ( !MoveTarget || !Pawn )
		{
			GetStateFrame()->LatentAction = 0;
			return;
		}
		if ( GetStateFrame()->LatentAction != 0 )
		{
			PostPollMove();
		}

		Destination = StartingDest;

		if( MoveTarget->GetAPawn() )
		{
			Pawn->DesiredSpeed = oldDesiredSpeed; //don't slow down when moving toward a pawn
			if ( !Pawn->bCanSwim && MoveTarget->PhysicsVolume->bWaterVolume )
			{
				MoveTimer = -1.f; //give up
			}
		}
	}
}
IMPLEMENT_FUNCTION( AController, AI_PollMoveToward, execPollMoveToward);

/* execTurnToward()
turn toward Focus
*/
void AController::FinishRotation()
{
	GetStateFrame()->LatentAction = AI_PollFinishRotation;
}

void AController::execPollFinishRotation( FFrame& Stack, RESULT_DECL )
{

	if( !Pawn || Pawn->ReachedDesiredRotation() )
		GetStateFrame()->LatentAction = 0;

}
IMPLEMENT_FUNCTION( AController, AI_PollFinishRotation, execPollFinishRotation);

UBOOL AController::BeyondFogDistance(FVector ViewPoint, FVector OtherPoint)
{
	return false;
}

/*
SeePawn()

returns true if Other was seen by this controller's pawn.  Chance of seeing other pawn decreases with increasing
distance or angle in peripheral vision
*/
DWORD AController::SeePawn(APawn *Other, UBOOL bMaySkipChecks)
{
	if ( !Other || !Pawn || Other->IsInvisible() )
		return 0;

	if (Other != Enemy)
		bLOSflag = !bLOSflag;
	else
		return LineOfSightTo(Other);

	if ( BeyondFogDistance(Pawn->Location, Other->Location) )
		return 0;

	FLOAT maxdist = Pawn->SightRadius;

	// fixed max sight distance
	if ( (Other->Location - Pawn->Location).SizeSquared() > maxdist * maxdist )
		return 0;

	FLOAT dist = (Other->Location - Pawn->Location).Size();

	// may skip if more than 1/5 of maxdist away (longer time to acquire)
	if ( bMaySkipChecks && (appFrand() * dist > 0.1f * maxdist) )
			return 0;

	// check field of view
	FVector SightDir = (Other->Location - Pawn->Location).SafeNormal();
	FVector LookDir = Rotation.Vector();
	FLOAT Stimulus = (SightDir | LookDir);
	if ( Stimulus < Pawn->PeripheralVision )
		return 0;

	// need to make this only have effect at edge of vision
	//if ( bMaySkipChecks && (appFrand() * (1.f - Pawn->PeripheralVision) < 1.f - Stimulus) )
	//	return 0;
	if ( bMaySkipChecks && bSlowerZAcquire && (appFrand() * dist > 0.1f * maxdist) )
	{
		// lower FOV vertically
		SightDir.Z *= 2.f;
		SightDir.Normalize();
		if ( (SightDir | LookDir) < Pawn->PeripheralVision )
			return 0;

		// notice other pawns at very different heights more slowly
		FLOAT heightMod = Abs(Other->Location.Z - Pawn->Location.Z);
		if ( appFrand() * dist < heightMod )
			return 0;
	}

	Stimulus = 1;
	return LineOfSightTo(Other, bMaySkipChecks);

}

AActor* AController::GetViewTarget()
{
	if ( Pawn )
		return Pawn;
	return this;
}

void APlayerController::UpdateViewTarget(AActor* NewViewTarget)
{
	if ( (NewViewTarget == ViewTarget) || !NewViewTarget )
		return;

	AActor* OldViewTarget = ViewTarget;
	ViewTarget = NewViewTarget;

	ViewTarget->eventBecomeViewTarget(this);
	if ( OldViewTarget )
		OldViewTarget->eventEndViewTarget(this);

	if ( !LocalPlayerController() && (GWorld->GetNetMode() != NM_Client) )
	{
		eventClientSetViewTarget(ViewTarget);
	}
}
	
AActor* APlayerController::GetViewTarget()
{
	if( PlayerCamera )
	{
		return PlayerCamera->GetViewTarget();
	}

	if ( RealViewTarget && !RealViewTarget->bDeleteMe )
	{
		if ( !ViewTarget || ViewTarget->bDeleteMe || !ViewTarget->GetAPawn() || (ViewTarget->GetAPawn()->PlayerReplicationInfo != RealViewTarget) )
		{
			// not viewing pawn associated with RealViewTarget, so look for one
			// Assuming on server, so PRI Owner is valid
			if ( !RealViewTarget->Owner )
			{
				RealViewTarget = NULL;
			}
			else
			{
				AController* PRIOwner = RealViewTarget->Owner->GetAController();
				if ( PRIOwner )
				{
					if ( PRIOwner->GetAPlayerController() && PRIOwner->GetAPlayerController()->ViewTarget && !PRIOwner->GetAPlayerController()->ViewTarget->bDeleteMe )
					{
						UpdateViewTarget(PRIOwner->GetAPlayerController()->ViewTarget);
					}
					else if ( PRIOwner->Pawn )
					{
						UpdateViewTarget(PRIOwner->Pawn);
					}
				}
				else
				{
					RealViewTarget = NULL;
				}
			}
		}
	}

	if ( !ViewTarget || ViewTarget->bDeleteMe )
	{
		if ( Pawn && !Pawn->bDeleteMe && !Pawn->bPendingDelete )
			UpdateViewTarget(Pawn);
		else
			UpdateViewTarget(this);
	}
	return ViewTarget;
}

/** Set the player controller to treat NewTarget as its new view target. */
void APlayerController::SetViewTarget( AActor* NewViewTarget, FLOAT TransitionTime )
{
	// Player camera overrides player controller implementation
	if( PlayerCamera )
	{
		PlayerCamera->SetViewTarget( NewViewTarget, TransitionTime );
		return;
	}

	if ( !NewViewTarget )
		NewViewTarget = this;

	// Update RealViewTarget (used to follow same player through pawn transitions, etc., when spectating)
	if ( NewViewTarget == this || (NewViewTarget->GetAPawn() && (NewViewTarget == Pawn)) ) 
	{	
		RealViewTarget = NULL;
	}
	else if ( NewViewTarget->GetAController() )
	{
		RealViewTarget = NewViewTarget->GetAController()->PlayerReplicationInfo;
	}
	else if ( NewViewTarget->GetAPawn() )
	{
		RealViewTarget = NewViewTarget->GetAPawn()->PlayerReplicationInfo;
	}
	else if ( Cast<APlayerReplicationInfo>(NewViewTarget) )
	{
		RealViewTarget = Cast<APlayerReplicationInfo>(NewViewTarget);
	}
	else
	{
		RealViewTarget = NULL;
	}
	
	UpdateViewTarget(NewViewTarget);

	if (GWorld->GetNetMode() != NM_Client)
		GetViewTarget();

	if ( ViewTarget == this  ) 
		RealViewTarget = NULL;
}

#define DEBUG_LOS 0
/*
 * LineOfSightTo()
 * returns TRUE if controller's pawn can see Other actor.
 * Checks line to center of other actor, and possibly to head or box edges depending on distance
 */
DWORD AController::LineOfSightTo(const AActor *Other, INT bUseLOSFlag, const FVector *chkLocation)
{
	if( !Other )
	{
		return 0;
	}

#if DEBUG_LOS
	debugf(TEXT("AController::LineOfSightTo. Controller: %s, Target: %s"), *GetName(), *Other->GetName());
#endif

	FVector ViewPoint;
	// check for a viewpoint override
	if( chkLocation != NULL )
	{
		ViewPoint = *chkLocation;
	}
	else
	{
		AActor*	ViewTarg = GetViewTarget();
		ViewPoint = ViewTarg->Location;
		if( ViewTarg == Pawn )
		{
			ViewPoint.Z += Pawn->BaseEyeHeight; //look from eyes
		}
	}

	if( BeyondFogDistance(ViewPoint, Other->Location) )
	{
#if DEBUG_LOS
		debugf(TEXT("AController::LineOfSightTo. BeyondFogDistance"));
#endif
		return 0;
	}

	FLOAT OtherRadius, OtherHeight;
	Other->GetBoundingCylinder(OtherRadius, OtherHeight);

	FCheckResult Hit(1.f);
	if( Other == Enemy )
	{
#if DEBUG_LOS
		debugf(TEXT("AController::LineOfSightTo. Enemy Branch"));
#endif
		GWorld->SingleLineCheck( Hit, this, Other->Location, ViewPoint, TRACE_World|TRACE_StopAtAnyHit );

		if( Hit.Actor && (Hit.Actor != Other) )
		{
#if DEBUG_LOS
			debugf(TEXT("AController::LineOfSightTo. HitActor: %s"), *Hit.Actor->GetName());
#endif
			GWorld->SingleLineCheck( Hit, this, Enemy->Location + FVector(0,0,Enemy->BaseEyeHeight), ViewPoint, TRACE_World|TRACE_StopAtAnyHit );
		}

		if( !Hit.Actor || (Hit.Actor == Other) )
		{
			// update enemy info 
			// NOTE that controllers update this info even if the enemy is behind them
			// unless they check for this in UpdateEnemyInfo()
			UpdateEnemyInfo(Enemy);
#if DEBUG_LOS
			debugf(TEXT("AController::LineOfSightTo. LOS successful #1"));
#endif
			return 1;
		}

		// only check sides if width of other is significant compared to distance
		if( OtherRadius * OtherRadius/(Other->Location - ViewPoint).SizeSquared() < 0.0001f )
		{
#if DEBUG_LOS
			debugf(TEXT("AController::LineOfSightTo. Don't check sides."));
#endif
			return 0;
		}
	}
	else
	{
		GWorld->SingleLineCheck( Hit, this, Other->Location, ViewPoint, TRACE_World|TRACE_StopAtAnyHit );
		if( !Hit.Actor || (Hit.Actor == Other) )
			return 1;

		FLOAT distSq = (Other->Location - ViewPoint).SizeSquared();
		if ( distSq > FARSIGHTTHRESHOLDSQUARED )
			return 0;
		if ( (!bIsPlayer || !Other->GetAPawn()) && (distSq > NEARSIGHTTHRESHOLDSQUARED) ) 
			return 0;
		
		//try viewpoint to head
		if ( !bUseLOSFlag || !bLOSflag )
		{
			GWorld->SingleLineCheck( Hit, this, Other->Location + FVector(0,0,OtherHeight), ViewPoint, TRACE_World|TRACE_StopAtAnyHit );
			if ( !Hit.Actor || (Hit.Actor == Other) )
				return 1;
		}

		// bLOSFlag used by SeePawn to reduce visibility checks
		if ( bUseLOSFlag && !bLOSflag )
			return 0;
		// only check sides if width of other is significant compared to distance
		if ( OtherRadius * OtherRadius/distSq < 0.00015f )
			return 0;
	}

	//try checking sides - look at dist to four side points, and cull furthest and closest
	FVector Points[4];
	Points[0] = Other->Location - FVector(OtherRadius, -1 * OtherRadius, 0);
	Points[1] = Other->Location + FVector(OtherRadius, OtherRadius, 0);
	Points[2] = Other->Location - FVector(OtherRadius, OtherRadius, 0);
	Points[3] = Other->Location + FVector(OtherRadius, -1 * OtherRadius, 0);
	int imin = 0;
	int imax = 0;
	FLOAT currentmin = Points[0].SizeSquared();
	FLOAT currentmax = currentmin;
	for ( INT i=1; i<4; i++ )
	{
		FLOAT nextsize = Points[i].SizeSquared();
		if (nextsize > currentmax)
		{
			currentmax = nextsize;
			imax = i;
		}
		else if (nextsize < currentmin)
		{
			currentmin = nextsize;
			imin = i;
		}
	}

	for ( INT i=0; i<4; i++ )
		if	( (i != imin) && (i != imax) )
		{
			GWorld->SingleLineCheck( Hit, this, Points[i], ViewPoint, TRACE_World|TRACE_StopAtAnyHit );
			if ( !Hit.Actor || (Hit.Actor == Other) )
				return 1;
		}
	return 0;
}

/* CanHear()

Returns 1 if controller can hear this noise
Several types of hearing are supported

Noises must be perceptible (based on distance, loudness, and the alerntess of the controller

  Options for hearing are: (assuming the noise is perceptible

  bLOSHearing = Hear any perceptible noise which is not blocked by geometry
  bMuffledHearing = Hear occluded noises if closer
*/
UBOOL AController::CanHear(const FVector& NoiseLoc, FLOAT Loudness, AActor *Other)
{
	if ( bUsePlayerHearing || !Other->Instigator || !Other->Instigator->Controller || !Pawn )
		return FALSE; //ignore sounds from uncontrolled (dead) pawns, or if don't have a pawn to control

	FLOAT DistSq = (Pawn->Location - NoiseLoc).SizeSquared();
	FLOAT Perceived = Loudness * Pawn->HearingThreshold * Pawn->HearingThreshold;

	// take pawn alertness into account (it ranges from -1 to 1 normally)
	Perceived *= ::Max(0.f,(Pawn->Alertness + 1.f));

	// check if sound is too quiet to hear
	if ( Perceived < DistSq )
		return FALSE;

	// if not checking for occlusion, then we're done
	if ( !Pawn->bLOSHearing )
		return TRUE;

	// check if sound is occluded
	FVector ViewLoc = Pawn->Location + FVector(0,0,Pawn->BaseEyeHeight);
	FCheckResult Hit(1.f);
	GWorld->SingleLineCheck(Hit, this, NoiseLoc, ViewLoc, TRACE_Level);
	if ( Hit.Time == 1.f )
		return TRUE;

	if ( Pawn->bMuffledHearing )
	{
		// distance sound can be heard is half normal
		return ( Perceived > 4.f * DistSq );
	}

	return FALSE;
}

void AController::CheckEnemyVisible()
{
	if ( Enemy )
	{
		check(Enemy->IsValid());
		if ( !LineOfSightTo(Enemy) )
			eventEnemyNotVisible();
	}
}

/* Player shows self to pawns that are ready
*/
void AController::ShowSelf()
{
	if ( !Pawn )
		return;
	for ( AController *C=GWorld->GetFirstController(); C!=NULL; C=C->NextController )
	{
		if( C!=this && C->ShouldCheckVisibilityOf(this) && C->SeePawn(Pawn) )
		{
			if ( bIsPlayer )
				C->eventSeePlayer(Pawn);
			else
				C->eventSeeMonster(Pawn);
		}
	}
}

/** ShouldCheckVisibilityOf()
returns true if should check whether pawn controlled by controller C is visible
*/
UBOOL AController::ShouldCheckVisibilityOf(AController *C)
{
	// only check visibility if this or C is a player, and sightcounter has counted down, and is probing event.
	if ( (bIsPlayer || C->bIsPlayer) && (SightCounter < 0.f) && (C->bIsPlayer ? IsProbing(NAME_SeePlayer) : IsProbing(NAME_SeeMonster)) )
	{
		// don't check visibility if on same team if bSeeFriendly==false
		return ( bSeeFriendly || (WorldInfo->Game && !WorldInfo->Game->bTeamGame) || !PlayerReplicationInfo || !PlayerReplicationInfo->Team 
			|| !C->PlayerReplicationInfo || !C->PlayerReplicationInfo->Team
			|| (PlayerReplicationInfo->Team != C->PlayerReplicationInfo->Team) );
	}
	return FALSE;
}

/*
SetPath()
Based on the results of the navigation network (which are stored in RouteCache[],
return the desired path.  Check if there are any intermediate goals (such as hitting a
switch) which must be completed before continuing toward the main goal
*/
AActor* AController::SetPath(INT bInitialPath)
{
	static AActor* ChosenPaths[4];

	if ( RouteCache.Num() == 0 )
		return NULL;

	AActor * bestPath = RouteCache(0);

	if ( !Pawn->ValidAnchor() )
		return bestPath;	// make sure on network before trying to find complex routes

	if ( bInitialPath )
	{
		for ( INT i=0; i<4; i++ )
			ChosenPaths[i] = NULL;
		// if this is setting the path toward the main (final) goal
		// make sure still same goal as before
		if ( RouteGoal == GoalList[0] )
		{
			// check for existing intermediate goals
			if ( GoalList[1] )
			{
				INT i = 1;
				while (i < 4 && GoalList[i] != NULL)
				{
					i++;
				}
				AActor* RealGoal = GoalList[i-1];
				if ( Pawn->actorReachable(RealGoal) )
				{
					// I can reach the intermediate goal, so
					GoalList[i-1] = NULL;
					bPreparingMove = false;
					return RealGoal;
				}
				// find path to new goal
				UBOOL bOldPrep = bPreparingMove;
				bPreparingMove = false;
				if ( Pawn->findPathToward( RealGoal,RealGoal->Location,NULL, 0.f,FALSE,UCONST_BLOCKEDPATHCOST, FALSE ) > 0.f )
				{
					bestPath = SetPath(0);
				}
				else
				{
					bPreparingMove = bOldPrep;
				}
			}
		}
		else
		{
			GoalList[0] = RouteGoal;
			for ( INT i=1; i<4; i++ )
				GoalList[i] = NULL;
		}
	}
	else
	{
		// add new goal to goal list
		for ( INT i=0; i<4; i++ )
		{
			if ( GoalList[i] == RouteGoal )
				break;
			if ( !GoalList[i] )
			{
				GoalList[i] = RouteGoal;
				break;
			}
		}
	}
	for ( INT i=0; i<4; i++ )
	{
		if ( ChosenPaths[i] == NULL )
		{
			ChosenPaths[i] = bestPath;
			break;
		}
		else if ( ChosenPaths[i] == bestPath )
			return bestPath;
	}
	if ( bestPath && bestPath->IsProbing(NAME_SpecialHandling) )
		bestPath = HandleSpecial(bestPath);
	return bestPath;
}

/** Allows operations on nodes in the route before emptying the cache */
void AController::RouteCache_Empty()
{
	RouteCache.Empty();
}
void AController::RouteCache_AddItem( ANavigationPoint* Nav )
{
	if( Nav )
	{
		RouteCache.AddItem( Nav );
	}
}
void AController::RouteCache_InsertItem( ANavigationPoint* Nav, INT Idx )
{
	if( Nav )
	{
		RouteCache.InsertItem( Nav, Idx );
	}
}
void AController::RouteCache_RemoveItem( ANavigationPoint* Nav )
{
	if( Nav )
	{
		RouteCache.RemoveItem( Nav );
	}
}
void AController::RouteCache_RemoveIndex( INT Index, INT Count )
{
	if( Index >= 0 && Index < RouteCache.Num() )
	{
		RouteCache.Remove( Index, Count );
	}
}

AActor* AController::HandleSpecial(AActor *bestPath)
{
	if ( !bCanDoSpecial || GoalList[3] )
		return bestPath;	//limit AI intermediate goal depth to 4

	AActor * newGoal = bestPath->eventSpecialHandling(Pawn);

	if ( newGoal && (newGoal != bestPath) )
	{
		UBOOL bOldPrep = bPreparingMove;
		bPreparingMove = false;
		// if can reach intermediate goal directly, return it
		if ( Pawn->actorReachable(newGoal) )
		{
			return newGoal;
		}

		// find path to new goal
		if( Pawn->findPathToward( newGoal, newGoal->Location, NULL, 0.f, FALSE, UCONST_BLOCKEDPATHCOST, FALSE ) > 0.f )
		{
			bestPath = SetPath(0);
		}
		else
		{
			debugfSuppressed(NAME_DevPath, TEXT("Failed to find path to special goal %s on the way to %s"), *newGoal->GetName(), *bestPath->GetName());
			bPreparingMove = bOldPrep;
		}
	}
	return bestPath;

}

/* AcceptNearbyPath() returns true if the controller will accept a path which gets close to
and withing sight of the destination if no reachable path can be found.
*/
INT AController::AcceptNearbyPath(AActor *goal)
{
	return 0;
}

INT AAIController::AcceptNearbyPath(AActor *goal)
{
	if ( Cast<AVehicle>(Pawn) )
		return true;
	return (goal && (goal->GetAPawn() || (goal->Physics == PHYS_Falling)) );
}

/* ForceReached()
* Controller can override Pawn NavigationPoint reach test
* return true if want to force successful reach by pawn of Nav
*/
UBOOL AController::ForceReached(ANavigationPoint *Nav, const FVector& TestPosition)
{
	return FALSE;
}

/** JumpOverWall()
Make pawn jump over an obstruction
*/
void AController::JumpOverWall(FVector WallNormal)
{
	FVector Dir = DesiredDirection();
	Dir.Z = 0.f;
	Dir = Dir.SafeNormal();

	// check distance to destination vs how we'll slide along wall
	if ( WallNormal.Z != 0.f )
	{
		WallNormal.Z = 0.f;
		WallNormal = WallNormal.SafeNormal();
	}
	WallNormal *= -1.f;
	// if the opposite of the wall normal is approximately in the direction we want to go anyway, then jump directly over it
	FLOAT DotP = Dir | WallNormal;
	if ( (DotP > 0.8f) || (DesiredDirection().Size2D() < 6.f*Pawn->CylinderComponent->CollisionRadius) )
	{
		Dir = WallNormal;
	}
	else if (CurrentPath != NULL && CurrentPath->Start != NULL)
	{
		// if the opposite of the wall normal is in the same direction as the closest point of our current path line, then jump directly over it
		FVector ClosestPoint;
		if ( PointDistToLine(Pawn->Location, CurrentPathDir, CurrentPath->Start->Location, ClosestPoint) > Pawn->CylinderComponent->CollisionRadius &&
			((ClosestPoint - Pawn->Location).SafeNormal() | WallNormal) > 0.5f )
		{
			Dir = WallNormal;
		}
	}

	Pawn->Velocity = Pawn->GroundSpeed * Dir;
	Pawn->Acceleration = Pawn->AccelRate * WallNormal;
	Pawn->Velocity.Z = Pawn->JumpZ;
	Pawn->setPhysics(PHYS_Falling);
}

void AController::NotifyJumpApex()
{
	eventNotifyJumpApex();
}

/* AdjustFromWall()
Gives controller a chance to adjust around an obstacle and keep moving
*/

void AController::AdjustFromWall(FVector HitNormal, AActor* HitActor)
{
}

void AAIController::AdjustFromWall(FVector HitNormal, AActor* HitActor)
{
	if ( bAdjustFromWalls
		&& ((GetStateFrame()->LatentAction == AI_PollMoveTo)
			|| (GetStateFrame()->LatentAction == AI_PollMoveToward)) )
	{
/* FIXMESTEVE
		if ( Pawn && MoveTarget )
		{
            AMover *HitMover = HitActor ? HitActor->GetAMover() : NULL;//Cast<AMover>(HitActor);
			if ( HitMover && MoveTarget->HasAssociatedLevelGeometry(HitMover) )
			{
				ANavigationPoint *Nav = Cast<ANavigationPoint>(MoveTarget);
				if ( !Nav || !Nav->bSpecialMove || !Nav->eventSuggestMovePreparation(Pawn) )
					eventNotifyHitMover(HitNormal,HitMover);
				return;
			}
		}
*/
		if ( bAdjusting )
		{
			MoveTimer = -1.f;
		}
		else
		{
			Pawn->SerpentineDir *= -1.f;
			if ( !Pawn->PickWallAdjust(HitNormal, HitActor) )
				MoveTimer = -1.f;
		}
	}
}

void AController::SetAdjustLocation(FVector NewLoc)
{
}

void AAIController::SetAdjustLocation(FVector NewLoc)
{
	bAdjusting = true;
	AdjustLoc = NewLoc;

}

void APlayerController::PostScriptDestroyed()
{
	PlayerInput = NULL;
	CheatManager = NULL;
	Super::PostScriptDestroyed();
}

void AController::PostBeginPlay()
{
	Super::PostBeginPlay();
	if ( !bDeleteMe )
	{
		GWorld->AddController( this );
	}
}

void AController::PostScriptDestroyed()
{
	Super::PostScriptDestroyed();
	GWorld->RemoveController( this );
}

void AController::UpdatePawnRotation()
{
	if ( Focus )
	{
		ANavigationPoint *NavFocus = Cast<ANavigationPoint>(Focus);
		if ( NavFocus && CurrentPath && CurrentPath->Start && (MoveTarget == NavFocus) && !Pawn->Velocity.IsZero() )
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
		else
		{
			FocalPoint = Focus->Location;
		}
	}

	// rotate pawn toward focus
	Pawn->rotateToward(FocalPoint);

	// face same direction as pawn
	Rotation = Pawn->Rotation;
}

/** SetRotationRate()
returns how fast pawn should rotate toward desired rotation
*/
FRotator AController::SetRotationRate(FLOAT deltaTime)
{
	if (Pawn == NULL || Pawn->IsHumanControlled())
	{
		return FRotator(0,0,0);
	}
	else
	{
		if ( bForceDesiredRotation )
		{
			Pawn->DesiredRotation = DesiredRotation;
		}
		return FRotator(appRound(RotationRate.Pitch * deltaTime), appRound(RotationRate.Yaw * deltaTime), appRound(RotationRate.Roll * deltaTime));
	}
}

void APlayerReplicationInfo::UpdatePing(FLOAT TimeStamp)
{
	// calculate round trip time
	FLOAT NewPing = ::Min(1.5f, WorldInfo->TimeSeconds - TimeStamp);

	if ( ExactPing < 0.004f )
	{
		// initialize ping
		ExactPing = ::Min(0.3f,NewPing);
	}
	else
	{
		// reduce impact of sudden transient ping spikes
		if ( NewPing > 2.f * ExactPing )
			NewPing = ::Min(NewPing, 3.f*ExactPing);

		// calculate approx. moving average
		ExactPing = ::Min(0.99f, 0.99f * ExactPing + 0.01f * NewPing); 
	}

	// since Ping is a byte, scale the value stored to maximize resolution (since ping is clamped to 1 sec)
	Ping = ::Min(appFloor(250.f * ExactPing), 255);
}

/** DesiredDirection()
returns the direction in which the controller wants its pawn to move.
*/
FVector AController::DesiredDirection()
{
	return Pawn->Velocity;
}

/** DesiredDirection()
returns the direction in which the controller wants its pawn to move.
AAIController implementation assumes destination is set properly.
*/
FVector AAIController::DesiredDirection()
{
	return Destination - Pawn->Location;
}

/**
 * Native function to determine if voice data should be received from this player.
 * Only called on the server to determine whether voice packet replication
 * should happen for the given sender.
 *
 * NOTE: This function is final because it can be called n^2 number of times
 * in a given frame, where n is the number of players. Change/overload this
 * function with caution as this can affect your network performance.
 *
 * @param Sender the player to check for mute status
 *
 * @return TRUE if this player is muted, FALSE otherwise
 */
UBOOL APlayerController::IsPlayerMuted(const FUniqueNetId& Sender)
{
	// Search the list for a matching Uid
	for (INT Index = 0; Index < VoicePacketFilter.Num(); Index++)
	{
		// Compare them as QWORDs for speed
		if ((QWORD&)VoicePacketFilter(Index) == (QWORD&)Sender)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/** 
 * Returns the player controller associated with this net id
 *
 * @param PlayerNetId the id to search for
 *
 * @return the player controller if found, otherwise NULL
 */
APlayerController* APlayerController::GetPlayerControllerFromNetId(FUniqueNetId PlayerNetId)
{
	// Get the world info so we can iterate the list
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	check(WorldInfo);
	// Iterate through the controller list looking for the net id
	for (AController* Controller = WorldInfo->ControllerList;
		Controller != NULL;
		Controller = Controller->NextController)
{
		// Determine if this is a player with replication
		APlayerController* PlayerController = Controller->GetAPlayerController();
		if (PlayerController != NULL && PlayerController->PlayerReplicationInfo != NULL)
		{
			// If the ids match, then this is the right player. Compare as QWORD for speed
			if ((QWORD&)PlayerController->PlayerReplicationInfo->UniqueId == (QWORD&)PlayerNetId)
			{
				return PlayerController;
			}
		}
	}
	return NULL;
}

/** called to notify the server when the client has loaded a new world via seamless travelling
 * @param WorldPackageName the name of the world package that was loaded
 */
void APlayerController::ServerNotifyLoadedWorld(FName WorldPackageName)
{
	if (GWorld->IsServer())
	{
		// update our info on what world the client is in
		UNetConnection* Connection = Cast<UNetConnection>(Player);
		if (Connection != NULL)
		{
			Connection->ClientWorldPackageName = WorldPackageName;
		}

		// if both the server and this client have completed the transition, handle it
		if (!GSeamlessTravelHandler.IsInTransition() && WorldPackageName == GWorld->GetOutermost()->GetFName() && GWorld->GetWorldInfo()->Game != NULL)
		{
			GWorld->GetWorldInfo()->Game->eventHandleSeamlessTravelPlayer(this);
		}
	}
}

/** returns whether the client has completely loaded the server's current world (valid on server only) */
UBOOL APlayerController::HasClientLoadedCurrentWorld()
{
	UNetConnection* Connection = Cast<UNetConnection>(Player);
	if (Connection != NULL)
	{
		if (Connection->GetUChildConnection())
		{
			Connection = ((UChildConnection*)Connection)->Parent;
		}
		return (Connection->ClientWorldPackageName == GWorld->GetOutermost()->GetFName());
	}
	else
	{
		// if we have no client connection, we're local, so we always have the current world
		return TRUE;
	}
}

/** 
 * worker function for APlayerController::SmoothTargetViewRotation()
 */
INT BlendRot(FLOAT DeltaTime, INT BlendC, INT NewC)
{
	if ( Abs(BlendC - NewC) > 32767 )
	{
		if ( BlendC > NewC )
			NewC += 65536;
		else
			BlendC += 65536;
	}
	if ( Abs(BlendC - NewC) > 4096 )
		BlendC = NewC;
	else
		BlendC = appTrunc(BlendC + (NewC - BlendC) * ::Min(1.f,24.f * DeltaTime));

	return (BlendC & 65535);
}

/**
 * Called client-side to smoothly interpolate received TargetViewRotation (result is in BlendedTargetViewRotation)
 * @parameter TargetPawn   is the pawn which is the current ViewTarget
 * @parameter DeltaSeconds is the time interval since the last smoothing update
 */
void APlayerController::SmoothTargetViewRotation(APawn* TargetPawn, FLOAT DeltaSeconds)
{
	if ( TargetPawn->bSimulateGravity )
		TargetViewRotation.Roll = 0;
	BlendedTargetViewRotation.Pitch = BlendRot(DeltaSeconds, BlendedTargetViewRotation.Pitch, TargetViewRotation.Pitch & 65535);
	BlendedTargetViewRotation.Yaw = BlendRot(DeltaSeconds, BlendedTargetViewRotation.Yaw, TargetViewRotation.Yaw & 65535);
	BlendedTargetViewRotation.Roll = BlendRot(DeltaSeconds, BlendedTargetViewRotation.Roll, TargetViewRotation.Roll & 65535);
}

/** 
 * This will turn the subtitles on or off depending on the value of bValue 
 *
 * @param bValue  to show or not to show
 **/
void APlayerController::SetShowSubtitles( UBOOL bValue )
{
	ULocalPlayer* LP = NULL;

	// Only let player 0 control the subtitle setting.
	LP = Cast<ULocalPlayer>(Player);

	if(LP && UUIInteraction::GetPlayerIndex(LP)==0)
	{
		// if we are not forcing subtitles to be off then use the value passed in
		if( GEngine->bSubtitlesForcedOff == FALSE )
		{
			GEngine->bSubtitlesEnabled = bValue;
		}
		// if we are forcing them off then disable subtitles in case they were some how magically turned on
		else
		{
			GEngine->bSubtitlesEnabled = FALSE;
		}


		debugf(TEXT("Changing subtitle setting, new value: %i"), GEngine->bSubtitlesEnabled);
	}
}

/** 
* This will turn return whether the subtitles are on or off
*
**/
UBOOL APlayerController::IsShowingSubtitles()
{
	return GEngine->bSubtitlesEnabled;
}

/** allows the game code an opportunity to modify post processing settings
 * @param PPSettings - the post processing settings to apply
 */
void APlayerController::ModifyPostProcessSettings(FPostProcessSettings& PPSettings) const
{
	if (PlayerCamera != NULL)
	{
		PlayerCamera->ModifyPostProcessSettings(PPSettings);
	}
}


/**
 * @return whether or not this Controller has Tilt Turned on
 **/
UBOOL APlayerController::IsControllerTiltActive() const
{
	UBOOL Retval = FALSE;

	return Retval;
}


/**
 * sets whether or not the Tilt functionality is turned on
 **/
void APlayerController::SetControllerTiltActive( UBOOL bActive )
{
}


/**
 * sets whether or not to ONLY use the tilt input controls
 **/
void APlayerController::SetOnlyUseControllerTiltInput( UBOOL bActive )
{
}


/**
 * sets whether or not to use the tilt forward and back input controls
 **/
void APlayerController::SetUseTiltForwardAndBack( UBOOL bActive )
{
}


