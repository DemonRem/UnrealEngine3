/*=============================================================================
	SpeedTreeComponent.cpp: SpeedTreeComponent implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"
#include "EngineMaterialClasses.h"
#include "ScenePrivate.h"
#if WITH_NOVODEX
#include "UnNovodexSupport.h"
#endif
#include "SpeedTree.h"

IMPLEMENT_CLASS(USpeedTreeComponent);

#if WITH_SPEEDTREE

extern CSpeedTreeRT::SGeometry GSpeedTreeGeometry;

/** Represents the static lighting of a SpeedTreeComponent's mesh to the scene manager. */
class FSpeedTreeLCI : public FLightCacheInterface
{
public:

	/** Initialization constructor. */
	FSpeedTreeLCI(const USpeedTreeComponent* InComponent,ESpeedTreeMeshType InMeshType):
		Component(InComponent),
		MeshType(InMeshType)
	{}

	// FLightCacheInterface
	virtual FLightInteraction GetInteraction(const class FLightSceneInfo* LightSceneInfo) const
	{
		// Check if the light is in the light-map.
		const FLightMap* LightMap = 
			ChooseByMeshType<FLightMap*>(
				MeshType,
				Component->BranchAndFrondLightMap,
				Component->LeafMeshLightMap,
				Component->LeafCardLightMap,
				Component->BillboardLightMap
				);
		if(LightMap)
		{
			if(LightMap->LightGuids.ContainsItem(LightSceneInfo->LightmapGuid))
			{
				return FLightInteraction::LightMap();
			}
		}

		// Check whether we have static lighting for the light.
		for(INT LightIndex = 0;LightIndex < Component->StaticLights.Num();LightIndex++)
		{
			const FSpeedTreeStaticLight& StaticLight = Component->StaticLights(LightIndex);

			if(StaticLight.Guid == LightSceneInfo->LightGuid)
			{
				const UShadowMap1D* ShadowMap = 
					ChooseByMeshType<UShadowMap1D*>(
						MeshType,
						StaticLight.BranchAndFrondShadowMap,
						StaticLight.LeafMeshShadowMap,
						StaticLight.LeafCardShadowMap,
						StaticLight.BillboardShadowMap
						);

				if(ShadowMap)
				{
					return FLightInteraction::ShadowMap1D(ShadowMap);
				}
				else
				{
					return FLightInteraction::Irrelevant();
				}
			}
		}

		return FLightInteraction::Uncached();
	}
	virtual FLightMapInteraction GetLightMapInteraction() const
	{
		const FLightMap* LightMap =
			ChooseByMeshType<FLightMap*>(
				MeshType,
				Component->BranchAndFrondLightMap,
				Component->LeafMeshLightMap,
				Component->LeafCardLightMap,
				Component->BillboardLightMap
				);
		if(LightMap)
		{
			return LightMap->GetInteraction();
		}
		else
		{
			return FLightMapInteraction();
		}
	}

private:

	const USpeedTreeComponent* const Component;
	ESpeedTreeMeshType MeshType;
};

class FSpeedTreeSceneProxy : public FPrimitiveSceneProxy
{
public:

	/**
	 * Chooses the material to render on the tree.  The InstanceMaterial is preferred, then the ArchetypeMaterial, then the default material.
	 * If a material doesn't have the necessary shaders, it falls back to the next less preferred material.
	 * @return The most preferred material that has the necessary shaders for rendering a SpeedTree.
	 */
	static const UMaterialInterface* GetSpeedTreeMaterial(UMaterialInterface* InstanceMaterial,UMaterialInterface* ArchetypeMaterial,UBOOL bHasStaticLighting,FMaterialViewRelevance& MaterialViewRelevance)
	{
		UMaterialInterface* Result = NULL;

		// Try the instance's material first.
		if(InstanceMaterial && InstanceMaterial->UseWithSpeedTree() && (!bHasStaticLighting || InstanceMaterial->UseWithStaticLighting()))
		{
			Result = InstanceMaterial;
		}

		// If that failed, try the archetype's material.
		if(!Result && ArchetypeMaterial && ArchetypeMaterial->UseWithSpeedTree() && (!bHasStaticLighting || ArchetypeMaterial->UseWithStaticLighting()))
		{
			Result = ArchetypeMaterial;
		}

		// If both failed, use the default material.
		if(!Result)
		{
			Result = GEngine->DefaultMaterial;
		}

		// Update the material relevance information from the resulting material.
		MaterialViewRelevance |= Result->GetViewRelevance();

		return Result;
	}

	FSpeedTreeSceneProxy( USpeedTreeComponent* InComponent ) 
	:	FPrimitiveSceneProxy( InComponent )
	,	SpeedTree( InComponent->SpeedTree )
	,	Component( InComponent )
	,	Bounds( InComponent->Bounds )
	,	RotatedLocalToWorld( InComponent->RotationOnlyMatrix.Inverse() * InComponent->LocalToWorld )
	,	RotationOnlyMatrix( InComponent->RotationOnlyMatrix )
	,	LevelColor(255,255,255)
	,	PropertyColor(255,255,255)
	,	LodNearDistance(InComponent->LodNearDistance)
	,	LodFarDistance(InComponent->LodFarDistance)
	,	LodLevelOverride(InComponent->LodLevelOverride)
	,	bUseLeaves(InComponent->bUseLeaves)
	,	bUseBranches(InComponent->bUseBranches)
	,	bUseFronds(InComponent->bUseFronds)
	,	bUseBillboards(InComponent->bUseBillboards)
	,	bCastShadow(InComponent->CastShadow)
	,	bSelected(InComponent->IsOwnerSelected())
	,	bShouldCollide(InComponent->ShouldCollide())
	,	bBlockZeroExtent(InComponent->BlockZeroExtent)
	,	bBlockNonZeroExtent(InComponent->BlockNonZeroExtent)
	,	bBlockRigidBody(InComponent->BlockRigidBody)
	{
		const UBOOL bHasStaticLighting = Component->BranchAndFrondLightMap != NULL || Component->StaticLights.Num();

		// Make sure applied materials have been compiled with the speed tree vertex factory.
		BranchMaterial = GetSpeedTreeMaterial(Component->BranchMaterial,SpeedTree->BranchMaterial,bHasStaticLighting,MaterialViewRelevance);
		FrondMaterial = GetSpeedTreeMaterial(Component->FrondMaterial,SpeedTree->FrondMaterial,bHasStaticLighting,MaterialViewRelevance);
		LeafMaterial = GetSpeedTreeMaterial(Component->LeafMaterial,SpeedTree->LeafMaterial,bHasStaticLighting,MaterialViewRelevance);
		BillboardMaterial = GetSpeedTreeMaterial(Component->BillboardMaterial,SpeedTree->BillboardMaterial,bHasStaticLighting,MaterialViewRelevance);
	}

	/** Determines if any collision should be drawn for this mesh. */
	UBOOL ShouldDrawCollision(const FSceneView* View)
	{
		if((View->Family->ShowFlags & SHOW_CollisionNonZeroExtent) && bBlockNonZeroExtent && bShouldCollide)
		{
			return TRUE;
		}

		if((View->Family->ShowFlags & SHOW_CollisionZeroExtent) && bBlockZeroExtent && bShouldCollide)
		{
			return TRUE;
		}	

		if((View->Family->ShowFlags & SHOW_CollisionRigidBody) && bBlockRigidBody)
		{
			return TRUE;
		}

		return FALSE;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		if(View->Family->ShowFlags & SHOW_SpeedTrees)
		{
			if( IsShown(View) )
			{
				Result.bDynamicRelevance = TRUE;

				Result.SetDPG( GetDepthPriorityGroup(View), TRUE );

				// only add to foreground DPG for debug rendering
				if(View->Family->ShowFlags & SHOW_Bounds)
				{
					Result.SetDPG(SDPG_Foreground,TRUE);
				}
			}
			if (IsShadowCast(View))
			{
				Result.bShadowRelevance = TRUE;
			}
			
			// Replicate the material relevance flags into the resulting primitive view relevance's material flags.
			MaterialViewRelevance.SetPrimitiveViewRelevance(Result);
		}
		return Result;
	}

	virtual void GetLightRelevance(FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped)
	{
		// Use the FSpeedTreeLCI to find the light's interaction type.
		// Assume that light relevance is the same for all mesh types.
		FSpeedTreeLCI SpeedTreeLCI(Component,STMT_BranchesAndFronds);
		const ELightInteractionType InteractionType = SpeedTreeLCI.GetInteraction(LightSceneInfo).GetType();

		// Attach the light to the primitive's static meshes.
		bDynamic = (InteractionType == LIT_Uncached);
		bRelevant = (InteractionType != LIT_CachedIrrelevant);
		bLightMapped = (InteractionType == LIT_CachedLightMap || InteractionType == LIT_CachedIrrelevant);
	}

	void DrawMeshElement( const FSceneView* View, FPrimitiveDrawInterface* PDI, UINT DPGIndex, FMeshElement* Element, const UMaterialInterface* Material, const USpeedTreeComponent* InComponent, FLOAT AlphaMaskValue, ESpeedTreeMeshType MeshType )
	{
		if( Element->NumPrimitives > 0 )
		{
			FSpeedTreeLCI SpeedTreeLCI(InComponent,MeshType);

			Element->LCI = &SpeedTreeLCI;
			Element->LocalToWorld = LocalToWorld;
			Element->WorldToLocal = LocalToWorld.Inverse();
			Element->DepthPriorityGroup	= DPGIndex;

			if( Material == NULL )
			{
				Element->MaterialRenderProxy = GEngine->DefaultMaterial->GetRenderProxy(bSelected);
			}
			else
			{
				((FSpeedTreeVertexFactory*)Element->VertexFactory)->SetAlphaAdjustment((84.0f - AlphaMaskValue) / 255.0f);
				((FSpeedTreeBranchVertexFactory*)Element->VertexFactory)->SetRotationOnlyMatrix(RotationOnlyMatrix);

				if (MeshType != STMT_Billboards)
				{
					((FSpeedTreeBranchVertexFactory*)Element->VertexFactory)->SetWindMatrixOffset(Component->WindMatrixOffset);
					((FSpeedTreeBranchVertexFactory*)Element->VertexFactory)->SetWindMatrices(SpeedTree->SRH->SpeedWind.GetWindMatrix(0));
				}

				if (MeshType == STMT_LeafCards || MeshType == STMT_LeafMeshes)
				{
					// set leaf rock/rustle angles
					FLOAT* pRockAngles = ((FSpeedTreeLeafCardVertexFactory*)Element->VertexFactory)->GetLeafRockAngles( );
					FLOAT* pRustleAngles = ((FSpeedTreeLeafCardVertexFactory*)Element->VertexFactory)->GetLeafRustleAngles( );
					SpeedTree->SRH->SpeedWind.GetRockAngles(pRockAngles);
					SpeedTree->SRH->SpeedWind.GetRockAngles(pRustleAngles);

					// convert degrees to radians
					for (UINT AngleIndex = 0; AngleIndex < 3; ++AngleIndex)
					{
						pRockAngles[AngleIndex] *= PI / 180.0f;
						pRustleAngles[AngleIndex] *= PI / 180.0f;
					}

					((FSpeedTreeLeafCardVertexFactory*)Element->VertexFactory)->SetLeafAngleScalars(SpeedTree->SRH->LeafAngleScalars);
					((FSpeedTreeLeafCardVertexFactory*)Element->VertexFactory)->SetCameraAlignMatrix(CachedCameraToWorld);
				}

				Element->MaterialRenderProxy = Material->GetRenderProxy(bSelected);
			}

			const FLinearColor ColWireframeColor(0.3f, 1.0f, 0.3f);
			DrawRichMesh( PDI, *Element, ColWireframeColor, LevelColor, PropertyColor, PrimitiveSceneInfo, bSelected );
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
		if( GetViewRelevance(View).GetDPG(DPGIndex) == TRUE )
		{
			const UBOOL bIsCollisionView = IsCollisionView(View);
			const UBOOL bDrawCollision = bIsCollisionView && ShouldDrawCollision(View);
			const UBOOL bDrawMesh = !bIsCollisionView;

			if(bDrawMesh)
			{
				// Set the current camera position and direction.
				CachedCameraToWorld = View->ViewMatrix.Inverse();
				FVector CurrentCameraOrigin = CachedCameraToWorld.GetOrigin();
				FVector CurrentCameraZ = CachedCameraToWorld.GetAxis(2);
				CachedCameraToWorld.SetOrigin(FVector(0.0f, 0.0f, 0.0f));
				CSpeedTreeRT::SetCamera(&CurrentCameraOrigin.X, &CurrentCameraZ.X);

				// set the position & transform of the component
				FVector Origin(LocalToWorld.GetOrigin());
				SpeedTree->SRH->SpeedTree->SetTreePosition(Origin.X, Origin.Y, Origin.Z);

				// Update wind if time has passed.
				SpeedTree->SRH->UpdateWind(FVector(1.0f, 0.0f, 0.0f), 0.5f, View->Family->CurrentWorldTime);

				// Update the geometry if the camera has moved.
				SpeedTree->SRH->UpdateGeometry(CurrentCameraOrigin,CurrentCameraZ,TRUE);

				// override the LOD level if parameter is set
				if( LodLevelOverride == -1.0f )
				{
					SpeedTree->SRH->SpeedTree->SetLodLimits( LodNearDistance, LodFarDistance );
					SpeedTree->SRH->SpeedTree->ComputeLodLevel();
				}
				else
				{
					SpeedTree->SRH->SpeedTree->SetLodLevel( LodLevelOverride );
				}

				// Access the LOD's rendering parameters.
				CSpeedTreeRT::SLodValues LodValues;
				SpeedTree->SRH->SpeedTree->GetLodValues(LodValues);

				// render leaves
				if( LodValues.m_anLeafActiveLods[0] > -1 && bUseLeaves && SpeedTree->SRH->bHasLeaves && GSystemSettings.bAllowSpeedTreeLeaves )
				{
					// leaf cards
					DrawMeshElement(
						View, 
						PDI, 
						DPGIndex,
						&SpeedTree->SRH->LeafCardElements(LodValues.m_anLeafActiveLods[0]),
						LeafMaterial,
						Component,
						LodValues.m_afLeafAlphaTestValues[0],
						STMT_LeafCards
						);

					// may need to draw 2 for LOD blending
					if( LodValues.m_anLeafActiveLods[1] > -1 )
					{
						DrawMeshElement(
							View, 
							PDI, 
							DPGIndex,
							&SpeedTree->SRH->LeafCardElements(LodValues.m_anLeafActiveLods[1]),
							LeafMaterial,
							Component,
							LodValues.m_afLeafAlphaTestValues[1],
							STMT_LeafCards
							);
					}

					// leaf meshes
					DrawMeshElement(
						View, 
						PDI, 
						DPGIndex,
						&SpeedTree->SRH->LeafMeshElements(LodValues.m_anLeafActiveLods[0]),
						LeafMaterial,
						Component,
						LodValues.m_afLeafAlphaTestValues[0],
						STMT_LeafMeshes
						);

					if( LodValues.m_anLeafActiveLods[1] > -1 )
					{
						DrawMeshElement(
							View, 
							PDI, 
							DPGIndex,
							&SpeedTree->SRH->LeafMeshElements(LodValues.m_anLeafActiveLods[1]),
							LeafMaterial,
							Component,
							LodValues.m_afLeafAlphaTestValues[1],
							STMT_LeafMeshes
							);
					}
				}

				// render fronds
				if( LodValues.m_nFrondActiveLod > -1 && bUseFronds && SpeedTree->SRH->bHasFronds && GSystemSettings.bAllowSpeedTreeFronds )
				{
					DrawMeshElement(
						View, 
						PDI, 
						DPGIndex,
						&SpeedTree->SRH->FrondElements(LodValues.m_nFrondActiveLod),
						FrondMaterial,
						Component,
						LodValues.m_fFrondAlphaTestValue,
						STMT_BranchesAndFronds
						);
				}

				// render branches
				if( LodValues.m_nBranchActiveLod > -1 && bUseBranches && SpeedTree->SRH->bHasBranches )
				{
					DrawMeshElement(
						View, 
						PDI, 
						DPGIndex,
						&SpeedTree->SRH->BranchElements(LodValues.m_nBranchActiveLod),
						BranchMaterial,
						Component,
						LodValues.m_fBranchAlphaTestValue,
						STMT_BranchesAndFronds
						);
				}

				// render billboards
				if( bUseBillboards )
				{
					if( LodValues.m_fBillboardFadeOut > 0.0f )
					{
						// compensate for shader rotation
						CurrentCameraZ = RotationOnlyMatrix.TransformNormal(CurrentCameraZ);
						CSpeedTreeRT::SetCamera( &CurrentCameraOrigin.X, &CurrentCameraZ.X );
						SpeedTree->SRH->UpdateBillboards( );

						// draw billboards
						UBOOL bHasHorizBillboard = GSpeedTreeGeometry.m_sHorzBillboard.m_pTexCoords != NULL;

						if( bHasHorizBillboard && GSpeedTreeGeometry.m_sHorzBillboard.m_fAlphaTestValue < 255.0f )
						{
							DrawMeshElement(
								View, 
								PDI,
								DPGIndex,
								&SpeedTree->SRH->BillboardElements[2],
								BillboardMaterial,
								Component,
								GSpeedTreeGeometry.m_sHorzBillboard.m_fAlphaTestValue,
								STMT_Billboards
								);
						}

						if( !bHasHorizBillboard || GSpeedTreeGeometry.m_sHorzBillboard.m_fAlphaTestValue > 84.0f )
						{
							DrawMeshElement(
								View, 
								PDI, 
								DPGIndex,
								&SpeedTree->SRH->BillboardElements[0],
								BillboardMaterial,
								Component, 
								GSpeedTreeGeometry.m_s360Billboard.m_afAlphaTestValues[0],
								STMT_Billboards
								);

							if( GSpeedTreeGeometry.m_s360Billboard.m_afAlphaTestValues[1] < 255.0f )
							{
								DrawMeshElement(
									View, 
									PDI, 
									DPGIndex,
									&SpeedTree->SRH->BillboardElements[1],
									BillboardMaterial,
									Component,
									GSpeedTreeGeometry.m_s360Billboard.m_afAlphaTestValues[1],
									STMT_Billboards
									);
							}
						}
					}
				}
			}

			if(bDrawCollision)
			{
				// Draw the speedtree's collision model
				INT		NumCollision = SpeedTree->SRH->GetNumCollisionPrimitives();
				FLOAT	UniformScale = LocalToWorld.GetAxis(0).Size();
				
				for( INT i=0; i<NumCollision; i++ )
				{
					CSpeedTreeRT::ECollisionObjectType Type;
					FVector Pos;
					FVector Dim;
					FVector EulerAngles;
					
					SpeedTree->SRH->GetCollisionPrimitive( i, Type, Pos, Dim, EulerAngles );
					Dim *= UniformScale;

					switch(Type)
					{
					case CSpeedTreeRT::CO_CAPSULE:
						{
							EulerAngles.X = -EulerAngles.X;
							EulerAngles.Y = -EulerAngles.Y;

							FMatrix MatUnscaledL2W(RotatedLocalToWorld);
							MatUnscaledL2W.RemoveScaling( );
							MatUnscaledL2W.SetOrigin(FVector(0.0f, 0.0f, 0.0f));
							FRotationMatrix MatRotate(FRotator::MakeFromEuler(EulerAngles));
							MatRotate *= MatUnscaledL2W;

							DrawWireCylinder(
								PDI, 
								RotatedLocalToWorld.TransformFVector(Pos) + MatRotate.TransformFVector(FVector(0, 0, Dim.Y * 0.5f)), 
								MatRotate.TransformNormal(FVector(1, 0, 0)), 
								MatRotate.TransformNormal(FVector(0, 1, 0)), 
								MatRotate.TransformNormal(FVector(0, 0, 1)), 
								GEngine->C_ScaleBoxHi, 
								Dim.X, 
								Dim.Y * 0.5f, 
								24, 
								DPGIndex
								);
						}
						break;
					case CSpeedTreeRT::CO_SPHERE:
						{
							FVector VecTransformed = RotatedLocalToWorld.TransformFVector(Pos);
							DrawCircle( PDI, VecTransformed, FVector(1, 0, 0), FVector(0, 1, 0), GEngine->C_ScaleBoxHi, Dim.X, 24, DPGIndex);
							DrawCircle( PDI, VecTransformed, FVector(1, 0, 0), FVector(0, 0, 1), GEngine->C_ScaleBoxHi, Dim.X, 24, DPGIndex);
							DrawCircle( PDI, VecTransformed, FVector(0, 1, 0), FVector(0, 0, 1), GEngine->C_ScaleBoxHi, Dim.X, 24, DPGIndex);
						}	
						break;
					case CSpeedTreeRT::CO_BOX:
						// boxes not supported
						break;
					default:
						break;
					}
				}
			}
		}
	
		// Render the bounds if the actor is selected.
		if( (View->Family->ShowFlags & SHOW_Bounds) && bSelected )
		{
			// Draw the tree's bounding box and sphere.
			DrawWireBox( PDI, Bounds.GetBox( ), FColor(72, 72, 255), SDPG_Foreground);
			DrawCircle( PDI, Bounds.Origin, FVector(1, 0, 0), FVector(0, 1, 0), FColor(255, 255, 0), Bounds.SphereRadius, 24, SDPG_Foreground);
			DrawCircle( PDI, Bounds.Origin, FVector(1, 0, 0), FVector(0, 0, 1), FColor(255, 255, 0), Bounds.SphereRadius, 24, SDPG_Foreground);
			DrawCircle( PDI, Bounds.Origin, FVector(0, 1, 0), FVector(0, 0, 1), FColor(255, 255, 0), Bounds.SphereRadius, 24, SDPG_Foreground);
		}
	}

	void BuildShadowVolume( const FLightSceneInfo* Light, FShadowVolumeCache& ShadowVolumeCache )
	{
		SCOPE_CYCLE_COUNTER(STAT_ShadowExtrusionTime);

		FVector4 LightPosition = LocalToWorld.Inverse().TransformFVector4(Light->GetPosition());
		WORD FirstExtrusionVertex = SpeedTree->SRH->BranchFrondPositionBuffer.Vertices.Num( ) / 2;
		FShadowIndexBuffer IndexBuffer;

		for( INT BranchesOrFronds=0; BranchesOrFronds<2; BranchesOrFronds++ )
		{
			// Only draw branch shadows.
			static const UBOOL bDrawBranchShadows = TRUE;
			static const UBOOL bDrawFrondShadows = FALSE;
			if(	(BranchesOrFronds == 0 && !bDrawBranchShadows) ||
				(BranchesOrFronds == 1 && !bDrawFrondShadows)
				)
			{
				continue;
			}

			INT EdgeStart	= 0;
			INT EdgeEnd		= SpeedTree->SRH->NumBranchEdges;
			FMeshElement* Section = &SpeedTree->SRH->BranchElements(0);
			if( BranchesOrFronds > 0 )
			{
				EdgeStart	= SpeedTree->SRH->NumBranchEdges;
				EdgeEnd		= SpeedTree->SRH->ShadowEdges.Num();
				Section		= &SpeedTree->SRH->FrondElements(0);
				if( !bUseFronds || !SpeedTree->SRH->bHasFronds )
				{
					continue;
				}
			}
			else if( !bUseBranches || !SpeedTree->SRH->bHasBranches )
			{
				continue;
			}

			if( Section == NULL || Section->NumPrimitives == 0 )
			{
				continue;
			}

			INT NumTriangles = Section->NumPrimitives;
			IndexBuffer.Indices.Reserve(IndexBuffer.Indices.Num( ) + NumTriangles * 3);

			// calculate triangle-light dot products
			FLOAT* PlaneDots = new FLOAT[NumTriangles];
			WORD* Indices = &SpeedTree->SRH->IndexBuffer.Indices(Section[0].FirstIndex);
			for( INT TriangleIndex=0; TriangleIndex<NumTriangles; TriangleIndex++ )
			{
				const FVector& V1 = SpeedTree->SRH->BranchFrondPositionBuffer.Vertices(Indices[TriangleIndex * 3 + 0]).Position;
				const FVector& V2 = SpeedTree->SRH->BranchFrondPositionBuffer.Vertices(Indices[TriangleIndex * 3 + 1]).Position;
				const FVector& V3 = SpeedTree->SRH->BranchFrondPositionBuffer.Vertices(Indices[TriangleIndex * 3 + 2]).Position;
				PlaneDots[TriangleIndex] = ((V2 - V3) ^ (V1 - V3)) | (FVector(LightPosition) - V1 * LightPosition.W);
			}

			// render volume faces (double sided)
			for( INT TriangleIndex=0; TriangleIndex<NumTriangles; TriangleIndex++ )
			{
				WORD Offset = IsNegativeFloat(PlaneDots[TriangleIndex]) ? FirstExtrusionVertex : 0;
				IndexBuffer.AddFace( Offset + Indices[TriangleIndex * 3 + 0],
					Offset + Indices[TriangleIndex * 3 + 1],
					Offset + Indices[TriangleIndex * 3 + 2]);

				Offset = FirstExtrusionVertex - Offset;
				IndexBuffer.AddFace( Offset + Indices[TriangleIndex * 3 + 2],
					Offset + Indices[TriangleIndex * 3 + 1],
					Offset + Indices[TriangleIndex * 3 + 0]);
			}

			// render volume edges (double sided)
			for( INT EdgeIndex=EdgeStart; EdgeIndex<EdgeEnd; EdgeIndex++ )
			{
				FMeshEdge& Edge = SpeedTree->SRH->ShadowEdges(EdgeIndex);
				if( Edge.Faces[1] == INDEX_NONE || (IsNegativeFloat(PlaneDots[Edge.Faces[0]]) != IsNegativeFloat(PlaneDots[Edge.Faces[1]])))
				{
					IndexBuffer.AddEdge(Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 1 : 0],
						Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 0 : 1],
						FirstExtrusionVertex);

					if( Edge.Faces[1] != INDEX_NONE )
					{
						IndexBuffer.AddEdge(Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 1 : 0],
							Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 0 : 1],
							FirstExtrusionVertex);
					}
				}
			}

			delete [] PlaneDots;
		}

		ShadowVolumeCache.AddShadowVolume(Light, IndexBuffer);
	}

	virtual void DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI, const FSceneView* View, const FLightSceneInfo* Light, UINT DPGIndex) 
	{
		SCOPE_CYCLE_COUNTER(STAT_ShadowVolumeRenderTime);

		const FShadowVolumeCache::FCachedShadowVolume* CachedShadowVolume = CachedShadowVolumes.GetShadowVolume(Light);

		// didn't find one, make a new one
		if( CachedShadowVolume == NULL )
		{
			BuildShadowVolume( Light, CachedShadowVolumes );
			CachedShadowVolume = CachedShadowVolumes.GetShadowVolume(Light);
		}

		// Draw the cached shadow volume
		if( CachedShadowVolume != NULL && CachedShadowVolume->NumTriangles > 0 )
		{
			INC_DWORD_STAT_BY(STAT_ShadowVolumeTriangles, CachedShadowVolume->NumTriangles);
			SVDI->DrawShadowVolume(
				CachedShadowVolume->IndexBufferRHI, 
				SpeedTree->SRH->ShadowVertexFactory, 
				LocalToWorld, 
				0, 
				CachedShadowVolume->NumTriangles, 
				0, 
				SpeedTree->SRH->BranchFrondPositionBuffer.Vertices.Num()
				);
		}
	}
	virtual void OnDetachLight(const FLightSceneInfo* Light)
	{
		CachedShadowVolumes.RemoveShadowVolume(Light);
	}

	virtual DWORD GetMemoryFootprint() const
	{ 
		return (sizeof(*this) + GetAllocatedSize()); 
	}
	DWORD GetAllocatedSize() const
	{ 
		return FPrimitiveSceneProxy::GetAllocatedSize(); 
	}
	virtual EMemoryStats GetMemoryStatType() const
	{ 
		return STAT_GameToRendererMallocOther; 
	}

private:

	USpeedTree*				SpeedTree;
	USpeedTreeComponent*	Component;
	FShadowVolumeCache		CachedShadowVolumes;
	FBoxSphereBounds		Bounds;

	FMatrix CachedCameraToWorld;
	FMatrix RotatedLocalToWorld;
	FMatrix RotationOnlyMatrix;

	const UMaterialInterface* BranchMaterial;
	const UMaterialInterface* FrondMaterial;
	const UMaterialInterface* LeafMaterial;
	const UMaterialInterface* BillboardMaterial;

	const FColor LevelColor;
	const FColor PropertyColor;

	const FLOAT LodNearDistance;
	const FLOAT LodFarDistance;
	const FLOAT LodLevelOverride;

	const BITFIELD bUseLeaves : 1;
	const BITFIELD bUseBranches : 1;
	const BITFIELD bUseFronds : 1;
	const BITFIELD bUseBillboards : 1;

	const BITFIELD bCastShadow : 1;
	const BITFIELD bSelected : 1;
	const BITFIELD bShouldCollide : 1;
	const BITFIELD bBlockZeroExtent : 1;
	const BITFIELD bBlockNonZeroExtent : 1;
	const BITFIELD bBlockRigidBody : 1;

	/** The view relevance for all the primitive's materials. */
	FMaterialViewRelevance MaterialViewRelevance;
};

FPrimitiveSceneProxy* USpeedTreeComponent::CreateSceneProxy(void)
{
	return ::new FSpeedTreeSceneProxy(this);
}

void USpeedTreeComponent::UpdateBounds( )
{
	// bounds for tree
	if( SpeedTree == NULL || !SpeedTree->IsInitialized() )
	{
		if( SpeedTreeIcon != NULL )
		{
			// bounds for icon
			const FLOAT IconScale = (Owner ? Owner->DrawScale : 1.0f) * (SpeedTreeIcon ? (FLOAT)Max(SpeedTreeIcon->SizeX, SpeedTreeIcon->SizeY) : 1.0f);
			Bounds = FBoxSphereBounds(LocalToWorld.GetOrigin( ), FVector(IconScale, IconScale, IconScale), appSqrt(3.0f * Square(IconScale)));
		}
		else
		{
			Super::UpdateBounds( );
		}
	}
	else
	{
		// speedtree bounds
		Bounds = SpeedTree->SRH->GetBounds( ).TransformBy(RotationOnlyMatrix.Inverse() * LocalToWorld);
		Bounds.BoxExtent += FVector(1.0f, 1.0f, 1.0f);
		Bounds.SphereRadius += 1.0f;
	}
}

UBOOL USpeedTreeComponent::PointCheck(FCheckResult& Result, const FVector& Location, const FVector& Extent, DWORD TraceFlags )
{
	if( SpeedTree == NULL || !SpeedTree->IsInitialized() )
	{
		return Super::PointCheck( Result, Location, Extent, TraceFlags );
	}

	UBOOL bReturn = FALSE;

	const FMatrix RotatedLocalToWorld = RotationOnlyMatrix.Inverse() * LocalToWorld;
	INT		NumCollision = SpeedTree->SRH->GetNumCollisionPrimitives();
	FLOAT	UniformScale = LocalToWorld.GetAxis(0).Size();

	for( INT i=0; i<NumCollision && !bReturn; i++ )
	{
		// get the collision object
		CSpeedTreeRT::ECollisionObjectType Type;
		FVector Pos;
		FVector Dim;
		FVector EulerAngles;
		SpeedTree->SRH->GetCollisionPrimitive( i, Type, Pos, Dim, EulerAngles );
		Dim *= UniformScale;

		switch( Type )
		{
		case CSpeedTreeRT::CO_CAPSULE:
			{
				EulerAngles.X = -EulerAngles.X;
				EulerAngles.Y = -EulerAngles.Y;

				FMatrix MatUnscaledL2W(RotatedLocalToWorld);
				MatUnscaledL2W.RemoveScaling( );
				MatUnscaledL2W.SetOrigin(FVector(0.0f, 0.0f, 0.0f));
				FRotationMatrix MatRotate(FRotator::MakeFromEuler(EulerAngles));
				MatRotate *= MatUnscaledL2W;
				
				FVector Transformed = RotatedLocalToWorld.TransformFVector(Pos) + MatRotate.TransformFVector(FVector(0, 0, Dim.Y * 0.5f));

				// rotate cLocation into collision object's space
				FVector NewLocation = MatRotate.TransformFVector(Location - Transformed) + Transformed;

				// *** portions taken from UCylinderComponent::LineCheck in UnActorComponent.cpp ***
				if( Square(Transformed.Z - NewLocation.Z) < Square(Dim.Y + Extent.Z)
				&&	Square(Transformed.X - NewLocation.X) + Square(Transformed.Y - NewLocation.Y) < Square(Dim.X + Extent.X) )
				{
					Result.Normal = (NewLocation - Transformed).SafeNormal();

					if( Result.Normal.Z < -0.5 )
					{
						Result.Location = FVector(NewLocation.X, NewLocation.Y, Transformed.Z - Extent.Z);
					}
					else if( Result.Normal.Z > 0.5 )
					{
						Result.Location = FVector(NewLocation.X, NewLocation.Y, Transformed.Z - Extent.Z);
					}
					else
					{
						Result.Location = (NewLocation - Extent.X * (Result.Normal * FVector(1, 1, 0)).SafeNormal( )) + FVector(0, 0, NewLocation.Z);
					}

					bReturn = TRUE;
				}

				// transform back into world coordinates if needed
				if( bReturn )
				{
					Result.Location = MatRotate.InverseTransformFVector( Result.Location - Transformed ) + Transformed;
					Result.Normal	= MatRotate.InverseTransformFVector( Result.Normal);
				}

			}
			break;
		case CSpeedTreeRT::CO_SPHERE:
			{
				// check the point in the sphere
				FVector Transformed = RotatedLocalToWorld.TransformFVector(Pos);
				if( (Location - Transformed).SizeSquared( ) < Dim.X * Dim.X)
				{
					Result.Normal	= (Location - Transformed).SafeNormal();
					Result.Location = Result.Normal * Dim.X;
					bReturn = true;
				}
			}			
			break;
		case CSpeedTreeRT::CO_BOX:
			// boxes not supported
			break;
		default:
			break;
		}
	}

	// other fcheckresult stuff
	if( bReturn )
	{
		Result.Material		= NULL;
		Result.Actor		= Owner;
		Result.Component	= this;
	}

	return !bReturn;
}

UBOOL USpeedTreeComponent::LineCheck(FCheckResult& Result, const FVector& End, const FVector& Start, const FVector& Extent, DWORD TraceFlags)
{
	if( SpeedTree == NULL || !SpeedTree->IsInitialized() )
	{
		return Super::LineCheck( Result, End, Start, Extent, TraceFlags );
	}

	UBOOL bReturn = FALSE;

	const FMatrix RotatedLocalToWorld = RotationOnlyMatrix.Inverse() * LocalToWorld;
	FLOAT	UniformScale = LocalToWorld.GetAxis(0).Size();
	FMatrix MatUnscaledL2W(RotatedLocalToWorld);
	MatUnscaledL2W.RemoveScaling( );
	MatUnscaledL2W.SetOrigin(FVector(0.0f, 0.0f, 0.0f));

	INT NumCollision = SpeedTree->SRH->GetNumCollisionPrimitives();
	for( INT i=0; i<NumCollision && !bReturn; i++ )
	{
		// get the collision primitive
		CSpeedTreeRT::ECollisionObjectType Type;
		FVector Pos;
		FVector Dim;
		FVector EulerAngles;
		SpeedTree->SRH->GetCollisionPrimitive( i, Type, Pos, Dim, EulerAngles );
		Dim *= UniformScale;

		switch( Type )
		{
		case CSpeedTreeRT::CO_CAPSULE:
			{
				EulerAngles.X = -EulerAngles.X;
				EulerAngles.Y = -EulerAngles.Y;

				FRotationMatrix MatRotate(FRotator::MakeFromEuler(EulerAngles));
				MatRotate *= MatUnscaledL2W;

				const FVector WorldCylinderOrigin = RotatedLocalToWorld.TransformFVector(Pos) + MatRotate.TransformFVector(FVector(0, 0, Dim.Y * 0.5f));

				// rotate start/end into collision object's space
				const FVector NewStart = MatRotate.InverseTransformFVector(Start - WorldCylinderOrigin);
				const FVector NewEnd = MatRotate.InverseTransformFVector(End - WorldCylinderOrigin);

				// *** portions taken from UCylinderComponent::LineCheck in UnActorComponent.cpp ***
				Result.Time = 1.0f;

				// Treat this actor as a cylinder.
				const FVector CylExtent(Dim.X, Dim.X, Dim.Y);
				const FVector NetExtent = Extent + CylExtent;

				// Quick X reject.
				const FLOAT MaxX = +NetExtent.X;
				if( NewStart.X > MaxX && NewEnd.X > MaxX )
				{
					break;
				}

				const FLOAT MinX = -NetExtent.X;
				if( NewStart.X < MinX && NewEnd.X < MinX )
				{
					break;
				}

				// Quick Y reject.
				const FLOAT MaxY = +NetExtent.Y;
				if( NewStart.Y > MaxY && NewEnd.Y > MaxY )
				{
					break;
				}

				const FLOAT MinY = -NetExtent.Y;
				if( NewStart.Y < MinY && NewEnd.Y < MinY )
				{
					break;
				}

				// Quick Z reject.
				const FLOAT TopZ = +NetExtent.Z;
				if( NewStart.Z > TopZ && NewEnd.Z > TopZ )
				{
					break;
				}

				const FLOAT BotZ = -NetExtent.Z;
				if( NewStart.Z < BotZ && NewEnd.Z < BotZ )
				{
					break;
				}

				// Clip to top of cylinder.
				FLOAT T0 = 0.0f;
				FLOAT T1 = 1.0f;
				if( NewStart.Z > TopZ && NewEnd.Z < TopZ )
				{
					FLOAT T = (TopZ - NewStart.Z) / (NewEnd.Z - NewStart.Z);
					if( T > T0 )
					{
						T0 = ::Max(T0, T);
						Result.Normal = FVector(0, 0, 1);
					}
				}
				else if( NewStart.Z < TopZ && NewEnd.Z > TopZ )
				{
					T1 = ::Min(T1, (TopZ - NewStart.Z) / (NewEnd.Z - NewStart.Z));
				}

				// Clip to bottom of cylinder.
				if( NewStart.Z < BotZ && NewEnd.Z > BotZ )
				{
					FLOAT T = (BotZ - NewStart.Z) / (NewEnd.Z - NewStart.Z);
					if( T > T0 )
					{
						T0 = ::Max(T0, T);
						Result.Normal = FVector(0, 0, -1);
					}
				}
				else if( NewStart.Z > BotZ && NewEnd.Z < BotZ )
				{
					T1 = ::Min(T1, (BotZ - NewStart.Z) / (NewEnd.Z - NewStart.Z));
				}

				// Reject.
				if (T0 >= T1)
				{
					break;
				}

				// Test setup.
				FLOAT   Kx        = NewStart.X;
				FLOAT   Ky        = NewStart.Y;

				// 2D circle clip about origin.
				FLOAT   Vx        = NewEnd.X - NewStart.X;
				FLOAT   Vy        = NewEnd.Y - NewStart.Y;
				FLOAT   A         = Vx * Vx + Vy * Vy;
				FLOAT   B         = 2.0f * (Kx * Vx + Ky * Vy);
				FLOAT   C         = Kx * Kx + Ky * Ky - Square(NetExtent.X);
				FLOAT   Discrim   = B * B - 4.0f * A * C;

				// If already inside sphere, oppose further movement inward.
				FVector LocalHitLocation;
				FVector LocalHitNormal;
				if( C < Square(1.0f) && NewStart.Z > BotZ && NewStart.Z < TopZ )
				{
					const FVector DirXY(
						NewEnd.X - NewStart.X,
						NewEnd.Y - NewStart.Y,
						0
						);
					FLOAT Dir = DirXY | NewStart;
					if( Dir < -0.01f )
					{
						Result.Time		= 0.0f;

						LocalHitLocation = NewStart;
						LocalHitNormal = NewStart * FVector(1, 1, 0);
					
						bReturn = TRUE;

						break;
					}
					else
					{
						break;
					}
				}

				// No intersection if discriminant is negative.
				if( Discrim < 0 )
				{
					break;
				}

				// Unstable intersection if velocity is tiny.
				if( A < Square(0.0001f) )
				{
					// Outside.
					if( C > 0 )
					{
						break;
					}
				}
				else
				{
					// Compute intersection times.
					Discrim = appSqrt(Discrim);
					FLOAT R2A = 0.5 / A;
					T1 = ::Min(T1, +(Discrim - B) * R2A);
					FLOAT T = -(Discrim + B) * R2A;
					if (T > T0)
					{
						T0 = T;
						LocalHitNormal = NewStart + (NewEnd - NewStart) * T0;
						LocalHitNormal.Z = 0;
					}
					if( T0 >= T1 )
					{
						break;
					}
				}
				Result.Time = Clamp(T0 - 0.001f, 0.0f, 1.0f);
				LocalHitLocation = NewStart + (NewEnd - NewStart) * Result.Time;

				// transform back into world coordinates
				Result.Location = WorldCylinderOrigin + MatRotate.TransformFVector(LocalHitLocation);
				Result.Normal = MatRotate.TransformNormal(LocalHitNormal).SafeNormal();

				bReturn = TRUE;
			}
			break;
		case CSpeedTreeRT::CO_SPHERE:
			{
				FVector Transformed = RotatedLocalToWorld.TransformFVector(Pos);

				// check the line through the sphere
				// *** portions taken from FLineSphereIntersection in UnMath.h ***

				FVector Dir = End - Start;
				FLOAT Length = Dir.Size( );

				if ( Length == 0.0f)
				{
					Dir = FVector(0, 0, 1);
					Length = 1.0f;
				}
				else
				{
					Dir /= Length;
				}

				const FVector	EO		= Start - Transformed;
				const FLOAT		V		= (Dir | (Transformed - Start));
				const FLOAT		Disc	= Dim.X * Dim.X - ((EO | EO) - V * V);
					
				if( EO.SizeSquared( ) < Dim.X * Dim.X )
				{
					Result.Time		= 0.0f;
					Result.Location = Start;
					Result.Normal	= (Start - Transformed).SafeNormal();
					Result			= TRUE;
					break;
				}
				
				if( Disc >= 0.0f )
				{
					Result.Time = (V - appSqrt(Disc)) / Length;
					if( Result.Time >= 0.0f && Result.Time <= 1.0f )
					{
						Result.Location = Start + Dir * Result.Time;
						Result.Normal	= (Result.Location - Transformed).SafeNormal();
						bReturn			= TRUE;
						break;
					}
				}
			}	
			break;
		case CSpeedTreeRT::CO_BOX:
			// boxes not supported
			break;
		default:
			break;
		}
	}

	// other fcheckresult stuff
	if( bReturn )
	{
		Result.Material		= NULL;
		Result.Actor		= Owner;
		Result.Component	= this;
	}

	return !bReturn;
}


#if WITH_NOVODEX
void USpeedTreeComponent::InitComponentRBPhys(UBOOL /*bFixed*/)
{
	// Don't create physics body at all if no collision (makes assumption it can't change at run time).
	if( !BlockRigidBody)
	{
		return;
	}

	if( GWorld->RBPhysScene && SpeedTree && SpeedTree->IsInitialized() )
	{
		// make novodex info
		NxActorDesc nxActorDesc;
		nxActorDesc.setToDefault( );

		NxMat33 MatIdentity;
		MatIdentity.id();

		const FMatrix RotatedLocalToWorld = RotationOnlyMatrix.Inverse() * LocalToWorld;
		INT NumPrimitives = SpeedTree->SRH->GetNumCollisionPrimitives();
		FLOAT UniformScale = LocalToWorld.GetAxis(0).Size();

		for( INT i=0; i<NumPrimitives; i++ )
		{
			CSpeedTreeRT::ECollisionObjectType Type;
			FVector Pos;
			FVector Dim;
			FVector EulerAngles;
			SpeedTree->SRH->GetCollisionPrimitive( i, Type, Pos, Dim, EulerAngles );

			switch( Type )
			{
			case CSpeedTreeRT::CO_SPHERE:
				{
					NxSphereShapeDesc* SphereDesc = new NxSphereShapeDesc;
					SphereDesc->setToDefault( );
					SphereDesc->radius = Dim.X * UniformScale * U2PScale;
					SphereDesc->localPose = NxMat34(MatIdentity, U2NPosition(RotatedLocalToWorld.TransformFVector(Pos)));
					nxActorDesc.shapes.pushBack(SphereDesc);
				}
				break;
			case CSpeedTreeRT::CO_CAPSULE:
				{
					EulerAngles.X = -EulerAngles.X;
					EulerAngles.Y = -EulerAngles.Y;
					Dim *= UniformScale;

					FMatrix MatUnscaledL2W(RotatedLocalToWorld);
					MatUnscaledL2W.RemoveScaling( );
					MatUnscaledL2W.SetOrigin(FVector(0.0f, 0.0f, 0.0f));
					FRotationMatrix MatRotate(FRotator::MakeFromEuler(EulerAngles));
					MatRotate *= MatUnscaledL2W;

					Pos = RotatedLocalToWorld.TransformFVector(Pos) + MatRotate.TransformFVector(FVector(0, 0, Dim.Y * 0.5f));
					FRotationMatrix MatPostRotate(FRotator(0, 0, 16384));
					FMatrix MatTransform = MatPostRotate * MatRotate;
					MatTransform.SetOrigin(Pos);

					NxCapsuleShapeDesc* CapsuleShape = new NxCapsuleShapeDesc;
					CapsuleShape->setToDefault( );
					CapsuleShape->radius = Dim.X * U2PScale;
					CapsuleShape->height = Dim.Y * U2PScale;
					CapsuleShape->localPose = U2NTransform(MatTransform);
					nxActorDesc.shapes.pushBack(CapsuleShape);
				}
				break;
			case CSpeedTreeRT::CO_BOX:
				// boxes not suported
				break;
			default:
				break;
			};
		}
		
		if( nxActorDesc.isValid() && nxActorDesc.shapes.size( ) > 0 && GWorld->RBPhysScene )
		{
			NxScene* NovodexScene = GWorld->RBPhysScene->GetNovodexPrimaryScene();
			check(NovodexScene);
			NxActor* nxActor = NovodexScene->createActor(nxActorDesc);
			
			if( nxActor )
			{
				BodyInstance = ConstructObject<URB_BodyInstance>(URB_BodyInstance::StaticClass( ), GWorld, NAME_None, RF_Transactional);
				BodyInstance->BodyData			= (FPointer)nxActor;
				BodyInstance->OwnerComponent	= this;
				nxActor->userData				= BodyInstance;
				BodyInstance->SceneIndex		= GWorld->RBPhysScene->NovodexSceneIndex;
			}
		}
			
		while(!nxActorDesc.shapes.isEmpty())
		{
			NxShapeDesc* Shape = nxActorDesc.shapes.back();
			nxActorDesc.shapes.popBack();
			delete Shape;
		};
	}

	UpdatePhysicsToRBChannels();
}
#endif // WITH_NOVODEX

#endif // #if WITH_SPEEDTREE

void USpeedTreeComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
#if WITH_SPEEDTREE
	// Don't allow the tree to be rotated if it has billboard leaves.
	if(IsValidComponent())
	{
		// Compute a rotation-less parent to world matrix.
		RotationOnlyMatrix = ParentToWorld;
		RotationOnlyMatrix.RemoveScaling();
		RotationOnlyMatrix.SetOrigin(FVector(0,0,0));
		RotationOnlyMatrix = RotationOnlyMatrix.Inverse();
		const FMatrix RotationlessParentToWorld = RotationOnlyMatrix * ParentToWorld;

		// Pass the rotation-less matrix to UPrimitiveComponent::SetParentToWorld.
		Super::SetParentToWorld(RotationlessParentToWorld);
	}
	else
#endif
	{
		Super::SetParentToWorld(ParentToWorld);
	}
}

UBOOL USpeedTreeComponent::IsValidComponent() const
{
#if WITH_SPEEDTREE
	// Only allow the component to be attached if it has a valid SpeedTree reference.
	return SpeedTree != NULL && SpeedTree->IsInitialized() && SpeedTree->SRH != NULL && SpeedTree->SRH->bIsInitialized && Super::IsValidComponent();
#else
	return FALSE;
#endif
}

void USpeedTreeComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(Ar.Ver() >= VER_SPEEDTREE_STATICLIGHTING)
	{
		// Serialize the component's static lighting.
		Ar << BranchAndFrondLightMap << LeafCardLightMap << BillboardLightMap;

		if(Ar.Ver() >= VER_SPEEDTREE_VERTEXSHADER_RENDERING)
		{
			Ar << LeafMeshLightMap;
		}

		if(Ar.Ver() < VER_SPEEDTREE_VERTEXSHADER_RENDERING)
		{
			// Discard out of date cached lighting.
			StaticLights.Empty( );
			BranchAndFrondLightMap = NULL;
			LeafCardLightMap = NULL;
			LeafMeshLightMap = NULL;
			BillboardLightMap = NULL;
		}
	}
	else if(Ar.IsLoading())
	{
		FLightingChannelContainer DynamicLightingChannel;
		DynamicLightingChannel.Dynamic = TRUE;

		FLightingChannelContainer NonDynamicLightingChannels;
		NonDynamicLightingChannels.SetAllChannels();
		NonDynamicLightingChannels.Dynamic = FALSE;

		// Reset lighting channels to the new default (Static) if they were set to the old default (Dynamic).
		if(LightingChannels.OverlapsWith(DynamicLightingChannel) && !LightingChannels.OverlapsWith(NonDynamicLightingChannels))
		{
			LightingChannels.bInitialized = FALSE;
		}
	}
}

void USpeedTreeComponent::PostLoad()
{
	Super::PostLoad();

	// Randomly permute the global wind matrices for each component.
	WindMatrixOffset = (FLOAT)(appRand() % 3);

	// Initialize the light-map resources.
	if(BranchAndFrondLightMap)
	{
		BranchAndFrondLightMap->InitResources();
	}
	if(LeafMeshLightMap)
	{
		LeafMeshLightMap->InitResources();
	}
	if(LeafCardLightMap)
	{
		LeafCardLightMap->InitResources();
	}
	if(BillboardLightMap)
	{
		BillboardLightMap->InitResources();
	}
}

void USpeedTreeComponent::PostEditChange(UProperty* PropertyThatChanged)
{
#if WITH_SPEEDTREE
	// make sure Lod level is valid
	if (LodLevelOverride > 1.0f)
	{
		LodLevelOverride = 1.0f;
	}

	if (LodLevelOverride < 0.0f && LodLevelOverride != -1.0f)
	{
		LodLevelOverride = -1.0f;
	}

#endif
	Super::PostEditChange(PropertyThatChanged);
}

void USpeedTreeComponent::PostEditUndo()
{
	// Initialize the light-map resources.
	if(BranchAndFrondLightMap)
	{
		BranchAndFrondLightMap->InitResources();
	}
	if(LeafMeshLightMap)
	{
		LeafMeshLightMap->InitResources();
	}
	if(LeafCardLightMap)
	{
		LeafCardLightMap->InitResources();
	}
	if(BillboardLightMap)
	{
		BillboardLightMap->InitResources();
	}

	Super::PostEditUndo();
}

USpeedTreeComponent::USpeedTreeComponent()
{
	// Randomly permute the global wind matrices for each component.
	WindMatrixOffset = (FLOAT)(appRand() % 3);
}
