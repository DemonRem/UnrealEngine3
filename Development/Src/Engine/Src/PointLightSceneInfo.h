/*=============================================================================
	PointLightSceneInfo.h: Point light scene info definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_POINTLIGHTSCENEINFO
#define _INC_POINTLIGHTSCENEINFO

/**
 * Compute the screen bounds of a point light along one axis.
 * Based on http://www.gamasutra.com/features/20021011/lengyel_06.htm
 * and http://sourceforge.net/mailarchive/message.php?msg_id=10501105
 */
static bool GetPointLightBounds(
	FLOAT LightX,
	FLOAT LightZ,
	FLOAT Radius,
	const FVector& Axis,
	FLOAT AxisSign,
	const FSceneView* View,
	FLOAT ViewX,
	FLOAT ViewSizeX,
	INT& OutMinX,
	INT& OutMaxX
	)
{
	// Vertical planes: T = <Nx, 0, Nz, 0>
	FLOAT Discriminant = (Square(LightX) - Square(Radius) + Square(LightZ)) * Square(LightZ);
	if(Discriminant >= 0)
	{
		float Nxa = (Radius * LightX - appSqrt(Discriminant)) / (Square(LightX) + Square(LightZ));
		float Nxb = (Radius * LightX + appSqrt(Discriminant)) / (Square(LightX) + Square(LightZ));
		float Nza = (Radius - Nxa * LightX) / LightZ;
		float Nzb = (Radius - Nxb * LightX) / LightZ;
		float Pza = LightZ - Radius * Nza;
		float Pzb = LightZ - Radius * Nzb;

		// Tangent a
		if(Pza > 0)
		{
			float Pxa = -Pza * Nza / Nxa;
			FVector4 P = View->ProjectionMatrix.TransformFVector4(FVector4(Axis.X * Pxa,Axis.Y * Pxa,Pza,1));
			FLOAT X = (Dot3(P,Axis) / P.W + 1.0f * AxisSign) / 2.0f * AxisSign;
			if(IsNegativeFloat(Nxa) ^ IsNegativeFloat(AxisSign))
			{
				OutMaxX = Min<LONG>(appCeil(ViewSizeX * X + ViewX),OutMaxX);
			}
			else
			{
				OutMinX = Max<LONG>(appFloor(ViewSizeX * X + ViewX),OutMinX);
			}
		}

		// Tangent b
		if(Pzb > 0)
		{
			float Pxb = -Pzb * Nzb / Nxb;
			FVector4 P = View->ProjectionMatrix.TransformFVector4(FVector4(Axis.X * Pxb,Axis.Y * Pxb,Pzb,1));
			FLOAT X = (Dot3(P,Axis) / P.W + 1.0f * AxisSign) / 2.0f * AxisSign;
			if(IsNegativeFloat(Nxb) ^ IsNegativeFloat(AxisSign))
			{
				OutMaxX = Min<LONG>(appCeil(ViewSizeX * X + ViewX),OutMaxX);
			}
			else
			{
				OutMinX = Max<LONG>(appFloor(ViewSizeX * X + ViewX),OutMinX);
			}
		}
	}

	return OutMinX <= OutMaxX;
}

extern TGlobalResource<FShadowFrustumVertexDeclaration> GShadowFrustumVertexDeclaration;

/**
 * The scene info for a point light.
 * This is in LightRendering.h because it is used by both PointLightComponent.cpp and SpotLightComponent.cpp
 */
template<typename LightPolicyType>
class TPointLightSceneInfo : public FLightSceneInfo
{
public:

	/** The light radius. */
	const FLOAT Radius;

	/** One over the light's radius. */
	const FLOAT InvRadius;

	/** The light falloff exponent. */
	const FLOAT FalloffExponent;

	/** Falloff for shadow when using LightShadow_Modulate */
	const FLOAT ShadowFalloffExponent;

	/** multiplied by the radius of object's bounding sphere, default - 1.1*/
	const FLOAT ShadowRadiusMultiplier;

	/** Initialization constructor. */
	TPointLightSceneInfo(const UPointLightComponent* Component):
		FLightSceneInfo(Component),
		Radius(Component->Radius),
		InvRadius(1.0f / Component->Radius),
		FalloffExponent(Component->FalloffExponent),
		ShadowFalloffExponent(Component->ShadowFalloffExponent)
		,ShadowRadiusMultiplier( Component->ShadowRadiusMultiplier )
	{}

	// FLightSceneInfo interface.

	/** @return radius of the light or 0 if no radius */
	virtual FLOAT GetRadius() const 
	{ 
		return Radius; 
	}

	virtual UBOOL AffectsBounds(const FBoxSphereBounds& Bounds) const
	{
		if((Bounds.Origin - this->LightToWorld.GetOrigin()).SizeSquared() > Square(Radius + Bounds.SphereRadius))
		{
			return FALSE;
		}

		if(!FLightSceneInfo::AffectsBounds(Bounds))
		{
			return FALSE;
		}

		return TRUE;
	}
	virtual UBOOL GetProjectedShadowInitializer(const FSphere& SubjectBoundingSphere,FProjectedShadowInitializer& OutInitializer) const
	{
		// For point lights, use a perspective projection looking at the primitive from the light position.
		FVector LightPosition = this->LightToWorld.GetOrigin();
		FVector LightVector = FVector(SubjectBoundingSphere) - LightPosition;
		FLOAT LightDistance = LightVector.Size();
		FLOAT SilhouetteRadius = 0.0f;
		FLOAT SubjectRadius = SubjectBoundingSphere.W;

		if(LightDistance > SubjectRadius)
		{
			SilhouetteRadius = SubjectRadius * appInvSqrt((LightDistance - SubjectRadius) * (LightDistance + SubjectRadius));
		}


		if( LightDistance <= SubjectRadius * ShadowRadiusMultiplier )
		{
			// Make primitive fit in a single <90 degree FOV projection.
			LightVector = SubjectRadius * LightVector.SafeNormal() * ShadowRadiusMultiplier;
			LightPosition = ( FVector( SubjectBoundingSphere ) - LightVector );
			LightDistance = SubjectRadius * ShadowRadiusMultiplier;
			SilhouetteRadius = 1.0f;

		}

		if ( SilhouetteRadius > 1.0f )
		{
			SilhouetteRadius = 1.0f;
		}

		// The primitive now fits in a single <90 degree FOV projection.
		OutInitializer.CalcTransforms(
			FTranslationMatrix(-LightPosition) *
				FInverseRotationMatrix((LightVector / LightDistance).Rotation()) *
				FScaleMatrix(FVector(1.0f,1.0f / SilhouetteRadius,1.0f / SilhouetteRadius)),
			FVector(1,0,0),
			SubjectBoundingSphere,
			FVector4(0,0,1,0),
			0.1f,
			Radius,
			FALSE
			);
		return TRUE;
	}
	virtual void SetScissorRect(FCommandContextRHI* Context,const FSceneView* View) const
	{
#if PS3 //only PS3 supports depth bounds test
		//transform the light's position into view space
		FVector ViewSpaceLightPosition = View->ViewMatrix.TransformFVector(LightToWorld.GetOrigin());

		//subtract the light's radius from the view space depth to get the near position
		//this assumes that Radius in world space is the same length in view space, ie ViewMatrix has no scaling
		FVector NearLightPosition = ViewSpaceLightPosition;
		NearLightPosition.Z -= Radius;

		//add the light's radius to the view space depth to get the far position
		FVector FarLightPosition = ViewSpaceLightPosition;
		FarLightPosition.Z += Radius;

		//transform both near and far positions into clip space
		FVector4 ClipSpaceNearPos = View->ProjectionMatrix.TransformFVector(NearLightPosition);
		FVector4 ClipSpaceFarPos = View->ProjectionMatrix.TransformFVector(FarLightPosition);

		//convert to normalized device coordinates, clamp to valid ranges
		FLOAT MinZ = Clamp(Max(ClipSpaceNearPos.Z, 0.0f) / ClipSpaceNearPos.W, 0.0f, 1.0f);
		FLOAT MaxZ = Clamp(ClipSpaceFarPos.Z / ClipSpaceFarPos.W, 0.0f, 1.0f);

		//only set depth bounds if ranges are valid
		//be sure to disable depth bounds test after drawing!
		if (MinZ < MaxZ)
		{
			RHISetDepthBoundsTest(Context, TRUE, MinZ, MaxZ);
		}
#endif

		// Calculate a scissor rectangle for the light's radius.
		if((this->LightToWorld.GetOrigin() - View->ViewOrigin).Size() > Radius)
		{
			FVector LightVector = View->ViewMatrix.TransformFVector(this->LightToWorld.GetOrigin());

			INT ScissorMinX = appFloor(View->RenderTargetX);
			INT ScissorMaxX = appCeil(View->RenderTargetX + View->RenderTargetSizeX);
			if(!GetPointLightBounds(
					LightVector.X,
					LightVector.Z,
					Radius,
					FVector(+1,0,0),
					+1,
					View,
					View->RenderTargetX,
					View->RenderTargetSizeX,
					ScissorMinX,
					ScissorMaxX))
			{
				return;
			}

			INT ScissorMinY = appFloor(View->RenderTargetY);
			INT ScissorMaxY = appCeil(View->RenderTargetY + View->RenderTargetSizeY);
			if(!GetPointLightBounds(
					LightVector.Y,
					LightVector.Z,
					Radius,
					FVector(0,+1,0),
					-1,
					View,
					View->RenderTargetY,
					View->RenderTargetSizeY,
					ScissorMinY,
					ScissorMaxY))
			{
				return;
			}

			RHISetScissorRect(Context,TRUE,ScissorMinX,ScissorMinY,ScissorMaxX,ScissorMaxY);
		}
	}
	virtual const FLightSceneDPGInfoInterface* GetDPGInfo(UINT DPGIndex) const
	{
		check(DPGIndex < SDPG_MAX_SceneRender);
		return &DPGInfos[DPGIndex];
	}
	virtual FLightSceneDPGInfoInterface* GetDPGInfo(UINT DPGIndex)
	{
		check(DPGIndex < SDPG_MAX_SceneRender);
		return &DPGInfos[DPGIndex];
	}

	/**
	* @return modulated shadow projection pixel shader for this light type
	*/
	virtual class FShadowProjectionPixelShaderInterface* GetModShadowProjPixelShader() const
	{
		return GetModProjPixelShaderRef <LightPolicyType> (ShadowFilterQuality);
	}

	/**
	* @return Branching PCF modulated shadow projection pixel shader for this light type
	*/
	virtual class FBranchingPCFProjectionPixelShaderInterface* GetBranchingPCFModProjPixelShader() const
	{
		return GetBranchingPCFModProjPixelShaderRef <LightPolicyType> (ShadowFilterQuality);
	}

	/**
	* @return modulated shadow projection pixel shader for this light type
	*/
	virtual class FModShadowVolumePixelShader* GetModShadowVolumeShader() const
	{
		TShaderMapRef<TModShadowVolumePixelShader<LightPolicyType> > ModShadowShader(GetGlobalShaderMap());
		return *ModShadowShader;
	}

	/**
	* @return modulated shadow projection bound shader state for this light type
	*/
	virtual FGlobalBoundShaderStateRHIRef* GetModShadowProjBoundShaderState() const
	{
		return &ModShadowProjBoundShaderState;
	}

#if SUPPORTS_VSM
	/**
	* @return VSM modulated shadow projection pixel shader for this light type
	*/
	virtual class FVSMModProjectionPixelShader* GetVSMModProjPixelShader() const
	{
		TShaderMapRef<TVSMModProjectionPixelShader<LightPolicyType> > ModShadowShader(GetGlobalShaderMap());
		return *ModShadowShader;
	}

	/**
	* @return VSM modulated shadow projection bound shader state for this light type
	*/
	virtual FBoundShaderStateRHIParamRef GetVSMModProjBoundShaderState() const
	{
		if (!IsValidRef(VSMModProjBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			appMemzero(Strides, sizeof(Strides));
			Strides[0] = sizeof(FVector);

			TShaderMapRef<FModShadowProjectionVertexShader> ModShadowProjVertexShader(GetGlobalShaderMap());
			TShaderMapRef<TVSMModProjectionPixelShader<LightPolicyType> > ModShadowProjPixelShader(GetGlobalShaderMap());

			VSMModProjBoundShaderState = RHICreateBoundShaderState(GShadowFrustumVertexDeclaration.VertexDeclarationRHI, Strides, ModShadowProjVertexShader->GetVertexShader(), ModShadowProjPixelShader->GetPixelShader());
		}
		return VSMModProjBoundShaderState;
	}
#endif //#if SUPPORTS_VSM

	/**
	* @return PCF Branching modulated shadow projection bound shader state for this light type
	*/
	virtual FGlobalBoundShaderStateRHIRef* GetBranchingPCFModProjBoundShaderState() const
	{
		//allow the BPCF implementation to choose which of the loaded bound shader states should be used
		FGlobalBoundShaderStateRHIRef* CurrentBPCFBoundShaderState = ChooseBPCFBoundShaderState(ShadowFilterQuality, &ModBranchingPCFLowQualityBoundShaderState, 
			&ModBranchingPCFMediumQualityBoundShaderState, &ModBranchingPCFHighQualityBoundShaderState);

		return CurrentBPCFBoundShaderState;
	}


	/**
	* @return modulated shadow projection volume shader state for this light type
	*/
	virtual FBoundShaderStateRHIParamRef GetModShadowVolumeBoundShaderState() const
	{
		if (!IsValidRef(ModShadowVolumeBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			appMemzero(Strides, sizeof(Strides));
			Strides[0] = sizeof(FSimpleElementVertex);

			TShaderMapRef<FModShadowVolumeVertexShader> ModShadowVolumeVertexShader(GetGlobalShaderMap());
			TShaderMapRef<TModShadowVolumePixelShader<LightPolicyType> > ModShadowVolumePixelShader(GetGlobalShaderMap());
			ModShadowVolumeBoundShaderState = RHICreateBoundShaderState(GSimpleElementVertexDeclaration.VertexDeclarationRHI, Strides, ModShadowVolumeVertexShader->GetVertexShader(), ModShadowVolumePixelShader->GetPixelShader());
		}

		return ModShadowVolumeBoundShaderState;
	}

private:

	/** The DPG info for the point light. */
	TLightSceneDPGInfo<LightPolicyType> DPGInfos[SDPG_MAX_SceneRender];
};

#endif
