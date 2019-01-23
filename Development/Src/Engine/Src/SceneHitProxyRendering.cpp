/*=============================================================================
	SceneHitProxyRendering.cpp: Scene hit proxy rendering.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/**
 * A vertex shader for rendering the depth of a mesh.
 */
class FHitProxyVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(FHitProxyVertexShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//only compile the hit proxy vertex shader on PC
		return (Platform == SP_PCD3D_SM3 || Platform == SP_PCD3D_SM2) && 
			//and only compile for default materials, or materials that are masked, 
			(Material->IsSpecialEngineMaterial() || Material->IsMasked() || 
			//or opaque two-sided materials, since these cannot be represented by the default material
			(Material->IsTwoSided() && !IsTranslucentBlendMode(Material->GetBlendMode())));
	}

	FHitProxyVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer),
		VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
	}
	FHitProxyVertexShader() {}
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << VertexFactoryParameters;
	}
	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);
	}
	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}
private:
	FVertexFactoryParameterRef VertexFactoryParameters;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyVertexShader,TEXT("HitProxyVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/**
 * A pixel shader for rendering the depth of a mesh.
 */
class FHitProxyPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FHitProxyPixelShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//only compile the hit proxy vertex shader on PC
		return (Platform == SP_PCD3D_SM3 || Platform == SP_PCD3D_SM2) 
			//and only compile for default materials, or materials that are masked, 
			&& (Material->IsSpecialEngineMaterial() || Material->IsMasked() 
			//or opaque two-sided materials, since these cannot be represented by the default material
			|| (Material->IsTwoSided() && !IsTranslucentBlendMode(Material->GetBlendMode())));
	}

	FHitProxyPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		HitProxyIdParameter.Bind(Initializer.ParameterMap,TEXT("HitProxyId"));
	}

	FHitProxyPixelShader() {}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View->Family->CurrentWorldTime, View->Family->CurrentRealTime, View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMaterialRenderProxy* MaterialRenderProxy,const FMatrix& LocalToWorld,UBOOL bBackFace)
	{
		MaterialParameters.SetLocalTransforms(Context,this,MaterialRenderProxy,LocalToWorld,bBackFace);
	}

	void SetHitProxyId(FCommandContextRHI* Context,FHitProxyId HitProxyId)
	{
		SetPixelShaderValue(Context,GetPixelShader(),HitProxyIdParameter,HitProxyId.GetColor().ReinterpretAsLinear());
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << MaterialParameters;
		Ar << HitProxyIdParameter;
	}

private:
	FMaterialPixelShaderParameters MaterialParameters;
	FShaderParameter HitProxyIdParameter;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyPixelShader,TEXT("HitProxyPixelShader"),TEXT("Main"),SF_Pixel,0,0);

FHitProxyDrawingPolicy::FHitProxyDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy
	):
	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy)
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
	VertexShader = MeshShaderIndex->GetShader<FHitProxyVertexShader>();
	PixelShader = MeshShaderIndex->GetShader<FHitProxyPixelShader>();
}

void FHitProxyDrawingPolicy::DrawShared(FCommandContextRHI* Context,const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
{
	// Set the depth-only shader parameters for the material.
	VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);
	PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);

	// Set the shared mesh resources.
	FMeshDrawingPolicy::DrawShared(Context,View);

	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState(Context, BoundShaderState);
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @param DynamicStride - optional stride for dynamic vertex data
* @return new bound shader state object
*/
FBoundShaderStateRHIRef FHitProxyDrawingPolicy::CreateBoundShaderState(DWORD DynamicStride)
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

void FHitProxyDrawingPolicy::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const FHitProxyId HitProxyId
	) const
{
	VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
	PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);
	PixelShader->SetHitProxyId(Context, HitProxyId);	
	FMeshDrawingPolicy::SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FMeshDrawingPolicy::ElementDataType());
}

void FHitProxyDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType)
{
	checkSlow( Scene->RequiresHitProxies() );

	// Add the static mesh to the DPG's hit proxy draw list.
	const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh->MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();
	if ( !Material->IsMasked() && !Material->IsTwoSided() || IsTranslucentBlendMode(Material->GetBlendMode()))
	{
		// Default material doesn't handle masked, and doesn't have the correct bIsTwoSided setting.
		MaterialRenderProxy = GEngine->DefaultMaterial->GetRenderProxy(FALSE);
	}
	Scene->DPGs[StaticMesh->DepthPriorityGroup].HitProxyDrawList.AddMesh(
		StaticMesh,
		StaticMesh->HitProxyId,
		FHitProxyDrawingPolicy(StaticMesh->VertexFactory,MaterialRenderProxy)
		);
}

UBOOL FHitProxyDrawingPolicyFactory::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FSceneView* View,
	ContextType DrawingContext,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();
	if ( !Material->IsMasked() && !Material->IsTwoSided() || IsTranslucentBlendMode(Material->GetBlendMode()))
	{
		// Default material doesn't handle masked, and doesn't have the correct bIsTwoSided setting.
		MaterialRenderProxy = GEngine->DefaultMaterial->GetRenderProxy(FALSE);
	}
	FHitProxyDrawingPolicy DrawingPolicy( Mesh.VertexFactory, MaterialRenderProxy );
	DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
	DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,HitProxyId);
	DrawingPolicy.DrawMesh(Context,Mesh);
	return TRUE;
}

void FSceneRenderer::RenderHitProxies()
{
#if !CONSOLE
	// Set the view family's destination render target as the render target.
	RHISetRenderTarget(GlobalContext,ViewFamily.RenderTarget->GetRenderTargetSurface(),GSceneRenderTargets.GetSceneDepthSurface());

	// Clear color and depth buffer for each view.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		RHISetViewport(GlobalContext,View.X,View.Y,0.0f,View.X + View.RenderTargetSizeX,View.Y + View.RenderTargetSizeY,1.0f);
		RHIClear(GlobalContext,TRUE,FLinearColor::White,FALSE,1,FALSE,0);
	}

	// Find the visible primitives.
	InitViews();

	// Draw the hit proxy IDs for all visible primitives.
	for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
	{
		// Depth tests + writes, no alpha blending.
		RHISetDepthState(GlobalContext,TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());
		RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views(ViewIndex);

			// Set the device viewport for the view.
			RHISetViewport(GlobalContext,View.X,View.Y,0.0f,View.X + View.RenderTargetSizeX,View.Y + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// Clear the depth buffer for each DPG.
			RHIClear(GlobalContext,FALSE,FLinearColor::Black,TRUE,1.0f,TRUE,0);

			// Draw the scene's hit proxy draw lists.
			Scene->DPGs[DPGIndex].HitProxyDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);

			// Draw all dynamic primitives using the hit proxy drawing policy.
			DrawDynamicPrimitiveSet<FHitProxyDrawingPolicyFactory>(
				GlobalContext,
				&View,
				FHitProxyDrawingPolicyFactory::ContextType(),
				DPGIndex,
				TRUE,
				TRUE
				);

			// Draw the view's batched simple elements(lines, sprites, etc).
			View.BatchedViewElements[DPGIndex].Draw(GlobalContext,View.ViewProjectionMatrix,View.SizeX,View.SizeY,TRUE);
		}
	}

	// After scene rendering, disable the depth buffer.
	RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());
#endif
}
