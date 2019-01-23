/*=============================================================================
	NullRHI.h: Null RHI definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if USE_NULL_RHI

//
// Private NullRHI definitions.
//

/**
 * The base class of NullRHI resources.
 */
class FNullRHIResource : public virtual FRefCountedObject
{
};

class FNullRHISamplerState : public FNullRHIResource
{};

class FNullRHIRasterizerState : public FNullRHIResource
{};

class FNullRHIDepthState : public FNullRHIResource
{};

class FNullRHIStencilState : public FNullRHIResource
{};

class FNullRHIBlendState : public FNullRHIResource
{};

class FNullRHIVertexDeclaration : public FNullRHIResource
{};

class FNullRHIVertexShader : public FNullRHIResource
{};

class FNullRHIPixelShader : public FNullRHIResource
{};

class FNullRHIBoundShaderState : public FNullRHIResource
{};

class FNullRHIIndexBuffer : public FNullRHIResource
{};

class FNullRHIVertexBuffer : public FNullRHIResource
{};

class FNullRHIOcclusionQuery : public FNullRHIResource
{};

class FNullRHICommandList : public FNullRHIResource
{};

/**
 * Encapsulates a reference to a NullRHI resource.
 */
template<typename ResourceType>
class TNullRHIResourceRef
{
public:
	TNullRHIResourceRef() 
	{}
	TNullRHIResourceRef(ResourceType* InResource)
	:	Resource(InResource)
	{}
	TNullRHIResourceRef(const TNullRHIResourceRef& Copy)
	{
		Resource = Copy.Resource;
	}
	TNullRHIResourceRef& operator=(ResourceType* InResource)
	{
		Resource = InResource;
		return *this;
	}
	friend UBOOL IsValidRef(const TNullRHIResourceRef& Ref)
	{
		return Ref.Resource != NULL;
	}
	void Release()
	{
		Resource = NULL;
	}
	typedef ResourceType* ResourcePointer;
	operator ResourcePointer() const
	{
		return (ResourceType*)Resource;
	}
	ResourcePointer operator->() const
	{
		return (ResourceType*)Resource;
	}
private:
	TRefCountPtr<ResourceType> Resource;
};

//
// RHI resource types.
//

typedef TNullRHIResourceRef<class FNullRHISamplerState> FSamplerStateRHIRef;
typedef FSamplerStateRHIRef::ResourcePointer FSamplerStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIRasterizerState> FRasterizerStateRHIRef;
typedef FRasterizerStateRHIRef::ResourcePointer FRasterizerStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIDepthState> FDepthStateRHIRef;
typedef FDepthStateRHIRef::ResourcePointer FDepthStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIStencilState> FStencilStateRHIRef;
typedef FStencilStateRHIRef::ResourcePointer FStencilStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIBlendState> FBlendStateRHIRef;
typedef FBlendStateRHIRef::ResourcePointer FBlendStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIVertexDeclaration> FVertexDeclarationRHIRef;
typedef FVertexDeclarationRHIRef::ResourcePointer FVertexDeclarationRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIPixelShader> FPixelShaderRHIRef;
typedef FPixelShaderRHIRef::ResourcePointer FPixelShaderRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIVertexShader> FVertexShaderRHIRef;
typedef FVertexShaderRHIRef::ResourcePointer FVertexShaderRHIParamRef;


// Need to be able to hash these
class FBoundShaderStateRHIRef : public TNullRHIResourceRef<class FNullRHIBoundShaderState>
{
public:
	UBOOL operator==(const FBoundShaderStateRHIRef& Other) const
	{
		return TRUE;
	}

	void Invalidate() {}

	friend DWORD GetTypeHash(const FBoundShaderStateRHIRef &Key)
	{
		return 0;
	}
};
typedef FBoundShaderStateRHIRef::ResourcePointer FBoundShaderStateRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIIndexBuffer> FIndexBufferRHIRef;
typedef FIndexBufferRHIRef::ResourcePointer FIndexBufferRHIParamRef;

typedef TNullRHIResourceRef<class FNullRHIVertexBuffer> FVertexBufferRHIRef;
typedef FVertexBufferRHIRef::ResourcePointer FVertexBufferRHIParamRef;

/**
 */
class FNullRHISurface : public FNullRHIResource
{
};
typedef TNullRHIResourceRef<FNullRHISurface> FSurfaceRHIRef;
typedef FSurfaceRHIRef::ResourcePointer FSurfaceRHIParamRef;

/**
 */
class FNullRHITexture : public FNullRHISurface
{
};
typedef TNullRHIResourceRef<FNullRHITexture> FTextureRHIRef;
typedef FTextureRHIRef::ResourcePointer FTextureRHIParamRef;

/**
 */
class FNullRHITexture2D : public FNullRHITexture
{
};
typedef TNullRHIResourceRef<FNullRHITexture2D> FTexture2DRHIRef;
typedef FTexture2DRHIRef::ResourcePointer FTexture2DRHIParamRef;

/**
 */
class FNullRHITextureCube : public FNullRHITexture
{
};
typedef TNullRHIResourceRef<FNullRHITextureCube> FTextureCubeRHIRef;
typedef FTextureCubeRHIRef::ResourcePointer FTextureCubeRHIParamRef;

/**
 */
class FNullRHIRenderTarget : public FNullRHISurface
{
};

/**
 */
class FNullRHIDepthStencilTarget : public FNullRHISurface
{
};
typedef TNullRHIResourceRef<FNullRHIDepthStencilTarget> FDepthStencilTargetRHIRef;
typedef FDepthStencilTargetRHIRef::ResourcePointer FDepthStencilTargetRHIParamRef;

/**
 */
typedef TNullRHIResourceRef<class FNullRHIOcclusionQuery> FOcclusionQueryRHIRef;
typedef FOcclusionQueryRHIRef::ResourcePointer FOcclusionQueryRHIParamRef;

#if RHI_SUPPORTS_COMMAND_LISTS
/**
 */
class FCommandListAddressRHI
{
};

/**
 */
typedef TNullRHIResourceRef<class FNullRHICommandList> FCommandListRHIRef;
typedef FCommandListRHIRef::ResourcePointer FCommandListRHIParamRef;

#endif // RHI_SUPPORTS_COMMAND_LISTS

/**
 * A RHI viewport.  Note that unlike all the other RHI resources, the viewport doesn't need to be reinitialized when the RHI is reset.
 */
typedef TNullRHIResourceRef<class FNullRHIViewport> FViewportRHIRef;
typedef FViewportRHIRef::ResourcePointer FViewportRHIParamRef;

/**
 */
typedef void FCommandContextRHI;

class FNullRHIViewport : public FNullRHIResource
{
public:

	friend FViewportRHIRef RHICreateViewport(void* WindowHandle,UINT SizeX,UINT SizeY,UBOOL bIsFullscreen);
	friend FSurfaceRHIRef GetViewportBackBufferRHI(FViewportRHIParamRef Viewport);

private:

	UINT SizeX;
	UINT SizeY;
	UBOOL bIsFullscreen;
	FSurfaceRHIRef BackBuffer;	

	FNullRHIViewport(UINT InSizeX,UINT InSizeY,UBOOL bInIsFullscreen);
};

enum ED3DRHIStats
{
	STAT_PresentTime = STAT_RHIFirstStat,
	STAT_DrawPrimitiveCalls,
	STAT_RHITriangles,
	STAT_RHILines,
};

typedef TNullRHIResourceRef<class FRefCountedObject> FSharedMemoryResourceRHIRef;
typedef FTexture2DRHIRef FSharedTexture2DRHIRef;



/**
* Perform PS3 GCM/RHI initialization that needs to wait until after appInit has happened
*/
void PS3PostAppInitRHIInitialization();

/**
* Memory regions that the GPU can access
*/
enum EMemoryRegion
{
	MR_Bad		= -1,
	MR_Local	= -1, // rand number
	MR_Host		= -1, // rand number
};


#endif // USE_NULL_RHI
