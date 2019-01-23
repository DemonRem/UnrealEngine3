/*=============================================================================
	UnStaticMeshLegacy.h:
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UNSTATICMESHLEGACY_H__
#define __UNSTATICMESHLEGACY_H__

/** All information about a static-mesh vertex with a variable number of texture coordinates. */
template<UINT NumTexCoords>
struct TLegacyStaticMeshFullVertex
{
	FVector Position;
	FLOAT ShadowExtrusionPredicate;
	FPackedNormal TangentX;
	FPackedNormal TangentY;
	FPackedNormal TangentZ;
	FColor Color;
	FVector2D UVs[NumTexCoords];

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TLegacyStaticMeshFullVertex& Vertex)
	{
		Ar << Vertex.Position << Vertex.ShadowExtrusionPredicate << Vertex.TangentX << Vertex.TangentY << Vertex.TangentZ;
		if(Ar.Ver() >= VER_STATICMESH_VERTEXCOLOR)
		{
			Ar << Vertex.Color;
		}
		for(UINT UVIndex = 0;UVIndex < NumTexCoords;UVIndex++)
		{
			Ar << Vertex.UVs[UVIndex];
		}
		return Ar;
	}
};

/** All information about a static-mesh vertex with a variable number of texture coordinates. No position data */
template<UINT NumTexCoords>
struct TLegacyStaticMeshNoPositionVertex
{
	FLOAT ShadowExtrusionPredicate;
	FPackedNormal TangentX;
	FPackedNormal TangentY;
	FPackedNormal TangentZ;
	FColor Color;
	FVector2D UVs[NumTexCoords];

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TLegacyStaticMeshNoPositionVertex& Vertex)
	{
		Ar << Vertex.ShadowExtrusionPredicate << Vertex.TangentX << Vertex.TangentY << Vertex.TangentZ;
		if(Ar.Ver() >= VER_STATICMESH_VERTEXCOLOR)
		{
			Ar << Vertex.Color;
		}
		for(UINT UVIndex = 0;UVIndex < NumTexCoords;UVIndex++)
		{
			Ar << Vertex.UVs[UVIndex];
		}
		return Ar;
	}
};

/** The implementation of the static mesh vertex data storage type. */
template<UINT NumTexCoords,template<UINT> class VertexType>
class TLegacyStaticMeshVertexData :
	public FStaticMeshVertexDataInterface,
	public TResourceArray<VertexType<NumTexCoords>,TRUE,VERTEXBUFFER_ALIGNMENT>
{
public:

	typedef TResourceArray<VertexType<NumTexCoords>,TRUE,VERTEXBUFFER_ALIGNMENT> ArrayType;

	virtual void ResizeBuffer(UINT NumVertices)
	{
		if((UINT)ArrayType::Num() < NumVertices)
		{
			// Enlarge the array.
			ArrayType::Add(NumVertices - ArrayType::Num());
		}
		else if((UINT)ArrayType::Num() > NumVertices)
		{
			// Shrink the array.
			ArrayType::Remove(NumVertices,ArrayType::Num() - NumVertices);
		}
	}
	virtual UINT GetStride() const
	{
		return sizeof(VertexType<NumTexCoords>);
	}
	virtual BYTE* GetDataPointer()
	{
		return (BYTE*)&(*this)(0);
	}
	virtual FResourceArrayInterface* GetResourceArray()
	{
		return this;
	}
	virtual void Serialize(FArchive& Ar)
	{
		ArrayType::BulkSerialize(Ar);
	}
};

/** Vertex buffer for a static mesh LOD */
class FLegacyStaticMeshVertexBuffer
{
public:

	/** Default constructor. */
	FLegacyStaticMeshVertexBuffer(UBOOL InbUsePositionData=TRUE)
	:	VertexData(NULL)
	,	Data(NULL)
	,	bUsePositionData(InbUsePositionData)
	{}
	
	/** Destructor. */
	~FLegacyStaticMeshVertexBuffer()
	{
		CleanUp();
	}

	/** Delete existing resources */
	void CleanUp()
	{
		delete VertexData;
		VertexData = NULL;
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshVertexBuffer& VertexBuffer)
	{
		Ar << VertexBuffer.NumTexCoords << VertexBuffer.Stride << VertexBuffer.NumVertices;

		if(Ar.IsLoading())
		{
			// Allocate the vertex data storage type.
			VertexBuffer.AllocateData();
		}

		// Serialize the vertex data.
		VertexBuffer.VertexData->Serialize(Ar);

		// Make a copy of the vertex data pointer.
		VertexBuffer.Data = VertexBuffer.VertexData->GetDataPointer();

		return Ar;
	}

	// Vertex data accessors.
	const FVector& VertexPosition(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			static FVector ZeroVec(0,0,0);
			return ZeroVec;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->Position;
		}		
	}

	const FLOAT& VertexShadowExtrusionPredicate(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->ShadowExtrusionPredicate;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->ShadowExtrusionPredicate;
		}
	}

	const FPackedNormal& VertexTangentX(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentX;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentX;
		}
	}

	const FPackedNormal& VertexTangentY(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentY;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentY;
		}
	}

	const FPackedNormal& VertexTangentZ(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentZ;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->TangentZ;
		}
	}

	const FColor& VertexColor(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->Color;
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->Color;
		}
	}

	const FVector2D& VertexUV(UINT VertexIndex,UINT UVIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUsePositionData )
		{
			return ((TLegacyStaticMeshNoPositionVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex];
		}
		else
		{
			return ((TLegacyStaticMeshFullVertex<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex];
		}
	}

	// Other accessors.
	UINT GetStride() const
	{
		return Stride;
	}
	UINT GetNumVertices() const
	{
		return NumVertices;
	}
	UINT GetNumTexCoords() const
	{
		return NumTexCoords;
	}
private:

	/** The vertex data storage type */
	class FStaticMeshVertexDataInterface* VertexData;

	/** The number of texcoords/vertex in the buffer. */
	UINT NumTexCoords;

	/** The cached vertex data pointer. */
	BYTE* Data;

	/** The cached vertex stride. */
	UINT Stride;

	/** The cached number of vertices. */
	UINT NumVertices;

	/** If TRUE then the position data is serialized */
	UBOOL bUsePositionData;

	/** Allocates the vertex data storage type. */
	void AllocateData()
	{
		// Clear any old VertexData before allocating.
		CleanUp();

		if( !bUsePositionData )
		{
			switch(NumTexCoords)
			{
			case 1: VertexData = new TLegacyStaticMeshVertexData<1,TLegacyStaticMeshNoPositionVertex>; break;
			case 2: VertexData = new TLegacyStaticMeshVertexData<2,TLegacyStaticMeshNoPositionVertex>; break;
			case 3: VertexData = new TLegacyStaticMeshVertexData<3,TLegacyStaticMeshNoPositionVertex>; break;
			case 4: VertexData = new TLegacyStaticMeshVertexData<4,TLegacyStaticMeshNoPositionVertex>; break;
			default: appErrorf(TEXT("Invalid number of texture coordinates"));
			};
		}
		else
		{
			switch(NumTexCoords)
			{
			case 1: VertexData = new TLegacyStaticMeshVertexData<1,TLegacyStaticMeshFullVertex>; break;
			case 2: VertexData = new TLegacyStaticMeshVertexData<2,TLegacyStaticMeshFullVertex>; break;
			case 3: VertexData = new TLegacyStaticMeshVertexData<3,TLegacyStaticMeshFullVertex>; break;
			case 4: VertexData = new TLegacyStaticMeshVertexData<4,TLegacyStaticMeshFullVertex>; break;
			default: appErrorf(TEXT("Invalid number of texture coordinates"));
			};
		}

		// Calculate the vertex stride.
		Stride = VertexData->GetStride();
	}
};

//
//	FStaticMeshVertex
//

struct FLegacyStaticMeshVertex
{
	FVector			Position;
	FPackedNormal	TangentX,
		TangentY,
		TangentZ;

	// Serializer.

	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshVertex& V)
	{
		// @warning BulkSerialize: FStaticMeshVertex is serialized as memory dump
		// See TArray::BulkSerialize for detailed description of implied limitations.
		Ar << V.Position;
		Ar << V.TangentX << V.TangentY << V.TangentZ;
		return Ar;
	}
};

/** Type for serialization of legacy position vertex buffer. */
struct FLegacyStaticMeshPositionVertexBuffer
{
	TResourceArray<FShadowVertex,TRUE,VERTEXBUFFER_ALIGNMENT> Positions;
	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshPositionVertexBuffer& B)
	{
		B.Positions.BulkSerialize(Ar);
		return Ar;
	}
};

/** Type for serialization of legacy tangent vertex buffer. */
struct FLegacyStaticMeshTangentVertexBuffer
{
	TResourceArray<FPackedNormal,TRUE,VERTEXBUFFER_ALIGNMENT> Tangents;
	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshTangentVertexBuffer& B)
	{
		B.Tangents.BulkSerialize(Ar);
		return Ar;
	}
};

/** Type for serialization of legacy UV vertex buffer. */
struct FLegacyStaticMeshUVBuffer
{
	TResourceArray<FVector2D,FALSE,VERTEXBUFFER_ALIGNMENT> UVs;
	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshUVBuffer& B)
	{
		B.UVs.BulkSerialize( Ar );
		if(Ar.Ver() < VER_RENDERING_REFACTOR)
		{
			UINT LegacySize;
			Ar << LegacySize;
		}
		return Ar;
	}
};

class FLegacyStaticMeshLightSerializer
{
public:
	FGuid Guid;
	FByteBulkData Visibility;
	UINT Size;

	void Serialize( FArchive& Ar, UObject* Owner )
	{
		Ar << Guid;
		if( Ar.Ver() < VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA )
		{
			Visibility.SerializeLikeLazyArray( Ar, Owner );
		}
		else
		{
			Visibility.Serialize( Ar, Owner );
		}
		Ar << Size;
	}
};

#endif // __UNSTATICMESHLEGACY_H__
