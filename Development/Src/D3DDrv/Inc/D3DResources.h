/*=============================================================================
	D3DResources.h: D3D resource RHI definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if USE_D3D_RHI

// Vertex declaration.
typedef TD3DRef<IDirect3DVertexDeclaration9> FVertexDeclarationRHIRef;
typedef IDirect3DVertexDeclaration9* FVertexDeclarationRHIParamRef;

// Shaders.
typedef TD3DRef<IDirect3DVertexShader9> FVertexShaderRHIRef;
typedef IDirect3DVertexShader9* FVertexShaderRHIParamRef;

typedef TD3DRef<IDirect3DPixelShader9> FPixelShaderRHIRef;
typedef IDirect3DPixelShader9* FPixelShaderRHIParamRef;

/**
* Combined shader state and vertex definition for rendering geometry. 
* Each unique instance consists of a vertex decl, vertex shader, and pixel shader.
*/
struct FBoundShaderStateRHIRef
{
	FVertexDeclarationRHIRef VertexDeclaration;
	FVertexShaderRHIRef VertexShader;
	FPixelShaderRHIRef PixelShader;

	/**
	* @return TRUE if the sate object has a valid decl & vs resource
	*/
	friend UBOOL IsValidRef(const FBoundShaderStateRHIRef& Ref)
	{
		return( IsValidRef(Ref.VertexShader) && IsValidRef(Ref.VertexDeclaration) );
	}

	void Invalidate()
	{
		VertexDeclaration = NULL;
		VertexShader = NULL;
		PixelShader = NULL;
	}

	/**
	* Equality is based on vertex decl, vertex shader and pixel shader
	* @param Other - instance to compare against
	* @return TRUE if equal
	*/
	UBOOL operator==(const FBoundShaderStateRHIRef& Other) const
	{
		return (VertexDeclaration == Other.VertexDeclaration && VertexShader == Other.VertexShader && PixelShader == Other.PixelShader);
	}

	/**
	* Get the hash for this type
	* @param Key - struct to hash
	* @return dword hash based on type
	*/
	friend DWORD GetTypeHash(const FBoundShaderStateRHIRef &Key)
	{
		return PointerHash(
			(IDirect3DVertexDeclaration9 *)Key.VertexDeclaration, 
			PointerHash((IDirect3DVertexShader9 *)Key.VertexShader, 
			PointerHash((IDirect3DPixelShader9 *)Key.PixelShader))
			);
	}
};

typedef const FBoundShaderStateRHIRef& FBoundShaderStateRHIParamRef;

// Index buffer.
typedef TD3DRef<IDirect3DIndexBuffer9> FIndexBufferRHIRef;
typedef IDirect3DIndexBuffer9* FIndexBufferRHIParamRef;

// Vertex buffer.
typedef TD3DRef<IDirect3DVertexBuffer9> FVertexBufferRHIRef;
typedef IDirect3DVertexBuffer9* FVertexBufferRHIParamRef;

// Textures.
template<typename ResourceType>
class FBaseTextureRHI : public TD3DRef<ResourceType>
{
public:
	FBaseTextureRHI(UBOOL bInSRGB=FALSE,ResourceType* InResource = NULL) 
		: TD3DRef<ResourceType>(InResource)
		, bSRGB(bInSRGB) 
	{

	}

	FBaseTextureRHI(const FBaseTextureRHI& Copy)
		: TD3DRef<ResourceType>(Copy)
		, bSRGB(Copy.bSRGB)
	{

	}

	FBaseTextureRHI& operator=(const FBaseTextureRHI& Other)
	{
		bSRGB = Other.bSRGB;
		TD3DRef<ResourceType>::operator=((ResourceType*)Other);
		return *this;
	}

	operator FBaseTextureRHI<IDirect3DBaseTexture9>() const
	{
		return FBaseTextureRHI<IDirect3DBaseTexture9>(bSRGB,*this);
	}

	FORCEINLINE UBOOL IsSRGB() const
	{ 
		return bSRGB; 
	}
private:
	BITFIELD bSRGB:1;
};

typedef FBaseTextureRHI<IDirect3DBaseTexture9> FTextureRHIRef;
typedef FTextureRHIRef FTextureRHIParamRef;

typedef FBaseTextureRHI<IDirect3DTexture9> FTexture2DRHIRef;
typedef FTexture2DRHIRef FTexture2DRHIParamRef;

typedef FBaseTextureRHI<IDirect3DCubeTexture9> FTextureCubeRHIRef;
typedef FTextureCubeRHIRef FTextureCubeRHIParamRef;

typedef TD3DRef<FD3DRefCountedObject> FSharedMemoryResourceRHIRef;
typedef FTexture2DRHIRef FSharedTexture2DRHIRef;


// Occlusion query.
typedef TD3DRef<IDirect3DQuery9> FOcclusionQueryRHIRef;
typedef IDirect3DQuery9* FOcclusionQueryRHIParamRef;

// Command lists.
typedef void FCommandContextRHI;

#endif
