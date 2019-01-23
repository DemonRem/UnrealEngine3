/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"
#include "EngineMaterialClasses.h"
#include "LensFlare.h"

IMPLEMENT_CLASS(AUTVehicle_SPMA);
IMPLEMENT_CLASS(AUTVWeap_SPMACannon);
IMPLEMENT_CLASS(AUTProj_SPMACamera);

/**
 * If we are deployed with a camera out, then the WeaponRotation has to be calculated from the Aimpoint and the location
 */

void AUTVehicle_SPMA::ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation)
{
	Super::ApplyWeaponRotation(SeatIndex,NewRotation);

	if ( SeatIndex == 0 )
	{
		if ( DeployedState != EDS_Undeployed  && DeployedState != EDS_UnDeploying)
		{
			AUTVWeap_SPMACannon* Gun = Cast<AUTVWeap_SPMACannon>(Seats(0).Gun);
			if (Gun != NULL)
			{
				if (Gun->RemoteCamera != NULL)
				{
					WeaponRotation = (Gun->RemoteCamera->GetCurrentTargetLocation(Controller) - Location).Rotation();
				}
				Gun->eventCalcTargetVelocity();
				WeaponRotation.Pitch = Gun->TargetVelocity.Rotation().Pitch;
			}
		}
		else
		{
			WeaponRotation = Rotation;
		}
		for (INT i = 0; i < Seats(SeatIndex).TurretControllers.Num(); i++)
		{
			Seats(SeatIndex).TurretControllers(i)->DesiredBoneRotation = WeaponRotation;
		}
	}
}


void AUTVehicle_SPMA::TickSpecial( FLOAT DeltaSeconds )
{
	// Calculate the best Firing Angle

	Super::TickSpecial(DeltaSeconds);

	if ( bDeadVehicle )
		return;

	// client side effects follow - return if server
	//@todo steve - skip if far away
	if ( WorldInfo->NetMode == NM_DedicatedServer || bDeadVehicle )
		return;

	FLOAT Speed = Velocity.Size() * 0.01f;
	if ( Speed < 0.2f )
		Speed = 0.f;
	else if ( (Velocity | Rotation.Vector()) < 0.f )
		Speed *= -1.f;

	FLOAT TreadSpeed = EDS_Undeployed != DeployedState?0.0:Wheels(0)->SpinVel * DeltaSeconds * (45.f/(FLOAT)PI) * 0.7;
	TreadPan+= TreadSpeed;

	// pan tread materials
	if ( TreadMaterialInstance )
	{
		TreadMaterialInstance->SetScalarParameterValue(TreadSpeedParameterName, TreadPan);
	}

	// rotate other wheels
	if ( Speed != 0.f )
	{
		FLOAT LeftRotation = Wheels(1)->CurrentRotation;
		USkelControlWheel* SkelControl = Cast<USkelControlWheel>( Mesh->FindSkelControl(LeftBigWheel) );
		if(SkelControl)
		{
			SkelControl->UpdateWheelControl( 0.f, LeftRotation, 0.f );
		}

		for ( int i=0; i<3; i++ )
		{
			SkelControl = Cast<USkelControlWheel>( Mesh->FindSkelControl(LeftSmallWheels[i]) );
			if(SkelControl)
			{
				SkelControl->UpdateWheelControl( 0.f, LeftRotation, 0.f );
			}
		}

		FLOAT RightRotation = Wheels(0)->CurrentRotation;
		SkelControl = Cast<USkelControlWheel>( Mesh->FindSkelControl(RightBigWheel) );
		if(SkelControl)
		{
			SkelControl->UpdateWheelControl( 0.f, RightRotation, 0.f );
		}

		for ( int i=0; i<3; i++ )
		{
			SkelControl = Cast<USkelControlWheel>( Mesh->FindSkelControl(RightSmallWheels[i]) );
			if(SkelControl)
			{
				SkelControl->UpdateWheelControl( 0.f, RightRotation, 0.f );
			}
		}
	}
}

FVector AUTProj_SPMACamera::GetCurrentTargetLocation(class AController* C)
{
	if ( C )
	{
		FVector Start = Location;
		FVector End = Start + ( C->Rotation.Vector() * MaxTargetRange);

		FCheckResult Hit(1.0f);

		if ( !GWorld->SingleLineCheck(Hit, this,  End, Start, TRACE_ProjTargets) )
		{
			LastTargetLocation = Hit.Location;
		}
		else
		{
			LastTargetLocation = End;
		}

		LastTargetNormal = Hit.Normal;
	}


	if(PSC_EndPoint && PSC_CurEndPoint)
	{
		PSC_EndPoint->Rotation = LastTargetNormal.Rotation();
		PSC_CurEndPoint->Rotation = LastTargetNormal.Rotation();
		
		if(PSC_EndPoint)
		{
			PSC_EndPoint->Translation=LastTargetLocation;
			PSC_EndPoint->BeginDeferredUpdateTransform();
			if(!PSC_EndPoint->bIsActive || PSC_EndPoint->bWasDeactivated)
			{
				PSC_EndPoint->ActivateSystem();
			}
		}
	}
	
	return LastTargetLocation;
}

void AUTProj_SPMACamera::KillTrajectory()
{
	if(PSC_Trail)
	{
		PSC_Trail->DeactivateSystem();
		PSC_Trail->KillParticlesForced();
	}
	if(PSC_StartPoint)
	{
		PSC_StartPoint->DeactivateSystem();
		PSC_StartPoint->KillParticlesForced();
	}
	if(PSC_EndPoint)
	{
		PSC_EndPoint->DeactivateSystem();
		PSC_EndPoint->KillParticlesForced();
	}
}

void AUTProj_SPMACamera::SimulateTrajectory(FVector TossVelocity)
{
	FVector Destination;
	FVector Start;
	FLOAT Gravity;
	FVector GravVect, LastLoc, NewLoc;
	if(!Instigator || !InstigatorGun)
	{
		return;
	}

	Destination=GetCurrentTargetLocation(Instigator->Controller);
	Start=InstigatorGun->eventGetPhysFireStartLocation();

	if(!(Instigator->IsLocallyControlled() && Instigator->IsHumanControlled()) || WorldInfo->NetMode == NM_DedicatedServer)
	{
		if(PSC_Trail)
		{
			PSC_Trail->DeactivateSystem();
		}
		if(PSC_StartPoint)
		{
			PSC_StartPoint->DeactivateSystem();
		}
		if(PSC_EndPoint)
		{
			PSC_EndPoint->DeactivateSystem();
		}
		return;
	}
	if (bDisplayingArc == false)
	{
		eventSetUpSimulation();
	}
	Gravity = InstigatorGun->MyVehicle->PhysicsVolume ? InstigatorGun->MyVehicle->PhysicsVolume->GetGravityZ() : GWorld->GetGravityZ();
	GravVect = FVector(0,0,2*Gravity);
	LastLoc = PSC_Trail?PSC_Trail->Translation:FVector(0.f);
	if(!InstigatorGun->bCanHitTargetVector || ((LastTargetVelocity - InstigatorGun->TargetVelocity).SizeSquared()>1000 || ((Destination - LastLoc).SizeSquared()>1000)))
	{
		LastTargetVelocity = InstigatorGun->TargetVelocity;
		if(PSC_StartPoint)
		{
			PSC_StartPoint->DeactivateSystem();
		}
		NewLoc = Start;
		LastLoc = Start;
		FVector NewVel;
		SimulationCount = 0;
		if(PSC_Trail)
		{
			PSC_Trail->DeactivateSystem();
			PSC_Trail->Translation=Start;
			PSC_Trail->ActivateSystem();
			PSC_Trail->KillParticlesForced();
		}
		PSC_Trail->SetSkipUpdateDynamicDataDuringTick(TRUE);
		//PSC_Trail->Tick(0.05f);
		UBOOL bHitGround = false; // for trace while not aiming correctly
		if(!(InstigatorGun->bCanHitTargetVector))
		{
			FVector SocketLocation, Aim;
			FRotator SocketRotation;
			InstigatorGun->MyVehicle->eventGetBarrelLocationAndRotation(InstigatorGun->SeatIndex, SocketLocation, SocketRotation);
			Aim = SocketRotation.Vector();
			TossVelocity = Aim * TossVelocity.Size();
		}
		while(((InstigatorGun->bCanHitTargetVector) && (Destination - NewLoc).SizeSquared()>1000) || (!(InstigatorGun->bCanHitTargetVector) && !bHitGround))
		{
			++SimulationCount;
			NewVel = TossVelocity + (0.0005f*GravVect)*SimulationCount*SimulationCount;
			NewVel += InstigatorGun->MyVehicle->PhysicsVolume->ZoneVelocity;
			NewLoc = LastLoc + (NewVel * 0.001f * SimulationCount);
			if((InstigatorGun->bCanHitTargetVector) && (Destination - LastLoc).SizeSquared2D() - (Destination - NewLoc).SizeSquared2D() < 0.f) // if we're moving away from the target in 2d space, something went wrong so just abort to the destination
			{
				NewLoc = Destination;
			}
			if(PSC_Trail)
			{
				PSC_Trail->Translation=(NewLoc);
				if(!(InstigatorGun->bCanHitTargetVector))
				{
					FVector BoxExtent(0.f,0.f,0.f);
					DWORD TraceFlags = TRACE_World|TRACE_StopAtAnyHit;
					// Trace the line.
					FCheckResult Hit(1.f);
					GWorld->SingleLineCheck( Hit, this, NewLoc, LastLoc, TraceFlags, BoxExtent );
					if(Hit.Time != 1.f || SimulationCount >= 8000) // 8000 is 8 seconds, the lifetime of the projectile.
					{
						if(SimulationCount >= 8000)
						{
							warnf(NAME_Warning, TEXT("SPMA Simulation taking over 8000 simulations. Current location: %s -- performance might suffer until simulation shortens."),*(NewLoc.ToString()));
						}
						bHitGround = true;
						NewLoc = Hit.Location;
						NewLoc.Z += 25.; // to get use up above and avoiding culling issues
					}
				}
				LastLoc = NewLoc;
				PSC_Trail->ConditionalUpdateTransform();
				PSC_Trail->ConditionalTick(0.05f);
				//PSC_Trail->Tick(0.5f);
			}
		}
		PSC_Trail->SetSkipUpdateDynamicDataDuringTick(FALSE);
		//PSC_Trail->ConditionalTick(0.0f);
		//PSC_Trail->Tick(0.5f);
		if(PSC_StartPoint)
		{
			PSC_StartPoint->Translation=(Start);
			PSC_StartPoint->BeginDeferredUpdateTransform();
			if(!PSC_StartPoint->bIsActive)
			{
				PSC_StartPoint->ActivateSystem();
			}
		}
		if(PSC_CurEndPoint)
		{
			PSC_CurEndPoint->Translation = NewLoc;
			PSC_CurEndPoint->BeginDeferredUpdateTransform();
			if(!InstigatorGun->bCanHitTargetVector && (!PSC_CurEndPoint->bIsActive || PSC_CurEndPoint->bWasDeactivated))
			{
				PSC_CurEndPoint->ActivateSystem();
			}
		}
		//DrawDebugLine(NewLoc,LastLoc,255,0,0,true);
	}

}
