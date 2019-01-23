/*=============================================================================
	UnGame.cpp: Unreal game engine.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineParticleClasses.h"
#include "EngineAudioDeviceClasses.h"
#include "UnNet.h"
#include "DemoRecording.h"
#include "RemoteControl.h"
#include "ChartCreation.h"

#if !CONSOLE && defined(_MSC_VER)
#include "..\Debugger\UnDebuggerCore.h"
#endif

IMPLEMENT_COMPARE_CONSTREF( FString, UnGame, { return appStricmp(*A,*B); } )

//
//	Object class implementation.
//

/*-----------------------------------------------------------------------------
	cleanup!!
-----------------------------------------------------------------------------*/

void UGameEngine::RedrawViewports()
{
	if ( GameViewport != NULL )
	{
		GameViewport->eventLayoutPlayers();
		if ( GameViewport->Viewport != NULL )
		{
			GameViewport->Viewport->Draw();
		}
	}
}

INT UGameEngine::ChallengeResponse( INT Challenge )
{
	return (Challenge*237) ^ (0x93fe92Ce) ^ (Challenge>>16) ^ (Challenge<<16);
}

void UGameEngine::UpdateConnectingMessage()
{
	if(GPendingLevel && GamePlayers.Num() && GamePlayers(0)->Actor)
	{
		if(GamePlayers(0)->Actor->ProgressTimeOut < GWorld->GetTimeSeconds())
		{
			TCHAR	Msg1[MAX_SPRINTF]=TEXT(""), 
					Msg2[MAX_SPRINTF]=TEXT("");

			appSprintf( Msg1, *LocalizeProgress(TEXT("ConnectingText"),TEXT("Engine")) );
			appSprintf( Msg2, *LocalizeProgress(TEXT("ConnectingURL"),TEXT("Engine")), *GPendingLevel->URL.Host, *GPendingLevel->URL.Map );
			SetProgress( Msg1, Msg2, 60.f );
		}
	}
}

/*-----------------------------------------------------------------------------
	Game init and exit.
-----------------------------------------------------------------------------*/

//
// Construct the game engine.
//
UGameEngine::UGameEngine()
: LastURL(TEXT(""))
{}

//
// Initialize the game engine.
//
void UGameEngine::Init()
{
	// Call base.
	UEngine::Init();

#if PS3
	// PS3 wants to look for DLC before loading initial map
	appPS3PreGameEngineInit();
#endif

	// Sanity checking.
	VERIFY_CLASS_OFFSET( A, Actor, Owner );
	VERIFY_CLASS_OFFSET( A, PlayerController,	ViewTarget );
	VERIFY_CLASS_OFFSET( A, Pawn, Health );
    VERIFY_CLASS_SIZE( UEngine );
    VERIFY_CLASS_SIZE( UGameEngine );
	
	VERIFY_CLASS_SIZE( UTexture );
	VERIFY_CLASS_OFFSET( U, Texture, UnpackMax );

	VERIFY_CLASS_SIZE( USkeletalMeshComponent );
	VERIFY_CLASS_OFFSET( U, SkeletalMeshComponent, SkeletalMesh );

	VERIFY_CLASS_SIZE( USkeletalMesh );
	VERIFY_CLASS_OFFSET( U, SkeletalMesh, RefBasesInvMatrix );

	VERIFY_CLASS_SIZE( UPlayer );
	VERIFY_CLASS_SIZE( ULocalPlayer );
	VERIFY_CLASS_SIZE( UAudioComponent );

	debugf(TEXT("Object size..............: %u"),sizeof(UObject));
	debugf(TEXT("Actor size...............: %u"),sizeof(AActor));
	debugf(TEXT("ActorComponent size......: %u"),sizeof(UActorComponent));
	debugf(TEXT("PrimitiveComponent size..: %u"),sizeof(UPrimitiveComponent));

	// Delete temporary files in cache.
	appCleanFileCache();

#if !CONSOLE && _MSC_VER
	if( ParseParam(appCmdLine(),TEXT("AUTODEBUG")) )
	{
		debugf(TEXT("Attaching to Debugger"));
		UDebuggerCore* Debugger = new UDebuggerCore();
		GDebugger 				= Debugger;

		// we want the UDebugger to break on the first bytecode it processes
		Debugger->SetBreakASAP(TRUE);
	}
#endif

	// If not a dedicated server.
	if( GIsClient )
	{	
		// Init client.
		UClass* ClientClass = StaticLoadClass( UClient::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.Client"), NULL, LOAD_None, NULL );
		Client = ConstructObject<UClient>( ClientClass );
		Client->Init( this );
	}


	// Initialize the viewport client.
	UGameViewportClient* ViewportClient = NULL;
	if(Client)
	{
		ViewportClient = ConstructObject<UGameViewportClient>(GameViewportClientClass,this);
		GameViewport = ViewportClient;
	}

	// Attach the viewport client to a new viewport.
	if(ViewportClient)
	{
		Parse(appCmdLine(), TEXT("ResX="), Client->StartupResolutionX);
		Parse(appCmdLine(), TEXT("ResY="), Client->StartupResolutionY);

		FViewportFrame* ViewportFrame = Client->CreateViewportFrame(
			ViewportClient,
			*LocalizeGeneral("Product",GPackage),
			Client->StartupResolutionX,
			Client->StartupResolutionY,
			Client->StartupFullscreen && !ParseParam(appCmdLine(),TEXT("WINDOWED"))
			);

		// Start up the networking layer before the UI and after
		// rendering has been set up (important!)
		InitOnlineSubsystem();

		FString Error;
		if(!ViewportClient->eventInit(Error))
		{
			appErrorf(TEXT("%s"),*Error);
		}

		GameViewport->SetViewport(ViewportFrame);
	}
	else
	{
#if WITH_PANORAMA
		extern void appPanoramaHookInit();
		// Initialize Panorama without rendering (dedicated server mode)
		appPanoramaHookInit();
#endif
		// Start up the networking layer that was specified
		InitOnlineSubsystem();
	}

	// Create default URL.
	// @note:  if we change how we determine the valid start up map update LaunchEngineLoop's GetStartupMap()
	FURL DefaultURL;
	DefaultURL.LoadURLConfig( TEXT("DefaultPlayer"), GGameIni );

	// Enter initial world.
	FString Error;
	TCHAR Parm[4096]=TEXT("");
	const TCHAR* Tmp = appCmdLine();
	if(	!ParseToken( Tmp, Parm, ARRAY_COUNT(Parm), 0 ) || Parm[0]=='-' )
	{
		appStrcpy( Parm, *FURL::DefaultLocalMap );
	}
	FURL URL( &DefaultURL, Parm, TRAVEL_Partial );
	if( !URL.Valid )
	{
		appErrorf( *LocalizeError(TEXT("InvalidUrl"),TEXT("Engine")), Parm );
	}
	UBOOL Success = Browse( URL, Error );

	// If waiting for a network connection, go into the starting level.
	if (!Success && appStricmp(Parm, *FURL::DefaultLocalMap) != 0)
	{
		// the map specified on the command-line couldn't be loaded.  ask the user if we should load the default map instead
		if ( appStricmp( *URL.Map, *FURL::DefaultLocalMap )!=0 &&
			appMsgf(AMT_OKCancel, *LocalizeError(TEXT("FailedMapload_NotFound"),TEXT("Engine")), *URL.Map) == 0 )
		{
			// user cancelled (maybe a typo while attempting to run a commandlet)
			appRequestExit( FALSE );
			return;
		}
		else
		{
			Success = Browse( FURL(&DefaultURL,*FURL::DefaultLocalMap,TRAVEL_Partial), Error );
		}
	}

	// Handle failure.
	if( !Success )
	{
		appErrorf( *LocalizeError(TEXT("FailedBrowse"),TEXT("Engine")), Parm, *Error );
	}

#if USING_REMOTECONTROL
	extern UBOOL GUsewxWindows;
	if( GUsewxWindows && RemoteControlExec && !GIsEditor )
	{
		// Suppress remote control for dedicated servers.
		if ( Client )
		{
			UBOOL bSuppressRemoteControl = FALSE;
			GConfig->GetBool( TEXT("RemoteControl"), TEXT("SuppressRemoteControlAtStartup"), bSuppressRemoteControl, GEngineIni );
			if ( ParseParam(appCmdLine(), TEXT("NORC")) || ParseParam(appCmdLine(), TEXT("NOREMOTECONTROL")) )
			{
				bSuppressRemoteControl = TRUE;
			}
			if ( !bSuppressRemoteControl )
			{
				RemoteControlExec->Show( TRUE );
				RemoteControlExec->SetFocusToGame();
			}
		}
	}
#endif

	debugf( NAME_Init, TEXT("Game engine initialized") );
}

//
// Game exit.
//
void UGameEngine::PreExit()
{
	Super::PreExit();

	if( GPendingLevel )
	{
		CancelPending();
	}

	// Clean up world.
	if ( GWorld != NULL )
	{
		// notify the gameinfo
		AGameInfo *Info = GWorld->GetGameInfo();
		if (Info != NULL)
		{
			Info->eventPreExit();
		}
		GWorld->FlushLevelStreaming( NULL, TRUE );
		GWorld->TermWorldRBPhys();
		GWorld->CleanupWorld();
	}
}

void UGameEngine::FinishDestroy()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Game exit.
		debugf( NAME_Exit, TEXT("Game engine shut down") );
	}

	Super::FinishDestroy();
}

//
// Progress text.
//
void UGameEngine::SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds )
{
	if( GamePlayers.Num() && GamePlayers(0)->Actor )
	{
		GamePlayers(0)->Actor->eventSetProgressMessage(0, Str1, FColor(255,255,255));
		GamePlayers(0)->Actor->eventSetProgressMessage(1, Str2, FColor(255,255,255));
		GamePlayers(0)->Actor->eventSetProgressTime(Seconds);
	}
}

/*-----------------------------------------------------------------------------
	Command line executor.
-----------------------------------------------------------------------------*/

class ParticleSystemUsage
{
public:
	UParticleSystem* Template;
	INT	Count;
	INT	ActiveTotal;
	INT	MaxActiveTotal;
	// Reported whether the emitters are instanced or not...
	INT	StoredMaxActiveTotal;

	TArray<INT>		EmitterActiveTotal;
	TArray<INT>		EmitterMaxActiveTotal;
	// Reported whether the emitters are instanced or not...
	TArray<INT>		EmitterStoredMaxActiveTotal;

	ParticleSystemUsage() :
		Template(NULL),
		Count(0),
		ActiveTotal(0),
		MaxActiveTotal(0),
		StoredMaxActiveTotal(0)
	{
	}

	~ParticleSystemUsage()
	{
	}
};

IMPLEMENT_COMPARE_POINTER( UStaticMesh, UnGame, { return B->GetResourceSize() - A->GetResourceSize(); } )

//
// This always going to be the last exec handler in the chain. It
// handles passing the command to all other global handlers.
//
UBOOL UGameEngine::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	const TCHAR* Str=Cmd;
	if( ParseCommand( &Str, TEXT("OPEN") ) )
	{
		// make sure the file exists if we are opening a local file
		FURL TestURL(&LastURL, Str, TRAVEL_Partial);
		if (TestURL.IsLocalInternal())
		{
			FString PackageFilename;
			if (!GPackageFileCache->FindPackageFile(*TestURL.Map, NULL, PackageFilename))
			{
				Ar.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
				return TRUE;
			}
		}
		SetClientTravel( Str, TRAVEL_Partial );
		return TRUE;
	}
	else if( ParseCommand( &Str, TEXT("STREAMMAP")) )
	{
		// make sure the file exists if we are opening a local file
		FURL TestURL(&LastURL, Str, TRAVEL_Partial);
		if (TestURL.IsLocalInternal())
		{
			FString PackageFilename;
			if (GPackageFileCache->FindPackageFile(*TestURL.Map, NULL, PackageFilename))
			{
				TArray<FName> LevelNames;
				LevelNames.AddItem(*TestURL.Map);
				PrepareMapChange(LevelNames);

				bShouldCommitPendingMapChange				= TRUE;
				bShouldSkipLevelStartupEventOnMapCommit		= FALSE;
				bShouldSkipLevelBeginningEventOnMapCommit	= FALSE;
				ConditionalCommitMapChange();
			}
			else
			{
				Ar.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
			}
		}
		else
		{
			Ar.Logf(TEXT("ERROR: Can only perform streaming load for local URLs."));
		}
		return TRUE;
	}
	else if( ParseCommand( &Str, TEXT("START") ) )
	{
		// make sure the file exists if we are opening a local file
		FURL TestURL(&LastURL, Str, TRAVEL_Absolute);
		if (TestURL.IsLocalInternal())
		{
			FString PackageFilename;
			if (!GPackageFileCache->FindPackageFile(*TestURL.Map, NULL, PackageFilename))
			{
				Ar.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
				return TRUE;
			}
		}

		SetClientTravel( Str, TRAVEL_Absolute );
		return TRUE;
	}
	else if (ParseCommand(&Str, TEXT("SERVERTRAVEL")) && GWorld->IsServer())
	{
		GWorld->GetWorldInfo()->eventServerTravel(Str);
		return TRUE;
	}
	else if( (GIsServer && !GIsClient) && ParseCommand( &Str, TEXT("SAY") ) )
	{
		GWorld->GetWorldInfo()->Game->eventBroadcast(NULL, Str, NAME_None);
		return TRUE;
	}
	else if( ParseCommand(&Str, TEXT("DISCONNECT")) )
	{
		// Remove ?Listen parameter, if it exists
		LastURL.RemoveOption( TEXT("Listen") );
		LastURL.RemoveOption( TEXT("LAN") ) ;

		if( GWorld->NetDriver && GWorld->NetDriver->ServerConnection && GWorld->NetDriver->ServerConnection->Channels[0] )
		{
			GWorld->NetDriver->ServerConnection->Channels[0]->Close();
			GWorld->NetDriver->ServerConnection->FlushNet();
		}
		if( GPendingLevel && GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GPendingLevel->NetDriver->ServerConnection->FlushNet();
		}
		SetClientTravel( TEXT("?closed"), TRAVEL_Absolute );

		return TRUE;
	}
	else if( ParseCommand(&Str, TEXT("RECONNECT")) )
	{
		if (LastRemoteURL.Valid && LastRemoteURL.Host != TEXT(""))
		{
			SetClientTravel(*LastRemoteURL.String(), TRAVEL_Absolute);
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("EXIT")) || ParseCommand(&Cmd,TEXT("QUIT")))
	{
		// Dump FPS chart before exiting.
		DumpFPSChart();
		// Dump distance factor chart before exiting.
		DumpDistanceFactorChart();
		// Dump Memory chart before exiting.
		DumpMemoryChart();

		if( GWorld->GetWorldInfo() && GWorld->GetWorldInfo()->Game )
		{
			GWorld->GetWorldInfo()->Game->eventGameEnding();
		}

		if( GWorld->NetDriver && GWorld->NetDriver->ServerConnection && GWorld->NetDriver->ServerConnection->Channels[0] )
		{
			GWorld->NetDriver->ServerConnection->Channels[0]->Close();
			GWorld->NetDriver->ServerConnection->FlushNet();
		}
		if( GPendingLevel && GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GPendingLevel->NetDriver->ServerConnection->FlushNet();
		}

		Ar.Log( TEXT("Closing by request") );
		appRequestExit( 0 );
		return TRUE;
	}
	else if( ParseCommand( &Str, TEXT("GETMAXTICKRATE") ) )
	{
		Ar.Logf( TEXT("%f"), GetMaxTickRate(0,FALSE) );
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("CANCEL") ) )
	{
		static UBOOL InCancel = 0;
		if( !InCancel )	
		{
			//!!Hack for broken Input subsystem.  JP.
			//!!Inside LoadMap(), ResetInput() is called,
			//!!which can retrigger an Exec call.
			InCancel = 1;
			if( GPendingLevel )
			{
				if( GPendingLevel->TrySkipFile() )
				{
					InCancel = 0;
					return TRUE;
				}
				SetProgress( *LocalizeProgress(TEXT("CancelledConnect"),TEXT("Engine")), TEXT(""), 2.f );
			}
			else
				SetProgress( TEXT(""), TEXT(""), 0.f );
			CancelPending();
			InCancel = 0;
		}
		return TRUE;
	}
	// This will list out the packages which are in the precache list and have not been "loaded" out.  (e.g. could be just there taking up memory!)
	else if( ParseCommand( &Cmd, TEXT("ListPrecacheMapPackages") ) )
	{
		TArray<FString> Packages;
		ULinkerLoad::GetListOfPackagesInPackagePrecacheMap( Packages );

		Sort<USE_COMPARE_CONSTREF(FString,UnGame)>( &Packages(0), Packages.Num() );

		debugf( TEXT( "Total Number Of Packages In PrecacheMap: %i " ), Packages.Num() );

		for( INT i = 0; i < Packages.Num(); ++i )
		{
			debugf( TEXT( "%i %s" ), i, *Packages(i) );
		}
		debugf( TEXT( "Total Number Of Packages In PrecacheMap: %i " ), Packages.Num() );

		return TRUE;

	}
	// we can't always do an obj linkers, as cooked games have their linkers tossed out.  So we need to look at the actual packages which are loaded
	else if( ParseCommand( &Cmd, TEXT("ListLoadedPackages") ) )
	{
		TArray<FString> Packages;

		for( TObjectIterator<UPackage> It; It; ++It )
		{
			UPackage* Package = *It;

			const UBOOL bIsARootPackage = Package->GetOuter() == NULL;

			if( bIsARootPackage == TRUE )
			{
				Packages.AddItem( Package->GetFullName() );
				//warnf( TEXT("Package %s"), *Package->GetFullName() );
			}
		}

		Sort<USE_COMPARE_CONSTREF(FString,UnGame)>( &Packages(0), Packages.Num() );

		debugf( TEXT( "Total Number Of Packages Loaded: %i " ), Packages.Num() );

		for( INT i = 0; i < Packages.Num(); ++i )
		{
			debugf( TEXT( "%i %s" ), i, *Packages(i) );
		}
		debugf( TEXT( "Total Number Of Packages Loaded: %i " ), Packages.Num() );

		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("MemReport") ) )
	{
#if !FINAL_RELEASE

		Exec( TEXT( "MEM DETAILED" ) );

		Exec( TEXT( "MEMORYSPLIT ACTUAL" ) );

		FMemoryChartEntry NewMemoryEntry = FMemoryChartEntry();
		NewMemoryEntry.UpdateMemoryChartStats();
		debugf( *NewMemoryEntry.ToString() );


		Exec( TEXT( "OBJ LIST -ALPHASORT" ) );
		Exec( TEXT( "LISTTEXTURES" ) );
		Exec( TEXT( "LISTSOUNDS" ) );

		Exec( TEXT( "ListLoadedPackages" ) );
		Exec( TEXT( "ListPrecacheMapPackages" ) );

#endif // !FINAL_RELEASE

		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("MEMORYSPLIT") ) )
	{
#if !FINAL_RELEASE
		// if this is set, this will be closer to what's actually allocated
		UBOOL bShowActualAllocated = (ParseToken(Cmd, 0) == TEXT("ACTUAL"));

		//  Gather various object resource sizes.
		INT		LevelDataSize			= 0;
		INT		BSPDataSize				= 0;
		INT		ParticlePeakSize		= 0;
		INT		ParticleActiveSize		= 0;

		// get a list of generic classes to track 
		TMultiMap<FString, FString>* ClassesToTrack;
		ClassesToTrack = GConfig->GetSectionPrivate(TEXT("MemorySplitClassesToTrack"), 0, 1, GEngineIni);

		// set up a map to store usage, with 0 initial mem used
		TMap<UClass*, INT> MemUsageByClass;
		if (ClassesToTrack)
		{
			for (TMultiMap<FString, FString>::TIterator It(*ClassesToTrack); It; ++It)
			{
				// find class by name
				UClass* Class = FindObject<UClass>(ANY_PACKAGE, *It.Value());

				if (Class)
				{
					MemUsageByClass.Set(Class, 0);
				}
			}
		}

		for( TObjectIterator<UObject> It; It; ++It )
		{
			UObject* Object = *It;

			INT ObjectSize = Object->GetResourceSize();
			if( ObjectSize == 0 )
			{
				FArchiveCountMem CountBytesSize( Object );
				// if we want accurate results, count the max (actual amount allocated)
				ObjectSize = bShowActualAllocated ? CountBytesSize.GetMax() : CountBytesSize.GetNum();
			}

			UBOOL bMemoryWasAccountedFor = FALSE;

			// look in the generic class tracking map for the exact class
			INT* MemUsage = MemUsageByClass.Find(Object->GetClass());

			// if not found, look for subclasses with fake IsA
			if (MemUsage == NULL)
			{
				for (TMap<UClass*, INT>::TIterator It(MemUsageByClass); It; ++It)
				{
					if (Object->IsA(It.Key()))
					{
						MemUsage = &It.Value();

						break;
					}
				}
			}

			// if we found the class in the map, update it's memory
			if (MemUsage)
			{
				*MemUsage += ObjectSize;

				bMemoryWasAccountedFor = TRUE;
			}

			// don't count the memory twice if we already counted it and we want
			// to show as accurate results as possibl
			if (bMemoryWasAccountedFor && bShowActualAllocated)
			{
				continue;
			}


			// handle objects that may be counted in two sections
			if(	Object->IsA(UModelComponent::StaticClass())
			||	Object->IsA(UBrushComponent::StaticClass()) )
			{
				LevelDataSize += ObjectSize;

				// don't count the memory twice if we want to be accurate
				if (!bShowActualAllocated)
				{
					BSPDataSize += ObjectSize;
				}
			}


			if( Object->IsA(ULevel::StaticClass())
			||	Object->IsA(UStaticMeshComponent::StaticClass())
			||	Object->IsA(USkeletalMeshComponent::StaticClass()) 
			||	Object->IsA(UBrushComponent::StaticClass()) )
			{
				LevelDataSize += ObjectSize;
			}

			if( Object->IsA(UModel::StaticClass()) )
			{
				BSPDataSize += ObjectSize;
			}

			if( Object->IsA(UParticleSystemComponent::StaticClass()) )
			{
				FArchiveCountMem CountBytesSize( Object);
				ParticlePeakSize    += CountBytesSize.GetMax();
				ParticleActiveSize += CountBytesSize.GetNum();
			}
		}

		// Gather audio stats.
		INT		SoundDataSize			= 0;

		// Gather texture memory stats.
		INT		TexturePoolAllocSize	= 0;
		INT		TexturePoolAvailSize	= 0;
		RHIGetTextureMemoryStats( TexturePoolAllocSize, TexturePoolAvailSize );

		// Gather malloc stats.
		SIZE_T	VirtualMemorySize		= 0;
		SIZE_T	PhysicalMemorySize		= 0;
		GMalloc->GetAllocationInfo( VirtualMemorySize, PhysicalMemorySize );

		debugf( TEXT("Memory split (in KByte)") );
		debugf( TEXT("  BSP                 : %6i"), BSPDataSize	/ 1024 );
		debugf( TEXT("  Level               : %6i"), LevelDataSize / 1024 );
		debugf( TEXT("  Particles (Active)  : %6i"), ParticleActiveSize / 1024 );
		debugf( TEXT("  Particles (Peak)    : %6i"), ParticlePeakSize / 1024 );
		
		for (TMap<UClass*, INT>::TIterator It(MemUsageByClass); It; ++It)
		{
#if PS3 && UNICODE
			debugf(TEXT("  %20ls: %6i"), *It.Key()->GetName(), It.Value() / 1024);
#else
			debugf(TEXT("  %20s: %6i"), *It.Key()->GetName(), It.Value() / 1024);
#endif
		}
#endif
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("PARTICLEMESHUSAGE") ) )
	{
		// Mapping from static mesh to particle systems using it.
		TMultiMap<UStaticMesh*,UParticleSystem*> StaticMeshToParticleSystemMap;
		// Unique array of referenced static meshes, used for sorting and index into map.
		TArray<UStaticMesh*> UniqueReferencedMeshes;

		// Iterate over all mesh modules to find and keep track of mesh to system mappings.
		for( TObjectIterator<UParticleModuleTypeDataMesh> It; It; ++It )
		{
			UStaticMesh* StaticMesh = It->Mesh;
			if( StaticMesh )
			{
				// Find particle system in outer chain.
				UParticleSystem* ParticleSystem = NULL;
				UObject* Outer = It->GetOuter();
				while( Outer && !ParticleSystem )
				{
					ParticleSystem = Cast<UParticleSystem>(Outer);
					Outer = Outer->GetOuter();
				}

				// Add unique mapping from static mesh to particle system.
				if( ParticleSystem )
				{
					StaticMeshToParticleSystemMap.AddUnique( StaticMesh, ParticleSystem );
					UniqueReferencedMeshes.AddUniqueItem( StaticMesh );
				}
			}
		}

		// Sort by resource size.
		Sort<USE_COMPARE_POINTER(UStaticMesh,UnGame)>( UniqueReferencedMeshes.GetTypedData(), UniqueReferencedMeshes.Num() );
		
		// Calculate total size for summary.
		INT TotalSize = 0;
		for( INT StaticMeshIndex=0; StaticMeshIndex<UniqueReferencedMeshes.Num(); StaticMeshIndex++ )
		{
			UStaticMesh* StaticMesh	= UniqueReferencedMeshes(StaticMeshIndex);
			TotalSize += StaticMesh->GetResourceSize();
		}		

		// Log sorted summary.
		debugf(TEXT("%5i KByte of static meshes referenced by particle systems:"),TotalSize / 1024);
		for( INT StaticMeshIndex=0; StaticMeshIndex<UniqueReferencedMeshes.Num(); StaticMeshIndex++ )
		{
			UStaticMesh* StaticMesh	= UniqueReferencedMeshes(StaticMeshIndex);
			
			// Find all particle systems using this static mesh.
			TArray<UParticleSystem*> ParticleSystems;
			StaticMeshToParticleSystemMap.MultiFind( StaticMesh, ParticleSystems );

			// Log meshes including resource size and referencing particle systems.
			debugf(TEXT("%5i KByte  %s"), StaticMesh->GetResourceSize() / 1024, *StaticMesh->GetFullName());
			for( INT ParticleSystemIndex=0; ParticleSystemIndex<ParticleSystems.Num(); ParticleSystemIndex++ )
			{
				UParticleSystem* ParticleSystem = ParticleSystems(ParticleSystemIndex);
				debugf(TEXT("             %s"),*ParticleSystem->GetFullName());
			}
		}
		
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("PARTICLEMEMORY") ) )
	{
		//  Gather various object resource sizes.
		INT		PSysCompCount			= 0;
		INT		ParticlePeakSize		= 0;
		INT		ParticleActiveSize		= 0;
		INT		ParticleMaxPeakSize		= 0;	// The single PSysComp that consumes the most memory
		FString ParticleMaxPeakSizeComp;

		for( TObjectIterator<UParticleSystemComponent> It; It; ++It )
		{
			UParticleSystemComponent* CheckPSysComp = *It;
			PSysCompCount++;

			FArchiveCountMem CountBytesSize(CheckPSysComp);
			INT	CheckParticleMaxPeakSize = CountBytesSize.GetMax();
			if (CheckParticleMaxPeakSize > ParticleMaxPeakSize)
			{
				ParticleMaxPeakSize	= CheckParticleMaxPeakSize;
				if (CheckPSysComp)
				{
					if (CheckPSysComp->Template)
					{
                        ParticleMaxPeakSizeComp = CheckPSysComp->Template->GetPathName();
					}
					else
					{
						ParticleMaxPeakSizeComp = FString(TEXT("No Template!"));
					}
				}
				else
				{
					ParticleMaxPeakSizeComp = FString(TEXT("No Component!"));
				}
			}
			ParticlePeakSize    += CheckParticleMaxPeakSize;
			ParticleActiveSize += CountBytesSize.GetNum();
		}

		// Gather malloc stats.
		debugf( TEXT("PSysComp Count     : %6i"), PSysCompCount );
		debugf( TEXT("Particles (Active) : %6i"), ParticleActiveSize / 1024 );
		debugf( TEXT("Particles (Peak)   : %6i"), ParticlePeakSize / 1024 );
		debugf( TEXT("Particles (Single) : %6i"), ParticleMaxPeakSize / 1024 );
		debugf( TEXT("    PSys           : %s"), *ParticleMaxPeakSizeComp );

		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("DUMPPARTICLECOUNTS") ) )
	{
		TMap<UParticleSystem*, ParticleSystemUsage> UsageMap;

		UBOOL bTrackUsage = ParseCommand(&Cmd, TEXT("USAGE"));
		UBOOL bTrackUsageOnly = ParseCommand(&Cmd, TEXT("USAGEONLY"));
		for( TObjectIterator<UObject> It; It; ++It )
		{
			UParticleSystemComponent* PSysComp = Cast<UParticleSystemComponent>(*It);
			if (PSysComp)
			{
				ParticleSystemUsage* Usage = NULL;

				if (bTrackUsageOnly == FALSE)
				{
					debugfSuppressed(NAME_DevParticle, TEXT("ParticleSystemComponent %s"), *(PSysComp->GetName()));
				}

				UParticleSystem* PSysTemplate = PSysComp->Template;
				if (PSysTemplate != NULL)
				{
					if (bTrackUsage || bTrackUsageOnly)
					{
						ParticleSystemUsage* pUsage = UsageMap.Find(PSysTemplate);
						if (pUsage == NULL)
						{
							ParticleSystemUsage TempUsage;
							TempUsage.Template = PSysTemplate;
							TempUsage.Count = 1;

							UsageMap.Set(PSysTemplate, TempUsage);
							Usage = UsageMap.Find(PSysTemplate);
							check(Usage);
						}					
						else
						{
							Usage = pUsage;
							Usage->Count++;
						}
					}
					if (bTrackUsageOnly == FALSE)
					{
						debugfSuppressed(NAME_DevParticle, TEXT("\tTemplate         : %s"), *(PSysTemplate->GetPathName()));
					}
				}
				else
				{
					if (bTrackUsageOnly == FALSE)
					{
						debugfSuppressed(NAME_DevParticle, TEXT("\tTemplate         : %s"), TEXT("NULL"));
					}
				}
				
				// Dump each emitter
				INT TotalActiveCount = 0;
				if (bTrackUsageOnly == FALSE)
				{
					debugfSuppressed(NAME_DevParticle, TEXT("\tEmitterCount     : %d"), PSysComp->EmitterInstances.Num());
				}

				if (PSysComp->EmitterInstances.Num() > 0)
				{
					for (INT EmitterIndex = 0; EmitterIndex < PSysComp->EmitterInstances.Num(); EmitterIndex++)
					{
						FParticleEmitterInstance* EmitInst = PSysComp->EmitterInstances(EmitterIndex);
						if (EmitInst)
						{
							UParticleLODLevel* LODLevel = EmitInst->SpriteTemplate ? EmitInst->SpriteTemplate->LODLevels(0) : NULL;
							if (bTrackUsageOnly == FALSE)
							{
								debugfSuppressed(NAME_DevParticle, TEXT("\t\tEmitter %2d:\tActive = %4d\tMaxActive = %4d"), 
									EmitterIndex, EmitInst->ActiveParticles, EmitInst->MaxActiveParticles);
							}
							TotalActiveCount += EmitInst->MaxActiveParticles;
							if (bTrackUsage || bTrackUsageOnly)
							{
								check(Usage);
								Usage->ActiveTotal += EmitInst->ActiveParticles;
								Usage->MaxActiveTotal += EmitInst->MaxActiveParticles;
								Usage->StoredMaxActiveTotal += EmitInst->MaxActiveParticles;
								if (Usage->EmitterActiveTotal.Num() <= EmitterIndex)
								{
									INT CheckIndex;
									CheckIndex = Usage->EmitterActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
									CheckIndex = Usage->EmitterMaxActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
				                    CheckIndex = Usage->EmitterStoredMaxActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
								}
								Usage->EmitterActiveTotal(EmitterIndex) = Usage->EmitterActiveTotal(EmitterIndex) + EmitInst->ActiveParticles;
								Usage->EmitterMaxActiveTotal(EmitterIndex) = Usage->EmitterMaxActiveTotal(EmitterIndex) + EmitInst->MaxActiveParticles;
			                    Usage->EmitterStoredMaxActiveTotal(EmitterIndex) = Usage->EmitterStoredMaxActiveTotal(EmitterIndex) + EmitInst->MaxActiveParticles;
							}
						}
						else
						{
							if (bTrackUsageOnly == FALSE)
							{
								debugfSuppressed(NAME_DevParticle, TEXT("\t\tEmitter %2d:\tActive = %4d\tMaxActive = %4d"), EmitterIndex, 0, 0);
							}
						}
					}
				}
				else
				if (PSysTemplate != NULL)
				{
					for (INT EmitterIndex = 0; EmitterIndex < PSysTemplate->Emitters.Num(); EmitterIndex++)
					{
						UParticleEmitter* Emitter = PSysTemplate->Emitters(EmitterIndex);
						if (Emitter)
						{
							INT MaxActive = 0;

							for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
							{
                                UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
								if (LODLevel)
								{
									if (LODLevel->PeakActiveParticles > MaxActive)
									{
										MaxActive = LODLevel->PeakActiveParticles;
									}
								}
							}

							if (bTrackUsage || bTrackUsageOnly)
							{
								check(Usage);
								Usage->StoredMaxActiveTotal += MaxActive;
								if (Usage->EmitterStoredMaxActiveTotal.Num() <= EmitterIndex)
								{
									INT CheckIndex;
									CheckIndex = Usage->EmitterActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
									CheckIndex = Usage->EmitterMaxActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
				                    CheckIndex = Usage->EmitterStoredMaxActiveTotal.AddZeroed(1);
									check(CheckIndex == EmitterIndex);
								}
								// Don't update the non-stored entries...
			                    Usage->EmitterStoredMaxActiveTotal(EmitterIndex) = Usage->EmitterStoredMaxActiveTotal(EmitterIndex) + MaxActive;
							}
						}
					}
				}
				if (bTrackUsageOnly == FALSE)
				{
					debugfSuppressed(NAME_DevParticle, TEXT("\tTotalActiveCount : %d"), TotalActiveCount);
				}
			}
		}

		if (bTrackUsage || bTrackUsageOnly)
		{
			debugfSuppressed(NAME_DevParticle, TEXT("PARTICLE USAGE DUMP:"));
			for (TMap<UParticleSystem*, ParticleSystemUsage>::TIterator It(UsageMap); It; ++It)
			{
				ParticleSystemUsage& Usage = It.Value();
				UParticleSystem* Template = Usage.Template;
				check(Template);

				debugfSuppressed(NAME_DevParticle, TEXT("\tParticleSystem..%s"), *(Usage.Template->GetPathName()));
				debugfSuppressed(NAME_DevParticle, TEXT("\t\tCount.....................%d"), Usage.Count);
				debugfSuppressed(NAME_DevParticle, TEXT("\t\tActiveTotal...............%5d"), Usage.ActiveTotal);
				debugfSuppressed(NAME_DevParticle, TEXT("\t\tMaxActiveTotal............%5d (%4d per instance)"), Usage.MaxActiveTotal, (Usage.MaxActiveTotal / Usage.Count));
				debugfSuppressed(NAME_DevParticle, TEXT("\t\tPotentialMaxActiveTotal...%5d (%4d per instance)"), Usage.StoredMaxActiveTotal, (Usage.StoredMaxActiveTotal / Usage.Count));
				debugfSuppressed(NAME_DevParticle, TEXT("\t\tEmitters..................%d"), Usage.EmitterActiveTotal.Num());
				check(Usage.EmitterActiveTotal.Num() == Usage.EmitterMaxActiveTotal.Num());
				for (INT EmitterIndex = 0; EmitterIndex < Usage.EmitterActiveTotal.Num(); EmitterIndex++)
				{
					INT EActiveTotal = Usage.EmitterActiveTotal(EmitterIndex);
					INT EMaxActiveTotal = Usage.EmitterMaxActiveTotal(EmitterIndex);
					INT EStoredMaxActiveTotal = Usage.EmitterStoredMaxActiveTotal(EmitterIndex);
					debugfSuppressed(NAME_DevParticle, TEXT("\t\t\tEmitter %2d - AT = %5d, MT = %5d (%4d per emitter), Potential MT = %5d (%4d per emitter)"),
						EmitterIndex, EActiveTotal,
						EMaxActiveTotal, (EMaxActiveTotal / Usage.Count),
						EStoredMaxActiveTotal, (EStoredMaxActiveTotal / Usage.Count)
						);
				}
			}
		}
		return TRUE;
	}
#if !CONSOLE && defined(_MSC_VER)
	else if( ParseCommand( &Cmd, TEXT("TOGGLEDEBUGGER") ) )
	{
		if( GDebugger )
		{
			delete GDebugger;
			GDebugger = NULL;
		}
		else
		{
			GDebugger = new UDebuggerCore();
		}
		return TRUE;
	}
#endif
	else if( GWorld->Exec( Cmd, Ar ) )
	{
		return TRUE;
	}
	else if( GWorld->GetGameInfo() && GWorld->GetGameInfo()->ScriptConsoleExec(Cmd,Ar,NULL) )
	{
		return TRUE;
	}
	// disallow set of actor properties if network game
	//@FIXME: need to set flag in standalone so can't join online games after set command
	else if ((ParseCommand(&Str, TEXT("SET")) || ParseCommand(&Str, TEXT("SETNOPEC"))) && GWorld->GetNetMode() != NM_Standalone)
	{
		return TRUE;
	}
	else if (UEngine::Exec(Cmd, Ar))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*-----------------------------------------------------------------------------
	Game entering.
-----------------------------------------------------------------------------*/
//
// Cancel pending level.
//
void UGameEngine::CancelPending()
{
	if( GPendingLevel )
	{
		if( GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GPendingLevel->NetDriver->ServerConnection->FlushNet();
		}
		GPendingLevel = NULL;
	}
}

//
// Browse to a specified URL, relative to the current one.
//
UBOOL UGameEngine::Browse( FURL URL, FString& Error )
{
	Error = TEXT("");
	const TCHAR* Option;

	// Convert .unreal link files.
	const TCHAR* LinkStr = TEXT(".unreal");//!!
	if( appStrstr(*URL.Map,LinkStr)-*URL.Map==appStrlen(*URL.Map)-appStrlen(LinkStr) )
	{
		debugf( TEXT("Link: %s"), *URL.Map );
		FString NewUrlString;
		if( GConfig->GetString( TEXT("Link")/*!!*/, TEXT("Server"), NewUrlString, *URL.Map ) )
		{
			// Go to link.
			URL = FURL( NULL, *NewUrlString, TRAVEL_Absolute );//!!
		}
		else
		{
			// Invalid link.
			Error = FString::Printf( *LocalizeError(TEXT("InvalidLink"),TEXT("Engine")), *URL.Map );
			return FALSE;
		}
	}

	// Crack the URL.
	debugf( TEXT("Browse: %s"), *URL.String() );

	// Handle it.
	if( !URL.Valid )
	{
		// Unknown URL.
		Error = FString::Printf( *LocalizeError(TEXT("InvalidUrl"),TEXT("Engine")), *URL.String() );
		return FALSE;
	}
	else if (URL.HasOption(TEXT("failed")) || URL.HasOption(TEXT("closed")))
	{
		// Handle failure URL.
		debugf( NAME_Log, *LocalizeError(TEXT("AbortToEntry"),TEXT("Engine")) );
		if (GWorld != NULL)
		{
			ResetLoaders( GWorld->GetOuter() );
		}
		LoadMap( FURL(&URL,*FURL::DefaultLocalMap,TRAVEL_Partial), NULL, Error );
		CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
		if( URL.HasOption(TEXT("failed")) )
		{
			if( !GPendingLevel )
			{
#if !CONSOLE
				SetProgress( *LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), TEXT(""), 6.f );
#endif
			}
		}
		// now remove "failed" and "closed" options from LastURL so it doesn't get copied on to future URLs
		LastURL.RemoveOption(TEXT("failed"));
		LastURL.RemoveOption(TEXT("closed"));
		return TRUE;
	}
	else if( URL.HasOption(TEXT("restart")) )
	{
		// Handle restarting.
		URL = LastURL;
	}
	else if( (Option=URL.GetOption(TEXT("load="),NULL))!=NULL )
	{
		// Handle loadgame.
		FString Error2, Temp=FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("Save%i.usa?load"), *GSys->SavePath, appAtoi(Option) );
		debugf(TEXT("Loading save game %s"),*Temp);
		if( LoadMap(FURL(&LastURL,*Temp,TRAVEL_Partial),NULL,Error2) )
		{
			LastURL = GWorld->URL;
			return TRUE;
		}
		else return FALSE;
	}

	// Handle normal URL's.
	if( URL.IsLocalInternal() )
	{
		// Local map file.
		return LoadMap( URL, NULL, Error );
	}
	else if( URL.IsInternal() && GIsClient )
	{
		// Force a demo stop on level change.
		if (GWorld && GWorld->DemoRecDriver != NULL)
		{
			GWorld->DemoRecDriver->Exec( TEXT("DEMOSTOP"), *GLog );
		}

		// Network URL.
		if( GPendingLevel )
			CancelPending();
		GPendingLevel = new UNetPendingLevel( URL );
		if( !GPendingLevel->NetDriver )
		{
			SetProgress( TEXT("Networking Failed"), *GPendingLevel->Error, 6.f );
			GPendingLevel = NULL;
		}
		return FALSE;
	}
	else if( URL.IsInternal() )
	{
		// Invalid.
		Error = LocalizeError(TEXT("ServerOpen"),TEXT("Engine"));
		return FALSE;
	}
	else
	{
		// External URL.
		//Client->Viewports(0)->Exec(TEXT("ENDFULLSCREEN"));
		appLaunchURL( *URL.String(), TEXT(""), &Error );
		return FALSE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MPSounds_PackageName		TEXT("MPSounds")
#define MPSounds_ReferencerName		MPSounds_PackageName TEXT("ObjRef")

static const INT MPSounds_ReferencerIndex = 0;
static const INT MPSounds_LocalizedReferencerIndex = 1;

/**
* Allocates global object referencers for MP sounds.
*/
static void AllocateObjectReferencersForMPSounds(UGameEngine* GameEngine)
{
	if ( GameEngine->ObjectReferencers.Num() < 2 )
	{
		GameEngine->ObjectReferencers.AddZeroed( 2 );
	}
}

/**
* Releases any existing handles to MP sounds.
*/
void FreeMPSounds(UEngine* InGameEngine)
{
	debugf( TEXT("Freeing MPSounds") );

	UGameEngine* GameEngine = Cast<UGameEngine>( InGameEngine );
	check( GameEngine );

	if ( GameEngine->ObjectReferencers.Num() > 0 )
	{
		GameEngine->ObjectReferencers(MPSounds_ReferencerIndex) = NULL;
		GameEngine->ObjectReferencers(MPSounds_LocalizedReferencerIndex) = NULL;
	}
}

/**
* Callback function for when the localized MP sounds package is loaded.
*
* @param	LocalizedMPSoundsPackage		The package that was loaded.
* @param	GameEngine						The GameEngine.
*/
static void AsyncLoadLocalizedMPSoundsCompletionCallback(UObject* LocalizedMPSoundsPackage, void* InGameEngine)
{
	UGameEngine* GameEngine = static_cast<UGameEngine*>( InGameEngine );
	check( GameEngine == GEngine );

	// Make sure object referencers for MP sounds have been allocated.
	AllocateObjectReferencersForMPSounds( GameEngine );

	// Release any previous object referencer.
	GameEngine->ObjectReferencers(MPSounds_LocalizedReferencerIndex) = NULL;

	if( LocalizedMPSoundsPackage )
	{	
		// Find the object referencer in the MP sounds package.
		const FString ObjectReferencerName( FString(MPSounds_ReferencerName) + TEXT("_") + UObject::GetLanguage() );
		UObjectReferencer* ObjectReferencer = FindObject<UObjectReferencer>( LocalizedMPSoundsPackage, *ObjectReferencerName );
		if( ObjectReferencer )
		{
			GameEngine->ObjectReferencers(MPSounds_LocalizedReferencerIndex) = ObjectReferencer;
		}
		else
		{
			debugf( NAME_Warning, TEXT("AsyncLoadLocalizedMPSoundsCompletionCallback: Couldn't find object referencer %s"), *ObjectReferencerName );
		}
	}
	else
	{
		debugf( NAME_Warning, TEXT("AsyncLoadLocalizedMPSoundsCompletionCallback: package load filed") );
	}
}

/**
* Callback function for when the MP sounds package is loaded.
*
* @param	MPSoundsPackage		The package that was loaded.
* @param	GameEngine			The GameEngine.
*/
static void AsyncLoadMPSoundsCompletionCallback(UObject* MPSoundsPackage, void* InGameEngine)
{
	UGameEngine* GameEngine = static_cast<UGameEngine*>( InGameEngine );
	check( GameEngine == GEngine );

	// Make sure object referencers for MP sounds have been allocated.
	AllocateObjectReferencersForMPSounds( GameEngine );

	// Release any previous object referencer.
	GameEngine->ObjectReferencers(MPSounds_ReferencerIndex) = NULL;

	if( MPSoundsPackage )
	{	
		// Find the object referencer in the MP sounds package.
		const FString ObjectReferencerName( MPSounds_ReferencerName );
		UObjectReferencer* ObjectReferencer = FindObject<UObjectReferencer>( MPSoundsPackage, *ObjectReferencerName );
		if( ObjectReferencer )
		{
			GameEngine->ObjectReferencers(MPSounds_ReferencerIndex) = ObjectReferencer;
		}
		else
		{
			debugf( NAME_Warning, TEXT("AsyncLoadMPSoundsCompletionCallback: Couldn't find object referencer %s"), *ObjectReferencerName );
		}
	}
	else
	{
		debugf( NAME_Warning, TEXT("AsyncLoadMPSoundsCompletionCallback: package load filed") );
	}
}

/**
* Loads the multiplayer sound packages.
*/
void LoadMPSounds(UEngine* GameEngine)
{
	// Release any existing handles to MP sounds.
	FreeMPSounds( GEngine );

	// Load localized MP sounds.
	const TCHAR* Language = UObject::GetLanguage();
	const FString LocalizedPreloadName( FString(MPSounds_PackageName) + LOCALIZED_SEEKFREE_SUFFIX + TEXT("_") + Language );

	FString LocalizedPreloadFilename;
	if( GPackageFileCache->FindPackageFile( *LocalizedPreloadName, NULL, LocalizedPreloadFilename ) )
	{
		debugf( TEXT("Issuing preload for %s"), *LocalizedPreloadFilename );
		UObject::LoadPackageAsync( LocalizedPreloadFilename, AsyncLoadLocalizedMPSoundsCompletionCallback, GameEngine );
	}
	else
	{
		debugf( TEXT("Couldn't find localized MP sound package %s"), *LocalizedPreloadName );
	}

	// Load non-localized MP sounds.
	const FString PreloadName( MPSounds_PackageName );
	FString PreloadFilename;
	if( GPackageFileCache->FindPackageFile( *PreloadName, NULL, PreloadFilename ) )
	{
		debugf( TEXT("Issuing preload for %s"), *PreloadFilename );
		UObject::LoadPackageAsync( PreloadFilename, AsyncLoadMPSoundsCompletionCallback, GameEngine );
	}
	else
	{
		debugf( TEXT("Couldn't find MP sound package %s"), *PreloadName );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern UBOOL GPrecacheNextFrame;

//
// Load a map.
//
UBOOL UGameEngine::LoadMap( const FURL& URL, UPendingLevel* Pending, FString& Error )
{
#if !FINAL_RELEASE
	// make sure level streaming isn't frozen
	if (GWorld)
	{
		GWorld->bIsLevelStreamingFrozen = FALSE;
	}
#endif

	// Force a demo stop on level change.
	if (GWorld && GWorld->DemoRecDriver != NULL)
	{
		GWorld->DemoRecDriver->Exec( TEXT("DEMOSTOP"), *GLog );
	}

	// send a callback message
	GCallbackEvent->Send(CALLBACK_PreLoadMap);

	// Dump and reset the FPS chart on map changes.
	DumpFPSChart();
	ResetFPSChart();

	DumpMemoryChart();
	ResetMemoryChart();

	// clean up any per-map loaded packages for the map we are leaving
	if (GWorld && GWorld->PersistentLevel)
	{
		CleanupPackagesToFullyLoad(FULLYLOAD_Map, GWorld->PersistentLevel->GetOutermost()->GetName());
	}

	// cleanup the existing per-game pacakges
	// @todo: It should be possible to not unload/load packages if we are going from/to the same gametype.
	//        would have to save the game pathname here and pass it in to SetGameInfo below
	CleanupPackagesToFullyLoad(FULLYLOAD_Game_PreLoadClass, TEXT(""));
	CleanupPackagesToFullyLoad(FULLYLOAD_Game_PostLoadClass, TEXT(""));


	// Cancel any pending async map changes.
	CancelPendingMapChange();
	GSeamlessTravelHandler.CancelTravel();

	DOUBLE	StartTime = appSeconds();

	Error = TEXT("");
	debugf( NAME_Log, TEXT("LoadMap: %s"), *URL.String() );
	GInitRunaway();

	// Get network package map.
	UPackageMap* PackageMap = NULL;
	if( Pending )
	{
		PackageMap = Pending->GetDriver()->ServerConnection->PackageMap;
	}

	if( GWorld )
	{
		// Display loading screen.
		if( !URL.HasOption(TEXT("quiet")) )
		{
			TransitionType = TT_Loading;
			TransitionDescription = URL.Map;
			RedrawViewports();
			TransitionType = TT_None;
		}

		// Clean up game state.
		GWorld->NetDriver = NULL;
		GWorld->FlushLevelStreaming( NULL, TRUE );
		GWorld->TermWorldRBPhys();
		GWorld->CleanupWorld();
		
		// send a message that all levels are going away (NULL means every sublevel is being removed
		// without a call to RemoveFromWorld for each)
		GCallbackEvent->Send(CALLBACK_LevelRemovedFromWorld, (UObject*)NULL);

		// Disassociate the players from their PlayerControllers.
		for(FPlayerIterator It(this);It;++It)
		{
			if(It->Actor)
			{
				if(It->Actor->Pawn)
				{
					GWorld->DestroyActor(It->Actor->Pawn, TRUE);
				}
				GWorld->DestroyActor(It->Actor, TRUE);
				It->Actor = NULL;
			}
			// clear post process volume so it doesn't prevent the world from being unloaded
			It->CurrentPPInfo.LastVolumeUsed = NULL;
			// reset split join info so we'll send one after loading the new map if necessary
			It->bSentSplitJoin = FALSE;
		}

		GWorld->RemoveFromRoot();
		GWorld = NULL;
	}

	// Stop all audio to remove references to current level.
	if( GEngine->Client && GEngine->Client->GetAudioDevice() )
	{
		GEngine->Client->GetAudioDevice()->Flush();
	}

	// Clean up the previous level out of memory.
	UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

#if !FINAL_RELEASE
	// There should be no UWorld instances at this point!
	for( TObjectIterator<UWorld> It; It; ++It )
	{
		UWorld* World = *It;
		// Print some debug information...
		debugf(TEXT("%s not cleaned up by garbage collection! "), *World->GetFullName());
		UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s.TheWorld"), *World->GetOutermost()->GetName()));
		TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( World, TRUE, GARBAGE_COLLECTION_KEEPFLAGS );
		FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, World );
		debugf(TEXT("%s"),*ErrorString);
		// before asserting.
		appErrorf( TEXT("%s not cleaned up by garbage collection!") LINE_TERMINATOR TEXT("%s") , *World->GetFullName(), *ErrorString );
	}
#endif

	if( GUseSeekFreeLoading )
	{
		UBOOL bSeparateSharedMPResources = FALSE;
		GConfig->GetBool( TEXT("Engine.Engine"), TEXT("bCookSeparateSharedMPResources"), bSeparateSharedMPResources, GEngineIni );
		if ( bSeparateSharedMPResources )
		{
			const UBOOL bIsMultiplayerMap = URL.Map.StartsWith(TEXT("MP_"));
			if ( bIsMultiplayerMap )
			{
				debugf( NAME_Log, TEXT("LoadMap: %s: issuing load request for shared MP resources"), *URL.String() );
				LoadMPSounds( GEngine );
			}
			else
			{
				debugf( NAME_Log, TEXT("LoadMap: %s: freeing any shared MP resources"), *URL.String() );
				FreeMPSounds( GEngine );
			}
		}

		// Load localized part of level first in case it exists.
		FString LocalizedMapPackageName	= URL.Map + LOCALIZED_SEEKFREE_SUFFIX;
		FString LocalizedMapFilename;
		if( GPackageFileCache->FindPackageFile( *LocalizedMapPackageName, NULL, LocalizedMapFilename ) )
		{
			LoadPackage( NULL, *LocalizedMapPackageName, LOAD_NoWarn );
		}
	}

	UPackage* MapOuter = NULL;
	// handle finding a downloaded map, as the USES handling code in UnPenLev.cpp won't actually load the pacakges
	// in the seekfree case, so the package won't be in memory
	if (Pending && GUseSeekFreeLoading && Pending && Pending->NetDriver && Pending->NetDriver->ServerConnection)
	{
		// cache map name
		FName MapName(*GPendingLevel->URL.Map);

		// first, look in the package map to get the guid for the map, so we can open by guid for package downloading
		for (INT PackageIndex = 0; PackageIndex < Pending->NetDriver->ServerConnection->PackageMap->List.Num(); PackageIndex++)
		{
			FPackageInfo& Info = Pending->NetDriver->ServerConnection->PackageMap->List(PackageIndex);

			// is this package entry the map?
			if (MapName == Info.PackageName)
			{
				// if we haven't loaded the map yet (seekfree loading, for instance), load it now
				// using the Guid, so we can open a downloaded map
				if (Info.Parent == NULL)
				{
					// make the package, and use this for the new linker (and to load the map from)
					MapOuter = CreatePackage(NULL, *GPendingLevel->URL.Map);

					// create the linker with the map name, and use the Guid so we find the downloaded version
					BeginLoad();
					GetPackageLinker(MapOuter, NULL, LOAD_NoWarn | LOAD_NoVerify | LOAD_Quiet, NULL, &Info.Guid);
					EndLoad();
				}

				// stop looking for the map
				break;
			}
		}
	}

	// Load level.
	UPackage* WorldPackage = LoadPackage(MapOuter, *URL.Map, LOAD_None);
	if( WorldPackage == NULL )
	{
		// it is now the responsibility of the caller to deal with a NULL return value and alert the user if necessary
		Error = FString::Printf(TEXT("Failed to load package '%s'"), *URL.Map);
		return FALSE;
	}

#if CONSOLE
	if( GUseSeekFreeLoading && !(WorldPackage->PackageFlags & PKG_DisallowLazyLoading) )
	{
		appErrorf(TEXT("Map '%s' has not been cooked correctly! Most likely stale version on the XDK."),*WorldPackage->GetName());
	}
#endif

	GWorld = FindObjectChecked<UWorld>( WorldPackage, TEXT("TheWorld") );
	GWorld->AddToRoot();
	GWorld->Init();
	
	// If pending network level.
	if( Pending )
	{
		// Setup network package info.
		if( GUseSeekFreeLoading )
		{
			// verify that we loaded everything we need here after loading packages required by the map itself to minimize extraneous loading/seeks
			// first, attempt to find or load any packages required, then try to find any we couldn't find the first time again (to catch forced exports inside packages loaded by the first loop)
			for (INT PackageIndex = 0; PackageIndex < PackageMap->List.Num(); PackageIndex++)
			{
				FPackageInfo& Info = PackageMap->List(PackageIndex);
				UPackage* Package = Info.Parent;

				if (Package == NULL)
				{
					// cache the package's name
					FString PackageName = Info.PackageName.ToString();

					// attempt to find it
					Package = FindPackage(NULL, *PackageName);
					if (Package == NULL)
					{
						// if that fails, load it
						// skip files server said had a base package as that means they are forced exports and we shouldn't attempt to load the "real" package file (if it even exists)
						//@todo: there might be problems with this for console clients connecting to PC servers
						if (Info.ForcedExportBasePackageName == NAME_None)
						{
							// check for localized package first as that may be required to load the base package
							FString LocalizedPackageName = PackageName + LOCALIZED_SEEKFREE_SUFFIX;
							FString LocalizedFileName;
							if (GPackageFileCache->FindPackageFile(*LocalizedPackageName, &Info.Guid, LocalizedFileName))
							{
								LoadPackage(NULL, *LocalizedPackageName, LOAD_None);
							}
							// load the base package
							Package = LoadPackage(NULL, *PackageName, LOAD_None);
						}

						if (Package == NULL && Info.ForcedExportBasePackageName != NAME_None)
						{
							// this package is a forced export inside another package, so try loading that other package
							FString BasePackageNameString(Info.ForcedExportBasePackageName.ToString());
							FString FileName;
							// don't check the Guid because we don't know the Guid of the base pacakge, and it will have
							// been already downloaded. This can break in the case of downloading partial pieces of a cooked package
							if (GPackageFileCache->FindPackageFile(*BasePackageNameString, NULL, FileName))
							{
								// find the guid of the localized base package if it exists
								INT BaseLocPackageIndex = -1;
								FName BaseLocPackageName = FName(*(BasePackageNameString + LOCALIZED_SEEKFREE_SUFFIX));
								for (INT PackageIndex3 = 0; PackageIndex3 < PackageMap->List.Num(); PackageIndex3++)
								{
									if (PackageMap->List(PackageIndex3).PackageName == BaseLocPackageName)
									{
										BaseLocPackageIndex = PackageIndex3;
										break;
									}
								}

								// if the localized base package exists in the package map, load it
								if (BaseLocPackageIndex)
								{
									// check for localized package first as that may be required to load the base package
									FString LocalizedFileName;
									if (GPackageFileCache->FindPackageFile(*BaseLocPackageName.ToString(), NULL, LocalizedFileName))
									{
										LoadPackage(NULL, *LocalizedFileName, LOAD_None);
									}
								}
								// load base package
								LoadPackage(NULL, *BasePackageNameString, LOAD_None);
								// now try to find it
								Package = FindPackage(NULL, *PackageName);
							}
						}
					}

					// if we have a valid package
					if (Package != NULL)
					{
						// check GUID
						if (Package->GetGuid() != Info.Guid)
						{
							Error = FString::Printf(TEXT("Package '%s' version mismatch"), *Package->GetName());
							debugf(NAME_Error, *Error);
							return FALSE;
						}
						// record info in package map
						Info.Parent = Package;
						Info.LocalGeneration = Package->GetGenerationNetObjectCount().Num();
						// tell the server what we have
						Pending->GetDriver()->ServerConnection->Logf(TEXT("HAVE GUID=%s GEN=%i"), *Info.Guid.String(), Info.LocalGeneration);
					}
					else
					{
						Error = FString::Printf(TEXT("Required package '%s' not found (download was already attempted)"), *Info.PackageName.ToString());
						debugf(NAME_Error, *Error);
						return FALSE;
					}
				}
			}
		}

		PackageMap->Compute();
	}

	// Handle pending level.
	if( Pending )
	{
		check(Pending==GPendingLevel);

		// Hook network driver up to level.
		GWorld->NetDriver = Pending->NetDriver;
		if( GWorld->NetDriver )
		{
			GWorld->NetDriver->Notify = GWorld;
			UPackage::NetObjectNotify = GWorld->NetDriver;
		}

		// Hook up the DemoRecDriver from the pending level
		GWorld->DemoRecDriver = Pending->DemoRecDriver;
		if (GWorld->DemoRecDriver)
		{
			GWorld->DemoRecDriver->Notify = GWorld;
		}

		// Setup level.
		GWorld->GetWorldInfo()->NetMode = NM_Client;
	}
	else
	{
		check(!GWorld->NetDriver);
	}

	// GEMINI_TODO: A nicer precaching scheme.
	GPrecacheNextFrame = TRUE;

	GWorld->SetGameInfo(URL);

	// Listen for clients.
	if( !Client || URL.HasOption(TEXT("Listen")) )
	{
		if( GPendingLevel )
		{
			check(!Pending);
			GPendingLevel = NULL;
		}
		FString Error2;
		if (!GWorld->Listen(URL, Error2))
		{
			appErrorf( *LocalizeError(TEXT("ServerListen"),TEXT("Engine")), *Error2 );
		}
	}

	// Initialize gameplay for the level.
	GWorld->BeginPlay(URL);

	// Remember the URL. Put this before spawning player controllers so that
	// a player controller can get the map name during initialization and
	// have it be correct
	LastURL = URL;
	if (GWorld->GetNetMode() == NM_Client)
	{
		// remember last server URL
		LastRemoteURL = URL;
	}

	// Client init.
	for(FPlayerIterator It(this);It;++It)
	{
		FString Error2;
		if(!It->SpawnPlayActor(URL.String(1),Error2))
		{
			appErrorf(TEXT("%s"),*Error2);
		}
	}

	// Remember DefaultPlayer options.
	if( GIsClient )
	{
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Name" ), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Team" ), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Class"), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Skin" ), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Face" ), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Voice" ), GGameIni );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("OverrideClass" ), GGameIni );
	}

	// Prime texture streaming.
	GStreamingManager->NotifyLevelChange();

	debugf(TEXT("########### Finished loading level: %f seconds"),appSeconds() - StartTime);

	// send a callback message
	GCallbackEvent->Send(CALLBACK_PostLoadMap);

	// load any per-map packages
	check(GWorld->PersistentLevel);
	LoadPackagesFully(FULLYLOAD_Map, GWorld->PersistentLevel->GetOutermost()->GetName());

	// Successfully started local level.
	return TRUE;
}

//
// Jumping viewport.
//
void UGameEngine::SetClientTravel( const TCHAR* NextURL, ETravelType InTravelType )
{
	// set TravelURL.  Will be processed safely on the next tick in UGameEngine::Tick().
	TravelURL    = NextURL;
	TravelType   = InTravelType;

	// Prevent crashing the game by attempting to connect to own listen server
	if ( LastURL.HasOption(TEXT("Listen")) )
		LastURL.RemoveOption(TEXT("Listen"));
}

/*-----------------------------------------------------------------------------
	Async persistent level map change.
-----------------------------------------------------------------------------*/

/**
 * Callback function used in UGameEngine::PrepareMapChange to pass to LoadPackageAsync.
 *
 * @param	LevelPackage	level package that finished async loading
 * @param	InGameEngine	pointer to game engine object to associated loaded level with so it won't be GC'ed
 */
static void AsyncMapChangeLevelLoadCompletionCallback( UObject* LevelPackage, void* InGameEngine )
{
	UGameEngine* GameEngine = (UGameEngine*) InGameEngine;
	check( GameEngine == GEngine );

	if( LevelPackage )
	{	
		// Try to find a UWorld object in the level package.
		UWorld* World = FindObject<UWorld>( LevelPackage, TEXT("TheWorld") );
		ULevel* Level = World ? World->PersistentLevel : NULL;	
		
		// Print out a warning and set the error if we couldn't find a level in this package.
		if( !Level )
		{
			// NULL levels can happen if existing package but not level is specified as a level name.
			GameEngine->PendingMapChangeFailureDescription = FString::Printf(TEXT("Couldn't find level in package %s"), *LevelPackage->GetName());
			debugf( NAME_Error, TEXT( "ERROR ERROR %s was not found in the PackageCache It must exist or the Level Loading Action will FAIL!!!! " ), *LevelPackage->GetName() );
			debugf( NAME_Warning, *GameEngine->PendingMapChangeFailureDescription );
			debugf( NAME_Error, TEXT( "ERROR ERROR %s was not found in the PackageCache It must exist or the Level Loading Action will FAIL!!!! " ), *LevelPackage->GetName() );
		}

		// Add loaded level to array to prevent it from being garbage collected.
		GameEngine->LoadedLevelsForPendingMapChange.AddItem( Level );
	}
	else
	{
		// Add NULL entry so we don't end up waiting forever on a level that is never going to be loaded.
		GameEngine->LoadedLevelsForPendingMapChange.AddItem( NULL );
		debugf( NAME_Warning, TEXT("NULL LevelPackage as argument to AsyncMapChangeLevelCompletionCallback") );
	}
}

/**
 * Prepares the engine for a map change by pre-loading level packages in the background.
 *
 * @param	LevelNames	Array of levels to load in the background; the first level in this
 *						list is assumed to be the new "persistent" one.
 *
 * @return	TRUE if all packages were in the package file cache and the operation succeeded, 
 *			FALSE otherwise. FALSE as a return value also indicates that the code has given
 *			up.
 */
UBOOL UGameEngine::PrepareMapChange(const TArray<FName>& LevelNames)
{
#if !FINAL_RELEASE
	// make sure level streaming isn't frozen
	GWorld->bIsLevelStreamingFrozen = FALSE;
#endif

	// Make sure we don't interrupt a pending map change in progress.
	if( !IsPreparingMapChange() )
	{
		LevelsToLoadForPendingMapChange.Empty();
		LevelsToLoadForPendingMapChange += LevelNames;

#if !FINAL_RELEASE
		// Verify that all levels specified are in the package file cache.
		FString Dummy;
		for( INT LevelIndex=0; LevelIndex<LevelsToLoadForPendingMapChange.Num(); LevelIndex++ )
		{
			const FName LevelName = LevelsToLoadForPendingMapChange(LevelIndex);
			if( !GPackageFileCache->FindPackageFile( *LevelName.ToString(), NULL, Dummy ) )
			{
				LevelsToLoadForPendingMapChange.Empty();
				PendingMapChangeFailureDescription = FString::Printf(TEXT("Couldn't find package for level %s"), *LevelName.ToString());
				return FALSE;
			}
			//@todo streaming: make sure none of the maps are already loaded/ being loaded?
		}
#endif

		// copy LevelNames into the WorldInfo's array to keep track of the map change that we're preparing (primarily for servers so clients that join in progress can be notified)
		if (GWorld != NULL)
		{
			GWorld->GetWorldInfo()->PreparingLevelNames = LevelNames;
		}

		// Kick off async loading of packages.
		for( INT LevelIndex=0; LevelIndex<LevelsToLoadForPendingMapChange.Num(); LevelIndex++ )
		{
			const FName LevelName = LevelsToLoadForPendingMapChange(LevelIndex);
			if( GUseSeekFreeLoading )
			{
				// Only load localized package if it exists as async package loading doesn't handle errors gracefully.
				FString LocalizedPackageName = LevelName.ToString() + LOCALIZED_SEEKFREE_SUFFIX;
				FString LocalizedFileName;
				if( GPackageFileCache->FindPackageFile( *LocalizedPackageName, NULL, LocalizedFileName ) )
				{
					// Load localized part of level first in case it exists. We don't need to worry about GC or completion 
					// callback as we always kick off another async IO for the level below.
					UObject::LoadPackageAsync( *LocalizedPackageName, NULL, NULL );
				}
			}
			UObject::LoadPackageAsync( *LevelName.ToString(), AsyncMapChangeLevelLoadCompletionCallback, this );
		}

		return TRUE;
	}
	else
	{
		PendingMapChangeFailureDescription = TEXT("Current map change still in progress");
		return FALSE;
	}
}

/**
 * Returns the failure description in case of a failed map change request.
 *
 * @return	Human readable failure description in case of failure, empty string otherwise
 */
FString UGameEngine::GetMapChangeFailureDescription()
{
	return PendingMapChangeFailureDescription;
}
	
/**
 * Returns whether we are currently preparing for a map change or not.
 *
 * @return TRUE if we are preparing for a map change, FALSE otherwise
 */
UBOOL UGameEngine::IsPreparingMapChange()
{
	return LevelsToLoadForPendingMapChange.Num() > 0;
}
	
/**
 * Returns whether the prepared map change is ready for commit having called.
 *
 * @return TRUE if we're ready to commit the map change, FALSE otherwise
 */
UBOOL UGameEngine::IsReadyForMapChange()
{
	return IsPreparingMapChange() && (LevelsToLoadForPendingMapChange.Num() == LoadedLevelsForPendingMapChange.Num());
}

/**
 * Commit map change if requested and map change is pending. Called every frame.
 */
void UGameEngine::ConditionalCommitMapChange()
{
	// Check whether there actually is a pending map change and whether we want it to be committed yet.
	if( bShouldCommitPendingMapChange && IsPreparingMapChange() )
	{
		// Block on remaining async data.
		if( !IsReadyForMapChange() )
		{
			FlushAsyncLoading();
			check( IsReadyForMapChange() );
		}
		
		// Perform map change.
		if( !CommitMapChange(bShouldSkipLevelStartupEventOnMapCommit, bShouldSkipLevelBeginningEventOnMapCommit) )
		{
			debugf(NAME_Warning, TEXT("Committing map change via %s was not successful: %s"), *GetFullName(), *GetMapChangeFailureDescription());
		}
		// No pending map change - called commit without prepare.
		else
		{
			debugf(TEXT("Committed map change via %s"), *GetFullName());
		}

		// We just commited, so reset the flag.
		bShouldCommitPendingMapChange = FALSE;
	}
}

/**
 * Finalizes the pending map change that was being kicked off by PrepareMapChange.
 *
 * @param bShouldSkipLevelStartupEvent TRUE if this function NOT fire the SeqEvent_LevelBStartup event.
 * @param bShouldSkipLevelBeginningEvent TRUE if this function NOT fire the SeqEvent_LevelBeginning event. Useful for when skipping to the middle of a map by savegame or something
 *
 * @return	TRUE if successful, FALSE if there were errors (use GetMapChangeFailureDescription 
 *			for error description)
 */
UBOOL UGameEngine::CommitMapChange(UBOOL bShouldSkipLevelStartupEvent, UBOOL bShouldSkipLevelBeginningEvent)
{
	if (!IsPreparingMapChange())
	{
		PendingMapChangeFailureDescription = TEXT("No map change is being prepared");
		return FALSE;
	}
	else if (!IsReadyForMapChange())
	{
		PendingMapChangeFailureDescription = TEXT("Map change is not ready yet");
		return FALSE;
	}
	else
	{
		// Dump and reset the FPS chart on map changes.
		DumpFPSChart();
		ResetFPSChart();

		// Dump and reset the Memory chart on map changes.
		DumpMemoryChart();
		ResetMemoryChart();

		// tell the game we are about to switch levels
        if (GWorld->GetGameInfo())
		{
			// get the actual persistent level's name
			FString PreviousMapName = GWorld->PersistentLevel->GetOutermost()->GetName();
			FString NextMapName = LevelsToLoadForPendingMapChange(0).ToString();

			// look for a persistent streamed in sublevel
			for (INT LevelIndex = 0; LevelIndex < GWorld->GetWorldInfo()->StreamingLevels.Num(); LevelIndex++)
			{
				ULevelStreamingPersistent* PersistentLevel = Cast<ULevelStreamingPersistent>(GWorld->GetWorldInfo()->StreamingLevels(LevelIndex));
				if (PersistentLevel)
				{
					PreviousMapName = PersistentLevel->PackageName.ToString();
					// only one persistent level
					break;
				}
			}
            GWorld->GetGameInfo()->eventPreCommitMapChange(PreviousMapName, NextMapName); 
		}

		// we are no longer preparing this change
		GWorld->GetWorldInfo()->PreparingLevelNames.Empty();
		// copy LevelNames into the WorldInfo's array to keep track of the last map change we performed (primarily for servers so clients that join in progress can be notified)
		GWorld->GetWorldInfo()->CommittedLevelNames = LevelsToLoadForPendingMapChange;

		// Iterate over level collection, marking them to be forcefully unloaded.
		AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
		for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* StreamingLevel	= WorldInfo->StreamingLevels(LevelIndex);
			if( StreamingLevel )
			{
				StreamingLevel->bIsRequestingUnloadAndRemoval = TRUE;
			}
		}

		// Collect garbage. @todo streaming: make sure that this doesn't stall due to async loading in the background
		UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, TRUE );

		// The new fake persistent level is first in the LevelsToLoadForPendingMapChange array.
		FName	FakePersistentLevelName = LevelsToLoadForPendingMapChange(0);
		ULevel*	FakePersistentLevel		= NULL;

		// Find level package in loaded levels array.
		for( INT LevelIndex=0; LevelIndex<LoadedLevelsForPendingMapChange.Num(); LevelIndex++ )
		{
			ULevel* Level = LoadedLevelsForPendingMapChange(LevelIndex);

			// NULL levels can happen if existing package but not level is specified as a level name.
			if( Level && (FakePersistentLevelName == Level->GetOutermost()->GetFName()) )
			{
				FakePersistentLevel = Level;
				break;
			}
		}
		check( FakePersistentLevel );

		// Construct a new ULevelStreamingPersistent for the new persistent level.
		ULevelStreamingPersistent* LevelStreamingPersistent = ConstructObject<ULevelStreamingPersistent>(
			ULevelStreamingPersistent::StaticClass(),
			UObject::GetTransientPackage(),
			*FString::Printf(TEXT("LevelStreamingPersistent_%s"), *FakePersistentLevel->GetOutermost()->GetName()) );


		// Propagate level and name to streaming object.
		LevelStreamingPersistent->LoadedLevel	= FakePersistentLevel;
		LevelStreamingPersistent->PackageName	= FakePersistentLevelName;
		// And add it to the world info's list of levels.
		WorldInfo->StreamingLevels.AddItem( LevelStreamingPersistent );

		// Add secondary levels to the world info levels array.
		WorldInfo->StreamingLevels += FakePersistentLevel->GetWorldInfo()->StreamingLevels;

		// fixup up any kismet streaming objects to force them to be loaded if they were preloaded, this
		// will keep streaming volumes from immediately unloading the levels that were just loaded
		for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* StreamingLevel	= WorldInfo->StreamingLevels(LevelIndex);
			// mark any kismet streamers to force be loaded
			if (StreamingLevel)
			{
				UBOOL bWasFound = FALSE;
				// was this one of the packages we wanted to load?
				for (INT LoadLevelIndex = 0; LoadLevelIndex < LevelsToLoadForPendingMapChange.Num(); LoadLevelIndex++)
				{
					if (LevelsToLoadForPendingMapChange(LoadLevelIndex) == StreamingLevel->PackageName)
					{
						bWasFound = TRUE;
						break;
					}
				}

				// if this level was preloaded, mark it as to be loaded and visible
				if (bWasFound)
				{
					StreamingLevel->bShouldBeLoaded		= TRUE;
					StreamingLevel->bShouldBeVisible	= TRUE;

					// notify players of the change
					for (AController *Controller = GWorld->GetWorldInfo()->ControllerList; Controller != NULL; Controller = Controller->NextController)
					{
						APlayerController *PC = Cast<APlayerController>(Controller);
						if (PC != NULL)
						{
							PC->eventLevelStreamingStatusChanged( 
								StreamingLevel, 
								StreamingLevel->bShouldBeLoaded, 
								StreamingLevel->bShouldBeVisible,
								StreamingLevel->bShouldBlockOnLoad );
						}
					}
				}
			}
		}
		
		// Kill actors we are supposed to remove reference to during seamless map transitions.
		GWorld->CleanUpBeforeLevelTransition();

		// Update level streaming, forcing existing levels to be unloaded and their streaming objects 
		// removed from the world info.	We can't kick off async loading in this update as we want to 
		// collect garbage right below.
		GWorld->FlushLevelStreaming( NULL, TRUE );
		
		// make sure any looping sounds, etc are stopped
		if (Client != NULL && Client->GetAudioDevice() != NULL)
		{
			GEngine->Client->GetAudioDevice()->StopAllSounds();
		}

		// Remove all unloaded levels from memory and perform full purge.
		UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, TRUE );
		
		// if there are pending streaming changes replicated from the server, apply them immediately
		if (PendingLevelStreamingStatusUpdates.Num() > 0)
		{
			for (INT i = 0; i < PendingLevelStreamingStatusUpdates.Num(); i++)
			{
				ULevelStreaming* LevelStreamingObject = NULL;
				for (INT j = 0; j < WorldInfo->StreamingLevels.Num(); j++)
				{
					if (WorldInfo->StreamingLevels(j) != NULL && WorldInfo->StreamingLevels(j)->PackageName == PendingLevelStreamingStatusUpdates(i).PackageName)
					{
						LevelStreamingObject = WorldInfo->StreamingLevels(j);
						if (LevelStreamingObject != NULL)
						{
							LevelStreamingObject->bShouldBeLoaded	= PendingLevelStreamingStatusUpdates(i).bShouldBeLoaded;
							LevelStreamingObject->bShouldBeVisible	= PendingLevelStreamingStatusUpdates(i).bShouldBeVisible;
						}
						else
						{
							check(LevelStreamingObject);
							debugfSuppressed(NAME_DevStreaming, TEXT("Unable to handle streaming object %s"),*LevelStreamingObject->GetName());
						}

						// break out of object iterator if we found a match
						break;
					}
				}

				if (LevelStreamingObject == NULL)
				{
					debugfSuppressed(NAME_DevStreaming, TEXT("Unable to find streaming object %s"), *PendingLevelStreamingStatusUpdates(i).PackageName.ToString());
				}
			}

			PendingLevelStreamingStatusUpdates.Empty();

			GWorld->FlushLevelStreaming(NULL, FALSE);
		}
		else
		{
			// This will cause the newly added persistent level to be made visible and kick off async loading for others.
			GWorld->FlushLevelStreaming( NULL, TRUE );
		}

		// delay the use of streaming volumes for a few frames
		GWorld->DelayStreamingVolumeUpdates(3);

		// notify the new levels that they are starting up
		for( INT LevelIndex=0; LevelIndex<LoadedLevelsForPendingMapChange.Num(); LevelIndex++ )
		{
			ULevel* Level = LoadedLevelsForPendingMapChange(LevelIndex);
			if( Level )
			{
				for (INT SeqIdx = 0; SeqIdx < Level->GameSequences.Num(); SeqIdx++)
				{
					USequence* Seq = Level->GameSequences(SeqIdx);
					if(Seq)
					{
						Seq->NotifyMatchStarted(!bShouldSkipLevelStartupEvent, !bShouldSkipLevelBeginningEvent);
					}
				}
			}
		}

		// Empty intermediate arrays.
		LevelsToLoadForPendingMapChange.Empty();
		LoadedLevelsForPendingMapChange.Empty();
		PendingMapChangeFailureDescription = TEXT("");

		// Prime texture streaming.
		GStreamingManager->NotifyLevelChange();

		// tell the game we are done switching levels
        if (GWorld->GetGameInfo())
		{
            GWorld->GetGameInfo()->eventPostCommitMapChange(); 
		}

		return TRUE;
	}
}

/**
 * Cancels pending map change.
 */
void UGameEngine::CancelPendingMapChange()
{
	// Empty intermediate arrays.
	LevelsToLoadForPendingMapChange.Empty();
	LoadedLevelsForPendingMapChange.Empty();

	// Reset state and make sure conditional map change doesn't fire.
	PendingMapChangeFailureDescription	= TEXT("");
	bShouldCommitPendingMapChange		= FALSE;
	
	// Reset array of levels to prepare for client.
	if( GWorld )
	{
		GWorld->GetWorldInfo()->PreparingLevelNames.Empty();
	}
}

/*-----------------------------------------------------------------------------
	Tick.
-----------------------------------------------------------------------------*/

//
// Get tick rate limitor.
//
FLOAT UGameEngine::GetMaxTickRate( FLOAT DeltaTime, UBOOL bAllowFrameRateSmoothing )
{
	FLOAT MaxTickRate = 0;

#if CONSOLE
	// Limit framerate on console if VSYNC is enabled to avoid jumps from 30 to 60 and back.
	if( GEnableVSync )
	{
		MaxTickRate = MaxSmoothedFrameRate;
	}
#else
	if( GWorld )
	{
		// Demo recording or playback.
		if( GWorld->DemoRecDriver )
		{
			// Demo recording from dedicated server.
			if( !GWorld->DemoRecDriver->ServerConnection && GWorld->NetDriver && !GIsClient )
			{
				// We're a dedicated server recording a demo, use the high framerate demo tick.
				MaxTickRate = Clamp( GWorld->DemoRecDriver->NetServerMaxTickRate, 20, 60 );
			}
			// Respect bNoFrameCap otherwise by disabling framerate smoothing
			else if( GWorld->DemoRecDriver->bNoFrameCap )
			{
				bAllowFrameRateSmoothing = FALSE;
			}
		}
		// In network games, limit framerate to not saturate bandwidth.
		else if( GWorld->NetDriver && (!GIsClient || GWorld->NetDriver->bClampListenServerTickRate))
		{
			// We're a dedicated server, use the LAN or Net tick rate.
			MaxTickRate = Clamp( GWorld->NetDriver->NetServerMaxTickRate, 10, 120 );
		}
		else if( GWorld->NetDriver && GWorld->NetDriver->ServerConnection )
		{
			//@todo FIXMESTEVE: take voice bandwidth into account.
			MaxTickRate = GWorld->NetDriver->ServerConnection->CurrentNetSpeed / GWorld->GetWorldInfo()->MoveRepSize;
			if( GWorld->NetDriver->ServerConnection->CurrentNetSpeed <= 10000 )
			{
				MaxTickRate = Clamp( MaxTickRate, 10.f, 90.f );
			}
		}
	}

	// Smooth the framerate if wanted. The code uses a simplistic running average. Other approaches, like reserving
	// a percentage of time, ended up creating negative feedback loops in conjunction with GPU load and were abandonend.
	if( bSmoothFrameRate && bAllowFrameRateSmoothing && GIsClient )
	{
		check( DeltaTime > 0 );
		// Running average delta time, initial value at 100 FPS so fast machines don't have to creep up
		// to a good frame rate due to code limiting upward "mobility".
		static FLOAT RunningAverageDeltaTime = 1 / 100.f;

		// Keep track of running average over 300 frames, clamping at min of 5 FPS for individual delta times.
		RunningAverageDeltaTime = Lerp<FLOAT>( RunningAverageDeltaTime, Min<FLOAT>( DeltaTime, 0.2f ), 1 / 300.f );

		// Work in FPS domain as that is what the function will return.
		FLOAT AverageFPS = 1.f / RunningAverageDeltaTime;

		// Clamp FPS into ini defined min/ max range.
		MaxTickRate = Clamp<FLOAT>( AverageFPS, MinSmoothedFrameRate, MaxSmoothedFrameRate );
	}
#endif

	return MaxTickRate;
}

//
// Update everything.
//
void UGameEngine::Tick( FLOAT DeltaSeconds )
{
	INT LocalTickCycles=0;
	clock(LocalTickCycles);

	check(GWorld);

    if( DeltaSeconds < 0.0f )
	{
		//@todo ship: replace with globally visible UDN page.
        appErrorf(TEXT("Negative delta time! Please see https://udn.epicgames.com/lists/showpost.php?list=ue3bugs&id=4364"));
	}

	// Ticks the FPS chart.
	TickFPSChart( DeltaSeconds );

	// Ticks the Memory chart.
	TickMemoryChart( DeltaSeconds );

	if ( GDebugger != NULL )
	{
		GDebugger->NotifyBeginTick();
	}

	// Tick the client code.
	INT LocalClientCycles=0;
	if( Client )
	{
		clock(LocalClientCycles);
		Client->Tick( DeltaSeconds );
		unclock(LocalClientCycles);
	}
	ClientCycles=LocalClientCycles;

	// Clean up the game viewports that have been closed.
	CleanupGameViewport();

	// If all viewports closed, time to exit.
	if(GIsClient && GameViewport == NULL )
	{
		debugf( TEXT("All Windows Closed") );
		appRequestExit( 0 );
		return;
	}

	// Release mouse if the game is paused. The low level input code might ignore the request when e.g. in fullscreen mode.
	if ( GameViewport != NULL )
	{
		GameViewport->UpdateMouseCapture();
	}

	// Figure out whether this is the initial tick for this world.
	UBOOL bInitialTick = GWorld && GWorld->GetTimeSeconds() == 0 ? TRUE : FALSE;

	// Update subsystems.
	{
		// This assumes that UObject::StaticTick only calls ProcessAsyncLoading.
		UObject::StaticTick( DeltaSeconds );
	}

	// Handle seamless travelling
	if (GSeamlessTravelHandler.IsInTransition())
	{
		GSeamlessTravelHandler.Tick();
	}

	// Decide whether to drop high detail because of frame rate.
	if ( Client )
	{
		GWorld->GetWorldInfo()->bDropDetail = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate,1.f,100.f)) && !GIsBenchmarking;
		GWorld->GetWorldInfo()->bAggressiveLOD = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate - 5.f,1.f,100.f)) && !GIsBenchmarking;
	}

	// Tick the world.
	GameCycles=0;
	clock(GameCycles);
	GWorld->Tick( LEVELTICK_All, DeltaSeconds );
	unclock(GameCycles);

	// Issue cause event after first tick to provide a chance for the game to spawn the player and such.
	if( bInitialTick )
	{
		const TCHAR* InitialExec = LastURL.GetOption(TEXT("causeevent="),NULL);
		if( InitialExec && GamePlayers.Num() && GamePlayers(0) )
		{
			debugf(TEXT("Issuing initial cause event passed from URL: %s"), InitialExec);
			GamePlayers(0)->Exec( *(FString("CAUSEEVENT ") + InitialExec), *GLog );
		}
	}

	// Tick the viewports.
	if ( GameViewport != NULL )
	{
		GameViewport->Tick(DeltaSeconds);
	}

	// Handle server travelling.
	if( GWorld->GetWorldInfo()->NextURL!=TEXT("") )
	{
		if( (GWorld->GetWorldInfo()->NextSwitchCountdown-=DeltaSeconds) <= 0.f )
		{
			debugf( TEXT("Server switch level: %s"), *GWorld->GetWorldInfo()->NextURL );
			FString Error;
			Browse( FURL(&LastURL,*GWorld->GetWorldInfo()->NextURL,TRAVEL_Relative), Error );
			GWorld->GetWorldInfo()->NextURL = TEXT("");
			return;
		}
	}

	// Handle client travelling.
	if( TravelURL != TEXT("") )
	{
		// Make sure we stop any vibration before loading next map.
		if(Client)
		{
			Client->ForceClearForceFeedback();
		}

        if( GWorld->GetGameInfo() )
		{
            GWorld->GetGameInfo()->eventGameEnding(); 
		}

		FString Error;
		Browse( FURL(&LastURL,*TravelURL,(ETravelType)TravelType), Error );
		TravelURL=TEXT("");

		return;
	}

	// Update the pending level.
	if( GPendingLevel )
	{
		GPendingLevel->Tick( DeltaSeconds );
		if( GPendingLevel->Error!=TEXT("") )
		{
			debugf(TEXT("-->No connection error"));
#if !CONSOLE
			// Pending connect failed.
			SetProgress( *LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), *GPendingLevel->Error, 4.f );
#endif
			debugf( NAME_Log, *LocalizeError(TEXT("Pending"),TEXT("Engine")), *GPendingLevel->URL.String(), *GPendingLevel->Error );
			GPendingLevel = NULL;
		}
		else if( GPendingLevel->Success && !GPendingLevel->FilesNeeded && !GPendingLevel->SentJoin )
		{
			// Attempt to load the map.
			FString Error;

			LoadMap( GPendingLevel->URL, GPendingLevel, Error );
			if( Error!=TEXT("") )
			{
				debugf(TEXT("-->No connection error - CREATE PC HACK"));

				// make sure there's a valid GWorld and PlayerController available to handle the error message
				// errors here are fatal; there's nothing left to fall back to
				check(GamePlayers.Num() > 0);
				if (GamePlayers(0)->Actor == NULL)
				{
					debugf(TEXT("-->No connection error - CREATE PC HACK GO"));

					if (GWorld != NULL)
					{
						// need a PlayerController for loading screen rendering
						GamePlayers(0)->Actor = CastChecked<APlayerController>(GWorld->SpawnActor(APlayerController::StaticClass()));
					}
					// we can't guarantee the current GWorld is in a valid state, so travel to the default map
					FString NewError;
					verify(Browse(FURL(&LastURL, TEXT("?failed"), TRAVEL_Relative), NewError));
					check(GWorld != NULL);
					//@todo: should have special code/error message for this
					check(GamePlayers(0)->Actor != NULL);
				}
				SetProgress( *LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), *Error, 4.f );
			}
			else
			{
				// Show connecting message, cause precaching to occur.
				TransitionType = TT_Connecting;
				RedrawViewports();

				// Send join.
				GPendingLevel->SendJoin();
				GPendingLevel->NetDriver = NULL;
				GPendingLevel->DemoRecDriver = NULL;
			}

			// Kill the pending level.
			GPendingLevel = NULL;
		}
	}

#if STATS	// Can't do this within the malloc classes as they get initialized before the stats
	FStatGroup* MemGroup = GStatManager.GetGroup(STATGROUP_Memory);
	// Update memory stats.
	if (MemGroup->bShowGroup == TRUE)
	{
#if PS3
		// handle special memory regions of PS3
		extern void UpdateMemStats();
		UpdateMemStats();
#else
		SIZE_T Virtual  = 0; 
		SIZE_T Physical = 0;
		GMalloc->GetAllocationInfo( Virtual, Physical );
		SET_DWORD_STAT(STAT_VirtualAllocSize,Virtual);
		SET_DWORD_STAT(STAT_PhysicalAllocSize,Physical);
#endif
		
#if WITH_FACEFX
		SET_DWORD_STAT(STAT_FaceFXCurrentAllocSize,OC3Ent::Face::FxGetCurrentBytesAllocated());
		SET_DWORD_STAT(STAT_FaceFXPeakAllocSize,OC3Ent::Face::FxGetPeakBytesAllocated());
#endif

	}
#endif

	// Update the transition screen.
	if(TransitionType == TT_Connecting)
	{
		// Check to see if all players have finished connecting.
		TransitionType = TT_None;
		for(FPlayerIterator PlayerIt(this);PlayerIt;++PlayerIt)
		{
			if(!PlayerIt->Actor)
			{
				// This player has not received a PlayerController from the server yet, so leave the connecting screen up.
				TransitionType = TT_Connecting;
				break;
			}
		}
	}
	else if(TransitionType == TT_None || TransitionType == TT_Paused)
	{
		// Display a paused screen if the game is paused.
		TransitionType = (GWorld->GetWorldInfo()->Pauser != NULL) ? TT_Paused : TT_None;
	}

	// Render everything.
	RedrawViewports();
	
	// Block on async loading if requested.
	if( GWorld->GetWorldInfo()->bRequestedBlockOnAsyncLoading )
	{
		// Only perform work if there is anything to do. This ensures we are not syncronizing with the GPU
		// and suspending the device needlessly.
		if( UObject::IsAsyncLoading() )
		{
			// Make sure vibration stops when blocking on load.
			if(GEngine->Client)
			{
				GEngine->Client->ForceClearForceFeedback();
			}

			// tell clients to do the same so they don't fall behind
			for (AController* C = GWorld->GetFirstController(); C != NULL; C = C->NextController)
			{
				APlayerController* PC = C->GetAPlayerController();
				if (PC != NULL)
				{
					UNetConnection* Conn = Cast<UNetConnection>(PC->Player);
					if (Conn != NULL && Conn->GetUChildConnection() == NULL)
					{
						// call the event to replicate the call
						PC->eventClientSetBlockOnAsyncLoading();
						// flush the connection to make sure it gets sent immediately
						Conn->FlushNet();
					}
				}
			}

			// Enqueue command to suspend rendering during blocking load.
			ENQUEUE_UNIQUE_RENDER_COMMAND( SuspendRendering, { extern UBOOL GGameThreadWantsToSuspendRendering; GGameThreadWantsToSuspendRendering = TRUE; } );
			// Flush rendering commands to ensure we don't have any outstanding work on rendering thread
			// and rendering is indeed suspended.
			FlushRenderingCommands();

			// Flushes level streaming requests, blocking till completion.
			GWorld->FlushLevelStreaming();

			// Resume rendering again now that we're done loading.
			ENQUEUE_UNIQUE_RENDER_COMMAND( ResumeRendering, { extern UBOOL GGameThreadWantsToSuspendRendering; GGameThreadWantsToSuspendRendering = FALSE; RHIResumeRendering(); } );
		}
		GWorld->GetWorldInfo()->bRequestedBlockOnAsyncLoading = FALSE;
	}

	// streamingServer
    if( GIsServer == TRUE )
	{
		GWorld->UpdateLevelStreaming();
	}

	// Update Audio. This needs to occur after rendering as the rendering code updates the listener position.
	if( Client && Client->GetAudioDevice() )
	{
		Client->GetAudioDevice()->Update( !GWorld->IsPaused() );
	}

	if( GIsClient )
	{
		// Update resource streaming after viewports have had a chance to update view information.
		GStreamingManager->UpdateResourceStreaming( DeltaSeconds );
	}

	unclock(LocalTickCycles);
	TickCycles=LocalTickCycles;

	if( GScriptCallGraph )
	{
		GScriptCallGraph->Tick();
	}

#if USING_REMOTECONTROL
	if ( RemoteControlExec )
	{
		RemoteControlExec->RenderInGame();
	}
#endif

	// See whether any map changes are pending and we requested them to be committed.
	ConditionalCommitMapChange();
}

#if !FINAL_RELEASE
/**
 * Handles freezing/unfreezing of rendering
 */
void UGameEngine::ProcessToggleFreezeCommand()
{
	if (GameViewport)
	{
		GameViewport->Viewport->ProcessToggleFreezeCommand();
	}
}

/**
 * Handles frezing/unfreezing of streaming
 */
void UGameEngine::ProcessToggleFreezeStreamingCommand()
{
	// if not already frozen, then flush async loading before we freeze so that we don't bollocks up any in-process streaming
	if (!GWorld->bIsLevelStreamingFrozen)
	{
		UObject::FlushAsyncLoading();
	}

	// toggle the frozen state
	GWorld->bIsLevelStreamingFrozen = !GWorld->bIsLevelStreamingFrozen;
}

#endif

/**
 * Returns the online subsystem object. Returns null if GEngine isn't a
 * game engine
 */
UOnlineSubsystem* UGameEngine::GetOnlineSubsystem(void)
{
	UOnlineSubsystem* OnlineSubsystem = NULL;
	// Make sure this is the correct type (not PIE)
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine != NULL)
	{
		OnlineSubsystem = GameEngine->OnlineSubsystem;
	}
	return OnlineSubsystem;
}

/**
 * Script to C++ thunking code
 */
void UGameEngine::execGetOnlineSubsystem( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(class UOnlineSubsystem**)Result=UGameEngine::GetOnlineSubsystem();
}

/**
 * Creates the online subsystem that was specified in UEngine's
 * OnlineSubsystemClass. This function is virtual so that licensees
 * can provide their own version without modifying Epic code.
 */
void UGameEngine::InitOnlineSubsystem(void)
{
	// See if a class was loaded
	if (OnlineSubsystemClass != NULL)
	{
		// Create an instance of the object
		OnlineSubsystem = ConstructObject<UOnlineSubsystem>(OnlineSubsystemClass);
		if (OnlineSubsystem)
		{
			// Try to initialize the subsystem
			// NOTE: The subsystem is responsible for fixing up its interface pointers
			if (OnlineSubsystem->eventInit() == FALSE)
			{
				debugf(NAME_Error,TEXT("Failed to init online subsytem (%s)"),
					*OnlineSubsystem->GetFullName());
				// Remove the ref so it will get gc-ed
				OnlineSubsystem = NULL;
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to create online subsytem (%s)"),
				*OnlineSubsystemClass->GetFullName());
		}
	}
}

/**
 * Spawns all of the registered server actors
 */
void UGameEngine::SpawnServerActors(void)
{
	for( INT i=0; i < ServerActors.Num(); i++ )
	{
		TCHAR Str[240];
		const TCHAR* Ptr = * ServerActors(i);
		if( ParseToken( Ptr, Str, ARRAY_COUNT(Str), 1 ) )
		{
			debugf(NAME_DevNet, TEXT("Spawning: %s"), Str );
			UClass* HelperClass = StaticLoadClass( AActor::StaticClass(), NULL, Str, NULL, LOAD_None, NULL );
			AActor* Actor = GWorld->SpawnActor( HelperClass );
			while( Actor && ParseToken(Ptr,Str,ARRAY_COUNT(Str),1) )
			{
				TCHAR* Value = appStrchr(Str,'=');
				if( Value )
				{
					*Value++ = 0;
					for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Actor->GetClass()); It; ++It )
						if
						(	appStricmp(*It->GetName(),Str)==0
						&&	(It->PropertyFlags & CPF_Config) )
							It->ImportText( Value, (BYTE*)Actor + It->Offset, 0, Actor );
				}
			}
		}
	}
}

/**
 * Adds a map/package array pair for pacakges to load at LoadMap
 *
 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
 * @param Tag Map/game for which the packages need to be loaded
 * @param Packages List of package names to fully load when the map is loaded
 * @param bLoadPackagesForCurrentMap If TRUE, the packages for the currently loaded map will be loaded now
 */
void UGameEngine::AddPackagesToFullyLoad(EFullyLoadPackageType FullyLoadType, const FString& Tag, const TArray<FName>& Packages, UBOOL bLoadPackagesForCurrentMap)
{
	// make a new entry
	const INT NewIndex = PackagesToFullyLoad.AddZeroed(1);

	// get the newly added entry
	FFullyLoadedPackagesInfo& PackagesInfo = PackagesToFullyLoad(NewIndex);

	// fill it out
	PackagesInfo.FullyLoadType = FullyLoadType;
	PackagesInfo.Tag = Tag;
	PackagesInfo.PackagesToLoad = Packages;

	// if desired, load the packages for the current map now
	if (bLoadPackagesForCurrentMap && GWorld && GWorld->PersistentLevel)
	{
		LoadPackagesFully(FullyLoadType, GWorld->PersistentLevel->GetOutermost()->GetName());
	}
}

/**
 * Empties the PerMapPackages array, and removes any currently loaded packages from the Root
 */
void UGameEngine::CleanupAllPackagesToFullyLoad()
{
	// make sure the packages for the current map are cleaned
	CleanupPackagesToFullyLoad(FULLYLOAD_Map, GWorld->PersistentLevel->GetOutermost()->GetName());
	// all pergame type packages need to be unloaded
	CleanupPackagesToFullyLoad(FULLYLOAD_Game_PreLoadClass, TEXT(""));
	CleanupPackagesToFullyLoad(FULLYLOAD_Game_PostLoadClass, TEXT(""));

	// empty the array
	PackagesToFullyLoad.Empty();
}

/**
 * Loads the PerMapPackages for the given map, and adds them to the RootSet
 *
 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
 * @param Tag Name of the map/game to load packages for ("" will match any)
 */
void UGameEngine::LoadPackagesFully(EFullyLoadPackageType FullyLoadType, const FString& Tag)
{
	// look for all entries for the given map
	for (INT MapIndex = 0; MapIndex < PackagesToFullyLoad.Num(); MapIndex++)
	{
		FFullyLoadedPackagesInfo& PackagesInfo = PackagesToFullyLoad(MapIndex);
		// is this entry for the map/game?
		if (PackagesInfo.FullyLoadType == FullyLoadType && (PackagesInfo.Tag == Tag || Tag == TEXT("")))
		{
			// go over all packages that need loading
			for (INT PackageIndex = 0; PackageIndex < PackagesInfo.PackagesToLoad.Num(); PackageIndex++)
			{
				// look for the package in the package cache
				FString PackagePath;
				if (GPackageFileCache->FindPackageFile(*PackagesInfo.PackagesToLoad(PackageIndex).ToString(), NULL, PackagePath))
				{
					// load the package
					// @todo: This would be nice to be async probably, but how would we add it to the root? (LOAD_AddPackageToRoot?)
					UPackage* Package = UObject::LoadPackage(NULL, *PackagePath, 0);

					// add package to root so we can find it
					Package->AddToRoot();

					// remember the object for unloading later
					PackagesInfo.LoadedObjects.AddItem(Package);

					// add the objects to the root set so that it will not be GC'd
					for (TObjectIterator<UObject> It; It; ++It)
					{
						if (It->IsIn(Package))
						{
//							debugf(TEXT("Adding %s to root"), *It->GetFullName());
							It->AddToRoot();

							// remember the object for unloading later
							PackagesInfo.LoadedObjects.AddItem(*It);
						}
					}
				}
				else
				{
					debugf(TEXT("Failed to find Package %s to FullyLoad [FullyLoadType = %d, Tag = %s]"), *PackagesInfo.PackagesToLoad(PackageIndex).ToString(), (INT)FullyLoadType, *Tag);
				}
			}
		}
	}
}

/**
 * Removes the PerMapPackages from the RootSet
 *
 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
 * @param Tag Name of the map/game to cleanup packages for ("" will match any)
 */
void UGameEngine::CleanupPackagesToFullyLoad(EFullyLoadPackageType FullyLoadType, const FString& Tag)
{
	for (INT MapIndex = 0; MapIndex < PackagesToFullyLoad.Num(); MapIndex++)
	{
		FFullyLoadedPackagesInfo& PackagesInfo = PackagesToFullyLoad(MapIndex);
		// is this entry for the map/game?
		if (PackagesInfo.FullyLoadType == FullyLoadType && (PackagesInfo.Tag == Tag || Tag == TEXT("")))
		{
			// mark all objects from this map as unneeded
			for (INT ObjectIndex = 0; ObjectIndex < PackagesInfo.LoadedObjects.Num(); ObjectIndex++)
			{
//				debugf(TEXT("Removing %s from root"), *PackagesInfo.LoadedObjects(ObjectIndex)->GetFullName());
				PackagesInfo.LoadedObjects(ObjectIndex)->RemoveFromRoot();	
			}
			// empty the array of pointers to the objects
			PackagesInfo.LoadedObjects.Empty();
		}
	}
}

