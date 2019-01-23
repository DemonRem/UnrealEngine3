/*=============================================================================
	DepthRendering.h: Depth rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

template<UBOOL>
class TDepthOnlyVertexShader;

/**
 * Outputs no color, but can be used to write the mesh's depth values to the depth buffer.
 * Only supports opaque materials.
 */
class FDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:

	FDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy
		);

	// FMeshDrawingPolicy interface.
	UBOOL Matches(const FDepthDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) && VertexShader == Other.VertexShader;
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

	friend INT Compare(const FDepthDrawingPolicy& A,const FDepthDrawingPolicy& B);

private:
	 TDepthOnlyVertexShader<FALSE> * VertexShader;
};

/**
 * Writes out depth for opaque materials on meshes which support a position-only vertex buffer.
 * Using the position-only vertex buffer saves vertex fetch bandwidth during the z prepass.
 */
class FPositionOnlyDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:

	FPositionOnlyDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy
		);

	// FMeshDrawingPolicy interface.
	UBOOL Matches(const FPositionOnlyDepthDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) && VertexShader == Other.VertexShader;
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

	friend INT Compare(const FPositionOnlyDepthDrawingPolicy& A,const FPositionOnlyDepthDrawingPolicy& B);

private:
	TDepthOnlyVertexShader<TRUE> * VertexShader;
};

/**
 * A drawing policy factory for the depth drawing policy.
 */
class FDepthDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = FALSE };
	struct ContextType {};

	static void AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType DrawingContext = ContextType());
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FSceneView* View,
		ContextType DrawingContext,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);
	static UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy);
};
