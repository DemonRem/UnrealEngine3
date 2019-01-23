/*=============================================================================
	UnModel.cpp: Unreal model functions
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	Struct serializers.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, FBspSurf& Surf )
{
	Ar << Surf.Material;
	Ar << Surf.PolyFlags;	
	if( Ar.Ver() < VER_DEFAULTPOLYFLAGS_CHANGE )
	{
		Surf.PolyFlags |= PF_DefaultFlags;
	}
	Ar << Surf.pBase << Surf.vNormal;
	Ar << Surf.vTextureU << Surf.vTextureV;
	Ar << Surf.iBrushPoly;
	Ar << Surf.Actor;
	Ar << Surf.Plane;
	Ar << Surf.ShadowMapScale;
	if ( Ar.Ver() >= VER_BSP_LIGHTING_CHANNEL_SUPPORT )
	{
		DWORD LightingChannels = Surf.LightingChannels.Bitfield;
		Ar << LightingChannels;
		Surf.LightingChannels.Bitfield = LightingChannels;
	}
	else
	{
		FLightingChannelContainer				DefaultLightingChannels;
		DefaultLightingChannels.Bitfield		= 0;
		DefaultLightingChannels.BSP				= TRUE;
		DefaultLightingChannels.bInitialized	= TRUE;
		Surf.LightingChannels					= DefaultLightingChannels;
	}

	return Ar;
}

FArchive& operator<<( FArchive& Ar, FPoly& Poly )
{
	INT LegacyNumVertices = Poly.Vertices.Num();
	if(Ar.Ver() < VER_FPOLYVERTEXARRAY)
	{
		Ar << LegacyNumVertices;
	}
	Ar << Poly.Base << Poly.Normal << Poly.TextureU << Poly.TextureV;
	if(Ar.Ver() < VER_FPOLYVERTEXARRAY)
	{
		if(Ar.IsLoading())
		{
			Poly.Vertices.Empty(LegacyNumVertices);
			Poly.Vertices.Add(LegacyNumVertices);
		}
		for( INT i=0; i<LegacyNumVertices; i++ )
		{	
			Ar << Poly.Vertices(i);
		}
	}
	else
	{
		Ar << Poly.Vertices;
	}
	Ar << Poly.PolyFlags;
	if( Ar.Ver() < VER_DEFAULTPOLYFLAGS_CHANGE )
	{
		Poly.PolyFlags |= PF_DefaultFlags;
	}
	Ar << Poly.Actor << Poly.ItemName;
	Ar << Poly.Material;
	Ar << Poly.iLink << Poly.iBrushPoly;
	Ar << Poly.ShadowMapScale;
	if ( Ar.Ver() >= VER_BSP_LIGHTING_CHANNEL_SUPPORT )
	{
		Ar << Poly.LightingChannels;
	}
	else
	{
		FLightingChannelContainer				DefaultLightingChannels;
		DefaultLightingChannels.Bitfield		= 0;
		DefaultLightingChannels.BSP				= TRUE;
		DefaultLightingChannels.bInitialized	= TRUE;
		Poly.LightingChannels					= DefaultLightingChannels.Bitfield;
	}

	return Ar;
}

FArchive& operator<<( FArchive& Ar, FBspNode& N )
{
	// @warning BulkSerialize: FBSPNode is serialized as memory dump
	// See TArray::BulkSerialize for detailed description of implied limitations.

	// Serialize in the order of variable declaration so the data is compatible with BulkSerialize
	Ar	<< N.Plane;
	if( Ar.Ver() < VER_REMOVED_ZONEMASK )
	{	
		FZoneSet ZoneMask;
		Ar << ZoneMask;
	}
	Ar	<< N.iVertPool
		<< N.iSurf
		<< N.iVertexIndex
		<< N.ComponentIndex 
		<< N.ComponentNodeIndex;

	if( Ar.Ver() >= VER_ADDED_COMPONENT_ELEMENT_INDEX )
	{
		Ar << N.ComponentElementIndex;
	}
	else
	{
		N.ComponentElementIndex = INDEX_NONE;
	}

	Ar	<< N.iChild[0]
		<< N.iChild[1]
		<< N.iChild[2]
		<< N.iCollisionBound
		<< N.iZone[0]
		<< N.iZone[1]
		<< N.NumVertices
		<< N.NodeFlags
		<< N.iLeaf[0]
		<< N.iLeaf[1];

	if( Ar.IsLoading() )
	{
		//@warning: this code needs to be in sync with UModel::Serialize as we use bulk serialization.
		N.NodeFlags &= ~(NF_IsNew|NF_IsFront|NF_IsBack);
	}

	return Ar;
}

FArchive& operator<<( FArchive& Ar, FZoneProperties& P )
{
	Ar	<< P.ZoneActor
		<< P.Connectivity
		<< P.Visibility
		<< P.LastRenderTime;
	return Ar;
}

/**
* Serializer
*
* @param Ar - archive to serialize with
* @param V - vertex to serialize
* @return archive that was used
*/
FArchive& operator<<(FArchive& Ar,FModelVertex& V)
{
	Ar << V.Position;
	Ar << V.TangentX;
	
	if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
	{
		FPackedNormal TangentY;
		Ar << TangentY;
		Ar << V.TangentZ;
		// store the sign of the determinant in TangentZ.W
		V.TangentZ.Vector.W = GetBasisDeterminantSignByte( V.TangentX, TangentY, V.TangentZ );
	}
	else
	{
		Ar << V.TangentZ;
	}	
	
	Ar << V.TexCoord;
	Ar << V.ShadowTexCoord;	

	return Ar;
}

/*---------------------------------------------------------------------------------------
	UModel object implementation.
---------------------------------------------------------------------------------------*/

void UModel::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	Ar << Bounds;

	Vectors.BulkSerialize( Ar );
	Points.BulkSerialize( Ar );
	Nodes.BulkSerialize( Ar );
	if( Ar.IsLoading() )
	{
		for( INT NodeIndex=0; NodeIndex<Nodes.Num(); NodeIndex++ )
		{
			Nodes(NodeIndex).NodeFlags &= ~(NF_IsNew|NF_IsFront|NF_IsBack);
		}
	}
	Ar << Surfs;
	Verts.BulkSerialize( Ar );
	Ar << NumSharedSides << NumZones;
	for( INT i=0; i<NumZones; i++ )
	{
		Ar << Zones[i];
	}
	Ar << Polys;

	LeafHulls.BulkSerialize( Ar );
	Leaves.BulkSerialize( Ar );
	Ar << RootOutside << Linked;
	PortalNodes.BulkSerialize( Ar );
	Edges.BulkSerialize(Ar);

	if( Ar.Ver() < VER_USE_UMA_RESOURCE_ARRAY_MESH_DATA &&
		Ar.IsLoading() )
	{
		// build vertex buffers for legacy load
		BuildVertexBuffers();
	}
	else
	{
		Ar << NumVertices; 
		// load/save vertex buffer
		Ar << VertexBuffer;
	}
}

void UModel::PostLoad()
{
	Super::PostLoad();
	
	// Ensure that if my Outer (usually a brush) is not loaded in game, then I am not.
	// This is to fix some content where add/subtract brushes were losing these flags on their UModels.
	UObject* MyOuter = GetOuter();
	check(MyOuter);
	if( MyOuter->HasAllFlags(RF_NotForClient | RF_NotForServer) )
	{
		SetFlags(RF_NotForClient | RF_NotForServer);
		Polys->SetFlags(RF_NotForClient | RF_NotForServer);
	}

	if( !GIsUCC && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		if( !UEngine::ShadowVolumesAllowed() )
		{
            RemoveShadowVolumeData();
		}

		UpdateVertices();
		if(!Edges.Num())
		{
			BuildShadowData();
		}
	}
}

void UModel::PreEditUndo()
{
	Super::PreEditUndo();

	// Release the model's resources.
	BeginReleaseResources();
	ReleaseResourcesFence.Wait();
}

void UModel::PostEditUndo()
{
	Super::PostEditUndo();

	// Reinitialize the model's resources.
	UpdateVertices();
}

/**
 * Used by various commandlets to purge Editor only data from the object.
 * 
 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
 */
void UModel::StripData(UE3::EPlatformType TargetPlatform)
{
	Super::StripData(TargetPlatform);

	// remove shadow volume data based on engine config setting
	if( !UEngine::ShadowVolumesAllowed() )
	{
		RemoveShadowVolumeData();
	}
}

void UModel::ModifySurf( INT InIndex, UBOOL UpdateMaster )
{
	Surfs.ModifyItem( InIndex );
	FBspSurf& Surf = Surfs(InIndex);
	if( UpdateMaster && Surf.Actor )
	{
		Surf.Actor->Brush->Polys->Element.ModifyItem( Surf.iBrushPoly );
	}
}
void UModel::ModifyAllSurfs( UBOOL UpdateMaster )
{
	for( INT i=0; i<Surfs.Num(); i++ )
		ModifySurf( i, UpdateMaster );

}
void UModel::ModifySelectedSurfs( UBOOL UpdateMaster )
{
	for( INT i=0; i<Surfs.Num(); i++ )
		if( Surfs(i).PolyFlags & PF_Selected )
			ModifySurf( i, UpdateMaster );

}

UBOOL UModel::Rename( const TCHAR* InName, UObject* NewOuter, ERenameFlags Flags )
{
	// Also rename the UPolys.
    if (NewOuter && Polys && Polys->GetOuter() == GetOuter())
	{
		if (Polys->Rename(*MakeUniqueObjectName(NewOuter, Polys->GetClass()).ToString(), NewOuter, Flags) == FALSE)
		{
			return FALSE;
		}
	}

    return Super::Rename( InName, NewOuter, Flags );
}

/**
 * Called after duplication & serialization and before PostLoad. Used to make sure UModel's FPolys
 * get duplicated as well.
 */
void UModel::PostDuplicate()
{
	Super::PostDuplicate();
	if( Polys )
	{
		Polys = CastChecked<UPolys>(UObject::StaticDuplicateObject( Polys, Polys, GetOuter(), NULL ));
	}
}

void UModel::BeginDestroy()
{
	Super::BeginDestroy();
	BeginReleaseResources();
}

UBOOL UModel::IsReadyForFinishDestroy()
{
	return ReleaseResourcesFence.GetNumPendingFences() == 0 && Super::IsReadyForFinishDestroy();
}

IMPLEMENT_CLASS(UModel);

/*---------------------------------------------------------------------------------------
	UModel implementation.
---------------------------------------------------------------------------------------*/

//
// Lock a model.
//
void UModel::Modify( UBOOL bAlwaysMarkDirty/*=FALSE*/ )
{
	Super::Modify(bAlwaysMarkDirty);

	// Modify all child objects.
	if( Polys )
	{
		Polys->Modify(bAlwaysMarkDirty);
	}
}

//
// Empty the contents of a model.
//
void UModel::EmptyModel( INT EmptySurfInfo, INT EmptyPolys )
{
	Nodes			.Empty();
	LeafHulls		.Empty();
	Leaves			.Empty();
	Verts			.Empty();
	PortalNodes		.Empty();

	if( EmptySurfInfo )
	{
		Vectors.Empty();
		Points.Empty();
		Surfs.Empty();
	}
	if( EmptyPolys )
	{
		Polys = new( GetOuter(), NAME_None, RF_Transactional )UPolys;
	}

	// Init variables.
	NumSharedSides	= 4;
	NumZones = 0;
	for( INT i=0; i<FBspNode::MAX_ZONES; i++ )
	{
		Zones[i].ZoneActor    = NULL;
		Zones[i].Connectivity = FZoneSet::IndividualZone(i);
		Zones[i].Visibility   = FZoneSet::AllZones();
	}
}

//
// Create a new model and allocate all objects needed for it.
//
UModel::UModel( ABrush* Owner, UBOOL InRootOutside )
:	Nodes		( this )
,	Verts		( this )
,	Vectors		( this )
,	Points		( this )
,	Surfs		( this )
,	VertexBuffer( this )
,	ShadowVertexBuffer( this )
,	RootOutside	( InRootOutside )
{
	SetFlags( RF_Transactional );
	EmptyModel( 1, 1 );
	if( Owner )
	{
		check(Owner->BrushComponent);
		Owner->Brush = this;
		Owner->InitPosRotScale();
	}
	if( GIsEditor && !GIsGame )
	{
		UpdateVertices();
	}
}

/**
 * Static constructor called once per class during static initialization via IMPLEMENT_CLASS
 * macro. Used to e.g. emit object reference tokens for realtime garbage collection or expose
 * properties for native- only classes.
 */
void UModel::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference( STRUCT_OFFSET( UModel, Polys ) );
	const DWORD SkipIndexIndex = TheClass->EmitStructArrayBegin( STRUCT_OFFSET( UModel, Surfs ), sizeof(FBspSurf) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( FBspSurf, Material ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( FBspSurf, Actor ) );
	TheClass->EmitStructArrayEnd( SkipIndexIndex );
}

//
// Build the model's bounds (min and max).
//
void UModel::BuildBound()
{
	if( Polys && Polys->Element.Num() )
	{
		TArray<FVector> NewPoints;
		for( INT i=0; i<Polys->Element.Num(); i++ )
			for( INT j=0; j<Polys->Element(i).Vertices.Num(); j++ )
				NewPoints.AddItem(Polys->Element(i).Vertices(j));
		Bounds = FBoxSphereBounds( &NewPoints(0), NewPoints.Num() );
	}
}

//
// Transform this model by its coordinate system.
//
void UModel::Transform( ABrush* Owner )
{
	check(Owner);

	Polys->Element.ModifyAllItems();

	for( INT i=0; i<Polys->Element.Num(); i++ )
		Polys->Element( i ).Transform( Owner->PrePivot, Owner->Location);

}

/**
 * Returns the scale dependent texture factor used by the texture streaming code.	
 */
FLOAT UModel::GetStreamingTextureFactor()
{
	FLOAT MaxTextureRatio = 0.f;

	if( Polys )
	{
		for( INT PolyIndex=0; PolyIndex<Polys->Element.Num(); PolyIndex++ )
		{
			FPoly& Poly = Polys->Element(PolyIndex);

			// Assumes that all triangles on a given poly share the same texture scale.
			if( Poly.Vertices.Num() > 2 )
			{
				FLOAT		L1	= (Poly.Vertices(0) - Poly.Vertices(1)).Size(),
							L2	= (Poly.Vertices(0) - Poly.Vertices(2)).Size();

				FVector2D	UV0	= FVector2D( (Poly.Vertices(0) - Poly.Base) | Poly.TextureU, (Poly.Vertices(0) - Poly.Base) | Poly.TextureV ),
							UV1	= FVector2D( (Poly.Vertices(1) - Poly.Base) | Poly.TextureU, (Poly.Vertices(1) - Poly.Base) | Poly.TextureV ),
							UV2	= FVector2D( (Poly.Vertices(2) - Poly.Base) | Poly.TextureU, (Poly.Vertices(2) - Poly.Base) | Poly.TextureV );

				if( Abs(L1 * L2) > Square(SMALL_NUMBER) )
				{
					FLOAT		T1	= (UV0 - UV1).Size() / 128,
								T2	= (UV0 - UV2).Size() / 128;

					if( Abs(T1 * T2) > Square(SMALL_NUMBER) )
					{
						MaxTextureRatio = Max3( MaxTextureRatio, L1 / T1, L2 / T2 );
					}
				}
			}
		}
	}

	return MaxTextureRatio;
}

/*---------------------------------------------------------------------------------------
	UModel basic implementation.
---------------------------------------------------------------------------------------*/

//
// Shrink all stuff to its minimum size.
//
void UModel::ShrinkModel()
{
	Vectors		.Shrink();
	Points		.Shrink();
	Verts		.Shrink();
	Nodes		.Shrink();
	Surfs		.Shrink();
	if( Polys     ) Polys    ->Element.Shrink();
	LeafHulls	.Shrink();
	PortalNodes	.Shrink();
}

void UModel::BeginReleaseResources()
{
	// Release the index buffers.
	for(TDynamicMap<UMaterialInterface*,FRawIndexBuffer32>::TIterator IndexBufferIt(MaterialIndexBuffers);IndexBufferIt;++IndexBufferIt)
	{
		BeginReleaseResource(&IndexBufferIt.Value());
	}

	// Release the vertex buffer and factory.
	BeginReleaseResource(&VertexBuffer);
	BeginReleaseResource(&VertexFactory);

	// Release the shadow vertex buffer and factory.
	BeginReleaseResource(&ShadowVertexBuffer);

	// Use a fence to keep track of the release progress.
	ReleaseResourcesFence.BeginFence();
}

void UModel::UpdateVertices()
{
	// Wait for pending resource release commands to execute.
	ReleaseResourcesFence.Wait();

	// rebuild vertex buffer if the resource array is not static 
	if( GIsEditor && !GIsGame && !VertexBuffer.Vertices.IsStatic() )
	{	
		BuildVertexBuffers();
	}
	// we should have the same # of vertices in the loaded vertex buffer
	check(NumVertices == VertexBuffer.Vertices.Num());	
	BeginInitResource(&VertexBuffer);
	if( GIsEditor && !GIsGame )
	{
		// needed since we may call UpdateVertices twice and the first time
		// NumVertices might be 0. 
		BeginUpdateResourceRHI(&VertexBuffer);
	}

	// Set up the vertex factory.
	TSetResourceDataContext<FLocalVertexFactory> VertexFactoryData(&VertexFactory);
	VertexFactoryData->PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer,FModelVertex,Position,VET_Float3);
	VertexFactoryData->TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer,FModelVertex,TangentX,VET_PackedNormal);
	VertexFactoryData->TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer,FModelVertex,TangentZ,VET_PackedNormal);
	VertexFactoryData->TextureCoordinates.Empty();
	VertexFactoryData->TextureCoordinates.AddItem(STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer,FModelVertex,TexCoord,VET_Float2));
	VertexFactoryData->ShadowMapCoordinateComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer,FModelVertex,ShadowTexCoord,VET_Float2);
	VertexFactoryData.Commit();
	BeginInitResource(&VertexFactory);

	if( UEngine::ShadowVolumesAllowed() )
	{
		// Set up the shadow vertex buffer and its vertex factory.
		BeginInitResource(&ShadowVertexBuffer);

		if( GIsEditor && !GIsGame )
		{
			// This is to force it to update. First time UpdateVertices() is called,
			// we may have 0 vertices so the vertex buffer doesn't get created.
			// Second time (from PostLoad) we need this call, because BeginInitResource
			// will not do anything.
			BeginUpdateResourceRHI(&ShadowVertexBuffer);
		}
	}
}

/** 
 *	Compute the "center" location of all the verts 
 */
FVector UModel::GetCenter()
{
	FVector Center(0.f);
	UINT Cnt = 0;
	for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
	{
		FBspNode& Node = Nodes(NodeIndex);
		UINT NumVerts = (Node.NodeFlags & PF_TwoSided) ? Node.NumVertices / 2 : Node.NumVertices;
		for(UINT VertexIndex = 0;VertexIndex < NumVerts;VertexIndex++)
		{
			const FVert& Vert = Verts(Node.iVertPool + VertexIndex);
			const FVector& Position = Points(Vert.pVertex);
			Center += Position;
			Cnt++;
		}
	}

	if( Cnt > 0 )
	{
		Center /= Cnt;
	}
	
	return Center;
}

/**
* Initialize vertex buffer data from UModel data
*/
void UModel::BuildVertexBuffers()
{
	// Calculate the size of the vertex buffer and the base vertex index of each node.
	NumVertices = 0;
	for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
	{
		FBspNode& Node = Nodes(NodeIndex);
		FBspSurf& Surf = Surfs(Node.iSurf);
		Node.iVertexIndex = NumVertices;
		NumVertices += (Surf.PolyFlags & PF_TwoSided) ? (Node.NumVertices * 2) : Node.NumVertices;
	}

	// size vertex buffer data
	VertexBuffer.Vertices.Empty(NumVertices);
	VertexBuffer.Vertices.Add(NumVertices);

	if(NumVertices > 0)
	{
		// Initialize the vertex data
		FModelVertex* DestVertex = (FModelVertex*)VertexBuffer.Vertices.GetData();
		for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			FBspNode& Node = Nodes(NodeIndex);
			FBspSurf& Surf = Surfs(Node.iSurf);
			const FVector& TextureBase = Points(Surf.pBase);
			const FVector& TextureX = Vectors(Surf.vTextureU);
			const FVector& TextureY = Vectors(Surf.vTextureV);

			// Use the texture coordinates and normal to create an orthonormal tangent basis.
			FVector TangentX = TextureX;
			FVector TangentY = TextureY;
			FVector TangentZ = Vectors(Surf.vNormal);
			CreateOrthonormalBasis(TangentX,TangentY,TangentZ);

			for(UINT VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
			{
				const FVert& Vert = Verts(Node.iVertPool + VertexIndex);
				const FVector& Position = Points(Vert.pVertex);
				DestVertex->Position = Position;
				DestVertex->TexCoord.X = ((Position - TextureBase) | TextureX) / 128.0f;
				DestVertex->TexCoord.Y = ((Position - TextureBase) | TextureY) / 128.0f;
				DestVertex->ShadowTexCoord = Vert.ShadowTexCoord;
				DestVertex->TangentX = TangentX;
				DestVertex->TangentZ = TangentZ;

				// store the sign of the determinant in TangentZ.W
				DestVertex->TangentZ.Vector.W = GetBasisDeterminantSign( TangentX, TangentY, TangentZ ) < 0 ? 0 : 255;

				DestVertex++;
			}

			if(Surf.PolyFlags & PF_TwoSided)
			{
				for(INT VertexIndex = Node.NumVertices - 1;VertexIndex >= 0;VertexIndex--)
				{
					const FVert& Vert = Verts(Node.iVertPool + VertexIndex);
					const FVector& Position = Points(Vert.pVertex);
					DestVertex->Position = Position;
					DestVertex->TexCoord.X = ((Position - TextureBase) | TextureX) / 128.0f;
					DestVertex->TexCoord.Y = ((Position - TextureBase) | TextureY) / 128.0f;
					DestVertex->ShadowTexCoord = Vert.BackfaceShadowTexCoord;
					DestVertex->TangentX = TangentX;
					DestVertex->TangentZ = -TangentZ;

					// store the sign of the determinant in TangentZ.W
					DestVertex->TangentZ.Vector.W = GetBasisDeterminantSign( TangentX, TangentY, -TangentZ ) < 0 ? 0 : 255;

					DestVertex++;
				}
			}
		}
	}
}

void UModel::BuildShadowData()
{
	if( UEngine::ShadowVolumesAllowed() )
	{
		// Clear the existing edges.
		Edges.Empty();

		// Create the node edges.
		TMultiMap<INT,INT> VertexToEdgeMap;
		for(INT NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			FBspNode& Node = Nodes(NodeIndex);
			FBspSurf& Surf = Surfs(Node.iSurf);

			if(Surf.PolyFlags & PF_TwoSided)
				continue;

			for(INT EdgeIndex = 0;EdgeIndex < Node.NumVertices;EdgeIndex++)
			{
				INT PointIndices[2] =
				{
					Verts(Node.iVertPool + EdgeIndex).pVertex,
						Verts(Node.iVertPool + ((EdgeIndex + 1) % Node.NumVertices)).pVertex
				};

				// Find existing edges which start on this edge's ending vertex.
				TArray<INT> PotentialMatchEdges;
				VertexToEdgeMap.MultiFind(PointIndices[1],PotentialMatchEdges);

				// Check if the ending vertex of any of the existing edges match this edge's start vertex.
				INT MatchEdgeIndex = INDEX_NONE;
				for(INT OtherEdgeIndex = 0;OtherEdgeIndex < PotentialMatchEdges.Num();OtherEdgeIndex++)
				{
					const FMeshEdge& OtherEdge = Edges(PotentialMatchEdges(OtherEdgeIndex));
					if(OtherEdge.Vertices[1] == PointIndices[0] && OtherEdge.Faces[1] == INDEX_NONE)
					{
						MatchEdgeIndex = PotentialMatchEdges(OtherEdgeIndex);
						break;
					}
				}

				if(MatchEdgeIndex != INDEX_NONE)
				{
					// Set the matching edge's opposite face to this node.
					Edges(MatchEdgeIndex).Faces[1] = NodeIndex;
				}
				else
				{
					// Create a new edge.
					FMeshEdge* NewEdge = new(Edges) FMeshEdge;
					NewEdge->Vertices[0] = PointIndices[0];
					NewEdge->Vertices[1] = PointIndices[1];
					NewEdge->Faces[0] = NodeIndex;
					NewEdge->Faces[1] = INDEX_NONE;
					VertexToEdgeMap.Set(PointIndices[0],Edges.Num() - 1);
				}
			}
		}
	}
}

/** 
* Removes all vertex data needed for shadow volume rendering 
*/
void UModel::RemoveShadowVolumeData()
{
#if !CONSOLE
	Edges.Empty();
#endif
}

