//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "UnPath.h"
#include "EngineInterpolationClasses.h"

IMPLEMENT_CLASS(AUTPlayerController);
IMPLEMENT_CLASS(AUTConsolePlayerController);
IMPLEMENT_CLASS(UUTPlayerInput);
IMPLEMENT_CLASS(UUTConsolePlayerInput);

/** 
 * HearSound()
 * If sound is audible, calls eventClientHearSound() so local or remote player will hear it.
 * UTPlayerController implementation determines audibility based on the closest to the sound location of 
 * the viewtarget or the controller's pawn if they are not the same
 */
void AUTPlayerController::HearSound( USoundCue* InSoundCue, AActor* SoundPlayer, const FVector& SoundLocation, UBOOL bStopWhenOwnerDestroyed )
{
	if( SoundPlayer == this )
	{
		// always play sounds by the playercontroller
		eventClientHearSound( InSoundCue, SoundPlayer, SoundLocation, bStopWhenOwnerDestroyed, FALSE );
		return;
	}

	// Get simple listener/speaker pair
	FVector ModifiedSoundLocation = SoundLocation;
	FVector HearLocation = Location;

	// Redirect through the view target if necessary
	if( ViewTarget )
	{
		HearLocation = ViewTarget->Location;

		if( Pawn && ( Pawn != ViewTarget ) && ( ( HearLocation - SoundLocation ).SizeSquared() > ( Pawn->Location - SoundLocation ).SizeSquared() ) )
		{
			// move sound location so that it's heard as if Pawn heard it.
			ModifiedSoundLocation = ModifiedSoundLocation + HearLocation - Pawn->Location;
		}
	}

	INT bIsOccluded = 0;

	if( !bAcuteHearing )
	{
		if( InSoundCue->IsAudible( ModifiedSoundLocation, HearLocation, SoundPlayer, bIsOccluded, bCheckSoundOcclusion ) )
		{
			eventClientHearSound( InSoundCue, SoundPlayer, ModifiedSoundLocation, bStopWhenOwnerDestroyed, bIsOccluded );
		}
	}
	else
	{
		// play sound at closer location to increase volume
		const FVector SoundDir = ModifiedSoundLocation - HearLocation;
		const FLOAT SoundDist = SoundDir.Size();
		ModifiedSoundLocation = HearLocation + ::Min( 0.25f * SoundDist, 2000.f ) * SoundDir;
		if( InSoundCue->IsAudible( ModifiedSoundLocation, HearLocation, SoundPlayer, bIsOccluded, bCheckSoundOcclusion ) )
		{
			eventClientHearSound( InSoundCue, NULL, ModifiedSoundLocation, bStopWhenOwnerDestroyed, FALSE );
		}
	}
}

void AUTPlayerController::CheckShake(FLOAT& MaxOffset, FLOAT& Offset, FLOAT& Rate, FLOAT Time)
{
	if (Abs(Offset) >= Abs(MaxOffset))
	{
		Offset = MaxOffset;
		if ( Time > 1.f )
		{
			if (Time * Abs(MaxOffset / Rate) <= 1.0)
			{
				MaxOffset = MaxOffset * (1 / Time - 1);
				Rate = Rate / (1 / Time - 1);
			}
			else
			{
				MaxOffset *= -1;
				Rate *= -1;
			}
		}
		else
		{
			MaxOffset = 0.f;
			Offset = 0.f;
			Rate = 0.f;
		}
	}
}

void AUTPlayerController::UpdateShakeRotComponent(FLOAT& Max, INT& Current, FLOAT& Rate, FLOAT Time, FLOAT DeltaTime)
{
    Current = appRound((Current & 65535) + Rate * DeltaTime) & 65535;
    if ( Current > 32768 )
	{
		Current -= 65536;
	}

    FLOAT fCurrent = Current;
    CheckShake(Max, fCurrent, Rate, Time);
    Current = (INT)fCurrent;
}
 
UBOOL AUTPlayerController::Tick(FLOAT DeltaSeconds, ELevelTick TickType)
{
	if ( Super::Tick(DeltaSeconds,TickType) )
	{
		if ( bPulseTeamColor )
		{
			PulseTimer -= DeltaSeconds;
			if (PulseTimer < 0.f)
			{
				bPulseTeamColor = false;
			}
		}

		if( bUsePhysicsRotation )
		{
			physicsRotation(DeltaSeconds);
		}
		if ( bBeaconPulseDir )
		{
			BeaconPulseScale += BeaconPulseRate * DeltaSeconds;
			if ( BeaconPulseScale > BeaconPulseMax )
			{
				BeaconPulseScale = BeaconPulseMax;
				bBeaconPulseDir = FALSE;
			}
		}
		else
		{
			BeaconPulseScale -= BeaconPulseRate * DeltaSeconds;
			if ( BeaconPulseScale < 1.f )
			{
				BeaconPulseScale = 1.f;
				bBeaconPulseDir = TRUE;
			}
		}

		return 1;
	}
	return 0;
}

UBOOL AUTPlayerController::MoveWithInterpMoveTrack(UInterpTrackMove* MoveTrack, UInterpTrackInstMove* MoveInst, FLOAT CurTime, FLOAT DeltaTime)
{
	if (CameraAnimPlayer != NULL && CameraAnimPlayer->MoveTrack == MoveTrack)
	{
		// figure out the movement delta and put it in ShakeOffset and ShakeRot
		MoveTrack->GetKeyTransformAtTime(MoveInst, CurTime, ShakeOffset, ShakeRot);
		ShakeOffset *= CameraAnimPlayer->CurrentBlendWeight;
		ShakeRot *= CameraAnimPlayer->CurrentBlendWeight;
		return TRUE;
	}
	else
	{
		// use default behavior
		return Super::MoveWithInterpMoveTrack(MoveTrack, MoveInst, CurTime, DeltaTime);
	}
}

void AUTPlayerController::ModifyPostProcessSettings(FPostProcessSettings& PPSettings) const
{
	Super::ModifyPostProcessSettings(PPSettings);

	if (CameraAnimPlayer != NULL && !CameraAnimPlayer->bFinished)
	{
		ACameraActor const* const DefaultCamActor = ACameraActor::StaticClass()->GetDefaultObject<ACameraActor>();
		if (DefaultCamActor) 
		{
			FLOAT const BlendWeight = CameraAnimPlayer->CurrentBlendWeight;
			FPostProcessSettings const& DefaultSettings = DefaultCamActor->CamOverridePostProcess;

			PPSettings.Bloom_Scale += BlendWeight * (CamOverridePostProcess.Bloom_Scale - DefaultSettings.Bloom_Scale);
			PPSettings.DOF_FalloffExponent += BlendWeight * (CamOverridePostProcess.DOF_FalloffExponent - DefaultSettings.DOF_FalloffExponent);
			PPSettings.DOF_BlurKernelSize += BlendWeight * (CamOverridePostProcess.DOF_BlurKernelSize - DefaultSettings.DOF_BlurKernelSize);
			PPSettings.DOF_MaxNearBlurAmount += BlendWeight * (CamOverridePostProcess.DOF_MaxNearBlurAmount - DefaultSettings.DOF_MaxNearBlurAmount);
			PPSettings.DOF_MaxFarBlurAmount += BlendWeight * (CamOverridePostProcess.DOF_MaxFarBlurAmount - DefaultSettings.DOF_MaxFarBlurAmount);
			PPSettings.DOF_FocusInnerRadius += BlendWeight * (CamOverridePostProcess.DOF_FocusInnerRadius - DefaultSettings.DOF_FocusInnerRadius);
			PPSettings.DOF_FocusDistance += BlendWeight * (CamOverridePostProcess.DOF_FocusDistance - DefaultSettings.DOF_FocusDistance);
			PPSettings.MotionBlur_MaxVelocity += BlendWeight * (CamOverridePostProcess.MotionBlur_MaxVelocity - DefaultSettings.MotionBlur_MaxVelocity);
			PPSettings.MotionBlur_Amount += BlendWeight * (CamOverridePostProcess.MotionBlur_Amount - DefaultSettings.MotionBlur_Amount);
			PPSettings.MotionBlur_CameraRotationThreshold += BlendWeight * (CamOverridePostProcess.MotionBlur_CameraRotationThreshold - DefaultSettings.MotionBlur_CameraRotationThreshold);
			PPSettings.MotionBlur_CameraTranslationThreshold += BlendWeight * (CamOverridePostProcess.MotionBlur_CameraTranslationThreshold - DefaultSettings.MotionBlur_CameraTranslationThreshold);
			PPSettings.Scene_Desaturation += BlendWeight * (CamOverridePostProcess.Scene_Desaturation - DefaultSettings.Scene_Desaturation);
			PPSettings.Scene_HighLights += BlendWeight * (CamOverridePostProcess.Scene_HighLights - DefaultSettings.Scene_HighLights);
			PPSettings.Scene_MidTones += BlendWeight * (CamOverridePostProcess.Scene_MidTones - DefaultSettings.Scene_MidTones);
			PPSettings.Scene_Shadows += BlendWeight * (CamOverridePostProcess.Scene_Shadows - DefaultSettings.Scene_Shadows);
		}
	}
}

/** Get the preferred custom character setup for this player (from player profile store). */
FCustomCharData AUTPlayerController::GetPlayerCustomCharData(const FString& CharString)
{
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());
	FCustomCharData FinalData;
	appMemzero(&FinalData, sizeof(FCustomCharData));

	UBOOL bGenerateRandomData = FALSE;
	ULocalPlayer *LP = Cast<ULocalPlayer>(Player);

	if(CharString.Len())
	{
		FinalData = Data->CharDataFromString(CharString);
		bGenerateRandomData = FALSE;

		debugf(TEXT("AUTPlayerController::GetPlayerCustomCharData() - Loaded custom character data from profile (ControllerId: %d)."), LP ? LP->ControllerId : -1);
	}

	if(bGenerateRandomData)
	{
		debugf(TEXT("AUTPlayerController::GetPlayerCustomCharData() - Unable to load custom character data from profile.  Randomly generating a character."));
		FinalData = Data->MakeRandomCharData();
	}

	return FinalData;
}

/** @return The index of the PC in the game player's array. */
INT AUTPlayerController::GetUIPlayerIndex()
{
	ULocalPlayer* LP = NULL;
	INT Result = INDEX_NONE;

	LP = Cast<ULocalPlayer>(Player);

	if(LP)
	{	
		Result = UUIInteraction::GetPlayerIndex(LP);
	}

	return Result;
}

/** Sets the display gamma. */
void AUTPlayerController::SetGamma(FLOAT GammaValue)
{
#if !PS3
	extern void SetDisplayGamma(FLOAT Gamma);
	SetDisplayGamma(GammaValue);
#endif
}


/** Determines whether this Pawn can be used for Adhesion or Friction **/
UBOOL AUTConsolePlayerController::IsValidTargetAdhesionFrictionTarget( const APawn* P, FLOAT MaxDistance ) const
{
	UBOOL Retval = FALSE;

	// check for self before we do any computation
	if( ( P != NULL ) && ( P != Pawn ) ) 
	{
		const FVector ToTargetNorm = P->Location - Pawn->Location;
		const FLOAT DistToTarget = ToTargetNorm.Size();

		//warnf( TEXT( "IsValidTargetAdhesionFrictionTarget: %s %i" ), *GP->GetName(), GP->PlayerReplicationInfo->eventGetTeamNum() );

		Retval = ( 
			 ( P->IsValidEnemyTargetFor( PlayerReplicationInfo, FALSE ) == TRUE )
			&& ( P->bCanBeAdheredTo == TRUE )
			&& ( P->bCanBeFrictionedTo == TRUE )
			&& ( DistToTarget < MaxDistance )
			&& ( P->Health > 0 ) 
			&& ( P->bHidden == FALSE )
			&& ( P->bDeleteMe == FALSE )
			&& ( const_cast<AUTConsolePlayerController*>(this)->LineOfSightTo( P ) == TRUE )
			);
	}

	return Retval;
}

/** 
 * This will look to see if the player is aiming directly at a projectile which does not want to
 * be adhered to. 
 **/
UBOOL IsDirectlyAimingAtAProjectile( const AUTConsolePlayerController* C, const FRotator& CamRot )
{
	UBOOL RetVal = FALSE;

	AActor*	ViewTarg = const_cast<AUTConsolePlayerController*>(C)->GetViewTarget();

	if( C->Pawn != NULL )
	{
		const FVector StartPoint = ViewTarg->Location + FVector(0,0,C->Pawn->BaseEyeHeight);
		const FVector EndPoint = StartPoint + CamRot.Vector() * 3000.0f; // @todo get this from the weapon

		FCheckResult HitProjectile;
		GWorld->SingleLineCheck( HitProjectile, const_cast<AUTConsolePlayerController*>(C), EndPoint, StartPoint, TRACE_Actors );

		//const_cast<AUTConsolePlayerController*>(C)->FlushPersistentDebugLines();
		//const_cast<AUTConsolePlayerController*>(C)->DrawDebugLine( StartPoint, EndPoint, 0, 255, 0, 1 ); 

		if( (HitProjectile.Actor != NULL )
			&& ( HitProjectile.Actor->GetAProjectile() != NULL )
			&& ( (HitProjectile.Actor->bCanBeAdheredTo == FALSE ) || (HitProjectile.Actor->bCanBeFrictionedTo == FALSE ) )

			)
		{
			//warnf( TEXT("aiming at a non adhere-to and non friction-to Projectile!!!" ) );
			RetVal = TRUE;
		}

	}

	return RetVal;
}



/**
 * This will find the best AdhesionFriction target based on the params passed in.
 **/
APawn* AUTConsolePlayerController::GetTargetAdhesionFrictionTarget( FLOAT MaxDistance, const FVector& CamLoc, const FRotator& CamRot ) const
{
	APawn* BestTarget = NULL;
	
	// how to deal with other objects that are not GamePawns  (e.g. shock ball, magic objects)
	// a shock ball should "always?" be the best

	const UBOOL bDirectlyAimingAtAProjectile = IsDirectlyAimingAtAProjectile( this, CamRot );

	if( bDirectlyAimingAtAProjectile == TRUE )
	{
		return NULL;
	}

	
	const ULocalPlayer* LocalPlayer = ConstCast<ULocalPlayer>(Player); // do this cast only once
 
	if( LocalPlayer != NULL )
	{
		FLOAT BestFrictionTargetScore = 0.0f;

		for( APawn* TempP = GWorld->GetWorldInfo()->PawnList; TempP != NULL; TempP = TempP->NextPawn )
		{
			// check for visibility here as we need a LocalPlayer and don't want to just pass it in to then do the same check
			if( ( LocalPlayer->GetActorVisibility( TempP ) == TRUE )
				&& ( IsValidTargetAdhesionFrictionTarget( TempP, MaxDistance ) == TRUE )
				)
			{
				//warnf( TEXT("PossibleTarget: %s"), *GamePawn->GetName() );
				// score all potential targets and find best one.
				const FLOAT TmpScore = ScoreTargetAdhesionFrictionTarget( TempP, MaxDistance, CamLoc, CamRot );

				// track best visible target
				if( TmpScore > BestFrictionTargetScore )
				{
					BestFrictionTargetScore = TmpScore;
					BestTarget = TempP;
					//warnf( TEXT("BestAdhesionFrictionTarget: %s at %f"), *BestTarget->GetName(), BestFrictionTargetScore );
				}
			}
		}
	}

	return BestTarget;
}


/** 
 * This will score both Adhesion and Friction targets.  We want the same scoring function as we
 * don't want the two different systems fighting over targets that are close.
 **/
FLOAT AUTConsolePlayerController::ScoreTargetAdhesionFrictionTarget( const APawn* P, FLOAT MaxDistance, const FVector& CamLoc, const FRotator& CamRot ) const
{
	FLOAT Score = 0.0f;

	if( P != NULL && Pawn != NULL )
	{
		// Initial Score based on how much we're aiming at them
		FVector ToTargetNorm = P->Location - Pawn->Location;
		const FLOAT DistToTarget = ToTargetNorm.Size();
		ToTargetNorm /= DistToTarget;

		Score = ToTargetNorm | CamRot.Vector();

		// If they're in front and within adhesion/friction range
		if( (Score > 0.f) && (DistToTarget < MaxDistance) )
		{
			// Adjust Score based on distance, so that closer targets are slightly favored
			Score += (1.f - (DistToTarget/MaxDistance)) * Score * 0.65f;
		}
	}

	return Score;
}

/** Used in the Character Editor - outputs current character setup as concise text form. */
void AUTPlayerController::ClipChar()
{
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());

	for( TObjectIterator<AUTCustomChar_Preview> It; It; ++It )
	{
		AUTCustomChar_Preview* C = *It;
		if(!C->IsTemplate())
		{
			FString CharText = Data->CharDataToString(C->Character.CharData);
			appClipboardCopy(*CharText);

			FString OutputMessage = CharText + FString(TEXT(" -> CLIPBOARD"));
			debugf(*OutputMessage);
			ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
			{
				if(LP && LP->ViewportClient && LP->ViewportClient->ViewportConsole)
				{
					LP->ViewportClient->ViewportConsole->eventOutputText(OutputMessage);
				}
			}
		}
	}
}

/** Used in the Character Editor - outputs total poly count for character setup on screen. */
void AUTPlayerController::CharPolyCount()
{
	// Struct for holding vert count for an LOD
	struct FCustomCharLODInfo
	{
		INT Polys;
		INT RigidCount;
		INT SoftCount;

		FCustomCharLODInfo()
		{
			Polys = 0;
			RigidCount = 0;
			SoftCount = 0;
		}
	};

	// Counts for each LOD
	FCustomCharLODInfo LODCount[4];

	// First we need to find the preview character
	for( TObjectIterator<AUTCustomChar_Preview> It; It; ++It )
	{
		AUTCustomChar_Preview* C = *It;
		if(!C->IsTemplate())
		{
			// Iterate over all its visible skeletal mesh components
			for(INT CompIdx=0; CompIdx<C->Components.Num(); CompIdx++)
			{
				USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(C->Components(CompIdx));
				if(SkelMeshComp && !SkelMeshComp->HiddenGame)
				{
					// Grab its skel mesh
					USkeletalMesh* SkelMesh = SkelMeshComp->SkeletalMesh;
					if(SkelMesh)
					{
						// The go over each LOD
						INT CountLOD = ::Min(4, SkelMesh->LODModels.Num());
						for(INT LODIdx = 0; LODIdx<CountLOD; LODIdx++)
						{
							FStaticLODModel& LODModel = SkelMesh->LODModels(LODIdx);
							FCustomCharLODInfo& Count = LODCount[LODIdx];
							Count.Polys += LODModel.GetTotalFaces();
							// Need to go over each chunk of that LOD
							for(INT ChunkIndex = 0;ChunkIndex < LODModel.Chunks.Num();ChunkIndex++)
							{
								Count.RigidCount += LODModel.Chunks(ChunkIndex).GetNumRigidVertices();
								Count.SoftCount += LODModel.Chunks(ChunkIndex).GetNumSoftVertices();
							}
						}
					}
				}
			}
		}
	}

	FString OutputMessage(TEXT("CHAR: "));
	for(INT i=0; i<4; i++)
	{
		INT TotalCount = LODCount[i].SoftCount + LODCount[i].RigidCount;
		if(TotalCount > 0)
		{
			OutputMessage += FString::Printf(TEXT("LOD %d: %d P (%d/%d RV)  "), i, LODCount[i].Polys, LODCount[i].RigidCount, TotalCount);
		}
	}

	debugf(*OutputMessage);
	ULocalPlayer* LP = Cast<ULocalPlayer>(Player);
	{
		if(LP && LP->ViewportClient && LP->ViewportClient->ViewportConsole)
		{
			LP->ViewportClient->ViewportConsole->eventOutputText(OutputMessage);
		}
	}
}

