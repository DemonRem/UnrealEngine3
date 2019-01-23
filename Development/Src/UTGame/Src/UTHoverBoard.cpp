/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"
#include "UTGameAIClasses.h"

IMPLEMENT_CLASS(AUTVehicle_Hoverboard);
IMPLEMENT_CLASS(UUTVehicleSimHoverboard);


// HACK TJAMES HACK (For Gamers Day), we should get a real RPM system
float UUTVehicleSimHoverboard::GetEngineOutput(ASVehicle* Vehicle)
{
	return ((Vehicle->Velocity.Size())/(Vehicle->MaxSpeed))*19000;
}
void UUTVehicleSimHoverboard::ProcessCarInput(ASVehicle* Vehicle)
{
	if( Vehicle->Driver == NULL )
	{
		Vehicle->OutputBrake = 1.0f;
		Vehicle->OutputGas = 0.0f;
		Vehicle->bOutputHandbrake = FALSE;
	}
	else
	{
		Vehicle->OutputGas = Vehicle->Throttle;
		Vehicle->OutputSteering = Vehicle->Steering;
		Vehicle->OutputRise = Vehicle->Rise;

		// Keep awake physics of any driven vehicle.
		check(Vehicle->CollisionComponent);
		Vehicle->CollisionComponent->WakeRigidBody();
	}

	if ( Vehicle->Controller)
	{
		if ( Vehicle->IsHumanControlled() )
		{			
			Vehicle->DriverViewPitch = Vehicle->Controller->Rotation.Pitch;
			Vehicle->DriverViewYaw = Vehicle->Controller->Rotation.Yaw;

			// If not crouching, left stick can also steer.
			if(GWorld->GetWorldInfo()->bUseConsoleInput && Vehicle->OutputRise >= 0.f)
			{				
				// Also don't do this if in the air doing a trick jump.
				AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(Vehicle);
				APlayerController* PC = Cast<APlayerController>(Vehicle->Controller);
				if(PC && PC->PlayerInput && Hoverboard && !Hoverboard->bTrickJumping)
				{
					PC->PlayerInput->aTurn = Clamp<FLOAT>(PC->PlayerInput->aTurn - Vehicle->OutputSteering, -1.f, 1.f);
				}
			}
		}
		else
		{
			FRotator ViewRot = (Vehicle->Controller->FocalPoint - Vehicle->Location).Rotation();
			Vehicle->DriverViewPitch = ViewRot.Pitch;
			Vehicle->DriverViewYaw = ViewRot.Yaw;
		}
	}
	else
	{
		Vehicle->DriverViewPitch = Vehicle->Rotation.Pitch;
		Vehicle->DriverViewYaw = Vehicle->Rotation.Yaw;
	}
}


#if WITH_NOVODEX
void AUTVehicle_Hoverboard::PostInitRigidBody(NxActor* nActor, NxActorDesc& ActorDesc, UPrimitiveComponent* PrimComp)
{
	Super::PostInitRigidBody(nActor, ActorDesc, PrimComp);

	check(LeanUprightConstraintInstance);

	LeanUprightConstraintSetup->PriAxis1 = FVector(0,0,1);
	LeanUprightConstraintSetup->SecAxis1 = FVector(0,1,0);

	LeanUprightConstraintSetup->PriAxis2 = FVector(0,0,1);
	LeanUprightConstraintSetup->SecAxis2 = FVector(0,1,0);

	LeanUprightConstraintInstance->AngularDriveSpring = LeanUprightStiffness;
	LeanUprightConstraintInstance->AngularDriveDamping = LeanUprightDamping;

	LeanUprightConstraintInstance->InitConstraint(NULL, this->CollisionComponent, LeanUprightConstraintSetup, 1.0f, this, NULL, FALSE);
}
#endif // WITH_NOVODEX

// Similiar to SimChopper only without mouse steering
void UUTVehicleSimHoverboard::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
	if (!Vehicle->bDriving && Vehicle->bChassisTouchingGround)
		return;

	AUTVehicle_Hoverboard* Hoverboard = Cast<AUTVehicle_Hoverboard>(Vehicle);
	if (!Hoverboard)
		return;

	FLOAT OutputSteer = Vehicle->OutputSteering;
	FLOAT OutputThrust = Vehicle->OutputGas;
	FLOAT OutputRise = Vehicle->OutputRise;

	// Zero force/torque accumulation.
	FVector Up(0.0f, 0.0f, 1.0f);
	FVector Force(0.0f, 0.0f, 0.0f);
	FVector Torque(0.0f, 0.0f, 0.0f);

	if( Vehicle->bDriving )
	{
		// Calc up (z), right(y) and forward (x) vectors
		FRotationMatrix R(Vehicle->Rotation);
		FVector DirX = R.GetAxis(0);
		FVector DirY = R.GetAxis(1);
		FVector DirZ = R.GetAxis(2);

		// 'World plane' forward & right vectors ie. no z component.
		FVector Forward = DirX;
		Forward.Normalize();

		FVector Right = DirY;
		Right.Normalize();

		FVector LocalUp = DirZ;
		LocalUp.Normalize();

		// Get body angular velocity
		FRigidBodyState rbState;
		Vehicle->GetCurrentRBState(rbState);
		FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
		FLOAT TurnAngVel = AngVel | Up;
		FLOAT RollAngVel = AngVel | DirX;
		FLOAT PitchAngVel = AngVel | DirY;

		FLOAT ForwardVelMag = Vehicle->Velocity | Forward;
		FLOAT RightVelMag = Vehicle->Velocity | Right;
		FLOAT UpVelMag = Vehicle->Velocity | Up;

		if( Hoverboard->bInTow && Hoverboard->TowInfo.TowTruck )
		{
			FMatrix AttachMatrix = Hoverboard->TowInfo.TowTruck->LocalToWorld();
			AttachMatrix.RemoveScaling();
			Hoverboard->DistanceJointInstance->MoveKinActorTransform(AttachMatrix);
		}

#if 0
		// Convert position 2 into world space to calc current distance
		FMatrix a2TM = FindBodyMatrix(Hoverboard, NAME_None);
		const FVector Pos2 = a2TM.TransformFVector(Hoverboard->TowLocalAttachPos);
		DrawWireStar(GWorld->LineBatcher, Pos2, 10.f, FColor(0,255,255), SDPG_World);
#endif

		if ( Hoverboard->bInTow && Abs(OutputThrust) > 0.2f )
		{
			if(OutputThrust > 0.f)
			{
				Hoverboard->CurrentTowDistance -= (Hoverboard->TowDistanceChangeSpeed * DeltaTime);
			}
			else
			{
				Hoverboard->CurrentTowDistance += (Hoverboard->TowDistanceChangeSpeed * DeltaTime);
			}

			// Ensure rope is not too long or negative
			Hoverboard->CurrentTowDistance = ::Clamp(Hoverboard->CurrentTowDistance, 0.f, Hoverboard->MaxTowDistance);
			Hoverboard->DistanceJointInstance->SetLinearLimitSize(Hoverboard->CurrentTowDistance);
		}


		FCheckResult Hit(1.0f);
		UBOOL bHit = !GWorld->SingleLineCheck(Hit, Vehicle, Vehicle->Location - FVector(0.0f, 0.0f, Hoverboard->WaterCheckLevel), Vehicle->Location, TRACE_AllBlocking | TRACE_SingleResult);
		UBOOL bIsOverDeepWater = Vehicle->bVehicleOnWater && (Hit.Time == 1.f);

		// Save the normal of the surface we are over.
		Hoverboard->GroundNormal = FVector(0,0,1);
		if(bHit)
		{
			Hoverboard->GroundNormal = Hit.Normal;
		}

		// Thrust and Strafe
		Vehicle->bOutputHandbrake = false;
		if ( (Vehicle->HasWheelsOnGround() || Hoverboard->bInAJump ) && !bIsOverDeepWater )
		{
			FVector UseForward = Forward;
			// When trick jumping - keep thrusting in the direction
			if(Hoverboard->bTrickJumping)
			{
				FRotator TakeoffRot = FRotator(0, appTrunc(Hoverboard->TakeoffYaw), 0);
				UseForward = TakeoffRot.Vector();
			}

			if ( !Hoverboard->bInTow && Vehicle->Driver && (Hit.Normal.Z > Vehicle->WalkableFloorZ) )
			{
				if ( (OutputThrust < 0.f) && ((Vehicle->Velocity | UseForward) < 0.f) )
				{
					if ( (Vehicle->Velocity | Forward) > -1.f * MaxReverseVelocity ) 
					{
						Force += OutputThrust * MaxReverseForce * UseForward;
					}
				}
				else
				{
					// When holding crouch to warm up for a jump, or actually executing that jump, keep thrusting.
					if ( Hoverboard->TrickJumpWarmup > 0.f || Hoverboard->bTrickJumping )
					{
						OutputThrust = 1.f;
					}
					Force += OutputThrust * MaxThrustForce * UseForward;
				}
			}
			if ( OutputSteer != 0.f && Vehicle->Rise >= 0.0f )
			{
				//Force -= OutputSteer * 2.f * ::Max(OutputThrust * MaxThrustForce, MaxStrafeForce) * Right;
				Vehicle->bOutputHandbrake = TRUE;
			}
		}

		// Linear Force Damping
		if ( Hoverboard->bInTow ) 
		{
			// Don't damp velocity when being towed
			Vehicle->bOutputHandbrake = TRUE;
		}
		else
		{
			// Damp velocity on water regardless of throttle
			if (bIsOverDeepWater)
			{
				Force -= LongDamping * ForwardVelMag * Forward;
				Force -= LatDamping * RightVelMag * Right;
			}
			else
			{
				Force -= (1.0f - Abs(OutputThrust)) * LongDamping * ForwardVelMag * Forward;
				//Force -= (1.0f - Abs(OutputSteer)) * LatDamping * RightVelMag * Right;
			}
		}

		// Apply downforce when going down hills.
		FVector ForwardInZPlane = DirX;
		ForwardInZPlane.Z = 0.f;
		ForwardInZPlane.Normalize();

		FLOAT DownhillFactor = Hoverboard->GroundNormal | ForwardInZPlane;
		if(DownhillFactor > 0.f)
		{
			Force += FVector(0,0,-DownhillFactor * Hoverboard->DownhillDownForce);
		}

		//// STEERING ////

		// When in air and spinning - update the yaw as we hold the stick.
		FLOAT SpinAmount = OutputSteer;
		if(Abs(Hoverboard->SpinHeadingOffset) < 2*(FLOAT)PI)
		{
			SpinAmount = Hoverboard->AutoSpin;
		}

		if ( Hoverboard->bDriving && Hoverboard->bInAJump && Hoverboard->bTrickJumping )
		{
			Hoverboard->SpinHeadingOffset += (Hoverboard->SpinSpeed * -SpinAmount * DeltaTime);
		}

		// When not jumping, holidng alt-fire lets you look around without steering.
		UBOOL bLookAroundMode = (Hoverboard->bGrab2 && !Hoverboard->bInAJump);
		if (!bLookAroundMode)
		{
			FVector LookDir(0,0,0);
			FLOAT SteerOffset = 0.f;
			FLOAT TurnDampingScale = 1.f;
			FLOAT TurnTorqueScale = 1.f;
			if(Hoverboard->bInTow && Hoverboard->TowInfo.TowTruck)
			{
				LookDir = (Hoverboard->TowInfo.TowTruck->Location - Hoverboard->Location).SafeNormal();
				SteerOffset = -OutputSteer * Hoverboard->HoverboardTowSteerMaxAngle * (PI/180.f);
			}
			else if(Hoverboard->bTrickJumping && Hoverboard->bInAJump && Hoverboard->bLeftGround)
			{
				// Steer towards takeoff yaw plus whatever spin offset we have built up.
				FRotator LookRot = FRotator(Vehicle->DriverViewPitch, appTrunc(Hoverboard->TakeoffYaw), 0);
				LookDir = LookRot.Vector();
				SteerOffset = Hoverboard->SpinHeadingOffset;

				// While actively trying to spin - reduce damping and increase torque
				if(Abs(SpinAmount) > 0.f)
				{
					TurnDampingScale = (1.f - Abs(SpinAmount));
					TurnTorqueScale = SpinTurnTorqueScale;
				}
			}
			else
			{
				FRotator LookRot = FRotator(Vehicle->DriverViewPitch, Vehicle->DriverViewYaw, 0);
				LookDir = LookRot.Vector();
			}

			// Project Look dir into z-plane
			FVector PlaneLookDir = LookDir;
			PlaneLookDir.Z = 0.0f;
			PlaneLookDir = PlaneLookDir.SafeNormal();

			FLOAT CurrentHeading = HeadingAngle(Forward);
			FLOAT DesiredHeading = HeadingAngle(PlaneLookDir) + SteerOffset;

			// Debug drawing for spin direction.
			if(FALSE)
			{
				FQuat Q(FVector(0,0,1), DesiredHeading);
				FVector V = Q.RotateVector(FVector(100,0,0));
				GWorld->LineBatcher->DrawLine(Hoverboard->Location, Hoverboard->Location + V, FColor(255,0,0), SDPG_World);
			}

			if ( !bHeadingInitialized )
			{
				TargetHeading = CurrentHeading;
				bHeadingInitialized = true;
			}	

			// Calc and store angle between 'looking' and 'pointing'
			Hoverboard->CurrentLookYaw = FindDeltaAngle(CurrentHeading, DesiredHeading);

			// Move 'target heading' towards 'desired heading' as fast as MaxYawRate allows.
			FLOAT DeltaTargetHeading = FindDeltaAngle(TargetHeading, DesiredHeading);
			TargetHeading = UnwindHeading(TargetHeading + DeltaTargetHeading);

			// Then put a 'spring' on the copter to target heading.
			FLOAT DeltaHeading = FindDeltaAngle(CurrentHeading, TargetHeading);
			FLOAT TurnTorqueMag = (DeltaHeading / PI) * TurnTorqueFactor * TurnTorqueScale;

			TurnTorqueMag = Clamp<FLOAT>( TurnTorqueMag, -MaxTurnTorque, MaxTurnTorque );
			Torque += TurnTorqueMag * LocalUp;

			Torque -= TurnAngVel * TurnDampingSpeedFunc.Eval(Abs(ForwardVelMag), 0.f) * LocalUp * TurnDampingScale;
		}

		// velocity damping to limit airspeed
		// Turn off damping when towing behind a moving vehicle.
		UBOOL bApplyDamping = TRUE;
		if( Hoverboard->bInTow && 
			Hoverboard->TowInfo.TowTruck && 
			Hoverboard->TowInfo.TowTruck->Velocity.Size() > 10.f )
		{
			bApplyDamping = FALSE;
		}

		if ( bApplyDamping )
		{
			Force -= Vehicle->GetDampingForce(Force);
		}
	}

#if WITH_NOVODEX // Update WheelShape to allow for suspension changes
	for (INT i = 0; i < Vehicle->Wheels.Num(); i++)
	{
		NxWheelShape* WheelShape = Vehicle->Wheels(i)->GetNxWheelShape();
		check(WheelShape);	

		SetNxWheelShapeParams(WheelShape, Vehicle->Wheels(i));
	}
#endif // WITH_NOVODEX

	// Apply force/torque to body.
	Vehicle->AddForce( Force );
	Vehicle->AddTorque( Torque );
}

void AUTVehicle_Hoverboard::AttachTowCable()
{
	check(DistanceJointInstance);
	
	if(!TowInfo.TowTruck)
	{
		return;
	}

	FVector TowTruckAttachWPos = TowInfo.TowTruck->Location;
	if( !TowInfo.TowTruck->Mesh->GetSocketWorldLocationAndRotation(TowInfo.TowAttachPoint, TowTruckAttachWPos, NULL) )
	{
		debugf(TEXT("AttachTowCable - Failed To Find Socket: %s"), *TowInfo.TowAttachPoint.ToString());
	}

	// Body 1 matrix 
	FVector wPos1 = TowTruckAttachWPos * U2PScale;
	FMatrix a1TM = FindBodyMatrix(TowInfo.TowTruck, NAME_None);
	a1TM.ScaleTranslation(FVector(U2PScale,U2PScale,U2PScale));
	FMatrix a1TMInv = a1TM.Inverse();

	DistanceJointSetup->Pos1 = a1TMInv.TransformFVector(wPos1);
	DistanceJointSetup->PriAxis1 = FVector(1,0,0);
	DistanceJointSetup->SecAxis1 = FVector(0,1,0);

	// Body 2 matrix is attachment point relative to hoverboard
	DistanceJointSetup->Pos2 = TowLocalAttachPos * U2PScale;
	DistanceJointSetup->PriAxis2 = FVector(1,0,0);
	DistanceJointSetup->SecAxis2 = FVector(0,1,0);

	// Convert position 2 into world space to calc current distance
	FMatrix a2TM = FindBodyMatrix(this, NAME_None);
	a2TM.ScaleTranslation(FVector(U2PScale,U2PScale,U2PScale));
	const FVector wPos2 = a2TM.TransformFVector(DistanceJointSetup->Pos2);
	CurrentTowDistance = (wPos1 - wPos2).Size() * P2UScale;

	DistanceJointSetup->LinearXSetup.LimitSize = CurrentTowDistance;
	DistanceJointSetup->LinearYSetup.LimitSize = CurrentTowDistance;
	DistanceJointSetup->LinearZSetup.LimitSize = CurrentTowDistance;

	DistanceJointSetup->bLinearLimitSoft = TRUE;
	DistanceJointSetup->LinearLimitStiffness = 30.0f;
	DistanceJointSetup->LinearLimitDamping = 6.0f;

	DistanceJointInstance->InitConstraint(TowInfo.TowTruck->CollisionComponent, this->CollisionComponent, DistanceJointSetup, 1.0f, this, NULL, TRUE);

	// Reset tow-blocked counter
	TowLineBlockedFor = 0.f;
}

void AUTVehicle_Hoverboard::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( bDeleteMe || bDeadVehicle )
		return;

	// Blend PhysicsWeight smoothly over time.
	if( PhysWeightBlendTimeToGo != 0.f && Driver && Driver->Mesh )
	{
		// Amount we want to change PhysicsWeight by.
		const FLOAT BlendDelta = TargetPhysicsWeight - Driver->Mesh->PhysicsWeight; 

		if( Abs(BlendDelta) > KINDA_SMALL_NUMBER && PhysWeightBlendTimeToGo > DeltaSeconds )
		{
			Driver->Mesh->PhysicsWeight	+= (BlendDelta / PhysWeightBlendTimeToGo) * DeltaSeconds;
			PhysWeightBlendTimeToGo	-= DeltaSeconds;
		}
		else
		{
			Driver->Mesh->PhysicsWeight	= TargetPhysicsWeight;
			PhysWeightBlendTimeToGo	= 0.f;
		}
	}

	LandedCountdown -= DeltaSeconds;

	// Handle landing and taking off.
	if (bInAJump || bTrickJumping)
	{
		// Note when we first leave the ground.
		if(!bLeftGround && !bVehicleOnGround)
		{
			bLeftGround = TRUE;
		}

		// If we were off the ground, and have landed (or we never left the ground) for a second.
		if( (bLeftGround || GWorld->GetTimeSeconds() > (LastJumpTime + 1.f)) && bVehicleOnGround )
		{
			bInAJump = FALSE;
			eventHoverboardLanded();

			// This is done on the server and replicated to clients.
			if(Role == ROLE_Authority)
			{
				bTrickJumping = FALSE;
			}

			bLeftGround = FALSE;
			eventToggleAnimBoard(FALSE, 0.f);
			LandedCountdown = 1.f;	
			AutoSpin = 0.f;
		}
		else if(bTrickJumping)
		{
			if((bGrab1 || bGrab2) && !bGrabbingBoard)
			{
				eventToggleAnimBoard(TRUE, 0.f);
			}
			else if(!(bGrab1 || bGrab2) && bGrabbingBoard)
			{
				if(IsLocallyControlled() && IsHumanControlled())
				{
					TargetPhysicsWeight = 1.0;
					PhysWeightBlendTimeToGo = 0.1;
				}

				eventToggleAnimBoard(FALSE, 0.1f);
			}
		}
	}

	// See if we are winding up for a jump
	UBOOL bWindingUp = (OutputRise < 0.0f);

	// Don't allow you to wind up for a jump while in the air
	if(bWindingUp && !bInAJump)
	{
		TrickJumpWarmup = ::Min<FLOAT>(TrickJumpWarmup + DeltaSeconds, TrickJumpWarmupMax);

		if(Abs(Steering) > 0.2f)
		{
			TrickSpinWarmup += DeltaSeconds;
			if(Steering > 0.f)
			{
				AutoSpin = 1.f;
			}
			else
			{
				AutoSpin = -1.f;
			}
		}
		else
		{
			TrickSpinWarmup = 0.f;
			AutoSpin = 0.f;
		}
	}

	// Calculate Local Up Vector
	FRotationMatrix R(Rotation);
	FVector DirZ = R.GetAxis(2);
	FVector DirX = R.GetAxis(0);

	FVector LocalUp = DirZ;
	LocalUp.Normalize();

	if ( Driver )
	{
		if (bDisableRepulsorsAtMaxFallSpeed)
		{
			URB_BodyInstance* BodyInstance = (Mesh != NULL) ? Mesh->GetRootBodyInstance() : NULL;
			UBOOL bEnableCollision = (Velocity.Z > -Driver->MaxFallSpeed && (BodyInstance == NULL || BodyInstance->PreviousVelocity.Z > -Driver->MaxFallSpeed));
			for (INT i = 0; i < Wheels.Num(); i++)
			{
				SetWheelCollision(i, bEnableCollision);
			}
		}

		// Show dust when on ground.
		UBOOL bHit = FALSE;
		FCheckResult Hit(1.f);
		FMatrix LToW = LocalToWorld();
		FVector UpVec = LToW.GetAxis(2);

		if(bVehicleOnGround)
		{
			bHit = !GWorld->SingleLineCheck(Hit, this, Location - (100.f * UpVec), Location, TRACE_World);
		}

		// If we are on the ground, and the line check hit something, display dust and move emitter to hit location.
		if(bHit)
		{
			if(HoverboardDust->bSuppressSpawning || !HoverboardDust->bIsActive)
			{
				HoverboardDust->ActivateSystem();
			}


			FVector ZAxis = -Hit.Normal;
			FVector XAxis, YAxis;
			ZAxis.FindBestAxisVectors( YAxis, XAxis );
			FMatrix ParticleTM( XAxis, YAxis, ZAxis, Hit.Location );

			HoverboardDust->Translation = ParticleTM.GetOrigin();
			HoverboardDust->Rotation = ParticleTM.Rotator();
			HoverboardDust->BeginDeferredUpdateTransform();

			HoverboardDust->SetFloatParameter(FName(TEXT("BoardVelMag")), Velocity.Size());
			HoverboardDust->SetFloatParameter(FName(TEXT("BoardHeight")), (Hit.Location - Location).Size());
			HoverboardDust->SetVectorParameter(FName(TEXT("BoardVel")), Velocity);
		}
		// Hide if not needed
		else
		{
			if(!HoverboardDust->bSuppressSpawning)
			{
				HoverboardDust->DeactivateSystem();
			}
		}

		FRigidBodyState rbState;
		GetCurrentRBState(rbState);
		FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
		FLOAT TurnAngVel = AngVel | FVector(0.0f, 0.0f, 1.0f);

		// Constrain the hoverboard pitch and roll to a dynamically generated orientation
		FRotator LeanRot(0,0,0);

		// YAW
		LeanRot.Yaw = Rotation.Yaw;

		// PITCH

		// Project forward vector into Z plane and normalize.
		FVector ForwardInZPlane = DirX;
		ForwardInZPlane.Z = 0.f;
		ForwardInZPlane.Normalize();

		FLOAT PitchAngle = -1.f * appAsin(GroundNormal | ForwardInZPlane);
		FLOAT NewTargetPitch = Rad2U * PitchAngle; 
		FLOAT MaxPitchChange = (MaxLeanPitchSpeed * DeltaSeconds);
		FLOAT DeltaPitch = Clamp<FLOAT>(NewTargetPitch - TargetPitch, -MaxPitchChange, MaxPitchChange);
		TargetPitch += DeltaPitch;
		LeanRot.Pitch = appTrunc(TargetPitch);

		// Add torque to lean as we turn.
		if(!bInAJump)
		{
			AddTorque( -1.f * DirX * ForwardVel * TurnAngVel * TurnLeanFactor );
		}

		FRotationMatrix RM(LeanRot);
		FVector LeanY = RM.GetAxis(1);
		FVector LeanZ = RM.GetAxis(2);

		//GWorld->LineBatcher->DrawLine(Location, Location + 50*LeanY, FColor(255,0,0), SDPG_World);
		//GWorld->LineBatcher->DrawLine(Location, Location + 50*LeanZ, FColor(0,255,0), SDPG_World);

		LeanUprightConstraintInstance->TermConstraint(NULL, FALSE);

		LeanUprightConstraintSetup->PriAxis1 = LeanZ;
		LeanUprightConstraintSetup->SecAxis1 = LeanY;

		LeanUprightConstraintSetup->PriAxis2 = FVector(0,0,1);
		LeanUprightConstraintSetup->SecAxis2 = FVector(0,1,0);

		LeanUprightConstraintInstance->AngularDriveSpring = LeanUprightStiffness;
		LeanUprightConstraintInstance->AngularDriveDamping = LeanUprightDamping;

		LeanUprightConstraintInstance->InitConstraint(NULL, this->CollisionComponent, LeanUprightConstraintSetup, 1.0f, this, NULL, FALSE);

		// Carving sound - DAVE-TODO: Un-hack this when final carving system is in-place (TJAMES - I'm taking this over.)
		if (!CurveSound->bWasPlaying)
			CurveSound->Play();	
		CurveSound->VolumeMultiplier = Abs(TurnAngVel) / 8192.0f;

		if (Controller != NULL)
		{
			APlayerController *PC = Controller->GetAPlayerController();

			if ( PC )
			{
				if ( HasWheelsOnGround() && (DoubleClickMove != DCLICK_None) )
				{
					eventRequestDodge();
					DoubleClickMove = DCLICK_None;
				}
			}
			else if ( !bInTow && HasWheelsOnGround() &&
				Velocity.SizeSquared() < Square(Driver->GroundSpeed) && FRotationMatrix(Rotation).GetAxis(2).Z < WalkableFloorZ )
			{
				// AI bails if moving slowly and ground is too steep
				eventContinueOnFoot();
				if (bDeleteMe)
				{
					return;
				}
			}

			// First see if we want to try and jump
			UBOOL bJumpNow = (OutputRise > 0.f || (OutputRise == 0.0f && TrickJumpWarmup > 0.0f) || bIsDodging);

			// Disallow if already doing one
			if(bJumpNow && bInAJump)
			{
				bJumpNow = FALSE;
			}

			// Disallow if it hasn't been long enough since last jump
			if(bJumpNow && (WorldInfo->TimeSeconds < LastJumpTime + JumpDelay))
			{
				bJumpNow = FALSE;
			}

			// Disallow if not over suitable surface
			if(bJumpNow)
			{
				// Don't jump if we are at a steep angle
				UBOOL bIsOverJumpableSurface = FALSE;
				if ((LocalUp | FVector(0.0f,0.0f,1.0f)) > 0.5f)
				{
					// Otherwise make sure there is ground underneath
					FCheckResult Hit2(1.0f);
					bIsOverJumpableSurface = !GWorld->SingleLineCheck(Hit2, this, Location - (LocalUp * JumpCheckTraceDist), Location, TRACE_AllBlocking);
				}

				if(!bIsOverJumpableSurface)
				{
					bJumpNow = FALSE;
				}
			}

			// Ok - jumping now!
			if ( bJumpNow )
			{	
				if ( Role == ROLE_Authority )
				{
					bDoHoverboardJump = !bDoHoverboardJump;
				}
				eventBoardJumpEffect();

				if ( Cast<AUTBot>(Controller) )
				{
					Rise = 0.f;
				}

				bInAJump = TRUE;
				bLeftGround = FALSE;

				// Remember the yaw we have when we take off.
				TakeoffYaw = DriverViewYaw;

				// If trick jumping
				if (TrickJumpWarmup > 0.0f && !bIsDodging)
				{
					bTrickJumping = TRUE;
					SpinHeadingOffset = 0.f;
				}

				AddImpulse( FVector(0.f, 0.f, JumpForceMag) + DodgeForce );
				DodgeForce = FVector(0.f, 0.f, 0.f);
				LastJumpTime = WorldInfo->TimeSeconds;
			}

			bIsDodging = FALSE;
			bNoZDamping = (WorldInfo->TimeSeconds - 0.25f < LastJumpTime);
		}

		if(OutputRise == 0.f)
		{
			TrickJumpWarmup = 0.f;
			TrickSpinWarmup = 0.f;
		}
	}

	// See if there is a line of sight between hoverboard and tow truck
	if (Role == ROLE_Authority && bInTow)
	{
		if (TowInfo.TowTruck && Driver) 
		{
			if (TowInfo.TowTruck->bDeleteMe || TowInfo.TowTruck->Health <= 0)
			{
				// break tow link if linked vehicle is dead
				eventBreakTowLink();
			}
			else
			{
				// if it is obstructed - start counting how long its for
				FCheckResult Hit(1.f);
				if (!GWorld->SingleLineCheck(Hit, this, TowInfo.TowTruck->Location, Location, TRACE_AllBlocking) && Hit.Actor != TowInfo.TowTruck)
				{
					TowLineBlockedFor += DeltaSeconds;
				}
				// Not obstructed - reset counter
				else
				{
					TowLineBlockedFor = 0.f;
				}

				// Been obstructed for too long - break the tow cable
				if(TowLineBlockedFor > TowLineBlockedBreakTime)
				{
					eventBreakTowLink();
				}
			}
		}
	}

	// If we have control for turning body to match look direction, update them here.
	if(SpineTurnControl)
	{
		FLOAT DesiredYaw = Clamp<FLOAT>(CurrentLookYaw, -MaxTrackYaw, MaxTrackYaw);
		FLOAT MaxDeltaYaw = DeltaSeconds * 3.f;
		FLOAT DeltaYaw = Clamp<FLOAT>(DesiredYaw - CurrentHeadYaw, -MaxDeltaYaw, MaxDeltaYaw);
		CurrentHeadYaw += DeltaYaw;

		FLOAT UnrealRot = CurrentHeadYaw * Rad2U;
		SpineTurnControl->BoneRotation.Yaw = appTrunc(UnrealRot);
	}

	if ( bInTow && TowInfo.TowTruck && TowBeamEmitter )
	{
		FVector EndPoint;

		if (TowInfo.TowAttachPoint == NAME_None || TowInfo.TowTruck->Mesh == NULL || !TowInfo.TowTruck->Mesh->GetSocketWorldLocationAndRotation(TowInfo.TowAttachPoint, EndPoint, NULL))
		{
			EndPoint = TowInfo.TowTruck->Location;
		}

		// if the beam would pass through the camera, adjust the endpoint so it ends in front of the camera (looks better)
		//@todo: this will have issues with splitscreen
		for (FPlayerIterator It(GEngine); It; ++It)
		{
			if (It->Actor != NULL)
			{
				FVector CameraLocation(It->Actor->Location);
				FRotator CameraRotation(It->Actor->Rotation);
				It->Actor->eventGetPlayerViewPoint(CameraLocation, CameraRotation);

				FVector CameraDir = CameraRotation.Vector();
				FVector BeamDir = (EndPoint - TowBeamEmitter->LocalToWorld.GetOrigin()).SafeNormal();
				if ((CameraDir | BeamDir) < -0.75f && ((EndPoint - CameraLocation).SafeNormal() | BeamDir) > 0.0f)
				{
					EndPoint = TowBeamEmitter->LocalToWorld.GetOrigin() + BeamDir * ((CameraLocation - TowBeamEmitter->LocalToWorld.GetOrigin()).Size() - 20.0f);
					break;
				}
			}
		}

		if ( TowBeamEmitter->HiddenGame )
		{
			TowBeamEmitter->SetHiddenGame(FALSE);
			TowBeamEmitter->SetActive(TRUE);
		}

		TowBeamEmitter->SetVectorParameter(TowBeamEndParameterName, EndPoint);
		if(TowBeamEndPointEffect)
		{
			if (TowBeamEndPointEffect->HiddenGame)
			{
				TowBeamEndPointEffect->SetHiddenGame(FALSE);
				TowBeamEndPointEffect->SetActive(TRUE);
			}
			TowBeamEndPointEffect->Translation = EndPoint;
			TowBeamEndPointEffect->BeginDeferredUpdateTransform();
		}

		if(TowControl)
		{
			// Make sure arm look at control is enabled
			if(TowControl->StrengthTarget < 0.5f)
			{
				TowControl->SetSkelControlActive(TRUE);
			}

			// Update target so arm points at it
			TowControl->TargetLocation = EndPoint;
		}
	}
	else
	{
		if ( TowBeamEmitter && !TowBeamEmitter->HiddenGame )
		{
			TowBeamEmitter->SetHiddenGame(TRUE);
			TowBeamEmitter->SetActive(FALSE);
		}
		if(TowBeamEndPointEffect && !TowBeamEndPointEffect->HiddenGame)
		{
			TowBeamEndPointEffect->SetHiddenGame(TRUE);
			TowBeamEndPointEffect->SetActive(FALSE);
		}

		// Make sure arm look at control is disabled
		if(TowControl && TowControl->StrengthTarget > 0.5f)
		{
			TowControl->SetSkelControlActive(FALSE);
		}
	}

}

