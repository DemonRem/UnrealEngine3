/*=============================================================================
	D3D11Resources.h: D3D resource RHI definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "BoundShaderStateCache.h"

/**
 * Combined shader state and vertex definition for rendering geometry. 
 * Each unique instance consists of a vertex decl, vertex shader, and pixel shader.
 */
class FD3D11BoundShaderState :
	public FRefCountedObject,
	public TDynamicRHIResource<RRT_BoundShaderState>
{
public:

	FCachedBoundShaderStateLink CacheLink;

	TRefCountPtr<ID3D11InputLayout> InputLayout;
	TRefCountPtr<ID3D11VertexShader> VertexShader;
	TRefCountPtr<ID3D11PixelShader> PixelShader;
	TRefCountPtr<ID3D11HullShader> HullShader;
	TRefCountPtr<ID3D11DomainShader> DomainShader;
	TRefCountPtr<ID3D11GeometryShader> GeometryShader;

	/** Initialization constructor. */
	FD3D11BoundShaderState(
		FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		DWORD* InStreamStrides,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI,
		ID3D11Device* Direct3DDevice
		);
};

/** This represents a vertex declaration that hasn't been combined with a specific shader to create a bound shader. */
class FD3D11VertexDeclaration : public FRefCountedObject, public TDynamicRHIResource<RRT_VertexDeclaration>
{
public:

	/** Elements of the vertex declaration. */
	TPreallocatedArray<D3D11_INPUT_ELEMENT_DESC,MaxVertexElementCount> VertexElements;

	/** Initialization constructor. */
	FD3D11VertexDeclaration(const FVertexDeclarationElementList& InElements);
};

/** This represents a vertex shader that hasn't been combined with a specific declaration to create a bound shader. */
class FD3D11VertexShader : public FRefCountedObject, public TDynamicRHIResource<RRT_VertexShader>
{
public:

	/** The vertex shader resource. */
	TRefCountPtr<ID3D11VertexShader> Resource;

	/** The vertex shader's bytecode. */
	TArray<BYTE> Code;

	/** Initialization constructor. */
	FD3D11VertexShader(ID3D11VertexShader* InResource,const TArray<BYTE>& InCode):
		Resource(InResource),
		Code(InCode)
	{}
};

/** Texture base class. */
template<ERHIResourceTypes ResourceTypeEnum>
class TD3D11TextureBase : public FRefCountedObject, public TDynamicRHIResource<ResourceTypeEnum>
{
public:

	/** The view that is used to access the texture from a shader. */
	TRefCountPtr<ID3D11ShaderResourceView> View;

	/** The view that is used to render to the texture for shader-based resolve. */
	TRefCountPtr<ID3D11RenderTargetView> RenderTargetView;

	/** The view that is used to render to the texture for shader-based resolve of depth format textures. */
	TRefCountPtr<ID3D11DepthStencilView> DepthStencilView;

	/** Tracks the texture slots that a shader resource view for this surface has been bound to. */
	TArray<INT> BoundShaderResourceSlots[SF_NumFrequencies];

	/** The width of the texture. */
	const UINT SizeX;

	/** The height of texture. */
	const UINT SizeY;

	/** Number of slices in a 2d Array texture, or the z dimension in a cube texture. */
	const UINT SizeZ;

	/** The number of mip-maps in the texture. */
	const UINT NumMips;

	/** The texture's format. */
	EPixelFormat Format;

	TD3D11TextureBase(
		class FD3D11DynamicRHI* InD3DRHI,
		ID3D11ShaderResourceView* InView,
		ID3D11RenderTargetView* InRenderTargetView,
		ID3D11DepthStencilView* InDepthStencilView,
		UINT InSizeX,
		UINT InSizeY,
		UINT InSizeZ,
		UINT InNumMips,
		EPixelFormat InFormat) 
		: D3DRHI(InD3DRHI)
		, View(InView)
		, RenderTargetView(InRenderTargetView)
		, DepthStencilView(InDepthStencilView)
		, SizeX(InSizeX)
		, SizeY(InSizeY)
		, SizeZ(InSizeZ)
		, NumMips(InNumMips)
		, Format(InFormat)
		, MemorySize(0)
	{}

	virtual ~TD3D11TextureBase() {}

	INT GetMemorySize() const
	{
		return MemorySize;
	}

	void SetMemorySize( INT InMemorySize )
	{
		MemorySize = InMemorySize;
	}

	/** Unsets all texture slots that a shader resource view for this texture has been bound to. */
	void UnsetTextureReferences()
	{
		ID3D11ShaderResourceView* NullView = NULL;
		for (INT FrequencyIndex = 0; FrequencyIndex < SF_NumFrequencies; FrequencyIndex++)
		{
			for (INT SlotIndex = 0; SlotIndex < BoundShaderResourceSlots[FrequencyIndex].Num(); SlotIndex++)
			{
				INT TextureIndex = BoundShaderResourceSlots[FrequencyIndex](SlotIndex);
				if (FrequencyIndex == SF_Pixel)
				{
					D3DRHI->GetDeviceContext()->PSSetShaderResources(TextureIndex, 1, &NullView);
				}
				else if (FrequencyIndex == SF_Geometry)
				{
					D3DRHI->GetDeviceContext()->GSSetShaderResources(TextureIndex, 1, &NullView);
				}
				else if (FrequencyIndex == SF_Domain)
				{
					D3DRHI->GetDeviceContext()->DSSetShaderResources(TextureIndex, 1, &NullView);
				}
				else if (FrequencyIndex == SF_Hull)
				{
					D3DRHI->GetDeviceContext()->HSSetShaderResources(TextureIndex, 1, &NullView);
				}
				else if (FrequencyIndex == SF_Vertex)
				{
					D3DRHI->GetDeviceContext()->VSSetShaderResources(TextureIndex, 1, &NullView);
				}
				else
				{
					check(0);
				}
			}
			BoundShaderResourceSlots[FrequencyIndex].Reset();
		}
	}

protected:

	/** The D3D11 RHI that created this texture. */
	FD3D11DynamicRHI* D3DRHI;

	/** Amount of memory allocated by this texture, in bytes. */
	INT MemorySize;
};

/** 2D texture (vanilla, cubemap or 2D array) */
template<ERHIResourceTypes ResourceTypeEnum>
class TD3D11Texture2D : public TD3D11TextureBase<ResourceTypeEnum>
{
public:

	/** The D3D texture resource.  Note that a Texture2D may also be an array of homogenous Texture2Ds in D3D11 forming a cubemap. */
	TRefCountPtr<ID3D11Texture2D> Resource;

	/** Whether the texture is a cube-map. */
	const BITFIELD bCubemap : 1;

	/** Initialization constructor. */
	TD3D11Texture2D(
		class FD3D11DynamicRHI* InD3DRHI,
		ID3D11Texture2D* InResource,
		ID3D11ShaderResourceView* InView,
		ID3D11RenderTargetView* InRenderTargetView,
		ID3D11DepthStencilView* InDepthStencilView,
		UINT InSizeX,
		UINT InSizeY,
		UINT InSizeZ,
		UINT InNumMips,
		EPixelFormat InFormat,
		UBOOL bInCubemap
		)
		: TD3D11TextureBase(
			InD3DRHI,
			InView, 
			InRenderTargetView,
			InDepthStencilView,
			InSizeX,
			InSizeY,
			InSizeZ,
			InNumMips,
			InFormat)
		, Resource(InResource)
		, bCubemap(bInCubemap)
	{
	}

	virtual ~TD3D11Texture2D()
	{
		if ( Resource.GetRefCount() == 1 )
		{
			D3D11TextureDeleted( *this );
		}
	}

	/**
	 * Locks one of the texture's mip-maps.
	 * @return A pointer to the specified texture data.
	 */
	void* Lock(UINT MipIndex,UINT ArrayIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride);

	/** Unlocks a previously locked mip-map. */
	void Unlock(UINT MipIndex,UINT ArrayIndex);
};

/** 3D Texture */
template<ERHIResourceTypes ResourceTypeEnum>
class TD3D11Texture3D : public TD3D11TextureBase<ResourceTypeEnum>
{
public:

	/** The D3D texture resource.  */
	TRefCountPtr<ID3D11Texture3D> Resource;

	/** Initialization constructor. */
	TD3D11Texture3D(
		class FD3D11DynamicRHI* InD3DRHI,
		ID3D11Texture3D* InResource,
		ID3D11ShaderResourceView* InView,
		UINT InSizeX,
		UINT InSizeY,
		UINT InSizeZ,
		UINT InNumMips,
		EPixelFormat InFormat
		)
		: TD3D11TextureBase(
			InD3DRHI,
			InView, 
			NULL,
			NULL,
			InSizeX,
			InSizeY,
			InSizeZ,
			InNumMips,
			InFormat)
		, Resource(InResource)
	{
	}

	virtual ~TD3D11Texture3D()
	{
		if ( Resource.GetRefCount() == 1 )
		{
			D3D11TextureDeleted( *this );
		}
	}
};

typedef TD3D11TextureBase<RRT_Texture>			FD3D11TextureBase;
typedef TD3D11Texture2D<RRT_Texture>			FD3D11Texture;
typedef TD3D11Texture2D<RRT_Texture2D>			FD3D11Texture2D;
typedef TD3D11Texture2D<RRT_Texture2DArray>		FD3D11Texture2DArray;
typedef TD3D11Texture3D<RRT_Texture3D>			FD3D11Texture3D;
typedef TD3D11Texture2D<RRT_TextureCube>		FD3D11TextureCube;
typedef TD3D11Texture2D<RRT_SharedTexture2D>	FD3D11SharedTexture2D;

/** D3D11 occlusion query */
class FD3D11OcclusionQuery : public FRefCountedObject, public TDynamicRHIResource<RRT_OcclusionQuery>
{
public:

	/** The query resource. */
	TRefCountPtr<ID3D11Query> Resource;

	/** The cached query result. */
	QWORD Result;

	/** TRUE if the query's result is cached. */
	UBOOL bResultIsCached : 1;

	/** Initialization constructor. */
	FD3D11OcclusionQuery(ID3D11Query* InResource):
		Resource(InResource),
		bResultIsCached(FALSE)
	{}
};

/** Index buffer resource class that stores stride information. */
class FD3D11IndexBuffer : public FRefCountedObject, public TDynamicRHIResource<RRT_IndexBuffer>
{
public:

	/** The index buffer resource */
	TRefCountPtr<ID3D11Buffer> Resource;
	UINT Stride;

	FD3D11IndexBuffer(ID3D11Buffer* InResource, UINT InStride) : 
		Resource(InResource),
		Stride(InStride)
	{}
};

// Resources that directly map to D3D resources.
template<typename D3DResourceType,ERHIResourceTypes ResourceTypeEnum> class TD3DResource : public D3DResourceType, public TDynamicRHIResource<ResourceTypeEnum> {};
typedef TD3DResource<ID3D11PixelShader,RRT_PixelShader>				FD3D11PixelShader;
typedef TD3DResource<ID3D11Buffer,RRT_VertexBuffer>			FD3D11VertexBuffer;
typedef TD3DResource<FRefCountedObject,RRT_SharedMemoryResource>	FD3D11SharedMemoryResource;
typedef TD3DResource<ID3D11HullShader,RRT_HullShader> FD3D11HullShader;
typedef TD3DResource<ID3D11DomainShader,RRT_DomainShader> FD3D11DomainShader;
typedef TD3DResource<ID3D11GeometryShader,RRT_GeometryShader> FD3D11GeometryShader;

// Release of dynamic buffers used as a workaround for DIPUP and DUP
void ReleaseDynamicVBandIBBuffers();

template <typename FTextureType>
void D3D11TextureDeleted( FTextureType& Texture )
{
}
void D3D11TextureDeleted( FD3D11Texture2D& Texture );
void D3D11TextureAllocated( FD3D11Texture2D& Texture );