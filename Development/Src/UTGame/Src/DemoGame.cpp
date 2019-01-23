//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================
#include "UTGame.h"
#include "UTGameSequenceClasses.h"
#include "EngineAudioDeviceClasses.h"

#include "UnPath.h"


IMPLEMENT_CLASS(USeqAct_DrumHit);
IMPLEMENT_CLASS(USeqAct_VelocityLoop);
IMPLEMENT_CLASS(UDemoCamMod_ScreenShake);

/**
* Overridden to support custom activation of various attachments.
*/
UBOOL USeqAct_DrumHit::UpdateOp(FLOAT DeltaTime)
{
	// check for an activation,
	if (InputLinks(0).bHasImpulse ||
		InputLinks(1).bHasImpulse)
	{
		// first play the hit sound
		if (HitSound != NULL)
		{
			AActor* SoundSource = NULL;
			// first check to see if sources were linked in
			TArray<UObject**> SoundSources;
			GetObjectVars(SoundSources,TEXT("Sound Source"));
			if (SoundSources.Num() > 0)
			{
				// grab the first valid one (first actor)
				for (INT Idx = 0; Idx < SoundSources.Num() && SoundSource == NULL; Idx++)
				{
					AActor* Actor = Cast<AActor>(*SoundSources(Idx));
					if (Actor != NULL &&
						!Actor->IsPendingKill())
					{
						SoundSource = Actor;
					}
				}
			}
			// default to the local player if no valid source was specified
			if (SoundSource == NULL)
			{
				for (FActorIterator It; It && SoundSource == NULL; ++It)
				{
					APlayerController* PC = Cast<APlayerController>(*It);
					if (PC != NULL &&
						!PC->IsPendingKill() &&
						PC->LocalPlayerController())
					{
						SoundSource = PC;
					}
				}
			}
			// play the actual sound cue
			check(SoundSource != NULL && "Unable to find valid sound source!");
			UAudioDevice::CreateComponent(HitSound,GWorld->Scene,SoundSource,1);
		}
		if (InputLinks(1).bHasImpulse)
		{
			// reset the pulse direction
			PulseDirection = 1.f;
			// cache any attached lights
			CachedLights.Empty();
			TArray<UObject**> Lights;
			GetObjectVars(Lights,TEXT("Light"));
			if (Lights.Num() > 0)
			{
				for (INT Idx = 0; Idx < Lights.Num(); Idx++)
				{
					ALight* Light = Cast<ALight>(*Lights(Idx));
					if (Light != NULL)
					{
						CachedLights.AddItem(Light);
					}
				}
			}
			// enable any emitters
			TArray<UObject**> Emitters;
			GetObjectVars(Emitters,TEXT("Emitter"));
			if (Emitters.Num() > 0)
			{
				for (INT Idx = 0; Idx < Emitters.Num(); Idx++)
				{
					AEmitter* Emitter = Cast<AEmitter>(*Emitters(Idx));
					if (Emitter != NULL &&
						Emitter->ParticleSystemComponent != NULL)
					{
						Emitter->ParticleSystemComponent->ActivateSystem();
					}
				}
			}
		}
		else
		{
			// no interpolation, only activating sound
			PulseDirection = 0.f;
			LightBrightness = 0.f;
			// reset all components to min brightness
			for (INT Idx = 0; Idx < CachedLights.Num(); Idx++)
			{
				ULightComponent* LightComponent = CachedLights(Idx)->LightComponent;
				if (LightComponent != NULL)
				{
					LightComponent->Brightness = BrightnessRange->Min;
				}
			}
		}
	}
	if (PulseDirection != 0.f)
	{
		// interpolate any attached lights
		for (INT Idx = 0; Idx < CachedLights.Num(); Idx++)
		{
			ULightComponent* LightComponent = CachedLights(Idx)->LightComponent;
			if (LightComponent != NULL)
			{
				LightComponent->Brightness = Lerp<FLOAT>(BrightnessRange->Min,BrightnessRange->Max,LightBrightness);
			}
		}
		if (PulseDirection == 1.f)
		{
			LightBrightness += DeltaTime * PulseDirection * (1.f/BrightnessRampUpRate);
		}
		else
		{
			LightBrightness += DeltaTime * PulseDirection * (1.f/BrightnessRampDownRate);
		}
		// check for a pulse down
		if (LightBrightness >= 1.f &&
			PulseDirection == 1.f)
		{
			LightBrightness = 1.f;
			PulseDirection = -1.f;
		}
		else
			if (PulseDirection == -1.f &&
				LightBrightness < 0.f)
			{
				LightBrightness = 0.f;
				// reset all components to min brightness
				for (INT Idx = 0; Idx < CachedLights.Num(); Idx++)
				{
					ULightComponent* LightComponent = CachedLights(Idx)->LightComponent;
					if (LightComponent != NULL)
					{
						LightComponent->Brightness = BrightnessRange->Min;
					}
				}
			}
	}
	// finish once pulse down completes
	return (PulseDirection == 0.f || (PulseDirection == -1.f && LightBrightness == 0.f));
}

void USeqAct_VelocityLoop::StartSound()
{
	for (INT Idx = 0; Idx < SoundSources.Num(); Idx++)
	{
		AActor* Source = SoundSources(Idx);
		if (Source != NULL)
		{
			UAudioDevice::CreateComponent(LoopSound,GWorld->Scene,Source,1);
		}
	}
	bPlayingSound = 1;
}

void USeqAct_VelocityLoop::StopSound()
{
	for (INT Idx = 0; Idx < SoundSources.Num(); Idx++)
	{
		AActor* Source = SoundSources(Idx);
		if (Source != NULL)
		{
			// search for the matching audio component
			for (INT CompIdx = 0; CompIdx < Source->Components.Num(); CompIdx++)
			{
				UAudioComponent* Comp = Cast<UAudioComponent>(Source->Components(CompIdx));
				if (Comp != NULL &&
					Comp->SoundCue == LoopSound)
				{
					Source->Components.Remove(CompIdx--,1);
					Comp->Stop();
				}
			}
		}
	}
	bPlayingSound = 0;
}

UBOOL USeqAct_VelocityLoop::UpdateOp(FLOAT DeltaTime)
{
	if (InputLinks(0).bHasImpulse)
	{
		debugf(TEXT("Hello!"));
		// kill any previous sources
		if (bPlayingSound)
		{
			StopSound();
			SoundSources.Empty();
		}
		// build a list of new sound sources
		TArray<UObject**> ObjVars;
		GetObjectVars(ObjVars,TEXT("Target"));
		for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
		{
			AActor* Source = Cast<AActor>(*ObjVars(Idx));
			if (Source != NULL)
			{
				SoundSources.AddItem(Source);
			}
		}
		// total the velocity
		TArray<FLOAT*> FloatVars;
		GetFloatVars(FloatVars,TEXT("Velocity"));
		TotalVelocity = 0.f;
		for (INT Idx = 0; Idx < FloatVars.Num(); Idx++)
		{
			TotalVelocity += *FloatVars(Idx);
		}
	}
	else if (InputLinks(1).bHasImpulse)
	{
		debugf(TEXT("GoodBye!"));
		if (bPlayingSound)
		{
			StopSound();
			SoundSources.Empty();
		}
		return TRUE;
	}
	else
	{
		// check to see if the sound should start/stop
		FLOAT CurrentVelocity = 0.f;
		for (INT Idx = 0; Idx < SoundSources.Num(); Idx++)
		{
			CurrentVelocity += SoundSources(Idx)->Velocity.Size();
		}
		if (CurrentVelocity > TotalVelocity)
		{
			// should be playing
			if (!bPlayingSound)
			{
				debugf(TEXT("PlaySound!"));
				StartSound();
			}
		}
		else
		{
			// shouldn't be playing
			if (bPlayingSound)
			{
				debugf(TEXT("StopSound!"));
				StopSound();
			}
		}
	}

	return FALSE;
}


void UDemoCamMod_ScreenShake::UpdateScreenShake(FLOAT DeltaTime, FScreenShakeStruct& Shake, FTPOV& OutPOV)
{
	Shake.TimeToGo -= DeltaTime;

	// Do not update screen shake if not needed
	if( Shake.TimeToGo <= 0.f )
	{
		return;
	}

	// Smooth fade out
	FLOAT ShakePct = Clamp<FLOAT>(Shake.TimeToGo / Shake.TimeDuration, 0.f, 1.f);
	ShakePct = ShakePct*ShakePct*(3.f - 2.f*ShakePct);

	// do not update if percentage is null
	if( ShakePct <= 0.f )
	{
		return;
	}

	// View Offset, Compute sin wave value for each component
	if( !Shake.LocAmplitude.IsZero() )
	{
		FVector	LocOffset = FVector(0);
		if( Shake.LocAmplitude.X != 0.0 ) 
		{
			Shake.LocSinOffset.X += DeltaTime * Shake.LocFrequency.X * ShakePct;
			LocOffset.X = Shake.LocAmplitude.X * appSin(Shake.LocSinOffset.X) * ShakePct;
		}
		if( Shake.LocAmplitude.Y != 0.0 ) 
		{
			Shake.LocSinOffset.Y += DeltaTime * Shake.LocFrequency.Y * ShakePct;
			LocOffset.Y = Shake.LocAmplitude.Y * appSin(Shake.LocSinOffset.Y) * ShakePct;
		}
		if( Shake.LocAmplitude.Z != 0.0 ) 
		{
			Shake.LocSinOffset.Z += DeltaTime * Shake.LocFrequency.Z * ShakePct;
			LocOffset.Z = Shake.LocAmplitude.Z * appSin(Shake.LocSinOffset.Z) * ShakePct;
		}

		// Offset is relative to camera orientation
		FRotationMatrix CamRotMatrix(OutPOV.Rotation);
		OutPOV.Location += LocOffset.X * CamRotMatrix.GetAxis(0) + LocOffset.Y * CamRotMatrix.GetAxis(1) + LocOffset.Z * CamRotMatrix.GetAxis(2);
	}

	// View Rotation, compute sin wave value for each component
	if( !Shake.RotAmplitude.IsZero() )
	{
		FVector	RotOffset = FVector(0);
		if( Shake.RotAmplitude.X != 0.0 ) 
		{
			Shake.RotSinOffset.X += DeltaTime * Shake.RotFrequency.X * ShakePct;
			RotOffset.X = Shake.RotAmplitude.X * appSin(Shake.RotSinOffset.X);
		}
		if( Shake.RotAmplitude.Y != 0.0 ) 
		{
			Shake.RotSinOffset.Y += DeltaTime * Shake.RotFrequency.Y * ShakePct;
			RotOffset.Y = Shake.RotAmplitude.Y * appSin(Shake.RotSinOffset.Y);
		}
		if( Shake.RotAmplitude.Z != 0.0 ) 
		{
			Shake.RotSinOffset.Z += DeltaTime * Shake.RotFrequency.Z * ShakePct;
			RotOffset.Z = Shake.RotAmplitude.Z * appSin(Shake.RotSinOffset.Z);
		}
		RotOffset				*= ShakePct;
		OutPOV.Rotation.Pitch	+= appTrunc(RotOffset.X);
		OutPOV.Rotation.Yaw		+= appTrunc(RotOffset.Y);
		OutPOV.Rotation.Roll	+= appTrunc(RotOffset.Z);
	}

	// Compute FOV change
	if( Shake.FOVAmplitude != 0.0 ) 
	{
		Shake.FOVSinOffset	+= DeltaTime * Shake.FOVFrequency * ShakePct;
		OutPOV.FOV			+= ShakePct * Shake.FOVAmplitude * appSin(Shake.FOVSinOffset);
	}
}
