/*=============================================================================
	MobileSupport.cpp: Mobile related enums and functions needed by the editor/cooker/mobile platforms

	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/**
 * An array that holds the number of bits required to store each component in the program key
 */
ES2ShaderProgramKeyField ES2ShaderProgramKeyFields[] = {
	//Reserved for primitive type & platform features
	{2, EPF_MAX},					//PKDT_PlatformFeatures
	{3, EPT_MAX},					//PKDT_PrimitiveType

	//World/Misc
	{1, 2},							// PKDT_IsDepthOnlyRendering
	{1, 2},							// PKDT_IsGradientFogEnabled
	{2, SSA_Max},					// PKDT_ParticleScreenAlignment

	//Vertex Factory Flags
	{1, 2},							// PKDT_IsLightmap
	{1, 2},							// PKDT_IsSkinned
	{1, 2},							// PKDT_IsDecal
	{1, 2},							// PKDT_IsSubUV

	//Material Provided
	{1, 2},							// PKDT_IsLightingEnabled
	{1, 2},							// PKDT_IsAlphaTestEnabled
	{1, 2},							// PKDT_UsingAdditiveMaterial
	{2, MTCS_MAX},					// PKDT_BaseTextureTexCoordsSource
	{2, MTCS_MAX},					// PKDT_DetailTextureTexCoordsSource
	{2, MTCS_MAX},					// PKDT_MaskTextureTexCoordsSource
	{1, 2},							// PKDT_IsTextureCoordinateTransformEnabled
	{1, 2},							// PKDT_TextureTransformTarget
	{1, 2},							// PKDT_IsSpecularEnabled
	{1, 2},							// PKDT_IsPixelSpecularEnabled
	{1, 2},							// PKDT_IsNormalMappingEnabled
	{1, 2},							// PKDT_IsEnvironmentMappingEnabled
	{1, 2},							// PKDT_MobileEnvironmentBlendMode,
	{1, 2},							// PKDT_IsMobileEnvironmentFresnelEnabled,
	{1, 2},							// PKDT_IsBumpOffsetEnabled
	{1, 2},							// PKDT_IsColorTextureBlendingEnabled
	{1, 2},							// PKDT_TextureBlendFactorSource
	{1, 2},							// PKDT_IsWaveVertexMovementEnabled
	{3, MSM_MAX},					// PKDT_SpecularMask
	{3, MAOS_MAX},					// PKDT_AmbientOcclusionSource
	//{2, FixedLightmapScale_Max},	// PKDT_FixedLightmapScaleFactor
	//{1, 2},							// PKDT_SimpleLightmapsStoredInLinearSpace
	{1, 2},							// PKDT_UseUniformColorMultiply
	{1, 2},							// PKDT_UseVertexColorMultiply
	{1, 2},							// PKDT_IsRimLightingEnabled
	{5, MVS_MAX},					// PKDT_RimLightingMaskSource
	{5, MVS_MAX},					// PKDT_EnvironmentMaskSource
	{1, 2},							// PKDT_IsEmissiveEnabled
	{2, MECS_MAX},					// PKDT_EmissiveColorSource
	{5, MVS_MAX},					// PKDT_EmissiveMaskSource
};

// Assure that the array above is always up to date - if it fires, some entries may need to be added or removed
checkAtCompileTime(ARRAY_COUNT(ES2ShaderProgramKeyFields) == PKDT_MAX, ES2ShaderProgramKeyFieldBitsSizeMismatch);

/**
 * Packs the initialized ProgramKeyData structure into a single QWORD
 *
 * @param KeyData	The initialized ProgramKeyData structure
 * @return			The program key for the current shader manager settings
 */
QWORD PackProgramKeyData( ProgramKeyData& KeyData )
{
	QWORD CurrentKey = 0;
	QWORD RunningMax = 0;

	//ensure that all values have been set before we try and use program key data
	checkf(KeyData.IsValid());

	for( INT Index = 0; Index < PKDT_MAX; Index++ )
	{
		const BYTE NextFieldValue = KeyData.FieldValue[Index];
		const BYTE NextFieldBits = ES2ShaderProgramKeyFields[Index].NumBits;

#if DEBUG
		// Safety check to make sure we're not running out of room for the new key
		QWORD MaxQWORD = ~0;
		if( (MaxQWORD >> NextFieldBits) < RunningMax )
		{
			warnf(NAME_Warning, TEXT("ES2 Program Key has exceeded key capacity"));
		}
#endif
		CurrentKey <<= NextFieldBits;
		CurrentKey += NextFieldValue;

#if DEBUG
		RunningMax <<= NextFieldBits;
		RunningMax += ((1 << NextFieldBits) - 1);
#endif
	}

	return CurrentKey;
}

/**
 * Unpacks the provided QWORD into the provided ProgramKeyData structure
 *
 * @param ProgramKey	The key to unpack
 * @param KeyData		The ProgramKeyData structure to fill
 */
void UnpackProgramKeyData( QWORD ProgramKey, ProgramKeyData& KeyData )
{
	for( INT Index = PKDT_MAX - 1; Index >= 0; Index-- )
	{
		const BYTE NextFieldBits = ES2ShaderProgramKeyFields[Index].NumBits;
		KeyData.FieldValue[Index] = ProgramKey & ((1 << NextFieldBits) - 1);
		ProgramKey >>= NextFieldBits;
	}
}

QWORD HexStringToQWord(const TCHAR *In)
{
	QWORD Result = 0;
	while (1)
	{
		if ( *In>=TEXT('0') && *In<=TEXT('9') )
		{
			Result = (Result << 4) + *In - TEXT('0');
		}
		else if ( *In>=TEXT('A') && *In<=TEXT('F') )
		{
			Result = (Result << 4) + 10 + *In - TEXT('A');
		}
		else if ( *In>=TEXT('a') && *In<=TEXT('f') )
		{
			Result = (Result << 4) + 10 + *In - TEXT('a');
		}
		else if ( (*In==TEXT('x') || *In==TEXT('X')) && Result == 0)
		{
		}
		else
		{
			break;
		}
		In++;
	}
	return Result;
}

/** Starts the sampling, tracks if all the parameters were set */
void ProgramKeyData::Start (void)
{
	bStarted = TRUE;
}

/** Stops allowing input and verifies all input has been set */
void ProgramKeyData::Stop (void)
{
	// Special key generation for "global shaders" (only GammaCorrection so far)
	if (FieldValue[PKDT_PrimitiveType] == EPT_GlobalShader)
	{
		// Clear out all fields except the type, just to make we have a clean key.
		for (INT FieldIndex = 0; FieldIndex < PKDT_MAX; ++ FieldIndex)
		{
			if ( FieldIndex != PKDT_GlobalShaderType )
			{
				OVERRIDE_PROGRAM_KEY_VALUE( FieldIndex, (*this), 0 );
			}
		}
		FieldValue[PKDT_PrimitiveType] = EPT_GlobalShader;
	}
	else
	{
		//STAGE 0 - conditionally remove settings that shouldn't be on because they are irrelevant
		//if not a standard mesh
		if (FieldValue[PKDT_PrimitiveType] != EPT_Default)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsDepthOnlyRendering, (*this), 0);

			//turn off lightmap scaling!!!!
			//OVERRIDE_PROGRAM_KEY_VALUE (PKDT_FixedLightmapScaleFactor, (*this), FixedLightmapScale_None);
			//OVERRIDE_PROGRAM_KEY_VALUE (PKDT_SimpleLightmapsStoredInLinearSpace, (*this), FALSE);

			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsSkinned, (*this), 0);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsDecal, (*this), 0);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsLightingEnabled, (*this), 0);
		}
		else
		{
			//the only reason we care about additive is for particles/lens flares
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_UsingAdditiveMaterial, (*this), FALSE);
		}
		//NOT PARTICLE
		if (FieldValue[PKDT_PrimitiveType]  != EPT_Particle)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_ParticleScreenAlignment, (*this), 0);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsSubUV, (*this), 0);
		}
		//SIMPLE
		if (FieldValue[PKDT_PrimitiveType]  == EPT_Simple)
		{
			for (INT KeyIndex = PKDT_IsDepthOnlyRendering; KeyIndex < PKDT_MAX; ++KeyIndex)
			{
				if ((KeyIndex != PKDT_IsAlphaTestEnabled) && (KeyIndex != PKDT_TextureTransformTarget) && (KeyIndex != PKDT_IsTextureCoordinateTransformEnabled))
				{
					OVERRIDE_PROGRAM_KEY_VALUE (KeyIndex, (*this), 0);
				}
			}
		}


		//STAGE 1 - conditionally force enable/disable based on system settings
#if MINIMIZE_ES2_SHADERS
		if (FieldValue[PKDT_PlatformFeatures]  == EPF_LowEndFeatures)
		{
			//Never fog
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsGradientFogEnabled, (*this), FALSE);
			//Never bump offset
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsBumpOffsetEnabled, (*this), FALSE);
			//Never environment map
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsEnvironmentMappingEnabled, (*this), FALSE);
			//Never rim light
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsRimLightingEnabled, (*this), FALSE);
			//No Specular
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsSpecularEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsPixelSpecularEnabled, (*this), FALSE);
			//No color blending
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsColorTextureBlendingEnabled, (*this), FALSE);
			//No normal mapping
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsNormalMappingEnabled, (*this), FALSE);
		}
#endif

		/** STAGE 2 - Remove flags that aren't respected (parent/child) based on "parent" states */
		//additive turns off fog
		if (FieldValue[PKDT_UsingAdditiveMaterial] == TRUE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsGradientFogEnabled, (*this), FALSE);
		}

		//Depth only
		if (FieldValue[PKDT_IsDepthOnlyRendering] == TRUE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsGradientFogEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsLightmap, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsLightingEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_BaseTextureTexCoordsSource, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_DetailTextureTexCoordsSource, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_MaskTextureTexCoordsSource, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsTextureCoordinateTransformEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_TextureTransformTarget, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsSpecularEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsNormalMappingEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsEnvironmentMappingEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_MobileEnvironmentBlendMode, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsBumpOffsetEnabled, (*this), FALSE);
			//ensure this is unlocked for shadowing
			LOCK_PROGRAM_KEY_VALUE(PKDT_IsColorTextureBlendingEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsColorTextureBlendingEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsRimLightingEnabled, (*this), FALSE);
		}

		// Emissive
		if( FieldValue[PKDT_IsEmissiveEnabled] == FALSE )
		{
			OVERRIDE_PROGRAM_KEY_VALUE( PKDT_EmissiveColorSource, (*this), 0 );
			OVERRIDE_PROGRAM_KEY_VALUE( PKDT_EmissiveMaskSource, (*this), 0 );
		}

		//ENVIRONMENT MAPPING
		if (FieldValue[PKDT_IsEnvironmentMappingEnabled] == FALSE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_MobileEnvironmentBlendMode, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsMobileEnvironmentFresnelEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_EnvironmentMaskSource, (*this), FALSE);
		}

		//LIGHTING
		if (FieldValue[PKDT_IsLightingEnabled] == FALSE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsLightmap, (*this), 0);
		}
		//LIGHT MAP
		if (FieldValue[PKDT_IsLightmap] == FALSE)
		{
			//OVERRIDE_PROGRAM_KEY_VALUE (PKDT_FixedLightmapScaleFactor, (*this), 0);
		}
		//SPECULAR
		if ((FieldValue[PKDT_IsSpecularEnabled] == FALSE))
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_IsPixelSpecularEnabled, (*this), FALSE);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_SpecularMask, (*this), FALSE);
		}
		//Color texture blend
		if (FieldValue[PKDT_IsColorTextureBlendingEnabled] == FALSE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_DetailTextureTexCoordsSource, (*this), 0);
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_TextureBlendFactorSource, (*this), 0);
		}
		//fog saturation is irrelevant if fog is disabled
		if (FieldValue[PKDT_IsTextureCoordinateTransformEnabled] == FALSE)
		{
			OVERRIDE_PROGRAM_KEY_VALUE (PKDT_TextureTransformTarget, (*this), FALSE);
		}
	}

	//now fully stop
	bStopped = TRUE;
	checkf(NumberFieldsSet == PKDT_MAX);
}


/**
 * Calls the visual C preprocessor, if available
 *
 * @param SourceCode		Code to preprocess
 * @param DestFile			Fully qualified (!) path to where the preprocessed source should go
 * @return					string containing the preprocessed source
 */
FString RunCPreprocessor(const FString &SourceCode,const TCHAR *DestFile, UBOOL CleanWhiteSPace)
{
	FFilename PreprocessedFileName = DestFile;
	FFilename UnPreprocessedFileName = FString(DestFile) + TEXT(".temp");
	FString SourceCodePlusNewline = SourceCode;
	SourceCodePlusNewline += TEXT("\r\n\r\n");
	appSaveStringToFile( SourceCodePlusNewline, *UnPreprocessedFileName );
	const FFilename ExecutableFileName(FString( appBaseDir() ) + FString(TEXT( "..\\UnrealCommand.exe" )));
	const FString CmdLineParams = FString::Printf( TEXT( "PreprocessShader %sGame Shipping %s %s -CleanWhitespace" ),GGameName,*UnPreprocessedFileName, *PreprocessedFileName);

	const UBOOL bLaunchDetached = TRUE;
	const UBOOL bLaunchHidden = TRUE;
	void* ProcHandle = appCreateProc( *ExecutableFileName, *CmdLineParams, bLaunchDetached, bLaunchHidden );
	if( ProcHandle != NULL )
	{
		GFileManager->Delete( *PreprocessedFileName ); // we want to make sure this is not stale
		UBOOL bProfilingComplete = FALSE;
		INT ReturnCode = 1;
		while( !bProfilingComplete )
		{
			bProfilingComplete = appGetProcReturnCode( ProcHandle, &ReturnCode );
			if( !bProfilingComplete )
			{
				appSleep( 0.01f );
			}
		}
	}
	GFileManager->Delete( *UnPreprocessedFileName ); 

	FString FinalCode;

	const UBOOL bLoadedSuccessfully = appLoadFileToString( FinalCode, *PreprocessedFileName );
	if( !bLoadedSuccessfully )
	{
		FinalCode.Empty();  // make double sure we return an empty string as failure
	}
	return FinalCode;
}


/**
 * Returns the base filename of a ES2 shader
 *
 * @param PrimitiveType		Primitive type to get name for
 * @param GlobalShaderType	Global shader type (if PrimitiveType is EPT_GlobalShader)
 * @param Kind				SF_VERTEX or SF_PIXEL
 * @return					string containing the shader name, including path
 */
FString GetES2ShaderFilename( EMobilePrimitiveType PrimitiveType, EMobileGlobalShaderType GlobalShaderType, EShaderFrequency Kind )
{
	FString Filename;
	switch (PrimitiveType)
	{
		case EPT_Default:
			Filename += TEXT("Default");
			break;
		case EPT_Particle:
			Filename += TEXT("ParticleSprite");
			break;
		case EPT_PointSpriteParticle:
			Filename += TEXT("ParticlePointSprite");
			break;
		case EPT_LensFlare:
			if (Kind == SF_Pixel)
			{
				Filename += TEXT("ParticleSprite");
			}
			else
			{
				Filename += TEXT("LensFlare");
			}
			break;
		case EPT_Simple:
			Filename += TEXT("Simple");
			break;
		case EPT_GlobalShader:
			switch ( GlobalShaderType )
			{
				case EGST_GammaCorrection:
					Filename += TEXT("GammaCorrection");
					break;
				case EGST_Filter:
					Filename += TEXT("Filter");
					break;
				case EGST_DOFAndBloomGather:
					Filename += TEXT("DOFAndBloomGather");
					break;
				case EGST_LUTBlender:
					Filename += TEXT("LUTBlender");
					break;
				case EGST_UberPostProcess:
					Filename += TEXT("UberPostProcess");
					break;
				case EGST_LightShaftDownSample:
					Filename += TEXT("LightShaftDownSample");
					break;
				case EGST_LightShaftBlur:
					Filename += TEXT("LightShaftBlur");
					break;
				case EGST_LightShaftApply:
					Filename += TEXT("LightShaftApply");
					break;
				case EGST_SimpleF32:
					Filename += TEXT("SimpleF32");
					break;
				default:
					check(0);
			}
			break;
		default:
			check(0);
	}
	if (Kind == SF_Pixel)
	{
		Filename += TEXT("PixelShader");
	}
	else
	{
		Filename += TEXT("VertexShader");
	}
	return Filename;
}

/**
 * Generates a define block for texture sources with a prefix
 *
 * @param DefinesString			Return value, adds to the current string
 * @param Prefix				Prefix to add to each define
 * @param InValue				Value for define with only prefix
 */
static void CreateValueSourceDefines( FString& DefinesString, const FString& InPrefix, const INT InValue )
{
	DefinesString += FString::Printf(TEXT("#define %s_CONSTANT %d\r\n"), *InPrefix, (INT)MVS_Constant );
	DefinesString += FString::Printf(TEXT("#define %s_VERTEX_COLOR_RED %d\r\n"), *InPrefix, (INT)MVS_VertexColorRed );
	DefinesString += FString::Printf(TEXT("#define %s_VERTEX_COLOR_GREEN %d\r\n"), *InPrefix, (INT)MVS_VertexColorGreen );
	DefinesString += FString::Printf(TEXT("#define %s_VERTEX_COLOR_BLUE %d\r\n"), *InPrefix, (INT)MVS_VertexColorBlue );
	DefinesString += FString::Printf(TEXT("#define %s_VERTEX_COLOR_ALPHA %d\r\n"), *InPrefix, (INT)MVS_VertexColorAlpha );
	DefinesString += FString::Printf(TEXT("#define %s_BASE_TEXTURE_RED %d\r\n"), *InPrefix, (INT)MVS_BaseTextureRed );
	DefinesString += FString::Printf(TEXT("#define %s_BASE_TEXTURE_GREEN %d\r\n"), *InPrefix, (INT)MVS_BaseTextureGreen );
	DefinesString += FString::Printf(TEXT("#define %s_BASE_TEXTURE_BLUE %d\r\n"), *InPrefix, (INT)MVS_BaseTextureBlue );
	DefinesString += FString::Printf(TEXT("#define %s_BASE_TEXTURE_ALPHA %d\r\n"), *InPrefix, (INT)MVS_BaseTextureAlpha );
	DefinesString += FString::Printf(TEXT("#define %s_MASK_TEXTURE_RED %d\r\n"), *InPrefix, (INT)MVS_MaskTextureRed );
	DefinesString += FString::Printf(TEXT("#define %s_MASK_TEXTURE_GREEN %d\r\n"), *InPrefix, (INT)MVS_MaskTextureGreen );
	DefinesString += FString::Printf(TEXT("#define %s_MASK_TEXTURE_BLUE %d\r\n"), *InPrefix, (INT)MVS_MaskTextureBlue );
	DefinesString += FString::Printf(TEXT("#define %s_MASK_TEXTURE_ALPHA %d\r\n"), *InPrefix, (INT)MVS_MaskTextureAlpha );
	DefinesString += FString::Printf(TEXT("#define %s_NORMAL_TEXTURE_ALPHA %d\r\n"), *InPrefix, (INT)MVS_NormalTextureAlpha );
	DefinesString += FString::Printf(TEXT("#define %s %d\r\n"), *InPrefix, InValue );
}


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
 * @param MobileSettingsMobileLODBias,   from settings
 * @param MobileSettingsMobileBoneCount,	from settings
 * @param MobileSettingsMobileBoneWeightCount,	from settings
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
	)
{
	const UBOOL bUseLighting = KeyData.FieldValue[PKDT_IsLightingEnabled];
	const UBOOL bUseLightmap = bUseLighting && KeyData.FieldValue[PKDT_IsLightmap];//( BaseFeatures & EShaderBaseFeatures::Lightmap ) != 0;
	const UBOOL bUseVertexLightmap = FALSE;//bUseLighting && ( BaseFeatures & EShaderBaseFeatures::VertexLightmap ) != 0;
	const UBOOL bUseGPUSkinning = KeyData.FieldValue[PKDT_IsSkinned];//( BaseFeatures & EShaderBaseFeatures::GPUSkinning ) != 0;
	const UBOOL bIsDecal = KeyData.FieldValue[PKDT_IsDecal];//( BaseFeatures & EShaderBaseFeatures::Decal ) != 0;
	const UBOOL bUseSubUVParticles = KeyData.FieldValue[PKDT_IsSubUV];//( BaseFeatures & EShaderBaseFeatures::SubUVParticles ) != 0;
	const UBOOL bUseDynamicDirectionalLight = bUseLighting && !bUseLightmap && !bUseVertexLightmap;
	const UBOOL bUseDynamicSkyLight = bUseLighting && !bUseLightmap && !bUseVertexLightmap;
	const EMobileGlobalShaderType GlobalShaderType = (EMobileGlobalShaderType) KeyData.FieldValue[PKDT_GlobalShaderType];

	// make a string of defines
	FString DefinesString;

	// Platform
	if ( Platform & UE3::PLATFORM_NGP )
	{
		DefinesString += TEXT("#define NGP 1\r\n");
	}

	// Pass features
	UBOOL bIsDepthOnly = KeyData.FieldValue[PKDT_IsDepthOnlyRendering] ? 1 : 0;
	DefinesString += FString::Printf(TEXT("#define DEPTH_ONLY %d\r\n"), bIsDepthOnly ? 1 : 0);

	// Base features
	DefinesString += FString::Printf(TEXT("#define USE_LIGHTMAP %d\r\n"), bUseLightmap ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_VERTEX_LIGHTMAP %d\r\n"), bUseVertexLightmap ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_GPU_SKINNING %d\r\n"), bUseGPUSkinning ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define SUBUV_PARTICLES %d\r\n"), bUseSubUVParticles ? 1 : 0);

	// Decals
	DefinesString += FString::Printf(TEXT("#define IS_DECAL %d\r\n"), bIsDecal ? 1 : 0);

	// Pixel shader: Default texture sampling LOD bias to use when sampling most textures
	DefinesString += FString::Printf(TEXT("#define DEFAULT_LOD_BIAS %0.2f\r\n"), MobileSettingsMobileLODBias );

	if( bUseGPUSkinning )
	{
		DefinesString += FString::Printf(TEXT("#define MAX_BONES %d\r\n"), MobileSettingsMobileBoneCount);
		DefinesString += FString::Printf(TEXT("#define MAX_BONE_WEIGHTS %d\r\n"), 
			bIsDepthOnly ? 1 : MobileSettingsMobileBoneWeightCount);
	}


	// Instance features
	DefinesString += FString::Printf(TEXT("#define USE_GRADIENT_FOG %d\r\n"), KeyData.FieldValue[PKDT_IsGradientFogEnabled] ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_ALPHAKILL %d\r\n"), KeyData.FieldValue[PKDT_IsAlphaTestEnabled] ? 1 : 0);


	// Texture coordinates mapping
	{
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_SOURCE_TEXCOORDS0 %d\r\n"), 0 );
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_SOURCE_TEXCOORDS1 %d\r\n"), 1 );
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_SOURCE_TEXCOORDS2 %d\r\n"), 2 );
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_SOURCE_TEXCOORDS3 %d\r\n"), 3 );

		// Base texture texture coordinate source
		DefinesString += FString::Printf(TEXT("#define BASE_TEXTURE_TEXCOORDS_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_BaseTextureTexCoordsSource]);

		// Detail texture texture coordinate source
		DefinesString += FString::Printf(TEXT("#define DETAIL_TEXTURE_TEXCOORDS_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_DetailTextureTexCoordsSource]);

		// Mask texture texture coordinate source
		DefinesString += FString::Printf(TEXT("#define MASK_TEXTURE_TEXCOORDS_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_MaskTextureTexCoordsSource]);
	}

	UBOOL bIsTextureCoordinateTransformEnabled = KeyData.FieldValue[PKDT_IsTextureCoordinateTransformEnabled];
	DefinesString += FString::Printf(TEXT("#define USE_TEXCOORD_XFORM %d\r\n"), bIsTextureCoordinateTransformEnabled ? 1 : 0);
	if( bIsTextureCoordinateTransformEnabled )
	{
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_XFORM_TARGET_BASE %d\r\n"), (INT)MTTT_BaseTexture );
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_XFORM_TARGET_DETAIL %d\r\n"), (INT)MTTT_DetailTexture );
		DefinesString += FString::Printf(TEXT("#define TEXCOORD_XFORM_TARGET %d\r\n"), KeyData.FieldValue[PKDT_TextureTransformTarget]);
	}

	const UBOOL bIsNormalMappingEnabled = KeyData.FieldValue[PKDT_IsNormalMappingEnabled];
	const UBOOL bIsEnvironmentMappingEnabled = KeyData.FieldValue[PKDT_IsEnvironmentMappingEnabled];

	// Specular
	{
		const UBOOL bIsPixelSpecularEnabled = KeyData.FieldValue[PKDT_IsSpecularEnabled] && KeyData.FieldValue[PKDT_IsPixelSpecularEnabled] && bIsNormalMappingEnabled;
		const UBOOL bIsVertexSpecularEnabled = KeyData.FieldValue[PKDT_IsSpecularEnabled] && !bIsPixelSpecularEnabled;
		const UBOOL bIsSpecularEnabled = bIsPixelSpecularEnabled || bIsVertexSpecularEnabled;
		DefinesString += FString::Printf(TEXT("#define USE_SPECULAR %d\r\n"), bIsSpecularEnabled ? 1 : 0);
		DefinesString += FString::Printf(TEXT("#define USE_VERTEX_SPECULAR %d\r\n"), bIsVertexSpecularEnabled ? 1 : 0);
		DefinesString += FString::Printf(TEXT("#define USE_PIXEL_SPECULAR %d\r\n"), bIsPixelSpecularEnabled ? 1 : 0);

		// We allow specular mask for both specular and character lighting map ('fake spec') features
		const UBOOL bIsSpecularMaskSupported = bIsSpecularEnabled;
		if( bIsSpecularMaskSupported )
		{
			// This string maps EMobileSpecularMask to #define values that we can switch off of in our shader
			FString SpecularMaskEnumDefineString;
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_CONSTANT %d\r\n" ), (INT)MSM_Constant );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_LUMINANCE %d\r\n" ), (INT)MSM_Luminance );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_DIFFUSE_RED %d\r\n" ), (INT)MSM_DiffuseRed );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_DIFFUSE_GREEN %d\r\n" ), (INT)MSM_DiffuseGreen );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_DIFFUSE_BLUE %d\r\n" ), (INT)MSM_DiffuseBlue );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_DIFFUSE_ALPHA %d\r\n" ), (INT)MSM_DiffuseAlpha );
			SpecularMaskEnumDefineString += FString::Printf( TEXT( "#define SPECMASK_MASK_TEXTURE_RGB %d\r\n" ), (INT)MSM_MaskTextureRGB );

			DefinesString += SpecularMaskEnumDefineString;
			DefinesString += FString::Printf(TEXT("#define SPECULAR_MASK %d\r\n"), KeyData.FieldValue[PKDT_SpecularMask]);
		}
	}

	DefinesString += FString::Printf(TEXT("#define USE_NORMAL_MAPPING %d\r\n"), bIsNormalMappingEnabled ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_ENVIRONMENT_MAPPING %d\r\n"), bIsEnvironmentMappingEnabled ? 1 : 0);
	if( bIsEnvironmentMappingEnabled )
	{
		CreateValueSourceDefines( DefinesString, TEXT("ENVIRONMENT_MASK_SOURCE"), (INT)KeyData.FieldValue[PKDT_EnvironmentMaskSource] );

		DefinesString += FString::Printf(TEXT("#define ENVIRONMENT_BLEND_ADD %d\r\n"), (INT)MEBM_Add );
		DefinesString += FString::Printf(TEXT("#define ENVIRONMENT_BLEND_LERP %d\r\n"), (INT)MEBM_Lerp );
		DefinesString += FString::Printf(TEXT("#define ENVIRONMENT_BLEND_MODE %d\r\n"), KeyData.FieldValue[PKDT_MobileEnvironmentBlendMode]);

		const UBOOL bUseEnvironmentFresnel = bIsEnvironmentMappingEnabled && KeyData.FieldValue[PKDT_IsMobileEnvironmentFresnelEnabled];
		DefinesString += FString::Printf(TEXT("#define USE_ENVIRONMENT_FRESNEL %d\r\n"), bUseEnvironmentFresnel ? 1 : 0);
	}


	const UBOOL bIsEmissiveEnabled = KeyData.FieldValue[PKDT_IsEmissiveEnabled];
	DefinesString += FString::Printf(TEXT("#define USE_EMISSIVE %d\r\n"), bIsEmissiveEnabled ? 1 : 0);
	if( bIsEmissiveEnabled )
	{
		CreateValueSourceDefines( DefinesString, TEXT("EMISSIVE_MASK_SOURCE"), (INT)KeyData.FieldValue[PKDT_EmissiveMaskSource] );

		DefinesString += FString::Printf(TEXT("#define EMISSIVE_COLOR_SOURCE_EMISSIVE_TEXTURE %d\r\n"), (INT)MECS_EmissiveTexture );
		DefinesString += FString::Printf(TEXT("#define EMISSIVE_COLOR_SOURCE_BASE_TEXTURE %d\r\n"), (INT)MECS_BaseTexture );
		DefinesString += FString::Printf(TEXT("#define EMISSIVE_COLOR_SOURCE_CONSTANT %d\r\n"), (INT)MECS_Constant );
		DefinesString += FString::Printf(TEXT("#define EMISSIVE_COLOR_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_EmissiveColorSource] );
	}


	const UBOOL bIsRimLightingEnabled = KeyData.FieldValue[PKDT_IsRimLightingEnabled];
	DefinesString += FString::Printf(TEXT("#define USE_RIM_LIGHTING %d\r\n"), bIsRimLightingEnabled ? 1 : 0);
	if( bIsRimLightingEnabled )
	{
		CreateValueSourceDefines( DefinesString, TEXT("RIM_LIGHTING_MASK_SOURCE"), (INT)KeyData.FieldValue[PKDT_RimLightingMaskSource] );
	}


	DefinesString += FString::Printf(TEXT("#define USE_BUMP_OFFSET %d\r\n"), KeyData.FieldValue[PKDT_IsBumpOffsetEnabled] ? 1 : 0);

	UBOOL bIsColorTextureBlendingEnabled = KeyData.FieldValue[PKDT_IsColorTextureBlendingEnabled];
	DefinesString += FString::Printf(TEXT("#define USE_COLOR_TEXTURE_BLENDING %d\r\n"), bIsColorTextureBlendingEnabled ? 1 : 0);
	if( bIsColorTextureBlendingEnabled )
	{
		DefinesString += FString::Printf(TEXT("#define TEXTURE_BLEND_FACTOR_SOURCE_VERTEX_COLOR %d\r\n"), (INT)MTBFS_VertexColor );
		DefinesString += FString::Printf(TEXT("#define TEXTURE_BLEND_FACTOR_SOURCE_MASK_TEXTURE %d\r\n"), (INT)MTBFS_MaskTexture );
		DefinesString += FString::Printf(TEXT("#define TEXTURE_BLEND_FACTOR_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_TextureBlendFactorSource]);
	}
	DefinesString += FString::Printf(TEXT("#define USE_PREMULTIPLIED_OPACITY %d\r\n"), KeyData.FieldValue[PKDT_UsingAdditiveMaterial] ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_VERTEX_MOVEMENT %d\r\n"), KeyData.FieldValue[PKDT_IsWaveVertexMovementEnabled] ? 1 : 0);
	if (bUseLightmap || bUseVertexLightmap)
	{
		const UBOOL bUseFixedLightmapScale = TRUE;
		INT FixedScaleValue = 2;
		DefinesString += FString::Printf(TEXT("#define USE_LIGHTMAP_FIXED_SCALE %d\r\n"), bUseFixedLightmapScale ? 1 : 0);
		DefinesString += FString::Printf(TEXT("#define LIGHTMAP_FIXED_SCALE_VALUE %d\r\n"), FixedScaleValue);

		DefinesString += FString::Printf(TEXT("#define LIGHTMAP_STORED_IN_LINEAR_SPACE %d\r\n"), 0);//KeyData.FieldValue[PKDT_SimpleLightmapsStoredInLinearSpace] ? 1 : 0);
	}
	DefinesString += FString::Printf(TEXT("#define USE_DYNAMIC_DIRECTIONAL_LIGHT %d\r\n"), bUseDynamicDirectionalLight ? 1 : 0);
	DefinesString += FString::Printf(TEXT("#define USE_DYNAMIC_SKY_LIGHT %d\r\n"), bUseDynamicSkyLight ? 1 : 0);

	// Ambient occlusion
	const UBOOL bUseAmbientOcclusion = bUseLighting && ( (EMobileAmbientOcclusionSource)KeyData.FieldValue[PKDT_AmbientOcclusionSource] != MAOS_Disabled );
	DefinesString += FString::Printf(TEXT("#define USE_AMBIENT_OCCLUSION %d\r\n"), bUseAmbientOcclusion ? 1 : 0);
	if( bUseAmbientOcclusion )
	{
		// This string maps EMobileAmbientOcclusionSource to #define values that we can switch off of in our shader
		FString AOSourceEnumDefineString;
		AOSourceEnumDefineString += FString::Printf( TEXT( "#define AOSOURCE_VERTEX_COLOR_RED %d\r\n" ), (INT)MAOS_VertexColorRed );
		AOSourceEnumDefineString += FString::Printf( TEXT( "#define AOSOURCE_VERTEX_COLOR_GREEN %d\r\n" ), (INT)MAOS_VertexColorGreen );
		AOSourceEnumDefineString += FString::Printf( TEXT( "#define AOSOURCE_VERTEX_COLOR_BLUE %d\r\n" ), (INT)MAOS_VertexColorBlue );
		AOSourceEnumDefineString += FString::Printf( TEXT( "#define AOSOURCE_VERTEX_COLOR_ALPHA %d\r\n" ), (INT)MAOS_VertexColorAlpha );

		DefinesString += AOSourceEnumDefineString;
		DefinesString += FString::Printf(TEXT("#define AMBIENT_OCCLUSION_SOURCE %d\r\n"), KeyData.FieldValue[PKDT_AmbientOcclusionSource]);
	}

	//Color multiply
	{
		DefinesString += FString::Printf(TEXT("#define USE_UNIFORM_COLOR_MULTIPLY %d\r\n"), KeyData.FieldValue[PKDT_UseUniformColorMultiply] ? 1 : 0);
		DefinesString += FString::Printf(TEXT("#define USE_VERTEX_COLOR_MULTIPLY %d\r\n"), KeyData.FieldValue[PKDT_UseVertexColorMultiply] ? 1 : 0);
	}

	// Screen alignment
	{
		// This string maps EMobileParticleScreenAlignment to #define values that we can switch off of in our shader
		FString ParticleScreenAlignmentEnumDefineString;
		ParticleScreenAlignmentEnumDefineString += FString::Printf( TEXT( "#define PARTICLESCREENALIGN_CAMERAFACING %d\r\n" ), (INT)SSA_CameraFacing );
		ParticleScreenAlignmentEnumDefineString += FString::Printf( TEXT( "#define PARTICLESCREENALIGN_VELOCITY %d\r\n" ), (INT)SSA_Velocity );
		ParticleScreenAlignmentEnumDefineString += FString::Printf( TEXT( "#define PARTICLESCREENALIGN_LOCKEDAXIS %d\r\n" ), (INT)SSA_LockedAxis );
		DefinesString += ParticleScreenAlignmentEnumDefineString;
		DefinesString += FString::Printf(TEXT("#define PARTICLE_SCREEN_ALIGNMENT %d\r\n"), KeyData.FieldValue[PKDT_ParticleScreenAlignment]);
	}


	// Windows OpenGL ES2 implementations do not support overriding the floating point precision, so we only set
	// these when compiling shaders for console targets.
	FString VertexShaderDefs, PixelShaderDefs;
	if ( !bIsCompilingForPC && !(Platform & UE3::PLATFORM_NGP) )
	{
		VertexShaderDefs = TEXT( "precision highp float;\r\n" );
		PixelShaderDefs = TEXT( "precision mediump float;\r\n" );
	}

	// load the actual shader files
	FString VertexShaderFilename, PixelShaderFilename;
	if ( Platform & UE3::PLATFORM_NGP )
	{
		VertexShaderFilename = appEngineDir() + TEXT("Shaders\\NGP\\") + GetES2ShaderFilename(PrimitiveType,GlobalShaderType,SF_Vertex) + TEXT(".usf");
		PixelShaderFilename = appEngineDir() + TEXT("Shaders\\NGP\\") + GetES2ShaderFilename(PrimitiveType,GlobalShaderType,SF_Pixel) + TEXT(".usf");
	}
	else
	{
		VertexShaderFilename = appEngineDir() + TEXT("Shaders\\ES2\\") + GetES2ShaderFilename(PrimitiveType,GlobalShaderType,SF_Vertex) + TEXT(".glsl");
		PixelShaderFilename = appEngineDir() + TEXT("Shaders\\ES2\\") + GetES2ShaderFilename(PrimitiveType,GlobalShaderType,SF_Pixel) + TEXT(".glsl");
	}
	FString VertexShaderText;
	if (!appLoadFileToString(VertexShaderText, *VertexShaderFilename))
	{
		appErrorf( TEXT("Failed to load shader file '%s'"),*VertexShaderFilename);
		return FALSE;
	}
	FString PixelShaderText;
	if (!appLoadFileToString(PixelShaderText, *PixelShaderFilename))
	{
		appErrorf( TEXT("Failed to load shader file '%s'"),*PixelShaderFilename);
		return FALSE;
	}
	OutVertexShader = DefinesString + TEXT("\r\n") + VertexShaderDefs + TEXT("\r\n") + CommonShaderPrefixFile + TEXT("\r\n") + VertexShaderPrefixFile + TEXT("\r\n") + VertexShaderText;
	OutPixelShader = DefinesString + TEXT("\r\n") + PixelShaderDefs + TEXT("\r\n") + CommonShaderPrefixFile + TEXT("\r\n") + PixelShaderPrefixFile + TEXT("\r\n") + PixelShaderText;
	return TRUE;
}
