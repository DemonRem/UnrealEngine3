//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameVehicleClasses.h"

IMPLEMENT_CLASS(UUTUIScene_VehicleHud);
	IMPLEMENT_CLASS(UVHud_Scorpion);

/*=========================================================================================
	Scorpion	
  ========================================================================================= */

void UVHud_Scorpion::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( BoostBar && MyVehicle )
	{
		FLOAT Perc = 0.0f;
		FLOAT BarOpacity = 1.0f;
		AUTVehicle_Scorpion* MyScorpion = Cast<AUTVehicle_Scorpion>(MyVehicle);
		if (MyScorpion)
		{
			FLOAT TimeSeconds = GWorld->GetWorldInfo()->TimeSeconds;

			if ( MyScorpion->bBoostersActivated )
			{
				Perc = 1 - (TimeSeconds - MyScorpion->BoostStartTime) / MyScorpion->MaxBoostDuration;
				BarOpacity = Abs(appSin(TimeSeconds * 5));

				if ( MyScorpion->ReadyToSelfDestruct() )
				{
					if ( LastBoostMsg != EjectMarkup )
					{	
						LastBoostMsg = EjectMarkup;
						BoostMsg->SetValue(LastBoostMsg);
					}

				}
				else
				{
					if ( LastBoostMsg != BoostMarkup )
					{	
						LastBoostMsg = BoostMarkup;
						BoostMsg->SetValue(LastBoostMsg);
					}
				}

			}
			else
			{
				Perc = (TimeSeconds - MyScorpion->BoostChargeTime) / MyScorpion->BoostChargeDuration;
				if ( LastBoostMsg != BoostMarkup )
				{	
					LastBoostMsg = BoostMarkup;
					BoostMsg->SetValue(LastBoostMsg);
				}
			}
		}
		BoostBar->Opacity = BarOpacity;
		BoostBar->SetValue(Perc,true);
	}

}

void UVHud_Scorpion::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{

	Super::Initialize(inOwnerScene, inOwner);

	BoostBar = Cast<UUIProgressBar>(FindChild(BoostBarName, true));
	BoostMsg = Cast<UUILabel>(FindChild(BoostMsgName, true));

	if (BoostMsg)
	{
		BoostMsg->SetValue(BoostMarkup);
		LastBoostMsg = BoostMarkup;
	}
}
