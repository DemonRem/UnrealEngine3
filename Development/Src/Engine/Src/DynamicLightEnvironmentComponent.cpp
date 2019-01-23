/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "DynamicLightEnvironmentComponent.h"

IMPLEMENT_CLASS(UDynamicLightEnvironmentComponent);

/** Compute the direction which the spherical harmonic is highest at. */
static FVector SHGetMaximumDirection(const FSHVector& SH,UBOOL bLowerHemisphere,UBOOL bUpperHemisphere)
{
	// This is an approximation which only takes into account first and second order spherical harmonics.
	FLOAT Z = SH.V[2];
	if(!bLowerHemisphere)
	{
		Z = Max(Z,0.0f);
	}
	if(!bUpperHemisphere)
	{
		Z = Min(Z,0.0f);
	}
	return FVector(
		-SH.V[3],
		-SH.V[1],
		Z
		);
}

/** Compute the direction which the spherical harmonic is lowest at. */
static FVector SHGetMinimumDirection(const FSHVector& SH,UBOOL bLowerHemisphere,UBOOL bUpperHemisphere)
{
	// This is an approximation which only takes into account first and second order spherical harmonics.
	FLOAT Z = -SH.V[2];
	if(!bLowerHemisphere)
	{
		Z = Max(Z,0.0f);
	}
	if(!bUpperHemisphere)
	{
		Z = Min(Z,0.0f);
	}
	return FVector(
		+SH.V[3],
		+SH.V[1],
		Z
		);
}

/** Computes a brightness and a fixed point color from a floating point color. */
static void ComputeLightBrightnessAndColor(const FLinearColor& InLinearColor,FColor& OutColor,FLOAT& OutBrightness)
{
	FLOAT MaxComponent = Max(DELTA,Max(InLinearColor.R,Max(InLinearColor.G,InLinearColor.B)));
	OutColor = InLinearColor / MaxComponent;
	OutBrightness = MaxComponent;
}

/** Returns the SH coefficients for a point light from the given direction and brightness. */
static FSHVector PointLightSH(const FVector& Direction)
{
	FSHVector Result = SHBasisFunction(Direction.SafeNormal());

	// Normalize the SH so its surface adds up to 1.
	static const FLOAT InvSum = appInvSqrt(Dot(Result,Result));
	Result *= InvSum;

	return Result;
}

// Sky light globals.
static UBOOL bComputedSkyFunctions = FALSE;
static FSHVector UpperSkyFunction;
static FSHVector LowerSkyFunction;
static FSHVector AmbientFunction;

/** Precomputes the SH coefficients for the upper and lower sky hemisphere functions. */
static void InitSkyLightSH()
{
	if(!bComputedSkyFunctions)
	{
		bComputedSkyFunctions = TRUE;

		// Use numerical integration to project the sky visibility functions into the SH basis.
		static const INT NumIntegrationSamples = 1024;
		for(INT DirectionIndex = 0;DirectionIndex < NumIntegrationSamples;DirectionIndex++)
		{
			FVector Direction = VRand();
			FSHVector DirectionBasis = SHBasisFunction(Direction);
			if(Direction.Z > 0.0f)
			{
				UpperSkyFunction += DirectionBasis;
			}
			else
			{
				LowerSkyFunction += DirectionBasis;
			}
		}

		// Normalize the sky SH projections.
		UpperSkyFunction /= Dot(UpperSkyFunction,PointLightSH(FVector(0,0,+1)));
		LowerSkyFunction /= Dot(LowerSkyFunction,PointLightSH(FVector(0,0,-1)));

		// The ambient function is simply a constant 1 across its surface; the combination of the upper and lower sky functions.
		AmbientFunction = UpperSkyFunction + LowerSkyFunction;
	}
}

/** Clamps each color component above 0. */
static FLinearColor GetPositiveColor(const FLinearColor& Color)
{
	return FLinearColor(
		Max(Color.R,0.0f),
		Max(Color.G,0.0f),
		Max(Color.B,0.0f),
		Color.A
		);
}

/** Remove skylighting from a light environment. */
static void ExtractEnvironmentSkyLight(FSHVectorRGB& LightEnvironment,FLinearColor& OutSkyColor,UBOOL bLowerHemisphere,UBOOL bUpperHemisphere)
{
	FSHVector LightEnvironmentLuminance = LightEnvironment.GetLuminance();
	FVector MinDirection = SHGetMinimumDirection(LightEnvironmentLuminance,bLowerHemisphere,bUpperHemisphere);
	FSHVector UnitLightSH = PointLightSH(MinDirection);

	FLinearColor Intensity = GetPositiveColor(Dot(LightEnvironment,UnitLightSH));

	if(Intensity.R > 0.0f || Intensity.G > 0.0f || Intensity.B > 0.0f)
	{
		OutSkyColor += Intensity;

		if(bLowerHemisphere)
		{
			LightEnvironment -= LowerSkyFunction * Intensity;
		}
		if(bUpperHemisphere)
		{
			LightEnvironment -= UpperSkyFunction * Intensity;
		}
	}
}

/** Adds a skylight to a light environment. */
static void AddSkyLightEnvironment(const USkyLightComponent* SkyLight,FSHVectorRGB& OutLightEnvironment)
{
	OutLightEnvironment += UpperSkyFunction * FLinearColor(SkyLight->LightColor) * SkyLight->Brightness;
	OutLightEnvironment += LowerSkyFunction * FLinearColor(SkyLight->LowerColor) * SkyLight->LowerBrightness;
}

FDynamicLightEnvironmentState::FDynamicLightEnvironmentState(UDynamicLightEnvironmentComponent* InComponent):
	Component(InComponent)
,	PredictedOwnerPosition(0,0,0)
,	LastUpdateTime(0)
,	InvisibleUpdateTime(InComponent->InvisibleUpdateTime)
,	MinTimeBetweenFullUpdates(InComponent->MinTimeBetweenFullUpdates)
,	bFirstFullUpdate(TRUE)
{
	InitSkyLightSH();

	// Initialize the random light visibility sample points.
	const INT NumLightVisibilitySamplePoints = Component->NumVolumeVisibilitySamples;
	LightVisibilitySamplePoints.Empty(NumLightVisibilitySamplePoints);
	for(INT PointIndex = 1;PointIndex < NumLightVisibilitySamplePoints;PointIndex++)
	{
		LightVisibilitySamplePoints.AddItem(
			FVector(
				-1.0f + 2.0f * appSRand(),
				-1.0f + 2.0f * appSRand(),
				-1.0f + 2.0f * appSRand()
				)
			);
	}

	// Always place one sample at the center of the owner's bounds.
	LightVisibilitySamplePoints.AddItem(FVector(0,0,0));
}

void FDynamicLightEnvironmentState::UpdateEnvironment(FLOAT DeltaTime,UBOOL bPerformFullUpdate,UBOOL bForceStaticLightUpdate)
{
	// Update the owner's components.
	Component->GetOwner()->ConditionalUpdateComponents();

	SCOPE_CYCLE_COUNTER(STAT_UpdateEnvironmentTime);
	INC_DWORD_STAT(STAT_EnvironmentUpdates);

	// Find the owner's bounds and lighting channels.
	const FBoxSphereBounds PreviousOwnerBounds = OwnerBounds;
	OwnerBounds = FBoxSphereBounds(Component->GetOwner()->Location,FVector(0,0,0),50);
	OwnerLightingChannels.Bitfield = 0;
	OwnerLightingChannels.bInitialized = TRUE;
	for(INT ComponentIndex = 0;ComponentIndex < Component->GetOwner()->AllComponents.Num();ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component->GetOwner()->AllComponents(ComponentIndex));

		// Only look at primitives which use this light environment.
		if(Primitive && Primitive->LightEnvironment == Component)
		{
			// Add the primitive's bounds to the composite owner bounds.
			OwnerBounds = OwnerBounds + Primitive->Bounds;

			// Add the primitive's lighting channels to the composite owner lighting channels.
			OwnerLightingChannels.Bitfield |= Primitive->LightingChannels.Bitfield;
		}
	}

	// Find the owner's package.
	OwnerPackage = Component->GetOwner()->GetOutermost();

	// Attempt to predict the owner's position at the next update.
	const FVector PreviousPredictedOwnerPosition = PredictedOwnerPosition;

	// Disable the velocity offset for the moment, as it causes artifacts when the velocity offset ends in a wall.
#if 0
	if(!bFirstFullUpdate)
	{
		// Compute the owner's velocity.
		const FLOAT FramesPerUpdate = MinTimeBetweenFullUpdates / Max(DELTA,DeltaTime);
		const FVector Velocity = (OwnerBounds.Origin - PreviousOwnerBounds.Origin) * FramesPerUpdate;

		// Predict that the owner will proceed with the same velocity.
		PredictedOwnerPosition = OwnerBounds.Origin + Velocity * 0.5f;
	}
	else
#endif
	{
		PredictedOwnerPosition = OwnerBounds.Origin;
	}

	// Only iterate over all lights performing the slow fully update if wanted.
	if( bPerformFullUpdate )
	{
		INC_DWORD_STAT(STAT_EnvironmentFullUpdates);

		// Only perform a full update of the static light environment if the primitive component has moved.
		if( bFirstFullUpdate || bForceStaticLightUpdate
		||	(PredictedOwnerPosition != PreviousPredictedOwnerPosition) 
		||	(OwnerBounds.SphereRadius != PreviousOwnerBounds.SphereRadius) )
		{
			// Reset as the below code is going to accumulate from scratch.
			NewStaticLightEnvironment = FSHVectorRGB();
			NewStaticShadowEnvironment	= FSHVectorRGB();

			// Iterate over static lights and update the static light environment.
			const INT MaxStaticLightIndex = GWorld->StaticLightList.GetMaxIndex();
			for(TSparseArray<ULightComponent*>::TConstIterator LightIt(GWorld->StaticLightList);LightIt;++LightIt)
			{
				const ULightComponent* Light = *LightIt;

				// Prefetch next index. This can potentially be empty but in practice isn't most of the time.
				const INT NextLightIndex = LightIt.GetIndex() + 1;
				if( NextLightIndex < MaxStaticLightIndex )
				{
					PREFETCH( &GWorld->StaticLightList(NextLightIndex) );
				}

				// Add static light to static light environment
				AddLightToEnvironment( Light, NewStaticLightEnvironment, NewStaticShadowEnvironment, PredictedOwnerPosition );
			}

			// Add the ambient shadow source.
			NewStaticShadowEnvironment += PointLightSH(Component->AmbientShadowSourceDirection) * Component->AmbientShadowColor;

			INC_DWORD_STAT(STAT_StaticEnvironmentFullUpdates);
		}
		else
		{
			INC_DWORD_STAT(STAT_StaticEnvironmentFullUpdatesSkipped);
		}
	}

	DynamicLightEnvironment = FSHVectorRGB();
	DynamicShadowEnvironment = FSHVectorRGB();
	DynamicLights.Empty(DynamicLights.Num());

	if(Component->bDynamic)
	{
		// Iterate over dynamic lights and update the dynamic light environment.
		const INT MaxDynamicLightIndex = GWorld->DynamicLightList.GetMaxIndex();
		for(TSparseArray<ULightComponent*>::TConstIterator LightIt(GWorld->DynamicLightList);LightIt;++LightIt)
		{
			ULightComponent* Light = *LightIt;

			// Prefetch next index. This can potentially be empty but in practice isn't most of the time.
			const INT NextLightIndex = LightIt.GetIndex() + 1;
			if( NextLightIndex < MaxDynamicLightIndex )
			{
				PREFETCH( &GWorld->DynamicLightList(NextLightIndex) );
			}

			if(GSystemSettings.bUseCompositeDynamicLights)
			{
				// Add the dynamic light to the light environment.
				AddLightToEnvironment(Light,DynamicLightEnvironment,DynamicShadowEnvironment,OwnerBounds.Origin);
			}
			else
			{
				// Add the dynamic light to the dynamic light list if it affects the owner.
				if(DoesLightAffectOwner(Light,OwnerBounds.Origin))
				{
					DynamicLights.AddItem(Light);
				}
			}
		}
	}

	// Smoothly interpolate the static light set.
	const FLOAT RemainingTransitionTime = Max(DELTA,LastUpdateTime + MinTimeBetweenFullUpdates - GWorld->GetTimeSeconds());
	const FLOAT TransitionAlpha =
		bFirstFullUpdate ?
			1.0f :
			Clamp(DeltaTime / RemainingTransitionTime,0.0f,1.0f);
	StaticLightEnvironment = StaticLightEnvironment * (1.0f - TransitionAlpha) + NewStaticLightEnvironment * TransitionAlpha;
	StaticShadowEnvironment = StaticShadowEnvironment * (1.0f - TransitionAlpha) + NewStaticShadowEnvironment * TransitionAlpha;
}

void FDynamicLightEnvironmentState::Tick(FLOAT DeltaTime)
{
	INC_DWORD_STAT(STAT_NumEnvironments);

	// Determine if the light environment's primitives have been rendered in the last second.
	const FLOAT CurrentTime			= GWorld->GetTimeSeconds();
	const FLOAT TimeSinceLastUpdate	= CurrentTime - LastUpdateTime;
	const UBOOL bVisible = (CurrentTime - Component->LastRenderTime) < 1.0f;

	// Only update the light environment if it's visible, or it hasn't been updated for the last InvisibleUpdateTime seconds.
	const UBOOL bDynamicUpdateNeeded = Component->bDynamic && (bVisible || TimeSinceLastUpdate > InvisibleUpdateTime);
	if(bFirstFullUpdate || bDynamicUpdateNeeded)
	{
		// Spread out updating invisible components	 over several frames to avoid spikes by varying invisible update time by +/- 20%.
		InvisibleUpdateTime = Component->InvisibleUpdateTime * (0.8 + 0.4 * appSRand());

		// Only perform full updates every so often.
		UBOOL bPerformFullUpdate = FALSE;
		if( bFirstFullUpdate || TimeSinceLastUpdate > MinTimeBetweenFullUpdates )
		{
			LastUpdateTime				= CurrentTime;
			bPerformFullUpdate			= TRUE;
			// Create variance to avoid spikes caused by multiple components being updated the same frame.
			MinTimeBetweenFullUpdates	= Component->MinTimeBetweenFullUpdates * (0.8 + 0.4 * appSRand());
		}

		// Update the light environment.
		UpdateEnvironment(DeltaTime,bPerformFullUpdate,FALSE);

		// Update the lights from the environment.
		CreateEnvironmentLightList();

		bFirstFullUpdate = FALSE;
	}
}

void FDynamicLightEnvironmentState::CreateRepresentativeLight(FSHVectorRGB& RemainingLightEnvironment,UBOOL bCastLight,UBOOL bCastShadows) const
{
	// Find the direction in the light environment with the highest luminance.
	FSHVector RemainingLuminance = RemainingLightEnvironment.GetLuminance();
	FVector MaxDirection = SHGetMaximumDirection(RemainingLuminance,TRUE,TRUE);
	if(MaxDirection.SizeSquared() >= DELTA)
	{
		// Calculate the light intensity for this direction.
		FSHVector UnitLightSH = PointLightSH(MaxDirection);
		FLinearColor Intensity = GetPositiveColor(Dot(RemainingLightEnvironment,UnitLightSH));

		if(Intensity.R > 0.0f || Intensity.G > 0.0f || Intensity.B > 0.0f)
		{
			// Remove this light from the environment.
			RemainingLightEnvironment -= UnitLightSH * Intensity;

			// Construct a directional light to represent the brightest direction of the remaining light environment.
			UPointLightComponent* Light = AllocateLight<UPointLightComponent>();
			const FVector LightDirection = MaxDirection.SafeNormal();
			const FVector LightPosition = OwnerBounds.Origin + LightDirection * OwnerBounds.SphereRadius * Component->LightDistance;
			Light->LightingChannels = OwnerLightingChannels;
			Light->Radius = OwnerBounds.SphereRadius * (Component->LightDistance + Component->ShadowDistance + 2);
			Light->bAffectsDefaultLightEnvironment = FALSE;

			if(bCastLight)
			{
				const FLOAT RadialAttenuation = appPow(Max(1.0f - ((LightPosition - OwnerBounds.Origin) / Light->Radius).SizeSquared(),0.0f),Light->FalloffExponent);
				FLinearColor LinearLightColor = (Intensity / RadialAttenuation).Desaturate( Component->LightDesaturation );
				ComputeLightBrightnessAndColor(LinearLightColor,Light->LightColor,Light->Brightness);
			}
			else
			{
				Light->Brightness = 0.0f;
			}

			Light->CastShadows = bCastShadows;
			if(bCastShadows)
			{
				Light->LightShadowMode = LightShadow_Modulate;
				Light->ShadowFalloffExponent = 1.0f / 3.0f;
				Light->ModShadowFadeoutTime = Component->ModShadowFadeoutTime;
				Light->ModShadowFadeoutExponent = Component->ModShadowFadeoutExponent;
				Light->ShadowFilterQuality = Component->ShadowFilterQuality;

				// Choose a ModShadowColor based on the percent of the light environment that this light represents.
				FLinearColor RemainingIntensity = GetPositiveColor(Dot(RemainingLightEnvironment,AmbientFunction));
				Light->ModShadowColor.R = Min(1.0f,RemainingIntensity.R / Max(RemainingIntensity.R + Intensity.R,DELTA));
				Light->ModShadowColor.G = Min(1.0f,RemainingIntensity.G / Max(RemainingIntensity.G + Intensity.G,DELTA));
				Light->ModShadowColor.B = Min(1.0f,RemainingIntensity.B / Max(RemainingIntensity.B + Intensity.B,DELTA));
			}

			Component->SetLightInteraction(Light,TRUE);
			
			// Attach the light after it is associated with the light environment to ensure it is only attached once.
			Light->ConditionalAttach(Component->GetScene(),NULL,FTranslationMatrix(LightPosition));
		}
	}
}

void FDynamicLightEnvironmentState::DetachRepresentativeLights() const
{
	// Detach the environment's representative lights.
	for(INT LightIndex = 0;LightIndex < RepresentativeLightPool.Num();LightIndex++)
	{
		RepresentativeLightPool(LightIndex)->ConditionalDetach();
	}
}

void FDynamicLightEnvironmentState::CreateEnvironmentLightList() const
{
	SCOPE_CYCLE_COUNTER(STAT_CreateLightsTime);

	FSHVectorRGB RemainingLightEnvironment = StaticLightEnvironment + DynamicLightEnvironment;

	// Detach the old representative lights.
	DetachRepresentativeLights();

	// Reset the light environment's light list.
	Component->ResetLightInteractions();

	// Add the dynamic lights to the light environment's light list.
	for(INT LightIndex = 0;LightIndex < DynamicLights.Num();LightIndex++)
	{
		if(DynamicLights(LightIndex))
		{
			Component->SetLightInteraction(DynamicLights(LightIndex),TRUE);
		}
	}

	// Move as much light as possible into the sky light.
	FLinearColor LowerSkyLightColor(FLinearColor::Black);
	FLinearColor UpperSkyLightColor(FLinearColor::Black);
	ExtractEnvironmentSkyLight(RemainingLightEnvironment,UpperSkyLightColor,FALSE,TRUE);
	ExtractEnvironmentSkyLight(RemainingLightEnvironment,LowerSkyLightColor,TRUE,FALSE);

	// Create a point light that is representative of the light environment.
	CreateRepresentativeLight(RemainingLightEnvironment,TRUE,FALSE);

	// Create a shadow-only point light that is representative of the shadow-casting light environment.
	if( Component->bCastShadows && GSystemSettings.bAllowLightEnvironmentShadows )
	{
		FSHVectorRGB RemainingShadowLightEnvironment = StaticShadowEnvironment + DynamicShadowEnvironment;
		CreateRepresentativeLight(RemainingShadowLightEnvironment,FALSE,TRUE);
	}

	// Add the remaining light to the sky light as ambient light.
	FLinearColor RemainingIntensity = Dot(RemainingLightEnvironment,AmbientFunction);
	UpperSkyLightColor += RemainingIntensity;
	LowerSkyLightColor += RemainingIntensity;
	RemainingLightEnvironment -= AmbientFunction * RemainingIntensity;

	// Create a sky light for the lights not represented by the directional lights.
	USkyLightComponent* SkyLight = AllocateLight<USkyLightComponent>();
	SkyLight->LightingChannels = OwnerLightingChannels;
	SkyLight->bAffectsDefaultLightEnvironment = FALSE;

	// Desaturate sky light color and add ambient glow afterwards.
	UpperSkyLightColor = UpperSkyLightColor.Desaturate( Component->LightDesaturation ) + Component->AmbientGlow;
	LowerSkyLightColor = LowerSkyLightColor.Desaturate( Component->LightDesaturation ) + Component->AmbientGlow;

	// Convert linear color to color and brightness pair.
	ComputeLightBrightnessAndColor(UpperSkyLightColor,SkyLight->LightColor,SkyLight->Brightness);
	ComputeLightBrightnessAndColor(LowerSkyLightColor,SkyLight->LowerColor,SkyLight->LowerBrightness);
	
	Component->SetLightInteraction(SkyLight,TRUE);

	// Attach the skylight after it is associated with the light environment to ensure it is only attached once.
	SkyLight->ConditionalAttach(Component->GetScene(),NULL,FMatrix::Identity);
}

void FDynamicLightEnvironmentState::AddReferencedObjects(TArray<UObject*>& ObjectArray)
{
	// Add the light environment's dynamic lights.
	for(INT LightIndex = 0;LightIndex < DynamicLights.Num();LightIndex++)
	{
		if (DynamicLights(LightIndex) != NULL && DynamicLights(LightIndex)->IsPendingKill())
		{
			DynamicLights(LightIndex) = NULL;
		}
		else
		{
			UObject::AddReferencedObject(ObjectArray,DynamicLights(LightIndex));
		}
	}

	// Add the light environment's representative lights.
	for(INT LightIndex = 0;LightIndex < RepresentativeLightPool.Num();LightIndex++)
	{
		UObject::AddReferencedObject(ObjectArray,RepresentativeLightPool(LightIndex));
	}
}

UBOOL FDynamicLightEnvironmentState::IsLightVisible(const ULightComponent* Light,const FVector& OwnerPosition,FLOAT& OutVisibilityFactor)
{
	SCOPE_CYCLE_COUNTER(STAT_LightVisibilityTime);

	// Sky lights are always visible.
	if(Light->IsA(USkyLightComponent::StaticClass()))
	{
		OutVisibilityFactor = 1.0f;
		return TRUE;
	}

	// Lights which don't cast static shadows are always visible.
	if(!Light->CastShadows || !Light->CastStaticShadows)
	{
		OutVisibilityFactor = 1.0f;
		return TRUE;
	}

	// Compute light visibility for one or more points within the owner's bounds.
	INT NumVisibleSamples = 0;
	for(INT SampleIndex = 0;SampleIndex < LightVisibilitySamplePoints.Num();SampleIndex++)
	{
		// Determine a random point to test visibility for in the owner's bounds.
		const FVector VisibilityTestPoint = PredictedOwnerPosition + LightVisibilitySamplePoints(SampleIndex) * OwnerBounds.BoxExtent;

		// Determine the direction from the primitive to the light.
		FVector4 LightPosition = Light->GetPosition();
		FVector LightVector = (FVector)LightPosition - VisibilityTestPoint * LightPosition.W;

		// Check the line between the light and the primitive's origin for occlusion.
		FCheckResult Hit(1.0f);
		const UBOOL bPointIsLit = GWorld->SingleLineCheck(
			Hit,
			NULL,
			VisibilityTestPoint,
			VisibilityTestPoint + LightVector,
			TRACE_Level|TRACE_Actors|TRACE_ShadowCast|TRACE_StopAtAnyHit,
			FVector(0,0,0),
			const_cast<ULightComponent*>(Light)
			);
		if(bPointIsLit)
		{
			NumVisibleSamples++;
		}
	}

	OutVisibilityFactor = (FLOAT)NumVisibleSamples / (FLOAT)LightVisibilitySamplePoints.Num();

	return OutVisibilityFactor > 0.0f;
}

template<typename LightType>
LightType* FDynamicLightEnvironmentState::AllocateLight() const
{
	// Try to find an unattached light of matching type in the representative light pool.
	for(INT LightIndex = 0;LightIndex < RepresentativeLightPool.Num();LightIndex++)
	{
		ULightComponent* Light = RepresentativeLightPool(LightIndex);
		if(Light && !Light->IsAttached() && Light->IsA(LightType::StaticClass()))
		{
			return CastChecked<LightType>(Light);
		}
	}

	// Create a new light.
	LightType* NewLight = ConstructObject<LightType>(LightType::StaticClass(),Component);
	RepresentativeLightPool.AddItem(NewLight);
	return NewLight;
}

UBOOL FDynamicLightEnvironmentState::DoesLightAffectOwner(const ULightComponent* Light,const FVector& OwnerPosition)
{
	// Skip disabled lights.
	if(!Light->bEnabled)
	{
		return FALSE;
	}

	// Use the CompositeDynamic lighting channel as the Dynamic lighting channel. 
	FLightingChannelContainer ConvertedLightingChannels = Light->LightingChannels;
	ConvertedLightingChannels.Dynamic = FALSE;
	if(ConvertedLightingChannels.CompositeDynamic)
	{
		ConvertedLightingChannels.CompositeDynamic = FALSE;
		ConvertedLightingChannels.Dynamic = TRUE;
	}

	// Skip lights which don't affect the owner's lighting channels.
	if(!ConvertedLightingChannels.OverlapsWith(OwnerLightingChannels))
	{
		return FALSE;
	}

	// Skip lights which don't affect the owner's level.
	if(!Light->AffectsLevel(OwnerPackage))
	{
		return FALSE;
	}

	// Skip lights which don't affect the owner's predicted bounds.
	if(!Light->AffectsBounds(FBoxSphereBounds(OwnerPosition,OwnerBounds.BoxExtent,OwnerBounds.SphereRadius)))
	{
		return FALSE;
	}

	return TRUE;
}

void FDynamicLightEnvironmentState::AddLightToEnvironment(const ULightComponent* Light, FSHVectorRGB& LightEnvironment, FSHVectorRGB& ShadowEnvironment,const FVector& OwnerPosition)
{
	// Determine whether the light affects the owner, and its visibility factor.
	FLOAT VisibilityFactor;
	if(DoesLightAffectOwner(Light,OwnerPosition) && IsLightVisible(Light,OwnerPosition,VisibilityFactor))
	{
		if(Light->IsA(USkyLightComponent::StaticClass()))
		{
			const USkyLightComponent* SkyLight = ConstCast<USkyLightComponent>(Light);

			// Add the sky light SH to the light environment SH.
			AddSkyLightEnvironment(SkyLight,LightEnvironment);

			if(Light->bCastCompositeShadow)
			{
				// Add the sky light SH to the shadow casting environment SH.
				AddSkyLightEnvironment(SkyLight,ShadowEnvironment);
			}
		}
		else
		{
			// Determine the direction from the primitive to the light.
			FVector4 LightPosition = Light->GetPosition();
			FVector LightVector = (FVector)LightPosition - OwnerPosition * LightPosition.W;

			// Compute the light's intensity at the actor's origin.
			const FLinearColor Intensity = Light->GetDirectIntensity(OwnerPosition) * VisibilityFactor;
			const FSHVectorRGB IndividualLightEnvironment = PointLightSH(LightVector) * Intensity;

			// Add the light to the light environment SH.
			LightEnvironment += IndividualLightEnvironment;

			if(Light->bCastCompositeShadow)
			{
				// Add the light to the shadow casting environment SH.
				ShadowEnvironment += IndividualLightEnvironment;
			}
		}
	}
}

void UDynamicLightEnvironmentComponent::FinishDestroy()
{
	Super::FinishDestroy();

	// Clean up the light environment's state.
	delete State;
	State = NULL;
}

void UDynamicLightEnvironmentComponent::AddReferencedObjects(TArray<UObject*>& ObjectArray)
{
	Super::AddReferencedObjects(ObjectArray);

	if(State)
	{
		State->AddReferencedObjects(ObjectArray);
	}
}

void UDynamicLightEnvironmentComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(!Ar.IsSaving() && !Ar.IsLoading())
	{
		// If serialization is being used to find references for garbage collection, use AddReferencedObjects to gather a list to serialize.
		TArray<UObject*> ReferencedObjects;
		AddReferencedObjects(ReferencedObjects);
		Ar << ReferencedObjects;
	}
}

void UDynamicLightEnvironmentComponent::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bEnabled)
	{
		// Update the light environment's state.
		check(State);
		State->Tick(DeltaTime);
	}
}

void UDynamicLightEnvironmentComponent::Attach()
{
	Super::Attach();

	if(bEnabled)
	{
		// Initialize the light environment's state the first time it's attached.
		if(!State)
		{
			State = new FDynamicLightEnvironmentState(this);
		}

		// Outside the game we're not ticked, so update the light environment on attach.
		if(!GIsGame)
		{
			State->UpdateEnvironment(MAX_FLT,TRUE,TRUE);

			// Add the light environment to the world's list, so it can be updated when static lights change.
			if(Scene->GetWorld())
			{
				Scene->GetWorld()->LightEnvironmentList.AddItem(this);
			}
		}

		// Recreate the lights.
		State->CreateEnvironmentLightList();
	}
}

void UDynamicLightEnvironmentComponent::UpdateTransform()
{
	Super::UpdateTransform();

	if(bEnabled)
	{
		// Outside the game we're not ticked, so update the light environment on attach.
		if(!GIsGame)
		{
			State->UpdateEnvironment(MAX_FLT,TRUE,TRUE);
			State->CreateEnvironmentLightList();
		}
	}
}

void UDynamicLightEnvironmentComponent::Detach()
{
	Super::Detach();

	// Remove the light environment from the world's list.
	if(!GIsGame && Scene->GetWorld())
	{
		for(TSparseArray<ULightEnvironmentComponent*>::TIterator It(Scene->GetWorld()->LightEnvironmentList);It;++It)
		{
			if(*It == this)
			{
				Scene->GetWorld()->LightEnvironmentList.Remove(It.GetIndex());
				break;
			}
		}
	}

	// Reset the light environment's light list.
	Lights.Empty();

	if(State)
	{
		// Detach the light environment's representative lights.
		State->DetachRepresentativeLights();
	}
}
