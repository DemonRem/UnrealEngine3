/*=============================================================================
	RHI.h: Render Hardware Interface definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnBuild.h"

/*
Platform independent RHI definitions.
The odd use of non-member functions to operate on resource references allows two different approaches to RHI implementation:
- Resource references are to RHI data structures, which directly contain the platform resource data.
- Resource references are to the platform's HAL representation of the resource.  This eliminates a layer of indirection for platforms such as
	D3D where the RHI doesn't directly store resource data.
*/

//
// RHI capabilities.
//

/** Maximum number of miplevels in a texture. */
enum { MAX_TEXTURE_MIP_COUNT = 13 };

/** The maximum number of vertex elements which can be used by a vertex declaration. */
enum { MaxVertexElementCount = 16 };

/** The maximum number of mip-maps that a texture can contain. 	*/
extern	INT		GMaxTextureMipCount;
/** The minimum number of mip-maps that always remain resident.	*/
extern 	INT		GMinTextureResidentMipCount;

/** TRUE if PF_DepthStencil textures can be created and sampled. */
extern UBOOL GSupportsDepthTextures;

/** 
* TRUE if PF_DepthStencil textures can be created and sampled to obtain PCF values. 
* This is different from GSupportsDepthTextures in three ways:
*	-results of sampling are PCF values, not depths
*	-a color target must be bound with the depth stencil texture even if never written to or read from,
*		due to API restrictions
*	-a dedicated resolve surface may or may not be necessary
*/
extern UBOOL GSupportsHardwarePCF;

/** TRUE if D24 textures can be created and sampled, retrieving 4 neighboring texels in a single lookup. */
extern UBOOL GSupportsFetch4;

/**
* TRUE if floating point filtering is supported
*/
extern UBOOL GSupportsFPFiltering;

/** TRUE if pixels are clipped to 0<Z<W instead of -W<Z<W. */
extern const UBOOL GPositiveZRange;

/** Can we handle quad primitives? */
extern const UBOOL GSupportsQuads;

/** Are we using an inverted depth buffer? Viewport MinZ/MaxZ reversed */
extern const UBOOL GUsesInvertedZ;

/** The offset from the upper left corner of a pixel to the position which is used to sample textures for fragments of that pixel. */
extern const FLOAT GPixelCenterOffset;

/** Shadow buffer size */
extern INT GShadowDepthBufferSize;

/** Bias exponent used to apply to shader color output when rendering to the scene color surface */
extern INT GCurrentColorExpBias;

/** Toggle for MSAA tiling support */
extern UBOOL GUseTilingCode;

/**
 *	The size to check against for Draw*UP call vertex counts.
 *	If greater than this value, the draw call will not occur.
 */
extern INT GDrawUPVertexCheckCount;
/**
 *	The size to check against for Draw*UP call index counts.
 *	If greater than this value, the draw call will not occur.
 */
extern INT GDrawUPIndexCheckCount;

/** TRUE if the rendering hardware supports vertex instancing. */
extern UBOOL GSupportsVertexInstancing;

/** Enable or disable vsync, overridden by the -novsync command line parameter. */
extern UBOOL GEnableVSync;

/** If FALSE code needs to patch up vertex declaration. */
extern UBOOL GVertexElementsCanShareStreamOffset;

//
// Common RHI definitions.
//

enum ESamplerFilter
{
	SF_Nearest,
	SF_Linear,
	SF_AnisotropicLinear
};

enum ESamplerAddressMode
{
	AM_Wrap,
	AM_Clamp,
	AM_Mirror
};

enum ESamplerMipMapLODBias
{
	MIPBIAS_None,
	MIPBIAS_Get4
};

enum ERasterizerFillMode
{
	FM_Point,
	FM_Wireframe,
	FM_Solid
};

enum ERasterizerCullMode
{
	CM_None,
	CM_CW,
	CM_CCW
};

enum EColorWriteMask
{
	CW_RED		= 0x01,
	CW_GREEN	= 0x02,
	CW_BLUE		= 0x04,
	CW_ALPHA	= 0x08,

	CW_RGB		= 0x07,
	CW_RGBA		= 0x0f,
};

enum ECompareFunction
{
	CF_Less,
	CF_LessEqual,
	CF_Greater,
	CF_GreaterEqual,
	CF_Equal,
	CF_NotEqual,
	CF_Never,
	CF_Always
};

enum EStencilOp
{
	SO_Keep,
	SO_Zero,
	SO_Replace,
	SO_SaturatedIncrement,
	SO_SaturatedDecrement,
	SO_Invert,
	SO_Increment,
	SO_Decrement
};

enum EBlendOperation
{
	BO_Add,
	BO_Subtract,
	BO_Min,
	BO_Max
};

enum EBlendFactor
{
	BF_Zero,
	BF_One,
	BF_SourceColor,
	BF_InverseSourceColor,
	BF_SourceAlpha,
	BF_InverseSourceAlpha,
	BF_DestAlpha,
	BF_InverseDestAlpha,
	BF_DestColor,
	BF_InverseDestColor
};

enum EVertexElementType
{
	VET_None,
	VET_Float1,
	VET_Float2,
	VET_Float3,
	VET_Float4,
	VET_PackedNormal,	// FPackedNormal
	VET_UByte4,
	VET_UByte4N,
	VET_Color,
	VET_Short2,
	VET_Short2N,		// 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
	VET_Half2			// 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa 
};

enum EVertexElementUsage
{
	VEU_Position,
	VEU_TextureCoordinate,
	VEU_BlendWeight,
	VEU_BlendIndices,
	VEU_Normal,
	VEU_Tangent,
	VEU_Binormal,
	VEU_Color
};

enum ECubeFace
{
	CubeFace_PosX=0,
	CubeFace_NegX,
	CubeFace_PosY,
	CubeFace_NegY,
	CubeFace_PosZ,
	CubeFace_NegZ,
	CubeFace_MAX
};

struct FVertexElement
{
	BYTE StreamIndex;
	BYTE Offset;
	BYTE Type;
	BYTE Usage;
	BYTE UsageIndex;

	FVertexElement() {}
	FVertexElement(BYTE InStreamIndex,BYTE InOffset,BYTE InType,BYTE InUsage,BYTE InUsageIndex):
		StreamIndex(InStreamIndex),
		Offset(InOffset),
		Type(InType),
		Usage(InUsage),
		UsageIndex(InUsageIndex)
	{}
};

typedef TStaticArray<FVertexElement,MaxVertexElementCount> FVertexDeclarationElementList;

struct FSamplerStateInitializerRHI
{
	ESamplerFilter Filter;
	ESamplerAddressMode AddressU;
	ESamplerAddressMode AddressV;
	ESamplerAddressMode AddressW;
	ESamplerMipMapLODBias MipBias;
};
struct FRasterizerStateInitializerRHI
{
	ERasterizerFillMode FillMode;
	ERasterizerCullMode CullMode;
	FLOAT DepthBias;
	FLOAT SlopeScaleDepthBias;
};
struct FDepthStateInitializerRHI
{
	UBOOL bEnableDepthWrite;
	ECompareFunction DepthTest;
};

struct FStencilStateInitializerRHI
{
	FStencilStateInitializerRHI(
		UBOOL bInEnableFrontFaceStencil = FALSE,
		ECompareFunction InFrontFaceStencilTest = CF_Always,
		EStencilOp InFrontFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InFrontFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InFrontFacePassStencilOp = SO_Keep,
		UBOOL bInEnableBackFaceStencil = FALSE,
		ECompareFunction InBackFaceStencilTest = CF_Always,
		EStencilOp InBackFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InBackFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InBackFacePassStencilOp = SO_Keep,
		DWORD InStencilReadMask = 0xFFFFFFFF,
		DWORD InStencilWriteMask = 0xFFFFFFFF,
		DWORD InStencilRef = 0) :
		bEnableFrontFaceStencil(bInEnableFrontFaceStencil),
		FrontFaceStencilTest(InFrontFaceStencilTest),
		FrontFaceStencilFailStencilOp(InFrontFaceStencilFailStencilOp),
		FrontFaceDepthFailStencilOp(InFrontFaceDepthFailStencilOp),
		FrontFacePassStencilOp(InFrontFacePassStencilOp),
		bEnableBackFaceStencil(bInEnableBackFaceStencil),
		BackFaceStencilTest(InBackFaceStencilTest),
		BackFaceStencilFailStencilOp(InBackFaceStencilFailStencilOp),
		BackFaceDepthFailStencilOp(InBackFaceDepthFailStencilOp),
		BackFacePassStencilOp(InBackFacePassStencilOp),
		StencilReadMask(InStencilReadMask),
		StencilWriteMask(InStencilWriteMask),
		StencilRef(InStencilRef)
	{
	}

	UBOOL bEnableFrontFaceStencil;
	ECompareFunction FrontFaceStencilTest;
	EStencilOp FrontFaceStencilFailStencilOp;
	EStencilOp FrontFaceDepthFailStencilOp;
	EStencilOp FrontFacePassStencilOp;
	UBOOL bEnableBackFaceStencil;
	ECompareFunction BackFaceStencilTest;
	EStencilOp BackFaceStencilFailStencilOp;
	EStencilOp BackFaceDepthFailStencilOp;
	EStencilOp BackFacePassStencilOp;
	DWORD StencilReadMask;
	DWORD StencilWriteMask;
	DWORD StencilRef;
};
struct FBlendStateInitializerRHI
{
	EBlendOperation ColorBlendOperation;
	EBlendFactor ColorSourceBlendFactor;
	EBlendFactor ColorDestBlendFactor;
	EBlendOperation AlphaBlendOperation;
	EBlendFactor AlphaSourceBlendFactor;
	EBlendFactor AlphaDestBlendFactor;
	ECompareFunction AlphaTest;
	BYTE AlphaRef;
};

enum EPrimitiveType
{
	PT_TriangleList = 0,
	PT_TriangleFan = 1,
	PT_TriangleStrip = 2,
	PT_LineList = 3,
	PT_QuadList = 4,

	PT_NumBits = 3
};

enum EParticleEmitterType
{
	PET_None = 0,		// Not a particle emitter
	PET_Sprite = 1,		// Sprite particle emitter
	PET_SubUV = 2,		// SubUV particle emitter
	PET_Mesh = 3,		// Mesh emitter

	PET_NumBits = 2
};

// Pixel shader constants that are reserved by the Engine.
enum EPixelShaderRegisters
{
	PSR_ColorBiasFactor = 0,			// Factor applied to the color output from the pixelshader
	PSR_ScreenPositionScaleBias = 1,	// Converts projection-space XY coordinates to texture-space UV coordinates
	PSR_MinZ_MaxZ_Ratio = 2,			// Converts device Z values to clip-space W values
};

// Vertex shader constants that are reserved by the Engine.
enum EVertexShaderRegister
{
	VSR_ViewProjMatrix = 0,		// View-projection matrix, transforming from World space to Projection space
	VSR_ViewOrigin = 4,			// World space position of the view's origin (camera position)
};

/**
 *	Screen Resoultion
 */
struct FScreenResolutionRHI
{
	DWORD	Width;
	DWORD	Height;
	DWORD	RefreshRate;
};

typedef TArray<FScreenResolutionRHI>	ScreenResolutionArray;

//
// Platform-specific RHI types.
//

#if USE_NULL_RHI
	// Use the NULL RHI.
	#include "NullRHI.h"
#elif _WINDOWS
	// #define a wrapper define that will make D3D files still compile even when using Null RHI
	#define USE_D3D_RHI 1
	// Use the D3D RHI on Windows.
	#include "..\..\D3DDrv\Inc\D3DDrv.h"
#elif XBOX
	// #define a wrapper define that will make XeD3D files still compile even when using Null RHI
	#define USE_XeD3D_RHI 1
	// Use the XeD3D RHI on Xenon
	#include "XeD3DDrv.h"
#elif PS3
	// Use PS3 RHI
    #define USE_PS3_RHI 1
	#include "PS3RHI.h"
#else
	// turn on the Null RHI define that will compile in NullRHI code
	#undef USE_NULL_RHI
	#define USE_NULL_RHI 1
	// Use the NULL RHI.
	#include "NullRHI.h"
#endif

//
// RHI globals.
//

/** True if the render hardware has been initialized. */
extern UBOOL GIsRHIInitialized;

//
// Platform independent RHI functions.
//

//
// RHI resource management functions.
//

extern FCommandContextRHI* RHIGetGlobalContext();

extern FSamplerStateRHIRef RHICreateSamplerState(const FSamplerStateInitializerRHI& Initializer);
extern FRasterizerStateRHIRef RHICreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer);
extern FDepthStateRHIRef RHICreateDepthState(const FDepthStateInitializerRHI& Initializer);
extern FStencilStateRHIRef RHICreateStencilState(const FStencilStateInitializerRHI& Initializer);
extern FBlendStateRHIRef RHICreateBlendState(const FBlendStateInitializerRHI& Initializer);

extern FVertexDeclarationRHIRef RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements);
extern FPixelShaderRHIRef RHICreatePixelShader(const TArray<BYTE>& Code);
extern FVertexShaderRHIRef RHICreateVertexShader(const TArray<BYTE>& Code);

/**
* Creates a bound shader state instance which encapsulates a decl, vertex shader, and pixel shader
* @param VertexDeclaration - existing vertex decl
* @param StreamStrides - optional stream strides
* @param VertexShader - existing vertex shader
* @param PixelShader - existing pixel shader
*/
extern FBoundShaderStateRHIRef RHICreateBoundShaderState(FVertexDeclarationRHIParamRef VertexDeclaration, DWORD *StreamStrides, FVertexShaderRHIParamRef VertexShader, FPixelShaderRHIParamRef PixelShader);

/**
* Initializes GRHIShaderPlatform
*/
extern void RHIInitializeShaderPlatform();

extern FIndexBufferRHIRef RHICreateIndexBuffer(UINT Stride,UINT Size,FResourceArrayInterface* ResourceArray,UBOOL bIsDynamic);

extern void* RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer,UINT Offset,UINT Size);
extern void RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer);

/**
 * @param ResourceArray - An optional pointer to a resource array containing the resource's data.
 */
extern FVertexBufferRHIRef RHICreateVertexBuffer(UINT Size,FResourceArrayInterface* ResourceArray,UBOOL bIsDynamic);

extern void* RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer,UINT Offset,UINT SizeRHI,UBOOL bReadOnly=FALSE);
extern void RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer);

/**
 * Retrieves texture memory stats.
 *
 * @param	OutAllocatedMemorySize	[out]	Size of allocated memory
 * @param	OutAvailableMemorySize	[out]	Size of available memory
 *
 * @return TRUE if supported, FALSE otherwise
 */
extern UBOOL RHIGetTextureMemoryStats( INT& AllocatedMemorySize, INT& AvailableMemorySize );

/** Flags used for texture creation */
enum ETextureCreateFlags
{
	// Texture is encoded in sRGB gamma space
	TexCreate_SRGB					= 1<<0,
	// Texture can be used as a resolve target
	TexCreate_ResolveTargetable		= 1<<1,
	// Texture is a depth stencil format that can be sampled
	TexCreate_DepthStencil			= 1<<2,
	// Texture will be created without a packed miptail
	TexCreate_NoMipTail				= 1<<3,
	// Texture will be created with an un-tiled format
	TexCreate_NoTiling				= 1<<4,
	// Texture that for a resolve target will only be written to/resolved once
	TexCreate_WriteOnce				= 1<<5,
};

/**
* Creates a 2D RHI texture resource
* @param SizeX - width of the texture to create
* @param SizeY - height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
extern FTexture2DRHIRef RHICreateTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,DWORD Flags);

/**
* Locks an RHI texture's mip surface for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - mip level index for the surface to retrieve
* @param bIsDataBeingWrittenTo - used to affect the lock flags 
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
extern void* RHILockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail=FALSE);

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - mip level index for the surface to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
extern void RHIUnlockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bLockWithinMiptail=FALSE);

/**
* For platforms that support packed miptails return the first mip level which is packed in the mip tail
* @return mip level for mip tail or -1 if mip tails are not used
*/
extern INT RHIGetMipTailIdx(FTexture2DRHIParamRef Texture);

/** specifies a texture and region to copy */
struct FCopyTextureRegion2D
{
	/** source texture to lock and copy from */
	FTexture2DRHIParamRef SrcTexture;
	/** horizontal texel offset for copy region */
	INT OffsetX;
	/** vertical texel offset for copy region */
	INT OffsetY;
	/** horizontal texel size for copy region */
	INT SizeX;
	/** vertical texel size for copy region */
	INT SizeY;	
	/** Starting mip index. This is treated as the base level (default is 0) */
	INT FirstMipIdx;
	/** constructor */
	FCopyTextureRegion2D(FTexture2DRHIParamRef InSrcTexture,INT InOffsetX=-1,INT InOffsetY=-1,INT InSizeX=-1,INT InSizeY=-1,INT InFirstMipIdx=0)
		:	SrcTexture(InSrcTexture)
		,	OffsetX(InOffsetX)
		,	OffsetY(InOffsetY)
		,	SizeX(InSizeX)
		,	SizeY(InSizeY)
		,	FirstMipIdx(InFirstMipIdx)
	{}
};

/**
 * Copies a region within the same mip levels of one texture to another.  An optional region can be speci
 * Note that the textures must be the same size and of the same format.
 * @param DstTexture - destination texture resource to copy to
 * @param MipIdx - mip level for the surface to copy from/to. This mip level should be valid for both source/destination textures
 * @param BaseSizeX - width of the texture (base level). Same for both source/destination textures
 * @param BaseSizeY - height of the texture (base level). Same for both source/destination textures 
 * @param Format - format of the texture. Same for both source/destination textures
 * @param Region - list of regions to specify rects and source textures for the copy
 */
extern void RHICopyTexture2D(FTexture2DRHIParamRef DstTexture, UINT MipIdx, INT BaseSizeX, INT BaseSizeY, INT Format, const TArray<FCopyTextureRegion2D>& Regions);

/**
 * Copies texture data freom one mip to another
 * Note that the mips must be the same size and of the same format.
 * @param SrcText Source texture to copy from
 * @param SrcMipIndex Mip index into the source texture to copy data from
 * @param DestText Destination texture to copy to
 * @param DestMipIndex Mip index in the destination texture to copy to - note this is probably different from source mip index if the base widths/heights are different
 * @param Size Size of mip data
 * @param Counter Thread safe counter used to flag end of transfer
 */
extern void RHICopyMipToMipAsync(FTexture2DRHIParamRef SrcTexture, INT SrcMipIndex, FTexture2DRHIParamRef DestTexture, INT DestMipIndex, INT Size, FThreadSafeCounter& Counter);

/**
 * Computes the device-dependent amount of memory required by a texture.  This size may be passed to RHICreateSharedMemory.
 * @param SizeX - The width of the texture.
 * @param SizeY - The height of the texture.
 * @param SizeZ - The depth of the texture.
 * @param Format - The format of the texture.
 * @return The amount of memory in bytes required by a texture with the given properties.
 */
extern SIZE_T RHICalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format);

/**
* Create resource memory to be shared by multiple RHI resources
* @param Size - aligned size of allocation
* @return shared memory resource RHI ref
*/
extern FSharedMemoryResourceRHIRef RHICreateSharedMemory(SIZE_T Size);

/**
 * Creates a RHI texture and if the platform supports it overlaps it in memory with another texture
 * Note that modifying this texture will modify the memory of the overlapped texture as well
 * @param SizeX - The width of the surface to create.
 * @param SizeY - The height of the surface to create.
 * @param Format - The surface format to create.
 * @param ResolveTargetTexture - The 2d texture to use the memory from if the platform allows
 * @param Flags - Surface creation flags
 * @return The surface that was created.
 */
extern FSharedTexture2DRHIRef RHICreateSharedTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,FSharedMemoryResourceRHIRef SharedMemory,DWORD Flags);

/**
* Creates a Cube RHI texture resource
* @param Size - width/height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
extern FTextureCubeRHIRef RHICreateTextureCube(UINT Size,BYTE Format,UINT NumMips,DWORD Flags);

/**
* Locks an RHI texture's mip surface for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - mip level index for the surface to retrieve
* @param bIsDataBeingWrittenTo - used to affect the lock flags 
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
extern void* RHILockTextureCubeFace(FTextureCubeRHIParamRef Texture,UINT FaceIndex,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail=FALSE);

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - mip level index for the surface to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
extern void RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef Texture,UINT FaceIndex,UINT MipIndex,UBOOL bLockWithinMiptail=FALSE);

/** Flags used for targetable surface creation */
enum ETargetableSurfaceCreateFlags
{
	// Without this the surface may simply be an alias for the texture's memory. Note that you must still resolve the surface.
    TargetSurfCreate_Dedicated		= 1<<0,
	// Surface must support being read from by RHIReadSurfaceData.
	TargetSurfCreate_Readable		= 1<<1,
	// Surface will be only written to one time, in case that helps the platform
	TargetSurfCreate_WriteOnce		= 1<<2,
};

/**
 * Creates a RHI surface that can be bound as a render target.
 * Note that a surface cannot be created which is both resolvable AND readable.
 * @param SizeX - The width of the surface to create.
 * @param SizeY - The height of the surface to create.
 * @param Format - The surface format to create.
 * @param ResolveTargetTexture - The 2d texture which the surface will be resolved to.  It must have been allocated with bResolveTargetable=TRUE
 * @param Flags - Surface creation flags
 * @param UsageStr - Text describing usage for this surface
 * @return The surface that was created.
 */
extern FSurfaceRHIRef RHICreateTargetableSurface(
	UINT SizeX,
	UINT SizeY,
	BYTE Format,
	FTexture2DRHIParamRef ResolveTargetTexture,
	DWORD Flags,
	const TCHAR* UsageStr
	);

/**
* Creates a RHI surface that can be bound as a render target and can resolve w/ a cube texture
* Note that a surface cannot be created which is both resolvable AND readable.
* @param SizeX - The width of the surface to create.
* @param Format - The surface format to create.
* @param ResolveTargetTexture - The cube texture which the surface will be resolved to.  It must have been allocated with bResolveTargetable=TRUE
* @param CubeFace - face from resolve texture to use as surface
* @param Flags - Surface creation flags
* @param UsageStr - Text describing usage for this surface
* @return The surface that was created.
*/
extern FSurfaceRHIRef RHICreateTargetableCubeSurface(
	UINT SizeX,
	BYTE Format,
	FTextureCubeRHIParamRef ResolveTargetTexture,
	ECubeFace CubeFace,
	DWORD Flags,
	const TCHAR* UsageStr
	);

struct FResolveParams
{
	/** used to specify face when resolving to a cube map texture */
	ECubeFace CubeFace;
	/** resolve RECT bounded by [X1,Y1]..[X2,Y2]. Or -1 for fullscreen */
	INT X1,Y1,X2,Y2;
	/** constructor */
	FResolveParams(INT InX1=-1, INT InY1=-1, INT InX2=-1, INT InY2=-1, ECubeFace InCubeFace=CubeFace_PosX)
		:	CubeFace(InCubeFace)
		,	X1(InX1), Y1(InY1), X2(InX2), Y2(InY2)
		
	{}
};

/**
* Copies the contents of the given surface to its resolve target texture.
* @param SourceSurface - surface with a resolve texture to copy to
* @param bKeepOriginalSurface - TRUE if the original surface will still be used after this function so must remain valid
* @param ResolveParams - optional resolve params
*/
extern void RHICopyToResolveTarget(FSurfaceRHIParamRef SourceSurface, UBOOL bKeepOriginalSurface, const FResolveParams& ResolveParams=FResolveParams());

/**
 * Copies the contents of the given surface's resolve target texture back to the surface.
 * If the surface isn't currently allocated, the copy may be deferred until the next time it is allocated.
 * @param DestSurface - surface with a resolve texture to copy from
 */
extern void RHICopyFromResolveTarget(FSurfaceRHIParamRef DestSurface);

/**
* Copies the contents of the given surface's resolve target texture back to the surface without doing
* anything to the pixels (no exponent correction, no gamma correction).
* If the surface isn't currently allocated, the copy may be deferred until the next time it is allocated.
*
* @param DestSurface - surface with a resolve texture to copy from
*/
extern void RHICopyFromResolveTargetFast(FSurfaceRHIParamRef DestSurface);

/**
 * @return True if the surface has memory allocated to it.
 */
extern UBOOL RHIIsSurfaceAllocated(FSurfaceRHIParamRef Surface);

/**
 * Discards the contents of a surface, allowing its memory to be freed.
 */
extern void RHIDiscardSurface(FSurfaceRHIParamRef Surface);

/**
 * Reads the contents of a surface to an output buffer.
 */
extern void RHIReadSurfaceData(FSurfaceRHIRef &Surface,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY,TArray<BYTE>& OutData,ECubeFace CubeFace=CubeFace_PosX);

extern FOcclusionQueryRHIRef RHICreateOcclusionQuery();
extern UBOOL RHIGetOcclusionQueryResult(FOcclusionQueryRHIParamRef OcclusionQuery,DWORD& OutNumPixels,UBOOL bWait);

#if RHI_SUPPORTS_COMMAND_LISTS
extern FCommandListRHIRef RHICreateCommandList();
extern FCommandListAddressRHI RHIGetCommandListBaseAddress(FCommandListRHIParamRef CommandList);
extern FCommandContextRHI* RHIBeginRecordingCommandList(FCommandListRHIParamRef CommandList,FCommandListAddressParamRHI Address);
extern void RHIEndRecordingCommandList(FCommandListRHIParamRef CommandList);
#endif

extern void RHIBeginDrawingViewport(FViewportRHIParamRef Viewport);
extern void RHIEndDrawingViewport(FViewportRHIParamRef Viewport,UBOOL bPresent,UBOOL bLockToVsync=FALSE);
extern FSurfaceRHIRef RHIGetViewportBackBuffer(FViewportRHIParamRef Viewport);

/*
 * Returns the total GPU time taken to render the last frame. Same metric as appCycles().
 */
extern DWORD RHIGetGPUFrameCycles();

/*
 * Returns an approximation of the available video memory that textures can use, rounded to the nearest MB, in MB.
 */
extern DWORD RHIGetAvailableTextureMemory();

/**
 * The following RHI functions must be called from the main thread.
 */
extern FViewportRHIRef RHICreateViewport(void* WindowHandle,UINT SizeX,UINT SizeY,UBOOL bIsFullscreen);
extern void RHIResizeViewport(FViewportRHIParamRef Viewport,UINT SizeX,UINT SizeY,UBOOL bIsFullscreen);
extern void RHITick( FLOAT DeltaTime );

//
// RHI commands.
//

// Flow control/fixups.
#if RHI_SUPPORTS_COMMAND_LISTS
extern FCommandListAddressRHI RHIGetCurrentCommandAddress(FCommandContextRHI* Context);
extern void RHICommandListCall(FCommandContextRHI* Context,FCommandListRHIParamRef CommandList,FCommandListAddressParamRHI Address);
extern void RHICommandListJump(FCommandContextRHI* Context,FCommandListRHIParamRef CommandList,FCommandListAddressParamRHI Address);
extern void RHICommandListReturn(FCommandContextRHI* Context);
#endif

// Vertex state.
extern void RHISetStreamSource(FCommandContextRHI* Context,UINT StreamIndex,FVertexBufferRHIParamRef VertexBuffer,UINT Stride,UBOOL bUseInstanceInstance = FALSE,UINT NumVerticesPerInstance = 0,UINT NumInstances = 1);

// Rasterizer state.
extern void RHISetRasterizerState(FCommandContextRHI* Context,FRasterizerStateRHIParamRef NewState);
extern void RHISetRasterizerStateImmediate(FCommandContextRHI* Context,const FRasterizerStateInitializerRHI &ImmediateState);
extern void RHISetViewport(FCommandContextRHI* Context,UINT MinX,UINT MinY,FLOAT MinZ,UINT MaxX,UINT MaxY,FLOAT MaxZ);
extern void RHISetScissorRect(FCommandContextRHI* Context,UBOOL bEnable,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY);
extern void RHISetDepthBoundsTest(FCommandContextRHI* Context,UBOOL bEnable,FLOAT MinZ,FLOAT MaxZ);

// Shader state.
/**
 * Set bound shader state. This will set the vertex decl/shader, and pixel shader
 * @param Context - context we are rendering to
 * @param BoundShaderState - state resource
 */
extern void RHISetBoundShaderState(FCommandContextRHI* Context,FBoundShaderStateRHIParamRef BoundShaderState);

extern void RHISetSamplerState(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT SamplerIndex,FSamplerStateRHIParamRef NewState,FTextureRHIParamRef NewTexture);
extern void RHISetVertexShaderParameter(FCommandContextRHI* Context,FVertexShaderRHIParamRef VertexShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue);
extern void RHISetPixelShaderParameter(FCommandContextRHI* Context,FPixelShaderRHIParamRef PixelShader,UINT BaseRegisterIndex,UINT NumVectors,const FLOAT* NewValue);
extern void RHISetRenderTargetBias( FCommandContextRHI* Context, FLOAT ColorBias );

/**
 * Set engine vertex shader parameters for the view.
 * @param Context				Context we are rendering to
 * @param View					The current view
 * @param ViewProjectionMatrix	Matrix that transforms from world space to projection space for the view
 * @param ViewOrigin			World space position of the view's origin
 */
extern void RHISetViewParameters( FCommandContextRHI* Context, const FSceneView* View, const FMatrix& ViewProjectionMatrix, const FVector4& ViewOrigin );

/**
 * Set engine pixel shader parameters for the view.
 * Some platforms needs to set this for each pixelshader, whereas others can set it once, globally.
 * @param Context							Context we are rendering to
 * @param View								The current view
 * @param PixelShader						The pixel shader to set the parameters for
 * @param SceneDepthCalcParameter			Handle for the scene depth calc parameter (PSR_MinZ_MaxZ_Ratio). May be NULL.
 * @param SceneDepthCalcValue				Handle for the scene depth calc parameter (PSR_MinZ_MaxZ_Ratio). Ignored if SceneDepthCalcParameter is NULL.
 * @param ScreenPositionScaleBiasParameter	Handle for the screen position scale and bias parameter (PSR_ScreenPositionScaleBias). May be NULL.
 * @param ScreenPositionScaleValue			Value for the screen position scale and bias parameter (PSR_ScreenPositionScaleBias). Ignored if ScreenPositionScaleBiasParameter is NULL.
 */
extern void RHISetViewPixelParameters( FCommandContextRHI* Context, const class FSceneView* View, FPixelShaderRHIParamRef PixelShader, const class FShaderParameter* SceneDepthCalcParameter, const class FShaderParameter* ScreenPositionScaleBiasParameter );

/**
 * Control the GPR (General Purpose Register) allocation 
 * @param NumVertexShaderRegisters - num of GPRs to allocate for the vertex shader (default is 64)
 * @param NumPixelShaderRegisters - num of GPRs to allocate for the pixel shader (default is 64)
 */
extern void RHISetShaderRegisterAllocation(UINT NumVertexShaderRegisters, UINT NumPixelShaderRegisters);

/**
 * Optimizes pixel shaders that are heavily texture fetch bound due to many L2 cache misses.
 * @param PixelShader	The pixel shader to optimize texture fetching for
 */
extern void RHIReduceTextureCachePenalty( FPixelShaderRHIParamRef PixelShader );

// Output state.
extern void RHISetDepthState(FCommandContextRHI* Context, FDepthStateRHIParamRef NewState);
extern void RHISetStencilState(FCommandContextRHI* Context, FStencilStateRHIParamRef NewState);
extern void RHISetBlendState(FCommandContextRHI* Context, FBlendStateRHIParamRef NewState);
extern void RHISetRenderTarget(FCommandContextRHI* Context, FSurfaceRHIParamRef NewRenderTarget, FSurfaceRHIParamRef NewDepthStencilTarget);
extern void RHISetColorWriteEnable(FCommandContextRHI* Context, UBOOL bEnable);
extern void RHISetColorWriteMask(FCommandContextRHI* Context, UINT ColorWriteMask);

// Hi stencil optimization
extern void RHIBeginHiStencilRecord(FCommandContextRHI* Context);
extern void RHIBeginHiStencilPlayback(FCommandContextRHI* Context);
extern void RHIEndHiStencil(FCommandContextRHI* Context);

// Occlusion queries.
extern void RHIBeginOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery);
extern void RHIEndOcclusionQuery(FCommandContextRHI* Context,FOcclusionQueryRHIParamRef OcclusionQuery);

// Primitive drawing.
extern void RHIDrawPrimitive(FCommandContextRHI* Context,UINT PrimitiveType,UINT BaseVertexIndex,UINT NumPrimitives);
extern void RHIDrawIndexedPrimitive(FCommandContextRHI* Context,FIndexBufferRHIParamRef IndexBuffer,UINT PrimitiveType,INT BaseVertexIndex,UINT MinIndex,UINT NumVertices,UINT StartIndex,UINT NumPrimitives);

// Immediate Primitive drawing
/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex 
 * @param OutVertexData Reference to the allocated vertex memory
 */
extern void RHIBeginDrawPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData);
/**
 * Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
 * @param Context Rendering device context to use
 */
extern void RHIEndDrawPrimitiveUP(FCommandContextRHI* Context);
/**
 * Draw a primitive using the vertices passed in
 * VertexData is NOT created by BeginDrawPrimitveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param VertexData A reference to memory preallocate in RHIBeginDrawPrimitiveUP
 * @param VertexDataStride Size of each vertex
 */
extern void RHIDrawPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, const void* VertexData,UINT VertexDataStride);

/**
 * Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawIndexedPrimitiveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param NumVertices The number of vertices to be written
 * @param VertexDataStride Size of each vertex
 * @param OutVertexData Reference to the allocated vertex memory
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumIndices Number of indices to be written
 * @param IndexDataStride Size of each index (either 2 or 4 bytes)
 * @param OutIndexData Reference to the allocated index memory
 */
extern void RHIBeginDrawIndexedPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT NumPrimitives, UINT NumVertices, UINT VertexDataStride, void*& OutVertexData, UINT MinVertexIndex, UINT NumIndices, UINT IndexDataStride, void*& OutIndexData);
/**
 * Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
 * @param Context Rendering device context to use
 */
extern void RHIEndDrawIndexedPrimitiveUP(FCommandContextRHI* Context);
/**
 * Draw a primitive using the vertices passed in as described the passed in indices. 
 * IndexData and VertexData are NOT created by BeginDrawIndexedPrimitveUP
 * @param Context Rendering device context to use
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumVertices The number of vertices in the vertex buffer
 * @param NumPrimitives THe number of primitives described by the index buffer
 * @param IndexData The memory preallocated in RHIBeginDrawIndexedPrimitiveUP
 * @param IndexDataStride The size of one index
 * @param VertexData The memory preallocate in RHIBeginDrawIndexedPrimitiveUP
 * @param VertexDataStride The size of one vertex
 */
extern void RHIDrawIndexedPrimitiveUP(FCommandContextRHI* Context, UINT PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT NumPrimitives, const void* IndexData, UINT IndexDataStride, const void* VertexData, UINT VertexDataStride);

/**
 * Draw a sprite particle emitter.
 *
 * @param Context Rendering device context to use
 * @param Mesh The mesh element containing the data for rendering the sprite particles
 */
extern void RHIDrawSpriteParticles(FCommandContextRHI* Context, const FMeshElement& Mesh);

/**
 * Draw a sprite subuv particle emitter.
 *
 * @param Context Rendering device context to use
 * @param Mesh The mesh element containing the data for rendering the sprite subuv particles
 */
extern void RHIDrawSubUVParticles(FCommandContextRHI* Context, const FMeshElement& Mesh);

// Raster operations.
extern void RHIClear(FCommandContextRHI* Context,UBOOL bClearColor,const FLinearColor& Color,UBOOL bClearDepth,FLOAT Depth,UBOOL bClearStencil,DWORD Stencil);

// Kick the rendering commands that are currently queued up in the GPU command buffer.
extern void RHIKickCommandBuffer( FCommandContextRHI* Context );

// Operations to suspend title rendering and yield control to the system
extern void RHISuspendRendering();
extern void RHIResumeRendering();

// MSAA-specific functions
extern void RHIMSAAInitPrepass();
extern void RHIMSAAFixViewport();
extern void RHIMSAABeginRendering();
extern void RHIMSAAEndRendering(const FTexture2DRHIRef& DepthTexture, const FTexture2DRHIRef& ColorTexture);
extern void RHIMSAARestoreDepth(const FTexture2DRHIRef& DepthTexture);

/**
 *	Retrieve available screen resolutions.
 *
 *	@param	ResolutionLists		TArray<TArray<FScreenResolutionRHI>> parameter that will be filled in.
 *	@param	bIgnoreRefreshRate	If TRUE, ignore refresh rates.
 *
 *	@return	UBOOL				TRUE if successfully filled the array
 */
extern UBOOL RHIGetAvailableResolutions(TArray<ScreenResolutionArray>& ResolutionLists, UBOOL bIgnoreRefreshRate = TRUE);
