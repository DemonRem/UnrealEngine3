//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UnPath.h"

IMPLEMENT_CLASS(AUTWeapon);
IMPLEMENT_CLASS(AUTWeaponShield);
IMPLEMENT_CLASS(AUTWeaponPickupFactory);
IMPLEMENT_CLASS(AUTRemoteRedeemer);
IMPLEMENT_CLASS(AUTProjectile);
IMPLEMENT_CLASS(AUTProj_ScavengerBoltBase);
IMPLEMENT_CLASS(UUTExplosionLight);
IMPLEMENT_CLASS(AUTWeap_ImpactHammer);
IMPLEMENT_CLASS(AUTVWeap_TowCable);
IMPLEMENT_CLASS(AUTWeap_FlakCannon);
IMPLEMENT_CLASS(AUTVWeap_NightshadeGun);
IMPLEMENT_CLASS(AUTProj_ScorpionGlob_Base);

UBOOL AUTWeaponShield::IgnoreBlockingBy(const AActor* Other) const
{
	return (Other->GetAProjectile() != NULL) ? FALSE : TRUE;
}

UBOOL AUTWeaponShield::ShouldTrace(UPrimitiveComponent* Primitive, AActor* SourceActor, DWORD TraceFlags)
{
	return (SourceActor != NULL && ((TraceFlags & TRACE_ComplexCollision) || SourceActor->GetAProjectile() || SourceActor->IsA(AWeapon::StaticClass())) && !IsOwnedBy(SourceActor)) ? TRUE : FALSE;
}

UBOOL AUTRemoteRedeemer::IsPlayerPawn() const
{
	return FALSE;
}

void AUTWeap_ImpactHammer::TickSpecial( FLOAT DeltaTime )
{
	Super::TickSpecial(DeltaTime);

	if ( bIsCurrentlyCharging && (WorldInfo->TimeSeconds - ChargeTime > MinChargeTime) )
	{
		// @todo fixmesteve - move the trace to TickSpecial as well
		eventImpactAutoFire();
	}
}


void AUTWeap_FlakCannon::TickSpecial(FLOAT DeltaSeconds)
{

	INT desiredTensOdometer;
	INT desiredOnesOdometer;
	INT OdometerDiffOnes, OdometerDiffTens;
	USkelControlSingleBone* SkelControl;
	
	Super::TickSpecial(DeltaSeconds);
	if (bIsAmmoOdometerAnimated && Instigator && Instigator->Weapon == this && Instigator->IsHumanControlled() && Instigator->IsLocallyControlled())
	{
		desiredOnesOdometer = (AmmoCount%10)*-6554;
		desiredTensOdometer = (AmmoCount/10)*-6554;
		
		// if we're where we want, we're done.
		if(curTensOdometer == desiredTensOdometer && curOnesOdometer == desiredOnesOdometer)
			return;

		OdometerDiffOnes = appTrunc(OdometerMaxPerSecOnes *DeltaSeconds);
		OdometerDiffTens = appTrunc(OdometerMaxPerSecTens *DeltaSeconds);

		//wrap around (range 0 through -65535)
		if(curOnesOdometer> 0)
		{
			curOnesOdometer = (curOnesOdometer)-65535;
		}
		if(curTensOdometer > 0)
		{
			curTensOdometer = (curTensOdometer)-65535;
		}
		if(curOnesOdometer < -65535)
		{
			curOnesOdometer= (curOnesOdometer)+65535;
		}
		if(curTensOdometer < -65535)
		{
			curTensOdometer=(curTensOdometer)+65535;
		}
		
		// deal with direction

		if((desiredOnesOdometer - curOnesOdometer) > 32768)
		{
			curOnesOdometer += 65536;	
		}
		else if((desiredOnesOdometer - curOnesOdometer) < -32768)
		{
			curOnesOdometer -= 65536;
		}
		if(curOnesOdometer-desiredOnesOdometer < 0) // opposite way
		{
			OdometerDiffOnes *= -1;
		}
		
		
		if((desiredTensOdometer - curTensOdometer) > 32768)
		{
			curTensOdometer += 65536;	
		}
		else if((desiredTensOdometer - curTensOdometer) < -32768)
		{
			curTensOdometer -= 65536;
		}
		if(curTensOdometer-desiredTensOdometer < 0) // opposite way
		{
			OdometerDiffTens *= -1;
		}		
		// then deal with overshoot,
		if(Abs(curOnesOdometer-desiredOnesOdometer) < Abs(OdometerDiffOnes))  // if we'd overshoot, go straight there
		{
			curOnesOdometer = desiredOnesOdometer;
		}
		else // otherwise move as far as we can
		{
			curOnesOdometer -= OdometerDiffOnes;
		}
		if(Abs(curTensOdometer-desiredTensOdometer) < Abs(OdometerDiffTens))
		{
			curTensOdometer = desiredTensOdometer;
		}
		else
		{
			curTensOdometer -= OdometerDiffTens;
		}
		
		
		// finally, set it to the new value:
		SkelControl = (USkelControlSingleBone*)SkeletonFirstPersonMesh->FindSkelControl(OnesPlaceSkelName);
		if(SkelControl != NULL)
		{
			SkelControl->BoneRotation.Pitch = curOnesOdometer;
		}
		SkelControl = (USkelControlSingleBone*)(SkeletonFirstPersonMesh->FindSkelControl(TensPlaceSkelName));
		if(SkelControl != NULL)
		{
			SkelControl->BoneRotation.Pitch = curTensOdometer;
		}
	}
}


/*-----------------------------------------------------------------------------
	FUTSkeletalMeshSceneProxy
	Support for rendering weapon at different FOV than world
-----------------------------------------------------------------------------*/

/**
 * A UT skeletal mesh component scene proxy.
 */
class FUTSkeletalMeshSceneProxy : public FSkeletalMeshSceneProxy
{
public:
	FUTSkeletalMeshSceneProxy(const USkeletalMeshComponent* Component, FLOAT InFOV )
	:	FSkeletalMeshSceneProxy(Component)
	,	FOV(InFOV)
	{
	}
	virtual ~FUTSkeletalMeshSceneProxy()
	{
	}

	/**
	 * Returns the world transform to use for drawing.
	 * @param View - Current view
	 * @param OutLocalToWorld - Will contain the local-to-world transform when the function returns.
	 * @param OutWorldToLocal - Will contain the world-to-local transform when the function returns.
	 */
	virtual void GetWorldMatrices( const FSceneView* View, FMatrix& OutLocalToWorld, FMatrix& OutWorldToLocal )
	{
		if (FOV != 0.0f)
		{
			const FMatrix LocalToView = LocalToWorld * View->ViewMatrix;
			const FMatrix ViewToWarpedView =
				FPerspectiveMatrix(FOV * PI / 360.0f, View->SizeX, View->SizeY, View->NearClippingDistance) *
				View->ProjectionMatrix.Inverse();

			OutLocalToWorld = LocalToView * ViewToWarpedView * View->ViewMatrix.Inverse();
			OutWorldToLocal = OutLocalToWorld.Inverse();
		}
		else
		{
			OutLocalToWorld = LocalToWorld;
			OutWorldToLocal = LocalToWorld.Inverse();
		}
	}

	FORCEINLINE void SetFOV(FLOAT NewFOV)
	{
		FOV = NewFOV;
	}

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const { return( FSkeletalMeshSceneProxy::GetAllocatedSize() ); }

private:
    FLOAT FOV;
};

FPrimitiveSceneProxy* UUTSkeletalMeshComponent::CreateSceneProxy()
{
	FSkeletalMeshSceneProxy* Result = NULL;

	// Only create a scene proxy for rendering if properly initialized
	if( SkeletalMesh && 
		SkeletalMesh->LODModels.IsValidIndex(PredictedLODLevel) &&
		!bHideSkin &&
		MeshObject )
	{
		Result = ::new FUTSkeletalMeshSceneProxy(this, FOV);
	}

	return Result;
}

void UUTSkeletalMeshComponent::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( bForceLoadTextures && (ClearStreamingTime < GWorld->GetWorldInfo()->TimeSeconds) )
	{
		eventPreloadTextures(FALSE, 0.f);
	}
}

void UUTSkeletalMeshComponent::SetFOV(FLOAT NewFOV)
{
	if (FOV != NewFOV)
	{
		FOV = NewFOV;
		if (SceneInfo != NULL)
		{
			// tell the rendering thread to update the proxy's FOV
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( UpdateFOVCommand, FPrimitiveSceneProxy*, Proxy, Scene_GetProxyFromInfo(SceneInfo), FLOAT, NewFOV, FOV,
														{
															((FUTSkeletalMeshSceneProxy*)Proxy)->SetFOV(NewFOV);
														} );
		}
	}
}

//--------------------------------------------------------------


void AUTWeaponPickupFactory::PostEditChange(UProperty* PropertyThatChanged)
{
	if (WeaponPickupClass != NULL && WeaponPickupClass->IsChildOf(AUTDeployable::StaticClass()))
	{
		appMsgf(AMT_OK, TEXT("Deployables must use UTDeployablePickupFactory!"));
		WeaponPickupClass = NULL;
	}

	Super::PostEditChange(PropertyThatChanged);
}

void AUTWeaponPickupFactory::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.Ver() < VER_FIXED_INCORRECT_COLLISIONTYPE)
	{
		bBlockActors = GetClass()->GetDefaultObject<AUTWeaponPickupFactory>()->bBlockActors;
	}
}

void AUTWeaponPickupFactory::CheckForErrors()
{
	Super::CheckForErrors();

	if (WeaponPickupClass == NULL)
	{
		GWarn->MapCheck_Add(MCTYPE_ERROR, this, TEXT("UTWeaponPickupFactory with no weapon type"), MCACTION_NONE);
	}
}

//--------------------------------------------------------------
// Projectiles

FLOAT AUTProjectile::GetGravityZ()
{
	return Super::GetGravityZ() * CustomGravityScaling;
}

void AUTProjectile::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if (bWideCheck && bCollideActors && (Instigator != NULL || InstigatorBaseVehicle != NULL))
	{
		APlayerReplicationInfo* InstigatorPRI = (Instigator != NULL) ? Instigator->PlayerReplicationInfo : InstigatorBaseVehicle->PlayerReplicationInfo;
		// hit enemy if just nearby (for console games)
		// @todo fixmesteve - make ActorOverlapCheck fast by passing desired class (same with RadiusCheck)
		FMemMark Mark(GMem);
		FCheckResult* Link = GWorld->Hash->ActorOverlapCheck(GMem, this, Location, CheckRadius);
		for( FCheckResult* result=Link; result; result=result->GetNext())
		{
			APawn* TargetPawn = Link->Actor ? Link->Actor->GetAPawn() : NULL;
			if ( TargetPawn && !IgnoreBlockingBy(TargetPawn) && TargetPawn->IsValidEnemyTargetFor(InstigatorPRI, FALSE) )
			{
				UBOOL bDoTouch = TRUE;
				if ( TargetPawn->Velocity.IsNearlyZero() )
				{
					// reduce effective radius if target not moving
					const FLOAT EffectiveRadius = 0.3f * CheckRadius;

					// find nearest point projectile will pass
					if ( Abs(Location.Z - TargetPawn->Location.Z) > TargetPawn->CylinderComponent->CollisionHeight + EffectiveRadius )
					{
						bDoTouch = FALSE;
					}
					else
					{
						FVector ClosestPoint;
						PointDistToLine(TargetPawn->Location, Velocity, Location, ClosestPoint);
						bDoTouch = (ClosestPoint - TargetPawn->Location).Size2D() < TargetPawn->CylinderComponent->CollisionRadius + EffectiveRadius;
					}
				}
				if ( bDoTouch )
				{
					eventTouch( TargetPawn, TargetPawn->CylinderComponent, Location, (Location - TargetPawn->Location).SafeNormal() );
					break;
				}
			}
		}
		Mark.Pop();
	}
	if ( bCheckProjectileLight && Instigator )
	{
		bCheckProjectileLight = false;
		if ( Instigator->IsHumanControlled() && Instigator->IsLocallyControlled() )
		{
			eventCreateProjectileLight();
		}
	}
}

UBOOL AUTProjectile::IgnoreBlockingBy(const AActor* Other) const
{
	return ((!bBlockedByInstigator && InstigatorBaseVehicle == Other) || Super::IgnoreBlockingBy(Other));
}

void UUTExplosionLight::ResetLight()
{
	if ( !bEnabled)
	{
		bEnabled = TRUE;
		// flag as dirty to guarantee an update this frame
		BeginDeferredReattach();
	}

	TimeShiftIndex = 0;
	Lifetime = 0.f;
}

void UUTExplosionLight::Attach()
{
	if (!bInitialized)
	{
		// pull initial light values from first TimeShift entry
		if (TimeShift.Num() > 0)
		{
			Radius = TimeShift(0).Radius;
			Brightness = TimeShift(0).Brightness;
			LightColor = TimeShift(0).LightColor;
		}
		bInitialized = TRUE;
	}

	Super::Attach();
}

/**
 * Updates time dependent state for this component.
 * Requires bAttached == true.
 * @param DeltaTime - The time since the last tick.
 */
void UUTExplosionLight::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( bCheckFrameRate )
	{
		bCheckFrameRate = FALSE;
		if ( DeltaTime < HighDetailFrameTime )
		{
			// if superhighdetail 
			if ( TimeShift.Num() > 0 )
				TimeShift(0).Radius *= 1.5f;
		}
	}
 
	if ( bEnabled )
	{
		if ( TimeShift.Num() <= TimeShiftIndex + 1 )
		{
			bEnabled = FALSE;
		}
		else
		{
			Lifetime += DeltaTime;
			if ( Lifetime > TimeShift(TimeShiftIndex+1).StartTime )
			{
				TimeShiftIndex++;
				if ( TimeShift.Num() <= TimeShiftIndex + 1 )
				{
					bEnabled = FALSE;
				}
			}
			if ( bEnabled )
			{
				// fade and color shift
				FLOAT InterpFactor = (Lifetime - TimeShift(TimeShiftIndex).StartTime)/(TimeShift(TimeShiftIndex+1).StartTime - TimeShift(TimeShiftIndex).StartTime);
				Radius = TimeShift(TimeShiftIndex).Radius * (1.f - InterpFactor) + TimeShift(TimeShiftIndex+1).Radius * InterpFactor;
				Brightness = TimeShift(TimeShiftIndex).Brightness * (1.f - InterpFactor) + TimeShift(TimeShiftIndex+1).Brightness * InterpFactor;
				LightColor.R = (BYTE)appTrunc(FLOAT(TimeShift(TimeShiftIndex).LightColor.R) * (1.f - InterpFactor) + FLOAT(TimeShift(TimeShiftIndex+1).LightColor.R) * InterpFactor);
				LightColor.G = (BYTE)appTrunc(FLOAT(TimeShift(TimeShiftIndex).LightColor.G) * (1.f - InterpFactor) + FLOAT(TimeShift(TimeShiftIndex+1).LightColor.G) * InterpFactor);
				LightColor.B = (BYTE)appTrunc(FLOAT(TimeShift(TimeShiftIndex).LightColor.B) * (1.f - InterpFactor) + FLOAT(TimeShift(TimeShiftIndex+1).LightColor.B) * InterpFactor);
				LightColor.A = (BYTE)appTrunc(FLOAT(TimeShift(TimeShiftIndex).LightColor.A) * (1.f - InterpFactor) + FLOAT(TimeShift(TimeShiftIndex+1).LightColor.A) * InterpFactor);
			}
		}
		BeginDeferredReattach();

		if (!bEnabled && DELEGATE_IS_SET(OnLightFinished))
		{
			delegateOnLightFinished(this);
		}
	}
}

/** returns terminal velocity (max speed while falling) for this actor.  Unless overridden, it returns the TerminalVelocity of the PhysicsVolume in which this actor is located.
*/
FLOAT AUTProjectile::GetTerminalVelocity()
{
	return (PhysicsVolume && PhysicsVolume->bWaterVolume) ? PhysicsVolume->TerminalVelocity : TerminalVelocity;
}


/*
GetNetBuoyancy()
determine how deep in water actor is standing:
0 = not in water,
1 = fully in water
*/
void AUTProjectile::GetNetBuoyancy(FLOAT &NetBuoyancy, FLOAT &NetFluidFriction)
{
	if ( PhysicsVolume->bWaterVolume )
	{
		NetBuoyancy = Buoyancy;
		NetFluidFriction = PhysicsVolume->FluidFriction;
	}
}

/**
  * Make sure TargetActor is still a valid target
  * If not, set it to none
  */
void AUTProj_ScavengerBoltBase::ValidateTarget()
{
	if ( !TargetActor || TargetActor->bDeleteMe || !Instigator || !Instigator->PlayerReplicationInfo
		|| ((Instigator->Location - TargetActor->Location).SizeSquared() > MaxAttackRangeSq) )
	{
		TargetActor = NULL;
	}
	if ( TargetActor )
	{
		APawn* TargetPawn = TargetActor->GetAPawn();
		if ( TargetPawn )
		{
			if ( (TargetPawn->Controller || (TargetPawn->Health <= 0)) && !TargetPawn->IsValidEnemyTargetFor(Instigator->PlayerReplicationInfo, false) )
			{
				TargetActor = NULL;
			}
			else
			{
				// also ignore pawns feigning death
				AUTPawn *UTTargetPawn = Cast<AUTPawn>(TargetPawn);
				if ( UTTargetPawn && UTTargetPawn->bFeigningDeath )
				{
					TargetActor = NULL;
				}
			}
		}
		else
		{
			AUTOnslaughtObjective* TargetNode = Cast<AUTOnslaughtObjective>(TargetActor);
			if ( TargetNode && TargetNode->bIsNeutral )
			{
				// ignore neutral nodes
				TargetActor = NULL;
			}
		}
	}
}

/*
  Scavenger seek accel update
*/
void AUTProj_ScavengerBoltBase::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);

	// make sure instigator and target are valid
	if ( !Instigator || Instigator->bDeleteMe )
	{
		if ( Role == ROLE_Authority )
		{
			eventKillBolt();
		}
		return;
	}

	if ( Role == ROLE_Authority )
	{
		if ( InstigatorController != Instigator->Controller )
		{
			eventKillBolt();
			return;
		}
		ValidateTarget();
	}

	UBOOL bActiveBeam = FALSE;
	FVector AccelNorm(0.f,0.f,0.f);
	if ( !TargetActor )
	{
		// return to and circle Scavenger owner
		FVector SeekingVector = Instigator->GetTargetLocation(this);
		AccelNorm = (SeekingVector - Location).SafeNormal();
		Acceleration = FastHomeAccel * AccelNorm;
	}
	else
	{
		// seek target
		FVector SeekingVector = TargetActor->GetTargetLocation(this);
		Acceleration = (SeekingVector - Location).SafeNormal();
		if ( TargetActor->GetAPawn() )
		{
			if ( (TargetActor == Instigator) && ((Instigator->Velocity | (Instigator->Location - Location)) > 0.f) )
			{
				Acceleration *= FastHomeAccel;
			}
			else
			{
				Acceleration *= SlowHomeAccel;
			}
		}
		else
		{
			Acceleration *= FastHomeAccel;
		}
		if ( (Location - SeekingVector).SizeSquared() < AttackRangeSq )
		{
			// make sure beam can hit target
			FCheckResult Hit(1.f);
			GWorld->SingleLineCheck( Hit, this, TargetActor->GetTargetLocation(this), Location, TRACE_AllBlocking );
			if ( !Hit.Actor || (Hit.Actor == TargetActor) || (Hit.Actor->Base == TargetActor) )
			{
				if ( WorldInfo->TimeSeconds - LastDamageTime > DamageFrequency )
				{
					eventDealDamage(Hit.Location);
				}
				bActiveBeam = TargetActor && !TargetActor->bDeleteMe;
			}
		}
	}

	// update attack beam
	if ( bActiveBeam )
	{
		if (  !BeamEmitter || BeamEmitter->HiddenGame )
		{
			eventSpawnBeam();
		}
		if ( BeamEmitter )
		{
			BeamEmitter->SetVectorParameter(BeamEndName, TargetActor->GetTargetLocation(this));
		}
	}
	else if ( BeamEmitter && !BeamEmitter->HiddenGame )
	{
		BeamEmitter->SetHiddenGame(TRUE);
	}
}

/*-----------------------------------------------------------------------------
Check to see if we can link to someone
-----------------------------------------------------------------------------*/
void AUTVWeap_TowCable::TickSpecial( FLOAT DeltaTime )
{
	Super::TickSpecial(DeltaTime);

	if (WorldInfo->NetMode != NM_DedicatedServer)
	{
		if (CrossScaleTime > 0.0f)
		{
			CrossScaler += (1.f - CrossScaler) * (DeltaTime / CrossScaleTime);
			CrossScaleTime -= DeltaTime;

			if (CrossScaleTime <= 0.0f)
			{
				CrossScaler = 1.f;
			}
		}
	}
	if  ( (Role == ROLE_Authority) && MyHoverboard && Instigator )
	{
		AUTGame* Game = Cast<AUTGame>(WorldInfo->Game);
		AUTVehicle* BestPick = NULL;
		if ( Game )
		{
			FLOAT BestDot = 0.0f;
			const FVector XAxis = FRotationMatrix(MyHoverboard->Rotation).GetAxis(0);
			const FVector HoverboardDir = MyHoverboard->Rotation.Vector();
			const FLOAT MaxAttachRangeSq = Square(MaxAttachRange);
			const BYTE HoverboardTeam = Instigator->eventGetTeamNum();

			for (AUTVehicle* Vehicle = Game->VehicleList; Vehicle != NULL; Vehicle = Vehicle->NextVehicle)
			{
				if ( Vehicle->Driver && Vehicle->Health > 0 && Vehicle->HoverBoardAttachSockets.Num() > 0 && 
						Vehicle->Team == HoverboardTeam &&
						(Vehicle->Location - MyHoverboard->Location).SizeSquared() <= MaxAttachRangeSq )
				{
					const FLOAT DotA = XAxis | ( MyHoverboard->Location - Vehicle->Location ).SafeNormal();
					const FLOAT DotB = HoverboardDir | Vehicle->Rotation.Vector();
					if ( DotA < -0.75f && DotB > 0.0f )
					{
						if (!BestPick || DotA < BestDot)
						{
							FCheckResult Hit(1.0f);
							if (GWorld->SingleLineCheck(Hit, MyHoverboard, MyHoverboard->Location, Vehicle->Location, TRACE_AllBlocking | TRACE_StopAtAnyHit) || Hit.Actor == Vehicle)
							{
								BestPick = Vehicle;
								BestDot = DotA;
							}
						}
					}
				}
			}
		}
		if (BestPick != PotentialTowTruck)
		{
			PotentialTowTruck = BestPick;
			bNetDirty = TRUE;
		}
	}
}

/*
  Scorpion ball seek accel update
*/
void AUTProj_ScorpionGlob_Base::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);

	if ( SeekPawn )
	{
		if ( SeekPawn->bDeleteMe )
		{
			SeekPawn = NULL;
			return;
		}

		if ( (SeekPawn->Location - Location).SizeSquared() < 9000000.0 )
		{
			if ( WorldInfo->TimeSeconds - LastTraceTime > 0.25f )
			{
				// Check visibility every 0.25 seconds.
				FCheckResult Hit(1.f);
				GWorld->SingleLineCheck( Hit, this, SeekPawn->Location, Location, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) );
				if ( Hit.Actor )
				{
					return;
				}
			}

			Velocity = MaxSpeed * (Velocity/MaxSpeed + TrackingFactor * DeltaSeconds * (SeekPawn->Location - Location).SafeNormal()).SafeNormal();
		}
	}
}
