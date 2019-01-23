/*=============================================================================
	MobileSupport.h: Mobile related enums and functions needed by the editor/cooker/mobile platforms

	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#define MINIMIZE_ES2_SHADERS 1

#define SHADER_MANIFEST_VERSION (1)

////////////////////////////////////
//
// Enums
// 
////////////////////////////////////

//Mobile platforms
enum EMobilePlatformFeatures
{
	EPF_HighEndFeatures,
	EPF_LowEndFeatures,

	EPF_MAX,
};


/**
 * The scene rendering stats.
 */
enum EMobileTextureUnit
{
	Base_MobileTexture,
	Detail_MobileTexture,
	Lightmap_MobileTexture,
	Normal_MobileTexture,
	Environment_MobileTexture,
	Mask_MobileTexture,
	Emissive_MobileTexture,
	
	MAX_MobileTexture
};

enum EMobilePrimitiveType
{
	EPT_Default,	//EPT_DefaultTextureLit, EPT_DefaultUnlit, EPT_SkeletalMesh
	EPT_Particle,	//EPT_ParticleSubUV
	EPT_PointSpriteParticle,
	EPT_LensFlare,
	EPT_Simple,
	EPT_GlobalShader,	// Note: All other bits in the key changes meaning - they now specify which type of global shader it is.
	EPT_MAX
};

/**
 * For global shaders. PKDT_GlobalShaderType is used instead of PKDT_EmissiveMaskSource.
 */
enum EMobileGlobalShaderType
{
	EGST_None,			// (Meaning it's not a global shader.)
	EGST_GammaCorrection,
	EGST_Filter,
	EGST_DOFAndBloomGather,
	EGST_LUTBlender,
	EGST_UberPostProcess,
	EGST_LightShaftDownSample,
	EGST_LightShaftBlur,
	EGST_LightShaftApply,
	EGST_SimpleF32,
	EGST_MAX,
};

/**
 * An enum that defines all of the components used to generate program keys
 */
enum EProgramKeyDataTypes
{
	//Reserved for primitive type & platform features
	PKDT_PlatformFeatures,
	PKDT_PrimitiveType,

	//World/Misc provided
	PKDT_IsDepthOnlyRendering,
	PKDT_IsGradientFogEnabled,
	PKDT_ParticleScreenAlignment,

	//Vertex Factory Flags
	PKDT_IsLightmap,
	PKDT_IsSkinned,
	PKDT_IsDecal,
	PKDT_IsSubUV,

	//Material provided
	PKDT_IsLightingEnabled,
	PKDT_IsAlphaTestEnabled,
	PKDT_UsingAdditiveMaterial,
	PKDT_BaseTextureTexCoordsSource,
	PKDT_DetailTextureTexCoordsSource,
	PKDT_MaskTextureTexCoordsSource,
	PKDT_IsTextureCoordinateTransformEnabled,
	PKDT_TextureTransformTarget,
	PKDT_IsSpecularEnabled,
	PKDT_IsPixelSpecularEnabled,
	PKDT_IsNormalMappingEnabled,
	PKDT_IsEnvironmentMappingEnabled,
	PKDT_MobileEnvironmentBlendMode,
	PKDT_IsMobileEnvironmentFresnelEnabled,
	PKDT_IsBumpOffsetEnabled,
	PKDT_IsColorTextureBlendingEnabled,
	PKDT_TextureBlendFactorSource,
	PKDT_IsWaveVertexMovementEnabled,
	PKDT_SpecularMask,
	PKDT_AmbientOcclusionSource,
	//Remove because we have to near double shader counts to avoid the multiply
	//PKDT_FixedLightmapScaleFactor,
	//This is never set, commenting out
	//PKDT_SimpleLightmapsStoredInLinearSpace,
	PKDT_UseUniformColorMultiply,
	PKDT_UseVertexColorMultiply,
	PKDT_IsRimLightingEnabled,
	PKDT_RimLightingMaskSource,
	PKDT_EnvironmentMaskSource,
	PKDT_IsEmissiveEnabled,
	PKDT_EmissiveColorSource,
	PKDT_EmissiveMaskSource,
	PKDT_GlobalShaderType = PKDT_EmissiveMaskSource,	// Global shader type piggy-backs on other bits that aren't used in this case.

	//Max Counter
	PKDT_MAX,
};


/** A macro used by both ES2RHI routines and in MaterialShared code for assigning ProgramKeyData values */
#define ASSIGN_PROGRAM_KEY_VALUE(E, K, V) \
	checkfSlow(((INT)(1 << (ES2ShaderProgramKeyFields[E].NumBits+1)) >= (ES2ShaderProgramKeyFields[E].MaxValue)), TEXT("Program key value has a max value that is too large for the number of bits!")); \
	checkfSlow((ES2ShaderProgramKeyFields[E].MaxValue > (INT)V), TEXT("Program key value is greater than maximum value!"));									\
	checkfSlow(K.IsStarted() && !K.IsStopped(), TEXT("Key Value protection was not used properly.  Please call Start on the ProgramKeyData"));					\
	checkfSlow(K.bFieldSet[E] == FALSE, TEXT("Key is being clobbered.  Use RESET_PROGRAM_KEY_VALUE to do this on purpose, as it corrects number of sets"));		\
	K.bFieldSet[E] = TRUE;																																	\
	K.NumberFieldsSet++;																																	\
	K.FieldValue[E] = V;

/** A macro used by both ES2RHI routines and in MaterialShared code for overwriting values when a condition is met */
#define OVERRIDE_PROGRAM_KEY_VALUE(E, K, V) \
	checkfSlow(((INT)(1 << (ES2ShaderProgramKeyFields[E].NumBits+1)) >= (ES2ShaderProgramKeyFields[E].MaxValue)), TEXT("Program key value has a max value that is too large for the number of bits!")); \
	checkfSlow((ES2ShaderProgramKeyFields[E].MaxValue > (INT)V), TEXT("Program key value is greater than maximum value!"));								\
	checkfSlow(K.IsStarted() && !K.IsStopped(), TEXT("Key Value protection was not used properly.  Please call Start on the ProgramKeyData"));			\
	checkfSlow(K.bFieldSet[E] == TRUE, TEXT("Key has not been set yet.  Override is incorrect at this point as the ref count hasn't been incremented"));\
	if (K.bFieldLocked[E]==FALSE)																															\
	{																																				\
		K.FieldValue[E] = V;																														\
	}

/** A macro used by to reset a particular field so we can set it again */
#define RESET_PROGRAM_KEY_VALUE( E, K)					\
	checkfSlow(((INT)(1 << (ES2ShaderProgramKeyFields[E].NumBits+1)) >= (ES2ShaderProgramKeyFields[E].MaxValue)), TEXT("Program key value has a max value that is too large for the number of bits!")); \
	checkfSlow(K.IsStarted() && !K.IsStopped(), TEXT("Key Value protection was not used properly.  Please call Start on the ProgramKeyData"));		\
	checkfSlow(K.bFieldSet[E] == TRUE, TEXT("No need to reset key, as it has never been set"));														\
	if (K.bFieldLocked[E]==FALSE)																														\
	{																																			\
		K.NumberFieldsSet--;																													\
		K.bFieldSet[E] = FALSE;																													\
		K.FieldValue[E] = 0;																													\
	}

/** A macro used by to reset a particular field so we can set it again */
#define LOCK_PROGRAM_KEY_VALUE( E, K, B)					\
	checkfSlow(K.IsStarted() && !K.IsStopped(), TEXT("Key Value protection was not used properly.  Please call Start on the ProgramKeyData"));		\
	checkfSlow(K.bFieldSet[E] == TRUE, TEXT("No need to reset key, as it has never been set"));														\
	K.bFieldLocked[E] = B;

struct ES2ShaderProgramKeyField
{
	INT NumBits;
	INT MaxValue;
};
/**
 * An array that holds the number of bits required to store each component in the program key
 */
extern ES2ShaderProgramKeyField ES2ShaderProgramKeyFields[];

/**
 * A simple structure used to pass around program key data values
 */
struct ProgramKeyData
{
	ProgramKeyData()
	{
		appMemzero( FieldValue, sizeof(FieldValue) );
		appMemzero( bFieldSet, sizeof(bFieldSet) );
		appMemzero( bFieldLocked, sizeof(bFieldLocked) );
		NumberFieldsSet = 0;
		bStarted = 0;
		bStopped = 0;
	}

	/** Starts the sampling, tracks if all the parameters were set */
	void Start (void);

	/** Stops allowing input and verifies all input has been set */
	void Stop (void);

	/** Whether we've started filling in the data */
	UBOOL IsStarted (void) const
	{
		return bStarted;
	}

	/** Whether we've started filling in the data */
	UBOOL IsStopped (void) const
	{
		return bStopped;
	}

	/** Returns if the key data was properly filled in */
	UBOOL IsValid (void) const
	{
		return (bStarted && bStopped && NumberFieldsSet == PKDT_MAX);
	}

	BYTE FieldValue[PKDT_MAX];
	BYTE bFieldSet[PKDT_MAX];
	BYTE bFieldLocked[PKDT_MAX];
	BYTE NumberFieldsSet;
private:
	BITFIELD bStarted : 1;
	BITFIELD bStopped : 1;
};

/**
 * Packs the initialized ProgramKeyData structure into a single QWORD
 *
 * @param KeyData	The initialized ProgramKeyData structure
 * @return			The key for the current texture format/alphatest settings
 */
extern QWORD PackProgramKeyData( ProgramKeyData& KeyData );

/**
 * Unpacks the provided QWORD into the provided ProgramKeyData structure
 *
 * @param ProgramKey	The key to unpack
 * @param KeyData		The ProgramKeyData structure to fill
 */
void UnpackProgramKeyData( QWORD ProgramKey, ProgramKeyData& KeyData );

extern QWORD HexStringToQWord(const TCHAR *In);

/**
 * Calls the visual C preprocessor, if available
 *
 * @param SourceCode		Code to preprocess
 * @param DestFile			Fully qualified (!) path to where the preprocessed source should go
 * @return					string containing the preprocessed source
 */
FString RunCPreprocessor(const FString &SourceCode,const TCHAR *DestFile, UBOOL CleanWhiteSPace = TRUE);


/**
 * Returns the base filename of a ES2 shader
 *
 * @param PrimitiveType		Primitive type to get name for
 * @param GlobalShaderType	Global shader type (if PrimitiveType is EPT_GlobalShader)
 * @param Kind				SF_VERTEX or SF_PIXEL
 * @return					string containing the shader name, including path
 */
FString GetES2ShaderFilename( EMobilePrimitiveType PrimitiveType, EMobileGlobalShaderType GlobalShaderType, EShaderFrequency Kind );

/**
 * Generates the shader source for an ES2 shader based on the relevant keys
 *
 * @param Platform					Target platform
 * @param KeyData					The initialized ProgramKeyData structure
 * @param ProgramKey				The key for the current texture format/alphatest settings
 * @param PrimitiveType				Primitive type to build shaders for
 * @param bIsCompilingForPC			True if we're compiling this shader for use on PC ES2
 * @param CommonShaderPrefixFile	preloaded Shaders\\ES2\\Prefix_Common.glsl
 * @param VertexShaderPrefixFile	preloaded Shaders\\ES2\\VertexShader_Common.glsl
 * @param PixelShaderPrefixFile		preloaded Shaders\\ES2\\PixelShader_Common.glsl
 * @param MobileSettings			settings to base the shader on
 * @param OutVertexShader			returned vertex shader, modified only if return == TRUE
 * @param OutPixelShader			returned pixel shader, modified only if return == TRUE
 * @return							TRUE if valid shader source was returned
 */
UBOOL ES2ShaderSourceFromKeys( 
	UE3::EPlatformType			Platform,
	QWORD						ProgramKey, 
	const ProgramKeyData&		KeyData, 
	EMobilePrimitiveType		PrimitiveType, 
	const UBOOL					bIsCompilingForPC,
	FString						CommonShaderPrefixFile,
	FString						VertexShaderPrefixFile,
	FString						PixelShaderPrefixFile,
	FLOAT						MobileSettingsMobileLODBias,
	INT							MobileSettingsMobileBoneCount,
	INT							MobileSettingsMobileBoneWeightCount,
	FString&					OutVertexShader,
	FString&					OutPixelShader
	);


/**
 * Mobile Material Vertex Shader Params
 */
class FMobileMaterialVertexParams
{
public:

	/** True if lighting is enabled */
	UBOOL bUseLighting;

	UBOOL bTextureTransformEnabled;
	EMobileTextureTransformTarget TextureTransformTarget;
	TMatrix<3,3> TextureTransform;

	/** Whether texture blending between base texture and detail texture is enabled */
	UBOOL bUseTextureBlend;
	/** Whether to lock on or off texture blending regardless of platform settings */
	UBOOL bLockTextureBlend;

	/** When texture blending is enabled, what to use as the blend factor */
	EMobileTextureBlendFactorSource TextureBlendFactorSource;

	/** Whether emissive is enabled (and has an appropriate texture set) */
	UBOOL bUseEmissive;

	/** Emissive color source */
	EMobileEmissiveColorSource EmissiveColorSource;

	/** Emissive mask source */
	EMobileValueSource EmissiveMaskSource;

	/** Emissive constant color */
	FLinearColor EmissiveColor;

	/** Whether normal mapping should be used */
	UBOOL bUseNormalMapping;

	/** Whether environment mapping should be used */
	UBOOL bUseEnvironmentMapping;

	/** Source for environment map amount */
	EMobileValueSource EnvironmentMaskSource;

	/** Environment map strength */
	FLOAT EnvironmentAmount;

	/** Environment map fresnel amount (0.0 = disabled) */
	FLOAT EnvironmentFresnelAmount;

	/** Environment map fresnel exponent */
	FLOAT EnvironmentFresnelExponent;

	/** Whether we should enable specular features when rendering */
	UBOOL bUseSpecular;

	/** Whether we should enable per-pixel specular features when rendering */
	UBOOL bUsePixelSpecular;

	/** Material specular color */
	FLinearColor SpecularColor;

	/** Material specular power */
	FLOAT SpecularPower;

	/** Rim lighting strength */
	FLOAT RimLightingStrength;

	/** Rim lighting exponent */
	FLOAT RimLightingExponent;

	/** Rim lighting amount source */
	EMobileValueSource RimLightingMaskSource;

	/** Rim lighting color */
	FLinearColor RimLightingColor;

	/**Wave vertex movement*/
	UBOOL bWaveVertexMovementEnabled;
	FLOAT VertexMovementTangentFrequencyMultiplier;
	FLOAT VertexMovementVerticalFrequencyMultiplier;
	FLOAT MaxVertexMovementAmplitude;

	//Sway constants
	FLOAT SwayFrequencyMultiplier;
	FLOAT SwayMaxAngle;

	UBOOL bAllowFog;

	//Use Vertex Color
	UBOOL bVertexColor;


	/** Blending mode used by the material */
	EBlendMode MaterialBlendMode;

	/** Base texture UV coordinate source */
	EMobileTexCoordsSource BaseTextureTexCoordsSource;

	/** Detail texture UV coordinate source */
	EMobileTexCoordsSource DetailTextureTexCoordsSource;

	/** Mask texture UV coordinate source */
	EMobileTexCoordsSource MaskTextureTexCoordsSource;

	/** Ambient occlusion data source */
	EMobileAmbientOcclusionSource AmbientOcclusionSource;

	/** Whether to useuniform color scaling (mesh particles) or not */
	UBOOL bUseUniformColorMultiply;
	/** Default color to modulate each vertex by */
	FLinearColor UniformMultiplyColor;

	/** Whether to use per vertex color scaling */
	UBOOL bUseVertexColorMultiply;

	//for tracking what material is requesting particular shaders
	FString MaterialName;
};


/**
 * Mobile Material Pixel Shader Params
 */
class FMobileMaterialPixelParams
{
public:
	/** Whether we should apply bump offset when rendering and the reference values to use */
	UBOOL bBumpOffsetEnabled;
	FLOAT BumpReferencePlane;
	FLOAT BumpHeightRatio;

	/** Source of specular mask for per-vertex specular on mobile */
	EMobileSpecularMask SpecularMask;

	/** Environment blend mode */
	EMobileEnvironmentBlendMode EnvironmentBlendMode;

	/** Environment map color scale */
	FLinearColor EnvironmentColorScale;
};


/**
 * Mobile Mesh Vertex Shader Params
 */
class FMobileMeshVertexParams
{
public:

	/** True if sky lighting is enabled on this mesh */
	UBOOL bEnableSkyLight;

	/** Brightest light source direction */
	FVector BrightestLightDirection;

	/** Brightest light color */
	FLinearColor BrightestLightColor;

	/** Position of key objects in world space */
	FVector CameraPosition;
	FVector ObjectPosition;
	FBoxSphereBounds ObjectBounds;
	const FMatrix* LocalToWorld;

	/** The particle screen alignment */
	ESpriteScreenAlignment ParticleScreenAlignment;
};
