/*=============================================================================
	UnModelRender.cpp: Unreal model rendering
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "UnDecalRenderData.h"
#include "LevelUtils.h"
#include "HModel.h"

/*
 * ScenePrivate.h is needed by callbacks from the rendering thread:
 *   UModelComponent::BuildShadowVolume()
 *   UModelSceneProxy::DrawShadowVolumes()
 */
#include "ScenePrivate.h"


/*-----------------------------------------------------------------------------
FModelVertexBuffer
-----------------------------------------------------------------------------*/

FModelVertexBuffer::FModelVertexBuffer(UModel* InModel):
	Model(InModel)
{}

void FModelVertexBuffer::InitRHI()
{
	// Calculate the buffer size.
	UINT Size = Vertices.GetResourceDataSize();
	if( Size > 0 )
	{
		// Create the buffer.
		VertexBufferRHI = RHICreateVertexBuffer(Size,&Vertices,FALSE);
	}
}

/**
* Serializer for this class
* @param Ar - archive to serialize to
* @param B - data to serialize
*/
FArchive& operator<<(FArchive& Ar,FModelVertexBuffer& B)
{
   B.Vertices.BulkSerialize(Ar);
   return Ar;
}

/*-----------------------------------------------------------------------------
FModelShadowVertexBuffer
-----------------------------------------------------------------------------*/

FModelShadowVertexBuffer::FModelShadowVertexBuffer(UModel* InModel)
:	FShadowVertexBuffer(0)
,	Model(InModel)
{
	check( Model );
}

void FModelShadowVertexBuffer::InitRHI()
{
	INT NumVertices = Model->Points.Num();
	if( NumVertices > 0)
	{
		Setup( NumVertices );
		FShadowVertexBuffer::InitRHI();
		UpdateVertices( &Model->Points(0), NumVertices, sizeof(Model->Points(0)) );
	}
}

/*-----------------------------------------------------------------------------
UModelComponent
-----------------------------------------------------------------------------*/

/**
 * Used to sort the model elements by material.
 */
void UModelComponent::BuildRenderData()
{
	UModel* TheModel = GetModel();

	// Initialize the component's light-map resources.
	for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
	{
		FModelElement& Element = Elements(ElementIndex);
		if(Element.LightMap != NULL)
		{
			Element.LightMap->InitResources();
		}
	}

	// Find the edges in UModel::Edges which are adjacent to this component's nodes.
	Edges.Empty();
	for(INT EdgeIndex = 0;EdgeIndex < TheModel->Edges.Num();EdgeIndex++)
	{
		FMeshEdge& Edge = TheModel->Edges(EdgeIndex);
		if(TheModel->Nodes(Edge.Faces[0]).ComponentIndex == ComponentIndex || (Edge.Faces[1] != INDEX_NONE && TheModel->Nodes(Edge.Faces[1]).ComponentIndex == ComponentIndex))
		{
			Edges.AddItem(EdgeIndex);
		}
	}

	// Build the component's index buffer and compute each element's bounding box.
	for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
	{
		FModelElement& Element = Elements(ElementIndex);

		// Find the index buffer for the element's material.
		FRawIndexBuffer32* IndexBuffer = TheModel->MaterialIndexBuffers.Find(Element.Material);
		if(!IndexBuffer)
		{
			IndexBuffer = &TheModel->MaterialIndexBuffers.Set(Element.Material,FRawIndexBuffer32());
		}

		Element.IndexBuffer = IndexBuffer;
		Element.FirstIndex = IndexBuffer->Indices.Num();
		Element.NumTriangles = 0;
		Element.MinVertexIndex = 0xffffffff;
		Element.MaxVertexIndex = 0;
		Element.BoundingBox.Init();
		for(INT NodeIndex = 0;NodeIndex < Element.Nodes.Num();NodeIndex++)
		{
			FBspNode& Node = TheModel->Nodes(Element.Nodes(NodeIndex));
			FBspSurf& Surf = TheModel->Surfs(Node.iSurf);

			// Don't put portal polygons in the static index buffer.
			if(Surf.PolyFlags & PF_Portal)
				continue;

			for(UINT BackFace = 0;BackFace < (UINT)((Surf.PolyFlags & PF_TwoSided) ? 2 : 1);BackFace++)
			{
				if(Node.iZone[1-BackFace] == GetZoneIndex() || GetZoneIndex() == INDEX_NONE)
				{
					for(INT VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
					{
						Element.BoundingBox += TheModel->Points(TheModel->Verts(Node.iVertPool + VertexIndex).pVertex);
					}

					for(INT VertexIndex = 2;VertexIndex < Node.NumVertices;VertexIndex++)
					{
						IndexBuffer->Indices.AddItem(Node.iVertexIndex + Node.NumVertices * BackFace);
						IndexBuffer->Indices.AddItem(Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex);
						IndexBuffer->Indices.AddItem(Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex - 1);
						Element.NumTriangles++;
					}
					Element.MinVertexIndex = Min(Node.iVertexIndex + Node.NumVertices * BackFace,Element.MinVertexIndex);
					Element.MaxVertexIndex = Max(Node.iVertexIndex + Node.NumVertices * BackFace + Node.NumVertices - 1,Element.MaxVertexIndex);
				}
			}
		}

		IndexBuffer->Indices.Shrink();
	}
}

/**
 * Called from the rendering thread to generate shadow volume indices for a particular light.
 * @param IndexBuffer	Index buffer to store the generated shadow volume indices
 * @param Light			The light to generate the shadow volume from
 */
void UModelComponent::BuildShadowVolume( FShadowIndexBuffer &IndexBuffer, const FLightSceneInfo* Light ) const
{
	FVector4 LightPosition = LocalToWorld.Inverse().TransformFVector4(Light->GetPosition());
	WORD FirstExtrudedVertex = Model->Points.Num();
	FLOAT* PlaneDots = new FLOAT[Nodes.Num()];

	IndexBuffer.Indices.Empty();

	for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		PlaneDots[NodeIndex] = Model->Nodes(Nodes(NodeIndex)).Plane | FPlane(LightPosition.X,LightPosition.Y,LightPosition.Z,-LightPosition.W);

	for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
	{
		FBspNode&	Node = Model->Nodes(Nodes(NodeIndex));
		FBspSurf&	Surf = Model->Surfs(Node.iSurf);
		if(!(Surf.PolyFlags & PF_TwoSided))
		{
			if(IsNegativeFloat(PlaneDots[NodeIndex]))
			{
				for(UINT VertexIndex = 2;VertexIndex < Node.NumVertices;VertexIndex++)
				{
					IndexBuffer.AddFace(
						Model->Verts(Node.iVertPool).pVertex,
						Model->Verts(Node.iVertPool + VertexIndex - 1).pVertex,
						Model->Verts(Node.iVertPool + VertexIndex).pVertex
						);
					IndexBuffer.AddFace(
						FirstExtrudedVertex + Model->Verts(Node.iVertPool).pVertex,
						FirstExtrudedVertex + Model->Verts(Node.iVertPool + VertexIndex).pVertex,
						FirstExtrudedVertex + Model->Verts(Node.iVertPool + VertexIndex - 1).pVertex
						);
				}
			}
		}
	}

	for(INT EdgeIndex = 0;EdgeIndex < Edges.Num();EdgeIndex++)
	{
		FMeshEdge& Edge = Model->Edges(Edges(EdgeIndex));
		FBspNode& Node0 = Model->Nodes(Edge.Faces[0]);
		if(Node0.ComponentIndex == ComponentIndex)
		{
			if(Edge.Faces[1] != INDEX_NONE && Model->Nodes(Edge.Faces[1]).ComponentIndex == ComponentIndex)
			{
				if(IsNegativeFloat(PlaneDots[Node0.ComponentNodeIndex]) == IsNegativeFloat(PlaneDots[Model->Nodes(Edge.Faces[1]).ComponentNodeIndex]))
					continue;
			}
			else if(!IsNegativeFloat(PlaneDots[Node0.ComponentNodeIndex]))
				continue;

			IndexBuffer.AddEdge(
				IsNegativeFloat(PlaneDots[Node0.ComponentNodeIndex]) ? Edge.Vertices[0] : Edge.Vertices[1],
				IsNegativeFloat(PlaneDots[Node0.ComponentNodeIndex]) ? Edge.Vertices[1] : Edge.Vertices[0],
				FirstExtrudedVertex
				);
		}
		else if(Edge.Faces[1] != INDEX_NONE)
		{
			FBspNode& Node1 = Model->Nodes(Edge.Faces[1]);
			if(Node1.ComponentIndex == ComponentIndex)
			{
				if(!IsNegativeFloat(PlaneDots[Node1.ComponentNodeIndex]))
					continue;

				IndexBuffer.AddEdge(
					IsNegativeFloat(PlaneDots[Node1.ComponentNodeIndex]) ? Edge.Vertices[1] : Edge.Vertices[0],
					IsNegativeFloat(PlaneDots[Node1.ComponentNodeIndex]) ? Edge.Vertices[0] : Edge.Vertices[1],
					FirstExtrudedVertex
					);
			}
		}
	}

	delete [] PlaneDots;

    IndexBuffer.Indices.Shrink();
}


/**
 * A dynamic model index buffer.
 */
class FModelDynamicIndexBuffer : public FIndexBuffer
{
public:

	FModelDynamicIndexBuffer(UINT InTotalIndices):
		FirstIndex(0),
		NextIndex(0),
		TotalIndices(InTotalIndices)
	{
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(UINT),TotalIndices * sizeof(UINT),NULL,FALSE);
		Lock();
	}

	~FModelDynamicIndexBuffer()
	{
		IndexBufferRHI.Release();
	}

	void AddNode(const UModel* Model,UINT NodeIndex,INT ZoneIndex)
	{
		const FBspNode& Node = Model->Nodes(NodeIndex);
		const FBspSurf& Surf = Model->Surfs(Node.iSurf);

		for(UINT BackFace = 0;BackFace < (UINT)((Surf.PolyFlags & PF_TwoSided) ? 2 : 1);BackFace++)
		{
			if(Node.iZone[1-BackFace] == ZoneIndex || ZoneIndex == INDEX_NONE)
			{
				for(INT VertexIndex = 2;VertexIndex < Node.NumVertices;VertexIndex++)
				{
					*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace;
					*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex;
					*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex - 1;
					NextIndex += 3;
				}
				MinVertexIndex = Min(Node.iVertexIndex + Node.NumVertices * BackFace,MinVertexIndex);
				MaxVertexIndex = Max(Node.iVertexIndex + Node.NumVertices * BackFace + Node.NumVertices - 1,MaxVertexIndex);
			}
		}
	}

	void Draw(
		const UModelComponent* Component,
		BYTE InDepthPriorityGroup,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FLightCacheInterface* LCI,
		FPrimitiveDrawInterface* PDI,
		class FPrimitiveSceneInfo *PrimitiveSceneInfo,
		const FLinearColor& LevelColor,
		const FLinearColor& PropertyColor
		)
	{
		if(NextIndex > FirstIndex)
		{
			RHIUnlockIndexBuffer(IndexBufferRHI);

			FMeshElement MeshElement;
			MeshElement.IndexBuffer = this;
			MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
			MeshElement.MaterialRenderProxy = MaterialRenderProxy;
			MeshElement.LCI = LCI;
			MeshElement.LocalToWorld = Component->LocalToWorld;
			MeshElement.WorldToLocal = Component->LocalToWorld.Inverse();
			MeshElement.FirstIndex = FirstIndex;
			MeshElement.NumPrimitives = (NextIndex - FirstIndex) / 3;
			MeshElement.MinVertexIndex = MinVertexIndex;
			MeshElement.MaxVertexIndex = MaxVertexIndex;
			MeshElement.Type = PT_TriangleList;
			MeshElement.DepthPriorityGroup = InDepthPriorityGroup;
			DrawRichMesh(PDI,MeshElement,FLinearColor::White, LevelColor, PropertyColor, PrimitiveSceneInfo,FALSE);

			FirstIndex = NextIndex;
			Lock();
		}
	}

private:
	UINT FirstIndex;
	UINT NextIndex;
	UINT MinVertexIndex;
	UINT MaxVertexIndex;
	UINT TotalIndices;
	UINT* Indices;
	
	void Lock()
	{
		if(NextIndex < TotalIndices)
		{
			Indices = (UINT*)RHILockIndexBuffer(IndexBufferRHI,FirstIndex * sizeof(UINT),TotalIndices * sizeof(UINT) - FirstIndex * sizeof(UINT));
			MaxVertexIndex = 0;
			MinVertexIndex = MAXINT;
		}
	}
};

IMPLEMENT_COMPARE_CONSTPOINTER( FDecalInteraction, UnModelRender,
{
	return (A->DecalState.SortOrder <= B->DecalState.SortOrder) ? -1 : 1;
} );

/**
 * A model component scene proxy.
 */
class FModelSceneProxy : public FPrimitiveSceneProxy
{
public:

	FModelSceneProxy(const UModelComponent* InComponent):
		FPrimitiveSceneProxy(InComponent),
		Component(InComponent),
		LevelColor(255,255,255),
		PropertyColor(255,255,255)
	{
		const TIndirectArray<FModelElement>& SourceElements = Component->GetElements();

		Elements.Empty(SourceElements.Num());
		for(INT ElementIndex = 0;ElementIndex < SourceElements.Num();ElementIndex++)
		{
			const FModelElement& SourceElement = SourceElements(ElementIndex);
			FElementInfo* Element = new(Elements) FElementInfo(SourceElement);
			MaterialViewRelevance |= Element->GetMaterial()->GetViewRelevance();
		}

		// Try to find a color for level coloration.
		UObject* ModelOuter = InComponent->GetModel()->GetOuter();
		ULevel* Level = Cast<ULevel>( ModelOuter );
		if ( Level )
		{
			ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
			if ( LevelStreaming )
			{
				LevelColor = LevelStreaming->DrawColor;
			}
		}

		// Get a color for property coloration.
		GEngine->GetPropertyColorationColor( (UObject*)InComponent, PropertyColor );
	}

	virtual HHitProxy* CreateHitProxies(const UPrimitiveComponent*,TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
	{
		HHitProxy* ModelHitProxy = new HModel(Component->GetModel());
		OutHitProxies.AddItem(ModelHitProxy);
		return ModelHitProxy;
	}

	/**
	 * Draws the primitive's decal elements.  This is called from the rendering thread for each frame of each view.
	 * The dynamic elements will only be rendered if GetViewRelevance declares decal relevance.
	 * Called in the rendering thread.
	 *
	 * @param	Context							The RHI command context to which the primitives are being rendered.
	 * @param	OpaquePDI						The interface which receives the opaque primitive elements.
	 * @param	TranslucentPDI					The interface which receives the translucent primitive elements.
	 * @param	View							The view which is being rendered.
	 * @param	InDepthPriorityGroup				The DPG which is being rendered.
	 * @param	bTranslucentReceiverPass		TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
	 */
	virtual void DrawDecalElements(
		FCommandContextRHI* Context, 
		FPrimitiveDrawInterface* OpaquePDI, 
		FPrimitiveDrawInterface* TranslucentPDI, 
		const FSceneView* View, 
		UINT InDepthPriorityGroup, 
		UBOOL bTranslucentReceiverPass
		)
	{
		SCOPE_CYCLE_COUNTER(STAT_DecalRenderTime);
		checkSlow( View->Family->ShowFlags & SHOW_Decals );

		if ( (!bTranslucentReceiverPass && MaterialViewRelevance.bTranslucency) || (bTranslucentReceiverPass && !MaterialViewRelevance.bTranslucency) )
		{
			return;
		}

		// Determine the DPG the primitive should be drawn in for this view.
		FPrimitiveViewRelevance ViewRel = GetViewRelevance(View);
		for (UINT DPGIndex = 0; DPGIndex < SDPG_MAX_SceneRender; DPGIndex++)
		{
			if (ViewRel.GetDPG(DPGIndex) == FALSE)
			{
				continue;
			}

			// Compute the set of decals in this DPG.
			TArray<FDecalInteraction*> DPGDecals;
			for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
			{
				FDecalInteraction* Interaction = Decals(DecalIndex);
				if ( InDepthPriorityGroup == Interaction->DecalState.DepthPriorityGroup )
				{
					if ( !Interaction->DecalState.MaterialViewRelevance.bLit )
					{
						DPGDecals.AddItem( Interaction );
					}
				}
			}
			// Sort and render all decals.
			Sort<USE_COMPARE_CONSTPOINTER(FDecalInteraction,UnModelRender)>( DPGDecals.GetTypedData(), DPGDecals.Num() );
			for ( INT DecalIndex = 0 ; DecalIndex < DPGDecals.Num() ; ++DecalIndex )
			{
				const FDecalInteraction* Decal	= DPGDecals(DecalIndex);
				const FDecalState& DecalState	= Decal->DecalState;

				// Compute a scissor rect by clipping the decal frustum vertices to the screen.
				// Don't draw the decal if the frustum projects off the screen.
				FVector2D MinCorner;
				FVector2D MaxCorner;
				if ( DecalState.QuadToClippedScreenSpaceAABB( View, MinCorner, MaxCorner ) )
				{
					// Set the decal scissor rect.
					RHISetScissorRect( Context, TRUE, appTrunc(MinCorner.X), appTrunc(MinCorner.Y), appTrunc(MaxCorner.X), appTrunc(MaxCorner.Y) );

					const FDecalRenderData* RenderData = Decal->RenderData;

					FMeshElement MeshElement;
					MeshElement.IndexBuffer = &RenderData->IndexBuffer;
					MeshElement.VertexFactory = &RenderData->VertexFactory;
					MeshElement.MaterialRenderProxy = DecalState.DecalMaterial->GetRenderProxy(FALSE);
					MeshElement.LCI = RenderData->LCI;
					MeshElement.LocalToWorld = DecalState.DecalFrame;
					MeshElement.WorldToLocal = DecalState.WorldToDecal;
					MeshElement.FirstIndex = 0;
					MeshElement.NumPrimitives = RenderData->NumTriangles;
					MeshElement.MinVertexIndex = 0;
					MeshElement.MaxVertexIndex = RenderData->GetNumVertices()-1;
					MeshElement.CastShadow = FALSE;
					MeshElement.DepthBias = DecalState.DepthBias;
					MeshElement.SlopeScaleDepthBias = DecalState.SlopeScaleDepthBias;
					MeshElement.Type = PT_TriangleList;
					MeshElement.DepthPriorityGroup = DPGIndex;

					FPrimitiveDrawInterface* PDI;
					if ( Decal->DecalState.MaterialViewRelevance.bTranslucency )
					{
						PDI = TranslucentPDI;
					}
					else
					{
						RHISetBlendState( Context, TStaticBlendState<>::GetRHI() );
						PDI = OpaquePDI;
					}

					INC_DWORD_STAT_BY(STAT_DecalTriangles,MeshElement.NumPrimitives);
					INC_DWORD_STAT(STAT_DecalDrawCalls);
					DrawRichMesh(PDI,MeshElement,FLinearColor(0.5f,1.0f,0.5f),LevelColor,PropertyColor,PrimitiveSceneInfo,FALSE);
				}
			}
		}

		// Restore the scissor rect.
		RHISetScissorRect( Context, FALSE, 0, 0, 0, 0 );
	}

	/**
	 * Draws the primitive's lit decal elements.  This is called from the rendering thread for each frame of each view.
	 * The dynamic elements will only be rendered if GetViewRelevance declares dynamic relevance.
	 * Called in the rendering thread.
	 *
	 * @param	Context					The RHI command context to which the primitives are being rendered.
	 * @param	PDI						The interface which receives the primitive elements.
	 * @param	View					The view which is being rendered.
	 * @param	InDepthPriorityGroup		The DPG which is being rendered.
	 * @param	bDrawingDynamicLights	TRUE if drawing dynamic lights, FALSE if drawing static lights.
	 */
	virtual void DrawLitDecalElements(
		FCommandContextRHI* Context,
		FPrimitiveDrawInterface* PDI,
		const FSceneView* View,
		UINT InDepthPriorityGroup,
		UBOOL bDrawingDynamicLights
		)
	{
		SCOPE_CYCLE_COUNTER(STAT_DecalRenderTime);
		checkSlow( View->Family->ShowFlags & SHOW_Decals );

		// Compute the set of decals in this DPG.
		TArray<FDecalInteraction*> DPGDecals;
		for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
		{
			FDecalInteraction* Interaction = Decals(DecalIndex);
			if ( InDepthPriorityGroup == Interaction->DecalState.DepthPriorityGroup )
			{
				if ( Interaction->DecalState.MaterialViewRelevance.bLit )
				{
					DPGDecals.AddItem( Interaction );
				}
			}
		}
		// Sort and render all decals.
		Sort<USE_COMPARE_CONSTPOINTER(FDecalInteraction,UnModelRender)>( DPGDecals.GetTypedData(), DPGDecals.Num() );
		for ( INT DecalIndex = 0 ; DecalIndex < DPGDecals.Num() ; ++DecalIndex )
		{
			const FDecalInteraction* Decal	= DPGDecals(DecalIndex);
			const FDecalState& DecalState	= Decal->DecalState;

			// Compute a scissor rect by clipping the decal frustum vertices to the screen.
			// Don't draw the decal if the frustum projects off the screen.
			FVector2D MinCorner;
			FVector2D MaxCorner;
			if ( DecalState.QuadToClippedScreenSpaceAABB( View, MinCorner, MaxCorner ) )
			{
				// Don't override the light's scissor rect
				if (!bDrawingDynamicLights)
				{
					// Set the decal scissor rect.
					RHISetScissorRect( Context, TRUE, appTrunc(MinCorner.X), appTrunc(MinCorner.Y), appTrunc(MaxCorner.X), appTrunc(MaxCorner.Y) );
				}

				const FDecalRenderData* RenderData = Decal->RenderData;

				FMeshElement MeshElement;
				MeshElement.IndexBuffer = &RenderData->IndexBuffer;
				MeshElement.VertexFactory = &RenderData->VertexFactory;
				MeshElement.MaterialRenderProxy = DecalState.DecalMaterial->GetRenderProxy(FALSE);
				// BSP uses the render data's Data field to store the model element index.
				MeshElement.LCI = &Elements(RenderData->Data);
				MeshElement.LocalToWorld = DecalState.DecalFrame;
				MeshElement.WorldToLocal = DecalState.WorldToDecal;
				MeshElement.FirstIndex = 0;
				MeshElement.NumPrimitives = RenderData->NumTriangles;
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = RenderData->GetNumVertices()-1;
				MeshElement.CastShadow = FALSE;
				MeshElement.DepthBias = DecalState.DepthBias;
				MeshElement.SlopeScaleDepthBias = DecalState.SlopeScaleDepthBias;
				MeshElement.Type = PT_TriangleList;
				MeshElement.DepthPriorityGroup = InDepthPriorityGroup;

				INC_DWORD_STAT_BY(STAT_DecalTriangles,MeshElement.NumPrimitives);
				INC_DWORD_STAT(STAT_DecalDrawCalls);
				DrawRichMesh(PDI,MeshElement,FLinearColor(0.5f,1.0f,0.5f),LevelColor,PropertyColor,PrimitiveSceneInfo,FALSE);
			}
		}

		if (!bDrawingDynamicLights)
		{
			// Restore the scissor rect.
			RHISetScissorRect( Context, FALSE, 0, 0, 0, 0 );
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
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
	{
		const UBOOL bDynamicBSPTriangles = (View->Family->ShowFlags & SHOW_Selection) || IsRichView(View) || IsCollisionView(View);
		
		// Determine the DPG the primitive should be drawn in for this view.
		if (GetViewRelevance(View).GetDPG(DPGIndex) == FALSE)
		{
			return;
		}

		// Draw the BSP triangles.
		if((View->Family->ShowFlags & SHOW_BSPTriangles) && (View->Family->ShowFlags & SHOW_BSP) && bDynamicBSPTriangles)
		{
			FLinearColor UtilColor = LevelColor;
			if( IsCollisionView(View) )
			{
				UtilColor = GEngine->C_BSPCollision;
			}

			if(View->Family->ShowFlags & SHOW_Selection)
			{
				UINT TotalIndices = 0;
				for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
				{
					const FModelElement& ModelElement = Component->GetElements()(ElementIndex);
					TotalIndices += ModelElement.NumTriangles * 3;
				}

				if(TotalIndices > 0)
				{
					FModelDynamicIndexBuffer IndexBuffer(TotalIndices);
					for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
					{
						const FModelElement& ModelElement = Component->GetElements()(ElementIndex);
						if(ModelElement.NumTriangles > 0)
						{
							const FElementInfo& ProxyElementInfo = Elements(ElementIndex);
							for(UINT bSelected = 0;bSelected < 2;bSelected++)
							{
								for(INT NodeIndex = 0;NodeIndex < ModelElement.Nodes.Num();NodeIndex++)
								{
									FBspNode& Node = Component->GetModel()->Nodes(ModelElement.Nodes(NodeIndex));
									FBspSurf& Surf = Component->GetModel()->Surfs(Node.iSurf);

									// Don't draw portal polygons.
									if(Surf.PolyFlags & PF_Portal)
										continue;

									if(((Surf.PolyFlags & PF_Selected) ? TRUE : FALSE) == bSelected)
									{
										IndexBuffer.AddNode(Component->GetModel(),ModelElement.Nodes(NodeIndex),Component->GetZoneIndex());
									}
								}
								IndexBuffer.Draw(
									Component,
									DPGIndex,
									ProxyElementInfo.GetMaterial()->GetRenderProxy(bSelected),
									&ProxyElementInfo,
									PDI,
									PrimitiveSceneInfo,
									UtilColor,
									PropertyColor
									);
							}
						}
					}
				}
			}
			else
			{
				for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
				{
					const FModelElement& ModelElement = Component->GetElements()(ElementIndex);
					if(ModelElement.NumTriangles > 0)
					{
						FMeshElement MeshElement;
						MeshElement.IndexBuffer = ModelElement.IndexBuffer;
						MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
						MeshElement.MaterialRenderProxy = Elements(ElementIndex).GetMaterial()->GetRenderProxy(FALSE);
						MeshElement.LCI = &Elements(ElementIndex);
						MeshElement.LocalToWorld = Component->LocalToWorld;
						MeshElement.WorldToLocal = Component->LocalToWorld.Inverse();
						MeshElement.FirstIndex = ModelElement.FirstIndex;
						MeshElement.NumPrimitives = ModelElement.NumTriangles;
						MeshElement.MinVertexIndex = ModelElement.MinVertexIndex;
						MeshElement.MaxVertexIndex = ModelElement.MaxVertexIndex;
						MeshElement.Type = PT_TriangleList;
						MeshElement.DepthPriorityGroup = DPGIndex;
						DrawRichMesh(PDI,MeshElement,FLinearColor::White,UtilColor,FLinearColor::White,PrimitiveSceneInfo,FALSE);
					}
				}
			}
		}
	}

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
	{
		if(!HasViewDependentDPG())
		{
			// Determine the DPG the primitive should be drawn in.
			BYTE PrimitiveDPG = GetStaticDepthPriorityGroup();

			for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
			{
				const FModelElement& ModelElement = Component->GetElements()(ElementIndex);
				if(ModelElement.NumTriangles > 0)
				{
					FMeshElement MeshElement;
					MeshElement.IndexBuffer = ModelElement.IndexBuffer;
					MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
					MeshElement.MaterialRenderProxy = Elements(ElementIndex).GetMaterial()->GetRenderProxy(FALSE);
					MeshElement.LCI = &Elements(ElementIndex);
					MeshElement.LocalToWorld = Component->LocalToWorld;
					MeshElement.WorldToLocal = Component->LocalToWorld.Inverse();
					MeshElement.FirstIndex = ModelElement.FirstIndex;
					MeshElement.NumPrimitives = ModelElement.NumTriangles;
					MeshElement.MinVertexIndex = ModelElement.MinVertexIndex;
					MeshElement.MaxVertexIndex = ModelElement.MaxVertexIndex;
					MeshElement.Type = PT_TriangleList;
					MeshElement.DepthPriorityGroup = PrimitiveDPG;
					PDI->DrawMesh(MeshElement,0,WORLD_MAX);
				}
			}
		}
	}

	/**
	 * Draws the primitive's shadow volumes.  This is called from the rendering thread,
	 * in the FSceneRenderer::RenderLights phase.
	 * @param SVDI - The interface which performs the actual rendering of a shadow volume.
	 * @param View - The view which is being rendered.
	 * @param Light - The light for which shadows should be drawn.
	 * @param DPGIndex - The depth priority group the light is being drawn for.
	 */
	virtual void DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI, const FSceneView* View, const FLightSceneInfo* Light,UINT DPGIndex)
	{
		checkSlow(UEngine::ShadowVolumesAllowed());

		// Determine the DPG the primitive should be drawn in for this view.
		if (GetViewRelevance(View).GetDPG(DPGIndex) == FALSE)
		{
			return;
		}

		SCOPE_CYCLE_COUNTER(STAT_ShadowVolumeRenderTime);

		// Check for the shadow volume in the cache.
		const FShadowVolumeCache::FCachedShadowVolume* CachedShadowVolume = CachedShadowVolumes.GetShadowVolume(Light);
		if (!CachedShadowVolume)
		{
			SCOPE_CYCLE_COUNTER(STAT_ShadowExtrusionTime);

			FShadowIndexBuffer IndexBuffer;
			Component->BuildShadowVolume( IndexBuffer, Light );

			// Add the new shadow volume to the cache.
			CachedShadowVolume = CachedShadowVolumes.AddShadowVolume(Light,IndexBuffer);
		}

		// Draw the cached shadow volume.
		if(CachedShadowVolume->NumTriangles)
		{
			INC_DWORD_STAT_BY(STAT_ShadowVolumeTriangles, CachedShadowVolume->NumTriangles);
			INT NumVertices = Component->GetModel()->Points.Num();
			const FLocalShadowVertexFactory &ShadowVertexFactory = Component->GetModel()->ShadowVertexBuffer.GetVertexFactory();
			SVDI->DrawShadowVolume( CachedShadowVolume->IndexBufferRHI, ShadowVertexFactory, Component->LocalToWorld, 0, CachedShadowVolume->NumTriangles, 0, NumVertices * 2 - 1 );
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		if((View->Family->ShowFlags & SHOW_BSPTriangles) && (View->Family->ShowFlags & SHOW_BSP))
		{
			if(IsShown(View))
			{
				if((View->Family->ShowFlags & SHOW_Selection) || IsRichView(View) || IsCollisionView(View) || HasViewDependentDPG())
				{
					Result.bDynamicRelevance = TRUE;
				}
				else
				{
					Result.bStaticRelevance = TRUE;
				}
				Result.SetDPG(GetDepthPriorityGroup(View),TRUE);
				Result.bDecalRelevance = HasRelevantDecals(View);
			}
			if (IsShadowCast(View))
			{
				Result.bShadowRelevance = TRUE;
			}
			MaterialViewRelevance.SetPrimitiveViewRelevance(Result);
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
	virtual void GetLightRelevance(FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped)
	{
		// Attach the light to the primitive's static meshes.
		bDynamic = TRUE;
		bRelevant = FALSE;
		bLightMapped = TRUE;

		if (Elements.Num() > 0)
		{
			for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
			{
				FElementInfo* LCI = &Elements(ElementIndex);
				if (LCI)
				{
					ELightInteractionType InteractionType = LCI->GetInteraction(LightSceneInfo).GetType();
					if(InteractionType != LIT_CachedIrrelevant)
					{
						bRelevant = TRUE;
						if(InteractionType != LIT_CachedLightMap)
						{
							bLightMapped = FALSE;
						}
						if(InteractionType != LIT_Uncached)
						{
							bDynamic = FALSE;
						}
					}
				}
			}
		}
		else
		{
			bRelevant = TRUE;
			bLightMapped = FALSE;
		}
	}

	/**
	 * Called by the rendering thread to notify the proxy when a light is no longer
	 * associated with the proxy, so that it can clean up any cached resources.
	 * @param Light - The light to be removed.
	 */
	virtual void OnDetachLight(const FLightSceneInfo* Light)
	{
		CachedShadowVolumes.RemoveShadowVolume(Light);
	}
	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const 
	{ 
		DWORD AdditionalSize = FPrimitiveSceneProxy::GetAllocatedSize();

		AdditionalSize += Elements.GetAllocatedSize();

		return( AdditionalSize ); 
	}

private:

	/** Cached shadow volumes. */
	FShadowVolumeCache CachedShadowVolumes;

	const UModelComponent* Component;

	class FElementInfo: public FLightCacheInterface
	{
	public:

		/** Initialization constructor. */
		FElementInfo(const FModelElement& ModelElement):
			Bounds(ModelElement.BoundingBox)
		{
			const UBOOL bHasStaticLighting = ModelElement.LightMap != NULL || ModelElement.ShadowMaps.Num();

			// Determine the material applied to the model element.
			Material = ModelElement.Material;

			// If there isn't an applied material, or if we need static lighting and it doesn't support it, fall back to the default material.
			if(!ModelElement.Material || (bHasStaticLighting && !ModelElement.Material->UseWithStaticLighting()))
			{
				Material = GEngine->DefaultMaterial;
			}

			// Build the static light interaction map.
			for(INT LightIndex = 0;LightIndex < ModelElement.IrrelevantLights.Num();LightIndex++)
			{
				StaticLightInteractionMap.Set(ModelElement.IrrelevantLights(LightIndex),FLightInteraction::Irrelevant());
			}
			LightMap = ModelElement.LightMap;
			if(LightMap)
			{
				for(INT LightIndex = 0;LightIndex < LightMap->LightGuids.Num();LightIndex++)
				{
					StaticLightInteractionMap.Set(LightMap->LightGuids(LightIndex),FLightInteraction::LightMap());
				}
			}
			for(INT LightIndex = 0;LightIndex < ModelElement.ShadowMaps.Num();LightIndex++)
			{
				UShadowMap2D* ShadowMap = ModelElement.ShadowMaps(LightIndex);
				if(ShadowMap && ShadowMap->IsValid())
				{
					StaticLightInteractionMap.Set(
						ShadowMap->GetLightGuid(),
						FLightInteraction::ShadowMap2D(
							ShadowMap->GetTexture(),
							ShadowMap->GetCoordinateScale(),
							ShadowMap->GetCoordinateBias()
							)
						);
				}
			}
		}

		// FLightCacheInterface.
		virtual FLightInteraction GetInteraction(const FLightSceneInfo* LightSceneInfo) const
		{
			// Check for a static light interaction.
			const FLightInteraction* Interaction = StaticLightInteractionMap.Find(LightSceneInfo->LightmapGuid);
			if(!Interaction)
			{
				Interaction = StaticLightInteractionMap.Find(LightSceneInfo->LightGuid);
			}
			if(Interaction)
			{
				return *Interaction;
			}
			
			// Cull the uncached light against the bounding box of the element.
			if( LightSceneInfo->AffectsBounds(Bounds) )
			{
				return FLightInteraction::Uncached();
			}
			else
			{
				return FLightInteraction::Irrelevant();
			}
		}
		virtual FLightMapInteraction GetLightMapInteraction() const
		{
			return LightMap ? LightMap->GetInteraction() : FLightMapInteraction();
		}

		// Accessors.
		UMaterialInterface* GetMaterial() const { return Material; }

	private:

		/** The element's material. */
		UMaterialInterface* Material;

		/** A map from persistent light IDs to information about the light's interaction with the model element. */
		TMap<FGuid,FLightInteraction> StaticLightInteractionMap;

		/** The light-map used by the element. */
		const FLightMap* LightMap;

		/** The element's bounding volume. */
		FBoxSphereBounds Bounds;
	};

	TArray<FElementInfo> Elements;

	FColor LevelColor;
	FColor PropertyColor;

	FMaterialViewRelevance MaterialViewRelevance;
};

FPrimitiveSceneProxy* UModelComponent::CreateSceneProxy()
{
	return ::new FModelSceneProxy(this);
}

UBOOL UModelComponent::ShouldRecreateProxyOnUpdateTransform() const
{
	return TRUE;
}

void UModelComponent::UpdateBounds()
{
	if(Model)
	{
		FBox	BoundingBox(0);
		for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			FBspNode& Node = Model->Nodes(Nodes(NodeIndex));
			for(INT VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
			{
				BoundingBox += Model->Points(Model->Verts(Node.iVertPool + VertexIndex).pVertex);
			}
		}
		Bounds = FBoxSphereBounds(BoundingBox.TransformBy(LocalToWorld));
	}
	else
	{
		Super::UpdateBounds();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Decals on BSP.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

FDecalRenderData* UModelComponent::GenerateDecalRenderData(FDecalState* Decal) const
{
	SCOPE_CYCLE_COUNTER(STAT_DecalBSPAttachTime);

	// Do nothing if the specified decal doesn't project on BSP.
	if ( !Decal->bProjectOnBSP )
	{
		return NULL;
	}

	// Return value.
	FDecalRenderData* DecalRenderData = NULL;

	// Temporaries. Moved out of loop to avoid reallocations for Poly case and costly recomputation.
	FDecalPoly Poly;
	FVector2D TempTexCoords;
	FDecalLocalSpaceInfo DecalInfo;
	FMatrix LocalToDecal;
	UBOOL bAlreadyInitializedDecalData = FALSE;

	// Iterate over all potential nodes.
	for( INT HitNodeIndexIndex=0; HitNodeIndexIndex<Decal->HitNodeIndices.Num(); HitNodeIndexIndex++ )
	{
		INT HitNodeIndex = Decal->HitNodeIndices(HitNodeIndexIndex);

		const FBspNode& Node	= Model->Nodes( HitNodeIndex );
		const ULevel*	Level	= CastChecked<ULevel>(GetOuter());

		// Nothing to do if requested node is not part of this model component
		if( Level->ModelComponents( Node.ComponentIndex ) != this )
		{
			continue;
		}

		// Don't attach to invisible or portal surfaces.
		const FBspSurf& Surf = Model->Surfs( Node.iSurf );
		if ( (Surf.PolyFlags & PF_Portal) || (Surf.PolyFlags & PF_Invisible) )
		{
			continue;
		}

		INT ComponentElementIndex = Node.ComponentElementIndex;

		// BSP built before the addition of ComponentElementIndex needs some fixups.
		if( ComponentElementIndex == INDEX_NONE )
		{
			for( INT ElementIndex=0; ElementIndex<Elements.Num(); ElementIndex++ )
			{
				if( Elements(ElementIndex).Nodes.FindItemIndex( HitNodeIndex ) != INDEX_NONE )
				{
					ComponentElementIndex = ElementIndex;
					break;
				}
			}
		}
		check( ComponentElementIndex != INDEX_NONE );
	
		// Transform decal properties into local space. Deferred till actually needed.
		if( !bAlreadyInitializedDecalData )
		{
			DecalInfo.Init( Decal, LocalToWorld );
			LocalToDecal = LocalToWorld * Decal->WorldToDecal;
			bAlreadyInitializedDecalData = TRUE;
		}

		// Don't perform software clipping for dynamic decals or when requested.
		const UBOOL bNoClip = Decal->bNoClip; //!Decal->DecalComponent->bStaticDecal || Decal->bNoClip;

		// Discard if the polygon faces away from the decal.
		// The dot product is inverted because BSP polygon winding is clockwise.
		const FLOAT Dot = -(DecalInfo.LocalLookVector | Node.Plane);
		const UBOOL bIsFrontFacing = Decal->bFlipBackfaceDirection ? -Dot > Decal->DecalComponent->BackfaceAngle : Dot > Decal->DecalComponent->BackfaceAngle;

		// Even if backface culling is disabled, reject triangles that view the decal at grazing angles.
		if ( bIsFrontFacing || ( Decal->bProjectOnBackfaces && Abs( Dot ) > Decal->DecalComponent->BackfaceAngle ) )
		{
			// Copy off the vertices into a temporary poly for clipping.
			Poly.Init();
			const INT FirstVertexIndex = Node.iVertPool;
			for ( INT VertexIndex = 0 ; VertexIndex < Node.NumVertices ; ++VertexIndex )
			{
				const FVert& ModelVert = Model->Verts( FirstVertexIndex + VertexIndex );
				new(Poly.Vertices) FVector( Model->Points( ModelVert.pVertex ) );
				new(Poly.ShadowTexCoords) FVector2D( ModelVert.ShadowTexCoord );
				Poly.Indices.AddItem( FirstVertexIndex + VertexIndex );
			}

			// Clip against the decal. Don't need to check in the 
			const UBOOL bClipPassed = bNoClip ? TRUE : Poly.ClipAgainstConvex( DecalInfo.Convex );
			if ( bClipPassed )
			{
				// Allocate a FDecalRenderData object if we haven't already.
				if ( !DecalRenderData )
				{
					DecalRenderData = new FDecalRenderData( NULL, TRUE, TRUE );
					// Store the model element index.
					DecalRenderData->Data = ComponentElementIndex;
				}

				const FVector TangentX( Model->Vectors(Surf.vTextureU).SafeNormal() );
				const FVector TangentY( Model->Vectors(Surf.vTextureV).SafeNormal() );

				const DWORD DecalFirstVertexIndex = DecalRenderData->GetNumVertices();

				checkSlow( Poly.Vertices.Num() == Poly.ShadowTexCoords.Num() );
				for ( INT i = 0 ; i < Poly.Vertices.Num() ; ++i )
				{
					// Generate texture coordinates for the vertex using the decal tangents.
					DecalInfo.ComputeTextureCoordinates( Poly.Vertices(i), TempTexCoords );

					const FVector TangentZ(Node.Plane);
					FPackedNormal TangentZPacked(TangentZ);
					// store sign of determinant in normal w component
					TangentZPacked.Vector.W = GetBasisDeterminantSign(TangentX,TangentY,TangentZ) < 0 ? 0 : 255;

					// Store the decal vertex.
					new(DecalRenderData->Vertices) FDecalVertex(LocalToDecal.TransformFVector( Poly.Vertices(i) ),
																TangentX,
																TangentZPacked,
																TempTexCoords,
																Poly.ShadowTexCoords( i ),
																FVector2D( 1.0f, 0.0f ),
																FVector2D( 0.0f, 1.0f ) );
				}

				// Triangulate the polygon and add indices to the index buffer
				for ( INT i = 0 ; i < Poly.Vertices.Num() - 2 ; ++i )
				{
					DecalRenderData->AddIndex( DecalFirstVertexIndex+0 );
					DecalRenderData->AddIndex( DecalFirstVertexIndex+i+2 );
					DecalRenderData->AddIndex( DecalFirstVertexIndex+i+1 );
				}
			}
		}
	}

	// Finalize the data.
	if( DecalRenderData )
	{
		DecalRenderData->NumTriangles = DecalRenderData->GetNumIndices()/3;
	}

	return DecalRenderData;
}
