/*=============================================================================
	ShadowRendering.h: Shadow rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Forward declarations.
class FProjectedShadowInfo;
// Globals
extern const UINT SHADOW_BORDER;

/**
 * Outputs no color, but can be used to write the mesh's depth values to the depth buffer.
 */
class FShadowDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:

	FShadowDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FProjectedShadowInfo* InShadowInfo
		);

	// FMeshDrawingPolicy interface.
	UBOOL Matches(const FShadowDepthDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) && VertexShader == Other.VertexShader && PixelShader == Other.PixelShader;
	}
	void DrawShared(FCommandContextRHI* Context,const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const;

	/** 
	 * Create bound shader state using the vertex decl from the mesh draw policy
	 * as well as the shaders needed to draw the mesh
	 * @param DynamicStride - optional stride for dynamic vertex data
	 * @return new bound shader state object
	 */
	FBoundShaderStateRHIRef CreateBoundShaderState(DWORD DynamicStride = 0);

	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const;
	friend INT Compare(const FShadowDepthDrawingPolicy& A,const FShadowDepthDrawingPolicy& B);

private:
	class FShadowDepthVertexShader* VertexShader;
	class FShadowDepthPixelShader* PixelShader;
	const class FProjectedShadowInfo* ShadowInfo;
};

/**
 * A drawing policy factory for the emissive drawing policy.
 */
class FShadowDepthDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = FALSE };
	typedef const FProjectedShadowInfo* ContextType;

	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);
	static UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		return FALSE;
	}
};

/**
 * A projected shadow transform.
 */
class FProjectedShadowInitializer
{
public:

	FMatrix PreSubjectMatrix;	// Z range from MinLightW to MaxSubjectW.
	FMatrix SubjectMatrix;		// Z range from MinSubjectW to MaxSubjectW.
	FMatrix PostSubjectMatrix;	// Z range from MinSubjectW to MaxShadowW.

	FLOAT MaxSubjectDepth;

	BITFIELD bDirectionalLight : 1;

	/**
	 * Calculates the shadow transforms for the given parameters.
	 */
	void CalcTransforms(
		const FMatrix& WorldToLight,
		const FVector& FaceDirection,
		const FSphere& SubjectBoundingSphere,
		const FVector4& WAxis,
		FLOAT MinLightW,
		FLOAT MaxLightW,
		UBOOL bInDirectionalLight
		);
};

/**
 * Information about a projected shadow.
 */
class FProjectedShadowInfo
{
public:

	friend class FShadowDepthVertexShader;
	friend class FShadowDepthPixelShader;
	friend class FShadowProjectionVertexShader;
	friend class FShadowProjectionPixelShader;
	friend class FShadowDepthDrawingPolicyFactory;

	const FLightSceneInfo* const LightSceneInfo;
	const FPrimitiveSceneInfo* const SubjectSceneInfo;

	FMatrix SubjectMatrix;
	FMatrix SubjectAndReceiverMatrix;
	FMatrix ReceiverMatrix;

	FMatrix InvReceiverMatrix;

	FLOAT MaxSubjectDepth;

	FConvexVolume SubjectFrustum;
	FConvexVolume ReceiverFrustum;

	UINT X;
	UINT Y;
	UINT Resolution;

	FLOAT FadeAlpha;

	BITFIELD bAllocated : 1;
	BITFIELD bRendered : 1;
	BITFIELD bDirectionalLight : 1;
	BITFIELD bPreShadow : 1;

	/** Initialization constructor. */
	FProjectedShadowInfo(
		const FLightSceneInfo* InLightSceneInfo,
		const FPrimitiveSceneInfo* InSubjectSceneInfo,
		const FProjectedShadowInitializer& Initializer,
		UBOOL bInPreShadow,
		UINT InResolution,
		FLOAT InFadeAlpha
		);

	/**
	 * Renders the shadow subject depth.
	 */
	void RenderDepth(FCommandContextRHI* Context, const class FSceneRenderer* SceneRenderer, BYTE DepthPriorityGroup);

	/**
	 * Projects the shadow onto the scene for a particular view.
	 */
	void RenderProjection(FCommandContextRHI* Context, const class FViewInfo* View, BYTE DepthPriorityGroup) const;

	/**
	 * Renders the projected shadow's frustum wireframe with the given FPrimitiveDrawInterface.
	 */
	void RenderFrustumWireframe(FPrimitiveDrawInterface* PDI) const;

	/**
	 * Adds a primitive to the shadow's subject list.
	 */
	void AddSubjectPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/**
	 * Adds a primitive to the shadow's receiver list.
	 */
	void AddReceiverPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/**
	* @return TRUE if this shadow info has any casting subject prims to render
	*/
	UBOOL HasSubjectPrims() const;

	/**
	 * Adds current subject primitives to out array.
	 *
	 * @param OutSubjectPrimitives [out]	Array to add current subject primitives to.
	 */
	void GetSubjectPrimitives( TArray<const FPrimitiveSceneInfo*>& OutSubjectPrimitives );

	/** Hash function. */
	friend DWORD GetTypeHash(const FProjectedShadowInfo* ProjectedShadowInfo)
	{
		return PointerHash(ProjectedShadowInfo);
	}

private:

	/** dynamic shadow casting elements */
	TArray<const FPrimitiveSceneInfo*> SubjectPrimitives;

	/** For preshadows, this contains the receiver primitives to mask the projection to. */
	TArray<const FPrimitiveSceneInfo*> ReceiverPrimitives;

	/** static shadow casting elements */
	TStaticMeshDrawList<FShadowDepthDrawingPolicy> SubjectMeshElements;

	/** bound shader state for stencil masking the shadow projection */
	static FBoundShaderStateRHIRef MaskBoundShaderState;
	/** bound shader state for shadow projection pass */
	static FGlobalBoundShaderStateRHIRef ShadowProjectionBoundShaderState;
	/** bound shader state for VSM shadow projection pass */
	static FGlobalBoundShaderStateRHIRef ShadowProjectionVSMBoundShaderState;
	/** bound shader state for Branching PCF shadow projection pass */
	static FGlobalBoundShaderStateRHIRef BranchingPCFLowQualityBoundShaderState;
	static FGlobalBoundShaderStateRHIRef BranchingPCFMediumQualityBoundShaderState;
	static FGlobalBoundShaderStateRHIRef BranchingPCFHighQualityBoundShaderState;
};

IMPLEMENT_COMPARE_POINTER(FProjectedShadowInfo,ShadowRendering,{ return B->Resolution - A->Resolution; });

/*-----------------------------------------------------------------------------
FShadowFrustumVertexDeclaration
-----------------------------------------------------------------------------*/

/** The shadow frustum vertex declaration resource type. */
class FShadowFrustumVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual ~FShadowFrustumVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.AddItem(FVertexElement(0,0,VET_Float3,VEU_Position,0));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.Release();
	}
};

/**
* A vertex shader for projecting a shadow depth buffer onto the scene.
*/
class FShadowProjectionVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowProjectionVertexShader,Global);
public:

	FShadowProjectionVertexShader() {}
	FShadowProjectionVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static UBOOL ShouldCache(EShaderPlatform Platform);

	void SetParameters(FCommandContextRHI* Context,const FSceneView* View,const FProjectedShadowInfo* ShadowInfo);
	virtual void Serialize(FArchive& Ar);

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}
};


/**
* A vertex shader for projecting a shadow depth buffer onto the scene.
* For use with modulated shadows
*/
class FModShadowProjectionVertexShader : public FShadowProjectionVertexShader
{
	DECLARE_SHADER_TYPE(FModShadowProjectionVertexShader,Global);
public:

	/**
	* Constructor
	*/
	FModShadowProjectionVertexShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FModShadowProjectionVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* Sets the current vertex shader
	* @param Context - command buffer context
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	void SetParameters(FCommandContextRHI* Context,const FSceneView* View,const FProjectedShadowInfo* ShadowInfo);

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
* 
*/
class F4SampleHwPCF
{
public:
	static const UINT NumSamples = 4;
	static const UBOOL bUseHardwarePCF = TRUE;
	static const UBOOL bUseFetch4 = FALSE;

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return Platform != SP_XBOXD3D;
	}
};

/**
* 
*/
class F4SampleManualPCF
{
public:
	static const UINT NumSamples = 4;
	static const UBOOL bUseHardwarePCF = FALSE;
	static const UBOOL bUseFetch4 = FALSE;

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}
};

/**
 * Policy to use on SM3 hardware that supports Hardware PCF
 */
class F16SampleHwPCF
{
public:
	static const UINT NumSamples = 16;
	static const UBOOL bUseHardwarePCF = TRUE;
	static const UBOOL bUseFetch4 = FALSE;

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return Platform == SP_PCD3D_SM3 || Platform == SP_PS3;
	}
};

/**
 * Policy to use on SM3 hardware that supports Fetch4
 */
class F16SampleFetch4PCF
{
public:
	static const UINT NumSamples = 16;
	static const UBOOL bUseHardwarePCF = FALSE;
	static const UBOOL bUseFetch4 = TRUE;

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return Platform == SP_PCD3D_SM3;
	}
};

/**
 * Policy to use on SM3 hardware with no support for Hardware PCF
 */
class F16SampleManualPCF
{
public:
	static const UINT NumSamples = 16;
	static const UBOOL bUseHardwarePCF = FALSE;
	static const UBOOL bUseFetch4 = FALSE;

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return Platform != SP_PCD3D_SM2;
	}
};

/**
 * Samples used with the SM3 version
 */
static const FVector2D SixteenSampleOffsets[] =
{
	FVector2D(-1.5f,-1.5f), FVector2D(-0.5f,-1.5f), FVector2D(+0.5f,-1.5f), FVector2D(+1.5f,-1.5f),
	FVector2D(-1.5f,-0.5f), FVector2D(-0.5f,-0.5f), FVector2D(+0.5f,-0.5f), FVector2D(+1.5f,-0.5f),
	FVector2D(-1.5f,+0.5f), FVector2D(-0.5f,+0.5f), FVector2D(+0.5f,+0.5f), FVector2D(+1.5f,+0.5f),
	FVector2D(-1.5f,+1.5f), FVector2D(-0.5f,+1.5f), FVector2D(+0.5f,+1.5f), FVector2D(+1.5f,+1.5f)
};

/**
 * Samples used with the SM2 version
 */
static const FVector2D FourSampleOffsets[] = 
{
	FVector2D(-0.5f,-0.5f), FVector2D(+0.5f,-0.5f), FVector2D(-0.5f,+0.5f), FVector2D(+0.5f,+0.5f)
};

/**
 * FShadowProjectionPixelShaderInterface - used to handle templated versions
 */

class FShadowProjectionPixelShaderInterface : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowProjectionPixelShaderInterface,Global);
public:

	FShadowProjectionPixelShaderInterface() 
		:	FGlobalShader()
	{}

	/**
	 * Constructor - binds all shader params and initializes the sample offsets
	 * @param Initializer - init data from shader compiler
	 */
	FShadowProjectionPixelShaderInterface(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{ }

	/**
	 * Sets the current pixel shader params
	 * @param Context - command buffer context
	 * @param View - current view
	 * @param ShadowInfo - projected shadow info for a single light
	 */
	virtual void SetParameters(FCommandContextRHI* Context,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{ }

};

/**
 * TShadowProjectionPixelShader
 * A pixel shader for projecting a shadow depth buffer onto the scene.  Used with any light type casting normal shadows.
 */
template<class UniformPCFPolicy> 
class TShadowProjectionPixelShader : public FShadowProjectionPixelShaderInterface
{
	DECLARE_SHADER_TYPE(TShadowProjectionPixelShader,Global);
public:

	TShadowProjectionPixelShader()
		: FShadowProjectionPixelShaderInterface()
	{ 
		SetSampleOffsets(); 
	}

	/**
	 * Constructor - binds all shader params and initializes the sample offsets
	 * @param Initializer - init data from shader compiler
	 */
	TShadowProjectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FShadowProjectionPixelShaderInterface(Initializer)
	{
		SceneTextureParams.Bind(Initializer.ParameterMap);
		ScreenToShadowMatrixParameter.Bind(Initializer.ParameterMap,TEXT("ScreenToShadowMatrix"));
		ShadowDepthTextureParameter.Bind(Initializer.ParameterMap,TEXT("ShadowDepthTexture"));
		SampleOffsetsParameter.Bind(Initializer.ParameterMap,TEXT("SampleOffsets"),TRUE);
		ShadowBufferSizeParameter.Bind(Initializer.ParameterMap,TEXT("ShadowBufferSize"),TRUE);

		SetSampleOffsets();
	}

	/**
	 *  Initializes the sample offsets
	 */
	void SetSampleOffsets()
	{
		check(UniformPCFPolicy::NumSamples == 4 || UniformPCFPolicy::NumSamples == 16);

		if (UniformPCFPolicy::NumSamples == 4)
		{
			appMemcpy(SampleOffsets, FourSampleOffsets, 4 * sizeof(FVector2D));
		}
		else if (UniformPCFPolicy::NumSamples == 16)
		{
			appMemcpy(SampleOffsets, SixteenSampleOffsets, 16 * sizeof(FVector2D));
		}
	}

	/**
	 * @param Platform - hardware platform
	 * @return TRUE if this shader should be cached
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return UniformPCFPolicy::ShouldCache(Platform);
	}

	/**
	 * Add any defines required by the shader
	 * @param OutEnvironment - shader environment to modify
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("NUM_SAMPLE_CHUNKS"),*FString::Printf(TEXT("%u"),UniformPCFPolicy::NumSamples / 4));
	}

	/**
	 * Sets the pixel shader's parameters
	 * @param Context - command buffer context
	 * @param View - current view
	 * @param ShadowInfo - projected shadow info for a single light
	 */
	virtual void SetParameters(FCommandContextRHI* Context,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		SceneTextureParams.Set(Context,View,this);

		// Set the transform from screen coordinates to shadow depth texture coordinates.
		const FLOAT InvBufferResolution = 1.0f / (FLOAT)GSceneRenderTargets.GetShadowDepthTextureResolution();
		const FLOAT ShadowResolutionFraction = 0.5f * (FLOAT)ShadowInfo->Resolution * InvBufferResolution;
		FMatrix	ScreenToShadow = FMatrix(
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,View->ProjectionMatrix.M[2][2],1),
			FPlane(0,0,View->ProjectionMatrix.M[3][2],0)
			) *
			View->InvViewProjectionMatrix *
			ShadowInfo->SubjectAndReceiverMatrix *
			FMatrix(
			FPlane(ShadowResolutionFraction,0,							0,									0),
			FPlane(0,						-ShadowResolutionFraction,	0,									0),
			FPlane(0,						0,							1.0f / ShadowInfo->MaxSubjectDepth,	0),
			FPlane(
			(ShadowInfo->X + SHADOW_BORDER + GPixelCenterOffset) * InvBufferResolution + ShadowResolutionFraction,
			(ShadowInfo->Y + SHADOW_BORDER + GPixelCenterOffset) * InvBufferResolution + ShadowResolutionFraction,
			0,
			1
			)
			);
		SetPixelShaderValue(Context,FShader::GetPixelShader(),ScreenToShadowMatrixParameter,ScreenToShadow);

		if (ShadowBufferSizeParameter.IsBound())
		{
			SetPixelShaderValue(Context,GetPixelShader(),ShadowBufferSizeParameter, FVector2D((FLOAT)GSceneRenderTargets.GetShadowDepthTextureResolution(), (FLOAT)GSceneRenderTargets.GetShadowDepthTextureResolution()));
		}

		FTexture2DRHIRef ShadowDepthSampler;
		FSamplerStateRHIParamRef DepthSamplerState;

		if (UniformPCFPolicy::bUseHardwarePCF)
		{
			//take advantage of linear filtering on nvidia depth stencil textures
			DepthSamplerState = TStaticSamplerState<SF_Linear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
			//sample the depth texture
			ShadowDepthSampler = GSceneRenderTargets.GetShadowDepthZTexture();
		}
		else if (GSupportsDepthTextures)
		{
			DepthSamplerState = TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
			//sample the depth texture
			ShadowDepthSampler = GSceneRenderTargets.GetShadowDepthZTexture();
		} 
		else if (UniformPCFPolicy::bUseFetch4)
		{
			//enable Fetch4 on this sampler
			DepthSamplerState = TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp,MIPBIAS_Get4>::GetRHI();
			//sample the depth texture
			ShadowDepthSampler = GSceneRenderTargets.GetShadowDepthZTexture();
		}
		else
		{
			DepthSamplerState = TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
			//sample the color depth texture
			ShadowDepthSampler = GSceneRenderTargets.GetShadowDepthColorTexture();
		}

		SetTextureParameter(
			Context,
			FShader::GetPixelShader(),
			ShadowDepthTextureParameter,
			DepthSamplerState,
			ShadowDepthSampler
			);

		const FLOAT CosRotation = appCos(0.25f * (FLOAT)PI);
		const FLOAT SinRotation = appSin(0.25f * (FLOAT)PI);
		const FLOAT TexelRadius = GEngine->ShadowFilterRadius / 2 * InvBufferResolution;
		//set the sample offsets
		for(INT SampleIndex = 0;SampleIndex < UniformPCFPolicy::NumSamples;SampleIndex += 2)
		{
			SetPixelShaderValue(
				Context,
				FShader::GetPixelShader(),
				SampleOffsetsParameter,
				FVector4(
				(SampleOffsets[0].X * +CosRotation + SampleOffsets[SampleIndex + 0].Y * +SinRotation) * TexelRadius,
				(SampleOffsets[0].X * -SinRotation + SampleOffsets[SampleIndex + 0].Y * +CosRotation) * TexelRadius,
				(SampleOffsets[1].X * +CosRotation + SampleOffsets[SampleIndex + 1].Y * +SinRotation) * TexelRadius,
				(SampleOffsets[1].X * -SinRotation + SampleOffsets[SampleIndex + 1].Y * +CosRotation) * TexelRadius
				),
				SampleIndex / 2
				);
		}
	}

	/**
	 * Serialize the parameters for this shader
	 * @param Ar - archive to serialize to
	 */
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << SceneTextureParams;
		Ar << ScreenToShadowMatrixParameter;
		Ar << ShadowDepthTextureParameter;
		Ar << SampleOffsetsParameter;
		Ar << ShadowBufferSizeParameter;
	}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}

private:
	FVector2D SampleOffsets[UniformPCFPolicy::NumSamples];
	FSceneTextureShaderParameters SceneTextureParams;
	FShaderParameter ScreenToShadowMatrixParameter;
	FShaderParameter ShadowDepthTextureParameter;
	FShaderParameter SampleOffsetsParameter;	
	FShaderParameter ShadowBufferSizeParameter;
};

/**
 * TModShadowProjectionPixelShader - pixel shader used with lights casting modulative shadows
 * Attenuation is based on light type so the modulated shadow projection is coupled with a LightTypePolicy type
 */
template<class LightTypePolicy, class UniformPCFPolicy>
class TModShadowProjectionPixelShader : public TShadowProjectionPixelShader<UniformPCFPolicy>, public LightTypePolicy::ModShadowPixelParamsType
{
	DECLARE_SHADER_TYPE(TModShadowProjectionPixelShader,Global);
public:
	typedef typename LightTypePolicy::SceneInfoType LightSceneInfoType;

	/**
	 * Constructor
	 */
	TModShadowProjectionPixelShader() {}

	/**
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	TModShadowProjectionPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	TShadowProjectionPixelShader<UniformPCFPolicy>(Initializer)
	{
		ShadowModulateColorParam.Bind(Initializer.ParameterMap,TEXT("ShadowModulateColor"));
		ScreenToWorldParam.Bind(Initializer.ParameterMap,TEXT("ScreenToWorld"),TRUE);
		LightTypePolicy::ModShadowPixelParamsType::Bind(Initializer.ParameterMap);
	}

	/**
	 * @param Platform - hardware platform
	 * @return TRUE if this shader should be cached
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TShadowProjectionPixelShader<UniformPCFPolicy>::ShouldCache(Platform);
	}

	/**
	 * Add any defines required by the shader or light policy
	 * @param OutEnvironment - shader environment to modify
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		TShadowProjectionPixelShader<UniformPCFPolicy>::ModifyCompilationEnvironment(OutEnvironment);
        LightTypePolicy::ModShadowPixelParamsType::ModifyCompilationEnvironment(OutEnvironment);	
	}

	/**
	 * Sets the pixel shader's parameters
	 * @param Context - command buffer context
	 * @param View - current view
	 * @param ShadowInfo - projected shadow info for a single light
	 */
	virtual void SetParameters(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		TShadowProjectionPixelShader<UniformPCFPolicy>::SetParameters(Context,View,ShadowInfo);		
		const FLightSceneInfo* LightSceneInfo = ShadowInfo->LightSceneInfo;

		// color to modulate shadowed areas on screen
		SetPixelShaderValue(
			Context,
			FShader::GetPixelShader(),
			ShadowModulateColorParam,
			Lerp(FLinearColor::White,LightSceneInfo->ModShadowColor,ShadowInfo->FadeAlpha)
			);
		// screen space to world space transform
		FMatrix ScreenToWorld = FMatrix(
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,(1.0f - Z_PRECISION),1),
			FPlane(0,0,-View->NearClippingDistance * (1.0f - Z_PRECISION),0)
			) * 
			View->InvViewProjectionMatrix;	
		SetPixelShaderValue( Context, FShader::GetPixelShader(), ScreenToWorldParam, ScreenToWorld );

		LightTypePolicy::ModShadowPixelParamsType::SetModShadowLight( Context, this, (const LightSceneInfoType*) ShadowInfo->LightSceneInfo );
	}

	/**
	 * Serialize the parameters for this shader
	 * @param Ar - archive to serialize to
	 */
	virtual void Serialize(FArchive& Ar)
	{
		TShadowProjectionPixelShader<UniformPCFPolicy>::Serialize(Ar);
		Ar << ShadowModulateColorParam;
		Ar << ScreenToWorldParam;
		LightTypePolicy::ModShadowPixelParamsType::Serialize(Ar);
	}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}

private:
	/** color to modulate shadowed areas on screen */
	FShaderParameter ShadowModulateColorParam;	
	/** needed to get world positions from deferred scene depth values */
	FShaderParameter ScreenToWorldParam;	
};

/**
 * Get the version of TModShadowProjectionPixelShader that should be used based on the hardware's capablities
 * @return a pointer to the chosen shader
 */
template<class LightTypePolicy>
FShadowProjectionPixelShaderInterface* GetModProjPixelShaderRef(BYTE LightShadowQuality)
{
	//apply the system settings bias to the light's shadow quality
	BYTE EffectiveShadowFilterQuality = Max(LightShadowQuality + GSystemSettings.ShadowFilterQualityBias, 0);

	//force using 4 samples for shader model 2 cards
	if (EffectiveShadowFilterQuality == SFQ_Low || GRHIShaderPlatform == SP_PCD3D_SM2)
	{
		if (GSupportsHardwarePCF)
		{
			TShaderMapRef<TModShadowProjectionPixelShader<LightTypePolicy,F4SampleHwPCF> > ModShadowShader(GetGlobalShaderMap());
			return *ModShadowShader;
		}
		else
		{
			TShaderMapRef<TModShadowProjectionPixelShader<LightTypePolicy,F4SampleManualPCF> > ModShadowShader(GetGlobalShaderMap());
			return *ModShadowShader;
		}
	}
	else
	{
		if (GSupportsHardwarePCF)
		{
			TShaderMapRef<TModShadowProjectionPixelShader<LightTypePolicy,F16SampleHwPCF> > ModShadowShader(GetGlobalShaderMap());
			return *ModShadowShader;
		}
		else if (GSupportsFetch4)
		{
			TShaderMapRef<TModShadowProjectionPixelShader<LightTypePolicy,F16SampleFetch4PCF> > ModShadowShader(GetGlobalShaderMap());
			return *ModShadowShader;
		}
		else
		{
			TShaderMapRef<TModShadowProjectionPixelShader<LightTypePolicy,F16SampleManualPCF> > ModShadowShader(GetGlobalShaderMap());
			return *ModShadowShader;
		}
	}
	
}

