/*=============================================================================
FogVolumeRendering.cpp: Implementation for rendering fog volumes.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "EngineFogVolumeClasses.h"


/*-----------------------------------------------------------------------------
FFogVolumeVertexShaderParameters
-----------------------------------------------------------------------------*/

/** Binds the parameters */
void FFogVolumeVertexShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	FirstDensityFunctionParameter.Bind(ParameterMap,TEXT("FirstDensityFunctionParameters"),TRUE);
	SecondDensityFunctionParameter.Bind(ParameterMap,TEXT("SecondDensityFunctionParameters"),TRUE);
	FogVolumeBoxMinParameter.Bind(ParameterMap,TEXT("FogVolumeBoxMin"),TRUE);
	FogVolumeBoxMaxParameter.Bind(ParameterMap,TEXT("FogVolumeBoxMax"),TRUE);
	ApproxFogColorParameter.Bind(ParameterMap,TEXT("ApproxFogColor"),TRUE);
}

/** Sets the parameters on the input VertexShader, using fog volume data from the input DensitySceneInfo. */
void FFogVolumeVertexShaderParameters::Set(
	FCommandContextRHI* Context, 
	const FMaterialRenderProxy* MaterialRenderProxy, 
	FShader* VertexShader, 
	const FFogVolumeDensitySceneInfo* FogVolumeSceneInfo
	) const
{
	if (FogVolumeSceneInfo)
	{
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FirstDensityFunctionParameter, FogVolumeSceneInfo->GetFirstDensityFunctionParameters());
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), SecondDensityFunctionParameter, FogVolumeSceneInfo->GetSecondDensityFunctionParameters());
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), ApproxFogColorParameter, FogVolumeSceneInfo->ApproxFogColor);
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FogVolumeBoxMinParameter, FogVolumeSceneInfo->VolumeBounds.Min);
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FogVolumeBoxMaxParameter, FogVolumeSceneInfo->VolumeBounds.Max);
	}
	else
	{
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FirstDensityFunctionParameter, FVector4(0.0f, 0.0f, 0.0f, 0.0f));
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), SecondDensityFunctionParameter, FVector4(0.0f, 0.0f, 0.0f, 0.0f));
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), ApproxFogColorParameter, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FogVolumeBoxMinParameter, FVector(0.0f, 0.0f, 0.0f));
		SetVertexShaderValue(Context, VertexShader->GetVertexShader(), FogVolumeBoxMaxParameter, FVector(0.0f, 0.0f, 0.0f));
	}
}

/** Serializer */
FArchive& operator<<(FArchive& Ar,FFogVolumeVertexShaderParameters& Parameters)
{
	Ar << Parameters.FirstDensityFunctionParameter;
	Ar << Parameters.SecondDensityFunctionParameter;
	Ar << Parameters.FogVolumeBoxMinParameter;
	Ar << Parameters.FogVolumeBoxMaxParameter;
	Ar << Parameters.ApproxFogColorParameter;
	return Ar;
}

/*-----------------------------------------------------------------------------
FFogVolumeDensitySceneInfo
-----------------------------------------------------------------------------*/
FFogVolumeDensitySceneInfo::FFogVolumeDensitySceneInfo(const UFogVolumeDensityComponent* InComponent, const FBox &InVolumeBounds) : 
	Component(InComponent),
	VolumeBounds(InVolumeBounds)
{
	if (InComponent != NULL)
	{
		ApproxFogColor = InComponent->ApproxFogLightColor;
	}
}

/*-----------------------------------------------------------------------------
FFogVolumeConstantDensitySceneInfo
-----------------------------------------------------------------------------*/
FFogVolumeConstantDensitySceneInfo::FFogVolumeConstantDensitySceneInfo(const UFogVolumeConstantDensityComponent* InComponent, const FBox &InVolumeBounds):
	FFogVolumeDensitySceneInfo(InComponent, InVolumeBounds),
	Density(InComponent->Density)
{}

UBOOL FFogVolumeConstantDensitySceneInfo::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId)
{
	return TFogIntegralDrawingPolicyFactory<FConstantDensityPolicy>::DrawDynamicMesh(Context, View, Mesh, bBackFace, bPreFog, PrimitiveSceneInfo, HitProxyId, this);
}

FVector4 FFogVolumeConstantDensitySceneInfo::GetFirstDensityFunctionParameters() const
{
	return FVector4(Density, 0.0f, 0.0f, 0.0f);
}

FLOAT FFogVolumeConstantDensitySceneInfo::GetMaxIntegral() const
{
	return Density * 1000000.0f;
}

/*-----------------------------------------------------------------------------
FFogVolumeLinearHalfspaceDensitySceneInfo
-----------------------------------------------------------------------------*/
FFogVolumeLinearHalfspaceDensitySceneInfo::FFogVolumeLinearHalfspaceDensitySceneInfo(const UFogVolumeLinearHalfspaceDensityComponent* InComponent, const FBox &InVolumeBounds):
	FFogVolumeDensitySceneInfo(InComponent, InVolumeBounds),
	PlaneDistanceFactor(InComponent->PlaneDistanceFactor),
	HalfspacePlane(InComponent->HalfspacePlane)
{}

UBOOL FFogVolumeLinearHalfspaceDensitySceneInfo::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId)
{
	return TFogIntegralDrawingPolicyFactory<FLinearHalfspaceDensityPolicy>::DrawDynamicMesh(Context, View, Mesh, bBackFace, bPreFog, PrimitiveSceneInfo, HitProxyId, this);
}

FVector4 FFogVolumeLinearHalfspaceDensitySceneInfo::GetFirstDensityFunctionParameters() const
{
	return FVector4(PlaneDistanceFactor, 0.0f, 0.0f, 0.0f);
}

FVector4 FFogVolumeLinearHalfspaceDensitySceneInfo::GetSecondDensityFunctionParameters() const
{
	return FVector4(HalfspacePlane.X, HalfspacePlane.Y, HalfspacePlane.Z, HalfspacePlane.W);
}

FLOAT FFogVolumeLinearHalfspaceDensitySceneInfo::GetMaxIntegral() const
{
	return PlaneDistanceFactor * 100000.0f;
}

/*-----------------------------------------------------------------------------
FFogVolumeSphericalDensitySceneInfo
-----------------------------------------------------------------------------*/
FFogVolumeSphericalDensitySceneInfo::FFogVolumeSphericalDensitySceneInfo(const UFogVolumeSphericalDensityComponent* InComponent, const FBox &InVolumeBounds):
	FFogVolumeDensitySceneInfo(InComponent, InVolumeBounds),
	MaxDensity(InComponent->MaxDensity),
	Sphere(FSphere(InComponent->SphereCenter, InComponent->SphereRadius))
{}

UBOOL FFogVolumeSphericalDensitySceneInfo::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId)
{
	return TFogIntegralDrawingPolicyFactory<FSphereDensityPolicy>::DrawDynamicMesh(Context, View, Mesh, bBackFace, bPreFog, PrimitiveSceneInfo, HitProxyId, this);
}

FVector4 FFogVolumeSphericalDensitySceneInfo::GetFirstDensityFunctionParameters() const
{
	return FVector4(MaxDensity, 0.0f, 0.0f, 0.0f);
}

FVector4 FFogVolumeSphericalDensitySceneInfo::GetSecondDensityFunctionParameters() const
{
	return FVector4(Sphere.X, Sphere.Y, Sphere.Z, Sphere.W);
}

FLOAT FFogVolumeSphericalDensitySceneInfo::GetMaxIntegral() const
{
	return MaxDensity * 250000.0f;
}

/*-----------------------------------------------------------------------------
FFogVolumeConeDensitySceneInfo
-----------------------------------------------------------------------------*/
FFogVolumeConeDensitySceneInfo::FFogVolumeConeDensitySceneInfo(const UFogVolumeConeDensityComponent* InComponent, const FBox &InVolumeBounds):
	FFogVolumeDensitySceneInfo(InComponent, InVolumeBounds),
	MaxDensity(InComponent->MaxDensity),
	ConeVertex(InComponent->ConeVertex),
	ConeRadius(InComponent->ConeRadius),
	ConeAxis(InComponent->ConeAxis),
	ConeMaxAngle(InComponent->ConeMaxAngle)
{}

UBOOL FFogVolumeConeDensitySceneInfo::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId)
{
	return TFogIntegralDrawingPolicyFactory<FConeDensityPolicy>::DrawDynamicMesh(Context, View, Mesh, bBackFace, bPreFog, PrimitiveSceneInfo, HitProxyId, this);
}

FVector4 FFogVolumeConeDensitySceneInfo::GetFirstDensityFunctionParameters() const
{
	return FVector4(ConeVertex, MaxDensity);
}

FVector4 FFogVolumeConeDensitySceneInfo::GetSecondDensityFunctionParameters() const
{
	return FVector4(ConeAxis, ConeRadius);
}

FLOAT FFogVolumeConeDensitySceneInfo::GetMaxIntegral() const
{
	return MaxDensity * 250000.0f;
}


IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralVertexShader<FConstantDensityPolicy>,TEXT("FogIntegralVertexShader"),TEXT("Main"),SF_Vertex,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralPixelShader<FConstantDensityPolicy>,TEXT("FogIntegralPixelShader"),TEXT("ConstantDensityMain"),SF_Pixel,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralVertexShader<FLinearHalfspaceDensityPolicy>,TEXT("FogIntegralVertexShader"),TEXT("Main"),SF_Vertex,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralPixelShader<FLinearHalfspaceDensityPolicy>,TEXT("FogIntegralPixelShader"),TEXT("LinearHalfspaceDensityMain"),SF_Pixel,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralVertexShader<FSphereDensityPolicy>,TEXT("FogIntegralVertexShader"),TEXT("Main"),SF_Vertex,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralPixelShader<FSphereDensityPolicy>,TEXT("FogIntegralPixelShader"),TEXT("SphericalDensityMain"),SF_Pixel,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralVertexShader<FConeDensityPolicy>,TEXT("FogIntegralVertexShader"),TEXT("Main"),SF_Vertex,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TFogIntegralPixelShader<FConeDensityPolicy>,TEXT("FogIntegralPixelShader"),TEXT("ConeDensityMain"),SF_Pixel,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);

IMPLEMENT_MATERIAL_SHADER_TYPE(,FFogVolumeApplyVertexShader,TEXT("FogVolumeApplyVertexShader"),TEXT("Main"),SF_Vertex,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(,FFogVolumeApplyPixelShader,TEXT("FogVolumeApplyPixelShader"),TEXT("Main"),SF_Pixel,VER_TRANSLUCENCY_IN_FOG_VOLUMES,0);


template<class DensityFunctionPolicy>
TFogIntegralDrawingPolicy<DensityFunctionPolicy>::TFogIntegralDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy)
	:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy)
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
	// get cached shaders
	VertexShader = MeshShaderIndex->GetShader<TFogIntegralVertexShader<DensityFunctionPolicy> >();
	PixelShader = MeshShaderIndex->GetShader<TFogIntegralPixelShader<DensityFunctionPolicy> >();
}

/**
* Match two draw policies
* @param Other - draw policy to compare
* @return TRUE if the draw policies are a match
*/
template<class DensityFunctionPolicy>
UBOOL TFogIntegralDrawingPolicy<DensityFunctionPolicy>::Matches(
	const TFogIntegralDrawingPolicy<DensityFunctionPolicy>& Other
	) const
{
	return FMeshDrawingPolicy::Matches(Other) &&
		VertexShader == Other.VertexShader && 
		PixelShader == Other.PixelShader;
}

/**
* Executes the draw commands which can be shared between any meshes using this drawer.
* @param CI - The command interface to execute the draw commands on.
* @param View - The view of the scene being drawn.
*/
template<class DensityFunctionPolicy>
void TFogIntegralDrawingPolicy<DensityFunctionPolicy>::DrawShared(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	FBoundShaderStateRHIParamRef BoundShaderState,
	const FFogVolumeDensitySceneInfo* DensitySceneInfo,
	UBOOL bBackFace
	) const
{
	// Set the translucent shader parameters for the material instance
	VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,&View);
	PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View, DensitySceneInfo, bBackFace);

	// Set shared mesh resources
	FMeshDrawingPolicy::DrawShared(Context,&View);

	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState(Context, BoundShaderState);
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @param DynamicStride - optional stride for dynamic vertex data
* @return new bound shader state object
*/
template<class DensityFunctionPolicy>
FBoundShaderStateRHIRef TFogIntegralDrawingPolicy<DensityFunctionPolicy>::CreateBoundShaderState(DWORD DynamicStride)
{
	FVertexDeclarationRHIParamRef VertexDeclaration;
	DWORD StreamStrides[MaxVertexElementCount];

	FMeshDrawingPolicy::GetVertexDeclarationInfo(VertexDeclaration, StreamStrides);
	if (DynamicStride)
	{
		StreamStrides[0] = DynamicStride;
	}

	return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, VertexShader->GetVertexShader(), PixelShader->GetPixelShader());
}

static UBOOL XOR(UBOOL A,UBOOL B)
{
	return (A && !B) || (!A && B);
}

/**
* Sets the render states for drawing a mesh.
* @param Context - command context
* @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
* @param Mesh - mesh element with data needed for rendering
* @param ElementData - context specific data for mesh rendering
*/
template<class DensityFunctionPolicy>
void TFogIntegralDrawingPolicy<DensityFunctionPolicy>::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const ElementDataType& ElementData
	) const
{
	// Set transforms
	VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
	PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);

	// Set rasterizer state.
	const FRasterizerStateInitializerRHI Initializer = {
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		IsTwoSided() ? CM_None : (XOR(bBackFace, Mesh.ReverseCulling||GInvertCullMode) ? CM_CCW : CM_CW),
		Mesh.DepthBias,
		Mesh.SlopeScaleDepthBias
	};
	RHISetRasterizerStateImmediate(Context, Initializer);
}


FFogVolumeApplyDrawingPolicy::FFogVolumeApplyDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy)
	:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy)
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
	// get cached shaders
	VertexShader = MeshShaderIndex->GetShader<FFogVolumeApplyVertexShader>();
	PixelShader = MeshShaderIndex->GetShader<FFogVolumeApplyPixelShader>();
}

/**
* Match two draw policies
* @param Other - draw policy to compare
* @return TRUE if the draw policies are a match
*/
UBOOL FFogVolumeApplyDrawingPolicy::Matches(
	const FFogVolumeApplyDrawingPolicy& Other
	) const
{
	return FMeshDrawingPolicy::Matches(Other) &&
		VertexShader == Other.VertexShader && 
		PixelShader == Other.PixelShader;
}

/**
* Executes the draw commands which can be shared between any meshes using this drawer.
* @param CI - The command interface to execute the draw commands on.
* @param View - The view of the scene being drawn.
*/
void FFogVolumeApplyDrawingPolicy::DrawShared(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	FBoundShaderStateRHIParamRef BoundShaderState, 
	const FFogVolumeDensitySceneInfo* DensitySceneInfo
	) const
{
	// Set the translucent shader parameters for the material instance
	VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,&View);
	PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View, DensitySceneInfo);

	// Set shared mesh resources
	FMeshDrawingPolicy::DrawShared(Context,&View);

	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState(Context, BoundShaderState);
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @param DynamicStride - optional stride for dynamic vertex data
* @return new bound shader state object
*/
FBoundShaderStateRHIRef FFogVolumeApplyDrawingPolicy::CreateBoundShaderState(DWORD DynamicStride)
{
	FVertexDeclarationRHIParamRef VertexDeclaration;
	DWORD StreamStrides[MaxVertexElementCount];

	FMeshDrawingPolicy::GetVertexDeclarationInfo(VertexDeclaration, StreamStrides);
	if (DynamicStride)
	{
		StreamStrides[0] = DynamicStride;
	}

	return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, VertexShader->GetVertexShader(), PixelShader->GetPixelShader());
}

/**
* Sets the render states for drawing a mesh.
* @param Context - command context
* @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
* @param Mesh - mesh element with data needed for rendering
* @param ElementData - context specific data for mesh rendering
*/
void FFogVolumeApplyDrawingPolicy::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const ElementDataType& ElementData
	) const
{
	// Set transforms
	VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
	PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);

	// Set rasterizer state.
	const FRasterizerStateInitializerRHI Initializer = {
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		IsTwoSided() ? CM_None : (XOR(bBackFace, Mesh.ReverseCulling||GInvertCullMode) ? CM_CCW : CM_CW),
		Mesh.DepthBias,
		Mesh.SlopeScaleDepthBias
	};
	RHISetRasterizerStateImmediate(Context, Initializer);
}


/**
* Render a dynamic mesh using a distortion mesh draw policy 
* @return TRUE if the mesh rendered
*/
UBOOL FFogVolumeApplyDrawingPolicyFactory::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo& View,
	ContextType DrawingContext,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId, 
	const FFogVolumeDensitySceneInfo* DensitySceneInfo
	)
{
	// draw dynamic mesh element using distortion mesh policy
	FFogVolumeApplyDrawingPolicy DrawingPolicy(
		Mesh.VertexFactory,
		Mesh.MaterialRenderProxy
		);
	DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()), DensitySceneInfo);
	DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FFogVolumeApplyDrawingPolicy::ElementDataType());
	DrawingPolicy.DrawMesh(Context,Mesh);

	return TRUE;
}

/**
Fog Overview:
	Fog contribution is approximated as SrcColor * f + (1-f) * FogVolumeColor, where SrcColor is whatever color is transmitted 
	through the fog volume, FogVolumeColor is the color that the fog volume adds to the scene, and f is the result of the transmittance function.
	So f = exp(-(F(x,y,z) over R)), where F(x,y,z) is the line integral of the density function, 
	and R is the ray from the camera to the first opaque object it intersects for that pixel, clipped to the fog volume.

The goals of this fog volume implementation are:
	-support multiple density functions (not just constant density) that can be evaluated efficiently
	-handle concave objects
	-handle any camera-fogVolume-other object interaction, which basically breaks down into
		-opaque objects intersecting the fog volume
		-the camera inside the fog volume
		-transparent objects intersecting the fog volume

Algorithm:

	clear stencil to 0 if there are any fog volumes
	//downsample depth?
	sort fog volumes with transparent objects and render back to front
	disable depth tests and writes
	for each fog volume
	{
		Integral Accumulation Pass
		begin rendering integral accumulation, the buffer can be downsampled to minimize pixel shader computations
		clear the integral accumulation buffer to 0 on current fog volume pixels
		add in F(backface) (which is the line integral of the density function evaluated from the backface depth to the camera), 
			but use min(backfaceDepth, closestOpaqueOccluderdepth) for depth
		same for frontfaces, but subtract 
		
		Fog Apply Pass
		begin rendering scene color
		need to make sure each pixel is only touched once here
		static int id = 1;
		set stencil to fail on id, set stencil to id, clear stencil if id == 256 and set id = 0
		render fog volume backfaces, lookup integral accumulation 
		{
			//clip() if integral is ~0?
			calculate f = exp(-integral accumulation)
			alpha blend SrcColor * f + (1-f) FogVolumeColor
		}
		
		disable stencil mask
	}
	restore state

Limitations of this implementation:
	-fog volume geometry
		-must be a closed mesh, so any ray cast through it will hit the same number of frontfaces as backfaces.
	-the density function must be chosen so that:
		-we can evaluate the line integral through the fog volume analytically (can't approximate with lots of samples)
		-we can evaluate the integral with only the depth information of the current face being rendered, and not any other faces of the mesh
		-need to evaluate the integral from current face to camera position.  This way when the camera is inside the fog volume 
		and a frontface is clipped by the nearplane the integral will still be correct.
	-fog on translucency intersecting the fog volumes is approximated per-vertex
		-sorting artifacts
		-clipping by the density function is handled, but clipping by the fog volume mesh is not.  Instead the ray is clipped to the AABB of the fog volume mesh.
		-max translucency nesting of 4 height fogs and 1 fog volume
		-fog volumes are affected by the height fog per-vertex, and there are artifacts since only the backface vertices are fogged.
Todo:
		-fixed point blending implementations (sm2 and xenon) 
			-visible banding, need some dithering to hide it
			-'stripe' artifacts when using linear filtering on the packed integral during the upsample
			-NAN's or some other artifact around the sphere and in the plane of the linear halfspace
			-only support a small number of frontface or backface overlaps (currently 4)
		-cone density function is not yet implemented
		-ATI x1x00 cards only blend fp16 but don't filter, which is not handled

References:

http://citeseer.ist.psu.edu/cachedpage/525331/2 - couldn't find the original paper, but the cached version is readable.  
		This is by far the most valuable resource I found on the subject.

http://www.cescg.org/CESCG-2004/web/Zdrojewska-Dorota/ - good overall explanation, but skips some of the most critical parts

http://http.download.nvidia.com/developer/SDK/Individual_Samples/3dgraphics_samples.html - the Fog Polygon Volumes sample.  
		Contains tons of useful information on constant density fog volumes, encoding depths, handling opaque intersections, etc.
*/

/**
* This index is used to only render each fog volume pixel once and minimize stencil clears during the apply pass.  
* It is set to 0 before translucency and fog volumes are rendered every frame, and wrapped when it reaches 256.
*/
UINT FogApplyStencilIndex = 0;

/**
* Resets the apply stencil index.  This is called each time translucency is rendered.
*/
void ResetFogVolumeIndex()
{
	FogApplyStencilIndex = 0;
}

/**
* Render a Fog Volume.  The density function to use is found in PrimitiveSceneInfo->Scene's FogVolumes map.
* @return TRUE if the mesh rendered
*
*/
UBOOL RenderFogVolume(
	  FCommandContextRHI* Context,
	  const FViewInfo* View,
	  const FMeshElement& Mesh,
	  UBOOL bBackFace,
	  UBOOL bPreFog,
	  const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	  FHitProxyId HitProxyId)
{
	UBOOL bDirty = FALSE;

	//find the density function information associated with this primitive
	FFogVolumeDensitySceneInfo** FogDensityInfoRef = PrimitiveSceneInfo->Scene->FogVolumes.Find(PrimitiveSceneInfo->Component);
	if (FogDensityInfoRef && View->Family->ShowFlags & SHOW_Fog)
	{
		FFogVolumeDensitySceneInfo* FogDensityInfo = *FogDensityInfoRef;
		check(FogDensityInfo);

		//calculate the dimensions of the integral accumulation buffers based on the fog downsample factor
		const UINT FogAccumulationDownsampleFactor = GSceneRenderTargets.GetFogAccumulationDownsampleFactor();
		const UINT FogAccumulationBufferX = View->RenderTargetX / FogAccumulationDownsampleFactor;
		const UINT FogAccumulationBufferY = View->RenderTargetY / FogAccumulationDownsampleFactor;
		const UINT FogAccumulationBufferSizeX = View->RenderTargetSizeX / FogAccumulationDownsampleFactor;
		const UINT FogAccumulationBufferSizeY = View->RenderTargetSizeY / FogAccumulationDownsampleFactor;

		//on sm3 and PS3 this will accumulate the integral for both faces
		//on sm2 and Xenon this will accumulate the integral for back faces only
		GSceneRenderTargets.BeginRenderingFogBackfacesIntegralAccumulation();
		RHISetViewport(Context,FogAccumulationBufferX,FogAccumulationBufferY,0.0f,FogAccumulationBufferX + FogAccumulationBufferSizeX,FogAccumulationBufferY + FogAccumulationBufferSizeY,1.0f);

		//clear all channels to 0 so we start with no integral
		RHIClear( Context, TRUE, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), FALSE, 0, FALSE, 0 );

		//no depth writes, no depth tests
		RHISetDepthState( Context, TStaticDepthState<FALSE,CF_Always>::GetRHI() );

		//additive blending for all 4 channels
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());

#if !XBOX
		//try to save some bandwidth with FP blending implementation, since we are only using the red channel
		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			RHISetColorWriteMask(Context, CW_RED);
		}
#endif

		//render fog volume backfaces, calculating the integral from the camera to the backface (or an intersecting opaque object)
		//and adding this to the accumulation buffer.
		bDirty |= FogDensityInfo->DrawDynamicMesh(
			Context,
			*View,
			Mesh,
			TRUE,
			bPreFog,
			PrimitiveSceneInfo,
			HitProxyId);

#if !XBOX
		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			//render fog volume frontfaces, calculating the integral from the camera to the frontface (or an intersecting opaque object) 
			//and subtracting this from the accumulation buffer.
			bDirty |= FogDensityInfo->DrawDynamicMesh(
				Context,
				*View,
				Mesh,
				FALSE,
				bPreFog,
				PrimitiveSceneInfo,
				HitProxyId);

			GSceneRenderTargets.FinishRenderingFogBackfacesIntegralAccumulation();

		}
		else
#endif
		{
			GSceneRenderTargets.FinishRenderingFogBackfacesIntegralAccumulation();

			GSceneRenderTargets.BeginRenderingFogFrontfacesIntegralAccumulation();
			//clear all channels to 0 so we start with no integral
			RHIClear( Context, TRUE, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), FALSE, 0, FALSE, 0 );

			//render fog volume frontfaces, calculating the integral from the camera to the frontface (or an intersecting opaque object) 
			//and adding this to the accumulation buffer for frontfaces.
			bDirty |= FogDensityInfo->DrawDynamicMesh(
				Context,
				*View,
				Mesh,
				FALSE,
				bPreFog,
				PrimitiveSceneInfo,
				HitProxyId);

			GSceneRenderTargets.FinishRenderingFogFrontfacesIntegralAccumulation();
		}

		//restore render targets assumed by transparency even if nothing was rendered in the accumulation passes
		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			GSceneRenderTargets.BeginRenderingSceneColor(); 

			// Alpha blend color = FogColor * (1 - FogFactor) + DestColor * FogFactor, preserve dest alpha since it stores depth on some platforms
			RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_InverseSourceAlpha,BF_SourceAlpha,BO_Add,BF_Zero,BF_One>::GetRHI());
		}
		else
		{
			GSceneRenderTargets.BeginRenderingSceneColorLDR();

			// Alpha blend color = FogColor * (1 - FogFactor) + DestColor * FogFactor
			// Alpha channel = DestAlpha * FogFactor - this is used to weight in scene color during the LDR translucency combine
			RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_InverseSourceAlpha,BF_SourceAlpha,BO_Add,BF_Zero,BF_SourceAlpha>::GetRHI());
		}

		RHISetViewport(Context,View->RenderTargetX,View->RenderTargetY,0.0f,View->RenderTargetX + View->RenderTargetSizeX,View->RenderTargetY + View->RenderTargetSizeY,1.0f);

		RHISetColorWriteMask(Context, CW_RED | CW_GREEN | CW_BLUE | CW_ALPHA);

		if (bDirty)
		{
			//we need to only apply the fog to each pixel within the fog volume ONCE
			//this is done with the stencil buffer
			//clear stencil for the first fog volume each frame and wrap the index around if necessary
			if (FogApplyStencilIndex == 0 || FogApplyStencilIndex > 254)
			{
				FogApplyStencilIndex = 0;
				RHIClear( Context, FALSE, FLinearColor::Black, FALSE, 0, TRUE, 0 );
			}

			FogApplyStencilIndex++;

			//accept if != stencil index, set to stencil index
			//this way each pixel is touched just once and a clear is not required after every fog volume
			FStencilStateInitializerRHI FogApplyStencilInitializer(
				TRUE,CF_NotEqual,SO_Keep,SO_Keep,SO_Replace,
				FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
				0xff,0xff,FogApplyStencilIndex);

			RHISetStencilState(Context,RHICreateStencilState(FogApplyStencilInitializer));
			
			//render fog volume backfaces so every fog volume pixel will be rendered even when the camera is inside
			//rendering both faces here will allow texturing on frontfaces but will add overdraw
			//many GPU's do not do early stencil reject when writing to stencil
			FFogVolumeApplyDrawingPolicyFactory::DrawDynamicMesh(
				Context,
				*View,
				FFogVolumeApplyDrawingPolicyFactory::ContextType(),
				Mesh,
				TRUE,
				bPreFog,
				PrimitiveSceneInfo,
				HitProxyId,
				FogDensityInfo);

			GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
		}

		//restore state assumed by translucency
		RHISetStencilState(Context,TStaticStencilState<>::GetRHI());
		RHISetDepthState( Context, TStaticDepthState<FALSE,CF_LessEqual>::GetRHI() );
	}

	return bDirty;
}

