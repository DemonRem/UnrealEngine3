/*=============================================================================
LandscapeRender.cpp: New terrain rendering
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnTerrain.h"
#include "LandscapeRender.h"
#include "ScenePrivate.h"

#define LANDSCAPE_LOD_DISTANCE_FACTOR 2.f
#define LANDSCAPE_MAX_COMPONENT_SIZE 255
#define LANDSCAPE_LOD_LEVELS 8


#if WITH_EDITOR
ELandscapeViewMode::Type GLandscapeViewMode = ELandscapeViewMode::Normal;
UMaterial* GLayerDebugColorMaterial = NULL;

void FLandscapeEditToolRenderData::UpdateDebugColorMaterial()
{
	if (!LandscapeComponent)
	{
		return;
	}

	ALandscape* Landscape = LandscapeComponent->GetLandscapeActor();

	// Debug Color Rendering Material....
	if (Landscape)
	{
		INT R = INDEX_NONE, G = INDEX_NONE, B = INDEX_NONE;
		FString DebugKey = LandscapeComponent->GetLayerDebugColorKey(R, G, B);

		UMaterialInstanceConstant* DebugMaterialInstance = Landscape->MaterialInstanceConstantMap.FindRef(*DebugKey);
		if (!GLayerDebugColorMaterial)
		{
			GLayerDebugColorMaterial = LoadObject<UMaterial>(NULL, TEXT("EditorLandscapeResources.DebugLayerColorMat"), NULL, LOAD_None, NULL);
		}

		if (GLayerDebugColorMaterial)
		{
			if( DebugMaterialInstance == NULL || DebugMaterialInstance->Parent != GLayerDebugColorMaterial )
			{
				DebugMaterialInstance = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), LandscapeComponent->GetOutermost(), FName(*FString::Printf(TEXT("%s_%s"),*Landscape->GetName(),*DebugKey)), RF_Public|RF_Standalone);
				// debugf(TEXT("Looking for key %s, making new combination %s"), *LayerKey, *CombinationMaterialInstance->GetName());
				Landscape->MaterialInstanceConstantMap.Set(*DebugKey,DebugMaterialInstance );

				DebugMaterialInstance->SetParent(GLayerDebugColorMaterial);
				FStaticParameterSet StaticParameters;
				DebugMaterialInstance->GetStaticParameterValues(&StaticParameters);

				UBOOL bNeedDebugColor = FALSE;

				// Look through our allocations to see if we need this layer.
				// If not found, this component doesn't use the layer, and WeightmapIndex remains as INDEX_NONE.
				for( INT LayerParameterIdx=0;LayerParameterIdx<StaticParameters.TerrainLayerWeightParameters.Num();LayerParameterIdx++ )
				{
					FStaticTerrainLayerWeightParameter& LayerParameter = StaticParameters.TerrainLayerWeightParameters(LayerParameterIdx);
					LayerParameter.WeightmapIndex = INDEX_NONE;

					if( TEXT("R") == LayerParameter.ParameterName)
					{
						LayerParameter.WeightmapIndex = R;
						LayerParameter.bOverride = TRUE;
						bNeedDebugColor = TRUE;
						// debugf(TEXT(" Layer %s channel %d"), *LayerParameter.ParameterName.ToString(), LayerParameter.WeightmapIndex);
					}
					else if( TEXT("G") == LayerParameter.ParameterName)
					{
						LayerParameter.WeightmapIndex = G;
						LayerParameter.bOverride = TRUE;
						bNeedDebugColor = TRUE;
						// debugf(TEXT(" Layer %s channel %d"), *LayerParameter.ParameterName.ToString(), LayerParameter.WeightmapIndex);
					}
					else if( TEXT("B") == LayerParameter.ParameterName)
					{
						LayerParameter.WeightmapIndex = B;
						LayerParameter.bOverride = TRUE;
						bNeedDebugColor = TRUE;
						// debugf(TEXT(" Layer %s channel %d"), *LayerParameter.ParameterName.ToString(), LayerParameter.WeightmapIndex);
					}
				}

				if (DebugMaterialInstance->SetStaticParameterValues(&StaticParameters))
				{
					//mark the package dirty if a compile was needed
					DebugMaterialInstance->MarkPackageDirty();
				}

				DebugMaterialInstance->InitResources();
				DebugMaterialInstance->UpdateStaticPermutation();
			}

			// Create the instance for this component, that will use the layer combination instance.
			if( DebugColorLayerMaterial == NULL )
			{
				DebugColorLayerMaterial = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), LandscapeComponent->GetOutermost(), FName(*FString::Printf(TEXT("%s_DebugColor"),*LandscapeComponent->GetName())), RF_Public|RF_Standalone);
			}

			DebugColorLayerMaterial->SetParent(DebugMaterialInstance);
			for( INT i=0; i < LandscapeComponent->WeightmapTextures.Num(); i++ )
			{
				// debugf(TEXT("Setting Weightmap%d = %s"), i, *WeightmapTextures(i)->GetName());
				DebugColorLayerMaterial->SetTextureParameterValue(FName(*FString::Printf(TEXT("Weightmap%d"),i)), LandscapeComponent->WeightmapTextures(i));
			}
		}
	}
}
#endif

//
// FLandscapeComponentSceneProxy
//
FLandscapeVertexBuffer* FLandscapeComponentSceneProxy::SharedVertexBuffer = NULL;
FLandscapeIndexBuffer** FLandscapeComponentSceneProxy::SharedIndexBuffers = NULL;

FLandscapeComponentSceneProxy::FLandscapeComponentSceneProxy(ULandscapeComponent* InComponent, FLandscapeEditToolRenderData* InEditToolRenderData)
:	FPrimitiveSceneProxy(InComponent)
,	MaxLOD(appCeilLogTwo(InComponent->SubsectionSizeQuads+1)-1)
,	ComponentSizeQuads(InComponent->ComponentSizeQuads)
,	NumSubsections(InComponent->NumSubsections)
,	SubsectionSizeQuads(InComponent->SubsectionSizeQuads)
,	SubsectionSizeVerts(InComponent->SubsectionSizeQuads+1)
,	SectionBaseX(InComponent->SectionBaseX)
,	SectionBaseY(InComponent->SectionBaseY)
,	DrawScaleXY(InComponent->GetLandscapeActor()->DrawScale * InComponent->GetLandscapeActor()->DrawScale3D.X)
,	ActorOrigin(InComponent->GetLandscapeActor()->Location)
,	StaticLightingResolution(InComponent->GetLandscapeActor()->StaticLightingResolution)
,	CurrentLOD(0)
,	CurrentSubX(0)
,	CurrentSubY(0)
,	WeightmapScaleBias(InComponent->WeightmapScaleBias)
,	WeightmapSubsectionOffset(InComponent->WeightmapSubsectionOffset)
,	LayerUVPan(InComponent->LayerUVPan)
,	HeightmapTexture(InComponent->HeightmapTexture)
,	HeightmapScaleBias(InComponent->HeightmapScaleBias)
,	HeightmapSubsectionOffset(InComponent->HeightmapSubsectionOffset)
,	XYOffsetTexture(NULL)
,	XYOffsetUVScaleBias(FVector4( 1.f/((FLOAT)InComponent->ComponentSizeQuads+1.f), 1.f/((FLOAT)InComponent->ComponentSizeQuads+1.f), 0, 0))
,	XYOffsetSubsectionOffset(((FLOAT)InComponent->SubsectionSizeQuads+1.f) / ((FLOAT)InComponent->ComponentSizeQuads+1.f))
,	XYOffsetScale(-10.f)
,	VertexFactory(NULL)
,	VertexBuffer(NULL)
,	IndexBuffers(NULL)
,	MaterialInterface(InComponent->MaterialInstance)
,	EditToolRenderData(InEditToolRenderData)
,	ComponentLightInfo(NULL)
{
	if( InComponent->GetLandscapeActor()->MaxLODLevel >= 0 )
	{
		MaxLOD = Min<INT>(MaxLOD, InComponent->GetLandscapeActor()->MaxLODLevel);
	}

	ComponentLightInfo = new FLandscapeLCI(InComponent);
	check(ComponentLightInfo);

	// Set Lightmap ScaleBias
	INT PatchExpandCountX = 1;
	INT PatchExpandCountY = 1;
	INT DesiredSize = 1;

	FLOAT LightMapRatio = ::GetTerrainExpandPatchCount(StaticLightingResolution, PatchExpandCountX, PatchExpandCountY, ComponentSizeQuads, DesiredSize);

	LightmapScaleBias.X = LightMapRatio / (FLOAT)(ComponentSizeQuads + 2 * PatchExpandCountX + 1 );
	LightmapScaleBias.Y = LightMapRatio / (FLOAT)(ComponentSizeQuads + 2 * PatchExpandCountY + 1 );
	
	// Should be same...
	PatchExpandCount = PatchExpandCountX;

	// Check material usage
	if( MaterialInterface == NULL || !MaterialInterface->CheckMaterialUsage(MATUSAGE_Landscape) )
	{
		MaterialInterface = GEngine->DefaultMaterial;
	}

	MaterialViewRelevance = MaterialInterface->GetViewRelevance();
}

FLandscapeComponentSceneProxy::~FLandscapeComponentSceneProxy()
{
	if( VertexFactory )
	{
		VertexFactory->ReleaseResource();
		delete VertexFactory;
		VertexFactory = NULL;
	}

	if( VertexBuffer )
	{
		check( SharedVertexBuffer == VertexBuffer );
		if( SharedVertexBuffer->Release() == 0 )
		{
			SharedVertexBuffer = NULL;
		}
		VertexBuffer = NULL;
	}

	if( IndexBuffers )
	{
		check( SharedIndexBuffers == IndexBuffers );
		UBOOL bCanDeleteArray = TRUE;
		for( INT i=0;i<LANDSCAPE_LOD_LEVELS;i++ )
		{
			if( SharedIndexBuffers[i]->Release() == 0 )
			{
				SharedIndexBuffers[i] = NULL;
			}
			else
			{
				bCanDeleteArray = FALSE;
			}
		}
		if( bCanDeleteArray )
		{
			delete[] SharedIndexBuffers;
			SharedIndexBuffers = NULL;
		}
		IndexBuffers = NULL;
	}

	delete ComponentLightInfo;
	ComponentLightInfo = NULL;
}

UBOOL FLandscapeComponentSceneProxy::CreateRenderThreadResources()
{
	check(HeightmapTexture != NULL);

	if( SharedVertexBuffer == NULL )
	{
		SharedVertexBuffer = new FLandscapeVertexBuffer(LANDSCAPE_MAX_COMPONENT_SIZE+1);
	}

	if( SharedIndexBuffers == NULL )
	{
		SharedIndexBuffers = new FLandscapeIndexBuffer*[LANDSCAPE_LOD_LEVELS];
		for( INT i=0;i<LANDSCAPE_LOD_LEVELS;i++ )
		{
			SharedIndexBuffers[i] = new FLandscapeIndexBuffer(((LANDSCAPE_MAX_COMPONENT_SIZE+1) >> i)-1, LANDSCAPE_MAX_COMPONENT_SIZE+1);
		}
	}
	for( INT i=0;i<LANDSCAPE_LOD_LEVELS;i++ )
	{
		SharedIndexBuffers[i]->AddRef();
	}
	IndexBuffers = SharedIndexBuffers;

	SharedVertexBuffer->AddRef();
	VertexBuffer = SharedVertexBuffer;

	VertexFactory = new FLandscapeVertexFactory(this);
	VertexFactory->Data.PositionComponent = FVertexStreamComponent(VertexBuffer, 0, sizeof(FLandscapeVertexBuffer::FLandscapeVertex), VET_Float2);
	VertexFactory->InitResource();

	return TRUE;
}

FPrimitiveViewRelevance FLandscapeComponentSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	const EShowFlags ShowFlags = View->Family->ShowFlags;
	if(ShowFlags & SHOW_Terrain)
	{
		if (IsShown(View))
		{
			Result.bDynamicRelevance = TRUE;
			Result.SetDPG(GetDepthPriorityGroup(View),TRUE);
			Result.bDecalStaticRelevance = FALSE;//!!HasRelevantStaticDecals(View);
			Result.bDecalDynamicRelevance = FALSE;//!!HasRelevantDynamicDecals(View);

			MaterialViewRelevance.SetPrimitiveViewRelevance(Result);

#if WITH_EDITOR
			if( EditToolRenderData && EditToolRenderData->ToolMaterial )
			{
				MaterialViewRelevance.bTranslucency = TRUE;
			}
#endif
		}
		if (IsShadowCast(View))
		{
			Result.bShadowRelevance = TRUE;
		}
		Result.bDecalStaticRelevance = FALSE; //HasRelevantStaticDecals(View);
		Result.bDecalDynamicRelevance = FALSE; //HasRelevantDynamicDecals(View);
	}
	return Result;
}

/**
*	Determines the relevance of this primitive's elements to the given light.
*	@param	LightSceneInfo			The light to determine relevance for
*	@param	bDynamic (output)		The light is dynamic for this primitive
*	@param	bRelevant (output)		The light is relevant for this primitive
*	@param	bLightMapped (output)	The light is light mapped for this primitive
*/
void FLandscapeComponentSceneProxy::GetLightRelevance(const FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped) const
{
	const ELightInteractionType InteractionType = ComponentLightInfo->GetInteraction(LightSceneInfo).GetType();

	// Attach the light to the primitive's static meshes.
	bDynamic = TRUE;
	bRelevant = FALSE;
	bLightMapped = TRUE;

	if (ComponentLightInfo)
	{
		ELightInteractionType InteractionType = ComponentLightInfo->GetInteraction(LightSceneInfo).GetType();
		if(InteractionType != LIT_CachedIrrelevant)
		{
			bRelevant = TRUE;
		}
		if(InteractionType != LIT_CachedLightMap && InteractionType != LIT_CachedIrrelevant)
		{
			bLightMapped = FALSE;
		}
		if(InteractionType != LIT_Uncached)
		{
			bDynamic = FALSE;
		}
	}
	else
	{
		bRelevant = TRUE;
		bLightMapped = FALSE;
	}
}

/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
* @param	DPGIndex - current depth priority 
* @param	Flags - optional set of flags from EDrawDynamicElementFlags
*/
void FLandscapeComponentSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
#if _WINDOWS || XBOX
	if( (GRHIShaderPlatform!=SP_PCD3D_SM3 && GRHIShaderPlatform!=SP_PCD3D_SM4 && GRHIShaderPlatform!=SP_PCD3D_SM5 && GRHIShaderPlatform!=SP_XBOXD3D) || GIsMobileGame )
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_TerrainRenderTime);

	// Determine the DPG the primitive should be drawn in for this view.
	if ((GetDepthPriorityGroup(View) == DPGIndex) && ((View->Family->ShowFlags & SHOW_Terrain) != 0))
	{
		// Update CurrentLOD
		for( INT SubY=0;SubY<NumSubsections;SubY++ )
		{
			CurrentSubY = SubY;

			for( INT SubX=0;SubX<NumSubsections;SubX++ )
			{
				CurrentSubX = SubX;

				// camera position in heightmap space
				FVector2D CameraPos(
					(View->ViewOrigin.X - ActorOrigin.X) / (DrawScaleXY) - (FLOAT)(SectionBaseX + SubX * SubsectionSizeQuads),
					(View->ViewOrigin.Y - ActorOrigin.Y) / (DrawScaleXY) - (FLOAT)(SectionBaseY + SubY * SubsectionSizeQuads)
				);

				FVector2D ComponentPosition(0.5f * (FLOAT)SubsectionSizeQuads, 0.5f * (FLOAT)SubsectionSizeQuads);

				FLOAT ComponentDistance = FVector2D(CameraPos-ComponentPosition).Size() - appSqrt(2.f * Square(0.5f*(FLOAT)SubsectionSizeQuads));
				FLOAT LodDistance = appSqrt(2.f * Square((FLOAT)SubsectionSizeQuads)) * LANDSCAPE_LOD_DISTANCE_FACTOR;

				CurrentLOD = Clamp<INT>( appFloor( ComponentDistance / LodDistance ), 0, MaxLOD );

				// Render!
				if( CurrentLOD >= 0 )
				{
					FMeshElement Mesh;
					Mesh.IndexBuffer = IndexBuffers[appCeilLogTwo((LANDSCAPE_MAX_COMPONENT_SIZE+1) / (SubsectionSizeVerts >> CurrentLOD))];
					Mesh.VertexFactory = VertexFactory;
					Mesh.NumPrimitives = Square(((SubsectionSizeVerts >> CurrentLOD) - 1)) * 2;

					INC_DWORD_STAT_BY(STAT_TerrainTriangles,Mesh.NumPrimitives);

					Mesh.DynamicVertexData = NULL;
					Mesh.DynamicVertexStride = 0;
					Mesh.DynamicIndexData = NULL;
					Mesh.DynamicIndexStride = 0;
					Mesh.LCI = ComponentLightInfo; 

#if WITH_EDITOR
					if ( GLandscapeViewMode == ELandscapeViewMode::DebugLayer )
					{
						if (EditToolRenderData && EditToolRenderData->DebugColorLayerMaterial)
						{
							Mesh.MaterialRenderProxy = EditToolRenderData->DebugColorLayerMaterial->GetRenderProxy(FALSE);
						}
						else
						{
							Mesh.MaterialRenderProxy = MaterialInterface->GetRenderProxy(FALSE);
						}
					}
					else
					{
						Mesh.MaterialRenderProxy = MaterialInterface->GetRenderProxy(FALSE);
					}
#else
					Mesh.MaterialRenderProxy = MaterialInterface->GetRenderProxy(FALSE);
#endif

					FVector ThisSubsectionTranslation( DrawScaleXY * (FLOAT)(SubX * SubsectionSizeQuads), DrawScaleXY * (FLOAT)(SubY * SubsectionSizeQuads), 0);
					Mesh.LocalToWorld = LocalToWorld.ConcatTranslation(ThisSubsectionTranslation);
					Mesh.WorldToLocal = LocalToWorld.Inverse();
					Mesh.FirstIndex = 0;
					Mesh.MinVertexIndex = 0;
					Mesh.MaxVertexIndex = (LANDSCAPE_MAX_COMPONENT_SIZE+1) * (SubsectionSizeVerts >> CurrentLOD) - 1;
					Mesh.UseDynamicData = FALSE;
					Mesh.ReverseCulling = LocalToWorldDeterminant < 0.0f ? TRUE : FALSE;;
					Mesh.CastShadow = TRUE;
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DPGIndex;
					Mesh.bUsePreVertexShaderCulling = FALSE;
					Mesh.PlatformMeshData = NULL;

					FLinearColor WireColors[7];
					WireColors[0] = FLinearColor(1,1,1,1);
					WireColors[1] = FLinearColor(1,0,0,1);
					WireColors[2] = FLinearColor(0,1,0,1);
					WireColors[3] = FLinearColor(0,0,1,1);
					WireColors[4] = FLinearColor(1,1,0,1);
					WireColors[5] = FLinearColor(1,0,1,1);
					WireColors[6] = FLinearColor(0,1,1,1);

					DrawRichMesh(PDI,Mesh,WireColors[CurrentLOD],FLinearColor(1.0f,1.0f,1.0f),FLinearColor(1.0f,1.0f,1.0f),PrimitiveSceneInfo,bSelected);

#if WITH_EDITOR
					// Render tool
					if( EditToolRenderData && EditToolRenderData->ToolMaterial )
					{
						Mesh.MaterialRenderProxy = EditToolRenderData->ToolMaterial->GetRenderProxy(0);
						PDI->DrawMesh(Mesh);
					}
#endif
				}
			}
		}

		if (View->Family->ShowFlags & SHOW_TerrainPatches)
		{
			DrawWireBox(PDI, PrimitiveSceneInfo->Bounds.GetBox(), FColor(255, 255, 0), DPGIndex);
		}
	}
#endif
}


//
// FLandscapeVertexBuffer
//

/** 
* Initialize the RHI for this rendering resource 
*/
void FLandscapeVertexBuffer::InitRHI()
{
	// create a static vertex buffer
	VertexBufferRHI = RHICreateVertexBuffer(Square(SizeVerts) * sizeof(FLandscapeVertex), NULL, RUF_Static);
	FLandscapeVertex* Vertex = (FLandscapeVertex*)RHILockVertexBuffer(VertexBufferRHI, 0, Square(SizeVerts) * sizeof(FLandscapeVertex),FALSE);

	for( INT y=0;y<SizeVerts;y++ )
	{
		for( INT x=0;x<SizeVerts;x++ )
		{
			Vertex->VertexX = x;
			Vertex->VertexY = y;
			Vertex++;
		}
	}

	RHIUnlockVertexBuffer(VertexBufferRHI);
}

//
// FLandscapeVertexBuffer
//
FLandscapeIndexBuffer::FLandscapeIndexBuffer(INT SizeQuads, INT VBSizeVertices)
{
	TArray<WORD> NewIndices;
	NewIndices.Empty(SizeQuads*SizeQuads*6);
	for( INT y=0;y<SizeQuads;y++ )
	{
		for( INT x=0;x<SizeQuads;x++ )
		{
			NewIndices.AddItem( (x+0) + (y+0) * VBSizeVertices );
			NewIndices.AddItem( (x+1) + (y+1) * VBSizeVertices );
			NewIndices.AddItem( (x+1) + (y+0) * VBSizeVertices );
			NewIndices.AddItem( (x+0) + (y+0) * VBSizeVertices );
			NewIndices.AddItem( (x+0) + (y+1) * VBSizeVertices );
			NewIndices.AddItem( (x+1) + (y+1) * VBSizeVertices );
		}
	}
	Indices = NewIndices;

	InitResource();
}

//
// FLandscapeVertexFactoryShaderParameters
//

/** VTF landscape vertex factory */

/** Shader parameters for use with FLandscapeVertexFactory */
class FLandscapeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	* Bind shader constants by name
	* @param	ParameterMap - mapping of named shader constants to indices
	*/
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorld"));
		WorldToLocalParameter.Bind(ParameterMap,TEXT("WorldToLocal"),TRUE);
		HeightmapUVScaleBiasParameter.Bind(ParameterMap,TEXT("HeightmapUVScaleBias"),TRUE);
		WeightmapUVScaleBiasParameter.Bind(ParameterMap,TEXT("WeightmapUVScaleBias"),TRUE);
		HeightmapTextureParameter.Bind(ParameterMap,TEXT("HeightmapTexture"),TRUE);
		LodValuesParameter.Bind(ParameterMap,TEXT("LodValues"),TRUE);
		LodDistancesValuesParameter.Bind(ParameterMap,TEXT("LodDistancesValues"),TRUE);
		SubsectionSizeQuadsLayerUVPanParameter.Bind(ParameterMap,TEXT("SectionSizeQuadsLayerUVPan"),TRUE);
		HeightmapLodBiasParameter.Bind(ParameterMap,TEXT("HeightmapLodBias"),TRUE);
		LightmapScaleBiasParameter.Bind(ParameterMap,TEXT("LandscapeLightmapScaleBias"),TRUE);
		XYOffsetUVScaleBiasParameter.Bind(ParameterMap, TEXT("XYOffsetUVScaleBias"),TRUE);
		XYOffsetScaleParameter.Bind(ParameterMap, TEXT("XYOffsetScale"),TRUE);
		XYOffsetTextureParameter.Bind(ParameterMap, TEXT("XYOffsetTexture"),TRUE);
	}

	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar)
	{
		Ar << LocalToWorldParameter;
		Ar << WorldToLocalParameter;
		Ar << HeightmapUVScaleBiasParameter;
		Ar << WeightmapUVScaleBiasParameter;
		Ar << HeightmapTextureParameter;
		Ar << LodValuesParameter;
		Ar << LodDistancesValuesParameter;
		Ar << SubsectionSizeQuadsLayerUVPanParameter;
		Ar << HeightmapLodBiasParameter;
		Ar << LightmapScaleBiasParameter;
		Ar << XYOffsetUVScaleBiasParameter;
		Ar << XYOffsetScaleParameter;
		Ar << XYOffsetTextureParameter;

	}

	/**
	* Set any shader data specific to this vertex factory
	*/
	virtual void Set(FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView& View) const
	{
		FLandscapeVertexFactory* LandscapeVF = (FLandscapeVertexFactory*)VertexFactory;
		FLandscapeComponentSceneProxy* SceneProxy = LandscapeVF->GetSceneProxy();
/*
		FLightMapInteraction LMInteract = SceneProxy->ComponentLightInfo->GetLightMapInteraction();
		FVector2D Scale = LMInteract.GetCoordinateScale();
		FVector2D  Bias = LMInteract.GetCoordinateBias();
*/
		if (LightmapScaleBiasParameter.IsBound())
		{
			FLOAT ExtendFactorX = (FLOAT)(SceneProxy->ComponentSizeQuads) * SceneProxy->LightmapScaleBias.X;
			FLOAT ExtendFactorY = (FLOAT)(SceneProxy->ComponentSizeQuads) * SceneProxy->LightmapScaleBias.Y;

			SceneProxy->LightmapScaleBias.W = (FLOAT)(SceneProxy->CurrentSubX) / (FLOAT)(SceneProxy->NumSubsections) * ExtendFactorX + SceneProxy->PatchExpandCount * SceneProxy->LightmapScaleBias.X;
			SceneProxy->LightmapScaleBias.Z = (FLOAT)(SceneProxy->CurrentSubY) / (FLOAT)(SceneProxy->NumSubsections) * ExtendFactorY + SceneProxy->PatchExpandCount * SceneProxy->LightmapScaleBias.Y;

			SetVertexShaderValue(VertexShader->GetVertexShader(),LightmapScaleBiasParameter,SceneProxy->LightmapScaleBias);
		}
	}

	/**
	* 
	*/
	virtual void SetMesh(FShader* VertexShader,const FMeshElement& Mesh,const FSceneView& View) const
	{
		FLandscapeVertexFactory* LandscapeVF = (FLandscapeVertexFactory*)Mesh.VertexFactory;
		FLandscapeComponentSceneProxy* SceneProxy = LandscapeVF->GetSceneProxy();

		if( HeightmapTextureParameter.IsBound() )
		{
			SetVertexShaderTextureParameter(
				VertexShader->GetVertexShader(),
				HeightmapTextureParameter,
				SceneProxy->HeightmapTexture->Resource->TextureRHI);
		}

		if( HeightmapLodBiasParameter.IsBound() )
		{	
			FLOAT HeightmapLodBias = SceneProxy->HeightmapTexture->Mips.Num() - SceneProxy->HeightmapTexture->ResidentMips;
			SetVertexShaderValue(VertexShader->GetVertexShader(),HeightmapLodBiasParameter,HeightmapLodBias);
		}

		if( LodDistancesValuesParameter.IsBound() )
		{
			FVector4 LodDistancesValues;

			if( SceneProxy->CurrentLOD < SceneProxy->MaxLOD )
			{
				LodDistancesValues.X = (View.ViewOrigin.X - SceneProxy->ActorOrigin.X) / (SceneProxy->DrawScaleXY) - (FLOAT)(SceneProxy->SectionBaseX + SceneProxy->SubsectionSizeQuads * SceneProxy->CurrentSubX);
				LodDistancesValues.Y = (View.ViewOrigin.Y - SceneProxy->ActorOrigin.Y) / (SceneProxy->DrawScaleXY) - (FLOAT)(SceneProxy->SectionBaseY + SceneProxy->SubsectionSizeQuads * SceneProxy->CurrentSubY);

				FLOAT LodDistance = appSqrt(2.f * Square(1.f*(FLOAT)SceneProxy->SubsectionSizeQuads)) * LANDSCAPE_LOD_DISTANCE_FACTOR;

				LodDistancesValues.Z = ((FLOAT)SceneProxy->CurrentLOD+0.5f) * LodDistance;
				LodDistancesValues.W = ((FLOAT)SceneProxy->CurrentLOD+1.f) * LodDistance;
			}
			else
			{
				// makes the LOD always negative, so there is no morphing.

				LodDistancesValues.X = 0.f;
				LodDistancesValues.Y = 0.f;
				LodDistancesValues.Z = -1.f;
				LodDistancesValues.W = -2.f;
			}

			SetVertexShaderValue(VertexShader->GetVertexShader(),LodDistancesValuesParameter,LodDistancesValues);
		}

		if (HeightmapUVScaleBiasParameter.IsBound())
		{
			FVector4 HeightmapUVScaleBias = SceneProxy->HeightmapScaleBias;
			HeightmapUVScaleBias.Z += SceneProxy->HeightmapSubsectionOffset * (FLOAT)SceneProxy->CurrentSubX;
			HeightmapUVScaleBias.W += SceneProxy->HeightmapSubsectionOffset * (FLOAT)SceneProxy->CurrentSubY; 
			SetVertexShaderValue(VertexShader->GetVertexShader(),HeightmapUVScaleBiasParameter,HeightmapUVScaleBias);
		}

		if (WeightmapUVScaleBiasParameter.IsBound())
		{
			FVector4 WeightmapUVScaleBias = SceneProxy->WeightmapScaleBias;
			WeightmapUVScaleBias.Z += SceneProxy->WeightmapSubsectionOffset * (FLOAT)SceneProxy->CurrentSubX;
			WeightmapUVScaleBias.W += SceneProxy->WeightmapSubsectionOffset * (FLOAT)SceneProxy->CurrentSubY; 
			SetVertexShaderValue(VertexShader->GetVertexShader(),WeightmapUVScaleBiasParameter,WeightmapUVScaleBias);
		}

		if( LodValuesParameter.IsBound() )
		{
			FVector4 LodValues;
			LodValues.X = (FLOAT)SceneProxy->CurrentLOD;

			// convert current LOD coordinates into highest LOD coordinates
			LodValues.Y = (FLOAT)SceneProxy->SubsectionSizeQuads / (FLOAT)(((SceneProxy->SubsectionSizeVerts) >> SceneProxy->CurrentLOD)-1);

			if( SceneProxy->CurrentLOD < SceneProxy->MaxLOD )
			{
			    // convert highest LOD coordinates into next LOD coordinates.
			    LodValues.Z = (FLOAT)(((SceneProxy->SubsectionSizeVerts) >> (SceneProxy->CurrentLOD+1))-1) / (FLOAT)SceneProxy->SubsectionSizeQuads;
    
			    // convert next LOD coordinates into highest LOD coordinates.
			    LodValues.W = 1.f / LodValues.Z;
			}
			else
			{
				LodValues.Z = 1.f;
				LodValues.W = 1.f;
			}

			SetVertexShaderValue(VertexShader->GetVertexShader(),LodValuesParameter,LodValues);
		}

		if( SubsectionSizeQuadsLayerUVPanParameter.IsBound() )
		{
			FVector4 SubsectionSizeQuadsLayerUVPan(
				(FLOAT)((SceneProxy->SubsectionSizeVerts >> SceneProxy->CurrentLOD) - 1),
				1.f/(FLOAT)((SceneProxy->SubsectionSizeVerts >> SceneProxy->CurrentLOD) - 1),
				SceneProxy->LayerUVPan.X + (FLOAT)(SceneProxy->CurrentSubX * SceneProxy->SubsectionSizeQuads),
				SceneProxy->LayerUVPan.Y + (FLOAT)(SceneProxy->CurrentSubY * SceneProxy->SubsectionSizeQuads)
				);
			SetVertexShaderValue(VertexShader->GetVertexShader(),SubsectionSizeQuadsLayerUVPanParameter,SubsectionSizeQuadsLayerUVPan);
		}

		if( XYOffsetUVScaleBiasParameter.IsBound() )
		{
			FVector4 XYOffsetUVScaleBias = SceneProxy->XYOffsetUVScaleBias;
			XYOffsetUVScaleBias.Z += SceneProxy->XYOffsetSubsectionOffset * (FLOAT)SceneProxy->CurrentSubX;
			XYOffsetUVScaleBias.W += SceneProxy->XYOffsetSubsectionOffset * (FLOAT)SceneProxy->CurrentSubY; 
			SetVertexShaderValue(VertexShader->GetVertexShader(),XYOffsetUVScaleBiasParameter,XYOffsetUVScaleBias);
		}

		if( XYOffsetScaleParameter.IsBound() )
		{
			FLOAT XYOffsetScale = SceneProxy->XYOffsetScale;
			SetVertexShaderValue(VertexShader->GetVertexShader(),XYOffsetScaleParameter,XYOffsetScale);
		}

		if( XYOffsetTextureParameter.IsBound() )
		{
			SetVertexShaderTextureParameter(
				VertexShader->GetVertexShader(),
				XYOffsetTextureParameter,
				SceneProxy->XYOffsetTexture ? SceneProxy->XYOffsetTexture->Resource->TextureRHI : GBlackTexture->TextureRHI);
		}

		SetVertexShaderValue(
			VertexShader->GetVertexShader(),
			LocalToWorldParameter,
			Mesh.LocalToWorld.ConcatTranslation(View.PreViewTranslation)
			);
		SetVertexShaderValue(VertexShader->GetVertexShader(),WorldToLocalParameter,Mesh.WorldToLocal);
	}

private:
	INT	TessellationLevel;
	FShaderParameter LocalToWorldParameter;
	FShaderParameter WorldToLocalParameter;
	FShaderParameter LightmapScaleBiasParameter;
	FShaderParameter HeightmapUVScaleBiasParameter;
	FShaderParameter WeightmapUVScaleBiasParameter;
	FShaderParameter LodValuesParameter;
	FShaderParameter LodDistancesValuesParameter;
	FShaderParameter SubsectionSizeQuadsLayerUVPanParameter;
	FShaderParameter HeightmapLodBiasParameter;
	FShaderParameter XYOffsetUVScaleBiasParameter;
	FShaderParameter XYOffsetScaleParameter;
	FShaderResourceParameter HeightmapTextureParameter;
	FShaderResourceParameter XYOffsetTextureParameter;
};

//
// FLandscapeVertexFactory
//

void FLandscapeVertexFactory::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// position decls
	Elements.AddItem(AccessStreamComponent(Data.PositionComponent, VEU_Position));

	// create the actual device decls
	InitDeclaration(Elements,FVertexFactory::DataType(),FALSE,FALSE);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FLandscapeVertexFactory, FLandscapeVertexFactoryShaderParameters, "LandscapeVertexFactory", TRUE, TRUE, TRUE, FALSE, VER_LANDSCAPEVERTEXFACTORY_ADD_XYOFFSET_PARAMS, 0);
