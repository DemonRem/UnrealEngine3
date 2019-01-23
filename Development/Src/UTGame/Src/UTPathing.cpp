//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "UTGameOnslaughtClasses.h"
//#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "UnPath.h"

IMPLEMENT_CLASS(AUTJumpPad);
IMPLEMENT_CLASS(UUTTrajectoryReachSpec);
IMPLEMENT_CLASS(UUTJumpPadReachSpec);
IMPLEMENT_CLASS(AUTTeamPlayerStart);
IMPLEMENT_CLASS(AUTScout);
IMPLEMENT_CLASS(AUTDefensePoint);

void AUTJumpPad::addReachSpecs(AScout *Scout, UBOOL bOnlyChanged)
{
	Super::addReachSpecs(Scout,bOnlyChanged);

	if ( GetGravityZ() >= 0.f )
	{
		GWarn->MapCheck_Add( MCTYPE_ERROR, this, TEXT("Jumppads not allowed in zero or positive gravity volumes!"));
		return;
	}

	// check that there are no other overlapping navigationpoints
	for (ANavigationPoint *Nav = GWorld->GetFirstNavigationPoint(); Nav != NULL; Nav = Nav->nextNavigationPoint)
	{
		if (Nav != NULL &&
			!Nav->bDeleteMe &&
			Nav != this &&
			(!bOnlyChanged || bPathsChanged || Nav->bPathsChanged) ) 
		{
			FVector XYDir = Nav->Location - Location;
			FLOAT ZDiff = Abs(XYDir.Z);
			if ( ZDiff < CylinderComponent->CollisionHeight + Nav->CylinderComponent->CollisionHeight )
			{
				XYDir.Z = 0.f;
				FLOAT MinDist = CylinderComponent->CollisionRadius + Nav->CylinderComponent->CollisionRadius;
				if ( XYDir.SizeSquared() < MinDist*MinDist )
				{
					GWarn->MapCheck_Add( MCTYPE_ERROR, Nav, TEXT("NavigationPoints must not touch a jumppad!"));
				}
			}
		}
	}

	if ( !JumpTarget )
	{
		GWarn->MapCheck_Add( MCTYPE_ERROR, this, TEXT("No JumpTarget set for this JumpPad"));
		return;
	}

	if ( CalculateJumpVelocity(Scout) )
	{
		UReachSpec *newSpec = ConstructObject<UReachSpec>(UUTJumpPadReachSpec::StaticClass(),GWorld->GetOuter(),NAME_None);

		// debugf(TEXT("Add Jump Reachspec for JumpPad at (%f, %f, %f)"), Location.X,Location.Y,Location.Z);
		FVector HumanSize = Scout->GetSize(((AUTScout*)(AUTScout::StaticClass()->GetDefaultActor()))->SizePersonFindName);
		newSpec->CollisionRadius = appTrunc(HumanSize.X);
		newSpec->CollisionHeight = appTrunc(HumanSize.Y);
		newSpec->Start = this;
		newSpec->End = JumpTarget;
		newSpec->reachFlags = R_JUMP + R_WALK;

		FVector JumpXY = JumpVelocity;
		JumpXY.Z = 0.f;
		// scale distance based on speed
		newSpec->Distance = (INT)((Location - JumpTarget->Location).Size() * ::Min(1.f, 1.5f*Scout->TestGroundSpeed/JumpXY.Size()));
		PathList.AddItem(newSpec);
	}
	else
	{
		GWarn->MapCheck_Add(MCTYPE_ERROR, this, *FString::Printf(TEXT("Jump to %s cannot be successfully made"), *JumpTarget->GetName()));
	}
}

UBOOL AUTJumpPad::CalculateJumpVelocity(AScout *Scout)
{
	if ( !JumpTarget )
	{
		JumpVelocity = FVector(0.f, 0.f, 0.f);
		return false;
	}

	FVector HumanSize = Scout->GetSize( ((AUTScout*)(AUTScout::StaticClass()->GetDefaultActor()))->SizePersonFindName);

	FVector Flight = JumpTarget->Location - Location;
	FLOAT FlightZ = Flight.Z;
	Flight.Z = 0.f;
	FLOAT FlightSize = Flight.Size();

	if ( FlightSize == 0.f )
	{
		JumpVelocity = FVector(0.f, 0.f, 0.f);
		return false;
	}

	FLOAT Gravity = GetGravityZ(); 

	FLOAT XYSpeed = FlightSize/JumpTime;
	FLOAT ZSpeed = FlightZ/JumpTime - Gravity * JumpTime;

	// trace check trajectory
	UBOOL bFailed = true;
	FVector FlightDir = Flight/FlightSize;
	FCheckResult Hit(1.f);

	// look for unobstructed trajectory, by increasing or decreasing flighttime
	UBOOL bDecreasing = true;
	FLOAT AdjustedJumpTime = JumpTime;

	while ( bFailed )
	{
		FVector StartVel = XYSpeed*FlightDir + FVector(0.f,0.f,ZSpeed);
		FLOAT StepSize = 0.0625f;
		FVector TraceStart = Location;
		bFailed = false;

		// trace trajectory to make sure it isn't obstructed
		for ( FLOAT Step=0.f; Step<1.f; Step+=StepSize )
		{
			FLOAT FlightTime = (Step+StepSize) * AdjustedJumpTime;
			FVector TraceEnd = Location + StartVel*FlightTime + FVector(0.f, 0.f, Gravity * FlightTime * FlightTime);
			if ( !GWorld->SingleLineCheck( Hit, this, TraceEnd, TraceStart, TRACE_World|TRACE_StopAtAnyHit, HumanSize ) )
			{
				bFailed = true;
				break;
			}
			TraceStart = TraceEnd;
		}

		if ( bFailed )
		{
			if ( bDecreasing )
			{
				AdjustedJumpTime -= 0.1f*JumpTime;
				if ( AdjustedJumpTime < 0.5f*JumpTime )
				{
					bDecreasing = false;
					AdjustedJumpTime = JumpTime + 0.2f*JumpTime;
				}
			}
			else
			{
				AdjustedJumpTime += 0.2f*JumpTime;
				if ( AdjustedJumpTime > 2.f*JumpTime )
				{
					// no valid jump possible
					JumpVelocity = FVector(0.f, 0.f, 0.f);
					return false;
				}

				XYSpeed = FlightSize/AdjustedJumpTime;
				ZSpeed = FlightZ/AdjustedJumpTime - Gravity * AdjustedJumpTime;
			}
		}
	}
	JumpVelocity = XYSpeed*FlightDir + FVector(0.f,0.f,ZSpeed);
	return true;
}

static void DrawLineArrow(FPrimitiveDrawInterface* PDI,const FVector &start,const FVector &end,const FColor &color,FLOAT mag)
{
	// draw a pretty arrow
	FVector dir = end - start;
	FLOAT dirMag = dir.Size();
	dir /= dirMag;
	FMatrix arrowTM = FMatrix::Identity;
	arrowTM.SetOrigin(start);
	FVector YAxis, ZAxis;
	dir.FindBestAxisVectors(YAxis,ZAxis);
	arrowTM.SetAxis(0,dir);
	arrowTM.SetAxis(1,YAxis);
	arrowTM.SetAxis(2,ZAxis);
	DrawDirectionalArrow(PDI,arrowTM,color,dirMag,mag,SDPG_World);
}

void UUTTrajectoryReachSpec::AddToDebugRenderProxy(FDebugRenderSceneProxy* DRSP)
{
#if GEMINI_TODO
	if ( Start && End.Nav )
	{
		FVector InitialVelocity = GetInitialVelocity();
		if ( InitialVelocity.IsZero() )
		{
			Super::Render(PDI);
			return;
		}
		FLOAT TotalFlightTime = (End->Location - Start->Location).Size2D()/InitialVelocity.Size2D();
		FLOAT Gravity = Start->GetGravityZ(); 
		FLOAT StepSize = 0.0625f;
		FVector TraceStart = Start->Location;

		for ( FLOAT Step=StepSize; Step<=1.f; Step+=StepSize )
		{
			FLOAT FlightTime = Step * TotalFlightTime;
			FVector TraceEnd = Start->Location + InitialVelocity*FlightTime + FVector(0.f, 0.f, Gravity * FlightTime * FlightTime);
			if ( Step+StepSize > 1.f )
				DrawLineArrow(PDI,TraceStart,TraceEnd,FLinearColor(PathColor().X,PathColor().Y,PathColor().Z,PathColor().W),8.f);
			else
				DrawLineArrow(PDI,TraceStart,TraceEnd,FLinearColor(PathColor().X,PathColor().Y,PathColor().Z,PathColor().W),0.f);
			TraceStart = TraceEnd;
		}
	}
#endif // GEMINI_TODO
}

FVector UUTJumpPadReachSpec::GetInitialVelocity()
{
	AUTJumpPad* JumpStart = Cast<AUTJumpPad>(Start);
	return JumpStart ? JumpStart->JumpVelocity : FVector(0.f,0.f,0.f);
}

FVector UUTTranslocatorReachSpec::GetInitialVelocity()
{
	return CachedVelocity;
}

INT UUTJumpPadReachSpec::CostFor(APawn* P)
{
	// vehicles can't use jump pads
	return P->IsA(AVehicle::StaticClass()) ? UCONST_BLOCKEDPATHCOST : Super::CostFor(P);
}

void AUTJumpPad::PostEditChange(UProperty* PropertyThatChanged)
{
	AScout*	Scout = FPathBuilder::GetScout();
	CalculateJumpVelocity(Scout);
	FPathBuilder::DestroyScout();

	Super::PostEditChange(PropertyThatChanged);
}

void AUTJumpPad::PostEditMove(UBOOL bFinished)
{
	if ( bFinished )
	{
		AScout*	Scout = FPathBuilder::GetScout();
		CalculateJumpVelocity(Scout);
	}

	Super::PostEditMove( bFinished );
}

void AUTPickupFactory::PostEditMove(UBOOL bFinished)
{
	if ( bFinished )
	{
		// align pickupbase mesh to floor
		if ( BaseMesh )
		{
			FCheckResult Hit(1.f);
			FLOAT CollisionHeight, CollisionRadius;
			GetBoundingCylinder(CollisionRadius, CollisionHeight);
			GWorld->SingleLineCheck( Hit, this, Location - FVector(0.f,0.f,1.5f*CollisionHeight), Location, TRACE_World, GetCylinderExtent() );
			if ( Hit.Time < 1.f )
			{
				Rotation = FindSlopeRotation(Hit.Normal, Rotation);
				FVector DefaultTranslation = Cast<AUTPickupFactory>(GetClass()->GetDefaultActor())->BaseMesh->Translation;
				BaseMesh->Translation = DefaultTranslation - FVector(CollisionRadius * (1.f - Hit.Normal.Z*Hit.Normal.Z));
				BaseMesh->ConditionalUpdateTransform();
			}
		}
	}

	Super::PostEditMove( bFinished );
}

void AUTPickupFactory::Spawned()
{
	Super::Spawned();

	if ( !GWorld->HasBegunPlay() )
	{
		PostEditMove( TRUE );
	}
}

void AUTTeamPlayerStart::PostEditChange(UProperty* PropertyThatChanged)
{
	UTexture2D* NewSprite = NULL;
	if (TeamNumber < TeamSprites.Num())
	{
		NewSprite = TeamSprites(TeamNumber);
	}
	else
	{
		// get sprite from defaults
		AUTTeamPlayerStart* Default = GetArchetype<AUTTeamPlayerStart>();
		for (INT i = 0; i < Default->Components.Num() && NewSprite == NULL; i++)
		{
			USpriteComponent* SpriteComponent = Cast<USpriteComponent>(Default->Components(i));
			if (SpriteComponent != NULL)
			{
				NewSprite = SpriteComponent->Sprite;
			}
		}
	}

	if (NewSprite != NULL)
	{
		// set the new sprite as the current one
		USpriteComponent* SpriteComponent = NULL;
		for (INT i = 0; i < Components.Num() && SpriteComponent == NULL; i++)
		{
			SpriteComponent = Cast<USpriteComponent>(Components(i));
		}
		if (SpriteComponent != NULL)
		{
			SpriteComponent->Sprite = NewSprite;
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

void AUTTeamPlayerStart::Spawned()
{
	Super::Spawned();

	PostEditChange(NULL);
}

void AUTDefensePoint::PostEditChange(UProperty* PropertyThatChanged)
{
	UTexture2D* NewSprite = NULL;
	if (DefendedObjective != NULL && DefendedObjective->DefenderTeamIndex < TeamSprites.Num())
	{
		NewSprite = TeamSprites(DefendedObjective->DefenderTeamIndex);
	}
	else
	{
		// get sprite from defaults
		AUTDefensePoint* Default = GetArchetype<AUTDefensePoint>();
		for (INT i = 0; i < Default->Components.Num() && NewSprite == NULL; i++)
		{
			USpriteComponent* SpriteComponent = Cast<USpriteComponent>(Default->Components(i));
			if (SpriteComponent != NULL)
			{
				NewSprite = SpriteComponent->Sprite;
			}
		}
	}

	if (NewSprite != NULL)
	{
		// set the new sprite as the current one
		USpriteComponent* SpriteComponent = NULL;
		for (INT i = 0; i < Components.Num() && SpriteComponent == NULL; i++)
		{
			SpriteComponent = Cast<USpriteComponent>(Components(i));
		}
		if (SpriteComponent != NULL)
		{
			SpriteComponent->Sprite = NewSprite;
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

void AUTDefensePoint::Spawned()
{
	Super::Spawned();

	// by default, all defense points have their own group
	DefenseGroup = GetFName();

	PostEditChange(NULL);
}


//@todo steve - eventually UTScout should become pathbuilder, spawns a UTPawn to do pathfinding
UBOOL AUTScout::SuggestJumpVelocity(FVector& JumpVelocity, FVector End, FVector Start)
{
	bRequiresDoubleJump = false;
	if ( Super::SuggestJumpVelocity(JumpVelocity, End, Start) )
		return true;

	//@todo steve - need to check if acceptable landing, if so no doublejump
	return false;

	bRequiresDoubleJump = true;

	// calculate using effective JumpZ with doublejump
	FLOAT RealJumpZ = JumpZ;
	JumpZ = 1.375f * JumpZ;
	UBOOL bResult = Super::SuggestJumpVelocity(JumpVelocity, End, Start);
	JumpZ = RealJumpZ;
	return bResult;
}

ETestMoveResult AUTScout::FindJumpUp(FVector Direction, FVector &CurrentPosition)
{
	bRequiresDoubleJump = false;

	ETestMoveResult success = Super::FindJumpUp(Direction, CurrentPosition);
	if ( success != TESTMOVE_Stopped )
		return success;

	// only path build double jump up if human sized or smaller
	FVector HumanSize = GetSize(SizePersonFindName);
	if ( (HumanSize.X < CylinderComponent->CollisionRadius) || (HumanSize.Y < CylinderComponent->CollisionHeight) )
	{
		return success;
	}
	bRequiresDoubleJump = true;
	FCheckResult Hit(1.f);
	FVector StartLocation = CurrentPosition;
	FVector CollisionExtent = GetDefaultCollisionSize();

	TestMove(FVector(0,0,MaxDoubleJumpHeight - MaxStepHeight), CurrentPosition, Hit, CollisionExtent);
	success = walkMove(Direction, CurrentPosition, CollisionExtent, Hit, NULL, MINMOVETHRESHOLD);

	StartLocation.Z = CurrentPosition.Z;
	if ( success != TESTMOVE_Stopped )
	{
		TestMove(-1.f*FVector(0,0,MaxDoubleJumpHeight), CurrentPosition, Hit, CollisionExtent);
		// verify walkmove didn't just step down
		StartLocation.Z = CurrentPosition.Z;
		if ((StartLocation - CurrentPosition).SizeSquared() < MINMOVETHRESHOLD * MINMOVETHRESHOLD)
			return TESTMOVE_Stopped;
	}
	else
		CurrentPosition = StartLocation;

	return success;
}

UBOOL AUTPawn::SuggestJumpVelocity(FVector& JumpVelocity, FVector End, FVector Start)
{
	bRequiresDoubleJump = false;
	if ( Super::SuggestJumpVelocity(JumpVelocity, End, Start) )
		return true;

	if ( !bCanDoubleJump )
		return false;

	//@todo steve - need to check if acceptable landing, if so no doublejump
	return false;

	bRequiresDoubleJump = true;

	// calculate using effective JumpZ with doublejump
	FLOAT RealJumpZ = JumpZ;
	JumpZ = 1.375f * JumpZ;
	UBOOL bResult = Super::SuggestJumpVelocity(JumpVelocity, End, Start);

	// set start jumpvelocity so double jump can be full
	if ( JumpVelocity.Z <= RealJumpZ )
	{
		JumpVelocity.Z = RealJumpZ;
		bRequiresDoubleJump = false;
	}
	else
	{
		JumpVelocity.Z = appSqrt(JumpVelocity.Z*JumpVelocity.Z - RealJumpZ*RealJumpZ);  
	}
	JumpZ = RealJumpZ;
	return bResult;
}

ETestMoveResult AUTPawn::FindJumpUp(FVector Direction, FVector &CurrentPosition)
{
	bRequiresDoubleJump = false;

	ETestMoveResult success = Super::FindJumpUp(Direction, CurrentPosition);
	if ( (success != TESTMOVE_Stopped) || !bCanDoubleJump )
		return success;

	bRequiresDoubleJump = true;
	FCheckResult Hit(1.f);
	FVector StartLocation = CurrentPosition;
	FVector CollisionExtent = GetDefaultCollisionSize();

	TestMove(FVector(0,0,MaxDoubleJumpHeight - MaxStepHeight), CurrentPosition, Hit, CollisionExtent);
	success = walkMove(Direction, CurrentPosition, CollisionExtent, Hit, NULL, MINMOVETHRESHOLD);

	StartLocation.Z = CurrentPosition.Z;
	if ( success != TESTMOVE_Stopped )
	{
		TestMove(-1.f*FVector(0,0,MaxDoubleJumpHeight), CurrentPosition, Hit, CollisionExtent);
		// verify walkmove didn't just step down
		StartLocation.Z = CurrentPosition.Z;
		if ((StartLocation - CurrentPosition).SizeSquared() < MINMOVETHRESHOLD * MINMOVETHRESHOLD)
			return TESTMOVE_Stopped;
	}
	else
		CurrentPosition = StartLocation;

	return success;
}

/* TryJumpUp()
Check if could jump up over obstruction
*/
UBOOL AUTPawn::TryJumpUp(FVector Dir, FVector Destination, DWORD TraceFlags, UBOOL bNoVisibility)
{
	FVector Out = 14.f * Dir;
	FCheckResult Hit(1.f);
	FVector Up = FVector(0.f,0.f,MaxJumpHeight);

	if ( bNoVisibility )
	{
		// do quick trace check first
		FVector Start = Location + FVector(0.f, 0.f, CylinderComponent->CollisionHeight);
		FVector End = Start + Up;
		GWorld->SingleLineCheck(Hit, this, End, Start, TRACE_World);
		UBOOL bLowCeiling = Hit.Time < 1.f;
		if ( bLowCeiling )
		{
			End = Hit.Location;
		}
		GWorld->SingleLineCheck(Hit, this, Destination, End, TraceFlags);
		if ( (Hit.Time < 1.f) && (Hit.Actor != Controller->MoveTarget) )
		{
			if ( !bCanDoubleJump || bLowCeiling )
			{
				return false;
			}

			// try double jump LOS
			GWorld->SingleLineCheck(Hit, this, End, Start, TRACE_World);
			if ( Hit.Time < 1.f )
				End = Hit.Location;
			GWorld->SingleLineCheck(Hit, this, Destination, End, TraceFlags);
			if ( (Hit.Time < 1.f) && (Hit.Actor != Controller->MoveTarget) )
			{
				return false;
			}
		}
	}

	GWorld->SingleLineCheck(Hit, this, Location + Up, Location, TRACE_World, GetCylinderExtent());
	FLOAT FirstHit = Hit.Time;
	if ( FirstHit > 0.5f )
	{
		GWorld->SingleLineCheck(Hit, this, Location + Up * Hit.Time + Out, Location + Up * Hit.Time, TraceFlags, GetCylinderExtent());
		if ( (Hit.Time < 1.f) && bCanDoubleJump && (FirstHit == 1.f) )
		{
			// try double jump
			Up = FVector(0.f,0.f,MaxJumpHeight);
			FVector doubleUp(0.f,0.f,MaxDoubleJumpHeight);
			GWorld->SingleLineCheck(Hit, this, Location + doubleUp, Location + Up, TRACE_World, GetCylinderExtent());
			if ( Hit.Time > 0.25f )
			{
				if ( Hit.Time == 1.f )
					Hit.Location = Location + doubleUp;
				GWorld->SingleLineCheck(Hit, this, Hit.Location + Out, Hit.Location, TraceFlags, GetCylinderExtent());
			}
		}
		return (Hit.Time == 1.f);
	}
	return false;
}

INT AUTPawn::calcMoveFlags()
{
	return ( bCanWalk * R_WALK + bCanFly * R_FLY + bCanSwim * R_SWIM + bJumpCapable * R_JUMP + bCanDoubleJump * R_HIGHJUMP ); 
}


//============================================================
// UTScout path building

void AUTScout::SetPrototype()
{
	if ( PrototypePawnClass )
	{
		AUTPawn* DefaultPawn = Cast<AUTPawn>(PrototypePawnClass->GetDefaultActor());

		// override UTScout defaults
		PathSizes(0).Radius = DefaultPawn->CrouchRadius + 1;
		PathSizes(0).Height = DefaultPawn->CrouchHeight;

		PathSizes(1).Radius = DefaultPawn->CylinderComponent->CollisionRadius + 1;
		PathSizes(1).Height = DefaultPawn->CylinderComponent->CollisionHeight;

		TestJumpZ = DefaultPawn->JumpZ;
		TestGroundSpeed = DefaultPawn->GroundSpeed;
		MaxStepHeight = DefaultPawn->MaxStepHeight;
		MaxJumpHeight = DefaultPawn->MaxJumpHeight;
		MaxDoubleJumpHeight = DefaultPawn->MaxDoubleJumpHeight;
	}
}

void AUTScout::CreateTranslocatorPath(ANavigationPoint* Nav, ANavigationPoint* DestNav, FCheckResult Hit, UBOOL bOnlyChanged)
{
	if ( !DestNav->bNoAutoConnect && !DestNav->bDestinationOnly 
		&& ((DestNav->Location - Nav->Location).SizeSquared() < MaxTranslocDistSq) 
		&& GWorld->SingleLineCheck( Hit, Nav, DestNav->Location, Nav->Location, TRACE_World|TRACE_StopAtAnyHit )
		&& !Nav->CheckSatisfactoryConnection(DestNav) )
	{
		FVector TossVelocity(0.f,0.f,0.f);
		FLOAT DesiredZPct = 0.05f; // prefer nearly horizontal throws
		FLOAT FallbackDesiredZPct = 0.50f; // use more vertical throw if necessary
		AUTProjectile* DefaultProj = TranslocProjClass->GetDefaultObject<AUTProjectile>();
		FVector CollisionSize = DefaultProj->GetCylinderExtent();
		FLOAT TossSpeed = DefaultProj->Speed; 
		FLOAT BaseTossZ = DefaultProj->TossZ;
		FLOAT TerminalVelocity = (Nav->PhysicsVolume && Nav->PhysicsVolume->bWaterVolume) ? Nav->PhysicsVolume->TerminalVelocity : DefaultProj->TerminalVelocity;
		
		debugfSuppressed(NAME_DevPath,TEXT("ADDING transloc SPEC from %s to %s!"), *Nav->GetName(), *DestNav->GetName());
		// make sure can make throw
		if (	SuggestTossVelocity(&TossVelocity, DestNav->Location, Nav->Location, TossSpeed, BaseTossZ, DesiredZPct, CollisionSize,TerminalVelocity) 
			||	SuggestTossVelocity(&TossVelocity, DestNav->Location, Nav->Location+FVector(0.f,0.f,50.f), TossSpeed, BaseTossZ, DesiredZPct, CollisionSize,TerminalVelocity)
			||	SuggestTossVelocity(&TossVelocity, DestNav->Location, Nav->Location, TossSpeed, BaseTossZ, FallbackDesiredZPct, CollisionSize,TerminalVelocity) )
		{
			// add a new transloc spec
			UUTTranslocatorReachSpec* newSpec = ConstructObject<UUTTranslocatorReachSpec>(UUTTranslocatorReachSpec::StaticClass(), GWorld->GetOuter(), NAME_None);
			Nav->bPathsChanged = Nav->bPathsChanged || !bOnlyChanged;
			newSpec->CollisionRadius = appTrunc(PathSizes(1).Radius);
			newSpec->CollisionHeight = appTrunc(PathSizes(1).Height);
			newSpec->Start = Nav;
			newSpec->End = DestNav;
			newSpec->Distance = appTrunc((Nav->Location - DestNav->Location).Size());
			newSpec->CachedVelocity = TossVelocity + FVector(0.f,0.f,BaseTossZ);
			Nav->PathList.AddItem(newSpec);

			// find out if it could ever be possible to jump to Nav and if so what JumpZ would be required
			SetCollisionSize(newSpec->CollisionRadius, newSpec->CollisionHeight);
			if ( Nav->PlaceScout(this) )
			{
				FVector CurrentPosition = Location;
				if ( DestNav->PlaceScout(this) )
				{
					FVector DesiredPosition = Location;
					FVector JumpVelocity(0.f, 0.f, 0.f);
					if ( SuggestJumpVelocity(JumpVelocity, DesiredPosition, CurrentPosition) &&
						FindBestJump(DesiredPosition, CurrentPosition) == TESTMOVE_Moved &&
						ReachedDestination(CurrentPosition, DestNav->Location, DestNav) )
					{
						// record the JumpZ needed to make this jump and the current gravity
						newSpec->RequiredJumpZ = JumpVelocity.Z;
						newSpec->OriginalGravityZ = newSpec->Start->GetGravityZ();
						debugfSuppressed(NAME_DevPath,TEXT("Succeeded jump test with RequiredJumpZ %f"), newSpec->RequiredJumpZ);
					}
				}
			}
		}
		else
		{
			debugfSuppressed(NAME_DevPath,TEXT("Failed toss from %s to %s"), *Nav->GetName(), *DestNav->GetName());
		}

		//@todo steve init for dodging up slopes!!!
	}
}

void AUTScout::AddSpecialPaths(INT NumPaths, UBOOL bOnlyChanged)
{
	debugf(TEXT("Adding special UT paths to %s"), *GWorld->GetOutermost()->GetName());
	
	// skip translocator paths for onslaught levels
	UUTMapInfo* MapInfo = Cast<UUTMapInfo>(GWorld->GetWorldInfo()->GetMapInfo());
	if (MapInfo != NULL && !MapInfo->bBuildTranslocatorPaths)
	{
		debugf(TEXT("Skip translocator paths due to MapInfo setting"));
		return;
	}
	else if (TranslocProjClass == NULL)
	{
		debugf(TEXT("No translocator projectile class"));
		return;
	}
	INT NumDone = 0;

	// temporarily set Scout's JumpZ insanely high for lowgrav/jumpboots/etc jump reach test
	FLOAT OldJumpZ = JumpZ;
	JumpZ = 100000.f;
	FCheckResult Hit(1.f);
	INT TwiceNumPaths = 2 * NumPaths;

	// add translocator paths to reverse connected destinations
	for( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav; Nav=Nav->nextNavigationPoint )
	{
		if ( (!bOnlyChanged || Nav->bPathsChanged) && !Nav->bNoAutoConnect && !Nav->PhysicsVolume->bWaterVolume )
		{
			GWarn->StatusUpdatef(NumDone, TwiceNumPaths, *SpecialReachSpecsWarningLog.ToString());
			NumDone++;

			for ( INT i=0; i<Nav->PathList.Num(); i++ )
			{
				CreateTranslocatorPath(Nav->PathList(i)->End, Nav, Hit, bOnlyChanged);
			}
		}
	}

	// add translocator paths to unconnected destinations
	for( ANavigationPoint *Nav=GWorld->GetFirstNavigationPoint(); Nav; Nav=Nav->nextNavigationPoint )
	{
		if ( (!bOnlyChanged || Nav->bPathsChanged) && !Nav->bNoAutoConnect && !Nav->PhysicsVolume->bWaterVolume )
		{
			GWarn->StatusUpdatef(NumDone, TwiceNumPaths, *SpecialReachSpecsWarningLog.ToString());
			NumDone++;

			// check all visible paths within MaxTranslocDist that don't already have a good path to them
			for( ANavigationPoint *DestNav=GWorld->GetFirstNavigationPoint(); DestNav; DestNav=DestNav->nextNavigationPoint )
			{
				CreateTranslocatorPath(Nav, DestNav, Hit, bOnlyChanged);
			}
		}
	}

	JumpZ = OldJumpZ;
}

static AUTGameObjective* TestObjective = NULL;

IMPLEMENT_COMPARE_POINTER(ANavigationPoint, UTPathing, { return appTrunc(appSqrt((TestObjective->Location - A->Location).SizeSquared() - (TestObjective->Location - B->Location).SizeSquared())); })

void AUTGameObjective::AddForcedSpecs(AScout* Scout)
{
	// put the five closest visible NavigationPoints in the ShootSpots array

	// create list of all non-blocking non-flying source NavigationPoints
	TArray<ANavigationPoint*> NavList;
	for (ANavigationPoint* N = GWorld->GetFirstNavigationPoint(); N != NULL; N = N->nextNavigationPoint)
	{
		if (N != this && !N->bBlockActors && !N->bDestinationOnly && !N->bFlyingPreferred)
		{
			NavList.AddItem(N);
		}
	}

	// sort by distance
	TestObjective = this;
	Sort<USE_COMPARE_POINTER(ANavigationPoint,UTPathing)>(NavList.GetTypedData(), NavList.Num());
	TestObjective = NULL;

	// put the first five that succeed a visibility trace into the ShootSpots array
	ShootSpots.Empty();
	FCheckResult Hit(1.0f);
	FVector TargetLoc = GetTargetLocation();
	for (INT i = 0; i < NavList.Num(); i++)
	{
		if (GWorld->SingleLineCheck(Hit, Scout, TargetLoc, NavList(i)->Location, TRACE_World | TRACE_StopAtAnyHit))
		{
			ShootSpots.AddItem(NavList(i));
			if (ShootSpots.Num() >= 5)
			{
				break;
			}
		}
	}

	// if bAllowOnlyShootable, we don't need to be reachable if we found any ShootSpots
	if (bAllowOnlyShootable && ShootSpots.Num() > 0)
	{
		bMustBeReachable = FALSE;
	}
	else
	{
		bMustBeReachable = GetArchetype<ANavigationPoint>()->bMustBeReachable;
	}
}

/*
 * Returns navigation anchor associated with this actor
 * (rather than actually checking if anchor is also reachable)
*/
ANavigationPoint* AUTCarriedObject::SpecifyEndAnchor(APawn* RouteFinder)
{
	if ( GWorld->GetTimeSeconds() - LastValidAnchorTime < 0.25f )
	{
		return LastAnchor;
	}
	return NULL;
}

/*
 * Notify actor of anchor finding result
 * @PARAM EndAnchor is the anchor found
 * @PARAM RouteFinder is the pawn which requested the anchor finding
 */
void AUTCarriedObject::NotifyAnchorFindingResult(ANavigationPoint* EndAnchor, APawn* RouteFinder)
{
	if ( EndAnchor )
	{
		// save valid anchor info
		LastValidAnchorTime = GWorld->GetTimeSeconds();
		LastAnchor = EndAnchor;
	}
	else
	{
		eventNotReachableBy(RouteFinder);
	}
}

/** Used to see when the 'base most' (ie end of Base chain) changes. */
void AUTCarriedObject::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);

	AActor* NewBase = GetBase();
	AActor* NewBaseBase = NULL;
	if(NewBase)
	{
		NewBaseBase = NewBase->GetBase();
	}
	

	if(NewBase != OldBase || NewBaseBase != OldBaseBase)
	{
		// Call script event when this happens.
		eventOnBaseChainChanged();
		OldBase = NewBase;
		OldBaseBase = NewBaseBase;

	}
}

