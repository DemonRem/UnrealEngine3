/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "ScenePrivate.h"

IMPLEMENT_CLASS(ULightEnvironmentComponent);

void ULightEnvironmentComponent::ResetLightInteractions()
{
	TArray<ULightComponent*> OldLights = Lights;

	// Reset the component's light list.
	Lights.Empty(Lights.Num());

	// Reset the scene info's light list.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ResetLightInteractions,
		FLightEnvironmentSceneInfo*,SceneInfo,SceneInfo,
		{
			SceneInfo->Lights.Empty();
		});

	// Flag the light for reattachment, to ensure the scene is updated to include its interaction with the light environment.
	for(INT LightIndex = 0;LightIndex < OldLights.Num();LightIndex++)
	{
		ULightComponent* Light = OldLights(LightIndex);
		if(Light && Light->IsAttached())
		{
			Light->BeginDeferredReattach();
		}
	}
}

void ULightEnvironmentComponent::SetLightInteraction(ULightComponent* Light,UBOOL bAffectThisLightEnvironment)
{
	// Update the component's light list.
	if(bAffectThisLightEnvironment)
	{
		Lights.AddUniqueItem(Light);
	}
	else
	{
		Lights.RemoveItem(Light);
	}

	// Update the scene info's light list.
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		SetLightInteraction,
		FLightEnvironmentSceneInfo*,SceneInfo,SceneInfo,
		ULightComponent*,Light,Light,
		UBOOL,bAffectThisLightEnvironment,bAffectThisLightEnvironment,
		{
			if(bAffectThisLightEnvironment)
			{
				SceneInfo->Lights.Set(Light,TRUE);
			}
			else
			{
				SceneInfo->Lights.Remove(Light);
			}
		});

	if(Light->IsAttached())
	{
		// Flag the light for reattachment, to ensure the scene is updated to include its interaction with the light environment.
		Light->BeginDeferredReattach();
	}
}

void ULightEnvironmentComponent::SetEnabled(UBOOL bNewEnabled)
{
	// Make a copy of the component's owner, since this component may be detached during the call.
	AActor* LocalOwner = Owner;

	// Detach the owner's components.
	// This assumes that all the components using the light environment are attached to the same actor as the light environment.
	if(LocalOwner)
	{
		LocalOwner->ClearComponents();
	}

	bEnabled = bNewEnabled;

	// Reattach the owner's components with the new enabled state.
	if(LocalOwner)
	{
		LocalOwner->ConditionalUpdateComponents(FALSE);
	}
}

ULightEnvironmentComponent::ULightEnvironmentComponent()
{
	if(!IsTemplate())
	{
		SceneInfo = new FLightEnvironmentSceneInfo(this);
	}
}

void ULightEnvironmentComponent::Attach()
{
	Super::Attach();
	Scene->AddLightEnvironment(this);
}

void ULightEnvironmentComponent::Detach()
{
	Super::Detach();
	Scene->RemoveLightEnvironment(this);
}

/**
 * Cleans up the FLightEnvironmentSceneInfo in the rendering thread when the light environment is deleted.
 */
static void DeleteSceneInfo(FLightEnvironmentSceneInfo* SceneInfo)
{
	check(IsInRenderingThread());

	// Ensure that no FLightSceneInfos or FPrimitiveSceneInfos reference the FLightEnvironmentSceneInfo.
	for(INT LightIndex = 0;LightIndex < SceneInfo->AttachedLights.Num();LightIndex++)
	{
		SceneInfo->AttachedLights(LightIndex)->LightEnvironments.RemoveItem(SceneInfo);
	}
	for(INT PrimitiveIndex = 0;PrimitiveIndex < SceneInfo->AttachedPrimitives.Num();PrimitiveIndex++)
	{
		SceneInfo->AttachedPrimitives(PrimitiveIndex)->LightEnvironmentSceneInfo = NULL;
	}

	delete SceneInfo;
}

void ULightEnvironmentComponent::FinishDestroy()
{
	if(SceneInfo)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			DeleteLightEnvironmentSceneInfo,
			FLightEnvironmentSceneInfo*,SceneInfo,SceneInfo,
			{
				DeleteSceneInfo(SceneInfo);
			});
	}
	SceneInfo = NULL;

	Super::FinishDestroy();
}
