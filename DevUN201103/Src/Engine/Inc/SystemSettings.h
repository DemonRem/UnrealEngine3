/*=============================================================================
	SystemSettings.cpp: Unreal engine HW compat scalability system.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __SYSTEMSETTINGS_H__
#define __SYSTEMSETTINGS_H__

/*-----------------------------------------------------------------------------
	System settings and scalability options.
-----------------------------------------------------------------------------*/

/** 
 * Split screen level can be one of these.
 * Add more if needed (4 view split screen for example)
 */
enum ESplitScreenLevel
{
	SPLITSCREENLEVEL_1 = 0,
	SPLITSCREENLEVEL_2,
	SPLITSCREENLEVEL_MAX
};

/** Friendly settings can be one of these levels */
enum EFriendlySettingsLevel
{
	FSL_Custom,
	FSL_Level1,
	FSL_Level2,
	FSL_Level3,
	FSL_Level4,
	FSL_Level5,
	FSL_LevelCount = FSL_Level5, // The number of friendly levels supported
};

/**
 * Class that handles the translation between System Settings and friendly settings
 */
 struct FSystemSettingsFriendly
 {
	 /** ctor */
	 FSystemSettingsFriendly()
		 :TextureDetail(FSL_Custom)
		 ,WorldDetail(FSL_Custom)
		 ,ShadowDetail(FSL_Custom)
		 ,bUseVSync(FALSE)
		 ,bUseMSAA(FALSE)
		 ,bFullScreen(FALSE)
		 ,ScreenPercentage(100)
		 ,bUpscaleScreenPercentage(TRUE)
		 ,ResX(0)
		 ,ResY(0)
	 {}

	 /** Friendly values */
	 EFriendlySettingsLevel TextureDetail;
	 EFriendlySettingsLevel WorldDetail;
	 EFriendlySettingsLevel ShadowDetail;
	 UBOOL bUseVSync;
	 UBOOL bUseMSAA;
	 UBOOL bFullScreen;
	 INT ScreenPercentage;
	 UBOOL bUpscaleScreenPercentage;
	 INT ResX;
	 INT ResY;
 };


/** Augments TextureLODSettings with access to TextureLODGroups. */
struct FExposedTextureLODSettings : public FTextureLODSettings
{
public:
	/** @return		A handle to the indexed LOD group. */
	FTextureLODGroup& GetTextureLODGroup(INT GroupIndex)
	{
		check( GroupIndex >= 0 && GroupIndex < TEXTUREGROUP_MAX );
		return TextureLODGroups[GroupIndex];
	}
	/** @return		A handle to the indexed LOD group. */
	const FTextureLODGroup& GetTextureLODGroup(INT GroupIndex) const
	{
		check( GroupIndex >= 0 && GroupIndex < TEXTUREGROUP_MAX );
		return TextureLODGroups[GroupIndex];
	}
};

/** Encapsulates all the system settings that make up World Detail friendly settings */
struct FSystemSettingsDataWorldDetail
{
	FSystemSettingsDataWorldDetail();

	friend bool operator==(const FSystemSettingsDataWorldDetail& LHS, const FSystemSettingsDataWorldDetail& RHS)
	{
		// WRH - 2007/09/22 - This relies on the fact that the various settings are binary comparable
		return appMemcmp(&LHS, &RHS, sizeof(LHS)) == 0;
	}

	friend bool operator!=(const FSystemSettingsDataWorldDetail& LHS, const FSystemSettingsDataWorldDetail& RHS)
	{
		return !(LHS==RHS);
	}

	/** Current detail mode; determines whether components of actors should be updated/ ticked.	*/
	INT		DetailMode;
	/** Whether to override certain system settings to highest quality regardless of performance impact. */
	UBOOL	bUseMaxQualityMode;
	/** Whether to allow rendering of SpeedTree leaves.					*/
	UBOOL	bAllowSpeedTreeLeaves;
	/** Whether to allow rendering of SpeedTree fronds.					*/
	UBOOL	bAllowSpeedTreeFronds;
	/** Whether to allow static decals.									*/
	UBOOL	bAllowStaticDecals;
	/** Whether to allow dynamic decals.								*/
	UBOOL	bAllowDynamicDecals;
	/** Whether to allow decals that have not been placed in static draw lists and have dynamic view relevance */
	UBOOL	bAllowUnbatchedDecals;
	/** Scale factor for distance culling decals						*/
	FLOAT	DecalCullDistanceScale;
	/** Whether to allow dynamic lights.								*/
	UBOOL	bAllowDynamicLights;
	/** Whether to composte dynamic lights into light environments.		*/
	UBOOL	bUseCompositeDynamicLights;
	/** Whether to allow light environments to use SH lights for secondary lighting	*/
	UBOOL	bAllowSHSecondaryLighting;
	/**  Whether to allow directional lightmaps, which use the material's normal and specular. */
	UBOOL	bAllowDirectionalLightMaps;
	/** Whether to allow motion blur.									*/
	UBOOL	bAllowMotionBlur;
	/** Whether to allow motion blur to be paused.						*/
	UBOOL	bAllowMotionBlurPause;
	/** Whether to allow depth of field.								*/
	UBOOL	bAllowDepthOfField;
	/** Whether to allow ambient occlusion.								*/
	UBOOL	bAllowAmbientOcclusion;
	/** Whether to allow bloom.											*/
	UBOOL	bAllowBloom;
	/** Whether to allow light shafts.									*/
	UBOOL   bAllowLightShafts;
	/** Whether to allow distortion.									*/
	UBOOL	bAllowDistortion;
	/** Whether to allow distortion to use bilinear filtering when sampling the scene color during its apply pass	*/
	UBOOL	bAllowFilteredDistortion;
	/** Whether to allow dropping distortion on particles based on WorldInfo::bDropDetail. */
	UBOOL	bAllowParticleDistortionDropping;
	/** Whether to allow downsampled transluency.						*/
	UBOOL	bAllowDownsampledTranslucency;
	/** Whether to allow rendering of LensFlares.						*/
	UBOOL	bAllowLensFlares;
	/** Whether to allow fog volumes.									*/
	UBOOL	bAllowFogVolumes;
	/** Whether to allow floating point render targets to be used.		*/
	UBOOL	bAllowFloatingPointRenderTargets;
	/** Whether to allow the rendering thread to lag one frame behind the game thread.	*/
	UBOOL	bAllowOneFrameThreadLag;
	/** LOD bias for skeletal meshes.									*/
	INT		SkeletalMeshLODBias;
	/** LOD bias for particle systems.									*/
	INT		ParticleLODBias;
	/** Whether to use D3D11 when it's available.						*/
	UBOOL	bAllowD3D11;
	/** Whether to allow radial blur effects to render.					*/
	UBOOL	bAllowRadialBlur;
	/** Whether to allow sub-surface scattering to render.				*/
	UBOOL	bAllowSubsurfaceScattering;
	/** Whether to allow image reflections to render.					*/
	UBOOL	bAllowImageReflections;
	/** Whether to allow image reflections to be shadowed.				*/
	UBOOL	bAllowImageReflectionShadowing;
	/** State of the console variable MotionBlurSkinning.				*/
	INT		MotionBlurSkinning;
	/** Global tessellation factor multiplier */
	FLOAT	TessellationFactorMultiplier;
	/** Whether to use high-precision GBuffers. */
	UBOOL	bHighPrecisionGBuffers;
	/** Whether to keep separate translucency (for better Depth of Field), experimental */
	UBOOL	bAllowSeparateTranslucency;
};

/** Encapsulates all the system settings involving fractured static meshes. */
struct FSystemSettingsDataFracturedDetail
{
	FSystemSettingsDataFracturedDetail();

	friend bool operator==(const FSystemSettingsDataFracturedDetail& LHS, const FSystemSettingsDataFracturedDetail& RHS)
	{
		// WRH - 2007/09/22 - This relies on the fact that the various settings are binary comparable
		return appMemcmp(&LHS, &RHS, sizeof(LHS)) == 0;
	}
	friend bool operator!=(const FSystemSettingsDataFracturedDetail& LHS, const FSystemSettingsDataFracturedDetail& RHS)
	{
		return !(LHS==RHS);
	}

	/** Whether to allow fractured meshes to take damage.				*/
	UBOOL	bAllowFracturedDamage;
	/** Scales the game-specific number of fractured physics objects allowed.	*/
	FLOAT	NumFracturedPartsScale;
	/** Percent chance of a rigid body spawning after a fractured static mesh is damaged directly.  [0-1] */
	FLOAT	FractureDirectSpawnChanceScale;
	/** Percent chance of a rigid body spawning after a fractured static mesh is damaged by radial blast.  [0-1] */
	FLOAT	FractureRadialSpawnChanceScale;
	/** Distance scale for whether a fractured static mesh should actually fracture when damaged */
	FLOAT	FractureCullDistanceScale;
};


/** Encapsulates all the system settings that make up Shadow Detail friendly settings */
struct FSystemSettingsDataShadowDetail
{
	FSystemSettingsDataShadowDetail();

	friend bool operator==(const FSystemSettingsDataShadowDetail& LHS, const FSystemSettingsDataShadowDetail& RHS)
	{
		// WRH - 2007/09/22 - This relies on the fact that the various settings are binary comparable
		return appMemcmp(&LHS, &RHS, sizeof(LHS)) == 0;
	}

	friend bool operator!=(const FSystemSettingsDataShadowDetail& LHS, const FSystemSettingsDataShadowDetail& RHS)
	{
		return !(LHS==RHS);
	}

	/** Whether to allow dynamic shadows.								*/
	UBOOL	bAllowDynamicShadows;
	/** Whether to allow dynamic light environments to cast shadows.	*/
	UBOOL	bAllowLightEnvironmentShadows;
	/** Quality bias for projected shadow buffer filtering.	 Higher values use better quality filtering.		*/
	INT		ShadowFilterQualityBias;
	/** min dimensions (in texels) allowed for rendering shadow subject depths */
	INT		MinShadowResolution;
	/** min dimensions (in texels) allowed for rendering preshadow depths */
	INT		MinPreShadowResolution;
	/** max square dimensions (in texels) allowed for rendering shadow subject depths */
	INT		MaxShadowResolution;
	/** max square dimensions (in texels) allowed for rendering whole scene shadow depths */
	INT		MaxWholeSceneDominantShadowResolution;
	/** The ratio of subject pixels to shadow texels.					*/
	FLOAT	ShadowTexelsPerPixel;
	FLOAT	PreShadowResolutionFactor;
	/** Toggle Branching PCF implementation for projected shadows */
	UBOOL	bEnableBranchingPCFShadows;
	/** Whether to allow hardware filtering optimizations like hardware PCF and Fetch4. */
	UBOOL	bAllowHardwareShadowFiltering;
	/** Toggle extra geometry pass needed for culling shadows on emissive and backfaces */
	UBOOL	bAllowBetterModulatedShadows;
	/** hack to allow for foreground DPG objects to cast shadows on the world DPG */
	UBOOL	bEnableForegroundShadowsOnWorld;
	/** Whether to allow foreground DPG self-shadowing */
	UBOOL	bEnableForegroundSelfShadowing;
	/** Whether to allow whole scene dominant shadows. */
	UBOOL	bAllowWholeSceneDominantShadows;
	/** Whether to use safe and conservative shadow frustum creation that wastes some shadowmap space. */
	UBOOL	bUseConservativeShadowBounds;
	/** Radius, in shadowmap texels, of the filter disk */
	FLOAT	ShadowFilterRadius;
	/** Depth bias that is applied in the depth pass for all types of projected shadows except VSM */
	FLOAT	ShadowDepthBias;
	/** Higher values make the per object soft shadow comparison sharper, lower values make the transition softer. */
	FLOAT	PerObjectShadowTransition;
	/** Scale applied to the penumbra size of Cascaded Shadow Map splits, useful for minimizing the transition between splits. */
	FLOAT	CSMSplitPenumbraScale;
	/** Scale applied to the soft comparison transition distance of Cascaded Shadow Map splits, useful for minimizing the transition between splits. */
	FLOAT	CSMSplitSoftTransitionDistanceScale;
	/** Scale applied to the depth bias of Cascaded Shadow Map splits, useful for minimizing the transition between splits. */
	FLOAT	CSMSplitDepthBiasScale;
	/** WholeSceneDynamicShadowRadius to use when using CSM to preview unbuilt lighting from a directional light. */
	FLOAT	UnbuiltWholeSceneDynamicShadowRadius;
	/** NumWholeSceneDynamicShadowCascades to use when using CSM to preview unbuilt lighting from a directional light. */
	INT		UnbuiltNumWholeSceneDynamicShadowCascades;
	/** How many unbuilt light-primitive interactions there can be for a light before the light switches to whole scene shadows. */
	INT		WholeSceneShadowUnbuiltInteractionThreshold;
	/** Resolution in texel below which shadows are faded out. */
	INT		ShadowFadeResolution;
	/** Resolution in texel below which preshadows are faded out. */
	INT		PreShadowFadeResolution;
	/** Controls the rate at which shadows are faded out. */
	FLOAT	ShadowFadeExponent;
};


/** Encapsulates all the system settings that make up Texture Detail friendly settings */
struct FSystemSettingsDataTextureDetail
{
	FSystemSettingsDataTextureDetail();

	friend bool operator==(const FSystemSettingsDataTextureDetail& LHS, const FSystemSettingsDataTextureDetail& RHS)
	{
		// WRH - 2007/09/22 - This relies on the fact that the various settings are binary comparable
		return appMemcmp(&LHS, &RHS, sizeof(LHS)) == 0;
	}
	friend bool operator!=(const FSystemSettingsDataTextureDetail& LHS, const FSystemSettingsDataTextureDetail& RHS)
	{
		return !(LHS==RHS);
	}

	/** Global texture LOD settings.									*/
	FExposedTextureLODSettings TextureLODSettings;

	/** If enabled, texture will only be streamed in, not out.			*/
	UBOOL	bOnlyStreamInTextures;
	/** Maximum level of anisotropy used.								*/
	INT		MaxAnisotropy;
	/** Scene capture streaming texture update distance scalar.			*/
	FLOAT	SceneCaptureStreamingMultiplier;
};

/** Encapsulates all the system settings that make up VSync friendly settings */
struct FSystemSettingsDataVSync
{
	FSystemSettingsDataVSync();

	friend bool operator==(const FSystemSettingsDataVSync& LHS, const FSystemSettingsDataVSync& RHS)
	{
		return LHS.bUseVSync == RHS.bUseVSync;
	}
	friend bool operator!=(const FSystemSettingsDataVSync& LHS, const FSystemSettingsDataVSync& RHS)
	{
		return !(LHS==RHS);
	}

	/** Whether to use VSync or not.									*/
	UBOOL	bUseVSync;
};

/** Encapsulates all the system settings that make up screen percentage friendly settings */
struct FSystemSettingsDataScreenPercentage
{
	FSystemSettingsDataScreenPercentage();

	friend bool operator==(const FSystemSettingsDataScreenPercentage& LHS, const FSystemSettingsDataScreenPercentage& RHS)
	{
		return LHS.ScreenPercentage == RHS.ScreenPercentage;
	}
	friend bool operator!=(const FSystemSettingsDataScreenPercentage& LHS, const FSystemSettingsDataScreenPercentage& RHS)
	{
		return !(LHS==RHS);
	}

	/** Percentage of screen main view should take up.					*/
	FLOAT	ScreenPercentage;
	/** Whether to upscale the screen to take up the full front buffer.	*/
	UBOOL	bUpscaleScreenPercentage;
};

/** Encapsulates all the system settings that make up resolution friendly settings */
struct FSystemSettingsDataResolution
{
	FSystemSettingsDataResolution();

	friend bool operator==(const FSystemSettingsDataResolution& LHS, const FSystemSettingsDataResolution& RHS)
	{
		return LHS.ResX == RHS.ResX && LHS.ResY == RHS.ResY && LHS.bFullscreen == RHS.bFullscreen;
	}
	friend bool operator!=(const FSystemSettingsDataResolution& LHS, const FSystemSettingsDataResolution& RHS)
	{
		return !(LHS==RHS);
	}

	/** Screen X resolution */
	INT ResX;
	/** Screen Y resolution */
	INT ResY;
	/** Fullscreen */
	UBOOL bFullscreen;
};

/** Encapsulates all the system settings that make up MSAA settings */
struct FSystemSettingsDataMSAA
{
	FSystemSettingsDataMSAA();

	friend bool operator==(const FSystemSettingsDataMSAA& LHS, const FSystemSettingsDataMSAA& RHS)
	{
		return LHS.MaxMultiSamples == RHS.MaxMultiSamples;
	}
	friend bool operator!=(const FSystemSettingsDataMSAA& LHS, const FSystemSettingsDataMSAA& RHS)
	{
		return !(LHS==RHS);
	}

	/** 
	 * Indicates whether the hardware anti-aliasing is used
	 */
	UBOOL UsesMSAA() const
	{
		return MaxMultiSamples > 1 && (GRHIShaderPlatform == SP_PCD3D_SM5 || GRHIShaderPlatform == SP_PCD3D_SM3 && bAllowD3D9MSAA);
	}

	/** The maximum number of MSAA samples to use.						*/
	INT		MaxMultiSamples;
	UBOOL	bAllowD3D9MSAA;
};

/** Encapsulates all the system settings related to mesh data - skeletal and static*/
struct FSystemSettingsDataMesh
{
	FSystemSettingsDataMesh();

	friend bool operator==(const FSystemSettingsDataMesh& LHS, const FSystemSettingsDataMesh& RHS)
	{
		return (
			(LHS.bForceCPUAccessToGPUSkinVerts == RHS.bForceCPUAccessToGPUSkinVerts) &&
			(LHS.bDisableSkeletalInstanceWeights == RHS.bDisableSkeletalInstanceWeights)
			);
	}
	friend bool operator!=(const FSystemSettingsDataMesh& LHS, const FSystemSettingsDataMesh& RHS)
	{
		return !(LHS==RHS);
	}

	/** Whether to force CPU access to GPU skinned vertex data. */
	UBOOL bForceCPUAccessToGPUSkinVerts;
	/** Whether to disable instanced skeletal weights. */
	UBOOL bDisableSkeletalInstanceWeights;
};

#if WITH_MOBILE_RHI

/** Encapsulates all the system settings related to mobile rendering*/
struct FSystemSettingsDataMobile
{
	FSystemSettingsDataMobile();

	friend bool operator==(const FSystemSettingsDataMobile& LHS, const FSystemSettingsDataMobile& RHS)
	{
		return (
			(LHS.MobileFeatureLevel == RHS.MobileFeatureLevel) &&
			(LHS.bAllowMobileFog == RHS.bAllowMobileFog) &&
			(LHS.bAllowMobileSpecular == RHS.bAllowMobileSpecular) &&
			(LHS.bAllowMobileBumpOffset == RHS.bAllowMobileBumpOffset) &&
			(LHS.bAllowMobileNormalMapping == RHS.bAllowMobileNormalMapping) &&
			(LHS.bAllowMobileEnvMapping == RHS.bAllowMobileEnvMapping) &&
			(LHS.bAllowMobileRimLighting == RHS.bAllowMobileRimLighting) &&
			(LHS.bAllowMobileColorBlending == RHS.bAllowMobileColorBlending) &&
			(LHS.bAllowMobileVertexMovement == RHS.bAllowMobileVertexMovement) &&
			(LHS.bUsePreprocessedShaders == RHS.bUsePreprocessedShaders) &&
			(LHS.bFlashRedForUncachedShaders == RHS.bFlashRedForUncachedShaders) &&
			(LHS.bCachePreprocessedShaders == RHS.bCachePreprocessedShaders) &&
			(LHS.bWarmUpPreprocessedShaders == RHS.bWarmUpPreprocessedShaders) &&
			(LHS.bProfilePreprocessedShaders == RHS.bProfilePreprocessedShaders) &&
			(LHS.bUseCPreprocessorOnShaders == RHS.bUseCPreprocessorOnShaders) &&
			(LHS.bLoadCPreprocessedShaders == RHS.bLoadCPreprocessedShaders) &&
			(LHS.bSharePixelShaders == RHS.bSharePixelShaders) &&
			(LHS.bShareVertexShaders == RHS.bShareVertexShaders) &&
			(LHS.bShareShaderPrograms == RHS.bShareShaderPrograms) &&
			(LHS.bEnableMSAA == RHS.bEnableMSAA) &&
			(LHS.MobileContentScaleFactor == RHS.MobileContentScaleFactor) &&
			(LHS.MobileLODBias == RHS.MobileLODBias) &&
			(LHS.MobileBoneCount == RHS.MobileBoneCount) &&
			(LHS.MobileBoneWeightCount == RHS.MobileBoneWeightCount) &&
			(LHS.MobileVertexScratchBufferSize == RHS.MobileVertexScratchBufferSize) &&
			(LHS.MobileIndexScratchBufferSize == RHS.MobileIndexScratchBufferSize)
			);
	}
	friend bool operator!=(const FSystemSettingsDataMobile& LHS, const FSystemSettingsDataMobile& RHS)
	{
		return !(LHS==RHS);
	}

	/** The baseline feature level of the device */
	INT MobileFeatureLevel;

	/** Whether to allow fog on mobile*/
	UBOOL bAllowMobileFog;

	/** Whether to allow vertex specular on mobile */
	UBOOL bAllowMobileSpecular;

	/** Whether to allow bump offset on mobile */
	UBOOL bAllowMobileBumpOffset;

	/** Whether to allow normal mapping on mobile */
	UBOOL bAllowMobileNormalMapping;

	/** Whether to allow environment mapping on mobile */
	UBOOL bAllowMobileEnvMapping;

	/** Whether to allow rim lighting on mobile */
	UBOOL bAllowMobileRimLighting;

	/** Whether to allow color blending on mobile */
	UBOOL bAllowMobileColorBlending;

	/** Whether to allow vertex movement on mobile */
	UBOOL bAllowMobileVertexMovement;

	/** Whether to use preprocessed shaders on mobile */
	UBOOL bUsePreprocessedShaders;

	/** Whether to flash the screen red (non-final release only) when a cached shader is not found at runtime */
	UBOOL bFlashRedForUncachedShaders;

	/** Whether to issue a "warm-up" draw call for mobile shaders as they are compiled */
	UBOOL bWarmUpPreprocessedShaders;

	/** Whether to dump out preprocessed shaders for mobile as they are encountered/compiled */
	UBOOL bCachePreprocessedShaders;

	/** Whether to run dumped out preprocessed shaders through the shader profiler */
	UBOOL bProfilePreprocessedShaders;

	/** Whether to run the C preprocessor on shaders  */
	UBOOL bUseCPreprocessorOnShaders;

	/** Whether to load the C preprocessed source  */
	UBOOL bLoadCPreprocessedShaders;
	
	/** Whether to share pixel shaders across multiple unreal shaders  */
	UBOOL bSharePixelShaders;

	/** Whether to share vertex shaders across multiple unreal shaders  */
	UBOOL bShareVertexShaders;

	/** Whether to share shaders program across multiple unreal shaders  */
	UBOOL bShareShaderPrograms;

	/** Whether to enable MSAA, if the OS supports it */
	UBOOL bEnableMSAA;

	/** The default global content scale factor to use on device (largely iOS specific) */
	FLOAT MobileContentScaleFactor;

	/** How much to bias all texture mip levels on mobile (usually 0 or negative) */
	FLOAT MobileLODBias;

	/** The maximum number of bones supported for skinning */
	INT MobileBoneCount;

	/** The maximum number of bones influences per vertex supported for skinning */
	INT MobileBoneWeightCount;

	/** The size of the scratch buffer for vertices (in kB) */
	INT MobileVertexScratchBufferSize;
	/** The size of the scratch buffer for indices (in kB) */
	INT MobileIndexScratchBufferSize;
};

#endif

#if WITH_APEX

/** Encapsulates all the system settings related to APEX */
struct FSystemSettingsDataApex
{
	FSystemSettingsDataApex();

	friend bool operator==(const FSystemSettingsDataApex& LHS, const FSystemSettingsDataApex& RHS)
	{
		return (
			(LHS.ApexLODResourceBudget == RHS.ApexLODResourceBudget) &&
			(LHS.ApexDestructionMaxChunkIslandCount == RHS.ApexDestructionMaxChunkIslandCount) &&
			(LHS.ApexDestructionMaxChunkSeparationLOD == RHS.ApexDestructionMaxChunkSeparationLOD)
			);
	}
	friend bool operator!=(const FSystemSettingsDataApex& LHS, const FSystemSettingsDataApex& RHS)
	{
		return !(LHS==RHS);
	}

	/** Resource budget for APEX LOD. Higher values indicate the system can handle more APEX load.*/
	FLOAT ApexLODResourceBudget;

	/** The maximum number of active PhysX actors which represent dynamic groups of chunks (islands).  
		If a fracturing event would cause more islands to be created, then oldest islands are released 
		and the chunks they represent destroyed.*/
	INT ApexDestructionMaxChunkIslandCount;

	/** Every destructible asset defines a min and max lifetime, and maximum separation distance for its chunks.
		Chunk islands are destroyed after this time or separation from their origins. This parameter sets the
		lifetimes and max separations within their min-max ranges. The valid range is [0,1]. */
	FLOAT ApexDestructionMaxChunkSeparationLOD;

	/** If TRUE, allow APEX clothing fetch (skinning etc) to be done on multiple threads */
	UBOOL bEnableParallelApexClothingFetch;
};

#endif


/** 
 * Struct that holds the actual data for the system settings. 
 *
 * Uses the insane derivation for backwards compatibility purposes only. Would be cleaner to use composition.
 */
struct FSystemSettingsData 
	: public FSystemSettingsDataWorldDetail
	, public FSystemSettingsDataTextureDetail
	, public FSystemSettingsDataVSync
	, public FSystemSettingsDataScreenPercentage
	, public FSystemSettingsDataResolution
	, public FSystemSettingsDataMSAA
	, public FSystemSettingsDataShadowDetail
	, public FSystemSettingsDataFracturedDetail
	, public FSystemSettingsDataMesh
#if WITH_MOBILE_RHI
	, public FSystemSettingsDataMobile
#endif
#if WITH_APEX
	, public FSystemSettingsDataApex
#endif
{
	/** ctor */
	FSystemSettingsData();

	/** loads settings from the given section in the given ini */
	void LoadFromIni(const TCHAR* IniSection, const TCHAR* IniFilename = GEngineIni, UBOOL bAllowMissingValues = TRUE, UBOOL* FoundValues=NULL);

	/** saves settings to the given section in the engine ini */
	void SaveToIni(const TCHAR* IniSection);

	/** Dumps the settings to the log file */
	void Dump(const TCHAR* DumpHeading);

private:
	/**
	 * Returns a string for the specified texture group LOD settings to the specified ini.
	 *
	 * @param	TextureGroupID		Index/enum of the group
	 * @param	GroupName			String representation of the texture group
	 */
	FString GetLODGroupString( TextureGroup TextureGroupID, const TCHAR* GroupName );

	void WriteTextureLODGroupsToIni(const TCHAR* IniSection);
	void WriteTextureLODGroupToIni(TextureGroup TextureGroupID, const TCHAR* GroupName, const TCHAR* IniSection);

	void DumpTextureLODGroups();
	void DumpTextureLODGroup(TextureGroup TextureGroupID, const TCHAR* GroupName);
};


/**
 * Class that actually holds the current system setttings
 *
 * Derive from FSystemSettingsData instead of holding one purely for backwards
 * compatibility reasons (each data element used to be a member of the class).
 */
class FSystemSettings : public FSystemSettingsData, public FExec, private FNoncopyable
{
public:
	/** Constructor, initializing all member variables. */
	FSystemSettings();

	/**
	 * Initializes system settings and included texture LOD settings.
	 *
	 * @param bSetupForEditor	Whether to initialize settings for Editor
	 */
	void Initialize( UBOOL bSetupForEditor );

	/**
	 * Sets new system settings (optionally writes out to the ini).
	 */
	void ApplyNewSettings(const FSystemSettingsData& NewSettings, UBOOL bWriteToIni);

	/** Applies setting overrides based on command line options. */
	void ApplyOverrides();

	/**
	 * Returns the current friendly settings as defined by the current system settings.
	 *
	 * We read from the ini because some settings are deferred until the next reboot.
	 */
	FSystemSettingsFriendly ConvertToFriendlySettings(bool bReadSettingsFromIni = TRUE);

	/**
	 * Sets the system settings using the given friendly settings (optionally writes out to the ini).
	 */
	void ApplyFriendlySettings(const FSystemSettingsFriendly& FriendlySettings, UBOOL bWriteToIni);

	/** 
	 * Ensures that the correct settings are being used based on split screen type.
	 */
	void UpdateSplitScreenSettings(ESplitScreenType NewSplitScreenType);

	/** 
	 * Sets the resolution and writes the values to Ini if changed but does not apply the changes (eg resize the viewport).
	 */
	void SetResolution(INT InSizeX, INT InSizeY, UBOOL InFullscreen);

	const FSystemSettingsData& GetDefaultSettings(EFriendlySettingsLevel Level);

	/**
	 * Exec handler implementation.
	 *
	 * @param Cmd	Command to parse
	 * @param Ar	Output device to log to
	 *
	 * @return TRUE if command was handled, FALSE otherwise
	 */
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );

	/**
	 * Scale X,Y offset/size of screen coordinates if the screen percentage is not at 100%
	 *
	 * @param X - in/out X screen offset
	 * @param Y - in/out Y screen offset
	 * @param SizeX - in/out X screen size
	 * @param SizeY - in/out Y screen size
	 */
	void ScaleScreenCoords( INT& X, INT& Y, UINT& SizeX, UINT& SizeY );

	/**
	 * Reverses the scale and offset done by ScaleScreenCoords() 
	 * if the screen percentage is not 100% and upscaling is allowed.
	 *
	 * @param OriginalX - out X screen offset
	 * @param OriginalY - out Y screen offset
	 * @param OriginalSizeX - out X screen size
	 * @param OriginalSizeY - out Y screen size
	 * @param InX - in X screen offset
	 * @param InY - in Y screen offset
	 * @param InSizeX - in X screen size
	 * @param InSizeY - in Y screen size
	 */
	void UnScaleScreenCoords( 
		INT &OriginalX, INT &OriginalY, 
		UINT &OriginalSizeX, UINT &OriginalSizeY, 
		FLOAT InX, FLOAT InY, 
		FLOAT InSizeX, FLOAT InSizeY);

	/** 
	 * Indicates whether upscaling is needed 
	 */
	UBOOL NeedsUpscale() const;

	/**
	 * DO NOT USE - This returns a value that is cached prior to GIsEditor
	 * being set and, therefore, is not guaranteed to match GIsEditor
	 */
	UBOOL GetIsEditor() const { return bIsEditor; }
	
	/** 
	 * Container for renderthread version of FSystemSettings 
	 */
	struct FRenderThreadSettings
	{
		UBOOL	bAllowMotionBlur;
		UBOOL	bAllowAmbientOcclusion;
		UBOOL	bAllowDynamicShadows;
		UBOOL	bAllowHardwareShadowFiltering;
		UBOOL	bAllowFogVolumes;
		UBOOL	bAllowSubsurfaceScattering;
		UBOOL	bAllowImageReflections;
		UBOOL	bAllowImageReflectionShadowing;
		UBOOL	bHighPrecisionGBuffers;
		UBOOL	bAllowSeparateTranslucencyRT;
		FLOAT	TessellationFactorMultiplier;
		INT		MinShadowResolution;
		INT		MaxShadowResolution;
		INT		MaxWholeSceneDominantShadowResolution;
		INT		bAllowUnbatchedDecals;
		INT		MotionBlurSkinningRT;

		// default constructor
		FRenderThreadSettings()
		{
		}

		// constructor
		// Copy over the settings from FSystemSettings
		FRenderThreadSettings(const FSystemSettings& Src)
		{
			bAllowMotionBlur				= Src.bAllowMotionBlur;
			bAllowAmbientOcclusion			= Src.bAllowAmbientOcclusion;
			bAllowDynamicShadows			= Src.bAllowDynamicShadows;
			bAllowHardwareShadowFiltering	= Src.bAllowHardwareShadowFiltering;
			bAllowFogVolumes				= Src.bAllowFogVolumes;
			bAllowSubsurfaceScattering		= Src.bAllowSubsurfaceScattering;
			bHighPrecisionGBuffers			= Src.bHighPrecisionGBuffers;
			bAllowImageReflections			= Src.bAllowImageReflections;
			bAllowImageReflectionShadowing	= Src.bAllowImageReflectionShadowing;
			TessellationFactorMultiplier	= Src.TessellationFactorMultiplier;
			MinShadowResolution				= Src.MinShadowResolution;
			MaxShadowResolution				= Src.MaxShadowResolution;
			MaxWholeSceneDominantShadowResolution = Src.MaxWholeSceneDominantShadowResolution;
			bAllowUnbatchedDecals			= Src.bAllowUnbatchedDecals;
			MotionBlurSkinningRT			= Src.MotionBlurSkinning;
			bAllowSeparateTranslucencyRT	= Src.bAllowSeparateTranslucency;
		}

#if WITH_MOBILE_RHI
		// all system settings are needed by mobile rendering thread
		FSystemSettingsDataMobile MobileSettings;
#endif
	};

	/** 
	 * Current renderthread state 
	 */
	FRenderThreadSettings	RenderThreadSettings;

private:
	/** Since System Settings is called into before GIsEditor is set, we must cache this value. */
	UBOOL bIsEditor;

	/** Split screen level that is currently being used. */
	ESplitScreenLevel CurrentSplitScreenLevel;

	/**
	 * These are the default system settings for each supported compatibility level.
	 * They are initialized from engine ini.
	 */ 
	FSystemSettingsData Defaults[FSL_LevelCount][SPLITSCREENLEVEL_MAX];

	/**
	 * Loads settings from the ini. (purposely override the inherited name so people can't accidentally call it.)
	 */
	void LoadFromIni();

	/**
	 * Saves current settings to the ini. (purposely override the inherited name so people can't accidentally call it.)
	 */
	void SaveToIni();

	/**
	 * Reads a single entry and parses it into the group array.
	 *
	 * @param	TextureGroupID		Index/enum of group to parse
	 * @param	MinLODSize			Minimum size, in pixels, below which the code won't bias.
	 * @param	MaxLODSize			Maximum size, in pixels, above which the code won't bias.
	 * @param	LODBias				Group LOD bias.
	 * @param	MipGenSettings		Defines how the the mip-map generation works, e.g. sharpening
	 */
	void SetTextureLODGroup(TextureGroup TextureGroupID, int MinLODSize, INT MaxLODSize, INT LODBias, TextureMipGenSettings MipGenSettings);

	/**
	 * Writes all texture group LOD settings to the specified ini.
	 *
	 * @param	IniFilename			The .ini file to save to.
	 * @param	IniSection			The .ini section to save to.
	 */
	void WriteTextureLODGroupsToIni(const TCHAR* IniSection);

	/**
	 * Writes the specified texture group LOD settings to the specified ini.
	 *
	 * @param	TextureGroupID		Index/enum of the group to parse
	 * @param	GroupName			String representation of the texture group, to be used as the ini key.
	 * @param	IniSection			The .ini section to save to.
	 */
	void WriteTextureLODGroupToIni(TextureGroup TextureGroupID, const TCHAR* GroupName, const TCHAR* IniSection);

	/**
	 * Recreates texture resources and drops mips.
	 *
	 * @return		TRUE if the settings were applied, FALSE if they couldn't be applied immediately.
	 */
	UBOOL UpdateTextureStreaming();

	/**
	 * Writes current settings to the logfile
	 // WRH - 2007/08/29 - Can't be const due to peculiar use of macros
	 */
	void DumpCurrentSettings(const TCHAR* DumpHeading);

	/**
	 * Helper for ApplyNewSettings when the engine is running. Applies the changes needed for the runtime system.
	 *
	 * We can assume the game is running if this code is called.
	 */
	void ApplySettingsAtRuntime(const FSystemSettingsData& NewSettings, UBOOL bWriteToInii);

	/** 
	 * Command run on the rendering thread which sets RT specific system settings
	 */
	void ApplySystemSettings_RenderingThread(const FSystemSettings::FRenderThreadSettings& NewSettings, UBOOL bInUpdateRenderTargets);

	/** 
	 * Makes System Settings take effect on the rendering thread 
	 */
	void ApplySystemSettingsToRenderThread(UBOOL bInUpdateRenderTargets = FALSE);
};

/**
 * Returns the friendly settings that correspond to the given system settings.
 */
FSystemSettingsFriendly ConvertToFriendlySettings(const FSystemSettingsData& Settings);
/**
 * Returns the system settings that correspond to the given friendly settings.
 * The SourceForCustomSettings parameter specifies how to initialize the data for friendly settings that are "Custom"
 * level. Since there's no direct translation, it just uses the existing settings from the supplied data.
 */
FSystemSettingsData ConvertToSystemSettings(const FSystemSettingsFriendly& FriendlySettings, const FSystemSettingsData& SourceForCustomSettings = FSystemSettingsData());


/**
 * Global system settings accessor
 */
extern FSystemSettings GSystemSettings;

#endif // __SYSTEMSETTINGS_H__
