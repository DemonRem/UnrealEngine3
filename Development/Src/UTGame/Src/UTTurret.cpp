/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UTGame.h"
#include "UTGameSequenceClasses.h"
#include "UTGameVehicleClasses.h"
#include "EngineInterpolationClasses.h"
#include "UnLinkedObjDrawUtils.h"

IMPLEMENT_CLASS(UUTSeqAct_TurretTrack);
IMPLEMENT_CLASS(UUTSeqEvent_TurretSpawn);
IMPLEMENT_CLASS(AUTVehicleFactory_TrackTurretBase);

void UUTSeqEvent_TurretSpawn::OnCreated()
{
	Super::OnCreated();

	// This code is borrowed from USequence.cpp - USeqEvent_Mover - Thanks Matt

	USequence* ParentSeq = CastChecked<USequence>(GetOuter());
	UUTSeqAct_TurretTrack* Track = ConstructObject<UUTSeqAct_TurretTrack>(UUTSeqAct_TurretTrack::StaticClass(), GetOuter(), NAME_None, RF_Transactional);
	Track->ParentSequence = ParentSeq;
	Track->ObjPosX = ObjPosX + 250;
	Track->ObjPosY = ObjPosY;
	ParentSeq->SequenceObjects.AddItem(Track);
	Track->OnCreated();
	Track->Modify();
	Track->bRewindOnPlay = false;
	if (Track->InputLinks.Num() != 1)
	{
		debugf(NAME_Warning, TEXT("Unable to auto-link to default Turret Track action because expected input link is missing"));
	}
	else
	{
		// link our "Spawned" connector to its "Spawned" connector
		INT OutputLinkIndex = OutputLinks(0).Links.Add();
		OutputLinks(0).Links(OutputLinkIndex).LinkedOp = Track;
		OutputLinks(0).Links(OutputLinkIndex).InputLinkIdx = 0;
		
		// if we have an Originator, create default interpolation data for it and link it up
		if (Originator != NULL)
		{
			UInterpData* InterpData = Track->FindInterpDataFromVariable();
			if (InterpData != NULL)
			{
				// create the new group
				UInterpGroup* NewGroup = ConstructObject<UInterpGroup>(UInterpGroup::StaticClass(), InterpData, NAME_None, RF_Transactional);
				NewGroup->GroupName = TurretGroupName;
				NewGroup->GroupColor = FColor::MakeRandomColor();
				NewGroup->Modify();
				InterpData->InterpGroups.AddItem(NewGroup);

				// Create the movement track

				UInterpTrackMove* NewMove = ConstructObject<UInterpTrackMove>(UInterpTrackMove::StaticClass(), NewGroup, NAME_None, RF_Transactional);
				NewMove->Modify();	
				NewGroup->InterpTracks.AddItem(NewMove);

				// tell the matinee action to update, which will create a new object variable connector for the group we created
				Track->UpdateConnectorsFromData();
				// create a new object variable
				USeqVar_Object* NewObjVar = ConstructObject<USeqVar_Object>(USeqVar_Object::StaticClass(), ParentSeq, NAME_None, RF_Transactional);
				NewObjVar->ObjPosX = Track->ObjPosX + 50 * Track->VariableLinks.Num();
				NewObjVar->ObjPosY = Track->ObjPosY + 200;
				NewObjVar->ObjValue = Originator;
				NewObjVar->Modify();
				ParentSeq->SequenceObjects.AddItem(NewObjVar);

				// hook up the new variable connector to the new variable
				INT NewLinkIndex = Track->FindConnectorIndex(TurretGroupName.ToString(), LOC_VARIABLE);
				checkSlow(NewLinkIndex != INDEX_NONE);
				Track->VariableLinks(NewLinkIndex).LinkedVariables.AddItem(NewObjVar);
			}
		}
	}
}

UBOOL UUTSeqEvent_TurretSpawn::RegisterEvent()
{
	UBOOL b = Super::RegisterEvent();
	eventTurretSpawned();
	return b;
}

// Returning true from here indicated we are done.
UBOOL UUTSeqAct_TurretTrack::UpdateOp(FLOAT deltaTime)
{
	if(bIsPlaying)
	{
		StepInterp(deltaTime, false);
	}
	return false;
}

void UUTSeqAct_TurretTrack::StepInterp(FLOAT DeltaTime, UBOOL bPreview)
{
	FLOAT TimeMod = 1.0;

	if (LatentActors.Num() > 0)
	{
		AUTVehicleFactory_TrackTurretBase* MyTurret;
		MyTurret = Cast<AUTVehicleFactory_TrackTurretBase>(LatentActors(0));
		if (MyTurret != NULL && MyTurret->TurretAccelRate > 0)
		{
			TimeMod = MyTurret->MovementModifier / MyTurret->TurretAccelRate;
			MyTurret->MovementModifier += DeltaTime;
		}
		TimeMod = Clamp<FLOAT>(TimeMod,0.f,1.f);
	}

	Super::StepInterp(DeltaTime * TimeMod, bPreview);
}

void UUTSeqAct_TurretTrack::Activated()
{
	USequenceOp::Activated();
	if( bIsPlaying )
	{
		// Don't think we should ever get here...
		return;
	}
		
	InitInterp();

	// For each Actor being interpolated- add us to its LatentActions list, and add it to our LatentActors list.
	// Similar to USequenceAction::Activated - but we don't call a handler function on the Object. Should we?
	TArray<UObject**> objectVars;
	GetObjectVars(objectVars);

	for(INT i=0; i<objectVars.Num(); i++)
	{
		AActor* Actor = Cast<AActor>( *(objectVars(i)) );
		if( Actor )
		{
			PreActorHandle(Actor);
			// if actor has already been ticked, reupdate physics with updated position from track.
			// Fixes Matinee viewing through a camera at old position for 1 frame.
			Actor->performPhysics(1.f);
			Actor->eventInterpolationStarted(this);

			AUTVehicleFactory_TrackTurretBase* Factory = Cast<AUTVehicleFactory_TrackTurretBase>(Actor);
			if (Factory != NULL)
			{
				UpdateInterp(Clamp(Factory->InitialPosition, 0.f, InterpData->InterpLength), FALSE, TRUE);
			}

		}
	}
	// if we are a server, create/update matinee actor for replication to clients
	if (GWorld->GetNetMode() == NM_DedicatedServer || GWorld->GetNetMode() == NM_ListenServer)
	{
		if (ReplicatedActor == NULL || ReplicatedActor->bDeleteMe)
		{
			ReplicatedActor = (AMatineeActor*)GWorld->SpawnActor(AMatineeActor::StaticClass());
			check(ReplicatedActor != NULL);
			ReplicatedActor->InterpAction = this;
		}
		if (ReplicatedActor != NULL)
		{
			ReplicatedActor->eventUpdate();
		}
	}
}


void UUTSeqAct_TurretTrack::DeActivated()
{
	// Remove this latent action from all actors it was working on, and empty our list of actors.
	for(INT i=0; i<LatentActors.Num(); i++)
	{
		AActor* Actor = LatentActors(i);
		if(Actor && !Actor->IsPendingKill())
		{
			Actor->LatentActions.RemoveItem(this);
			Actor->eventInterpolationFinished(this);
		}
	}
	if (ReplicatedActor != NULL)
	{
		ReplicatedActor->eventUpdate();
	}

	LatentActors.Empty();

	// Do any interpolation sequence cleanup-  destroy track/group instances etc.
	TermInterp();
}

void AUTVehicle_TrackTurretBase::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	AUTVehicleFactory_TrackTurretBase* const TurretFactory = Cast<AUTVehicleFactory_TrackTurretBase>(ParentFactory);
	if ( Health > 0 && TurretFactory )
	{
		// don't update turret rot if in fixed or free cameras
		AUTPawn* const DriverPawn = Cast<AUTPawn>(Driver);
		if ( DriverPawn && !DriverPawn->bFixedView )
		{
			AUTPlayerController* const PC = Cast<AUTPlayerController>(Controller);
			if (PC && !PC->bDebugFreeCam)
			{
				// Adjust the movement
				TurretFactory->AdjustMovement(Steering, Throttle);
			}
		}

		if ( TurretFactory->bInMotion )
		{
			if ( !bInMotion )
			{
				if (TurretMoveStart != NULL)
				{
					TurretMoveStart->Play();
				}
				if (TurretMoveLoop != NULL)
				{
					TurretMoveLoop->Play();
				}
				if (TurretMoveStop != NULL)
				{
					TurretMoveStop->Stop();
				}
				bInMotion = TRUE;
			}
		}
		else 
		{
			if ( bInMotion )
			{
				if (TurretMoveStart != NULL)
				{
					TurretMoveStart->Stop();
				}
				if (TurretMoveLoop != NULL)
				{
					TurretMoveLoop->Stop();
				}
				if (TurretMoveStop != NULL)
				{
					TurretMoveStop->Play();
				}
				bInMotion = FALSE;
			}
		}
	}
	else
	{
		if (TurretMoveStart != NULL)
		{
			TurretMoveStart->Stop();
		}
		if (TurretMoveLoop != NULL)
		{
			TurretMoveLoop->Stop();
		}
		if (TurretMoveStop != NULL)
		{
			TurretMoveStop->Stop();
		}
		bInMotion = FALSE;
	}
}
void AUTVehicle_TrackTurretBase::PostNetReceiveBase(AActor* NewBase)
{
	Super::PostNetReceiveBase(NewBase);

	if (NewBase != NULL)
	{
		RelativeLocation = FVector(0,0,0);
		RelativeRotation = FRotator(0,0,0);
		GWorld->FarMoveActor( this, Base->Location, 0, 1, 1 );
	}
}


void AUTVehicleFactory_TrackTurretBase::AdjustMovement(FLOAT Steering, FLOAT Throttle)
{
	UInterpTrackMove*		MoveTrack;
	UInterpTrackInstMove*	MoveInst;
	USeqAct_Interp*			Seq;

	ETurretMoveDir MoveDir;

	if (ChildVehicle && ChildVehicle->bDriving && FindInterpMoveTrack(&MoveTrack, &MoveInst, &Seq))
	{
		if (Steering != 0.f || Throttle != 0.f)		// We are trying to move somewhere
		{
			if (Abs(Steering - LastSteering) < KINDA_SMALL_NUMBER && Abs(Throttle - LastThrottle) < KINDA_SMALL_NUMBER)
			{
				// if the control inputs haven't changed, just keep going in the same direction
				MoveDir = ETurretMoveDir(LastMoveDir);
			}
			else
			{
				// Grab 2 reference points from the path, then figure out which we are heading towards

				FLOAT ForwardRefPos = Clamp<FLOAT>(Seq->Position + 0.15 , 0.f, Seq->InterpData->InterpLength);
				FLOAT BackwardRefPos = Clamp<FLOAT>(Seq->Position - 0.15 , 0.f, Seq->InterpData->InterpLength);

				FVector ForwardPos, BackwardPos;
				FRotator Rot;

				MoveTrack->GetLocationAtTime(MoveInst, ForwardRefPos, ForwardPos, Rot);
				MoveTrack->GetLocationAtTime(MoveInst, BackwardRefPos, BackwardPos, Rot);
			
				FVector Directional = ChildVehicle->WeaponRotation.Vector() ; 
				
				FLOAT ForwardDot = (ForwardPos-Location).SafeNormal() | Directional.SafeNormal();
				FLOAT BackwardDot = (BackwardPos-Location).SafeNormal() | Directional.SafeNormal();

				if ( Throttle != 0.f && ( ForwardDot > 0.0 || BackwardDot > 0.0 ) ) // Need Throttle
				{
					if (ForwardDot > 0.f)
					{
						MoveDir = Throttle > 0.f ? TMD_Forward : TMD_Reverse;
					}
					else
					{
						MoveDir = Throttle > 0.f ? TMD_Reverse : TMD_Forward;
					}
				}
				else	// Need Steering
				{
					FVector X,Y,Z;
					FRotationMatrix(ChildVehicle->WeaponRotation).GetAxes(X,Y,Z);
					FLOAT LeftDot = (ForwardPos-Location).SafeNormal() | Y.SafeNormal();

					if (LeftDot > 0.f)
					{
						MoveDir = Steering > 0 ? TMD_Reverse : TMD_Forward;
					}
					else
					{
						MoveDir = Steering > 0 ? TMD_Forward : TMD_Reverse;
					}
				}

				if ( LastMoveDir == TMD_Stop ) // We are starting up
				{
					Seq->Play();
					Seq->bReversePlayback = MoveDir == TMD_Reverse;
				}
				else if (LastMoveDir != MoveDir)
				{
					MovementModifier = 0.f;
					Seq->ChangeDirection();
				}
			}

			bInMotion = (Seq->Position > 0.f && Seq->Position < Seq->InterpData->InterpLength);
		}
		else
		{
			bInMotion = false;

			MoveDir = TMD_Stop;
			MovementModifier = 0.f;
			Seq->Stop();
		}

		if (Seq->ReplicatedActor != NULL)
		{
			Seq->ReplicatedActor->eventUpdate();
		}

		LastSteering = Steering;
		LastThrottle = Throttle;
		LastMoveDir = MoveDir;
	}
}

void AUTVehicleFactory_TrackTurretBase::TurretDeathReset()
{
	UInterpTrackMove*		MoveTrack;
	UInterpTrackInstMove*	MoveInst;
	USeqAct_Interp*			Seq;

	if (FindInterpMoveTrack(&MoveTrack, &MoveInst, &Seq))
	{
		if ( Seq->bIsPlaying )
		{
			Seq->Stop();
		}

		Seq->SetPosition(InitialPosition, TRUE);
		LastPosition = Seq->Position;

		if(Seq->ReplicatedActor != NULL)
		{
			Seq->ReplicatedActor->eventUpdate();
		}
	}

}
void AUTVehicleFactory_TrackTurretBase::ResetTurret()
{
	UInterpTrackMove*		MoveTrack;
	UInterpTrackInstMove*	MoveInst;
	USeqAct_Interp*			Seq;

	if (ChildVehicle != NULL && FindInterpMoveTrack(&MoveTrack, &MoveInst, &Seq))
	{
		if (!Seq->bIsPlaying)
		{
			Seq->Play();
			if (Seq->Position > InitialPosition)
			{
				Seq->ChangeDirection();
			}

			if (Seq->ReplicatedActor != NULL)
			{
				Seq->ReplicatedActor->eventUpdate();
			}
		}
	}
}

void AUTVehicleFactory_TrackTurretBase::ForceTurretStop()
{
	UInterpTrackMove*		MoveTrack;
	UInterpTrackInstMove*	MoveInst;
	USeqAct_Interp*			Seq;

	if (ChildVehicle && FindInterpMoveTrack(&MoveTrack, &MoveInst, &Seq))
	{
		if ( Seq->bIsPlaying )
		{
			Seq->Stop();
		}

		if (Seq->ReplicatedActor != NULL)
		{
			Seq->ReplicatedActor->eventUpdate();
		}


	}
}

/** used with turret movement code to only allow blocking checks for turret when doing our special collision check */
static UBOOL bAllowTurretBlocking = TRUE;

/**
 * Override PHYSInterpolating() and ignore matinee changes to the rotation of the turret
 */

void AUTVehicleFactory_TrackTurretBase::physInterpolating(FLOAT DeltaTime)
{
	UInterpTrackMove*		MoveTrack;
	UInterpTrackInstMove*	MoveInst;
	USeqAct_Interp*			Seq;

	//debugf(TEXT("AActor::physInterpolating %f - %s"), GWorld->GetTimeSeconds(), *GetFName());

	// If we have a movement track currently working on this Actor, update position to co-incide with it.
	if( FindInterpMoveTrack(&MoveTrack, &MoveInst, &Seq) )
	{
		if (Role == ROLE_Authority && ChildVehicle != NULL && !ChildVehicle->Driver && Seq->bIsPlaying &&
			   Seq->Position > InitialPosition - DeltaTime && Seq->Position < InitialPosition + DeltaTime )
		{
			TurretDeathReset();
		}

		// We found a movement track - so use it to update the current position.
		FVector		NewPos = Location;
		FRotator	NewRot = Rotation;

		MoveTrack->GetLocationAtTime(MoveInst, Seq->Position, NewPos, NewRot);

		if ( bIgnoreMatineeRotation )
		{
			NewRot = Rotation;
		}

		// cancel move if ChildVehicle would be blocked by anything
		// we do this and disallow turret blocking for MoveActor() so that:
		// -the movement can't ever fail and confuse the interpolation
		// -we can collide against vehicles without colliding against the world, even though both check bCollideWorld (because vehicles are encroachers - legacy engine thing that's scary to fix)
		UBOOL bMoveCancelled = FALSE;
		if (ChildVehicle != NULL && ChildVehicle->bCollideActors)
		{
			// figure out movement delta
			FVector Delta = NewPos - Location;
			if (!Delta.IsZero())
			{
				if (NewRot != Rotation)
				{
					FMatrix OldMatrix = FRotationMatrix(Rotation).Transpose();
					FMatrix NewMatrix = FRotationMatrix(NewRot);
					Delta += NewMatrix.TransformFVector(OldMatrix.TransformFVector(ChildVehicle->RelativeLocation)) - ChildVehicle->RelativeLocation;
				}
				// calculate traceflags
				DWORD TraceFlags = TRACE_Pawns | TRACE_Others | TRACE_Volumes;
				if (ChildVehicle->bCollideWorld)
				{
					TraceFlags |= TRACE_World;
				}
				if (ChildVehicle->bCollideComplex)
				{
					TraceFlags |= TRACE_ComplexCollision;
				}
				// calculate trace origin
				FVector ColCenter = (ChildVehicle->CollisionComponent->IsValidComponent()) ? ChildVehicle->CollisionComponent->Bounds.Origin : ChildVehicle->Location;
				// perform the trace
				FMemMark Mark(GMem);
				for (FCheckResult* Hit = GWorld->MultiLineCheck(GMem, ColCenter + Delta, ColCenter, ChildVehicle->GetCylinderExtent(), TraceFlags, ChildVehicle); Hit != NULL; Hit = Hit->GetNext())
				{
					if ( Hit->Actor != NULL && !ChildVehicle->IsBasedOn(Hit->Actor) && !Hit->Actor->IsBasedOn(ChildVehicle) &&
						(Hit->Actor->IsA(AVehicle::StaticClass()) || ChildVehicle->IsBlockedBy(Hit->Actor, Hit->Component)) )
					{
						bMoveCancelled = TRUE;
						break;
					}
				}
				Mark.Pop();
			}
		}
		if (bMoveCancelled)
		{
			// revert matinee position to last successful position
			Seq->UpdateInterp(LastPosition, FALSE, FALSE);
			if (Seq->ReplicatedActor != NULL)
			{
				Seq->ReplicatedActor->eventUpdate();
			}
		}
		else
		{
			FVector OldLocation = Location;

			// set velocity for our desired move (so any events that get triggered, such as encroachment, know where we were trying to go)
			Velocity = (NewPos - Location) / DeltaTime;

			// turn off turret blocking for movement as we already did our collision test
			bAllowTurretBlocking = FALSE;
			FCheckResult Hit(1.f);
			GWorld->MoveActor(this, NewPos - Location, NewRot, MOVE_NoFail, Hit);
			bAllowTurretBlocking = TRUE;

			// set velocity for the movement that actually happened
			Velocity = (Location - OldLocation) / DeltaTime;

			//debugf(TEXT("FindInterpMoveTrack %s DesiredDist: %f ActualDist: %f"), *Seq->GetFName(), (NewPos-OldLocation).Size(), (OldLocation-Location).Size());

			// If based on something - update the RelativeLocation and RelativeRotation so that its still correct with the new position.
			AActor* BaseActor = GetBase();
			if (BaseActor)
			{
				FRotationTranslationMatrix BaseTM(BaseActor->Rotation,BaseActor->Location);
				FMatrix InvBaseTM = BaseTM.Inverse();
				FRotationTranslationMatrix ActorTM(Rotation,Location);

				FMatrix RelTM = ActorTM * InvBaseTM;
				RelativeLocation = RelTM.GetOrigin();
				RelativeRotation = RelTM.Rotator();
			}

			LastPosition = Seq->Position;
		}
	}
	else
	{
		Velocity = FVector(0.f);
	}
}

UBOOL AUTVehicle_TrackTurretBase::IgnoreBlockingBy(const AActor* Other) const
{
	return (bAllowTurretBlocking ? Super::IgnoreBlockingBy(Other) : TRUE);
}
