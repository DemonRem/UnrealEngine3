/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class Texture extends Surface
	native
	abstract
	dependson(Engine);

// This needs to be mirrored in UnEdFact.cpp.
enum TextureCompressionSettings
{
	TC_Default,
	TC_Normalmap,
	TC_Displacementmap,
	TC_NormalmapAlpha,
	TC_Grayscale,
	TC_HighDynamicRange
};

// @warning: When you update this, you must add an entry to GPixelFormats(see UnRenderUtils.cpp) and also update XeTools.cpp.
enum EPixelFormat
{
	PF_Unknown,
	PF_A32B32G32R32F,
	PF_A8R8G8B8,
	PF_G8,
	PF_G16,
	PF_DXT1,
	PF_DXT3,
	PF_DXT5,
	PF_UYVY,
	PF_FloatRGB,		// A RGB FP format with platform-specific implementation, for use with render targets
	PF_FloatRGBA,		// A RGBA FP format with platform-specific implementation, for use with render targets
	PF_DepthStencil,	// A depth+stencil format with platform-specific implementation, for use with render targets
	PF_ShadowDepth,		// A depth format with platform-specific implementation, for use with render targets
	PF_FilteredShadowDepth, // A depth format with platform-specific implementation, that can be filtered by hardware
	PF_R32F,
	PF_FloatRGBA_Full,	// A RGBA FP format with platform-specific implementation, for use with render targets. Full 16bit precision	
	PF_G16R16,
	PF_G16R16F,
	PF_G32R32F,
	PF_A2B10G10R10,
	PF_A16B16G16R16,
	PF_D24
};

enum TextureFilter
{
	TF_Nearest,
	TF_Linear
};

enum TextureAddress
{
	TA_Wrap,
	TA_Clamp,
	TA_Mirror
};

// @warning: update UTextureFactory::StaticConstructor in UnEdFact.cpp if this is changed
// @warning: update FTextureLODSettings::Initialize and GetTextureGroupNames in Texture.cpp if this is changed
enum TextureGroup
{
	TEXTUREGROUP_World,
	TEXTUREGROUP_WorldNormalMap,
	TEXTUREGROUP_WorldSpecular,
	TEXTUREGROUP_Character,
	TEXTUREGROUP_CharacterNormalMap,
	TEXTUREGROUP_CharacterSpecular,
	TEXTUREGROUP_Weapon,
	TEXTUREGROUP_WeaponNormalMap,
	TEXTUREGROUP_WeaponSpecular,
	TEXTUREGROUP_Vehicle,
	TEXTUREGROUP_VehicleNormalMap,
	TEXTUREGROUP_VehicleSpecular,
	TEXTUREGROUP_Effects,
	TEXTUREGROUP_Skybox,
	TEXTUREGROUP_UI,
	TEXTUREGROUP_LightAndShadowMap,
	TEXTUREGROUP_RenderTarget
};

//@warning: make sure to update UTexture::PostEditChange if you add an option that might require recompression.

// Texture settings.

var()	bool							SRGB;
var		bool							RGBE;

var()	float							UnpackMin[4],
										UnpackMax[4];

var native const UntypedBulkData_Mirror	SourceArt{FByteBulkData};

var()	bool							CompressionNoAlpha;
var		bool							CompressionNone;
var		bool							CompressionNoMipmaps;
var()	bool							CompressionFullDynamicRange;
var()	bool							DeferCompression;

/** Allows artists to specify that a texture should never have its miplevels dropped which is useful for e.g. HUD and menu textures */
var()	bool							NeverStream;

/** When TRUE, mip-maps are dithered for smooth transitions. */
var()	bool							bDitherMipMapAlpha;

/** If TRUE, the color border pixels are preseved by mipmap generation.  One flag per color channel. */
var()	bool							bPreserveBorderR;
var()	bool							bPreserveBorderG;
var()	bool							bPreserveBorderB;
var()	bool							bPreserveBorderA;

/** Whether the async resource release process has already been kicked off or not */
var		transient const private bool	bAsyncResourceReleaseHasBeenStarted;

var()	TextureCompressionSettings		CompressionSettings;

/** The texture filtering mode to use when sampling this texture. */
var()	TextureFilter					Filter;

/** Texture group this texture belongs to for LOD bias */
var()	TextureGroup					LODGroup;

/** A bias to the index of the top mip level to use. */
var()	int								LODBias;
/** Cached combined group and texture LOD bias to use.	*/
var		transient int					CachedCombinedLODBias;

var()	editconst string				SourceFilePath;         // Path to the resource used to construct this texture
var()	editconst string				SourceFileTimestamp;    // Date/Time-stamp of the file from the last import

/** The texture's resource. */
var native const pointer				Resource{FTextureResource};

cpptext
{
	/**
	 * Resets the resource for the texture.
	 */
	void ReleaseResource();

	/**
	 * Creates a new resource for the texture, and updates any cached references to the resource.
	 */
	virtual void UpdateResource();

	/**
	 * Implemented by subclasses to create a new resource for the texture.
	 */
	virtual FTextureResource* CreateResource() PURE_VIRTUAL(UTexture::CreateResource,return NULL;);

	/**
	 * Returns the cached combined LOD bias based on texture LOD group and LOD bias.
	 *
	 * @return	LOD bias
	 */
	FORCEINLINE INT GetCachedLODBias()
	{
		return CachedCombinedLODBias;
	}

	/**
	 * Compresses the texture based on the compression settings. Make sure to update UTexture::PostEditChange
	 * if you add any variables that might require recompression.
	 */
	virtual void Compress();

	/**
	 * @return The material value type of this texture.
	 */
	virtual EMaterialValueType GetMaterialType() PURE_VIRTUAL(UTexture::GetMaterialType,return MCT_Texture;);

	/**
	 * Updates the streaming status of the texture and performs finalization when appropriate. The function returns
	 * TRUE while there are pending requests in flight and updating needs to continue.
	 *
	 * @return	TRUE if there are requests in flight, FALSE otherwise
	 */
	virtual UBOOL UpdateStreamingStatus()
	{
		return FALSE;
	}

	// UObject interface.
	virtual void PreEditChange(UProperty* PropertyThatChanged);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual void PreSave();
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform);
}

defaultproperties
{
	SRGB=True
	UnpackMax(0)=1.0
	UnpackMax(1)=1.0
	UnpackMax(2)=1.0
	UnpackMax(3)=1.0
	Filter=TF_Linear
}
