/*=============================================================================
	Light actor creation from FBX data.
=============================================================================*/

#include "UnrealEd.h"

#if WITH_FBX

#include "Factories.h"
#include "Engine.h"
#include "EngineMaterialClasses.h"

#include "UnFbxImporter.h"

using namespace UnFbx;

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
ALight* CFbxImporter::CreateLight(KFbxLight* FbxLight)
{
	ALight* UnrealLight = NULL;
	FString ActorName = ANSI_TO_TCHAR(MakeName(FbxLight->GetName()));

	// create the light actor
	switch (FbxLight->LightType.Get())
	{
	case KFbxLight::ePOINT:
		UnrealLight = Cast<ALight>(GWorld->SpawnActor(APointLight::StaticClass(),*ActorName));
		break;
	case KFbxLight::eDIRECTIONAL:
		UnrealLight = Cast<ALight>(GWorld->SpawnActor(ADirectionalLight::StaticClass(),*ActorName));
		break;
	case KFbxLight::eSPOT:
		UnrealLight = Cast<ALight>(GWorld->SpawnActor(ASpotLight::StaticClass(),*ActorName));
		break;
	}

	if (UnrealLight)
	{
		FillLightComponent(FbxLight,UnrealLight->LightComponent);	
	}
	
	return UnrealLight;
}

UBOOL CFbxImporter::FillLightComponent(KFbxLight* FbxLight, ULightComponent* UnrealLightComponent)
{
	fbxDouble3 Color = FbxLight->Color.Get();
	FColor UnrealColor( BYTE(255.0*Color[0]), BYTE(255.0*Color[1]), BYTE(255.0*Color[2]) );
	UnrealLightComponent->LightColor = UnrealColor;

	fbxDouble1 Intensity = FbxLight->Intensity.Get();
	UnrealLightComponent->Brightness = (FLOAT)Intensity/100.f;

	UnrealLightComponent->CastShadows = FbxLight->CastShadows.Get();

	switch (FbxLight->LightType.Get())
	{
	// point light properties
	case KFbxLight::ePOINT:
		{
			UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(UnrealLightComponent);
			if (PointLightComponent)
			{
				fbxDouble1 DecayStart = FbxLight->DecayStart.Get();
				PointLightComponent->Radius = Converter.ConvertDist(DecayStart);

				KFbxLight::EDecayType Decay = FbxLight->DecayType.Get();
				if (Decay == KFbxLight::eNONE)
				{
					PointLightComponent->Radius = K_FLOAT_MAX;
				}
			}
			else
			{
				warnf(NAME_Error,TEXT("FBX Light type 'Point' does not match unreal light component"));
			}
		}
		break;
	// spot light properties
	case KFbxLight::eSPOT:
		{
			USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(UnrealLightComponent);
			if (SpotLightComponent)
			{
				fbxDouble1 DecayStart = FbxLight->DecayStart.Get();
				SpotLightComponent->Radius = Converter.ConvertDist(DecayStart);
				KFbxLight::EDecayType Decay = FbxLight->DecayType.Get();
				if (Decay == KFbxLight::eNONE)
				{
					SpotLightComponent->Radius = K_FLOAT_MAX;
				}
				SpotLightComponent->InnerConeAngle = FbxLight->HotSpot.Get();
				SpotLightComponent->OuterConeAngle = FbxLight->ConeAngle.Get();
			}
			else
			{
				warnf(NAME_Error,TEXT("FBX Light type 'Spot' does not match unreal light component"));
			}
		}
		break;
	// directional light properties 
	case KFbxLight::eDIRECTIONAL:
		{
			// nothing specific
		}
		break;
	}

	return TRUE;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
ACameraActor* CFbxImporter::CreateCamera(KFbxCamera* FbxCamera)
{
	ACameraActor* UnrealCamera = NULL;
	FString ActorName = ANSI_TO_TCHAR(MakeName(FbxCamera->GetName()));
	UnrealCamera = Cast<ACameraActor>(GWorld->SpawnActor(ACameraActor::StaticClass(),*ActorName));
	if (UnrealCamera)
	{
		UnrealCamera->FOVAngle = FbxCamera->FieldOfView.Get();
	}
	return UnrealCamera;
}

#endif // WITH_FBX
