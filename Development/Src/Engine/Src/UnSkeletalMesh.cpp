/*=============================================================================
	UnSkeletalMesh.cpp: Unreal skeletal mesh and animation implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnFaceFXSupport.h"
#include "EngineAnimClasses.h"
#include "EnginePhysicsClasses.h"
#include "ScenePrivate.h"
#include "GPUSkinVertexFactory.h"
#include "UnSkeletalMeshSorting.h"
#include "EngineSequenceClasses.h"
#include "EngineInterpolationClasses.h"

#include "EngineMeshClasses.h"
#if WITH_APEX
#include "UnNovodexSupport.h"
#include "NvApexCommands.h"
#include "NvApexScene.h"
#include "NvApexRender.h"
#endif

#if WITH_FACEFX
using namespace OC3Ent;
using namespace Face;
#endif

IMPLEMENT_CLASS(USkeletalMesh);
IMPLEMENT_CLASS(USkeletalMeshSocket);
IMPLEMENT_CLASS(ASkeletalMeshActor);
IMPLEMENT_CLASS(ASkeletalMeshCinematicActor);
IMPLEMENT_CLASS(ASkeletalMeshActorMAT);
IMPLEMENT_CLASS(ASkeletalMeshActorBasedOnExtremeContent);
IMPLEMENT_CLASS(ASkeletalMeshActorSpawnable);

/*-----------------------------------------------------------------------------
	FSkeletalMeshVertexBuffer
-----------------------------------------------------------------------------*/

/**
* Constructor
*/
FSkeletalMeshVertexBuffer::FSkeletalMeshVertexBuffer() 
:	bInflucencesByteSwapped(FALSE)
,	bUseFullPrecisionUVs(FALSE)
,	bUseCPUSkinning(FALSE)
,	bUsePackedPosition(TRUE)
,	bProcessedPackedPositions(FALSE)
,	VertexData(NULL)
,	Data(NULL)
,	Stride(0)
,	NumVertices(0)
,	MeshOrigin(FVector(0.f, 0.f, 0.f))
, 	MeshExtension(FVector(1.f,1.f,1.f))
{
}

/**
* Destructor
*/
FSkeletalMeshVertexBuffer::~FSkeletalMeshVertexBuffer()
{
	CleanUp();
}

/**
* Assignment. Assumes that vertex buffer will be rebuilt 
*/
FSkeletalMeshVertexBuffer& FSkeletalMeshVertexBuffer::operator=(const FSkeletalMeshVertexBuffer& Other)
{
	VertexData = NULL;
	bUseFullPrecisionUVs = Other.bUseFullPrecisionUVs;
	bUsePackedPosition = Other.bUsePackedPosition;
	bUseCPUSkinning = Other.bUseCPUSkinning;
	return *this;
}

/**
* Constructor (copy)
*/
FSkeletalMeshVertexBuffer::FSkeletalMeshVertexBuffer(const FSkeletalMeshVertexBuffer& Other)
:	bInflucencesByteSwapped(FALSE)
,	bUseFullPrecisionUVs(Other.bUseFullPrecisionUVs)
,	bUseCPUSkinning(Other.bUseCPUSkinning)
,	bUsePackedPosition(Other.bUsePackedPosition)
,	bProcessedPackedPositions(Other.bProcessedPackedPositions)
,	VertexData(NULL)
,	Data(NULL)
,	Stride(0)
,	NumVertices(0)
,	MeshOrigin(Other.MeshOrigin)
, 	MeshExtension(Other.MeshExtension)
{
}

/**
 * @return text description for the resource type
 */
FString FSkeletalMeshVertexBuffer::GetFriendlyName() const
{ 
	return TEXT("Skeletal-mesh vertex buffer"); 
}

/** 
 * Delete existing resources 
 */
void FSkeletalMeshVertexBuffer::CleanUp()
{
	delete VertexData;
	VertexData = NULL;
}

/**
 * Initialize the RHI resource for this vertex buffer
 */
void FSkeletalMeshVertexBuffer::InitRHI()
{
	check(VertexData);
	FResourceArrayInterface* ResourceArray = VertexData->GetResourceArray();
	if( ResourceArray->GetResourceDataSize() > 0 )
	{
		// Create the vertex buffer.
		VertexBufferRHI = RHICreateVertexBuffer(ResourceArray->GetResourceDataSize(),ResourceArray,RUF_Static);
	}
}

/**
* Serializer for this class
* @param Ar - archive to serialize to
* @param B - data to serialize
*/
FArchive& operator<<(FArchive& Ar,FSkeletalMeshVertexBuffer& VertexBuffer)
{
	// This must be done before allocate data is called.
	if( Ar.Ver() >= VER_ADDED_MULTIPLE_UVS_TO_SKELETAL_MESH )
	{
		Ar << VertexBuffer.NumTexCoords;
	}
	else
	{
		// There is only one set of texture coordinates if this vertex buffer was saved before we supported multiple texture coordinate sets.
		VertexBuffer.NumTexCoords = 1;
	}

	if( Ar.IsSaving() && (GCookingTarget & UE3::PLATFORM_Console))
	{
		// Call the correct version of convert to packed position depending on how many texture coordinates we have 
		const UINT NumTexCoords = VertexBuffer.GetNumTexCoords();
		switch( NumTexCoords )
		{
		case 1: VertexBuffer.ConvertToPackedPosition<1>(); break;
		case 2: VertexBuffer.ConvertToPackedPosition<2>(); break;
		case 3: VertexBuffer.ConvertToPackedPosition<3>(); break;
		case 4: VertexBuffer.ConvertToPackedPosition<4>(); break;
		}

		if( Ar.ForceByteSwapping() && !VertexBuffer.bInflucencesByteSwapped )
		{
			// swap order for InfluenceBones/InfluenceWeights byte entries
			for( UINT VertIdx=0; VertIdx < VertexBuffer.GetNumVertices(); VertIdx++ )
			{
				FGPUSkinVertexBase& Vert = *VertexBuffer.GetVertexPtr(VertIdx);
				for( INT i=0; i < MAX_INFLUENCES/2; i++ )
				{
					Exchange(Vert.InfluenceBones[i],Vert.InfluenceBones[MAX_INFLUENCES-1-i]);
					Exchange(Vert.InfluenceWeights[i],Vert.InfluenceWeights[MAX_INFLUENCES-1-i]);
				}
			}
			VertexBuffer.bInflucencesByteSwapped=TRUE;
		}
	}

	if( Ar.Ver() < VER_USE_FLOAT16_SKELETAL_MESH_UVS )
	{
		// handle legacy data
 		TArray<FSoftSkinVertex> LegacyVerts;
 		LegacyVerts.BulkSerialize(Ar);
		VertexBuffer.Init(LegacyVerts);
	} 
	else
	{
		Ar << VertexBuffer.bUseFullPrecisionUVs;

		// I need to know this value before AllocateData
		if ( Ar.Ver() >= VER_SKELETAL_MESH_SUPPORT_PACKED_POSITION )
		{
			Ar << VertexBuffer.bUsePackedPosition;
			// Serialize MeshExtension and Origin
			// I need to save them for console to pick it up later
			Ar << VertexBuffer.MeshExtension << VertexBuffer.MeshOrigin;
		}

		if( Ar.IsLoading() )
		{
			// allocate vertex data on load
			VertexBuffer.AllocateData();
		}

		//Only allocate render resources if not running/cooking dedicated server
#if SUPPORTS_SCRIPTPATCH_CREATION
		UBOOL bShouldLoadVertexResources = Ar.IsLoading() && !GIsSeekFreePCServer && (GPatchingTarget != UE3::PLATFORM_WindowsServer);
#else
		UBOOL bShouldLoadVertexResources = Ar.IsLoading() && !GIsSeekFreePCServer;
#endif
		UBOOL bShouldSaveVertexResources = Ar.IsSaving() && (GCookingTarget != UE3::PLATFORM_WindowsServer);
		// if Ar is counting, it still should serialize. Need to count VertexData
		if (bShouldLoadVertexResources || bShouldSaveVertexResources || Ar.IsCountingMemory())
		{
			if( VertexBuffer.VertexData != NULL )
			{
				VertexBuffer.VertexData->Serialize(Ar);	

				// update cached buffer info
				VertexBuffer.Data = VertexBuffer.VertexData->GetDataPointer();
				VertexBuffer.Stride = VertexBuffer.VertexData->GetStride();
				VertexBuffer.NumVertices = VertexBuffer.VertexData->GetNumVertices();
			}
		}
	}

	return Ar;
}

/**
* Initializes the buffer with the given vertices.
* @param InVertices - The vertices to initialize the buffer with.
*/
void FSkeletalMeshVertexBuffer::Init(const TArray<FSoftSkinVertex>& InVertices)
{
	// Make sure if this is console, use compressed otherwise, use not compressed
	AllocateData();
	
	VertexData->ResizeBuffer(InVertices.Num());
	
	Data = VertexData->GetDataPointer();
	Stride = VertexData->GetStride();
	NumVertices = VertexData->GetNumVertices();
	
	for( INT VertIdx=0; VertIdx < InVertices.Num(); VertIdx++ )
	{
		const FSoftSkinVertex& SrcVertex = InVertices(VertIdx);
		SetVertex(VertIdx,SrcVertex);
	}
}


/** 
* @param UseCPUSkinning - set to TRUE if using cpu skinning with this vertex buffer
*/
void FSkeletalMeshVertexBuffer::SetUseCPUSkinning(UBOOL UseCPUSkinning)
{
	bUseCPUSkinning = UseCPUSkinning;
}

// Handy macro for allocating the correct vertex data class (which has to be known at compile time) depending on the data type and number of UVs.  
#define ALLOCATE_VERTEX_DATA_TEMPLATE( VertexDataType, NumUVs )											\
	switch(NumUVs)																						\
	{																									\
		case 1: VertexData = new TSkeletalMeshVertexData< VertexDataType<1> >(bNeedsCPUAccess); break;	\
		case 2: VertexData = new TSkeletalMeshVertexData< VertexDataType<2> >(bNeedsCPUAccess); break;	\
		case 3: VertexData = new TSkeletalMeshVertexData< VertexDataType<3> >(bNeedsCPUAccess); break;	\
		case 4: VertexData = new TSkeletalMeshVertexData< VertexDataType<4> >(bNeedsCPUAccess); break;	\
		default: appErrorf(TEXT("Invalid number of texture coordinates"));								\
	}																									\

/** 
* Allocates the vertex data storage type. 
*/
void FSkeletalMeshVertexBuffer::AllocateData()
{
	// Clear any old VertexData before allocating.
	CleanUp();

	// only set vertex data as CPU accessible on PS3 since we can read directly from local mem (although slow)
	// otherwise treat as CPU accessible since the vertex data must persist for mesh constructioning
#if PS3
	const UBOOL bNeedsCPUAccess = bUseCPUSkinning | ((const FSystemSettingsDataMesh&)GSystemSettings).bForceCPUAccessToGPUSkinVerts;
#else
	const UBOOL bNeedsCPUAccess = TRUE;
#endif

	if( !bUseFullPrecisionUVs )
	{	
		if (GetUsePackedPosition())
		{
			ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat16Uvs32Xyz, NumTexCoords );
		}
		else
		{
			ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat16Uvs, NumTexCoords );
		}
	}
	else
	{
		if (GetUsePackedPosition())
		{
			ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat32Uvs32Xyz, NumTexCoords );
		}
		else
		{
			ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat32Uvs, NumTexCoords );
		}

	}
}

/** 
* Allocates the vertex data to packed position type. 
* This is to avoid confusion from using AllocateData
* This only happens during cooking and other than that this won't be used
*/
template<UINT NumTexCoordsT>
void FSkeletalMeshVertexBuffer::AllocatePackedData(const TArray< TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT> >& InVertices)
{
	// Clear any old VertexData before allocating.
	CleanUp();

	check ( bUsePackedPosition );
	check ( !bUseFullPrecisionUVs );
	// only set vertex data as CPU accessible on PS3 since we can read directly from local mem (although slow)
	// otherwise treat as CPU accessible since the vertex data must persist for mesh constructioning
#if PS3
	const UBOOL bNeedsCPUAccess = bUseCPUSkinning | ((const FSystemSettingsDataMesh&)GSystemSettings).bForceCPUAccessToGPUSkinVerts;
#else
	const UBOOL bNeedsCPUAccess = TRUE;
#endif

	ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat16Uvs32Xyz, NumTexCoords );
	*(TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT> >*)VertexData = InVertices;

	Data = VertexData->GetDataPointer();
	Stride = VertexData->GetStride();
	NumVertices = VertexData->GetNumVertices();
}

/** 
* Allocates the vertex data to packed position type. 
* This is to avoid confusion from using AllocateData
* This only happens during cooking and other than that this won't be used
*/
template<UINT NumTexCoordsT>
void FSkeletalMeshVertexBuffer::AllocatePackedData(const TArray< TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT> >& InVertices)
{
	// Clear any old VertexData before allocating.
	CleanUp();

	check ( bUsePackedPosition );
	check ( bUseFullPrecisionUVs );

	// only set vertex data as CPU accessible on PS3 since we can read directly from local mem (although slow)
	// otherwise treat as CPU accessible since the vertex data must persist for mesh constructioning
#if PS3
	const UBOOL bNeedsCPUAccess = bUseCPUSkinning | ((const FSystemSettingsDataMesh&)GSystemSettings).bForceCPUAccessToGPUSkinVerts;
#else
	const UBOOL bNeedsCPUAccess = TRUE;
#endif

	ALLOCATE_VERTEX_DATA_TEMPLATE( TGPUSkinVertexFloat32Uvs32Xyz, NumTexCoords );
	*(TSkeletalMeshVertexData< TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT> >*)VertexData = InVertices;

	Data = VertexData->GetDataPointer();
	Stride = VertexData->GetStride();
	NumVertices = VertexData->GetNumVertices();

}

/** 
* Copy the contents of the source vertex to the destination vertex in the buffer 
*
* @param VertexIndex - index into the vertex buffer
* @param SrcVertex - source vertex to copy from
*/
void FSkeletalMeshVertexBuffer::SetVertex(UINT VertexIndex,const FSoftSkinVertex& SrcVertex)
{
	checkSlow(VertexIndex < GetNumVertices());
	BYTE* VertBase = Data + VertexIndex * Stride;
	((FGPUSkinVertexBase*)(VertBase))->TangentX = SrcVertex.TangentX;
	((FGPUSkinVertexBase*)(VertBase))->TangentZ = SrcVertex.TangentZ;
	// store the sign of the determinant in TangentZ.W
	((FGPUSkinVertexBase*)(VertBase))->TangentZ.Vector.W = GetBasisDeterminantSignByte( SrcVertex.TangentX, SrcVertex.TangentY, SrcVertex.TangentZ );
	appMemcpy(((FGPUSkinVertexBase*)(VertBase))->InfluenceBones,SrcVertex.InfluenceBones,sizeof(SrcVertex.InfluenceBones));
	appMemcpy(((FGPUSkinVertexBase*)(VertBase))->InfluenceWeights,SrcVertex.InfluenceWeights,sizeof(SrcVertex.InfluenceWeights));
	if( !bUseFullPrecisionUVs )
	{
#if CONSOLE // I don't expect this to happen in CONSOLE. If so this won't work with PackedPosition. Having check just in case.
		check (FALSE);
#else
		((TGPUSkinVertexFloat16Uvs<MAX_TEXCOORDS>*)(VertBase))->Position = SrcVertex.Position;
		for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
		{
			((TGPUSkinVertexFloat16Uvs<MAX_TEXCOORDS>*)(VertBase))->UVs[UVIndex] = FVector2DHalf( SrcVertex.UVs[UVIndex] );
		}
#endif
	}
	else
	{
#if CONSOLE // I don't expect this to happen in CONSOLE. If so this won't work with PackedPosition. Having check just in case.
		check (FALSE);
#else
		((TGPUSkinVertexFloat32Uvs<MAX_TEXCOORDS>*)(VertBase))->Position = SrcVertex.Position;
		for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
		{
			((TGPUSkinVertexFloat32Uvs<MAX_TEXCOORDS>*)(VertBase))->UVs[UVIndex] = FVector2D( SrcVertex.UVs[UVIndex] );
		}
#endif
	}
}

/**
* Convert the existing data in this mesh from 16 bit to 32 bit UVs.
* Without rebuilding the mesh (loss of precision)
*/
template<UINT NumTexCoordsT>
void FSkeletalMeshVertexBuffer::ConvertToFullPrecisionUVs()
{
	if( !bUseFullPrecisionUVs )
	{
		if ( GetUsePackedPosition() )
		{
			TArray< TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT> > DestVertexData;
			TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT> >& SrcVertexData = *(TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT> >*)VertexData;			
			DestVertexData.Add(SrcVertexData.Num());
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT>& DestVert = DestVertexData(VertIdx);
				appMemcpy(&DestVert,&SrcVert,sizeof(FGPUSkinVertexBase));
				DestVert.Position = SrcVert.Position;
				for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
				{
					DestVert.UVs[UVIndex] = FVector2D(SrcVert.UVs[UVIndex]);
				}
			}

			bUseFullPrecisionUVs = TRUE;
			
			*this =DestVertexData;
		}
		else
		{
			TArray< TGPUSkinVertexFloat32Uvs<NumTexCoordsT> > DestVertexData;
			TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs<NumTexCoordsT> >& SrcVertexData = *(TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs<NumTexCoordsT> >*)VertexData;			
			DestVertexData.Add(SrcVertexData.Num());
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat16Uvs<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				TGPUSkinVertexFloat32Uvs<NumTexCoordsT>& DestVert = DestVertexData(VertIdx);
				appMemcpy(&DestVert,&SrcVert,sizeof(FGPUSkinVertexBase));
				DestVert.Position = SrcVert.Position;
				for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
				{
					DestVert.UVs[UVIndex] = FVector2D(SrcVert.UVs[UVIndex]);
				}
				
			}

			bUseFullPrecisionUVs = TRUE;
			*this = DestVertexData;
		}
	}
}

/**
* Convert the existing data in this mesh from 12 bytes to 4 bytes xyz
*/
template<UINT NumTexCoordsT>
void FSkeletalMeshVertexBuffer::ConvertToPackedPosition()
{
	// Only process position packing once 
	if (bProcessedPackedPositions)
	{
		return;
	}
	bProcessedPackedPositions = TRUE;

	// if CPU skinning, then can't use packed position, set it to FALSE
	if( GUsingMobileRHI || bUseCPUSkinning == TRUE || (GCookingTarget & UE3::PLATFORM_Mobile))
	{
		bUsePackedPosition = FALSE;
	}

	if ( bUsePackedPosition )
	{
		FBox ExtentBox(0);
		FPackedPosition LocalPosition;

#if !FINAL_RELEASE
		FLOAT			ErrorCheck;
#endif
		if ( bUseFullPrecisionUVs )
		{
			// Get Extension first
			TSkeletalMeshVertexData< TGPUSkinVertexFloat32Uvs<NumTexCoordsT> >& SrcVertexData = *(TSkeletalMeshVertexData< TGPUSkinVertexFloat32Uvs<NumTexCoordsT> >*)VertexData;			
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat32Uvs<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				ExtentBox += SrcVert.Position;
			}

			MeshOrigin = ExtentBox.GetCenter();
			MeshExtension = ExtentBox.GetExtent();
			// I need to round up ExtendVector 
			MeshExtension = FVector(appFloor(MeshExtension.X + 1.0f), appFloor(MeshExtension.Y + 1.0f), appFloor(MeshExtension.Z + 1.0f));

			// now convert position from -1 to 1 within the box
			TArray< TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT> > DestVertexData;
			DestVertexData.Add(SrcVertexData.Num());
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat32Uvs<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				TGPUSkinVertexFloat32Uvs32Xyz<NumTexCoordsT>& DestVert = DestVertexData(VertIdx);
				appMemcpy(&DestVert,&SrcVert,sizeof(FGPUSkinVertexBase));

				DestVert.Position = (SrcVert.Position-MeshOrigin)/MeshExtension;
				for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
				{
					DestVert.UVs[UVIndex] = SrcVert.UVs[UVIndex];
				}

#if !FINAL_RELEASE
				// error check. Sadly I don't know what skeletalmesh's name is
				ErrorCheck = (SrcVert.Position-(FVector)DestVert.Position*MeshExtension-MeshOrigin).Size();
				if (ErrorCheck > 10.f)
				{
					debugf(TEXT("Compressing error is more than 10. Verify visual glitches. "));
				}
#endif
			}

			AllocatePackedData(DestVertexData);
		}
		else
		{
			// Get Extension first
			TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs<NumTexCoordsT> >& SrcVertexData = *(TSkeletalMeshVertexData< TGPUSkinVertexFloat16Uvs<NumTexCoordsT> >*)VertexData;			
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat16Uvs<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				ExtentBox += SrcVert.Position;
			}

			MeshOrigin = ExtentBox.GetCenter();
			MeshExtension = ExtentBox.GetExtent();
			// I need to round up ExtendVector
			MeshExtension = FVector(appFloor(MeshExtension.X + 1.0f), appFloor(MeshExtension.Y + 1.0f), appFloor(MeshExtension.Z + 1.0f));

			// now convert position from -1 to 1 within the box
			TArray< TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT> > DestVertexData;
			DestVertexData.Add(SrcVertexData.Num());
			for( INT VertIdx=0; VertIdx < SrcVertexData.Num(); VertIdx++ )
			{
				TGPUSkinVertexFloat16Uvs<NumTexCoordsT>& SrcVert = SrcVertexData(VertIdx);
				TGPUSkinVertexFloat16Uvs32Xyz<NumTexCoordsT>& DestVert = DestVertexData(VertIdx);
				appMemcpy(&DestVert,&SrcVert,sizeof(FGPUSkinVertexBase));

				DestVert.Position = (SrcVert.Position-MeshOrigin)/MeshExtension;
				
				for( UINT UVIndex = 0; UVIndex < NumTexCoords; ++UVIndex )
				{
					DestVert.UVs[UVIndex] = SrcVert.UVs[UVIndex];
				}

#if !FINAL_RELEASE
				// error check. Sadly I don't know what skeletalmesh's name is
				ErrorCheck = (SrcVert.Position-(FVector)DestVert.Position*MeshExtension-MeshOrigin).Size();
				if (ErrorCheck > 10.f)
				{
					debugf(TEXT("Compressing error is more than 10. Verify visual glitches. "));
				}
#endif
			}

			AllocatePackedData(DestVertexData);
		}
	}
	else
	{
		MeshExtension = FVector(1.f, 1.f, 1.f);
		MeshOrigin = FVector(0.f, 0.f, 0.f);
	}
}

/*-----------------------------------------------------------------------------
FSkeletalMeshVertexColorBuffer
-----------------------------------------------------------------------------*/

/**
 * Constructor
 */
FSkeletalMeshVertexColorBuffer::FSkeletalMeshVertexColorBuffer() 
:	VertexData(NULL),
	Data(NULL),
	Stride(0),
	NumVertices(0)
{

}

/**
 * Destructor
 */
FSkeletalMeshVertexColorBuffer::~FSkeletalMeshVertexColorBuffer()
{
	// clean up everything
	CleanUp();
}

/**
 * Assignment. Assumes that vertex buffer will be rebuilt 
 */

FSkeletalMeshVertexColorBuffer& FSkeletalMeshVertexColorBuffer::operator=(const FSkeletalMeshVertexColorBuffer& Other)
{
	VertexData = NULL;
	return *this;
}

/**
 * Copy Constructor
 */
FSkeletalMeshVertexColorBuffer::FSkeletalMeshVertexColorBuffer(const FSkeletalMeshVertexColorBuffer& Other)
:	VertexData(NULL),
	Data(NULL),
	Stride(0),
	NumVertices(0)
{

}

/**
 * @return text description for the resource type
 */
FString FSkeletalMeshVertexColorBuffer::GetFriendlyName() const
{
	return TEXT("Skeletal0-mesh vertex color buffer");
}

/** 
 * Delete existing resources 
 */
void FSkeletalMeshVertexColorBuffer::CleanUp()
{
	delete VertexData;
	VertexData = NULL;
}

/**
 * Initialize the RHI resource for this vertex buffer
 */
void FSkeletalMeshVertexColorBuffer::InitRHI()
{
	check(VertexData);
	FResourceArrayInterface* ResourceArray = VertexData->GetResourceArray();
	if( ResourceArray->GetResourceDataSize() > 0 )
	{
		VertexBufferRHI = RHICreateVertexBuffer( ResourceArray->GetResourceDataSize(), ResourceArray, RUF_Static );
	}
}

/**
 * Serializer for this class
 * @param Ar - archive to serialize to
 * @param B - data to serialize
 */
FArchive& operator<<( FArchive& Ar, FSkeletalMeshVertexColorBuffer& VertexBuffer )
{
	if( Ar.Ver() >= VER_ADDED_SKELETAL_MESH_VERTEX_COLORS )
	{
		if( Ar.IsLoading() )
		{
			VertexBuffer.AllocateData();
		}

		//Only allocate render resources if not running/cooking dedicated server
		UBOOL bShouldLoadVertexResources = Ar.IsLoading() && !GIsSeekFreePCServer;
		UBOOL bShouldSaveVertexResources = Ar.IsSaving() && (GCookingTarget != UE3::PLATFORM_WindowsServer);

		if ( bShouldLoadVertexResources || bShouldSaveVertexResources || Ar.IsCountingMemory() )
		{
			if( VertexBuffer.VertexData != NULL )
			{
				VertexBuffer.VertexData->Serialize( Ar );

				// update cached buffer info
				VertexBuffer.Data = VertexBuffer.VertexData->GetDataPointer();
				VertexBuffer.Stride = VertexBuffer.VertexData->GetStride();
				VertexBuffer.NumVertices = VertexBuffer.VertexData->GetNumVertices();
			}
		}
	}

	return Ar;
}

/**
 * Initializes the buffer with the given vertices.
 * @param InVertices - The vertices to initialize the buffer with.
 */
void FSkeletalMeshVertexColorBuffer::Init( const TArray<FSoftSkinVertex>& InVertices )
{
	// Allocate new data
	AllocateData();

	// Resize the buffer to hold enough data for all passed in vertices
	VertexData->ResizeBuffer( InVertices.Num() );

	Data = VertexData->GetDataPointer();
	Stride = VertexData->GetStride();
	NumVertices = VertexData->GetNumVertices();

	// Copy color info from each vertex
	for( INT VertIdx=0; VertIdx < InVertices.Num(); ++VertIdx )
	{
		const FSoftSkinVertex& SrcVertex = InVertices(VertIdx);
		SetColor(VertIdx,SrcVertex.Color);
	}
}

/** 
 * Allocates the vertex data storage type. 
 */
void FSkeletalMeshVertexColorBuffer::AllocateData()
{
	CleanUp();

	// only set vertex data as CPU accessible on PS3 since we can read directly from local mem (although slow)
	// otherwise treat as CPU accessible since the vertex data must persist for mesh constructioning
#if PS3
	const UBOOL bNeedsCPUAccess = ((const FSystemSettingsDataMesh&)GSystemSettings).bForceCPUAccessToGPUSkinVerts;
#else
	const UBOOL bNeedsCPUAccess = TRUE;
#endif

	VertexData = new TSkeletalMeshVertexData<FGPUSkinVertexColor>(bNeedsCPUAccess);
}

/** 
 * Copy the contents of the source color to the destination vertex in the buffer 
 *
 * @param VertexIndex - index into the vertex buffer
 * @param SrcColor - source color to copy from
 */
void FSkeletalMeshVertexColorBuffer::SetColor( UINT VertexIndex, const FColor& SrcColor )
{
	checkSlow( VertexIndex < GetNumVertices() );
	BYTE* VertBase = Data + VertexIndex * Stride;
	((FGPUSkinVertexColor*)(VertBase))->VertexColor = SrcColor;
}

/*-----------------------------------------------------------------------------
FGPUSkinVertexBase
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
*/
void FGPUSkinVertexBase::Serialize(FArchive& Ar, FVector & OutPosition)
{
	// Since previous Position was a member of FGPUSkinVertexBase
	// This need to deliver to the children (derived classes)
	if( Ar.Ver() < VER_SKELETAL_MESH_SUPPORT_PACKED_POSITION )
	{
		Ar << OutPosition;
	}

	Serialize(Ar);
}

void FGPUSkinVertexBase::Serialize(FArchive& Ar)
{
	Ar << TangentX;
	if( Ar.Ver() < VER_SKELETAL_MESH_REMOVE_BINORMAL_TANGENT_VECTOR )
	{
		FPackedNormal TempTangentY;
		Ar << TempTangentY;
		Ar << TangentZ;
		// store the sign of the determinant in TangentZ.W
		TangentZ.Vector.W = GetBasisDeterminantSignByte( TangentX, TempTangentY, TangentZ );
	}
	else
	{
		Ar << TangentZ;
	}

	// serialize bone and weight BYTE arrays in order
	// this is required when serializing as bulk data memory (see TArray::BulkSerialize notes)
	for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
	{
		Ar << InfluenceBones[InfluenceIndex];
	}
	for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
	{
		Ar << InfluenceWeights[InfluenceIndex];
	}
}

/*-----------------------------------------------------------------------------
	FSoftSkinVertex
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
* @param V - vertex to serialize
* @return archive that was used
*/
FArchive& operator<<(FArchive& Ar,FSoftSkinVertex& V)
{
	Ar << V.Position;
	Ar << V.TangentX << V.TangentY << V.TangentZ;

	if( Ar.Ver() >= VER_ADDED_MULTIPLE_UVS_TO_SKELETAL_MESH )
	{
		for( INT UVIdx = 0; UVIdx < MAX_TEXCOORDS; ++UVIdx )
		{
			Ar << V.UVs[UVIdx];
		}
	}
	else
	{
		Ar << V.UVs[0].X << V.UVs[0].Y;
	}

	// Serialize vertex color if our version is high enough, otherwise set all colors to white
	if( Ar.Ver() >= VER_ADDED_SKELETAL_MESH_VERTEX_COLORS )
	{
		Ar << V.Color;
	}
	else
	{
		V.Color = FColor(255,255,255);
	}

	// serialize bone and weight BYTE arrays in order
	// this is required when serializing as bulk data memory (see TArray::BulkSerialize notes)
	for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
	{
		Ar << V.InfluenceBones[InfluenceIndex];
	}
	for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
	{
		Ar << V.InfluenceWeights[InfluenceIndex];
	}

	return Ar;
}

/*-----------------------------------------------------------------------------
	FRigidSkinVertex
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
* @param V - vertex to serialize
* @return archive that was used
*/
FArchive& operator<<(FArchive& Ar,FRigidSkinVertex& V)
{
	Ar << V.Position;
	Ar << V.TangentX << V.TangentY << V.TangentZ;

	if( Ar.Ver() >= VER_ADDED_MULTIPLE_UVS_TO_SKELETAL_MESH )
	{
		for( INT UVIdx = 0; UVIdx < MAX_TEXCOORDS; ++UVIdx )
		{
			Ar << V.UVs[UVIdx];
		}
	}
	else
	{
		Ar << V.UVs[0].X << V.UVs[0].Y;
	}

	// Serialize vertex color if our version is high enough, otherwise set all colors to white
	if( Ar.Ver() >= VER_ADDED_SKELETAL_MESH_VERTEX_COLORS )
	{
		Ar << V.Color;
	}
	else
	{
		V.Color = FColor(255,255,255);
	}
	
	Ar << V.Bone;

	return Ar;
}

/*-----------------------------------------------------------------------------
	FSkeletalMeshVertexInfluences
-----------------------------------------------------------------------------*/

/**
 * Initialize the RHI resource for this vertex buffer
 */
void FSkeletalMeshVertexInfluences::InitRHI()
{
	if(Influences.GetResourceDataSize() > 0)
	{
		// Create the vertex buffer.
		VertexBufferRHI = RHICreateVertexBuffer(Influences.GetResourceDataSize(), &Influences, RUF_Static);
	}
}

/*-----------------------------------------------------------------------------
	FDynamicIndexBuffer
-------------------------------------------------------------------------------*/
FDynamicIndexBuffer::~FDynamicIndexBuffer()
{
	if (IndexBuffer)
	{
		delete IndexBuffer;
	}
}

void FDynamicIndexBuffer::InitResources()
{
	check(IsInGameThread());
	BeginInitResource( IndexBuffer );
}

void FDynamicIndexBuffer::ReleaseResources()
{
	check(IsInGameThread());
	if( IndexBuffer )
	{
		BeginReleaseResource( IndexBuffer );
	}
}

void FDynamicIndexBuffer::RebuildIndexBuffer( const FDynamicIndexBufferData& InData )
{
	if( IndexBuffer )
	{
		delete IndexBuffer;
	}
#if DISALLOW_32BIT_INDICES
	DataTypeSize = 2;
	NeedsCPUAccess = InData.bNeedsCPUAccess;
	IndexBuffer = new FRawStaticIndexBuffer(NeedsCPUAccess);

	if( InData.bSetUpForInstancing )
	{
		((FRawStaticIndexBuffer*)IndexBuffer)->SetupForInstancing( InData.NumVertsPerInstance );
	}

	CopyArray( InData.Indices );
#else
	DataTypeSize = InData.DataTypeSize;
	NeedsCPUAccess = InData.bNeedsCPUAccess;

	if( DataTypeSize == 2 )
	{
		IndexBuffer = new FRawStaticIndexBuffer16or32<WORD>(NeedsCPUAccess);

		if( InData.bSetUpForInstancing )
		{
			((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->SetupForInstancing( InData.NumVertsPerInstance );
		}
	}
	else
	{
		IndexBuffer = new FRawStaticIndexBuffer16or32<DWORD>(NeedsCPUAccess);
		if( InData.bSetUpForInstancing )
		{
			((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->SetupForInstancing( InData.NumVertsPerInstance );
		}
	}

	CopyArray( InData.Indices );
#endif
}

INT FDynamicIndexBuffer::Num() const
{
	if( IndexBuffer )
	{
#if DISALLOW_32BIT_INDICES
		return ((FRawStaticIndexBuffer*)IndexBuffer)->Indices.Num();
#else
		if (DataTypeSize == 2)
		{
			return ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.Num();
		}
		else
		{
			return ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.Num();
		}
#endif
	}
	else
	{
		return 0;
	}
}

INT FDynamicIndexBuffer::AddItem(DWORD Val)
{
	check( IndexBuffer );
#if DISALLOW_32BIT_INDICES
	return ((FRawStaticIndexBuffer*)IndexBuffer)->Indices.AddItem((WORD)Val);
#else

	if (DataTypeSize == 2)
	{
		return ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.AddItem((WORD)Val);
	}
	else
	{
		return ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.AddItem(Val);
	}
#endif
}

DWORD FDynamicIndexBuffer::Get(UINT Idx) const
{
	check( IndexBuffer );
#if DISALLOW_32BIT_INDICES
	return (DWORD)((FRawStaticIndexBuffer*)IndexBuffer)->Indices(Idx);
#else
	if (DataTypeSize == 2)
	{
		return (DWORD)((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices(Idx);
	}
	else
	{
		return ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices(Idx);
	}
#endif
}

void* FDynamicIndexBuffer::GetPointerTo(UINT Idx)
{
	check( IndexBuffer );
#if DISALLOW_32BIT_INDICES
	return (void*)(&((FRawStaticIndexBuffer*)IndexBuffer)->Indices(Idx));
#else
	if (DataTypeSize == 2)
	{
		return (void*)(&((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices(Idx));
	}
	else
	{
		return (void*)(&((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices(Idx));
	}
#endif
}

void FDynamicIndexBuffer::GetArray( TArray<DWORD>& OutArray ) const
{
	check( IndexBuffer );

#if DISALLOW_32BIT_INDICES
	OutArray.Reset();
	INT NumIndices = ((FRawStaticIndexBuffer*)IndexBuffer)->Indices.Num();
	OutArray.Add( NumIndices );

	for (INT I = 0; I < NumIndices; ++I)
	{
		OutArray(I) = (DWORD)((FRawStaticIndexBuffer*)IndexBuffer)->Indices(I);
	}
#else
	if (DataTypeSize == 2)
	{

		OutArray.Reset();
		INT NumIndices = ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.Num();
		OutArray.Add( NumIndices );

		for (INT I = 0; I < NumIndices; ++I)
		{
			OutArray(I) = (DWORD)((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices(I);
		}
	}
	else
	{
		OutArray = ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices;
	}
#endif
}

void FDynamicIndexBuffer::CopyArray(const TArray<DWORD>& NewArray)
{
	check( IndexBuffer );

#if !CONSOLE
	// On console the resource arrays can't be emptied directly
	Empty();
#endif

#if DISALLOW_32BIT_INDICES
	TArray<WORD> WordArray;
	for (INT i = 0; i < NewArray.Num(); ++i)
	{
		WordArray.AddItem((WORD)NewArray(i));
	}

	for (INT i = 0; i < NewArray.Num(); ++i)
	{
		((FRawStaticIndexBuffer*)IndexBuffer)->Indices = WordArray;
	}
#else
	if (DataTypeSize == 2)
	{
// On console the resource arrays can't have items added directly to them
#if CONSOLE
		TArray<WORD> WordArray;
		for (INT i = 0; i < NewArray.Num(); ++i)
		{
			WordArray.AddItem((WORD)NewArray(i));
		}

		((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices = WordArray;
#else
		for (INT i = 0; i < NewArray.Num(); ++i)
		{
			((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.AddItem((WORD)NewArray(i));
		}
#endif
	}
	else
	{
// On console the resource arrays can't have items added directly to them
#if CONSOLE
		((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices = NewArray;
#else
		for (INT i = 0; i < NewArray.Num(); ++i)
		{
			((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.AddItem(NewArray(i));
		}
#endif
	}
#endif
}

void FDynamicIndexBuffer::Insert(INT Idx, INT Num)
{
	check( IndexBuffer );
#if DISALLOW_32BIT_INDICES
	((FRawStaticIndexBuffer*)IndexBuffer)->Indices.Insert(Idx, Num);
#else
	if (DataTypeSize == 2)
	{
		((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.Insert(Idx, Num);
	}
	else
	{
		((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.Insert(Idx, Num);
	}
#endif
}

void FDynamicIndexBuffer::Remove(INT Idx, INT Num)
{
	check( IndexBuffer );
#if DISALLOW_32BIT_INDICES
	((FRawStaticIndexBuffer*)IndexBuffer)->Indices.Remove(Idx, Num);
#else
	if (DataTypeSize == 2)
	{
		((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.Remove(Idx, Num);
	}
	else
	{
		((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.Remove(Idx, Num);
	}
#endif
}

void FDynamicIndexBuffer::Empty(INT Slack)
{
	if (!IndexBuffer)
	{
		return;
	}
#if DISALLOW_32BIT_INDICES
	((FRawStaticIndexBuffer*)IndexBuffer)->Indices.Empty(Slack);
#else
	if (DataTypeSize == 2)
	{
		((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.Empty(Slack);
	}
	else
	{
		((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.Empty(Slack);
	}
#endif
}

INT FDynamicIndexBuffer::GetResourceDataSize()
{
	if (!IndexBuffer)
	{
		return 0;
	}
#if DISALLOW_32BIT_INDICES
	return ((FRawStaticIndexBuffer*)IndexBuffer)->Indices.GetResourceDataSize();
#else
	if (DataTypeSize == 2)
	{
		return ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->Indices.GetResourceDataSize();
	}
	else
	{
		return ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->Indices.GetResourceDataSize();
	}
#endif
}

FArchive& operator<<(FArchive& Ar, FDynamicIndexBuffer& Buffer)
{
	if (Ar.IsLoading() && (Ar.Ver() < VER_DWORD_SKELETAL_MESH_INDICES))
	{
		Buffer.NeedsCPUAccess = TRUE;
		Buffer.DataTypeSize = 2;
	}
	else
	{
		Ar << Buffer.NeedsCPUAccess;
		Ar << Buffer.DataTypeSize;
	}

#if DISALLOW_32BIT_INDICES
	if (!Buffer.IndexBuffer)
	{
		Buffer.IndexBuffer = new FRawStaticIndexBuffer(Buffer.NeedsCPUAccess);
		Ar << *(FRawStaticIndexBuffer*)Buffer.IndexBuffer;
	}
#else
	if (Buffer.DataTypeSize == 2)
	{
		if (!Buffer.IndexBuffer)
		{
			Buffer.IndexBuffer = new FRawStaticIndexBuffer16or32<WORD>(Buffer.NeedsCPUAccess);
		}

		((FRawStaticIndexBuffer16or32<WORD>*)Buffer.IndexBuffer)->Serialize( Ar );
	}
	else
	{
		if (!Buffer.IndexBuffer)
		{
			Buffer.IndexBuffer = new FRawStaticIndexBuffer16or32<DWORD>(Buffer.NeedsCPUAccess);
		}

		((FRawStaticIndexBuffer16or32<DWORD>*)Buffer.IndexBuffer)->Serialize( Ar );

	}
#endif
	return Ar;
}

#if WITH_EDITOR
void FDynamicIndexBuffer::GetIndexBufferData( FDynamicIndexBufferData& OutData )
{
	OutData.DataTypeSize = DataTypeSize;
	OutData.bNeedsCPUAccess = NeedsCPUAccess;
	if( DataTypeSize == 2 )
	{		
		OutData.NumVertsPerInstance = ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->GetNumVertsPerInstance();
		OutData.bSetUpForInstancing = ((FRawStaticIndexBuffer16or32<WORD>*)IndexBuffer)->GetSetupForInstancing();
	}
	else
	{
		OutData.NumVertsPerInstance = ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->GetNumVertsPerInstance();
		OutData.bSetUpForInstancing = ((FRawStaticIndexBuffer16or32<DWORD>*)IndexBuffer)->GetSetupForInstancing();
	}	

	GetArray( OutData.Indices );

}

FDynamicIndexBuffer::FDynamicIndexBuffer(const FDynamicIndexBuffer& Other)
: NeedsCPUAccess(FALSE)
, DataTypeSize(0)
, IndexBuffer(NULL)
{
	// Cant copy this index buffer.  Delete the index buffer type.
	// assumes it will be rebuilt later
	IndexBuffer = NULL;
}

FDynamicIndexBuffer& FDynamicIndexBuffer::operator=(const FDynamicIndexBuffer& Buffer)
{
	// Cant copy this index buffer.  Delete the index buffer type.
	// assumes it will be rebuilt later
	if( IndexBuffer )
	{
		delete IndexBuffer;
		IndexBuffer = NULL;
	}

	return *this;
}
#endif

/*-----------------------------------------------------------------------------
	FStaticLODModel
-----------------------------------------------------------------------------*/

/**
* Special serialize function passing the owning UObject along as required by FUnytpedBulkData
* serialization.
*
* @param	Ar		Archive to serialize with
* @param	Owner	UObject this structure is serialized within
* @param	Idx		Index of current array entry being serialized
*/
void FStaticLODModel::Serialize( FArchive& Ar, UObject* Owner, INT Idx )
{
	Ar << Sections;
	Ar << DynamicIndexBuffer;
	if (Ar.Ver() < VER_REMOVED_SHADOW_VOLUMES)
	{
		TArray<WORD> LegacyShadowIndices;
		Ar << LegacyShadowIndices;
	}
	Ar << ActiveBoneIndices;
	if (Ar.Ver() < VER_REMOVED_SHADOW_VOLUMES)
	{
		TArray<BYTE> LegacyShadowTriangleDoubleSided;
		Ar << LegacyShadowTriangleDoubleSided;
	}
	Ar << Chunks;
	Ar << Size;
	Ar << NumVertices;
	if (Ar.Ver() < VER_REMOVED_SHADOW_VOLUMES)
	{
		TArray<FMeshEdge> LegacyEdges;
		Ar << LegacyEdges;
	}
	Ar << RequiredBones;

	if( Ar.IsLoading() && Ar.Ver() < VER_DWORD_SKELETAL_MESH_INDICES )
	{
		LegacyRawPointIndices.Serialize( Ar, Owner );
		WORD* Src = (WORD*)LegacyRawPointIndices.Lock(LOCK_READ_ONLY);

		RawPointIndices.Lock(LOCK_READ_WRITE);
		INT* Dest = (INT*)RawPointIndices.Realloc( LegacyRawPointIndices.GetElementCount() );
		for( INT I = 0; I < LegacyRawPointIndices.GetElementCount(); ++I )
		{
			Dest[I] = Src[I];
		}
		LegacyRawPointIndices.Unlock();
		RawPointIndices.Unlock();
	}
	else
	{
		RawPointIndices.Serialize( Ar, Owner );
	}

	USkeletalMesh* SkelMeshOwner = CastChecked<USkeletalMesh>(Owner);

	if( Ar.IsLoading() )
	{
		// set cpu skinning flag on the vertex buffer so that the resource arrays know if they need to be CPU accessible
		VertexBufferGPUSkin.SetUseCPUSkinning(SkelMeshOwner->bForceCPUSkinning);
	}

	if( Ar.Ver() >= VER_ADDED_MULTIPLE_UVS_TO_SKELETAL_MESH )
	{
		Ar << NumTexCoords;
	}
	else
	{
		// There is only one texture coordinate set if this LOD was created before we supported multiple UV's
		NumTexCoords = 1;
	}

	Ar << VertexBufferGPUSkin;

	// Ensure backwards compatibility with old packages and only serialize color data if the mesh has vertex colors.
	if( Ar.Ver() >= VER_ADDED_SKELETAL_MESH_VERTEX_COLORS && SkelMeshOwner->bHasVertexColors)
	{
		Ar << ColorVertexBuffer;
	}

	if( Ar.Ver() >= VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCES )
	{
		Ar << VertexInfluences;
	}
}

/**
* Initialize the LOD's render resources.
*
* @param Parent Parent mesh
*/
void FStaticLODModel::InitResources(class USkeletalMesh* Parent)
{
	check(Parent);
#if DISALLOW_32BIT_INDICES
	INC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 2 );
#else
	if (DynamicIndexBuffer.DataTypeSize == 2)
	{
		INC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 2 );
	}
	else
	{
		INC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 4);
	}
#endif

	DynamicIndexBuffer.InitResources();

	if( !Parent->IsCPUSkinned() )
	{
		INC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, VertexBufferGPUSkin.GetVertexDataSize() );
        BeginInitResource(&VertexBufferGPUSkin);
	}

	for (INT VertexInfluenceIdx = 0; VertexInfluenceIdx<VertexInfluences.Num(); VertexInfluenceIdx++)
	{
		BeginInitResource(&VertexInfluences(VertexInfluenceIdx));
	}

	if( Parent->bHasVertexColors )
	{	
		// Only init the color buffer if the mesh has vertex colors
		INC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, ColorVertexBuffer.GetVertexDataSize() );
		BeginInitResource(&ColorVertexBuffer);
	}
}

/**
* Releases the LOD's render resources.
*/
void FStaticLODModel::ReleaseResources()
{
#if DISALLOW_32BIT_INDICES
	DEC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 2 );
#else
	if (DynamicIndexBuffer.DataTypeSize == 2)
	{
		DEC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 2 );
	}
	else
	{
		DEC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, DynamicIndexBuffer.Num() * 4);
	}
#endif
	DEC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, VertexBufferGPUSkin.GetVertexDataSize() );
	DEC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, ColorVertexBuffer.GetVertexDataSize() );

	DynamicIndexBuffer.ReleaseResources();

	BeginReleaseResource(&VertexBufferGPUSkin);
	BeginReleaseResource(&ColorVertexBuffer);

	for (INT VertexInfluenceIdx = 0; VertexInfluenceIdx<VertexInfluences.Num(); VertexInfluenceIdx++)
	{
		BeginReleaseResource(&VertexInfluences(VertexInfluenceIdx));
	}

}

/**
* Utility function for returning total number of faces in this LOD. 
*/
INT FStaticLODModel::GetTotalFaces()
{
	INT TotalFaces = 0;
	for(INT i=0; i<Sections.Num(); i++)
	{
		TotalFaces += Sections(i).NumTriangles;
	}

	return TotalFaces;
}

/** 
 *	Utility for finding the chunk that a particular vertex is in.
 */
void FStaticLODModel::GetChunkAndSkinType(INT InVertIndex, INT& OutChunkIndex, INT& OutVertIndex, UBOOL& bOutSoftVert) const
{
	OutChunkIndex = 0;
	OutVertIndex = 0;
	bOutSoftVert = FALSE;

	INT VertCount = 0;

	// Iterate over each chunk
	for(INT ChunkCount = 0; ChunkCount < Chunks.Num(); ChunkCount++)
	{
		const FSkelMeshChunk& Chunk = Chunks(ChunkCount);
		OutChunkIndex = ChunkCount;

		// Is it in Rigid vertex range?
		if(InVertIndex < VertCount + Chunk.GetNumRigidVertices())
		{
			OutVertIndex = InVertIndex - VertCount;
			bOutSoftVert = FALSE;
			return;
		}
		VertCount += Chunk.GetNumRigidVertices();

		// Is it in Soft vertex range?
		if(InVertIndex < VertCount + Chunk.GetNumSoftVertices())
		{
			OutVertIndex = InVertIndex - VertCount;
			bOutSoftVert = TRUE;
			return;
		}
		VertCount += Chunk.GetNumSoftVertices();
	}

	// InVertIndex should always be in some chunk!
	//check(FALSE);
	return;
}


/**
* Fill array with vertex position and tangent data from skel mesh chunks.
*
* @param Vertices Array to fill.
*/
void FStaticLODModel::GetVertices(TArray<FSoftSkinVertex>& Vertices)
{
	Vertices.Empty(NumVertices);
	Vertices.Add(NumVertices);
        
	// Initialize the vertex data
	// All chunks are combined into one (rigid first, soft next)
	FSoftSkinVertex* DestVertex = (FSoftSkinVertex*)Vertices.GetData();
	for(INT ChunkIndex = 0;ChunkIndex < Chunks.Num();ChunkIndex++)
	{
		const FSkelMeshChunk& Chunk = Chunks(ChunkIndex);
		//check(Chunk.NumRigidVertices == Chunk.RigidVertices.Num());
		//check(Chunk.NumSoftVertices == Chunk.SoftVertices.Num());
		for(INT VertexIndex = 0;VertexIndex < Chunk.RigidVertices.Num();VertexIndex++)
		{
			const FRigidSkinVertex& SourceVertex = Chunk.RigidVertices(VertexIndex);
			DestVertex->Position = SourceVertex.Position;
			DestVertex->TangentX = SourceVertex.TangentX;
			DestVertex->TangentY = SourceVertex.TangentY;
			DestVertex->TangentZ = SourceVertex.TangentZ;
			// store the sign of the determinant in TangentZ.W
			DestVertex->TangentZ.Vector.W = GetBasisDeterminantSignByte( SourceVertex.TangentX, SourceVertex.TangentY, SourceVertex.TangentZ );

			// copy all texture coordinate sets
			appMemcpy( DestVertex->UVs, SourceVertex.UVs, sizeof(FVector2D)*MAX_TEXCOORDS );

			DestVertex->Color = SourceVertex.Color;
			DestVertex->InfluenceBones[0] = SourceVertex.Bone;
			DestVertex->InfluenceWeights[0] = 255;
			for(INT InfluenceIndex = 1;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
			{
				DestVertex->InfluenceBones[InfluenceIndex] = 0;
				DestVertex->InfluenceWeights[InfluenceIndex] = 0;
			}
			DestVertex++;
		}
		appMemcpy(DestVertex,&Chunk.SoftVertices(0),Chunk.SoftVertices.Num() * sizeof(FSoftSkinVertex));
		DestVertex += Chunk.SoftVertices.Num();
	}
}

/**
* Initialize position and tangent vertex buffers from skel mesh chunks
*
* @param Mesh Parent mesh
*/
void FStaticLODModel::BuildVertexBuffers(class USkeletalMesh* Mesh)
{
	check(Mesh);

	if( Mesh->GetOutermost()->PackageFlags & PKG_Cooked )
	{
		// Can't build vertex buffers from cooked data because 
		// chunk rigid/soft vertices are stripped by the cooker.
		return;
	}

	TArray<FSoftSkinVertex> Vertices;
	GetVertices(Vertices);

	// match UV precision for mesh vertex buffer to setting from parent mesh
	VertexBufferGPUSkin.SetUseFullPrecisionUVs(Mesh->bUseFullPrecisionUVs);
	// set CPU skinning on vertex buffer since it affects the type of TResourceArray needed
	VertexBufferGPUSkin.SetUseCPUSkinning(Mesh->IsCPUSkinned());
	// match packed position for mesh vertex buffer to setting from parent mesh if CPU skin is off
	VertexBufferGPUSkin.SetUsePackedPosition(!Mesh->IsCPUSkinned() && Mesh->bUsePackedPosition);
	// Set the number of texture coordinate sets
	VertexBufferGPUSkin.SetNumTexCoords( NumTexCoords );

	// init vertex buffer with the vertex array
	VertexBufferGPUSkin.Init(Vertices);

	// Init the color buffer if this mesh has vertex colors.
	if( Mesh->bHasVertexColors )
	{
		ColorVertexBuffer.Init(Vertices);
	}
}

/**
* Sort the triangles in the LODmodel
*
* @param ETriangleSortOption NewTriangleSorting new sorting method
*/
void FStaticLODModel::SortTriangles( USkeletalMesh* SkelMesh, INT SectionIndex, ETriangleSortOption NewTriangleSorting )
{
#if WITH_EDITOR
	FSkelMeshSection& Section = Sections(SectionIndex);
	if( NewTriangleSorting == Section.TriangleSorting )
	{
		return;
	}

	if( NewTriangleSorting == TRISORT_CustomLeftRight )
	{
		// Make a second copy of index buffer data for this section
		INT NumNewIndices = Section.NumTriangles*3;
		DynamicIndexBuffer.Insert(Section.BaseIndex, NumNewIndices);
		appMemcpy( DynamicIndexBuffer.GetPointerTo(Section.BaseIndex), DynamicIndexBuffer.GetPointerTo(Section.BaseIndex+NumNewIndices), NumNewIndices*DynamicIndexBuffer.DataTypeSize );

		// Fix up BaseIndex for indices in other sections
		for( INT OtherSectionIdx = 0; OtherSectionIdx < Sections.Num(); OtherSectionIdx++ )
		{
			if( Sections(OtherSectionIdx).BaseIndex > Section.BaseIndex )
			{
				Sections(OtherSectionIdx).BaseIndex += NumNewIndices;
			}
		}
	}
	else
	if( Section.TriangleSorting == TRISORT_CustomLeftRight )
	{
		// Remove the second copy of index buffer data for this section
		INT NumRemovedIndices = Section.NumTriangles*3;
		DynamicIndexBuffer.Remove(Section.BaseIndex, NumRemovedIndices);
		// Fix up BaseIndex for indices in other sections
		for( INT OtherSectionIdx = 0; OtherSectionIdx < Sections.Num(); OtherSectionIdx++ )
		{
			if( Sections(OtherSectionIdx).BaseIndex > Section.BaseIndex )
			{
				Sections(OtherSectionIdx).BaseIndex -= NumRemovedIndices;
			}
		}
	}

	TArray<FSoftSkinVertex> Vertices;
	GetVertices(Vertices);

	switch( NewTriangleSorting )
	{
	case TRISORT_None:
		{
			TArray<DWORD> Indices;
			DynamicIndexBuffer.GetArray( Indices );
			SortTriangles_None( SkelMesh, Section.NumTriangles, &Vertices(0), Indices.GetData() + Section.BaseIndex );
			DynamicIndexBuffer.CopyArray( Indices );
		}
		break;
	case TRISORT_CenterRadialDistance:
		{
			TArray<DWORD> Indices;
			DynamicIndexBuffer.GetArray( Indices );
			SortTriangles_CenterRadialDistance( SkelMesh, Section.NumTriangles, &Vertices(0), Indices.GetData() + Section.BaseIndex );
			DynamicIndexBuffer.CopyArray( Indices );
		}
		break;
	case TRISORT_Random:
		{
			TArray<DWORD> Indices;
			DynamicIndexBuffer.GetArray( Indices );
			SortTriangles_Random( Section.NumTriangles, &Vertices(0), Indices.GetData() + Section.BaseIndex );
			DynamicIndexBuffer.CopyArray( Indices );
		}
		break;
	case TRISORT_Tootle:
		{
			TArray<DWORD> Indices;
			DynamicIndexBuffer.GetArray( Indices );
			SortTriangles_Tootle( Section.NumTriangles, NumVertices, &Vertices(0), Indices.GetData() + Section.BaseIndex );
			DynamicIndexBuffer.CopyArray( Indices );
		}
		break;
	case TRISORT_MergeContiguous:
		{
			TArray<DWORD> Indices;
			DynamicIndexBuffer.GetArray( Indices );
			SortTriangles_MergeContiguous( Section.NumTriangles, NumVertices, &Vertices(0), Indices.GetData() + Section.BaseIndex );
			DynamicIndexBuffer.CopyArray( Indices );
		}
		break;
	}

	Section.TriangleSorting = NewTriangleSorting;
#endif
}

/*-----------------------------------------------------------------------------
USkeletalMesh
-----------------------------------------------------------------------------*/

/**
* Calculate max # of bone influences used by this skel mesh chunk
*/
void FSkelMeshChunk::CalcMaxBoneInfluences()
{
	// if we only have rigid verts then there is only one bone
	MaxBoneInfluences = 1;
	// iterate over all the soft vertices for this chunk and find max # of bones used
	for( INT VertIdx=0; VertIdx < SoftVertices.Num(); VertIdx++ )
	{
		FSoftSkinVertex& SoftVert = SoftVertices(VertIdx);

		// calc # of bones used by this soft skinned vertex
		INT BonesUsed=0;
		for( INT InfluenceIdx=0; InfluenceIdx < MAX_INFLUENCES; InfluenceIdx++ )
		{
			if( SoftVert.InfluenceWeights[InfluenceIdx] > 0 )
			{
				BonesUsed++;
			}
		}
		// reorder bones so that there aren't any unused influence entries within the [0,BonesUsed] range
		for( INT InfluenceIdx=0; InfluenceIdx < BonesUsed; InfluenceIdx++ )
		{
			if( SoftVert.InfluenceWeights[InfluenceIdx] == 0 )
			{
                for( INT ExchangeIdx=InfluenceIdx+1; ExchangeIdx < MAX_INFLUENCES; ExchangeIdx++ )
				{
                    if( SoftVert.InfluenceWeights[ExchangeIdx] != 0 )
					{
						Exchange(SoftVert.InfluenceWeights[InfluenceIdx],SoftVert.InfluenceWeights[ExchangeIdx]);
						Exchange(SoftVert.InfluenceBones[InfluenceIdx],SoftVert.InfluenceBones[ExchangeIdx]);
						break;
					}
				}
			}
		}

		// maintain max bones used
		MaxBoneInfluences = Max(MaxBoneInfluences,BonesUsed);			
	}
}

/*-----------------------------------------------------------------------------
	USkeletalMesh
-----------------------------------------------------------------------------*/

/**
* Initialize the mesh's render resources.
*/
void USkeletalMesh::InitResources()
{
	// initialize resources for each lod
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).InitResources(this);
	}
}

/**
* Releases the mesh's render resources.
*/
void USkeletalMesh::ReleaseResources()
{
	// release resources for each lod
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).ReleaseResources();
	}

	// insert a fence to signal when these commands completed
	ReleaseResourcesFence.BeginFence();
}

IMPLEMENT_COMPARE_CONSTREF( FLOAT, UnSkeletalMesh, { return (B - A) > 0 ? 1 : -1 ; } )

/**
 * Returns the scale dependent texture factor used by the texture streaming code.	
 *
 * @param RequestedUVIndex UVIndex to look at
 * @return scale dependent texture factor
 */
FLOAT USkeletalMesh::GetStreamingTextureFactor( INT RequestedUVIndex )
{
	check(RequestedUVIndex >= 0);
	check(RequestedUVIndex < MAX_TEXCOORDS);

	// If the streaming texture factor cache doesn't have the right number of entries, it needs to be updated.
	if(CachedStreamingTextureFactors.Num() != MAX_TEXCOORDS)
	{
		if(!CONSOLE)
		{
			// Reset the cached texture factors.
			CachedStreamingTextureFactors.Empty(MAX_TEXCOORDS);
			CachedStreamingTextureFactors.AddZeroed(MAX_TEXCOORDS);

			FStaticLODModel& LODModel = LODModels(0);
			INT NumTotalTriangles = LODModel.GetTotalFaces();

			TArray<FLOAT> TexelRatios[MAX_TEXCOORDS];
			FLOAT MaxTexelRatio = 0.0f;
			for(INT UVIndex = 0;UVIndex < MAX_TEXCOORDS;UVIndex++)
			{
				TexelRatios[UVIndex].Empty( NumTotalTriangles );
			}

			for( INT SectionIndex = 0; SectionIndex < LODModel.Sections.Num();SectionIndex++ )
			{
				FSkelMeshSection& Section = LODModel.Sections(SectionIndex);
				TArray<DWORD> Indices;
				LODModel.DynamicIndexBuffer.GetArray( Indices );

				const DWORD* SrcIndices = Indices.GetData() + Section.BaseIndex;
				DWORD NumTriangles = Section.NumTriangles;

				// Figure out Unreal unit per texel ratios.
				for (DWORD TriangleIndex=0; TriangleIndex < NumTriangles; TriangleIndex++ )
				{
					//retrieve indices
					DWORD Index0 = SrcIndices[TriangleIndex*3];
					DWORD Index1 = SrcIndices[TriangleIndex*3+1];
					DWORD Index2 = SrcIndices[TriangleIndex*3+2];

					const FVector Pos0 = LODModel.VertexBufferGPUSkin.GetVertexPosition(Index0);
					const FVector Pos1 = LODModel.VertexBufferGPUSkin.GetVertexPosition(Index1);
					const FVector Pos2 = LODModel.VertexBufferGPUSkin.GetVertexPosition(Index2);
					FLOAT L1 = (Pos0 - Pos1).Size();
					FLOAT L2 = (Pos0 - Pos2).Size();

					INT NumUVs = LODModel.NumTexCoords;
					for(INT UVIndex = 0;UVIndex < Min(NumUVs,(INT)MAX_TEXCOORDS);UVIndex++)
					{
						const FVector2D UV0 = LODModel.VertexBufferGPUSkin.GetVertexUV(Index0, UVIndex);
						const FVector2D UV1 = LODModel.VertexBufferGPUSkin.GetVertexUV(Index1, UVIndex);
						const FVector2D UV2 = LODModel.VertexBufferGPUSkin.GetVertexUV(Index2, UVIndex);

						FLOAT T1 = (UV0 - UV1).Size();
						FLOAT T2 = (UV0 - UV2).Size();

						if( Abs(T1 * T2) > Square(SMALL_NUMBER) )
						{
							const FLOAT TexelRatio = Max( L1 / T1, L2 / T2 );
							TexelRatios[UVIndex].AddItem( TexelRatio );

							// Update max texel ratio
							if( TexelRatio > MaxTexelRatio )
							{
								MaxTexelRatio = TexelRatio;
							}
						}
					}
				}

				for(INT UVIndex = 0;UVIndex < MAX_TEXCOORDS;UVIndex++)
				{
					if( TexelRatios[UVIndex].Num() )
					{
						// Disregard upper 75% of texel ratios.
						// This is to ignore backfacing surfaces or other non-visible surfaces that tend to map a small number of texels to a large surface.
						Sort<USE_COMPARE_CONSTREF(FLOAT,UnSkeletalMesh)>( &(TexelRatios[UVIndex](0)), TexelRatios[UVIndex].Num() );
						FLOAT TexelRatio = TexelRatios[UVIndex]( appTrunc(TexelRatios[UVIndex].Num() * 0.75f) );
						if ( UVIndex == 0 )
						{
							TexelRatio *= StreamingDistanceMultiplier;
						}
						CachedStreamingTextureFactors(UVIndex) = TexelRatio;
					}
				}
			}
		}
		else
		{
			// Streaming texture factors cannot be computed on consoles, since the raw data has been cooked out.
			debugfSuppressed( TEXT("USkeletalMesh::GetStreamingTextureFactor is being called on the console which is slow.  You need to resave the map to have the editor precalculate the StreamingTextureFactor for:  %s  Please resave your map. If you are calling this directly then we just return 0.0f instead of appErrorfing"), *GetFullName() );
			return 0.0f;
		}		
	}

	return CachedStreamingTextureFactors(RequestedUVIndex);
}

/**
 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
 *
 * @return size of resource as to be displayed to artists/ LDs in the Editor.
 */
INT USkeletalMesh::GetResourceSize()
{
	if (GExclusiveResourceSizeMode)
	{
		return 0;
	}
	else
	{
		FArchiveCountMem CountBytesSize( this );
		INT ResourceSize = CountBytesSize.GetNum();
		return ResourceSize;
	}
}

/**
 * Operator for MemCount only
 */
FArchive &operator<<( FArchive& Ar, FTriangleSortSettings& S )
{
	Ar << S.TriangleSorting;
	Ar << S.CustomLeftRightAxis;
	Ar << S.CustomLeftRightBoneName;
	return Ar;
}

/**
 * Operator for MemCount only, so it only serializes the arrays that needs to be counted.
 */
FArchive &operator<<( FArchive& Ar, FSkeletalMeshLODInfo& I )
{
	Ar << I.LODMaterialMap;
	Ar << I.bEnableShadowCasting;
	Ar << I.TriangleSortSettings;
	return Ar;
}


/** 
* Some property has changed so cleanup resources
*
* @param	PropertyAboutToChange - property that is changing
*/
void USkeletalMesh::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	// Release the mesh's render resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the edit change doesn't occur while a resource is still
	// allocated, and potentially accessing the mesh data.
	ReleaseResourcesFence.Wait();
}

/** 
* Some property has changed so recreate resources
*
* @param	PropertyAboutToChange - property that changed
*/
void USkeletalMesh::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UBOOL bFullPrecisionUVsReallyChanged = FALSE;

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	// reassure if UsePackedPosition == FALSE when ForceCPUSkinning == TRUE
	// if so, turn PackedPosition off to ensure users see same result as how it works internally
	if ( GIsEditor &&
		PropertyThatChanged && bForceCPUSkinning && bUsePackedPosition ) 
	{
		// disable Packed Position 
		bUsePackedPosition = FALSE;
	}

	if( GIsEditor &&
		PropertyThatChanged &&
		PropertyThatChanged->GetFName() == FName(TEXT("bUseFullPrecisionUVs")) )
	{
		bFullPrecisionUVsReallyChanged = TRUE;
		if (!bUseFullPrecisionUVs && !GVertexElementTypeSupport.IsSupported(VET_Half2) )
		{
			bUseFullPrecisionUVs = TRUE;
			warnf(TEXT("16 bit UVs not supported. Reverting to 32 bit UVs"));			
			bFullPrecisionUVsReallyChanged = FALSE;
		}
	}

	// Apply any triangle sorting changes
	if( PropertyThatChanged && PropertyThatChanged->GetFName() == FName(TEXT("TriangleSorting")) )
	{
		for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
		{
			for( INT SectionIndex = 0; SectionIndex < LODModels(LODIndex).Sections.Num();SectionIndex++ )
			{
				LODModels(LODIndex).SortTriangles( this, SectionIndex, (ETriangleSortOption)LODInfo(LODIndex).TriangleSortSettings(SectionIndex).TriangleSorting );
			}
		}
	}

	// rebuild vertex buffers
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).BuildVertexBuffers(this);
	}

	// Reinitialize the mesh's render resources.
	InitResources();

	// Rebuild any per-poly collision data we want.
	UpdatePerPolyKDOPs();

	// invalidate the components that use the mesh 
	// when switching between cpu/gpu skinning or 16-bit UVs
 	if( ( GIsEditor && PropertyThatChanged && PropertyThatChanged->GetFName() == FName(TEXT("bForceCPUSkinning")) ) || 
		bFullPrecisionUVsReallyChanged )
	{
		TArray<AActor*> ActorsToUpdate;
		for( TObjectIterator<USkeletalMeshComponent> It; It; ++It )
		{
			USkeletalMeshComponent* MeshComponent = *It;
			if( MeshComponent && 
				!MeshComponent->IsTemplate() &&
				MeshComponent->SkeletalMesh == this )
			{
				FComponentReattachContext ReattachContext(MeshComponent);
			}
		}
	}	

	if ( GIsEditor && PropertyThatChanged && PropertyThatChanged->GetName() == TEXT("StreamingDistanceMultiplier") )
	{
		// Allow recalculating the texture factor.
		CachedStreamingTextureFactors.Empty();
		// Recalculate in a few seconds.
		ULevel::TriggerStreamingDataRebuild();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

/** 
* This object wants to be destroyed so cleanup the rendering resources 
*/
void USkeletalMesh::BeginDestroy()
{
	Super::BeginDestroy();

	// Release the mesh's render resources.
	ReleaseResources();
}

/**
* Check for asynchronous resource cleanup completion
*
* @return	TRUE if the rendering resources have been released
*/
UBOOL USkeletalMesh::IsReadyForFinishDestroy()
{
	// see if we have hit the resource flush fence
	return ReleaseResourcesFence.GetNumPendingFences() == 0;
}

/** 
* Serialize 
*/
void USkeletalMesh::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << Bounds;
 	Ar << Materials;
	Ar << Origin << RotOrigin;
	Ar << RefSkeleton;			// Reference skeleton.
	Ar << SkeletalDepth;		// How many bones beep the heirarchy goes.
	LODModels.Serialize( Ar, this );
	// make sure we're counting properly
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		// adding all native, but non serialized variables
		// I didn't see this gets caught here, but if gets called, I need to properly changed to right format.
		if (Ar.Ver() < VER_FBONEATOM_QUATERNION_TRANSFORM_SUPPORT)
		{
			check(0);
			TArray<FMatrix> OldRefBasesInvMatrix;
			Ar << OldRefBasesInvMatrix;
			RefBasesInvMatrix.Empty();
			RefBasesInvMatrix.Add(OldRefBasesInvMatrix.Num());
			for (INT I=0; I<OldRefBasesInvMatrix.Num(); ++I)
			{
				RefBasesInvMatrix(I) = FBoneAtom(OldRefBasesInvMatrix(I));
			}
		}
		else
		{
			Ar << RefBasesInvMatrix;
		}
		ClothMesh.CountBytes(Ar);
		ClothMeshScale.CountBytes(Ar);
		Ar << ClothTornTriMap;
		CachedSoftBodyMeshes.CountBytes(Ar);
		CachedSoftBodyMeshScales.CountBytes(Ar);
	}

	Ar << NameIndexMap;
	Ar << PerPolyBoneKDOPs;

	if (Ar.Ver() >= VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCE_MAPPING)
	{
		Ar << BoneBreakNames;
		if (Ar.Ver() >= VER_ADDED_EXTRA_SKELMESH_VERTEX_INFLUENCE_CUSTOM_MAPPING)
		{
			Ar << BoneBreakOptions;
		}
		else if ( Ar.IsLoading() )
		{
			BoneBreakOptions.Empty(BoneBreakNames.Num());
			// add empty ones - this is by default BONEBREAK_SoftPreferred
			BoneBreakOptions.AddZeroed(BoneBreakNames.Num());
		}
	}
	else if (Ar.IsLoading())
	{
		BoneBreakNames.Empty();
		BoneBreakOptions.Empty();
	}
	if ( Ar.Ver() >= VER_APEX_CLOTHING )
	{
		Ar << ClothingAssets;
	}
	else
	{
		ClothingAssets.Empty();
		if ( Materials.Num() > 0 )
		{
			ClothingAssets.Add( Materials.Num());
			for (INT i=0; i<Materials.Num(); i++)
			{
				ClothingAssets(i) = NULL;
			}
		}
	}
	if ( Ar.Ver() >= VER_DYNAMICTEXTUREINSTANCES )
	{
		Ar << CachedStreamingTextureFactors;
	}
#if !CONSOLE
	// Strip away loaded Editor-only data if we're a client and never care about saving.
	if( Ar.IsLoading() && GIsClient && !GIsEditor && !GIsUCC )
	{
		// Console platform is not a mistake, this ensures that as much as possible will be tossed.
		StripData( UE3::PLATFORM_Console, FALSE );
	}
#endif
}

/** 
* Postload 
*/
void USkeletalMesh::PostLoad()
{
	Super::PostLoad();

	// If LODInfo is missing - create array of correct size.
	if( LODInfo.Num() != LODModels.Num() )
	{
		LODInfo.Empty(LODModels.Num());
		LODInfo.AddZeroed(LODModels.Num());

		for(INT i=0; i<LODInfo.Num(); i++)
		{
			LODInfo(i).LODHysteresis = 0.02f;

			// Presize the per-section shadow array.
			LODInfo(i).bEnableShadowCasting.Empty( LODModels(i).Sections.Num() );
			for ( INT SectionIndex = 0 ; SectionIndex < LODModels(i).Sections.Num() ; ++SectionIndex )
			{
				LODInfo(i).bEnableShadowCasting.AddItem( TRUE );
			}
		}
	}

	for(INT LodIndex=0; LodIndex<LODInfo.Num(); LodIndex++)
	{
		FSkeletalMeshLODInfo& ThisLODInfo = LODInfo(LodIndex);
		FStaticLODModel& ThisLODModel = LODModels(LodIndex);

		// Presize the per-section TriangleSortSettings array
		if( ThisLODInfo.TriangleSortSettings.Num() > ThisLODModel.Sections.Num() )
		{
			ThisLODInfo.TriangleSortSettings.Remove( ThisLODModel.Sections.Num(), ThisLODInfo.TriangleSortSettings.Num()-ThisLODModel.Sections.Num() );
		}
		else
		if( ThisLODModel.Sections.Num() > ThisLODInfo.TriangleSortSettings.Num() )
		{
			ThisLODInfo.TriangleSortSettings.AddZeroed( ThisLODModel.Sections.Num()-ThisLODInfo.TriangleSortSettings.Num() );
		}
	}

	if( GetLinker() && (GetLinker()->Ver() < VER_ADDED_SKELETAL_MESH_SORTING_LEFTRIGHT_BONE) )
	{
		for(INT LodIndex=0; LodIndex<LODInfo.Num(); LodIndex++)
		{
			FSkeletalMeshLODInfo& ThisLODInfo = LODInfo(LodIndex);
			FStaticLODModel& ThisLODModel = LODModels(LodIndex);

			// Fix up any mismatch in the TriangleSorting between LODInfo and Section
			// There was a bug in the mesh reimport code that failed to update the LODInfo or 
			// initialize it
			for ( INT SectionIndex = 0 ; SectionIndex < ThisLODModel.Sections.Num() ; ++SectionIndex )
			{
				if( SectionIndex < ThisLODInfo.OLD_TriangleSorting.Num() && 
					ThisLODInfo.OLD_TriangleSorting(SectionIndex) != ThisLODModel.Sections(SectionIndex).TriangleSorting )
				{
					debugf(NAME_Warning, TEXT("Fixing up incorrect TriangleSorting setting for %s LOD %d section %d (%d vs %d)"), *GetPathName(), LodIndex, SectionIndex, ThisLODInfo.OLD_TriangleSorting(SectionIndex), ThisLODModel.Sections(SectionIndex).TriangleSorting);
					ThisLODModel.Sections(SectionIndex).TriangleSorting = TRISORT_None;
					ThisLODInfo.OLD_TriangleSorting(SectionIndex) = TRISORT_None;
				}
			}

			// Copy the data from the OLD_TriangleSorting array.
			for ( INT SectionIndex = 0 ; SectionIndex < ThisLODModel.Sections.Num() ; ++SectionIndex )
			{
				if( SectionIndex < ThisLODInfo.OLD_TriangleSorting.Num() )
				{
					ThisLODInfo.TriangleSortSettings(SectionIndex).TriangleSorting = ThisLODInfo.OLD_TriangleSorting(SectionIndex);
				}
			}
		}
	}

	// Revert to using 32 bit Float UVs on hardware that doesn't support rendering with 16 bit Float UVs 
	if( !GIsCooking && !bUseFullPrecisionUVs && !GVertexElementTypeSupport.IsSupported(VET_Half2) )
	{
		bUseFullPrecisionUVs=TRUE;
		// convert each LOD level to 32 bit UVs
		for( INT LODIdx=0; LODIdx < LODModels.Num(); LODIdx++ )
		{
			FStaticLODModel& LODModel = LODModels(LODIdx);
			// Determine the correct version of ConvertToFullPrecisionUVs based on the number of UVs in the vertex buffer
			const UINT NumTexCoords = LODModel.VertexBufferGPUSkin.GetNumTexCoords();
			switch(NumTexCoords)
			{
			case 1: LODModel.VertexBufferGPUSkin.ConvertToFullPrecisionUVs<1>(); break;
			case 2: LODModel.VertexBufferGPUSkin.ConvertToFullPrecisionUVs<2>(); break; 
			case 3: LODModel.VertexBufferGPUSkin.ConvertToFullPrecisionUVs<3>(); break; 
			case 4: LODModel.VertexBufferGPUSkin.ConvertToFullPrecisionUVs<4>(); break; 
			}
		}
	}

	// initialize rendering resources
	if (!GIsUCC)
	{
		InitResources();
	}

	CalculateInvRefMatrices();

	// If the name->index is empty - build now (for old content)
	if(NameIndexMap.Num() == 0)
	{
		InitNameIndexMap();
	}

	// Create run time UID	
	SkelMeshRUID = appCreateRuntimeUID();
}

/**
* Verify SkeletalMeshLOD is set up correctly	
*/
void USkeletalMesh::DebugVerifySkeletalMeshLOD()
{
	// if LOD do not have displayfactor set up correctly
	if (LODInfo.Num() > 1)
	{
		for(INT i=1; i<LODInfo.Num(); i++)
		{
			if (LODInfo(i).DisplayFactor <= 0.1f)
			{
				// too small
				debugf(NAME_Warning, TEXT("SkelMeshLOD (%s) : DisplayFactor for LOD %d may be too small (%0.5f)"), *GetPathName(), i, LODInfo(i).DisplayFactor);
			}
		}
	}
	else
	{
		// no LODInfo
		debugf(NAME_Warning, TEXT("SkelMeshLOD (%s) : LOD does not exist"), *GetPathName());
	}
}
void USkeletalMesh::PreSave()
{
	Super::PreSave();

	// Make sure collision data is up to date. Don't execute on default objects!
	// We also don't do this during cooking, because the chunk verts we need get stripped out.
	if(!IsTemplate() && !GIsCooking)
	{
		UpdatePerPolyKDOPs();
	}
}

void USkeletalMesh::FinishDestroy()
{
	ClearClothMeshCache();
#if WITH_NOVODEX && !NX_DISABLE_SOFTBODY
	ClearSoftBodyMeshCache();
#endif	//#if WITH_NOVODEX
	Super::FinishDestroy();
}

/** Called when object is duplicated - avoid duplicate GUIDs  */
void USkeletalMesh::PostDuplicate()
{
	Super::PostDuplicate();
	SkelMeshRUID = appCreateRuntimeUID();
}

/**
 * Used by various commandlets to purge editor only and platform-specific data from various objects
 * 
 * @param PlatformsToKeep Platforms for which to keep platform-specific data
 * @param bStripLargeEditorData If TRUE, data used in the editor, but large enough to bloat download sizes, will be removed
 */
void USkeletalMesh::StripData(UE3::EPlatformType PlatformsToKeep, UBOOL bStripLargeEditorData)
{
	Super::StripData(PlatformsToKeep, bStripLargeEditorData); 

	// if we aren't keeping any non-stripped platforms, we can remove the data
	if (!(PlatformsToKeep & ~UE3::PLATFORM_Stripped) || GIsCookingForDemo)
	{
		for( INT LODIdx=0; LODIdx < LODModels.Num(); LODIdx++ )
		{
			FStaticLODModel& LODModel = LODModels(LODIdx);

			LODModel.RawPointIndices.RemoveBulkData();
			LODModel.LegacyRawPointIndices.RemoveBulkData();

			for( INT ChunkIdx=0; ChunkIdx < LODModel.Chunks.Num(); ChunkIdx++ )
			{
				FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIdx);
				// stored in vertex buffer
				Chunk.RigidVertices.Empty();
				Chunk.SoftVertices.Empty();
			}
		}
	}

	//Dedicated servers don't need vertex/index buffer data, so
	// toss if the only platform to keep is WindowsServer
	if (!(PlatformsToKeep & ~UE3::PLATFORM_WindowsServer))
	{
		for( INT LODIdx=0; LODIdx < LODModels.Num(); LODIdx++ )
		{
			FStaticLODModel& LODModel = LODModels(LODIdx);
								  
			//Strip Index/Vertex buffers
			LODModel.DynamicIndexBuffer.Empty();
			LODModel.VertexBufferGPUSkin.CleanUp();
			LODModel.ColorVertexBuffer.CleanUp();
			
			//Gore mesh vertex influence data
			LODModel.VertexInfluences.Empty();

			//Zero out some counts for safety
			LODModel.NumVertices = 0;
			for (INT SectionIdx = 0; SectionIdx < LODModel.Sections.Num(); SectionIdx++)
			{
				FSkelMeshSection& Section = LODModel.Sections(SectionIdx);
				Section.NumTriangles = 0;
				Section.BaseIndex = 0;
			}

			for (INT ChunkIdx = 0; ChunkIdx < LODModel.Chunks.Num(); ChunkIdx++)
			{
			   FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIdx);
			   Chunk.BaseVertexIndex = 0;
			}
		}
	}
}

/** Clear and create the NameIndexMap of bone name to bone index. */
void USkeletalMesh::InitNameIndexMap()
{
	// Start by clearing the current map.
	NameIndexMap.Empty();

	// Then iterate over each bone, adding the name and bone index.
	for(INT i=0; i<RefSkeleton.Num(); i++)
	{
		FName BoneName = RefSkeleton(i).Name;
		if(BoneName != NAME_None)
		{
			NameIndexMap.Set(BoneName, i);
		}
	}
}

UBOOL USkeletalMesh::IsCPUSkinned() const
{
	// Only use GPU skinning if all the chunks in the mesh use fewer than MAX_GPUSKIN_BONES simultaneous bones.
	UBOOL bUseCPUSkinning = bForceCPUSkinning;

	if (!bUseCPUSkinning && LODModels.Num())
	{
		for (INT ChunkIndex = 0; ChunkIndex < LODModels(0).Chunks.Num(); ChunkIndex++)
		{
			if (LODModels(0).Chunks(ChunkIndex).BoneMap.Num() > MAX_GPUSKIN_BONES)
			{
				debugf(TEXT("'%s' has too many bones (%d) for GPU skinning"), *GetFullName(), LODModels(0).Chunks(ChunkIndex).BoneMap.Num());
				bUseCPUSkinning = TRUE;
				break;
			}
		}
	}

	return bUseCPUSkinning;
}

/**
 * Match up startbone by name. Also allows the attachment tag aliases.
 * Pretty much the same as USkeletalMeshComponent::MatchRefBone
 */
INT USkeletalMesh::MatchRefBone(FName StartBoneName) const
{
	INT BoneIndex = INDEX_NONE;
	if( StartBoneName != NAME_None )
	{
		const INT* IndexPtr = NameIndexMap.Find(StartBoneName);
		if(IndexPtr)
		{
			BoneIndex = *IndexPtr;
		}
	}
	return BoneIndex;
}

/** 
 *	Find a socket object in this SkeletalMesh by name. 
 *	Entering NAME_None will return NULL. If there are multiple sockets with the same name, will return the first one.
 */
USkeletalMeshSocket* USkeletalMesh::FindSocket(FName InSocketName)
{
	if(InSocketName == NAME_None)
	{
		return NULL;
	}

	for(INT i=0; i<Sockets.Num(); i++)
	{
		USkeletalMeshSocket* Socket = Sockets(i);
		if(Socket && Socket->SocketName == InSocketName)
		{
			return Socket;
		}
	}

	return NULL;
}

/** 
* GetRefPoseMatrix 
*/
FMatrix USkeletalMesh::GetRefPoseMatrix( INT BoneIndex ) const
{
	check( BoneIndex >= 0 && BoneIndex < RefSkeleton.Num() );
	return FQuatRotationTranslationMatrix( RefSkeleton(BoneIndex).BonePos.Orientation, RefSkeleton(BoneIndex).BonePos.Position );
}

/*-----------------------------------------------------------------------------
USkeletalMeshSocket
-----------------------------------------------------------------------------*/

/** 
* Utility that returns the current matrix for this socket. Returns false if socket was not valid (bone not found etc) 
*/
UBOOL USkeletalMeshSocket::GetSocketMatrix(FMatrix& OutMatrix, class USkeletalMeshComponent* SkelComp) const
{
	INT BoneIndex = SkelComp->MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FRotationTranslationMatrix RelSocketMatrix( RelativeRotation, RelativeLocation );
		OutMatrix = RelSocketMatrix * BoneMatrix;
		return TRUE;
	}

	return FALSE;
}

/** 
*	Utility that returns the current matrix for this socket with an offset. Returns false if socket was not valid (bone not found etc) 
 *	
 *	@param	OutMatrix		The resulting socket matrix
 *	@param	SkelComp		The skeletal mesh component that the socket comes from
 *	@param	InOffset		The additional offset to apply to the socket location
 *	@param	InRotation		The additional rotation to apply to the socket rotation
 *
 *	@return	UBOOL			TRUE if successful, FALSE if not
 */
UBOOL USkeletalMeshSocket::GetSocketMatrixWithOffset(FMatrix& OutMatrix, class USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const
{
	INT BoneIndex = SkelComp->MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FRotationTranslationMatrix RelSocketMatrix(RelativeRotation, RelativeLocation);
		FRotationTranslationMatrix RelOffsetMatrix(InRotation, InOffset);
		OutMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
		return TRUE;
	}

	return FALSE;
}

/** 
 *	Utility that returns the current position of this socket with an offset. Returns false if socket was not valid (bone not found etc)
 *	
 *	@param	OutPosition		The resulting position
 *	@param	SkelComp		The skeletal mesh component that the socket comes from
 *	@param	InOffset		The additional offset to apply to the socket location
 *	@param	InRotation		The additional rotation to apply to the socket rotation
 *
 *	@return	UBOOL			TRUE if successful, FALSE if not
 */
UBOOL USkeletalMeshSocket::GetSocketPositionWithOffset(FVector& OutPosition, class USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const
{
	INT BoneIndex = SkelComp->MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FRotationTranslationMatrix RelSocketMatrix(RelativeRotation, RelativeLocation);
		FRotationTranslationMatrix RelOffsetMatrix(InRotation, InOffset);
		FMatrix SocketMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
		OutPosition = SocketMatrix.GetOrigin();
		return TRUE;
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------
	ASkeletalMeshActor
-----------------------------------------------------------------------------*/

//@compatibility
void ASkeletalMeshActor::PostLoad()
{
	if (GetLinker() && GetLinker()->Ver() < VER_REMOVED_DEFAULT_SKELETALMESHACTOR_COLLISION)
	{
		bCollideActors = bCollideActors_OldValue_DEPRECATED;
	}

	Super::PostLoad();
}

/** Updates controls based on ControlTargets array. */
void ASkeletalMeshActor::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);

	// Iterate over ControlTargets array
	for(INT i=0; i<ControlTargets.Num(); i++)
	{
		// Check we have valid name and actor
		if(ControlTargets(i).ControlName != NAME_None && ControlTargets(i).TargetActor)
		{
			// Find the control
			USkelControlBase* Control = SkeletalMeshComponent->FindSkelControl(ControlTargets(i).ControlName);
			if(Control)
			{
				// If found, update location
				Control->SetControlTargetLocation(ControlTargets(i).TargetActor->Location);
			}
		}
	}
}

/** Override ForceUpdateComponents so we don't update the SkeletalMeshComponent (which is the CollisionComponent) every MoveActor call.  */
void ASkeletalMeshActor::ForceUpdateComponents(UBOOL bCollisionUpdate,UBOOL bTransformOnly)
{
	MarkComponentsAsDirty(bTransformOnly);

	// bCollisionUpdate is TRUE when this is called from MoveActor - normally we just want to update the 'important' CollisionComponent
	// But for SkeletalMeshActors (without physics) we don't really care - so skip updating any components if its a 'collision update'
	if( !bCollisionUpdate || SkeletalMeshComponent->bHasPhysicsAssetInstance || SkeletalMeshComponent->bEnableClothSimulation)
	{
		ConditionalUpdateComponents(bCollisionUpdate);
	}
}

/*  VerifyAnimationMatchSkeletalMesh: 
 *  Verify Animation Matches to SkeletalMesh it's about to play
 *  Since for Matinee, it's hard to verify if animation it's playing matches to the skeletalmesh
 *  This function verifies and make sure # of bone tracks matches to the skeletalmesh ref pose tracks.
 *  If you skip this check, and if it doesn't match, you'll have crash later on when playing
 *  To prevent crash, adding this verification process
 */
 // Base Version
UBOOL VerifyAnimationMatchSkeletalMesh(UAnimSet * AnimSet, USkeletalMesh * SkeletalMesh)
{
	if ( AnimSet && SkeletalMesh )
	{
		INT AnimLinkupIndex = AnimSet->GetMeshLinkupIndex( SkeletalMesh );

		if ( AnimLinkupIndex == INDEX_NONE || AnimLinkupIndex >= AnimSet->LinkupCache.Num() || SkeletalMesh->SkelMeshRUID == 0 )
		{
			debugf(NAME_Warning, TEXT("[%s:%s] Animation Link up missing"), *SkeletalMesh->GetName(), *AnimSet->GetName());
			return FALSE;
		}

		FAnimSetMeshLinkup* AnimLinkup = &AnimSet->LinkupCache(AnimLinkupIndex);

		if( AnimLinkup->BoneToTrackTable.Num() != SkeletalMesh->RefSkeleton.Num() )
		{
			debugf(NAME_Warning, TEXT("[%s:%s] Animation does not work with the skeletalmesh."), *SkeletalMesh->GetName(), *AnimSet->GetName());		
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

UBOOL VerifyAnimationMatchSkeletalMesh(UAnimNodeSequence * AnimSeqNode, USkeletalMesh * SkeletalMesh)
{
	if ( AnimSeqNode && AnimSeqNode->AnimSeq && SkeletalMesh )
	{
		return VerifyAnimationMatchSkeletalMesh(AnimSeqNode->AnimSeq->GetAnimSet(), SkeletalMesh);
	}

	return FALSE;
}

// This one is used for SkeletalMeshMAT when it doesn't know which animset is going to play
// Go through verify all animsets the SkeletalComp has. 
// If anything fails, it won't play
UBOOL VerifyAnimationMatchSkeletalMesh(USkeletalMeshComponent * SkeletalComp)
{
	if ( SkeletalComp )
	{
		for ( INT I=0; I<SkeletalComp->AnimSets.Num(); ++I )
		{
			if (VerifyAnimationMatchSkeletalMesh(SkeletalComp->AnimSets(I), SkeletalComp->SkeletalMesh) == FALSE)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/** Update list of AnimSets for this Pawn */
void ASkeletalMeshActor::UpdateAnimSetList()
{
	// Restore default AnimSets
	RestoreAnimSetsToDefault();

	// Build New list
	BuildAnimSetList();

	// Force AnimTree to be updated with new AnimSets array
	if (SkeletalMeshComponent != NULL)
	{
		SkeletalMeshComponent->bDisableWarningWhenAnimNotFound = TRUE;
		SkeletalMeshComponent->UpdateAnimations();
		SkeletalMeshComponent->bDisableWarningWhenAnimNotFound = FALSE;
	}
}

/** Build AnimSet list, called by UpdateAnimSetList() */
void ASkeletalMeshActor::BuildAnimSetList()
{
	if (SkeletalMeshComponent)
	{
		SaveDefaultsToAnimSets();
	}

	// Add the AnimSets from Matinee
	for(INT i=0; i<InterpGroupList.Num(); i++)
	{
		UInterpGroup* AnInterpGroup = InterpGroupList(i);
		if( AnInterpGroup )
		{
			AddAnimSets( AnInterpGroup->GroupAnimSets );
		}
	}
}

/** Add a given list of anim sets on the top of the list (so they override the other ones */
void ASkeletalMeshActor::AddAnimSets(const TArray<class UAnimSet*>& CustomAnimSets)
{
	for(INT i=0; i<CustomAnimSets.Num(); i++)
	{
		SkeletalMeshComponent->AnimSets.AddUniqueItem( CustomAnimSets(i) );
	}
}

/** Restore Mesh's AnimSets to defaults, as defined in the default properties */
void ASkeletalMeshActor::RestoreAnimSetsToDefault()
{
	if (GIsEditor)
	{
		// only in editor save animation state and recover 
		// - make sure to check the flag for SkelMeshComponent we saved the state BEFORE recovering
		// this is to avoid same flag 
		if ( SkeletalMeshComponent->bValidTemporarySavedAnimSets )
		{
			// recover saved animation state if saved information is valid - still check if we have any
			if (SavedAnimSeqName != NAME_None)
			{
				// save animation state
				if( SkeletalMeshComponent->Animations && SkeletalMeshComponent->Animations->IsA(UAnimNodeSequence::StaticClass()) )
				{
					UAnimNodeSequence* Seq = CastChecked<UAnimNodeSequence>(SkeletalMeshComponent->Animations);
					Seq->AnimSeqName = SavedAnimSeqName;
					Seq->CurrentTime = SavedCurrentTime;
				}
			}

		}
	}
	
	SkeletalMeshComponent->RestoreSavedAnimSets();
}

/** Save Mesh's defaults to AnimSets, back up*/
void ASkeletalMeshActor::SaveDefaultsToAnimSets()
{
	if (GIsEditor)
	{
		// make sure our state is not cached state
		if ( SkeletalMeshComponent->bValidTemporarySavedAnimSets == FALSE )
		{
			// save animation state
			if( SkeletalMeshComponent->Animations && SkeletalMeshComponent->Animations->IsA(UAnimNodeSequence::StaticClass()) )
			{
				// if animation name exists, save with time
				UAnimNodeSequence* Seq = CastChecked<UAnimNodeSequence>(SkeletalMeshComponent->Animations);
				if (Seq->AnimSeqName!=NAME_None)
				{
					SavedAnimSeqName = Seq->AnimSeqName;
					SavedCurrentTime = Seq->CurrentTime;
				}
			}
		}
	}

	SkeletalMeshComponent->SaveAnimSets();
}

/** 
* PreviewBeginAnimControl 
*/
void ASkeletalMeshActor::PreviewBeginAnimControl(UInterpGroup* InInterpGroup)
{
	MAT_BeginAnimControl(InInterpGroup);

	SkeletalMeshComponent->InitAnimTree();
}

/** 
 * PreviewBeginAnimControl 
 */
void ASkeletalMeshActorMAT::PreviewBeginAnimControl(UInterpGroup* InInterpGroup)
{
	// We need an AnimTree in Matinee in the editor to preview the animations, so instance one now if we don't have one.
	// This function can get called multiple times, but this is safe - will only instance the first time.
	if( !SkeletalMeshComponent->Animations && SkeletalMeshComponent->AnimTreeTemplate )
	{
		SkeletalMeshComponent->Animations = SkeletalMeshComponent->AnimTreeTemplate->CopyAnimTree(SkeletalMeshComponent);
	}

	// In the editor we don't have access to Script, so cache slot nodes here :(
	SlotNodes.Empty();
	if( SkeletalMeshComponent->Animations )
	{
		TArray<UAnimNode*> Nodes;
		SkeletalMeshComponent->Animations->GetNodesByClass(Nodes, UAnimNodeSlot::StaticClass());

		for(INT i=0; i<Nodes.Num(); i++)
		{
			UAnimNodeSlot* SlotNode = Cast<UAnimNodeSlot>(Nodes(i));
			if( SlotNode )
			{
				SlotNodes.AddItem(SlotNode);
			}
		}
	}
	else
	{
		debugf(TEXT("PreviewBeginAnimControl, no AnimTree, couldn't cache slots!") );
	}

	Super::PreviewBeginAnimControl(InInterpGroup);
}

/** 
* Start AnimControl. Add required AnimSets. 
*/
void ASkeletalMeshActor::MAT_BeginAnimControl(UInterpGroup* InInterpGroup)
{
	// Add our InterpGroup to the list
	// this list will be used to build animlist
	InterpGroupList.AddUniqueItem(InInterpGroup);

	// Update AnimSet list.
	UpdateAnimSetList();
}

/** 
* PreviewFinishAnimControl 
*/
void ASkeletalMeshActor::PreviewFinishAnimControl(UInterpGroup* InInterpGroup)
{
	MAT_FinishAnimControl(InInterpGroup);

	// In editor free up AnimSets.
	// In-game, we'll keep the references until the SkelMeshActor is streamed out.
	// Take out our group from the list
	InterpGroupList.RemoveItem(InInterpGroup);

	// Update AnimSet list.
	UpdateAnimSetList();

	// Update space bases to reset it back to ref pose
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* PreviewFinishAnimControl 
*/
void ASkeletalMeshActorMAT::PreviewFinishAnimControl(UInterpGroup* InInterpGroup)
{
	MAT_FinishAnimControl(InInterpGroup);
	// In editor free up AnimSets.
	// In-game, we'll keep the references until the SkelMeshActor is streamed out.
	// Take out our group from the list
	InterpGroupList.RemoveItem(InInterpGroup);

	// Update AnimSet list.
	UpdateAnimSetList();

	// When done in Matinee in the editor, drop the AnimTree instance.
	SkeletalMeshComponent->Animations = NULL;

	// before free up, clears root motion
	// Clear references to AnimSets which were added.
	FAnimSlotInfo SlotNodeInfo;
	// clear the weight
	SlotNodeInfo.ChannelWeights.AddItem(0.0f);

	for(INT SlotIdx=0; SlotIdx<SlotNodes.Num(); SlotIdx++)
	{
		UAnimNodeSlot* SlotNode = SlotNodes(SlotIdx);
		if( SlotNode )
		{	
			SlotNode->MAT_SetAnimWeights(SlotNodeInfo);
			//unset root motion
			SlotNode->SetRootBoneAxisOption(RBA_Default, RBA_Default, RBA_Default);
		}
	}

	// In the editor, free up the slot nodes.
	SlotNodes.Empty();

	// Update space bases to reset it back to ref pose
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* End AnimControl. Release required AnimSets 
*/
void ASkeletalMeshActor::MAT_FinishAnimControl(UInterpGroup* InInterpGroup)
{
}

/** 
* PreviewSetAnimPosition 
*/
void ASkeletalMeshActor::PreviewSetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bLooping, UBOOL bFireNotifies, UBOOL bEnableRootMotion, FLOAT DeltaTime)
{
	UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(SkeletalMeshComponent->Animations);

	// Do nothing if no anim tree or its not an AnimNodeSequence
	if(!SeqNode)
	{
		return;
	}

	if(SeqNode->AnimSeqName != InAnimSeqName || SeqNode->AnimSeq == NULL )
	{
		SeqNode->SetAnim(InAnimSeqName);
		// first time jump on it
		SeqNode->SetPosition(InPosition, FALSE);
	}

	if (SkeletalMeshComponent)
	{
		if ( bEnableRootMotion )
		{
			SkeletalMeshComponent->RootMotionMode = RMM_Translate;
			SeqNode->SetRootBoneAxisOption(RBA_Translate, RBA_Translate, RBA_Translate);
			SkeletalMeshComponent->RootMotionRotationMode = RMRM_RotateActor;
			SeqNode->SetRootBoneRotationOption(RRO_Extract, RRO_Extract, RRO_Extract);
		}
		else
		{
			SkeletalMeshComponent->RootMotionMode = RMM_Ignore;
			SeqNode->SetRootBoneAxisOption(RBA_Default, RBA_Default, RBA_Default);
			SkeletalMeshComponent->RootMotionRotationMode = RMRM_Ignore;
			SeqNode->SetRootBoneRotationOption(RRO_Default, RRO_Default, RRO_Default);
		}
	}

	// Verify if skeletalmesh can work with given animation
	if (VerifyAnimationMatchSkeletalMesh(SeqNode, SkeletalMeshComponent->SkeletalMesh) == FALSE)
	{
		return;
	}

	SeqNode->bLooping = bLooping;
	// set previous time here before setting position
	SeqNode->PreviousTime = SeqNode->CurrentTime;
	SeqNode->SetPosition(InPosition, bFireNotifies);

	// Update space bases so new animation position has an effect.
	SkeletalMeshComponent->UpdateSkelPose(DeltaTime, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* PreviewSetAnimPosition 
*/
void ASkeletalMeshActorMAT::PreviewSetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bLooping, UBOOL bFireNotifies, UBOOL bEnableRootMotion, FLOAT DeltaTime)
{
	MAT_SetAnimPosition(SlotName, ChannelIndex, InAnimSeqName, InPosition, bFireNotifies, bLooping, bEnableRootMotion);

	// Update space bases so new animation position has an effect.
	SkeletalMeshComponent->UpdateSkelPose(DeltaTime, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* Update AnimTree from track info 
*/
void ASkeletalMeshActorMAT::MAT_SetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bFireNotifies, UBOOL bLooping, UBOOL bEnableRootMotion)
{
	// Ensure anims are updated correctly in cinematics even when the mesh isn't rendered.
	SkeletalMeshComponent->LastRenderTime = GWorld->GetTimeSeconds();

	// Forward animation positions to slots. They will forward to relevant channels.
	for(INT i=0; i<SlotNodes.Num(); i++)
	{
		UAnimNodeSlot* SlotNode = SlotNodes(i);
		if( SlotNode && SlotNode->NodeName == SlotName )
		{	
			// Verify if skeletalmesh can work with given animation		
			if ( VerifyAnimationMatchSkeletalMesh(SkeletalMeshComponent) == FALSE)
			{
				return;
			}

			SlotNode->MAT_SetAnimPosition(ChannelIndex, InAnimSeqName, InPosition, bFireNotifies, bLooping, bEnableRootMotion);
		}
	}
}

/** 
* PreviewSetAnimWeights 
*/
void ASkeletalMeshActor::PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos)
{
	// do nothing
}

/** 
 * PreviewSetAnimWeights 
 */
void ASkeletalMeshActorMAT::PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos)
{
	MAT_SetAnimWeights(SlotInfos);
}

/** Called each from while the Matinee action is running, to set the animation weights for the actor. */
void ASkeletalMeshActor::SetAnimWeights( const TArray<struct FAnimSlotInfo>& SlotInfos )
{
	// do nothing
}

/** Called each from while the Matinee action is running, to set the animation weights for the actor. */
void ASkeletalMeshActorMAT::SetAnimWeights( const TArray<struct FAnimSlotInfo>& SlotInfos )
{
	MAT_SetAnimWeights( SlotInfos );
}

/** 
 * Update AnimTree from track weights 
 */
void ASkeletalMeshActorMAT::MAT_SetAnimWeights(const TArray<FAnimSlotInfo>& SlotInfos)
{
#if 0
	debugf( TEXT("-- SET ANIM WEIGHTS ---") );
	for(INT i=0; i<SlotInfos.Num(); i++)
	{
		debugf( TEXT("SLOT: %s"), *(SlotInfos(i).SlotName) );
		for(INT j=0; j<SlotInfos(i).ChannelWeights.Num(); j++)
		{
			debugf( TEXT("   CHANNEL %d: %1.3f"), j, SlotInfos(i).ChannelWeights(j) );
		}
	}
#endif

	// Forward channel weights to relevant slot(s)
	for(INT SlotInfoIdx=0; SlotInfoIdx<SlotInfos.Num(); SlotInfoIdx++)
	{
		const FAnimSlotInfo& SlotInfo = SlotInfos(SlotInfoIdx);

		for(INT SlotIdx=0; SlotIdx<SlotNodes.Num(); SlotIdx++)
		{
			UAnimNodeSlot* SlotNode = SlotNodes(SlotIdx);
			if( SlotNode && SlotNode->NodeName == SlotInfo.SlotName )
			{	
				SlotNode->MAT_SetAnimWeights(SlotInfo);
			}
		}
	}
}

void ASkeletalMeshActorMAT::PreviewSetMorphWeight(FName MorphNodeName, FLOAT MorphWeight)
{
	MAT_SetMorphWeight(MorphNodeName, MorphWeight);
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

void ASkeletalMeshActorMAT::PreviewSetSkelControlScale(FName SkelControlName, FLOAT Scale)
{
	MAT_SetSkelControlScale(SkelControlName, Scale);
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}


/** Function used to control FaceFX animation in the editor (Matinee). */
void ASkeletalMeshActor::PreviewUpdateFaceFX(UBOOL bForceAnim, const FString& GroupName, const FString& SeqName, FLOAT InPosition)
{
	//debugf( TEXT("GroupName: %s  SeqName: %s  InPos: %f"), *GroupName, *SeqName, InPosition );

	check(SkeletalMeshComponent);

	// Scrubbing case
	if(bForceAnim)
	{
#if WITH_FACEFX
		if(SkeletalMeshComponent->FaceFXActorInstance)
		{
			// FaceFX animations start at a neagative time, where zero time is where the sound begins ie. a pre-amble before the audio starts.
			// Because Matinee just think of things as starting at zero, we need to determine this start offset to place ourselves at the 
			// correct point in the FaceFX animation.

			// Get the FxActor
			FxActor* fActor = SkeletalMeshComponent->FaceFXActorInstance->GetActor();

			if (fActor)
			{
				// Find the Group by name
				FxSize GroupIndex = fActor->FindAnimGroup(TCHAR_TO_ANSI(*GroupName));
				if(FxInvalidIndex != GroupIndex)
				{
					FxAnimGroup& fGroup = fActor->GetAnimGroup(GroupIndex);

					// Find the animation by name
					FxSize SeqIndex = fGroup.FindAnim(TCHAR_TO_ANSI(*SeqName));
					if(FxInvalidIndex != SeqIndex)
					{
						const FxAnim& fAnim = fGroup.GetAnim(SeqIndex);

						// Get the offset (will be 0.0 or negative)
						FLOAT StartOffset = fAnim.GetStartTime();

						// Force FaceFX to a particular point
						SkeletalMeshComponent->FaceFXActorInstance->ForceTick(TCHAR_TO_ANSI(*SeqName), TCHAR_TO_ANSI(*GroupName), InPosition + StartOffset);

						// Update the skeleton, morphs etc.
						// The FALSE here stops us from calling regular Tick on the FaceFXActorInstance, and clobbering the ForceTick we just did.
						SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
						SkeletalMeshComponent->ConditionalUpdateTransform();
					}
				}
			}
		}
#endif
	}
	// Playback in Matinee case
	else
	{
		SkeletalMeshComponent->UpdateSkelPose();
		SkeletalMeshComponent->ConditionalUpdateTransform();
	}
}

/** Used by matinee playback to start a FaceFX animation playing. */
void ASkeletalMeshActor::PreviewActorPlayFaceFX(const FString& GroupName, const FString& SeqName, USoundCue* InSoundCue)
{
	check(SkeletalMeshComponent);
	// FaceFXAnimSetRef set to NULL because matinee mounts sets during initialization
	SkeletalMeshComponent->PlayFaceFXAnim(NULL, SeqName, GroupName, InSoundCue);
}

/** Used by matinee to stop current FaceFX animation playing. */
void ASkeletalMeshActor::PreviewActorStopFaceFX()
{
	check(SkeletalMeshComponent);
	SkeletalMeshComponent->StopFaceFXAnim();
}

/** Used in Matinee to get the AudioComponent we should play facial animation audio on. */
UAudioComponent* ASkeletalMeshActor::PreviewGetFaceFXAudioComponent()
{
	return FacialAudioComp;
}

/** Get the UFaceFXAsset that is currently being used by this Actor when playing facial animations. */
UFaceFXAsset* ASkeletalMeshActor::PreviewGetActorFaceFXAsset()
{
	check(SkeletalMeshComponent);
	if(SkeletalMeshComponent->SkeletalMesh)
	{
		return SkeletalMeshComponent->SkeletalMesh->FaceFXAsset;
	}

	return NULL;
}

/** Check SkeletalMeshActor for errors. */
#if WITH_EDITOR
void ASkeletalMeshActor::CheckForErrors()
{
	Super::CheckForErrors();

	if(SkeletalMeshComponent && SkeletalMeshComponent->SkeletalMesh)
	{
		UDynamicLightEnvironmentComponent* DynLightEnv = Cast<UDynamicLightEnvironmentComponent>(SkeletalMeshComponent->LightEnvironment);
		if(!SkeletalMeshComponent->PhysicsAsset 
			&& SkeletalMeshComponent->CastShadow 
			&& SkeletalMeshComponent->bCastDynamicShadow
			// Warn if the DLE is disabled or enabled but shadow casting is disabled
			&& (!DynLightEnv || !DynLightEnv->IsEnabled() || DynLightEnv->bCastShadows))
		{
			GWarn->MapCheck_Add(MCTYPE_PERFORMANCEWARNING, this, TEXT("SkeletalMeshActor casts shadow but has no PhysicsAsset assigned.  The shadow will be low res and inefficient."), MCACTION_NONE, TEXT("SkelMeshActorNoPhysAsset"));
		}

		const UBOOL bPreShadowAllowed = SkeletalMeshComponent->LightEnvironment && SkeletalMeshComponent->LightEnvironment->IsEnabled() && !CastChecked<UDynamicLightEnvironmentComponent>(SkeletalMeshComponent->LightEnvironment)->bUseBooleanEnvironmentShadowing;
		if(SkeletalMeshComponent->CastShadow 
			&& SkeletalMeshComponent->bCastDynamicShadow 
			&& SkeletalMeshComponent->IsAttached() 
			&& SkeletalMeshComponent->Bounds.SphereRadius > 2000.0f
			&& bPreShadowAllowed)
		{
			// Large shadow casting objects that create preshadows will cause a massive performance hit, since preshadows are meant for small shadow casters.
			// Setting bUseBooleanEnvironmentShadowing=TRUE will prevent the preshadow from being created.
			GWarn->MapCheck_Add(MCTYPE_PERFORMANCEWARNING, this, TEXT("Large actor casts a shadow and will cause an extreme performance hit unless bUseBooleanEnvironmentShadowing is set to TRUE."), MCACTION_NONE, TEXT("ActorLargeShadowCaster"));
		}

		if (SkeletalMeshComponent->bAcceptsLights 
			&& SkeletalMeshComponent->AlwaysLoadOnClient 
			&& SkeletalMeshComponent->AlwaysLoadOnServer
			&& (!LightEnvironment || !LightEnvironment->IsEnabled())
			// Don't warn for meshes with custom lighting channels (often using cinematic lighting)
			&& (SkeletalMeshComponent->LightingChannels.Dynamic || SkeletalMeshComponent->LightingChannels.Static))
		{
			GWarn->MapCheck_Add( MCTYPE_PERFORMANCEWARNING, Owner, *FString::Printf(TEXT("Skeletal Mesh component not using a light environment or custom lighting channels, will be inefficient and have incorrect lighting!") ), MCACTION_NONE, TEXT("NotUsingALightEnv") );
		}

		if(!SkeletalMeshComponent->PhysicsAsset && bCollideActors)
		{
			GWarn->MapCheck_Add(MCTYPE_WARNING, this, TEXT("SkeletalMeshActor has collision enabled but has no PhysicsAsset assigned."), MCACTION_NONE, TEXT("SkelMeshActorNoPhysAsset"));
		}
	}
}
#endif


/*-----------------------------------------------------------------------------
	ASkeletalMeshActorMAT
-----------------------------------------------------------------------------*/

FString ASkeletalMeshActor::GetDetailedInfoInternal() const
{
	FString Result;  

	if( SkeletalMeshComponent != NULL )
	{
		Result = SkeletalMeshComponent->GetDetailedInfoInternal();
	}
	else
	{
		Result = TEXT("No_SkeletalMeshComponent");
	}

	return Result;  
}

void ASkeletalMeshActorMAT::MAT_SetMorphWeight(FName MorphNodeName, FLOAT MorphWeight)
{
	if(SkeletalMeshComponent)
	{
		UMorphNodeWeight* WeightNode = Cast<UMorphNodeWeight>(SkeletalMeshComponent->FindMorphNode(MorphNodeName));
		if(WeightNode)
		{
			WeightNode->SetNodeWeight(MorphWeight);
		}
	}
}

void ASkeletalMeshActorMAT::MAT_SetSkelControlScale(FName SkelControlName, FLOAT Scale)
{
	if(SkeletalMeshComponent)
	{
		USkelControlBase* Control = SkeletalMeshComponent->FindSkelControl(SkelControlName);
		if(Control)
		{
			Control->BoneScale = Scale;
		}
	}
}

/* This is where we add information on slots and channels to the array that was passed in. */
void ASkeletalMeshActorMAT::GetAnimControlSlotDesc(TArray<struct FAnimSlotDesc>& OutSlotDescs)
{
	if( !SkeletalMeshComponent->Animations )
	{
		// fail
		appMsgf(AMT_OK, TEXT("SkeletalMeshActorMAT has no AnimTree Instance."));
		return;
	}

	// Find all AnimSlotNodes in the AnimTree
	for(INT i=0; i<SlotNodes.Num(); i++)
	{
		// Number of channels available on this slot node.
		const INT NumChannels = SlotNodes(i)->Children.Num() - 1;

		if( SlotNodes(i)->NodeName != NAME_None && NumChannels > 0 )
		{
			// Add a new element
			const INT Index = OutSlotDescs.Add(1);
			OutSlotDescs(Index).SlotName	= SlotNodes(i)->NodeName;
			OutSlotDescs(Index).NumChannels	= NumChannels;
		}
	}
}




/*-----------------------------------------------------------------------------
FSkeletalMeshSceneProxy
-----------------------------------------------------------------------------*/
#include "LevelUtils.h"
#include "UnSkeletalRender.h"

#include "ScenePrivate.h"

/** 
 * Constructor. 
 * @param	Component - skeletal mesh primitive being added
 */
FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(const USkeletalMeshComponent* Component, const FColor& InBoneColor)
 		:	FPrimitiveSceneProxy(Component, Component->SkeletalMesh->GetFName())
		,	Owner(Component->GetOwner())
		,	SkeletalMesh(Component->SkeletalMesh)
		,	MeshObject(Component->MeshObject)
 		,	PhysicsAsset(Component->PhysicsAsset)
 		,	LevelColor(255,255,255)
		,	PropertyColor(255,255,255)
 		,	bCastShadow(Component->CastShadow)
 		,	bShouldCollide(Component->ShouldCollide())
 		,	bDisplayBones(Component->bDisplayBones)
		,	bForceWireframe(Component->bForceWireframe)
		,	bMaterialsNeedMorphUsage(FALSE)
		,	MaterialViewRelevance(Component->GetMaterialViewRelevance())
		,	BoneColor(InBoneColor)
		,   WireframeOverlayColor(255, 255, 255, 255)
{
	check(MeshObject);
	check(SkeletalMesh);		  

	bIsCPUSkinned = SkeletalMesh->IsCPUSkinned();

	// Don't show bones if we have a parent anim component - would be invalid refpose in that case
	if(Component->ParentAnimComponent)
	{
		bDisplayBones = FALSE;
	}

	// setup materials and performance classification for each LOD.
	LODSections.AddZeroed(SkeletalMesh->LODModels.Num());
	for(INT LODIdx=0; LODIdx < SkeletalMesh->LODModels.Num(); LODIdx++)
	{
		const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIdx);
		const FSkeletalMeshLODInfo& Info = SkeletalMesh->LODInfo(LODIdx);
		check( Info.bEnableShadowCasting.Num() == LODModel.Sections.Num() );
		FLODSectionElements& LODSection = LODSections(LODIdx);

		// Presize the array
		LODSection.SectionElements.Empty( LODModel.Sections.Num() );
		for(INT SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
		{
			const FSkelMeshSection& Section = LODModel.Sections(SectionIndex);

			// If we are at a dropped LOD, route material index through the LODMaterialMap in the LODInfo struct.
			INT UseMaterialIndex = Section.MaterialIndex;			
			if(LODIdx > 0)
			{
				if(Section.MaterialIndex < Info.LODMaterialMap.Num())
				{
					UseMaterialIndex = Info.LODMaterialMap(Section.MaterialIndex);
					UseMaterialIndex = ::Clamp( UseMaterialIndex, 0, SkeletalMesh->Materials.Num() );
				}
			}

			// If Section is hidden, do not cast shadow
			const UBOOL bSectionHidden = MeshObject->IsMaterialHidden(LODIdx,UseMaterialIndex);
            // If the material is NULL, or isn't flagged for use with skeletal meshes, it will be replaced by the default material.
			UMaterialInterface* Material = Component->GetMaterial(UseMaterialIndex);
			LODSection.SectionElements.AddItem(
				FSectionElementInfo(
					(Material && Material->CheckMaterialUsage(MATUSAGE_SkeletalMesh)) ? Material : GEngine->DefaultMaterial,
					!bSectionHidden && bCastShadow && Info.bEnableShadowCasting( SectionIndex ),
					UseMaterialIndex
					));
		}
		// mapping from new sections used when swapping to instance weights to the SectionElements already setup for the base mesh
		for (INT InfluenceIdx=0; InfluenceIdx < LODModel.VertexInfluences.Num(); InfluenceIdx++)
		{
			const FSkeletalMeshVertexInfluences& VertInfluences = LODModel.VertexInfluences(InfluenceIdx);
			if (VertInfluences.Sections.Num() > 0)
			{
				// Add a new mapping since there are unique sections for the instance weights
				TArray<INT>& NewMapping = *new(LODSection.InstanceWeightsSectionElementsMapping) TArray<INT>();
				NewMapping.Empty(VertInfluences.Sections.Num());
				for (INT InstSectionIndex=0; InstSectionIndex < VertInfluences.Sections.Num(); InstSectionIndex++)
				{
					const FSkelMeshSection& InstSection = VertInfluences.Sections(InstSectionIndex);
					// find an existing entry with a matching material id
					INT FoundIdx = 0/*INDEX_NONE*/;
					for (INT SectionIndex=0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
					{
						const FSkelMeshSection& BaseSection = LODModel.Sections(SectionIndex);
						if (BaseSection.MaterialIndex == InstSection.MaterialIndex)
						{
							FoundIdx = SectionIndex;
							break;
						}
					}
					//@todo sz - need some warnings/logs when not found 
					check(FoundIdx != INDEX_NONE);
					NewMapping.AddItem(FoundIdx);
				}
			}
		}
		LODSection.InstanceWeightsSectionElementsMapping.Shrink();
	}

	// Try to find a color for level coloration.
	if( Owner )
	{
		ULevel* Level = Owner->GetLevel();
		ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
		if ( LevelStreaming )
		{
			LevelColor = LevelStreaming->DrawColor;
		}
	}

	// Get a color for property coloration
	GEngine->GetPropertyColorationColor( (UObject*)Component, PropertyColor );
}

// FPrimitiveSceneProxy interface.

/**
 * Adds a decal interaction to the primitive.  This is called in the rendering thread by AddDecalInteraction_GameThread.
 */
void FSkeletalMeshSceneProxy::AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction)
{
	FPrimitiveSceneProxy::AddDecalInteraction_RenderingThread( DecalInteraction );
	if ( MeshObject )
	{
		MeshObject->AddDecalInteraction_RenderingThread( DecalInteraction );
	}
}

/**
 * Removes a decal interaction from the primitive.  This is called in the rendering thread by RemoveDecalInteraction_GameThread.
 */
void FSkeletalMeshSceneProxy::RemoveDecalInteraction_RenderingThread(UDecalComponent* DecalComponent)
{
	FPrimitiveSceneProxy::RemoveDecalInteraction_RenderingThread( DecalComponent );
	if ( MeshObject )
	{
		MeshObject->RemoveDecalInteraction_RenderingThread( DecalComponent );
	}
}

/**
 *	Called during FSceneRenderer::InitViews for view processing on scene proxies before rendering them
 *  Only called for primitives that are visible and have bDynamicRelevance
 *
 *	@param	ViewFamily		The ViewFamily to pre-render for
 *	@param	VisibilityMap	A BitArray that indicates whether the primitive was visible in that view (index)
 *	@param	FrameNumber		The frame number of this pre-render
 */
void FSkeletalMeshSceneProxy::PreRenderView(const FSceneViewFamily* ViewFamily, const TBitArray<FDefaultBitArrayAllocator>& VisibilityMap, INT FrameNumber)
{
	/** Update the LOD level we want for this skeletal mesh (back on the game thread). */
	if (MeshObject)
	{
		for (INT ViewIndex = 0; ViewIndex < ViewFamily->Views.Num(); ViewIndex++)
		{
			if (VisibilityMap(ViewIndex) == TRUE)
			{
				const FSceneView* View = ViewFamily->Views(ViewIndex);
				MeshObject->UpdateMinDesiredLODLevel(View, PrimitiveSceneInfo->Bounds, FrameNumber);
			}
		}
	}
}

/** 
 * Iterates over sections,chunks,elements based on current instance weight usage 
 */
class FSkeletalMeshSectionIter
{
public:
	FSkeletalMeshSectionIter(const INT InLODIdx, const FSkeletalMeshObject& InMeshObject, const FStaticLODModel& InLODModel, const FSkeletalMeshSceneProxy::FLODSectionElements& InLODSectionElements, const TArray<FSkeletalMeshLODInfo>& InLODInfo)
		: bUseInstance(
			InMeshObject.LODInfo(InLODIdx).bUseInstancedVertexInfluences && 
			InMeshObject.LODInfo(InLODIdx).InstanceWeightUsage == IWU_FullSwap && 
			InLODModel.VertexInfluences.IsValidIndex(InMeshObject.LODInfo(InLODIdx).InstanceWeightIdx) &&
			InLODModel.VertexInfluences(InMeshObject.LODInfo(InLODIdx).InstanceWeightIdx).Sections.Num() > 0)
		, LODIdx(InLODIdx)
		, SectionIndex(0)
		, MeshObject(InMeshObject)
		, LODSectionElements(InLODSectionElements)
		, Sections(bUseInstance ? InLODModel.VertexInfluences(MeshObject.LODInfo(InLODIdx).InstanceWeightIdx).Sections : InLODModel.Sections)
		, Chunks(bUseInstance ? InLODModel.VertexInfluences(MeshObject.LODInfo(InLODIdx).InstanceWeightIdx).Chunks : InLODModel.Chunks)
	{}
	FORCEINLINE FSkeletalMeshSectionIter& operator++()
	{
		SectionIndex++;
		return *this;
	}
	FORCEINLINE operator bool() const 
	{ 
		return SectionIndex < Sections.Num();
	}
	FORCEINLINE const FSkelMeshChunk& GetChunk() const
	{
		return Chunks(GetSection().ChunkIndex);
	}
	FORCEINLINE const FSkelMeshSection& GetSection() const
	{
		return Sections(SectionIndex);
	}
	FORCEINLINE const FTwoVectors& GetCustomLeftRightVectors() const
	{
		return MeshObject.GetCustomLeftRightVectors(SectionIndex);
	}
	FORCEINLINE const FSkeletalMeshSceneProxy::FSectionElementInfo& GetSectionElementInfo() const
	{
		return bUseInstance ? 
			LODSectionElements.SectionElements(LODSectionElements.InstanceWeightsSectionElementsMapping(MeshObject.LODInfo(LODIdx).InstanceWeightIdx)(SectionIndex)) :
			LODSectionElements.SectionElements(SectionIndex);
	}
private:
	const UBOOL bUseInstance;
	const INT LODIdx;
	INT SectionIndex;
	const FSkeletalMeshObject& MeshObject;
	const FSkeletalMeshSceneProxy::FLODSectionElements& LODSectionElements;
	const TArray<FSkelMeshSection>& Sections;
	const TArray<FSkelMeshChunk>& Chunks;
};

/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
* @param	DPGIndex - current depth priority 
* @param	Flags - optional set of flags from EDrawDynamicElementFlags
*/
void FSkeletalMeshSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
	if( !MeshObject )
	{
		return;
	}		  

	const INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	if (GetDepthPriorityGroup(View) == DPGIndex)
	{
		if(IsCollisionView(View))
		{
			//JTODO: Hook up drawing physics asset under correct circumstances
		}
		else
		{
			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			for (FSkeletalMeshSectionIter Iter(LODIndex,*MeshObject,LODModel,LODSection,SkeletalMesh->LODInfo); Iter; ++Iter)
			{
				const FSkelMeshSection& Section = Iter.GetSection();
				const FSkelMeshChunk& Chunk = Iter.GetChunk();
				const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();
				const FTwoVectors& CustomLeftRightVectors = Iter.GetCustomLeftRightVectors();

				// If hidden skip the draw
				if (MeshObject->IsMaterialHidden(LODIndex,SectionElementInfo.UseMaterialIndex))
				{
					continue;
				}

				DrawDynamicElementsSection(PDI, View, DPGIndex, LODModel, LODIndex, Section, Chunk, SectionElementInfo, CustomLeftRightVectors );
			}
		}
	}

#if !FINAL_RELEASE
	// debug drawing
	TArray<FBoneAtom>* BoneSpaceBases = MeshObject->GetSpaceBases();

	if( bDisplayBones )
	{
		if(BoneSpaceBases)
		{
			DebugDrawBones(PDI,View,*BoneSpaceBases, LODModel, BoneColor);
		}
	}
	if( PhysicsAsset )
	{
		DebugDrawPhysicsAsset(PDI,View);
	}

	// Draw per-poly collision data.
	if((View->Family->ShowFlags & SHOW_Collision) && BoneSpaceBases)
	{
		DebugDrawPerPolyCollision(PDI, *BoneSpaceBases);
	}

	if (View->Family->ShowFlags & SHOW_SkeletalMeshes)
	{
		RenderBounds(PDI, DPGIndex, View->Family->ShowFlags, PrimitiveSceneInfo->Bounds, !Owner || Owner->IsSelected());
	}
#endif
}

/** 
* Draw only one section of the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
* @param	DPGIndex - current depth priority 
* @param	Flags - optional set of flags from EDrawDynamicElementFlags
* @param 	ForceLOD - Force this LOD. If -1, use current LOD of mesh. 
* @param	InMaterial - which material section to draw
*/
void FSkeletalMeshSceneProxy::DrawDynamicElementsByMaterial(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags, INT ForceLOD, INT InMaterial)
{
	// Check if this has valid DynamicData
	if( !MeshObject || !MeshObject->HaveValidDynamicData())
	{
		return;
	}		  

	const INT LODIndex = ( ForceLOD<0 )? MeshObject->GetLOD() : ForceLOD;
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	if (GetDepthPriorityGroup(View) == DPGIndex)
	{
		if(IsCollisionView(View))
		{
			//JTODO: Hook up drawing physics asset under correct circumstances
		}
		else
		{
			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			for (FSkeletalMeshSectionIter Iter(LODIndex,*MeshObject,LODModel,LODSection,SkeletalMesh->LODInfo); Iter; ++Iter)
			{
				const FSkelMeshSection& Section = Iter.GetSection();
				const FSkelMeshChunk& Chunk = Iter.GetChunk();
				const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();
				const FTwoVectors& CustomLeftRightVectors = Iter.GetCustomLeftRightVectors();

				if (SectionElementInfo.UseMaterialIndex != InMaterial)
				{
					continue;
				}

				DrawDynamicElementsSection(PDI, View, DPGIndex, LODModel, LODIndex, Section, Chunk, SectionElementInfo, CustomLeftRightVectors );
			}
		}
	}
}
void FSkeletalMeshSceneProxy::DrawDynamicElementsSection(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,
														const FStaticLODModel& LODModel, const INT LODIndex, const FSkelMeshSection& Section, 
														const FSkelMeshChunk& Chunk, const FSectionElementInfo& SectionElementInfo, const FTwoVectors& CustomLeftRightVectors )
{
	UBOOL bIsSelected = bSelected;

	// if the mesh isn't selected but the mesh section is selected in the AnimSetViewer, find the mesh component and make sure that it can be highlighted (ie. are we rendering for the AnimSetViewer or not?)
	if( !bIsSelected && Section.bSelected )
	{
		USkeletalMeshComponent* SkelMeshComp = (USkeletalMeshComponent*)PrimitiveSceneInfo->Component;
		if( SkelMeshComp )
		{
			if( SkelMeshComp->bCanHighlightSelectedSections )
			{
				bIsSelected = TRUE;
			}
		}
	}

	FLinearColor WireframeLinearColor(WireframeOverlayColor);

	// If hidden skip the draw
	if (MeshObject->IsMaterialHidden(LODIndex,SectionElementInfo.UseMaterialIndex))
	{
		return;
	}

	FMeshElement Mesh;		
	Mesh.bWireframe |= bForceWireframe;

	const FIndexBuffer* DynamicIndexBuffer = MeshObject->GetDynamicIndexBuffer(LODIndex);
	//Tearing is only enabled when we are not welding the mesh.
	if(SkeletalMesh->bEnableClothTearing && (DynamicIndexBuffer != NULL) && (SkeletalMesh->ClothWeldingMap.Num() == 0))
	{
		Mesh.IndexBuffer = DynamicIndexBuffer;
		Mesh.MaxVertexIndex = LODModel.NumVertices + SkeletalMesh->ClothTearReserve - 1;
	}
	else if(SkeletalMesh->bEnableValidBounds && (DynamicIndexBuffer != NULL) && (SkeletalMesh->ClothWeldingMap.Num() == 0))
	{
		Mesh.IndexBuffer = DynamicIndexBuffer;
		Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
	}
	else
	{
		Mesh.IndexBuffer = LODModel.DynamicIndexBuffer.IndexBuffer;
		Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
	}

	Mesh.VertexFactory = MeshObject->GetVertexFactory(LODIndex,Section.ChunkIndex);
	Mesh.DynamicVertexData = NULL;
	Mesh.MaterialRenderProxy = SectionElementInfo.Material->GetRenderProxy(bIsSelected, bHovered);

#if WITH_APEX
	FIApexClothing *Clothing = MeshObject->ApexClothing;
	if ( Clothing )
	{
		if ( GApexRender && Clothing->UsesMaterialIndex((physx::PxU32)Section.MaterialIndex) )
		{
			bool bWireframe = (View && View->Family && (View->Family->ShowFlags & SHOW_Wireframe)) ? true : false;
			physx::apex::NxUserRenderer *Renderer = GApexRender->CreateDynamicRenderer( PDI,bIsSelected,bCastShadow,bWireframe,View,DPGIndex);

			physx::PxF32 eyePos[3];
			eyePos[0] = View->ViewOrigin.X*U2PScale;
			eyePos[1] = View->ViewOrigin.Y*U2PScale;
			eyePos[2] = View->ViewOrigin.Z*U2PScale;
			if ( PDI->IsRenderingVelocities() )
			{
				return;
			}
			if ( Renderer && Clothing->IsReadyToRender(*Renderer,eyePos) )
			{
				GApexRender->ReleaseDynamicRenderer(Renderer);
				return;
			}
			GApexRender->ReleaseDynamicRenderer(Renderer);
		}
	}
#endif

	Mesh.LCI = NULL;
	GetWorldMatrices( View, Mesh.LocalToWorld, Mesh.WorldToLocal );
	Mesh.FirstIndex = Section.BaseIndex;

	// Select which indices to use if TRISORT_CustomLeftRight
	if( Section.TriangleSorting == TRISORT_CustomLeftRight )
	{
		switch( MeshObject->CustomSortAlternateIndexMode )
		{
		case CSAIM_Left:
			// Left view - use second set of indices.
			Mesh.FirstIndex += Section.NumTriangles * 3;
			break;
		case  CSAIM_Right:
			// Right view - use first set of indices.
			break;
		default:
			// Calculate the viewing direction
			FVector SortWorldOrigin = Mesh.LocalToWorld.TransformFVector(CustomLeftRightVectors.v1);
			FVector SortWorldDirection = Mesh.LocalToWorld.TransformNormal(CustomLeftRightVectors.v2);

			if( (SortWorldDirection | (SortWorldOrigin - View->ViewOrigin)) < 0.f )
			{
				Mesh.FirstIndex += Section.NumTriangles * 3;
			}
			break;
		}
	}

	Mesh.NumPrimitives = Section.NumTriangles;
	if( GIsEditor && MeshObject->ProgressiveDrawingFraction != 1.f )
	{
		if( Mesh.MaterialRenderProxy->GetMaterial()->GetBlendMode() == BLEND_Translucent )
		{
			Mesh.NumPrimitives = appRound(((FLOAT)Section.NumTriangles)*Clamp<FLOAT>(MeshObject->ProgressiveDrawingFraction,0.f,1.f));
			if( Mesh.NumPrimitives == 0 )
			{
				return;
			}
		}
	}
	Mesh.MinVertexIndex = Chunk.BaseVertexIndex;
	Mesh.UseDynamicData = FALSE;
	Mesh.ReverseCulling = (LocalToWorldDeterminant < 0.0f);
	Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DPGIndex;
	Mesh.bUsePreVertexShaderCulling = FALSE;
	Mesh.PlatformMeshData = NULL;

	const INT NumPasses = DrawRichMesh(
		PDI,
		Mesh,
		WireframeLinearColor,
		LevelColor,
		PropertyColor,
		PrimitiveSceneInfo,
		bIsSelected
		);

	const INT NumVertices = Chunk.NumRigidVertices + Chunk.NumSoftVertices;
	INC_DWORD_STAT_BY(STAT_GPUSkinVertices,(DWORD)(bIsCPUSkinned ? 0 : NumVertices * NumPasses));
	INC_DWORD_STAT_BY(STAT_SkelMeshTriangles,Mesh.NumPrimitives * NumPasses);
	INC_DWORD_STAT(STAT_SkelMeshDrawCalls);
}

IMPLEMENT_COMPARE_CONSTPOINTER( FDecalInteraction, UnSkeletalMeshRender,
{
	return (A->DecalState.SortOrder <= B->DecalState.SortOrder) ? -1 : 1;
} );

/**
* Draws the primitive's dynamic decal elements.  This is called from the rendering thread for each frame of each view.
* The dynamic elements will only be rendered if GetViewRelevance declares dynamic relevance.
* Called in the rendering thread.
*
* @param	PDI						The interface which receives the primitive elements.
* @param	View					The view which is being rendered.
* @param	InDepthPriorityGroup	The DPG which is being rendered.
* @param	bDynamicLightingPass	TRUE if drawing dynamic lights, FALSE if drawing static lights.
* @param	bDrawOpaqueDecals		TRUE if we want to draw opaque decals
* @param	bDrawTransparentDecals	TRUE if we want to draw transparent decals
* @param	bTranslucentReceiverPass	TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
*/
void FSkeletalMeshSceneProxy::DrawDynamicDecalElements(
									  FPrimitiveDrawInterface* PDI,
									  const FSceneView* View,
									  UINT InDepthPriorityGroup,
									  UBOOL bDynamicLightingPass,
									  UBOOL bDrawOpaqueDecals,
									  UBOOL bDrawTransparentDecals,
									  UBOOL bTranslucentReceiverPass
									  )
{
	SCOPE_CYCLE_COUNTER(STAT_DecalRenderDynamicSkelTime);

	// lit decals on static meshes not support for now
	if( !MeshObject || bDynamicLightingPass )
	{
		return;
	}

#if !FINAL_RELEASE
	const UBOOL bRichView = IsRichView(View);
#else
	const UBOOL bRichView = FALSE;
#endif

	checkSlow( View->Family->ShowFlags & SHOW_Decals );

	const INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	
	if( GetDepthPriorityGroup(View) == InDepthPriorityGroup )
	{
		// Compute the set of decals in this DPG.
		FMemMark MemStackMark(GRenderingThreadMemStack);
		TArray<FDecalInteraction*,TMemStackAllocator<GRenderingThreadMemStack> > DPGDecals;
		for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
		{
			FDecalInteraction* Interaction = Decals(DecalIndex);
			if( // only render decals that haven't been added to a static batch
				(!Interaction->DecalStaticMesh || bRichView) &&
				// match current DPG
				InDepthPriorityGroup == Interaction->DecalState.DepthPriorityGroup &&
				// only render transparent or opaque decals as they are requested
				((Interaction->DecalState.MaterialViewRelevance.bTranslucency && bDrawTransparentDecals) || (Interaction->DecalState.MaterialViewRelevance.bOpaque && bDrawOpaqueDecals)) &&
				// only render lit decals during dynamic lighting pass
				((Interaction->DecalState.MaterialViewRelevance.bLit && bDynamicLightingPass) || !bDynamicLightingPass) )
			{
				DPGDecals.AddItem( Interaction );
			}
		}

		if ( DPGDecals.Num() > 0 )
		{
			// Sort decals for the translucent receiver pass
			if( bTranslucentReceiverPass )
			{
				Sort<USE_COMPARE_CONSTPOINTER(FDecalInteraction,UnSkeletalMeshRender)>( DPGDecals.GetTypedData(), DPGDecals.Num() );
			}

			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			for ( INT DecalIndex = 0 ; DecalIndex < DPGDecals.Num() ; ++DecalIndex )
			{
				const FDecalInteraction* Decal	= DPGDecals(DecalIndex);
				FDecalState DecalState = Decal->DecalState;

				FMatrix DecalMatrix;
				FBoneAtom DecalRefToLocalMatrix;
				FVector DecalLocation;
				FVector2D DecalOffset;
				MeshObject->TransformDecalState( DecalState, DecalMatrix, DecalLocation, DecalOffset, DecalRefToLocalMatrix );

				DecalState.TransformFrustumVerts( DecalRefToLocalMatrix );
				DecalState.bUseSoftwareClip = FALSE;

				for (FSkeletalMeshSectionIter Iter(LODIndex,*MeshObject,LODModel,LODSection,SkeletalMesh->LODInfo); Iter; ++Iter)
				{
					const FSkelMeshSection& Section = Iter.GetSection();
					const FSkelMeshChunk& Chunk = Iter.GetChunk();
					const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();

					FMeshElement Mesh;		
					Mesh.bWireframe |= bForceWireframe;
					Mesh.IndexBuffer = LODModel.DynamicIndexBuffer.IndexBuffer; 

					FDecalVertexFactoryBase* DecalVertexFactory = MeshObject->GetDecalVertexFactory(LODIndex,Section.ChunkIndex,Decal);
					DecalVertexFactory->SetDecalMatrix( DecalMatrix );
					DecalVertexFactory->SetDecalLocation( DecalLocation );
					DecalVertexFactory->SetDecalOffset( DecalOffset );
					Mesh.VertexFactory = DecalVertexFactory->CastToFVertexFactory();

					Mesh.DynamicVertexData = NULL;
					Mesh.MaterialRenderProxy = DecalState.DecalMaterial->GetRenderProxy(FALSE);
					Mesh.LCI = NULL;
					GetWorldMatrices( View, Mesh.LocalToWorld, Mesh.WorldToLocal );
					Mesh.FirstIndex = Section.BaseIndex;
					Mesh.NumPrimitives = Section.NumTriangles;
					Mesh.MinVertexIndex = Chunk.BaseVertexIndex;
					Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
					Mesh.UseDynamicData = FALSE;
					Mesh.ReverseCulling = (LocalToWorldDeterminant < 0.0f);
					Mesh.CastShadow = FALSE;
					Mesh.DepthBias = DecalState.DepthBias;
					Mesh.SlopeScaleDepthBias = DecalState.SlopeScaleDepthBias;
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)InDepthPriorityGroup;
					Mesh.bIsDecal = TRUE;
					Mesh.DecalState = &DecalState;
					Mesh.bUsePreVertexShaderCulling = FALSE;
					Mesh.PlatformMeshData = NULL;

					static const FLinearColor WireColor(0.5f,1.0f,0.5f);
					const INT NumPasses = DrawRichMesh(
						PDI,
						Mesh,
						WireColor,
						LevelColor,
						PropertyColor,
						PrimitiveSceneInfo,
						FALSE
						);

					INC_DWORD_STAT_BY(STAT_DecalTriangles,Mesh.NumPrimitives*NumPasses);
					INC_DWORD_STAT(STAT_DecalDrawCalls);
				}
			}
		}
	}
}

/**
 * Returns the world transform to use for drawing.
 * @param View - Current view
 * @param OutLocalToWorld - Will contain the local-to-world transform when the function returns.
 * @param OutWorldToLocal - Will contain the world-to-local transform when the function returns.
 */
void FSkeletalMeshSceneProxy::GetWorldMatrices( const FSceneView* View, FMatrix& OutLocalToWorld, FMatrix& OutWorldToLocal )
{
	OutLocalToWorld = LocalToWorld;
	OutWorldToLocal = LocalToWorld.Inverse();
}

/**
 * Relevance is always dynamic for skel meshes unless they are disabled
 */
FPrimitiveViewRelevance FSkeletalMeshSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	if (View->Family->ShowFlags & SHOW_SkeletalMeshes)
	{
		if(IsShown(View))
		{
			Result.bNeedsPreRenderView = TRUE;
			Result.bDynamicRelevance = TRUE;
			Result.SetDPG(GetDepthPriorityGroup(View),TRUE);

			// only add to foreground DPG for debug rendering
			if( bDisplayBones ||
				View->Family->ShowFlags & (SHOW_Bounds|SHOW_Collision) )
			{
				Result.SetDPG(SDPG_Foreground,TRUE);
			}		
			Result.bDecalStaticRelevance = HasRelevantStaticDecals(View);
			Result.bDecalDynamicRelevance = HasRelevantDynamicDecals(View);
		}
		if (IsShadowCast(View))
		{
			Result.bShadowRelevance = TRUE;
		}
		MaterialViewRelevance.SetPrimitiveViewRelevance(Result);
	}
	return Result;
}

/** Util for getting LOD index currently used by this SceneProxy. */
INT FSkeletalMeshSceneProxy::GetCurrentLODIndex()
{
	if(MeshObject)
	{
		return MeshObject->GetLOD();
	}
	else
	{
		return 0;
	}
}

/** 
 * Render bones for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawBones(FPrimitiveDrawInterface* PDI,const FSceneView* View, const TArray<FBoneAtom>& InSpaceBases, const class FStaticLODModel& LODModel, const FColor& LineColor)
{
	FMatrix LocalToWorld, WorldToLocal;
	GetWorldMatrices( View, LocalToWorld, WorldToLocal );

	TArray<FMatrix> WorldBases;
	WorldBases.Add( InSpaceBases.Num() );

	// If we are using a weight swap, only draw the bones relevant to the swap
	TArray<BYTE> RequiredBones;
	FSkeletalMeshObject::FSkelMeshObjectLODInfo& MeshLODInfo = MeshObject->LODInfo(GetCurrentLODIndex());
	if (MeshLODInfo.InstanceWeightUsage == IWU_FullSwap && MeshLODInfo.bUseInstancedVertexInfluences && LODModel.VertexInfluences.Num() > 0)
	{
		RequiredBones = LODModel.VertexInfluences(0).RequiredBones;
	}
	else
	{
		RequiredBones = LODModel.RequiredBones;
	}
	

	for(INT i=0; i<RequiredBones.Num(); i++)
	{
		INT BoneIndex = RequiredBones(i);
		check(BoneIndex < InSpaceBases.Num());

		// transform bone mats to world space
		WorldBases(BoneIndex) = InSpaceBases(BoneIndex).ToMatrix() * LocalToWorld;

		const FColor& BoneColor = LineColor; 
		if( BoneColor.A != 0 )
		{
			if( BoneIndex == 0 )
			{
				PDI->DrawLine(WorldBases(BoneIndex).GetOrigin(), LocalToWorld.GetOrigin(), FColor(255, 0, 255), SDPG_Foreground);
			}
			else
			{
				INT ParentIdx = SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
				PDI->DrawLine(WorldBases(BoneIndex).GetOrigin(), WorldBases(ParentIdx).GetOrigin(), BoneColor, SDPG_Foreground);
			}
			// Display colored coordinate system axes for each joint.			  
			// Red = X
			FVector XAxis =  WorldBases(BoneIndex).TransformNormal( FVector(1.0f,0.0f,0.0f));
			XAxis.Normalize();
			PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + XAxis * 3.75f, FColor( 255, 80, 80), SDPG_Foreground );			
			// Green = Y
			FVector YAxis =  WorldBases(BoneIndex).TransformNormal( FVector(0.0f,1.0f,0.0f));
			YAxis.Normalize();
			PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + YAxis * 3.75f, FColor( 80, 255, 80), SDPG_Foreground ); 
			// Blue = Z
			FVector ZAxis =  WorldBases(BoneIndex).TransformNormal( FVector(0.0f,0.0f,1.0f));
			ZAxis.Normalize();
			PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + ZAxis * 3.75f, FColor( 80, 80, 255), SDPG_Foreground ); 
		}
	}
}

/** 
 * Render physics asset for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawPhysicsAsset(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	FMatrix LocalToWorld, WorldToLocal;
	GetWorldMatrices( View, LocalToWorld, WorldToLocal );

	FMatrix ScalingMatrix = LocalToWorld;
	FVector TotalScale = ScalingMatrix.ExtractScaling();

	// Only valid if scaling if uniform.
	if( TotalScale.IsUniform() )
	{
		TArray<FBoneAtom>* BoneSpaceBases = MeshObject->GetSpaceBases();
		if(BoneSpaceBases)
		{
			const EShowFlags ShowFlags = View->Family->ShowFlags;
			check(PhysicsAsset);
			if( (ShowFlags & SHOW_Collision) && bShouldCollide )
			{
				PhysicsAsset->DrawCollision(PDI, SkeletalMesh, *BoneSpaceBases, LocalToWorld, TotalScale.X);
			}
			if( ShowFlags & SHOW_Constraints )
			{
				PhysicsAsset->DrawConstraints(PDI, SkeletalMesh, *BoneSpaceBases, LocalToWorld, TotalScale.X);
			}
		}
	}
}

/** Render any per-poly collision data for tri's rigidly weighted to bones. */
void FSkeletalMeshSceneProxy::DebugDrawPerPolyCollision(FPrimitiveDrawInterface* PDI, const TArray<FBoneAtom>& InSpaceBases)
{
	check(SkeletalMesh->PerPolyCollisionBones.Num() == SkeletalMesh->PerPolyBoneKDOPs.Num());
	// Iterate over each bone with per-poly collision data
	for(INT BoneIdx=0; BoneIdx<SkeletalMesh->PerPolyBoneKDOPs.Num(); BoneIdx++)
	{
		// Pick a different colour for each bone.
		FColor BoneColor = DebugUtilColor[BoneIdx%NUM_DEBUG_UTIL_COLORS];
		const FPerPolyBoneCollisionData& Data = SkeletalMesh->PerPolyBoneKDOPs(BoneIdx);
		INT BoneIndex = SkeletalMesh->MatchRefBone(SkeletalMesh->PerPolyCollisionBones(BoneIdx));
		if(BoneIndex != INDEX_NONE)
		{
			// Calculate transform from bone-space to world-space
			FMatrix BoneToWorld = InSpaceBases(BoneIndex).ToMatrix() * LocalToWorld;
			// Iterate over each triangle
			for(INT TriIndex=0; TriIndex<Data.KDOPTree.Triangles.Num(); TriIndex++)
			{
				const FkDOPCollisionTriangle<WORD>& Tri = Data.KDOPTree.Triangles(TriIndex);

				// TRansform verts into world space
				FVector WorldVert1 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v1) );
				FVector WorldVert2 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v2) );
				FVector WorldVert3 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v3) );

				// Draw wire triangle
				PDI->DrawLine( WorldVert1, WorldVert2, BoneColor, SDPG_World);
				PDI->DrawLine( WorldVert2, WorldVert3, BoneColor, SDPG_World);
				PDI->DrawLine( WorldVert3, WorldVert1, BoneColor, SDPG_World);
			}
		}
	}
}

/** 
 * Render soft body tetrahedra for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawSoftBodyTetras(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	const TArray<INT>& IndexData = SkeletalMesh->SoftBodyTetraIndices;
	const TArray<FVector>* PosDataPtr = MeshObject->GetSoftBodyTetraPosData();
	
	if(PosDataPtr && PosDataPtr->Num() > 0)
	{
		const TArray<FVector>& PosData = *PosDataPtr;

		for(INT i=0; i<IndexData.Num(); i+=4)
		{
			INT Idx0 = IndexData(i + 0);
			FVector pt0 = PosData(Idx0) * P2UScale;
		
			INT Idx1 = IndexData(i + 1);
			FVector pt1 = PosData(Idx1) * P2UScale;

			INT Idx2 = IndexData(i + 2);
			FVector pt2 = PosData(Idx2) * P2UScale;

			INT Idx3 = IndexData(i + 3);
			FVector pt3 = PosData(Idx3) * P2UScale;

			PDI->DrawLine(pt2, pt1, FColor(0, 255, 0), SDPG_Foreground);
			PDI->DrawLine(pt1, pt0, FColor(0, 255, 0), SDPG_Foreground);
			PDI->DrawLine(pt1, pt3, FColor(0, 255, 0), SDPG_Foreground);
			PDI->DrawLine(pt2, pt3, FColor(0, 255, 0), SDPG_Foreground);
			PDI->DrawLine(pt2, pt0, FColor(0, 255, 0), SDPG_Foreground);
			PDI->DrawLine(pt0, pt3, FColor(0, 255, 0), SDPG_Foreground);
		}
	}
}

/**
* Updates morph material usage for materials referenced by each LOD entry
*
* @param bNeedsMorphUsage - TRUE if the materials used by this skeletal mesh need morph target usage
*/
void FSkeletalMeshSceneProxy::UpdateMorphMaterialUsage(UBOOL bNeedsMorphUsage)
{
	if( bNeedsMorphUsage != bMaterialsNeedMorphUsage )
	{
		// keep track of current morph material usage for the proxy
		bMaterialsNeedMorphUsage = bNeedsMorphUsage;

		// create new LOD sections entries based on updated material usage
		TArray<FLODSectionElements> NewLODSections;
		NewLODSections = LODSections;
		for( INT LodIdx=0; LodIdx < NewLODSections.Num(); LodIdx++ )
		{
			FLODSectionElements& LODSection = NewLODSections(LodIdx);
			for( INT SectIdx=0; SectIdx < LODSection.SectionElements.Num(); SectIdx++ )
			{
				FSectionElementInfo& SectionElement = LODSection.SectionElements(SectIdx);
				if( SectionElement.Material )
				{
					const UBOOL bCheckMorphUsage = !bMaterialsNeedMorphUsage || (bMaterialsNeedMorphUsage && SectionElement.Material->CheckMaterialUsage(MATUSAGE_MorphTargets));
					const UBOOL bCheckSkelUsage = SectionElement.Material->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
					// make sure morph material usage and default skeletal usage are both valid
					if( !bCheckMorphUsage || !bCheckSkelUsage  )
					{
						// fallback to default material if needed
						SectionElement.Material = GEngine->DefaultMaterial;	
					}					
				}
			}
		}

		// update the new LODSections on the render thread proxy
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateSkelProxyLODSectionElementsCmd,
			TArray<FLODSectionElements>,NewLODSections,NewLODSections,
			FSkeletalMeshSceneProxy*,SkelMeshSceneProxy,this,
		{
			SkelMeshSceneProxy->LODSections = NewLODSections;
		});
	}
}

/**
* Checks/updates material usage on proxy based on current morph target usage
*/
void USkeletalMeshComponent::UpdateMorphMaterialUsageOnProxy()
{
	// update morph material usage
	if( SceneInfo && 
		SceneInfo->Proxy )
	{
		const UBOOL bHasMorphs = ActiveMorphs.Num() > 0;
		((FSkeletalMeshSceneProxy*)SceneInfo->Proxy)->UpdateMorphMaterialUsage(bHasMorphs);
	}
}

// UObject interface
// Override to have counting working better
void USkeletalMeshComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
		 	
	if(Ar.IsCountingMemory())
 	{
		// add all native variables - mostly bigger chunks 
 		SpaceBases.CountBytes(Ar);
 		LocalAtoms.CountBytes(Ar);
		CachedLocalAtoms.CountBytes(Ar);
		CachedSpaceBases.CountBytes(Ar);
 		RequiredBones.CountBytes(Ar);
 		ComposeOrderedRequiredBones.CountBytes(Ar);
 		ParentBoneMap.CountBytes(Ar);
 		TemporarySavedAnimSets.CountBytes(Ar);
 		MorphTargetIndexMap.CountBytes(Ar);
 		InstanceVertexWeightBones.CountBytes(Ar);
 		ClothMeshWeldedPosData.CountBytes(Ar);
 		ClothMeshWeldedNormalData.CountBytes(Ar);
 		ClothMeshWeldedIndexData.CountBytes(Ar);
 	}
}

/**
* Returns the size of the object/ resource for display to artists/ LDs in the Editor.
*
* @return size of resource as to be displayed to artists/ LDs in the Editor.
*/
INT USkeletalMeshComponent::GetResourceSize()
{
	INT ResourceSize = 0;
 	// Get Mesh Object's memory
 	if(MeshObject)
 	{
 		ResourceSize += MeshObject->GetResourceSize();
 	}

	return ResourceSize;
}
/** Create the scene proxy needed for rendering a skeletal mesh */
FPrimitiveSceneProxy* USkeletalMeshComponent::CreateSceneProxy()
{
	FSkeletalMeshSceneProxy* Result = NULL;

	// only create a scene proxy for rendering if
	// properly initialized
	if( SkeletalMesh && 
		SkeletalMesh->LODModels.IsValidIndex(PredictedLODLevel) &&
		!bHideSkin &&
		MeshObject )
	{
		Result = ::new FSkeletalMeshSceneProxy(this);
	}

	return Result;
}

UBOOL USkeletalMeshComponent::ExtractRootMotionCurve( FName AnimName, FLOAT SampleRate, FRootMotionCurve& out_RootMotionInterpCurve )
{
	const UAnimSequence* AnimSeq = FindAnimSequence( AnimName );
	if( AnimSeq == NULL ) 
	{
		warnf( TEXT("ExtractRootMotionCurve: unable to find AnimName: [%s] on [%s]"), *AnimName.ToString(), *GetDetailedInfo() );
		return FALSE;
	}
	
	const INT RootBoneIndex	= SkeletalMesh->LODModels(0).RequiredBones(0);
	if( RootBoneIndex != INDEX_NONE )
	{
		FLOAT CurrentTime = 0.f;
		FVector PrevPos(0);

		UBOOL bFoundEnd = FALSE;
		INT PointsAdded = 0;

		out_RootMotionInterpCurve.AnimName = AnimName;
		out_RootMotionInterpCurve.MaxCurveTime = AnimSeq->SequenceLength;
		out_RootMotionInterpCurve.Curve.Points.Empty();

		while( CurrentTime <= AnimSeq->SequenceLength )
		{
			FBoneAtom CurrentFrameAtom;
			AnimSeq->GetBoneAtom( CurrentFrameAtom, RootBoneIndex, CurrentTime, FALSE, bUseRawData);

			const FVector RootBoneDelta = CurrentFrameAtom.GetTranslation() - PrevPos;

			// Create one key to enforce the current value.
			FInterpCurvePoint<FVector> Key;
			Key.InVal			= CurrentTime;
			Key.OutVal			= RootBoneDelta;
			Key.ArriveTangent	= FVector(0.0f, 0.0f, 0.0f);
			Key.LeaveTangent	= FVector(0.0f, 0.0f, 0.0f);
			Key.InterpMode		= CIM_Linear;

//			debugf(TEXT("ROOTBONEDELTA %s -- %f"), *RootBoneDelta.ToString(), CurrentTime );

			out_RootMotionInterpCurve.Curve.Points.AddItem( Key );
			PointsAdded++;

			PrevPos = CurrentFrameAtom.GetTranslation();
			CurrentTime += SampleRate;
			if( CurrentTime >= AnimSeq->SequenceLength )
			{
				CurrentTime = AnimSeq->SequenceLength;
				if( bFoundEnd )
				{
					break;
				}
				bFoundEnd = TRUE;
			}
		}

		return (PointsAdded > 0);
	}

	return FALSE;
}

