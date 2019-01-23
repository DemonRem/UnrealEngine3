/*=============================================================================
FogVolumeRendering.h: Definitions for rendering fog volumes.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
enum EFogVolumeDensityFunction
{
	FVDF_None,
	FVDF_Constant,
	FVDF_ConstantHeight,
	FVDF_LinearHalfspace,
	FVDF_Sphere,
	FVDF_Cone
};

/*-----------------------------------------------------------------------------
FFogVolumeVertexShaderParameters - encapsulates the parameters needed to calculate fog from a fog volume in a vertex shader.
-----------------------------------------------------------------------------*/
class FFogVolumeVertexShaderParameters
{
public:

	/** Binds the parameters */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** Sets the parameters on the input VertexShader, using fog volume data from the input DensitySceneInfo. */
	void Set(
		FCommandContextRHI* Context,
		const FMaterialRenderProxy* MaterialRenderProxy,
		FShader* VertexShader, 
		const FFogVolumeDensitySceneInfo* FogVolumeSceneInfo
		) const;

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FFogVolumeVertexShaderParameters& P);

private:
	FShaderParameter FirstDensityFunctionParameter;
	FShaderParameter SecondDensityFunctionParameter;
	FShaderParameter FogVolumeBoxMinParameter;
	FShaderParameter FogVolumeBoxMaxParameter;
	FShaderParameter ApproxFogColorParameter;
};

/**
* Base FogSceneInfo - derivatives store render thread density function data.
*/
class FFogVolumeDensitySceneInfo
{
public:

	/** The fog component the scene info is for. */
	const class UFogVolumeDensityComponent* Component;

	/** 
	* A color used to approximate the material color when fogging intersecting translucency. 
	* This is necessary because we can't evaluate the material when applying fog to the translucent object.
	*/
	FLinearColor ApproxFogColor;

	/** The AABB of the associated fog volume primitive component. */
	FBox VolumeBounds;

	/** Initialization ctor */
	FFogVolumeDensitySceneInfo(const class UFogVolumeDensityComponent* InComponent, const FBox &InVolumeBounds);

	/** Draw a mesh with this density function. */
	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId) = 0;

	/** Get the density function parameters that will be passed to the integral pixel shader. */
	virtual FVector4 GetFirstDensityFunctionParameters() const = 0;

	/** Get the density function parameters that will be passed to the integral pixel shader. */
	virtual FVector4 GetSecondDensityFunctionParameters() const
	{
		return FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	/** 
	* Returns an estimate of the maximum integral value that will be calculated by this density function. 
	* This will be used to normalize the integral on platforms that it has to be stored in fixed point.
	* Too large of estimates will increase aliasing, too small will cause the fog volume to be clamped.
	*/
	virtual FLOAT GetMaxIntegral() const = 0;

	virtual EFogVolumeDensityFunction GetDensityFunctionType() const = 0;
};

/**
* Constant density fog
*/
class FFogVolumeConstantDensitySceneInfo : public FFogVolumeDensitySceneInfo
{
public:

	/** The constant density factor */
	FLOAT Density;

	/** Default constructor for creating a fog volume scene info without the corresponding component */
	FFogVolumeConstantDensitySceneInfo() :
		FFogVolumeDensitySceneInfo(NULL, FBox(0)),
		Density(0.005f)
	{}

	/** Initialization constructor. */
	FFogVolumeConstantDensitySceneInfo(const class UFogVolumeConstantDensityComponent* InComponent, const FBox &InVolumeBounds);

	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId);

	virtual FVector4 GetFirstDensityFunctionParameters() const;
	virtual FLOAT GetMaxIntegral() const;
	virtual EFogVolumeDensityFunction GetDensityFunctionType() const
	{
		return FVDF_Constant;
	}
};

/**
* Constant density fog, limited to a height
*/
class FFogVolumeConstantHeightDensitySceneInfo : public FFogVolumeDensitySceneInfo
{
public:

	/** The constant density factor */
	FLOAT Density;

	/** Maximum height */
	FLOAT Height;

	/** Initialization constructor. */
	FFogVolumeConstantHeightDensitySceneInfo(const class UFogVolumeConstantHeightDensityComponent* InComponent, const FBox &InVolumeBounds);

	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId);

	virtual FVector4 GetFirstDensityFunctionParameters() const;
	virtual FLOAT GetMaxIntegral() const;
	virtual EFogVolumeDensityFunction GetDensityFunctionType() const
	{
		return FVDF_ConstantHeight;
	}
};

/** Halfspace fog defined by a plane, with density increasing linearly away from the plane. */
class FFogVolumeLinearHalfspaceDensitySceneInfo : public FFogVolumeDensitySceneInfo
{
public:

	/** Linear density factor, scales the density contribution from the distance to the plane. */
	FLOAT PlaneDistanceFactor;

	/** Plane in worldspace that defines the fogged halfspace, whose normal points away from the fogged halfspace. */
	FPlane HalfspacePlane;

	/** Initialization constructor. */
	FFogVolumeLinearHalfspaceDensitySceneInfo(const class UFogVolumeLinearHalfspaceDensityComponent* InComponent, const FBox &InVolumeBounds);

	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId);

	virtual FVector4 GetFirstDensityFunctionParameters() const;
	virtual FVector4 GetSecondDensityFunctionParameters() const;
	virtual FLOAT GetMaxIntegral() const;
	virtual EFogVolumeDensityFunction GetDensityFunctionType() const
	{
		return FVDF_LinearHalfspace;
	}
};


/** Spherical fog with density decreasing toward the edges of the sphere for soft edges. */
class FFogVolumeSphericalDensitySceneInfo : public FFogVolumeDensitySceneInfo
{
public:

	/** The density at the center of the sphere, which is the maximum density. */
	FLOAT MaxDensity;

	/** The sphere in worldspace */
	FSphere Sphere;

	/** Initialization constructor. */
	FFogVolumeSphericalDensitySceneInfo(const class UFogVolumeSphericalDensityComponent* InComponent, const FBox &InVolumeBounds);

	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId);

	virtual FVector4 GetFirstDensityFunctionParameters() const;
	virtual FVector4 GetSecondDensityFunctionParameters() const;
	virtual FLOAT GetMaxIntegral() const;
	virtual EFogVolumeDensityFunction GetDensityFunctionType() const
	{
		return FVDF_Sphere;
	}
};



/**  */
class FFogVolumeConeDensitySceneInfo : public FFogVolumeDensitySceneInfo
{
public:

	/** The density along the axis, which is the maximum density. */
	FLOAT MaxDensity;

	/** World space position of the cone vertex. */
	FVector ConeVertex;

	/** Distance from the vertex at which the cone ends. */
	FLOAT ConeRadius;

	/** Axis defining the direction of the cone. */
	FVector ConeAxis;

	/** Angle from axis that defines the cone size. */
	FLOAT ConeMaxAngle;

	/** Initialization constructor. */
	FFogVolumeConeDensitySceneInfo(const class UFogVolumeConeDensityComponent* InComponent, const FBox &InVolumeBounds);

	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId);

	virtual FVector4 GetFirstDensityFunctionParameters() const;
	virtual FVector4 GetSecondDensityFunctionParameters() const;
	virtual FLOAT GetMaxIntegral() const;
	virtual EFogVolumeDensityFunction GetDensityFunctionType() const
	{
		return FVDF_Cone;
	}
};

class FNoDensityPolicy
{	
public:

	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;
	
	/** Empty parameter component. */
	class VertexShaderParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
		}
		void Set(
			FCommandContextRHI* Context,
			const FMaterialRenderProxy* MaterialRenderProxy,
			FShader* VertexShader,
			ElementDataType ElementData
			) const
		{
		}

		/** Serializer. */
		friend FArchive& operator<<(FArchive& Ar,VertexShaderParametersType& P)
		{
			return Ar;
		}
	};

	static const EFogVolumeDensityFunction DensityFunctionType = FVDF_None;

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return TRUE;
	}
	
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("FOGVOLUMEDENSITY_NONE"),TEXT("1"));
	}
};

class FConstantDensityPolicy
{	
public:

	typedef FFogVolumeVertexShaderParameters VertexShaderParametersType;
	typedef const FFogVolumeDensitySceneInfo* ElementDataType;

	static const EFogVolumeDensityFunction DensityFunctionType = FVDF_Constant;

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//don't compile the translucency vertex shader for GPU skinned vertex factories with this density function since it will run out of constant registers
		if (!Material->IsUsedWithFogVolumes() && appStrstr(VertexFactoryType->GetName(), TEXT("FGPUSkin")))
		{
			return FALSE;
		}
		return TRUE;
	}
	
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("FOGVOLUMEDENSITY_CONSTANT"),TEXT("1"));
	}
};

class FLinearHalfspaceDensityPolicy
{	
public:
	
	typedef FFogVolumeVertexShaderParameters VertexShaderParametersType;
	typedef const FFogVolumeDensitySceneInfo* ElementDataType;

	static const EFogVolumeDensityFunction DensityFunctionType = FVDF_LinearHalfspace;

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//don't compile the translucency vertex shader for GPU skinned vertex factories with this density function since it will run out of constant registers
		if (!Material->IsUsedWithFogVolumes() && appStrstr(VertexFactoryType->GetName(), TEXT("FGPUSkin")))
		{
			return FALSE;
		}
		return TRUE;
	}
	
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("FOGVOLUMEDENSITY_LINEARHALFSPACE"),TEXT("1"));
	}
};

class FSphereDensityPolicy
{	
public:
	
	typedef FFogVolumeVertexShaderParameters VertexShaderParametersType;
	typedef const FFogVolumeDensitySceneInfo* ElementDataType;

	static const EFogVolumeDensityFunction DensityFunctionType = FVDF_Sphere;

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//don't compile the translucency vertex shader for GPU skinned vertex factories with this density function since it will run out of constant registers
		if (!Material->IsUsedWithFogVolumes() && appStrstr(VertexFactoryType->GetName(), TEXT("FGPUSkin")))
		{
			return FALSE;
		}
		return TRUE;
	}
	
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("FOGVOLUMEDENSITY_SPHEREDENSITY"),TEXT("1"));
	}
};

class FConeDensityPolicy
{	
public:
	
	typedef FFogVolumeVertexShaderParameters VertexShaderParametersType;
	typedef const FFogVolumeDensitySceneInfo* ElementDataType;

	static const EFogVolumeDensityFunction DensityFunctionType = FVDF_Cone;

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//not fully implemented
		return FALSE;
	}
	
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("FOGVOLUMEDENSITY_CONEDENSITY"),TEXT("1"));
	}
};

/**
* A vertex shader for rendering meshes during the integral accumulation passes.
*/
template<class DensityFunctionPolicy>
class TFogIntegralVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(TFogIntegralVertexShader,MeshMaterial);

public:
	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->IsUsedWithFogVolumes()
			&& DensityFunctionPolicy::ShouldCache(Platform,Material,VertexFactoryType);
	}

	TFogIntegralVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FShader(Initializer)
		,	VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
		HeightFogParameters.Bind(Initializer.ParameterMap);
	}

	TFogIntegralVertexShader()
	{
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << VertexFactoryParameters;
		Ar << HeightFogParameters;
	}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);
		HeightFogParameters.Set(Context, MaterialRenderProxy, View, this);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}

private:
	FVertexFactoryParameterRef VertexFactoryParameters;
	FHeightFogVertexShaderParameters HeightFogParameters;
};

/**
* A pixel shader for rendering meshes during the integral accumulation passes.
* Density function-specific parameters are packed into FirstDensityFunctionParameters and SecondDensityFunctionParameters
*/
template<class DensityFunctionPolicy>
class TFogIntegralPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(TFogIntegralPixelShader,MeshMaterial);

public:
	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->IsUsedWithFogVolumes()
			&& DensityFunctionPolicy::ShouldCache(Platform,Material,VertexFactoryType);
	}

	TFogIntegralPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FShader(Initializer)
	{
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		DepthFilterSampleOffsets.Bind(Initializer.ParameterMap,TEXT("DepthFilterSampleOffsets"),TRUE);
		ScreenToWorldParameter.Bind(Initializer.ParameterMap,TEXT("ScreenToWorld"),TRUE);
		CameraPosParameter.Bind(Initializer.ParameterMap,TEXT("FogCameraPosition"),TRUE);
		FaceParameter.Bind(Initializer.ParameterMap,TEXT("FaceScale"),TRUE);
		FirstDensityFunctionParameters.Bind(Initializer.ParameterMap,TEXT("FirstDensityFunctionParameters"),TRUE);
		SecondDensityFunctionParameters.Bind(Initializer.ParameterMap,TEXT("SecondDensityFunctionParameters"),TRUE);
		InvMaxIntegralParameter.Bind(Initializer.ParameterMap,TEXT("InvMaxIntegral"), TRUE);
	}

	TFogIntegralPixelShader()
	{
	}

	void SetParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo,
		UBOOL bBackFace)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View.Family->CurrentWorldTime, View.Family->CurrentRealTime, &View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);

		const FVector2D SceneDepthTexelSize = FVector2D( 1.0f / (FLOAT)GSceneRenderTargets.GetBufferSizeX(), 1.0f / (FLOAT)GSceneRenderTargets.GetBufferSizeY());

		//sample the texel to the left and above
		const FVector4 FirstOffsets = FVector4(
			-SceneDepthTexelSize.X, 0,
			0, SceneDepthTexelSize.Y);

		SetPixelShaderValue(Context, GetPixelShader(), DepthFilterSampleOffsets, FirstOffsets, 0);

		//sample the texel to the right and below
		const FVector4 SecondOffsets = FVector4(
			SceneDepthTexelSize.X, 0,
			0, -SceneDepthTexelSize.Y);

		SetPixelShaderValue(Context, GetPixelShader(), DepthFilterSampleOffsets, SecondOffsets, 1);

		//Transform from post projection space to world space, used to reconstruct world space positions to calculate the fog integral
		FMatrix ScreenToWorld = 
			FMatrix(
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,(1.0f - Z_PRECISION),1),
			FPlane(0,0,-View.NearClippingDistance * (1.0f - Z_PRECISION),0)
			) * View.InvViewProjectionMatrix;

		SetPixelShaderValue(Context, GetPixelShader(), ScreenToWorldParameter, ScreenToWorld );
		SetPixelShaderValue(Context, GetPixelShader(), CameraPosParameter, View.ViewOrigin);

		//set the face parameter with 1 if a backface is being rendered, or -1 if a frontface is being rendered
		SetPixelShaderValue(Context, GetPixelShader(), FaceParameter, bBackFace ? 1.0f : -1.0f);
		
		//set the parameters specific to the DensityFunctionPolicy
		SetPixelShaderValue(Context, GetPixelShader(), FirstDensityFunctionParameters, DensitySceneInfo->GetFirstDensityFunctionParameters());
		SetPixelShaderValue(Context, GetPixelShader(), SecondDensityFunctionParameters, DensitySceneInfo->GetSecondDensityFunctionParameters());
		SetPixelShaderValue(Context, GetPixelShader(), InvMaxIntegralParameter, 1.0f / DensitySceneInfo->GetMaxIntegral());
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMaterialRenderProxy* MaterialRenderProxy,const FMatrix& LocalToWorld,UBOOL bBackFace)
	{
		MaterialParameters.SetLocalTransforms(Context,this,MaterialRenderProxy,LocalToWorld,bBackFace);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << MaterialParameters;	
		Ar << DepthFilterSampleOffsets;
		Ar << ScreenToWorldParameter;
		Ar << CameraPosParameter;
		Ar << FaceParameter;
		Ar << FirstDensityFunctionParameters;
		Ar << SecondDensityFunctionParameters;
		Ar << InvMaxIntegralParameter;
	}

private:
	FMaterialPixelShaderParameters MaterialParameters;
	FShaderParameter DepthFilterSampleOffsets;
	FShaderParameter ScreenToWorldParameter;
	FShaderParameter CameraPosParameter;
	FShaderParameter FaceParameter;
	FShaderParameter FirstDensityFunctionParameters;
	FShaderParameter SecondDensityFunctionParameters;
	FShaderParameter InvMaxIntegralParameter;
};


/**
* Policy for accumulating fog line integrals
*/
template<class DensityFunctionPolicy>
class TFogIntegralDrawingPolicy : public FMeshDrawingPolicy
{
public:
	/** context type */
	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;

	/**
	* Constructor
	* @param InIndexBuffer - index buffer for rendering
	* @param InVertexFactory - vertex factory for rendering
	* @param InMaterialRenderProxy - material instance for rendering
	*/
	TFogIntegralDrawingPolicy(const FVertexFactory* InVertexFactory,const FMaterialRenderProxy* InMaterialRenderProxy);

	// FMeshDrawingPolicy interface.

	/**
	* Match two draw policies
	* @param Other - draw policy to compare
	* @return TRUE if the draw policies are a match
	*/
	UBOOL Matches(const TFogIntegralDrawingPolicy& Other) const;

	/**
	* Executes the draw commands which can be shared between any meshes using this drawer.
	* @param CI - The command interface to execute the draw commands on.
	* @param View - The view of the scene being drawn.
	*/
	void DrawShared(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		FBoundShaderStateRHIParamRef BoundShaderState, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo,
		UBOOL bBackFace) const;

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState(DWORD DynamicStride = 0);

	/**
	* Sets the render states for drawing a mesh.
	* @param Context - command context
	* @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
	* @param Mesh - mesh element with data needed for rendering
	* @param ElementData - context specific data for mesh rendering
	*/
	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const;

private:
	TFogIntegralVertexShader<DensityFunctionPolicy>* VertexShader;
	TFogIntegralPixelShader<DensityFunctionPolicy>* PixelShader;
};

/**
* Fog integral mesh drawing policy factory. 
* Creates the policies needed for rendering a mesh based on its material
*/
template<class DensityFunctionPolicy>
class TFogIntegralDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = FALSE };

	/**
	* Render a dynamic mesh using a fog integral mesh drawing policy 
	* @return TRUE if the mesh rendered
	*/
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo)
	{
		TFogIntegralDrawingPolicy<DensityFunctionPolicy> DrawingPolicy(
			Mesh.VertexFactory,
			Mesh.MaterialRenderProxy
			);
		DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()), DensitySceneInfo, bBackFace);
		DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,typename TFogIntegralDrawingPolicy<DensityFunctionPolicy>::ElementDataType());
		DrawingPolicy.DrawMesh(Context,Mesh);

		return TRUE;
	}
};


/**
* A vertex shader for rendering meshes during the fog apply pass.
*/
class FFogVolumeApplyVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(FFogVolumeApplyVertexShader,MeshMaterial);

public:
	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->IsUsedWithFogVolumes();
	}

	FFogVolumeApplyVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FShader(Initializer)
		,	VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
		HeightFogParameters.Bind(Initializer.ParameterMap);
	}

	FFogVolumeApplyVertexShader()
	{
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << VertexFactoryParameters;
		Ar << HeightFogParameters;
	}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);

		HeightFogParameters.Set(Context, MaterialRenderProxy, View, this);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}

private:
	FVertexFactoryParameterRef VertexFactoryParameters;
	FHeightFogVertexShaderParameters HeightFogParameters;
};


/**
* A pixel shader for rendering meshes during the fog apply pass.
*/
class FFogVolumeApplyPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FFogVolumeApplyPixelShader,MeshMaterial);

public:
	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->IsUsedWithFogVolumes();
	}

	FFogVolumeApplyPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FShader(Initializer)
	{
		MaxIntegralParameter.Bind(Initializer.ParameterMap,TEXT("MaxIntegral"), TRUE);
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		AccumulatedFrontfacesLineIntegralTextureParam.Bind(Initializer.ParameterMap,TEXT("AccumulatedFrontfacesLineIntegralTexture"), TRUE);
		AccumulatedBackfacesLineIntegralTextureParam.Bind(Initializer.ParameterMap,TEXT("AccumulatedBackfacesLineIntegralTexture"), TRUE);
	}

	FFogVolumeApplyPixelShader()
	{
	}

	void SetParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View.Family->CurrentWorldTime, View.Family->CurrentRealTime, &View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);

		//set the fog integral accumulation samplers so that the apply pass can lookup the result of the accumulation passes
		//linear filter since we're upsampling
		SetTextureParameter(
			Context,
			GetPixelShader(),
			AccumulatedFrontfacesLineIntegralTextureParam,
			TStaticSamplerState<SF_Linear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetFogFrontfacesIntegralAccumulationTexture()
			);

		SetTextureParameter(
			Context,
			GetPixelShader(),
			AccumulatedBackfacesLineIntegralTextureParam,
			TStaticSamplerState<SF_Linear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetFogBackfacesIntegralAccumulationTexture()
			);

		SetPixelShaderValue(Context, GetPixelShader(), MaxIntegralParameter, DensitySceneInfo->GetMaxIntegral());
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMaterialRenderProxy* MaterialRenderProxy,const FMatrix& LocalToWorld,UBOOL bBackFace)
	{
		MaterialParameters.SetLocalTransforms(Context,this,MaterialRenderProxy,LocalToWorld,bBackFace);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << MaxIntegralParameter;
		Ar << MaterialParameters;	
		Ar << AccumulatedFrontfacesLineIntegralTextureParam;
		Ar << AccumulatedBackfacesLineIntegralTextureParam;
	}

private:
	FShaderParameter MaxIntegralParameter;
	FMaterialPixelShaderParameters MaterialParameters;
	FShaderParameter AccumulatedFrontfacesLineIntegralTextureParam;
	FShaderParameter AccumulatedBackfacesLineIntegralTextureParam;
};


/**
* Policy for applying fog contribution to scene color
*/
class FFogVolumeApplyDrawingPolicy : public FMeshDrawingPolicy
{
public:
	/** context type */
	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;

	/**
	* Constructor
	* @param InIndexBuffer - index buffer for rendering
	* @param InVertexFactory - vertex factory for rendering
	* @param InMaterialRenderProxy - material instance for rendering
	*/
	FFogVolumeApplyDrawingPolicy(const FVertexFactory* InVertexFactory,const FMaterialRenderProxy* InMaterialRenderProxy);

	// FMeshDrawingPolicy interface.

	/**
	* Match two draw policies
	* @param Other - draw policy to compare
	* @return TRUE if the draw policies are a match
	*/
	UBOOL Matches(const FFogVolumeApplyDrawingPolicy& Other) const;

	/**
	* Executes the draw commands which can be shared between any meshes using this drawer.
	* @param CI - The command interface to execute the draw commands on.
	* @param View - The view of the scene being drawn.
	*/
	void DrawShared(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		FBoundShaderStateRHIParamRef BoundShaderState, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo) const;

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState(DWORD DynamicStride = 0);

	/**
	* Sets the render states for drawing a mesh.
	* @param Context - command context
	* @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
	* @param Mesh - mesh element with data needed for rendering
	* @param ElementData - context specific data for mesh rendering
	*/
	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const;

private:
	FFogVolumeApplyVertexShader* VertexShader;
	FFogVolumeApplyPixelShader* PixelShader;
};

/**
* Fog apply mesh drawing policy factory. 
* Creates the policies needed for rendering a mesh based on its material
*/
class FFogVolumeApplyDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = FALSE };
	struct ContextType {};

	/**
	* Render a dynamic mesh using a fog apply mesh draw policy 
	* @return TRUE if the mesh rendered
	*/
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId, 
		const FFogVolumeDensitySceneInfo* DensitySceneInfo
		);
};

extern void ResetFogVolumeIndex();

/**
* Render a Fog Volume.  The density function to use is found in PrimitiveSceneInfo->Scene's FogVolumes map.
* @return TRUE if the mesh rendered
*/
extern UBOOL RenderFogVolume(
	 FCommandContextRHI* Context,
	 const FViewInfo* View,
	 const FMeshElement& Mesh,
	 UBOOL bBackFace,
	 UBOOL bPreFog,
	 const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	 FHitProxyId HitProxyId);
