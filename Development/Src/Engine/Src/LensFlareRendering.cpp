/**
 *	LensFlareRendering.cpp: LensFlare rendering functionality.
 *	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "LensFlare.h"
#include "TileRendering.h"
#include "ScenePrivate.h"

/** Sorting helper */
IMPLEMENT_COMPARE_CONSTREF(FLensFlareElementOrder,LensFlareRendering,{ return A.RayDistance > B.RayDistance ? 1 : -1; });

/**
 *	FLensFlareDynamicData
 */
FLensFlareDynamicData::FLensFlareDynamicData(const ULensFlareComponent* InLensFlareComp, FLensFlareSceneProxy* InProxy) :
	VertexData(NULL)
{
	SourceElement = NULL;
	appMemzero(&Reflections, sizeof(TArrayNoInit<FLensFlareElement*>));

	if (InLensFlareComp && InLensFlareComp->Template)
	{
		ULensFlare* LensFlare = InLensFlareComp->Template;
		check(LensFlare);

		SourceElement = &(LensFlare->SourceElement);
		Reflections.AddZeroed(LensFlare->Reflections.Num());
		for (INT ElementIndex = 0; ElementIndex < LensFlare->Reflections.Num(); ElementIndex++)
		{
			Reflections(ElementIndex) = &(LensFlare->Reflections(ElementIndex));
		}

		INT ElementCount = 1 + LensFlare->Reflections.Num();
		VertexData = new FLensFlareVertex[ElementCount * 4];
	}

	SortElements();

	// Create the vertex factory for rendering the lens flares
	VertexFactory = new FLensFlareVertexFactory();
}

FLensFlareDynamicData::~FLensFlareDynamicData()
{
	// Clean up
	delete [] VertexData;
	delete VertexFactory;
	VertexFactory = NULL;
	Reflections.Empty();
}

DWORD FLensFlareDynamicData::GetMemoryFootprint( void ) const
{
	DWORD CalcSize = 
		sizeof(this) + 
		sizeof(FLensFlareElement) +							// Source
		sizeof(FLensFlareElement) * Reflections.Num() +		// Reflections
		sizeof(FLensFlareVertex) * (1 + Reflections.Num());	// VertexData

	return CalcSize;
}

void FLensFlareDynamicData::InitializeRenderResources(const ULensFlareComponent* InLensFlareComp, FLensFlareSceneProxy* InProxy)
{
	check(IsInGameThread());
	if (VertexFactory)
	{
		BeginInitResource(VertexFactory);
	}
}

void FLensFlareDynamicData::RenderThread_InitializeRenderResources(FLensFlareSceneProxy* InProxy)
{
	check(IsInRenderingThread());
	if (VertexFactory && (VertexFactory->IsInitialized() == FALSE))
	{
		VertexFactory->Init();
	}
}

void FLensFlareDynamicData::ReleaseRenderResources(const ULensFlareComponent* InLensFlareComp, FLensFlareSceneProxy* InProxy)
{
	check(IsInGameThread());
	if (VertexFactory)
	{
		BeginReleaseResource(VertexFactory);
	}
}

void FLensFlareDynamicData::RenderThread_ReleaseRenderResources()
{
	check(IsInRenderingThread());
	if (VertexFactory)
	{
		VertexFactory->Release();
	}
}

/**
 *	Render thread only draw call
 *	
 *	@param	Proxy		The scene proxy for the lens flare
 *	@param	PDI			The PrimitiveDrawInterface
 *	@param	View		The SceneView that is being rendered
 *	@param	DPGIndex	The DrawPrimitiveGroup being rendered
 *	@param	Flags		Rendering flags
 */
void FLensFlareDynamicData::Render(FLensFlareSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex, DWORD Flags)
{
	//@todo. Fill in or remove...
}

/** Render the source element. */
void FLensFlareDynamicData::RenderSource(FLensFlareSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex, DWORD Flags)
{
	// Determine the position of the source
	FVector	SourcePosition = Proxy->GetLocalToWorld().GetOrigin();

	FVector CameraUp	= -View->InvViewProjectionMatrix.TransformNormal(FVector(1.0f,0.0f,0.0f)).SafeNormal();
	FVector CameraRight	= -View->InvViewProjectionMatrix.TransformNormal(FVector(0.0f,1.0f,0.0f)).SafeNormal();

	FVector CameraToSource = View->ViewOrigin - SourcePosition;
	FLOAT DistanceToSource = CameraToSource.Size();

	// Determine the position of the source
	FVector	WorldPosition = SourcePosition;
	FVector ScreenPosition = View->WorldToScreen(WorldPosition);
	FVector2D PixelPosition;
	View->ScreenToPixel(ScreenPosition, PixelPosition);

	FPlane StartTemp = View->Project(WorldPosition);
	FVector StartLine(StartTemp.X, StartTemp.Y, 0.0f);
	FPlane EndTemp(-StartTemp.X, -StartTemp.Y, StartTemp.Z, 1.0f);
	FVector4 EndLine = View->InvViewProjectionMatrix.TransformFVector4(EndTemp);
	EndLine.X /= EndLine.W;
	EndLine.Y /= EndLine.W;
	EndLine.Z /= EndLine.W;
	EndLine.W = 1.0f;
	FVector2D StartPos = FVector2D(StartLine);
	
	FLOAT LocalAspectRatio = View->SizeX / View->SizeY;
	FMaterialRenderProxy* MaterialRenderProxy;
	FVector2D DrawSize;
	FLensFlareElementValues LookupValues;

	FMeshElement Mesh;
	Mesh.UseDynamicData			= TRUE;
	Mesh.IndexBuffer			= NULL;
	Mesh.VertexFactory			= VertexFactory;
	Mesh.DynamicVertexStride	= sizeof(FLensFlareVertex);
	Mesh.DynamicIndexData		= NULL;
	Mesh.DynamicIndexStride		= 0;
	Mesh.LCI					= NULL;
	Mesh.LocalToWorld			= Proxy->GetLocalToWorld();
	Mesh.WorldToLocal			= Proxy->GetWorldToLocal();
	Mesh.FirstIndex				= 0;
	Mesh.MinVertexIndex			= 0;
	Mesh.MaxVertexIndex			= 3;
	Mesh.ParticleType			= PET_None;
	Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
	Mesh.CastShadow				= Proxy->GetCastShadow();
	Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;
	Mesh.NumPrimitives			= 2;
	Mesh.Type					= PT_TriangleFan;

	FLensFlareElement* Element;
	FVector ElementPosition;
	FLensFlareVertex* Vertices;
	FLensFlareVertex ConstantVertexData;
	for (INT ElementIndex = 0; ElementIndex < ElementOrder.Num(); ElementIndex++)
	{
		FLensFlareElementOrder& ElementOrderEntry = ElementOrder(ElementIndex);
		if (ElementOrderEntry.ElementIndex >= 0)
		{
			continue;
		}
		else
		{
			Vertices = &(VertexData[0]);
			Element = SourceElement;
			ElementPosition = StartLine;
		}

		if (Element)
		{
			GetElementValues(ElementPosition, StartLine, View, DistanceToSource, Element, LookupValues);

			if (LookupValues.LFMaterial)
			{
				DrawSize = FVector2D(Element->Size) * LookupValues.Scaling;
				MaterialRenderProxy = LookupValues.LFMaterial->GetRenderProxy(Proxy->GetSelected());

				ConstantVertexData.Position = FVector(0.0f);
				ConstantVertexData.Size.X = DrawSize.X;
				ConstantVertexData.Size.Y = DrawSize.Y;
				ConstantVertexData.Size.Z = LookupValues.AxisScaling.X;
				ConstantVertexData.Size.W = LookupValues.AxisScaling.Y;
				ConstantVertexData.Rotation = LookupValues.Rotation;
				ConstantVertexData.Color = LookupValues.Color;
				if (Element->bModulateColorBySource)
				{
					ConstantVertexData.Color *= Proxy->GetSourceColor();
				}
				ConstantVertexData.RadialDist_SourceRatio.X = LookupValues.RadialDistance;
				ConstantVertexData.RadialDist_SourceRatio.Y = LookupValues.SourceDistance;
				ConstantVertexData.RadialDist_SourceRatio.Z = Element->RayDistance;
				ConstantVertexData.Occlusion_Intensity.X = Proxy->GetCoveragePercentage();
				ConstantVertexData.Occlusion_Intensity.Y = Proxy->GetConeStrength();

				memcpy(&(Vertices[0]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[1]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[2]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[3]), &ConstantVertexData, sizeof(FLensFlareVertex));

				Vertices[0].TexCoord = FVector2D(0.0f, 0.0f);
				Vertices[1].TexCoord = FVector2D(0.0f, 1.0f);
				Vertices[2].TexCoord = FVector2D(1.0f, 1.0f);
				Vertices[3].TexCoord = FVector2D(1.0f, 0.0f);

				Mesh.DynamicVertexData		= Vertices;
				Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;
				Mesh.MaterialRenderProxy	= MaterialRenderProxy;

				DrawRichMesh(
					PDI, 
					Mesh, 
					FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
					FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
					FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
					Proxy->GetPrimitiveSceneInfo(),
					Proxy->GetSelected()
					);
			}
		}
	}
}

/** Render the reflection elements. */
void FLensFlareDynamicData::RenderReflections(FLensFlareSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex, DWORD Flags)
{
	// Determine the position of the source
	FVector	WorldPosition = Proxy->GetLocalToWorld().GetOrigin();
	FVector ScreenPosition = View->WorldToScreen(WorldPosition);
	FVector2D PixelPosition;
	View->ScreenToPixel(ScreenPosition, PixelPosition);

	FVector CameraToSource = View->ViewOrigin - WorldPosition;
	FLOAT DistanceToSource = CameraToSource.Size();

	FPlane StartTemp = View->Project(WorldPosition);
	// Draw a line reflected about the center
	if (Proxy->GetRenderDebug() == TRUE)
	{	
		DrawWireStar(PDI, WorldPosition, 25.0f, FColor(255,0,0), SDPG_Foreground);
	}

	FVector StartLine(StartTemp.X, StartTemp.Y, 0.0f);
	FPlane EndTemp(-StartTemp.X, -StartTemp.Y, StartTemp.Z, 1.0f);
	FVector4 EndLine = View->InvViewProjectionMatrix.TransformFVector4(EndTemp);
	EndLine.X /= EndLine.W;
	EndLine.Y /= EndLine.W;
	EndLine.Z /= EndLine.W;
	EndLine.W = 1.0f;
	FVector2D StartPos = FVector2D(StartLine);
	
	if (Proxy->GetRenderDebug() == TRUE)
	{	
		PDI->DrawLine(WorldPosition, EndLine, FLinearColor(1.0f, 1.0f, 0.0f), SDPG_Foreground);
		DrawWireStar(PDI, EndLine, 25.0f, FColor(0,255,0), SDPG_Foreground);
	}

	FTileRenderer TileRenderer;
	// draw a full-view tile (the TileRenderer uses SizeX, not RenderTargetSizeX)
	check(View->SizeX == View->RenderTargetSizeX);

	FLOAT LocalAspectRatio = View->SizeX / View->SizeY;

	FMaterialRenderProxy* MaterialRenderProxy;
	FVector2D DrawSize;
	FLensFlareElementValues LookupValues;

	FMeshElement Mesh;
	Mesh.UseDynamicData			= TRUE;
	Mesh.IndexBuffer			= NULL;
	Mesh.VertexFactory			= VertexFactory;
	Mesh.DynamicVertexStride	= sizeof(FLensFlareVertex);
	Mesh.DynamicIndexData		= NULL;
	Mesh.DynamicIndexStride		= 0;
	Mesh.LCI					= NULL;
	Mesh.LocalToWorld			= FMatrix::Identity;//Proxy->GetLocalToWorld();
	Mesh.WorldToLocal			= FMatrix::Identity;//Proxy->GetWorldToLocal();
	Mesh.FirstIndex				= 0;
	Mesh.MinVertexIndex			= 0;
	Mesh.MaxVertexIndex			= 3;
	Mesh.ParticleType			= PET_None;
	Mesh.ReverseCulling			= Proxy->GetLocalToWorldDeterminant() < 0.0f ? TRUE : FALSE;
	Mesh.CastShadow				= Proxy->GetCastShadow();
	Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;
	Mesh.NumPrimitives			= 2;
	Mesh.Type					= PT_TriangleFan;

	FLensFlareElement* Element;
	FVector ElementPosition;
	FLensFlareVertex* Vertices;
	FLensFlareVertex ConstantVertexData;
	for (INT ElementIndex = 0; ElementIndex < ElementOrder.Num(); ElementIndex++)
	{
		FLensFlareElementOrder& ElementOrderEntry = ElementOrder(ElementIndex);
		if (ElementOrderEntry.ElementIndex >= 0)
		{
			Vertices = &(VertexData[4 + ElementOrderEntry.ElementIndex* 4]);
			Element = Reflections(ElementOrderEntry.ElementIndex);
			if (Element)
			{
				ElementPosition = FVector(
					((StartPos * (1.0f - Element->RayDistance)) +
					(-StartPos * Element->RayDistance)), 0.0f
					);
			}
		}
		else
		{
			// The source is rendered in RenderSource
			continue;
		}

		if (Element)
		{
			GetElementValues(ElementPosition, StartLine, View, DistanceToSource, Element, LookupValues);

			if (LookupValues.LFMaterial)
			{
				DrawSize = FVector2D(Element->Size) * LookupValues.Scaling;
				MaterialRenderProxy = LookupValues.LFMaterial->GetRenderProxy(Proxy->GetSelected());

				FVector4 ElementProjection = FVector4(ElementPosition.X, ElementPosition.Y, 0.1f, 1.0f);
				FPlane ElementPosition = View->InvViewProjectionMatrix.TransformFVector4(ElementProjection);
				FVector SpritePosition = FVector(ElementPosition.X / ElementPosition.W,
												 ElementPosition.Y / ElementPosition.W,
												 ElementPosition.Z / ElementPosition.W);
				DrawSize /= ElementPosition.W;

				ConstantVertexData.Position = SpritePosition;
				ConstantVertexData.Size.X = DrawSize.X;
				ConstantVertexData.Size.Y = DrawSize.Y;
				ConstantVertexData.Size.Z = LookupValues.AxisScaling.X;
				ConstantVertexData.Size.W = LookupValues.AxisScaling.Y;
				ConstantVertexData.Rotation = LookupValues.Rotation;
				ConstantVertexData.Color = LookupValues.Color;
				if (Element->bModulateColorBySource)
				{
					ConstantVertexData.Color *= Proxy->GetSourceColor();
				}
				ConstantVertexData.RadialDist_SourceRatio.X = LookupValues.RadialDistance;
				ConstantVertexData.RadialDist_SourceRatio.Y = LookupValues.SourceDistance;
				ConstantVertexData.RadialDist_SourceRatio.Z = Element->RayDistance;
				ConstantVertexData.Occlusion_Intensity.X = Proxy->GetCoveragePercentage();
				ConstantVertexData.Occlusion_Intensity.Y = Proxy->GetConeStrength();

				memcpy(&(Vertices[0]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[1]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[2]), &ConstantVertexData, sizeof(FLensFlareVertex));
				memcpy(&(Vertices[3]), &ConstantVertexData, sizeof(FLensFlareVertex));

				Vertices[0].TexCoord = FVector2D(0.0f, 0.0f);
				Vertices[1].TexCoord = FVector2D(0.0f, 1.0f);
				Vertices[2].TexCoord = FVector2D(1.0f, 1.0f);
				Vertices[3].TexCoord = FVector2D(1.0f, 0.0f);

				Mesh.DynamicVertexData		= Vertices;
				Mesh.DepthPriorityGroup		= (ESceneDepthPriorityGroup)DPGIndex;
				Mesh.MaterialRenderProxy	= MaterialRenderProxy;

				DrawRichMesh(
					PDI, 
					Mesh, 
					FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
					FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
					FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
					Proxy->GetPrimitiveSceneInfo(),
					Proxy->GetSelected()
					);
			}
		}
	}
}

/**
 *	Retrieve the various values for the given element
 *
 *	@param	ScreenPosition		The position of the element in screen space
 *	@param	SourcePosition		The position of the source in screen space
 *	@param	View				The SceneView being rendered
 *	@param	Element				The element of interest
 *	@param	Values				The values to fill in
 *
 *	@return	UBOOL				TRUE if successful
 */
UBOOL FLensFlareDynamicData::GetElementValues(FVector ScreenPosition, FVector SourcePosition, const FSceneView* View, 
	FLOAT DistanceToSource, FLensFlareElement* Element, FLensFlareElementValues& Values)
{
	check(Element);

	// The lookup value will be in the range of [0..~1.4] with 
	// 0.0 being right on top of the source (looking directly at the source)
	// 1.0 being on the opposite edge of the screen horizontally or vertically
	// 1.4 being on the opposite edge of the screen diagonally
	FLOAT LookupValue;

	Values.RadialDistance = FVector2D(ScreenPosition.X, ScreenPosition.Y).Size();
	if (Element->bNormalizeRadialDistance)
	{
		// The center point is at 0,0.
		// We need to extend a line through the screen position to the edge of the screen
		// and determine the length, then normal the RadialDistance with that value.
		FVector EdgePosition;
		if (Abs(ScreenPosition.X) > Abs(ScreenPosition.Y))
		{
			EdgePosition.X = 1.0f;
			EdgePosition.Y = Abs(ScreenPosition.Y / ScreenPosition.X);
		}
		else
		{
			EdgePosition.Y = 1.0f;
			EdgePosition.X = Abs(ScreenPosition.X / ScreenPosition.Y);
		}

		Values.RadialDistance /= FVector2D(EdgePosition.X, EdgePosition.Y).Size();
	}
	
	FVector2D SourceTemp = FVector2D(SourcePosition.X - ScreenPosition.X, SourcePosition.Y - ScreenPosition.Y);
	SourceTemp /= 2.0f;
	Values.SourceDistance = SourceTemp.Size();
	if (Element->bUseSourceDistance)
	{
		LookupValue = Values.SourceDistance;
	}
	else
	{
		LookupValue = Values.RadialDistance;
	}

	FVector DistScale_Scale = Element->DistMap_Scale.GetValue(DistanceToSource);
	FVector DistScale_Color = Element->DistMap_Color.GetValue(DistanceToSource);
	FLOAT DistScale_Alpha = Element->DistMap_Alpha.GetValue(DistanceToSource);

	INT MaterialIndex = appTrunc(Element->LFMaterialIndex.GetValue(LookupValue));
	if ((MaterialIndex >= 0) && (MaterialIndex < Element->LFMaterials.Num()))
	{
		Values.LFMaterial = Element->LFMaterials(MaterialIndex);
	}
	else
	{
		Values.LFMaterial = Element->LFMaterials(0);
	}

	if ((Values.LFMaterial == NULL) || (Values.LFMaterial->UseWithLensFlare() == FALSE))
	{
		Values.LFMaterial = GEngine->DefaultMaterial;
	}

	Values.Scaling = Element->Scaling.GetValue(LookupValue);
	Values.AxisScaling = Element->AxisScaling.GetValue(LookupValue) * DistScale_Scale;
	Values.Rotation = Element->Rotation.GetValue(LookupValue);
	FVector Color = Element->Color.GetValue(LookupValue) * DistScale_Color;
	FLOAT Alpha = Element->Alpha.GetValue(LookupValue) * DistScale_Alpha;
	Values.Color = FLinearColor(Color.X, Color.Y, Color.Z, Alpha);
	Values.Offset = Element->Offset.GetValue(LookupValue);

	return FALSE;
}

/**
 *	Sort the contained elements along the ray
 */
void FLensFlareDynamicData::SortElements()
{
	ElementOrder.Empty();
	// Put the source in first...
	if (SourceElement && (SourceElement->LFMaterials.Num() > 0) && SourceElement->LFMaterials(0))
	{
		new(ElementOrder)FLensFlareElementOrder(-1, SourceElement->RayDistance);
	}

	for (INT ElementIndex = 0; ElementIndex < Reflections.Num(); ElementIndex++)
	{
		FLensFlareElement* Element = Reflections(ElementIndex);
		if (Element && (Element->LFMaterials.Num() > 0))
		{
			new(ElementOrder)FLensFlareElementOrder(ElementIndex, Element->RayDistance);
		}
	}
	
	Sort<USE_COMPARE_CONSTREF(FLensFlareElementOrder,LensFlareRendering)>(&(ElementOrder(0)),ElementOrder.Num());
}

//
//	Scene Proxies
//
/** Initialization constructor. */
FLensFlareSceneProxy::FLensFlareSceneProxy(const ULensFlareComponent* Component) :
	  FPrimitiveSceneProxy(Component)
	, Owner(Component->GetOwner())
	, bSelected(Component->IsOwnerSelected())
	, CullDistance(Component->CachedCullDistance > 0 ? Component->CachedCullDistance : WORLD_MAX)
	, bCastShadow(Component->CastShadow)
	, bHasTranslucency(Component->HasUnlitTranslucency())
	, bHasDistortion(Component->HasUnlitDistortion())
	, bUsesSceneColor(bHasTranslucency && Component->UsesSceneColor())
	, OuterCone(Component->OuterCone)
	, InnerCone(Component->InnerCone)
	, ConeFudgeFactor(Component->ConeFudgeFactor)
	, Radius(Component->Radius)
	, ConeStrength(1.0f)
	, SourceColor(Component->SourceColor)
	, DynamicData(NULL)
	, LastDynamicData(NULL)
	, SelectedWireframeMaterialInstance(
		GEngine->WireframeMaterial->GetRenderProxy(FALSE),
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),TRUE)
		)
	, DeselectedWireframeMaterialInstance(
		GEngine->WireframeMaterial->GetRenderProxy(FALSE),
		GetSelectionColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),FALSE)
		)
{
	check(IsInGameThread());
	if (Component->Template == NULL)
	{
		return;
	}

	SourceDPG = Component->Template->SourceDPG;
	ReflectionsDPG = Component->Template->ReflectionsDPG;
	bRenderDebug = Component->Template->bRenderDebugLines;

	ScreenPercentageMap = &(Component->Template->ScreenPercentageMap);

	static const FLOAT BoundsOffset = 1.0f;
	static const FLOAT BoundsScale = 1.1f;
	static const FLOAT BoundsScaledOffset = BoundsOffset * BoundsScale;
	static const FVector BoundsScaledOffsetVector = FVector(1,1,1) * BoundsScaledOffset;
	OcclusionBounds = FBoxSphereBounds(
		Component->Bounds.Origin,
		Component->Bounds.BoxExtent * BoundsScale + BoundsScaledOffsetVector,
		Component->Bounds.SphereRadius * BoundsScale + BoundsScaledOffset
		);

	// Make a local copy of the template elements...
	DynamicData = new FLensFlareDynamicData(Component, this);
	check(DynamicData);
	if (DynamicData)
	{
		DynamicData->InitializeRenderResources(NULL, this);
	}
}

FLensFlareSceneProxy::~FLensFlareSceneProxy()
{
	if (DynamicData)
	{
		check(IsInRenderingThread());
		DynamicData->RenderThread_ReleaseRenderResources();
	}
	delete DynamicData;
	DynamicData = NULL;

	ViewOcclusionResources.Release();
}

/**
 * Draws the primitive's static elements.  This is called from the game thread once when the scene proxy is created.
 * The static elements will only be rendered if GetViewRelevance declares static relevance.
 * Called in the game thread.
 * @param PDI - The interface which receives the primitive elements.
 */
void FLensFlareSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{

}

/** 
 * Draw the scene proxy as a dynamic element
 *
 * @param	PDI - draw interface to render to
 * @param	View - current view
 * @param	DPGIndex - current depth priority 
 * @param	Flags - optional set of flags from EDrawDynamicElementFlags
 */
void FLensFlareSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
	if (View->Family->ShowFlags & SHOW_LensFlares)
	{
		if (DynamicData != NULL)
		{
			// Update the occlusion data every frame
			if (UpdateAndRenderOcclusionData(PDI, View, DPGIndex, Flags) == FALSE)
			{
				return;
			}

			// Check if the flare is relevant, and if so, render it.
			if (CheckViewStatus(View) == FALSE)
			{
				return;
			}

			// Set the occlusion value
			if (DPGIndex == SourceDPG)
			{
				// Render the source in the dynamic data
				DynamicData->RenderSource(this, PDI, View, DPGIndex, Flags);
			}
			
			if (DPGIndex == ReflectionsDPG)
			{

				if ((Flags & DontAllowStaticMeshElementData) == 0)
				{
					// Render each reflection in the dynamic data
					DynamicData->RenderReflections(this, PDI, View, DPGIndex, Flags);
				}
			}
		}

		if ((DPGIndex == SDPG_Foreground) && (View->Family->ShowFlags & SHOW_Bounds) && (GIsGame || !Owner || Owner->IsSelected()))
		{
			// Draw the static mesh's bounding box and sphere.
			DrawWireBox(PDI,PrimitiveSceneInfo->Bounds.GetBox(), FColor(72,72,255),SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,1,0),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
			DrawCircle(PDI,PrimitiveSceneInfo->Bounds.Origin,FVector(0,1,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
		}
	}
}

FPrimitiveViewRelevance FLensFlareSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;

	const EShowFlags ShowFlags = View->Family->ShowFlags;
	if (IsShown(View) && (ShowFlags & SHOW_LensFlares))
	{
		Result.bDynamicRelevance = TRUE;
		Result.bStaticRelevance = FALSE;
		Result.bUsesDynamicMeshElementData = TRUE;

		Result.SetDPG(SourceDPG, TRUE);
		Result.SetDPG(ReflectionsDPG, TRUE);

		if (!(View->Family->ShowFlags & SHOW_Wireframe) && (View->Family->ShowFlags & SHOW_Materials))
		{
			Result.bTranslucentRelevance = bHasTranslucency;
			Result.bDistortionRelevance = bHasDistortion;
			Result.bUsesSceneColor = bUsesSceneColor;
		}
		if (View->Family->ShowFlags & SHOW_Bounds)
		{
			Result.SetDPG(SDPG_Foreground,TRUE);
			Result.bOpaqueRelevance = TRUE;
		}
	}

	// Lens flares never cast shadows...
	Result.bShadowRelevance = FALSE;

	return Result;
}

/** 
 *	Get the results of the last frames occlusion and kick off the one for the next frame
 *
 *	@param	PDI - draw interface to render to
 *	@param	View - current view
 *	@param	DPGIndex - current depth priority 
 *	@param	Flags - optional set of flags from EDrawDynamicElementFlags
 */
UBOOL FLensFlareSceneProxy::UpdateAndRenderOcclusionData(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
	if (View->State == NULL)
	{
		return FALSE;
	}

	FSceneViewState* State = (FSceneViewState*)(View->State);
	check(State);

	FLOAT LocalCoveragePercentage;
	if (State->GetPrimitiveCoveragePercentage(PrimitiveSceneInfo->Component, LocalCoveragePercentage) == TRUE)
	{
		if (ScreenPercentageMap)
		{
			CoveragePercentage = ScreenPercentageMap->GetValue(LocalCoveragePercentage);
		}
		else
		{
			CoveragePercentage = 1.0f;
		}
	}

	return TRUE;
}

/** 
 *	Check if the flare is relevant for this view
 *
 *	@param	View - current view
 */
UBOOL FLensFlareSceneProxy::CheckViewStatus(const FSceneView* View)
{
	UBOOL bResult = TRUE;

	if ((OuterCone != 0.0f) || (Radius != 0.0f))
	{
		UBOOL bIsPerspective = ( View->ProjectionMatrix.M[3][3] < 1.0f );
		if (bIsPerspective == FALSE)
		{
			ConeStrength = 1.0f;
			return TRUE;
		}

		// Determine the line from the world to the camera
		FVector	LFToView = LocalToWorld.GetOrigin() - View->ViewOrigin;

		// Quick radius check... see if within range
		if (Radius != 0.0f)
		{
			FLOAT Distance = LFToView.Size();
			if (Distance > Radius)
			{
				ConeStrength = 0.0f;
				return FALSE;
			}
		}

		// More complex checks for cone
		if (OuterCone != 0.0f)
		{
			// Get the lens flare facing direction and the camera facing direction (in world-space)
			FVector LFDirection = LocalToWorld.GetAxis(0);
			FVector ViewDirection = View->ViewMatrix.Inverse().TransformNormal(FVector(0.0f, 0.0f, 1.0f));

			// Normalize them just in case
			LFDirection.Normalize();
			ViewDirection.Normalize();

			// Normalize the LF to View directional vector
			FVector LFToViewVector = LFToView;
			LFToViewVector.Normalize();

			// Calculate the dot product for both the source and the camera
			FLOAT SourceDot = LFDirection | -LFToViewVector;
			FLOAT CameraDot = ViewDirection | LFToViewVector;

			FVector SourceCross = LFDirection ^ -LFToViewVector;
			FVector CameraCross = ViewDirection ^ LFToViewVector;
			UBOOL bSourceAngleNegative = (SourceCross.Z < 0.0f) ? TRUE : FALSE;
			UBOOL bCameraAngleNegative = (CameraCross.Z < 0.0f) ? TRUE : FALSE;

			// Determine the angle between them
			FLOAT ViewAngle = ViewDirection | LFDirection;

			FLOAT SourceAngleDeg = appAcos(SourceDot) * 180.0f / PI;
			FLOAT CameraAngleDeg = appAcos(CameraDot) * 180.0f / PI;
			FLOAT TotalAngle = 
				(bSourceAngleNegative ? -SourceAngleDeg : SourceAngleDeg) + 
				(bCameraAngleNegative ? -CameraAngleDeg : CameraAngleDeg);
			FLOAT FudgedTotalAngle = TotalAngle * ConeFudgeFactor;

			FLOAT ClampedInnerConeAngle = Clamp(InnerCone,0.0f,89.99f);
			FLOAT ClampedOuterConeAngle = Clamp(OuterCone,ClampedInnerConeAngle + 0.001f,89.99f + 0.001f);

			if (Abs(FudgedTotalAngle) <= ClampedInnerConeAngle)
			{
				ConeStrength = 1.0f;
			}
			else
			if (Abs(FudgedTotalAngle) <= ClampedOuterConeAngle)
			{
				ConeStrength = 1.0f - (Abs(FudgedTotalAngle) - ClampedInnerConeAngle) / (ClampedOuterConeAngle - ClampedInnerConeAngle);
			}
			else
			{
				// Outside the cone!
				ConeStrength = 0.0f;
				bResult = FALSE;
			}
		}
	}
	else
	{
		// No cone active
		ConeStrength = 1.0f;
	}

	return bResult;
}
