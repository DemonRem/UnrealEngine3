/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"

IMPLEMENT_CLASS(AUTVehicle_Fury);
IMPLEMENT_CLASS(UUTSkelControl_NecrisRaptorWings);
IMPLEMENT_CLASS(UUTAnimBlendByCollision);


void UUTAnimBlendByCollision::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{

	if ( !bForceBlend && !bPulseBlend )
	{
		if ( TraceSocket != NAME_None && SkelComponent != NULL && SkelComponent->GetOwner() )
		{
			FVector TraceStart, TraceEnd;
			FLOAT BlendStrengthPerc = 0.0;
			AUTVehicle* POwner = Cast<AUTVehicle>(SkelComponent->GetOwner()->GetAPawn());

			if ( GIsGame && POwner && TraceDistance > 0.0 )
			{
				SkelComponent->GetSocketWorldLocationAndRotation(TraceSocket, TraceStart, NULL);
				TraceEnd = TraceStart + ( POwner->Rotation.Vector() * TraceDistance);
				FCheckResult Hit(1.f);
				GWorld->SingleLineCheck(Hit, POwner, TraceEnd, TraceStart, TRACE_AllBlocking);
				FLOAT Dist = 0.0f;

				LastHitActor = Hit.Actor;

				if ( Hit.Actor )
				{


					Dist = (Hit.Location - TraceStart).Size() - TraceAdjustment;
					if (Dist < 0.0f)
					{
						Dist = 0.0f;
					}

					BlendStrengthPerc = 1.0 - Clamp<FLOAT>(  ( Dist / ( TraceDistance - TraceAdjustment )  ) , 0.0, 1.0);
				}
				SetBlendTarget(BlendStrengthPerc,0.15f);
			}
		}
	}
	else if ( bPulseBlend )
	{
		if ( Child2Weight >= 1.0 )
		{
			PulseDelay -= DeltaSeconds;
			if ( PulseDelay <= 0.0f )
			{
				PulseDelay = 0.0f;
				bPulseBlend = FALSE;
			}
		}
	}

	Super::TickAnim(DeltaSeconds, TotalWeight);
}

void AUTVehicle_Fury::Boost(struct FJetSFXInfo& SFXInfo,FLOAT RampUpTime,FLOAT RampDownTime)
{
	SFXInfo.ExhaustRampUpTime = RampUpTime;
	SFXInfo.ExhaustRampDownTime = RampDownTime;
	SFXInfo.Exhaust = 1.0;
}
void AUTVehicle_Fury::BoostTo(struct FJetSFXInfo& SFXInfo,FLOAT NewExhaust,FLOAT RampTime)
{

	if (NewExhaust != SFXInfo.Exhaust)
	{

		if ( SFXInfo.ExhaustCur > NewExhaust ) 
		{
			SFXInfo.ExhaustRampDownTime = RampTime;
		}
		else if ( SFXInfo.ExhaustCur < NewExhaust )
		{
			SFXInfo.ExhaustRampUpTime = RampTime;
		}

		SFXInfo.ExhaustCur = SFXInfo.Exhaust;
		SFXInfo.Exhaust = NewExhaust;
	}

}

void AUTVehicle_Fury::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);
	
	if ( bDriving )
	{
		if ( Mesh->Translation.Z != 0.0 )
		{
			if ( TargetTranslationTime > 0.0 )
			{
				Mesh->Translation.Z += ( 0 - Mesh->Translation.Z ) * ( DeltaSeconds / TargetTranslationTime );
				TargetTranslationTime -= DeltaSeconds;
			}
			else
			{
				Mesh->Translation.Z = 0;
			}
			Mesh->BeginDeferredUpdateTransform();
		}

		FLOAT EngineStrengthForward = VState.ServerGas > 128  ? 0.75 : 0.1;
		FLOAT EngineStrengthLeft; 
		FLOAT EngineStrengthRight; 

		if ( VState.ServerSteering > 128 )
		{
			EngineStrengthLeft = 0.0f;
			EngineStrengthRight = 0.75f;
		}
		else if ( VState.ServerSteering < 128 )
		{
			EngineStrengthLeft = 0.75f;
			EngineStrengthRight = 0.0f;
		}
		else
		{
			EngineStrengthLeft = 0.1f;
			EngineStrengthRight = 0.1f;
		}

		// Boosting

		if ( BoostAttempt != EBD_None )
		{
			// turbo mode

			if ( BoostStatus == EBD_None )
			{
				if ( BoostCost[BoostAttempt] <= BoostEnergy ) // Starting boost
				{
					FLOAT RampDownTime = MaxBoostDuration[BoostAttempt] - BoostRampUpTime;
					switch (BoostAttempt)
					{
						case EBD_Forward:
							Boost(JetSFX[0], BoostRampUpTime, RampDownTime > BoostRampDownTime ? BoostRampDownTime : RampDownTime);
							break;

						case EBD_Left:
							Boost(JetSFX[2], BoostRampUpTime, RampDownTime > BoostRampDownTime ? BoostRampDownTime : RampDownTime);
							break;
						case EBD_Right:
							Boost(JetSFX[1], BoostRampUpTime, RampDownTime > BoostRampDownTime ? BoostRampDownTime : RampDownTime);
							break;
					}

					eventActivateRocketBoosters(BoostAttempt);
				}
			}
		}

		else
		{
			BoostEnergy = Clamp<FLOAT> ( ( BoostEnergy + (BoostRechargeRate * DeltaSeconds)), 0.0f, 1.0f);
		}


		BoostAttempt = EBD_None;

		if ( (Role == ROLE_Authority) || IsLocallyControlled() )
		{
			if ( BoostStatus != EBD_None )
			{
				if ( WorldInfo->TimeSeconds > BoostEndTime  ) // Ran out of Boost
				{
					eventDeactivateRocketBoosters();
				}
			}
		}

		if ( BoostStatus != EBD_None )
		{
			FVector BoostDir;
			FRotationMatrix R(Rotation);
			switch (BoostStatus)
			{
				case EBD_Forward:  
					BoostDir = Rotation.Vector();
					break;

				case EBD_Left:
					BoostDir = R.GetAxis(1) * -1;
					break;

				case EBD_Right:
					BoostDir = R.GetAxis(1);

					break;
			}

			if ( Velocity.SizeSquared() < BoostPowerSpeed*BoostPowerSpeed )
			{
				if ( BoostDir.Z > 0.7f )
					AddForce( (1.f - BoostDir.Z) * BoosterForceMagnitude[BoostStatus] * BoostDir );
				else
					AddForce( BoosterForceMagnitude[BoostStatus] * BoostDir );
			}
			else
			{
					AddForce( 0.25f * BoosterForceMagnitude[BoostStatus] * BoostDir );
			}
		}
		else
		{
			BoostTo(JetSFX[0],EngineStrengthForward, 0.5f);
			BoostTo(JetSFX[1],EngineStrengthLeft, 0.5f);
			BoostTo(JetSFX[2],EngineStrengthRight, 0.5f);
		}

		// Process the Jet Effects

		for (INT i=0;i<3;i++)
		{
			FLOAT Timer = 0.0f;
			if ( JetSFX[i].ExhaustRampUpTime > 0.0f )
			{
				JetSFX[i].ExhaustCur += ( JetSFX[i].Exhaust - JetSFX[i].ExhaustCur ) * ( DeltaSeconds / JetSFX[i].ExhaustRampUpTime );
				JetSFX[i].ExhaustRampUpTime -= DeltaSeconds;

				if ( JetSFX[i].ExhaustRampUpTime <= 0.0f )
				{
					BoostTo(JetSFX[0],EngineStrengthForward, 0.5f);
					BoostTo(JetSFX[1],EngineStrengthLeft, 0.5f);
					BoostTo(JetSFX[2],EngineStrengthRight, 0.5f);
				}

			}
			else if ( JetSFX[i].ExhaustRampDownTime > 0.0f )
			{
				JetSFX[i].ExhaustCur += ( JetSFX[i].Exhaust - JetSFX[i].ExhaustCur ) * ( DeltaSeconds / JetSFX[i].ExhaustRampDownTime );
				JetSFX[i].ExhaustRampDownTime -= DeltaSeconds;
			}

			if ( VehicleEffects(0).EffectRef )
			{
				VehicleEffects(0).EffectRef->SetFloatParameter(JetSFX[i].ExhaustTag, JetSFX[i].ExhaustCur);
			}
		}

		// Tweak the roll depending on the speed

		FRigidBodyState rbState;
		GetCurrentRBState(rbState);
		FLOAT AngVel = rbState.AngVel.Size();

		FLOAT RollScale = 1.0;
		FLOAT PitchScale = 1.0;

		// Collision snaps the constrain back hard

		if ( WorldInfo->TimeSeconds > LastCollisionSoundTime + 0.5 )
		{
			if ( AngVel > UprightRollMinThreshold )
			{
				FLOAT Perc = (AngVel - UprightRollMinThreshold) / (UprightRollMaxThreshold - UprightRollMinThreshold);
				RollScale = 5 + (UprightMaxModifier * Perc);
			}
		}	
		StayUprightConstraintInstance->SetAngularDOFLimitScale(RollScale,PitchScale,1.0,StayUprightConstraintSetup);

		UUTVehicleSimChopper* SObj = Cast<UUTVehicleSimChopper>(SimObj);
		FLOAT TurnModifier = 0.0;

		if (WorldInfo->NetMode != NM_DedicatedServer && BeamLockedInfo.LockedTarget != NULL)
		{
			FVector TargetLoc;
			APawn* P = Cast<APawn>(BeamLockedInfo.LockedTarget);
			if (P != NULL && BeamLockedInfo.LockedBone != NAME_None)
			{
				TargetLoc = P->Mesh->GetBoneLocation( BeamLockedInfo.LockedBone );
			}
			else
			{
				TargetLoc = BeamLockedInfo.LockedTarget->Location;
			}

			for (INT i = 0; i < 4; i++)
			{
				if (BeamEmitter[i] != NULL)
				{
					BeamEmitter[i]->SetVectorParameter(EndPointParamName, TargetLoc);
					UBOOL bNowHidden = FALSE;

					FVector TraceStart;
					FCheckResult Hit(1.f);
					Mesh->GetSocketWorldLocationAndRotation(BeamSockets[i], TraceStart, NULL);

					FVector TraceEnd = TargetLoc;
					TraceEnd = TraceEnd + ( (TraceEnd - TraceStart).SafeNormal() * 64 );
					if (!GWorld->SingleLineCheck(Hit, this, TraceEnd, TraceStart, TRACE_ProjTargets | TRACE_ComplexCollision) && Hit.Actor != BeamLockedInfo.LockedTarget)
					{
						bNowHidden = TRUE;
					}

					BeamEmitter[i]->SetHiddenGame(bNowHidden);
				}
			}
		}

		// Check to make sure we are not playing the landed animation.

		if ( BlendNode && BlendNode->Child2Weight >= 1.0 )
		{
			eventPlayTakeOff();
		}
	}
	else if (Physics != PHYS_None)
	{
		StayUprightConstraintInstance->SetAngularDOFLimitScale(1.0,1.0,1.0,StayUprightConstraintSetup);

		if ( TargetTranslationTime > 0.0 )
		{
			Mesh->Translation.Z += ( TargetTranslationZ - Mesh->Translation.Z ) * ( DeltaSeconds / TargetTranslationTime );
			TargetTranslationTime -= DeltaSeconds;
			Mesh->BeginDeferredUpdateTransform();
		}
		else if ( Mesh->Translation.Z != TargetTranslationZ )
		{
			Mesh->Translation.Z = TargetTranslationZ;
			Mesh->BeginDeferredUpdateTransform();
		}
		
		// Trace Straight down and if we are close to the ground.. switch to the land animation
		FVector TraceStart = Location;
		FVector TraceEnd;
		TraceEnd = TraceStart + ( FVector(0,0,-1) * 1024 );

		FCheckResult Hit(1.f);
		if ( !GWorld->SingleLineCheck(Hit, this, TraceEnd, TraceStart, TRACE_World) )
		{
			if (BlendNode != NULL && BlendNode->Child2Weight <= 0.0)
			{
				eventPlayLanding();
			}
		}
	}
}

void UUTSkelControl_NecrisRaptorWings::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{

	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());

	FLOAT RequiredAngles[2];
	FLOAT GndDist[2] = { 100,100 };

	if (!bInitialized)
	{
		UpperWingIndex = SkelComp->MatchRefBone( UpperWingBoneName );
		bInitialized = true;
	}

	if ( OwnerVehicle && SkelComp && OwnerVehicle->Driver )
	{
		FVector JointStart, AdjJointStart;
		FVector TipStart, End;

		FCheckResult Hit(1.0f);

		UBOOL bWasCollision = false;

		SkelComp->GetSocketWorldLocationAndRotation(JointSocket, JointStart, NULL);
		SkelComp->GetSocketWorldLocationAndRotation(TipSocket, TipStart, NULL);

		// Trace from the same height

		TipStart.Z = JointStart.Z;

		// Adjust the Joint start position by the velocity ignoring the Z
		FVector Vel = OwnerVehicle->Velocity;
		Vel.Z = 0;

		AdjJointStart = JointStart + (Vel * 0.2f );

		// Look for collisions and adjust if needed

		if ( !GWorld->SingleLineCheck(Hit, OwnerVehicle, AdjJointStart , JointStart, TRACE_AllBlocking ) )
		{
			AdjJointStart = Hit.Location;
			bWasCollision = true;
		}
			
		// Trace the Expected Position

		FVector TraceVector;
		TraceVector = bInverted ? FVector(0,0,1) : FVector(0,0,-1);

		End = AdjJointStart + (TraceVector * WingLength);

//		GWorld->LineBatcher->DrawLine(AdjJointStart,End,FColor(255,0,0));

		if ( !GWorld->SingleLineCheck(Hit, OwnerVehicle, End, AdjJointStart, TRACE_AllBlocking ) )
		{
			GndDist[0] = (Hit.Location - AdjJointStart).Size();
			RequiredAngles[0] = CalcRequiredAngle(AdjJointStart, Hit.Location, WingLength);				
			bWasCollision = true;
		}
		else
			RequiredAngles[0] = 0.0f;

		// Trace from the Wing Tip
		
		End = TipStart + (TraceVector * WingLength);
//		GWorld->LineBatcher->DrawLine(TipStart,End,FColor(255,255,0));

		if ( !GWorld->SingleLineCheck(Hit, OwnerVehicle, End, TipStart, TRACE_AllBlocking ) )
		{
			GndDist[1] = (Hit.Location - TipStart).Size();
			RequiredAngles[1] = CalcRequiredAngle(TipStart, Hit.Location, WingLength);
		}
		else
			RequiredAngles[1] = 0.0f;

		FLOAT MaxRequiredAngle = Max(RequiredAngles[0], RequiredAngles[1]);
		FLOAT GndDistance = (RequiredAngles[0] > RequiredAngles[1]) ? GndDist[0] : GndDist[1];
		INT RequiredPitch = appTrunc(MaxRequiredAngle * 182.0444) & 65535;
		
		UBOOL bMakeUpdate = false;
		
		FLOAT NewPitchRate = 0.0f;

		if (MaxRequiredAngle > DesiredAngle )
		{
			bMakeUpdate = true;
			UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds + MinUpTime;
			NewPitchRate = 16384.0f / (GndDistance / OwnerVehicle->Velocity.Size()) * DeltaSeconds;  //125535.0f * DeltaSeconds;

		}
		else if (bWasCollision)
		{
			UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds + MinUpTime;
		}
		else if (GWorld->GetWorldInfo()->TimeSeconds > UpTimeSeconds)
		{
			bMakeUpdate = true;
			NewPitchRate = 8192.0f * DeltaSeconds;
		}

		if (bMakeUpdate)
		{
			if ( !bForcedDesired )
			{
				DesiredPitch = RequiredPitch;
				PitchRate    = NewPitchRate;
			}
			DesiredAngle = appTrunc(MaxRequiredAngle);
		}
	}
	else
	{
		PitchRate = 32767.0f * DeltaSeconds;
		DesiredPitch = 16384;
		UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds; 
	}


	Super::TickSkelControl(DeltaSeconds, SkelComp);

}

void UUTSkelControl_NecrisRaptorWings::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
/*
	if ( bInitialized && SkelComp->GetOwner() )
	{
		// Find the actual Index to look up the data

		ActualPitch = SkelComp->GetOwner()->fixedTurn(ActualPitch, DesiredPitch, appTrunc(PitchRate));
		if ( (ActualPitch & 65535) == (DesiredPitch & 65535) )
		{
			bForcedDesired = 0;
		}

		if (BoneIndex == UpperWingIndex)
		{
			BoneRotation.Pitch = appTrunc(ActualPitch * -0.75) & 65535;
		}
		else
		{
			BoneRotation.Pitch = ActualPitch & 65535;
		}
	}
*/
	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms); 

}

