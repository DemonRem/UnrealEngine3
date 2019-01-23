/*=============================================================================
LandscapeRender.h: New terrain rendering
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _LANDSCAPERENDER_H
#define _LANDSCAPERENDER_H

class FLandscapeVertexFactory;
class FLandscapeVertexBuffer;
class FLandscapeComponentSceneProxy;
#include "../Src/ScenePrivate.h"


#define MAX_LANDSCAPE_SUBSECTIONS 3

#if WITH_EDITOR
namespace ELandscapeViewMode
{
	enum Type
	{
		Invalid = -1,
		/** Color only */
		Normal = 0,
		EditLayer,
		/** Layer debug only */
		DebugLayer,
	};
}

extern ELandscapeViewMode::Type GLandscapeViewMode;
#endif

/** vertex factory for VTF-heightmap terrain  */
class FLandscapeVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FLandscapeVertexFactory);
	FLandscapeComponentSceneProxy* SceneProxy;
public:

	FLandscapeVertexFactory(FLandscapeComponentSceneProxy* InSceneProxy)
	:	SceneProxy(InSceneProxy)
	{}

	FLandscapeComponentSceneProxy* GetSceneProxy()
	{
		return SceneProxy;
	}

	struct DataType
	{
		/** The stream to read the vertex position from. */
		FVertexStreamComponent PositionComponent;
	};

	/**
	* Should we cache the material's shadertype on this platform with this vertex factory? 
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		// only compile landscape materials for landscape vertex factory
		// The special engine materials must be compiled for the landscape vertex factory because they are used with it for wireframe, etc.
		return (Platform==SP_PCD3D_SM3 || Platform==SP_PCD3D_SM4 || Platform==SP_PCD3D_SM5 || Platform==SP_XBOXD3D) && (Material->IsUsedWithLandscape() || Material->IsSpecialEngineMaterial()) && !Material->IsUsedWithDecals();
	}

	/**
	* Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	*/
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("VTF_TERRAIN"),TEXT("1"));
	}

	// FRenderResource interface.
	virtual void InitRHI();

	static UBOOL SupportsTessellationShaders() { return TRUE; }

	/** stream component data bound to this vertex factory */
	DataType Data;  
};

//
// FLandscapeVertexBuffer
//
class FLandscapeVertexBuffer : public FVertexBuffer, public FRefCountedObject
{
	INT SizeVerts;
public:
	struct FLandscapeVertex
	{
		FLOAT VertexX;
		FLOAT VertexY;
	};
	
	/** Constructor. */
	FLandscapeVertexBuffer(INT InSizeVerts)
	: 	SizeVerts(InSizeVerts)
	{
		InitResource();
	}

	/** Destructor. */
	virtual ~FLandscapeVertexBuffer()
	{
		ReleaseResource();
	}

	/** 
	* Initialize the RHI for this rendering resource 
	*/
	virtual void InitRHI();
};

//
// FLandscapeIndexBuffer
//
class FLandscapeIndexBuffer : public FRawStaticIndexBuffer, public FRefCountedObject
{
public:
	FLandscapeIndexBuffer(INT SizeQuads, INT VBSizeVertices);

	/** Destructor. */
	virtual ~FLandscapeIndexBuffer()
	{
		ReleaseResource();
	}
};

extern UMaterial* GLayerDebugColorMaterial;

//
// FLandscapeEditToolRenderData
//
struct FLandscapeEditToolRenderData
{
	FLandscapeEditToolRenderData()
	:	ToolMaterial(NULL),
		DebugColorLayerMaterial(NULL),
		LandscapeComponent(NULL)
	{}

	FLandscapeEditToolRenderData(ULandscapeComponent* InComponent)
		:	ToolMaterial(NULL),
			DebugColorLayerMaterial(NULL),
			LandscapeComponent(InComponent)
	{}

	// Material used to render the tool.
	UMaterialInterface* ToolMaterial;

	// Material used to render debug color layer.
	UMaterialInstanceConstant* DebugColorLayerMaterial;
	ULandscapeComponent* LandscapeComponent;

#ifdef WITH_EDITOR
	void UpdateDebugColorMaterial();
#endif

	// Game thread update
	void Update( UMaterialInterface* InNewToolMaterial )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateEditToolRenderData,
			FLandscapeEditToolRenderData*, LandscapeEditToolRenderData, this,
			UMaterialInterface*, NewToolMaterial, InNewToolMaterial,
		{
			LandscapeEditToolRenderData->ToolMaterial = NewToolMaterial;
		});
	}

	// Allows game thread to queue the deletion by the render thread
	void Cleanup()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			CleanupEditToolRenderData,
			FLandscapeEditToolRenderData*, LandscapeEditToolRenderData, this,
		{
			delete LandscapeEditToolRenderData;
		});
	}
};

//
// FLandscapeComponentSceneProxy
//
class FLandscapeComponentSceneProxy : public FPrimitiveSceneProxy
{
	class FLandscapeLCI : public FLightCacheInterface
	{
	public:
		/** Initialization constructor. */
		FLandscapeLCI(const ULandscapeComponent* InComponent)
		{
			Component = InComponent;
		}

		// FLightCacheInterface
		virtual FLightInteraction GetInteraction(const class FLightSceneInfo* LightSceneInfo) const
		{
			for (INT LightIndex = 0; LightIndex < Component->IrrelevantLights.Num(); LightIndex++)
			{
				if(Component->IrrelevantLights(LightIndex) == LightSceneInfo->LightGuid)
				{
					return FLightInteraction::Irrelevant();
				}
			}

			if(Component->LightMap && Component->LightMap->LightGuids.ContainsItem(LightSceneInfo->LightmapGuid))
			{
				return FLightInteraction::LightMap();
			}

			for(INT LightIndex = 0;LightIndex < Component->ShadowMaps.Num();LightIndex++)
			{
				const UShadowMap2D* const ShadowMap = Component->ShadowMaps(LightIndex);
				if(ShadowMap && ShadowMap->IsValid() && ShadowMap->GetLightGuid() == LightSceneInfo->LightGuid)
				{
					return FLightInteraction::ShadowMap2D(
						ShadowMap->GetTexture(),
						ShadowMap->GetCoordinateScale(),
						ShadowMap->GetCoordinateBias(),
						ShadowMap->IsShadowFactorTexture()
						);
				}
			}

			return FLightInteraction::Uncached();
		}

		virtual FLightMapInteraction GetLightMapInteraction() const
		{
			if (Component->LightMap)
			{
				return Component->LightMap->GetInteraction();
			}
			else
			{
				return FLightMapInteraction();
			}
		}

	private:
		/** A map from persistent light IDs to information about the light's interaction with the model element. */
		//TMap<FGuid,FLightInteraction> StaticLightInteractionMap;

		/** The light-map used by the element. */
		const ULandscapeComponent* Component;
	};

	INT						MaxLOD;
	INT						ComponentSizeQuads;	// Size of component in quads
	INT						NumSubsections;
	INT						SubsectionSizeQuads;
	INT						SubsectionSizeVerts;
	INT						SectionBaseX;
	INT						SectionBaseY;
	FLOAT					DrawScaleXY;
	FVector					ActorOrigin;
	INT						StaticLightingResolution;

	// values set during rendering
	INT						CurrentLOD;
	INT						CurrentSubX;
	INT						CurrentSubY;

	// Values for light-map
	INT						PatchExpandCount;

	FVector4 WeightmapScaleBias;
	FLOAT WeightmapSubsectionOffset;
	FVector2D LayerUVPan;

	UTexture2D* HeightmapTexture;
	FVector4 HeightmapScaleBias;
	FLOAT HeightmapSubsectionOffset;

	UTexture2D* XYOffsetTexture;
	FVector4 XYOffsetUVScaleBias;
	FLOAT XYOffsetSubsectionOffset;
	FLOAT XYOffsetScale;

	FVector4 LightmapScaleBias;

	FLandscapeVertexFactory* VertexFactory;
	FLandscapeVertexBuffer* VertexBuffer;
	FLandscapeIndexBuffer** IndexBuffers;
	
	UMaterialInterface* MaterialInterface;
	FMaterialViewRelevance MaterialViewRelevance;

	// Reference counted vertex buffer shared among all landscape scene proxies
	static FLandscapeVertexBuffer* SharedVertexBuffer;
	static FLandscapeIndexBuffer** SharedIndexBuffers;

	FLandscapeEditToolRenderData* EditToolRenderData;

	// FLightCacheInterface
	FLandscapeLCI* ComponentLightInfo;

	virtual ~FLandscapeComponentSceneProxy();

public:

	FLandscapeComponentSceneProxy(ULandscapeComponent* InComponent, FLandscapeEditToolRenderData* InEditToolRenderData);

	// FPrimitiveSceneProxy interface.

	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	* @param	DPGIndex - current depth priority 
	* @param	Flags - optional set of flags from EDrawDynamicElementFlags
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags);

	/**
	*	Called when the rendering thread adds the proxy to the scene.
	*	This function allows for generating renderer-side resources.
	*/
	virtual UBOOL CreateRenderThreadResources();

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }

	FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);

	/**
	 *	Determines the relevance of this primitive's elements to the given light.
	 *	@param	LightSceneInfo			The light to determine relevance for
	 *	@param	bDynamic (output)		The light is dynamic for this primitive
	 *	@param	bRelevant (output)		The light is relevant for this primitive
	 *	@param	bLightMapped (output)	The light is light mapped for this primitive
	 */
	virtual void GetLightRelevance(const FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped) const;

	friend class FLandscapeVertexFactoryShaderParameters;
};

namespace 
{
	static FLOAT GetTerrainExpandPatchCount(INT LightMapRes, INT& X, INT& Y, INT ComponentSize, INT& DesiredSize)
	{
		if (LightMapRes <= 0) return 0.f;
		// Assuming DXT_1 compression at the moment...
		INT PixelPaddingX = GPixelFormats[PF_DXT1].BlockSizeX;
		INT PixelPaddingY = GPixelFormats[PF_DXT1].BlockSizeY;
/*
		if (GAllowLightmapCompression == FALSE)
		{
			PixelPaddingX = GPixelFormats[PF_A8R8G8B8].BlockSizeX;
			PixelPaddingY = GPixelFormats[PF_A8R8G8B8].BlockSizeY;
		}
*/

		INT PatchExpandCountX = (TERRAIN_PATCH_EXPAND_SCALAR * PixelPaddingX) / LightMapRes;
		INT PatchExpandCountY = (TERRAIN_PATCH_EXPAND_SCALAR * PixelPaddingY) / LightMapRes;

		X = Max<INT>(1, PatchExpandCountX);
		Y = Max<INT>(1, PatchExpandCountY);

		DesiredSize = Min<INT>((ComponentSize + 1) * LightMapRes, 4096);
		INT CurrentSize = Min<INT>((2*X + ComponentSize + 1) * LightMapRes, 4096);

		// Find proper Lightmap Size
		if (CurrentSize > DesiredSize)
		{
			// Find maximum bit
			INT PriorSize = DesiredSize;
			while (DesiredSize > 0)
			{
				PriorSize = DesiredSize;
				DesiredSize = DesiredSize & ~(DesiredSize & ~(DesiredSize-1));
			}

			DesiredSize = PriorSize << 1; // next bigger size
			if ( CurrentSize * CurrentSize <= ((PriorSize * PriorSize) << 1)  )
			{
				DesiredSize = PriorSize;
			}
		}

		INT DestSize = (FLOAT)DesiredSize / CurrentSize * (ComponentSize*LightMapRes);
		//FLOAT LightMapRatio = (FLOAT)DestSize / ComponentSize;
		FLOAT LightMapRatio = (FLOAT)DestSize / (ComponentSize*LightMapRes) * CurrentSize / DesiredSize;
		return LightMapRatio;
		//X = Y = 1;
		//return 1.0f;
	}
}

#endif // _LANDSCAPERENDER_H