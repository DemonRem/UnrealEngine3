/*=============================================================================
	VSMShadowRendering.h: VSM (Variance Shadow Map) rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if SUPPORTS_VSM

/**
* A pixel shader for projecting a shadow variance buffer onto the scene.
*/
class FVSMProjectionPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FVSMProjectionPixelShader,Global);
public:

	/** 
	* Constructor
	*/
	FVSMProjectionPixelShader() {}

	/** 
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FVSMProjectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* Sets the current pixel shader params
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	void SetParameters(FCommandContextRHI* Context,const FSceneView* View,const FProjectedShadowInfo* ShadowInfo);

	// FShader interface.	

	/**
	* @param Platform - hardware platform
	* @return TRUE if this shader should be cached
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform);	
	
	/**
	* Serialize shader parameters for this shader
	* @param Ar - archive to serialize with
	*/
	virtual void Serialize(FArchive& Ar);

	/** VSM tweak epsilon */
	static FLOAT VSMEpsilon;
	/** VSM tweak exponent */
	static FLOAT VSMExponent;

private:
	/** params to sample scene color/depth */
	FSceneTextureShaderParameters SceneTextureParams;
	/** screen space to shadow map transform param */
	FShaderParameter ScreenToShadowMatrixParameter;
	/** shadow variance moments (M,M*M) param */
	FShaderParameter ShadowVarianceTextureParameter;
	/** shadow depth texture param */
	FShaderParameter ShadowDepthTextureParam;
	/** VSM tweak epsilon param */
	FShaderParameter VSMEpsilonParam;
	/** VSM tweak exponent param */
	FShaderParameter VSMExponentParam;
};

/**
* A pixel shader for projecting a shadow variance buffer onto the scene.
* Attenuates shadow based on distance and modulates its color.
* For use with modulated shadows.
*/
class FVSMModProjectionPixelShader : public FVSMProjectionPixelShader
{
	DECLARE_SHADER_TYPE(FVSMModProjectionPixelShader,Global);
public:

	/**
	* Constructor
	*/
	FVSMModProjectionPixelShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FVSMModProjectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* Sets the current pixel shader
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	virtual void SetParameters( FCommandContextRHI* Context, const FSceneView* View, const FProjectedShadowInfo* ShadowInfo );

	// FShader interface.

	/**
	* Serialize the parameters for this shader
	* @param Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar);

private:
	/** color to modulate shadowed areas on screen */
	FShaderParameter ShadowModulateColorParam;	
	/** needed to get world positions from deferred scene depth values */
	FShaderParameter ScreenToWorldParam;	
};

/**
* Attenuation is based on light type so the modulated shadow projection 
* is coupled with a LightTypePolicy type. Use with FModShadowProjectionVertexShader
*/
template<class LightTypePolicy>
class TVSMModProjectionPixelShader : public FVSMModProjectionPixelShader, public LightTypePolicy::ModShadowPixelParamsType
{
	DECLARE_SHADER_TYPE(TVSMModProjectionPixelShader,Global);
public:
	typedef typename LightTypePolicy::SceneInfoType LightSceneInfoType;

	/**
	* Constructor
	*/
	TVSMModProjectionPixelShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	TVSMModProjectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FVSMModProjectionPixelShader(Initializer)
	{
		LightTypePolicy::ModShadowPixelParamsType::Bind(Initializer.ParameterMap);
	}

	/**
	* Add any defines required by the shader or light policy
	* @param OutEnvironment - shader environment to modify
	*/
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		FVSMModProjectionPixelShader::ModifyCompilationEnvironment(OutEnvironment);
		LightTypePolicy::ModShadowPixelParamsType::ModifyCompilationEnvironment(OutEnvironment);		
	}

	/**
	* Sets the current pixel shader params
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	void SetParameters(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		) const
	{
		FVSMModProjectionPixelShader::SetParameters(Context,View,ShadowInfo);
		LightTypePolicy::ModShadowPixelParamsType::SetModShadowLight( Context, this, (const LightSceneInfoType*) ShadowInfo->LightSceneInfo );
	}

	/**
	* Serialize the parameters for this shader
	* @param Ar - archive to serialize to
	*/
	void Serialize(FArchive& Ar)
	{
		FVSMModProjectionPixelShader::Serialize(Ar);
		LightTypePolicy::ModShadowPixelParamsType::Serialize(Ar);
	}
};

/**
* Renders the shadow subject variance by sampling the existing shadow depth texture
* @param Context - command context to render with
* @param SceneRenderer - current scene rendering info
* @param Shadows - array of projected shadow infos
* @param DPGIndex - DPG level rendering the shadow projection pass
*/
void RenderShadowVariance(FCommandContextRHI* Context, const class FSceneRenderer* SceneRenderer, const TArray<FProjectedShadowInfo*>& Shadows, UINT DPGIndex );

#endif //#if SUPPORTS_VSM


