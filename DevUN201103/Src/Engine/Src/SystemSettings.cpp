/*=============================================================================
	ScalabilityOptions.cpp: Unreal engine HW compat scalability system.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnTerrain.h"
#include "EngineSpeedTreeClasses.h"
#include "EngineDecalClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "SceneRenderTargets.h"
#if IPHONE
	#include "IPhoneObjCWrapper.h"
#endif

/*-----------------------------------------------------------------------------
	FSystemSettings
-----------------------------------------------------------------------------*/

/** Global accessor */
FSystemSettings GSystemSettings;

/**
 * Helpers for writing to specific ini sections
 */
#if _WINDLL
	// Play In Browser needs different SystemSettings
	static const TCHAR* GIniSectionGame = TEXT("SystemSettingsPIB");
#else
	static const TCHAR* GIniSectionGame = TEXT("SystemSettings");
#endif
static const TCHAR* GIniSectionEditor = TEXT("SystemSettingsEditor");

static inline const TCHAR* GetSectionName(UBOOL bIsEditor)
{
#if MOBILE
	return appGetMobileSystemSettingsSectionName();
#else

	// if we are cooking, look for an override on the commandline
	FString OverrideSubName;
	if (Parse(appCmdLine(), TEXT("-SystemSettings="), OverrideSubName))
	{
		// append it to SystemSettings, unless it starts with SystemSettings
		static TCHAR ReturnedOverrideName[256] = TEXT("SystemSettings");
		// cache how long the SystemSettings is
		static INT SystemSettingsStrLen = appStrlen(ReturnedOverrideName);
		if (appStrnicmp(*OverrideSubName, TEXT("SystemSettings"), SystemSettingsStrLen) == 0)
		{
			appStrcpy(ReturnedOverrideName + SystemSettingsStrLen, ARRAY_COUNT(ReturnedOverrideName), (*OverrideSubName + SystemSettingsStrLen));
		}
		else
		{
			appStrcpy(ReturnedOverrideName + SystemSettingsStrLen, ARRAY_COUNT(ReturnedOverrideName), *OverrideSubName);
		}
		return ReturnedOverrideName;
	}

	// return the proper section for using editor or not
	return bIsEditor ? GIniSectionEditor : GIniSectionGame;
#endif
}


/**
 * Helper for ConvertToFriendlySettings
 */
template <class FSystemSettingsDataSubClass>
EFriendlySettingsLevel FindMatchingSystemSetting( const FSystemSettingsData& Settings )
{
	// Search the Defaults in reverse order in case any settings are equal, we'll always return the larger value.
	// Note that identical default values for more than one level is not recommended because it makes makes the
	// mapping of system to friendly settings not one-to-one.
	for (INT IntLevel=FSL_Level5;IntLevel>=FSL_Level1;--IntLevel)
	{
		EFriendlySettingsLevel Level = (EFriendlySettingsLevel)IntLevel;
		const FSystemSettingsDataSubClass& DefaultSettings = GSystemSettings.GetDefaultSettings(Level);
		if (DefaultSettings == (const FSystemSettingsDataSubClass&)Settings)
		{
			return Level;
		}
	}
	return FSL_Custom;
}

/**
 * Uses the default settings mappings to return a friendly setting corresponding to a system setting.
 */
FSystemSettingsFriendly ConvertToFriendlySettings( const FSystemSettingsData& Settings )
{
	FSystemSettingsFriendly Result;

	// Texture Detail
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataTextureDetail>(Settings);
		Result.TextureDetail = Level;
	}

	// World Detail
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataWorldDetail>(Settings);
		Result.WorldDetail = Level;
	}

	// Shadow Detail
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataShadowDetail>(Settings);
		Result.ShadowDetail = Level;
	}

	// VSync
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataVSync>(Settings);
		const FSystemSettingsData* Data = &GSystemSettings;
		if (Level != FSL_Custom)
		{
			Data = &GSystemSettings.GetDefaultSettings(Level);
		}
		Result.bUseVSync = Data->bUseVSync;
	}

	// Screen Percentage
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataScreenPercentage>(Settings);
		const FSystemSettingsData* Data = &GSystemSettings;
		if (Level != FSL_Custom)
		{
			Data = &GSystemSettings.GetDefaultSettings(Level);
		}
		Result.ScreenPercentage = (INT)Data->ScreenPercentage;
		Result.bUpscaleScreenPercentage = Data->bUpscaleScreenPercentage;
	}

	// Resolution
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataResolution>(Settings);
		const FSystemSettingsData* Data = &GSystemSettings;
		if (Level != FSL_Custom)
		{
			Data = &GSystemSettings.GetDefaultSettings(Level);
		}
		Result.ResX = Data->ResX;
		Result.ResY = Data->ResY;
		Result.bFullScreen = Data->bFullscreen;
	}

	// MSAA
	{
		EFriendlySettingsLevel Level = FindMatchingSystemSetting<FSystemSettingsDataMSAA>(Settings);
		const FSystemSettingsData* Data = &GSystemSettings;
		if (Level != FSL_Custom)
		{
			Data = &GSystemSettings.GetDefaultSettings(Level);
		}
		Result.bUseMSAA = Data->UsesMSAA();
	}

	return Result;
}

/**
 * Uses the default settings mappings to return a system setting corresponding to a friendly setting.
 * The SourceForCustomSettings parameter specifies how to initialize the data for friendly settings that are "Custom"
 * level. Since there's no direct translation, it just uses the existing settings from the supplied data.
 */
FSystemSettingsData ConvertToSystemSettings(const FSystemSettingsFriendly& FriendlySettings, const FSystemSettingsData& SourceForCustomSettings)
{
	FSystemSettingsData Result = SourceForCustomSettings;

	// Texture Detail
	{
		// Can only set these values if they are not customized!
		if (FriendlySettings.TextureDetail != FSL_Custom)
		{
			(FSystemSettingsDataTextureDetail&)Result = (const FSystemSettingsDataTextureDetail&)GSystemSettings.GetDefaultSettings(FriendlySettings.TextureDetail);
		}
	}

	// World Detail
	{
		// Can only set these values if they are not customized!
		if (FriendlySettings.WorldDetail != FSL_Custom)
		{
			(FSystemSettingsDataWorldDetail&)Result = (const FSystemSettingsDataWorldDetail&)GSystemSettings.GetDefaultSettings(FriendlySettings.WorldDetail);
		}
	}

	// Shadow Detail
	{
		// Can only set these values if they are not customized!
		if (FriendlySettings.ShadowDetail != FSL_Custom)
		{
			(FSystemSettingsDataShadowDetail&)Result = (const FSystemSettingsDataShadowDetail&)GSystemSettings.GetDefaultSettings(FriendlySettings.ShadowDetail);
		}
	}

	// VSync
	{
		Result.bUseVSync = FriendlySettings.bUseVSync;
	}

	// Screen Percentage
	{
		Result.ScreenPercentage = FriendlySettings.ScreenPercentage;
		Result.bUpscaleScreenPercentage = FriendlySettings.bUpscaleScreenPercentage;
	}

	// Resolution
	{
		Result.ResX = FriendlySettings.ResX;
		Result.ResY = FriendlySettings.ResY;
		Result.bFullscreen = FriendlySettings.bFullScreen;
	}

	// MSAA
	{
		Result.MaxMultiSamples = FriendlySettings.bUseMSAA ? 4 : 1;
	}

	return Result;
}


// allow for mobile only platform settings, separated out so they don't need to be specified for 
// non-mobile platforms
#if WITH_MOBILE_RHI

#define MOBILE_SWITCHES	\
		{ TEXT("MobileFog")								, &bAllowMobileFog },						\
		{ TEXT("MobileSpecular")						, &bAllowMobileSpecular },					\
		{ TEXT("MobileBumpOffset")						, &bAllowMobileBumpOffset },				\
		{ TEXT("MobileNormalMapping")					, &bAllowMobileNormalMapping },				\
		{ TEXT("MobileEnvMapping")						, &bAllowMobileEnvMapping },				\
		{ TEXT("MobileRimLighting")						, &bAllowMobileRimLighting },				\
		{ TEXT("MobileColorBlending")					, &bAllowMobileColorBlending },				\
		{ TEXT("MobileVertexMovement")					, &bAllowMobileVertexMovement },			\
		{ TEXT("MobileUsePreprocessedShaders")			, &bUsePreprocessedShaders },				\
		{ TEXT("MobileFlashRedForUncachedShaders")		, &bFlashRedForUncachedShaders},			\
		{ TEXT("MobileWarmUpPreprocessedShaders")		, &bWarmUpPreprocessedShaders },			\
		{ TEXT("MobileCachePreprocessedShaders")		, &bCachePreprocessedShaders },				\
		{ TEXT("MobileProfilePreprocessedShaders")		, &bProfilePreprocessedShaders },			\
		{ TEXT("MobileUseCPreprocessorOnShaders")		, &bUseCPreprocessorOnShaders },			\
		{ TEXT("MobileLoadCPreprocessedShaders")		, &bLoadCPreprocessedShaders },				\
		{ TEXT("MobileSharePixelShaders")				, &bSharePixelShaders },					\
		{ TEXT("MobileShareVertexShaders")				, &bShareVertexShaders },					\
		{ TEXT("MobileShareShaderPrograms")				, &bShareShaderPrograms },					\
		{ TEXT("MobileEnableMSAA")						, &bEnableMSAA },							\

#define MOBILE_FLOAT_VALUES \
		{ TEXT("MobileLODBias")							, &MobileLODBias },							\
		{ TEXT("MobileContentScaleFactor")				, &MobileContentScaleFactor },				\

#define MOBILE_INT_VALUES \
		{ TEXT("MobileFeatureLevel")					, &MobileFeatureLevel },					\
		{ TEXT("MobileBoneCount")						, &MobileBoneCount },						\
		{ TEXT("MobileBoneWeightCount")					, &MobileBoneWeightCount },					\
		{ TEXT("MobileVertexScratchBufferSize")			, &MobileVertexScratchBufferSize },			\
		{ TEXT("MobileIndexScratchBufferSize")			, &MobileIndexScratchBufferSize },			\

#else

// non-mobile, just have nothing
#define MOBILE_SWITCHES
#define MOBILE_FLOAT_VALUES
#define MOBILE_INT_VALUES

#endif

// allow for APEX only settings
#if WITH_APEX

#define APEX_SWITCHES	\
		{ TEXT("bEnableParallelApexClothingFetch")		, &bEnableParallelApexClothingFetch},		\

#define APEX_FLOAT_VALUES \
		{ TEXT("ApexLODResourceBudget")					, &ApexLODResourceBudget},					\
		{ TEXT("ApexDestructionMaxChunkSeparationLOD")	, &ApexDestructionMaxChunkSeparationLOD},	\

#define APEX_INT_VALUES \
		{ TEXT("ApexDestructionMaxChunkIslandCount")	, &ApexDestructionMaxChunkIslandCount},		\

#else

// non-APEX, just have nothing
#define APEX_SWITCHES
#define APEX_FLOAT_VALUES
#define APEX_INT_VALUES

#endif



#define PROPERTY_TEXT_TO_ADDRESS_MAPPING											\
	/** Helper struct for all switches/ boolean config values. */					\
	struct { const TCHAR* Name; UBOOL* SwitchPtr; } Switches[] =					\
	{																				\
		{ TEXT("StaticDecals")				, &bAllowStaticDecals },				\
		{ TEXT("DynamicDecals")				, &bAllowDynamicDecals },				\
		{ TEXT("UnbatchedDecals")			, &bAllowUnbatchedDecals },				\
		{ TEXT("DynamicLights")				, &bAllowDynamicLights },				\
		{ TEXT("DynamicShadows")			, &bAllowDynamicShadows },				\
		{ TEXT("LightEnvironmentShadows")	, &bAllowLightEnvironmentShadows },		\
		{ TEXT("CompositeDynamicLights")	, &bUseCompositeDynamicLights },		\
		{ TEXT("SHSecondaryLighting")		, &bAllowSHSecondaryLighting },		\
		{ TEXT("DirectionalLightmaps")		, &bAllowDirectionalLightMaps },		\
		{ TEXT("MotionBlur")				, &bAllowMotionBlur },					\
		{ TEXT("MotionBlurPause")			, &bAllowMotionBlurPause },				\
		{ TEXT("DepthOfField")				, &bAllowDepthOfField },				\
		{ TEXT("AmbientOcclusion")			, &bAllowAmbientOcclusion },			\
		{ TEXT("Bloom")						, &bAllowBloom },						\
		{ TEXT("bAllowLightShafts")			, &bAllowLightShafts },					\
		{ TEXT("Distortion")				, &bAllowDistortion },					\
		{ TEXT("FilteredDistortion")		, &bAllowFilteredDistortion },			\
		{ TEXT("DropParticleDistortion")	, &bAllowParticleDistortionDropping },	\
		{ TEXT("bAllowDownsampledTranslucency"), &bAllowDownsampledTranslucency },	\
		{ TEXT("SpeedTreeLeaves")			, &bAllowSpeedTreeLeaves },				\
		{ TEXT("bUseMaxQualityMode")		, &bUseMaxQualityMode },				\
		{ TEXT("SpeedTreeFronds")			, &bAllowSpeedTreeFronds },				\
		{ TEXT("OnlyStreamInTextures")		, &bOnlyStreamInTextures },				\
		{ TEXT("LensFlares")				, &bAllowLensFlares },					\
		{ TEXT("FogVolumes")				, &bAllowFogVolumes },					\
		{ TEXT("FloatingPointRenderTargets"), &bAllowFloatingPointRenderTargets },	\
		{ TEXT("OneFrameThreadLag")			, &bAllowOneFrameThreadLag },			\
		{ TEXT("UseVsync")					, &bUseVSync},							\
		{ TEXT("UpscaleScreenPercentage")	, &bUpscaleScreenPercentage },			\
		{ TEXT("Fullscreen")				, &bFullscreen},						\
		{ TEXT("bAllowD3D9MSAA")			, &bAllowD3D9MSAA },					\
		{ TEXT("AllowD3D11")				, &bAllowD3D11 },						\
		{ TEXT("AllowRadialBlur")			, &bAllowRadialBlur },					\
		{ TEXT("AllowSubsurfaceScattering")	, &bAllowSubsurfaceScattering },		\
		{ TEXT("AllowImageReflections")		, &bAllowImageReflections },			\
		{ TEXT("AllowImageReflectionShadowing"),&bAllowImageReflectionShadowing },	\
		{ TEXT("bEnableBranchingPCFShadows"), &bEnableBranchingPCFShadows },		\
		{ TEXT("bAllowHardwareShadowFiltering"), &bAllowHardwareShadowFiltering },		\
		{ TEXT("bAllowBetterModulatedShadows"), &bAllowBetterModulatedShadows },	\
		{ TEXT("bEnableForegroundShadowsOnWorld"), &bEnableForegroundShadowsOnWorld },	\
		{ TEXT("bEnableForegroundSelfShadowing"), &bEnableForegroundSelfShadowing },	\
		{ TEXT("bAllowWholeSceneDominantShadows"), &bAllowWholeSceneDominantShadows },	\
		{ TEXT("bUseConservativeShadowBounds"), &bUseConservativeShadowBounds },	\
		{ TEXT("bAllowFracturedDamage")		, &bAllowFracturedDamage },				\
		{ TEXT("bForceCPUAccessToGPUSkinVerts")		, &bForceCPUAccessToGPUSkinVerts },		\
		{ TEXT("bDisableSkeletalInstanceWeights")	, &bDisableSkeletalInstanceWeights },	\
		{ TEXT("HighPrecisionGBuffers")		, &bHighPrecisionGBuffers },			\
		{ TEXT("bAllowSeparateTranslucency")		, &bAllowSeparateTranslucency },			\
		MOBILE_SWITCHES																\
		APEX_SWITCHES																\
	};																				\
																					\
	/** Helper struct for all integer config values */								\
	struct { const TCHAR* Name; INT* IntValuePtr; } IntValues[] =					\
	{																				\
		{ TEXT("SkeletalMeshLODBias")		, &SkeletalMeshLODBias },				\
		{ TEXT("ParticleLODBias")			, &ParticleLODBias },					\
		{ TEXT("DetailMode")				, &DetailMode },						\
		{ TEXT("ShadowFilterQualityBias")	, &ShadowFilterQualityBias },			\
		{ TEXT("MaxAnisotropy")				, &MaxAnisotropy },						\
		{ TEXT("MaxMultiSamples")		, &MaxMultiSamples },				\
		{ TEXT("MinShadowResolution")		, &MinShadowResolution },				\
		{ TEXT("MinPreShadowResolution")	, &MinPreShadowResolution },			\
		{ TEXT("MaxShadowResolution")		, &MaxShadowResolution },				\
		{ TEXT("MaxWholeSceneDominantShadowResolution"), &MaxWholeSceneDominantShadowResolution },	\
		{ TEXT("ResX")						, &ResX },								\
		{ TEXT("ResY")						, &ResY },								\
		{ TEXT("UnbuiltNumWholeSceneDynamicShadowCascades")	, &UnbuiltNumWholeSceneDynamicShadowCascades },	\
		{ TEXT("WholeSceneShadowUnbuiltInteractionThreshold"), &WholeSceneShadowUnbuiltInteractionThreshold }, \
		{ TEXT("ShadowFadeResolution")		, &ShadowFadeResolution },				\
		{ TEXT("PreShadowFadeResolution")	, &PreShadowFadeResolution },			\
		{ TEXT("MotionBlurSkinning")		, &MotionBlurSkinning },					\
		MOBILE_INT_VALUES															\
		APEX_INT_VALUES																\
	};																				\
																					\
	/** Helper struct for all float config values. */								\
	struct { const TCHAR* Name; FLOAT* FloatValuePtr; } FloatValues[] =				\
	{																				\
		{ TEXT("ScreenPercentage")					, &ScreenPercentage },					\
		{ TEXT("SceneCaptureStreamingMultiplier")	, &SceneCaptureStreamingMultiplier },	\
		{ TEXT("ShadowTexelsPerPixel")				, &ShadowTexelsPerPixel },				\
		{ TEXT("PreShadowResolutionFactor")			, &PreShadowResolutionFactor },			\
		{ TEXT("ShadowFilterRadius")				, &ShadowFilterRadius },				\
		{ TEXT("ShadowDepthBias")					, &ShadowDepthBias },					\
		{ TEXT("PerObjectShadowTransition")			, &PerObjectShadowTransition },			\
		{ TEXT("CSMSplitPenumbraScale")				, &CSMSplitPenumbraScale },				\
		{ TEXT("CSMSplitSoftTransitionDistanceScale")	, &CSMSplitSoftTransitionDistanceScale },	\
		{ TEXT("CSMSplitDepthBiasScale")			, &CSMSplitDepthBiasScale },			\
		{ TEXT("UnbuiltWholeSceneDynamicShadowRadius"), &UnbuiltWholeSceneDynamicShadowRadius },	\
		{ TEXT("ShadowFadeExponent")				, &ShadowFadeExponent },				\
		{ TEXT("NumFracturedPartsScale")			, &NumFracturedPartsScale },			\
		{ TEXT("FractureDirectSpawnChanceScale")	, &FractureDirectSpawnChanceScale },	\
		{ TEXT("FractureRadialSpawnChanceScale")	, &FractureRadialSpawnChanceScale },	\
		{ TEXT("FractureCullDistanceScale")			, &FractureCullDistanceScale },			\
		{ TEXT("DecalCullDistanceScale")			, &DecalCullDistanceScale },			\
		{ TEXT("TessellationFactorMultiplier")		, &TessellationFactorMultiplier },		\
		MOBILE_FLOAT_VALUES																	\
		APEX_FLOAT_VALUES																	\
	};

/**
 * ctor
 */
FSystemSettingsDataWorldDetail::FSystemSettingsDataWorldDetail()
{
}

/**
* ctor
*/
FSystemSettingsDataFracturedDetail::FSystemSettingsDataFracturedDetail()
{
}

/**
* ctor
*/
FSystemSettingsDataShadowDetail::FSystemSettingsDataShadowDetail()
{
}

/**
 * ctor
 */
FSystemSettingsDataTextureDetail::FSystemSettingsDataTextureDetail()
{
}

/**
 * ctor
 */
FSystemSettingsDataVSync::FSystemSettingsDataVSync()
{
}

/**
 * ctor
 */
FSystemSettingsDataScreenPercentage::FSystemSettingsDataScreenPercentage()
{
}

/**
 * ctor
 */
FSystemSettingsDataResolution::FSystemSettingsDataResolution()
{
}

/**
 * ctor
 */
FSystemSettingsDataMSAA::FSystemSettingsDataMSAA()
{
}

/**
 * ctor
 */
FSystemSettingsDataMesh::FSystemSettingsDataMesh()
{
}

#if WITH_MOBILE_RHI
/**
 * ctor
 */
FSystemSettingsDataMobile::FSystemSettingsDataMobile()
{
}
#endif

#if WITH_APEX
/**
 * ctor
 */
FSystemSettingsDataApex::FSystemSettingsDataApex()
{
}
#endif

/**
 * ctor
 */
FSystemSettingsData::FSystemSettingsData()
{
}

/** Initializes and instance with the values from the given IniSection in the engine ini */
void FSystemSettingsData::LoadFromIni( const TCHAR* IniSection, const TCHAR* IniFilename, UBOOL bAllowMissingValues, UBOOL* FoundValues )
{
	PROPERTY_TEXT_TO_ADDRESS_MAPPING;

	UBOOL bCheckFoundValuesAtEnd = FALSE;
	
	// FoundValues == NULL is the outermost recursion level
	if (!bAllowMissingValues && FoundValues == NULL)
	{
		// we need to check the FoundValues after at the end of this function
		bCheckFoundValuesAtEnd = TRUE;

		// allocate space for tracking found values if it's not already allocated
		INT NumValues = ARRAY_COUNT(Switches) + ARRAY_COUNT(IntValues) + ARRAY_COUNT(FloatValues);
		FoundValues = (UBOOL*)appMalloc(sizeof(UBOOL) * NumValues);

		// initialize the memory to all values are unfound
		appMemzero(FoundValues, sizeof(UBOOL) * NumValues);
	}

	// first, look for a parent section to base off of
	FString BasedOnSection;
	if (GConfig->GetString(IniSection, TEXT("BasedOn"), BasedOnSection, IniFilename))
	{
		// recurse with the BasedOn section if it existed, always allowing for missing values
		LoadFromIni(*BasedOnSection, IniFilename, TRUE, FoundValues);
	}

	// Read booleans from .ini.
	for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
	{
		const UBOOL bFoundValue = GConfig->GetBool( IniSection, Switches[SwitchIndex].Name, *Switches[SwitchIndex].SwitchPtr, IniFilename );
		// track if we found it
		if (FoundValues) 
		{
			FoundValues[SwitchIndex] |= bFoundValue;
		}
	}
	// Read int values from ini.
	for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
	{
		const UBOOL bFoundValue = GConfig->GetInt( IniSection, IntValues[IntValueIndex].Name, *IntValues[IntValueIndex].IntValuePtr, IniFilename );
		// track if we found it
		if (FoundValues) 
		{
			FoundValues[ARRAY_COUNT(Switches) + IntValueIndex] |= bFoundValue;
		}
	}
	// Read float values from .ini.
	for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
	{
		const UBOOL bFoundValue = GConfig->GetFloat( IniSection, FloatValues[FloatValueIndex].Name, *FloatValues[FloatValueIndex].FloatValuePtr, IniFilename );
		// track if we found it
		if (FoundValues) 
		{
			FoundValues[ARRAY_COUNT(Switches) + ARRAY_COUNT(IntValues) + FloatValueIndex] |= bFoundValue;
		}
	}

	// Read the texture group LOD settings.
	TextureLODSettings.Initialize( IniFilename, IniSection );

	// if this is the top of the recursion stack, and we care about missing values, report them
	if (bCheckFoundValuesAtEnd)
	{
		// look for not found values
		for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
		{
			checkf(FoundValues[SwitchIndex], 
				TEXT("Couldn't find BOOL system setting %s in Ini section %s in Ini file %s!"), Switches[SwitchIndex].Name, IniSection, IniFilename);
		}
		// look for not found values
		for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
		{
			checkf(FoundValues[ARRAY_COUNT(Switches) + IntValueIndex],
				TEXT("Couldn't find INT system setting %s in Ini section %s in Ini file %s!"), IntValues[IntValueIndex].Name, IniSection, IniFilename);
		}
		// look for not found values
		for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
		{
			checkf(FoundValues[ARRAY_COUNT(Switches) + ARRAY_COUNT(IntValues) + FloatValueIndex], 
				TEXT("Couldn't find FLOAT system setting %s in Ini section %s in Ini file %s!"), FloatValues[FloatValueIndex].Name, IniSection, IniFilename);
		}

		// then free the buffer
		delete [] FoundValues;
	}
}

void FSystemSettingsData::SaveToIni( const TCHAR* IniSection )
{
	PROPERTY_TEXT_TO_ADDRESS_MAPPING;

	// Save the BOOLs.
	for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
	{
		GConfig->SetBool( IniSection, Switches[SwitchIndex].Name, *Switches[SwitchIndex].SwitchPtr, GEngineIni );
	}
	// Save the INTs.
	for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
	{
		GConfig->SetInt( IniSection, IntValues[IntValueIndex].Name, *IntValues[IntValueIndex].IntValuePtr, GEngineIni );
	}
	// Save the FLOATs.
	for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
	{
		GConfig->SetFloat( IniSection, FloatValues[FloatValueIndex].Name, *FloatValues[FloatValueIndex].FloatValuePtr, GEngineIni );
	}

	// Save the texture group LOD settings.
	WriteTextureLODGroupsToIni( IniSection );

	GConfig->Flush( FALSE, GEngineIni );
}

/**
 * Writes all texture group LOD settings to the specified ini.
 *
 * @param	IniSection			The .ini section to save to.
 */
void FSystemSettingsData::WriteTextureLODGroupsToIni(const TCHAR* IniSection)
{
#define WRITETEXTURELODGROUPTOINI(Group) WriteTextureLODGroupToIni(Group, TEXT(#Group), IniSection);
	FOREACH_ENUM_TEXTUREGROUP(WRITETEXTURELODGROUPTOINI)
#undef WRITETEXTURELODGROUPTOINI
}

/**
* Returns a string for the specified texture group LOD settings to the specified ini.
*
* @param	TextureGroupID		Index/enum of the group
* @param	GroupName			String representation of the texture group
*/
FString FSystemSettingsData::GetLODGroupString( TextureGroup TextureGroupID, const TCHAR* GroupName )
{
	const FExposedTextureLODSettings::FTextureLODGroup& Group = TextureLODSettings.GetTextureLODGroup(TextureGroupID);

	const INT MinLODSize = 1 << Group.MinLODMipCount;
	const INT MaxLODSize = 1 << Group.MaxLODMipCount;

	FName MinMagFilter = NAME_Aniso;
	FName MipFilter = NAME_Linear;
	switch(Group.Filter)
	{
		case SF_Point:
			MinMagFilter = NAME_Point;
			MipFilter = NAME_Point;
			break;
		case SF_Bilinear:
			MinMagFilter = NAME_Linear;
			MipFilter = NAME_Point;
			break;
		case SF_Trilinear:
			MinMagFilter = NAME_Linear;
			MipFilter = NAME_Linear;
			break;
		case SF_AnisotropicPoint:
			MinMagFilter = NAME_Aniso;
			MipFilter = NAME_Point;
			break;
		case SF_AnisotropicLinear:
			MinMagFilter = NAME_Aniso;
			MipFilter = NAME_Linear;
			break;
	}

	FString NumStreamedMipsText;
	if ( Group.NumStreamedMips >= 0 )
	{
		NumStreamedMipsText = FString::Printf( TEXT(",NumStreamedMips=%i"), Group.NumStreamedMips );
	}

	return FString::Printf( TEXT("(MinLODSize=%i,MaxLODSize=%i,LODBias=%i,MinMagFilter=%s,MipFilter=%s%s,MipGenSettings=%s)"),
		MinLODSize, MaxLODSize, Group.LODBias, *MinMagFilter.GetNameString(), *MipFilter.GetNameString(), *NumStreamedMipsText, UTexture::GetMipGenSettingsString(Group.MipGenSettings) );
}

/**
 * Writes the specified texture group LOD settings to the specified ini.
 *
 * @param	TextureGroupID		Index/enum of the group to parse
 * @param	GroupName			String representation of the texture group, to be used as the ini key.
 * @param	IniSection			The .ini section to save to.
 */
void FSystemSettingsData::WriteTextureLODGroupToIni(TextureGroup TextureGroupID, const TCHAR* GroupName, const TCHAR* IniSection)
{
	const FString Entry = GetLODGroupString( TextureGroupID, GroupName );
	GConfig->SetString( IniSection, GroupName, *Entry, GEngineIni );
}

/**
 * Dump settings to log
 */
void FSystemSettingsData::Dump( const TCHAR* DumpHeading )
{
	PROPERTY_TEXT_TO_ADDRESS_MAPPING;

	debugf(NAME_Init, DumpHeading);
	debugf(NAME_Init, TEXT("System Settings:"));
	// Save the BOOLs.
	for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
	{
		debugf(NAME_Init, TEXT("\t%s=%s"), Switches[SwitchIndex].Name, *Switches[SwitchIndex].SwitchPtr ? TEXT("true") : TEXT("false"));
	}
	// Save the INTs.
	for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
	{
		debugf(NAME_Init, TEXT("\t%s=%d"), IntValues[IntValueIndex].Name, *IntValues[IntValueIndex].IntValuePtr );
	}
	// Save the FLOATs.
	for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
	{
		debugf(NAME_Init, TEXT("\t%s=%.3f")	, FloatValues[FloatValueIndex].Name, *FloatValues[FloatValueIndex].FloatValuePtr );
	}

	// Save the TextureLODSettings
	DumpTextureLODGroups();

	// write out friendly equivalents (make sure to use current values and not values on disk).
	FSystemSettingsFriendly FriendlySettings = ConvertToFriendlySettings(*this);

	struct ToString
	{
		const TCHAR* operator()(EFriendlySettingsLevel Level) const
		{
			switch(Level)
			{
			case FSL_Level1: return TEXT("Level1");
			case FSL_Level2: return TEXT("Level2");
			case FSL_Level3: return TEXT("Level3");
			case FSL_Level4: return TEXT("Level4");
			case FSL_Level5: return TEXT("Level5");
				// no default
			}
			return TEXT("Custom");
		}
	};

	debugf(NAME_Init, TEXT("Friendly System Settings:"));
	debugf(NAME_Init, TEXT("\tTextureDetail=%s"), ToString()(FriendlySettings.TextureDetail));
	debugf(NAME_Init, TEXT("\tWorldDetail=%s"), ToString()(FriendlySettings.WorldDetail));
	debugf(NAME_Init, TEXT("\tShadowDetail=%s"), ToString()(FriendlySettings.ShadowDetail));
	debugf(NAME_Init, TEXT("\tbUseVSync=%d"), FriendlySettings.bUseVSync);
	debugf(NAME_Init, TEXT("\tbUseMSAA=%d"), FriendlySettings.bUseMSAA);
	debugf(NAME_Init, TEXT("\tScreenPercentage=%3d"), FriendlySettings.ScreenPercentage);
	debugf(NAME_Init, TEXT("\tUpscaleScreenPercentage=%d"), FriendlySettings.bUpscaleScreenPercentage);
	debugf(NAME_Init, TEXT("\tResX=%4d"), FriendlySettings.ResX);
	debugf(NAME_Init, TEXT("\tResY=%4d"), FriendlySettings.ResY);
	debugf(NAME_Init, TEXT("\tFullscreen=%d"), FriendlySettings.bFullScreen);
}

/**
 * Dump helpers
 */
void FSystemSettingsData::DumpTextureLODGroups()
{
#define DUMPTEXTURELODGROUP(Group) DumpTextureLODGroup(Group, TEXT(#Group));
	FOREACH_ENUM_TEXTUREGROUP(DUMPTEXTURELODGROUP)
#undef DUMPTEXTURELODGROUP
}

/**
 * Dump helpers
 */
void FSystemSettingsData::DumpTextureLODGroup(TextureGroup TextureGroupID, const TCHAR* GroupName)
{
	const FString Entry = GetLODGroupString( TextureGroupID, GroupName );
	debugf(TEXT("\t%s: %s"), GroupName, *Entry );
}

/**
 * Constructor, initializing all member variables.
 */
FSystemSettings::FSystemSettings() :
	bIsEditor( FALSE ),
	CurrentSplitScreenLevel(SPLITSCREENLEVEL_1)
{
	// there should only ever be one of these
	static INT InstanceCount = 0;
	++InstanceCount;
	check(InstanceCount == 1);

	// Renderthread state (using this is safe as we only copy the state of some member variables)
	RenderThreadSettings = FSystemSettings::FRenderThreadSettings(*this);
}

/**
 * Initializes system settings and included texture LOD settings.
 *
 * @param bSetupForEditor	Whether to initialize settings for Editor
 */
void FSystemSettings::Initialize( UBOOL bSetupForEditor )
{
	// Since System Settings is called into before GIsEditor is set, we must cache this value.
	bIsEditor = bSetupForEditor;

	// Load the settings that will be the default for every other compat level, and the editor, and the other split screen levels.
	FSystemSettingsData DefaultSettings;
	DefaultSettings.LoadFromIni(GetSectionName(FALSE), GEngineIni, FALSE);

	// read the compat level settings from the ini
	for (INT Level=0;Level<ARRAY_COUNT(Defaults);++Level)
	{
		Defaults[Level][0] = DefaultSettings;
		FString Section = FString::Printf(TEXT("AppCompatBucket%d"), Level+1);
		// use global SystemSettings if default sections don't exist.
		if (GConfig->GetSectionPrivate(*Section, FALSE, TRUE, GCompatIni) != NULL)
		{
			Defaults[Level][0].LoadFromIni(*Section, GCompatIni);
		}
		else
		{
			Defaults[Level][0].LoadFromIni(GetSectionName(bIsEditor));
		}
	}

	// Initialize all split screen level settings to the default split screen level (SPLITSCREENLEVEL_1)
	for (INT Level=0;Level<ARRAY_COUNT(Defaults);++Level)
	{
		for (INT SSLevel=1;SSLevel<ARRAY_COUNT(Defaults[0]);++SSLevel)
		{
			FString SSSection = FString::Printf(TEXT("SystemSettingsSplitScreen%d"), SSLevel+1);
			Defaults[Level][SSLevel] = DefaultSettings;
			// Override defaults with the subset of settings that are actually specified for this split screen level
			Defaults[Level][SSLevel].LoadFromIni(bIsEditor ? GIniSectionEditor : *SSSection);
		}
	}

	(FSystemSettingsData&)(*this) = DefaultSettings;
	LoadFromIni();

	ApplyOverrides();

#if CONSOLE
	// Overwrite resolution from Ini with resolution from the console RHI
	ResX = GScreenWidth;
	ResY = GScreenHeight;
#endif

	// Apply settings to render thread values.
	ApplySystemSettingsToRenderThread();
}

/**
 * Exec handler implementation.
 *
 * @param Cmd	Command to parse
 * @param Ar	Output device to log to
 *
 * @return TRUE if command was handled, FALSE otherwise
 */
UBOOL FSystemSettings::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	PROPERTY_TEXT_TO_ADDRESS_MAPPING;

	// Keep track of previous detail mode and lightmap type.
	const INT OldDetailMode = DetailMode;
	const UBOOL bOldAllowDirectionalLightMaps = bAllowDirectionalLightMaps;
	const UBOOL bOldAllowDynamicDecals = bAllowDynamicDecals;
	const FLOAT OldDecalCullDistanceScale = DecalCullDistanceScale;
	const UBOOL bOldAllowRadialBlur = bAllowRadialBlur;
	const INT OldMaxMultisamples = MaxMultiSamples;

	// Keep track whether the command was handled or not.
	UBOOL bHandledCommand = FALSE;

	if( ParseCommand(&Cmd,TEXT("SCALE")) )
	{
		// Some of these settings are used in both threads so we need to stop the rendering thread before changing them.
		FlushRenderingCommands();

		if( ParseCommand(&Cmd,TEXT("DUMP")) )
		{
			Dump(TEXT("System settings Dump:"));
			return TRUE;
		}
		if( ParseCommand(&Cmd,TEXT("DUMPINI")) )
		{
			FSystemSettingsData IniData;
			IniData.LoadFromIni(GetSectionName(bIsEditor));
			IniData.Dump(TEXT("System settings Dump (from ini):"));
			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("LEVEL")) )
		{
			FString Str(ParseToken(Cmd, 0));
			if (Str.Len() > 0)
			{
				INT Level = appAtoi(*Str);
				if (Level >= 1 && Level <= 5)
				{
					// don't put us into fullscreen if we're not already in it.
					FSystemSettingsData NewData = GetDefaultSettings(EFriendlySettingsLevel(Level));
					NewData.bFullscreen = bFullscreen && NewData.bFullscreen;
					ApplyNewSettings(NewData, FALSE); 
					Ar.Logf(TEXT("Applying Default system settings for compat level: %d"), Level);
				}
			}
			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("LOWEND")) )
		{
			bAllowStaticDecals				= FALSE;
			bAllowDynamicDecals				= FALSE;
			bAllowUnbatchedDecals			= TRUE;
			DecalCullDistanceScale			= 1.0f;
			bAllowDynamicLights				= FALSE;
			bAllowDynamicShadows			= FALSE;
			bAllowLightEnvironmentShadows	= FALSE;
			bUseCompositeDynamicLights		= TRUE;
			bAllowSHSecondaryLighting		= FALSE;
			//bAllowDirectionalLightMaps		= FALSE;		// Can't be toggled at run-time
			bAllowMotionBlur				= FALSE;
			bAllowDepthOfField				= TRUE;
			bAllowAmbientOcclusion			= FALSE;
			bAllowBloom						= TRUE;
			bAllowDistortion				= FALSE;
			bAllowFilteredDistortion		= TRUE;
			bAllowParticleDistortionDropping= TRUE;
			bAllowSpeedTreeLeaves			= FALSE;
			bAllowSpeedTreeFronds			= FALSE;
			bOnlyStreamInTextures			= FALSE;
			bAllowLensFlares				= FALSE;
			bAllowFogVolumes				= FALSE;
			bAllowFloatingPointRenderTargets= FALSE;
			//bAllowOneFrameThreadLag			= TRUE;			// Only toggle explicitly
			bUseVSync						= FALSE;
			bUpscaleScreenPercentage		= TRUE;
			SkeletalMeshLODBias				= 1;
			ParticleLODBias					= INT_MAX;
			DetailMode						= DM_Low;
			ShadowFilterQualityBias			= -1;
			MaxAnisotropy					= 0;
			ScreenPercentage				= 100.f;
			SceneCaptureStreamingMultiplier	= 0.5f;
			MaxMultiSamples					= 1;
			MinShadowResolution				= 32;
			MaxShadowResolution				= 512;
			MaxWholeSceneDominantShadowResolution = 512;
			ShadowTexelsPerPixel			= 2.0f;
			PreShadowResolutionFactor		= .5f;
			bAllowFracturedDamage			= FALSE;
			NumFracturedPartsScale			= 0;
			FractureDirectSpawnChanceScale	= 0;
			FractureRadialSpawnChanceScale	= 0;
			FractureCullDistanceScale		= 0.25f;
			bAllowRadialBlur				= TRUE;
			bAllowSubsurfaceScattering		= FALSE;
			bAllowImageReflections			= FALSE;
			bAllowImageReflectionShadowing	= FALSE;
			bHighPrecisionGBuffers			= FALSE;

			bHandledCommand					= TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("HIGHEND")) )
		{
			bAllowStaticDecals				= TRUE;
			bAllowDynamicDecals				= TRUE;
			bAllowUnbatchedDecals			= TRUE;
			DecalCullDistanceScale			= 1.0f;
			bAllowDynamicLights				= TRUE;
			bAllowDynamicShadows			= TRUE;
			bAllowLightEnvironmentShadows	= TRUE;
			bUseCompositeDynamicLights		= FALSE;
			bAllowSHSecondaryLighting		= TRUE;
			//bAllowDirectionalLightMaps		= TRUE;			// Can't be toggled at run-time
			bAllowMotionBlur				= TRUE;
			bAllowDepthOfField				= TRUE;
			bAllowAmbientOcclusion			= TRUE;
			bAllowBloom						= TRUE;
			bAllowDistortion				= TRUE;
			bAllowFilteredDistortion		= TRUE;
			bAllowParticleDistortionDropping= FALSE;
			bAllowSpeedTreeLeaves			= TRUE;
			bAllowSpeedTreeFronds			= TRUE;
			bOnlyStreamInTextures			= FALSE;
			bAllowLensFlares				= TRUE;
			bAllowFogVolumes				= TRUE;
			bAllowFloatingPointRenderTargets= TRUE;
			//bAllowOneFrameThreadLag			= TRUE;			// Only toggle explicitly
			bUseVSync						= TRUE;
			bUpscaleScreenPercentage		= TRUE;
			SkeletalMeshLODBias				= 0;
			ParticleLODBias					= 0;
			DetailMode						= DM_High;
			ShadowFilterQualityBias			= INT_MAX;
			MaxAnisotropy					= 16;
			ScreenPercentage				= 100.f;
			SceneCaptureStreamingMultiplier	= 1.0f;
			MaxMultiSamples					= 4;
			MinShadowResolution				= 32;
			MaxShadowResolution				= 512;
			MaxWholeSceneDominantShadowResolution = 2048;
			ShadowTexelsPerPixel			= 2.0f;
			PreShadowResolutionFactor		= 1.0f;
			bAllowFracturedDamage			= TRUE;
			NumFracturedPartsScale			= 1;
			FractureDirectSpawnChanceScale	= 1;
			FractureRadialSpawnChanceScale	= 1;
			FractureCullDistanceScale		= 1;
			bAllowRadialBlur				= TRUE;
			bAllowSubsurfaceScattering		= FALSE;
			bAllowImageReflections			= FALSE;
			bAllowImageReflectionShadowing	= FALSE;
			bHighPrecisionGBuffers			= FALSE;

			bHandledCommand					= TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("RESET")) )
		{
			// Reset values to defaults from ini.
			LoadFromIni();
			bHandledCommand = TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("SET")) )
		{
			// Search for a specific boolean
			for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
			{
				if( ParseCommand(&Cmd,Switches[SwitchIndex].Name) )
				{
					UBOOL bNewValue = ParseCommand(&Cmd,TEXT("TRUE"));
					*Switches[SwitchIndex].SwitchPtr = bNewValue;
					Ar.Logf(TEXT("Switch %s set to %u"), Switches[SwitchIndex].Name, bNewValue);
					bHandledCommand	= TRUE;

					debugf(TEXT("%s %s"),Switches[SwitchIndex].Name,bNewValue ? TEXT("TRUE") : TEXT("FALSE"));
				}
			}

			// Search for a specific int value.
			for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
			{
				if( ParseCommand(&Cmd,IntValues[IntValueIndex].Name) )
				{
					INT NewValue = appAtoi( Cmd );
					*IntValues[IntValueIndex].IntValuePtr = NewValue;
					Ar.Logf(TEXT("Int %s set to %u"), IntValues[IntValueIndex].Name, NewValue);
					bHandledCommand	= TRUE;

					debugf(TEXT("%s %d"),IntValues[IntValueIndex].Name,NewValue);
				}
			}

			// Search for a specific float value.
			for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
			{
				if( ParseCommand(&Cmd,FloatValues[FloatValueIndex].Name) )
				{
					FLOAT NewValue = appAtof( Cmd );
					*FloatValues[FloatValueIndex].FloatValuePtr = NewValue;
					Ar.Logf(TEXT("Float %s set to %f"), FloatValues[FloatValueIndex].Name, NewValue);
					bHandledCommand	= TRUE;

					debugf(TEXT("%s %0.3f"),FloatValues[FloatValueIndex].Name,NewValue);
				}
			}
		}
		else if( ParseCommand(&Cmd,TEXT("TOGGLE")) )
		{
			// Search for a specific boolean
			for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
			{
				if( ParseCommand(&Cmd,Switches[SwitchIndex].Name) )
				{
					*Switches[SwitchIndex].SwitchPtr = !*Switches[SwitchIndex].SwitchPtr;
					Ar.Logf(TEXT("Switch %s toggled, new value %u"), Switches[SwitchIndex].Name, *Switches[SwitchIndex].SwitchPtr);
					bHandledCommand	= TRUE;
				}
			}
		}
		else if( ParseCommand(&Cmd, TEXT("DECR")) )
		{
			FLOAT Step = ( 16.f / 1280 ) * 100.0f;
			ScreenPercentage = Clamp( ScreenPercentage - Step, Step, 100.0f );
			Ar.Logf(TEXT("ScreenPercentage decremented to %f"), ScreenPercentage);
			bHandledCommand	= TRUE;
		}
		else if( ParseCommand(&Cmd, TEXT("INCR")) )
		{
			FLOAT Step = ( 16.f / 1280 ) * 100.0f;
			ScreenPercentage = Clamp( ScreenPercentage + Step, Step, 100.0f );
			Ar.Logf(TEXT("ScreenPercentage incremented to %f"), ScreenPercentage);
			bHandledCommand	= TRUE;
		}
		else if ( ParseCommand(&Cmd, TEXT("ADJUST")) )
		{
			static UBOOL Adjusting = FALSE;
			static FString SaveLS;
			static FString SaveRS;
			Adjusting = ! Adjusting;
			UPlayerInput *Input = GEngine->GamePlayers(0)->Actor->PlayerInput;
			if ( Adjusting == TRUE ) 
			{
				SaveLS= Input->GetBind( TEXT("XboxTypeS_LeftShoulder") );
				SaveRS = Input->GetBind( TEXT("XboxTypeS_RightShoulder") );
				Input->ScriptConsoleExec( TEXT("setbind XboxTypeS_LeftShoulder scale decr"), Ar, NULL );
				Input->ScriptConsoleExec( TEXT("setbind XboxTypeS_RightShoulder scale incr"), Ar, NULL );
			}
			else 
			{
				FString SetBind;
				SetBind = FString::Printf( TEXT("setbind XboxTypeS_LeftShoulder %s"), *SaveLS );
				Input->ScriptConsoleExec( *SetBind, Ar, NULL );
				SetBind = FString::Printf( TEXT("setbind XboxTypeS_RightShoulder %s"), *SaveRS );
				Input->ScriptConsoleExec( *SetBind, Ar, NULL );
			}
			bHandledCommand = TRUE;
		}

		if (!bHandledCommand)
		{
			Ar.Logf(TEXT("Unrecognized system setting"));
		}
		else
		{
			// Write the new settings to the INI.
			SaveToIni();

			// We don't support switching lightmap type at run-time.
			if( bAllowDirectionalLightMaps != bOldAllowDirectionalLightMaps )
			{
				debugf(TEXT("Can't change lightmap type at run-time."));
				bAllowDirectionalLightMaps = bOldAllowDirectionalLightMaps;
			}
			
			// Reattach components if world-detail settings have changed.
			if( OldDetailMode != DetailMode )
			{
				// decals should always reattach after all other primitives have been attached
				TArray<UClass*> ExcludeComponents;
				ExcludeComponents.AddItem(UDecalComponent::StaticClass());
				ExcludeComponents.AddItem(UAudioComponent::StaticClass());

				FGlobalComponentReattachContext PropagateDetailModeChanges(ExcludeComponents);
			}	
			// Reattach decal components if needed 
			if( OldDetailMode != DetailMode )
			{
				TComponentReattachContext<UDecalComponent> PropagateDecalComponentChanges;
			}
			if( bOldAllowRadialBlur != bAllowRadialBlur )
			{
				TComponentReattachContext<URadialBlurComponent> PropagateRadialBlurComponentChanges;
			}

			const UBOOL bUpdateSceneRenderTargets = OldMaxMultisamples != MaxMultiSamples;
			// Activate certain system settings on the renderthread
			GSystemSettings.ApplySystemSettingsToRenderThread(bUpdateSceneRenderTargets);

#if !CONSOLE
			// Dump settings to the log so we know what values are being used.
			Dump(TEXT("System settings changed by exec command:"));
#endif
		}
	}

	return bHandledCommand;
}


/**
 * Scale X,Y offset/size of screen coordinates if the screen percentage is not at 100%
 *
 * @param X - in/out X screen offset
 * @param Y - in/out Y screen offset
 * @param SizeX - in/out X screen size
 * @param SizeY - in/out Y screen size
 */
void FSystemSettings::ScaleScreenCoords( INT& X, INT& Y, UINT& SizeX, UINT& SizeY )
{
	// Take screen percentage option into account if percentage != 100.
	if( GSystemSettings.ScreenPercentage != 100.f && !bIsEditor )
	{
		// Clamp screen percentage to reasonable range.
		FLOAT ScaleFactor = Clamp( GSystemSettings.ScreenPercentage / 100.f, 0.0f, 1.f );

		FLOAT ScaleFactorX = ScaleFactor;
		FLOAT ScaleFactorY = ScaleFactor;
#if XBOX // When bUpscaleScreenPercentage is false, we use the hardware scaler, which has alignment constraints
		if ( bUpscaleScreenPercentage == false ) 
		{
			ScaleFactorX = ( INT( ScaleFactor * GScreenWidth  + 0.5f ) & ~0xf ) / FLOAT( GScreenWidth  );
			ScaleFactorY = ( INT( ScaleFactor * GScreenHeight + 0.5f ) & ~0xf ) / FLOAT( GScreenHeight );
		}
#endif
		INT	OrigX = X;
		INT OrigY = Y;
		UINT OrigSizeX = SizeX;
		UINT OrigSizeY = SizeY;

		// Scale though make sure we're at least covering 1 pixel.
		SizeX = Max(1,appTrunc(ScaleFactorX * OrigSizeX));
		SizeY = Max(1,appTrunc(ScaleFactorY * OrigSizeY));

		// Center scaled view.
		X = OrigX + (OrigSizeX - SizeX) / 2;
		Y = OrigY + (OrigSizeY - SizeY) / 2;
	}
}

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
void FSystemSettings::UnScaleScreenCoords( 
	INT &OriginalX, INT &OriginalY, 
	UINT &OriginalSizeX, UINT &OriginalSizeY, 
	FLOAT InX, FLOAT InY, 
	FLOAT InSizeX, FLOAT InSizeY)
{
	if (NeedsUpscale())
	{
		FLOAT ScaleFactor = Clamp( GSystemSettings.ScreenPercentage / 100.f, 0.0f, 1.f );

		//undo scaling
		OriginalSizeX = appTrunc(InSizeX / ScaleFactor);
		OriginalSizeY = appTrunc(InSizeY / ScaleFactor);

		//undo centering
		OriginalX = appTrunc(InX - (OriginalSizeX - InSizeX) / 2.0f);
		OriginalY = appTrunc(InY - (OriginalSizeY - InSizeY) / 2.0f);
	}
	else
	{
		OriginalSizeX = appTrunc(InSizeX);
		OriginalSizeY = appTrunc(InSizeY);

		OriginalX = appTrunc(InX);
		OriginalY = appTrunc(InY);
	}
}

/** Indicates whether upscaling is needed */
UBOOL FSystemSettings::NeedsUpscale() const
{
	return bUpscaleScreenPercentage && ScreenPercentage < 100.0f && !bIsEditor;
}

/**
 * Reads a single entry and parses it into the group array.
 *
 * @param	TextureGroupID		Index/enum of group to parse
 * @param	MinLODSize			Minimum size, in pixels, below which the code won't bias.
 * @param	MaxLODSize			Maximum size, in pixels, above which the code won't bias.
 * @param	LODBias				Group LOD bias.
 */
void FSystemSettings::SetTextureLODGroup(TextureGroup TextureGroupID, int MinLODSize, INT MaxLODSize, INT LODBias, TextureMipGenSettings MipGenSettings)
{
	TextureLODSettings.GetTextureLODGroup(TextureGroupID).MinLODMipCount	= appCeilLogTwo( MinLODSize );
	TextureLODSettings.GetTextureLODGroup(TextureGroupID).MaxLODMipCount	= appCeilLogTwo( MaxLODSize );
	TextureLODSettings.GetTextureLODGroup(TextureGroupID).LODBias			= LODBias;
	TextureLODSettings.GetTextureLODGroup(TextureGroupID).MipGenSettings	= MipGenSettings;
}

/**
* Recreates texture resources and drops mips.
*
* @return		TRUE if the settings were applied, FALSE if they couldn't be applied immediately.
*/
UBOOL FSystemSettings::UpdateTextureStreaming()
{
	if ( GStreamingManager )
	{
		// Make sure textures can be streamed out so that we can unload current mips.
		const UBOOL bOldOnlyStreamInTextures = bOnlyStreamInTextures;
		bOnlyStreamInTextures = FALSE;

		for( TObjectIterator<UTexture2D> It; It; ++It )
		{
			UTexture* Texture = *It;

			// Update cached LOD bias.
			Texture->CachedCombinedLODBias = TextureLODSettings.CalculateLODBias( Texture );
		}

		// Make sure we iterate over all textures by setting it to high value.
		GStreamingManager->SetNumIterationsForNextFrame( 100 );
		// Update resource streaming with updated texture LOD bias/ max texture mip count.
		GStreamingManager->UpdateResourceStreaming( 0 );
		// Block till requests are finished.
		GStreamingManager->BlockTillAllRequestsFinished();

		// Restore streaming out of textures.
		bOnlyStreamInTextures = bOldOnlyStreamInTextures;
	}

	return TRUE;
}

void FSystemSettings::ApplySystemSettings_RenderingThread(const FSystemSettings::FRenderThreadSettings& NewSettings, UBOOL bInUpdateRenderTargets)
{
	// Should we create or release certain rendertargets?
	const UBOOL bUpdateRenderTargets =
		bInUpdateRenderTargets || 
		(GSystemSettings.RenderThreadSettings.bAllowMotionBlur		!= NewSettings.bAllowMotionBlur) ||
		(GSystemSettings.RenderThreadSettings.bAllowAmbientOcclusion!= NewSettings.bAllowAmbientOcclusion) ||
		(GSystemSettings.RenderThreadSettings.bAllowDynamicShadows	!= NewSettings.bAllowDynamicShadows) ||
		(GSystemSettings.RenderThreadSettings.bAllowHardwareShadowFiltering	!= NewSettings.bAllowHardwareShadowFiltering) ||
		(GSystemSettings.RenderThreadSettings.bAllowFogVolumes		!= NewSettings.bAllowFogVolumes) ||
		(GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering	!= NewSettings.bAllowSubsurfaceScattering) ||
		(GSystemSettings.RenderThreadSettings.bHighPrecisionGBuffers!= NewSettings.bHighPrecisionGBuffers) ||
		(GSystemSettings.RenderThreadSettings.MinShadowResolution	!= NewSettings.MinShadowResolution) ||
		(GSystemSettings.RenderThreadSettings.MaxShadowResolution	!= NewSettings.MaxShadowResolution) ||
		(GSystemSettings.RenderThreadSettings.MaxWholeSceneDominantShadowResolution	!= NewSettings.MaxWholeSceneDominantShadowResolution);

	GSystemSettings.RenderThreadSettings = NewSettings;

// Shouldn't be reallocating render targets on console
#if !CONSOLE
	if(bUpdateRenderTargets)
	{
		GSceneRenderTargets.UpdateRHI();
	}
#endif
}

/** Makes System Settings take effect on the renderthread */
void FSystemSettings::ApplySystemSettingsToRenderThread(UBOOL bUpdateRenderTargets)
{
	// this initialized the FRenderThreadSettings from FSystemSettings
	FSystemSettings::FRenderThreadSettings NewSettings(*this);

#if WITH_MOBILE_RHI
	UBOOL bMobileSettingsChanged = NewSettings.MobileSettings != (const FSystemSettingsDataMobile&)(*this);
	NewSettings.MobileSettings			= (const FSystemSettingsDataMobile&)(*this);
#endif

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		ActivateSystemSettings,
		FSystemSettings*,SystemSettings,this,
		FSystemSettings::FRenderThreadSettings,NewSettings,NewSettings,
		UBOOL,bUpdateRenderTargets,bUpdateRenderTargets,
	{
		SystemSettings->ApplySystemSettings_RenderingThread(NewSettings, bUpdateRenderTargets);
	});
}

/*-----------------------------------------------------------------------------
	Resolution.
-----------------------------------------------------------------------------*/

/** 
 * Sets the resolution and writes the values to Ini if changed but does not apply the changes (eg resize the viewport).
 */
void FSystemSettings::SetResolution(INT InSizeX, INT InSizeY, UBOOL InFullscreen)
{
	if (!bIsEditor)
	{
		const UBOOL bResolutionChanged = 
			ResX != InSizeX ||
			ResY != InSizeY ||
			bFullscreen != InFullscreen;

		if (bResolutionChanged)
		{
			ResX = InSizeX;
			ResY = InSizeY;
			bFullscreen = InFullscreen;
			SaveToIni();
		}
	}
}

/**
 * Overriden function that selects the proper ini section to write to
 */
void FSystemSettings::LoadFromIni()
{
	FSystemSettingsData::LoadFromIni(GetSectionName(bIsEditor));

#if CONSOLE
	// Always default to using VSYNC on consoles.
	bUseVSync = TRUE;
#endif

	// Disable VSYNC if -novsync is on the command line.
	bUseVSync = bUseVSync && !ParseParam(appCmdLine(), TEXT("novsync"));

	// Enable VSYNC if -vsync is on the command line.
	bUseVSync = bUseVSync || ParseParam(appCmdLine(), TEXT("vsync"));
}

void FSystemSettings::SaveToIni()
{
	// don't write changes in the editor
	if (bIsEditor)
	{
		debugf(TEXT("Can't save system settings to ini in an editor mode"));
		return;
	}
	FSystemSettingsData::SaveToIni(GetSectionName(bIsEditor));
}

/**
 * Returns the current friendly settings as defined by the current system settings.
 *
 * Normally we read from the ini because some settings are deferred until the next reboot
 * so are only in the ini.
 */
FSystemSettingsFriendly FSystemSettings::ConvertToFriendlySettings( bool bReadSettingsFromIni /*= TRUE*/ )
{
	if (bReadSettingsFromIni)
	{
		// use the values in the ini file
		FSystemSettingsData IniData;
		IniData.LoadFromIni(GetSectionName(bIsEditor));
		return ::ConvertToFriendlySettings(IniData);
	}

	return ::ConvertToFriendlySettings(*this);
}

/**
 * applies the system settings using the given friendly settings
 */
void FSystemSettings::ApplyFriendlySettings( const FSystemSettingsFriendly& FriendlySettings, UBOOL bWriteToIni )
{
	ApplyNewSettings(ConvertToSystemSettings(FriendlySettings, *this), bWriteToIni);
}

/** 
 * Ensures that the correct settings are being used based on split screen type.
 */
void FSystemSettings::UpdateSplitScreenSettings(ESplitScreenType NewSplitScreenType)
{
	ESplitScreenLevel NewLevel = NewSplitScreenType == eSST_NONE ? SPLITSCREENLEVEL_1 : SPLITSCREENLEVEL_2;

	if (NewLevel != CurrentSplitScreenLevel)
	{
		CurrentSplitScreenLevel = NewLevel;
		//@todo - Don't assume level 5 is the current settings
		FSystemSettingsData NewData = GetDefaultSettings(FSL_Level5);
		if( GEngine && GEngine->GameViewport )
		{
			GEngine->GameViewport->OverrideSplitscreenSettings(NewData,NewSplitScreenType);
		}

		ApplyNewSettings(NewData, FALSE); 
	}
}

const FSystemSettingsData& FSystemSettings::GetDefaultSettings( EFriendlySettingsLevel Level )
{
	check(Level>=FSL_Level1 && Level<=FSL_Level5);
	return Defaults[Level-1][CurrentSplitScreenLevel];
}

/**
 * Helper for ApplyNewSettings when the engine is running. Applies the changes needed for the runtime system.
 *
 * We can assume the game is running if this code is called.
 */
void FSystemSettings::ApplySettingsAtRuntime(const FSystemSettingsData& NewSettings, UBOOL bWriteToIni)
{
	// Some of these settings are shared between threads, so we
	// must flush the rendering thread before changing anything.
	FlushRenderingCommands();

	// Track settings we might have to put back
	FExposedTextureLODSettings InMemoryTextureLODSettings = TextureLODSettings;

	// Read settings from .ini.  This is necessary because settings which need to wait for a restart
	// will be on disk but may not be in memory.  Therefore, we read from disk before capturing old
	// values to revert to.
	LoadFromIni();

	// see what settings are actually changing.
	// Ugly casts because system settings is multi-inherited from all the consituent classes for backwards compatibility

	// Texture Detail
	UBOOL bTextureDetailChanged = (const FSystemSettingsDataTextureDetail&)(*this) != (const FSystemSettingsDataTextureDetail&)NewSettings;
	// World Detail
	UBOOL bWorldDetailChanged = (const FSystemSettingsDataWorldDetail&)(*this) != (const FSystemSettingsDataWorldDetail&)NewSettings;
	// Shadow Detail
	UBOOL bShadowDetailChanged = (const FSystemSettingsDataShadowDetail&)(*this) != (const FSystemSettingsDataShadowDetail&)NewSettings;
	// VSync
	UBOOL bVSyncChanged = (const FSystemSettingsDataVSync&)(*this) != (const FSystemSettingsDataVSync&)NewSettings;
	// Screen Percentage
	UBOOL bScreenPercentageChanged = (const FSystemSettingsDataScreenPercentage&)(*this) != (const FSystemSettingsDataScreenPercentage&)NewSettings;
	// Resolution
	UBOOL bResolutionChanged = (const FSystemSettingsDataResolution&)(*this) != (const FSystemSettingsDataResolution&)NewSettings;
	// MSAA
	UBOOL bMSAAChanged = (const FSystemSettingsDataMSAA&)(*this) != (const FSystemSettingsDataMSAA&)NewSettings;
	// special case changes we need to look out for
	UBOOL bDetailModeChanged = DetailMode != NewSettings.DetailMode;
	UBOOL bAllowDirectionalLightMapsChanged = bAllowDirectionalLightMaps != NewSettings.bAllowDirectionalLightMaps;
	// change in decal detail mode
	const UBOOL bDecalCullDistanceScaleChanged = DecalCullDistanceScale != NewSettings.DecalCullDistanceScale;
	// change in mode for dynamic path for decals
	const UBOOL bAllowUnbatchedDecalsChanged = bAllowUnbatchedDecals != NewSettings.bAllowUnbatchedDecals;
	// Mesh
	UBOOL bMeshChanged = (const FSystemSettingsDataMesh&)(*this) != (const FSystemSettingsDataMesh&)NewSettings;
	UBOOL bPrevForceCPUAccessToGPUSkinVerts = ((const FSystemSettingsDataMesh&)(*this)).bForceCPUAccessToGPUSkinVerts;
	UBOOL bPrevDisableSkeletalInstanceWeights = ((const FSystemSettingsDataMesh&)(*this)).bDisableSkeletalInstanceWeights;

	// Set new settings. Would look prettier if we didn't derive from the Data class...
	(FSystemSettingsData&)(*this) = NewSettings;

	// apply any runtime changes that need to be made
	UBOOL bUpdateTextureStreamingSucceeded = FALSE;
	if (bTextureDetailChanged)
	{
		bUpdateTextureStreamingSucceeded = GSystemSettings.UpdateTextureStreaming();
	}

	if (bResolutionChanged)
	{
		if ( GEngine
			&& GEngine->GameViewport
			&& GEngine->GameViewport->ViewportFrame )
		{
			GEngine->GameViewport->ViewportFrame->Resize(ResX, ResY, bFullscreen);
		}
	}

	// Not allowed to change mesh settings...
	if (bMeshChanged)
	{
		debugf(TEXT("Can't change Mesh system settings at run-time."));
		((FSystemSettingsDataMesh&)(*this)).bForceCPUAccessToGPUSkinVerts = bPrevForceCPUAccessToGPUSkinVerts;
		((FSystemSettingsDataMesh&)(*this)).bDisableSkeletalInstanceWeights = bPrevDisableSkeletalInstanceWeights;
	}

	// If requested, save the settings to ini.
	if ( bWriteToIni )
	{
		SaveToIni();
	}

	// We don't support switching lightmap type at run-time.
	if( bAllowDirectionalLightMapsChanged )
	{
		debugf(TEXT("Can't change lightmap type at run-time."));
		bAllowDirectionalLightMaps = !bAllowDirectionalLightMaps;
	}

	// If texture detail settings couldn't be applied because we're loading seekfree,
	// revert the new settings to their previous in-memory values.
	if ( bTextureDetailChanged && !bUpdateTextureStreamingSucceeded )
	{
		TextureLODSettings = InMemoryTextureLODSettings;
	}

	// Reattach components if world-detail settings have changed.
	if( bDetailModeChanged )
	{
		// decals should always reattach after all other primitives have been attached
		TArray<UClass*> ExcludeComponents;
		ExcludeComponents.AddItem(UDecalComponent::StaticClass());
		ExcludeComponents.AddItem(UAudioComponent::StaticClass());

		FGlobalComponentReattachContext PropagateDetailModeChanges(ExcludeComponents);
	}	
	// Reattach decal components if needed
	if( bDetailModeChanged )
	{
		TComponentReattachContext<UDecalComponent> PropagateDecalComponentChanges;
	}

	// Activate certain system settings on the renderthread
	GSystemSettings.ApplySystemSettingsToRenderThread();
}

/**
 * Sets new system settings (optionally writes out to the ini). 
 */
void FSystemSettings::ApplyNewSettings( const FSystemSettingsData& NewSettings, UBOOL bWriteToIni )
{
	// we can set any setting before the engine is initialized so don't bother restoring values.
	UBOOL bEngineIsInitialized = GEngine != NULL;

	// if the engine is running, there are certain values we can't set immediately
	if (bEngineIsInitialized)
	{
		// apply settings to the runtime system.
		ApplySettingsAtRuntime(NewSettings, bWriteToIni);

		ApplyOverrides();
	}
	else
	{
		// if the engine is not initialized we don't need to worry about all the deferred settings etc. 
		// as we do above.
		// Set new settings. Would look prettier if we didn't derive from the Data class...
		(FSystemSettingsData&)(*this) = NewSettings;

		// If requested, save the settings to ini.
		if ( bWriteToIni )
		{
			SaveToIni();
		}

		ApplyOverrides();

		// Activate certain system settings on the renderthread
		GSystemSettings.ApplySystemSettingsToRenderThread();
	}

	// Dump settings to the log so we know what values are being used.
	if (bWriteToIni && !bIsEditor)
	{
#if !CONSOLE
		Dump(TEXT("System Settings changed using SetFriendlyGraphicsSettings:"));
#endif
	}
}

void FSystemSettings::ApplyOverrides()
{
	if (ParseParam(appCmdLine(),TEXT("MAXQUALITYMODE")))
	{
		bUseMaxQualityMode = TRUE;
	}

	if (ParseParam(appCmdLine(),TEXT("MSAA")))
	{
		MaxMultiSamples = GOptimalMSAALevel;
	}

#if CONSOLE
	// Overwrite resolution from Ini with resolution from the console RHI
	ResX = GScreenWidth;
	ResY = GScreenHeight;
	bUseMaxQualityMode = FALSE;
#else
	// Dump(TEXT("Startup System Settings:"));
#endif

	if (bUseMaxQualityMode)
	{
		// Modify various system settings to get the best quality regardless of performance impact
		MaxAnisotropy = 16;
		ShadowFilterQualityBias++;
		MinShadowResolution = 16;
		// Disable shadow fading out over distance
		ShadowFadeResolution = 1;
		MinPreShadowResolution = 16;
		PreShadowFadeResolution = 1;
		// Increase shadow texel density
		ShadowTexelsPerPixel = 4.0f;
		// Don't downsample preshadows
		PreShadowResolutionFactor = 1.0f;
		// Use 4096 shadow maxes
		MaxShadowResolution = 4096;
		MaxWholeSceneDominantShadowResolution = 4096;

		// No point in changing scene color format if depth is not stored in the alpha
		if (!GSupportsDepthTextures)
		{
			// Use a 32 bit fp scene color and depth, which reduces distant shadow artifacts due to storing scene depth in 16 bit fp significantly
			GSceneRenderTargets.SetSceneColorBufferFormat(PF_A32B32G32R32F);
		}

		for (INT GroupIndex = 0; GroupIndex < TEXTUREGROUP_MAX; GroupIndex++)
		{
			FTextureLODSettings::FTextureLODGroup& CurrentGroup = TextureLODSettings.GetTextureLODGroup(GroupIndex);
			// Use the best quality texture filtering
			CurrentGroup.Filter = SF_AnisotropicLinear;
			// Raise texture max sizes to 4096
			CurrentGroup.MinLODMipCount = 12;
			CurrentGroup.MaxLODMipCount = 12;
			CurrentGroup.LODBias = -1000;
		}
	}
}

