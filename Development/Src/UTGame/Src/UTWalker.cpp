/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAIClasses.h"

IMPLEMENT_CLASS(AUTVehicle_Walker);
IMPLEMENT_CLASS(AUTVehicle_Scavenger);
IMPLEMENT_CLASS(AUTVehicle_DarkWalker);
IMPLEMENT_CLASS(AUTWalkerBody);
IMPLEMENT_CLASS(AUTWalkerBody_Scavenger);
IMPLEMENT_CLASS(UUTWalkerStepHandle);

/** Utility function to move a vector into the given space */
static FVector TransformWorldToLocal(FVector const& WorldVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).Transpose().TransformNormal( WorldVect );
}
/** Utility function to move a vector out of a given space */
static FVector TransformLocalToWorld(FVector const& LocalVect, FRotator const& SystemRot)
{
	return FRotationMatrix(SystemRot).TransformNormal( LocalVect );
}

FLOAT AUTWalkerBody::GetGravityZ()
{
	return CustomGravityScale * GWorld->GetGravityZ();
}

UBOOL AUTVehicle_Walker::Tick( FLOAT DeltaTime, enum ELevelTick TickType )
{
	bWasOnGround = bVehicleOnGround;
	return Super::Tick(DeltaTime, TickType);
}

/** 
 * Returns TRUE if the walker is considered "moving" in the XY place, ie above
 * a certain XY velocity threshold.
 */
UBOOL AUTVehicle_Walker::IsMoving2D() const
{
	return (Velocity.SizeSquared2D() > 10000.f);
}

void AUTVehicle_Scavenger::InitStayUpright()
{
	if ( StayUprightConstraintInstance )
	{
//@TODO FIXMESTEVE Get this working
		StayUprightConstraintInstance->InitConstraint(NULL, CollisionComponent, StayUprightConstraintSetup, 1.0f, this, NULL, FALSE);
	}
}

void AUTVehicle_Scavenger::ImpactEffect(FVector HitPos)
{
	if(ImpactParticle)
	{
		FRotationTranslationMatrix LocalMatrix(Rotation,Location); // where we are
		FRotationTranslationMatrix HitMatrix((HitPos-Location).Rotation(),HitPos); // where we hit
		const FMatrix& ResultMatrix = HitMatrix * LocalMatrix.Inverse();
		const FRotator& ImpactRot = ResultMatrix.Rotator();
		const FVector& ImpactPos = ResultMatrix.GetOrigin();
		ImpactParticle->Rotation = ImpactRot;
		ImpactParticle->Translation = ImpactPos;
		ImpactParticle->BeginDeferredUpdateTransform();
		ImpactParticle->ActivateSystem();
		
	}
}

void AUTVehicle_Scavenger::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);
	if ( Role == ROLE_Authority )
	{
		// manager seeker
		if ( !Driver || bIsInBallMode )
		{
			if ( ActiveSeeker )
			{
				GWorld->DestroyActor( ActiveSeeker );
				ActiveSeeker = NULL;
			}
		}
		else if ( !ActiveSeeker || ActiveSeeker->bDeleteMe )
		{
			eventSpawnSeeker();
		}
	}

	if ( bDeadVehicle )
		return;

	if ( LandingFinishTime > WorldInfo->TimeSeconds )
	{
		Rise = -1.f;
	}
	else if ( bWasOnGround )
	{
		if ( bPreviousInAir )
		{
			if ( WorldInfo->TimeSeconds - InAirStart > 0.5f )
				LandingFinishTime = WorldInfo->TimeSeconds + 0.4f;
		}
		InAirStart = WorldInfo->TimeSeconds + 100.f;
	}
	else if ( !bPreviousInAir )
	{
		InAirStart = WorldInfo->TimeSeconds;
	}

	bPreviousInAir = !bWasOnGround;
	if ( Driver )
	{
		if ( !bIsInBallMode )
		{
			if ( Rise > 0.f )
			{
				// trying to jump
				//debugf(TEXT("bWasOnGround %d bHoldingDuck %d (WorldInfo->TimeSeconds - JumpDelay %f  >= LastJumpTime %f)"),bWasOnGround, bHoldingDuck, (WorldInfo->TimeSeconds - JumpDelay), LastJumpTime);
				if ( bWasOnGround && !bHoldingDuck && (WorldInfo->TimeSeconds - JumpDelay >= LastJumpTime) )
				{
					// If we are on the ground, and press Rise, and we not currently in the middle of a jump, start a new one.
					if ( Role == ROLE_Authority )
						bDoBikeJump = !bDoBikeJump;

					AddImpulse( FVector(0.f,0.f,JumpForceMag) );
					if ( Cast<AUTBot>(Controller) )
						Rise = 0.f;

					LastJumpTime = WorldInfo->TimeSeconds;
				}
			}
			else if ( Rise < 0.f )
			{
				// Duck!
				if ( !bHoldingDuck )
				{
					bHoldingDuck = TRUE;

					eventScavengerDuckEffect();
					
					if ( Cast<AUTBot>(Controller) )
						Rise = 0.f;
				}
			}
			else
			{
				if (bHoldingDuck)
				{
					bHoldingDuck = FALSE;
					eventScavengerDuckEffect();
				}
			}
			bNoZDamping = (WorldInfo->TimeSeconds - 0.25f < LastJumpTime);
			if(RollAudioComp)
			{
				RollAudioComp->Stop();
			}
		}
		else 
		{
			if(RollAudioComp)
			{
				const FLOAT curSpd = Velocity.Size2D();
				if(curSpd > 1.0f) // if we're moving or we're on the gas.
				{
					FCheckResult HitRes(1.0f);
					FTraceHitInfo HitInfo;
					FVector TraceStart(Location.X,Location.Y,Location.Z);
					if (CylinderComponent != NULL)
					{
						TraceStart.Z -= CylinderComponent->CollisionHeight - CylinderComponent->Translation.Z;
					}
					FVector EndPt(TraceStart.X,TraceStart.Y,TraceStart.Z-100);  // got these numbers from UTPawn's GetMaterialBelowFeet()

					//DrawDebugLine(TraceStart,EndPt,0,255,0,false);

					GWorld->SingleLineCheck(HitRes, this, EndPt, TraceStart, TRACE_World | TRACE_Material);

					DetermineCorrectPhysicalMaterial<FCheckResult, FTraceHitInfo>( HitRes, HitInfo );
					// we now have a phys material so we can see if we need to update the sound
					HitInfo.Material = HitRes.Material ? HitRes.Material->GetMaterial() : NULL;
					const UUTPhysicalMaterialProperty* UTPMP = (HitInfo.Material && HitInfo.Material->PhysMaterial)
															? Cast<UUTPhysicalMaterialProperty>(HitInfo.Material->PhysMaterial->PhysicalMaterialProperty)
															: NULL;
					if (RollAudioComp != NULL && UTPMP != NULL && UTPMP->MaterialType != CurrentRollMaterial) // if we're on a material that's not what we're on already.
					{
						INT match = -1;
						for(INT i=0;i<RollSoundList.Num();++i)
						{
							if(RollSoundList(i).MaterialType == UTPMP->MaterialType)
							{
								match = i;
								CurrentRollMaterial = UTPMP->MaterialType;
								break;
							}
						}
						if(match != -1)
						{
							if(RollAudioComp->bWasPlaying) // we have a new match, so fade out the old one and let the garbage collector take care of it.
							{
								RollAudioComp->FadeOut(0.3f,0.0f);
							}
							RollAudioComp = CreateAudioComponent(RollSoundList(match).Sound, FALSE, TRUE, FALSE);
						}
					}
					if (RollAudioComp != NULL)
					{
						if(HitRes.Time == 1.f)
						{
							RollAudioComp->Stop();
							BallAudio->Stop();
						}
						else
						{
							if(!RollAudioComp->bWasPlaying)
							{
								RollAudioComp->Play();
							}
							if(BallAudio)
							{
								BallAudio->AdjustVolume(0.1f, Min<FLOAT>(1.0f,curSpd/(AirSpeed*0.10f)));
								BallAudio->SetFloatParameter( FName(TEXT("PitchModulationParam")), SimObj->GetEngineOutput( this ) );
							}
							RollAudioComp->AdjustVolume(0.1f, Min<FLOAT>(1.0f,curSpd/(AirSpeed*0.10f)) ); // go to full volume if >10%, else to the % of 10%
							RollAudioComp->PitchMultiplier = 0.5f + 1.25f*(curSpd/AirSpeed); // 0 = 0.5, 40% = 1.0, 80% = 1.5
						}
					}
				}
				else if (RollAudioComp != NULL) // not moving, stop Rolls.
				{
					RollAudioComp->Stop();
				}
			}
			
			if( bSpinAttackActive && (Role == ROLE_Authority) && Instigator )
			{
				// hit all overlapping actors
				FMemMark Mark(GMem);
				FCheckResult* Link = GWorld->Hash->ActorOverlapCheck(GMem, this, Location, SpinAttackRadius);
				for( FCheckResult* result=Link; result; result=result->GetNext())
				{
					APawn* TargetPawn = Link->Actor ? Link->Actor->GetAPawn() : NULL;
					if ( TargetPawn && TargetPawn->IsValidEnemyTargetFor(Instigator->PlayerReplicationInfo, FALSE) )
					{
						eventSpinAttackVictim(TargetPawn, DeltaSeconds);
					}
					
				}
				Mark.Pop();
				if( WorldInfo->TimeSeconds - SpinStartTime > SpinTime )
				{
					eventEndBallMode();
				}
			}

			if ( (Role == ROLE_Authority) || IsLocallyControlled() )
			{
				if ( bBallBoostActivated &&  (WorldInfo->TimeSeconds - BoostStartTime > MaxBoostDuration) ) 
				{
					// Ran out of Boost
					eventDeactivateBallBooster();
				}
			}
		}
	}

	if( bDoBikeJump != bOldDoBikeJump )
	{
        bOldDoBikeJump = bDoBikeJump;
		eventScavengerJumpEffect();
	}
}


void AUTVehicle_DarkWalker::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( bDeadVehicle )
		return;

	/*
	if ( LandingFinishTime > WorldInfo->TimeSeconds )
	{
		Rise = -1.f;
	}
	else if ( bWasOnGround )
	{
		if ( bPreviousInAir )
		{
			if ( WorldInfo->TimeSeconds - InAirStart > 0.5f )
				LandingFinishTime = WorldInfo->TimeSeconds + 0.4f;
		}
		InAirStart = WorldInfo->TimeSeconds + 100.f;
	}
	else if ( !bPreviousInAir )
	{
		InAirStart = WorldInfo->TimeSeconds;
	}
	*/
	bPreviousInAir = !bWasOnGround;

	if ( Driver )
	{
		if ( Rise < 0.f )
		{
			// Duck!
			if ( !bHoldingDuck )
			{
				bHoldingDuck = true;
				eventWalkerDuckEffect();
				
				if ( Cast<AUTBot>(Controller) )
    				Rise = 0.f;
    		}
		}
		else if ( bHoldingDuck )
		{
			bHoldingDuck = false;
			eventWalkerDuckEffect();
		}
	}

	if ( bDriving )
	{
		for (FPlayerIterator It(GEngine); It; ++It)
		{
			if(It->Actor != NULL && It->Actor->Pawn != NULL && It->Actor->Pawn->IsLocallyControlled() && It->Actor->Pawn->IsHumanControlled())
			{
				PlayWarningSoundIfInCone(It->Actor->Pawn);
			}
		}
	}
	else
	{
		// put to sleep if not driving and suspension has settled
		if ( Mesh )
		{
			UBOOL bWheelsSettled = TRUE;
			INT const NumWheels = Wheels.Num();
			for (INT i=0; i<NumWheels; i++)
			{
				// NOTE:  testing floats here for equality are these going to get set correctly?
				if (Wheels(i)->SuspensionTravel != WheelSuspensionTravel[CurrentStance])
				{
					bWheelsSettled = FALSE;
					break;
				}
			}

			if (bWheelsSettled)
			{
				Mesh->PutRigidBodyToSleep();
			}
		}
	}
}

void AUTVehicle_DarkWalker::PlayWarningSoundIfInCone(APawn* Target)
{
	
	if(Target == NULL) { debugf(NAME_Warning, TEXT("No Target on Darkwalker Warning Cone Sound")); return; }
	if(WarningConeSound == NULL) { debugf(NAME_Warning, TEXT("No Sound for Darkwalker Warning Cone")); return;}
	FVector PointOnCone;
	FVector StartPt(0, 0, 0);
	FRotator ViewRotation(0, 0, 0);
	eventGetBarrelLocationAndRotation(0,StartPt,ViewRotation);
	
	const FVector& MyViewDirection = ViewRotation.Vector() * LengthDarkWalkerWarningCone; // = (WeaponRotation).Vector() * LengthDarkWalkerWarningCone;
	// HANDY DEBUG:
	//FColor DColor = FColor(0,255,0);
	//DrawDebugCone(StartPt,MyViewDirection, LengthDarkWalkerWarningCone, .52, .52, 10, DColor, false); 
	const FVector& VectorToTarget = Target->Location-StartPt;
	//FLOAT DistToTarget = VectorToTarget.Size();

	const FLOAT dotResult = VectorToTarget.SafeNormal() | MyViewDirection.SafeNormal(); // = cos of angle.

	
	FLOAT score;
	const FLOAT WithinAngle = 0.866025404f; // 30 DEGREES COS
	if(dotResult >= WithinAngle)
	{
		score = (dotResult-WithinAngle)/(1-WithinAngle);
		//score *= DistToTarget/LengthDarkWalkerWarningCone; // We're going to use an attenuation node
		//now we play sound at volume modifier score
		WarningConeSound->AdjustVolume(0.0f,score);
		WarningConeSound->SetFloatParameter(ConeParam,score);
		if(!(WarningConeSound->bWasPlaying) || WarningConeSound->bFinished) // either not playing or it was and it has finished
		{
			WarningConeSound->Play();
		}

	}
	else
	{
		WarningConeSound->Stop();
	}

	return;
	

}


FVector AUTVehicle_Walker::GetWalkerBodyLoc()
{
	FVector NewLoc;
	FRotator NewRot;
	Mesh->GetSocketWorldLocationAndRotation(BodyAttachSocketName, NewLoc, &NewRot);
	NewLoc = Location;
	return NewLoc;
}

FVector AUTVehicle_Scavenger::GetWalkerBodyLoc()
{
	FVector NewLoc; 
	FRotator NewRot;
	const AUTWalkerBody_Scavenger* Scav = Cast<AUTWalkerBody_Scavenger>(BodyActor);
	if(Scav)
	{
		Mesh->GetSocketWorldLocationAndRotation(Scav->SphereCenterName,NewLoc,&NewRot);
		if(BodyActor->SkeletalMeshComponent->PhysicsWeight != 0.f)
		{
			const FVector& CentLoc = Super::GetWalkerBodyLoc();
			const FVector& Diff = CentLoc - NewLoc;
			return CentLoc + (Diff * (1.f - Scav->SkeletalMeshComponent->PhysicsWeight));
		}
		else
		{
			return NewLoc;
		}
	}
	return Super::GetWalkerBodyLoc();
}

void AUTVehicle_Walker::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if (bDeadVehicle)
	{
		if(bAnimateDeadLegs && BodyActor)
		{
			BodyActor->AnimateLegs(DeltaSeconds, (WorldInfo->TimeSeconds - InAirStart > 0.6f) );
		}
		return;
	}

	// figure out what stance we're in
	{
		PreviousStance = CurrentStance;
		if ( bDriving )
		{
			CurrentStance = bHoldingDuck ? WalkerStance_Crouched : WalkerStance_Standing;
		}
		else
		{
			CurrentStance = WalkerStance_Parked;
		}
	}

	// update body handle to move body along with the vehicle
	if (BodyActor)
	{
		// make sure body handle is grabbed
		if ( !BodyHandle->GrabbedComponent )
		{
			URB_BodyInstance* const BodyInst = BodyActor->SkeletalMeshComponent->FindBodyInstanceNamed(BodyActor->BodyBoneName);
			if (BodyInst && BodyInst->IsValidBodyInstance())
			{
				FRotationMatrix R(Rotation);
				BodyHandle->GrabComponent(BodyActor->SkeletalMeshComponent, BodyActor->BodyBoneName, BodyInst->GetUnrealWorldTM().GetOrigin() + BaseBodyOffset.X*R.GetAxis(0) + BaseBodyOffset.Y*R.GetAxis(1) + BaseBodyOffset.Z*R.GetAxis(2), TRUE);
			}
		}
			
		// update body handle location
		FVector NewLoc;
		NewLoc = GetWalkerBodyLoc();
		BodyHandle->SetLocation(NewLoc);
		

		// update body handle orientation
		if (IsMoving2D())
		{
			// we are going to find which of 3 possible leg mesh orientations
			// is best to align the feet to the direction we are walking.
			// if we aren't walking, we'll just keep the orientation we have until we move again.
			FVector Vel2D = Velocity;
			Vel2D.Z = 0.f;
			FRotator const LegsRot = Vel2D.Rotation();

			FRotator const CurrentRot(BodyHandle->GetOrientation());
			INT const DeltaYaw = FRotator::NormalizeAxis(LegsRot.Yaw - CurrentRot.Yaw);

			FRotator GoalRot = LegsRot;
			if (Abs(DeltaYaw) > 10923)		// 10923 = (int)65536/6 = 60 deg
			{
				if (DeltaYaw < 0)
				{
					GoalRot.Yaw += 10923;
				}
				else
				{
					GoalRot.Yaw -= 10923;
				}
			}

			const FRotator& CurBodyHandleRot(BodyHandle->GetOrientation());
			const FRotator& InterpedGoalRot = RInterpTo(CurBodyHandleRot, GoalRot, DeltaSeconds, BodyHandleOrientInterpSpeed);
			BodyHandle->SetOrientation(InterpedGoalRot.Quaternion());		
		}

		BodyActor->AnimateLegs(DeltaSeconds, (WorldInfo->TimeSeconds - InAirStart > 0.6f) );
	}

	// adjust wheel suspension depending on whether driving, parked, or crouched
	const FLOAT DesiredSuspensionTravel = WheelSuspensionTravel[CurrentStance];

	const FLOAT ClampedDeltaSeconds = ::Min(DeltaSeconds, 0.1f);
	for (INT i = 0; i < Wheels.Num(); i++)
	{
		UBOOL bAdjustSuspension = false;

		if ( Wheels(i)->SuspensionTravel > DesiredSuspensionTravel )
		{
			bAdjustSuspension = true;
			Wheels(i)->SuspensionTravel = ::Max(DesiredSuspensionTravel, Wheels(i)->SuspensionTravel - SuspensionTravelAdjustSpeed*ClampedDeltaSeconds);
		}
		else if ( Wheels(i)->SuspensionTravel < DesiredSuspensionTravel )
		{
			bAdjustSuspension = true;
			Wheels(i)->SuspensionTravel = ::Min(DesiredSuspensionTravel, Wheels(i)->SuspensionTravel + SuspensionTravelAdjustSpeed*ClampedDeltaSeconds);
		}
		if ( bAdjustSuspension )
		{
			Wheels(i)->BoneOffset.Z = Wheels(i)->SuspensionTravel + Wheels(i)->WheelRadius;
#if WITH_NOVODEX
			NxWheelShape* WheelShape = Wheels(i)->GetNxWheelShape();
			SimObj->SetNxWheelShapeParams(WheelShape, Wheels(i));
#endif
		}
	}
}

/**
 * One-time initialization to set up the legs and feet. 
 */
void AUTWalkerBody::InitFeet()
{
	if ( (WalkerVehicle == NULL) || bIsDead )
	{
		return;
	}

	AssignFootDirections();
	CalcDesiredFootLocations();

	// assign an initial position to each foot
	for ( INT i=0; i<3; i++ )
	{
		StepStage[i] = -1;
		MoveFootToLoc(LegMapping[i], StepAnimData[i].DesiredFootPosition, StepAnimData[i].DesiredFootPosNormal, 0.f);
	}
}

void AUTWalkerBody::AssignFootDirections()
{
	// strategy here is to have overall minumum traversal

	// get directional vectors to desired foot locations
	FVector ToDesiredFootLocXYDir[WalkerLeg_MAX];
	{
		FRotator LegsRot;
		GetLegsFacingRot(LegsRot);

		for (INT i=0; i<WalkerLeg_MAX; ++i)
		{
			ToDesiredFootLocXYDir[i] = TransformLocalToWorld(BaseLegDirLocal[i], LegsRot);
		}
	}

	// get current shoulder directional vectors
	FVector CurrentShoulderXYDir[3];
	{
		for ( INT i=0; i<3; i++ )
		{
			// shoulder directions
			FRotator BodyHandleRot(WalkerVehicle->BodyHandle->GetOrientation());

			// note: there's a subtle dependence here that the model is set up such that
			// the leg indices match up with the EWalkerLegID enum.  That is, leg 0 in the model
			// is the rear, leg 1 is the front left, leg 2 is the front right.  If the model changes
			// it would be easy enough to store a separate BaseLegDirLocal in leg-index space.
			CurrentShoulderXYDir[i] = TransformLocalToWorld(BaseLegDirLocal[i], BodyHandleRot);
		}
	}

	// debug
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+CurrentShoulderXYDir[0]*380.f, 255, 0, 0, FALSE);	// red
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+CurrentShoulderXYDir[1]*380.f, 0, 255, 0, FALSE);	// green
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+CurrentShoulderXYDir[2]*380.f, 0, 0, 255, FALSE);	// blue
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+ToDesiredFootLocXYDir[0]*280.f, 255, 255, 0, FALSE);		// yellow
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+ToDesiredFootLocXYDir[1]*280.f, 0, 255, 255, FALSE);		// cyan
	//DrawDebugLine(WalkerVehicle->Location, WalkerVehicle->Location+ToDesiredFootLocXYDir[2]*280.f, 255, 0, 255, FALSE);		// magenta
	//DrawDebugLine(Location, Location+Rotation.Vector()*1024.f, 255, 255, 255, FALSE);

	// calc angles.  these will be in the range [0..Pi)
	// optimization -- can speed this up by minimizing acos calls?  might be able to get away with 5 acos calls to 
	// calc chain of delta angles (A to B, B to C, C to D, etc), then use addition to find final angles
	// Angles from shoulder to desired foot loc, range [0..Pi)
	FLOAT Leg0Angles[3], Leg1Angles[3], Leg2Angles[3];
	for (INT i=0; i<3; ++i)
	{
		// Clamps are necessary, since fp error in the dot can push the inputs to acos out of range,
		// resulting in NaNs.
		Leg0Angles[i] = appAcos(Clamp(CurrentShoulderXYDir[0] | ToDesiredFootLocXYDir[i], -1.f, 1.f));
		Leg1Angles[i] = appAcos(Clamp(CurrentShoulderXYDir[1] | ToDesiredFootLocXYDir[i], -1.f, 1.f));
		Leg2Angles[i] = appAcos(Clamp(CurrentShoulderXYDir[2] | ToDesiredFootLocXYDir[i], -1.f, 1.f));
	}

	// the 6 possible ways to map the feet
	static const INT CandiateLegMapping[6][3] = 
	{
		{0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,1,0}, {2,0,1},
	};

	// find best leg mapping (best meaning has the lowest total angular traversal to achieve).
	const INT* BestLegMapping = NULL;
	FLOAT BestCost = 99999.f;
	for (INT i=0; i<6; ++i)
	{
		const INT* const LM = CandiateLegMapping[i];
		const FLOAT AngularCost = Leg0Angles[LM[0]] + Leg1Angles[LM[1]] + Leg2Angles[LM[2]];

		if (AngularCost < BestCost)
		{
			BestCost = AngularCost;
			BestLegMapping = LM;
		}
	}

	check(BestLegMapping != NULL);
	
	// we did the angular calcs for candidate mapping from legindex->legid, but we
	// want to store legid->legidx, so we invert here
	LegMapping[BestLegMapping[0]] = 0;
	LegMapping[BestLegMapping[1]] = 1;
	LegMapping[BestLegMapping[2]] = 2;
}


// will be a parallel to xy plane
void AUTWalkerBody::GetLegsFacingRot(FRotator &OutRot) const
{
	FVector Vel2D = WalkerVehicle->Velocity;
	Vel2D.Z = 0.f;

	if (WalkerVehicle->IsMoving2D())
	{
		OutRot = Vel2D.Rotation();
	}
	else
	{
		OutRot = WalkerVehicle->Rotation;
		OutRot.Pitch = 0;
		OutRot.Roll = 0;
	}
}


UBOOL AUTWalkerBody::FindGroundForFoot(FVector const& TraceSeed, INT LegID)
{
	FCheckResult Hit(1.f);

	// start trace up somewhat to deal with crawling up steep hills
	FVector StartTrace = TraceSeed;
	StartTrace.Z += 700.f;

	// trace from TraceSeed upwards to StartTrace to make sure we're not going to
	// try and put the foot on a platform above our heads
	GWorld->SingleLineCheck(Hit, WalkerVehicle, StartTrace, TraceSeed, TRACE_AllBlocking | TRACE_SingleResult);
	if (Hit.Time < 1.f)
	{
		StartTrace = Hit.Location;
	}

	// end trace straight down a bunch
	FVector EndTrace = StartTrace;
	EndTrace.Z -= 2000.f;

	// do the main trace
	GWorld->SingleLineCheck(Hit, WalkerVehicle, EndTrace, StartTrace, TRACE_AllBlocking | TRACE_SingleResult);

	if (Hit.Time < 1.f)
	{
		if ( (Hit.Location - WalkerVehicle->Location).SizeSquared() > Square(MaxLegReach) )
		{
			// will I be able to reach momentarily?  Look forward a half second.
			const FVector& AnticipatedWalkerLocation = WalkerVehicle->Location + WalkerVehicle->Velocity * 0.5f;
			if ( (Hit.Location - AnticipatedWalkerLocation).SizeSquared() > Square(MaxLegReach) )
			{
				// farther away than leg will reach, fail
				return FALSE;
			}
		}
	
		StepAnimData[LegID].DesiredFootPosition = Hit.Location - (Hit.Normal * FootEmbedDistance);
		StepAnimData[LegID].DesiredFootPosNormal = Hit.Normal;
		StepAnimData[LegID].DesiredFootPosPhysMaterial = Hit.PhysMaterial;
		StepAnimData[LegID].bNoValidFootHold = FALSE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void AUTWalkerBody::CalcDesiredFootLocations()
{
	// desired positions form an equilateral triangle around the base of the walker, with 
	// one vertex directly behind

	const FLOAT LegLength = MaxLegReach * LegSpreadFactor;

	// get XY velocity info
	FVector Vel2D = WalkerVehicle->Velocity;
	Vel2D.Z = 0.f;
	const FLOAT Vel2DMag = Vel2D.Size();
	//FVector Vel2DNorm = Vel2D / Vel2DMag;

	// find distance to ground beneath walker body
	FLOAT HoverDist = WalkerVehicle->WheelSuspensionTravel[WalkerVehicle->CurrentStance] + WalkerVehicle->HoverAdjust[WalkerVehicle->CurrentStance];
	if (WalkerVehicle->Wheels.Num() > 0)
	{
		HoverDist += WalkerVehicle->Wheels(0)->WheelRadius;
	}
	const FLOAT FootRadius = (LegLength > HoverDist) ? appSqrt(Square(LegLength) - Square(HoverDist)) : 0.f;

	FRotator LegsRot;
	GetLegsFacingRot(LegsRot);

	for (INT LegID=0; LegID<WalkerLeg_MAX; ++LegID)
	{
		// adjust positions for velocity, to offset motion during steps and so feet appear to reach
		// forward to pull the walker
		const FVector& VelocityAdjustment = Vel2D * FootPosVelAdjScale[LegID];
		const FVector& OffsetLocal = BaseLegDirLocal[LegID] * FootRadius;
		FVector TraceSeed = WalkerVehicle->Location + TransformLocalToWorld(OffsetLocal, LegsRot) + VelocityAdjustment;

	    // we check to see if the previous location of the walker and its legs is the same as it is now.
	    // if it is then we don't need to do th expensive line traces
		const FVector& LegPositionDifference = ( PreviousTraceSeedLocation[LegID] - TraceSeed );
		
		if( LegPositionDifference.Size() > 1.0f )
		{
			PreviousTraceSeedLocation[LegID] = TraceSeed;

			if (!FindGroundForFoot(TraceSeed, LegID))
			{
				// back it up halfway and try again
				TraceSeed += (WalkerVehicle->Location - TraceSeed) * 0.5f;
				if (!FindGroundForFoot(TraceSeed, LegID))
				{
					// back up almost all the way and try one last time
					TraceSeed += (WalkerVehicle->Location - TraceSeed) * 0.9f;
					if (!FindGroundForFoot(TraceSeed, LegID))
					{
						// leg cannot find ground.  dangle.
						StepAnimData[LegID].bNoValidFootHold = TRUE;
					}
				}
			}
		}
	}
}

/** Returns true if walker is stationary but out of balance (as in, it needs to reposition its feet to appear balanced) */
UBOOL AUTWalkerBody::IsUnbalanced() const
{
	UBOOL bStepping = FALSE;

	// are any of the legs stretched to the wig-out limit?
	for (INT i=0; i<3; ++i)	
	{
		if ( ((CurrentFootPosition[i] - WalkerVehicle->Location).Size()) > MaxLegReach )
		{
			// leg stretched too far
			return TRUE;
		}

		if (StepStage[i] >= 0.f)
		{
			bStepping = TRUE;
		}
	}

	// are the legs roughly evenly spaced around the body?
	// only test this if all legs are on the ground
	if (!bStepping)
	{
		FVector VSum(0.f);
		for (INT i=0; i<3; ++i)	
		{
			VSum += (WalkerVehicle->Location - StepAnimData[i].DesiredFootPosition).SafeNormal2D();
		}
		if (VSum.SizeSquared() > 1.f)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void AUTWalkerBody_Scavenger::AnimateLegs( FLOAT DeltaSeconds, UBOOL bIsFalling )
{
	if ( bIsInBallMode )
	{
		SkeletalMeshComponent->PhysicsWeight = Max(SkeletalMeshComponent->PhysicsWeight-(DeltaSeconds*0.66f),0.0f);
		if(Physics != PHYS_None)
		{
			setPhysics(PHYS_None);	
		}
		
		if ( !bStartedBallMode && SkeletalMeshComponent->PhysicsWeight == 0.0f )
		{
			for ( INT LegIdx=0; LegIdx<3; LegIdx++ )
			{
				FootConstraints[LegIdx]->ReleaseComponent();
				ShoulderSkelControl[LegIdx]->SetSkelControlActive(FALSE);
			}
			eventCloak(TRUE);
			bStartedBallMode = TRUE;
		}

		FRotator NewRot = Rotation;
		const AUTVehicle_Scavenger* Scav = Cast<AUTVehicle_Scavenger>(WalkerVehicle);
		if( Scav && Scav->bSpinAttackActive )
		{
			
			NewRot.Yaw += appFloor(DeltaSeconds*spinRate);
			spinRate = Min(spinRate + appFloor(200000.f*DeltaSeconds), 131072); // up the spin rate
			if(PawnGrabber[0]->GrabbedComponent)
			{
				FVector GrabberLoc;
				FRotator GrabRot;
				SkeletalMeshComponent->GetSocketWorldLocationAndRotation(TEXT("LegOneRag"),GrabberLoc,&GrabRot);
				PawnGrabber[0]->SetLocation(GrabberLoc);
			}
		}
		else
		{
			spinRate = 32768;
		}
		if(Physics == PHYS_None)
		{
			FVector NewLoc;
			FRotator TossRot;
			WalkerVehicle->Mesh->GetSocketWorldLocationAndRotation(SphereCenterName,NewLoc,&TossRot);
			SetLocation(NewLoc);
		}
		SetRotation(NewRot);

		return;
	}
	else if(Physics == PHYS_None)
	{
		FVector NewLoc;
		FRotator TossRot;
		WalkerVehicle->Mesh->GetSocketWorldLocationAndRotation(SphereCenterName,NewLoc,&TossRot);
		SetLocation(NewLoc);
		setPhysics(PHYS_RigidBody);
	}
	SkeletalMeshComponent->PhysicsWeight = Min(SkeletalMeshComponent->PhysicsWeight+(DeltaSeconds*0.66f),1.0f);
	//debugf(TEXT("Weight: %f -- Strength %f"),SkeletalMeshComponent->PhysicsWeight,ShoulderSkelControl[0]->StrengthTarget);
	if(SkeletalMeshComponent->PhysicsWeight == 1.0f && ShoulderSkelControl[0]->StrengthTarget != 1.f)
	{
		UAnimNodeBlendList* RetractionBlender = Cast<UAnimNodeBlendList>((SkeletalMeshComponent->FindAnimNode(RetractionBlend)));
		if(RetractionBlender)
		{
			InitFeet();
			for ( INT LegIdx=0; LegIdx<3; LegIdx++ )
			{
				ShoulderSkelControl[LegIdx]->SetSkelControlActive(TRUE);
				FootConstraints[LegIdx]->BeginDeferredReattach();
				//FootConstraints[LegIdx]->bInterpolating = TRUE;
			}
			RetractionBlender->SetActiveChild(0, 0.5f);
		}
	}
	Super::AnimateLegs(DeltaSeconds, bIsFalling);
}


void AUTWalkerBody::AnimateLegs( FLOAT DeltaSeconds, UBOOL bIsFalling )
{
	if ( bIsDead || !WalkerVehicle || !SkeletalMeshComponent )
	{
		// no leg animation when walker is dead
		return;
	}

	// cache current foot locations
	for ( INT i=0; i<3; i++ )
	{
		CurrentFootPosition[i] = SkeletalMeshComponent->GetBoneLocation(FootBoneName[i]);
	}

	// check for water effects if on client and rendered recently
	if (WorldInfo->NetMode != NM_DedicatedServer && WorldInfo->TimeSeconds - LastRenderTime < 1.0f)
	{
		FMemMark Mark(GMem);
		for (INT i = 0; i < 3; i++)
		{
			UBOOL bNowInWater = FALSE;
			for (const FCheckResult* Link = GWorld->Hash->ActorPointCheck(GMem, CurrentFootPosition[i], FVector(0.f,0.f,0.f), TRACE_PhysicsVolumes); Link != NULL; Link = Link->GetNext())
			{
				const APhysicsVolume* const Volume = Cast<APhysicsVolume>(Link->Actor);
				if (Volume != NULL && Volume->bWaterVolume)
				{
					bNowInWater = TRUE;
					break;
				}
			}

			if (bNowInWater != FootInWater[i])
			{
				eventSpawnFootWaterEffect(i);
				FootInWater[i] = bNowInWater;
			}
		}
		Mark.Pop();
	}

	const UBOOL bMoving = (WalkerVehicle->bDriving && WalkerVehicle->IsMoving2D());

	// do we need to reposition the feet because we changed stances?
	UBOOL bRepositionFeetBecauseStanceChanged = FALSE;
	if (WalkerVehicle->CurrentStance != WalkerVehicle->PreviousStance)
	{
		// don't reposition for transitions to crouch
		if (WalkerVehicle->CurrentStance != WalkerStance_Crouched)
		{
			bRepositionFeetBecauseStanceChanged = TRUE;
		}
	}

	if ( bMoving || bRepositionFeetBecauseStanceChanged || (!WalkerVehicle->bDriving && !WalkerVehicle->IsSleeping()) || IsUnbalanced() )
	{
		// if not moving, we'll just skip this and hold where we are
		AssignFootDirections();
		CalcDesiredFootLocations();
	}

	// debug code for measuring leg reach
	//for (INT i=0; i<3; ++i)
	//{
	//	FLOAT V = (WalkerVehicle->Location - DesiredFootPositionNew[i]).Size();
	//	debugf(TEXT("Leg %d: %f"), i, V);
	//}

	// debug to show desired foot locations
	static UBOOL bShowDesiredFootLocations = FALSE;
	if (bShowDesiredFootLocations)
	{
		DrawDebugSphere(StepAnimData[WalkerLeg_Rear].DesiredFootPosition, 40.f, 10, 255, 0, 0, FALSE);		// rear
		DrawDebugSphere(StepAnimData[WalkerLeg_FrontLeft].DesiredFootPosition, 40.f, 10, 0, 255, 0, FALSE);		// front left
		DrawDebugSphere(StepAnimData[WalkerLeg_FrontRight].DesiredFootPosition, 40.f, 10, 0, 0, 255, FALSE);		// front right
		//	DrawDebugCoordinateSystem(Location, Rotation, 256, FALSE);					// pos/rot of legs actor
		//	DrawDebugCoordinateSystem(WalkerVehicle->Location, WalkerVehicle->Rotation, 256, FALSE);					// pos/rot of legs actor
	}

	// crouching if parked or user presses crouch
	UBOOL const bCrouchedMode = bHasCrouchMode && (!WalkerVehicle->bDriving || WalkerVehicle->bHoldingDuck);

	// tick any active step sequences
	ProcessSteps(DeltaSeconds);

	// see if we have any legs that are overextended or otherwise need to be immediately dealt with
	for ( INT LegID=0; LegID<3; LegID++ )
	{
		INT const LegIdx = LegMapping[LegID];
		if ( (StepStage[LegIdx] < 0) && (IgnoreFoot[LegIdx] == 0) )
		{
			const FLOAT FootDistFromBodySq = (WalkerVehicle->Location - CurrentFootPosition[LegIdx]).SizeSquared();
			if ( FootDistFromBodySq > Square(MaxLegReach) )
			{
				if (StepAnimData[LegID].bNoValidFootHold)
				{
					// stretched too far and no place to go, just release foot and dangle
					FootConstraints[LegIdx]->ReleaseComponent();
				}			
				else
				{
					// do an emergency step to get back into a good place
					const FLOAT DeltaZ = CurrentFootPosition[LegIdx].Z - StepAnimData[LegID].DesiredFootPosition.Z;
					if (DeltaZ < MaxLegReach*0.5f)		// half of leg reach seems like a decent threshold
					{
						// foot below or roughly even with desired loc, do a normal step
						PerformStep((EWalkerLegID)LegID);
					}
					else
					{
						// foot is above desired loc, just bring it down
						BeginStepStage((EWalkerLegID)LegID, 1);
					}
				}
			}
		}
	}

	// only allow one foot in the air at a time for normal stepping
	INT StepCount = 0;
	for (INT i=0; i<3; ++i)
	{
		if (StepStage[i] >= 0)
		{
			++StepCount;
		}
	}

	if (StepCount == 0)
	{
		FLOAT BestDistSq = 0.f;
		INT BestLegID = -1;

		// see if any foot is too far, if so, move furthest
		// to be clear, we're iterating over the legs in EWalkerLegID space
		for ( INT LegID=0; LegID<3; LegID++ )
		{
			const INT LegIdx = LegMapping[LegID];

			if (IgnoreFoot[LegIdx] == 0)
			{
				const FLOAT FootDistFromIdealSq = (StepAnimData[LegID].DesiredFootPosition - CurrentFootPosition[LegIdx]).SizeSquared2D();

				if ( (FootDistFromIdealSq > Square(MinStepDist)) /*&& (NextStepTime[LegIdx] < WorldInfo->TimeSeconds)*/ )
				{
					if ( FootDistFromIdealSq > BestDistSq )
					{
						BestDistSq = FootDistFromIdealSq;
						BestLegID = LegID;
					}
				}
			}
		}

		if ( BestLegID >= 0 )
		{
			//debugf(TEXT("Bestfoot %d at %f"),BestFoot,WorldInfo->TimeSeconds);
			PerformStep((EWalkerLegID)BestLegID);
		}
	}

	// point shoulders toward goal foot positions
	for (INT LegIdx=0; LegIdx<3; ++LegIdx)
	{
		const FVector& ShoulderBoneLoc = SkeletalMeshComponent->GetBoneLocation(ShoulderBoneName[LegIdx]);
		const FVector& ToLookatNorm = (FootConstraints[LegIdx]->Location - ShoulderBoneLoc).SafeNormal();
		ShoulderSkelControl[LegIdx]->DesiredTargetLocation = ShoulderBoneLoc + ToLookatNorm * 256.f;

		// for debugging
		//DrawDebugBox(ShoulderSkelControl[LegIdx]->TargetLocation, FVector(16.f,16.f,16.f), 255, 255, 255);
	}
}

/** experimental walker leg code */
void AUTWalkerBody::DoTestStep(INT LegIdx, FLOAT Mag)
{
	if ( (LegIdx >= 0) && (LegIdx < 3)/* && (Mag != 0.f) */)
	{
		PerformStep((EWalkerLegID)LegIdx);
	}
}

/** 
 * Moves specified foot to the specified location.
 * 
 * @param LegIdx Leg index (not LedID) of the foot you want to move.
 * @param Loc Destination position
 * @param SurfaceNormal Normal of the surface at the desired.
 * @param MoveTime How long the interpolation should take.
 */
void AUTWalkerBody::MoveFootToLoc(INT LegIdx, FVector const& Loc, FVector const& SurfaceNormal, FLOAT MoveTime)
{
	// make sure foot component is grabbed
	if (FootConstraints[LegIdx]->GrabbedComponent == NULL)
	{
		const FVector& FootLocation = SkeletalMeshComponent->GetBoneLocation(FootBoneName[LegIdx]);
		FootConstraints[LegIdx]->GrabComponent(SkeletalMeshComponent, FootBoneName[LegIdx], FootLocation, TRUE);
		FootConstraints[LegIdx]->SetLocation(FootLocation);
	}

	if (MoveTime > 0.f)
	{
		FootConstraints[LegIdx]->SetSmoothLocation(Loc, MoveTime);
	}
	else
	{
		FootConstraints[LegIdx]->SetLocation(Loc);
	}

	// set orientation part of the constraint as well
	const FVector& InvNormal = -SurfaceNormal;
	FootConstraints[LegIdx]->SetOrientation( InvNormal.Rotation().Quaternion() );
}

/** Stop performing a step on the specified leg. */
void AUTWalkerBody::FinishStep(INT LegIdx)
{
	StepStage[LegIdx] = -1;
	eventPlayFootStep(LegIdx);
}

/** experimental walker leg code */
void AUTWalkerBody::ProcessSteps(FLOAT DeltaTime)
{
	for (INT LegID=0; LegID<3; ++LegID)
	{
		INT const LegIdx = LegMapping[LegID];
		
		if (StepStage[LegIdx] >= 0)
		{
			if ( (WorldInfo->TimeSeconds > NextStepStageTime[LegIdx]) && (NextStepStageTime[LegIdx] > 0.f) )
			{
				// advance stage
				BeginStepStage((EWalkerLegID)LegID, StepStage[LegIdx]+1);
			}
			else
			{
				// process step stages
				switch (StepStage[LegIdx])
				{
				case 0:
				case 1:
					{
						FVector EndInterp = StepAnimData[LegID].DesiredFootPosition;
						EndInterp.Z += FootStepEndLift;
						FootConstraints[LegIdx]->UpdateSmoothLocationWithGoalInterp(EndInterp);
					}
					break;
				case 2:
					{
						const URB_Handle* const FootHandle = FootConstraints[LegIdx];

						FVector GoalPos;
						{
							if (FootHandle->GrabbedComponent == NULL)
							{
								// Foot isn't being moved anywhere, so we're happy where we are
								GoalPos = CurrentFootPosition[LegIdx];
							}
							else if (FootHandle->bInterpolating)
							{
								// handle is moving, goal is where it will end up
								GoalPos = FootHandle->Destination;
							}
							else
							{
								// handle is stationary, goal is where it is
								GoalPos = FootHandle->Location;
							}
						}

						const FLOAT DistSqFromGoal = (CurrentFootPosition[LegIdx] - GoalPos).SizeSquared();

						if (DistSqFromGoal < LandedFootDistSq)
						{
							FinishStep(LegIdx);
						}
						else
						{
							FootConstraints[LegIdx]->UpdateSmoothLocation(StepAnimData[LegID].DesiredFootPosition);
						}
					}
					break;
				}
			}
		}
	}
}


/** Cause specified leg to begin doing a step to its desired location. */
void AUTWalkerBody::PerformStep(EWalkerLegID LegID)
{
	BeginStepStage(LegID, 0);
}


/** Begin executing the specified state of a step sequence. */
void AUTWalkerBody::BeginStepStage(EWalkerLegID LegID, INT StageIdx)
{
	INT const LegIdx = LegMapping[LegID];

	// store it
	StepStage[LegIdx] = StageIdx;

	switch (StageIdx)
	{
	case 0:
		// starting step.  animation does much of the work, but the physics is not strong enough
		// to lift the leg as much as we'd like.  So we augment that moving an rb_handle attached to the foot.
		// In order to create a nice smooth curvy step path for the foot, the handle interpolates towards a moving
		// target.  This target begins FootStepStartLift units over the starting foot location, and ends 
		// FootStepEndLift units above the desired foot location.
		{
			FVector StartInterp = CurrentFootPosition[LegIdx];
			StartInterp.Z += FootStepStartLift;

			FVector EndInterp = StepAnimData[LegID].DesiredFootPosition;
			EndInterp.Z += FootStepEndLift;

			FootConstraints[LegIdx]->SetSmoothLocationWithGoalInterp(StartInterp, EndInterp, StepStageTimes(0));

			// play step anim.
			if (FootStepAnimNode[LegIdx])
			{
				FootStepAnimNode[LegIdx]->PlayAnim();
			}

			// set timer for next stage
			NextStepStageTime[LegIdx] = WorldInfo->TimeSeconds + StepStageTimes(0);
		}
		break;
	case 1:
		// Foot is hovering over our goal location.  Stick it to ground in the goal position by quickly
		// moving the foot handle.
		{
			FootConstraints[LegIdx]->StopGoalInterp();
			MoveFootToLoc(LegIdx, StepAnimData[LegID].DesiredFootPosition, StepAnimData[LegID].DesiredFootPosNormal, StepStageTimes(1));
			NextStepStageTime[LegIdx] = WorldInfo->TimeSeconds + StepStageTimes(1);
		}
		break;
	case 2:
		// All done, just spin here, code elsewhere will detect end of step.
		{
			NextStepStageTime[LegIdx] = WorldInfo->TimeSeconds + StepStageTimes(2);
		}
		break;
	case 3:
		// This is the safety net -- if leg hasn't finished the step by now, force it to finish.
		// if this gets hit, foot will likely try to step again since it never made it to its goal
		FinishStep(LegIdx);
		NextStepStageTime[LegIdx] = 0.f;		// stop advancing
		break;
	}
}

void UUTWalkerStepHandle::Tick(FLOAT DeltaTime)
{
	// interpolate the handle's goal location and update it, if necessary
	if (InterpFactor < 1.f)
	{
		bInterpolating = TRUE;			// make sure the RB_Handle is still interpolating, even if it gets to where it's going
		InterpFactor += DeltaTime / InterpTime;
		InterpFactor = Min(InterpFactor, 1.f);
		FVector const CurrentHandleGoalLoc = GoalInterpStartLoc + (GoalInterpDelta * InterpFactor);
		UpdateSmoothLocation(CurrentHandleGoalLoc);
		//Owner->DrawDebugBox(CurrentHandleGoalLoc, FVector(16.f,16.f,16.f), 255, 255, 0);
	}

	Super::Tick(DeltaTime);
}

void UUTWalkerStepHandle::SetSmoothLocationWithGoalInterp(const FVector& StartLoc,const FVector& EndLoc,FLOAT MoveTime)
{
	GoalInterpDelta = EndLoc - StartLoc;
	GoalInterpStartLoc = StartLoc;
	InterpTime = MoveTime;
	InterpFactor = 0.f;

	SetSmoothLocation(StartLoc, MoveTime);
}

void UUTWalkerStepHandle::UpdateSmoothLocationWithGoalInterp(const FVector& NewEndLoc)
{
	GoalInterpDelta = NewEndLoc - GoalInterpStartLoc;
}

void UUTWalkerStepHandle::StopGoalInterp()
{
	InterpFactor = 1.f;
}


