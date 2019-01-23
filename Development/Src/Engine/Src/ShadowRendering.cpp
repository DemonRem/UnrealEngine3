/*=============================================================================
	ShadowRendering.cpp: Shadow rendering implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "UnTextureLayout.h"

// Globals
const UINT SHADOW_BORDER=5; 

/**
 * Returns the material which should be used to draw the shadow depth for the given material.
 * If the material is masked, it returns the given material; otherwise it returns GEngine->DefaultMaterial.
 */
static const FMaterialRenderProxy* GetShadowDepthMaterial(const FMaterialRenderProxy* MaterialRenderProxy)
{
	return MaterialRenderProxy->GetMaterial()->IsMasked() ?
			MaterialRenderProxy :
			GEngine->DefaultMaterial->GetRenderProxy(FALSE);
}

/**
 * A vertex shader for rendering the depth of a mesh.
 */
class FShadowDepthVertexShader : public FShader
{
	DECLARE_SHADER_TYPE(FShadowDepthVertexShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile the shadow depth shaders for the default material, and masked materials.
		return	Material->IsMasked() || Material->IsSpecialEngineMaterial();
	}

	FShadowDepthVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer),
		VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
		ShadowMatrixParameter.Bind(Initializer.ParameterMap,TEXT("ShadowMatrix"));
		ProjectionMatrixParameter.Bind(Initializer.ParameterMap,TEXT("ProjectionMatrix"));
	}
	FShadowDepthVertexShader() {}
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << VertexFactoryParameters;
		Ar << ShadowMatrixParameter;
		Ar << ProjectionMatrixParameter;
	}
	void SetParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);
		SetVertexShaderValue(Context,GetVertexShader(),ShadowMatrixParameter,ShadowInfo->SubjectAndReceiverMatrix);
		SetVertexShaderValue(Context,GetVertexShader(),ProjectionMatrixParameter,ShadowInfo->SubjectMatrix);
	}
	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}
private:
	FVertexFactoryParameterRef VertexFactoryParameters;
	FShaderParameter ShadowMatrixParameter;
	FShaderParameter ProjectionMatrixParameter;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FShadowDepthVertexShader,TEXT("ShadowDepthVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/**
 * A pixel shader for rendering the depth of a mesh.
 */
class FShadowDepthPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FShadowDepthPixelShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile the shadow depth shaders for the default material, and masked materials.
		return	Material->IsMasked() || Material->IsSpecialEngineMaterial();
	}

	FShadowDepthPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		InvMaxSubjectDepthParameter.Bind(Initializer.ParameterMap,TEXT("InvMaxSubjectDepth"), TRUE);
		DepthBiasParameter.Bind(Initializer.ParameterMap,TEXT("DepthBias"),TRUE);
	}

	FShadowDepthPixelShader() {}

	void SetParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View->Family->CurrentWorldTime, View->Family->CurrentRealTime, View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);

        SetPixelShaderValue(Context,GetPixelShader(),InvMaxSubjectDepthParameter,1.0f / ShadowInfo->MaxSubjectDepth);
		FLOAT DepthBias = GEngine->DepthBias;
#if SUPPORTS_VSM
		const UBOOL bEnableVSMShadows = ShadowInfo->LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_VSM || 
			(GEngine->bEnableVSMShadows && ShadowInfo->LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_Default);
		if (bEnableVSMShadows)
		{
			//no spatial bias for VSM shadows, they only need a statistical bias
			DepthBias = 0.0f;
		} 
		else 
#endif //#if SUPPORTS_VSM
		if (ShouldUseBranchingPCF(ShadowInfo->LightSceneInfo->ShadowProjectionTechnique))
		{
			//small tweakable to make the effect of the offset used with Branching PCF match up with the default PCF
			DepthBias += .001f;
		}
		SetPixelShaderValue(Context,GetPixelShader(),DepthBiasParameter,DepthBias);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMaterialRenderProxy* MaterialRenderProxy,const FMatrix& LocalToWorld,UBOOL bBackFace)
	{
		MaterialParameters.SetLocalTransforms(Context,this,MaterialRenderProxy,LocalToWorld,bBackFace);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << MaterialParameters;
		Ar << InvMaxSubjectDepthParameter;
		if( Ar.Ver() >= VER_SHADOW_DEPTH_SHADER_RECOMPILE )
		{
			Ar << DepthBiasParameter;
		}
	}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}

private:
	FMaterialPixelShaderParameters MaterialParameters;
	FShaderParameter InvMaxSubjectDepthParameter;
	FShaderParameter DepthBiasParameter;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FShadowDepthPixelShader,TEXT("ShadowDepthPixelShader"),TEXT("Main"),SF_Pixel,VER_SHADOW_DEPTH_SHADER_RECOMPILE,0);

/** The shadow frustum vertex declaration. */
TGlobalResource<FShadowFrustumVertexDeclaration> GShadowFrustumVertexDeclaration;

/*-----------------------------------------------------------------------------
	FShadowProjectionVertexShader
-----------------------------------------------------------------------------*/

UBOOL FShadowProjectionVertexShader::ShouldCache(EShaderPlatform Platform)
{
	return TRUE;
}

FShadowProjectionVertexShader::FShadowProjectionVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
{
}

void FShadowProjectionVertexShader::SetParameters(FCommandContextRHI* Context,
										const FSceneView* View,
										const FProjectedShadowInfo* ShadowInfo
										)
{
}

void FShadowProjectionVertexShader::Serialize(FArchive& Ar)
{
	FShader::Serialize(Ar);
}

IMPLEMENT_SHADER_TYPE(,FShadowProjectionVertexShader,TEXT("ShadowProjectionVertexShader"),TEXT("Main"),SF_Vertex,0,0);


/*-----------------------------------------------------------------------------
	FModShadowProjectionVertexShader
-----------------------------------------------------------------------------*/

/**
 * Constructor - binds all shader params
 * @param Initializer - init data from shader compiler
 */
FModShadowProjectionVertexShader::FModShadowProjectionVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
:	FShadowProjectionVertexShader(Initializer)
{		
}

/**
 * Sets the current vertex shader
 * @param Context - command buffer context
 * @param View - current view
 * @param ShadowInfo - projected shadow info for a single light
 */
void FModShadowProjectionVertexShader::SetParameters(	FCommandContextRHI* Context,
													 const FSceneView* View,
													 const FProjectedShadowInfo* ShadowInfo
													 )
{
	FShadowProjectionVertexShader::SetParameters(Context,View,ShadowInfo);
}

/**
 * Serialize the parameters for this shader
 * @param Ar - archive to serialize to
 */
void FModShadowProjectionVertexShader::Serialize(FArchive& Ar)
{
	FShadowProjectionVertexShader::Serialize(Ar);
}

IMPLEMENT_SHADER_TYPE(,FModShadowProjectionVertexShader,TEXT("ModShadowProjectionVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/**
 * Implementations for TShadowProjectionPixelShader.  
 */

//Shader Model 2 compatible version that uses Hardware PCF
IMPLEMENT_SHADER_TYPE(template<>,TShadowProjectionPixelShader<F4SampleHwPCF>,TEXT("ShadowProjectionPixelShader"),TEXT("HardwarePCFMain"),SF_Pixel,VER_IMPLEMENTED_FETCH4,0);
//Shader Model 2 compatible version
IMPLEMENT_SHADER_TYPE(template<>,TShadowProjectionPixelShader<F4SampleManualPCF>,TEXT("ShadowProjectionPixelShader"),TEXT("Main"),SF_Pixel,VER_IMPLEMENTED_FETCH4,0);
//Full version that uses Hardware PCF
IMPLEMENT_SHADER_TYPE(template<>,TShadowProjectionPixelShader<F16SampleHwPCF>,TEXT("ShadowProjectionPixelShader"),TEXT("HardwarePCFMain"),SF_Pixel,VER_IMPLEMENTED_FETCH4,0);
//Full version that uses Fetch4
IMPLEMENT_SHADER_TYPE(template<>,TShadowProjectionPixelShader<F16SampleFetch4PCF>,TEXT("ShadowProjectionPixelShader"),TEXT("Fetch4Main"),SF_Pixel,0,0);
//Full version
IMPLEMENT_SHADER_TYPE(template<>,TShadowProjectionPixelShader<F16SampleManualPCF>,TEXT("ShadowProjectionPixelShader"),TEXT("Main"),SF_Pixel,VER_IMPLEMENTED_FETCH4,0);

/**
* Get the version of TShadowProjectionPixelShader that should be used based on the hardware's capablities
* @return a pointer to the chosen shader
*/
FShadowProjectionPixelShaderInterface* GetProjPixelShaderRef(BYTE LightShadowQuality)
{
	FShadowProjectionPixelShaderInterface * PixelShader = NULL;

	//apply the system settings bias to the light's shadow quality
	BYTE EffectiveShadowFilterQuality = Max(LightShadowQuality + GSystemSettings.ShadowFilterQualityBias, 0);

	//force shader model 2 cards to the 4 sample path
	if (EffectiveShadowFilterQuality == SFQ_Low || GRHIShaderPlatform == SP_PCD3D_SM2)
	{
		if (GSupportsHardwarePCF)
		{
			TShaderMapRef<TShadowProjectionPixelShader<F4SampleHwPCF> > FourSamplePixelShader(GetGlobalShaderMap());
			PixelShader = *FourSamplePixelShader;
		}
		else
		{
			TShaderMapRef<TShadowProjectionPixelShader<F4SampleManualPCF> > FourSamplePixelShader(GetGlobalShaderMap());
			PixelShader = *FourSamplePixelShader;
		}
	}
	//todo - implement medium quality path, 9 samples?
	else
	{
		if (GSupportsHardwarePCF)
		{
			TShaderMapRef<TShadowProjectionPixelShader<F16SampleHwPCF> > SixteenSamplePixelShader(GetGlobalShaderMap());
			PixelShader = *SixteenSamplePixelShader;
		}
		else if (GSupportsFetch4)
		{
			TShaderMapRef<TShadowProjectionPixelShader<F16SampleFetch4PCF> > SixteenSamplePixelShader(GetGlobalShaderMap());
			PixelShader = *SixteenSamplePixelShader;
		}
		else
		{
			TShaderMapRef<TShadowProjectionPixelShader<F16SampleManualPCF> > SixteenSamplePixelShader(GetGlobalShaderMap());
			PixelShader = *SixteenSamplePixelShader;
		}
	}
	return PixelShader;
}

/*-----------------------------------------------------------------------------
	FShadowDepthDrawingPolicy
-----------------------------------------------------------------------------*/

FShadowDepthDrawingPolicy::FShadowDepthDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FProjectedShadowInfo* InShadowInfo
	):
	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy),
	ShadowInfo(InShadowInfo)
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
	VertexShader = MeshShaderIndex->GetShader<FShadowDepthVertexShader>();
	PixelShader = MeshShaderIndex->GetShader<FShadowDepthPixelShader>();
}

void FShadowDepthDrawingPolicy::DrawShared(FCommandContextRHI* Context,const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
{
	// Set the depth-only shader parameters for the material.
	VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View,ShadowInfo);
	PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View,ShadowInfo);

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
FBoundShaderStateRHIRef FShadowDepthDrawingPolicy::CreateBoundShaderState(DWORD DynamicStride)
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

void FShadowDepthDrawingPolicy::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const ElementDataType& ElementData
	) const
{
	VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
	PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);
	FMeshDrawingPolicy::SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,ElementData);
}

INT Compare(const FShadowDepthDrawingPolicy& A,const FShadowDepthDrawingPolicy& B)
{
	COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
	COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
	COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
	COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
	return 0;
}

UBOOL FShadowDepthDrawingPolicyFactory::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FSceneView* View,
	const FProjectedShadowInfo* ShadowInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	if ( !Mesh.CastShadow )
	{
		return FALSE;
	}
	FShadowDepthDrawingPolicy DrawingPolicy(Mesh.VertexFactory,GetShadowDepthMaterial(Mesh.MaterialRenderProxy),ShadowInfo);
	DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
	DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FMeshDrawingPolicy::ElementDataType());
	DrawingPolicy.DrawMesh(Context,Mesh);
	return TRUE;
}

/*-----------------------------------------------------------------------------
	FProjectedShadowInitializer
-----------------------------------------------------------------------------*/

void FProjectedShadowInitializer::CalcTransforms(
	const FMatrix& WorldToLight,
	const FVector& FaceDirection,
	const FSphere& SubjectBoundingSphere,
	const FVector4& WAxis,
	FLOAT MinLightW,
	FLOAT MaxLightW,
	UBOOL bInDirectionalLight
	)
{
	bDirectionalLight = bInDirectionalLight;

	struct FShadowProjectionMatrix: FMatrix
	{
		FShadowProjectionMatrix(FLOAT MinZ,FLOAT MaxZ,const FVector4& WAxis):
			FMatrix(
				FPlane(1,	0,	0,													WAxis.X),
				FPlane(0,	1,	0,													WAxis.Y),
				FPlane(0,	0,	(WAxis.Z * MaxZ + WAxis.W) / (MaxZ - MinZ),			WAxis.Z),
				FPlane(0,	0,	-MinZ * (WAxis.Z * MaxZ + WAxis.W) / (MaxZ - MinZ),	WAxis.W)
				)
		{}
	};

	FVector	XAxis,
			YAxis;
	FaceDirection.FindBestAxisVectors(XAxis,YAxis);
	FMatrix	WorldToFace = WorldToLight * FBasisVectorMatrix(-XAxis,YAxis,FaceDirection.SafeNormal(),FVector(0,0,0));

	FLOAT MaxSubjectZ = WorldToFace.TransformFVector(SubjectBoundingSphere).Z + SubjectBoundingSphere.W;
	FLOAT MinSubjectZ = Max(MaxSubjectZ - SubjectBoundingSphere.W * 2,MinLightW);

	PreSubjectMatrix = WorldToFace * FShadowProjectionMatrix(MinLightW,MaxSubjectZ,WAxis);
	SubjectMatrix = WorldToFace * FShadowProjectionMatrix(MinSubjectZ,MaxSubjectZ,WAxis);
	PostSubjectMatrix = WorldToFace * FShadowProjectionMatrix(MinSubjectZ,MaxLightW,WAxis);

	MaxSubjectDepth = SubjectMatrix.TransformFVector((FVector)SubjectBoundingSphere + WorldToLight.Inverse().TransformNormal(FaceDirection) * SubjectBoundingSphere.W).Z;
}

/*-----------------------------------------------------------------------------
	FProjectedShadowInfo
-----------------------------------------------------------------------------*/

FProjectedShadowInfo::FProjectedShadowInfo(
	const FLightSceneInfo* InLightSceneInfo,
	const FPrimitiveSceneInfo* InSubjectSceneInfo,
	const FProjectedShadowInitializer& Initializer,
	UBOOL bInPreShadow,
	UINT InResolution,
	FLOAT InFadeAlpha
	):
	LightSceneInfo(InLightSceneInfo),
	SubjectSceneInfo(InSubjectSceneInfo),
	SubjectAndReceiverMatrix(Initializer.SubjectMatrix),
	MaxSubjectDepth(Initializer.MaxSubjectDepth),
	Resolution(InResolution),
	FadeAlpha(InFadeAlpha),
	bAllocated(FALSE),
	bRendered(FALSE),
	bDirectionalLight(Initializer.bDirectionalLight),
	bPreShadow(bInPreShadow)
{
	if(bPreShadow)
	{
		SubjectMatrix = Initializer.PreSubjectMatrix;
		ReceiverMatrix = Initializer.SubjectMatrix;
	}
	else
	{
		SubjectMatrix = Initializer.SubjectMatrix;
		ReceiverMatrix = Initializer.PostSubjectMatrix;
	}

	InvReceiverMatrix = ReceiverMatrix.Inverse();
	SubjectFrustum = GetViewFrustumBounds(SubjectMatrix,TRUE);
	ReceiverFrustum = GetViewFrustumBounds(ReceiverMatrix,TRUE);
}

void FProjectedShadowInfo::RenderDepth(FCommandContextRHI* Context, const FSceneRenderer* SceneRenderer,BYTE DepthPriorityGroup)
{
	// Set the viewport for the shadow.
	RHISetViewport(
		Context,
		X,
		Y,
		0.0f,
		X + SHADOW_BORDER*2 + Resolution,
		Y + SHADOW_BORDER*2 + Resolution,
		1.0f
		);

	if( GSupportsDepthTextures || GSupportsHardwarePCF || GSupportsFetch4)
	{
		// Clear depth only.
		RHIClear(Context,FALSE,FColor(255,255,255),TRUE,1.0f,FALSE,0);
	}
	else
	{
		// Clear color and depth.
		RHIClear(Context,TRUE,FColor(255,255,255),TRUE,1.0f,FALSE,0);
	}	

	// Set the viewport for the shadow.
	RHISetViewport(
		Context,
		X + SHADOW_BORDER,
		Y + SHADOW_BORDER,
		0.0f,
		X + SHADOW_BORDER + Resolution,
		Y + SHADOW_BORDER + Resolution,
		1.0f
		);

	// Opaque blending, depth tests and writes, solid rasterization w/ back-face culling.
	RHISetBlendState(Context,TStaticBlendState<>::GetRHI());	
	RHISetDepthState(Context,TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());
	RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI());

	// Choose an arbitrary view where this shadow is in the right DPG to use for rendering the depth.
	const FViewInfo* View = NULL;
	for(INT ViewIndex = 0;ViewIndex < SceneRenderer->Views.Num();ViewIndex++)
	{
		const FViewInfo* CheckView = &SceneRenderer->Views(ViewIndex);
		FPrimitiveViewRelevance ViewRel = SubjectSceneInfo->Proxy->GetViewRelevance(CheckView);
		if (ViewRel.GetDPG(DepthPriorityGroup) == TRUE)
		{
			View = CheckView;
			break;
		}
	}
	check(View);

	// Draw the subject's static elements.
	SubjectMeshElements.DrawAll(Context,View);

	// Draw the subject's dynamic elements.
	TDynamicPrimitiveDrawer<FShadowDepthDrawingPolicyFactory> Drawer(Context,View,DepthPriorityGroup,this,TRUE);
	for(INT PrimitiveIndex = 0;PrimitiveIndex < SubjectPrimitives.Num();PrimitiveIndex++)
	{
		Drawer.SetPrimitive(SubjectPrimitives(PrimitiveIndex));
		SubjectPrimitives(PrimitiveIndex)->Proxy->DrawDynamicElements(&Drawer,View,DepthPriorityGroup);
	}
}

FBoundShaderStateRHIRef FProjectedShadowInfo::MaskBoundShaderState;
FGlobalBoundShaderStateRHIRef FProjectedShadowInfo::ShadowProjectionBoundShaderState;
#if SUPPORTS_VSM
FGlobalBoundShaderStateRHIRef FProjectedShadowInfo::ShadowProjectionVSMBoundShaderState;
#endif //#if SUPPORTS_VSM
FGlobalBoundShaderStateRHIRef FProjectedShadowInfo::BranchingPCFLowQualityBoundShaderState;
FGlobalBoundShaderStateRHIRef FProjectedShadowInfo::BranchingPCFMediumQualityBoundShaderState;
FGlobalBoundShaderStateRHIRef FProjectedShadowInfo::BranchingPCFHighQualityBoundShaderState;

#if PS3
UBOOL GOptimizeStencilClear = FALSE;
#else
UBOOL GOptimizeStencilClear = TRUE;
#endif
UBOOL GUseHiStencil = TRUE;

void FProjectedShadowInfo::RenderProjection(FCommandContextRHI* Context, const FViewInfo* View, BYTE DepthPriorityGroup) const
{
	// Compute the primitive's view relevance.  Note that the view won't necessarily have it cached,
	// since the primitive might not be visible.
	FPrimitiveViewRelevance ViewRelevance = SubjectSceneInfo->Proxy->GetViewRelevance(View);

	// Don't render shadows for subjects which aren't view relevant.
	if (ViewRelevance.bShadowRelevance == FALSE)
	{
		return;
	}

	if (ViewRelevance.GetDPG(DepthPriorityGroup) == FALSE)
	{
		return;
	}

	FVector FrustumVertices[8];
	
	for(UINT Z = 0;Z < 2;Z++)
	{
		for(UINT Y = 0;Y < 2;Y++)
		{
			for(UINT X = 0;X < 2;X++)
			{
				FVector4 UnprojectedVertex = InvReceiverMatrix.TransformFVector4(
					FVector4(
						(X ? -1.0f : 1.0f),
						(Y ? -1.0f : 1.0f),
						(Z ?  0.0f : 1.0f),
						1.0f
						)
					);
				FrustumVertices[GetCubeVertexIndex(X,Y,Z)] = UnprojectedVertex / UnprojectedVertex.W;
			}
		}
	}

	// Find the projection shaders.
	TShaderMapRef<FShadowProjectionVertexShader> VertexShader(GetGlobalShaderMap());

#if 0 // XBOX
	if( DepthPriorityGroup == SDPG_Foreground &&
		LightSceneInfo->LightShadowMode == LightShadow_Modulate )
	{
		// hack for rendering shadows from foreground items onto the world. We're ignoring hte stencil mask 
		// since the world depth values are not in the depth buffer (we only have foreground depth values).
		RHISetDepthState(Context,TStaticDepthState<FALSE,CF_Always>::GetRHI());
		RHISetColorWriteEnable(Context,TRUE);
		RHISetRasterizerState(Context,
			GInvertCullMode ? TStaticRasterizerState<FM_Solid,CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI());
	}
	else
#endif
	{

	// Depth test wo/ writes, no color writing.
	RHISetDepthState(Context,TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
	RHISetColorWriteEnable(Context,FALSE);

	if (GUseHiStencil)
	{
		RHIBeginHiStencilRecord(Context);
	}

	// If this is a preshadow, mask the projection by the receiver primitives.
	// Otherwise, mask the projection to any pixels inside the frustum.
	if(bPreShadow)
	{
		// Set stencil to one.
		RHISetStencilState(Context,TStaticStencilState<
			TRUE,CF_Always,SO_Keep,SO_Keep,SO_Replace,
			FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
			0xff,0xff,1
			>::GetRHI());

		// Draw the receiver's dynamic elements.
		TDynamicPrimitiveDrawer<FDepthDrawingPolicyFactory> Drawer(Context,View,DepthPriorityGroup,FDepthDrawingPolicyFactory::ContextType(),TRUE);
		for(INT PrimitiveIndex = 0;PrimitiveIndex < ReceiverPrimitives.Num();PrimitiveIndex++)
		{
			Drawer.SetPrimitive(ReceiverPrimitives(PrimitiveIndex));
			ReceiverPrimitives(PrimitiveIndex)->Proxy->DrawDynamicElements(&Drawer,View,DepthPriorityGroup);
		}
	}
	else
	{
		// Solid rasterization wo/ backface culling.
		RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

		// Increment stencil on front-facing zfail, decrement on back-facing zfail.
		RHISetStencilState(Context,TStaticStencilState<
			TRUE,CF_Always,SO_Keep,SO_Increment,SO_Keep,
			TRUE,CF_Always,SO_Keep,SO_Decrement,SO_Keep,
			0xff,0xff
		>::GetRHI());

		// Cache the bound shader state
		if (!IsValidRef(MaskBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			appMemzero(Strides, sizeof(Strides));
			Strides[0] = sizeof(FVector);

			MaskBoundShaderState = RHICreateBoundShaderState(GShadowFrustumVertexDeclaration.VertexDeclarationRHI, Strides, VertexShader->GetVertexShader(), FPixelShaderRHIRef());
		}

		// Set the projection vertex shader parameters
		VertexShader->SetParameters(Context,View,this);

		RHISetBoundShaderState(Context, MaskBoundShaderState);

		// Draw the frustum using the stencil buffer to mask just the pixels which are inside the shadow frustum.
		RHIDrawIndexedPrimitiveUP(Context, PT_TriangleList, 0, 8, 12, GCubeIndices, sizeof(WORD), FrustumVertices, sizeof(FVector));
	}

	// no depth test or writes, solid rasterization w/ back-face culling.
	RHISetDepthState(Context,TStaticDepthState<FALSE,CF_Always>::GetRHI());
	RHISetColorWriteEnable(Context,TRUE);
	RHISetRasterizerState(Context,
		GInvertCullMode ? TStaticRasterizerState<FM_Solid,CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI());

	if (!GOptimizeStencilClear)
	{
		// Test stencil for != 0.
		RHISetStencilState(Context,TStaticStencilState<
			TRUE,CF_NotEqual,SO_Keep,SO_Keep,SO_Keep,
			FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
			0xff,0xff,0
			>::GetRHI());
	}
	else
	{
		// Test stencil for != 0 and replace with 0
		RHISetStencilState(Context,TStaticStencilState<
			TRUE,CF_NotEqual,SO_Keep,SO_Keep,SO_Replace,
			FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
			0xff,0xff,0
			>::GetRHI());
	}

	if (GUseHiStencil)
	{
		RHIBeginHiStencilPlayback(Context);
	}

	}	//DPGIndex != SDPG_Foreground

#if SUPPORTS_VSM
	const UBOOL bEnableVSMShadows = LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_VSM || 
		(GEngine->bEnableVSMShadows && LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_Default);
#endif //#if SUPPORTS_VSM

	if( LightSceneInfo->LightShadowMode == LightShadow_Modulate )
	{
		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			// modulated blending, preserve alpha
			RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_DestColor,BF_Zero,BO_Add,BF_Zero,BF_One>::GetRHI());
		}
		else
		{
			// opaque, blending will be done in the shader
			RHISetBlendState(Context,TStaticBlendState<>::GetRHI());
		}

#if SUPPORTS_VSM
		if( bEnableVSMShadows )
		{
			// Set the VSM modulated shadow projection shaders.
			TShaderMapRef<FModShadowProjectionVertexShader> ModShadowProjVertexShader(GetGlobalShaderMap());
			FVSMModProjectionPixelShader* VSMModShadowProjPixelShader = LightSceneInfo->GetVSMModProjPixelShader();
			checkSlow(VSMModShadowProjPixelShader);
			ModShadowProjVertexShader->SetParameters(Context,View,this);
			VSMModShadowProjPixelShader->SetParameters(Context,View,this);
			RHISetBoundShaderState(Context, LightSceneInfo->GetVSMModProjBoundShaderState());
		}
		else 
#endif //#if SUPPORTS_VSM
		if(ShouldUseBranchingPCF(LightSceneInfo->ShadowProjectionTechnique))
		{

			// Set the Branching PCF modulated shadow projection shaders.
			TShaderMapRef<FModShadowProjectionVertexShader> ModShadowProjVertexShader(GetGlobalShaderMap());
			FBranchingPCFProjectionPixelShaderInterface* BranchingPCFModShadowProjPixelShader = LightSceneInfo->GetBranchingPCFModProjPixelShader();
			checkSlow(BranchingPCFModShadowProjPixelShader);
			
			ModShadowProjVertexShader->SetParameters(Context,View,this);
			BranchingPCFModShadowProjPixelShader->SetParameters(Context,View,this);

			SetGlobalBoundShaderState(Context, *LightSceneInfo->GetBranchingPCFModProjBoundShaderState(), GShadowFrustumVertexDeclaration.VertexDeclarationRHI, 
				*ModShadowProjVertexShader, BranchingPCFModShadowProjPixelShader, sizeof(FVector));
		}
		else
		{
			// Set the modulated shadow projection shaders.
			TShaderMapRef<FModShadowProjectionVertexShader> ModShadowProjVertexShader(GetGlobalShaderMap());
			
			FShadowProjectionPixelShaderInterface* ModShadowProjPixelShader = LightSceneInfo->GetModShadowProjPixelShader();
			checkSlow(ModShadowProjPixelShader);

			ModShadowProjVertexShader->SetParameters(Context,View,this);
			ModShadowProjPixelShader->SetParameters(Context,View,this);

			SetGlobalBoundShaderState(Context, *LightSceneInfo->GetModShadowProjBoundShaderState(), GShadowFrustumVertexDeclaration.VertexDeclarationRHI, 
				*ModShadowProjVertexShader, ModShadowProjPixelShader, sizeof(FVector));
		}
	}
	else
	{
		//modulated blending, shadows may overlap
		RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_DestColor,BF_Zero>::GetRHI());
	
		// Set the shadow projection vertex shader.
		VertexShader->SetParameters(Context,View,this);

#if SUPPORTS_VSM
		if( bEnableVSMShadows )
		{
			// VSM shadow projection pixel shader
			TShaderMapRef<FVSMProjectionPixelShader> PixelShaderVSM(GetGlobalShaderMap());
			PixelShaderVSM->SetParameters(Context,View,this);

			SetGlobalBoundShaderState(Context, ShadowProjectionVSMBoundShaderState, GShadowFrustumVertexDeclaration.VertexDeclarationRHI, 
				*VertexShader, *PixelShaderVSM, sizeof(FVector));
		}
		else 
#endif //#if SUPPORTS_VSM
		if (ShouldUseBranchingPCF(LightSceneInfo->ShadowProjectionTechnique))
		{
			// Branching PCF shadow projection pixel shader
			FShader* BranchingPCFPixelShader = SetBranchingPCFParameters(Context,View,this,LightSceneInfo->ShadowFilterQuality);

			FGlobalBoundShaderStateRHIRef* CurrentBPCFBoundShaderState = ChooseBPCFBoundShaderState(LightSceneInfo->ShadowFilterQuality, &BranchingPCFLowQualityBoundShaderState, 
				&BranchingPCFMediumQualityBoundShaderState, &BranchingPCFHighQualityBoundShaderState);
			
			SetGlobalBoundShaderState(Context, *CurrentBPCFBoundShaderState, GShadowFrustumVertexDeclaration.VertexDeclarationRHI, 
				*VertexShader, BranchingPCFPixelShader, sizeof(FVector));
		} 
		else
		{
			FShadowProjectionPixelShaderInterface * PixelShader = GetProjPixelShaderRef(LightSceneInfo->ShadowFilterQuality);
			PixelShader->SetParameters(Context,View,this);

			SetGlobalBoundShaderState(Context, ShadowProjectionBoundShaderState, GShadowFrustumVertexDeclaration.VertexDeclarationRHI, 
				*VertexShader, PixelShader, sizeof(FVector));
		}
	}

	// Draw the frustum using the projection shader..
	RHIDrawIndexedPrimitiveUP(Context, PT_TriangleList, 0, 8, 12, GCubeIndices, sizeof(WORD), FrustumVertices, sizeof(FVector));

	if (GUseHiStencil)
	{
		RHIEndHiStencil(Context);
	}

	// Reset the stencil state.
	RHISetStencilState(Context,TStaticStencilState<>::GetRHI());

	if (!GOptimizeStencilClear)
	{
		// Clear the stencil buffer to 0.
		RHIClear(Context,FALSE,FColor(0,0,0),FALSE,0,TRUE,0);
	}
}

void FProjectedShadowInfo::RenderFrustumWireframe(FPrimitiveDrawInterface* PDI) const
{
	// Find the ID of an arbitrary subject primitive to use to color the shadow frustum.
	INT SubjectPrimitiveId = 0;
	if(SubjectPrimitives.Num())
	{
		SubjectPrimitiveId = SubjectPrimitives(0)->Id;
	}

	// Render the wireframe for the frustum derived from ReceiverMatrix.
	DrawFrustumWireframe(
		PDI,
		ReceiverMatrix,
		FColor(FLinearColor::FGetHSV(((SubjectPrimitiveId + LightSceneInfo->Id) * 31) & 255,0,255)),
		SDPG_Foreground
		);
}

void FProjectedShadowInfo::AddSubjectPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	if(!PrimitiveSceneInfo->StaticMeshes.Num())
	{
		// Add the primitive to the subject primitive list.
		SubjectPrimitives.AddItem(PrimitiveSceneInfo);
	}
	else
	{
		// Add the primitive's static mesh elements to the draw lists.
		for(INT MeshIndex = 0;MeshIndex < PrimitiveSceneInfo->StaticMeshes.Num();MeshIndex++)
		{
			FStaticMesh* StaticMesh = &PrimitiveSceneInfo->StaticMeshes(MeshIndex);
			if ( StaticMesh->CastShadow )
			{
				// Add the static mesh to the shadow's subject draw list.
				SubjectMeshElements.AddMesh(
					StaticMesh,
					FShadowDepthDrawingPolicy::ElementDataType(),
					FShadowDepthDrawingPolicy(StaticMesh->VertexFactory,GetShadowDepthMaterial(StaticMesh->MaterialRenderProxy),this)
					);
			}
		}
	}
}

void FProjectedShadowInfo::AddReceiverPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	// Add the primitive to the receiver primitive list.
	ReceiverPrimitives.AddItem(PrimitiveSceneInfo);
}

/**
 * @return TRUE if this shadow info has any casting subject prims to render
 */
UBOOL FProjectedShadowInfo::HasSubjectPrims() const
{
	return( SubjectPrimitives.Num() > 0 || SubjectMeshElements.NumMeshes() > 0 );
}

/**
 * Adds current subject primitives to out array.
 *
 * @param OutSubjectPrimitives [out]	Array to add current subject primitives to.
 */
void FProjectedShadowInfo::GetSubjectPrimitives( TArray<const FPrimitiveSceneInfo*>& OutSubjectPrimitives )
{
	OutSubjectPrimitives += SubjectPrimitives;
}

/*-----------------------------------------------------------------------------
FSceneRenderer
-----------------------------------------------------------------------------*/

static void DrawWireSphere(FPrimitiveDrawInterface* PDI,FSphere Sphere,const FLinearColor& SphereColor)
{
	DrawCircle( PDI, Sphere, FVector(1,0,0), FVector(0,1,0), SphereColor, Sphere.W, 64, SDPG_World );
	DrawCircle( PDI, Sphere, FVector(1,0,0), FVector(0,0,1), SphereColor, Sphere.W, 64, SDPG_World );
	DrawCircle( PDI, Sphere, FVector(0,1,0), FVector(0,0,1), SphereColor, Sphere.W, 64, SDPG_World );
}

void FSceneRenderer::CreateProjectedShadow(const FLightPrimitiveInteraction* Interaction,TArray<FProjectedShadowInfo*>& OutPreShadows)
{
	const FPrimitiveSceneInfo* PrimitiveSceneInfo = Interaction->GetPrimitiveSceneInfo();
	const FLightSceneInfo* LightSceneInfo = Interaction->GetLight();

	if(Interaction->GetDynamicShadowType() == DST_Projected && !PrimitiveSceneInfo->ShadowParent)
	{
		// Check if the shadow is visible in any of the views.
		UBOOL bShadowIsPotentiallyVisibleNextFrame = FALSE;
		UBOOL bShadowIsVisibleThisFrame = FALSE;
		UBOOL bSubjectIsVisible = FALSE;
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views(ViewIndex);

			// Compute the subject primitive's view relevance.  Note that the view won't necessarily have it cached,
			// since the primitive might not be visible.
			const FPrimitiveViewRelevance ViewRelevance = PrimitiveSceneInfo->Proxy->GetViewRelevance(&View);

			// Check if the subject primitive's shadow is view relevant.
			const UBOOL bShadowIsViewRelevant = (ViewRelevance.IsRelevant() || ViewRelevance.bShadowRelevance);

			// Check if the shadow and preshadow are occluded.
			const UBOOL bShadowIsOccluded =
				!View.bIgnoreOcclusionQueries &&
				View.State &&
				((FSceneViewState*)View.State)->IsShadowOccluded(PrimitiveSceneInfo->Component,LightSceneInfo->LightComponent);

			// The shadow is visible if it is view relevant and unoccluded.
			bShadowIsVisibleThisFrame |= (bShadowIsViewRelevant && !bShadowIsOccluded);
			bShadowIsPotentiallyVisibleNextFrame |= bShadowIsViewRelevant;
			
			// Check if the subject is visible this frame.
			const UBOOL bSubjectIsVisibleInThisView = View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id);
		    bSubjectIsVisible |= bSubjectIsVisibleInThisView;
		}

		if(!bShadowIsVisibleThisFrame && !bShadowIsPotentiallyVisibleNextFrame)
		{
			// Don't setup the shadow info for shadows which don't need to be rendered or occlusion tested.
			return;
		}

		// Compute the composite bounds of this group of shadow primitives.
		FBoxSphereBounds Bounds(PrimitiveSceneInfo->Bounds);
		for(FPrimitiveSceneInfo* ShadowChild = PrimitiveSceneInfo->FirstShadowChild;
			ShadowChild;
			ShadowChild = ShadowChild->NextShadowChild
			)
		{
			Bounds = Bounds + ShadowChild->Bounds;
		}

		// Compute the projected shadow initializer for this primitive-light pair.
		FProjectedShadowInitializer ShadowInitializer;
		if(LightSceneInfo->GetProjectedShadowInitializer(Bounds.GetSphere(),ShadowInitializer))
		{
			// Shadowing constants.
			static const FLOAT ShadowResolutionScale = 2.0f;
			const UINT MinShadowResolution = (LightSceneInfo->MinShadowResolution > 0) ? LightSceneInfo->MinShadowResolution : GEngine->MinShadowResolution;
			const UINT MaxShadowResolution = (LightSceneInfo->MaxShadowResolution > 0) ? LightSceneInfo->MaxShadowResolution : GEngine->MaxShadowResolution;

			// Compute the maximum resolution required for the shadow by any view.
			UINT MaxDesiredResolution = 0;
			FLOAT MinElapsedFadeTime = LightSceneInfo->ModShadowFadeoutTime;
			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				FViewInfo& View = Views(ViewIndex);

				// Determine the amount of shadow buffer resolution to use for the view.
				FVector4 ScreenPosition = View.WorldToScreen(Bounds.Origin);
				FLOAT ScreenRadius = Max(
					View.SizeX / 2.0f * View.ProjectionMatrix.M[0][0],
					View.SizeY / 2.0f * View.ProjectionMatrix.M[1][1]
					) *
					Bounds.SphereRadius /
					Max(ScreenPosition.W,1.0f);

				const UINT ShadowBufferResolution = GSceneRenderTargets.GetShadowDepthTextureResolution();
				MaxDesiredResolution = Max(
					MaxDesiredResolution,
					Clamp<UINT>(
						appTrunc(ScreenRadius * ShadowResolutionScale),
						Min(MinShadowResolution,ShadowBufferResolution - SHADOW_BORDER*2),
						Min(MaxShadowResolution - SHADOW_BORDER*2,ShadowBufferResolution - SHADOW_BORDER*2)
						)
					);
				
				//find the minimum elapsed time since the subject was visible
				MinElapsedFadeTime = Min(MinElapsedFadeTime, View.Family->CurrentWorldTime - PrimitiveSceneInfo->LastRenderTime);
			}

			FLOAT ElapsedTimeFadeFraction = 1.0f;
			if (LightSceneInfo->ModShadowFadeoutTime > 0.0001f)
			{
				//Fade the shadow out the longer the subject is not visible
				ElapsedTimeFadeFraction = appPow(1.0f - Min(MinElapsedFadeTime / LightSceneInfo->ModShadowFadeoutTime, 1.0f), LightSceneInfo->ModShadowFadeoutExponent);
			}

			// Fade the shadow out as it approaches the minimum resolution.
			const FLOAT ShadowResolutionFraction =
				(FLOAT)(Max<UINT>(MaxDesiredResolution - MinShadowResolution,0)) / (FLOAT)(Max<UINT>(MaxShadowResolution - MinShadowResolution,1));

			const FLOAT FadeAlpha = appPow(ShadowResolutionFraction, GEngine->ModShadowFadeDistanceExponent) * ElapsedTimeFadeFraction;

			// Only continue processing the shadow if it hasn't entirely faded away.
			if(FadeAlpha > 0.000001f)
			{
				// Create a projected shadow for this interaction's shadow.
				FProjectedShadowInfo* ProjectedShadowInfo = new FProjectedShadowInfo(
					LightSceneInfo,
					PrimitiveSceneInfo,
					ShadowInitializer,
					FALSE,
					MaxDesiredResolution,
					FadeAlpha
					);
				ProjectedShadows.AddRawItem(ProjectedShadowInfo);

			    // If the subject is visible in at least one view, create a preshadow for static primitives shadowing the subject.
			    FProjectedShadowInfo* ProjectedPreShadowInfo = NULL;
			    if(bSubjectIsVisible)
			    {
				    // Create a projected shadow for this interaction's preshadow.
				    ProjectedPreShadowInfo = new FProjectedShadowInfo(
					    LightSceneInfo,
					    PrimitiveSceneInfo,
					    ShadowInitializer,
						TRUE,
						MaxDesiredResolution / 2,
						FadeAlpha
					    );
				    ProjectedShadows.AddRawItem(ProjectedPreShadowInfo);
				    OutPreShadows.AddItem(ProjectedPreShadowInfo);
			    }
    
			    for(FPrimitiveSceneInfo* ShadowChild = PrimitiveSceneInfo->FirstShadowChild;
				    ShadowChild;
				    ShadowChild = ShadowChild->NextShadowChild
				    )
			    {
				    // Add the subject primitive to the projected shadow.
				    ProjectedShadowInfo->AddSubjectPrimitive(ShadowChild);
    
				    if(ProjectedPreShadowInfo)
				    {
					    // Add the subject primitive to the projected shadow as the receiver.
					    ProjectedPreShadowInfo->AddReceiverPrimitive(ShadowChild);
				    }
			    }
    
			    // Add the subject primitive to the projected shadow.
			    ProjectedShadowInfo->AddSubjectPrimitive(Interaction->GetPrimitiveSceneInfo());
    
			    if(ProjectedPreShadowInfo)
			    {
				    // Add the subject primitive to the projected shadow as the receiver.
				    ProjectedPreShadowInfo->AddReceiverPrimitive(Interaction->GetPrimitiveSceneInfo());
			    }
    
#if STATS
			    // Gather dynamic shadow stats.
			    if( bShouldGatherDynamicShadowStats )
			    {
				    FCombinedShadowStats ShadowStat;
				    ShadowStat.ShadowResolution = ProjectedShadowInfo->Resolution;
				    ProjectedShadowInfo->GetSubjectPrimitives( ShadowStat.SubjectPrimitives );
				    InteractionToDynamicShadowStatsMap.Set( const_cast<FLightPrimitiveInteraction*>(Interaction), ShadowStat );
			    }
#endif
			}
		}
	}
}

void FSceneRenderer::InitDynamicShadows()
{
	SCOPE_CYCLE_COUNTER(STAT_DynamicShadowSetupTime);

	// Pre-allocate for worst case.
	VisibleShadowCastingLightInfos.Empty(Scene->Lights.GetMaxIndex());

	for(TSparseArray<FLightSceneInfo*>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
	{
		FLightSceneInfo* LightSceneInfo = *LightIt;

		if( LightSceneInfo->bCastDynamicShadow && LightSceneInfo->bProjectedShadows )
		{	
			// see if the light is visible in any view
			UBOOL bIsVisibleInAnyView = FALSE;

			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				// View frustums are only checked when lights have visible primitives or have modulated shadows,
				// so we don't need to check for that again here
				bIsVisibleInAnyView = Views(ViewIndex).VisibleLightInfos(LightSceneInfo->Id).bInViewFrustum;
				if (bIsVisibleInAnyView) 
				{
					break;
				}
			}

			if( bIsVisibleInAnyView )
			{
				// Add to array of visible shadow casting lights if there is a view/ DPG that is affected.
				VisibleShadowCastingLightInfos.AddItem( LightSceneInfo );
#if STATS
				// Purely for stats gathering purposes.
				TMap<FProjectedShadowInfo*,const FLightPrimitiveInteraction*> PreShadowToInteractionMap;
#endif
				// Find dynamic primitives which cast a projected shadow.
				TArray<FProjectedShadowInfo*> PreShadows;
				for(const FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicPrimitiveList;
					Interaction;
					Interaction = Interaction->GetNextPrimitive()
					)
				{
					INT OldPreShadowsNum = PreShadows.Num();
					// Create projected shadow infos and add the one created for the preshadow (if used) to the PreShadows array.
					CreateProjectedShadow(Interaction,PreShadows);
#if STATS
					// New PreShadow, keep track of mapping to interaction so we can find it again below.
					if( bShouldGatherDynamicShadowStats && OldPreShadowsNum != PreShadows.Num() )
					{
						check( PreShadows.Num() - OldPreShadowsNum == 1 );
						PreShadowToInteractionMap.Set( PreShadows(PreShadows.Num()-1), Interaction );
					}
#endif
				}

				if(PreShadows.Num())
				{
					// Find static primitives which are part of a preshadow.
					for(const FLightPrimitiveInteraction* Interaction = LightSceneInfo->StaticPrimitiveList;
						Interaction;
						Interaction = Interaction->GetNextPrimitive()
						)
					{
						FPrimitiveSceneInfo* PrimitiveSceneInfo = Interaction->GetPrimitiveSceneInfo();
						const FBoxSphereBounds& PrimitiveBounds = PrimitiveSceneInfo->Bounds;

						for(INT ShadowIndex = 0;ShadowIndex < PreShadows.Num();ShadowIndex++)
						{
							FProjectedShadowInfo* ProjectedShadowInfo = PreShadows(ShadowIndex);

							// Check if this primitive is in the shadow's frustum.
							if(ProjectedShadowInfo->SubjectFrustum.IntersectBox(PrimitiveBounds.Origin,PrimitiveBounds.BoxExtent))
							{
								// Add this primitive to the shadow.
								ProjectedShadowInfo->AddSubjectPrimitive(PrimitiveSceneInfo);

#if STATS
								// Add preshadow primitives to shadow stats if we're gathering.
								if( bShouldGatherDynamicShadowStats )
								{
									const FLightPrimitiveInteraction* OriginalInteraction = PreShadowToInteractionMap.FindRef( ProjectedShadowInfo );
									check( OriginalInteraction );
									FCombinedShadowStats* ShadowStats = InteractionToDynamicShadowStatsMap.Find( const_cast<FLightPrimitiveInteraction*>(OriginalInteraction) );
									check( ShadowStats );
									ShadowStats->PreShadowPrimitives.AddItem( PrimitiveSceneInfo );
								}
#endif
							}
						}
					}
				}
			}
		}
	}

	// Initialize the views' ShadowVisibilityMaps and remove shadows without subjects.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);

		View.ShadowVisibilityMap = FBitArray(FALSE,ProjectedShadows.Num());
		for( INT ShadowIndex=0; ShadowIndex<ProjectedShadows.Num(); ShadowIndex++ )
		{
			const FProjectedShadowInfo& ProjectedShadowInfo = ProjectedShadows(ShadowIndex);
			
			// Only operate on shadows with subjects.
			if( ProjectedShadowInfo.HasSubjectPrims() )
			{
				// Compute the subject primitive's view relevance.  Note that the view won't necessarily have it cached,
				// since the primitive might not be visible.
				const FPrimitiveViewRelevance ViewRelevance = ProjectedShadowInfo.SubjectSceneInfo->Proxy->GetViewRelevance(&View);

				// Check if the subject primitive's shadow is view relevant.
				const UBOOL bShadowIsViewRelevant = (ViewRelevance.IsRelevant() || ViewRelevance.bShadowRelevance);

				// Check if the shadow and preshadow are occluded.
				const UBOOL bShadowIsOccluded =
					!View.bIgnoreOcclusionQueries &&
					View.State &&
					((FSceneViewState*)View.State)->IsShadowOccluded(ProjectedShadowInfo.SubjectSceneInfo->Component,ProjectedShadowInfo.LightSceneInfo->LightComponent);

				// The shadow is visible if it is view relevant and unoccluded.
				View.ShadowVisibilityMap(ShadowIndex) = bShadowIsViewRelevant && !bShadowIsOccluded;

				// Draw the shadow frustum.
				if((ViewFamily.ShowFlags & SHOW_ShadowFrustums) && bShadowIsViewRelevant && !bShadowIsOccluded && !ProjectedShadowInfo.bPreShadow)
				{
					FViewElementPDI ShadowFrustumPDI(&Views(ViewIndex),NULL);
					ProjectedShadowInfo.RenderFrustumWireframe(&ShadowFrustumPDI);
				}
			}
			// Remove shadow if it has no subjects.
			else
			{
				// Decrement shadow index as we're removing current. We can't iterate in reverse as shadow
				// visibility map uses indices.
				ProjectedShadows.Remove( ShadowIndex-- );
			}
		}
	}
}
	 
/**
 * Used by RenderLights to figure out if projected shadows need to be rendered to the attenuation buffer.
 *
 * @param LightSceneInfo Represents the current light
 * @return TRUE if anything needs to be rendered
 */
UBOOL FSceneRenderer::CheckForProjectedShadows( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex )
{
	// Find the projected shadows cast by this light.
	for(INT ShadowIndex = 0;ShadowIndex < ProjectedShadows.Num();ShadowIndex++)
	{
		FProjectedShadowInfo* ProjectedShadowInfo = &ProjectedShadows(ShadowIndex);
		if(ProjectedShadowInfo->LightSceneInfo == LightSceneInfo)
		{
			// Check that the shadow is visible in at least one view before rendering it.
			UBOOL bShadowIsVisible = FALSE;
			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				const FViewInfo& View = Views(ViewIndex);
				FPrimitiveViewRelevance ViewRel = ProjectedShadowInfo->SubjectSceneInfo->Proxy->GetViewRelevance(&View);
				bShadowIsVisible |= (ViewRel.GetDPG(DPGIndex) && View.ShadowVisibilityMap(ShadowIndex));
			}

			if(bShadowIsVisible)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}
	 
/**
 * Used by RenderLights to render shadows to the attenuation buffer.
 *
 * @param LightSceneInfo Represents the current light
 * @return TRUE if anything got rendered
 */
UBOOL FSceneRenderer::RenderProjectedShadows( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex )
{
	SCOPE_CYCLE_COUNTER(STAT_ProjectedShadowDrawTime);

	UBOOL bAttenuationBufferDirty = FALSE;

	// Find the projected shadows cast by this light.
	TArray<FProjectedShadowInfo*> Shadows;
	for(INT ShadowIndex = 0;ShadowIndex < ProjectedShadows.Num();ShadowIndex++)
	{
		FProjectedShadowInfo* ProjectedShadowInfo = &ProjectedShadows(ShadowIndex);
		if(ProjectedShadowInfo->LightSceneInfo == LightSceneInfo)
		{
			// Check that the shadow is visible in at least one view before rendering it.
			UBOOL bShadowIsVisible = FALSE;
			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				const FViewInfo& View = Views(ViewIndex);
				FPrimitiveViewRelevance ViewRel = ProjectedShadowInfo->SubjectSceneInfo->Proxy->GetViewRelevance(&View);
				bShadowIsVisible |= (ViewRel.GetDPG(DPGIndex) && View.ShadowVisibilityMap(ShadowIndex));
			}

			if(bShadowIsVisible)
			{
				// Add the shadow to the list of visible shadows cast by this light.
				INC_DWORD_STAT(STAT_ProjectedShadows);
				Shadows.AddItem(ProjectedShadowInfo);
			}
		}
	}

	// Sort the projected shadows by resolution.
	Sort<USE_COMPARE_POINTER(FProjectedShadowInfo,ShadowRendering)>(&Shadows(0),Shadows.Num());

	INT PassNumber			= 0;
	INT NumShadowsRendered	= 0;
	while(NumShadowsRendered < Shadows.Num())
	{
		INT NumAllocatedShadows = 0;

		// Allocate shadow texture space to the shadows.
		const UINT ShadowBufferResolution = GSceneRenderTargets.GetShadowDepthTextureResolution();
		FTextureLayout ShadowLayout(1,1,ShadowBufferResolution,ShadowBufferResolution);
		for(INT ShadowIndex = 0;ShadowIndex < Shadows.Num();ShadowIndex++)
		{
			FProjectedShadowInfo* ProjectedShadowInfo = Shadows(ShadowIndex);
			if(!ProjectedShadowInfo->bRendered)
			{
				if(ShadowLayout.AddElement(
					&ProjectedShadowInfo->X,
					&ProjectedShadowInfo->Y,
					ProjectedShadowInfo->Resolution + SHADOW_BORDER*2,
					ProjectedShadowInfo->Resolution + SHADOW_BORDER*2
					))
				{
					ProjectedShadowInfo->bAllocated = TRUE;
					NumAllocatedShadows++;
#if STATS
					// Keep track of pass number.
					if( bShouldGatherDynamicShadowStats )
					{
						// Disregard passes used to render preshadow as those are taken into account by shadow subject rendering and we wouldn't
						// be able to find the interaction.
						if( !ProjectedShadowInfo->bPreShadow )
						{
							// Brute force find the matching interaction. This is SLOW but fast enough.
							FCombinedShadowStats* ShadowStats = NULL;
							for( TMap<FLightPrimitiveInteraction*,FCombinedShadowStats>::TIterator It(InteractionToDynamicShadowStatsMap); It; ++It )
							{
								FLightPrimitiveInteraction* Interaction = It.Key();
								if( Interaction->GetLight() == LightSceneInfo 
								&&	Interaction->GetPrimitiveSceneInfo() == ProjectedShadowInfo->SubjectSceneInfo )
								{
									// Found a match, keep track of address. This is only safe as we are not modifying the TMap below.
									ShadowStats = &It.Value();
								}
							}
							check( ShadowStats );
							// Keep track of pass number.
							ShadowStats->ShadowPassNumber = PassNumber;
						}
					}
#endif
				}
			}
		}

		// Abort if we encounter a shadow that doesn't fit in the render target.
		if(!NumAllocatedShadows)
		{
			break;
		}

#if SUPPORTS_VSM
		const UBOOL bEnableVSMShadows = LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_VSM || 
			(GEngine->bEnableVSMShadows && LightSceneInfo->ShadowProjectionTechnique == ShadowProjTech_Default);
#else
		const UBOOL bEnableVSMShadows = FALSE;
#endif //#if SUPPORTS_VSM

		// Render the shadow depths.
		{
			SCOPED_DRAW_EVENT(EventShadowDepths)(DEC_SCENE_ITEMS,TEXT("Shadow Depths"));
			
			GSceneRenderTargets.BeginRenderingShadowDepth();			

#if 0 // XBOX
			if( DPGIndex == SDPG_Foreground &&
				!bEnableVSMShadows )
			{
				// Hack for rendering shadows from foreground objects onto the world.
				// Clear entire contents of depth texture since we may need to sample from outside of shadow info bounds
				RHIClear(GlobalContext,FALSE,FLinearColor::White,TRUE,1.0f,FALSE,0);                
			}
#endif

			// keep track of the max RECT needed for resolving the variance surface	
			FResolveParams ResolveParams;
			ResolveParams.X1 = 0;
			ResolveParams.Y1 = 0;
			UBOOL bResolveParamsInit=FALSE;
			// render depth for each shadow
			for(INT ShadowIndex = 0;ShadowIndex < Shadows.Num();ShadowIndex++)
			{
				FProjectedShadowInfo* ProjectedShadowInfo = Shadows(ShadowIndex);
				if(ProjectedShadowInfo->bAllocated)
				{
					ProjectedShadowInfo->RenderDepth(GlobalContext, this, DPGIndex);

					// init values
					if( !bResolveParamsInit )
					{
						ResolveParams.X2 = ProjectedShadowInfo->X + ProjectedShadowInfo->Resolution + SHADOW_BORDER*2;
						ResolveParams.Y2 = ProjectedShadowInfo->Y + ProjectedShadowInfo->Resolution + SHADOW_BORDER*2;
						bResolveParamsInit=TRUE;
					}
					// keep track of max extents
					else 
					{
						ResolveParams.X2 = Max<UINT>(ProjectedShadowInfo->X + ProjectedShadowInfo->Resolution + SHADOW_BORDER*2,ResolveParams.X2);
						ResolveParams.Y2 = Max<UINT>(ProjectedShadowInfo->Y + ProjectedShadowInfo->Resolution + SHADOW_BORDER*2,ResolveParams.Y2);
					}
				}
			}
			if( DPGIndex == SDPG_Foreground )
			{
				// for the foreground case we need the entire buffer to always be resolved
				GSceneRenderTargets.FinishRenderingShadowDepth(FResolveParams());
			}
			else
			{
				// only resolve the portion of the shadow buffer that we rendered to
				GSceneRenderTargets.FinishRenderingShadowDepth(ResolveParams);
			}
		}

#if SUPPORTS_VSM
		// Render shadow variance 
		if( bEnableVSMShadows )
		{	
			SCOPED_DRAW_EVENT(EventShadowVariance)(DEC_SCENE_ITEMS,TEXT("Shadow Variance"));

			RenderShadowVariance(GlobalContext,this,Shadows,DPGIndex);
		}
#endif //#if SUPPORTS_VSM

		// Render the shadow projections.
		{
			SCOPED_DRAW_EVENT(EventShadowProj)(DEC_SCENE_ITEMS,TEXT("Shadow Projection"));

			if(LightSceneInfo->LightShadowMode == LightShadow_Modulate)
			{
				GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
			}
			else
			{
				GSceneRenderTargets.BeginRenderingLightAttenuation();
			}

			//manually blended modulated shadows need to swap scene color targets in between each shadow 
			//so that overlapping shadows will not overwrite each other
			UBOOL bResolveAfterEachShadow = LightSceneInfo->LightShadowMode == LightShadow_Modulate && !CanBlendWithFPRenderTarget(GRHIShaderPlatform);

			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

				const FViewInfo& View = Views(ViewIndex);

				// Set the device viewport for the view.
				RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
				RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

				// Set the light's scissor rectangle.
				LightSceneInfo->SetScissorRect(GlobalContext,&View);

				// Project the shadow depth buffers onto the scene.
				for(INT ShadowIndex = 0;ShadowIndex < Shadows.Num();ShadowIndex++)
				{

					if (bResolveAfterEachShadow)
					{
						//bind the new scene color target and reset the viewport
						GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
						RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
					}

					FProjectedShadowInfo* ProjectedShadowInfo = Shadows(ShadowIndex);
					if(ProjectedShadowInfo->bAllocated)
					{
						// Only project the shadow in its subject's DPG.
						ProjectedShadowInfo->RenderProjection(GlobalContext, &View, DPGIndex);
					}

					if (bResolveAfterEachShadow)
					{
						//propagate the rendered shadow to the scene color texture
						//so that it will be included in the next shader blend
						GSceneRenderTargets.FinishRenderingSceneColor(TRUE);
					}
				}

				// Reset the scissor rectangle.
				RHISetScissorRect(GlobalContext,FALSE,0,0,0,0);
				RHISetDepthBoundsTest(GlobalContext, FALSE, 0.0f, 1.0f);
			}

			// resolve occurs after all shadow passes are completed
			//GSceneRenderTargets.FinishRenderingLightAttenuation();

			// Mark and count the rendered shadows.
			for(INT ShadowIndex = 0;ShadowIndex < Shadows.Num();ShadowIndex++)
			{
				FProjectedShadowInfo* ProjectedShadowInfo = Shadows(ShadowIndex);
				if(ProjectedShadowInfo->bAllocated)
				{
					ProjectedShadowInfo->bAllocated = FALSE;
					ProjectedShadowInfo->bRendered = TRUE;
					NumShadowsRendered++;
				}
			}

			// Mark the attenuation buffer as dirty.
			bAttenuationBufferDirty = TRUE;
		}

		// Increment pass number used by stats.
		PassNumber++;
	}
	return bAttenuationBufferDirty;
}
