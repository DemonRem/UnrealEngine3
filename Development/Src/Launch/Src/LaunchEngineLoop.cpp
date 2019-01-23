 /*=============================================================================
	LaunchEngineLoop.cpp: Main engine loop.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "LaunchPrivate.h"

// Static linking support forward declaration.
void InitializeRegistrantsAndRegisterNames();

//	AutoInitializeRegistrants* declarations.
	extern void AutoInitializeRegistrantsCore( INT& Lookup );
	extern void AutoInitializeRegistrantsEngine( INT& Lookup );
	extern void AutoInitializeRegistrantsGameFramework( INT& Lookup );
	extern void AutoInitializeRegistrantsUnrealScriptTest( INT& Lookup );
	extern void AutoInitializeRegistrantsIpDrv( INT& Lookup );
#if defined(XBOX)
	extern void AutoInitializeRegistrantsXeAudio( INT& Lookup );
	extern void AutoInitializeRegistrantsXeDrv( INT& Lookup );
	extern void AutoInitializeRegistrantsOnlineSubsystemLive( INT& Lookup );
#elif PS3
	extern void AutoInitializeRegistrantsPS3Drv( INT& Lookup );
#elif PLATFORM_UNIX
	extern void AutoInitializeRegistrantsLinuxDrv( INT& Lookup );
#else
	extern void AutoInitializeRegistrantsEditor( INT& Lookup );
	extern void AutoInitializeRegistrantsUnrealEd( INT& Lookup );
	extern void AutoInitializeRegistrantsALAudio( INT& Lookup );
	extern void AutoInitializeRegistrantsWinDrv( INT& Lookup );
#endif
#if   GAMENAME == WARGAME
	extern void AutoInitializeRegistrantsWarfareGame( INT& Lookup );
#if !CONSOLE
	extern void AutoInitializeRegistrantsWarfareEditor( INT& Lookup );

	extern void AutoInitializeRegistrantsOnlineSubsystemLive( INT& Lookup );
	extern void AutoInitializeRegistrantsOnlineSubsystemPC( INT& Lookup );

#elif PS3
	extern void AutoInitializeRegistrantsOnlineSubsystemPS3( INT& Lookup );
#endif
#if defined(XBOX)
	extern void AutoInitializeRegistrantsVinceOnlineSubsystemLive( INT& Lookup );
#endif
#elif GAMENAME == GEARGAME
	extern void AutoInitializeRegistrantsGearGame( INT& Lookup );
#if !CONSOLE
	extern void AutoInitializeRegistrantsGearEditor( INT& Lookup );
#endif
#elif GAMENAME == UTGAME
	extern void AutoInitializeRegistrantsUTGame( INT& Lookup );
#if !CONSOLE
	extern void AutoInitializeRegistrantsUTEditor( INT& Lookup );
#if WITH_GAMESPY
	extern void AutoInitializeRegistrantsOnlineSubsystemGameSpy( INT& Lookup );
#endif
#elif PS3
#if WITH_GAMESPY
	extern void AutoInitializeRegistrantsOnlineSubsystemGameSpy( INT& Lookup );
#endif
#endif

#elif GAMENAME == EXAMPLEGAME
	extern void AutoInitializeRegistrantsExampleGame( INT& Lookup );
#if !CONSOLE
	extern void AutoInitializeRegistrantsExampleEditor( INT& Lookup );
	extern void AutoInitializeRegistrantsOnlineSubsystemPC( INT& Lookup );
#endif
#else
	#error Hook up your game name here
#endif

//	AutoGenerateNames* declarations.
	extern void AutoGenerateNamesCore();
	extern void AutoGenerateNamesEngine();
	extern void AutoGenerateNamesGameFramework();
	extern void AutoGenerateNamesUnrealScriptTest();
#if !CONSOLE && !PLATFORM_UNIX
	extern void AutoGenerateNamesEditor();
	extern void AutoGenerateNamesUnrealEd();
#endif
	extern void AutoGenerateNamesIpDrv();
#if XBOX
	extern void AutoGenerateNamesOnlineSubsystemLive();
#endif
#if   GAMENAME == WARGAME
	extern void AutoGenerateNamesWarfareGame();
#if !CONSOLE
	extern void AutoGenerateNamesWarfareEditor();

	extern void AutoGenerateNamesOnlineSubsystemLive();
	extern void AutoGenerateNamesOnlineSubsystemPC();

#elif PS3
	extern void AutoGenerateNamesOnlineSubsystemPS3();
#endif
#if XBOX
	extern void AutoGenerateNamesVinceOnlineSubsystemLive();
#endif
#elif GAMENAME == GEARGAME
	extern void AutoGenerateNamesGearGame();
#if !CONSOLE
	extern void AutoGenerateNamesGearEditor();
#endif
#elif GAMENAME == UTGAME
	extern void AutoGenerateNamesUTGame();
#if !CONSOLE
	extern void AutoGenerateNamesUTEditor();
#if WITH_GAMESPY
	extern void AutoGenerateNamesOnlineSubsystemGameSpy();
#endif
#elif PS3
#if WITH_GAMESPY
	extern void AutoGenerateNamesOnlineSubsystemGameSpy();
#endif
#endif

#elif GAMENAME == EXAMPLEGAME
	extern void AutoGenerateNamesExampleGame();
#if !CONSOLE
	extern void AutoGenerateNamesExampleEditor();
	extern void AutoGenerateNamesOnlineSubsystemPC();
#endif
#else
	#error Hook up your game name here
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

#define SPAWN_CHILD_PROCESS_TO_COMPILE 0

// General.
#if !CONSOLE
extern "C" {HINSTANCE hInstance;}
#endif

#if PS3
// we use a different GPackage here so that PS3 will use PS3Launch.log to not conflict with PC's
// Launch.log when using -writetohost
extern "C" {TCHAR GPackage[64]=TEXT("PS3Launch");}
#else
extern "C" {TCHAR GPackage[64]=TEXT("Launch");}
#endif

/** Critical section used by MallocThreadSafeProxy for synchronization										*/

#if XBOX
/** Remote debug command																					*/
static CHAR							RemoteDebugCommand[1024];		
/** Critical section for accessing remote debug command														*/
static FCriticalSection				RemoteDebugCriticalSection;	
#endif

/** Helper function called on first allocation to create and initialize GMalloc */
void GCreateMalloc()
{
#if PS3
	// this must be called before new (which calls malloc) is called
	FMallocPS3::StaticInit();
	GMalloc = new FMallocPS3();
#elif defined(XBOX)
	GMalloc = new FMallocXenon();
#elif _DEBUG
	GMalloc = new FMallocDebug();
#elif __GNUC__
	GMalloc = new FMallocAnsi();
#else
	GMalloc = new FMallocWindows();
#endif

#if TRACK_MEM_USING_STAT_SECTIONS
	GMalloc = new FMallocProxySimpleTrack( GMalloc );
#elif KEEP_ALLOCATION_BACKTRACE
    #if __WIN32__
	    GMalloc = new FMallocProfiler( GMalloc );
    #else
	    GMalloc = new FMallocDebugProxy( GMalloc );
    #endif
#elif USE_MALLOC_PROFILER
	GMalloc = new FMallocProfiler( GMalloc );
#endif

	// if the allocator is already thread safe, there is no need for the thread safe proxy
	if (!GMalloc->IsInternallyThreadSafe())
	{
		GMalloc = new FMallocThreadSafeProxy( GMalloc );
	}
}

#if defined(XBOX)
static FOutputDeviceFile					Log;
static FOutputDeviceAnsiError				Error;
static FFeedbackContextAnsi					GameWarn;
static FFileManagerXenon					FileManager;
#elif PS3
static FSynchronizeFactoryPU				SynchronizeFactory;
static FThreadFactoryPU						ThreadFactory;
static FOutputDeviceFile					Log;
static FOutputDeviceAnsiError				Error;
static FFeedbackContextAnsi					GameWarn;
static FFileManagerPS3						FileManager;
static FQueuedThreadPoolPS3					ThreadPool;
#elif PLATFORM_UNIX
static FSynchronizeFactoryLinux				SynchronizeFactory;
static FThreadFactoryLinux					ThreadFactory;
static FOutputDeviceFile					Log;
static FOutputDeviceAnsiError				Error;
static FFeedbackContextAnsi					GameWarn;
static FFileManagerLinux					FileManager;
#else
#include "FFeedbackContextEditor.h"
#include "ALAudio.h"
static FOutputDeviceFile					Log;
static FOutputDeviceWindowsError			Error;
static FFeedbackContextWindows				GameWarn;
static FFeedbackContextEditor				UnrealEdWarn;
static FFileManagerWindows					FileManager;
static FCallbackEventDeviceEditor			UnrealEdEventCallback;
static FCallbackQueryDeviceEditor			UnrealEdQueryCallback;
static FOutputDeviceConsoleWindows			LogConsole;
static FOutputDeviceConsoleWindowsInherited InheritedLogConsole(LogConsole);
static FSynchronizeFactoryWin				SynchronizeFactory;
static FThreadFactoryWin					ThreadFactory;
static FQueuedThreadPoolWin					ThreadPool;
#endif

static FCallbackEventObserver				GameEventCallback;
static FCallbackQueryDevice					GameQueryCallback;

#if !FINAL_RELEASE
// this will allow the game to receive object propagation commands from the network
FListenPropagator							ListenPropagator;
#endif

extern	TCHAR								GCmdLine[4096];
/** Whether we are using wxWindows or not */
extern	UBOOL								GUsewxWindows;

/** Thread used for async IO manager */
FRunnableThread*							AsyncIOThread;

/** 
* if TRUE then FDeferredUpdateResource::UpdateResources needs to be called 
* (should only be set on the rendering thread)
*/
UBOOL FDeferredUpdateResource::bNeedsUpdate = TRUE;

#if __WIN32__
#include "..\..\Launch\Resources\resource.h"
/** Resource ID of icon to use for Window */
#if   GAMENAME == WARGAME || GAMENAME == GEARGAME
INT			GGameIcon	= IDICON_GoW;
INT			GEditorIcon	= IDICON_Editor;
#elif GAMENAME == UTGAME
INT			GGameIcon	= IDICON_UTGame;
INT			GEditorIcon	= IDICON_Editor;
#elif GAMENAME == EXAMPLEGAME
INT			GGameIcon	= IDICON_DemoGame;
INT			GEditorIcon	= IDICON_DemoEditor;
#else
	#error Hook up your game name here
#endif

#include "../Debugger/UnDebuggerCore.h"
#endif

#if USE_BINK_CODEC && !USE_NULL_RHI
#include "../Bink/Src/FullScreenMovieBink.h"
#endif

/** global for full screen movie player */
FFullScreenMovieSupport* GFullScreenMovie = NULL;
/** Initialize the global full screen movie player */
void appInitFullScreenMoviePlayer()
{
	check( GFullScreenMovie == NULL );

	// handle disabling of movies
	if( appStrfind(GCmdLine, TEXT("nomovie")) != NULL || GIsEditor || !GIsGame )
	{
		GFullScreenMovie = FFullScreenMovieFallback::StaticInitialize();
	}
	else
	{
#if USE_BINK_CODEC && !USE_NULL_RHI
		GFullScreenMovie = FFullScreenMovieBink::StaticInitialize();
#else
		GFullScreenMovie = FFullScreenMovieFallback::StaticInitialize();
#endif
	}
	check( GFullScreenMovie != NULL );
}

// From UMakeCommandlet.cpp - See if scripts need rebuilding at runtime
extern UBOOL AreScriptPackagesOutOfDate();

#if WITH_PANORAMA || _XBOX
/**
 * Returns the title id for this game
 * Licensees need to add their own game(s) here!
 */
DWORD appGetTitleId(void)
{
#if   GAMENAME == EXAMPLEGAME
	return 0;
#elif GAMENAME == WARGAME
	return 0x4D53082B;
#elif GAMENAME == UTGAME
	return 0x4D5707DB;
#elif GAMENAME == GEARGAME
	return 0x4D53082D;
#else
	#error Hook up your game's title id here
#endif
}
#endif

#if WITH_GAMESPY
/**
 * Returns the game name to use with GameSpy
 * Licensees need to add their own game(s) here!
 */
const TCHAR* appGetGameSpyGameName(void)
{
#if GAMENAME == UTGAME
	static TCHAR GSGameName[7];
	GSGameName[0] = TEXT('u');
	GSGameName[1] = TEXT('t');
	GSGameName[2] = TEXT('3');
	GSGameName[3] = TEXT('p');
	GSGameName[4] = TEXT('c');
	GSGameName[5] = 0;
	return GSGameName;
#elif GAMENAME == WARGAME || GAMENAME == GEARGAME || GAMENAME == EXAMPLEGAME
	return NULL;
#else
	#error Hook up your game's GameSpy game name here
#endif
}

/**
 * Returns the secret key used by this game
 * Licensees need to add their own game(s) here!
 */
const TCHAR* appGetGameSpySecretKey(void)
{
#if GAMENAME == UTGAME
	static TCHAR GSSecretKey[7];
	GSSecretKey[0] = TEXT('n');
	GSSecretKey[1] = TEXT('T');
	GSSecretKey[2] = TEXT('2');
	GSSecretKey[3] = TEXT('M');
	GSSecretKey[4] = TEXT('t');
	GSSecretKey[5] = TEXT('z');
	GSSecretKey[6] = 0;
	return GSSecretKey;
#elif GAMENAME == WARGAME || GAMENAME == GEARGAME || GAMENAME == EXAMPLEGAME
	return NULL;
#else
	#error Hook up your game's secret key here
#endif
}
#endif

/**
 * The single function that sets the gamename based on the #define GAMENAME
 * Licensees need to add their own game(s) here!
 */
void appSetGameName()
{
	// Initialize game name
#if   GAMENAME == EXAMPLEGAME
	appStrcpy(GGameName, TEXT("Example"));
#elif GAMENAME == WARGAME
	appStrcpy(GGameName, TEXT("War"));
#elif GAMENAME == GEARGAME
	appStrcpy(GGameName, TEXT("Gear"));
#elif GAMENAME == UTGAME
	appStrcpy(GGameName, TEXT("UT"));
#else
	#error Hook up your game name here
#endif
}

/**
 * A single function to get the list of the script packages that are used by 
 * the current game (as specified by the GAMENAME #define)
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetGameScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages)
{
#if CONSOLE || PLATFORM_UNIX
	// consoles and unix NEVER want to load editor only packages
	bCanIncludeEditorOnlyPackages = FALSE;
#endif

#if   GAMENAME == EXAMPLEGAME
	PackageNames.AddItem(TEXT("ExampleGame"));
//	@todo: ExampleEditor is not in .u form yet
//	if (bCanIncludeEditorOnlyPackages)
//	{
//		PackageNames.AddItem(TEXT("ExampleEditor"));
//	}
#elif GAMENAME == WARGAME
	PackageNames.AddItem(TEXT("WarfareGame"));
	PackageNames.AddItem(TEXT("WarfareGameContent"));
	// we need to cook this as it has weapons which players can carry across maps
	//PackageNames.AddItem(TEXT("WarfareGameContentWeapons"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("WarfareEditor"));
	}
#elif GAMENAME == GEARGAME
	PackageNames.AddItem(TEXT("GearGame"));
	PackageNames.AddItem(TEXT("GearGameContent"));
	// we need to cook this as it has weapons which players can carry across maps
	//PackageNames.AddItem(TEXT("GearGameContentWeapons"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("GearEditor"));
	}

#elif GAMENAME == UTGAME
	PackageNames.AddItem(TEXT("UTGame"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("UTEditor"));
	}

#else
	#error Hook up your game name here
#endif
}

/**
 * A single function to get the list of the script packages containing native
 * classes that are used by the current game (as specified by the GAMENAME #define)
 * Licensees need to add their own game(s) to the definition of this function!
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetGameNativeScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages)
{
#if CONSOLE || PLATFORM_UNIX
	// consoles and unix NEVER want to load editor only packages
	bCanIncludeEditorOnlyPackages = FALSE;
#endif

#if   GAMENAME == EXAMPLEGAME
	PackageNames.AddItem(TEXT("ExampleGame"));
#if XBOX
	// only include this if we are not checking native class sizes.  Currently, the native classes which reside on specific console
	// platforms will cause native class size checks to fail even tho class sizes are hopefully correct on the target platform as the PC 
	// doesn't have access to that native class.
	if( ParseParam(appCmdLine(),TEXT("CHECK_NATIVE_CLASS_SIZES")) == FALSE )
	{
		PackageNames.AddItem(TEXT("OnlineSubsystemLive"));
	}
#elif PS3 
#else
	PackageNames.AddItem(TEXT("OnlineSubsystemPC"));
#endif
#elif GAMENAME == WARGAME
	PackageNames.AddItem(TEXT("WarfareGame"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("WarfareEditor"));
	}

#if XBOX
	// only include this if we are not checking native class sizes.  Currently, the native classes which reside on specific console
	// platforms will cause native class size checks to fail even tho class sizes are hopefully correct on the target platform as the PC 
	// doesn't have access to that native class.
	if( ParseParam(appCmdLine(),TEXT("CHECK_NATIVE_CLASS_SIZES")) == FALSE )
	{
		PackageNames.AddItem(TEXT("OnlineSubsystemLive"));
		PackageNames.AddItem(TEXT("VinceOnlineSubsystemLive"));
	}
#elif PS3
	// only include this if we are not checking native class sizes.  Currently, the native classes which reside on specific console
	// platforms will cause native class size checks to fail even tho class sizes are hopefully correct on the target platform as the PC 
	// doesn't have access to that native class.
	if( ParseParam(appCmdLine(),TEXT("CHECK_NATIVE_CLASS_SIZES")) == FALSE )
	{
		PackageNames.AddItem(TEXT("OnlineSubsystemPS3"));
	}
#else
#if WITH_PANORAMA
	PackageNames.AddItem(TEXT("OnlineSubsystemLive"));
#else
	PackageNames.AddItem(TEXT("OnlineSubsystemPC"));
#endif
#endif
#elif GAMENAME == GEARGAME
	PackageNames.AddItem(TEXT("GearGame"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("GearEditor"));
	}
#elif GAMENAME == UTGAME
	PackageNames.AddItem(TEXT("UTGame"));
	if (bCanIncludeEditorOnlyPackages)
	{
		PackageNames.AddItem(TEXT("UTEditor"));
	}
#if XBOX
	// only include this if we are not checking native class sizes.  Currently, the native classes which reside on specific console
	// platforms will cause native class size checks to fail even tho class sizes are hopefully correct on the target platform as the PC 
	// doesn't have access to that native class.
	if( ParseParam(appCmdLine(),TEXT("CHECK_NATIVE_CLASS_SIZES")) == FALSE )
	{
		PackageNames.AddItem(TEXT("OnlineSubsystemLive"));
	}
#elif PS3 
#if WITH_GAMESPY
	PackageNames.AddItem(TEXT("OnlineSubsystemGameSpy"));
#endif
#else
#if WITH_GAMESPY
	PackageNames.AddItem(TEXT("OnlineSubsystemGameSpy"));
#endif
#endif
#else
	#error Hook up your game name here
#endif
}

/**
 * A single function to get the list of the script packages that are used by the base engine.
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetEngineScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages)
{
#if CONSOLE || PLATFORM_UNIX
	// consoles and unix NEVER want to load editor only packages
	bCanIncludeEditorOnlyPackages = FALSE;
#endif
	PackageNames.AddItem(TEXT("Core"));
	PackageNames.AddItem(TEXT("Engine"));
	PackageNames.AddItem(TEXT("GameFramework"));
	if( bCanIncludeEditorOnlyPackages )
	{
		PackageNames.AddItem(TEXT("Editor"));
		PackageNames.AddItem(TEXT("UnrealEd"));
	}	
	PackageNames.AddItem(TEXT("UnrealScriptTest"));
	PackageNames.AddItem(TEXT("IpDrv"));
}

/**
 * Gets the list of all native script packages that the game knows about.
 * 
 * @param PackageNames The output list of package names
 * @param bExcludeGamePackages TRUE if the list should only contain base engine packages
 * @param bIncludeLocalizedSeekFreePackages TRUE if the list should include the _LOC_int loc files
 */
void GetNativeScriptPackageNames(TArray<FString>& PackageNames, UBOOL bExcludeGamePackages, UBOOL bIncludeLocalizedSeekFreePackages)
{
	// Assemble array of native script packages.
	appGetEngineScriptPackageNames(PackageNames, !GUseSeekFreeLoading);
	if( !bExcludeGamePackages )
	{
		appGetGameNativeScriptPackageNames(PackageNames, !GUseSeekFreeLoading);
	}

	// required for seek free loading
	if( GUseSeekFreeLoading )
	{
	bIncludeLocalizedSeekFreePackages = TRUE;
	}

	// insert any localization for Seek free packages if requested
	if (bIncludeLocalizedSeekFreePackages)
	{
		for( INT PackageIndex=0; PackageIndex<PackageNames.Num(); PackageIndex++ )
		{
			// only insert localized package if it exists
			FString LocalizedPackageName = PackageNames(PackageIndex) + LOCALIZED_SEEKFREE_SUFFIX;
			FString LocalizedFileName;
			if( !GUseSeekFreeLoading || GPackageFileCache->FindPackageFile( *LocalizedPackageName, NULL, LocalizedFileName ) )
			{
				PackageNames.InsertItem(*LocalizedPackageName, PackageIndex);
				// skip over the package that we just localized
				PackageIndex++;
			}
		}
	}
}

/**
 * Gets the list of packages that are precached at startup for seek free loading
 *
 * @param PackageNames The output list of package names
 * @param EngineConfigFilename Optional engine config filename to use to lookup the package settings
 */
void GetNonNativeStartupPackageNames(TArray<FString>& PackageNames, const TCHAR* EngineConfigFilename=NULL)
{
	// if we aren't cooking, we actually just want to use the cooked startup package as the only startup package
	if (!GIsUCC)
	{
		PackageNames.AddItem(FString(TEXT("Startup_")) + UObject::GetLanguage());
	}
	else
	{
		// look for any packages that we want to force preload at startup
		TMultiMap<FString,FString>* PackagesToPreload = GConfig->GetSectionPrivate( TEXT("Engine.StartupPackages"), 0, 1, 
			EngineConfigFilename ? EngineConfigFilename : GEngineIni );
		if (PackagesToPreload)
		{
			// go through list and add to the array
			for( TMultiMap<FString,FString>::TIterator It(*PackagesToPreload); It; ++It )
			{
				if (It.Key() == TEXT("Package"))
				{
					// add this package to the list to be fully loaded later
					PackageNames.AddItem(*(It.Value()));
				}
			}
		}
	}
}

/**
 * Kicks off a list of packages to be read in asynchronously in the background by the
 * async file manager. The package will be serialized from RAM later.
 * 
 * @param PackageNames The list of package names to async preload
 */
void AsyncPreloadPackageList(const TArray<FString>& PackageNames)
{
	// Iterate over all native script packages and preload them.
	for (INT PackageIndex=0; PackageIndex<PackageNames.Num(); PackageIndex++)
	{
		// let ULinkerLoad class manage preloading
		ULinkerLoad::AsyncPreloadPackage(*PackageNames(PackageIndex));
	}
}

/**
 * Fully loads a list of packages.
 * 
 * @param PackageNames The list of package names to load
 */
void LoadPackageList(const TArray<FString>& PackageNames)
{
	// Iterate over all native script packages and fully load them.
	for( INT PackageIndex=0; PackageIndex<PackageNames.Num(); PackageIndex++ )
	{
		UObject* Package = UObject::LoadPackage(NULL, *PackageNames(PackageIndex), LOAD_None);
	}
}

/**
 * This will load up all of the various "core" .u packages.
 * 
 * We do this as an optimization for load times.  We also do this such that we can be assured that 
 * all of the .u classes are loaded so we can then verify them.
 *
 * @param bExcludeGamePackages	Whether to exclude game packages
 */
void LoadAllNativeScriptPackages( UBOOL bExcludeGamePackages )
{
	TArray<FString> PackageNames;

	// call the shared function to get all native script package names
	GetNativeScriptPackageNames(PackageNames, bExcludeGamePackages, FALSE);

	// load them
	LoadPackageList(PackageNames);
}

/**
 * @return Name of the startup map from the engine ini file
 */
FString GetStartupMap()
{
	// default map
	FString DefaultLocalMap;
	GConfig->GetString(TEXT("URL"), TEXT("LocalMap"), DefaultLocalMap, GEngineIni);
		
	// parse the map name out and add it to the list
	FURL TempURL(NULL, *DefaultLocalMap, TRAVEL_Absolute);
	return FFilename(TempURL.Map).GetBaseFilename();
}

/**
 * Load all startup packags. If desired preload async followed by serialization from memory.
 * Only native script packages are loaded from memory if we're not using the GUseSeekFreeLoading
 * codepath as we need to reset the loader on those packages and don't want to keep the bulk
 * data around. Native script packages don't have any bulk data so this doesn't matter.
 *
 * The list of additional packages can be found at Engine.StartupPackages and if seekfree loading
 * is enabled, the startup map is going to be preloaded as well.
 */
void LoadStartupPackages()
{
	// should script packages load from memory?
	UBOOL bSerializeStartupPackagesFromMemory = FALSE;
	GConfig->GetBool(TEXT("Engine.StartupPackages"), TEXT("bSerializeStartupPackagesFromMemory"), bSerializeStartupPackagesFromMemory, GEngineIni);

	// Get all native script package names.
	TArray<FString> NativeScriptPackages;
	GetNativeScriptPackageNames(NativeScriptPackages, FALSE, FALSE);

	// Get list of non-native startup packages.
	TArray<FString> NonNativeStartupPackages;
	GetNonNativeStartupPackageNames(NonNativeStartupPackages);

	if( bSerializeStartupPackagesFromMemory )
	{
		// start preloading them
		AsyncPreloadPackageList(NativeScriptPackages);

		if( GUseSeekFreeLoading )
		{
			// kick them off to be preloaded
			AsyncPreloadPackageList(NonNativeStartupPackages);

			// kick off startup map (but don't add it to the list to load later, because UGameEngine::Init will load it)
			ULinkerLoad::AsyncPreloadPackage(*GetStartupMap());
		}
	}

	// Load the native script packages.
	LoadPackageList(NativeScriptPackages);

	if( !GUseSeekFreeLoading && !GIsEditor )
	{
		// Reset loaders on native script packages as they are always fully loaded. This ensures the memory
		// for the async preloading process can be reclaimed.
		for( INT PackageIndex=0; PackageIndex<NativeScriptPackages.Num(); PackageIndex++ )
		{
			const FString& PackageName = NativeScriptPackages(PackageIndex);
			UPackage* Package = FindObjectChecked<UPackage>(NULL,*PackageName,TRUE);
			UObject::ResetLoaders( Package );
		}
	}
#if !CONSOLE
	// with PC seekfree, the shadercaches are loaded specially, they aren't cooked in, so at 
	// this point, their linkers are still in memory and should be reset
	else
	{
		for (TObjectIterator<UShaderCache> It; It; ++It)
		{
			UObject::ResetLoaders(It->GetOutermost());
		}
	}
#endif

	// Load the other startup packages.
	LoadPackageList(NonNativeStartupPackages);
}

/**
 * Get a list of all packages that may be needed at startup, and could be
 * loaded async in the background when doing seek free loading
 *
 * @param PackageNames The output list of package names
 * @param EngineConfigFilename Optional engine config filename to use to lookup the package settings
 */
void appGetAllPotentialStartupPackageNames(TArray<FString>& PackageNames, const TCHAR* EngineConfigFilename)
{
	// native script packages
	GetNativeScriptPackageNames(PackageNames, FALSE, TRUE);
	// startup packages from .ini
	GetNonNativeStartupPackageNames(PackageNames, EngineConfigFilename);
	// add the startup map
	PackageNames.AddItem(*GetStartupMap());

	// go through and add localized versions of each package for all known languages
	// since they could be used at runtime depending on the language at runtime
	INT NumPackages = PackageNames.Num();
	for (INT PackageIndex = 0; PackageIndex < NumPackages; PackageIndex++)
	{
		// add the packagename with _XXX language extension
	    for (INT LangIndex = 0; GKnownLanguageExtensions[LangIndex]; LangIndex++)
		{
			PackageNames.AddItem(*(PackageNames(PackageIndex) + TEXT("_") + GKnownLanguageExtensions[LangIndex]));
		}
	}
}

/**
 * Checks for native class script/ C++ mismatch of class size and member variable
 * offset. Note that only the first and last member variable of each class in addition
 * to all member variables of noexport classes are verified to work around a compiler
 * limitation. The code is disabled by default as it has quite an effect on compile
 * time though is an invaluable tool for sanity checking and bringing up new
 * platforms.
 */
void CheckNativeClassSizes()
{
#if CHECK_NATIVE_CLASS_SIZES  // pass in via /DCHECK_NATIVE_CLASS_SIZES or set CL=/DCHECK_NATIVE_CLASS_SIZES and then this will be activated.  Good for setting up on your continuous integration machine
	debugf(TEXT("CheckNativeClassSizes..."));
	UBOOL Mismatch = FALSE;
	VERIFY_CLASS_SIZE_NODIE(ACoverNode);
	VERIFY_CLASS_SIZE_NODIE(AWeapon);
	VERIFY_CLASS_SIZE_NODIE(AKActor);
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name)
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#define VERIFY_CLASS_SIZES
	#include "CoreClasses.h"
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
	#include "EngineFoliageClasses.h"
	#include "EngineSpeedTreeClasses.h"
	#include "EngineLensFlareClasses.h"
	#include "GameFrameworkClasses.h"
	#include "GameFrameworkAnimClasses.h"
	#include "UnrealScriptTestClasses.h"
#if !CONSOLE	
	#include "EditorClasses.h"
	#include "UnrealEdClasses.h"
	#include "UnrealEdCascadeClasses.h"
	#include "UnrealEdPrivateClasses.h"
#endif
	#include "IpDrvClasses.h"
#if GAMENAME == WARGAME
#include "WarfareGameClasses.h"
#include "WarfareGameCameraClasses.h"
#include "WarfareGameSequenceClasses.h"
#include "WarfareGameSpecialMovesClasses.h"
#include "WarfareGameVehicleClasses.h"
#include "WarfareGameSoundClasses.h"
#include "WarfareGameAIClasses.h"
#if !CONSOLE
	#include "WarfareEditorClasses.h"
#endif
#elif GAMENAME == GEARGAME
#include "GearGameClasses.h"
#include "GearGameCameraClasses.h"
#include "GearGameSequenceClasses.h"
#include "GearGameSpecialMovesClasses.h"
#include "GearGameVehicleClasses.h"
#include "GearGameSoundClasses.h"
#include "GearGameAIClasses.h"
#if !CONSOLE
	#include "GearEditorClasses.h"
#endif
#elif GAMENAME == UTGAME
#include "UTGameClasses.h"
#include "UTGameAnimationClasses.h"
#include "UTGameSequenceClasses.h"
#include "UTGameUIClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameUIFrontEndClasses.h"
#if !CONSOLE
	#include "UTEditorClasses.h"
#endif

#elif GAMENAME == EXAMPLEGAME
	#include "ExampleGameClasses.h"
#if !CONSOLE
    #include "ExampleEditorClasses.h"
#endif
#else
	#error Hook up your game name here
#endif
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
	#undef VERIFY_CLASS_SIZES

	if( ( Mismatch == TRUE ) && ( GIsUnattended == TRUE ) )
	{
		appErrorf( NAME_Error, *LocalizeUnrealEd("Error_ScriptClassSizeMismatch") );
	}
	else if( Mismatch == TRUE )
	{
		appErrorf( NAME_FriendlyError, *LocalizeUnrealEd("Error_ScriptClassSizeMismatch") );
	}
	else
	{
		debugf(TEXT("CheckNativeClassSizes completed with no errors"));
	}
#endif  // #fi CHECK_NATIVE_CLASS_SIZES 
}


//#define EPICINTERNAL_CHECK_FOR_WARFARE_MILESTONE_BUILD 0 // this is set by the build machine
#define EPICINTERNAL_THIS_IS_WARFARE_MILESTONE_BUILD 0 // in the warfare branch set this to 1.  In the main branch set this to 0
/**
 * This is meant for internal epic usage to check and see if the game is allowed to run.
 * This is specifically meant to be used to check and see if warfare is using a special
 * milestone build or not
 **/
void CheckForWarfareMiletoneBuild()
{
	if( ( FString(GGameName) == TEXT("War") )
		&& ( EPICINTERNAL_THIS_IS_WARFARE_MILESTONE_BUILD == 0 )
		)
	{
		appMsgf( AMT_OK, TEXT("Your build is not the special warfare build!  Please use the warfare-artist-sync.bat to get the correct build." ) );
		GIsRequestingExit = TRUE;
	}
}

/**
 * This is meant for internal epic usage to check and see if the editor is allowed to run.  
 * It looks to see if a number of files exist and based on which ones exist and their contents
 * it will either allow the editor to run or it will tell the user to upgrade.
 **/
void CheckForQABuildCandidateStatus()
{
	// check to see here if we are allowed to run the editor!
	// Query whether this is an epic internal build or not.

	// QA build process code
	UBOOL bOnlyUseQABuildSemaphore = FALSE;

	if( ( GFileManager->FileSize( TEXT("\\\\Build-Server-01") PATH_SEPARATOR TEXT("BuildFlags") PATH_SEPARATOR TEXT("OnlyUseQABuildSemaphore.txt") ) >= 0 )
		&& ( FString(GGameName) != TEXT("War") )
		)
	{
		bOnlyUseQABuildSemaphore = TRUE;
	}

	const TCHAR* QABuildInfoFileName = TEXT("\\\\Build-Server-01") PATH_SEPARATOR TEXT("BuildFlags") PATH_SEPARATOR TEXT("QABuildInfo.ini");

	// if the QABuildInfo.ini file exists
	if( GFileManager->FileSize( QABuildInfoFileName ) >= 0 )
	{
		FConfigFile TmpConfigFile;
		TmpConfigFile.Read( QABuildInfoFileName );
		DOUBLE QABuildVersion = 0;

		TmpConfigFile.GetDouble( TEXT("QAVersion"), TEXT("EngineVersion"), QABuildVersion );

		if( ( bOnlyUseQABuildSemaphore == TRUE )
			&& ( GEngineVersion != static_cast<INT>(QABuildVersion) )
			)
		{
			appMsgf( AMT_OK, TEXT("Your build is not the QA Candidate that is being tested.  Please sync to the QA build which is labeled as:  currentQABuildInTesting" ) );
			GIsRequestingExit = TRUE;
		}
		else if( GEngineVersion < static_cast<INT>(QABuildVersion) )
		{
			appMsgf( AMT_OK, TEXT("Your build is out of date.  Please sync to the latest build which labeled as:  latestUnrealEngine3" ) );
			GIsRequestingExit = TRUE;
		}
	}
}

/**
 * Update GCurrentTime/ GDeltaTime while taking into account max tick rate.
 */
void appUpdateTimaAndHandleMaxTickRate()
{
	static DOUBLE LastTime = appSeconds();

	// Figure out whether we want to use real or fixed time step.
	UBOOL bUseFixedTimeStep = GIsBenchmarking;
#if ALLOW_TRACEDUMP
	extern UBOOL GShouldTraceGameTick;
	extern UBOOL GShouldTraceRenderTick;
	bUseFixedTimeStep = bUseFixedTimeStep || GShouldTraceGameTick || GShouldTraceRenderTick;
#endif

	// Calculate delta time and update time.
	if( bUseFixedTimeStep )
	{
		GDeltaTime		= 1.0f / 30.0f;
		LastTime		= GCurrentTime;
		GCurrentTime	+= GDeltaTime;
	}
	else
	{
		// Get max tick rate based on network settings and current delta time.
		GCurrentTime		= appSeconds();
		FLOAT WaitTime		= 0;
		FLOAT DeltaTime		= GCurrentTime - LastTime;
		FLOAT MaxTickRate	= GEngine->GetMaxTickRate( DeltaTime );
		// Convert from max FPS to wait time.
		if( MaxTickRate > 0 )
		{
			WaitTime = Max( 1.f / MaxTickRate - DeltaTime, 0.f );
		}

		// Enforce maximum framerate and smooth framerate by waiting.
		STAT( DOUBLE ActualWaitTime = 0.f ); 
		if( WaitTime > 0 )
		{
			STAT(FScopeSecondsCounter ActualWaitTimeTimer(ActualWaitTime));
			SCOPE_CYCLE_COUNTER(STAT_GameTickWaitTime);

			// Sleep if we're waiting more than 5 ms. We set the scheduler granularity to 1 ms
			// at startup on PC. We reserve 2 ms of slack time which we will wait for by giving
			// up our timeslice.
			if( WaitTime > 5 / 1000.f )
			{
				appSleep( WaitTime - 3 / 1000.f );
			}

			// Give up timeslice for remainder of wait time.
			DOUBLE WaitStartTime = GCurrentTime;
			while( GCurrentTime - WaitStartTime < WaitTime )
			{
				GCurrentTime = appSeconds();
				appSleep( 0 );
			}
		}

		SET_FLOAT_STAT(STAT_GameTickWantedWaitTime,WaitTime * 1000.f);
		SET_FLOAT_STAT(STAT_GameTickAdditionalWaitTime,Max<FLOAT>((ActualWaitTime-WaitTime)*1000.f,0.f));

		GDeltaTime		= GCurrentTime - LastTime;
		LastTime		= GCurrentTime;
	}
}

/*-----------------------------------------------------------------------------
	FEngineLoop implementation.
-----------------------------------------------------------------------------*/

INT FEngineLoop::PreInit( const TCHAR* CmdLine )
{
	// setup the streaming resource flush function pointer
	GFlushStreamingFunc = &FlushResourceStreaming;

	// Set the game name.
	appSetGameName();

	// Figure out whether we want to override the package map with the seekfree version. Needs to happen before the first call
	// to UClass::Link!
	GUseSeekFreePackageMap = GUseSeekFreePackageMap || ParseParam( CmdLine, TEXT("SEEKFREEPACKAGEMAP") );

#if !CONSOLE
	GUseSeekFreeLoading = ParseParam( CmdLine, TEXT("SEEKFREELOADING") );
#endif

	// Override compression settings wrt size.
	GAlwaysBiasCompressionForSize = ParseParam( CmdLine, TEXT("BIASCOMPRESSIONFORSIZE") );

#ifndef XBOX
	// This is done in appXenonInit on Xenon.
	GSynchronizeFactory = &SynchronizeFactory;
	GThreadFactory		= &ThreadFactory;
#ifdef __GNUC__
#if PS3
	//@todo joeg -- Get this working for linux
	GThreadPool = &ThreadPool;
	verify(GThreadPool->Create(1));
#endif
	appInit( CmdLine, &Log, NULL, &Error, &GameWarn, &FileManager, &GameEventCallback, &GameQueryCallback, FConfigCacheIni::Factory );
#else	// __GNUC__
	GThreadPool = &ThreadPool;
	verify(GThreadPool->Create(1));
	// see if we were launched from our .com command line launcher
	InheritedLogConsole.Connect();

#if GAMENAME == UTGAME
	#if WITH_GAMESPY
		FString ModdedCmdLine(CmdLine);
		FString IgnoredString;
		// Append the default engine ini overload if not present
		if (Parse(CmdLine,TEXT("DEFENGINEINI="),IgnoredString) == FALSE)
		{
			ModdedCmdLine += TEXT(" -DEFENGINEINI=");
			ModdedCmdLine += appGameConfigDir();
			ModdedCmdLine += TEXT("DefaultEngineGameSpy.ini");
			CmdLine = *ModdedCmdLine;
		}
	#endif
#elif GAMENAME == WARGAME
	#if WITH_PANORAMA
		FString ModdedCmdLine(CmdLine);
		FString IgnoredString;
		// Append the default engine ini overload if not present
		if (Parse(CmdLine,TEXT("DEFENGINEINI="),IgnoredString) == FALSE)
		{
			ModdedCmdLine += TEXT(" -DEFENGINEINI=");
			ModdedCmdLine += appGameConfigDir();
			ModdedCmdLine += TEXT("DefaultEngineG4WLive.ini");
			CmdLine = *ModdedCmdLine;
		}
	#endif
#endif

	appInit( CmdLine, &Log, &InheritedLogConsole, &Error, &GameWarn, &FileManager, &GameEventCallback, &GameQueryCallback, FConfigCacheIni::Factory );
#endif	// __GNUC__

#else	// XBOX
	appInit( CmdLine, &Log, NULL       , &Error, &GameWarn, &FileManager, &GameEventCallback, &GameQueryCallback, FConfigCacheIni::Factory );
	// WarGame uses a movie so we don't need to load and display the splash screen.
#if GAMENAME != WARGAME
	appXenonShowSplash(TEXT("Xbox360\\Splash.bmp"));
#endif
#endif	// XBOX

	UBOOL bSetupSystemSettingsForEditor = FALSE;
#if !CONSOLE
	// Figure out whether we're the editor, ucc or the game.
	const SIZE_T CommandLineSize = appStrlen(appCmdLine())+1;
	TCHAR* CommandLine			= new TCHAR[ CommandLineSize ];
	appStrcpy( CommandLine, CommandLineSize, appCmdLine() );
	const TCHAR* ParsedCmdLine	= CommandLine;
	FString Token				= ParseToken( ParsedCmdLine, 0);

	// trim any whitespace at edges of string - this can happen if the token was quoted with leaing or trailing whitespace
	// VC++ tends to do this in its "external tools" config
	Token = Token.Trim();

	bSetupSystemSettingsForEditor = Token == TEXT("EDITOR");
#endif

	// Initialize system settings before textures are loaded from disk, ...
	GSystemSettings.Initialize( bSetupSystemSettingsForEditor );
	
	// Initialize global shadow volume setting
    GConfig->GetBool( TEXT("Engine.Engine"), TEXT("AllowShadowVolumes"), GAllowShadowVolumes, GEngineIni );

#if !CONSOLE
#if SPAWN_CHILD_PROCESS_TO_COMPILE
	// If the scripts need rebuilding, ask the user if they'd like to rebuild them
	if(Token != TEXT("MAKE"))
	{	
		UBOOL bIsDone = FALSE;
		while (!bIsDone && AreScriptPackagesOutOfDate())
		{
			// figure out if we should compile scripts or not
			UBOOL bShouldCompileScripts = GIsUnattended;
			if( ( GIsUnattended == FALSE ) && ( GUseSeekFreeLoading == FALSE ) )
			{
				INT Result = appMsgf(AMT_YesNoCancel,TEXT("Scripts are outdated. Would you like to rebuild now?"));

				// check for Cancel
				if (Result == 2)
				{
					exit(1);
				}
				else
				{
					// 0 is Yes, 1 is No (unlike AMT_YesNo, where 1 is Yes)
					bShouldCompileScripts = Result == 0;
				}
			}

			if (bShouldCompileScripts)
			{
#ifdef _MSC_VER
				// get executable name
				TCHAR ExeName[MAX_PATH];
				GetModuleFileName(NULL, ExeName, ARRAY_COUNT(ExeName));

				// create the new process, with params as follows:
				//  - if we are running unattended, pass on the unattended flag
				//  - if not unattended, then tell the subprocess to pause when there is an error compiling
				//  - if we are running silently, pass on the flag
				void* ProcHandle = appCreateProc(ExeName, *FString::Printf(TEXT("make %s %s"), 
					GIsUnattended ? TEXT("-unattended") : TEXT("-nopauseonsuccess"), 
					GIsSilent ? TEXT("-silent") : TEXT("")));
				
				INT ReturnCode;
				// wait for it to finish and get return code
				while (!appGetProcReturnCode(ProcHandle, &ReturnCode))
				{
					appSleep(0);
				}

				// if we are running unattended, we can't run forever, so only allow one pass of compiling script code
				if (GIsUnattended)
				{
					bIsDone = TRUE;
				}
#else
				// if we can't spawn childprocess, just make within this process
				Token = TEXT("MAKE");
				bIsDone = TRUE;
#endif
			}
			else
			{
				bIsDone = TRUE;
			}
		}

		// reload the package cache, as the child process may have created script packages!
		GPackageFileCache->CachePaths();
	}
#else // SPAWN_CHILD_PROCESS_TO_COMPILE

	// If the scripts need rebuilding, ask the user if they'd like to rebuild them
	if(Token != TEXT("MAKE") && AreScriptPackagesOutOfDate())
	{
		// If we are unattended, don't popup the dialog, just continue and do nothing
		// Default response in unattended mode for appMsgf is cancel - which exits immediately. It should default to 'No' in this case
		if( ( GIsUnattended == FALSE ) && ( GUseSeekFreeLoading == FALSE ) )
		{
			switch ( appMsgf(AMT_YesNoCancel, TEXT("Scripts are outdated. Would you like to rebuild now?")) )
			{
				case 0:		// Yes - compile
					Token = TEXT("MAKE");
					break;

				case 1:		// No - do nothing
					break;

				case 2:		// Cancel - exit
					appRequestExit( TRUE );
			}
		}
	}

#endif

	if( Token == TEXT("MAKE") )
	{
		// allow ConditionalLink() to call Link() for non-intrinsic classes during make
		GUglyHackFlags |= HACK_ClassLoadingDisabled;

		// Rebuilding script requires some hacks in the engine so we flag that.
		GIsUCCMake = TRUE;
	}
#endif	// CONSOLE

	// Deal with static linking.
	InitializeRegistrantsAndRegisterNames();

	// create global debug commincations object (for talking to UnrealConsole)
//#if CONSOLE
#if !FINAL_RELEASE
	GDebugChannel = new FDebugServer();
	GDebugChannel->Init(DefaultDebugChannelReceivePort, DefaultDebugChannelSendPort);
#endif
//#endif

	// Create the streaming manager and add the default streamers.
	FStreamingManagerTexture* TextureStreamingManager = new FStreamingManagerTexture();
	TextureStreamingManager->AddTextureStreamingHandler( new FStreamingHandlerTextureStatic() );
	TextureStreamingManager->AddTextureStreamingHandler( new FStreamingHandlerTextureLevelForced() );
	GStreamingManager = new FStreamingManagerCollection();
	GStreamingManager->AddStreamingManager( TextureStreamingManager );
	
	GIOManager = new FIOManager();

	// Create the async IO manager.
#if XBOX
	FAsyncIOSystemXenon*	AsyncIOSystem = new FAsyncIOSystemXenon();
#elif PS3
	FAsyncIOSystemPS3*		AsyncIOSystem = new FAsyncIOSystemPS3();
#elif _MSC_VER
	FAsyncIOSystemWindows*	AsyncIOSystem = new FAsyncIOSystemWindows();
#else
	#error implement async io manager for platform
#endif
	// This will auto- register itself with GIOMananger and be cleaned up when the manager is destroyed.
	AsyncIOThread = GThreadFactory->CreateThread( AsyncIOSystem, TEXT("AsyncIOSystem"), 0, 0, 0, TPri_BelowNormal );
	check(AsyncIOThread);
#if XBOX
	// See UnXenon.h
	AsyncIOThread->SetProcessorAffinity(ASYNCIO_HWTHREAD);
#endif

	// Init physics engine before loading anything, in case we want to do things like cook during post-load.
	InitGameRBPhys();

#if WITH_FACEFX
	// Make sure FaceFX is initialized.
	UnInitFaceFX();
#endif // WITH_FACEFX

#if !CONSOLE
#ifndef __GNUC__
	if( Token == TEXT("EDITOR") )
	{		
		// release our .com launcher -- this is to mimic previous behavior of detaching the console we launched from
		InheritedLogConsole.DisconnectInherited();

#if __WIN32__
		// here we start COM (used by some console support DLLs)
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		// make sure we aren't running on Remote Desktop
		if (GetSystemMetrics(SM_REMOTESESSION))
		{
			appMsgf(AMT_OK, *LocalizeError(TEXT("Error_RemoteDesktop"), TEXT("Launch")));
			return 1;
		}
#endif

		// We're the editor.
		GIsClient	= 1;
		GIsServer	= 1;
		GIsEditor	= 1;
		GIsUCC		= 0;
		GGameIcon	= GEditorIcon;

		// UnrealEd requires a special callback handler and feedback context.
		GCallbackEvent	= &UnrealEdEventCallback;
		GCallbackQuery	= &UnrealEdQueryCallback;
		GWarn		= &UnrealEdWarn;

		// Remove "EDITOR" from command line.
		appStrcpy( GCmdLine, ParsedCmdLine );

		// Set UnrealEd as the current package (used for e.g. log and localization files).
		appStrcpy( GPackage, TEXT("UnrealEd") );
 
#if EPICINTERNAL_CHECK_FOR_QA_SEMAPHORE // this should be set by the build machine
		// so if we are checking for qa semaphore BUT we also are doing a warfare milestone build and we are warfare then don't check all other games do check
#if !( EPICINTERNAL_CHECK_FOR_WARFARE_MILESTONE_BUILD && ( GAMENAME == WARGAME ) )
		if( GIsEpicInternal == TRUE )
		{
			//CheckForQABuildCandidateStatus();
			if( GIsRequestingExit == TRUE )
			{
				//appPreExit()  // this crashes as we have not initialized anything yet and appPreExit assumes we have
				return 1;
			}
		}
#endif // warfare milestone build checking
#endif
	}
	else
#endif
	{
		// See whether the first token on the command line is a commandlet.

		//@hack: We need to set these before calling StaticLoadClass so all required data gets loaded for the commandlets.
		GIsClient	= 1;
		GIsServer	= 1;
		GIsEditor	= 1;
		GIsUCC		= 1;
		UBOOL bIsSeekFreeDedicatedServer = FALSE;

		UClass* Class = NULL;

		// We need to disregard the empty token as we try finding Token + "Commandlet" which would result in finding the
		// UCommandlet class if Token is empty.
		if( Token.Len() && appStrnicmp(*Token,TEXT("-"),1) != 0 )
		{
			DWORD CommandletLoadFlags = LOAD_NoWarn|LOAD_Quiet;
			UBOOL bNoFail = FALSE;
			if ( Token == TEXT("RUN") )
			{
				Token = ParseToken( ParsedCmdLine, 0);
				if ( Token.Len() == 0 )
				{
					warnf(TEXT("You must specify a commandlet to run, e.g. game.exe run test.samplecommandlet"));
					return 1;
				}
				bNoFail = TRUE;
				if ( Token == TEXT("MAKE") || Token == TEXT("MAKECOMMANDLET") )
				{
					// We can't bind to .u files if we want to build them via the make commandlet, hence the LOAD_DisallowFiles.
					CommandletLoadFlags |= LOAD_DisallowFiles;

					// allow the make commandlet to be invoked without requiring the package name
					Token = TEXT("Editor.MakeCommandlet");
				}

				if ( Token == TEXT("SERVER") || Token == TEXT("SERVERCOMMANDLET") )
				{
					// allow the server commandlet to be invoked without requiring the package name
					Token = TEXT("Engine.ServerCommandlet");
					bIsSeekFreeDedicatedServer = GUseSeekFreeLoading;
				}

				Class = LoadClass<UCommandlet>(NULL, *Token, NULL, CommandletLoadFlags, NULL );
				if ( Class == NULL && Token.InStr(TEXT("Commandlet")) == INDEX_NONE )
				{
					Class = LoadClass<UCommandlet>(NULL, *(Token+TEXT("Commandlet")), NULL, CommandletLoadFlags, NULL );
				}
			}
			else
			{
				// allow these commandlets to be invoked without requring the package name
				if ( Token == TEXT("MAKE") || Token == TEXT("MAKECOMMANDLET") )
				{
					// We can't bind to .u files if we want to build them via the make commandlet, hence the LOAD_DisallowFiles.
					CommandletLoadFlags |= LOAD_DisallowFiles;

					// allow the make commandlet to be invoked without requiring the package name
					Token = TEXT("Editor.MakeCommandlet");
				}

				else if ( Token == TEXT("SERVER") || Token == TEXT("SERVERCOMMANDLET") )
				{
					// allow the server commandlet to be invoked without requiring the package name
					Token = TEXT("Engine.ServerCommandlet");
					bIsSeekFreeDedicatedServer = GUseSeekFreeLoading;
				}
			}

			if (bIsSeekFreeDedicatedServer)
			{
				// when starting up a seekfree dedicated server, we act like we're the game starting up with the seekfree
				GIsEditor = FALSE;
				GIsUCC = FALSE;
				GIsGame = TRUE;

				// load up the seekfree startup packages
				LoadStartupPackages();
			}

			// See whether we're trying to run a commandlet. @warning: only works for native commandlets
			 
			//  Try various common name mangling approaches to find the commandlet class...
			if( !Class )
			{
				Class = FindObject<UClass>(ANY_PACKAGE,*Token,FALSE);
			}
			if( !Class )
			{
				Class = FindObject<UClass>(ANY_PACKAGE,*(Token+TEXT("Commandlet")),FALSE);
			}

			if( !Class && Token.InStr(TEXT(".")) != -1)
			{
				Class = LoadObject<UClass>(NULL,*Token,NULL,LOAD_NoWarn,NULL);
			}
			if( !Class && Token.InStr(TEXT(".")) != -1 )
			{
				Class = LoadObject<UClass>(NULL,*(Token+TEXT("Commandlet")),NULL,LOAD_NoWarn,NULL);
			}



			// ... and if successful actually load it.
			if( Class )
			{
				if ( Class->HasAnyClassFlags(CLASS_Intrinsic) )
				{
					// if this commandlet is native-only, we'll need to manually load its parent classes to ensure that it has
					// correct values in its PropertyLink array after it has been linked
					TArray<UClass*> ClassHierarchy;
					for ( UClass* ClassToLoad = Class->GetSuperClass(); ClassToLoad != NULL; ClassToLoad = ClassToLoad->GetSuperClass() )
					{
						ClassHierarchy.AddItem(ClassToLoad);
					}
					for ( INT i = ClassHierarchy.Num() - 1; i >= 0; i-- )
					{
						UClass* LoadedClass = UObject::StaticLoadClass( UObject::StaticClass(), NULL, *(ClassHierarchy(i)->GetPathName()), NULL, CommandletLoadFlags, NULL );
						check(LoadedClass);
					}
				}
				Class = UObject::StaticLoadClass( UCommandlet::StaticClass(), NULL, *Class->GetPathName(), NULL, CommandletLoadFlags, NULL );
			}
			
			if ( Class == NULL && bNoFail == TRUE )
			{
				appMsgf(AMT_OK, TEXT("Failed to find a commandlet named '%s'"), *Token);
				return 1;
			}
		}

		if( Class != NULL )
		{
			//@todo - make this block a separate function

			// The first token was a commandlet so execute it.
	
			// Remove commandlet name from command line.
			appStrcpy( GCmdLine, ParsedCmdLine );

			// Set UCC as the current package (used for e.g. log and localization files).
			appStrcpy( GPackage, TEXT("UCC") );

			// Bring up console unless we're a silent build.
			if( GLogConsole && !GIsSilent )
			{
				GLogConsole->Show( TRUE );
			}

			// log out the engine meta data when running a commandlet
			debugf( NAME_Init, TEXT("Version: %i"), GEngineVersion );
			debugf( NAME_Init, TEXT("Epic Internal: %i"), GIsEpicInternal );
			debugf( NAME_Init, TEXT("Compiled: %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__) );
			debugf( NAME_Init, TEXT("Command line: %s"), appCmdLine() );
			debugf( NAME_Init, TEXT("Base directory: %s"), appBaseDir() );
			debugf( NAME_Init, TEXT("Character set: %s"), sizeof(TCHAR)==1 ? TEXT("ANSI") : TEXT("Unicode") );
			debugf( TEXT("Executing %s"), *Class->GetFullName() );

			// Allow commandlets to individually override those settings.
			UCommandlet* Default = CastChecked<UCommandlet>(Class->GetDefaultObject(TRUE));
			Class->ConditionalLink();

			if ( GIsRequestingExit )
			{
				// commandlet set GIsRequestingExit in StaticInitialize
				return 1;
			}

			GIsClient	= Default->IsClient;
			GIsServer	= Default->IsServer;
			GIsEditor	= Default->IsEditor;
			GIsGame		= !GIsEditor;
			GIsUCC		= TRUE;

			// Reset aux log if we don't want to log to the console window.
			if( !Default->LogToConsole )
			{
				GLog->RemoveOutputDevice( GLogConsole );
			}

#if __WIN32__
			if( ParseParam(appCmdLine(),TEXT("AUTODEBUG")) )
			{
				debugf(TEXT("Attaching to Debugger"));
				UDebuggerCore* Debugger = new UDebuggerCore();
				GDebugger 			= Debugger;

				// we want the UDebugger to break on the first bytecode it processes
				Debugger->SetBreakASAP(TRUE);

				// we need to allow the debugger to process debug opcodes prior to the first tick, so enable the debugger.
				Debugger->NotifyBeginTick();
			}
#endif

			GIsInitialLoad		= FALSE;
			GIsRequestingExit	= TRUE;	// so CTRL-C will exit immediately

			// allow the commandlet the opportunity to create a custom engine
			Class->GetDefaultObject<UCommandlet>()->CreateCustomEngine();
			if ( GEngine == NULL )
			{
				if ( GIsEditor )
				{
					UClass* EditorEngineClass	= UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.EditorEngine"), NULL, LOAD_None, NULL );

					// must do this here so that the engine object that we create on the next line receives the correct property values
					EditorEngineClass->GetDefaultObject(TRUE);
					EditorEngineClass->ConditionalLink();
					GEngine = GEditor			= ConstructObject<UEditorEngine>( EditorEngineClass );
					debugf(TEXT("Initializing Editor Engine..."));
					GEditor->InitEditor();
					debugf(TEXT("Initializing Editor Engine Completed"));
				}
				else
				{
					// pretend we're the game (with no client) again while loading/initializing the engine from seekfree 
					if (bIsSeekFreeDedicatedServer)
					{
						GIsEditor = FALSE;
						GIsGame = TRUE;
						GIsUCC = FALSE;
					}
					UClass* EngineClass = UObject::StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.GameEngine"), NULL, LOAD_None, NULL );
					// must do this here so that the engine object that we create on the next line receives the correct property values
					EngineClass->GetDefaultObject(TRUE);
					EngineClass->ConditionalLink();
					GEngine = ConstructObject<UEngine>( EngineClass );
					debugf(TEXT("Initializing Game Engine..."));
					GEngine->Init();
					debugf(TEXT("Initializing Game Engine Completed"));

					// reset the flags if needed
					if (bIsSeekFreeDedicatedServer)
					{
						GIsEditor	= Default->IsEditor;
						GIsGame		= !GIsEditor;
						GIsUCC		= TRUE;
					}
				}
			}
	

			// Load in the console support dlls so commandlets can convert data
			GConsoleSupportContainer->LoadAllConsoleSupportModules();

			UCommandlet* Commandlet = ConstructObject<UCommandlet>( Class );
			check(Commandlet);

			// Execute the commandlet.
			DOUBLE CommandletExecutionStartTime = appSeconds();

			Commandlet->InitExecution();
			Commandlet->ParseParms( appCmdLine() );
			INT ErrorLevel = Commandlet->Main( appCmdLine() );

			// Log warning/ error summary.
			if( Commandlet->ShowErrorCount )
			{
				if( GWarn->Errors.Num() || GWarn->Warnings.Num() )
				{
					SET_WARN_COLOR(COLOR_WHITE);
					warnf(TEXT(""));
					warnf(TEXT("Warning/Error Summary"));
					warnf(TEXT("---------------------"));

					SET_WARN_COLOR(COLOR_RED);
					for(INT I = 0; I < Min(50, GWarn->Errors.Num()); I++)
					{
						warnf((TCHAR*)GWarn->Errors(I).GetCharArray().GetData());
					}
					if (GWarn->Errors.Num() > 50)
					{
						SET_WARN_COLOR(COLOR_WHITE);
						warnf(TEXT("NOTE: Only first 50 warnings displayed."));
					}

					SET_WARN_COLOR(COLOR_YELLOW);
					for(INT I = 0; I < Min(50, GWarn->Warnings.Num()); I++)
					{
						warnf((TCHAR*)GWarn->Warnings(I).GetCharArray().GetData());
					}
					if (GWarn->Warnings.Num() > 50)
					{
						SET_WARN_COLOR(COLOR_WHITE);
						warnf(TEXT("NOTE: Only first 50 warnings displayed."));
					}
				}

				warnf(TEXT(""));

				if( ErrorLevel != 0 )
				{
					SET_WARN_COLOR(COLOR_RED);
					warnf( TEXT("Commandlet->Main return this error code: %d"), ErrorLevel );
					warnf( TEXT("With %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
				}
				else if( ( GWarn->Errors.Num() == 0 ) )
				{
					SET_WARN_COLOR(GWarn->Warnings.Num() ? COLOR_YELLOW : COLOR_GREEN);
					warnf( TEXT("Success - %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
				}
				else
				{
					SET_WARN_COLOR(COLOR_RED);
					warnf( TEXT("Failure - %d error(s), %d warning(s)"), GWarn->Errors.Num(), GWarn->Warnings.Num() );
					ErrorLevel = 1;
				}
				CLEAR_WARN_COLOR();
			}
			else
			{
				warnf( TEXT("Finished.") );
			}
		
			DOUBLE CommandletExecutionEndTime = appSeconds();
			warnf( TEXT("\nExecution of commandlet took:  %.2f seconds"), CommandletExecutionEndTime - CommandletExecutionStartTime );

			// We're ready to exit!
			return ErrorLevel;
		}
		else
#else
	{
#endif
		{
#if __WIN32__
			// make sure we aren't running on Remote Desktop
			if (GetSystemMetrics(SM_REMOTESESSION))
			{
				appMsgf(AMT_OK, *LocalizeError(TEXT("Error_RemoteDesktop"), TEXT("Launch")));
				return 1;
			}
#endif

			// We're a regular client.
			GIsClient		= 1;
			GIsServer		= 0;
#if !CONSOLE
			GIsEditor		= 0;
			GIsUCC			= 0;

			//@todo ship: development hack to force PC to be in line with Xbox 360.
			GSystemSettings.DetailMode = DM_Medium;
#endif

// handle launching dedicated server on console
#if CONSOLE
			// copy command line
			TCHAR* CommandLine	= new TCHAR[ appStrlen(appCmdLine())+1 ];
			appStrcpy( CommandLine, appStrlen(appCmdLine())+1, appCmdLine() );	
			// parse first token
			const TCHAR* ParsedCmdLine	= CommandLine;
			FString Token = ParseToken( ParsedCmdLine, 0);
			Token = Token.Trim();
			// dedicated server command line option
			if( Token == TEXT("SERVER") )
			{
				GIsClient = 0;
				GIsServer = 1;

				// Pretend RHI hasn't been initialized to mimick PC behavior.
				GIsRHIInitialized = FALSE;

				// Remove commandlet name from command line
				appStrcpy( GCmdLine, ParsedCmdLine );
#if XBOX
				// show server splash screen
				appXenonShowSplash(TEXT("Xbox360\\SplashServer.bmp"));
#endif
			}		
#endif

#if !FINAL_RELEASE
			// regular clients can listen for propagation requests
			FObjectPropagator::SetPropagator(&ListenPropagator);
#endif
		}
	}

	// at this point, GIsGame is always not GIsEditor, because the only other way to set GIsGame is to be in a PIE world
	GIsGame = !GIsEditor;

#if EPICINTERNAL_CHECK_FOR_WARFARE_MILESTONE_BUILD
	CheckForWarfareMiletoneBuild(); // this will set GIsRequestingExit
#endif

	// Exit if wanted.
	if( GIsRequestingExit )
	{
		if ( GEngine != NULL )
		{
			GEngine->PreExit();
		}
		appPreExit();
		// appExit is called outside guarded block.
		return 1;
	}

	// Movie recording.
	GIsDumpingMovie	= ParseParam(appCmdLine(),TEXT("DUMPMOVIE"));

	// Benchmarking.
	GIsBenchmarking	= ParseParam(appCmdLine(),TEXT("BENCHMARK"));

	// create the global full screen movie player. 
	// This needs to happen before the rendering thread starts since it adds a rendering thread tickable object
	appInitFullScreenMoviePlayer();
	
#if __WIN32__
	if(!GIsBenchmarking)
	{
		// release our .com launcher -- this is to mimic previous behavior of detaching the console we launched from
		InheritedLogConsole.DisconnectInherited();
	}
#endif

	// Don't update ini files if benchmarking or -noini
	if( GIsBenchmarking || ParseParam(appCmdLine(),TEXT("NOINI")))
	{
		GConfig->Detach( GEngineIni );
		GConfig->Detach( GInputIni );
		GConfig->Detach( GGameIni );
		GConfig->Detach( GEditorIni );
	}

#if !CONSOLE
	// -forceshadermodel2 will force the shader model 2 path even on sm3 hardware
	if (ParseParam(appCmdLine(),TEXT("FORCESHADERMODEL2")))
	{
		warnf(TEXT("Forcing Shader Model 2 Path, including floating point blending emulation."));
		GRHIShaderPlatform = SP_PCD3D_SM2;
		GSupportsFPFiltering = FALSE;
	}
#endif

	// -onethread will disable renderer thread
	if (GIsClient && !ParseParam(appCmdLine(),TEXT("ONETHREAD")))
	{
		// Create the rendering thread.  Note that commandlets don't make it to this point.
		if(GNumHardwareThreads > 1)
		{
			GUseThreadedRendering = TRUE;
			StartRenderingThread();
		}
	}
	return 0;
}

INT FEngineLoop::Init()
{	
	// Initialize shader platform.
	RHIInitializeShaderPlatform();

	// Load all startup packages, always including all native script packages. This is a superset of 
	// LoadAllNativeScriptPackages, which is done by the cooker, etc.
	LoadStartupPackages();

	if( !GUseSeekFreeLoading )
	{
		// Load the shader cache for the current shader platform if it hasn't already been loaded.
		GetLocalShaderCache( GRHIShaderPlatform );
	}

#if CONSOLE	
	// play a looping movie from RAM while booting up
	GFullScreenMovie->GameThreadInitiateStartupSequence();
#endif

	// Iterate over all class objects and force the default objects to be created. Additionally also
	// assembles the token reference stream at this point. This is required for class objects that are
	// not taken into account for garbage collection but have instances that are.
	for( TObjectIterator<UClass> It; It; ++It )
	{
		UClass*	Class = *It;
		// Force the default object to be created.
		Class->GetDefaultObject( TRUE );
		// Assemble reference token stream for garbage collection/ RTGC.
		Class->AssembleReferenceTokenStream();
	}

	// Iterate over all objects and mark them to be part of root set.
	for( FObjectIterator It; It; ++It )
	{
		UObject*		Object		= *It;
		ULinkerLoad*	LinkerLoad	= Cast<ULinkerLoad>(Object);
		// Exclude linkers from root set if we're using seekfree loading.
		if( !GUseSeekFreeLoading || (LinkerLoad == NULL || LinkerLoad->HasAnyFlags(RF_ClassDefaultObject)) )
		{
			Object->AddToRoot();
		}
	}
	debugf(TEXT("%i objects alive at end of initial load."), UObject::GetObjectArrayNum());

	GIsInitialLoad = FALSE;

	// Figure out which UEngine variant to use.
	UClass* EngineClass = NULL;
	if( !GIsEditor )
	{
		// We're the game.
		EngineClass = UObject::StaticLoadClass( UGameEngine::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.GameEngine"), NULL, LOAD_None, NULL );
		GEngine = ConstructObject<UEngine>( EngineClass );
	}
	else
	{
#if !CONSOLE && !PLATFORM_UNIX
		// We're UnrealEd.
		EngineClass = UObject::StaticLoadClass( UUnrealEdEngine::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.UnrealEdEngine"), NULL, LOAD_None, NULL );
		GEngine = GEditor = GUnrealEd = ConstructObject<UUnrealEdEngine>( EngineClass );

		//@todo: put this in a better place? i think we want it for all editor, unrealed, ucc, etc
		// load any Console support modules that exist
		GConsoleSupportContainer->LoadAllConsoleSupportModules();

		debugf(TEXT("Supported Consoles:"));
		for (FConsoleSupportIterator It; It; ++It)
		{
			debugf(TEXT("  %s"), It->GetConsoleName());
		}
#else
		check(0);
#endif
	}

	// If the -nosound or -benchmark parameters are used, disable sound.
	if(ParseParam(appCmdLine(),TEXT("nosound")) || GIsBenchmarking)
	{
		GEngine->bUseSound = FALSE;
	}

	check( GEngine );
	debugf(TEXT("Initializing Engine..."));
	GEngine->Init();
	debugf(TEXT("Initializing Engine Completed"));

	// Verify native class sizes and member offsets.
	CheckNativeClassSizes();

	// Init variables used for benchmarking and ticking.
	GCurrentTime				= appSeconds();
	MaxFrameCounter				= 0;
	MaxTickTime					= 0;
	TotalTickTime				= 0;
	LastFrameCycles				= appCycles();

	FLOAT FloatMaxTickTime		= 0;
	Parse(appCmdLine(),TEXT("SECONDS="),FloatMaxTickTime);
	MaxTickTime					= FloatMaxTickTime;
	MaxFrameCounter				= appTrunc( MaxTickTime * 30 );

	// Optionally Exec an exec file
	FString Temp;
	if( Parse(appCmdLine(), TEXT("EXEC="), Temp) )
	{
		Temp = FString(TEXT("exec ")) + Temp;
		UGameEngine* Engine = Cast<UGameEngine>(GEngine);
		if ( Engine != NULL && Engine->GamePlayers.Num() && Engine->GamePlayers(0) )
		{
			Engine->GamePlayers(0)->Exec( *Temp, *GLog );
		}
	}

	GIsRunning		= TRUE;

	// let the propagator do it's thing now that we are done initializing
	GObjectPropagator->Unpause();

#if !CONSOLE
	// play a looping movie from RAM while booting up
	GFullScreenMovie->GameThreadInitiateStartupSequence();
#endif

	// stop the initial startup movies now. 
	// movies won't actually stop until startup sequence has finished
	GFullScreenMovie->GameThreadStopMovie();

	warnf(TEXT(">>>>>>>>>>>>>> Initial startup: %.2fs <<<<<<<<<<<<<<<"), appSeconds() - GStartTime);

	// handle test movie
	if( appStrfind(GCmdLine, TEXT("movietest")) != NULL )
	{
		// make sure language is correct for localized center channel
		UObject::SetLanguage(*appGetLanguageExt());
		// get the optional moviename from the command line (-movietest=Test.sfd)
		FString TestMovieName;
		Parse(GCmdLine, TEXT("movietest="), TestMovieName);
		// use default if not specified
		if( TestMovieName.Len() > 0 )
		{
			// play movie and wait for it to be done
			GFullScreenMovie->GameThreadPlayMovie(MM_PlayOnceFromStream, *TestMovieName);
			GFullScreenMovie->GameThreadWaitForMovie();			
		}
	}

	return 0;
}

void FEngineLoop::Exit()
{
	GIsRunning	= 0;
	GLogConsole	= NULL;

#if TRACK_FILEIO_STATS
	if( ParseParam( appCmdLine(), TEXT("DUMPFILEIOSTATS") ) )
	{
		GetFileIOStats()->DumpStats();
	}
#endif

#if !CONSOLE
	// Save the local shader cache if it has changed. This avoids loss of shader compilation work due to crashing.
    // GEMINI_TODO: Revisit whether this is worth thet slowdown in material editing once the engine has stabilized.
	SaveLocalShaderCaches();
#endif

	// Output benchmarking data.
	if( GIsBenchmarking )
	{
		FLOAT	MinFrameTime = 1000.f,
				MaxFrameTime = 0.f,
				AvgFrameTime = 0;

		// Determine min/ max framerate (discarding first 10 frames).
		for( INT i=10; i<FrameTimes.Num(); i++ )
		{		
			MinFrameTime = Min( MinFrameTime, FrameTimes(i) );
			MaxFrameTime = Max( MaxFrameTime, FrameTimes(i) );
			AvgFrameTime += FrameTimes(i);
		}
		AvgFrameTime /= FrameTimes.Num() - 10;

		// Output results to Benchmark/benchmark.log
		FString OutputString = TEXT("");
		appLoadFileToString( OutputString, *(appGameDir() + TEXT("Logs\\benchmark.log")) );
		OutputString += FString::Printf(TEXT("min= %6.2f   avg= %6.2f   max= %6.2f%s"), 1000.f / MaxFrameTime, 1000.f / AvgFrameTime, 1000.f / MinFrameTime, LINE_TERMINATOR );
		appSaveStringToFile( OutputString, *(appGameDir() + TEXT("Logs\\benchmark.log")) );

		FrameTimes.Empty();
	}

	// Make sure we're not in the middle of loading something.
	UObject::FlushAsyncLoading();
	// Block till all outstanding resource streaming requests are fulfilled.
	GStreamingManager->BlockTillAllRequestsFinished();
	
	if ( GEngine != NULL )
	{
		GEngine->PreExit();
	}
	appPreExit();
	DestroyGameRBPhys();

#if WITH_FACEFX
	// Make sure FaceFX is shutdown.
	UnShutdownFaceFX();
#endif // WITH_FACEFX

	// Stop the rendering thread.
	StopRenderingThread();

	delete GStreamingManager;
	GStreamingManager	= NULL;

	delete GIOManager;
	GIOManager			= NULL;

	// shutdown debug communications
	delete GDebugChannel;
	GDebugChannel = NULL;

	GThreadFactory->Destroy( AsyncIOThread );

#if STATS
	// Shutdown stats
	GStatManager.Destroy();
#endif
	// Shutdown sockets layer
	appSocketExit();
}

void FEngineLoop::Tick()
{
	if (GHandleDirtyDiscError)
	{
		appSleepInfinite();
	}

	// Flush debug output which has been buffered by other threads.
	GLog->FlushThreadedLogs();

#if !CONSOLE
	// If the local shader cache has changed, save it.
	static UBOOL bIsFirstTick = TRUE;
	if ((!GIsEditor && !UObject::IsAsyncLoading()) || bIsFirstTick)
	{
		SaveLocalShaderCaches();
		bIsFirstTick = FALSE;
	}
#endif

#if TRACK_DECOMPRESS_TIME
    extern DOUBLE GTotalUncompressTime;
    static DOUBLE LastUncompressTime = 0.0;
    if (LastUncompressTime != GTotalUncompressTime)
    {
	    debugf(TEXT("Total decompression time for load was %.3f seconds"), GTotalUncompressTime);
	    LastUncompressTime = GTotalUncompressTime;
    }
#endif

	if( GDebugger )
	{
		GDebugger->NotifyBeginTick();
	}

	// Exit if frame limit is reached in benchmark mode.
	if( (GIsBenchmarking && MaxFrameCounter && (GFrameCounter > MaxFrameCounter))
	// or timelimt is reached if set.
	||	(MaxTickTime && (TotalTickTime > MaxTickTime)) )
	{
		GEngine->DumpFPSChart();
		appRequestExit(0);
	}

	// Set GCurrentTime, GDeltaTime and potentially wait to enforce max tick rate.
	appUpdateTimaAndHandleMaxTickRate();

	ENQUEUE_UNIQUE_RENDER_COMMAND(
		ResetDeferredUpdates,
	{
		FDeferredUpdateResource::ResetNeedsUpdate();
	});

	// Update.
	GEngine->Tick( GDeltaTime );

	// Update RHI.
	RHITick( GDeltaTime );

	// Increment global frame counter. Once for each engine tick.
	GFrameCounter++;

	// Disregard first few ticks for total tick time as it includes loading and such.
	if( GFrameCounter > 5 )
	{
		TotalTickTime+=GDeltaTime;
	}

	// Find the objects which need to be cleaned up the next frame.
	FPendingCleanupObjects* PreviousPendingCleanupObjects = PendingCleanupObjects;
	PendingCleanupObjects = GetPendingCleanupObjects();

	// Keep track of this frame's location in the rendering command queue.
	PendingFrameFence.BeginFence();

	// Give the rendering thread time to catch up if it's more than one frame behind.
	PendingFrameFence.Wait( 1 );

	// Delete the objects which were enqueued for deferred cleanup before the previous frame.
	delete PreviousPendingCleanupObjects;

#if !CONSOLE && !PLATFORM_UNIX
	// Handle all incoming messages if we're not using wxWindows in which case this is done by their
	// message loop.
	if( !GUsewxWindows )
	{
		MSG Msg;
		while( PeekMessageX(&Msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage( &Msg );
			DispatchMessageX( &Msg );
		}
	}

	
	// check to see if the window in the foreground was made by this process (ie, does this app
	// have focus)
	DWORD ForegroundProcess;
	GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundProcess);
	UBOOL HasFocus = ForegroundProcess == GetCurrentProcessId();

	// If editor thread doesn't have the focus, don't suck up too much CPU time.
	if( GIsEditor )
	{
		static UBOOL HadFocus=1;
		if( HadFocus && !HasFocus )
		{
			// Drop our priority to speed up whatever is in the foreground.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		}
		else if( HasFocus && !HadFocus )
		{
			// Boost our priority back to normal.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
		}
		if( !HasFocus )
		{
			// Sleep for a bit to not eat up all CPU time.
			appSleep(0.005f);
		}
		HadFocus = HasFocus;
	}

	// if its our window, allow sound, otherwise silence it
	GALGlobalVolumeMultiplier = HasFocus ? 1.0f : 0.0f;

#endif

	if( GIsBenchmarking )
	{
#if STATS
		FrameTimes.AddItem( GFPSCounter.GetFrameTime() * 1000.f );
#endif
	}

#if XBOX
	// Handle remote debugging commands.
	{
		FScopeLock ScopeLock(&RemoteDebugCriticalSection);
		if( RemoteDebugCommand[0] != '\0' )
		{
			new(GEngine->DeferredCommands) FString(ANSI_TO_TCHAR(RemoteDebugCommand));
		}
		RemoteDebugCommand[0] = '\0';
	}
#endif

	if ( GDebugChannel )
	{
		GDebugChannel->Tick();
	}

	// Execute deferred commands.
	for( INT DeferredCommandsIndex=0; DeferredCommandsIndex<GEngine->DeferredCommands.Num(); DeferredCommandsIndex++ )
	{
		// Use LocalPlayer if available...
		if( GEngine->GamePlayers.Num() && GEngine->GamePlayers(0) )
		{
			ULocalPlayer* Player = GEngine->GamePlayers(0);
			{
				Player->Exec( *GEngine->DeferredCommands(DeferredCommandsIndex), *GLog );
			}
		}
		// and fall back to UEngine otherwise.
		else
		{
			GEngine->Exec( *GEngine->DeferredCommands(DeferredCommandsIndex), *GLog );
		}
		
	}
	GEngine->DeferredCommands.Empty();
}


/*-----------------------------------------------------------------------------
	Static linking support.
-----------------------------------------------------------------------------*/

/**
 * Initializes all registrants and names for static linking support.
 */
void InitializeRegistrantsAndRegisterNames()
{
	// Static linking.
	for( INT k=0; k<ARRAY_COUNT(GNativeLookupFuncs); k++ )
	{
		GNativeLookupFuncs[k] = NULL;
	}
	INT Lookup = 0;
	// Auto-generated lookups and statics
	AutoInitializeRegistrantsCore( Lookup );
	AutoInitializeRegistrantsEngine( Lookup );
	AutoInitializeRegistrantsGameFramework( Lookup );
	AutoInitializeRegistrantsUnrealScriptTest( Lookup );
	AutoInitializeRegistrantsIpDrv( Lookup );
#if defined(XBOX)
	AutoInitializeRegistrantsXeAudio( Lookup );
	AutoInitializeRegistrantsXeDrv( Lookup );
	AutoInitializeRegistrantsOnlineSubsystemLive( Lookup );
#elif PS3
	AutoInitializeRegistrantsPS3Drv( Lookup );
#elif PLATFORM_UNIX
	AutoInitializeRegistrantsLinuxDrv( Lookup );
#else
	AutoInitializeRegistrantsEditor( Lookup );
	AutoInitializeRegistrantsUnrealEd( Lookup );
	AutoInitializeRegistrantsALAudio( Lookup );
	AutoInitializeRegistrantsWinDrv( Lookup );
#endif
#if   GAMENAME == WARGAME
	AutoInitializeRegistrantsWarfareGame( Lookup );
#if !CONSOLE
	AutoInitializeRegistrantsWarfareEditor( Lookup );

	AutoInitializeRegistrantsOnlineSubsystemLive( Lookup );
	AutoInitializeRegistrantsOnlineSubsystemPC( Lookup );

#elif PS3
	AutoInitializeRegistrantsOnlineSubsystemPS3( Lookup );
#endif
#if defined(XBOX)
	AutoInitializeRegistrantsVinceOnlineSubsystemLive( Lookup );
#endif
#elif GAMENAME == GEARGAME
	AutoInitializeRegistrantsGearGame( Lookup );
#if !CONSOLE
	AutoInitializeRegistrantsGearEditor( Lookup );
#endif
#elif GAMENAME == UTGAME
	AutoInitializeRegistrantsUTGame( Lookup );
#if !CONSOLE
	AutoInitializeRegistrantsUTEditor( Lookup );
#if WITH_GAMESPY
	AutoInitializeRegistrantsOnlineSubsystemGameSpy( Lookup );
#endif
#elif PS3
#if WITH_GAMESPY
	AutoInitializeRegistrantsOnlineSubsystemGameSpy( Lookup );
#endif
#endif

#elif GAMENAME == EXAMPLEGAME
	AutoInitializeRegistrantsExampleGame( Lookup );
#if !CONSOLE
	AutoInitializeRegistrantsExampleEditor( Lookup );
	AutoInitializeRegistrantsOnlineSubsystemPC( Lookup );
#endif
#else
	#error Hook up your game name here
#endif
	// It is safe to increase the array size of GNativeLookupFuncs if this assert gets triggered.
	check( Lookup < ARRAY_COUNT(GNativeLookupFuncs) );

//	AutoGenerateNames* declarations.
	AutoGenerateNamesCore();
	AutoGenerateNamesEngine();
	AutoGenerateNamesGameFramework();
	AutoGenerateNamesUnrealScriptTest();
#if !CONSOLE && !PLATFORM_UNIX
	AutoGenerateNamesEditor();
	AutoGenerateNamesUnrealEd();
#endif
	AutoGenerateNamesIpDrv();
#if XBOX
	AutoGenerateNamesOnlineSubsystemLive();
#endif
#if   GAMENAME == WARGAME
	AutoGenerateNamesWarfareGame();
#if !CONSOLE
	AutoGenerateNamesWarfareEditor();

	AutoGenerateNamesOnlineSubsystemLive();
	AutoGenerateNamesOnlineSubsystemPC();

#elif PS3
	AutoGenerateNamesOnlineSubsystemPS3();
#endif
#if XBOX
	AutoGenerateNamesVinceOnlineSubsystemLive();
#endif
#elif GAMENAME == GEARGAME
	AutoGenerateNamesGearGame();
#if !CONSOLE
	AutoGenerateNamesGearEditor();
#endif
#elif GAMENAME == UTGAME
	AutoGenerateNamesUTGame();
#if !CONSOLE
	AutoGenerateNamesUTEditor();
#if WITH_GAMESPY
	AutoGenerateNamesOnlineSubsystemGameSpy();
#endif
#elif PS3
#if WITH_GAMESPY
	AutoGenerateNamesOnlineSubsystemGameSpy();
#endif
#endif

#elif GAMENAME == EXAMPLEGAME
	AutoGenerateNamesExampleGame();
#if !CONSOLE
	AutoGenerateNamesExampleEditor();
	AutoGenerateNamesOnlineSubsystemPC();
#endif
#else
	#error Hook up your game name here
#endif
}

/*-----------------------------------------------------------------------------
	Remote debug channel support.
-----------------------------------------------------------------------------*/

#if (defined XBOX) && ALLOW_NON_APPROVED_FOR_SHIPPING_LIB

static int dbgstrlen( const CHAR* str )
{
    const CHAR* strEnd = str;
    while( *strEnd )
        strEnd++;
    return strEnd - str;
}
static inline CHAR dbgtolower( CHAR ch )
{
    if( ch >= 'A' && ch <= 'Z' )
        return ch - ( 'A' - 'a' );
    else
        return ch;
}
static INT dbgstrnicmp( const CHAR* str1, const CHAR* str2, int n )
{
    while( n > 0 )
    {
        if( dbgtolower( *str1 ) != dbgtolower( *str2 ) )
            return *str1 - *str2;
        --n;
        ++str1;
        ++str2;
    }
    return 0;
}
static void dbgstrcpy( CHAR* strDest, const CHAR* strSrc )
{
    while( ( *strDest++ = *strSrc++ ) != 0 );
}

HRESULT __stdcall DebugConsoleCmdProcessor( const CHAR* Command, CHAR* Response, DWORD ResponseLen, PDM_CMDCONT pdmcc )
{
	// Skip over the command prefix and the exclamation mark.
	Command += dbgstrlen("UNREAL") + 1;

	// Check if this is the initial connect signal
	if( dbgstrnicmp( Command, "__connect__", 11 ) == 0 )
	{
		// If so, respond that we're connected
		lstrcpynA( Response, "Connected.", ResponseLen );
		return XBDM_NOERR;
	}

	{
		FScopeLock ScopeLock(&RemoteDebugCriticalSection);
		if( RemoteDebugCommand[0] )
		{
			// This means the application has probably stopped polling for debug commands
			dbgstrcpy( Response, "Cannot execute - previous command still pending." );
		}
		else
		{
			dbgstrcpy( RemoteDebugCommand, Command );
		}
	}

	return XBDM_NOERR;
}

#endif

/** 
* Returns whether the line can be broken between these two characters
*/
UBOOL appCanBreakLineAt( TCHAR Previous, TCHAR Current )
{
#if (GAMENAME == WARGAME) && !PS3
	// This function is part of code from the Microsoft's July XDK and so cannot be used on the PS3
	extern bool WordWrap_CanBreakLineAt( WCHAR prev, WCHAR curr );

	return( WordWrap_CanBreakLineAt( Previous, Current ) );
#else
	return( appIsPunct( Current ) || appIsWhitespace( Current ) );
#endif
}

