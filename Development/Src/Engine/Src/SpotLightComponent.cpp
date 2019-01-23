/*=============================================================================
	SpotLightComponent.cpp: LightComponent implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "LightRendering.h"
#include "PointLightSceneInfo.h"

IMPLEMENT_CLASS(USpotLightComponent);

/**
 * The spot light policy for TMeshLightingDrawingPolicy.
 */
class FSpotLightPolicy
{
public:
	typedef class FSpotLightSceneInfo SceneInfoType;
	class VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightPositionAndInvRadiusParameter.Bind(ParameterMap,TEXT("LightPositionAndInvRadius"));
		}
		void SetLight(FCommandContextRHI* Context,FShader* VertexShader,const FSpotLightSceneInfo* Light) const;
		void Serialize(FArchive& Ar)
		{
			Ar << LightPositionAndInvRadiusParameter;
		}
	private:
		FShaderParameter LightPositionAndInvRadiusParameter;
	};
	class PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			SpotAnglesParameter.Bind(ParameterMap,TEXT("SpotAngles"),TRUE);
			SpotDirectionParameter.Bind(ParameterMap,TEXT("SpotDirection"),TRUE);
			LightColorAndFalloffExponentParameter.Bind(ParameterMap,TEXT("LightColorAndFalloffExponent"),TRUE);
		}
		void SetLight(FCommandContextRHI* Context,FShader* PixelShader,const FSpotLightSceneInfo* Light) const;
		void Serialize(FArchive& Ar)
		{
			Ar << SpotAnglesParameter;
			Ar << SpotDirectionParameter;
			Ar << LightColorAndFalloffExponentParameter;
		}
	private:
		FShaderParameter SpotAnglesParameter;
		FShaderParameter SpotDirectionParameter;
		FShaderParameter LightColorAndFalloffExponentParameter;
	};

	/**
	* Modulated shadow shader params associated with this light policy
	*/
	class ModShadowPixelParamsType
	{
	public:
		void Bind( const FShaderParameterMap& ParameterMap )
		{
			LightPositionParam.Bind(ParameterMap,TEXT("LightPosition"));
			FalloffExponentParam.Bind(ParameterMap,TEXT("FalloffExponent"));
			SpotDirectionParam.Bind(ParameterMap,TEXT("SpotDirection"),TRUE);
			SpotAnglesParam.Bind(ParameterMap,TEXT("SpotAngles"),TRUE);
		}
		void SetModShadowLight( FCommandContextRHI* Context,FShader* PixelShader, const FSpotLightSceneInfo* Light ) const;
		void Serialize( FArchive& Ar )
		{
			Ar << LightPositionParam;
			Ar << FalloffExponentParam;
			Ar << SpotDirectionParam;
			Ar << SpotAnglesParam;
		}
		static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.Definitions.Set(TEXT("MODSHADOW_LIGHTTYPE_SPOT"),TEXT("1"));
		}
	private:
		/** world position of light casting a shadow. Note: w = 1.0 / Radius */
		FShaderParameter LightPositionParam;
		/** attenuation exponent for light casting a shadow */
		FShaderParameter FalloffExponentParam;
		/** spot light direction vector in world space */
		FShaderParameter SpotDirectionParam;
		/** spot light cone cut-off angles */
		FShaderParameter SpotAnglesParam;
	};
};

IMPLEMENT_LIGHT_SHADER_TYPE(FSpotLightPolicy,TEXT("SpotLightVertexShader"),TEXT("SpotLightPixelShader"),VER_FP_BLENDING_FALLBACK,0);

/**
 * The scene info for a spot light.
 */
class FSpotLightSceneInfo : public TPointLightSceneInfo<FSpotLightPolicy>
{
public:

	/** Cosine of the spot light's inner cone angle. */
	FLOAT CosInnerCone;

	/** Cosine of the spot light's outer cone angle. */
	FLOAT CosOuterCone;

	/** 1 / (CosInnerCone - CosOuterCone) */
	FLOAT InvCosConeDifference;

	/** Sine of the spot light's outer cone angle. */
	FLOAT SinOuterCone;

	/** Initialization constructor. */
	FSpotLightSceneInfo(const USpotLightComponent* Component):
		TPointLightSceneInfo<FSpotLightPolicy>(Component)
	{
		FLOAT ClampedInnerConeAngle = Clamp(Component->InnerConeAngle,0.0f,89.0f) * (FLOAT)PI / 180.0f;
		FLOAT ClampedOuterConeAngle = Clamp(Component->OuterConeAngle * (FLOAT)PI / 180.0f,ClampedInnerConeAngle + 0.001f,89.0f * (FLOAT)PI / 180.0f + 0.001f);
		CosOuterCone = appCos(ClampedOuterConeAngle);
		SinOuterCone = appSin(ClampedOuterConeAngle);
		CosInnerCone = appCos(ClampedInnerConeAngle);
		InvCosConeDifference = 1.0f / (CosInnerCone - CosOuterCone);
	}

	// FLightSceneInfo interface.
	virtual UBOOL AffectsBounds(const FBoxSphereBounds& Bounds) const
	{
		if(!TPointLightSceneInfo<FSpotLightPolicy>::AffectsBounds(Bounds))
		{
			return FALSE;
		}

		FVector	U = GetOrigin() - (Bounds.SphereRadius / SinOuterCone) * GetDirection(),
				D = Bounds.Origin - U;
		FLOAT	dsqr = D | D,
				E = GetDirection() | D;
		if(E > 0.0f && E * E >= dsqr * Square(CosOuterCone))
		{
			D = Bounds.Origin - GetOrigin();
			dsqr = D | D;
			E = -(GetDirection() | D);
			if(E > 0.0f && E * E >= dsqr * Square(SinOuterCone))
				return dsqr <= Square(Bounds.SphereRadius);
			else
				return TRUE;
		}

		return FALSE;
	}
};

void FSpotLightPolicy::VertexParametersType::SetLight(FCommandContextRHI* Context,FShader* VertexShader,const FSpotLightSceneInfo* Light) const
{
	FVector Origin = Light->GetOrigin();
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LightPositionAndInvRadiusParameter,FVector4(Origin,Light->InvRadius));
}

void FSpotLightPolicy::PixelParametersType::SetLight(FCommandContextRHI* Context,FShader* PixelShader,const FSpotLightSceneInfo* Light) const
{
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),SpotAnglesParameter,FVector4(Light->CosOuterCone,Light->InvCosConeDifference,0,0));
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),SpotDirectionParameter,Light->GetDirection());
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),LightColorAndFalloffExponentParameter,
		FVector4(Light->Color.R,Light->Color.G,Light->Color.B,Light->FalloffExponent));
}

void FSpotLightPolicy::ModShadowPixelParamsType::SetModShadowLight(FCommandContextRHI* Context,FShader* PixelShader,const FSpotLightSceneInfo* Light) const
{
	// set world light position and falloff rate
	SetPixelShaderValue( Context, PixelShader->GetPixelShader(), LightPositionParam, FVector4(Light->GetOrigin(), 1.0f / Light->Radius) );
	SetPixelShaderValue( Context, PixelShader->GetPixelShader(), FalloffExponentParam, Light->ShadowFalloffExponent );
	// set spot light direction
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),SpotDirectionParam,Light->GetDirection());
	// set spot light inner/outer cone angles
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),SpotAnglesParam,FVector4(Light->CosOuterCone,Light->InvCosConeDifference,0,0));
}

FLightSceneInfo* USpotLightComponent::CreateSceneInfo() const
{
	return new FSpotLightSceneInfo(this);
}

UBOOL USpotLightComponent::AffectsBounds(const FBoxSphereBounds& Bounds) const
{
	if(!Super::AffectsBounds(Bounds))
	{
		return FALSE;
	}

	FLOAT	ClampedInnerConeAngle = Clamp(InnerConeAngle,0.0f,89.0f) * (FLOAT)PI / 180.0f,
			ClampedOuterConeAngle = Clamp(OuterConeAngle * (FLOAT)PI / 180.0f,ClampedInnerConeAngle + 0.001f,89.0f * (FLOAT)PI / 180.0f + 0.001f);

	FLOAT	Sin = appSin(ClampedOuterConeAngle),
			Cos = appCos(ClampedOuterConeAngle);

	FVector	U = GetOrigin() - (Bounds.SphereRadius / Sin) * GetDirection(),
			D = Bounds.Origin - U;
	FLOAT	dsqr = D | D,
			E = GetDirection() | D;
	if(E > 0.0f && E * E >= dsqr * Square(Cos))
	{
		D = Bounds.Origin - GetOrigin();
		dsqr = D | D;
		E = -(GetDirection() | D);
		if(E > 0.0f && E * E >= dsqr * Square(Sin))
			return dsqr <= Square(Bounds.SphereRadius);
		else
			return TRUE;
	}

	return FALSE;
}

void USpotLightComponent::Attach()
{
	Super::Attach();

	if ( PreviewInnerCone )
	{
		PreviewInnerCone->ConeRadius = Radius;
		PreviewInnerCone->ConeAngle = InnerConeAngle;
		PreviewInnerCone->Translation = Translation;
	}

	if ( PreviewOuterCone )
	{
		PreviewOuterCone->ConeRadius = Radius;
		PreviewOuterCone->ConeAngle = OuterConeAngle;
		PreviewOuterCone->Translation = Translation;
	}
}

FLinearColor USpotLightComponent::GetDirectIntensity(const FVector& Point) const
{
	FLOAT	ClampedInnerConeAngle = Clamp(InnerConeAngle,0.0f,89.0f) * (FLOAT)PI / 180.0f,
			ClampedOuterConeAngle = Clamp(OuterConeAngle * (FLOAT)PI / 180.0f,ClampedInnerConeAngle + 0.001f,89.0f * (FLOAT)PI / 180.0f + 0.001f),
			OuterCone = appCos(ClampedOuterConeAngle),
			InnerCone = appCos(ClampedInnerConeAngle);

	FVector LightVector = (Point - GetOrigin()).SafeNormal();
	FLOAT SpotAttenuation = Square(Clamp<FLOAT>(((LightVector | GetDirection()) - OuterCone) / (InnerCone - OuterCone),0.0f,1.0f));
	return Super::GetDirectIntensity(Point) * SpotAttenuation;
}

/**
* @return ELightComponentType for the light component class 
*/
ELightComponentType USpotLightComponent::GetLightType() const
{
	return LightType_Spot;
}

void USpotLightComponent::PostLoad()
{
	Super::PostLoad();

	if ( GIsEditor
	&& !IsTemplate(RF_ClassDefaultObject)

	// FReloadObjectArcs call PostLoad() *prior* to instancing components for objects being reloaded, so this check is invalid in that case
	&& (GUglyHackFlags&HACK_IsReloadObjArc) == 0 )
	{
		const INT LinkerVersion = GetLinkerVersion();
		if ( LinkerVersion < VER_FIXED_SPOTLIGHTCOMPONENTS )
		{
			ASpotLight* OwnerLight = Cast<ASpotLight>(GetOuter());
			if ( OwnerLight != NULL )
			{
				// we could probably just iterate through the Components array of our OwnerLight, looking for the component that is the archetype for
				// our OwnerLight.LightComponent.PreviewInnerCone and PreviewOuterCone, but this may not be completely reliable (though it *should* be) if our own
				// archetype or our owner light's archetype was messed up.  Instead, we'll do it the really reliable though slightly slower way.
				ASpotLight* OwnerArchetype = OwnerLight->GetArchetype<ASpotLight>();
				check(OwnerArchetype);

				USpotLightComponent* ArchetypeSpotLightComponent = Cast<USpotLightComponent>(OwnerArchetype->LightComponent);
				check(ArchetypeSpotLightComponent);
				check(ArchetypeSpotLightComponent->PreviewInnerCone);
				check(ArchetypeSpotLightComponent->PreviewOuterCone);

				// similar fix to PointLightComponent; here we need to fixup two components: PreviewInnerCone and PreviewOuterCone
				// PreviewInnerCone
				if ( PreviewInnerCone == NULL || (PreviewInnerCone->GetOuter() != OwnerLight || !OwnerLight->Components.ContainsItem(PreviewInnerCone)) )
				{
					// find the DrawLightConeComponent contained in the Components array of our OwnerLight which corresponds to the PreviewInnerCone
					for ( INT ComponentIndex = 0; ComponentIndex < OwnerLight->Components.Num(); ComponentIndex++ )
					{
						UDrawLightConeComponent* LightConeComp = Cast<UDrawLightConeComponent>(OwnerLight->Components(ComponentIndex));
						if ( LightConeComp != NULL )
						{
							if ( LightConeComp->GetArchetype() == ArchetypeSpotLightComponent->PreviewInnerCone )
							{
								Modify(GIsUCC);

								// found the one that should be assigned as the value for PreviewInnerCone
								if ( PreviewInnerCone == NULL )
								{
									// somehow, we had a NULL PreviewInnerCone
									debugf(TEXT("Fixing up NULL PreviewInnerCone reference for '%s': %s.  Package should be resaved to reduce load times."),
										*GetFullName(), *LightConeComp->GetFullName());
								}
								else if ( PreviewInnerCone == LightConeComp )
								{
									// this might happen if both this component and the owner light are pointing to the same DrawLightConeComponent, but it was instanced using the wrong outer
									debugf(TEXT("Correcting Outer for PreviewInnerCone component '%s'. Package should be resaved to reduce load times."), *PreviewInnerCone->GetFullName());
									PreviewInnerCone->Rename(NULL, OwnerLight, REN_ForceNoResetLoaders);
								}
								else
								{
									debugf(TEXT("Fixing up invalid PreviewInnerCone reference for '%s': %s  =>  %s.  Package should be resaved to reduce load times."),
										*GetFullName(), *PreviewInnerCone->GetFullName(), *LightConeComp->GetFullName());
								}
								PreviewInnerCone = LightConeComp;
								break;
							}
						}
					}
				}

				// PreviewOuterCone
				if ( PreviewOuterCone == NULL || (PreviewOuterCone->GetOuter() != OwnerLight || !OwnerLight->Components.ContainsItem(PreviewOuterCone)) )
				{
					// find the DrawLightConeComponent contained in the Components array of our OwnerLight which corresponds to the PreviewOuterCone
					for ( INT ComponentIndex = 0; ComponentIndex < OwnerLight->Components.Num(); ComponentIndex++ )
					{
						UDrawLightConeComponent* LightConeComp = Cast<UDrawLightConeComponent>(OwnerLight->Components(ComponentIndex));
						if ( LightConeComp != NULL )
						{
							if ( LightConeComp->GetArchetype() == ArchetypeSpotLightComponent->PreviewOuterCone )
							{
								Modify(GIsUCC);

								// found the one that should be assigned as the value for PreviewOuterCone
								if ( PreviewOuterCone == NULL )
								{
									// somehow, we had a NULL PreviewOuterCone
									debugf(TEXT("Fixing up NULL PreviewOuterCone reference for '%s': %s.  Package should be resaved to reduce load times."),
										*GetFullName(), *LightConeComp->GetFullName());
								}
								else if ( PreviewOuterCone == LightConeComp )
								{
									// this might happen if both this component and the owner light are pointing to the same DrawLightConeComponent, but it was instanced using the wrong outer
									debugf(TEXT("Correcting Outer for PreviewOuterCone component '%s'. Package should be resaved to reduce load times."), *PreviewOuterCone->GetFullName());
									PreviewOuterCone->Rename(NULL, OwnerLight, REN_ForceNoResetLoaders);
								}
								else
								{
									debugf(TEXT("Fixing up invalid PreviewOuterCone reference for '%s': %s  =>  %s.  Package should be resaved to reduce load times."),
										*GetFullName(), *PreviewOuterCone->GetFullName(), *LightConeComp->GetFullName());
								}
								PreviewOuterCone = LightConeComp;
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			if ( PreviewInnerCone != NULL && PreviewInnerCone->GetOuter() != GetOuter() )
			{
				// so if we are here, then the owning light actor was definitely created after the fixup code was added, so there is some way that this bug is still occurring
				// I need to figure out how this is occurring so let's annoy the designer into bugging me.
				debugf(TEXT("%s has an invalid PreviewInnerCone '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately!"), *GetFullName(), *PreviewInnerCone->GetFullName());
				//@todo ronp - remove this once we've verified that this is no longer occurring.
				appMsgf(AMT_OK, TEXT("%s has an invalid PreviewInnerCone '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately! (this message has already been written to the log)"), *GetFullName(), *PreviewInnerCone->GetFullName());
			}
			else if ( PreviewOuterCone != NULL && PreviewOuterCone->GetOuter() != GetOuter() )
			{
				// so if we are here, then the owning light actor was definitely created after the fixup code was added, so there is some way that this bug is still occurring
				// I need to figure out how this is occurring so let's annoy the designer into bugging me.
				debugf(TEXT("%s has an invalid PreviewOuterCone '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately!"), *GetFullName(), *PreviewOuterCone->GetFullName());
				//@todo ronp - remove this once we've verified that this is no longer occurring.
				appMsgf(AMT_OK, TEXT("%s has an invalid PreviewOuterCone '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately! (this message has already been written to the log)"), *GetFullName(), *PreviewOuterCone->GetFullName());
			}
		}
	}
}

