/*=============================================================================
	UnStaticMesh.h: Static mesh class definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#define STATICMESH_VERSION 15

/**
 * FStaticMeshTriangle
 *
 * @warning BulkSerialize: FStaticMeshTriangle is serialized as memory dump
 * See TArray::BulkSerialize for detailed description of implied limitations.
 */
struct FStaticMeshTriangle
{
	FVector		Vertices[3];
	FVector2D	UVs[3][8];
	FColor		Colors[3];
	INT			MaterialIndex;
	DWORD		SmoothingMask;
	INT			NumUVs;
};

/**
 * Bulk data array of FStaticMeshTriangle
 */
struct FStaticMeshTriangleBulkData : public FUntypedBulkData
{
	/**
	 * Returns size in bytes of single element.
	 *
	 * @return Size in bytes of single element
	 */
	virtual INT GetElementSize() const;

	/**
	 * Serializes an element at a time allowing and dealing with endian conversion and backward compatiblity.
	 * 
	 * @warning BulkSerialize: FStaticMeshTriangle is serialized as memory dump
	 * See TArray::BulkSerialize for detailed description of implied limitations.
	 *
	 * @param Ar			Archive to serialize with
	 * @param Data			Base pointer to data
	 * @param ElementIndex	Element index to serialize
	 */
	virtual void SerializeElement( FArchive& Ar, void* Data, INT ElementIndex );

	/**
	 * Returns whether single element serialization is required given an archive. This e.g.
	 * can be the case if the serialization for an element changes and the single element
	 * serialization code handles backward compatibility.
	 */
	virtual UBOOL RequiresSingleElementSerialization( FArchive& Ar );
};

/** 
 * All information about a static-mesh vertex with a variable number of texture coordinates.  16 bit float UVs
 * Position information is stored separately to reduce vertex fetch bandwidth in passes that only need position. (z prepass)
 */
struct FStaticMeshFullVertex
{
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FColor Color;	
};

template<UINT NumTexCoords>
struct TStaticMeshFullVertexFloat16UVs : public FStaticMeshFullVertex
{
	FVector2DHalf UVs[NumTexCoords];

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TStaticMeshFullVertexFloat16UVs& Vertex)
	{
		Ar << Vertex.TangentX;
		if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
		{
			FPackedNormal TangentY;
			Ar << TangentY;
			Ar << Vertex.TangentZ;
			// store the sign of the determinant in TangentZ.W
			Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte( Vertex.TangentX, TangentY, Vertex.TangentZ );
		}
		else
		{
			Ar << Vertex.TangentZ;
		}
		Ar << Vertex.Color;

		for(UINT UVIndex = 0;UVIndex < NumTexCoords;UVIndex++)
		{
			if( Ar.Ver() < VER_USE_FLOAT16_MESH_UVS )
			{
				FVector2D TempUVs;
				Ar << TempUVs;
				Vertex.UVs[UVIndex] = TempUVs;
			}
			else
			{
				Ar << Vertex.UVs[UVIndex];
			}
		}
		return Ar;
	}
};

template<UINT NumTexCoords>
struct TStaticMeshFullVertexFloat32UVs : public FStaticMeshFullVertex
{
	FVector2D UVs[NumTexCoords];

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TStaticMeshFullVertexFloat32UVs& Vertex)
	{
		Ar << Vertex.TangentX;
		if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
		{
			FPackedNormal TangentY;
			Ar << TangentY;
			Ar << Vertex.TangentZ;
			// store the sign of the determinant in TangentZ.W
			Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte( Vertex.TangentX, TangentY, Vertex.TangentZ );
		}
		else
		{
			Ar << Vertex.TangentZ;
		}
		Ar << Vertex.Color;

		for(UINT UVIndex = 0;UVIndex < NumTexCoords;UVIndex++)
		{
			if( Ar.Ver() < VER_USE_FLOAT16_MESH_UVS )
			{
				FVector2D TempUVs;
				Ar << TempUVs;
				Vertex.UVs[UVIndex] = TempUVs;
			}
			else
			{
				Ar << Vertex.UVs[UVIndex];
			}
		}
		return Ar;
	}
};

/** The information used to build a static-mesh vertex. */
struct FStaticMeshBuildVertex
{
	FVector Position;
	FPackedNormal TangentX;
	FPackedNormal TangentY;
	FPackedNormal TangentZ;
	FVector2D UVs[MAX_TEXCOORDS];
	FColor Color;
};

/**
 * A set of static mesh triangles which are rendered with the same material.
 */
class FStaticMeshElement
{
public:

	UMaterialInterface*		Material;
	/** A work area to hold the imported name during ASE importing (transient, should not be serialized) */
	FString					Name;			

	UBOOL					EnableCollision,
							OldEnableCollision,
							bEnableShadowCasting;

	UINT					FirstIndex,
							NumTriangles,
							MinVertexIndex,
							MaxVertexIndex;

	/** Constructor. */
	FStaticMeshElement():
		Material(NULL),
		EnableCollision(FALSE),
		OldEnableCollision(FALSE),
		bEnableShadowCasting(TRUE)
	{}

	FStaticMeshElement(UMaterialInterface* InMaterial):
		Material(InMaterial),
		EnableCollision(TRUE),
		OldEnableCollision(TRUE),
		bEnableShadowCasting(TRUE)
	{
		EnableCollision = OldEnableCollision = 1;
	}

	UBOOL operator==( const FStaticMeshElement& In ) const
	{
		return Material==In.Material;
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshElement& E)
	{
		if ( Ar.Ver() < VER_SHADOW_CASTING_PER_MESH_ELEMENT && Ar.IsLoading() )
		{
			return Ar	<< E.Material
						<< E.EnableCollision
						<< E.OldEnableCollision
						<< E.FirstIndex
						<< E.NumTriangles
						<< E.MinVertexIndex
						<< E.MaxVertexIndex;
		}
		else
		{
			return Ar	<< E.Material
						<< E.EnableCollision
						<< E.OldEnableCollision
						<< E.bEnableShadowCasting
						<< E.FirstIndex
						<< E.NumTriangles
						<< E.MinVertexIndex
						<< E.MaxVertexIndex;
		}
	}
};

/** An interface to the static-mesh vertex data storage type. */
class FStaticMeshVertexDataInterface
{
public:

	/** Virtual destructor. */
	virtual ~FStaticMeshVertexDataInterface() {}

	/**
	* Resizes the vertex data buffer, discarding any data which no longer fits.
	* @param NumVertices - The number of vertices to allocate the buffer for.
	*/
	virtual void ResizeBuffer(UINT NumVertices) = 0;

	/** @return The stride of the vertex data in the buffer. */
	virtual UINT GetStride() const = 0;

	/** @return A pointer to the data in the buffer. */
	virtual BYTE* GetDataPointer() = 0;

	/** @return A pointer to the FResourceArrayInterface for the vertex data. */
	virtual FResourceArrayInterface* GetResourceArray() = 0;

	/** Serializer. */
	virtual void Serialize(FArchive& Ar) = 0;
};

/** A vertex that stores just position. */
struct FPositionVertex
{
	FVector	Position;

	friend FArchive& operator<<(FArchive& Ar,FPositionVertex& V)
	{
		Ar << V.Position;
		return Ar;
	}
};

/** A vertex buffer of positions. */
class FPositionVertexBuffer : public FVertexBuffer
{
public:

	/** Default constructor. */
	FPositionVertexBuffer();

	/** Destructor. */
	~FPositionVertexBuffer();

	/** Delete existing resources */
	void CleanUp();

	/**
	* Initializes the buffer with the given vertices, used to convert legacy layouts.
	* @param InVertices - The vertices to initialize the buffer with.
	*/
	void Init(const class FLegacyStaticMeshVertexBuffer& InVertexBuffer);

	/**
	* Initializes the buffer with the given vertices, used to convert legacy layouts.
	* @param InVertices - The vertices to initialize the buffer with.
	*/
	void Init(const TArray<FStaticMeshBuildVertex>& InVertices);

	/**
	* Removes the cloned vertices used for extruding shadow volumes.
	* @param NumVertices - The real number of static mesh vertices which should remain in the buffer upon return.
	*/
	void RemoveShadowVolumeVertices(UINT InNumVertices);

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FPositionVertexBuffer& VertexBuffer);

	/**
	* Specialized assignment operator, only used when importing LOD's. 
	*/
	void operator=(const FPositionVertexBuffer &Other);

	// Vertex data accessors.
	FORCEINLINE FVector& VertexPosition(UINT VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FPositionVertex*)(Data + VertexIndex * Stride))->Position;
	}
	FORCEINLINE const FVector& VertexPosition(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FPositionVertex*)(Data + VertexIndex * Stride))->Position;
	}
	// Other accessors.
	FORCEINLINE UINT GetStride() const
	{
		return Stride;
	}
	FORCEINLINE UINT GetNumVertices() const
	{
		return NumVertices;
	}

	// FRenderResource interface.
	virtual void InitRHI();
	virtual FString GetFriendlyName() const { return TEXT("PositionOnly Static-mesh vertices"); }

private:

	/** The vertex data storage type */
	class FPositionVertexData* VertexData;

	/** The cached vertex data pointer. */
	BYTE* Data;

	/** The cached vertex stride. */
	UINT Stride;

	/** The cached number of vertices. */
	UINT NumVertices;

	/** Allocates the vertex data storage type. */
	void AllocateData();
};

/** Vertex buffer for a static mesh LOD */
class FStaticMeshVertexBuffer : public FVertexBuffer
{
public:

	/** Default constructor. */
	FStaticMeshVertexBuffer();

	/** Destructor. */
	~FStaticMeshVertexBuffer();

	/** Delete existing resources */
	void CleanUp();

	/**
	* Initializes the buffer with the given vertices.
	* @param InVertexBuffer - The legacy vertex buffer to copy data from.
	*/
	void Init(const class FLegacyStaticMeshVertexBuffer& InVertexBuffer);

	/**
	 * Initializes the buffer with the given vertices.
	 * @param InVertices - The vertices to initialize the buffer with.
	 * @param InNumTexCoords - The number of texture coordinate to store in the buffer.
	 */
	void Init(const TArray<FStaticMeshBuildVertex>& InVertices,UINT InNumTexCoords);

	/**
	 * Removes the cloned vertices used for extruding shadow volumes.
	 * @param NumVertices - The real number of static mesh vertices which should remain in the buffer upon return.
	 */
	void RemoveShadowVolumeVertices(UINT NumVertices);

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshVertexBuffer& VertexBuffer);

	/**
	* Specialized assignment operator, only used when importing LOD's. 
	*/
	void operator=(const FStaticMeshVertexBuffer &Other);

	FORCEINLINE FPackedNormal& VertexTangentX(UINT VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->TangentX;
	}
	FORCEINLINE const FPackedNormal& VertexTangentX(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->TangentX;
	}

	/**
	* Calculate the binormal (TangentY) vector using the normal,tangent vectors
	*
	* @param VertexIndex - index into the vertex buffer
	* @return binormal (TangentY) vector
	*/
	FORCEINLINE FVector VertexTangentY(UINT VertexIndex) const
	{
		const FPackedNormal& TangentX = VertexTangentX(VertexIndex);
		const FPackedNormal& TangentZ = VertexTangentZ(VertexIndex);
		return (FVector(TangentZ) ^ FVector(TangentX)) * ((FLOAT)TangentZ.Vector.W  / 127.5f - 1.0f);
	}

	FORCEINLINE FPackedNormal& VertexTangentZ(UINT VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->TangentZ;
	}
	FORCEINLINE const FPackedNormal& VertexTangentZ(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->TangentZ;
	}

	FORCEINLINE FColor& VertexColor(UINT VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->Color;
	}

	FORCEINLINE const FColor& VertexColor(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshFullVertex*)(Data + VertexIndex * Stride))->Color;
	}

	/**
	* Set the vertex UV values at the given index in the vertex buffer
	*
	* @param VertexIndex - index into the vertex buffer
	* @param UVIndex - [0,MAX_TEXCOORDS] value to index into UVs array
	* @param Vec2D - UV values to set
	*/
	FORCEINLINE void SetVertexUV(UINT VertexIndex,UINT UVIndex,const FVector2D& Vec2D)
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUseFullPrecisionUVs )
		{
			((TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex] = Vec2D;
		}
		else
		{
			((TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex] = Vec2D;
		}		
	}

	/**
	* Fet the vertex UV values at the given index in the vertex buffer
	*
	* @param VertexIndex - index into the vertex buffer
	* @param UVIndex - [0,MAX_TEXCOORDS] value to index into UVs array
	* @param 2D UV values
	*/
	FORCEINLINE const FVector2D GetVertexUV(UINT VertexIndex,UINT UVIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		if( !bUseFullPrecisionUVs )
		{
			return ((TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex];
		}
		else
		{
			return ((TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>*)(Data + VertexIndex * Stride))->UVs[UVIndex];
		}		
	}

	// Other accessors.
	FORCEINLINE UINT GetStride() const
	{
		return Stride;
	}
	FORCEINLINE UINT GetNumVertices() const
	{
		return NumVertices;
	}
	FORCEINLINE UINT GetNumTexCoords() const
	{
		return NumTexCoords;
	}
	FORCEINLINE UBOOL GetUseFullPrecisionUVs() const
	{
		return bUseFullPrecisionUVs;
	}
	FORCEINLINE void SetUseFullPrecisionUVs(UBOOL UseFull)
	{
		bUseFullPrecisionUVs = UseFull;
	}

	// FRenderResource interface.
	virtual void InitRHI();
	virtual FString GetFriendlyName() const { return TEXT("Static-mesh vertices"); }

private:

	/** The vertex data storage type */
	FStaticMeshVertexDataInterface* VertexData;

	/** The number of texcoords/vertex in the buffer. */
	UINT NumTexCoords;

	/** The cached vertex data pointer. */
	BYTE* Data;

	/** The cached vertex stride. */
	UINT Stride;

	/** The cached number of vertices. */
	UINT NumVertices;

	/** Corresponds to UStaticMesh::UseFullPrecisionUVs. if TRUE then 32 bit UVs are used */
	UBOOL bUseFullPrecisionUVs;

	/** Allocates the vertex data storage type. */
	void AllocateData();
};

/** 
* A vertex that stores a shadow volume extrusion info. 
*/
struct FStaticMeshShadowExtrusionVertex
{
	FLOAT ShadowExtrusionPredicate;

	friend FArchive& operator<<(FArchive& Ar,FStaticMeshShadowExtrusionVertex& V)
	{
		Ar << V.ShadowExtrusionPredicate;
		return Ar;
	}
};

/** 
* A vertex buffer with shadow volume extrusion data. 
*/
class FShadowExtrusionVertexBuffer : public FVertexBuffer
{
public:

	/** 
	* Default constructor. 
	*/
	FShadowExtrusionVertexBuffer();

	/** 
	* Destructor. 
	*/
	~FShadowExtrusionVertexBuffer();

	/** 
	* Delete existing resources 
	*/
	void CleanUp();

	/**
	* Initializes the buffer with the given vertices, used to convert legacy layouts.
	*
	* @param InVertices - The vertices to initialize the buffer with.
	*/
	void Init(const class FLegacyStaticMeshVertexBuffer& InVertexBuffer);

	/**
	* Initializes the buffer with the given vertices, used to convert legacy layouts.
	*
	* @param InVertices - The vertices to initialize the buffer with.
	*/
	void Init(const TArray<FStaticMeshBuildVertex>& InVertices);

	/**
	* Removes the cloned vertices used for extruding shadow volumes.
	*
	* @param NumVertices - The real number of static mesh vertices which should remain in the buffer upon return.
	*/
	void RemoveShadowVolumeVertices(UINT InNumVertices);

	/** 
	* Serializer. 
	* 
	* @param Ar - archive to serialize with
	* @param VertexBuffer - data to serialize to/from
	* @return archive that was used
	*/
	friend FArchive& operator<<(FArchive& Ar,FShadowExtrusionVertexBuffer& VertexBuffer);

	/**
	* Specialized assignment operator, only used when importing LOD's. 
	*
	* @param Other - instance to copy from
	*/
	void operator=(const FShadowExtrusionVertexBuffer &Other);

	// Vertex data accessors.
	FORCEINLINE FLOAT& VertexShadowExtrusionPredicate(UINT VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshShadowExtrusionVertex*)(Data + VertexIndex * Stride))->ShadowExtrusionPredicate;
	}
	FORCEINLINE const FLOAT& VertexShadowExtrusionPredicate(UINT VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return ((FStaticMeshShadowExtrusionVertex*)(Data + VertexIndex * Stride))->ShadowExtrusionPredicate;
	}
	// Other accessors.
	FORCEINLINE UINT GetStride() const
	{
		return Stride;
	}
	FORCEINLINE UINT GetNumVertices() const
	{
		return NumVertices;
	}

	// FRenderResource interface.

	/** 
	* Initialize the vertex buffer rendering resource. Called by the render thread
	*/ 
	virtual void InitRHI();

private:

	/** The vertex data storage type */
	class FStaticMeshShadowExtrusionVertexData* VertexData;

	/** The cached vertex data pointer. */
	BYTE* Data;

	/** The cached vertex stride. */
	UINT Stride;

	/** The cached number of vertices. */
	UINT NumVertices;

	/** Allocates the vertex data storage type. */
	void AllocateData();
};

#include "UnkDOP.h"

/**
* FStaticMeshRenderData - All data to define rendering properties for a certain LOD model for a mesh.
*/
class FStaticMeshRenderData
{
public:

	/** The buffer containing vertex data. */
	FStaticMeshVertexBuffer VertexBuffer;
	/** The buffer containing the position vertex data. */
	FPositionVertexBuffer PositionVertexBuffer;
	/** The buffer containing the shadow volume extrusion factors */
	FShadowExtrusionVertexBuffer ShadowExtrusionVertexBuffer;

	/** The number of vertices in the LOD. */
	UINT NumVertices;
	/** Index buffer resource for rendering */
	FRawStaticIndexBuffer<TRUE>				IndexBuffer;
	/** Index buffer resource for rendering wireframe mode */
	FRawIndexBuffer							WireframeIndexBuffer;
	/** Index buffer resource for rendering wireframe mode */
	TArray<FStaticMeshElement>				Elements;
	TArray<FMeshEdge>						Edges;
	TArray<BYTE>							ShadowTriangleDoubleSided;
	/** Source data for mesh */
	FStaticMeshTriangleBulkData				RawTriangles;

	/**
	 * Special serialize function passing the owning UObject along as required by FUnytpedBulkData
	 * serialization.
	 *
	 * @param	Ar		Archive to serialize with
	 * @param	Owner	UObject this structure is serialized within
	 */
	void Serialize( FArchive& Ar, UObject* Owner );

	/** Constructor */
	FStaticMeshRenderData();

	/** @return The triangle count of this LOD. */
	INT GetTriangleCount() const;

	/**
	* Fill an array with triangles which will be used to build a KDOP tree
	* @param kDOPBuildTriangles - the array to fill
	*/
	void GetKDOPTriangles(TArray<FkDOPBuildCollisionTriangle<WORD> >& kDOPBuildTriangles);

	/**
	* Build rendering data from a raw triangle stream
	* @param kDOPBuildTriangles output collision tree. A dummy can be passed if you do not specify BuildKDop as TRUE
	* @param Whether to build and return a kdop tree from the mesh data
	* @param Parent Parent mesh
	*/
	void Build(TArray<FkDOPBuildCollisionTriangle<WORD> >& kDOPBuildTriangles, UBOOL BuildKDop, class UStaticMesh* Parent);

	/**
	 * Initialize the LOD's render resources.
	 * @param Parent Parent mesh
	 */
	void InitResources(class UStaticMesh* Parent);

	/** Releases the LOD's render resources. */
	void ReleaseResources();

	// Rendering data.
	FLocalVertexFactory VertexFactory;
	FLocalShadowVertexFactory ShadowVertexFactory;
};

/** Used to expose information in the editor for one section or a particular LOD. */
struct FStaticMeshLODElement
{
	/** Material to use for this section of this LOD. */
	UMaterialInterface*	Material;

	/** Whether to enable shadow casting for this section of this LOD. */
	UBOOL bEnableShadowCasting;

	/** Whether to enable collision for this section of this LOD */
	BITFIELD bEnableCollision:1;

	friend FArchive& operator<<(FArchive& Ar,FStaticMeshLODElement& LODElement)
	{
		// For GC - serialise pointer to material
		if( !Ar.IsLoading() && !Ar.IsSaving() )
		{
			Ar << LODElement.Material;
		}
		return Ar;
	}
};

/**
 * FStaticMeshLODInfo - Editor-exposed properties for a specific LOD
 */
struct FStaticMeshLODInfo
{
	/** Used to expose properties for each */
	TArray<FStaticMeshLODElement>			Elements;

	friend FArchive& operator<<(FArchive& Ar,FStaticMeshLODInfo& LODInfo)
	{
		if( Ar.Ver() < VER_RENDERING_REFACTOR )
		{
			FLOAT Dummy;
			Ar << Dummy;
		}

		// For GC - serialise pointer to materials
		if( !Ar.IsLoading() && !Ar.IsSaving() )
		{
			Ar << LODInfo.Elements;
		}
		return Ar;
	}
};

//
//	UStaticMesh
//

class UStaticMesh : public UObject
{
	DECLARE_CLASS(UStaticMesh,UObject,CLASS_SafeReplace|CLASS_CollapseCategories|CLASS_Intrinsic,Engine);
public:
	/** Array of LODs, holding their associated rendering and collision data */
	TIndirectArray<FStaticMeshRenderData>	LODModels;
	/** Per-LOD information exposed to the editor */
	TArray<FStaticMeshLODInfo>				LODInfo;
	/** LOD distance ratio for this mesh */
	FLOAT									LODDistanceRatio;
	/** Range at which only the lowest detail LOD can be displayed */
	FLOAT									LODMaxRange;
	FRotator								ThumbnailAngle;
	FLOAT									ThumbnailDistance;

	INT										LightMapResolution,
											LightMapCoordinateIndex;
	
	// Collision data.

	typedef TkDOPTree<class FStaticMeshCollisionDataProvider,WORD> kDOPTreeType;
	kDOPTreeType							kDOPTree;

	URB_BodySetup*							BodySetup;
	FBoxSphereBounds						Bounds;

	/** Array of physics-engine shapes that can be used by multiple StaticMeshComponents. */
	TArray<void*>							PhysMesh;

	/** Scale of each PhysMesh entry. Arrays should be same size. */
	TArray<FVector>							PhysMeshScale3D;

	// Artist-accessible options.

	UBOOL									UseSimpleLineCollision,
											UseSimpleBoxCollision,
											UseSimpleRigidBodyCollision,
											DoubleSidedShadowVolumes,
											UseFullPrecisionUVs;

	INT										InternalVersion;

	/** A fence which is used to keep track of the rendering thread releasing the static mesh resources. */
	FRenderCommandFence ReleaseResourcesFence;

	// UObject interface.

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	virtual void PreEditChange(UProperty* PropertyAboutToChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual UBOOL Rename( const TCHAR* NewName=NULL, UObject* NewOuter=NULL, ERenameFlags Flags=REN_None );

	/** Set the physics triangle mesh representations owned by this StaticMesh to be destroyed. */
	void ClearPhysMeshCache();

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform);

	/**
	 * Called after duplication & serialization and before PostLoad. Used to e.g. make sure UStaticMesh's UModel
	 * gets copied as well.
	 */
	virtual void PostDuplicate();

	/**
	 * Callback used to allow object register its direct object references that are not already covered by
	 * the token stream.
	 *
	 * @param ObjectArray	array to add referenced objects to via AddReferencedObject
	 */
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );

	// UStaticMesh interface.

	void Build();

	/**
	 * Initialize the static mesh's render resources.
	 */
	void InitResources();

	/**
	 * Releases the static mesh's render resources.
	 */
	void ReleaseResources();

	/**
	 * Returns the scale dependent texture factor used by the texture streaming code.	
	 *
	 * @param RequestedUVIndex UVIndex to look at
	 * @return scale dependent texture factor
	 */
	FLOAT GetStreamingTextureFactor( INT UVIndex );

	/** 
	 * Returns a one line description of an object for viewing in the generic browser
	 */
	virtual FString GetDesc();

	/** 
	 * Returns detailed info to populate listview columns
	 */
	virtual FString GetDetailedDescription( INT InIndex );

	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
	 *
	 * @return size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual INT GetResourceSize();

	/** 
	* Removes all vertex data needed for shadow volume rendering 
	*/
	void RemoveShadowVolumeData();
};

struct FStaticMeshComponentLODInfo
{
	TArray<UShadowMap2D*> ShadowMaps;
	TArray<UShadowMap1D*> ShadowVertexBuffers;
	FLightMapRef LightMap;

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshComponentLODInfo& I)
	{
		return Ar << I.ShadowMaps << I.ShadowVertexBuffers << I.LightMap;
	}
};

//
//	UStaticMeshComponent
//

class UStaticMeshComponent : public UMeshComponent
{
	DECLARE_CLASS(UStaticMeshComponent,UMeshComponent,CLASS_NoExport,Engine);
public:
	UStaticMeshComponent();

	/** Force drawing of a specific lodmodel-1. 0 is automatic selection */
	INT									ForcedLodModel;
	/** LOD that was desired for rendering this StaticMeshComponent last frame. */
	INT									PreviousLODLevel;

	UStaticMesh* StaticMesh;
	FColor WireframeColor;

	/** 
	 *	Ignore this instance of this static mesh when calculating streaming information. 
	 *	This can be useful when doing things like applying character textures to static geometry, 
	 *	to avoid them using distance-based streaming.
	 */
	BITFIELD bIgnoreInstanceForTextureStreaming:1;

	/** Whether to override the lightmap resolution defined in the static mesh */
	BITFIELD bOverrideLightMapResolution:1;
	/** Light map resolution used if bOverrideLightMapResolution is TRUE */ 
	INT	OverriddenLightMapResolution;

	/** Subdivision step size for static vertex lighting.				*/
	INT	SubDivisionStepSize;
	/** Minimum number of subdivisions, needs to be at least 2.			*/
	INT MinSubDivisions;
	/** Maximum number of subdivisions.									*/
	INT MaxSubDivisions;
	/** Whether to use subdivisions or just the triangle's vertices.	*/
	BITFIELD bUseSubDivisions:1;

	TArray<FGuid> IrrelevantLights;	// Statically irrelevant lights.

	/** Per-LOD instance information */
	TArray<FStaticMeshComponentLODInfo> LODData;
	
	// UStaticMeshComponent interface

	void SetStaticMesh(UStaticMesh* NewMesh);

	/**
	 * Returns whether this primitive only uses unlit materials.
	 *
	 * @return TRUE if only unlit materials are used for rendering, false otherwise.
	 */
	virtual UBOOL UsesOnlyUnlitMaterials() const;

	/**
	 * Returns the lightmap resolution used for this primivite instnace in the case of it supporting texture light/ shadow maps.
	 * 0 if not supported or no static shadowing.
	 *
	 * @param Width		[out]	Width of light/shadow map
	 * @param Height	[out]	Height of light/shadow map
	 */
	virtual void GetLightMapResolution( INT& Width, INT& Height ) const;

	/**
	 * Returns the light and shadow map memory for this primite in its out variables.
	 *
	 * Shadow map memory usage is per light whereof lightmap data is independent of number of lights, assuming at least one.
	 *
	 * @param [out] LightMapMemoryUsage		Memory usage in bytes for light map (either texel or vertex) data
	 * @param [out]	ShadowMapMemoryUsage	Memory usage in bytes for shadow map (either texel or vertex) data
	 */
	virtual void GetLightAndShadowMapMemoryUsage( INT& LightMapMemoryUsage, INT& ShadowMapMemoryUsage ) const;

	/**
	 *	UStaticMeshComponent::GetMaterial
	 * @param MaterialIndex Index of material
	 * @param LOD Lod level to query from
	 * @return Material instance for this component at index
	 */
	virtual UMaterialInterface* GetMaterial(INT MaterialIndex, INT LOD) const;

	/**
	* Intersects a line with this component
	*
	* @param	LODIndex				LOD to check against
	*/
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags, UINT LODIndex);

	/** Initializes the resources used by the static mesh component. */
	private: void InitResources(); public:

	/**
	 * Updates all associated lightmaps with the new value for allowing directional lightmaps.
	 * This is only called when switching lightmap modes in-game and should never be called on cooked builds.
	 */
	virtual void UpdateLightmapType(UBOOL bAllowDirectionalLightMaps);

	// UMeshComponent interface.
	virtual INT GetNumElements() const;
	virtual UMaterialInterface* GetMaterial(INT MaterialIndex) const;

	// UPrimitiveComponent interface.
	virtual class FDecalRenderData* GenerateDecalRenderData(class FDecalState* Decal) const;
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options);
	virtual void GetStaticTriangles(FPrimitiveTriangleDefinitionInterface* PTDI) const;

	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags);
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);

	virtual void UpdateBounds();

	virtual void InitComponentRBPhys(UBOOL bFixed);
	virtual void TermComponentRBPhys(FRBPhysScene* InScene);
	virtual class URB_BodySetup* GetRBBodySetup();
	virtual FKCachedConvexData* GetCachedPhysConvexData(const FVector& InScale3D);

	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual UBOOL ShouldRecreateProxyOnUpdateTransform() const;

	// UActorComponent interface.
	virtual void CheckForErrors();

	protected: virtual void Attach(); public:
	virtual UBOOL IsValidComponent() const;

	virtual void InvalidateLightingCache();

	// UObject interface.
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize(FArchive& Ar);
	virtual void PostEditUndo();

	/**
	 * Called after all objects referenced by this object have been serialized. Order of PostLoad routed to 
	 * multiple objects loaded in one set is not deterministic though ConditionalPostLoad can be forced to
	 * ensure an object has been "PostLoad"ed.
	 */
	virtual void PostLoad();

	/** 
	 * Called when any property in this object is modified in UnrealEd
	 *
	 * @param	PropertyThatChanged		changed property
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Returns whether native properties are identical to the one of the passed in component.
	 *
	 * @param	Other	Other component to compare against
	 *
	 * @return TRUE if native properties are identical, FALSE otherwise
	 */
	virtual UBOOL AreNativePropertiesIdenticalTo( UComponent* Other ) const;

	// Script functions

	DECLARE_FUNCTION(execSetStaticMesh);
};

/**
 * This struct provides the interface into the static mesh collision data
 */
class FStaticMeshCollisionDataProvider
{
	/**
	 * The component this mesh is attached to
	 */
	const UStaticMeshComponent* Component;
	/**
	 * The mesh that is being collided against
	 */
	UStaticMesh* Mesh;

	/**
	 * The LOD that is being collided against
	 */
	UINT CurrentLOD;
	/**
	 * Pointer to vertex buffer containing position data.
	 */
	FPositionVertexBuffer* PositionVertexBuffer;

	/** Hide default ctor */
	FStaticMeshCollisionDataProvider(void)
	{
	}

public:
	/**
	 * Sets the component and mesh members
	 */
	FORCEINLINE FStaticMeshCollisionDataProvider(const UStaticMeshComponent* InComponent, UINT InCurrentLOD = 0) :
		Component(InComponent),
		Mesh(InComponent->StaticMesh),
		CurrentLOD(InCurrentLOD),
		PositionVertexBuffer(&InComponent->StaticMesh->LODModels(InCurrentLOD).PositionVertexBuffer)
	{
	}

	/**
	 * Given an index, returns the position of the vertex
	 *
	 * @param Index the index into the vertices array
	 */
	FORCEINLINE const FVector& GetVertex(WORD Index) const
	{
		return PositionVertexBuffer->VertexPosition(Index);
	}

	/**
	 * Returns the material for a triangle based upon material index
	 *
	 * @param MaterialIndex the index into the materials array
	 */
	FORCEINLINE UMaterialInterface* GetMaterial(WORD MaterialIndex) const
	{
		return Component->GetMaterial(MaterialIndex);
	}

	/**
	 * Returns the kDOPTree for this mesh
	 */
	FORCEINLINE const TkDOPTree<FStaticMeshCollisionDataProvider,WORD>& GetkDOPTree(void) const
	{
		return Mesh->kDOPTree;
	}

	/**
	 * Returns the local to world for the component
	 */
	FORCEINLINE const FMatrix& GetLocalToWorld(void) const
	{
		return Component->LocalToWorld;
	}

	/**
	 * Returns the world to local for the component
	 */
	FORCEINLINE const FMatrix GetWorldToLocal(void) const
	{
		return Component->LocalToWorld.Inverse();
	}

	/**
	 * Returns the local to world transpose adjoint for the component
	 */
	FORCEINLINE FMatrix GetLocalToWorldTransposeAdjoint(void) const
	{
		return Component->LocalToWorld.TransposeAdjoint();
	}

	/**
	 * Returns the determinant for the component
	 */
	FORCEINLINE FLOAT GetDeterminant(void) const
	{
		return Component->LocalToWorldDeterminant;
	}
};

//
//	FStaticMeshComponentReattachContext - Destroys StaticMeshComponents using a given StaticMesh and recreates them when destructed.
//	Used to ensure stale rendering data isn't kept around in the components when importing over or rebuilding an existing static mesh.
//

struct FStaticMeshComponentReattachContext
{
	UStaticMesh* StaticMesh;
	TIndirectArray<FComponentReattachContext> ReattachContexts;

	// Constructor/destructor.

	FStaticMeshComponentReattachContext(UStaticMesh* InStaticMesh):
		StaticMesh(InStaticMesh)
	{
		for(TObjectIterator<UStaticMeshComponent> It;It;++It)
		{
			if(It->StaticMesh == StaticMesh)
			{
				new(ReattachContexts) FComponentReattachContext(*It);
				if(It->IsAttached())
				{
					((UActorComponent*)*It)->InvalidateLightingCache();
				}
			}
		}

		// Flush the rendering commands generated by the detachments.
		// The static mesh scene proxies reference the UStaticMesh, and this ensures that they are cleaned up before the UStaticMesh changes.
		FlushRenderingCommands();
	}
};
