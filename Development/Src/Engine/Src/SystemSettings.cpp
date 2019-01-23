/*=============================================================================
	ScalabilityOptions.cpp: Unreal engine HW compat scalability system.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnTerrain.h"
#include "EngineSpeedTreeClasses.h"
#include "EngineDecalClasses.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

/** Global scalability object. */
FSystemSettings GSystemSettings;

/*-----------------------------------------------------------------------------
	FSystemSettings
-----------------------------------------------------------------------------*/

#define PROPERTY_TEXT_TO_ADDRESS_MAPPING										\
	/** Helper struct for all switches/ boolean config values. */				\
	struct { const TCHAR* Name; UBOOL* SwitchPtr; } Switches[] =				\
	{																			\
		{ TEXT("STATICDECALS")				, &bAllowStaticDecals },			\
		{ TEXT("DYNAMICDECALS")				, &bAllowDynamicDecals },			\
		{ TEXT("DYNAMICLIGHTS")				, &bAllowDynamicLights },			\
		{ TEXT("DYNAMICSHADOWS")			, &bAllowDynamicShadows },			\
		{ TEXT("LIGHTENVIRONMENTSHADOWS")	, &bAllowLightEnvironmentShadows },	\
		{ TEXT("COMPOSITEDYNAMICLIGHTS")	, &bUseCompositeDynamicLights },	\
		{ TEXT("DIRECTIONALLIGHTMAPS")		, &bAllowDirectionalLightMaps },	\
		{ TEXT("MOTIONBLUR")				, &bAllowMotionBlur },				\
		{ TEXT("DEPTHOFFIELD")				, &bAllowDepthOfField },			\
		{ TEXT("BLOOM")						, &bAllowBloom },					\
		{ TEXT("QUALITYBLOOM")				, &bUseHighQualityBloom },			\
		{ TEXT("SPEEDTREELEAVES")			, &bAllowSpeedTreeLeaves },			\
		{ TEXT("SPEEDTREEFRONDS")			, &bAllowSpeedTreeFronds },			\
		{ TEXT("ONLYSTREAMINTEXTURES")		, &bOnlyStreamInTextures },			\
		{ TEXT("LENSFLARES")				, &bAllowLensFlares },				\
	};																			\
																				\
	/** Helper struct for all integer config values */							\
	struct { const TCHAR* Name; INT* IntValuePtr; } IntValues[] =				\
	{																			\
		{ TEXT("SKELETALMESHLODBIAS")		, &SkeletalMeshLODBias },			\
		{ TEXT("PARTICLELODBIAS")			, &ParticleLODBias },				\
		{ TEXT("DETAILMODE")				, &DetailMode },					\
	};																			\
																				\
	/** Helper struct for all float config values. */							\
	struct { const TCHAR* Name; FLOAT* FloatValuePtr; } FloatValues[] =			\
	{																			\
		{ TEXT("SCREENPERCENTAGE")			, &ScreenPercentage },				\
	};																			\



/** Constructor, initializing all member variables. */
FSystemSettings::FSystemSettings()
// Scalability options
:	bAllowStaticDecals				( TRUE )
,	bAllowDynamicDecals				( TRUE )
,	bAllowDynamicLights				( TRUE )
,	bAllowDynamicShadows			( TRUE )
,	bAllowLightEnvironmentShadows	( TRUE )
,	bUseCompositeDynamicLights		( FALSE )
,	ShadowFilterQualityBias			( 0 )
,	bAllowDirectionalLightMaps		( TRUE )
,	bAllowMotionBlur				( TRUE )
,	bAllowDepthOfField				( TRUE )
,	bAllowBloom						( TRUE )
,	bUseHighQualityBloom			( TRUE )
,	bAllowSpeedTreeLeaves			( TRUE )
,	bAllowSpeedTreeFronds			( TRUE )
,	bOnlyStreamInTextures			( FALSE ) // @todo: enabling it consumes a lot of virtual address space !CONSOLE )
,	bAllowLensFlares				( TRUE )
,	SkeletalMeshLODBias				( 0 )
,	ParticleLODBias					( 0 )
,	ScreenPercentage				( 100.f )
#if PS3
,	DetailMode						( DM_Low )
#elif XBOX
,	DetailMode						( DM_Medium )
#else
,	DetailMode						( DM_High )
#endif
// System configuration
,	CPUMemory						( 0 )
,	CPUFrequency					( 0 )
,	CPUCount						( 0 )
,	CPUVendorString					( TEXT("Unknown") )
,	GPUShaderModel					( 0 )
,	GPUMemory						( 0 )
,	GPUVendorId						( INDEX_NONE )
,	GPUDeviceId						( INDEX_NONE )
{}

/**
 * Initializes system settings and included texture LOD settings.
 *
 * @param bSetupForEditor	Whether to initialize settings for Editor
 *
 */
void FSystemSettings::Initialize( UBOOL bSetupForEditor )
{
	PROPERTY_TEXT_TO_ADDRESS_MAPPING;

	// Always stream out textures in the Editor. We don't use GIsEditor as it's not set at this point.
	bOnlyStreamInTextures = bOnlyStreamInTextures && !bSetupForEditor;

	// Initialize texture LOD settings before any are loaded from disk.
	TextureLODSettings.Initialize( GEngineIni, TEXT("TextureLODSettings") );

	// Store detail mode as we don't allow overriding via ini at the moment.
	INT OldDetailMode = DetailMode;

	// Read booleans from .ini.
	for( INT SwitchIndex=0; SwitchIndex<ARRAY_COUNT(Switches); SwitchIndex++ )
	{
		GConfig->GetBool( TEXT("SystemSettings"), Switches[SwitchIndex].Name, *Switches[SwitchIndex].SwitchPtr, GEngineIni );
	}
	// Read int values from ini.
	for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
	{
		GConfig->GetInt( TEXT("SystemSettings"), IntValues[IntValueIndex].Name, *IntValues[IntValueIndex].IntValuePtr, GEngineIni );
	}
	// Read float values from .ini.
	for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
	{
		GConfig->GetFloat( TEXT("SystemSettings"), FloatValues[FloatValueIndex].Name, *FloatValues[FloatValueIndex].FloatValuePtr, GEngineIni );
	}

	// Restore detail mode.
	DetailMode = OldDetailMode;
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

	// Keep track of previous detail mode.
	INT OldDetailMode = DetailMode;
	UBOOL bOldAllowDirectionalLightMaps = bAllowDirectionalLightMaps;

	//Some of these settings are used in both threads so we need to stop the rendering thread before changing them.
	FlushRenderingCommands();

	// Keep track whether the command was handled or not.
	UBOOL bHandledCommand = FALSE;

	if( ParseCommand(&Cmd,TEXT("SCALE")) )
	{
		if( ParseCommand(&Cmd,TEXT("FASTEST")) )
		{
			bAllowStaticDecals				= FALSE;
			bAllowDynamicDecals				= FALSE;
			bAllowDynamicLights				= FALSE;
			bAllowDynamicShadows			= FALSE;
			bAllowLightEnvironmentShadows	= FALSE;
			bUseCompositeDynamicLights		= TRUE;
			bAllowDirectionalLightMaps		= FALSE;
			bAllowMotionBlur				= FALSE;
			bAllowDepthOfField				= FALSE;
			bAllowBloom						= FALSE;
			bUseHighQualityBloom			= FALSE;
			bAllowSpeedTreeLeaves			= FALSE;
			bAllowSpeedTreeFronds			= FALSE;
			bOnlyStreamInTextures			= FALSE;
			bAllowLensFlares				= FALSE;
			SkeletalMeshLODBias				= INT_MAX;
			ParticleLODBias					= INT_MAX;
			ScreenPercentage				= 50.f;
			DetailMode						= DM_Low;
			bHandledCommand					= TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("LOWEND")) )
		{
			bAllowStaticDecals				= FALSE;
			bAllowDynamicDecals				= FALSE;
			bAllowDynamicLights				= FALSE;
			bAllowDynamicShadows			= FALSE;
			bAllowLightEnvironmentShadows	= FALSE;
			bUseCompositeDynamicLights		= TRUE;
			bAllowDirectionalLightMaps		= FALSE;
			bAllowMotionBlur				= FALSE;
			bAllowDepthOfField				= TRUE;
			bAllowBloom						= TRUE;
			bUseHighQualityBloom			= FALSE;
			bAllowSpeedTreeLeaves			= FALSE;
			bAllowSpeedTreeFronds			= FALSE;
			bOnlyStreamInTextures			= FALSE;
			bAllowLensFlares				= FALSE;
			SkeletalMeshLODBias				= 1;
			ParticleLODBias					= 2;
			ScreenPercentage				= 75.f;
			DetailMode						= DM_Low;
			bHandledCommand					= TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("PRETTIEST")) || ParseCommand(&Cmd,TEXT("HIGHEND")) )
		{
			bAllowStaticDecals				= TRUE;
			bAllowDynamicDecals				= TRUE;
			bAllowDynamicLights				= TRUE;
			bAllowDynamicShadows			= TRUE;
			bAllowLightEnvironmentShadows	= TRUE;
			bUseCompositeDynamicLights		= FALSE;
			bAllowDirectionalLightMaps		= TRUE;
			bAllowMotionBlur				= TRUE;
			bAllowDepthOfField				= TRUE;
			bAllowBloom						= TRUE;
			bUseHighQualityBloom			= TRUE;
			bAllowSpeedTreeLeaves			= TRUE;
			bAllowSpeedTreeFronds			= TRUE;
			bOnlyStreamInTextures			= FALSE;
			bAllowLensFlares				= TRUE;
			SkeletalMeshLODBias				= 0;
			ParticleLODBias					= 0;
			ScreenPercentage				= 100.f;
			DetailMode						= DM_High;
			bHandledCommand					= TRUE;
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
					bHandledCommand	= TRUE;
				}
			}

			// Search for a specific int value.
			for( INT IntValueIndex=0; IntValueIndex<ARRAY_COUNT(IntValues); IntValueIndex++ )
			{
				if( ParseCommand(&Cmd,IntValues[IntValueIndex].Name) )
				{
					INT NewValue = appAtoi( Cmd );
					*IntValues[IntValueIndex].IntValuePtr = NewValue;
					bHandledCommand	= TRUE;
				}
			}

			// Search for a specific float value.
			for( INT FloatValueIndex=0; FloatValueIndex<ARRAY_COUNT(FloatValues); FloatValueIndex++ )
			{
				if( ParseCommand(&Cmd,FloatValues[FloatValueIndex].Name) )
				{
					FLOAT NewValue = appAtof( Cmd );
					*FloatValues[FloatValueIndex].FloatValuePtr = NewValue;
					bHandledCommand	= TRUE;
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
					bHandledCommand	= TRUE;
				}
			}
		}
	}

	//update lightmap resources if needed
	UpdateLightmapType(bOldAllowDirectionalLightMaps);

	// Handle changes in detail mode.
	if( OldDetailMode != DetailMode 
		|| bOldAllowDirectionalLightMaps != bAllowDirectionalLightMaps)
	{
		FGlobalComponentReattachContext PropagateDetailModeChanges;
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
	if( GSystemSettings.ScreenPercentage != 100.f )
	{
		// Clamp screen percentage to reasonable range.
		FLOAT ScaleFactor = Clamp( GSystemSettings.ScreenPercentage / 100.f, 0.0f, 100.f );

		INT	OrigX = X;
		INT OrigY = Y;
		UINT OrigSizeX = SizeX;
		UINT OrigSizeY = SizeY;

		// Scale though make sure we're at least covering 1 pixel.
		SizeX = Max(1,appTrunc(ScaleFactor * OrigSizeX));
		SizeY = Max(1,appTrunc(ScaleFactor * OrigSizeY));

		// Center scaled view.
		X = OrigX + (OrigSizeX - SizeX) / 2;
		Y = OrigY + (OrigSizeY - SizeY) / 2;
	}
}

/**
 * Updates the lightmap type on all primitives that store references to lighmaps.
 * This function will disallow changing bAllowDirectionalLightMaps on cooked builds.
 */
void FSystemSettings::UpdateLightmapType(UBOOL bOldAllowDirectionalLightMaps)
{
	if (bOldAllowDirectionalLightMaps != bAllowDirectionalLightMaps)
	{
		if (GUseSeekFreeLoading)
		{
			bAllowDirectionalLightMaps = bOldAllowDirectionalLightMaps;
			warnf(TEXT("Can't change lightmap type on cooked builds!"));
		}
		else
		{
			for(TObjectIterator<UStaticMeshComponent> ComponentIt;ComponentIt;++ComponentIt)
			{
				ComponentIt->UpdateLightmapType(bAllowDirectionalLightMaps);
			}
			for(TObjectIterator<UModelComponent> ComponentIt;ComponentIt;++ComponentIt)
			{
				ComponentIt->UpdateLightmapType(bAllowDirectionalLightMaps);
			}
			for(TObjectIterator<UTerrainComponent> ComponentIt;ComponentIt;++ComponentIt)
			{
				if (ComponentIt->LightMap)
				{		
					ComponentIt->LightMap->UpdateLightmapType(bAllowDirectionalLightMaps);
				}
			}
			for(TObjectIterator<USpeedTreeComponent> ComponentIt;ComponentIt;++ComponentIt)
			{
				if (ComponentIt->BranchAndFrondLightMap)
				{		
					ComponentIt->BranchAndFrondLightMap->UpdateLightmapType(bAllowDirectionalLightMaps);
				}
				if (ComponentIt->LeafCardLightMap)
				{		
					ComponentIt->LeafCardLightMap->UpdateLightmapType(bAllowDirectionalLightMaps);
				}
				if (ComponentIt->LeafMeshLightMap)
				{		
					ComponentIt->LeafMeshLightMap->UpdateLightmapType(bAllowDirectionalLightMaps);
				}
				if (ComponentIt->BillboardLightMap)
				{		
					ComponentIt->BillboardLightMap->UpdateLightmapType(bAllowDirectionalLightMaps);
				}
			}
			for(TObjectIterator<UDecalComponent> ComponentIt;ComponentIt;++ComponentIt)
			{
				ComponentIt->UpdateLightmapType(bAllowDirectionalLightMaps);
			}
		}
	}
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

