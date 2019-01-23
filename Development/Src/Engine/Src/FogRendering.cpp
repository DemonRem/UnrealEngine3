/*=============================================================================
	FogRendering.cpp: Fog rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** Binds the parameters. */
void FHeightFogVertexShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	FogMinHeightParameter.Bind(ParameterMap,TEXT("FogMinHeight"), TRUE);
	FogMaxHeightParameter.Bind(ParameterMap,TEXT("FogMaxHeight"), TRUE);
	FogDistanceScaleParameter.Bind(ParameterMap,TEXT("FogDistanceScale"), TRUE);
	FogExtinctionDistanceParameter.Bind(ParameterMap,TEXT("FogExtinctionDistance"), TRUE);
	FogInScatteringParameter.Bind(ParameterMap,TEXT("FogInScattering"), TRUE);
	FogStartDistanceParameter.Bind(ParameterMap,TEXT("FogStartDistance"), TRUE);
}

/** 
* Sets the parameter values, this must be called before rendering the primitive with the shader applied. 
* @param VertexShader - the vertex shader to set the parameters on
*/
void FHeightFogVertexShaderParameters::Set(FCommandContextRHI* Context, const FMaterialRenderProxy* MaterialRenderProxy, const FSceneView* View, FShader* VertexShader) const
{
	FViewInfo* ViewInfo = (FViewInfo*)View;

	// Set the fog constants.
	if ( !MaterialRenderProxy->GetMaterial()->IsDecalMaterial() )
	{
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogMinHeightParameter,ViewInfo->FogMinHeight);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogMaxHeightParameter,ViewInfo->FogMaxHeight);
		SetVertexShaderValues(
			Context,
			VertexShader->GetVertexShader(),
			FogInScatteringParameter,
			(FLinearColor*)ViewInfo->FogInScattering,
			Min<UINT>(4,FogInScatteringParameter.GetNumRegisters())
			);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogDistanceScaleParameter,ViewInfo->FogDistanceScale);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogExtinctionDistanceParameter,ViewInfo->FogExtinctionDistance);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogStartDistanceParameter,ViewInfo->FogStartDistance);
	}
	else
	{
		// Decals get the default ('fog-less') values.
		static const FLOAT DefaultFogMinHeight[4] = { 0.f, 0.f, 0.f, 0.f };
		static const FLOAT DefaultFogMaxHeight[4] = { 0.f, 0.f, 0.f, 0.f };
		static const FLOAT DefaultFogDistanceScale[4] = { 0.f, 0.f, 0.f, 0.f };
		static const FLOAT DefaultFogStartDistance[4] = { 0.f, 0.f, 0.f, 0.f };
		static const FLOAT DefaultFogExtinctionDistance[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
		static const FLinearColor DefaultFogInScattering[4] = { FLinearColor::Black, FLinearColor::Black, FLinearColor::Black, FLinearColor::Black };

		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogMinHeightParameter,DefaultFogMinHeight);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogMaxHeightParameter,DefaultFogMaxHeight);
		SetVertexShaderValues(
			Context,
			VertexShader->GetVertexShader(),
			FogInScatteringParameter,
			(FLinearColor*)DefaultFogInScattering,
			Min<UINT>(4,FogInScatteringParameter.GetNumRegisters())
			);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogDistanceScaleParameter,DefaultFogDistanceScale);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogExtinctionDistanceParameter,DefaultFogExtinctionDistance);
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),FogStartDistanceParameter,DefaultFogStartDistance);
	}
}

/** Serializer. */
FArchive& operator<<(FArchive& Ar,FHeightFogVertexShaderParameters& Parameters)
{
	Ar << Parameters.FogDistanceScaleParameter;
	Ar << Parameters.FogExtinctionDistanceParameter;
	Ar << Parameters.FogMinHeightParameter;
	Ar << Parameters.FogMaxHeightParameter;
	Ar << Parameters.FogInScatteringParameter;
	Ar << Parameters.FogStartDistanceParameter;
	return Ar;
}


/** A vertex shader for rendering height fog. */
template<UINT NumLayers>
class THeightFogVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(THeightFogVertexShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	THeightFogVertexShader( )	{ }
	THeightFogVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		ScreenPositionScaleBiasParameter.Bind(Initializer.ParameterMap,TEXT("ScreenPositionScaleBias"));
		FogMinHeightParameter.Bind(Initializer.ParameterMap,TEXT("FogMinHeight"));
		FogMaxHeightParameter.Bind(Initializer.ParameterMap,TEXT("FogMaxHeight"));
		ScreenToWorldParameter.Bind(Initializer.ParameterMap,TEXT("ScreenToWorld"));
	}

	void SetParameters(FCommandContextRHI* Context,const FViewInfo& View)
	{
		// Set the transform from screen coordinates to scene texture coordinates.
		// NOTE: Need to set explicitly, since this is a vertex shader!
		SetVertexShaderValue(Context,GetVertexShader(),ScreenPositionScaleBiasParameter,View.ScreenPositionScaleBias);

		// Set the fog constants.
		SetVertexShaderValue(Context,GetVertexShader(),FogMinHeightParameter,View.FogMinHeight);
		SetVertexShaderValue(Context,GetVertexShader(),FogMaxHeightParameter,View.FogMaxHeight);

		FMatrix ScreenToWorld = FMatrix(
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,(1.0f - Z_PRECISION),1),
			FPlane(0,0,-View.NearClippingDistance * (1.0f - Z_PRECISION),0)
			) *
			View.InvViewProjectionMatrix *
			FTranslationMatrix(-(FVector)View.ViewOrigin);

		// Set the view constants, as many as were bound to the parameter.
		SetVertexShaderValues(Context,GetVertexShader(),ScreenToWorldParameter, (FVector4*)&ScreenToWorld, ScreenToWorldParameter.GetNumRegisters());
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << ScreenPositionScaleBiasParameter;
		Ar << FogMinHeightParameter;
		Ar << FogMaxHeightParameter;
		Ar << ScreenToWorldParameter;
	}

private:
	FShaderParameter ScreenPositionScaleBiasParameter;
	FShaderParameter FogMinHeightParameter;
	FShaderParameter FogMaxHeightParameter;
	FShaderParameter ScreenToWorldParameter;
};

IMPLEMENT_SHADER_TYPE(template<>,THeightFogVertexShader<1>,TEXT("HeightFogVertexShader"),TEXT("OneLayerMain"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,THeightFogVertexShader<4>,TEXT("HeightFogVertexShader"),TEXT("FourLayerMain"),SF_Vertex,0,0);

/** A vertex shader for rendering height fog. */
template<UINT NumLayers>
class THeightFogPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(THeightFogPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	THeightFogPixelShader( )	{ }
	THeightFogPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		FogDistanceScaleParameter.Bind(Initializer.ParameterMap,TEXT("FogDistanceScale"));
		FogExtinctionDistanceParameter.Bind(Initializer.ParameterMap,TEXT("FogExtinctionDistance"));
		FogInScatteringParameter.Bind(Initializer.ParameterMap,TEXT("FogInScattering"));
		FogStartDistanceParameter.Bind(Initializer.ParameterMap,TEXT("FogStartDistance"));
	}

	void SetParameters(FCommandContextRHI* Context,const FViewInfo& View)
	{
		SceneTextureParameters.Set(Context, &View, this);

		// Set the fog constants.
		SetPixelShaderValues(
			Context,
			GetPixelShader(),
			FogInScatteringParameter,
			(FLinearColor*)View.FogInScattering,
			Min<UINT>(NumLayers,FogInScatteringParameter.GetNumRegisters())
			);
		SetPixelShaderValue(Context,GetPixelShader(),FogDistanceScaleParameter,View.FogDistanceScale);
		SetPixelShaderValue(Context,GetPixelShader(),FogExtinctionDistanceParameter,View.FogExtinctionDistance);
		SetPixelShaderValue(Context,GetPixelShader(),FogStartDistanceParameter,View.FogStartDistance);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << SceneTextureParameters;
		Ar << FogDistanceScaleParameter;
		Ar << FogExtinctionDistanceParameter;
		Ar << FogInScatteringParameter;
		Ar << FogStartDistanceParameter;
	}

private:
	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderParameter FogDistanceScaleParameter;
	FShaderParameter FogExtinctionDistanceParameter;
	FShaderParameter FogInScatteringParameter;
	FShaderParameter FogStartDistanceParameter;
};

IMPLEMENT_SHADER_TYPE(template<>,THeightFogPixelShader<1>,TEXT("HeightFogPixelShader"),TEXT("OneLayerMain"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,THeightFogPixelShader<4>,TEXT("HeightFogPixelShader"),TEXT("FourLayerMain"),SF_Pixel,0,0);

/** The fog vertex declaration resource type. */
class FFogVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	// Destructor
	virtual ~FFogVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.AddItem(FVertexElement(0,0,VET_Float2,VEU_Position,0));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.Release();
	}
};

/** Vertex declaration for the light function fullscreen 2D quad. */
TGlobalResource<FFogVertexDeclaration> GFogVertexDeclaration;

void FSceneRenderer::InitFogConstants()
{
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);

		// set fog consts based on height fog components
		if(View.Family->ShowFlags & SHOW_Fog)
		{
			// Remap the fog layers into back to front order.
			INT FogLayerMap[4];
			INT NumFogLayers = 0;
			for(INT AscendingFogIndex = Min(Scene->Fogs.Num(),4) - 1;AscendingFogIndex >= 0;AscendingFogIndex--)
			{
				const FHeightFogSceneInfo& FogSceneInfo = Scene->Fogs(AscendingFogIndex);
				if(FogSceneInfo.Height > View.ViewOrigin.Z)
				{
					for(INT DescendingFogIndex = 0;DescendingFogIndex <= AscendingFogIndex;DescendingFogIndex++)
					{
						FogLayerMap[NumFogLayers++] = DescendingFogIndex;
					}
					break;
				}
				FogLayerMap[NumFogLayers++] = AscendingFogIndex;
			}

			// Calculate the fog constants.
			for(INT LayerIndex = 0;LayerIndex < NumFogLayers;LayerIndex++)
			{
				// remapped fog layers in ascending order
				const FHeightFogSceneInfo& FogSceneInfo = Scene->Fogs(FogLayerMap[LayerIndex]);
				// log2(1-density)
				View.FogDistanceScale[LayerIndex] = appLoge(1.0f - FogSceneInfo.Density) / appLoge(2.0f);
				if(FogLayerMap[LayerIndex] + 1 < NumFogLayers)
				{
					// each min height is adjusted to aligned with the max height of the layer above
					View.FogMinHeight[LayerIndex] = Scene->Fogs(FogLayerMap[LayerIndex] + 1).Height;
				}
				else
				{
					// lowest layer extends down
					View.FogMinHeight[LayerIndex] = -HALF_WORLD_MAX;
				}
				// max height is set by the actor's height
				View.FogMaxHeight[LayerIndex] = FogSceneInfo.Height;
				// This formula is incorrect, but must be used to support legacy content.  The in-scattering color should be computed like this:
				// FogInScattering[LayerIndex] = FLinearColor(FogComponent->LightColor) * (FogComponent->LightBrightness / (appLoge(2.0f) * FogDistanceScale[LayerIndex]));
				View.FogInScattering[LayerIndex] = FogSceneInfo.LightColor / appLoge(0.5f);
				// anything beyond the extinction distance goes to full fog
				View.FogExtinctionDistance[LayerIndex] = FogSceneInfo.ExtinctionDistance;
				// start distance where fog affects the scene
				View.FogStartDistance[LayerIndex] = Max( 0.f, FogSceneInfo.StartDistance );			
			}
		}
	}
}

FGlobalBoundShaderStateRHIRef OneLayerFogBoundShaderState;
FGlobalBoundShaderStateRHIRef FourLayerFogBoundShaderState;

UBOOL FSceneRenderer::RenderFog(UINT DPGIndex)
{
	if((DPGIndex == SDPG_World) && (Scene->Fogs.Num()))
	{
		SCOPED_DRAW_EVENT(EventFog)(DEC_SCENE_ITEMS,TEXT("Fog"));

		static const FVector2D Vertices[4] =
		{
			FVector2D(-1,-1),
			FVector2D(-1,+1),
			FVector2D(+1,+1),
			FVector2D(+1,-1),
		};
		static const WORD Indices[6] =
		{
			0, 1, 2,
			0, 2, 3
		};

		GSceneRenderTargets.BeginRenderingSceneColor();
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views(ViewIndex);

			// Set the device viewport for the view.
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// No depth tests, no backface culling.
			RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());
			RHISetRasterizerState(GlobalContext,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

			if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
			{
				RHISetBlendState(GlobalContext,TStaticBlendState<BO_Add,BF_One,BF_SourceAlpha>::GetRHI());
			}
			else
			{
				RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
			}

			//use the optimized one layer version if there is only one height fog layer
			if (Scene->Fogs.Num() == 1)
			{
				TShaderMapRef<THeightFogVertexShader<1> > VertexShader(GetGlobalShaderMap());
				TShaderMapRef<THeightFogPixelShader<1> > OneLayerHeightFogPixelShader(GetGlobalShaderMap());

				SetGlobalBoundShaderState(GlobalContext, OneLayerFogBoundShaderState, GFogVertexDeclaration.VertexDeclarationRHI, *VertexShader, *OneLayerHeightFogPixelShader, sizeof(FVector2D));
				VertexShader->SetParameters(GlobalContext,View);
				OneLayerHeightFogPixelShader->SetParameters(GlobalContext,View);
			}
			//otherwise use the four layer version
			else
			{
				TShaderMapRef<THeightFogVertexShader<4> > VertexShader(GetGlobalShaderMap());
				TShaderMapRef<THeightFogPixelShader<4> > FourLayerHeightFogPixelShader(GetGlobalShaderMap());

				SetGlobalBoundShaderState(GlobalContext, FourLayerFogBoundShaderState, GFogVertexDeclaration.VertexDeclarationRHI, *VertexShader, *FourLayerHeightFogPixelShader, sizeof(FVector2D));
				VertexShader->SetParameters(GlobalContext,View);
				FourLayerHeightFogPixelShader->SetParameters(GlobalContext,View);
			}

			// disable alpha writes in order to preserve scene depth values on PC
			RHISetColorWriteMask(GlobalContext, CW_RED|CW_GREEN|CW_BLUE);

			// Draw a quad covering the view.
			RHIDrawIndexedPrimitiveUP(
				GlobalContext,
				PT_TriangleList,
				0,
				ARRAY_COUNT(Vertices),
				2,
				Indices,
				sizeof(Indices[0]),
				Vertices,
				sizeof(Vertices[0])
				);

			// restore color write mask
			RHISetColorWriteMask(GlobalContext, CW_RED|CW_GREEN|CW_BLUE|CW_ALPHA);
		}

		//no need to resolve since we used alpha blending
		GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
		return TRUE;
	}

	return FALSE;
}
