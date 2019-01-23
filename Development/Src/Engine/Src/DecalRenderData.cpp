/*=============================================================================
	DecalRenderData.cpp: Utility classes for rendering decals.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "UnDecalRenderData.h"
//#include "LevelUtils.h"

IMPLEMENT_CLASS(UDecalMaterial);

/*-----------------------------------------------------------------------------
   FDecalVertex
-----------------------------------------------------------------------------*/

/**
 * FDecalVertex serializer.
 */
FArchive& operator<<(FArchive& Ar, FDecalVertex& DecalVertex)
{
	Ar << DecalVertex.Position;
	Ar << DecalVertex.TangentX;
	if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
	{
		FPackedNormal TangentY;
		Ar << TangentY;
		Ar << DecalVertex.TangentZ;
		// store determinant sign in w component of normal
		DecalVertex.TangentZ.Vector.W = GetBasisDeterminantSignByte(DecalVertex.TangentX,TangentY,DecalVertex.TangentZ);
	}
	else
	{
		Ar << DecalVertex.TangentZ;
	}
	
	Ar << DecalVertex.UV;

	if ( Ar.Ver() < VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD )
	{
		check( Ar.IsLoading() );
		DecalVertex.LightMapCoordinate = FVector2D( 0.f, 0.f );
	}
	else
	{
		Ar << DecalVertex.LightMapCoordinate;
	}
	Ar << DecalVertex.NormalTransform0;
	Ar << DecalVertex.NormalTransform1;
	return Ar;
}

/*-----------------------------------------------------------------------------
   FDecalVertexBuffer
-----------------------------------------------------------------------------*/

/**
 * FRenderResource interface for FDecalVertexBuffer.
 * Has to be defined after FDecalRenderData has been properly declared.
 */
void FDecalVertexBuffer::InitRHI()
{
	const INT NumVerts = DecalRenderData->Vertices.Num();
	if ( NumVerts > 0 )
	{
		const UINT Size = NumVerts*sizeof(FDecalVertex);
		VertexBufferRHI = RHICreateVertexBuffer( Size, NULL, FALSE );
		void* DestBuf = RHILockVertexBuffer( VertexBufferRHI, 0, Size );
		appMemcpy( DestBuf, DecalRenderData->Vertices.GetData(), Size );
		RHIUnlockVertexBuffer( VertexBufferRHI );
	}
}

/*-----------------------------------------------------------------------------
	FDecalLocalSpaceInfo
-----------------------------------------------------------------------------*/

FDecalLocalSpaceInfo::FDecalLocalSpaceInfo(const FDecalState* InDecal, const FMatrix& InReceiverLocalToWorld)		
{
	Init( InDecal, InReceiverLocalToWorld );
}

void FDecalLocalSpaceInfo::Init(const FDecalState* InDecal, const FMatrix& InReceiverLocalToWorld)
{
	check( InDecal );
	Decal = InDecal;

	ReceiverWorldToLocal	= InReceiverLocalToWorld.Inverse();
	TextureTransform		= InReceiverLocalToWorld * Decal->WorldTexCoordMtx;
	TextureHitLocation		= Decal->WorldTexCoordMtx.TransformFVector( Decal->HitLocation );

	for ( INT PlaneIndex = 0 ; PlaneIndex < Decal->Planes.Num() ; ++PlaneIndex )
	{
		Convex.Planes.AddItem( Decal->Planes(PlaneIndex).TransformBy( ReceiverWorldToLocal ) );
	}

	LocalLookVector = ReceiverWorldToLocal.TransformNormal( Decal->OrientationVector ).SafeNormal();
	LocalLocation = ReceiverWorldToLocal.TransformFVector( Decal->HitLocation );
	LocalTangent = ReceiverWorldToLocal.TransformNormal( Decal->HitTangent ).SafeNormal();
	LocalBinormal = ReceiverWorldToLocal.TransformNormal( Decal->HitBinormal ).SafeNormal();
}

/**
 * Computes decal texture coordinates from the the specified world-space position.
 *
 * @param		InPos			World-space position.
 * @param		OutTexCoords	[out] Decal texture coordinates.
 */
void FDecalLocalSpaceInfo::ComputeTextureCoordinates(const FVector& InPos, FVector2D& OutTexCoords) const
{
	const FVector OutPos = TextureTransform.TransformFVector( InPos ) - TextureHitLocation;
	OutTexCoords.X = (-OutPos.X+0.5f+Decal->OffsetX);
	OutTexCoords.Y = (-OutPos.Y+0.5f+Decal->OffsetY);
}

/*-----------------------------------------------------------------------------
   FDecalRenderData
-----------------------------------------------------------------------------*/

/**
 * Prepares resources for deletion.
 * This is only called by the rendering thread.
 */
FDecalRenderData::~FDecalRenderData()
{
	// Prepare resources for deletion.
	// ReleaseResources_RenderingThread() will assert we're in the rendering thread.
	ReleaseResources_RenderingThread();

	// Delete receiver resource structs.  These will assert if any resources are still initialized.
	for ( INT Index = 0 ; Index < ReceiverResources.Num() ; ++Index )
	{
		FReceiverResource* Resource = ReceiverResources(Index);
		delete Resource;
	}
}

/**
 * Initializes resources.
 * This is only called by the game thread, when a receiver is attached to a decal.
 */
void FDecalRenderData::InitResources_GameThread()
{
	check(IsInGameThread());
	if( NumTriangles > 0 )
	{
		if ( bUsesVertexResources )
		{
			BeginInitResource( &DecalVertexBuffer );
			BeginUpdateResourceRHI( &DecalVertexBuffer );

			// Initialize the decal's vertex factory.
			TSetResourceDataContext<FLocalVertexFactory> VertexFactoryData(&VertexFactory);
			VertexFactoryData->PositionComponent			= STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, Position, VET_Float3 );
			VertexFactoryData->TangentBasisComponents[0]	= STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, TangentX, VET_PackedNormal );
			VertexFactoryData->TangentBasisComponents[1]	= STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, TangentZ, VET_PackedNormal );

			// Texture coordinates.
			VertexFactoryData->TextureCoordinates.Empty();
			VertexFactoryData->TextureCoordinates.AddItem( STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, UV, VET_Float2 ) );

			// @todo DB: At the moment, FLocalVertexFactory asserts on non VET_Float2 texture coordinate channels,
			// @todo DB: so the normal transform information has to be split across two texture coordinates.

			VertexFactoryData->TextureCoordinates.AddItem( STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, NormalTransform0, VET_Float2 ) );
			VertexFactoryData->TextureCoordinates.AddItem( STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, NormalTransform1, VET_Float2 ) );

			VertexFactoryData->ShadowMapCoordinateComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( &DecalVertexBuffer, FDecalVertex, LightMapCoordinate, VET_Float2 );

			VertexFactoryData.Commit();
			BeginInitResource( &VertexFactory );
		}
		if ( bUsesIndexResources )
		{
			BeginInitResource( &IndexBuffer );
		}
	}
}

/**
 * Prepares resources for deletion.
 * This is only called by the rendering thread.
 */
void FDecalRenderData::ReleaseResources_RenderingThread()
{
	check(IsInRenderingThread());
	if( NumTriangles > 0 )
	{
		if ( bUsesVertexResources )
		{
			DecalVertexBuffer.Release();
			VertexFactory.Release();
		}
		if ( bUsesIndexResources )
		{
			IndexBuffer.Release();
		}
	}
	// Release any receiver-specific resources.
	for ( INT Index = 0 ; Index < ReceiverResources.Num() ; ++Index )
	{
		ReceiverResources(Index)->Release_RenderingThread();
	}
}

// Legacy serialization
namespace
{
	template<typename Index>
	class FDecalIndexBuffer
	{
	public:
		typedef Index				IndexType;
		typedef TArray<IndexType>	IndexArray;

		/**
		* Updates resources.
		*/
		void Update()
		{
		}

		// FIndexBuffer interface.
		/**
		* Copies the data stored in this buffer to the specified output buffer.
		*
		* @param	Buffer	Output buffer.
		*/
		virtual void GetData(void* Buffer)
		{
			appMemcpy( Buffer, Indices.GetData(), Indices.Num() * sizeof(IndexType) );
		}

		/**
		* Returns the number of indices currently stored in this buffer.
		*/
		FORCEINLINE INT GetNumIndices() const
		{
			return Indices.Num();
		}

		typedef TIndexedContainerIterator< IndexArray >			TIndexIterator;
		typedef TIndexedContainerConstIterator< IndexArray >	TIndexConstIterator;

		TIndexIterator			IndexItor()				{ return TIndexIterator( Indices ); }
		TIndexConstIterator		IndexConstItor() const	{ return TIndexConstIterator( Indices ); }

		IndexArray	Indices;
	};

	/**
	* FDecalIndexBuffer serializer.
	*/
	template<typename Index>
	FArchive& operator<<(FArchive& Ar, FDecalIndexBuffer<Index>& DecalIndexBuffer)
	{
		DecalIndexBuffer.Indices.BulkSerialize( Ar );
		if(Ar.Ver() < VER_RENDERING_REFACTOR)
		{
			UINT LegacySize = 0;
			Ar << LegacySize;
		}
		return Ar;
	}
}

namespace
{
struct FOldDecalVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD
{
	FVector			Position;
	FPackedNormal	TangentX;
	FPackedNormal	TangentY;
	FPackedNormal	TangentZ;

	/** Decal mesh texture coordinates. */
	FVector2D		UV;

	/** Transforms receiver tangent basis into decal tangent basis for normal map lookup. */
	FVector2D		NormalTransform0;
	FVector2D		NormalTransform1;
};
FArchive& operator<<(FArchive& Ar, FOldDecalVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD& DecalVertex)
{
	Ar << DecalVertex.Position;
	Ar << DecalVertex.TangentX << DecalVertex.TangentY << DecalVertex.TangentZ;
	Ar << DecalVertex.UV;
	Ar << DecalVertex.NormalTransform0;
	Ar << DecalVertex.NormalTransform1;
	return Ar;
}
}

/**
 * FDecalRenderData serializer.
 */
FArchive& operator<<(FArchive& Ar, FDecalRenderData& DecalRenderData)
{
	if ( Ar.Ver() < VER_DECAL_STATIC_DECALS_SERIALIZED )
	{
		// Eat vertices.
		typedef FOldDecalVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD	VertexType;
		typedef TArray<VertexType>											VertexArray;
		VertexArray Vertices;
		Vertices.BulkSerialize( Ar );

		// Eat intex buffer from old renderer.
		typedef WORD						IndexType;
		typedef FDecalIndexBuffer<IndexType>IndexBufferType;
		IndexBufferType						DecalIndexBuffer;
		Ar << DecalIndexBuffer;

		if ( Ar.Ver() >= VER_RENDERING_REFACTOR )
		{
			FRawIndexBuffer IndexBuffer;
			Ar << IndexBuffer;
			UINT NumTriangles = 0;
			Ar << NumTriangles;
		}
		if ( Ar.Ver() == VER_DECAL_RENDERDATA )
		{
			UObject* LightMap = NULL;
			Ar << LightMap;
		}
	}

	return Ar;
}

namespace
{
static FORCEINLINE void CopyOldVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD(FDecalVertex& Dst, const FOldDecalVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD& Src)
{
	Dst.Position = Src.Position;
	Dst.TangentX = Src.TangentX;
	Dst.TangentZ = Src.TangentZ;

	// store the sign of the determinant in TangentZ.W
	Dst.TangentZ.Vector.W = GetBasisDeterminantSignByte( Src.TangentX, Src.TangentY, Src.TangentZ );

	Dst.UV = Src.UV;
	Dst.LightMapCoordinate = FVector2D( 0.f, 0.f );
	Dst.NormalTransform0 = Src.NormalTransform0;
	Dst.NormalTransform1 = Src.NormalTransform1;
}
}
/**
 * FStaticReceiverData serializer.
 */
FArchive& operator<<(FArchive& Ar, FStaticReceiverData& Tgt)
{
	if ( Ar.Ver() < VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD )
	{
		Ar << Tgt.Component;

		check( Ar.IsLoading() );

		// Load the old vertex data.
		typedef FOldDecalVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD	VertexType;
		typedef TArray<VertexType>											VertexArray;

		VertexArray OldVertices;
		OldVertices.BulkSerialize(Ar);

		// Copy over the old vertex data into the new format.
		Tgt.Vertices.Empty( OldVertices.Num() );
		Tgt.Vertices.Add( OldVertices.Num() );
		for ( INT Index = 0 ; Index < OldVertices.Num() ; ++Index )
		{
			CopyOldVertex_VER_DECAL_ADDED_DECAL_VERTEX_LIGHTMAP_COORD( Tgt.Vertices(Index), OldVertices(Index) );
		}

		Tgt.Indices.BulkSerialize(Ar);
		Ar << Tgt.NumTriangles;

		// Clear any existing lightmap reference.
		Tgt.LightMap1D = NULL;
	}
	else
	{
		Ar << Tgt.Component;
		Tgt.Vertices.BulkSerialize(Ar);
		Tgt.Indices.BulkSerialize(Ar);
		Ar << Tgt.NumTriangles;
		Ar << Tgt.LightMap1D;
	}

	return Ar;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// FDecalMaterialInstance 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

struct FDecalMaterialInstance : FMaterialRenderProxy
{
public:
	FMaterialRenderProxy* ParentInstance;

	FDecalMaterialInstance(FMaterialRenderProxy* InParentInstance, FLOAT InDepthBias, FLOAT InSlopeScaleDepthBias)
		:	ParentInstance( InParentInstance )
	{
#if GEMINI_TODO
		Material = ParentInstance->Material;
		DepthBias = InDepthBias;
		SlopeScaleDepthBias = InSlopeScaleDepthBias;

		// @todo DB: Look into why disabling depth writing interferes with lighting.
		bDisableDepthWrite = FALSE;
#endif
	}

	// FMaterialRenderProxy interface.  Simply forward calls on to the parent
	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		return ParentInstance->GetVectorValue( ParameterName, OutValue, Context );
	}
	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const
	{
		return ParentInstance->GetScalarValue( ParameterName, OutValue, Context );
	}
	virtual UBOOL GetTextureValue(const FName& ParameterName, const FTexture** OutValue) const
	{
		return ParentInstance->GetTextureValue( ParameterName, OutValue );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FDecalMaterial
//
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

struct FDecalMaterial : public FMaterialResource
{
public:
	FDecalMaterial(UMaterial* InMaterial)
		:	FMaterialResource(InMaterial)
	{}

	virtual UBOOL IsDecalMaterial() const
	{
		return TRUE;
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled.
	 *
	 * @param	Platform			The platform currently being compiled for.
	 * @param	ShaderType			Which shader is being compiled.
	 * @param	VertexFactory		Which vertex factory is being compiled (can be NULL).
	 *
	 * @return						TRUE if the shader should be compiled.
	 */
	virtual UBOOL ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
	{
		// Don't compile decal materials for HitProxy shaders.

#if 0
		// AJS: I disabled this to solve a crash when a decal material is applied directly to a mesh.
		// Is that the right fix?
		if ( appStristr(ShaderType->GetName(), TEXT("HitProxy")) )
		{
			return FALSE;
		}
#endif
	
		return TRUE;
	}

	INT CompileProperty(EMaterialProperty Property,FMaterialCompiler* Compiler) const
	{
		INT SelectionColorIndex = Compiler->ComponentMask(Compiler->VectorParameter(NAME_SelectionColor,FLinearColor::Black),1,1,1,0);

		switch(Property)
		{
		case MP_EmissiveColor: return Compiler->Add(Compiler->ForceCast(Material->EmissiveColor.Compile(Compiler,FColor(0,0,0)),MCT_Float3),SelectionColorIndex);
		case MP_Opacity: return Material->Opacity.Compile(Compiler,1.0f);
		case MP_OpacityMask: return Material->OpacityMask.Compile(Compiler,1.0f);
		case MP_Distortion: return Material->Distortion.Compile(Compiler,FVector2D(0,0));
		case MP_TwoSidedLightingMask: return Compiler->Mul(Compiler->ForceCast(Material->TwoSidedLightingMask.Compile(Compiler,0.0f),MCT_Float),Material->TwoSidedLightingColor.Compile(Compiler,FColor(255,255,255)));
		case MP_DiffuseColor: return Compiler->Mul(Compiler->ForceCast(Material->DiffuseColor.Compile(Compiler,FColor(128,128,128)),MCT_Float3),Compiler->Sub(Compiler->Constant(1.0f),SelectionColorIndex));
		case MP_SpecularColor: return Material->SpecularColor.Compile(Compiler,FColor(128,128,128));
		case MP_SpecularPower: return Material->SpecularPower.Compile(Compiler,15.0f);
		case MP_Normal:
			{
				const INT Normal = Material->Normal.Compile(Compiler,FVector(0,0,1));
				const INT FinalX = Compiler->Dot( Compiler->TextureCoordinate(1), Compiler->ComponentMask(Normal,1,1,0,0) );
				const INT FinalY = Compiler->Dot( Compiler->TextureCoordinate(2), Compiler->ComponentMask(Normal,1,1,0,0) );
				const INT FinalZ = Compiler->ComponentMask( Normal, 0,0,1,0 );
				return Compiler->AppendVector(
					Compiler->AppendVector(
					FinalX,
					FinalY
					),
					FinalZ
					);
			}
		case MP_CustomLighting: return Material->CustomLighting.Compile(Compiler,FColor(0,0,0));
		default:
			return INDEX_NONE;
		};
	}
};

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UDecalMaterial
//
///////////////////////////////////////////////////////////////////////////////////////////////////

FMaterialResource* UDecalMaterial::AllocateResource()
{
	return new FDecalMaterial(this);
}

void UDecalMaterial::PostEditChange(UProperty* PropertyThatChanged)
{
	// Make sure bUsedWithSkeletalMesh is set to TRUE for all decal materials.
	bUsedWithSkeletalMesh = TRUE;
	Super::PostEditChange(PropertyThatChanged);
}

void UDecalMaterial::PreSave()
{
	Super::PreSave();
	bUsedWithSkeletalMesh = TRUE;
}

void UDecalMaterial::PostLoad()
{
	bUsedWithSkeletalMesh = TRUE;
	Super::PostLoad();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FDecalPoly
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static FORCEINLINE void Copy(FDecalPoly* Dst, const FDecalPoly* Src, INT Index)
{
	new(Dst->Vertices) FVector( Src->Vertices(Index) );
	new(Dst->ShadowTexCoords) FVector2D( Src->ShadowTexCoords(Index) );
	Dst->Indices.AddItem( Src->Indices(Index) );
}


// Split a poly and keep only the front half.
// @return		Number of vertices, 0 if clipped away.
INT FDecalPoly::Split( const FVector &Normal, const FVector &Base )
{
	// Split it.
	static FDecalPoly Front;
	Front.Init();
	switch( SplitWithPlaneFast( FPlane(Base,Normal), &Front ))
	{
	case SP_Back:
		return 0;
	case SP_Split:
		*this = Front;
		return Vertices.Num();
	default:
		return Vertices.Num();
	}
}

/**
 * Split with plane quickly for in-game geometry operations.
 * Results are always valid. May return sliver polys.
 */
INT FDecalPoly::SplitWithPlaneFast(const FPlane& Plane, FDecalPoly* FrontPoly) const
{
	enum EPlaneClassification
	{
		V_FRONT=0,
		V_BACK=1
	};
	EPlaneClassification Status,PrevStatus;
	check(Vertices.Num());
	EPlaneClassification* VertStatus = (EPlaneClassification*) appAlloca( sizeof(EPlaneClassification) * Vertices.Num() );
	int Front=0,Back=0;

	EPlaneClassification* StatusPtr = &VertStatus[0];
	check(StatusPtr);
	for( int i=0; i<Vertices.Num(); i++ )
	{
		const FLOAT Dist = Plane.PlaneDot(Vertices(i));
		if( Dist >= 0.f )
		{
			*StatusPtr++ = V_FRONT;
			if( Dist > +THRESH_SPLIT_POLY_WITH_PLANE )
			{
				Front=1;
			}
		}
		else
		{
			*StatusPtr++ = V_BACK;
			if( Dist < -THRESH_SPLIT_POLY_WITH_PLANE )
			{
				Back=1;
			}
		}
	}
	ESplitType Result;
	if( !Front )
	{
		if( Back ) Result = SP_Back;
		else       Result = SP_Coplanar;
	}
	else if( !Back )
	{
		Result = SP_Front;
	}
	else
	{
		// Split.
		if( FrontPoly )
		{
			const FVector	*V		= &Vertices(0);
			const FVector	*W		= &Vertices(Vertices.Num()-1);
			const FVector2D *STCV	= &ShadowTexCoords(0);
			const FVector2D *STCW	= &ShadowTexCoords(ShadowTexCoords.Num()-1);
			PrevStatus				= VertStatus[Vertices.Num()-1];
			StatusPtr				= &VertStatus[0];

			for( int i=0; i<Vertices.Num(); i++ )
			{
				Status = *StatusPtr++;
				if( Status != PrevStatus )
				{
					// Crossing.
					const FVector& P1 = *W;
					const FVector& P2 = *V;
					const FVector P2MP1(P2-P1);
					const FLOAT t = ((Plane.W - (P1|Plane))/(P2MP1|Plane));
					const FVector Intersection = P1 + (P2MP1*t);

					// ShadowTexCoord
					const FVector2D& STC1 = *STCW;
					const FVector2D& STC2 = *STCV;
					const FVector2D STC2MSTC1(STC2-STC1);
					const FVector2D NewShadowTexCoord = STC1 + (STC2MSTC1*t);

					new(FrontPoly->Vertices) FVector(Intersection);
					new(FrontPoly->ShadowTexCoords) FVector2D(NewShadowTexCoord);
					FrontPoly->Indices.AddItem( Indices(i) );

					if( PrevStatus == V_BACK )
					{			
						Copy(FrontPoly, this, i);
					}
				}
				else if( Status==V_FRONT )
				{
					Copy(FrontPoly, this, i);
				}

				PrevStatus = Status;
				W          = V++;
				STCW       = STCV++;
			}
			FrontPoly->FaceNormal	= FaceNormal;
		}
		Result = SP_Split;
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FDecalState::TransformFrustumVerts
//
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Transforms the decal frustum vertices by the specified matrix.
 */
void FDecalState::TransformFrustumVerts(const FMatrix& FrustumVertexTransform)
{
	// Transform decal frustum verts by the specified matrix.
	for ( INT Index = 0 ; Index < 8 ; ++Index )
	{
		FrustumVerts[Index] = FrustumVertexTransform.TransformFVector( FrustumVerts[Index] );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FDecalState::QuadToClippedScreenSpaceAABB
//
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Computes an axis-aligned bounding box of the decal frustum vertices projected to the screen.
 *
 * @param		SceneView				Scene projection
 * @param		OutMin					[out] Min vertex of the screen AABB.
 * @param		OutMax					[out] Max vertex of the screen AABB.
 * @param		FrustumVertexTransform	A transform applied to the frustum verts before screen projection.
 * @return								FALSE if the AABB has zero area, TRUE otherwise.
 */
UBOOL FDecalState::QuadToClippedScreenSpaceAABB(const FSceneView* SceneView, FVector2D& OutMin, FVector2D& OutMax, const FMatrix& FrustumVertexTransform) const
{
	static UBOOL GEnableScissorTestForDecals = TRUE;
	static UBOOL GShowFullyVisible           = TRUE;
	static UBOOL GShowPartiallyVisible       = TRUE;

	if ( !GEnableScissorTestForDecals )
	{
		OutMin = FVector2D(0.f, 0.f);
		OutMax = FVector2D(SceneView->SizeX, SceneView->SizeY);

		return TRUE;
	}

	enum {
		Front        = 0x01,
		Back         = 0x02,
		FrontAndBack = Front | Back,
	};

	FVector4 ScreenPoints[8];
	INT Sides[8];
	INT TotalSide = 0;

	// Transform decal frustum verts to screen space and classify sidedness w.r.t the near clipping plane.
	for ( INT Index = 0 ; Index < 8 ; ++Index )
	{
		ScreenPoints[Index] = SceneView->WorldToScreen( FrustumVertexTransform.TransformFVector(FrustumVerts[Index]) );
		Sides[Index] = (ScreenPoints[Index].W > SceneView->NearClippingDistance) ? Front : Back;
		TotalSide |= Sides[Index];
	}

	// Return if all verts project behind the near clipping plane.
	if ( TotalSide == Back )
	{
		return FALSE;
	}

	FVector2D MinCorner(FLT_MAX, FLT_MAX);
	FVector2D MaxCorner(-FLT_MAX, -FLT_MAX);

	const FLOAT HalfWidth  = SceneView->SizeX * 0.5f;
	const FLOAT HalfHeight = SceneView->SizeY * 0.5f;

	const FLOAT X = SceneView->X + HalfWidth;
	const FLOAT Y = SceneView->Y + HalfHeight;

	// If all verts project in front of the near clipping plane . . .
	if ( GShowFullyVisible && (TotalSide == Front) )
	{
		FVector2D NewMin(FLT_MAX, FLT_MAX);
		FVector2D NewMax(-FLT_MAX, -FLT_MAX);
		for ( INT Index = 0 ; Index < 8 ; ++Index )
		{
			const FLOAT InvW = 1.0f / ScreenPoints[Index].W;
			const FVector2D Pixel(
			X + ScreenPoints[Index].X * InvW * HalfWidth,
			Y - ScreenPoints[Index].Y * InvW * HalfHeight );

			MinCorner.X = Min(Pixel.X, MinCorner.X);
			MinCorner.Y = Min(Pixel.Y, MinCorner.Y);
			MaxCorner.X = Max(Pixel.X, MaxCorner.X);
			MaxCorner.Y = Max(Pixel.Y, MaxCorner.Y);
		}

		OutMin.X = Clamp(MinCorner.X, 0.f, SceneView->SizeX);
		OutMin.Y = Clamp(MinCorner.Y, 0.f, SceneView->SizeY);

		OutMax.X = Clamp(MaxCorner.X, 0.f, SceneView->SizeX);
		OutMax.Y = Clamp(MaxCorner.Y, 0.f, SceneView->SizeY);

		return (OutMin.X < OutMax.X) && (OutMin.Y < OutMax.Y);
	}

	// If some verts verts project in front and some project in back . . .
	if ( GShowPartiallyVisible && (TotalSide == FrontAndBack) )
	{
		for ( INT Index = 0 ; Index < 4 ; ++Index )
		{
			const INT NextIndex = (Index + 1) % 4;

			if ( Sides[Index] == Front )
			{
				const FLOAT InvW = 1.0f / ScreenPoints[Index].W;
				const FVector2D Pixel(
				X + ScreenPoints[Index].X * InvW * HalfWidth,
				Y - ScreenPoints[Index].Y * InvW * HalfHeight );

				MinCorner.X = Min(Pixel.X, MinCorner.X);
				MinCorner.Y = Min(Pixel.Y, MinCorner.Y);
				MaxCorner.X = Max(Pixel.X, MaxCorner.X);
				MaxCorner.Y = Max(Pixel.Y, MaxCorner.Y);
			}

			if ( (Sides[Index] | Sides[NextIndex]) == FrontAndBack )
			{
				const FLOAT T = (SceneView->NearClippingDistance - ScreenPoints[Index].W) / (ScreenPoints[NextIndex].W - ScreenPoints[Index].W);
				const FVector2D ClipPoint(
				ScreenPoints[Index].X * (1 - T) + ScreenPoints[NextIndex].X * T,
				ScreenPoints[Index].Y * (1 - T) + ScreenPoints[NextIndex].Y * T );

				const FLOAT InvW = 1.0f / SceneView->NearClippingDistance;
				const FVector2D Pixel(
				X + ClipPoint.X * InvW * HalfWidth,
				Y - ClipPoint.Y * InvW * HalfHeight );

				MinCorner.X = Min(Pixel.X, MinCorner.X);
				MinCorner.Y = Min(Pixel.Y, MinCorner.Y);
				MaxCorner.X = Max(Pixel.X, MaxCorner.X);
				MaxCorner.Y = Max(Pixel.Y, MaxCorner.Y);
			}
		}

		for ( INT Index = 4 ; Index < 8 ; ++Index )
		{
			const INT NextIndex = (Index<7) ? (Index + 1) : 4;

			if ( Sides[Index] == Front )
			{
				const FLOAT InvW = 1.0f / ScreenPoints[Index].W;
				const FVector2D Pixel(
				X + ScreenPoints[Index].X * InvW * HalfWidth,
				Y - ScreenPoints[Index].Y * InvW * HalfHeight );

				MinCorner.X = Min(Pixel.X, MinCorner.X);
				MinCorner.Y = Min(Pixel.Y, MinCorner.Y);
				MaxCorner.X = Max(Pixel.X, MaxCorner.X);
				MaxCorner.Y = Max(Pixel.Y, MaxCorner.Y);
			}

			if ( (Sides[Index] | Sides[NextIndex]) == FrontAndBack )
			{
				const FLOAT T = (SceneView->NearClippingDistance - ScreenPoints[Index].W) / (ScreenPoints[NextIndex].W - ScreenPoints[Index].W);
				const FVector2D ClipPoint(
				ScreenPoints[Index].X * (1 - T) + ScreenPoints[NextIndex].X * T,
				ScreenPoints[Index].Y * (1 - T) + ScreenPoints[NextIndex].Y * T );

				const FLOAT InvW = 1.0f / SceneView->NearClippingDistance;
				const FVector2D Pixel(
				X + ClipPoint.X * InvW * HalfWidth,
				Y - ClipPoint.Y * InvW * HalfHeight );

				MinCorner.X = Min(Pixel.X, MinCorner.X);
				MinCorner.Y = Min(Pixel.Y, MinCorner.Y);
				MaxCorner.X = Max(Pixel.X, MaxCorner.X);
				MaxCorner.Y = Max(Pixel.Y, MaxCorner.Y);
			}
		}

		OutMin.X = Clamp( MinCorner.X, 0.f, SceneView->SizeX );
		OutMin.Y = Clamp( MinCorner.Y, 0.f, SceneView->SizeY );

		OutMax.X = Clamp( MaxCorner.X, 0.f, SceneView->SizeX );
		OutMax.Y = Clamp( MaxCorner.Y, 0.f, SceneView->SizeY );

		return (OutMin.X < OutMax.X) && (OutMin.Y < OutMax.Y);
	}

	return FALSE;
}

