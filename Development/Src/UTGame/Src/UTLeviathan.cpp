/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"

IMPLEMENT_CLASS(AUTVehicle_Leviathan);

FVector AUTVehicle_Leviathan::GetTargetLocation(AActor* RequestedBy)
{
	if ( !RequestedBy )
		return Super::GetTargetLocation();

	const INT NearestTurret = eventCheckActiveTurret(RequestedBy->Location, 0.f);

	if ( NearestTurret >= 0 )
	{
		return TurretCollision[NearestTurret]->LocalToWorld.GetOrigin();
	}
	else
	{
		return Super::GetTargetLocation();
	}
}

void AUTVehicle_Leviathan::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( bDeadVehicle )
		return;

}

/**
 * Handle Processing the different turrets depending on the mode 
 */

void AUTVehicle_Leviathan::ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation)
{
	Super::ApplyWeaponRotation(SeatIndex,NewRotation);
	
	FRotator DriverRot, MainRot, WeapRot;

	if (SeatIndex == 0 && Weapon)
	{
		if (DeployedState != EDS_Deployed || !Weapon->eventIsFiring())
		{
			WeapRot = SeatWeaponRotation(SeatIndex,MainRot,TRUE);

			if (DeployedState == EDS_Undeployed)
			{
				DriverRot = WeapRot;
				MainRot = Rotation;
			}
			else if (DeployedState == EDS_Deployed)
			{
				MainRot = WeapRot;
				DriverRot = Rotation;
			}
			else
			{
				MainRot = Rotation;
				DriverRot = Rotation;
			}

			CachedTurrets[0]->DesiredBoneRotation = DriverRot;
			CachedTurrets[1]->DesiredBoneRotation = DriverRot;
			CachedTurrets[2]->DesiredBoneRotation = MainRot;
			CachedTurrets[3]->DesiredBoneRotation = MainRot;
		}
		else if (DeployedState == EDS_Deployed)
		{
			CachedTurrets[0]->DesiredBoneRotation = Rotation;
			CachedTurrets[1]->DesiredBoneRotation = Rotation;
		}

		
	}
}

FVector AUTVehicle_Leviathan::GetSeatPivotPoint(INT SeatIndex)
{
	if ( SeatIndex > 0 )
	{
		return Super::GetSeatPivotPoint(SeatIndex);
	}
	else
	{
		if (DeployedState == EDS_Deployed)
		{
			return Mesh->GetBoneLocation(MainTurretPivot);
		}
		else
		{
			return Mesh->GetBoneLocation(DriverTurretPivot);
		}
	}
}
