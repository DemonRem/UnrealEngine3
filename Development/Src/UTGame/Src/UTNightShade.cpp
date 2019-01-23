/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"

//#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(AUTStealthVehicle);
IMPLEMENT_CLASS(AUTVehicle_NightShade);
IMPLEMENT_CLASS(AUTVehicle_StealthBender);

void AUTStealthVehicle::SetArmLocation(FLOAT DeltaSeconds)
{
	UUTSkelControl_TurretConstrained* Turret;
	Turret = Cast<UUTSkelControl_TurretConstrained>(Mesh->FindSkelControl(TurretName));
	if(Turret)
	{
		if(DeployedState == EDS_UnDeploying || DeployedState==EDS_Undeployed)
		{
			FRotator Desire = Mesh->LocalToWorld.Rotator();
			Desire.Yaw = (Desire.Yaw + 32768)%65536;
			Turret->bConstrainYaw = false; // we know where we're going.
			Turret->DesiredBoneRotation = Desire;
		}
		else
		{
			if(Steering != 0.0)
			{
				if((!TurretArmMoveSound->bWasPlaying) || TurretArmMoveSound->bFinished)
				{
					TurretArmMoveSound->Play();
				}
			}
			else
			{
				TurretArmMoveSound->Stop();
			}

			AimYawOffset += appTrunc(-Steering*DeltaSeconds*ArmSpeedTune);
			if(AimYawOffset < -16384)
			{
				AimYawOffset = -16384;
			}
			else if(AimYawOffset > 16384)
			{
				AimYawOffset = 16384;
			}
			if(Turret)
			{
				Turret->DesiredBoneRotation = Mesh->LocalToWorld.Rotator();
				Turret->DesiredBoneRotation.Yaw += (AimYawOffset  + 32768)%65536;
			}
		}
		
		
	}
}


/**
@RETURN true if pawn is invisible to AI
*/
UBOOL AUTVehicle_NightShade::IsInvisible()
{
	return TRUE;
}


void AUTStealthVehicle::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	// go slower (less visible) if holding crouch
	AirSpeed = (Rise < 0.f) ? SlowSpeed : ((APawn*)(GetClass()->GetDefaultActor()))->AirSpeed;

	if ( BodyMaterialInstance != NULL )
	{
		// fade out inner color
		const FLOAT Fade = 1.f - 3.f*::Min(0.3f,DeltaSeconds);
		HitEffectColor *= Fade; // fade down

		FLOAT Speed = Velocity.Size();
		if ( Rise < 0.f )
		{
			Speed *= 0.2f; // reduce brightness more
		}
															// This is simpler than it looks. Sanity check seat(0) and its gun, if it's firing in mode 0 then go to a minimum of 75% decloaked, otherwise just use speed.
		BodyMaterialInstance->SetScalarParameterValue(SkinTranslucencyName, ((Seats.Num()>0 && (Seats(0).Gun)) && (Seats(0).Gun->eventIsFiring()) && (Seats(0).Gun->CurrentFireMode==0))?0.75+(0.25*Speed/MaxSpeed):Speed/MaxSpeed);

		if ( !Driver )
		{
			APlayerController *LocalPlayer = NULL;
			for( INT iPlayers=0; iPlayers<GEngine->GamePlayers.Num() && LocalPlayer==NULL; iPlayers++ )
			{
				if ( GEngine->GamePlayers(iPlayers) )
					LocalPlayer = GEngine->GamePlayers(iPlayers)->Actor;
			}
			if ( LocalPlayer && LocalPlayer->PlayerReplicationInfo && LocalPlayer->PlayerReplicationInfo->Team && (LocalPlayer->PlayerReplicationInfo->Team->TeamIndex == Team) )
			{
				// make NightShade visible to players on the same team
				HitEffectColor = 1.0f;
			}
		}
		BodyMaterialInstance->SetScalarParameterValue(HitEffectName, HitEffectColor);
	}
	SetArmLocation(DeltaSeconds);
}



