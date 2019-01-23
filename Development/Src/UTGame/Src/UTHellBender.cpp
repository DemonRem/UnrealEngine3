/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "EngineAnimClasses.h"
#include "EngineMaterialClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAnimationClasses.h"

//#include "EngineParticleClasses.h"

IMPLEMENT_CLASS(AUTVehicle_HellBender);

void AUTVehicle_HellBender::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( Driver )
	{		

		const FLOAT EngineRPM = Cast<UUTVehicleSimCar>(SimObj)->EngineRPMCurve.Eval(ForwardVel, 0.0f);
		const FLOAT MaxRPM = Cast<UUTVehicleSimCar>(SimObj)->MaxRPM;

		// TODOJOE: Make a nice generic exhaust system

		const FLOAT Exhaust = Clamp<FLOAT>( (EngineRPM / MaxRPM), 0.0, 1.0);

		if (VehicleEffects.Num() > 4)
		{
			if (VehicleEffects(3).EffectRef != NULL)
			{
				VehicleEffects(3).EffectRef->SetFloatParameter(ExhaustEffectName, Exhaust);
			}
			if (VehicleEffects(4).EffectRef != NULL)
			{
				VehicleEffects(4).EffectRef->SetFloatParameter(ExhaustEffectName,Exhaust);
			}
		}
	}
		// Update brake light and reverse light
	// Both lights default to off.
	UBOOL bSetBrakeLightOn = false;
	UBOOL bSetReverseLightOn = false;

	// check if scorpion is braking
	if ( (OutputBrake > 0.f) || bOutputHandbrake )
	{
		bSetBrakeLightOn = true;
		if ( !bBrakeLightOn )
		{	
			// turn on brake light
			bBrakeLightOn = true;
			if(DamageMaterialInstance)
			{
				DamageMaterialInstance->SetScalarParameterValue(BrakeLightParameterName, 60.f );
			}
		}
	}

	// check if scorpion is in reverse
	if ( Throttle < 0.0f )
	{
		bSetReverseLightOn = true;
		if ( !bReverseLightOn )
		{
			// turn on reverse light
			bReverseLightOn = true;
			if(DamageMaterialInstance)
			{
				DamageMaterialInstance->SetScalarParameterValue(ReverseLightParameterName, 50.f );
			}
		}
	}

	if ( bBrakeLightOn && !bSetBrakeLightOn )
	{
		// turn off brake light
		bBrakeLightOn = false;
		if(DamageMaterialInstance)
		{
			DamageMaterialInstance->SetScalarParameterValue(BrakeLightParameterName, 0.f );
		}
	}
	if ( bReverseLightOn && !bSetReverseLightOn )
	{
		// turn off reverse light
		bReverseLightOn = false;
		if(DamageMaterialInstance)
		{
			DamageMaterialInstance->SetScalarParameterValue(ReverseLightParameterName, 0.f );
		}
	}

}

