/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "UnTerrain.h"
#include "EngineAIClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineSoundClasses.h"
#include "EngineSoundNodeClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineParticleClasses.h"
#include "EngineAnimClasses.h"
#include "EngineDecalClasses.h"
#include "EngineFogVolumeClasses.h"
#include "EnginePrefabClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "EngineUISequenceClasses.h"
#include "EngineFoliageClasses.h"
#include "EngineSpeedTreeClasses.h"
#include "LensFlare.h"
#include "UnStatChart.h"
#include "UnNet.h"
#include "UnCodecs.h"
#include "RemoteControl.h"
#include "FFileManagerGeneric.h"
#include "DemoRecording.h"
#if !CONSOLE && defined(_MSC_VER)
#include "../../UnrealEd/Inc/DebugToolExec.h"
#endif

/*-----------------------------------------------------------------------------
	Static linking.
-----------------------------------------------------------------------------*/

#define STATIC_LINKING_MOJO 1

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "EngineGameEngineClasses.h"
#include "EngineClasses.h"
#include "EngineAIClasses.h"
#include "EngineMaterialClasses.h"
#include "EngineTerrainClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineSoundClasses.h"
#include "EngineSoundNodeClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineParticleClasses.h"
#include "EngineAnimClasses.h"
#include "EngineDecalClasses.h"
#include "EngineFogVolumeClasses.h"
#include "EnginePrefabClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "EngineUISequenceClasses.h"
#include "EngineSceneClasses.h"
#include "EngineFoliageClasses.h"
#include "EngineSpeedTreeClasses.h"
#include "EngineLensFlareClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "EngineGameEngineClasses.h"
#include "EngineClasses.h"
#include "EngineAIClasses.h"
#include "EngineMaterialClasses.h"
#include "EngineTerrainClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineSoundClasses.h"
#include "EngineSoundNodeClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineParticleClasses.h"
#include "EngineAnimClasses.h"
#include "EngineDecalClasses.h"
#include "EngineFogVolumeClasses.h"
#include "EnginePrefabClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "EngineUISequenceClasses.h"
#include "EngineSceneClasses.h"
#include "EngineFoliageClasses.h"
#include "EngineSpeedTreeClasses.h"
#include "EngineLensFlareClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NATIVES_ONLY
#undef NAMES_ONLY

/**
 * Initialize registrants, basically calling StaticClass() to create the class and also 
 * populating the lookup table.
 *
 * @param	Lookup	current index into lookup table
 */
void AutoInitializeRegistrantsEngine( INT& Lookup )
{
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_GAMEENGINE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_AI
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_ANIM
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_DECAL
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_FOGVOLUME
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_INTERPOLATION
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_MATERIAL
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_PARTICLE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_PHYSICS
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_PREFAB
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_SEQUENCE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_SOUND
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_TERRAIN
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_USERINTERFACE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_UIPRIVATE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_UISEQUENCE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_SCENE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_SOUNDNODE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_FOLIAGE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_SPEEDTREE
	AUTO_INITIALIZE_REGISTRANTS_ENGINE_LENSFLARE
}

/**
 * Auto generates names.
 */
void AutoGenerateNamesEngine()
{
	#define NAMES_ONLY
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#define AUTOGENERATE_NAME(name) ENGINE_##name = FName(TEXT(#name));
	#include "EngineGameEngineClasses.h"
	#include "EngineClasses.h"
	#include "EngineAIClasses.h"
	#include "EngineMaterialClasses.h"
	#include "EngineTerrainClasses.h"
	#include "EnginePhysicsClasses.h"
	#include "EngineSequenceClasses.h"
	#include "EngineSoundClasses.h"
	#include "EngineInterpolationClasses.h"
	#include "EngineParticleClasses.h"
	#include "EngineAnimClasses.h"
	#include "EngineDecalClasses.h"
	#include "EngineFogVolumeClasses.h"
	#include "EnginePrefabClasses.h"
	#include "EngineUserInterfaceClasses.h"
	#include "EngineUIPrivateClasses.h"
	#include "EngineUISequenceClasses.h"
	#include "EngineSceneClasses.h"
	#include "EngineFoliageClasses.h"
	#include "EngineSpeedTreeClasses.h"
	#include "EngineLensFlareClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}

// Register input keys.
#define DEFINE_KEY(Name, Unused) FName KEY_##Name;
#include "UnKeys.h"
#undef DEFINE_KEY

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

/** Global engine pointer. */
UEngine*		GEngine			= NULL;
/** TRUE if we allow shadow volumes. FALSE will strip out any vertex data needed for shadow volumes (less memory) */
UBOOL			GAllowShadowVolumes = TRUE;

#if !FINAL_RELEASE
/** TRUE if we debug material names with SCOPED_DRAW_EVENT. Toggle with "ShowMaterialDrawEvents" console command. */
UBOOL			GShowMaterialDrawEvents = FALSE;
#endif

IMPLEMENT_COMPARE_CONSTREF( FString, UnEngine, { return appStricmp(*A,*B); } )

/*-----------------------------------------------------------------------------
	Object class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UEngine);
IMPLEMENT_CLASS(UGameEngine);
IMPLEMENT_CLASS(UDebugManager);
IMPLEMENT_CLASS(USaveGameSummary);
IMPLEMENT_CLASS(UObjectReferencer);

/*-----------------------------------------------------------------------------
	Engine init and exit.
-----------------------------------------------------------------------------*/

//
// Construct the engine.
//
UEngine::UEngine()
{
}

//
// Init class.
//
void UEngine::StaticConstructor()
{
}

//
// Initialize the engine.
//
void UEngine::Init()
{
	// Add input key names.
	#define DEFINE_KEY(Name, Unused) KEY_##Name = FName(TEXT(#Name));
	#include "UnKeys.h"
	#undef DEFINE_KEY

	// Subsystems.
	FURL::StaticInit();
	GEngineMem.Init( 65536 );
	
	// Initialize random number generator.
	if( GIsBenchmarking || ParseParam(appCmdLine(),TEXT("FIXEDSEED")) )
	{
		appRandInit( 0 );
		appSRandInit( 0 );
	}
	else
	{
		appRandInit( appCycles() );
		appSRandInit( appCycles() );
	}

	// Add to root.
	AddToRoot();

	if( !GIsUCCMake )
	{	
		// Make sure the engine is loaded, because some classes need GEngine->DefaultMaterial to be valid
		LoadObject<UClass>(UEngine::StaticClass()->GetOuter(), *UEngine::StaticClass()->GetName(), NULL, LOAD_Quiet|LOAD_NoWarn, NULL );
		// This reads the Engine.ini file to get the proper DefaultMaterial, etc.
		LoadConfig();
		// Initialize engine's object references.
		InitializeObjectReferences();

		// Ensure all native classes are loaded.
		UObject::BeginLoad();
		for( TObjectIterator<UClass> It; It; ++It )
		{
			UClass* Class = *It;
			if( !Class->GetLinker() )
			{
				LoadObject<UClass>( Class->GetOuter(), *Class->GetName(), NULL, LOAD_Quiet|LOAD_NoWarn, NULL );
			}
		}
		UObject::EndLoad();

		for ( TObjectIterator<UClass> It; It; ++It )
		{
			It->ConditionalLink();
		}

		// Create debug manager object
		DebugManager = ConstructObject<UDebugManager>( UDebugManager::StaticClass() );
#if !CONSOLE && defined(_MSC_VER)
		// Create debug exec.	
		GDebugToolExec = new FDebugToolExec;
#endif		

#if USING_REMOTECONTROL
		// Create RemoteControl.
		if ( !GIsUCC )
		{
			RegisterCoreRemoteControlPages();
			RemoteControlExec = new FRemoteControlExec;
		}
#endif

		// Create the global chart-drawing object.
		GStatChart = new FStatChart();
	}

	if( GIsEditor && !GIsUCCMake )
	{
		UWorld::CreateNew();
	}

	debugf( NAME_Init, TEXT("UEngine initialized") );
}

/**
 * Loads a special material and verifies that it is marked as a special material (some shaders
 * will only be compiled for materials marked as "special engine material")
 *
 * @param MaterialName Fully qualified name of a material to load/find
 * @param Material Reference to a material object pointer that will be filled out
 * @param bCheckUsage Check if the material has been marked to be used as a special engine material
 */
void LoadSpecialMaterial(const FString& MaterialName, UMaterial*& Material, UBOOL bCheckUsage)
{
	// only bother with materials that aren't already loaded
	if (Material == NULL)
	{
		// find or load the object
		Material = LoadObject<UMaterial>(NULL, *MaterialName, NULL, LOAD_None, NULL);	

		if (!Material)
		{
#if CONSOLE
			debugf(TEXT("ERROR: Failed to load special material '%s'. This will probably have bad consequences (depending on its use)"), *MaterialName);
#else
			appErrorf(TEXT("Failed to load special material '%s'"), *MaterialName);
#endif
		}
		// if the material wasn't marked as being a special engine material, then not all of the shaders 
		// will have been compiled on it by this point, so we need to compile them and alert the use
		// to set the bit
		else if (!Material->bUsedAsSpecialEngineMaterial && bCheckUsage) 
		{
#if CONSOLE
			// consoles must have the flag set properly in the editor
			appErrorf(TEXT("The special material (%s) was not marked with bUsedAsSpecialEngineMaterial. Make sure this flag is set in the editor, save the package, and compile shaders for this platform"), *MaterialName);
#else
			Material->bUsedAsSpecialEngineMaterial = TRUE;
			Material->MarkPackageDirty();

			// make sure all necessary shaders for the default are compiled, now that the flag is set
			Material->CacheResourceShaders();

			appMsgf(AMT_OK, TEXT("The special material (%s) has not been marked with bUsedAsSpecialEngineMaterial.\nThis will prevent shader precompiling properly, so the flag has been set automatically.\nMake sure to save the package and distribute to everyone using this material."), *MaterialName);
#endif
		}
	}
}
/**
 * Loads all Engine object references from their corresponding config entries.
 */
void UEngine::InitializeObjectReferences()
{
	// initialize the special engine/editor materials
	LoadSpecialMaterial(DefaultMaterialName, DefaultMaterial, TRUE);
	LoadSpecialMaterial(WireframeMaterialName, WireframeMaterial, TRUE);
	LoadSpecialMaterial(EmissiveTexturedMaterialName, EmissiveTexturedMaterial, FALSE);
	LoadSpecialMaterial(LevelColorationLitMaterialName, LevelColorationLitMaterial, TRUE);
	LoadSpecialMaterial(LevelColorationUnlitMaterialName, LevelColorationUnlitMaterial, TRUE);
	LoadSpecialMaterial(SceneCaptureReflectActorMaterialName, SceneCaptureReflectActorMaterial, FALSE);
	LoadSpecialMaterial(SceneCaptureCubeActorMaterialName, SceneCaptureCubeActorMaterial, FALSE);
	LoadSpecialMaterial(TerrainCollisionMaterialName, TerrainCollisionMaterial, TRUE);
	LoadSpecialMaterial(TerrainErrorMaterialName, TerrainErrorMaterial, TRUE);
	LoadSpecialMaterial(DefaultFogVolumeMaterialName, DefaultFogVolumeMaterial, FALSE);
	LoadSpecialMaterial(DefaultUICaretMaterialName, DefaultUICaretMaterial, FALSE);

	if (GIsEditor == TRUE)
	{
		LoadSpecialMaterial(GeomMaterialName, GeomMaterial, FALSE);
		LoadSpecialMaterial(TickMaterialName, TickMaterial, FALSE);
		LoadSpecialMaterial(CrossMaterialName, CrossMaterial, FALSE);
		LoadSpecialMaterial(EditorBrushMaterialName, EditorBrushMaterial, FALSE);

		if ( !RemoveSurfaceMaterial )
		{
			RemoveSurfaceMaterial = LoadObject<UMaterial>(NULL, *RemoveSurfaceMaterialName, NULL, LOAD_None, NULL);	
		}
	}
	else
	{
		LoadSpecialMaterial(DefaultMaterialName, GeomMaterial, FALSE);
		LoadSpecialMaterial(DefaultMaterialName, TickMaterial, FALSE);
		LoadSpecialMaterial(DefaultMaterialName, CrossMaterial, FALSE);
		LoadSpecialMaterial(DefaultMaterialName, EditorBrushMaterial, FALSE);
	}

	if( RandomAngleTexture == NULL )
	{
		RandomAngleTexture = LoadObject<UTexture2D>(NULL, *RandomAngleTextureName, NULL, LOAD_None, NULL);	
	}

	if ( DefaultPhysMaterial == NULL )
	{
		DefaultPhysMaterial = LoadObject<UPhysicalMaterial>(NULL, *DefaultPhysMaterialName, NULL, LOAD_None, NULL);	
	}
	if ( ConsoleClass == NULL )
	{
		ConsoleClass = LoadClass<UConsole>(NULL, *ConsoleClassName, NULL, LOAD_None, NULL);
	}

	if ( GameViewportClientClass == NULL )
	{
		GameViewportClientClass = LoadClass<UGameViewportClient>(NULL, *GameViewportClientClassName, NULL, LOAD_None, NULL);
	}

	if ( LocalPlayerClass == NULL )
	{
		LocalPlayerClass = LoadClass<ULocalPlayer>(NULL, *LocalPlayerClassName, NULL, LOAD_None, NULL);
	}

	if ( DataStoreClientClass == NULL )
	{
		DataStoreClientClass = LoadClass<UDataStoreClient>(NULL, *DataStoreClientClassName, NULL, LOAD_None, NULL);
	}

	if (OnlineSubsystemClass == NULL && DefaultOnlineSubsystemName.Len())
	{
		OnlineSubsystemClass = LoadClass<UOnlineSubsystem>(NULL, *DefaultOnlineSubsystemName, NULL, LOAD_None, NULL);
	}

	// Load the default engine post process chain used for the game and main editor view
	if( DefaultPostProcess == NULL && DefaultPostProcessName.Len() )
	{
		DefaultPostProcess = LoadObject<UPostProcessChain>(NULL,*DefaultPostProcessName,NULL,LOAD_None,NULL);
	}

	if (GIsEditor == TRUE)
	{
		// Load the post process chain used for skeletal mesh thumbnails
		if( ThumbnailSkeletalMeshPostProcess == NULL && ThumbnailSkeletalMeshPostProcessName.Len() )
		{
			ThumbnailSkeletalMeshPostProcess = LoadObject<UPostProcessChain>(NULL,*ThumbnailSkeletalMeshPostProcessName,NULL,LOAD_None,NULL);
		}
		// Load the post process chain used for particle system thumbnails
		if( ThumbnailParticleSystemPostProcess == NULL && ThumbnailParticleSystemPostProcessName.Len() )
		{
			ThumbnailParticleSystemPostProcess = LoadObject<UPostProcessChain>(NULL,*ThumbnailParticleSystemPostProcessName,NULL,LOAD_None,NULL);
		}
		// Load the post process chain used for material thumbnails
		if( ThumbnailMaterialPostProcess == NULL && ThumbnailMaterialPostProcessName.Len() )
		{
			ThumbnailMaterialPostProcess = LoadObject<UPostProcessChain>(NULL,*ThumbnailMaterialPostProcessName,NULL,LOAD_None,NULL);
		}
	}
	else
	{
		// Load the post process chain used for skeletal mesh thumbnails
		if( ThumbnailSkeletalMeshPostProcess == NULL && DefaultPostProcessName.Len() )
		{
			ThumbnailSkeletalMeshPostProcess = LoadObject<UPostProcessChain>(NULL,*DefaultPostProcessName,NULL,LOAD_None,NULL);
		}
		// Load the post process chain used for particle system thumbnails
		if( ThumbnailParticleSystemPostProcess == NULL && DefaultPostProcessName.Len() )
		{
			ThumbnailParticleSystemPostProcess = LoadObject<UPostProcessChain>(NULL,*DefaultPostProcessName,NULL,LOAD_None,NULL);
		}
		// Load the post process chain used for material thumbnails
		if( ThumbnailMaterialPostProcess == NULL && DefaultPostProcessName.Len() )
		{
			ThumbnailMaterialPostProcess = LoadObject<UPostProcessChain>(NULL,*DefaultPostProcessName,NULL,LOAD_None,NULL);
		}
	}

	// Load the default UI post process chain
	if( DefaultUIScenePostProcess == NULL && DefaultUIScenePostProcessName.Len() )
	{
		DefaultUIScenePostProcess = LoadObject<UPostProcessChain>(NULL,*DefaultUIScenePostProcessName,NULL,LOAD_None,NULL);
	}

	// set the font object pointers
	if( TinyFont == NULL && TinyFontName.Len() )
	{
		TinyFont = LoadObject<UFont>(NULL,*TinyFontName,NULL,LOAD_None,NULL);
	}
	if( SmallFont == NULL && SmallFontName.Len() )
	{
		SmallFont = LoadObject<UFont>(NULL,*SmallFontName,NULL,LOAD_None,NULL);
	}
	if( MediumFont == NULL && MediumFontName.Len() )
	{
		MediumFont = LoadObject<UFont>(NULL,*MediumFontName,NULL,LOAD_None,NULL);
	}
	if( LargeFont == NULL && LargeFontName.Len() )
	{
		LargeFont = LoadObject<UFont>(NULL,*LargeFontName,NULL,LOAD_None,NULL);
	}

	// Additional fonts.
	AdditionalFonts.Empty( AdditionalFontNames.Num() );
	for ( INT FontIndex = 0 ; FontIndex < AdditionalFontNames.Num() ; ++FontIndex )
	{
		const FString& FontName = AdditionalFontNames(FontIndex);
		UFont* NewFont = NULL;
		if( FontName.Len() )
		{
			NewFont = LoadObject<UFont>(NULL,*FontName,NULL,LOAD_None,NULL);
		}
		AdditionalFonts.AddItem( NewFont );
	}
}

//
// Exit the engine.
//
void UEngine::FinishDestroy()
{
	// Remove from root.
	RemoveFromRoot();

	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Clean up debug tool.
		delete GDebugToolExec;
		GDebugToolExec		= NULL;

#if USING_REMOTECONTROL
		// Clean up RemoteControl.
		delete RemoteControlExec;
		RemoteControlExec		= NULL;
#endif

		// Shut down all subsystems.
		Client				= NULL;
		GEngine				= NULL;
		FURL::StaticExit();
		// GEngineMem.Exit(); // Causes crash at exit - no solid repro and it's harmless to not free up GEngineMem at exit

		delete GStatChart;
		GStatChart			= NULL;
	}
	Super::FinishDestroy();
}

//
// Progress indicator.
//
void UEngine::SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds )
{
}

void UEngine::CleanupGameViewport()
{
	// Clean up the viewports that have been closed.
	for(FPlayerIterator It(this);It;++It)
	{
		if(It->ViewportClient && !It->ViewportClient->Viewport)
		{
			It->ViewportClient = NULL;
			It.RemoveCurrent();
		}
	}

	if ( GameViewport != NULL && GameViewport->Viewport == NULL )
	{
		GameViewport->DetachViewportClient();
		GameViewport = NULL;
	}
}

FViewport* UEngine::GetAViewport()
{
	if(GameViewport)
	{
		return GameViewport->Viewport;
	}

	return NULL;
}

/**
 * Returns a pointer to the current world.
 */
AWorldInfo* UEngine::GetCurrentWorldInfo()
{
	return GWorld->GetWorldInfo();
}

/**
 * Returns the engine's default tiny font
 */
UFont* UEngine::GetTinyFont()
{
	return GEngine->TinyFont;
}

/**
 * Returns the engine's default small font
 */
UFont* UEngine::GetSmallFont()
{
	return GEngine->SmallFont;
}

/**
 * Returns the engine's default medium font
 */
UFont* UEngine::GetMediumFont()
{
	return GEngine->MediumFont;
}

/**
 * Returns the engine's default large font
 */
UFont* UEngine::GetLargeFont()
{
	return GEngine->LargeFont;
}

/**
 * Returns the specified additional font.
 *
 * @param	AdditionalFontIndex		Index into the AddtionalFonts array.
 */
UFont* UEngine::GetAdditionalFont(INT AdditionalFontIndex)
{
	return GEngine->AdditionalFonts(AdditionalFontIndex);
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

//
// This always going to be the last exec handler in the chain. It
// handles passing the command to all other global handlers.
//
UBOOL UEngine::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// See if any other subsystems claim the command.
	if( GSys				&&	GSys->Exec			(Cmd,Ar) ) return TRUE;
	if(	UObject::StaticExec							(Cmd,Ar) ) return TRUE;
	if( Client				&&	Client->Exec		(Cmd,Ar) ) return TRUE;
	if( GDebugToolExec		&&	GDebugToolExec->Exec(Cmd,Ar) ) return TRUE;
	if( GStatChart			&&	GStatChart->Exec	(Cmd,Ar) ) return TRUE;
	if( GMalloc				&&	GMalloc->Exec		(Cmd,Ar) ) return TRUE;
	if(	GObjectPropagator->Exec						(Cmd,Ar) ) return TRUE;
	if( GSystemSettings.Exec						(Cmd,Ar) ) return TRUE;
#if USING_REMOTECONTROL
	if( RemoteControlExec	&&	RemoteControlExec->Exec	(Cmd,Ar) ) return TRUE;
#endif

	// Handle engine command line.
	if( ParseCommand(&Cmd,TEXT("SHOWLOG")) )
	{
		// Toggle display of console log window.
		if( GLogConsole )
		{
			GLogConsole->Show( !GLogConsole->IsShown() );
		}
		return 1;
	}
	else if( ParseCommand(&Cmd, TEXT("GAMEVER")) ||  ParseCommand(&Cmd, TEXT("GAMEVERSION")))
	{
		Ar.Logf( TEXT("GEngineVersion:  %d  GBuiltFromChangeList:  %d"), GEngineVersion, GBuiltFromChangeList );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("CRACKURL")) )
	{
		FURL URL(NULL,Cmd,TRAVEL_Absolute);
		if( URL.Valid )
		{
			Ar.Logf( TEXT("     Protocol: %s"), *URL.Protocol );
			Ar.Logf( TEXT("         Host: %s"), *URL.Host );
			Ar.Logf( TEXT("         Port: %i"), URL.Port );
			Ar.Logf( TEXT("          Map: %s"), *URL.Map );
			Ar.Logf( TEXT("   NumOptions: %i"), URL.Op.Num() );
			for( INT i=0; i<URL.Op.Num(); i++ )
				Ar.Logf( TEXT("     Option %i: %s"), i, *URL.Op(i) );
			Ar.Logf( TEXT("       Portal: %s"), *URL.Portal );
			Ar.Logf( TEXT("       String: '%s'"), *URL.String() );
		}
		else Ar.Logf( TEXT("BAD URL") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("DEFER")) )
	{
		new(DeferredCommands)FString(Cmd);
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("ToggleRenderingThread")) )
	{
		if(GIsThreadedRendering)
		{
			StopRenderingThread();
			GUseThreadedRendering = FALSE;
		}
		else
		{
			GUseThreadedRendering = TRUE;
			StartRenderingThread();
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("RecompileShaders")) )
	{
		class FTestTimer
		{
		public:
			FTestTimer(const TCHAR* InInfoStr=TEXT("Test"))
				: InfoStr(InInfoStr),
				bAlreadyStopped(FALSE)
			{
                StartTime = appSeconds();
			}

			FTestTimer(FString InInfoStr)
				: InfoStr(InInfoStr),
				bAlreadyStopped(FALSE)
			{
				StartTime = appSeconds();
			}

			void Stop(UBOOL DisplayLog = TRUE)
			{
				if (!bAlreadyStopped)
				{
					bAlreadyStopped = TRUE;
					EndTime = appSeconds();
					TimeElapsed = EndTime-StartTime;
					if (DisplayLog)
					{
						warnf(TEXT("		[%s] took [%.4f] s"),*InfoStr,TimeElapsed);
					}
				}
			}

			~FTestTimer()
			{
				Stop(TRUE);
			}

			DOUBLE StartTime,EndTime;
			DOUBLE TimeElapsed;
			FString InfoStr;
			UBOOL bAlreadyStopped;

		};

		FString FlagStr(ParseToken(Cmd, 0));
		if( FlagStr.Len() > 0 )
		{
			FlushShaderFileCache();
			FlushRenderingCommands();

			if( appStricmp(*FlagStr,TEXT("Changed"))==0)
			{
				TArray<FShaderType*> OutdatedShaderTypes;
				TArray<FVertexFactoryType*> OutdatedVFTypes;
				
				{
					FTestTimer SearchTimer(TEXT("Searching for changed files"));
					FShaderType::GetOutdatedTypes(OutdatedShaderTypes);
					FVertexFactoryType::GetOutdatedTypes(OutdatedVFTypes);
				}

				if (OutdatedShaderTypes.Num() > 0 || OutdatedVFTypes.Num() > 0)
				{
					FTestTimer TestTimer(TEXT("RecompileShaders Changed"));
					UMaterial::UpdateMaterialShaders(OutdatedShaderTypes, OutdatedVFTypes);
					RecompileGlobalShaders(OutdatedShaderTypes);
				}
				else
				{
					warnf(TEXT("No Shader changes found."));
				}
			}
			else if( appStricmp(*FlagStr,TEXT("Shadow"))==0)
			{
				FTestTimer TestTimer(TEXT("RecompileShaders Shadow"));
				RecompileGlobalShaderGroup(SRG_GLOBAL_MISC_SHADOW);
			}
			else if( appStricmp(*FlagStr,TEXT("BPCF"))==0)
			{
				FTestTimer TestTimer(TEXT("RecompileShaders BPCF"));
				RecompileGlobalShaderGroup(SRG_GLOBAL_BPCF_SHADOW_LOW);
			}
			else if( appStricmp(*FlagStr,TEXT("GlobalMisc"))==0)
			{
				FTestTimer TestTimer(TEXT("RecompileShaders GlobalMisc"));
				RecompileGlobalShaderGroup(SRG_GLOBAL_MISC);
			}
			else if( appStricmp(*FlagStr,TEXT("Global"))==0)
			{
				FTestTimer TestTimer(TEXT("RecompileShaders Global"));
				RecompileGlobalShaders();
			}
			else if( appStricmp(*FlagStr,TEXT("VF"))==0)
			{
				FString RequestedVertexFactoryName(ParseToken(Cmd, 0));

				FVertexFactoryType* FoundType = FVertexFactoryType::GetVFByName(RequestedVertexFactoryName);
				
				if (!FoundType)
				{
					warnf( TEXT("Couldn't find Vertex Factory %s! \nExisting Vertex Factories:"), *RequestedVertexFactoryName);
					for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
					{
						const TCHAR* VertexFactoryTypeName = It->GetName();
						warnf( TEXT("%s"), VertexFactoryTypeName);
					}
					return 1;
				}

				FTestTimer TestTimer(TEXT("RecompileShaders VertexFactory"));
				TArray<FShaderType*> OutdatedShaderTypes;
				TArray<FVertexFactoryType*> OutdatedVFTypes;
				OutdatedVFTypes.Push(FoundType);

				UMaterial::UpdateMaterialShaders(OutdatedShaderTypes, OutdatedVFTypes);
			}
			else if( appStricmp(*FlagStr,TEXT("Material"))==0)
			{
				FString RequestedMaterialName(ParseToken(Cmd, 0));
				FTestTimer TestTimer(FString::Printf(TEXT("Recompile Material %s"), *RequestedMaterialName));
				UBOOL bMaterialFound = FALSE;
				for( TObjectIterator<UMaterial> It; It; ++It )
				{
					UMaterial* Material = *It;
					if( Material && Material->GetName() == RequestedMaterialName)
					{
						bMaterialFound = TRUE;
						// <Pre/Post>EditChange will force a re-creation of the resource,
						// in turn recompiling the shader.
						Material->PreEditChange(NULL);
						Material->PostEditChange(NULL);
						break;
					}
				}

				if (!bMaterialFound)
				{
					TestTimer.Stop(FALSE);
					warnf(TEXT("Couldn't find Material %s!"), *RequestedMaterialName);
				}
			}
			else if( appStricmp(*FlagStr,TEXT("MaterialShaderType"))==0)
			{
				FString RequestedShaderTypeName(ParseToken(Cmd, 0));

				FShaderType* FoundType = FMeshMaterialShaderType::GetTypeByName(RequestedShaderTypeName);

				if (!FoundType)
				{
					FoundType = FMaterialShaderType::GetTypeByName(RequestedShaderTypeName);
				}

				if (!FoundType)
				{
					warnf( TEXT("Couldn't find Shader Type %s! \nExisting Shader Types:"), *RequestedShaderTypeName);
					for(TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next())
					{
						FMaterialShaderType* MaterialShaderType = It->GetMaterialShaderType();
						FMeshMaterialShaderType* MeshMaterialShaderType = It->GetMeshMaterialShaderType();
						if (MaterialShaderType || MeshMaterialShaderType)
			{
							const TCHAR* ShaderTypeName = It->GetName();
							warnf( TEXT("%s"), ShaderTypeName);
						}
					}
					return 1;
			}

				FTestTimer TestTimer(TEXT("RecompileShaders ShaderType"));

				TArray<FShaderType*> OutdatedShaderTypes;
				OutdatedShaderTypes.Push(FoundType);
				TArray<FVertexFactoryType*> OutdatedVFTypes;

				UMaterial::UpdateMaterialShaders(OutdatedShaderTypes, OutdatedVFTypes);
		} 
			else if( appStricmp(*FlagStr,TEXT("All"))==0)
		{
			FTestTimer TestTimer(TEXT("RecompileShaders"));
			RecompileGlobalShaders();
			for( TObjectIterator<UMaterial> It; It; ++It )
			{
				UMaterial* Material = *It;
				if( Material )
				{
					debugf(TEXT("recompiling [%s]"),*Material->GetFullName());

					// <Pre/Post>EditChange will force a re-creation of the resource,
					// in turn recompiling the shader.
					Material->PreEditChange(NULL);
					Material->PostEditChange(NULL);
				}
			}
		}
		
		return 1;
	}

		warnf( TEXT("Invalid parameter. Options are: \n'Changed', 'Shadow', 'BPCF', 'GlobalMisc', 'Global', \n'VF [name]', 'Material [name]', 'MaterialShaderType [name]', 'All'"));
		
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("RecompileGlobalShaders")) )
	{
		RecompileGlobalShaders();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("DUMPSHADERSTATS")) )
	{
		DumpShaderStats();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("DISTFACTORSTATS")) )
	{
		DumpDistanceFactorChart();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("rebuilddistro")) )
	{
		for (TObjectIterator<UDistributionFloat> It; It; ++It)
		{
			It->bIsDirty = TRUE;
		}
		for (TObjectIterator<UDistributionVector> It; It; ++It)
		{
			It->bIsDirty = TRUE;
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEVSM")) )
	{
		FString FlagStr(ParseToken(Cmd, 0));
		if( FlagStr.Len() > 0 )
		{
            if( appStricmp(*FlagStr,TEXT("1"))==0 || appStricmp(*FlagStr,TEXT("true"))==0 )
			{
				bEnableVSMShadows = TRUE;
			}
			else if( appStricmp(*FlagStr,TEXT("0"))==0 || appStricmp(*FlagStr,TEXT("false"))==0 )
			{
				bEnableVSMShadows = FALSE;
			}
		}
		else
		{
			bEnableVSMShadows = !bEnableVSMShadows;
		}
		warnf( TEXT("VSM Shadows %s"),bEnableVSMShadows ? TEXT("ENABLED") : TEXT("DISABLED") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEBPCF")) )
	{
		FString FlagStr(ParseToken(Cmd, 0));
		if( FlagStr.Len() > 0 )
		{
			if( appStricmp(*FlagStr,TEXT("1"))==0 || appStricmp(*FlagStr,TEXT("true"))==0 )
			{
				bEnableBranchingPCFShadows = TRUE;
			}
			else if( appStricmp(*FlagStr,TEXT("0"))==0 || appStricmp(*FlagStr,TEXT("false"))==0 )
			{
				bEnableBranchingPCFShadows = FALSE;
			}
		}
		else
		{
			bEnableBranchingPCFShadows = !bEnableBranchingPCFShadows;
		}
		warnf( TEXT("Branching PCF Shadows %s"),bEnableBranchingPCFShadows ? TEXT("ENABLED") : TEXT("DISABLED") );
		return 1;
	} 
	else if( ParseCommand(&Cmd,TEXT("SHADOWRADIUS")) )
	{
		FLOAT NewRadius = appAtof(Cmd);
		if(NewRadius > 0.0f)
		{
			ShadowFilterRadius = NewRadius;
			warnf( TEXT("New Shadow Filter Radius %f"), NewRadius);
		} 
		else
		{
			warnf( TEXT("Format is 'shadowradius [float]'"));
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SHADOWDEPTHBIAS")) )
	{
		FLOAT NewDepthBias = appAtof(Cmd);
		if(NewDepthBias > 0.0f)
		{
			DepthBias = NewDepthBias;
			warnf( TEXT("New Depth Bias %f"), DepthBias);
		} 
		else
		{
			warnf( TEXT("Format is 'shadowdepthbias [float]'"));
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("RESETFPSCHART")) )
	{
		ResetFPSChart();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TEXTUREDENSITY")) )
	{
		FString MinDensity(ParseToken(Cmd, 0));
		FString IdealDensity(ParseToken(Cmd, 0));
		FString MaxDensity(ParseToken(Cmd, 0));
		GEngine->MinTextureDensity   = ( MinDensity.Len()   > 0 ) ? appAtof(*MinDensity)   : GEngine->MinTextureDensity;
		GEngine->IdealTextureDensity = ( IdealDensity.Len() > 0 ) ? appAtof(*IdealDensity) : GEngine->IdealTextureDensity;
		GEngine->MaxTextureDensity   = ( MaxDensity.Len()   > 0 ) ? appAtof(*MaxDensity)   : GEngine->MaxTextureDensity;
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SHADERCOMPLEXITY")) )
	{
		FString FlagStr(ParseToken(Cmd, 0));
		if( FlagStr.Len() > 0 )
		{
			if( appStricmp(*FlagStr,TEXT("TOGGLEADDITIVE"))==0)
			{
				GEngine->bUseAdditiveComplexity = !GEngine->bUseAdditiveComplexity;
			}
			else if( appStricmp(*FlagStr,TEXT("TOGGLEPIXEL"))==0)
			{
				GEngine->bUsePixelShaderComplexity = !GEngine->bUsePixelShaderComplexity;
			}
			else if( appStricmp(*FlagStr,TEXT("MAX"))==0)
			{
				FLOAT NewMax = appAtof(Cmd);
				if (NewMax > 0.0f)
				{
					if (GEngine->bUsePixelShaderComplexity)
					{
						if (GEngine->bUseAdditiveComplexity)
						{
							GEngine->MaxPixelShaderAdditiveComplexityCount = NewMax;
						}
						else
						{
							GEngine->MaxPixelShaderOpaqueComplexityCount = NewMax;
						}
					}
					else
					{
						GEngine->MaxVertexShaderComplexityCount = NewMax;
					}
				}
			}
			else
			{
				Ar.Logf( TEXT("Format is 'shadercomplexity [toggleadditive] [togglepixel] [max $int]"));
				return TRUE;
			}

			FLOAT CurrentMax = 0.0f;
			if (GEngine->bUsePixelShaderComplexity)
			{
				CurrentMax = GEngine->bUseAdditiveComplexity ? GEngine->MaxPixelShaderAdditiveComplexityCount : GEngine->MaxPixelShaderOpaqueComplexityCount;
			}
			else
			{
				CurrentMax = GEngine->MaxVertexShaderComplexityCount;
			}

			Ar.Logf( TEXT("New ShaderComplexity Settings: Pixel Complexity = %u, Additive = %u, Max = %f"), GEngine->bUsePixelShaderComplexity, GEngine->bUseAdditiveComplexity, CurrentMax);
		} 
		else
		{
			Ar.Logf( TEXT("Format is 'shadercomplexity [toggleadditive] [togglepixel] [max $int]"));
		}
		return TRUE; 
	}
	else if( ParseCommand(&Cmd, TEXT("TOGGLECOLLISIONOVERLAY")) )
	{
		GEngine->bRenderTerrainCollisionAsOverlay = !(GEngine->bRenderTerrainCollisionAsOverlay);
		debugf(TEXT("Render terrain collision as overlay = %s"),
			GEngine->bRenderTerrainCollisionAsOverlay ? TEXT("TRUE") : TEXT("FALSE"));
		return TRUE;
	}
	else if( ParseCommand(&Cmd, TEXT("LISTMISSINGPHYSICALMATERIALS")) )
	{
		// Gather all material (instances) without a physical material association.
		TArray<FString> MaterialNames;
		for(  TObjectIterator<UMaterialInterface> It; It; ++It )
		{
			UMaterialInterface* MaterialInterface = *It;
			if( MaterialInterface->GetPhysicalMaterial() == NULL )
			{
				new(MaterialNames)FString(MaterialInterface->GetFullName());
			}
		}
		if( MaterialNames.Num() )
		{
			// Sort the list lexigraphically.
			Sort<USE_COMPARE_CONSTREF(FString,UnEngine)>( MaterialNames.GetTypedData(), MaterialNames.Num() );
			// Log the names.
			debugf(TEXT("Materials with no associated physical material:"));
			for( INT i=0; i<MaterialNames.Num(); i++ )
			{
				debugf(TEXT("%s"),*MaterialNames(i));
			}
		}
		else
		{
			debugf(TEXT("All materials have physical materials associated."));
		}
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("FULLMOTIONBLUR")))
	{
		extern INT GMotionBlurFullMotionBlur;
		FString Parameter(ParseToken(Cmd, 0));
		GMotionBlurFullMotionBlur = (Parameter.Len() > 0) ? Clamp(appAtoi(*Parameter),-1,1) : -1;
		warnf( TEXT("Motion Blur FullMotionBlur is now set to: %s"),
			(GMotionBlurFullMotionBlur < 0 ? TEXT("DEFAULT") : (GMotionBlurFullMotionBlur > 0 ? TEXT("TRUE") : TEXT("FALSE"))) );
		return TRUE;
	}
#if !FINAL_RELEASE
	else if( ParseCommand(&Cmd,TEXT("FREEZERENDERING")) )
	{
		ProcessToggleFreezeCommand();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("FREEZESTREAMING")) )
	{
		ProcessToggleFreezeStreamingCommand();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("FREEZEALL")) )
	{
		ProcessToggleFreezeCommand();
		ProcessToggleFreezeStreamingCommand();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SHOWMATERIALDRAWEVENTS")) )
	{
		GShowMaterialDrawEvents = !GShowMaterialDrawEvents;
		warnf( TEXT("Show material names in SCOPED_DRAW_EVENT: %s"), GShowMaterialDrawEvents ? TEXT("TRUE") : TEXT("FALSE") );
		return TRUE;
	}
#endif
#if TRACK_FILEIO_STATS
	else if( ParseCommand(&Cmd,TEXT("DUMPFILEIOSTATS")) )
	{
		GetFileIOStats()->DumpStats();
		return TRUE;
	}
#endif
	else if( ParseCommand(&Cmd, TEXT("MOVIETEST")) )
	{
		if( GFullScreenMovie )
		{
			FString TestMovieName( ParseToken( Cmd, 0 ) );
			if( TestMovieName.Len() == 0 )
			{
				TestMovieName = TEXT("AttractMode");
			}
			// Play movie and block on playback.
			GFullScreenMovie->GameThreadPlayMovie(MM_PlayOnceFromStream, *TestMovieName);
			GFullScreenMovie->GameThreadWaitForMovie();
			GFullScreenMovie->GameThreadStopMovie();
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd, TEXT("AVAILABLETEXMEM")) )
	{
		warnf(TEXT("Available texture memory as reported by the RHI: %dMB"), RHIGetAvailableTextureMemory());
		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("DUMPAVAILABLERESOLUTIONS")))
	{
		debugf(TEXT("DumpAvailableResolutions"));
		
		TArray<ScreenResolutionArray> ResolutionLists;
		if (RHIGetAvailableResolutions(ResolutionLists, FALSE))
		{
			for (INT AdapterIndex = 0; AdapterIndex < ResolutionLists.Num(); AdapterIndex++)
			{
				ScreenResolutionArray& ResArray = ResolutionLists(AdapterIndex);
				for (INT ModeIndex = 0; ModeIndex < ResArray.Num(); ModeIndex++)
				{
					FScreenResolutionRHI& ScreenRes = ResArray(ModeIndex);
					debugf(TEXT("Adapter %2d - %4d x %4d @ %d"), 
						AdapterIndex, ScreenRes.Width, ScreenRes.Height, ScreenRes.RefreshRate);
				}
			}
		}
		else
		{
			debugf(TEXT("Failed to get available resolutions!"));
		}
		return TRUE;
	}
	else 
	{
		return 0;
	}
}

INT UEngine::ChallengeResponse( INT Challenge )
{
	return 0;
}

/**
 * Computes a color to use for property coloration for the given object.
 *
 * @param	Object		The object for which to compute a property color.
 * @param	OutColor	[out] The returned color.
 * @return				TRUE if a color was successfully set on OutColor, FALSE otherwise.
 */
UBOOL UEngine::GetPropertyColorationColor(UObject* Object, FColor& OutColor)
{
	return FALSE;
}


/*-----------------------------------------------------------------------------
	UServerCommandlet.
-----------------------------------------------------------------------------*/

INT UServerCommandlet::Main( const FString& Params )
{
	GIsRunning = 1;
	GIsRequestingExit = FALSE;

#if !CONSOLE && !__GNUC__// Windows only
	/**
	 * Used by the .com wrapper to notify that the Ctrl-C handler was triggered.
	 * This shared event is checked each tick so that the log file can be
	 * cleanly flushed
	 */
	FEvent* ComWrapperShutdownEvent = GSynchronizeFactory->CreateSynchEvent(TRUE,TEXT("ComWrapperShutdown"));
#endif

	// Main loop.
	while( GIsRunning && !GIsRequestingExit )
	{
		// Update GCurrentTime/ GDeltaTime while taking into account max tick rate.
 		extern void appUpdateTimaAndHandleMaxTickRate();
		appUpdateTimaAndHandleMaxTickRate();

		// Tick the engine.
		GEngine->Tick( GDeltaTime );
	
#if STATS
		// Write all stats for this frame out
		GStatManager.AdvanceFrame();
#endif

#if !CONSOLE && !__GNUC__
		// See if the Ctrl-C handler of the .com wrapper asked us to exit
		if (ComWrapperShutdownEvent->Wait(0) == TRUE)
		{
			// inform GameInfo that we're exiting
			if (GWorld != NULL && GWorld->GetWorldInfo() != NULL && GWorld->GetWorldInfo()->Game != NULL)
			{
				GWorld->GetWorldInfo()->Game->eventGameEnding();
			}
			// It wants us to exit
			GIsRequestingExit = TRUE;
		}
#endif
	}

#if !CONSOLE && !__GNUC__
	// Create an event that is shared across apps and is manual reset
	GSynchronizeFactory->Destroy(ComWrapperShutdownEvent);
#endif

	GIsRunning = 0;
	return 0;
}

IMPLEMENT_CLASS(UServerCommandlet)


INT SmokeTest_CheckNativeClassSizes( const TCHAR* Parms );
INT SmokeTest_RunServer( const TCHAR* Parms );

/**
 * smoke test to check CheckNativeClassSizes  this doesn't work just yet
 *
 *
 * Note:  probably need to move the init() call to below and then set our IsFoo=1 for the type of 
 * test we want
 *
 */
INT USmokeTestCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	INT Retval = 0;

	// put various smoke test testing code in here before moving off to their
	// own commandlet

	if( ParseParam(appCmdLine(),TEXT("SERVER")) == TRUE )
	{
		Retval = SmokeTest_RunServer( Parms );
	}
	else if( ParseParam(appCmdLine(),TEXT("CHECK_NATIVE_CLASS_SIZES")) == TRUE )
	{
		Retval = SmokeTest_CheckNativeClassSizes( Parms );
	}
	// other tests we want are to launch the editor and then quit
	// launch the game and then quit
	else
	{
		// exit with no error
		Retval = 0;
	}

	GIsRequestingExit = TRUE; // so CTRL-C will exit immediately


	return Retval;
}
IMPLEMENT_CLASS(USmokeTestCommandlet)

/**
 * Run the server for one tick and then quit
 */
INT SmokeTest_RunServer( const TCHAR* Parms )
{
	INT Retval = 0;

	GIsRunning = 1;

	// Main loop.
	while( GIsRunning && !GIsRequestingExit )
	{
		// Update GCurrentTime/ GDeltaTime while taking into account max tick rate.
 		extern void appUpdateTimaAndHandleMaxTickRate();
		appUpdateTimaAndHandleMaxTickRate();
		
		// Tick the engine.
		GEngine->Tick( GDeltaTime );

		GIsRequestingExit = TRUE; // so CTRL-C will exit immediately
	}

	GIsRunning = 0;

	return Retval;
}


/**
 * test to check CheckNativeClassSizes 
 */
INT SmokeTest_CheckNativeClassSizes( const TCHAR* Parms )
{
	GIsRequestingExit = TRUE; // so CTRL-C will exit immediately

	LoadAllNativeScriptPackages( FALSE ); // after this we can verify them

	// Verify native class sizes and member offsets.
	CheckNativeClassSizes();

	return 0;
}



/*-----------------------------------------------------------------------------
	Actor iterator implementations.
-----------------------------------------------------------------------------*/

/**
 * Returns the actor count.
 *
 * @param total actor count
 */
INT FActorIteratorBase::GetProgressDenominator()
{
	return GetActorCount();
}
/**
 * Returns the actor count.
 *
 * @param total actor count
 */
INT FActorIteratorBase::GetActorCount()
{
	INT TotalActorCount = 0;
	for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = GWorld->Levels(LevelIndex);
		TotalActorCount += Level->Actors.Num();
	}
	return TotalActorCount;
}
/**
 * Returns the dynamic actor count.
 *
 * @param total dynamic actor count
 */
INT FActorIteratorBase::GetDynamicActorCount()
{
	INT TotalActorCount = 0;
	for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = GWorld->Levels(LevelIndex);
		TotalActorCount += Level->Actors.Num() - Level->iFirstDynamicActor;
	}
	return TotalActorCount;
}
/**
 * Returns the net relevant actor count.
 *
 * @param total net relevant actor count
 */
INT FActorIteratorBase::GetNetRelevantActorCount()
{
	INT TotalActorCount = 0;
	for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = GWorld->Levels(LevelIndex);
		TotalActorCount += Level->Actors.Num() - Level->iFirstNetRelevantActor;
	}
	return TotalActorCount;
}



