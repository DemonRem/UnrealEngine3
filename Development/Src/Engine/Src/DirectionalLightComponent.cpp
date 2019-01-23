/*=============================================================================
	DirectionalLightComponent.cpp: DirectionalLightComponent implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "LightRendering.h"

IMPLEMENT_CLASS(UDirectionalLightComponent);

/**
 * The directional light policy for TMeshLightingDrawingPolicy.
 */
class FDirectionalLightPolicy
{
public:
	typedef FLightSceneInfo SceneInfoType;
	class VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightDirectionParameter.Bind(ParameterMap,TEXT("LightDirection"));
		}
		void SetLight(FCommandContextRHI* Context,FShader* VertexShader,const SceneInfoType* Light) const
		{
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LightDirectionParameter,FVector4(-Light->GetDirection(),0));
		}
		void Serialize(FArchive& Ar)
		{
			Ar << LightDirectionParameter;
		}
	private:
		FShaderParameter LightDirectionParameter;
	};
	class PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightColorParameter.Bind(ParameterMap,TEXT("LightColor"),TRUE);
		}
		void SetLight(FCommandContextRHI* Context,FShader* PixelShader,const SceneInfoType* Light) const
		{
			SetPixelShaderValue(Context,PixelShader->GetPixelShader(),LightColorParameter,Light->Color);
		}
		void Serialize(FArchive& Ar)
		{
			Ar << LightColorParameter;
		}
	private:
		FShaderParameter LightColorParameter;
	};

	/**
	* Modulated shadow shader params associated with this light policy
	*/
	class ModShadowPixelParamsType
	{
	public:
		void Bind( const FShaderParameterMap& ParameterMap )
		{			
		}
		void SetModShadowLight( FCommandContextRHI* Context, FShader* PixelShader, const SceneInfoType* Light ) const;
		void Serialize( FArchive& Ar )
		{
		}
		static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
		{
			OutEnvironment.Definitions.Set(TEXT("MODSHADOW_LIGHTTYPE_DIRECTIONAL"),TEXT("1"));
		}
		
	};
};

void FDirectionalLightPolicy::ModShadowPixelParamsType::SetModShadowLight(FCommandContextRHI* Context,FShader* PixelShader,const SceneInfoType* Light) const
{	
}

IMPLEMENT_LIGHT_SHADER_TYPE(FDirectionalLightPolicy,TEXT("DirectionalLightVertexShader"),TEXT("DirectionalLightPixelShader"),VER_FP_BLENDING_FALLBACK,0)


/**
 * The scene info for a directional light.
 */
class FDirectionalLightSceneInfo : public FLightSceneInfo
{
public:

	/** Initialization constructor. */
	FDirectionalLightSceneInfo(const UDirectionalLightComponent* Component):
		FLightSceneInfo(Component)
	{}

	// FLightSceneInfo interface.
	virtual UBOOL GetProjectedShadowInitializer(const FSphere& SubjectBoundingSphere,FProjectedShadowInitializer& OutInitializer) const
	{
		// For directional lights, use an orthographic projection.
		OutInitializer.CalcTransforms(
			FTranslationMatrix(-(FVector)SubjectBoundingSphere) *
				FInverseRotationMatrix(FVector(WorldToLight.M[0][2],WorldToLight.M[1][2],WorldToLight.M[2][2]).SafeNormal().Rotation()) *
				FScaleMatrix(FVector(1.0f,1.0f / SubjectBoundingSphere.W,1.0f / SubjectBoundingSphere.W)),
			FVector(1,0,0),
			SubjectBoundingSphere,
			FVector4(0,0,0,1),
			-HALF_WORLD_MAX,
			HALF_WORLD_MAX,
			TRUE
			);
		return TRUE;
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
		return GetModProjPixelShaderRef <FDirectionalLightPolicy> (ShadowFilterQuality);
	}

	/**
	* @return Branching PCF modulated shadow projection pixel shader for this light type
	*/
	virtual class FBranchingPCFProjectionPixelShaderInterface* GetBranchingPCFModProjPixelShader() const
	{
		return GetBranchingPCFModProjPixelShaderRef <FDirectionalLightPolicy> (ShadowFilterQuality);
	} 

	/**
	* @return modulated shadow projection pixel shader for this light type
	*/
	virtual class FModShadowVolumePixelShader* GetModShadowVolumeShader() const
	{
		TShaderMapRef<TModShadowVolumePixelShader<FDirectionalLightPolicy> > ModShadowShader(GetGlobalShaderMap());
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
		TShaderMapRef<TVSMModProjectionPixelShader<FDirectionalLightPolicy> > ModShadowShader(GetGlobalShaderMap());
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
			TShaderMapRef<TVSMModProjectionPixelShader<FDirectionalLightPolicy> > ModShadowProjPixelShader(GetGlobalShaderMap());

			extern TGlobalResource<FShadowFrustumVertexDeclaration> GShadowFrustumVertexDeclaration;
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
	* @return modulated shadow volume bound shader state for this light type
	*/
	virtual FBoundShaderStateRHIParamRef GetModShadowVolumeBoundShaderState() const
	{
		if (!IsValidRef(ModShadowVolumeBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			appMemzero(Strides, sizeof(Strides));
			Strides[0] = sizeof(FSimpleElementVertex);

			TShaderMapRef<FModShadowVolumeVertexShader> ModShadowVolumeVertexShader(GetGlobalShaderMap());
			TShaderMapRef<TModShadowVolumePixelShader<FDirectionalLightPolicy> > ModShadowVolumePixelShader(GetGlobalShaderMap());

			ModShadowVolumeBoundShaderState = RHICreateBoundShaderState(GSimpleElementVertexDeclaration.VertexDeclarationRHI, Strides, ModShadowVolumeVertexShader->GetVertexShader(), ModShadowVolumePixelShader->GetPixelShader());
		}

		return ModShadowVolumeBoundShaderState;
	}

private:

	/** The DPG info for the point light. */
	TLightSceneDPGInfo<FDirectionalLightPolicy> DPGInfos[SDPG_MAX_SceneRender];
};

FLightSceneInfo* UDirectionalLightComponent::CreateSceneInfo() const
{
	return new FDirectionalLightSceneInfo(this);
}

FVector4 UDirectionalLightComponent::GetPosition() const
{
	return FVector4(-GetDirection() * TraceDistance, 0 );
}

/**
* @return ELightComponentType for the light component class 
*/
ELightComponentType UDirectionalLightComponent::GetLightType() const
{
	return LightType_Directional;
}
