/*=============================================================================
	UnTerrain.h: New terrain
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#ifndef UNTERRAIN_H
#define UNTERRAIN_H

#define TERRAIN_MAXTESSELATION				16
#define TERRAIN_CACHED_MATERIALS			128
#define TERRAIN_CACHED_MATERIAL_INCREMENT	8

#define TERRAIN_ZSCALE				(1.0f/128.0f)

#define TERRAIN_UV_XY	2
#define TERRAIN_UV_XZ	3
#define TERRAIN_UV_YZ	4

//#define _USE_LOCAL_TO_WORLD_

#include "UnTerrainBVTree.h"

//
//	FTerrainPatchBounds
//

struct FTerrainPatchBounds
{
	FLOAT	MinHeight,
			MaxHeight,
			MaxDisplacement;

	friend FArchive& operator<<(FArchive& Ar,FTerrainPatchBounds& P)
	{
		return Ar << P.MinHeight << P.MaxHeight << P.MaxDisplacement;
	}
};

//
//	FTerrainMaterialMask - Contains a bitmask for selecting weighted materials on terrain.
//
struct FTerrainMaterialMask
{
protected:
	QWORD	BitMask;
	INT		NumBits;

public:

	// Constructor/destructor.
	FTerrainMaterialMask( DWORD InNumBits )
	:	BitMask(0)
	,	NumBits(InNumBits)
	{}

	// Accessors.
	UBOOL Get(UINT Index) const 
	{ 
		return BitMask & (((QWORD)1) << (Index & 63)); 
	}
	void Set(UINT Index,UBOOL Value)
	{ 
		if( Value ) 
		{
			BitMask |= ((QWORD)1) << (Index & 63); 
		}
		else
		{
			BitMask &= ~(((QWORD)1) << (Index & 63)); 
		}
	}
	INT Num() const
	{
		return NumBits;
	}
	// operator==
	UBOOL operator==(const FTerrainMaterialMask& OtherMask) const
	{
		return (NumBits == OtherMask.NumBits) && (BitMask == OtherMask.BitMask);
	}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar,FTerrainMaterialMask& M)
	{
		Ar << M.NumBits;
		if( Ar.Ver() < VER_HARDCODED_TERRAIN_MATERIAL_MASK_SIZE )
		{
			M.BitMask = 0;
			for( INT i=0; i<((M.NumBits + 31) / 32); i++ )
			{
				INT OldBits;
				Ar << OldBits;
				M.BitMask |= ((QWORD)OldBits) << (i*32);
			}
		}
		else
		{
			Ar << M.BitMask;
		}
		return Ar;
	}
};

//
//	UTerrainComponent
//
class UTerrainComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UTerrainComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	TArray<UShadowMap2D*>	ShadowMaps;
	TArray<FGuid>			IrrelevantLights;

	struct FTerrainObject*	TerrainObject;

	INT						SectionBaseX,
							SectionBaseY,
							SectionSizeX,
							SectionSizeY;
	/** The actual section size in vertices...										*/
	INT						TrueSectionSizeX;
	INT						TrueSectionSizeY;

	FLightMapRef LightMap;

	TArray<FTerrainPatchBounds>		PatchBounds;
	TArray<INT>						PatchBatches;	// Indices into the batches array.
	TArray<FTerrainMaterialMask>	BatchMaterials;	// A list of terrain material masks used by patches in this component.
	INT								FullBatch;		// A batch index containing all materials used by patches in this component.

	UINT*							PatchBatchOffsets;				// Offsets of patch batches into the index buffer
	UINT*							WorkingOffsets;
	INT*							PatchBatchTriangles;			// Number of triangles in each patch batch
	INT*							PatchCachedTessellationValues;	// Cached tessellation values for each patch
	BYTE*							TesselationLevels;

	/** Used for in-game collision tests against terrain. */
	FTerrainBVTree BVTree;

	/**
	 * This is a low poly version of the terrain vertices in local space. The
	 * triangle data is created based upon Terrain->CollisionTesselationLevel
	 */
	TArray<FVector> CollisionVertices;

	/** Physics engine version of heightfield data. */
	void* RBHeightfield;

	/**
	 *	Indicates the the terrain collision level should be rendered.
	 */
	UBOOL			bDisplayCollisionLevel;

	/**
	 * Builds the collision data for this terrain
	 */
	void BuildCollisionData(void);

	/**
	 * @return Whether or not the collision data for this component is dirty.
	 */
	UBOOL IsCollisionDataDirty() const
	{
		// @todo: Replace this with a proper flag, this is only a stub function right now.

		return TRUE;
	}

	// UObject interface.

	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize( FArchive& Ar );
	virtual void PostLoad();
	virtual void FinishDestroy();

	/**
	 * Rebuilds the collision data for saving
	 */
	virtual void PreSave(void);

	// UPrimitiveComponent interface.

	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags);
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);
	virtual void UpdateBounds();

	virtual void InitComponentRBPhys(UBOOL bFixed);

	friend struct FTerrainObject;
	friend class FTerrainComponentSceneProxy;

	// UActorComponent interface.
protected:
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();

	/**
	 * Returns context specific material.
	 *
	 * @param	Material	material to apply context sensitive changes
	 * @param	Context		relevant context (used to retrieve show flags)
	 * @return	material adapted depending on context like selection or show flags
	 */
	FMaterialRenderProxy* GetContextSpecificMaterial( FMaterialRenderProxy* Material, const FSceneView* View );

public:

	virtual void InvalidateLightingCache();

	// UPrimitiveComponent interface.
	virtual FDecalRenderData* GenerateDecalRenderData(class FDecalState* Decal) const;
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options);
	virtual void GetStaticTriangles(FPrimitiveTriangleDefinitionInterface* PTDI) const;

	// Init

	void Init(INT InBaseX,INT InBaseY,INT InSizeX,INT InSizeY,INT InTrueSizeX,INT InTrueSizeY);

	// UpdatePatchBounds

	void UpdatePatchBounds();

	/** builds/updates a list of unique blended material combinations used by quads in this terrain section and puts them in the PatchBatches array.
	 * Also updates FullBatch with the index of the mask for the full set.
	 */
	void UpdatePatchBatches();

	// GetTerrain

	class ATerrain* GetTerrain() const { return CastChecked<ATerrain>(GetOuter()); }

	// GetLocalVertex - Returns a vertex in the component's local space.

	FVector GetLocalVertex(INT X,INT Y) const;

	// GetWorldVertex - Returns a vertex in the component's local space.

	FVector GetWorldVertex(INT X,INT Y) const;

	// 
	void RenderPatches(const FSceneView* View,FPrimitiveDrawInterface* PDI);

	/**
	 * Gets the terrain collision data needed to pass to Novodex or to the
	 * kDOP code. Note: this code generates vertices/indices based on the
	 * Terrain->CollisionTessellationLevel
	 *
	 * @param Vertices the out array that gets each vert in the terrain
	 * @param Indices the out array that holds the generated indices
	 * @param ScaleFactor the scaling to apply to generated verts
	 */
	void GetCollisionData(TArray<FVector>& Vertices,TArray<INT>& Indices,
		const FVector& ScaleFactor = FVector(1.f,1.f,1.f));

	virtual FPrimitiveSceneProxy* CreateSceneProxy();
};

//
//	FNoiseParameter
//

struct FNoiseParameter
{
	FLOAT	Base,
			NoiseScale,
			NoiseAmount;

	// Constructors.

	FNoiseParameter() {}
	FNoiseParameter(FLOAT InBase,FLOAT InScale,FLOAT InAmount):
		Base(InBase),
		NoiseScale(InScale),
		NoiseAmount(InAmount)
	{}

	// Sample
	FLOAT Sample(INT X,INT Y) const;

	// TestGreater - Returns 1 if TestValue is greater than the parameter.
	UBOOL TestGreater(INT X,INT Y,FLOAT TestValue) const;

	// TestLess
	UBOOL TestLess(INT X,INT Y,FLOAT TestValue) const { return !TestGreater(X,Y,TestValue); }

	UBOOL operator==(const FNoiseParameter& SrcNoise)
	{
		if ((Base == SrcNoise.Base) &&
			(NoiseScale == SrcNoise.NoiseScale) &&
			(NoiseAmount == SrcNoise.NoiseAmount))
		{
			return TRUE;
		}

		return FALSE;
	}

	void operator=(const FNoiseParameter& SrcNoise)
	{
		Base = SrcNoise.Base;
		NoiseScale = SrcNoise.NoiseScale;
		NoiseAmount = SrcNoise.NoiseAmount;
	}
};

//
//	FTerrainHeight - Needs to be a struct to mirror the UnrealScript Terrain definition properly.
//

struct FTerrainHeight
{
	WORD	Value;

	// Constructor.

	FTerrainHeight() {}
	FTerrainHeight(WORD InValue): Value(InValue) {}

	// Serializer.

	friend FArchive& operator<<(FArchive& Ar,FTerrainHeight& H)
	{
		return Ar << H.Value;
	}
};

//
//	FTerrainInfoData - Needs to be a struct to mirror the UnrealScript Terrain definition properly.
//

struct FTerrainInfoData
{
	BYTE	Data;

	enum InfoFlags
	{
		/**	This flag indicates that the current terrain 'quad' is not visible...		*/
		TID_Visibility_Off		= 0x0001,
		/** This flag indicates that the 'quad' should have it's triangles 'flipped'	*/
		TID_OrientationFlip		= 0x0002,
		/** This flag indicates the patch is unreachable.								
			Used to tag areas the player(s) should not be able to reach but are sitll
			visible (tops of mountains, etc.).											*/
		TID_Unreachable			= 0x0004,
		/** This flag indicates the patch is locked.
		 *	The vertices can no longer be edited.
		 */
		TID_Locked				= 0x0008,
//		TID_	= 0x0010,
//		TID_	= 0x0020,
//		TID_	= 0x0040,
//		TID_	= 0x0080,
//		TID_	= 0x0100,
//		TID_	= 0x0200,
//		TID_	= 0x0400,
//		TID_	= 0x0800,
//		TID_	= 0x1000,
//		TID_	= 0x2000,
//		TID_	= 0x4000,
//		TID_	= 0x8000,
	};

	// Constructor.

	FTerrainInfoData() {}
	FTerrainInfoData(BYTE InData) : 
		Data(InData)
	{
	}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, FTerrainInfoData& H)
	{
		return Ar << H.Data;
	}

	// Getters
	inline UBOOL IsVisible() const
	{
		return ((Data & TID_Visibility_Off) == 0);
	}

	inline UBOOL IsOrientationFlipped() const
	{
		return ((Data & TID_OrientationFlip) != 0);
	}

	inline UBOOL IsUnreachable() const
	{
		return ((Data & TID_Unreachable) != 0);
	}

	inline UBOOL IsLocked() const
	{
		return ((Data & TID_Locked) != 0);
	}

	// Setters
	inline void SetIsVisible(UBOOL bVisibile)
	{
		if (bVisibile)
		{
			Data &= ~TID_Visibility_Off;
		}
		else
		{
			Data |= TID_Visibility_Off;
		}
	}

	inline void SetIsOrientationFlipped(UBOOL bOrientationFlipped)
	{
		if (bOrientationFlipped)
		{
			Data |= TID_OrientationFlip;
		}
		else
		{
			Data &= ~TID_OrientationFlip;
		}
	}

	inline void SetIsUnreachable(UBOOL bUnreachable)
	{
		if (bUnreachable)
		{
			Data |= TID_Unreachable;
		}
		else
		{
			Data &= ~TID_Unreachable;
		}
	}

	inline void SetIsLocked(UBOOL bLocked)
	{
		if (bLocked)
		{
			Data |= TID_Locked;
		}
		else
		{
			Data &= ~TID_Locked;
		}
	}
};

/**
 *	Helper function for terrain.
 *
 *	@param	Size	The size requested
 *	@return	INT		The size required by the platform
 */
static inline INT Terrain_GetPaddedSize(INT Size)
{
	if (GPlatformNeedsPowerOfTwoTextures) // power of two
	{
		return (1 << appCeilLogTwo(Size));
	}
	else
	{
		return Size;
	}
}

//
//	FTerrainWeightedMaterial
//

struct FTerrainWeightedMaterial 
{
	TArray<BYTE>			Data;

	INT						SizeX;
	INT						SizeY;

	class ATerrain*			Terrain;
	UBOOL					Highlighted;
	FColor					HighlightColor;
	UBOOL					bWireframeHighlighted;
	FColor					WireframeColor;

	class UTerrainMaterial*	Material;
	INT						WeightMapIndex;
	INT						WeightMapSubIndex;

	// Constructors.

	FTerrainWeightedMaterial();
	FTerrainWeightedMaterial(ATerrain* Terrain,const TArray<BYTE>& InData,class UTerrainMaterial* InMaterial,
		UBOOL InHighlighted, const FColor& InHighlightColor, UBOOL bInWireframeHighlighted, const FColor& InWireframeColor);
	virtual ~FTerrainWeightedMaterial();

	// Accessors.

	FORCEINLINE const BYTE& Weight(INT X,INT Y) const
	{
		check(X >= 0 && X < (INT)SizeX && Y >= 0 && Y < (INT)SizeY);
		return Data(Y * SizeX + X);
	}

	FORCEINLINE FLOAT FilteredWeight(INT IntX,FLOAT FracX,INT IntY,FLOAT FracY) const;


	friend FArchive& operator<<(FArchive& Ar,FTerrainWeightedMaterial& M);
};

//
//	FFilterLimit
//

struct FFilterLimit
{
	BITFIELD		Enabled : 1;
	FNoiseParameter	Noise;

	UBOOL TestLess(INT X,INT Y,FLOAT TestValue) const { return Enabled && Noise.TestLess(X,Y,TestValue); }
	UBOOL TestGreater(INT X,INT Y,FLOAT TestValue) const { return Enabled && Noise.TestGreater(X,Y,TestValue); }

	UBOOL operator==(const FFilterLimit& SrcFilter)
	{
		if ((Enabled == SrcFilter.Enabled) &&
			(Noise == SrcFilter.Noise))
		{
			return TRUE;
		}

		return FALSE;
	}

	void operator=(const FFilterLimit& SrcFilter)
	{
		Enabled = SrcFilter.Enabled;
		Noise = SrcFilter.Noise;
	}
};

//
//	FTerrainFilteredMaterial
//

struct FTerrainFilteredMaterial
{
	BITFIELD				UseNoise : 1;
	FLOAT					NoiseScale,
							NoisePercent;

	FFilterLimit			MinHeight;
	FFilterLimit			MaxHeight;

	FFilterLimit			MinSlope;
	FFilterLimit			MaxSlope;

	FLOAT					Alpha;
	class UTerrainMaterial*	Material;

	// BuildWeightMap - Filters a base weightmap based on the parameters and applies the weighted material to the terrain.  The base weightmap is also modified to remove the pixels used by this material.

	void BuildWeightMap(TArray<BYTE>& BaseWeightMap,
		UBOOL Highlighted, const FColor& InHighlightColor, UBOOL bInWireframeHighlighted, const FColor& InWireframeColor,
		class ATerrain* Terrain, class UTerrainLayerSetup* Layer, INT MinX, INT MinY, INT MaxX, INT MaxY) const;

	UBOOL operator==(const FTerrainFilteredMaterial& SrcMaterial)
	{
		if ((UseNoise == SrcMaterial.UseNoise) && 
			(NoiseScale == SrcMaterial.NoiseScale) && 
			(NoisePercent == SrcMaterial.NoisePercent) && 
			(MinHeight == SrcMaterial.MinHeight) && 
			(MaxHeight == SrcMaterial.MaxHeight) && 
			(MinSlope == SrcMaterial.MinSlope) && 
			(MaxSlope == SrcMaterial.MaxSlope) && 
			(Alpha == SrcMaterial.Alpha) && 
			(Material == SrcMaterial.Material))
		{
			return TRUE;
		}

		return FALSE;
	}

	void operator=(const FTerrainFilteredMaterial& SrcMaterial)
	{
		UseNoise = SrcMaterial.UseNoise;
		NoiseScale = SrcMaterial.NoiseScale;
		NoisePercent = SrcMaterial.NoisePercent;
		MinHeight = SrcMaterial.MinHeight;
		MaxHeight = SrcMaterial.MaxHeight;
		MinSlope = SrcMaterial.MinSlope;
		MaxSlope = SrcMaterial.MaxSlope;
		Alpha = SrcMaterial.Alpha;
		Material = SrcMaterial.Material;
	}
};

//
//	FTerrainLayer
//

struct FTerrainLayer
{
	FString						Name;
	class UTerrainLayerSetup*	Setup;
	INT							AlphaMapIndex;
	BITFIELD					Highlighted : 1;
	BITFIELD					WireframeHighlighted : 1;
	BITFIELD					Hidden : 1;
	BITFIELD					Locked : 1;
	FColor						HighlightColor;
	FColor						WireframeColor;
	/** rectangle encompassing all the vertices this layer affects. TerrainLayerSetup::SetMaterial() uses this to avoid rebuilding
	 *	terrain that has not changed
	 */
	INT MinX, MinY, MaxX, MaxY;
};

enum ETerrainLayerType
{
	TLT_Empty				= -1,
	TLT_HeightMap			= 0,
	TLT_Layer,
	TLT_ProceduralLayer,
	TLT_DecoLayer,
	TLT_TerrainMaterial,
	TLT_FilteredMaterial,
	TLT_Decoration,
	TLT_Foliage,
	TLT_DisplacementMap,
	TLT_MAX_COUNT
};

inline TCHAR*	GetTerrainLayerTypeString(ETerrainLayerType Type)
{
	switch (Type)
	{
	case TLT_Empty:				return TEXT("EMPTY");
	case TLT_HeightMap:			return TEXT("HeightMap");
	case TLT_Layer:				return TEXT("Layer");
	case TLT_DecoLayer:			return TEXT("DecoLayer");
	case TLT_TerrainMaterial:	return TEXT("TerrainMaterial");
	case TLT_FilteredMaterial:	return TEXT("FilteredMaterial");
	case TLT_Decoration:		return TEXT("Decoration");
	case TLT_Foliage:			return TEXT("Foliage");
	}

	return TEXT("INVALID");
}

union FTerrainLayerId
{
	void*			Id;
	struct
	{
		BITFIELD	Type : 2,
					Index : 30;
	};
};


//
//	FAlphaMap
//

struct FAlphaMap
{
	TArray<BYTE>	Data;

	friend FArchive& operator<<(FArchive& Ar,FAlphaMap& M)
	{
		return Ar << M.Data;
	}
};

//
//	FTerrainDecorationInstance
//

struct FTerrainDecorationInstance
{
	class UPrimitiveComponent*	Component;
	FLOAT						X,
								Y,
								Scale;
	INT							Yaw;

	UBOOL operator==(const FTerrainDecorationInstance& SrcInstance)
	{
		if (Component && SrcInstance.Component)
		{
			if (Component != SrcInstance.Component)
			{
				return FALSE;
			}
		}
		else
		if (Component || SrcInstance.Component)
		{
			return FALSE;
		}

		if ((X != SrcInstance.X) ||
			(Y != SrcInstance.Y) ||
			(Scale != SrcInstance.Scale) ||
			(Yaw != SrcInstance.Yaw))
		{
			return FALSE;
		}

		return TRUE;
	}

	UBOOL operator!=(const FTerrainDecorationInstance& SrcInstance)
	{
		if (Component && SrcInstance.Component)
		{
			if (Component != SrcInstance.Component)
			{
				return TRUE;
			}
		}
		else
		if (Component || SrcInstance.Component)
		{
			return TRUE;
		}

		if ((X != SrcInstance.X) ||
			(Y != SrcInstance.Y) ||
			(Scale != SrcInstance.Scale) ||
			(Yaw != SrcInstance.Yaw))
		{
			return TRUE;
		}

		return FALSE;
	}
};

//
//	FTerrainDecoration
//

struct FTerrainDecoration
{
	class UPrimitiveComponentFactory*	Factory;
	FLOAT								MinScale,
										MaxScale,
										Density,
										SlopeRotationBlend;
	INT									RandSeed;

	TArrayNoInit<FTerrainDecorationInstance>	Instances;

	UBOOL operator==(const FTerrainDecoration& SrcDecoration)
	{
		UStaticMeshComponentFactory* MeshFactory = Cast<UStaticMeshComponentFactory>(Factory);
		UStaticMeshComponentFactory* SrcMeshFactory = Cast<UStaticMeshComponentFactory>(SrcDecoration.Factory);

		if (MeshFactory && SrcMeshFactory)
		{
			if (MeshFactory->StaticMesh != SrcMeshFactory->StaticMesh)
			{
				return FALSE;
			}
		}
		else
		if (MeshFactory || SrcMeshFactory)
		{
			return FALSE;
		}

		if ((MinScale != SrcDecoration.MinScale) ||
			(MaxScale != SrcDecoration.MaxScale) ||
			(Density != SrcDecoration.Density) ||
			(SlopeRotationBlend != SrcDecoration.SlopeRotationBlend) ||
			(RandSeed != SrcDecoration.RandSeed))
		{
			return FALSE;
		}

		if (Instances.Num() != SrcDecoration.Instances.Num())
		{
			return FALSE;
		}

		for (INT Index = 0; Index < Instances.Num(); Index++)
		{
			if (Instances(Index) != SrcDecoration.Instances(Index))
			{
				return FALSE;
			}
		}

		return TRUE;
	}
};

//
//	FTerrainDecoLayer
//

struct FTerrainDecoLayer
{
	FStringNoInit						Name;
	TArrayNoInit<FTerrainDecoration>	Decorations;
	INT									AlphaMapIndex;

	void operator=(const FTerrainDecoLayer& InCopy)
	{
		if (InCopy.Name.Len() == 0)
		{
			Name.Empty();
		}
		else
		{
			Name = InCopy.Name;
		}

		AlphaMapIndex = InCopy.AlphaMapIndex;

	}

	UBOOL operator==(const FTerrainDecoLayer& Other)
	{
		if( Name != Other.Name )
			return FALSE;

		if( Decorations.Num() != Other.Decorations.Num() )
			return FALSE;

		for( INT i=0;i<Decorations.Num();i++ )
		{
			if( !(Decorations(i) == Other.Decorations(i)) )
				return FALSE;
		}

		return TRUE;
	}

};

//
//	QuadLerp - Linearly interpolates between four adjacent terrain vertices, taking into account triangulation.
//

template<class T> T QuadLerp(const T& P00,const T& P10,const T& P01,const T& P11,FLOAT U,FLOAT V)
{
	if(U > V)
	{
		if(V < 1.0f)
			return Lerp(Lerp(P00,P11,V),Lerp(P10,P11,V),(U - V) / (1.0f - V));
		else
			return P11;
	}
	else
	{
		if(V > 0.0f)
			return Lerp(Lerp(P00,P01,V),Lerp(P00,P11,V),U / V);
		else
			return P00;
	}
}

//
//	FTerrainPatch - A displaced bicubic fit to 4x4 terrain samples.
//

struct FTerrainPatch
{
	FLOAT	Heights[4][4];

	FTerrainPatch() {}
};

//
//	FPatchSampler - Used to sample a FTerrainPatch at a fixed frequency.
//

struct FPatchSampler
{
	FLOAT	CubicBasis[TERRAIN_MAXTESSELATION + 1][4];
	FLOAT	CubicBasisDeriv[TERRAIN_MAXTESSELATION + 1][4];
	UINT	MaxTesselation;

	// Constructor.

	FPatchSampler(UINT InMaxTesselation);

	// Sample - Samples a terrain patch's height at a given position.

	FLOAT Sample(const FTerrainPatch& Patch,UINT X,UINT Y) const;

	// SampleDerivX - Samples a terrain patch's dZ/dX at a given position.

	FLOAT SampleDerivX(const FTerrainPatch& Patch,UINT X,UINT Y) const;

	// SampleDerivY - Samples a terrain patch's dZ/dY at a given position.

	FLOAT SampleDerivY(const FTerrainPatch& Patch,UINT Y,UINT X) const;

private:

	FLOAT Cubic(FLOAT P0,FLOAT P1,FLOAT P2,FLOAT P3,UINT I) const;
	FLOAT CubicDeriv(FLOAT P0,FLOAT P1,FLOAT P2,FLOAT P3,UINT I) const;
};

//
//	FTerrainMaterialResource
//

class FTerrainMaterialResource: /***public UMaterial, ***//***private***/public FMaterial, public FMaterialRenderProxy
{
public:
//    DECLARE_CLASS(FTerrainMaterialResource,UMaterial,0,Engine)

	friend class ATerrain;

	// Constructors.

	FTerrainMaterialResource() : Mask(0) {}
	FTerrainMaterialResource(ATerrain* InTerrain,const FTerrainMaterialMask& InMask):
		Terrain(InTerrain),
		Mask(InMask)
	{}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return TRUE if the shader should be compiled
	 */
	virtual UBOOL ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const;

	// FMaterial interface.
	virtual INT CompileProperty(EMaterialProperty Property,FMaterialCompiler* Compiler) const;
	virtual UBOOL IsTwoSided() const { return FALSE; }
	virtual UBOOL IsLightFunction() const { return FALSE; }
	virtual UBOOL IsUsedWithFogVolumes() const { return FALSE; }
	virtual UBOOL IsSpecialEngineMaterial() const { return FALSE; }
	virtual UBOOL IsUsedWithStaticLighting() const { return TRUE; }
	virtual UBOOL IsTerrainMaterial() const { return TRUE; }
	virtual UBOOL IsDecalMaterial() const { return FALSE; }
	virtual UBOOL IsWireframe() const { return FALSE; }
	virtual UBOOL IsDistorted() const { return FALSE; }
	virtual UBOOL IsMasked() const { return FALSE; }
	virtual enum EBlendMode GetBlendMode() const { return BLEND_Opaque; }
	virtual enum EMaterialLightingModel GetLightingModel() const { return MLM_Phong; }
	virtual FLOAT GetOpacityMaskClipValue() const { return 0.0f; }
	virtual FString GetFriendlyName() const;
	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual UBOOL IsPersistent() const { return TRUE; }

	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial() const { return this; }
	virtual UBOOL GetVectorValue(const FName& ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const;
	virtual UBOOL GetScalarValue(const FName& ParameterName, FLOAT* OutValue, const FMaterialRenderContext& Context) const;
	virtual UBOOL GetTextureValue(const FName& ParameterName,const FTexture** OutValue) const;

	// Serializer
	friend FArchive& operator<<(FArchive& Ar,FTerrainMaterialResource& R);

	/** Called after deserialization of the material during loading. */
	void PostLoad();
	/** Called before the resource is saved.						 */
	void PreSave();

	// Accessors.
	const FTerrainMaterialMask& GetMask() const { return Mask; }

	TArray<UTexture2D*> WeightMaps;
	TMap<FName, UTexture2D*> WeightMapsMap;

private:
	
	ATerrain* Terrain;
	FTerrainMaterialMask Mask;
	/**
	 *	Array of the underlying material IDs.
	 *	This allows for catching materials that were changed.
	 */
	TArray<FGuid> MaterialIds;

	/** The materials which this terrain material depends on. */
	TArray<const FMaterialRenderProxy*> MaterialDependencies;

	/**
	 * Compiles a single property of a single external material for use by this terrain material.
	 */
	INT CompileTerrainMaterial(EMaterialProperty Property,FMaterialCompiler* Compiler,UTerrainMaterial* TerrainMaterial,UBOOL Highlighted, FColor& HighlightColor) const;

	/** Builds a list of materials the terrain material is dependent on, and registers it as a shader observer with each one. */
	void UpdateMaterialDependencies();
};

//
//	FMaterialInstancePointer
//
typedef FTerrainMaterialResource*	FTerrainMaterialResourcePointer;

/** Selected vertex structure - used for vertex editing		*/
struct FSelectedTerrainVertex
{
	/** The position of the vertex.					*/
	INT		X, Y;
	/** The weight of the selection.				*/
	FLOAT	Weight;
};

//
//	EngineTerrainClasses
//
#include "EngineTerrainClasses.h"

// Tessellation level helpers
// Used for caching the smooth index buffers for each component
//
// 3 2    22    11    11    00    0
// 1 9    43    87    21    65    0
// |-|----||----||----||----||----|
//     -Y    +Y    -X    +X   INT
// This allows for a max tessellation of 64 (2^6 = 64)
// (NOTE: The value should have 1 added to it - ie, 0 = 1, 63 = 64...)
//
// Mask values
#define TERRAIN_SMOOTHIB_MASK_VALUE				0x0000003F
#define TERRAIN_SMOOTHIB_MASK_INTERIOR			0x0000003F
#define TERRAIN_SMOOTHIB_MASK_POSX				0x00000FC0
#define TERRAIN_SMOOTHIB_MASK_NEGX				0x0003F000
#define TERRAIN_SMOOTHIB_MASK_POSY				0x00FC0000
#define TERRAIN_SMOOTHIB_MASK_NEGY				0x3F000000
//
// Shift values
#define TERRAIN_SMOOTHIB_SHIFT_INTERIOR			0
#define TERRAIN_SMOOTHIB_SHIFT_POSX				6
#define TERRAIN_SMOOTHIB_SHIFT_NEGX				12
#define TERRAIN_SMOOTHIB_SHIFT_POSY				18
#define TERRAIN_SMOOTHIB_SHIFT_NEGY				24
//
// Accessors
// Get
#define TERRAIN_SMOOTHIB_GET_INTERIOR(x)		(((x & TERRAIN_SMOOTHIB_MASK_INTERIOR) >> TERRAIN_SMOOTHIB_SHIFT_INTERIOR) + 1)
#define TERRAIN_SMOOTHIB_GET_POSX(x)			(((x & TERRAIN_SMOOTHIB_MASK_POSX    ) >> TERRAIN_SMOOTHIB_SHIFT_POSX    ) + 1)
#define TERRAIN_SMOOTHIB_GET_NEGX(x)			(((x & TERRAIN_SMOOTHIB_MASK_NEGX    ) >> TERRAIN_SMOOTHIB_SHIFT_NEGX    ) + 1)
#define TERRAIN_SMOOTHIB_GET_POSY(x)			(((x & TERRAIN_SMOOTHIB_MASK_POSY    ) >> TERRAIN_SMOOTHIB_SHIFT_POSY    ) + 1)
#define TERRAIN_SMOOTHIB_GET_NEGY(x)			(((x & TERRAIN_SMOOTHIB_MASK_NEGY    ) >> TERRAIN_SMOOTHIB_SHIFT_NEGY    ) + 1)
// Set - assumes x started at 0!
#define TERRAIN_SMOOTHIB_SET_INTERIOR(x,Val)	(x |= (((Val - 1) & TERRAIN_SMOOTHIB_MASK_VALUE) << TERRAIN_SMOOTHIB_SHIFT_INTERIOR))
#define TERRAIN_SMOOTHIB_SET_POSX(x,Val)		(x |= (((Val - 1) & TERRAIN_SMOOTHIB_MASK_VALUE) << TERRAIN_SMOOTHIB_SHIFT_POSX    ))
#define TERRAIN_SMOOTHIB_SET_NEGX(x,Val)		(x |= (((Val - 1) & TERRAIN_SMOOTHIB_MASK_VALUE) << TERRAIN_SMOOTHIB_SHIFT_NEGX    ))
#define TERRAIN_SMOOTHIB_SET_POSY(x,Val)		(x |= (((Val - 1) & TERRAIN_SMOOTHIB_MASK_VALUE) << TERRAIN_SMOOTHIB_SHIFT_POSY    ))
#define TERRAIN_SMOOTHIB_SET_NEGY(x,Val)		(x |= (((Val - 1) & TERRAIN_SMOOTHIB_MASK_VALUE) << TERRAIN_SMOOTHIB_SHIFT_NEGY    ))

struct FTerrainSmoothIndexBuffer;

#endif	//#ifndef UNTERRAIN_H
