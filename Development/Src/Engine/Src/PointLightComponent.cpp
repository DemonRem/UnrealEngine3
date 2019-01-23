/*=============================================================================
	PointLightComponent.cpp: PointLightComponent implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "LightRendering.h"
#include "PointLightSceneInfo.h"

IMPLEMENT_CLASS(UPointLightComponent);

/**
 * The point light policy for TMeshLightingDrawingPolicy.
 */
class FPointLightPolicy
{
public:
	typedef TPointLightSceneInfo<FPointLightPolicy> SceneInfoType;
	class VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightPositionAndInvRadiusParameter.Bind(ParameterMap,TEXT("LightPositionAndInvRadius"));
		}
		void SetLight(FCommandContextRHI* Context,FShader* VertexShader,const TPointLightSceneInfo<FPointLightPolicy>* Light) const;
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
			LightColorAndFalloffExponentParameter.Bind(ParameterMap,TEXT("LightColorAndFalloffExponent"),TRUE);
		}
		void SetLight(FCommandContextRHI* Context,FShader* PixelShader,const TPointLightSceneInfo<FPointLightPolicy>* Light) const;
		void Serialize(FArchive& Ar)
		{
			Ar << LightColorAndFalloffExponentParameter;
		}
	private:
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
		}
		void SetModShadowLight( FCommandContextRHI* Context,FShader* PixelShader, const TPointLightSceneInfo<FPointLightPolicy>* Light ) const;
		void Serialize( FArchive& Ar )
		{
			Ar << LightPositionParam;
			Ar << FalloffExponentParam;
		}
		static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.Definitions.Set(TEXT("MODSHADOW_LIGHTTYPE_POINT"),TEXT("1"));
		}
	private:
		/** world position of light casting a shadow. Note: w = 1.0 / Radius */
		FShaderParameter LightPositionParam;
		/** attenuation exponent for light casting a shadow */
		FShaderParameter FalloffExponentParam;
	};
};

//
void FPointLightPolicy::VertexParametersType::SetLight(FCommandContextRHI* Context,FShader* VertexShader,const TPointLightSceneInfo<FPointLightPolicy>* Light) const
{
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LightPositionAndInvRadiusParameter,FVector4(Light->GetOrigin(),Light->InvRadius));
}

//
void FPointLightPolicy::PixelParametersType::SetLight(FCommandContextRHI* Context,FShader* PixelShader,const TPointLightSceneInfo<FPointLightPolicy>* Light) const
{
	SetPixelShaderValue(
		Context,
		PixelShader->GetPixelShader(),
		LightColorAndFalloffExponentParameter,
		FVector4(
			Light->Color.R,
			Light->Color.G,
			Light->Color.B,
			Light->FalloffExponent
			)
		);
}

void FPointLightPolicy::ModShadowPixelParamsType::SetModShadowLight(FCommandContextRHI* Context,FShader* PixelShader,const TPointLightSceneInfo<FPointLightPolicy>* Light) const
{
	// set world light position and falloff rate
	SetPixelShaderValue( Context, PixelShader->GetPixelShader(), LightPositionParam, FVector4(Light->GetOrigin(), 1.0f / Light->Radius) );
	SetPixelShaderValue( Context, PixelShader->GetPixelShader(), FalloffExponentParam, Light->ShadowFalloffExponent );
}

IMPLEMENT_LIGHT_SHADER_TYPE(FPointLightPolicy,TEXT("PointLightVertexShader"),TEXT("PointLightPixelShader"),VER_FP_BLENDING_FALLBACK,0);

//
FLightSceneInfo* UPointLightComponent::CreateSceneInfo() const
{
	return new TPointLightSceneInfo<FPointLightPolicy>(this);
}

//
UBOOL UPointLightComponent::AffectsBounds(const FBoxSphereBounds& Bounds) const
{
	if((Bounds.Origin - LightToWorld.GetOrigin()).SizeSquared() > Square(Radius + Bounds.SphereRadius))
	{
		return FALSE;
	}

	if(!Super::AffectsBounds(Bounds))
	{
		return FALSE;
	}

	return TRUE;
}

//
void UPointLightComponent::SetTransformedToWorld()
{
	// apply our translation to the parent matrix

	LightToWorld = FMatrix(
						FPlane(+0,+0,+1,+0),
						FPlane(+0,+1,+0,+0),
						FPlane(+1,+0,+0,+0),
						FPlane(+0,+0,+0,+1)
						) *
					FTranslationMatrix(Translation) * 
					CachedParentToWorld;
	LightToWorld.RemoveScaling();
	WorldToLight = LightToWorld.Inverse();
}

//
void UPointLightComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	CachedParentToWorld = ParentToWorld;
}

//
void UPointLightComponent::UpdatePreviewLightRadius()
{
	if ( PreviewLightRadius )
	{
		PreviewLightRadius->SphereRadius = Radius;
		PreviewLightRadius->Translation = Translation;
	}
}

//
void UPointLightComponent::Attach()
{
	// Call SetTransformedToWorld before ULightComponent::Attach, so the FLightSceneInfo is constructed with the right transform.
	SetTransformedToWorld();

	Super::Attach();

	UpdatePreviewLightRadius();
}

//
void UPointLightComponent::UpdateTransform()
{
	SetTransformedToWorld();

	Super::UpdateTransform();

	UpdatePreviewLightRadius();
}

//
FVector4 UPointLightComponent::GetPosition() const
{
	return FVector4(LightToWorld.GetOrigin(),1);
}

/**
* @return ELightComponentType for the light component class 
*/
ELightComponentType UPointLightComponent::GetLightType() const
{
	return LightType_Point;
}

//
FBox UPointLightComponent::GetBoundingBox() const
{
	return FBox(GetOrigin() - FVector(Radius,Radius,Radius),GetOrigin() + FVector(Radius,Radius,Radius));
}

//
FLinearColor UPointLightComponent::GetDirectIntensity(const FVector& Point) const
{
	FLOAT RadialAttenuation = appPow( Max(1.0f - ((GetOrigin() - Point) / Radius).SizeSquared(),0.0f), FalloffExponent );
	return Super::GetDirectIntensity(Point) * RadialAttenuation;;
}

//
void UPointLightComponent::SetTranslation(FVector NewTranslation)
{
	FComponentReattachContext ReattachContext(this);
	Translation = NewTranslation;
}

/**
 * Called after property has changed via e.g. property window or set command.
 *
 * @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
 */
void UPointLightComponent::PostEditChange(UProperty* PropertyThatChanged)
{
	// Make sure exponent is > 0.
	FalloffExponent = Max( (FLOAT) KINDA_SMALL_NUMBER, FalloffExponent );
	Super::PostEditChange( PropertyThatChanged );
}

void UPointLightComponent::PostLoad()
{
	Super::PostLoad();
	if ( GIsEditor
	&& !IsTemplate(RF_ClassDefaultObject)

	// FReloadObjectArcs call PostLoad() *prior* to instancing components for objects being reloaded, so this check is invalid in that case
	&& (GUglyHackFlags&HACK_IsReloadObjArc) == 0 )
	{
		const INT LinkerVersion = GetLinkerVersion();
		if ( PreviewLightRadius == NULL )
		{
			if ( LinkerVersion < VER_FIXED_POINTLIGHTCOMPONENT_LIGHTRADIUS )
			{
				ALight* OwnerLight = Cast<ALight>(GetOuter());
				if ( OwnerLight != NULL )
				{
					// find the DrawLightRadiusComponent contained in our owner
					for ( INT ComponentIndex = 0; ComponentIndex < OwnerLight->Components.Num(); ComponentIndex++ )
					{
						UDrawLightRadiusComponent* LightRadiusComp = Cast<UDrawLightRadiusComponent>(OwnerLight->Components(ComponentIndex));
						if ( LightRadiusComp != NULL )
						{
							Modify(TRUE);
							debugf(TEXT("Fixing up PreviewLightRadius reference for '%s': %s.  Package should be resaved to reduce load times."), *GetFullName(), *PreviewLightRadius->GetFullName());
							PreviewLightRadius = LightRadiusComp;
							break;
						}
					}
				}
			}
		}
		else if ( PreviewLightRadius->GetOuter() != GetOuter() )
		{
			// the component assigned as our PreviewLightRadius must be the same component that our owner light has in its components array.
			// if it isn't then this actor must have been created when component instancing wasn't working correctly (or this actor was created
			// by pasting in T3D text which was created when component instancing wasn't working correctly).
			if ( LinkerVersion < VER_FIXED_PLC_LIGHTRADIUS_POST_COMPONENTFIX )
			{
				ALight* OwnerLight = Cast<ALight>(GetOuter());
				if ( OwnerLight != NULL )
				{
					// find the DrawLightRadiusComponent contained in our owner
					for ( INT ComponentIndex = 0; ComponentIndex < OwnerLight->Components.Num(); ComponentIndex++ )
					{
						UDrawLightRadiusComponent* LightRadiusComp = Cast<UDrawLightRadiusComponent>(OwnerLight->Components(ComponentIndex));
						if ( LightRadiusComp != NULL )
						{
							// it's possible that the DrawLightRadiusComponent is the one that was serialized as the value for this PLC's PreviewLightRadius;
							// if so, then we need to rename it into the correct Outer
							if ( PreviewLightRadius == LightRadiusComp )
							{
								Modify(TRUE);
								debugf(TEXT("Correcting Outer for PreviewLightRadius component '%s'. Package should be resaved to reduce load times."), *PreviewLightRadius->GetFullName());
								LightRadiusComp->Rename(NULL, GetOuter(), REN_ForceNoResetLoaders);
								break;
							}
							else
							{
								Modify(TRUE);
								debugf(TEXT("Resetting invalid PreviewLightRadius reference for '%s':  "), *GetFullName());
								debugf(TEXT("OLD: %s"), *PreviewLightRadius->GetFullName());
								debugf(TEXT("NEW: %s"), *LightRadiusComp->GetFullName());
								debugf(TEXT("Package should be resaved to reduce load times."));

								PreviewLightRadius = LightRadiusComp;
								break;
							}
						}
					}
				}
			}
			else
			{
				// so if we are here, then the owning light actor was definitely created after the fixup code was added, so there is some way that this bug is still occurring
				// I need to figure out how this is occurring so let's annoy the designer into bugging me.
				debugf(TEXT("PointLightComponent %s has an invalid PreviewLightRadius '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately!"), *GetFullName(), *PreviewLightRadius->GetFullName());
				//@todo ronp - remove this once we've verified that this is no longer occurring.
				appMsgf(AMT_OK, TEXT("PointLightComponent %s has an invalid PreviewLightRadius '%s' even though package has been resaved since this bug was fixed.  Please let Ron know about this immediately!"), *GetFullName(), *PreviewLightRadius->GetFullName());
			}
		}
	}
}
