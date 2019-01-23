/*=============================================================================
	ShadowVolumeRendering.h: Shadow volume rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
* A vertex shader for projecting a shadow depth buffer onto the scene.
* For use with modulated shadows
*/
class FModShadowVolumeVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FModShadowVolumeVertexShader,Global);
public:

	/**
	* Constructor
	*/
	FModShadowVolumeVertexShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FModShadowVolumeVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* @param Platform - current platform being compiled
	* @return TRUE if this global shader should be compiled
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform);

	/**
	* Sets the current vertex shader
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	void SetParameters(FCommandContextRHI* Context,const FSceneView* View,const FLightSceneInfo* LightSceneInfo);

	/**
	* Serialize the parameters for this shader
	* @param Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar);

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}
};

/**
* Attenuates shadow volume based on distance and modulates its color.
* For use with modulated shadows.
*/
class FModShadowVolumePixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FModShadowVolumePixelShader,Global);
public:

	/**
	* Constructor
	*/
	FModShadowVolumePixelShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FModShadowVolumePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* @param Platform - current platform being compiled
	* @return TRUE if this global shader should be compiled
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform);

	/**
	* Sets the current pixel shader
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	virtual void SetParameters(FCommandContextRHI* Context,const FSceneView* View,const FLightSceneInfo* LightSceneInfo);

	/**
	* Serialize the parameters for this shader
	* @param Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar);

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}

private:
	/* scene texture shader parameter parameters */
	FSceneTextureShaderParameters SceneTextureParams;
	/** color to modulate shadowed areas on screen */
	FShaderParameter ShadowModulateColorParam;
	/** needed to get world positions from deferred scene depth values */
	FShaderParameter ScreenToWorldParam;	
};

/**
* Attenuation is based on light type so the modulated shadow volume
* is coupled with a LightTypePolicy type
*/
template<typename LightTypePolicy>
class TModShadowVolumePixelShader : public FModShadowVolumePixelShader, public LightTypePolicy::ModShadowPixelParamsType
{
	DECLARE_SHADER_TYPE(TModShadowVolumePixelShader,Global);
public:
	typedef typename LightTypePolicy::SceneInfoType LightSceneInfoType;

	/**
	* Constructor
	*/
	TModShadowVolumePixelShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	TModShadowVolumePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FModShadowVolumePixelShader(Initializer)
	{
		LightTypePolicy::ModShadowPixelParamsType::Bind(Initializer.ParameterMap);
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		FModShadowVolumePixelShader::ModifyCompilationEnvironment(OutEnvironment);
		LightTypePolicy::ModShadowPixelParamsType::ModifyCompilationEnvironment(OutEnvironment);
	}

	/**
	* Sets the current pixel shader
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	void SetParameters(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FLightSceneInfo* LightSceneInfo
		)
	{
		FModShadowVolumePixelShader::SetParameters(Context,View,LightSceneInfo);
		LightTypePolicy::ModShadowPixelParamsType::SetModShadowLight( Context, this, (const LightSceneInfoType*) LightSceneInfo );
	}

	/**
	* Serialize the parameters for this shader
	* @param Ar - archive to serialize to
	*/
	void Serialize(FArchive& Ar)
	{
		FModShadowVolumePixelShader::Serialize(Ar);
		LightTypePolicy::ModShadowPixelParamsType::Serialize(Ar);
	}

};

