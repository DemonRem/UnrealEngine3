/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "EngineMaterialClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameAnimationClasses.h"

IMPLEMENT_CLASS(AUTVehicleBase);
IMPLEMENT_CLASS(AUTVehicle);
IMPLEMENT_CLASS(AUTWeaponPawn);
IMPLEMENT_CLASS(AUTAirVehicle);
IMPLEMENT_CLASS(AUTHoverVehicle);
IMPLEMENT_CLASS(AUTVehicle_Deployable);
IMPLEMENT_CLASS(AUTVehicle_Goliath);
IMPLEMENT_CLASS(AUTVehicle_Manta);
IMPLEMENT_CLASS(AUTVehicle_Raptor);
IMPLEMENT_CLASS(AUTVehicle_Cicada);
IMPLEMENT_CLASS(AUTVehicle_TrackTurretBase);
IMPLEMENT_CLASS(AUTVehicle_Viper);
IMPLEMENT_CLASS(UUTVehicleSimChopper);
IMPLEMENT_CLASS(UUTVehicleSimHover);
IMPLEMENT_CLASS(UUTVehicleSimCar);
IMPLEMENT_CLASS(UUTVehicleSimTank);
IMPLEMENT_CLASS(UUTVehicleSimHoverTank);
IMPLEMENT_CLASS(AUTVehicleWeapon);
IMPLEMENT_CLASS(UUTVehicleWheel);
IMPLEMENT_CLASS(AVehicleMovementEffect);
#ifndef RAD_TO_DEG
	#define RAD_TO_DEG 57.2957795132
#endif


void AVehicleMovementEffect::TickSpecial(FLOAT DeltaTime)
{
	if(AirEffect && Base)
	{
		FLOAT VelSq =Base->Velocity.SizeSquared();
		// First make sure we're moving fast enough to care:
		if(VelSq > MinVelocityForAirEffect)
		{
			UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(AirEffect->GetMaterial(0));
			// Set up the material data, if there is no material data create it.
			if(MIC)
			{
				FLOAT Result = Min(1.0f,Max(VelSq-MinVelocityForAirEffect,0.0f)/MaxVelocityForAirEffect);
				if(AirCurrentLevel - Result > 0)
				{
					AirCurrentLevel = Result;
				}
				else
				{
					FLOAT MaxDelta = AirMaxDelta*DeltaTime;
					if(Result-AirCurrentLevel > MaxDelta)
					{
						AirCurrentLevel = Result;
					}
					else
					{
						AirCurrentLevel += MaxDelta;
					}
				}
				MIC->SetScalarParameterValue(AirEffectScalar,AirCurrentLevel);
			}
			else
			{
				// EQUIVILENT TO: CreateAndSetMaterialInstanceConstant() in MeshComponent
				MIC = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), AirEffect);
				if(MIC)
				{
					MIC->SetParent(AirEffect->GetMaterial(0));
					AirEffect->SetMaterial(0,MIC);
				}
			}
			AirEffect->SetHiddenGame(false);
			
			// Here we take our current rotation, look at where we want to go, and set up a quaternion to perform the delta:
			FMatrix AirCurrentRotation = AirEffect->LocalToWorld; // where are
			AirCurrentRotation.SetAxis(2,FVector(0.f,0.f,0.f));
			FVector NewAirDir = -(Base->Velocity).SafeNormal(); // where want
			FVector CurrentAirDir = AirCurrentRotation.GetAxis(0).SafeNormal();
			FQuat DeltaAirQuat = FQuatFindBetween(CurrentAirDir,NewAirDir);
			FMatrix NewAirCurrentRotation = AirCurrentRotation * FQuatRotationTranslationMatrix(DeltaAirQuat,FVector(0,0,0));
			AirEffect->Rotation = NewAirCurrentRotation.Rotator();

			// Handy for debugging:
			//DrawDebugCoordinateSystem(Location,AirEffect->Rotation,500,false);
			AirEffect->BeginDeferredUpdateTransform();
		}
		else
		{
			AirEffect->SetHiddenGame(true);
		}
	}
}

void AUTVehicle::OnRigidBodyCollision(const FRigidBodyCollisionInfo& Info0, const FRigidBodyCollisionInfo& Info1, const FCollisionImpactData& RigidCollisionData)
{
	Super::OnRigidBodyCollision(Info0, Info1, RigidCollisionData);

	AActor* OtherActor = (Info0.Actor != this) ? Info0.Actor : Info1.Actor;
	FLOAT ImpactMag=RigidCollisionData.TotalNormalForceVector.Size();

	// update LastBlockingVehicle, so bots detect when another vehicle is on top of their destination
	AUTBot* B = Cast<AUTBot>(Controller);
	if (B != NULL)
	{
		B->LastBlockingVehicle = Cast<AVehicle>(OtherActor);
	}

	// If the impact force is non-zero
	if(ImpactMag > KINDA_SMALL_NUMBER)
	{
		FVector ImpactNorm = RigidCollisionData.TotalNormalForceVector/ImpactMag;
		FLOAT ForwardImpactMag = Abs(Mesh->LocalToWorld.GetAxis(0) | ImpactNorm);
		if(ForwardImpactMag > 0.7f)
		{
			bFrontalCollision = TRUE;

			if(OtherActor && OtherActor->Physics != PHYS_RigidBody)
			{
				bFrontalCollisionWithFixed = TRUE;
			}
		}
	}

	if(GWorld->GetNetMode() != NM_DedicatedServer && Health <= 0 && LastDeathImpactTime + 0.6 < GWorld->GetTimeSeconds() && Info0.Actor != NULL && Info1.Actor != NULL) // impact sounds on clients for dead vehicles
	{
		LastDeathImpactTime = GWorld->GetTimeSeconds();
		FVector ContactLoc = RigidCollisionData.ContactInfos(0).ContactPosition;
		// Notes to self: using consistent self destruct: Speedbike numbers: 1000-7000, Goliath numbers: all over 40k
		if(ImpactMag >= 20000.0f && LargeChunkImpactSound != NULL) // large chunk
		{
			PlaySound(LargeChunkImpactSound,true,true,true,&ContactLoc);
		}
		else if(ImpactMag >= 4000.0f && MediumChunkImpactSound != NULL) // medium chunk
		{
			PlaySound(MediumChunkImpactSound,true,true,true,&ContactLoc);
		}
		else if(ImpactMag >= 1000.0f && SmallChunkImpactSound != NULL) // small chunk
		{
			PlaySound(SmallChunkImpactSound,true,true,true,&ContactLoc);
		}
	}
}

void AUTVehicle::PostEditChange( UProperty* PropertyThatChanged )
{
	if (!GIsEditor && !IsTemplate())
	{
		eventOnPropertyChange( *PropertyThatChanged->GetName() );
	}

	Super::PostEditChange(PropertyThatChanged);
}

/** 
 * In order to have an easily extendable system, we use the following accessor function to perform property lookup based
 * on the name of the property.  This name is precached when the vehicle is constructed.  For more information see the
 * comments in UTVehicle.uc and https://udn.epicgames.com/Three/PropertyOverview
 *
 * @param	SeatIndex		The Seat in question
 * @param	NewRot			If writing, the new rotation to write
 * @param	bReadValue		If true, we are reading this value, not writing it
 *
 * @return the value if bReadValue is true
 */

FRotator AUTVehicle::SeatWeaponRotation(INT SeatIndex,FRotator NewRot,UBOOL bReadValue)
{
	FRotator Result = FRotator(0,0,0);
	if ( SeatIndex>=0 && SeatIndex < Seats.Num() )
 	{
		if ( !Seats(SeatIndex).WeaponRotationProperty )
		{
			// Find the UProperty in question

			UProperty* Prop = FindField<UProperty>(GetClass(), Seats(SeatIndex).WeaponRotationName);
			if (Prop != NULL)
			{

				// check to make sure the property is an FRotator.  We do this by insuring it's a UStructProperty named
				// Rotator.

				if (Prop->GetClass() != UStructProperty::StaticClass() || ((UStructProperty*)Prop)->Struct->GetFName() != NAME_Rotator)
				{
					debugf( NAME_Warning, TEXT("WeaponRotation property type mismatch: %s is %s, expected Rotator"), *Seats(SeatIndex).WeaponRotationName.ToString(), 
							(Prop->GetClass() != UStructProperty::StaticClass()) ? *Prop->GetClass()->GetName() : *((UStructProperty*)Prop)->Struct->GetName() );
					Prop = NULL;
				}
			}

			// Short circut if we couldn't find the property

			if (Prop == NULL)
			{
				return Result;
			}
			
			Seats(SeatIndex).WeaponRotationProperty = Prop;
		}

		/*
			Process the value.  A property doesn't hold the value of the property, it describes where in its owner 
			struct's (or class's) _instance_ to find the value of the property. So, the code gets the offset of the 
			property that it found by name, adds that offset to the beginning of the memory used by the vehicle instance, 
			and then copies what that memory location is pointing to. 
		*/

		BYTE* PropLoc = (BYTE*) this + ((UProperty*) Seats(SeatIndex).WeaponRotationProperty)->Offset;

		if ( bReadValue )
		{
			((UProperty*) Seats(SeatIndex).WeaponRotationProperty)->CopySingleValue(&Result, PropLoc);
		}
		else
		{
			((UProperty*) Seats(SeatIndex).WeaponRotationProperty)->CopySingleValue(PropLoc, &NewRot);
			bNetDirty=true;
		}
	}

	return Result;
}

/** 
 * In order to have an easily extendable system, we use the following accessor function to perform property lookup based
 * on the name of the property.  This name is precached when the vehicle is constructed.  For more information see the
 * comments in UTVehicle.uc and https://udn.epicgames.com/Three/PropertyOverview
 *
 * @param	SeatIndex		The Seat in question
 * @param	NewLoc			If writing, the new location to write
 * @param	bReadValue		If true, we are reading this value, not writing it
 *
 * @return the value if bReadValue is true
 */


FVector AUTVehicle::SeatFlashLocation(INT SeatIndex,FVector NewLoc,UBOOL bReadValue)
{
	FVector Result = FVector(0,0,0);

	if ( SeatIndex>=0 && SeatIndex < Seats.Num() )
	{
		if ( !Seats(SeatIndex).FlashLocationProperty )
		{
			UProperty* Prop = FindField<UProperty>(GetClass(), Seats(SeatIndex).FlashLocationName);
			if (Prop != NULL)
			{
				if (Prop->GetClass() != UStructProperty::StaticClass() || ((UStructProperty*)Prop)->Struct->GetFName() != NAME_Vector)
				{
					debugf( NAME_Warning, TEXT("FlashLocation property type mismatch: %s is %s, expected Vector"), *Seats(SeatIndex).FlashLocationName.ToString(), 
							(Prop->GetClass() != UStructProperty::StaticClass()) ? *Prop->GetClass()->GetName() : *((UStructProperty*)Prop)->Struct->GetName() );
					Prop = NULL;
				}
			}

			if (Prop == NULL)
			{
				return Result;
			}

			Seats(SeatIndex).FlashLocationProperty = Prop;
		}

		BYTE* PropLoc = (BYTE*) this + ((UProperty*) Seats(SeatIndex).FlashLocationProperty)->Offset;
		if ( bReadValue )
		{
			((UProperty*) Seats(SeatIndex).FlashLocationProperty)->CopySingleValue(&Result, PropLoc);
		}
		else
		{
			((UProperty*) Seats(SeatIndex).FlashLocationProperty)->CopySingleValue(PropLoc, &NewLoc);
			bNetDirty=true;
		}
	}
	return Result;
}

/** 
 * In order to have an easily extendable system, we use the following accessor function to perform property lookup based
 * on the name of the property.  This name is precached when the vehicle is constructed.  For more information see the
 * comments in UTVehicle.uc and https://udn.epicgames.com/Three/PropertyOverview
 *
 * @param	SeatIndex		The Seat in question
 * @param	NewCount		If writing, the new count to write
 * @param	bReadValue		If true, we are reading this value, not writing it
 *
 * @return the value if bReadValue is true
 */


BYTE AUTVehicle::SeatFlashCount(INT SeatIndex, BYTE NewCount, UBOOL bReadValue)
{
	BYTE Result = 0;

	if ( SeatIndex>=0 && SeatIndex < Seats.Num() )
	{
		if ( !Seats(SeatIndex).FlashCountProperty)
		{
			UProperty* Prop = FindField<UProperty>(GetClass(), Seats(SeatIndex).FlashCountName);
			if (Prop != NULL)
			{
				if (Prop->GetClass() != UByteProperty::StaticClass())
				{
					debugf(NAME_Warning, TEXT("FlashCount property type mismatch: %s is %s, expected ByteProperty"), *Seats(SeatIndex).FlashCountName.ToString(), *Prop->GetClass()->GetName());
					Prop = NULL;
				}
			}

			if (Prop == NULL)
			{
				return Result;
			}

			Seats(SeatIndex).FlashCountProperty = Prop;
		}
		
		BYTE* PropLoc = (BYTE*) this + ((UProperty*) Seats(SeatIndex).FlashCountProperty)->Offset;
		if ( bReadValue )
		{
			((UProperty*) Seats(SeatIndex).FlashCountProperty)->CopySingleValue(&Result, PropLoc);
		}
		else
		{
			((UProperty*) Seats(SeatIndex).FlashCountProperty)->CopySingleValue(PropLoc, &NewCount);
			bNetDirty=true;
		}
	}
	return Result;
}

/** 
 * In order to have an easily extendable system, we use the following accessor function to perform property lookup based
 * on the name of the property.  This name is precached when the vehicle is constructed.  For more information see the
 * comments in UTVehicle.uc and https://udn.epicgames.com/Three/PropertyOverview
 *
 * @param	SeatIndex		The Seat in question
 * @param	NewFireMode		writing, the new firing mode to write
 * @param	bReadValue		If true, we are reading this value, not writing it
 *
 * @return the value if bReadValue is true
 */

BYTE AUTVehicle::SeatFiringMode(INT SeatIndex, BYTE NewFireMode, UBOOL bReadValue)
{
	BYTE Result = 0;

	if ( SeatIndex>=0 && SeatIndex < Seats.Num() )
	{
		if ( !Seats(SeatIndex).FiringModeProperty )
		{
			UProperty* Prop = FindField<UProperty>(GetClass(), Seats(SeatIndex).FiringModeName);
			if (Prop != NULL)
			{
				if (Prop->GetClass() != UByteProperty::StaticClass())
				{
					debugf(NAME_Warning, TEXT("FiringMode property type mismatch: %s is %s, expected ByteProperty"), *Seats(SeatIndex).FiringModeName.ToString(), *Prop->GetClass()->GetName());
					Prop = NULL;
				}
			}

			if (Prop == NULL)
			{
				return Result;
			}

			Seats(SeatIndex).FiringModeProperty = Prop;
		}

		BYTE* PropLoc = (BYTE*) this + ((UProperty*) Seats(SeatIndex).FiringModeProperty)->Offset;
		if ( bReadValue )
		{
			((UProperty*) Seats(SeatIndex).FiringModeProperty)->CopySingleValue(&Result, PropLoc);
		}
		else
		{
			((UProperty*) Seats(SeatIndex).FiringModeProperty)->CopySingleValue(PropLoc, &NewFireMode);
			bNetDirty=true;
		}
	}
	return Result;
}

void AUTVehicle::execIsSeatControllerReplicationViewer(FFrame& Stack, RESULT_DECL)
{
	P_GET_INT(SeatIndex);
	P_FINISH;

	UBOOL bResult = FALSE;
	if (SeatIndex < Seats.Num() && Seats(SeatIndex).SeatPawn != NULL)
	{
		for (INT i = 0; i < WorldInfo->ReplicationViewers.Num(); i++)
		{
			if (WorldInfo->ReplicationViewers(i).InViewer == Seats(SeatIndex).SeatPawn->Controller)
			{
				bResult = TRUE;
				break;
			}
		}
	}

	*(UBOOL*)Result = bResult;
}

void AUTVehicle::InitDamageSkel()
{
	UAnimTree* AnimTree = Cast<UAnimTree>(Mesh->Animations);
	if(AnimTree)
	{
		TArray<USkelControlBase*>	Controls;
		AnimTree->GetSkelControls(Controls);
		INT j=0;
		for(INT i=0; i<Controls.Num(); ++i)
		{
			UUTSkelControl_Damage* DamageC = Cast<UUTSkelControl_Damage>(Controls(i));
			if(DamageC)
			{
				DamageSkelControls.Push(DamageC);
			}
		}
	}
}

void AUTVehicle::ApplyMorphDamage(FVector HitLocation,INT Damage, FVector Momentum)
{
	FLOAT Dist = -1.f;
	FLOAT BestDist = 0.f;
	FVector BoneLocation;
	INT MorphIndex = -1;
	FName CurBone;


	if( DamageSkelControls.Num() > 0)
	{
		UAnimTree* AnimTree = Cast<UAnimTree>(Mesh->Animations);
		if(AnimTree)
		{
			FLOAT SkelDist = -1.f;
			FLOAT BestDist = 0.f;
			INT BestSkelControl = -1;
			for(int j=0;j < DamageSkelControls.Num(); ++j)
			{
				// This is copied from the LookAt Skel Control... I agree with the below comment:
				// Find our bone index... argh wish there was a better way to know
				INT ControlBoneIndex = INDEX_NONE;
				if(DamageSkelControls(j)->HealthPerc > 0.f) // only care about 'live' bones
				{
					for(INT i=0; i<Mesh->RequiredBones.Num() && ControlBoneIndex == INDEX_NONE; i++)
					{
						const INT BoneIndex = Mesh->RequiredBones(i);

						if( (Mesh->SkelControlIndex.Num() > 0) && (Mesh->SkelControlIndex(BoneIndex) != 255) )
						{
							const INT ControlIndex = Mesh->SkelControlIndex(BoneIndex);
							USkelControlBase* Control = AnimTree->SkelControlLists(ControlIndex).ControlHead;
							while( Control )
							{
								// we found us... so we found the boneindex... wheee
								if( Control == DamageSkelControls(j) )
								{
									ControlBoneIndex = BoneIndex;
									break;
								}
								Control = Control->NextControl;
							}
						}
					}
					if( ControlBoneIndex == INDEX_NONE )
					{
						debugf(TEXT("Failure to find DamageSkelControl"));
						break;
					}
					else
					{
						FLOAT Dist = (Mesh->SpaceBases(ControlBoneIndex).GetOrigin()+Location - HitLocation).SizeSquared();
						if(Dist < BestDist || BestSkelControl < 0)
						{
							BestDist = Dist;
							BestSkelControl = j;
						}
					}
				}
			}

			// now that we have the best bone (phew! harder than need be!) Deal damage to it.
			if(BestSkelControl >= 0)
			{
				UUTSkelControl_DamageSpring* Spring = Cast<UUTSkelControl_DamageSpring>(DamageSkelControls(BestSkelControl));
				if (Spring != NULL)
				{
					Spring->LastHitMomentum = Momentum;
					Spring->LastHitTime = GWorld->GetTimeSeconds();
				}

				DamageSkelControls(BestSkelControl)->HealthPerc -= Min(DamageSkelControls(BestSkelControl)->HealthPerc,(FLOAT)Damage/(FLOAT)DamageSkelControls(BestSkelControl)->DamageMax);
			}
		}
	}
	// Quick exit if this vehicle doesn't have morph targets

	if ( DamageMorphTargets.Num() <= 0 )
	{
		return;
	}

	// Find the Influence bone that is closest to the hit

	for (INT i=0;i<Mesh->SkeletalMesh->RefSkeleton.Num();i++)
	{
		CurBone = Mesh->SkeletalMesh->RefSkeleton(i).Name;

		INT InfluenceBoneIndex = -1;
		for (INT j=0;j<DamageMorphTargets.Num();j++)
		{
			if (CurBone == DamageMorphTargets(j).InfluenceBone)
			{
				InfluenceBoneIndex = j;
				break;
			}
		}

		if ( InfluenceBoneIndex >= 0 )
		{
			BoneLocation = Mesh->GetBoneLocation(CurBone);

			Dist = (HitLocation - BoneLocation).Size();
			if (MorphIndex < 0 || Dist < BestDist)
			{
				BestDist = Dist;
				MorphIndex = InfluenceBoneIndex;
			}
		}
	}


	if ( MorphIndex >= 0 )	// We have the best
	{
		// Traverse the morph chain dealing out damage as needed

		while ( Damage > 0 )
		{
			// Deal some damage
			if ( DamageMorphTargets(MorphIndex).Health > 0 )
			{
				if ( DamageMorphTargets(MorphIndex).Health <= Damage )
				{
					Damage -= DamageMorphTargets(MorphIndex).Health;
					//debugf(TEXT("1. Adjusting Node %s %i"),*DamageMorphTargets(MorphIndex).MorphNodeName, DamageMorphTargets(MorphIndex).Health);
					DamageMorphTargets(MorphIndex).Health = 0;
		
					// This node is dead, so reset to the remaining damage and force this node's health to 0.  This
					// will allow the next node to get the proper damage amount
				}
				else
				{
					//debugf(TEXT("2. Adjusting Node %s %i"),*DamageMorphTargets(MorphIndex).MorphNodeName, DamageMorphTargets(MorphIndex).Health);
					DamageMorphTargets(MorphIndex).Health -= Damage;
					Damage = 0;
				}

				if ( DamageMorphTargets(MorphIndex).Health <= 0 )
				{
					eventMorphTargetDestroyed(MorphIndex);
				}
			}
				
			// Calculate the new Weight for the MorphTarget influenced by this node and set it.

			AUTVehicle* DefaultVehicle = (AUTVehicle*)( GetClass()->GetDefaultActor() );

			FLOAT Weight = 1.0 - ( FLOAT(DamageMorphTargets(MorphIndex).Health) / FLOAT(DefaultVehicle->DamageMorphTargets(MorphIndex).Health) );
			UMorphNodeWeight* MorphNode = DamageMorphTargets(MorphIndex).MorphNode;
			if (MorphNode != NULL)
			{
				MorphNode->SetNodeWeight(Weight);
			}
			else
			{
				debugf(NAME_Warning, TEXT("Failed to find MorphNode for DamageMorphTarget %i '%s' to apply damage"), MorphIndex, *DamageMorphTargets(MorphIndex).MorphNodeName.ToString());
			}

			// Contine the chain if we can.
			if ( DamageMorphTargets(MorphIndex).LinkedMorphNodeName != NAME_None && DamageMorphTargets(MorphIndex).LinkedMorphNodeIndex != MorphIndex )
			{
				MorphIndex = DamageMorphTargets(MorphIndex).LinkedMorphNodeIndex;
			}
			else
			{
				Damage = 0;
			}
		}
	}
	UpdateDamageMaterial();
}

/** 
 * This function calculates the various damage parameters in the damage material overlay.  It scans the DamageMorphTargets
 * list and generates a list of damage params and their weights.  It then blank assigns them.
 */
void AUTVehicle::UpdateDamageMaterial()
{
	if (DamageMaterialInstance != NULL)
	{
		TArray<FName> DamageNames;
		TArray<INT>	Healths;
		TArray<INT> MaxHealths;

		// Get a quick link to the default vehicle
		AUTVehicle* DefaultVehicle = GetArchetype<AUTVehicle>();

		for (INT i = 0; i < DamageMorphTargets.Num(); i++)
		{
			for (INT j = 0; j < DamageMorphTargets(i).DamagePropNames.Num(); j++)
			{
				INT ItemIndex;
				if (DamageNames.Num() == 0 || !DamageNames.FindItem(DamageMorphTargets(i).DamagePropNames(j), ItemIndex))
				{
					DamageNames.AddItem(DamageMorphTargets(i).DamagePropNames(j));
					Healths.AddItem(DamageMorphTargets(i).Health);
					MaxHealths.AddItem(DefaultVehicle->DamageMorphTargets(i).Health);
				}
				else
				{
					Healths(ItemIndex) += DamageMorphTargets(i).Health;
					MaxHealths(ItemIndex) += DefaultVehicle->DamageMorphTargets(i).Health;
				}
			}
		}

		for (INT i = 0; i < DamageNames.Num(); i++)
		{
			DamageMaterialInstance->SetScalarParameterValue(DamageNames(i), 1.0 - (FLOAT(Healths(i)) / FLOAT(MaxHealths(i))));
		}
	}
}

void AUTVehicle::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	// Reset frontal collision flags.
	bFrontalCollision = FALSE;
	bFrontalCollisionWithFixed = FALSE;

	// use appropriate physical material depending on whether being driven

	if ( WorldInfo->NetMode != NM_DedicatedServer )
	{
		if ( bDeadVehicle )
		{
			if (bIsBurning && BurnOutMaterialInstances.Num() > 0)
			{
				RemainingBurn += DeltaSeconds * 10.f/BurnOutTime;
				if ( RemainingBurn < 10.f )
				{
					for (INT i = 0; i < BurnOutMaterialInstances.Num(); i++)
					{
						if (BurnOutMaterialInstances(i) != NULL)
						{
							BurnOutMaterialInstances(i)->SetScalarParameterValue(BurnTimeParameterName, RemainingBurn); 
							if (RemainingBurn > 7.5f)
							{
								BurnOutMaterialInstances(i)->SetScalarParameterValue(BurnValueParameterName, 20.f * (10.f - RemainingBurn) / 2.5f); 
							}
						}
					}
				}
				else
				{
					bHidden = true;
				}
			}
			return;
		}


		// deal with wheel sounds/effects
		if (bVehicleOnGround) // on the ground, with tire sounds, moving anywhere but straight up/down
		{
			if (TireAudioComp != NULL || WheelParticleEffects.Num() > 0)
			{
				FLOAT curSpd = Velocity.Size2D();
				
				// while moving:
				if(curSpd > 1.0f) // if we're moving or we're on the gas.
				{
					FCheckResult HitRes(1.0f);
					FTraceHitInfo HitInfo;
					FVector TraceStart(Location.X,Location.Y,Location.Z);
					if (CylinderComponent != NULL)
					{
						TraceStart.Z -= CylinderComponent->CollisionHeight - CylinderComponent->Translation.Z;
					}
					FVector EndPt(TraceStart.X,TraceStart.Y,TraceStart.Z-32);  // got these numbers from UTPawn's GetMaterialBelowFeet()

					GWorld->SingleLineCheck(HitRes, this, EndPt, TraceStart, TRACE_World | TRACE_Material);

					DetermineCorrectPhysicalMaterial<FCheckResult, FTraceHitInfo>( HitRes, HitInfo );
					// we now have a phys material so we can see if we need to update the sound
					HitInfo.Material = HitRes.Material ? HitRes.Material->GetMaterial() : NULL;
					UUTPhysicalMaterialProperty* UTPMP = (HitInfo.Material && HitInfo.Material->PhysMaterial)
															? Cast<UUTPhysicalMaterialProperty>(HitInfo.Material->PhysMaterial->PhysicalMaterialProperty)
															: NULL;
					if (TireAudioComp != NULL && UTPMP != NULL && UTPMP->MaterialType != CurrentTireMaterial) // if we're on a material that's not what we're on already.
					{
						INT match = -1;
						for(INT i=0;i<TireSoundList.Num();++i)
						{
							if(TireSoundList(i).MaterialType == UTPMP->MaterialType)
							{
								match = i;
								CurrentTireMaterial = UTPMP->MaterialType;
								break;
							}
						}
						if(match != -1)
						{
							if(TireAudioComp->bWasPlaying) // we have a new match, so fade out the old one and let the garbage collector take care of it.
							{
								TireAudioComp->FadeOut(0.3f,0.0f);
							}
							TireAudioComp = CreateAudioComponent(TireSoundList(match).Sound, FALSE, TRUE, FALSE);
						}
					}
					if (WheelParticleEffects.Num() > 0 && HitRes.Time < 1.0f)
					{
						// figure out the material type, then check any wheels requesting material specific effects and change any that are different
						FName MaterialType = NAME_None;
						// if the trace endpoint is underwater, override with 'water' type
						FMemMark Mark(GMem);
						UBOOL bNowInWater = FALSE;
						for (FCheckResult* Link = GWorld->Hash->ActorPointCheck(GMem, HitRes.Location, FVector(0.f,0.f,0.f), TRACE_PhysicsVolumes); Link != NULL; Link = Link->GetNext())
						{
							APhysicsVolume* Volume = Cast<APhysicsVolume>(Link->Actor);
							if (Volume != NULL && Volume->bWaterVolume)
							{
								bNowInWater = TRUE;
								break;
							}
						}
						Mark.Pop();

						if (bNowInWater)
						{
							MaterialType = FName(TEXT("Water"));
						}
						else if (UTPMP != NULL)
						{
							MaterialType = UTPMP->MaterialType;
						}
						INT EffectIndex = 0;
						if (MaterialType != NAME_None)
						{
							for (INT i = 0; i < WheelParticleEffects.Num(); i++)
							{
								if (WheelParticleEffects(i).MaterialType == MaterialType)
								{
									EffectIndex = i;
									break;
								}
							}
						}

						for (INT i = 0; i < Wheels.Num(); i++)
						{
							UUTVehicleWheel* Wheel = Cast<UUTVehicleWheel>(Wheels(i));
							if ( Wheel != NULL && Wheel->bUseMaterialSpecificEffects && Wheel->WheelParticleComp != NULL
								&& Wheel->WheelParticleComp->Template != WheelParticleEffects(EffectIndex).ParticleTemplate )
							{
								Wheel->eventSetParticleEffect(this, WheelParticleEffects(EffectIndex).ParticleTemplate);
							}
						}
					}
					if (TireAudioComp != NULL)
					{
						if(!TireAudioComp->bWasPlaying)
						{
							TireAudioComp->Play();
						}
						TireAudioComp->AdjustVolume(0.1f, Min<float>(1.0,curSpd/(AirSpeed*0.10f)) ); // go to full volume if >10%, else to the % of 10%
						TireAudioComp->PitchMultiplier = 0.5f + 1.25f*(curSpd/AirSpeed); // 0 = 0.5, 40% = 1.0, 80% = 1.5
					}
				}
				else if (TireAudioComp != NULL) // not moving, stop tires.
				{
					TireAudioComp->Stop();//TireAudioComp->FadeOu(1.0f,0.0f);
				}
			}
		}

		// toggle any wheel effects that only want to be played when the wheel is moving a certain direction
		for (INT i = 0; i < Wheels.Num(); i++)
		{
			UUTVehicleWheel* Wheel = Cast<UUTVehicleWheel>(Wheels(i));
			if (Wheel != NULL && Wheel->WheelParticleComp != NULL && Wheel->EffectDesiredSpinDir != 0.0f)
			{
				UBOOL bActivate = (Wheel->SpinVel / Wheel->EffectDesiredSpinDir > 0.0f);
				if (bActivate != Wheel->WheelParticleComp->bIsActive)
				{
					if (bActivate)
					{
						Wheel->WheelParticleComp->DeactivateSystem();
					}
					else
					{
						Wheel->WheelParticleComp->ActivateSystem();
					}
				}
			}
		}
	}

    if ( Role == ROLE_Authority )
	{
		// if being tracked, update trackers
		if ( Trackers.Num() > 0 )
		{
			FEnemyPosition NewPosition;
			NewPosition.Position = GetTargetLocation();
			NewPosition.Velocity = Velocity;
			NewPosition.Time = WorldInfo->TimeSeconds;

			for ( INT i=0; i<Trackers.Num(); i++ )
			{
				if ( !Trackers(i) || Trackers(i)->bDeleteMe || (Trackers(i)->CurrentlyTrackedEnemy != this) )
				{
					Trackers.Remove(i,1);
					i--;
				}
				else
				{
					Trackers(i)->SavedPositions.AddItem(NewPosition);
				}
			}
		}

		if ( PhysicsVolume && PhysicsVolume->bWaterVolume && (WaterDamage > 0.f) )
		{
			//@todo steve - check that cylinder center is in water
			eventTakeWaterDamage(DeltaSeconds);
			if ( bDeleteMe )
				return;
		}

		// check if vehicle is upside down and on ground
		if ( bIsInverted && bWasChassisTouchingGroundLastTick )
		{
			if ( WorldInfo->TimeSeconds - LastCheckUpsideDownTime > 0.5f )
			{
				if (WorldInfo->TimeSeconds - LastCheckUpsideDownTime > 1.f)
				{
					if ( bIsScraping && ScrapeSound )
					{
						ScrapeSound->Stop();
						bIsScraping = FALSE;
					}
				}

				// Check if we are upside down and touching the level every 0.5 seconds.
				if ( bEjectPassengersWhenFlipped )
				{
					FlippedCount++;
					if ( FlippedCount > 2 )
					{
						if (Driver)
							eventDriverLeave(TRUE);

						for ( INT i=0; i<Seats.Num(); i++ )
							if ( Seats(i).SeatPawn )
								Seats(i).SeatPawn->eventDriverLeave(true);

						FlippedCount = 0;
					}
					LastCheckUpsideDownTime = WorldInfo->TimeSeconds;
				}
			}
			if ( !bCanFlip )
			{
				AccruedFireDamage += UpsideDownDamagePerSec * DeltaSeconds;
			}
		}	
		else
		{
			if ( ScrapeSound )
			{
				if ( bWasChassisTouchingGroundLastTick && (Velocity.SizeSquared() > 200000.f) && (WorldInfo->TimeSeconds - LastCollisionSoundTime > CollisionIntervalSecs) )
				{
					if ( !bIsScraping )
					{
						ScrapeSound->Play();
						bIsScraping = TRUE;
					}
				}
				else if ( bIsScraping )
				{
					ScrapeSound->Stop();
					bIsScraping = FALSE;
				}
			}
			FlippedCount = 0;
		}

		if ( !Controller && (WorldInfo->TimeSeconds - DeltaSeconds < ResetTime) && (WorldInfo->TimeSeconds >= ResetTime) )
		{
			eventCheckReset();
			if ( bDeleteMe )
				return;
		}

		//check for bots in danger of being run over every half a second
		if ( WorldInfo->TimeSeconds - LastRunOverWarningTime > 0.5f )
		{
			FLOAT SpeedSquared = Velocity.SizeSquared();
			if (SpeedSquared > MinRunOverSpeed * MinRunOverSpeed)
			{
				FVector VelNormal = Velocity.SafeNormal();
				FLOAT WarningDistSquared = SpeedSquared * 2.f;
				for ( AController* C=WorldInfo->ControllerList; C!=NULL; C=C->NextController )
					if ( (C != Controller) && C->Pawn && C->PlayerReplicationInfo && !C->Pawn->IsA(AVehicle::StaticClass()) )
					{
						// warn friendly and enemy bots about potentially being run over
						FVector Dir = C->Pawn->Location - Location;
						if ( Dir.SizeSquared() < WarningDistSquared )
						{
							AUTBot *B = Cast<AUTBot>(C);
							if ( B && ((VelNormal | Dir.SafeNormal()) > MinRunOverWarningAim) )
								B->eventReceiveRunOverWarning(this, appSqrt(SpeedSquared), VelNormal);
						}
					}
				LastRunOverWarningTime = WorldInfo->TimeSeconds;
			}
		}

		// fire damage to empty burning vehicles
        if (FLOAT(Health) / FLOAT(GetClass()->GetDefaultObject<AUTVehicle>()->Health) < FireDamageThreshold)
		{
			// check if vehicle is empty
			UBOOL bIsEmpty = TRUE;
  			for ( INT i=0; i<Seats.Num(); i++ )
			{
				if ( Seats(i).SeatPawn && Seats(i).SeatPawn->Controller )
				{
					bIsEmpty = FALSE;
					break;
				}
			}
			if ( bIsEmpty )
			{
				AccruedFireDamage += FireDamagePerSec * DeltaSeconds;
			}
        }
	}

	if ( AccruedFireDamage > 1.f )
	{
		eventTakeFireDamage();
		if ( bDeleteMe )
			return;
	}

    if ( Controller && (Role == ROLE_Authority || IsLocallyControlled()) && Driver )
	{
		// Don't do anything if the pawn is in fixed view
		AUTPawn *UTDriverPawn = Cast<AUTPawn>(Driver);
		if ( UTDriverPawn && !UTDriverPawn->bFixedView)
		{
			FRotator Rot = Controller->Rotation;
			ApplyWeaponRotation(0, Rot);
		}
	}

	// Handle seats making noise when moving.
	if ( WorldInfo->NetMode != NM_DedicatedServer )	
	{
		for (INT i=0;i < Seats.Num(); i++)
		{
			if ( Seats(i).SeatMotionAudio )
			{
				// We have to check rotations as the TurretController might be set to instant
				// and we wouldn't see the motion.
				FRotator CurrentRotation = SeatWeaponRotation(i, FRotator(0,0,0), true);
				UBOOL bInMotion = (CurrentRotation.Vector() - Seats(i).LastWeaponRotation.Vector()).Size() > KINDA_SMALL_NUMBER;

				// Now look at each controller directly and see if it's in motion
				for (INT TCIndex = 0 ; TCIndex  < Seats(i).TurretControllers.Num(); TCIndex++)
				{
					if  ( Seats(i).TurretControllers(TCIndex)->bIsInMotion )
					{
						bInMotion = true;
						break;
					}
				}

				// Handle it
				if ( bInMotion )
				{
					if (!Seats(i).SeatMotionAudio->bWasPlaying || Seats(i).SeatMotionAudio->bFinished)
					{
						Seats(i).SeatMotionAudio->Play();
					}
				}
				else
				{
					// To avoid annoying sound-bites, we insure the sound has played for 150ms at least before stopping it
					if ( (Seats(i).SeatMotionAudio->bWasPlaying || !Seats(i).SeatMotionAudio->bFinished) && Seats(i).SeatMotionAudio->PlaybackTime > 0.15  )
					{
						Seats(i).SeatMotionAudio->Stop();
					}
				}
				Seats(i).LastWeaponRotation = CurrentRotation;
			}
		}
	}
}

static FTakeHitInfo OldLastTakeHitInfo;
static FLOAT OldHealth;

void AUTVehicle::PreNetReceive()
{
	Super::PreNetReceive();

	OldLastTakeHitInfo = LastTakeHitInfo;
	OldHealth = Health;
}

void AUTVehicle::PostNetReceive()
{
	Super::PostNetReceive();

	//@note: the ordering of these two checks is important here because we'd like the client to try to figure out everything using the LastTakeHitInfo where possible,
	//		and only use the direct health change for corrections
	if (OldLastTakeHitInfo != LastTakeHitInfo)
	{
		eventPlayTakeHitEffects();
	}
	if (OldHealth != Health)
	{
		eventReceivedHealthChange();
	}
}

/**
 * This function will calculate the current firing vector and create a turret rotation from it.  
 *
 * @param	SeatIndex	- The Seat we are calc'ing the rotation for
 * @param	NewRotation	- The new Pawn Rotation.  This is ignored if there is a controlled
 */

void AUTVehicle::ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation)
{
	if (Seats.IsValidIndex(SeatIndex) && Seats(SeatIndex).SeatPawn)
	{
		// @HACK - We don't want to have to replicate the entire seats array, so when we see that the 
		// vehicle has a gun, steal it if we are seat 0.
		if (SeatIndex == 0 && Weapon && !Seats(SeatIndex).Gun)
		{
			Seats(SeatIndex).Gun = Cast<AUTVehicleWeapon>(Weapon);
		}

		AController* C = Seats(SeatIndex).SeatPawn->Controller;

		Seats(SeatIndex).AimTarget = NULL;

		if ( C )
		{
			APlayerController* PC = C->GetAPlayerController();
			FVector AimPoint;

			if ( PC )
			{
				FVector CamLoc;
				FRotator CamRot;
				PC->eventGetPlayerViewPoint(CamLoc, CamRot);

				FLOAT TraceRange;
				TArray<AActor*> IgnoredActors;
				if (Seats(SeatIndex).Gun != NULL)
				{
					TraceRange = Seats(SeatIndex).Gun->AimTraceRange;
					// turn off bProjTarget on Actors we should ignore for the aiming trace
					for (INT i = 0; i < Seats(SeatIndex).Gun->AimingTraceIgnoredActors.Num(); i++)
					{
						AActor* IgnoredActor = Seats(SeatIndex).Gun->AimingTraceIgnoredActors(i);
						if (IgnoredActor != NULL && IgnoredActor->bProjTarget)
						{
							IgnoredActor->bProjTarget = FALSE;
							IgnoredActors.AddItem(IgnoredActor);
						}
					}
				}
				else
				{
					TraceRange = 5000.0f;
				}

				AimPoint = CamLoc + CamRot.Vector() * TraceRange;

				FCheckResult Hit(1.0f);
				if (!GWorld->SingleLineCheck(Hit, this, AimPoint, CamLoc, TRACE_ProjTargets | TRACE_ComplexCollision))
				{
					AimPoint = Hit.Location;
				}

				// Cache who we are aiming at

				Seats(SeatIndex).AimPoint  = AimPoint;
				Seats(SeatIndex).AimTarget = Hit.Actor;

				// restore bProjTarget on Actors we turned it off for
				for (INT i = 0; i < IgnoredActors.Num(); i++)
				{
					IgnoredActors(i)->bProjTarget = TRUE;
				}
			}
			else 
			{
				AimPoint = C->FocalPoint;
			}

			FVector Pivot = GetSeatPivotPoint(SeatIndex);

			NewRotation = (AimPoint - Pivot).Rotation();

			// Set the value

			SeatWeaponRotation(SeatIndex, NewRotation, false);
		}

		for (INT i=0;i<Seats(SeatIndex).TurretControllers.Num(); i++)
		{
			Seats(SeatIndex).TurretControllers(i)->DesiredBoneRotation = NewRotation;
		}
	}
}

/** 
 * Returns the pivot point to use for a given turret
 *
 * @Param	SeatIndex	- The Seat to look up
 * @returns a locational vector of the pivot point
 */
FVector AUTVehicle::GetSeatPivotPoint(INT SeatIndex)
{
	INT BarrelIndex = GetBarrelIndex(SeatIndex);
	INT ArrayLen = Seats(SeatIndex).GunPivotPoints.Num();

	if ( Mesh && ArrayLen > 0 )
	{
		if ( BarrelIndex >= ArrayLen )
		{
			BarrelIndex = ArrayLen - 1;
		}

		FName Pivot = Seats(SeatIndex).GunPivotPoints(BarrelIndex);
		return Mesh->GetBoneLocation(Pivot);
	}
	else
	{
		return Location;
	}
}

/** 
 * Returns the pivot point to use for a given turret
 *
 * @Param	SeatIndex	- The Seat to look up
 * @returns a locational vector of the pivot point
 */
FVector AUTVehicle_Raptor::GetSeatPivotPoint(INT SeatIndex)
{
	FVector Loc;
	if (Mesh)
	{
		Mesh->GetSocketWorldLocationAndRotation(TurretPivotSocketName,Loc,NULL);
		return Loc;
	}
	else 
	{
		return Location;
	}
}

/** 
 * Returns the index of the current "in use" barrel
 *
 * @Param	SeatIndex	- The Seat to look up
 * @returns the index of the barrel that will be used for the next shot
 */
INT AUTVehicle::GetBarrelIndex(INT SeatIndex)
{
	if ( Seats(SeatIndex).GunSocket.Num() < 0 )
	{
		return 0;
	}
	else
	{
		return Seats(SeatIndex).GunSocket.Num() > 0 ? Seats(SeatIndex).BarrelIndex % Seats(SeatIndex).GunSocket.Num() : 0;
	}
}


/**
 * This function is used by Vehicle Factories to force the rotation on a vehicle's turrets
 *
 * @param	SeatIndex	- The Seat we are calc'ing the rotation for
 * @param	NewRotation	- The new Pawn Rotation.  
 */
void AUTVehicle::ForceWeaponRotation(INT SeatIndex,FRotator NewRotation)
{
	ApplyWeaponRotation(SeatIndex, NewRotation);
}

void AUTVehicle_Goliath::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	// client side effects follow - return if server
	//@todo steve - skip if far away
	if ( LastRenderTime < WorldInfo->TimeSeconds - 0.2f || bDeadVehicle )
		return;

	FLOAT WheelRadius = Wheels(0)->WheelRadius;

	FLOAT LeftTrackSpeed = ( Cast<USVehicleSimTank>(SimObj)->LeftTrackVel * WheelRadius );
	FLOAT RightTrackSpeed = ( Cast<USVehicleSimTank>(SimObj)->RightTrackVel * WheelRadius );

	LeftTreadSpeed += DeltaSeconds * (LeftTrackSpeed * 0.001f);
	RightTreadSpeed += DeltaSeconds * (RightTrackSpeed * 0.001f);

	// pan tread materials
	if ( LeftTreadMaterialInstance )
	{
		LeftTreadMaterialInstance->SetScalarParameterValue(TreadSpeedParameterName, LeftTreadSpeed);
	}
	if ( RightTreadMaterialInstance )
	{
		RightTreadMaterialInstance->SetScalarParameterValue(TreadSpeedParameterName, RightTreadSpeed); //RightTrackSpeed * TexPan2UnrealScale);
	}

	// rotate other wheels
	if ( Velocity.SizeSquared() > 0.01 )
	{											
		FLOAT LeftRotation = Wheels(3)->CurrentRotation;
		USkelControlWheel* SkelControl = Cast<USkelControlWheel>( Mesh->FindSkelControl(LeftBigWheel) );
		if(SkelControl)
		{
			SkelControl->UpdateWheelControl( 0.f, LeftRotation, 0.f); //LeftBigWheelRot, 0.f );
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
			SkelControl->UpdateWheelControl( 0.f, RightRotation, 0.f); 
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

	// --Dave-TODO
	//if ( Driver )
	//{		
	//	FLOAT EngineRPM = Cast<UUTVehicleSimCar>(SimObj)->EngineRPMCurve.Eval(ForwardVel, 0.0f);
	//	FName EffectName(TEXT("goliathexhaust"));
	//	VehicleEffects(2).EffectRef->SetFloatParameter(EffectName,EngineRPM);
	//	VehicleEffects(3).EffectRef->SetFloatParameter(EffectName,EngineRPM);
	//}
}

/** Returns a float representative of the vehicle's engine output. */
float UUTVehicleSimCar::GetEngineOutput(ASVehicle* Vehicle)
{
	return EngineRPMCurve.Eval(Vehicle->ForwardVel, 0.0f);
}

/** Grab the view direction as well, for LookToSteer support. */
void UUTVehicleSimCar::ProcessCarInput(ASVehicle* Vehicle)
{
	Super::ProcessCarInput(Vehicle);

	if ( Vehicle->IsHumanControlled() )
	{			
		Vehicle->DriverViewPitch = Vehicle->Controller->Rotation.Pitch;
		Vehicle->DriverViewYaw = Vehicle->Controller->Rotation.Yaw;
	}
	else
	{
		Vehicle->DriverViewPitch = Vehicle->Rotation.Pitch;
		Vehicle->DriverViewYaw = Vehicle->Rotation.Yaw;
	}
}

void UUTVehicleSimCar::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
	if( Vehicle->bDriving )
	{
		FLOAT Speed = Abs(Vehicle->ForwardVel);

		// Tweaks to grip for various reasons
		FLOAT LongGripScale = 1.f;

		/////////// STEERING ///////////
		FLOAT maxSteerAngle = MaxSteerAngleCurve.Eval(Vehicle->Velocity.Size(), 0.f);

		// change steering if not all wheels on ground
		INT NumWheelsOnGround = 0;
		for ( INT i=0; i<Vehicle->Wheels.Num(); i++ )
		{
			if ( Vehicle->Wheels(i)->bWheelOnGround )
			{
				NumWheelsOnGround++;
			}
		}
		UBOOL bForceHandbrake = FALSE;
		if ( (NumWheelsOnGround < NumWheelsForFullSteering) && (Speed > StopThreshold) )
		{
			FLOAT RampupRateMultiplier = 1.f;
			Vehicle->OutputGas *= 0.3f;
			if ( NumWheelsOnGround < NumWheelsForFullSteering - 1 ) 
			{
				Vehicle->OutputGas *= 0.2f;
				RampupRateMultiplier = 3.f;

				if ( bAutoHandbrake && (NumWheelsOnGround == 2) && (Vehicle->Wheels.Num() == 4) 
					&& (Vehicle->Wheels(0)->bWheelOnGround == Vehicle->Wheels(2)->bWheelOnGround) )
				{
					// if both on same side, apply handbrake
					bForceHandbrake = TRUE;
					Vehicle->bOutputHandbrake = TRUE; // @TODO FIXMESTEVE - turn off to let Phil tweak tire friction
				}
			}
			CurrentSteeringReduction = ::Max(SteeringReductionFactor, CurrentSteeringReduction - RampupRateMultiplier*SteeringReductionRampUpRate*DeltaTime);
			if ( Speed > SteeringReductionMinSpeed )
			{
				if ( Speed > SteeringReductionSpeed )
					maxSteerAngle *= CurrentSteeringReduction; 
				else
					maxSteerAngle *= (1.f - (Speed-SteeringReductionMinSpeed)/(SteeringReductionSpeed-SteeringReductionMinSpeed) * (1.f - CurrentSteeringReduction)); 
			}
			else
			{
				CurrentSteeringReduction = ::Min(1.f,CurrentSteeringReduction + 0.5f*SteeringReductionRampUpRate*DeltaTime);
			}
		}
		else
		{
			CurrentSteeringReduction = ::Min(1.f,CurrentSteeringReduction + 0.5f*SteeringReductionRampUpRate*DeltaTime);
		}

		FLOAT maxSteer = DeltaTime * SteerSpeed;
		FLOAT deltaSteer;
		AUTVehicle* UTV = Cast<AUTVehicle>(Vehicle);
		if(UTV && !UTV->ForceMovementDirection.IsZero() )
		{
			FRotationMatrix R(Vehicle->Rotation);
			FVector Forwards = R.GetAxis(0);
			// Steering:
			FLOAT DesiredSteer = FindDeltaAngle(HeadingAngle(Forwards),HeadingAngle(UTV->ForceMovementDirection))*RAD_TO_DEG;
			ActualSteering = Clamp<FLOAT>(DesiredSteer,-maxSteer,maxSteer);
			if(Vehicle->OutputGas < 0.f) // wheeled vehicles need to steer the other way when going in reverse.
			{
				ActualSteering *= -1.0f;
			}
			FLOAT DotProduct = Forwards | UTV->ForceMovementDirection;
			// don't allow gas in opposite direction of force movement.
			if((!UTV->bForceDirectionAllowedNegative && ((DotProduct < (1.0-KINDA_SMALL_NUMBER)) && (DotProduct < 0.f && Vehicle->OutputGas > 0.f) || (DotProduct > 0.f && Vehicle->OutputGas < 0.f))))
			{
				Vehicle->OutputGas = 0.0f; // GOOD IDEA: We have to move to steer, but this would take us farther 'out', go the other way player! :)
				Vehicle->OutputBrake = 1.0f;
			}
			//Vehicle->OutputGas *= Abs(DotProduct); // BAD IDEA: We have to move to steer, so cutting gas while we correct isn't good.
		}
		else
		{
			deltaSteer = (-Vehicle->OutputSteering * maxSteerAngle) - ActualSteering; // Amount we want to move (target - current)
			deltaSteer = Clamp<FLOAT>(deltaSteer, -maxSteer, maxSteer);
			ActualSteering += deltaSteer;
		}
	 
		/////////// THROTTLE ///////////
		// scale throttle speed used based on current speed
		FRotationMatrix R(Vehicle->Rotation);
		FLOAT CurrentThrottleSpeed = ThrottleSpeed * ::Max<FLOAT>(Square(Square((Vehicle->AirSpeed - Speed)/Vehicle->AirSpeed)), 0.1f);
		FLOAT OldActualThrottle = ActualThrottle;
		if ( (Vehicle->OutputGas == 0.f) || ((Vehicle->OutputGas > 0.f) != (ActualThrottle > 0.f)) )
			bHasForcedThrottle = FALSE;

		if ( Vehicle->OutputGas >= 0.f )
		{
			ActualThrottle = ::Max(ActualThrottle, 0.f);
			if ( Vehicle->ForwardVel <= 0.f )
			{
				bForceThrottle = TRUE;
				ActualThrottle = ::Min<FLOAT>(ActualThrottle + DeltaTime, Vehicle->OutputGas);	
			}
			else
			{
				if ( bForceThrottle && !bHasForcedThrottle )
				{
					ActualThrottle = ::Min<FLOAT>(CurrentThrottleSpeed*DeltaTime, Vehicle->OutputGas);	
					bHasForcedThrottle = TRUE;
				}
				else if ( Vehicle->OutputGas <= ActualThrottle )
				{
					ActualThrottle = Vehicle->OutputGas;
				}
				else
				{
					ActualThrottle = ::Min<FLOAT>(ActualThrottle + CurrentThrottleSpeed * DeltaTime, Vehicle->OutputGas);	
				}
				bForceThrottle = FALSE;
			}
		}
		else
		{
			if ( Vehicle->ForwardVel >= 0.f )
			{
				bForceThrottle = TRUE;
				ActualThrottle = ::Max<FLOAT>(ActualThrottle - DeltaTime, Vehicle->OutputGas);	
			}
			else
			{
				if ( bForceThrottle && !bHasForcedThrottle )
				{
					ActualThrottle = ::Max<FLOAT>(-1.f * CurrentThrottleSpeed*DeltaTime, Vehicle->OutputGas);	
					bHasForcedThrottle = TRUE;
				}
				else if ( Vehicle->OutputGas >= ActualThrottle )
				{
					ActualThrottle = Vehicle->OutputGas;
				}
				else
				{
					ActualThrottle = ::Max<FLOAT>(ActualThrottle - CurrentThrottleSpeed * DeltaTime, Vehicle->OutputGas);	
				}
				bForceThrottle = FALSE;
			}
		}

		/////////// TORQUE CURVE APPROXIMATION //////////////

		FLOAT HardTurnGripScaling = 1.f;

		// Braking
		FLOAT BrakeTorque = Vehicle->OutputBrake * MaxBrakeTorque;

		// Torque
		FLOAT TorqueEval = TorqueVSpeedCurve.Eval(Vehicle->ForwardVel, 0.0f);;
		FLOAT MotorTorque = ActualThrottle * TorqueEval;
		if ( (Vehicle->ForwardVel > Vehicle->AirSpeed) && ((Vehicle->Velocity | Vehicle->Rotation.Vector()) > 0.f) )
		{
			// force vehicle to slow down if above airspeed limit
			MotorTorque = 0.f;
			BrakeTorque = MaxBrakeTorque;
		}
		else if ( ActualThrottle == 0.0f )
		{
			if ( Speed > StopThreshold )
			{
				MotorTorque -= EngineBrakeFactor * Vehicle->ForwardVel;				
			}
		}
		else if ( (Abs(Vehicle->OutputSteering) > 0.6f) && (Abs(Vehicle->ForwardVel) > MinHardTurnSpeed) 
			&& ((Vehicle->OutputGas > 0.f) == (Vehicle->ForwardVel > 0.f))
			&& (!Vehicle->bOutputHandbrake || bForceHandbrake) )
		{
			// reduce torque and throttle if turning hard
			FLOAT HardPct = 2.5f * (Abs(Vehicle->OutputSteering) - 0.6f);
			MotorTorque *= Lerp(1.f, HardTurnMotorTorque, HardPct);
			ActualThrottle = OldActualThrottle;

			// Reduce grip as well on console
			if(GWorld->GetWorldInfo()->bUseConsoleInput)
			{
				HardTurnGripScaling = Lerp(1.f, ConsoleHardTurnGripFactor, HardPct);
			}
		}

		// Lose torque when climbing too steep
		if ( R.GetAxis(2).Z < Vehicle->WalkableFloorZ )
		{
			MotorTorque = 0.f;
		}

		FLOAT TotalSpinVel = 0.0f;
		if (LSDFactor > 0.0f)
		{
			for(INT i=0; i<Vehicle->Wheels.Num(); i++)
			{
				USVehicleWheel* vw = Vehicle->Wheels(i);

				// Accumulate wheel spin speeds to use for LSD
				TotalSpinVel += vw->SpinVel;
			}
		}

		FLOAT TotalMotorTorque = MotorTorque * Vehicle->NumPoweredWheels;
		FLOAT TotalBrakeTorque = BrakeTorque * Vehicle->NumPoweredWheels;
		FLOAT EvenSplit = 1.f/(FLOAT)Vehicle->NumPoweredWheels;



		// Reduce wheel grip when colliding with things head on
		if(UTV->bFrontalCollision)
		{
			LongGripScale *= FrontalCollisionGripFactor;
		}

		// Do model for each wheel.
		for(INT i=0; i<Vehicle->Wheels.Num(); i++)
		{
			USVehicleWheel* vw = Vehicle->Wheels(i);

			if (vw->bPoweredWheel)
			{
				/////////// LIMITED SLIP DIFFERENTIAL ///////////

				// Heuristic to divide torque up so that the wheels that are spinning slower get more of it.
				// Sum of LSDFactor across all wheels should be 1.
				FLOAT LSDSplit, UseSplit;

				if (LSDFactor > 0.0f)
				{	
					// If no wheels are spinning, just do an even split.
					if(TotalSpinVel > 0.1f)
						LSDSplit = (TotalSpinVel - vw->SpinVel)/(((FLOAT)Vehicle->NumPoweredWheels-1.f) * TotalSpinVel);
					else
						LSDSplit = EvenSplit;

					UseSplit = ((1-LSDFactor) * EvenSplit) + (LSDFactor * LSDSplit);
				}
				else
					UseSplit = EvenSplit;

				vw->BrakeTorque = UseSplit * TotalBrakeTorque;
				vw->MotorTorque = UseSplit * TotalMotorTorque;

				// Calculate torque applied back to chassis if wheel is on the ground
				if (vw->bWheelOnGround)
					vw->ChassisTorque = -1.0f * vw->MotorTorque * ChassisTorqueScale;
				else
					vw->ChassisTorque = 0.0f;

	#if WITH_NOVODEX // Update WheelShape in case it changed due to handbrake
				NxWheelShape* WheelShape = vw->GetNxWheelShape();
				check(WheelShape);	
				
				FLOAT LatGripScale = 1.f;
				if(vw->SteerFactor == 0.f)
				{
					LatGripScale *= HardTurnGripScaling;
				}

				SetNxWheelShapeParams(WheelShape, vw, LongGripScale, LatGripScale);
	#endif // WITH_NOVODEX
			}

			/////////// STEERING  ///////////

			// Pass on steering to wheels that want it.
			vw->Steer = ActualSteering * vw->SteerFactor;
		}
	}
	else
	{
		// no driver - just brake to a stop
		FLOAT TotalMotorTorque = 0.f;
		FLOAT TotalBrakeTorque = 0.f;
		FLOAT EvenSplit = 1.f/(FLOAT)Vehicle->NumPoweredWheels;

		if ( bDriverlessBraking )
		{
			if ( Abs(Vehicle->ForwardVel) > StopThreshold && Vehicle->bVehicleOnGround && !Vehicle->bIsInverted)
			{
				TotalMotorTorque -= EngineBrakeFactor * Vehicle->ForwardVel;
				TotalMotorTorque *= Vehicle->NumPoweredWheels;
			}
			else
			{
				TotalBrakeTorque = MaxBrakeTorque * Vehicle->NumPoweredWheels;
			}
		}

		// Do model for each wheel.
		for(INT i=0; i<Vehicle->Wheels.Num(); i++)
		{
			USVehicleWheel* vw = Vehicle->Wheels(i);

			if (vw->bPoweredWheel)
			{
				vw->BrakeTorque = EvenSplit * TotalBrakeTorque;
				vw->MotorTorque = EvenSplit * TotalMotorTorque;

				// Calculate torque applied back to chassis if wheel is on the ground
				if (vw->bWheelOnGround)
					vw->ChassisTorque = -1.0f * vw->MotorTorque * ChassisTorqueScale;
				else
					vw->ChassisTorque = 0.0f;
			}
			if ( Vehicle->bUpdateWheelShapes )
			{
#if WITH_NOVODEX 
				NxWheelShape* WheelShape = vw->GetNxWheelShape();
				check(WheelShape);	
				SetNxWheelShapeParams(WheelShape, vw);
#endif // WITH_NOVODEX 
			}
			vw->Steer = 0.f;
		}
		Vehicle->bUpdateWheelShapes = FALSE;
	}
}

void UUTVehicleSimCar::UpdateHandbrake(ASVehicle* Vehicle)
{
	// If we are not forcibly holding down the handbrake for some reason (Scorpion boost),
	// pressing 'down' (ie making Rise negative) turns on the handbrake.
	if ( !Vehicle->bHoldingDownHandbrake )
		Vehicle->bOutputHandbrake = (Vehicle->Rise < 0.f);
}

void AUTHoverVehicle::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if (WorldInfo->NetMode != NM_DedicatedServer && bDriving && Mesh != NULL && !bDeadVehicle)
	{
		for (INT i = 0; i < GroundEffectIndices.Num(); i++)
		{
			const INT GroundEffectIndex = GroundEffectIndices(i);
			if (GroundEffectIndex < VehicleEffects.Num() && VehicleEffects(GroundEffectIndex).EffectRef != NULL && VehicleEffects(GroundEffectIndex).EffectSocket != NAME_None)
			{
				FVector Start;
				Mesh->GetSocketWorldLocationAndRotation(VehicleEffects(GroundEffectIndex).EffectSocket, Start, NULL);
				FVector End = Start + (FVector(0,0,-1) * MaxGroundEffectDist);
				FCheckResult Hit(1.0f);
				if (!GWorld->SingleLineCheck(Hit, this, End, Start, TRACE_World))
				{	
					FVector Offset = Hit.Location - Start;
					VehicleEffects(GroundEffectIndex).EffectRef->Translation = Offset;
					
					FLOAT DistPerc = Offset.Size() / MaxGroundEffectDist;
					VehicleEffects(GroundEffectIndex).EffectRef->SetFloatParameter(GroundEffectDistParameterName, DistPerc);

					if (VehicleEffects(GroundEffectIndex).EffectRef->HiddenGame)
					{
						VehicleEffects(GroundEffectIndex).EffectRef->SetActive(TRUE);
						VehicleEffects(GroundEffectIndex).EffectRef->SetHiddenGame(FALSE);
					}
					else
					{
						VehicleEffects(GroundEffectIndex).EffectRef->ConditionalUpdateTransform();
					}
				}
				else if (!VehicleEffects(GroundEffectIndex).EffectRef->HiddenGame)
				{
					VehicleEffects(GroundEffectIndex).EffectRef->SetActive(FALSE);
					VehicleEffects(GroundEffectIndex).EffectRef->SetHiddenGame(TRUE);
				}
			}
		}
	}
}

void AUTAirVehicle::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if ( bDriving && Mesh )
	{
		if ( (Role == ROLE_Authority) && (Velocity.SizeSquared() < 4900.f) && !Cast<AAIController>(Controller) )
		{
			// random jostling if hovering
			FVector Jostle(appFrand()*2.f - 1.f,appFrand()*2.f - 1.f,appFrand()*2.f - 1.f);
			Jostle = 200.f * Jostle.SafeNormal();
			Mesh->AddImpulse(Jostle, Location);
		}
		if ( WorldInfo->NetMode != NM_DedicatedServer && !bDeadVehicle )
		{
			// contrails
			if (ContrailEffectIndices.Num() > 0)
			{
				FColor ContrailColor(0,0,0);
				if ( Velocity.SizeSquared() > 250000.f )
				{
					//FLOAT V = Clamp<FLOAT>((Velocity.Size() - 500) / 2000, 0.0, 1.0);
					INT RGB = INT (255.0f * 1.0);// * V);
					ContrailColor = FColor(RGB, RGB, RGB);
				}

				for (INT i = 0; i < ContrailEffectIndices.Num(); i++)
				{
					const INT ContrailIndex = ContrailEffectIndices(i);
					if (ContrailIndex < VehicleEffects.Num() && VehicleEffects(ContrailIndex).EffectRef != NULL)
					{
						VehicleEffects(ContrailIndex).EffectRef->SetColorParameter(ContrailColorParameterName, ContrailColor);
					}
				}
			}
		}
	}
}

void AUTVehicle_Manta::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( bDeadVehicle )
		return;

	AirSpeed = FullAirSpeed;

	if (Controller != NULL)
	{
		// trying to jump
		if ( Rise > 0.f )
		{
			if (!bHoldingDuck && WorldInfo->TimeSeconds - JumpDelay >= LastJumpTime)
			{
				// Calculate Local Up Vector
				FRotationMatrix R(Rotation);
				FVector DirZ = R.GetAxis(2);

				FVector LocalUp = DirZ;
				LocalUp.Normalize();
				
				UBOOL bIsOverJumpableSurface = FALSE;

				// Don't jump if we are at a steep angle
				if ((LocalUp | FVector(0.0f,0.0f,1.0f)) > 0.5f)
				{
					// Otherwise make sure there is ground underneath
					// use an extent trace so what we hit matches up with what the vehicle and physics collision would hit
					FCheckResult Hit(1.0f);
					bIsOverJumpableSurface = !GWorld->SingleLineCheck(Hit, this, Location - (LocalUp * JumpCheckTraceDist), Location, TRACE_AllBlocking, FVector(1.f, 1.f, 1.f));
				}
				if (bIsOverJumpableSurface)
				{
					// If we are on the ground, and press Rise, and we not currently in the middle of a jump, start a new one.
					if (Role == ROLE_Authority)
					{
   						bDoBikeJump = !bDoBikeJump;
					}
					eventMantaJumpEffect();

					if ( Cast<AUTBot>(Controller) )
    					Rise = 0.f;

					AddImpulse( FVector(0.f,0.f,JumpForceMag) );
					LastJumpTime = WorldInfo->TimeSeconds;
					//debugf(TEXT("Jump with VelZ %f LocZ %f Speed %f"), Velocity.Z, Location.Z, Velocity.Size());
				}
			}
			bNoZDamping = (WorldInfo->TimeSeconds - 0.25f < LastJumpTime);
		}
		else if ( (Rise < 0.f) || bPressingAltFire )
		{
			// Duck!
			AirSpeed = CrouchedAirSpeed;
			if ( !bHoldingDuck )
			{
				bHoldingDuck = true;

				eventMantaDuckEffect();
				
				if ( Cast<AUTBot>(Controller) )
    				Rise = 0.f;
    		}
		}
		else if (bHoldingDuck)
		{
			bHoldingDuck = false;
			eventMantaDuckEffect();
		}
/*		if ( !HasWheelsOnGround() && (Abs(Velocity.Z) < 50.f) )
		{
			debugf(TEXT("In air at LocZ %f"),Location.Z);
		}
*/
	}

	if ( bDriving )
	{
		FLOAT DesiredSuspensionTravel = FullWheelSuspensionTravel;
		if ( bHoldingDuck )
		{
			AddForce( FVector(0.f,0.f,DuckForceMag) );
			DesiredSuspensionTravel = CrouchedWheelSuspensionTravel;
		}

		FLOAT ClampedDeltaSeconds = ::Min(DeltaSeconds, 0.1f);
		for (INT i=0; i<Wheels.Num(); i++)
		{
			UBOOL bAdjustSuspension = false;

			if ( Wheels(i)->SuspensionTravel > DesiredSuspensionTravel )
			{
				// instant crouch
				bAdjustSuspension = true;
				Wheels(i)->SuspensionTravel = DesiredSuspensionTravel;
				SimObj->WheelSuspensionStiffness = CrouchedWheelSuspensionStiffness;
			}
			else if ( Wheels(i)->SuspensionTravel < DesiredSuspensionTravel )
			{
				// slow rise
				bAdjustSuspension = true;
				Wheels(i)->SuspensionTravel = ::Min(DesiredSuspensionTravel, Wheels(i)->SuspensionTravel + SuspensionTravelAdjustSpeed*ClampedDeltaSeconds);
				SimObj->WheelSuspensionStiffness = FullWheelSuspensionStiffness;
			}
			if ( bAdjustSuspension )
			{
				Wheels(i)->BoneOffset.Z = -1.f * (Wheels(i)->SuspensionTravel + Wheels(i)->WheelRadius + BoneOffsetZAdjust); 
	#if WITH_NOVODEX
				NxWheelShape* WheelShape = Wheels(i)->GetNxWheelShape();
				SimObj->SetNxWheelShapeParams(WheelShape, Wheels(i));
	#endif
			}
		}
	}

	if ( WorldInfo->NetMode != NM_DedicatedServer )
	{
		// client side only effects
		if (FanEffectIndex >= 0 && FanEffectIndex < VehicleEffects.Num() && VehicleEffects(FanEffectIndex).EffectRef != NULL)
		{
			if ( bDriving )
			{
				if ( !Velocity.IsZero() )
				{
					DesiredBladeBlur = BladeBlur = 1.f + Clamp<FLOAT>( (0.002f * Velocity.Size()), 0.f, 2.f);
				}
				else
				{
					DesiredBladeBlur = 1.f;
				}
			}
			else
			{
				DesiredBladeBlur = 0.0f;
			}
		
			if (BladeBlur!=DesiredBladeBlur)
			{
				if (BladeBlur > DesiredBladeBlur)
				{
					BladeBlur = Clamp<FLOAT>( (BladeBlur-DeltaSeconds), DesiredBladeBlur,3);
				}
				else
				{
					BladeBlur = Clamp<FLOAT>( (BladeBlur+DeltaSeconds), 0, DesiredBladeBlur);
				}
			}
			VehicleEffects(FanEffectIndex).EffectRef->SetFloatParameter(FanEffectParameterName, BladeBlur);
		}
	}
}

UBOOL AUTVehicle_Cicada::HasRelevantDriver()
{
	return (Driver != NULL || (Seats.Num() > 1 && Seats(1).SeatPawn != NULL && Seats(1).SeatPawn->Controller != NULL));
}

void AUTVehicle_Cicada::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if ( bDriving )
	{
		if ( !Controller ) 
		{
			if (Seats.Num() > 1 && Seats(1).SeatPawn != NULL && Seats(1).SeatPawn->Controller != NULL && (Location.Z < WorldInfo->StallZ) )
			{
				// if turret passenger, try to bring vehicle to a halt
				Rise = Clamp<FLOAT>((-1.f * Velocity.Z)/GetMaxRiseForce(), -1.f, 1.f);
			}
		}
		else 
		{
			AUTBot* Bot = Cast<AUTBot>(Controller);
			if ( Bot )
			{
				if ( Bot->bScriptedFrozen )
				{
					Rise = Clamp<FLOAT>((-1.f * Velocity.Z) / GetMaxRiseForce(), 0.f, 1.f);
				}
				else if ( Bot->GetStateFrame()->LatentAction != UCONST_LATENT_MOVETOWARD )
				{
					if (Rise < 0.f)
					{
						if (Velocity.Z < 0.f)
						{
							if (Velocity.Z < -1000.f)
							{
								Rise = -0.001f;
							}
							//@todo steve - use dist to adjust down as get closer
							FCheckResult Hit(1.0f);
							GWorld->SingleLineCheck(Hit, this, Location - FVector(0.f, 0.f, 2000.f), Location, TRACE_World);
							if (Hit.Actor != NULL)
							{
								if ((Location.Z - Hit.Location.Z) / (-1.f * Velocity.Z) < 0.85f)
								{
									Rise = 1.f;
								}
							}
						}
					}
					else if (Rise == 0.f)
					{
						FCheckResult Hit(1.0f);
						if (!GWorld->SingleLineCheck(Hit, this, Location - FVector(0.f, 0.f, 500.f), Location, TRACE_World | TRACE_StopAtAnyHit))
						{
							Rise = Clamp<FLOAT>((-1.f * Velocity.Z) / GetMaxRiseForce(), 0.f, 1.f);
						}
					}
				}
			}
		}
	}

	if (LastRenderTime > WorldInfo->TimeSeconds - 0.2f)
	{
		if (JetControl != NULL)
		{
			FLOAT JetScale = Clamp<FLOAT>(1-JetControl->ControlStrength, 0.2f , 1.0f);
			for (INT i=0;i<JetEffectIndices.Num();i++)
			{
				if ( JetEffectIndices(i) < VehicleEffects.Num() && VehicleEffects(i).EffectRef )
				{
					VehicleEffects( JetEffectIndices(i) ).EffectRef->SetFloatParameter(JetScalingParam, JetScale);
				}
			}
		}
	}
}

void UUTVehicleSimChopper::ProcessCarInput(ASVehicle* Vehicle)
{
	if( !Vehicle->HasRelevantDriver() )
	{
		Vehicle->OutputBrake = 1.0f;
		Vehicle->OutputGas = 0.0f;
		Vehicle->bOutputHandbrake = FALSE;
		Vehicle->OutputSteering = 0.0;
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

void UUTVehicleSimHoverTank::ProcessCarInput(ASVehicle* Vehicle)
{
	if( !Vehicle->HasRelevantDriver() )
	{
		Vehicle->OutputBrake = 1.0f;
		Vehicle->OutputGas = 0.0f;
		Vehicle->bOutputHandbrake = TRUE;
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

	// Grab the view direction as well, for LookToSteer support.
	if ( Vehicle->IsHumanControlled() )
	{			
		Vehicle->DriverViewPitch = Vehicle->Controller->Rotation.Pitch;
		Vehicle->DriverViewYaw = Vehicle->Controller->Rotation.Yaw;
	}
	else
	{
		Vehicle->DriverViewPitch = Vehicle->Rotation.Pitch;
		Vehicle->DriverViewYaw = Vehicle->Rotation.Yaw;
	}
}

// HACK TJAMES HACK (For Gamers Day), we should get a real RPM system
float UUTVehicleSimHoverTank::GetEngineOutput(ASVehicle* Vehicle)
{
	return ((Vehicle->Velocity.Size())/(Vehicle->MaxSpeed))*10000;
}

// HACK TJAMES HACK (For Gamers Day), we should get a real RPM system
float UUTVehicleSimChopper::GetEngineOutput(ASVehicle* Vehicle)
{
	return ((Vehicle->Velocity.Size())/(Vehicle->MaxSpeed))*5000;
}

// HACK TJAMES HACK (For Gamers Day), we should get a real RPM system

float UUTVehicleSimHover::GetEngineOutput(ASVehicle* Vehicle)
{
	return ((Vehicle->Velocity.Size())/(Vehicle->MaxSpeed))*5000;
}

/**
  * Returns "forward" rotation used as frame of reference for applying vehicle forces
  */
void UUTVehicleSimChopper::GetRotationAxes(ASVehicle* Vehicle, FVector &DirX, FVector &DirY, FVector &DirZ)
{
	FRotationMatrix R(Vehicle->Rotation);
	DirX = R.GetAxis(0);
	DirY = R.GetAxis(1);
	DirZ = R.GetAxis(2);
}

/**
  * if bCanClimbSlopes and on ground, "forward" depends on the slope
  */
void UUTVehicleSimHover::GetRotationAxes(ASVehicle* Vehicle, FVector &DirX, FVector &DirY, FVector &DirZ)
{
	FRotationMatrix R(Vehicle->Rotation);
	DirX = R.GetAxis(0);
	DirY = R.GetAxis(1);
	DirZ = R.GetAxis(2);
	if ( !bCanClimbSlopes || !Vehicle->bVehicleOnGround )
	{
		return;
	}

	FVector NormalSum(0.f, 0.f, 0.f);
	for(INT i=0; i<Vehicle->Wheels.Num(); i++)
	{
#if WITH_NOVODEX
		USVehicleWheel* vw = Vehicle->Wheels(i);
		check(vw);
		NxWheelShape* WheelShape = vw->GetNxWheelShape();
		check(WheelShape);

		if ( vw->bWheelOnGround )
		{
			NormalSum += vw->ContactNormal;
		}
#endif // WITH_NOVODEX

	}

	if ( NormalSum.IsZero() )
	{
		return;
	}
	// Calc up (z), right(y) and forward (x) vectors
	NormalSum.Normalize();

	DirX = DirX - (DirX | NormalSum) * NormalSum;
	DirY = DirY - (DirY | NormalSum) * NormalSum;
	DirZ = DirZ - (DirZ | NormalSum) * NormalSum;
}

void UUTVehicleSimChopper::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
	if( Vehicle->bDriving )
	{
		// OutputSteering is actually a strafe value
		FLOAT OutputStrafe = Vehicle->OutputSteering;
		FLOAT OutputThrust = Vehicle->OutputGas;
		FLOAT OutputRise = Vehicle->OutputRise;

		// Zero force/torque accumulation.
		FVector Up(0.0f, 0.0f, 1.0f);
		FVector Force(0.0f, 0.0f, 0.0f);
		FVector Torque(0.0f, 0.0f, 0.0f);

		// Calc up (z), right(y) and forward (x) vectors
		FVector DirX, DirY, DirZ;
		GetRotationAxes(Vehicle, DirX, DirY, DirZ);

		// 'World plane' forward & right vectors ie. no z component.
		FVector Forward = DirX;
		if (!bAllowZThrust)
			Forward.Z = 0.0f;
		Forward.Normalize();

		FVector Right = DirY;
		if (!bAllowZThrust)
			Right.Z = 0.0f;
		Right.Normalize();

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

		if ( bStabilizeStops )
		{
			Force += StabilizationForce(Vehicle, DeltaTime, ((OutputThrust == 0.f) && (OutputStrafe == 0.f)) );
		}

		// Thrust
		AUTVehicle* UTV = Cast<AUTVehicle>(Vehicle);
		check(UTV);
		if(UTV && !UTV->ForceMovementDirection.IsZero())
		{
			FLOAT DotProduct = UTV->ForceMovementDirection | Forward;
			if(!UTV->bForceDirectionAllowedNegative && DotProduct < 0.f) // going backwards while that is not allowed; outright halt forces and turn around
			{
				Force -= ( Vehicle->Velocity );
				Force += MaxReverseForce * UTV->ForceMovementDirection;
			}
			else
			{
				UBOOL ReverseThrust = DotProduct<0.f;
				FLOAT ForcePercent = Abs(DotProduct);
				if(!(!UTV->bForceDirectionAllowedNegative && OutputThrust < 0.f))
				{
					Force += ForcePercent * UTV->ForceMovementDirection *  OutputThrust * (ReverseThrust?MaxReverseForce:MaxThrustForce);
				}
			}
		}
		else
		{
			if ( (OutputThrust < 0.f) && (!bFullThrustOnDirectionChange  || ((Vehicle->Velocity | Forward) < 0.f)) )
				Force += OutputThrust * MaxReverseForce * Forward;
			else
				Force += OutputThrust * MaxThrustForce * Forward;

			// Strafe
			if ( bFullThrustOnDirectionChange && ((Vehicle->Velocity | OutputStrafe*Right) > 0.f) )
				Force -= OutputStrafe * MaxThrustForce * Right;
			else
				Force -= OutputStrafe * MaxStrafeForce * Right;

			Force -= (1.0f - Abs(OutputThrust)) * LongDamping * ForwardVelMag * Forward;
			Force -= (1.0f - Abs(OutputStrafe)) * LatDamping * RightVelMag * Right;

			// Rise
			AccumulatedTime += DeltaTime;
			UBOOL bAddRandForce = false;
			if (AccumulatedTime > RandForceInterval)
			{
				AccumulatedTime = 0.0f;
				bAddRandForce = true;
			}
			if( Vehicle->Location.Z > Vehicle->WorldInfo->StallZ )
			{
				Force += Up * ::Min(-1.f*UpDamping*UpVelMag, OutputRise*MaxRiseForce);
			}
			else if ( OutputRise == 0.f ) // If not pushing up or down, apply vertical damping and small perturbation force.
			{
				Force -= (UpDamping * UpVelMag * Up);

				if ( bAddRandForce && (Vehicle->Role == ROLE_Authority) )
				{
					RandForce.X = 2 * (appFrand() - 0.5) * MaxRandForce;
					RandForce.Y = 2 * (appFrand() - 0.5) * MaxRandForce;
					RandForce.Z = 2 * (appFrand() - 0.5) * MaxRandForce;
					RandTorque.X = 2 * (appFrand() - 0.5) * MaxRandForce;
					RandTorque.Y = 2 * (appFrand() - 0.5) * MaxRandForce;
					RandTorque.Z = 2 * (appFrand() - 0.5) * MaxRandForce;

					Force += RandForce;
					Torque += RandTorque;
				}
			}
			else
			{
				Force += OutputRise * MaxRiseForce * Up;
			}
		}

		FRotator LookRot = FRotator(Vehicle->DriverViewPitch, Vehicle->DriverViewYaw, 0);
		FVector LookDir = LookRot.Vector();

		// Try to turn the helicopter to match the way the camera is facing.

		//// YAW ////
		// Project Look dir into z-plane
		FVector PlaneLookDir = LookDir;
		PlaneLookDir.Z = 0.0f;
		PlaneLookDir = PlaneLookDir.SafeNormal();

		FLOAT CurrentHeading = HeadingAngle(Forward);
		FLOAT DesiredHeading = HeadingAngle((UTV && UTV->ForceMovementDirection == FVector(0,0,0))?PlaneLookDir:UTV->ForceMovementDirection);

		if ( !bHeadingInitialized )
		{
			TargetHeading = CurrentHeading;
			bHeadingInitialized = true;
		}		

		// Move 'target heading' towards 'desired heading' as fast as MaxYawRate allows.
		FLOAT DeltaTargetHeading = FindDeltaAngle(TargetHeading, DesiredHeading);
		FLOAT MaxDeltaHeading = DeltaTime * MaxYawRate;
		DeltaTargetHeading = Clamp<FLOAT>(DeltaTargetHeading, -MaxDeltaHeading, MaxDeltaHeading);
		TargetHeading = UnwindHeading(TargetHeading + DeltaTargetHeading);
		
		// Then put a 'spring' on the copter to target heading.
		FLOAT DeltaHeading = FindDeltaAngle(CurrentHeading, TargetHeading);
		FLOAT TurnTorqueMag = (DeltaHeading / PI) * TurnTorqueFactor;
		//debugf(TEXT("TurnTorqueMag: %.2f"), TurnTorqueMag);
		TurnTorqueMag = Clamp<FLOAT>( TurnTorqueMag, -TurnTorqueMax, TurnTorqueMax );
		Torque += TurnTorqueMag * Up;

		//// ROLL ////
		// Add roll torque about local X vector as helicopter turns.
		FLOAT RollTorqueMag = ( (-DeltaHeading / PI) * RollTorqueTurnFactor ) + ( OutputStrafe * RollTorqueStrafeFactor );
		RollTorqueMag = Clamp<FLOAT>( RollTorqueMag, -RollTorqueMax, RollTorqueMax );
		Torque += ( RollTorqueMag * DirX );

		//// PITCH ////
		FLOAT PitchTorqueMag = OutputThrust * PitchTorqueFactor;
		PitchTorqueMag = Clamp<FLOAT>( PitchTorqueMag, -PitchTorqueMax, PitchTorqueMax );
		Torque += PitchTorqueMag * DirY ;
		Torque += (Vehicle->Rotation.Vector().Z - LookDir.Z)*PitchViewCorrelation*DirY;
		
		FLOAT ActualTurnDamping;
		if (bStrafeAffectsTurnDamping && OutputStrafe != 0.0)
		{
			ActualTurnDamping = StrafeTurnDamping;
		}
		else
		{
			ActualTurnDamping = TurnDamping;
		}

		// Steer (yaw) damping
		Torque -= TurnAngVel * ActualTurnDamping * Up;

		// Roll damping
		Torque -= RollAngVel * RollDamping * DirX;

		// Pitch damping
		Torque -= PitchAngVel * PitchDamping * DirY;

		// velocity damping to limit airspeed
		Force -= Vehicle->GetDampingForce(Force);

		// Apply force/torque to body.
		Vehicle->AddForce( Force );
		Vehicle->AddTorque( Torque );
	}
	else if ( bStabilizeStops )
	{
		// Apply stabilization force to body.
		Vehicle->AddForce( StabilizationForce(Vehicle, DeltaTime, TRUE) );

		// when no driver, also damp rotation
		Vehicle->AddTorque( StabilizationTorque(Vehicle, DeltaTime, TRUE) );
	}
}

/**
  *  Used by some vehicles to limit their maximum velocity
  *	 @PARAM InForce is the force being applied to this vehicle from USVehicleSimBase::UpdateVehicle()
  *  @RETURN damping force 
  */
FVector AUTVehicle::GetDampingForce(const FVector& InForce)
{
	checkSlow(AirSpeed > 0.f );
	
	FVector DampedVelocity = Velocity;
	// perhaps don't damp downward z velocity if vehicle isn't touching ground
	DampedVelocity.Z = (bNoZDamping || (bNoZDampingInAir && !HasWheelsOnGround())) ? 0.f : DampedVelocity.Z;
	
	return InForce.Size() * ::Min(DampedVelocity.SizeSquared()/Square(1.03f*AirSpeed), 2.f) * DampedVelocity.SafeNormal();
}

FVector UUTVehicleSimChopper::StabilizationForce(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize)
{
	FVector VehicleVelocity = Vehicle->Velocity;

	if ( !bAllowZThrust )
		VehicleVelocity.Z = 0.f;

	if ( !bShouldStabilize )
	{
		CurrentStabilizationMultiplier = StabilizationForceMultiplier;
		OldVelocity = VehicleVelocity;
		return FVector(0.f,0.f,0.f);
	}

	// return stabilization multiplier to desired value over time
	CurrentStabilizationMultiplier = StabilizationForceMultiplier * ::Max(0.f, 1.f - DeltaTime);
	OldVelocity = VehicleVelocity;
	return -1.f * CurrentStabilizationMultiplier*VehicleVelocity;
}

FVector UUTVehicleSimHoverTank::StabilizationForce(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize)
{
	FVector VehicleVelocity = Vehicle->Velocity;

	if ( !bShouldStabilize )
	{
		CurrentStabilizationMultiplier = StabilizationForceMultiplier;
		OldVelocity = VehicleVelocity;
		return FVector(0.f,0.f,0.f);
	}

	// return stabilization multiplier to desired value over time
	CurrentStabilizationMultiplier = StabilizationForceMultiplier + (CurrentStabilizationMultiplier - StabilizationForceMultiplier) * ::Max(0.f, 1.f - 2.f*DeltaTime);

	// increase stabilization multiplier if velocity not decreasing fast enough
	if ( (VehicleVelocity | OldVelocity) > ::Max(0.f, (1.f - DeltaTime) * OldVelocity.SizeSquared()) )
	{
		CurrentStabilizationMultiplier *= (1.f + 4.f*DeltaTime);
	}

	OldVelocity = VehicleVelocity;
	return -1.f * CurrentStabilizationMultiplier*VehicleVelocity;
}

FVector UUTVehicleSimChopper::StabilizationTorque(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize)
{
		// Calc up (z) vector
		FRotationMatrix R(Vehicle->Rotation);
		FVector DirZ = R.GetAxis(2);

		// Steer (yaw) damping
		// Get body angular velocity
		FRigidBodyState rbState;
		Vehicle->GetCurrentRBState(rbState);
		FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
		FLOAT TurnAngVel = AngVel | DirZ;
		return -1.f * TurnAngVel * TurnDamping * DirZ;
}

FVector UUTVehicleSimHoverTank::StabilizationTorque(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize)
{
		// Calc up (z) vector
		FRotationMatrix R(Vehicle->Rotation);
		FVector DirZ = R.GetAxis(2);

		// Steer (yaw) damping
		// Get body angular velocity
		FRigidBodyState rbState;
		Vehicle->GetCurrentRBState(rbState);
		FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
		FLOAT TurnAngVel = AngVel | DirZ;
		return -1.f * TurnAngVel * TurnDamping * DirZ;
}

FLOAT AUTAirVehicle::GetGravityZ()
{
	if ( bDriving )
	{
		return -0.1f;
	}
	return Super::GetGravityZ();
}

void UUTVehicleSimHover::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
	// handle outputrise differently from chopper
	Vehicle->OutputRise = 0.f;

	if ( bDisableWheelsWhenOff )
	{
		// set wheel collision based on whether have a driver
		if ( Vehicle->bDriving && !bUnPoweredDriving )
		{
			if ( !bRepulsorCollisionEnabled && (Vehicle->Wheels.Num() > 0) )
			{
				for ( INT i=0; i<Vehicle->Wheels.Num(); i++ )
				{
					Vehicle->SetWheelCollision( i, true );
				}

				bRepulsorCollisionEnabled = true;
			}
		}
		else if ( bRepulsorCollisionEnabled )
		{
			bRepulsorCollisionEnabled = false;
			for ( INT i=0; i<Vehicle->Wheels.Num(); i++ )
			{
				Vehicle->SetWheelCollision(i,false);
			}
		}
	}	

#if WITH_NOVODEX // Update WheelShape in case it changed due to handbrake or parking
	if ( Vehicle->bUpdateWheelShapes )
	{
		for(INT i=0; i<Vehicle->Wheels.Num(); i++)
		{
			USVehicleWheel* vw = Vehicle->Wheels(i);
			NxWheelShape* WheelShape = vw->GetNxWheelShape();
			check(WheelShape);	
			SetNxWheelShapeParams(WheelShape, vw);
		}
		Vehicle->bUpdateWheelShapes = FALSE;
	}
#endif // WITH_NOVODEX

	if ( bUnPoweredDriving && Vehicle->Controller )
	{
		// allow nudging of the ball
		// @TODO FIXMESTEVE - need "SpecialUpdateVehicle()" virtual function instead of this hack
		AUTVehicle_Scavenger *Scavenger = Cast<AUTVehicle_Scavenger>(Vehicle);
		if( Scavenger  && (Scavenger->Velocity.SizeSquared() < Square(Scavenger->bBallBoostActivated ? Scavenger->MaxBoostSpeed : Scavenger->MaxBallSpeed)) )
		{
			// OutputSteering is actually a strafe value
			FLOAT OutputStrafe = Vehicle->OutputSteering;
			FLOAT OutputThrust = Vehicle->OutputGas;
			FLOAT OutputRise = Vehicle->OutputRise;

			// Zero force/torque accumulation.
			FVector Force(0.0f, 0.0f, 0.0f);

			// Calc up (z), right(y) and forward (x) vectors
			FRotationMatrix R(Vehicle->Controller->Rotation);
			FVector DirX = R.GetAxis(0);
			FVector DirY = R.GetAxis(1);
			FVector DirZ = R.GetAxis(2);

			// 'World plane' forward & right vectors ie. no z component.
			FVector Forward = DirX;
			Forward.Z = 0.0f;
			Forward.Normalize();
			
			if ( Scavenger->bBallBoostActivated )
			{
				Force += Scavenger->MaxBoostForce * Forward;
			}
			else
			{
				Force += Scavenger->MaxBallForce * OutputThrust * Forward;
			}

			// Apply force/torque to body.
			Vehicle->AddForce( Force );
		}
	}
	else
	{
		Super::UpdateVehicle(Vehicle, DeltaTime);
	}
}


void UUTVehicleSimHoverTank::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
	if( Vehicle->bDriving )
	{
		FLOAT OutputThrust = Vehicle->OutputGas;
		FLOAT OutputRise = Vehicle->OutputRise;

		// Zero force/torque accumulation.
		FVector Force(0.0f, 0.0f, 0.0f);
		FVector Torque(0.0f, 0.0f, 0.0f);

		// Calc up (z), right(y) and forward (x) vectors
		FRotationMatrix R(Vehicle->Rotation);
		FVector DirX = R.GetAxis(0);
		FVector DirY = R.GetAxis(1);
		FVector DirZ = R.GetAxis(2);

		// 'World plane' forward & right vectors ie. no z component.
		FVector Forward = DirX;
		Forward.Z = 0.0f;
		Forward.Normalize();

		FVector Right = DirY;
		Right.Z = 0.0f;
		Right.Normalize();

		FLOAT ForwardVelMag = Vehicle->Velocity | Forward;
		FLOAT RightVelMag = Vehicle->Velocity | Right;

		if ( bStabilizeStops )
			Force += StabilizationForce(Vehicle, DeltaTime, (OutputThrust == 0.f));

		// Thrust
		AUTVehicle* UTV= Cast<AUTVehicle>(Vehicle);
		if(UTV->ForceMovementDirection != FVector(0,0,0))
		{
			FLOAT DotProduct = UTV->ForceMovementDirection | Forward;
			
			if(!UTV->bForceDirectionAllowedNegative && DotProduct < 0.f) // going backwards while that is not allowed; outright halt forces and turn around
			{
				Force -= ( Vehicle->Velocity );
			}
			else
			{
				UBOOL ReverseThrust = DotProduct<0.f;
				FLOAT ForcePercent = Abs(DotProduct);
				if(UTV->bForceDirectionAllowedNegative)
				{
					Force += ForcePercent * UTV->ForceMovementDirection * OutputThrust * (OutputThrust>0.f?MaxThrustForce:MaxReverseForce);
					Force -= (1-ForcePercent) * Vehicle->Velocity * (1-Abs(OutputThrust))* (OutputThrust>0.f?MaxThrustForce:MaxReverseForce);
				}
				else if(ReverseThrust && OutputThrust < 0.f)
				{
					Force += ForcePercent * UTV->ForceMovementDirection *  Abs(OutputThrust) * MaxReverseForce;
					Force -= (1-ForcePercent) * Vehicle->Velocity * (1-Abs(OutputThrust)) * MaxReverseForce;
				}
				else if(!ReverseThrust && OutputThrust > 0.f)
				{
					Force += ForcePercent * UTV->ForceMovementDirection *  OutputThrust * MaxThrustForce;
					Force += (1-ForcePercent) * Vehicle->Velocity * (1-OutputThrust) * MaxThrustForce;
				}
				else
				{
					Force -= Vehicle->Velocity;
				}
			}

			FLOAT CurrentHeading = HeadingAngle(Forward);
			FLOAT DesiredHeading = HeadingAngle(UTV->ForceMovementDirection);

			if ( !bHeadingInitialized )
			{
				TargetHeading = CurrentHeading;
				bHeadingInitialized = true;
			}		

			// Move 'target heading' towards 'desired heading' as fast as MaxYawRate allows.
			FLOAT DeltaTargetHeading = FindDeltaAngle(TargetHeading, DesiredHeading);
			FLOAT MaxDeltaHeading = DeltaTime*2.0f;
			DeltaTargetHeading = Clamp<FLOAT>(DeltaTargetHeading, -MaxDeltaHeading, MaxDeltaHeading);
			TargetHeading = UnwindHeading(TargetHeading + DeltaTargetHeading);
			
			// Then put a 'spring' on the copter to target heading.
			FLOAT DeltaHeading = FindDeltaAngle(CurrentHeading, TargetHeading);
			FLOAT TurnTorqueMag = (DeltaHeading / PI) * TurnTorqueMax;
			//debugf(TEXT("TurnTorqueMag: %.2f"), TurnTorqueMag);
			TurnTorqueMag = Clamp<FLOAT>( TurnTorqueMag, -TurnTorqueMax, TurnTorqueMax );
			Torque = TurnTorqueMag * DirZ;

		}
		else
		{
			if ( (OutputThrust < 0.f) && ((Vehicle->Velocity | Forward) < 0.f) )
				Force += OutputThrust * MaxReverseForce * Forward;
			else
				Force += OutputThrust * MaxThrustForce * Forward;

			Force -= (1.0f - Abs(OutputThrust)) * LongDamping * ForwardVelMag * Forward;
			Force -= LatDamping * RightVelMag * Right;

			if(bTurnInPlaceOnSteer || Abs(OutputThrust) > 0.1f)
			{
				Torque = -1.f * Vehicle->OutputSteering * DirZ * TurnTorqueMax;
			}

			if ( OutputThrust < 0.f )
			{
				bWasReversedSteering = TRUE;
			}
			else if ( bWasReversedSteering )
			{
				// keep steering reversed if not pushing forward
				if ( (OutputThrust > 0.f) || (ForwardVelMag > -10.f) )
					bWasReversedSteering = FALSE;
			}

			if ( bWasReversedSteering )
			{
				Torque *= -1.f;
			}

			// Steer (yaw) damping
			// Get body angular velocity
			FRigidBodyState rbState;
			Vehicle->GetCurrentRBState(rbState);
			FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
			FLOAT TurnAngVel = AngVel | DirZ;
			Torque -= TurnAngVel * TurnDamping * DirZ;
		}

		// velocity damping to limit airspeed
		Force -= Vehicle->GetDampingForce(Force);

		// Apply force/body.
		Vehicle->AddForce( Force );
		// Apply Torque
		Vehicle->AddTorque( Torque );

	}
	else if ( bStabilizeStops )
	{
		// Apply stabilization force to body.
		Vehicle->AddForce( StabilizationForce(Vehicle, DeltaTime, TRUE) );

		// when no driver, also damp rotation
		Vehicle->AddTorque( StabilizationTorque(Vehicle, DeltaTime, TRUE) );
	}

	// set wheel collision based on whether have a driver
	UBOOL bAdjustWheels = FALSE;
	FLOAT DrivingWheelAdjustFactor = 0.f;

	if ( Vehicle->bDriving )
	{
		if ( CurrentGroundDist < DrivingGroundDist )
		{
			CurrentGroundDist = ::Min(DrivingGroundDist, CurrentGroundDist + GroundDistAdjustSpeed*DeltaTime);
			bAdjustWheels = TRUE;
			DrivingWheelAdjustFactor = WheelAdjustFactor;
		}
	}
	else if ( CurrentGroundDist > ParkedGroundDist )
	{
		CurrentGroundDist = ::Max(ParkedGroundDist, CurrentGroundDist - GroundDistAdjustSpeed*DeltaTime);
		bAdjustWheels = TRUE;
	}

	if ( bAdjustWheels || Vehicle->bUpdateWheelShapes )
	{
		for ( INT i=0; i<Vehicle->Wheels.Num(); i++ )
		{
			Vehicle->Wheels(i)->SuspensionTravel = ::Max(0.f, CurrentGroundDist + DrivingWheelAdjustFactor - Vehicle->Wheels(i)->WheelRadius);
			Vehicle->Wheels(i)->BoneOffset.Z = Vehicle->Wheels(i)->SuspensionTravel + Vehicle->Wheels(i)->WheelRadius;
#if WITH_NOVODEX 
			NxWheelShape* WheelShape = Vehicle->Wheels(i)->GetNxWheelShape();
			SetNxWheelShapeParams(WheelShape, Vehicle->Wheels(i));
#endif // WITH_NOVODEX
		}
		Vehicle->bUpdateWheelShapes = FALSE;
		UAnimTree* Tree = Cast<UAnimTree>(Vehicle->Mesh->Animations);
		if ( Tree )
		{
			// Get all controls in tree.
			TArray<USkelControlBase*> Controls;
			Tree->GetSkelControls(Controls);

			// Iterate over looking for hugground controls.
			for(INT i=0; i<Controls.Num(); i++)
			{
				UUTSkelControl_HugGround *GroundControl = Cast<UUTSkelControl_HugGround>(Controls(i));
				if ( GroundControl )
				{
					GroundControl->DesiredGroundDist = CurrentGroundDist;
				}
			}
		}
	}

	Super::UpdateVehicle(Vehicle, DeltaTime);
}


UBOOL AUTVehicle::JumpOutCheck(AActor *GoalActor, FLOAT Distance, FLOAT ZDiff)
{
	if ( GoalActor && (ZDiff > -500.f) && (WorldInfo->TimeSeconds - LastJumpOutCheck > 1.f) )
	{
		FLOAT GoalRadius, GoalHeight;
		GoalActor->GetBoundingCylinder(GoalRadius, GoalHeight);
		if ( Distance < ::Min(2.f*GoalRadius,ObjectiveGetOutDist) )
		{
			LastJumpOutCheck = WorldInfo->TimeSeconds;
			eventJumpOutCheck();
			return (Controller == NULL);
		}
	}
	return false;
}

FLOAT AUTVehicle::GetMaxRiseForce()
{
	UUTVehicleSimChopper* SimChopper = Cast<UUTVehicleSimChopper>(SimObj);
	if ( SimChopper )
		return SimChopper->MaxRiseForce;
	return 100.f;
}

void AUTVehicle_Raptor::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	bForwardMode = Driver && (Throttle > 0.f) && 
					((Velocity | Rotation.Vector()) > ::Max(200.f, 0.6f*Velocity.Size()));
}



AVehicle* AUTWeaponPawn::GetVehicleBase()
{
	return MyVehicle;
}

void AUTWeaponPawn::TickSpecial( FLOAT DeltaSeconds )
{
	if (Controller)
	{
		// Don't Do anything if the pawn is in fixed view
		AUTPawn* const DriverPawn = Cast<AUTPawn>(Driver);

		if ( DriverPawn && !DriverPawn->bFixedView )
		{

			AUTPlayerController* const PC = Cast<AUTPlayerController>(Controller);
			if (PC && !PC->bDebugFreeCam)
			{
				FRotator Rot = Controller->Rotation;
				MyVehicle->ApplyWeaponRotation(MySeatIndex, Rot);
			}
		}
	}
}


UBOOL AUTVehicle::CheckAutoDestruct(ATeamInfo* InstigatorTeam, FLOAT CheckRadius)
{
	// check if close enough to something to auto destruct
	FMemMark Mark(GMem);
	FCheckResult* ActorCheckResult = GWorld->Hash->ActorPointCheck( GMem, Location, FVector(CheckRadius,CheckRadius,CheckRadius), TRACE_ProjTargets );

	// if still going fast, only explode if have gone by the target
	UBOOL bOnlyIfPast = (Velocity.SizeSquared() > 1000000.f);
	for( FCheckResult* Hits=ActorCheckResult; Hits!=NULL; Hits=Hits->GetNext() )
	{
		if ( Hits->Actor && Hits->Actor->GetAPawn() )
		{
			// explode if pawn on different team
			if ( !InstigatorTeam || !Hits->Actor->GetAPawn()->PlayerReplicationInfo || (Hits->Actor->GetAPawn()->PlayerReplicationInfo->Team != InstigatorTeam) )
			{
				// blow up
				if ( !bOnlyIfPast || (((Hits->Actor->Location - Location) | Velocity) < 0.f) )
				{
    				eventSelfDestruct(Hits->Actor);
					Mark.Pop();
					return true;
				}
			}
		}
		else
		{
			AUTGameObjective* HitObjective = Cast<AUTGameObjective>(Hits->Actor);
			if ( HitObjective )
			{
				// explode if objective on different team
				if ( !InstigatorTeam || (HitObjective->DefenderTeamIndex != InstigatorTeam->TeamIndex) )
				{
					// blow up
					if ( !bOnlyIfPast || (((HitObjective->Location - Location) | Velocity) < 0.f) )
					{
						eventSelfDestruct(HitObjective);
						Mark.Pop();
						return true;
					}
				}
			}
		}
	}
	Mark.Pop();
	return false;
}

FLOAT AUTHoverVehicle::GetGravityZ()
{
	if ( Location.Z < WorldInfo->StallZ )
	{
		return Super::GetGravityZ() * CustomGravityScaling;
	}
	return Super::GetGravityZ() * ((Velocity.Z > 0.f) ? StallZGravityScaling : 1.f);
}

void AUTVehicle_Viper::TickSpecial( FLOAT DeltaSeconds )
{
	if ( bDeadVehicle )
	{
		CustomGravityScaling = NormalGravity;
		Super::TickSpecial(DeltaSeconds);
		UUTSkelControl_SpinControl* Spinner = Cast<UUTSkelControl_SpinControl>(Mesh->FindSkelControl(SelfDestructSpinName));
		if(Spinner)
		{
			Spinner->SetSkelControlStrength(0.0,0.0);
		}
		return;
	}

	Super::TickSpecial(DeltaSeconds);
	
	// moved up for CurveSound, also used for Corkscrew
	FRotationMatrix R(Rotation);
	FRigidBodyState rbState;
	GetCurrentRBState(rbState);

	if( CurveSound != NULL )
	{
		FVector AngVel(rbState.AngVel.X, rbState.AngVel.Y, rbState.AngVel.Z);
		const FVector& Up = R.GetAxis(2);
		const FLOAT TurnAngVel = AngVel | Up; 

		// Carving sound - DAVE-TODO: Un-hack this when final carving system is in-place (TJAMES - I'm taking this over; system seems to work pretty well as is though.)
		CurveSound->VolumeMultiplier = Abs(TurnAngVel) / 2048.0f;
		if (!CurveSound->bWasPlaying && CurveSound->VolumeMultiplier > 0.25f)
		{
			CurveSound->Play();	
		}
	}

	// check if auto-destruct engaged
	if ( bSelfDestructArmed )
	{
		// corkscrew
 		UUTSkelControl_SpinControl* Spinner = Cast<UUTSkelControl_SpinControl>(Mesh->FindSkelControl(SelfDestructSpinName));
		if(Spinner)
		{
				Spinner->SetSkelControlStrength(1.0,0.0);
		}

		// client and server
		CustomGravityScaling = 0.f;

		FVector Boost = -1.f * BoostForce * Velocity.SizeSquared()/Square(1.03f*AirSpeed) * Velocity.SafeNormal();
		if ( Velocity.Size() < AirSpeed )
		{
			Boost = 2.f * BoostDir -  BoostForce * Velocity.SafeNormal();
		}

		AddForce( Boost );
		// keep wheels turned on
		bDriving = TRUE;

		// SelfDestructInstigator set only on server
		if ( SelfDestructInstigator )
		{
			if ( (WorldInfo->TimeSeconds - DestructStartTime > MaxDestructDuration) )
			{
				// blow up
				eventSelfDestruct(NULL);
				return;
			}

			ATeamInfo* InstigatorTeam = SelfDestructInstigator->PlayerReplicationInfo ? SelfDestructInstigator->PlayerReplicationInfo->Team : NULL;
			if ( CheckAutoDestruct(InstigatorTeam, 150.f) )
				return;
		}
	}
	else if ((Role == ROLE_Authority) || Controller != NULL)
	{
		CustomGravityScaling = (Controller && (Rise > 0.f)) ? GlidingGravity : NormalGravity;
		if(Rise <= 0.0)
		{
			bSelfDestructInProgress = false;
			bSelfDestructReady = false;
		}
	}

	// handle glide animation
	if (GlideBlend != NULL)
	{
		FLOAT DesiredGlideWeight = (CustomGravityScaling == GlidingGravity) ? 1.0f : 0.0f;
		if (GlideBlend->Child2WeightTarget != DesiredGlideWeight)
		{
			GlideBlend->SetBlendTarget(DesiredGlideWeight, GlideBlendTime);
		}
	}

	if (LastRenderTime > WorldInfo->TimeSeconds - 1.0f)
	{
		if ( ExhaustIndex < VehicleEffects.Num() && VehicleEffects(ExhaustIndex).EffectRef )
		{
			FLOAT ExhaustSize = Clamp<FLOAT>( (Velocity.Size() / 2200), 0.0, 1.0);
			VehicleEffects( ExhaustIndex ).EffectRef->SetFloatParameter(ExhaustParamName, ExhaustSize );
		}
	}

	JumpCountdown -= DeltaSeconds;
	FLOAT CurrentAirSpeed = AirSpeed;
	AirSpeed = FullAirSpeed;

	//@fixme FIXME: move into UTJumpingHoverVehicle superclass so code is shared with hoverboard and other jumping hover vehicles
	if (Controller != NULL)
	{
		// no z damping during jump
		bNoZDamping = (Rise > 0.f);

		// trying to jump
		if ( bNoZDamping )
		{
			if (!bHoldingDuck && JumpCountdown <= 0.f && WorldInfo->TimeSeconds - JumpDelay >= LastJumpTime)
			{
				// Calculate Local Up Vector
				FRotationMatrix R2(Rotation);
				FVector DirZ = R2.GetAxis(2);

				FVector LocalUp = DirZ;
				LocalUp.Normalize();
				
				UBOOL bIsOverJumpableSurface = FALSE;

				// Don't jump if we are at a steep angle
				if ((LocalUp | FVector(0.0f,0.0f,1.0f)) > 0.5f)
				{
					// Otherwise make sure there is ground underneath
					// use an extent trace so what we hit matches up with what the vehicle and physics collision would hit
					FCheckResult Hit(1.0f);
					bIsOverJumpableSurface = !GWorld->SingleLineCheck(Hit, this, Location - (LocalUp * JumpCheckTraceDist), Location, TRACE_AllBlocking, FVector(1.f, 1.f, 1.f));
				}
				if (bIsOverJumpableSurface)
				{
					// If we are on the ground, and press Rise, and we not currently in the middle of a jump, start a new one.
					if (Role == ROLE_Authority)
					{
   						bDoBikeJump = !bDoBikeJump;
					}
					eventViperJumpEffect();

					if ( Cast<AUTBot>(Controller) )
    					Rise = 0.f;

					JumpCountdown = JumpDuration;
					LastJumpTime = WorldInfo->TimeSeconds;
				}
			}
			if ( bStopWhenGlide )
			{
				// smoothly reduce airspeed
				AirSpeed = ::Max(GlideAirSpeed, CurrentAirSpeed*(1.f-GlideSpeedReductionRate*DeltaSeconds));
			}
		}
		else if ( (Rise < 0.f) || bPressingAltFire )
		{
			// Duck!
			if ( !bHoldingDuck )
			{
				bHoldingDuck = true;
				
				if ( Cast<AUTBot>(Controller) )
    				Rise = 0.f;

				JumpCountdown = 0.0; // Stops any jumping that was going on.
    		}
		}
		else 
		{
			bHoldingDuck = false;
		}
	}

	if ( bDriving && JumpCountdown > 0.f )
	{
		if ( Location.Z < WorldInfo->StallZ )
		{
			AddForce( FVector(0.f,0.f,JumpForceMag) );
		}
		else
		{
			JumpCountdown = 0.f;
		}
	}
}

UBOOL UUTVehicleWheel::WantsParticleComponent()
{
	return (bUseMaterialSpecificEffects || Super::WantsParticleComponent());
}



/** Simulate the engine model of the treaded vehicle */

void UUTVehicleSimTank::UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime)
{
#if WITH_NOVODEX

	AUTVehicle* UTV=Cast<AUTVehicle>(Vehicle);
	// Lose torque when climbing too steep
	FRotationMatrix R(Vehicle->Rotation);
	FLOAT EngineTorque;
	UBOOL bInvertTorque;
	if(UTV && UTV->ForceMovementDirection != FVector(0,0,0)) // we need to recalculate the steering numbers to force us in the 'right' direction:
	{
		Vehicle->OutputSteering = Clamp<FLOAT>((FindDeltaAngle(HeadingAngle(UTV->ForceMovementDirection),HeadingAngle(R.GetAxis(0))))*10.0f,-1.0,1.0);
		FLOAT Threshold = bForceOnTarget?0.25f:0.1f;
		if(Abs(Vehicle->OutputSteering)>Threshold*2.0 || (Vehicle->OutputGas < 0.f && !UTV->bForceDirectionAllowedNegative))
		{
			Vehicle->OutputGas = 0.0;
		}
		else
		{
			if(Abs(Vehicle->OutputSteering)>Threshold)
			{
				Vehicle->OutputSteering = 0.0f;
				bForceOnTarget = true;
			}
		}
		EngineTorque = Clamp<FLOAT>(Abs(Vehicle->OutputGas) + Abs(TurnInPlaceThrottle * Vehicle->OutputSteering), -1.0, 1.0) * MaxEngineTorque;
		bInvertTorque = (Vehicle->OutputGas < 0.f);
	}
	else
	{
		// Determine how much torque we are getting from the engine
		if(bTurnInPlaceOnSteer || (Vehicle->OutputRise > 0.f))
		{
			EngineTorque = Clamp<FLOAT>(Abs(Vehicle->OutputGas) + Abs(TurnInPlaceThrottle * Vehicle->OutputSteering), -1.0, 1.0) * MaxEngineTorque;
		}
		else
		{
			EngineTorque = Clamp<FLOAT>(Abs(Vehicle->OutputGas), -1.0, 1.0) * MaxEngineTorque;
		}

		bInvertTorque = (Vehicle->OutputGas < 0.f);
		bForceOnTarget = false;
	}

	if ( R.GetAxis(2).Z < Vehicle->WalkableFloorZ )
	{
		if ( (Vehicle->OutputGas > 0.f) == (R.GetAxis(0).Z > 0.f) )
		{
			// Kill torque if trying to go up
			EngineTorque = 0.f;
		}
	}
	
	
	if (Vehicle->OutputSteering != 0.f)
	{
		FLOAT InsideTrackFactor;
		if( Abs(Vehicle->OutputGas) > 0.f )
		{
			// Smoothly modify inside track speed as we steer
			InsideTrackFactor = Lerp(0.5f, InsideTrackTorqueFactor, Abs(Vehicle->OutputSteering));
		}
		else
		{
			InsideTrackFactor = -0.5f;
		}

		//FLOAT InsideTrackFactor = Clamp(InsideTrackTorqueCurve.Eval(Vehicle->ForwardVel, 0.0f), -1.0f, 1.0f);
		
		// Determine how to split up the torque based on the InsideTrackTorqueCurve
		FLOAT InsideTrackTorque = EngineTorque * InsideTrackFactor;
		FLOAT OutsideTrackTorque = EngineTorque * (1.0f - Abs(InsideTrackFactor)); 

		if (Vehicle->OutputSteering < 0.f) // Turn Right
		{
			LeftTrackTorque = OutsideTrackTorque; 
			RightTrackTorque = InsideTrackTorque;
		}
		else // Turn Left
		{	
			LeftTrackTorque = InsideTrackTorque;
			RightTrackTorque = OutsideTrackTorque;
		}
	}
	else
	{
		// If not steering just split up the torque equally between the two tracks
		LeftTrackTorque = EngineTorque * 0.5f;
		RightTrackTorque = EngineTorque * 0.5f;
	}
	

	// Invert torques when you want to drive backwards.
	if(bInvertTorque)
	{
		LeftTrackTorque *= -1.f;
		RightTrackTorque *= -1.f;
	}

	LeftTrackVel += (LeftTrackTorque - (LeftTrackVel * EngineDamping)) * DeltaTime;
	RightTrackVel += (RightTrackTorque - (RightTrackVel * EngineDamping)) * DeltaTime;


	UUTVehicleSimTank* DefaultTank = (UUTVehicleSimTank*)GetClass()->GetDefaultObject();
	if(UTV->bFrontalCollision)
	{
		WheelLongExtremumValue = DefaultTank->WheelLongExtremumValue * FrontalCollisionGripFactor;
		WheelLongAsymptoteValue = DefaultTank->WheelLongAsymptoteValue * FrontalCollisionGripFactor;
	}
	else
	{
		WheelLongExtremumValue = DefaultTank->WheelLongExtremumValue;
		WheelLongAsymptoteValue = DefaultTank->WheelLongAsymptoteValue;
	}

	// Do the simulation for each wheel.
	ApplyWheels(LeftTrackVel,RightTrackVel,Vehicle);
#endif // WITH_NOVODEX
}

