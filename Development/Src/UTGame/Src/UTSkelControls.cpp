/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameAnimationClasses.h"
#include "UTGameVehicleClasses.h"

IMPLEMENT_CLASS(UUTSkelControl_TurretConstrained);
IMPLEMENT_CLASS(UUTSkelControl_MantaBlade);
IMPLEMENT_CLASS(UUTSkelControl_MantaFlaps);
IMPLEMENT_CLASS(UUTSkelControl_RaptorWing);
IMPLEMENT_CLASS(UUTSkelControl_RaptorTail);
IMPLEMENT_CLASS(UUTSkelControl_TankTread);
IMPLEMENT_CLASS(UUTSkelControl_HugGround);
IMPLEMENT_CLASS(UUTSkelControl_LockRotation);
IMPLEMENT_CLASS(UUTSkelControl_Oscillate);
IMPLEMENT_CLASS(UUTSkelControl_Damage);
IMPLEMENT_CLASS(UUTSkelControl_DamageHinge);
IMPLEMENT_CLASS(UUTSkelControl_DamageSpring);
IMPLEMENT_CLASS(UUTSkelControl_CantileverBeam);
IMPLEMENT_CLASS(UUTSkelControl_MassBoneScaling);
IMPLEMENT_CLASS(UUTSkelControl_CicadaEngine);
IMPLEMENT_CLASS(UUTSkelControl_JetThruster);
IMPLEMENT_CLASS(UUTSkelControl_DarkWalkerHeadTurret);
IMPLEMENT_CLASS(UUTSkelControl_Rotate);
IMPLEMENT_CLASS(UUTSkelControl_TurretMultiBone);
IMPLEMENT_CLASS(UUTSkelControl_LookAt);
IMPLEMENT_CLASS(UUTSkelControl_HoverboardSuspension);
IMPLEMENT_CLASS(UUTSkelControl_HoverboardSwing);
IMPLEMENT_CLASS(UUTSkelControl_HoverboardVibration);
IMPLEMENT_CLASS(UUTSkelControl_SpinControl);

static INT RotationalTurn(INT Current, INT Desired, FLOAT DeltaRatePerSecond)
{

	const INT DeltaRate = appTrunc( DeltaRatePerSecond );

	if (DeltaRate == 0)
		return (Current & 65535);

	INT result = Current & 65535;
	Current = result;
	Desired = Desired & 65535;

	if (Current > Desired)
	{
		if (Current - Desired < 32768)
			result -= Min((Current - Desired), Abs(DeltaRate));
		else
			result += Min((Desired + 65536 - Current), Abs(DeltaRate));
	}
	else
	{
		if (Desired - Current < 32768)
			result += Min((Desired - Current), Abs(DeltaRate));
		else
			result -= Min((Current + 65536 - Desired), Abs(DeltaRate));
	}
	return (result & 65535);
}

/**********************************************************************************
 * UTSkelControl_TurretConstrained
 *
 * Most of the UT Vehicles use this class to constrain their weapon turrets
 **********************************************************************************/

/**
 * This function will check a rotational value and make sure it's constrained within
 * a given angle.  It returns the value
 */
static INT CheckConstrainValue(INT Rotational, INT MinAngle, INT MaxAngle)
{
	INT NormalizedRotational = Rotational & 65535;

	if (NormalizedRotational > 32767)
	{
		NormalizedRotational = NormalizedRotational - 65535;
	}

	// Convert from Degrees to Unreal Units

	MinAngle = appTrunc( FLOAT(MinAngle) * 182.0444);
	MaxAngle = appTrunc( FLOAT(MaxAngle) * 182.0444);

	if ( NormalizedRotational > MaxAngle )
	{
		return MaxAngle;
	}
	else if ( NormalizedRotational < MinAngle )
	{
		return MinAngle;
	}

	return NormalizedRotational;
}

/**
 * This function performs the magic.  It will attempt to rotate the turret to face the rotational specified in DesiredBoneRotation.
 * 
 */
void UUTSkelControl_TurretConstrained::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());
	if ( bFixedWhenFiring && OwnerVehicle )
	{
		if (OwnerVehicle->SeatFlashLocation(AssociatedSeatIndex, FVector(0,0,0),TRUE) != FVector(0,0,0) ||
				OwnerVehicle->SeatFlashCount(AssociatedSeatIndex,0,TRUE) > 0 )
		{
			return;
		}
	}

	if ( bResetWhenUnattended && OwnerVehicle &&
			OwnerVehicle->Seats.IsValidIndex(AssociatedSeatIndex) &&
			(OwnerVehicle->SeatMask & (1 << AssociatedSeatIndex)) == 0 )
	{
		StrengthTarget = 0.0;
		ControlStrength = 0.0;
		Super::TickSkelControl(DeltaSeconds, SkelComp);
		return;
	}
	else
	{
		StrengthTarget = 1.0;
		ControlStrength = 1.0;
	}

	FVector LocalDesiredVect; 
	FRotator LocalDesired;

	// Convert the Desired to Local Space

	LocalDesiredVect = SkelComp->LocalToWorld.InverseTransformNormal( DesiredBoneRotation.Vector() );
	LocalDesired = LocalDesiredVect.Rotation();
	
	LocalDesired.Yaw	*= bInvertYaw   ? -1 : 1;
	LocalDesired.Pitch	*= bInvertPitch ? -1 : 1;
	LocalDesired.Roll	*= bInvertRoll  ? -1 : 1;

	// Constrain the Desired Location.


	// Look up the proper step givin the current yaw

	FTurretConstraintData FMinAngle = MinAngle;
	FTurretConstraintData FMaxAngle = MaxAngle;

	INT NormalizedYaw = LocalDesired.Yaw & 65535;

	for (INT i=0;i<Steps.Num(); i++)
	{
		if ( NormalizedYaw >= Steps(i).StepStartAngle && NormalizedYaw <= Steps(i).StepEndAngle )
		{
			FMinAngle = Steps(i).MinAngle;
			FMaxAngle = Steps(i).MaxAngle;
			break;
		}
	}

	// constrain the rotation
	if (bConstrainYaw)
	{
		LocalDesired.Yaw = CheckConstrainValue(LocalDesired.Yaw, FMinAngle.YawConstraint,FMaxAngle.YawConstraint);
	}
	if (bConstrainPitch)
	{
		LocalDesired.Pitch = CheckConstrainValue(LocalDesired.Pitch, FMinAngle.PitchConstraint,FMaxAngle.PitchConstraint);
	}
	if (bConstrainRoll)
	{
		LocalDesired.Roll = CheckConstrainValue(LocalDesired.Roll, FMinAngle.RollConstraint,FMaxAngle.RollConstraint);
	}

	// If we are not Pointing at the desired rotation, rotate towards it
	FRotator LocalConstrainedBoneRotation = SkelComp->LocalToWorld.InverseTransformNormal(ConstrainedBoneRotation.Vector()).Rotation().Denormalize();
	if (LocalConstrainedBoneRotation != LocalDesired)
	{
		if (LagDegreesPerSecond>0 && SkelComp->GetOwner())
		{
			INT DeltaDegrees = appTrunc((LagDegreesPerSecond * 182.0444) * DeltaSeconds);

			if (LocalConstrainedBoneRotation.Yaw != LocalDesired.Yaw)
			{
				LocalConstrainedBoneRotation.Yaw = SkelComp->GetOwner()->fixedTurn(LocalConstrainedBoneRotation.Yaw, LocalDesired.Yaw, DeltaDegrees);
			}

			if (LocalConstrainedBoneRotation.Pitch != LocalDesired.Pitch)
			{
				LocalConstrainedBoneRotation.Pitch = SkelComp->GetOwner()->fixedTurn(LocalConstrainedBoneRotation.Pitch, LocalDesired.Pitch, DeltaDegrees);
			}

			if (LocalConstrainedBoneRotation.Roll != LocalDesired.Roll)
			{
				LocalConstrainedBoneRotation.Roll = SkelComp->GetOwner()->fixedTurn(LocalConstrainedBoneRotation.Roll, LocalDesired.Roll, DeltaDegrees);
			}
		}
		else
		{
			LocalConstrainedBoneRotation = LocalDesired;
		}
	}
	// set the bone rotation to the final clamped value
	UBOOL bNewInMotion;
	if(BoneRotation == LocalConstrainedBoneRotation)
	{
		bNewInMotion = FALSE;
	}
	else
	{
		bNewInMotion = TRUE;
		BoneRotation = LocalConstrainedBoneRotation;
	}

	// also save the current world space rotation for future ticks
	// this is so that the movement of the actor/component won't affect the rotation rate
	ConstrainedBoneRotation = SkelComp->LocalToWorld.TransformNormal(LocalConstrainedBoneRotation.Vector()).Rotation();

	// find out if we're still in motion and call delegate if the status has changed
	
	if (bNewInMotion != bIsInMotion)
	{
		bIsInMotion = bNewInMotion;

		// Notify anyone listening

		if (DELEGATE_IS_SET(OnTurretStatusChange))
		{
			delegateOnTurretStatusChange(bIsInMotion);
		}
	}
	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

/**********************************************************************************
 * UTSkelControl_MantaBlade
 *
 * Spins the manta blades
 **********************************************************************************/

void UUTSkelControl_MantaBlade::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());
	if (OwnerVehicle && OwnerVehicle->bDriving)
		DesiredRotationsPerSecond=MaxRotationsPerSecond;
	else
		DesiredRotationsPerSecond=0;

	if (SpinUpTime>0)
	{
		if ( RotationsPerSecond < DesiredRotationsPerSecond )
		{
			RotationsPerSecond += MaxRotationsPerSecond * SpinUpTime * DeltaSeconds;
			if (RotationsPerSecond>DesiredRotationsPerSecond)
				RotationsPerSecond = DesiredRotationsPerSecond;
		}
		else if (RotationsPerSecond > DesiredRotationsPerSecond)
		{
			RotationsPerSecond -= MaxRotationsPerSecond * SpinUpTime * DeltaSeconds;
			if (RotationsPerSecond<DesiredRotationsPerSecond)
				RotationsPerSecond = DesiredRotationsPerSecond;
		}
	}
	else
		RotationsPerSecond = DesiredRotationsPerSecond;

	if (bCounterClockwise)
		DeltaSeconds *= -1;

	if (RotationsPerSecond > 0)
	{
		BoneRotation.Yaw = BoneRotation.Yaw + INT(RotationsPerSecond * 65536 * DeltaSeconds);
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

/**********************************************************************************
 * UTSkelControl_MantaFlags
 **********************************************************************************/


void UUTSkelControl_MantaFlaps::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());
	if (OwnerVehicle && OwnerVehicle->bDriving)
	{

		const FLOAT ZForce = Abs(OwnerVehicle->Velocity.Z);
		FLOAT NewPitch = 0.0f;

		// Get body angular velocity
		FRigidBodyState rbState;
		OwnerVehicle->GetCurrentRBState(rbState);


		if ( ZForce > 100.f )
		{
			NewPitch += maxPitch * (ZForce - 100.f) / 900.f * 182.044f;
			if (OwnerVehicle->Velocity.Z > 0.f)
			{
				NewPitch *= -1;
			}

			// smooth change
			if ( Abs(NewPitch - OldZPitch) > MaxPitchChange * DeltaSeconds )
			{
				NewPitch = (NewPitch > OldZPitch) ? OldZPitch + MaxPitchChange*DeltaSeconds : OldZPitch - MaxPitchChange*DeltaSeconds;
			}
		}
		else
		{
			// let pitch reduce smoothly over time
			NewPitch = OldZPitch * (1.f - MaxPitchTime * DeltaSeconds);
		}
		OldZPitch = NewPitch;

		if (Abs(rbState.AngVel.Z) > 100.0f)
		{
			if (ControlName == RightFlapControl)
				NewPitch += -maxPitch * (rbState.AngVel.Z / 4000.0f) * 182.044f;			

			if (ControlName == LeftFlapControl)
				NewPitch += maxPitch * (rbState.AngVel.Z / 4000.0f) * 182.044f;		
		}

		BoneRotation.Pitch = Clamp<INT>(appTrunc(NewPitch), appTrunc(-maxPitch * 182.044f), appTrunc(maxPitch * 182.044f));
	}
	else
		BoneRotation.Pitch = 0;

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

/************************************************************************************
 * UTSkelControl_RaptorWing
 *
 * Controls the wings of a Raptor
 ************************************************************************************/

void UUTSkelControl_RaptorWing::ForceSingleWingUp(UBOOL bRight, FLOAT Rate)
{
	const INT Idx = (bRight)? 1 : 0;

	PitchRate[Idx] = Rate;
	DesiredPitch[Idx] = -16384;
	bForcedDesired[Idx] = 1;

}
void UUTSkelControl_RaptorWing::ForceWingsUp(FLOAT Rate)
{
	ForceSingleWingUp(FALSE, Rate);
	ForceSingleWingUp(TRUE, Rate);
}
	
void UUTSkelControl_RaptorWing::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle_Raptor* OwnerVehicle = Cast<AUTVehicle_Raptor>(SkelComp->GetOwner());

	if (!bInitialized)
	{
		for (INT i=0;i<2;i++)
		{
			Bones[i] = SkelComp->MatchRefBone( TipBones[i] );
		}
		bInitialized = true;
	}


	if ( OwnerVehicle && SkelComp && OwnerVehicle->bDriving )
	{
		// wings up if descending or going forward
		if ( OwnerVehicle->bForwardMode || (OwnerVehicle->Velocity.Z < (bAlreadyDescending ? -40.f : -200.f))  )
		{
			bAlreadyDescending = TRUE;
			PitchRate[0]= PitchRate[1] = 32767.0f * DeltaSeconds;
			DesiredPitch[0] = DesiredPitch[1] = -16384;
			UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds; 
		}
		else
		{
			bAlreadyDescending = FALSE;

			FVector JointStart, AdjJointStart;
			FVector TipStart, End;
			FCheckResult Hit(1.0f);
			UBOOL bWasCollision = FALSE;
			FLOAT RequiredAngles[4];

			// Perform the 4 traces (2 from each tip, 2 from each joint + velocity)
			for (INT Wing=0;Wing<2;Wing++)
			{
				SkelComp->GetSocketWorldLocationAndRotation(JointSockets[Wing], JointStart, NULL);
				SkelComp->GetSocketWorldLocationAndRotation(TipSockets[Wing], TipStart, NULL);

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
				End = AdjJointStart + (FVector(0,0,-1) * WingLength);
				if ( !GWorld->SingleLineCheck(Hit, OwnerVehicle, End, AdjJointStart, TRACE_AllBlocking ) )
				{
					RequiredAngles[Wing * 2] = CalcRequiredAngle(AdjJointStart, Hit.Location, WingLength);
					bWasCollision = true;
				}
				else
					RequiredAngles[Wing * 2] = 0.0f;

				// Trace from the Wing Tip
				End = TipStart + (FVector(0,0,-1) * WingLength);
				if ( !GWorld->SingleLineCheck(Hit, OwnerVehicle, End, TipStart, TRACE_AllBlocking ) )
				{
					RequiredAngles[Wing * 2 +1] = CalcRequiredAngle(TipStart, Hit.Location, WingLength);
				}
				else
					RequiredAngles[Wing * 2 +1] = 0.0f;
			}

			FLOAT MaxRequiredAngle = 0.0f;
			for (INT i=0;i<4;i++)
			{
				if (RequiredAngles[i] > MaxRequiredAngle)
					MaxRequiredAngle = RequiredAngles[i];
			}
			
			INT RequiredPitch = INT(MaxRequiredAngle * - 182.0444) & 65535;
			UBOOL bMakeUpdate = false;
			FLOAT NewPitchRate = 0.0f;

			if (MaxRequiredAngle > DesiredAngle )
			{
				bMakeUpdate = true;
				UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds + MinUpTime;
				NewPitchRate = 125535.0f * DeltaSeconds;

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
				for (INT w=0;w<2;w++)
				{
					if ( !bForcedDesired[w] )
					{
						DesiredPitch[w] = RequiredPitch;
						PitchRate[w]    = NewPitchRate;
					}
				}
				DesiredAngle = appTrunc(MaxRequiredAngle);
			}
		}
	}
	else
	{
		bAlreadyDescending = FALSE;
		PitchRate[0]= PitchRate[1] = 32767.0f * DeltaSeconds;
		DesiredPitch[0] = DesiredPitch[1] = -16384;
		UpTimeSeconds = GWorld->GetWorldInfo()->TimeSeconds; 
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

void UUTSkelControl_RaptorWing::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	if (bInitialized)
	{
		// Find the actual Index to look up the data

		INT BoneIdx = -1;
		for (INT i=0;i<2;i++)
		{
			if (BoneIndex == Bones[i])
				BoneIdx = i;
		}

		if (BoneIdx >= 0)
		{
			ActualPitch[BoneIdx] = SkelComp->GetOwner()->fixedTurn(ActualPitch[BoneIdx], DesiredPitch[BoneIdx], appTrunc(PitchRate[BoneIdx]));
			if ( (ActualPitch[BoneIdx] & 65535) == (DesiredPitch[BoneIdx] & 65535) )
			{
				bForcedDesired[BoneIdx] = 0;
			}

			BoneRotation.Pitch = ActualPitch[BoneIdx] & 65535;
		}
	}

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms); 
}

/************************************************************************************
 * UTSkelControl_RaptorWing
 *
 * Controls the tail of a Raptor
 ************************************************************************************/


void UUTSkelControl_RaptorTail::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{

	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if (!SkelComp)
		return;

	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());

	if (OwnerVehicle)
	{
		if ( bInitialized )
		{
			LastVehicleYaw = OwnerVehicle->Rotation.Yaw;
			TailYaw = 0;
		}

		if ( OwnerVehicle->bDriving )
		{
			INT VehicleYaw = OwnerVehicle->Rotation.Yaw;	

			INT Dist = (VehicleYaw - LastVehicleYaw) & 65535;
			if (Dist>32767)
				Dist-=65536;

			if (Abs(Dist) > Deadzone ) // Slight amount of dead zone
			{
				const INT Weight = Clamp<INT>(Abs(Dist),0,400);
				const FLOAT Perc = FLOAT(Weight) / 400;
				DesiredTailYaw = INT( (YawConstraint * 182.0444) * Perc) * (Dist>0?1:-1);
			}
			else
			{
				DesiredTailYaw = 0;
			}

			LastVehicleYaw = VehicleYaw;

			if (DesiredTailYaw < TailYaw)
			{
				TailYaw -= appTrunc((YawConstraint * 182.0444) * DeltaSeconds);
				if (TailYaw < DesiredTailYaw)
					TailYaw = DesiredTailYaw;
			}
			else if (DesiredTailYaw > TailYaw)
			{
				TailYaw += appTrunc((YawConstraint * 182.0444) * DeltaSeconds);
				if (TailYaw > DesiredTailYaw)
					TailYaw = DesiredTailYaw;
			}

			BoneRotation.Yaw= TailYaw & 65535;

		}
	}
}

/************************************************************************************
 * UTSkelControl_TankTread
 *
 * Tank Treads
 ************************************************************************************/

FLOAT UUTSkelControl_TankTread::CalcAdjustment(FVector TraceStart, FVector TraceEnd, FVector Offsets, AActor* Owner)
{
	TraceStart = TraceStart + (FRotationMatrix(Owner->Rotation).TransformNormal(Offsets));
	TraceEnd = TraceEnd + (FRotationMatrix(Owner->Rotation).TransformNormal(Offsets));

	FCheckResult Hit(1.0f);

	//GWorld->LineBatcher->DrawLine(TraceStart, TraceEnd, FColor(255,255,255));
	//GWorld->LineBatcher->DrawLine(TraceStart, FVector(0,0,0), FColor(255,255,0));
	//GWorld->LineBatcher->DrawLine(TraceEnd,   FVector(0,0,0), FColor(255,255,0));


	if ( !GWorld->SingleLineCheck(Hit, Owner, TraceEnd , TraceStart, TRACE_AllBlocking ) )
	{
		//GWorld->LineBatcher->DrawLine(Hit.Location, FVector(0,0,0), FColor(0,255,0));
		
		return SpaceAbove - (TraceStart - Hit.Location).Size();
	}
	else
		return SpaceBelow * -1;
}

void UUTSkelControl_TankTread::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	check(SkelComp);

	if ( SkelComp && !bInitialized && TreadBone != NAME_None )
	{
		TreadIndex = SkelComp->MatchRefBone( TreadBone );
		Adjustment = 0.0f;
		TargetAdjustment = 0.0f;
		bInitialized = true;
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if (bInitialized)
	{
		AActor* Owner = SkelComp->GetOwner();

		//FLOAT Dist = 0.0f;
		if (Owner != NULL && Owner->Velocity.SizeSquared() > 10.f)
		{
			const FVector TotalScale = Owner->DrawScale * Owner->DrawScale3D;
			const FVector BoneLoc = TotalScale * (SkelComp->SkeletalMesh->RefSkeleton(TreadIndex).BonePos.Position);	
			
			FMatrix ActorToWorld = Owner->LocalToWorld();
			ActorToWorld.RemoveScaling();

			FMatrix CompToWorld = SkelComp->LocalToWorld;
			CompToWorld.RemoveScaling();

			const FVector TraceStart = CompToWorld.TransformFVector( BoneLoc ) + ActorToWorld.TransformNormal( FVector(0,0,SpaceAbove));
			const FVector TraceEnd = CompToWorld.TransformFVector(BoneLoc)  - ActorToWorld.TransformNormal( FVector(0,0, SpaceBelow ));

			// + Debug

			TargetAdjustment = CalcAdjustment(TraceStart, TraceEnd, FVector(0,CenterOffset,0), Owner);

			bLastDirWasBackwards = ((Owner->Velocity | Owner->Rotation.Vector()) < 0.f);

			for (INT i = 0; i < AlternateScanOffsets.Num(); i++)
			{
				if ( bAlwaysScan || (bLastDirWasBackwards && AlternateScanOffsets(i) < 0) || (!bLastDirWasBackwards && AlternateScanOffsets(i)>0) )
				{
					const FLOAT SecondaryAdj = CalcAdjustment(TraceStart, TraceEnd, FVector(AlternateScanOffsets(i),CenterOffset,0), Owner);
					if (SecondaryAdj > TargetAdjustment)
					{
						TargetAdjustment = SecondaryAdj;
					}
				}
			}

			if (Adjustment > TargetAdjustment)
			{
				const FLOAT NewAdj = Adjustment - ( (Adjustment-TargetAdjustment) * DeltaSeconds * 4);
				Adjustment = Clamp<FLOAT>( NewAdj, TargetAdjustment, Adjustment);
			}
			else if (Adjustment < TargetAdjustment)
			{
				const FLOAT NewAdj = Adjustment + ( (TargetAdjustment - Adjustment) * DeltaSeconds * 4);
				Adjustment = Clamp<FLOAT>(NewAdj, Adjustment, TargetAdjustment);
			}
		}
	}

}

void UUTSkelControl_TankTread::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	checkSlow(OutBoneTransforms.Num() == 0);

	FMatrix NewBoneTM = SkelComp->SpaceBases(BoneIndex);
	
	if (bInitialized)
	{
		// Find the distance from the wheel to the ground

		FVector NewOrigin = NewBoneTM.GetOrigin() + FVector(0,0,Adjustment+2);
		NewBoneTM.SetOrigin(NewOrigin);
	}

	OutBoneTransforms.AddItem(NewBoneTM);
}

void UUTSkelControl_TankTread::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	checkSlow(OutBoneIndices.Num() == 0);
	if (!bDormant)
	{
		OutBoneIndices.AddItem(BoneIndex);
	}
}

/************************************************************************************
 * UTSkelControl_HugGround
 ************************************************************************************/

void UUTSkelControl_HugGround::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	checkSlow(SkelComp != NULL);
	checkSlow(SkelComp->SkeletalMesh != NULL);

	AActor* Owner = SkelComp->GetOwner();
	if (Owner != NULL)
	{
		const FVector& OldBoneTranslation = BoneTranslation;

		// calculate the trace start (the bone's reference location) and the trace end (MaxDist units in the Actor's negative Z axis) in world space
		FMatrix ActorToWorld = Owner->LocalToWorld();
		ActorToWorld.RemoveScaling();

		FMatrix CompToWorld = SkelComp->LocalToWorld;
		CompToWorld.RemoveScaling();

		const FVector& BoneLocation = CompToWorld.TransformFVector(SkelComp->SpaceBases(BoneIndex).GetOrigin());
		const FVector& TraceEnd = BoneLocation - ActorToWorld.TransformNormal(FVector(0.f, 0.f, MaxDist + DesiredGroundDist));

		FCheckResult Hit(1.0f);
		if (GWorld->SingleLineCheck(Hit, Owner, TraceEnd, BoneLocation + ActorToWorld.TransformNormal(FVector(0.f, 0.f, MaxDist)), TRACE_World ))
		{
			// if we hit nothing, we place the bone MaxDist units from its reference pose and with no rotation change
			BoneTranslation = ActorToWorld.TransformNormal(FVector(0.f, 0.f, -MaxDist));
		}
		// place the bone DesiredGroundDist units from the hit location or MaxDist above the reference pose, whichever is less
		else if (Hit.Time * ((MaxDist * 2.0f) + DesiredGroundDist) < DesiredGroundDist)
		{
			BoneTranslation = ActorToWorld.TransformNormal(FVector(0.f, 0.f, MaxDist));
		}
		else
		{
			BoneTranslation = Hit.Location + ActorToWorld.TransformNormal(FVector(0.f, 0.f, DesiredGroundDist)) - BoneLocation;
		}

		if (ParentBone != NAME_None)
		{
			INT ParentBoneIndex = SkelComp->MatchRefBone(ParentBone);
			if (ParentBoneIndex == INDEX_NONE)
			{
				debugf(NAME_Warning, TEXT("Invalid ParentBone for %s on mesh %s - setting to 'None'"), *GetName(), *SkelComp->SkeletalMesh->GetName());
				ParentBone = NAME_None;
			}
			else
			{
				const FMatrix& ParentMatrix = SkelComp->GetBoneMatrix(ParentBoneIndex);
				// rotate the bone towards the parent bone
				bApplyRotation = true;
				FVector Dir = ParentMatrix.GetOrigin() - (BoneLocation + BoneTranslation);
				// move the translation along the rotation to stay the desired distance from the parent bone
				if (XYDistFromParentBone > 0.f)
				{
					const FVector DirNoZ(Dir.X, Dir.Y, 0.f);
					const FLOAT Dist = DirNoZ.Size();
					if (Dist > XYDistFromParentBone)
					{
						FVector Change = DirNoZ.UnsafeNormal() * (Dist - XYDistFromParentBone);
						BoneTranslation += Change;
						Dir -= Change;
					}
				}

				// pull back the parent location half our distance along the opposite direction of its rotation
				Dir -= ParentMatrix.Rotator().Vector() * (Dir.Size() * 0.5);

				if (ZDistFromParentBone > 0.f && Dir.Z > ZDistFromParentBone)
				{
					BoneTranslation.Z += Dir.Z - ZDistFromParentBone;
				}

				if (bOppositeFromParent)
				{
					Dir = (BoneLocation + BoneTranslation) - ParentMatrix.GetOrigin();
				}
				else
				{
					Dir = ParentMatrix.GetOrigin() - (BoneLocation + BoneTranslation);
				}
				BoneRotation = SkelComp->SpaceBases(BoneIndex).TransformNormal(Dir).Rotation();
				BoneRotation.Yaw = 0;
			}
		}
		else
		{
			bApplyRotation = false;
		}

		// cap BoneTranslation change, if necessary
		if (MaxTranslationPerSec > 0.0f)
		{
			const FLOAT MaxChange = (Owner->WorldInfo->TimeSeconds - LastUpdateTime) * MaxTranslationPerSec;
			const FVector& Diff = BoneTranslation - OldBoneTranslation;
			if (Diff.Size() > MaxChange)
			{
				BoneTranslation = OldBoneTranslation + (Diff.SafeNormal() * MaxChange);
			}
			LastUpdateTime = Owner->WorldInfo->TimeSeconds;
		}
	}

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

/************************************************************************************
 * UTSkelControl_LockRotation
 ************************************************************************************/

void UUTSkelControl_LockRotation::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	checkSlow(OutBoneIndices.Num() == 0);
	OutBoneIndices.AddItem(BoneIndex);
}

void UUTSkelControl_LockRotation::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	checkSlow(SkelComp != NULL);

	AActor* Owner = SkelComp->GetOwner();
	if (Owner != NULL)
	{
		// SpaceBases are in component space - so we need to calculate the BoneRotationSpace -> Component transform
		FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, LockRotationSpace, RotationSpaceBoneName);
		ComponentToFrame.SetOrigin(FVector(0.f));
		
		// get the desired rotation
		FRotator DesiredRotation = (SkelComp->SpaceBases(BoneIndex) * ComponentToFrame).Rotator();
		if (bLockPitch)
		{
			DesiredRotation.Pitch = Owner->fixedTurn(DesiredRotation.Pitch, LockRotation.Pitch, MaxDelta.Pitch);
		}
		if (bLockYaw)
		{
			DesiredRotation.Yaw = Owner->fixedTurn(DesiredRotation.Yaw, LockRotation.Yaw, MaxDelta.Yaw);
		}
		if (bLockRoll)
		{
			DesiredRotation.Roll = Owner->fixedTurn(DesiredRotation.Roll, LockRotation.Roll, MaxDelta.Roll);
		}

		// calculate the transform of the bone to get to the desired rotation
		FRotationMatrix RotInFrame(DesiredRotation);
		FMatrix FrameToComponent = ComponentToFrame.Inverse();
		FMatrix RotInComp = RotInFrame * FrameToComponent;
		RotInComp.SetOrigin(SkelComp->SpaceBases(BoneIndex).GetOrigin());

		OutBoneTransforms.AddItem(RotInComp);
	}
}
/************************************************************************************
 * UTSkelControl_Oscillate
 *
 * Controls the treads of the Necris Tank
 ************************************************************************************/

void UUTSkelControl_Oscillate::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	if (GWorld->HasBegunPlay())
	{
		if (bReverseDirection)
		{
			CurrentTime -= DeltaSeconds;
			if (CurrentTime <= -Period)
			{
				CurrentTime = -Period - (CurrentTime + Period);
				bReverseDirection = false;
			}
		}
		else
		{
			CurrentTime += DeltaSeconds;
			if (CurrentTime >= Period)
			{
				CurrentTime = Period - (CurrentTime - Period);
				bReverseDirection = true;
			}
		}
	}

	BoneTranslation = MaxDelta * (CurrentTime / Period);

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}
/************************************************************************************
 * UTSkelControl_Damage
 *
 * The root of the skel_controls for simulating vehicle damage
 ************************************************************************************/

/**
 * Precache links to the UTVehicle as well as any associated DamageMorphTarget
 *
 * @param	SkelComp	The SkeletalMeshComponent that owns this SkelControl
 * @returns true if we have successfully initialized this control
 */

UBOOL UUTSkelControl_Damage::InitializeControl(USkeletalMeshComponent* SkelComp)
{
	if ( !bInitialized )
	{
		if ( SkelComp && SkelComp->GetOwner() )
		{
			OwnerVehicle = Cast<AUTVehicle>( SkelComp->GetOwner() );
		}

		return (OwnerVehicle != NULL);
	}
	return true;
}

/**
 * The root SkelControl_Damage is responsible for controlling it's ControlStrength by querying the 
 * vehicle (and/or the vehicle's DamageMorphTarget) to check to see if it needs to activate.
 *
 * @See USkelControlBase::TickSkelControl for params
 */
void UUTSkelControl_Damage::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if ( !bInitialized )
	{
		bInitialized = InitializeControl(SkelComp);
	}

	if ( bInitialized && OwnerVehicle )
	{

		// Get the health value to test against.  If we have a linked damage morph target, then use the current
		// health of that target, otherwise use the main vehicle health

		if (HealthPerc >= 1.0)
		{
			bIsBreaking = false;
			bIsBroken = false;
		}

		if ( !bIsBroken )
		{
			// Check to see if we meet that activation threshold.  If we do and it's appropriate, 
			// set the control strength to be a factor of the remaining health, otherwise come on strong.

			if ( HealthPerc < ActivationThreshold )
			{
					ControlStrength = 1 - HealthPerc;
			}
			else
			{
				// Turn it off
				ControlStrength = 0.0;
			}
		}
		else if (bIsBroken || bIsBreaking)
		{
			ControlStrength = 1.0;
		}
		else
		{
			ControlStrength = 0.0;
		}
	}
}

/**
 * Check to see if this control should break 
 *
 * @See USkelControlBase::CalculateNewbonetransforms for params
 */

void UUTSkelControl_Damage::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	if (bInitialized && ControlStrength >= BreakThreshold)
	{
		if (!bIsBroken && !bIsBreaking )
		{
			FLOAT TargetPct = (ControlStrength - BreakThreshold) / (1.0 - BreakThreshold);
			if ( appFrand() < TargetPct )
			{
				BreakTimer = GWorld->GetTimeSeconds() + BreakTime;
				bIsBreaking = true;
			}
		}

		if ( bIsBreaking )
		{
			if (GWorld->GetTimeSeconds() > BreakTimer)
			{
				eventBreakAPart( SkelComp->GetBoneMatrix(BoneIndex).GetOrigin() );
			}
		}
	}

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

/**
 * Looks to see if this control is broken and returns the bone scale if it is 
 */
FLOAT UUTSkelControl_Damage::GetBoneScale(INT BoneIndex, USkeletalMeshComponent* SkelComp)
{
	return (bIsBroken) ? 0.0 : 1.0;
}

/************************************************************************************
 * UTSkelControl_DamageHinge
 *
 * Used for Hoods and such
 ************************************************************************************/

/**
 * Each frame, look at the owning vehicle and adjust the hinge velocity.  
 *
 * @See USkelControlBase::TickSkelControl for params
 */

void UUTSkelControl_DamageHinge::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if ( !bIsBroken && bInitialized )
	{
		FLOAT AngVel;

		// Get the angular velocity given the set pivot axis.  We will use this to adjust the angle of the hinge

		switch (PivotAxis)
		{
			case AXIS_X: 
				AngVel = OwnerVehicle->VState.RBState.AngVel.X;
				break;

			case AXIS_Z: 
				AngVel = OwnerVehicle->VState.RBState.AngVel.Z;
				break;

			default: 
				AngVel = OwnerVehicle->VState.RBState.AngVel.Y;
				break;

		}

		AngVel *= AVModifier;

		// FIXME: Should I make this configurable?

		if (AngVel > -2.0 && AngVel < 2.0)
		{
			AngVel = 0;
		}

		// Adjust the Hinge

		const FLOAT Force = SpringStiffness * CurrentAngle;
		const FLOAT MaxUAngle = MaxAngle * 182.0444;
		CurrentAngle += AngVel + Force * 0.95; 
		CurrentAngle = Clamp<FLOAT>(CurrentAngle,0, MaxUAngle );
		BoneRotation.Pitch = appTrunc( CurrentAngle );
	}
}

/************************************************************************************
 * UTSkelControl_DamageSpring
 *
 * Used for fenders/etc
 ************************************************************************************/

int UUTSkelControl_DamageSpring::CalcAxis(INT &InAngle, FLOAT CurVelocity, FLOAT MinUAngle, FLOAT MaxUAngle)
{
	FLOAT CurAngle = FLOAT(InAngle);
	
	if (CurVelocity > -2.0 && CurVelocity < 2.0)
	{
		CurVelocity = 0.0;
	}

	CurAngle += ( CurVelocity * AVModifier );

	const FLOAT Force = SpringStiffness * CurAngle;  

	CurAngle = (CurAngle + Force) * Falloff; 
	CurAngle = Clamp<FLOAT>(CurAngle, MinUAngle, MaxUAngle );
	InAngle = appTrunc(CurAngle);
	return InAngle;
	
}

/**
 * @See USkelControlBase::TickSkelControl for params
 */

void UUTSkelControl_DamageSpring::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if ( !bIsBroken )
	{
		if ( bInitialized )
		{
			FVector AngVel = OwnerVehicle->VState.RBState.AngVel;
			BoneRotation.Pitch = CalcAxis(CurrentAngle.Pitch, AngVel.Y, FLOAT(MinAngle.Pitch), FLOAT(MaxAngle.Pitch) );
			BoneRotation.Roll =  CalcAxis(CurrentAngle.Roll,  AngVel.X, FLOAT(MinAngle.Roll),  FLOAT(MaxAngle.Roll) );
			BoneRotation.Yaw =   CalcAxis(CurrentAngle.Yaw,   AngVel.Z, FLOAT(MinAngle.Yaw),   FLOAT(MaxAngle.Yaw) );
		}
		if ( SkelComp && OwnerVehicle )
		{
			if(HealthPerc <= 0) // dead part
			{
				TArray<INT>		AffectedBones;
				bIsBreaking = true;
				AffectedBones.Reset();
				GetAffectedBones(0, SkelComp, AffectedBones);
				if( AffectedBones.Num() > 0 )
				{
					eventBreakAPart(SkelComp->GetBoneMatrix(AffectedBones(0)).GetOrigin());
				}

				//eventBreakAPart( SkelComp->GetBoneMatrix(SkelComp->MatchRefBone(OwnerVehicle->DamageMorphTargets(LinkedMorphTargetIndex).InfluenceBone)).GetOrigin() );
				if(!bIsBreaking && !bIsBroken) // we were told this is a bad time to break; so heal up just a hair and wait for a new damage attempt.
				{
					HealthPerc=SMALL_NUMBER;
				}
			}
		}
	}
}

/**
 * Give each control a slightly random AVModifer +/- 10% 
 */

UBOOL UUTSkelControl_DamageSpring::InitializeControl(USkeletalMeshComponent* SkelComp)
{
	const UBOOL Res = Super::InitializeControl(SkelComp);
	if (Res)
	{
		AVModifier += ( (AVModifier * 0.2) * appFrand() ) - (AVModifier * 0.1);
	}

	return Res;
}

void UUTSkelControl_CantileverBeam::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	WorldSpaceGoal = (SkelComp->LocalToWorld.GetOrigin())+InitialWorldSpaceGoalOffset;
	FVector DistFromGoal = TargetLocation - WorldSpaceGoal;

	FVector Force = (DistFromGoal *-SpringStiffness);
	Force -= (SpringDamping * Velocity);
	// apply force to Velocity:
	Velocity += (Force*DeltaSeconds);
	TargetLocation += (Velocity*DeltaSeconds);
}


void UUTSkelControl_MassBoneScaling::SetBoneScale(FName BoneName, FLOAT Scale)
{
	USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(GetOuter()->GetOuter());
	if (SkelComp != NULL)
	{
		const INT BoneIndex = SkelComp->MatchRefBone(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			debugf(NAME_Warning, TEXT("Failed to find bone named %s in mesh %s"), *BoneName.ToString(), *SkelComp->SkeletalMesh->GetName());
		}
		else
		{
			const INT NumToAdd = BoneIndex - (BoneScales.Num() - 1);
			if (NumToAdd > 0)
			{
				BoneScales.Add(NumToAdd);
				for (INT i = 0; i < NumToAdd; i++)
				{
					BoneScales(BoneScales.Num() - i - 1) = 1.0f;
				}
			}
			BoneScales(BoneIndex) = Scale;
		}
	}
}

FLOAT UUTSkelControl_MassBoneScaling::GetBoneScale(FName BoneName)
{
	USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(GetOuter()->GetOuter());
	if (SkelComp != NULL)
	{
		const INT BoneIndex = SkelComp->MatchRefBone(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			debugf(NAME_Warning, TEXT("Failed to find bone named %s in mesh %s"), *BoneName.ToString(), *SkelComp->SkeletalMesh->GetName());
			return 0.f;
		}
		else
		{
			return GetBoneScale(BoneIndex, SkelComp);
		}
	}
	else
	{
		return 0.f;
	}
}

void UUTSkelControl_MassBoneScaling::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	if (BoneIndex < BoneScales.Num() && Abs(BoneScales(BoneIndex) - 1.0f) > KINDA_SMALL_NUMBER)
	{
		OutBoneIndices.AddItem(BoneIndex);
	}
}

void UUTSkelControl_MassBoneScaling::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	OutBoneTransforms.AddItem(SkelComp->SpaceBases(BoneIndex));
}

FLOAT UUTSkelControl_MassBoneScaling::GetBoneScale(INT BoneIndex, USkeletalMeshComponent* SkelComp)
{
	return (BoneIndex < BoneScales.Num()) ? BoneScales(BoneIndex) : 1.0f;
}

void UUTSkelControl_CicadaEngine::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());
	PitchTime = PitchRate;
	if (OwnerVehicle != NULL && OwnerVehicle->bDriving && SkelComp->LastRenderTime > OwnerVehicle->WorldInfo->TimeSeconds - 1.0f)
	{
		const FLOAT Thrust = OwnerVehicle->OutputGas;
		if ( Thrust != LastThrust )
		{
			if ( Thrust > 0 )
			{
				DesiredPitch = appTrunc(ForwardPitch * 182.0444);
			}
			else if ( Thrust < 0 )
			{
				DesiredPitch = appTrunc(BackPitch * 182.0444);
			}
			else
			{
				DesiredPitch = 0;
			}

			// Use the Speed to determine the rate at which it moves
			const FLOAT Speed = Clamp<FLOAT>( OwnerVehicle->Velocity.Size2D(), MinVelocity, MaxVelocity );
			const FLOAT Pct = (Speed - MinVelocity) / (MaxVelocity - MinVelocity);
			PitchTime *= ( 1 + ( (MaxVelocityPitchRateMultiplier - 1) * Pct) );
		}
		LastThrust = Thrust;
	}
	else
	{
		DesiredPitch = 0;
	}

	if ( BoneRotation.Pitch != DesiredPitch )
	{
		BoneRotation.Pitch += appTrunc((DesiredPitch - BoneRotation.Pitch) * (DeltaSeconds / PitchTime));
		PitchTime -= DeltaSeconds;
		if ( PitchTime <= 0 || DesiredPitch == BoneRotation.Pitch )
		{
			PitchTime = 0.0;
			BoneRotation.Pitch = DesiredPitch;
		}
	}

	Super::TickSkelControl(DeltaSeconds,SkelComp);

}

void UUTSkelControl_JetThruster::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle* OwnerVehicle = Cast<AUTVehicle>(SkelComp->GetOwner());
	
	if ( !GIsEditor )
	{
		if (OwnerVehicle != NULL && SkelComp->LastRenderTime > OwnerVehicle->WorldInfo->TimeSeconds - 1.0f)
		{
			FLOAT NewDesiredStrength;

			if (OwnerVehicle->bDriving)
			{
				const FLOAT DP = OwnerVehicle->Velocity.SafeNormal() | OwnerVehicle->Rotation.Vector().SafeNormal();
				if ( DP > 0.0f )
				{
					const FLOAT Thrust = Clamp<FLOAT>(OwnerVehicle->Velocity.Size2D(), 0.0f, MaxForwardVelocity);
					NewDesiredStrength =  1 - (Thrust / MaxForwardVelocity);
				}
				else
				{
					NewDesiredStrength = 1.0f;
				}
			}
			else
			{
				NewDesiredStrength = 1.0;
			}

			if (NewDesiredStrength != DesiredStrength)
			{
				BlendTimeToGo += ( NewDesiredStrength - DesiredStrength ) * BlendRate;
				BlendTimeToGo = Clamp<FLOAT>(BlendTimeToGo, 0.0f, BlendRate);
				StrengthTarget = NewDesiredStrength;
			}
		}
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}
void UUTSkelControl_DarkWalkerHeadTurret::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	if ( FramePlayerName != NAME_None )
	{
		Player = Cast<UUTAnimNodeFramePlayer>( SkelComp->FindAnimNode(FramePlayerName) );
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);
	
	if ( Player )
	{
		const FLOAT MinPitch = FLOAT(MinAngle.PitchConstraint) * 182.0444;
		const FLOAT MaxPitch = FLOAT(MaxAngle.PitchConstraint) * 182.0444;

		FLOAT CurrPitch = BoneRotation.Pitch & 65535;

		if ( CurrPitch > 32767 )
		{
			CurrPitch -= 65535;
		}

		const FLOAT Perc = (CurrPitch - MinPitch) / (MaxPitch-MinPitch);
		Player->SetAnim(Player->AnimSeqName);
		Player->SetAnimPosition(Perc);
	}

}

void UUTSkelControl_Rotate::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	if ( DesiredBoneRotation.Pitch != BoneRotation.Pitch )
	{
		BoneRotation.Pitch = RotationalTurn(BoneRotation.Pitch, DesiredBoneRotation.Pitch, FLOAT(DesiredBoneRotationRate.Pitch) *  DeltaSeconds);
	}

	if ( DesiredBoneRotation.Yaw != BoneRotation.Yaw )
	{
		BoneRotation.Yaw = RotationalTurn(BoneRotation.Yaw, DesiredBoneRotation.Yaw, FLOAT(DesiredBoneRotationRate.Yaw) *  DeltaSeconds);
	}

	if ( DesiredBoneRotation.Roll != BoneRotation.Roll )
	{
		BoneRotation.Roll = RotationalTurn(BoneRotation.Roll, DesiredBoneRotation.Roll, FLOAT(DesiredBoneRotationRate.Roll) *  DeltaSeconds);
	}

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

void UUTSkelControl_TurretMultiBone::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	if ( bConstrainPitch )
	{
		// Calculate the Constraint Percentage.  We can assume we are within the constraint

		FLOAT CurPitch = ConstrainedBoneRotation.Pitch & 65535;
		if (CurPitch > 32767)
		{
			CurPitch  = CurPitch - 65535;
		}

		if (CurPitch > 0)
		{
			ConstraintPct = CurPitch / (MaxAngle.PitchConstraint * 182.0444);
		}
		else
		{
			ConstraintPct = -1.0f * (CurPitch / (MinAngle.PitchConstraint * 182.0444));
		}
	}
}


void UUTSkelControl_TurretMultiBone::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	// Cache all of the indexes so we don't have to look them up later

	if ( !bIndexCacheIsValid )
	{
		LeftTurretArmIndex = SkelComp->MatchRefBone( LeftTurretArmBone );
		RightTurretArmIndex = SkelComp->MatchRefBone( RightTurretArmBone );
		PivotUpIndex = SkelComp->MatchRefBone( PivotUpBone );
		bIndexCacheIsValid = TRUE;
	}

	// ConstrainedBoneRotation is in world space, so we need to convert it to local space
	BoneRotation = SkelComp->LocalToWorld.InverseTransformNormal(ConstrainedBoneRotation.Vector()).Rotation();

	if ( bConstrainPitch )
	{
		// If we are the Arms, we can adjust up to the constraint 

		if (BoneIndex == LeftTurretArmIndex || BoneIndex == RightTurretArmIndex )
		{
			if ( ConstraintPct > 0 && ConstraintPct > PivotUpPerc )
			{
				BoneRotation.Pitch = appTrunc((MaxAngle.PitchConstraint * 182.0444) * PivotUpPerc);
			}
			else if ( ConstraintPct < 0 && ConstraintPct < PivotDownPerc )
			{
				BoneRotation.Pitch = appTrunc((MinAngle.PitchConstraint * 182.0444) * Abs(PivotDownPerc));
			}
		}

		if ( BoneIndex == PivotUpIndex )
		{
			if ( ConstraintPct > 0 && ConstraintPct > PivotUpPerc )
			{
				BoneRotation.Pitch = appTrunc((MaxAngle.PitchConstraint * 182.0444) * (ConstraintPct - PivotUpPerc));
			}
			else if ( ConstraintPct > 0 )
			{
				BoneRotation.Pitch = 0;
			}

			if ( ConstraintPct < 0 && ConstraintPct < PivotDownPerc )
			{
				BoneRotation.Pitch = appTrunc((MinAngle.PitchConstraint * 182.0444) * (ConstraintPct - PivotDownPerc) * -1);
			}
			else if ( ConstraintPct < 0 )
			{
				BoneRotation.Pitch = 0;
			}
		}
	}

	// Skip the default TurretConstrained CalculateNewBoneTransforms

	USkelControlSingleBone::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}


UBOOL UUTSkelControl_LookAt::ApplyLookDirectionLimits(FVector& DesiredLookDir, const FVector &CurrentLookDir, INT BoneIndex, USkeletalMeshComponent* SkelComp)
{
	const FRotator BaseLookDirRot = CurrentLookDir.Rotation();
	FRotator DesiredLookDirRot = DesiredLookDir.Rotation();
	FRotator DeltaRot = (DesiredLookDirRot - BaseLookDirRot).Normalize();	

	//FRotator DeltaRotTest = (DesiredLookDir - CurrentLookDir).Rotation().Normalize();

	// clamp the delta
	if (bLimitPitch)
	{
		const INT PitchLimitUnr = appTrunc(Abs(PitchLimit) * (65536.f/360.f));
		DeltaRot.Pitch = Clamp(DeltaRot.Pitch, -PitchLimitUnr, PitchLimitUnr);
	}
	if (bLimitYaw)
	{
		const INT YawLimitUnr = appTrunc(Abs(YawLimit) * (65536.f/360.f));
		DeltaRot.Yaw = Clamp(DeltaRot.Yaw, -YawLimitUnr, YawLimitUnr);
	}
	if (bLimitRoll)
	{
		const INT RollLimitUnr = appTrunc(Abs(RollLimit) * (65536.f/360.f));
		DeltaRot.Roll = Clamp(DeltaRot.Roll, -RollLimitUnr, RollLimitUnr);
	}

	// re-apply the clamped delta
	//FVector DesiredLookDirTest = DeltaRot.Vector() + CurrentLookDir;
	DesiredLookDirRot = BaseLookDirRot + DeltaRot;
	DesiredLookDir = DesiredLookDirRot.Vector();

	// let super do any limiting it wants to as well
	return Super::ApplyLookDirectionLimits(DesiredLookDir, CurrentLookDir, BoneIndex, SkelComp);
}


void UUTSkelControl_LookAt::DrawSkelControl3D(const FSceneView* View, FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	if (bShowPerAxisLimits && (bLimitYaw || bLimitPitch || bLimitRoll) )
	{
		// Calculate transform for cone.
		FVector YAxis, ZAxis;
		LimitLookDir.FindBestAxisVectors(YAxis, ZAxis);
		const FVector	ConeOrigin		= SkelComp->SpaceBases(BoneIndex).GetOrigin();
		const FLOAT		MaxAngle1Radians = YawLimit * ((FLOAT)PI/180.f);
		const FLOAT		MaxAngle2Radians = PitchLimit * ((FLOAT)PI/180.f);
		const FMatrix	ConeToWorld		= FScaleMatrix(FVector(30.f)) * FMatrix(LimitLookDir, YAxis, ZAxis, ConeOrigin) * SkelComp->LocalToWorld;

		UMaterialInterface* LimitMaterial = LoadObject<UMaterialInterface>(NULL, TEXT("EditorMaterials.PhAT_JointLimitMaterial"), NULL, LOAD_None, NULL);

		DrawCone(PDI, ConeToWorld, MaxAngle1Radians, MaxAngle2Radians, 40, TRUE, FColor(64,255,64), LimitMaterial->GetRenderProxy(FALSE), SDPG_World);
	}

	Super::DrawSkelControl3D(View, PDI, SkelComp, BoneIndex);
}

void UUTSkelControl_LookAt::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	// UT version automatically does interpolation
	TargetLocation = VInterpTo(TargetLocation, DesiredTargetLocation, TargetLocationInterpSpeed, DeltaSeconds);

	Super::TickSkelControl(DeltaSeconds, SkelComp);
}


void UUTSkelControl_HoverboardSuspension::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(SkelComp->GetOwner());
	// Check we have a hoverboard and it has two valid wheels.
	if(!Hoverboard || Hoverboard->Wheels.Num() != 2 || !Hoverboard->Wheels(0) || !Hoverboard->Wheels(1))
	{
		return;
	}

	const FLOAT FrontWheelHeight = Hoverboard->Wheels(0)->SuspensionPosition;
	const FLOAT RearWheelHeight = Hoverboard->Wheels(1)->SuspensionPosition;
	const FLOAT AverageHeight = (FrontWheelHeight + RearWheelHeight) * 0.5f;
	BoneTranslation = FVector(0,0,1) * (TransOffset + Clamp<FLOAT>((AverageHeight - TransIgnore) * TransScale, 0.f, MaxTrans));

	const FLOAT HeightDiff = (FrontWheelHeight - RearWheelHeight);
	const FLOAT Rot = Clamp<FLOAT>(HeightDiff * RotScale * 2 * (FLOAT)PI, -MaxRot, MaxRot);
	BoneRotation  = FQuat( FVector(0,1,0), Rot);

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

void UUTSkelControl_HoverboardSwing::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(SkelComp->GetOwner());
	// Check we have a hoverboard and it has two valid wheels.
	if(!Hoverboard || !Hoverboard->CollisionComponent || !Hoverboard->CollisionComponent->BodyInstance)
	{
		return;
	}

	// Look at yaw speed and use that to adjust 'swing out' of the board.
	const FLOAT UseVel = Clamp(Hoverboard->ForwardVel, -MaxUseVel, MaxUseVel);
	const FVector AngVel = Hoverboard->CollisionComponent->BodyInstance->GetUnrealWorldAngularVelocity();
	const FLOAT TargetRot = Clamp<FLOAT>(UseVel * AngVel.Z * SwingScale * 2 * (FLOAT)PI, -MaxSwing, MaxSwing);
	const FLOAT MaxDeltaRot = DeltaSeconds * MaxSwingPerSec;
	const FLOAT DeltaRot = Clamp<FLOAT>(TargetRot - CurrentSwing, -MaxDeltaRot, MaxDeltaRot);
	CurrentSwing += DeltaRot;
}

void UUTSkelControl_HoverboardSwing::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	BoneRotation = FQuat( FVector(1,0,0), CurrentSwing );

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

void UUTSkelControl_HoverboardVibration::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(SkelComp->GetOwner());
	if(Hoverboard)
	{
		FLOAT Speed = Hoverboard->Velocity.Size();
		VibInput += (VibFrequency * Speed * DeltaSeconds * 2 * (FLOAT)PI);
	}
}

void UUTSkelControl_HoverboardVibration::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(SkelComp->GetOwner());
	// Check we have a hoverboard and it has two valid wheels.
	if(!Hoverboard || Hoverboard->Wheels.Num() != 2 || !Hoverboard->Wheels(0) || !Hoverboard->Wheels(1) 
		|| !Hoverboard->CollisionComponent || !Hoverboard->CollisionComponent->BodyInstance)
	{
		return;
	}

	// Don't vibrate board if wheels are not on the ground
	if(!Hoverboard->Wheels(0)->bWheelOnGround || !Hoverboard->Wheels(1)->bWheelOnGround)
	{
		return;
	}

	FVector AngVel = Hoverboard->CollisionComponent->BodyInstance->GetUnrealWorldAngularVelocity();
	FLOAT Speed = Hoverboard->Velocity.Size();
	FLOAT VibeMag = (Speed * VibSpeedAmpScale) +  (Speed * Abs<FLOAT>(AngVel.Z) * VibTurnAmpScale);
	VibeMag = Min<FLOAT>(VibeMag, VibMaxAmplitude);
	FLOAT Vibe = VibeMag * appSin( VibInput );

	BoneTranslation = FVector(0,0,Vibe);

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

void UUTSkelControl_SpinControl::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent *SkelComp)
{
	const FLOAT RotationAmount = DeltaSeconds*DegreesPerSecond*182.0444;
	if(!Axis.IsZero())
	{
		Axis.Normalize();
		BoneRotation.Add(appTrunc(Axis.Y*RotationAmount),appTrunc(Axis.Z*RotationAmount),appTrunc(Axis.X*RotationAmount));
	}
	Super::TickSkelControl(DeltaSeconds,SkelComp);
}
