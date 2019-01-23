/*=============================================================================
	DemoRecDrv.cpp: Unreal demo recording network driver.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Jack Porter.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"
#include "DemoRecording.h"
#define PACKETSIZE 512

/*-----------------------------------------------------------------------------
	UDemoRecConnection.
-----------------------------------------------------------------------------*/

UDemoRecConnection::UDemoRecConnection()
{
	MaxPacket   = PACKETSIZE;
	InternalAck = 1;
}

/**
 * Intializes an "addressless" connection with the passed in settings
 *
 * @param InDriver the net driver associated with this connection
 * @param InState the connection state to start with for this connection
 * @param InURL the URL to init with
 * @param InConnectionSpeed Optional connection speed override
 */
void UDemoRecConnection::InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, INT InConnectionSpeed)
{
	// default implementation
	Super::InitConnection(InDriver, InState, InURL, InConnectionSpeed);

	// the driver must be a DemoRecording driver (GetDriver makes assumptions to avoid Cast'ing each time)
	check(InDriver->IsA(UDemoRecDriver::StaticClass()));
}

UDemoRecDriver* UDemoRecConnection::GetDriver()
{
	return (UDemoRecDriver*)Driver;
}

FString UDemoRecConnection::LowLevelGetRemoteAddress()
{
	return TEXT("");
}

void UDemoRecConnection::LowLevelSend( void* Data, INT Count )
{
	if (!GetDriver()->ServerConnection && GetDriver()->FileAr)
	{
		*GetDriver()->FileAr << GetDriver()->LastDeltaTime << GetDriver()->FrameNum << Count;
		GetDriver()->FileAr->Serialize( Data, Count );
		//@todo demorec: if GetDriver()->GetFileAr()->IsError(), print error, cancel demo recording
	}
}

FString UDemoRecConnection::LowLevelDescribe()
{
	return TEXT("Demo recording driver connection");
}

INT UDemoRecConnection::IsNetReady( UBOOL Saturate )
{
	return 1;
}

void UDemoRecConnection::FlushNet()
{
	// in playback, there is no data to send except
	// channel closing if an error occurs.
	if( !GetDriver()->ServerConnection )
	{
		Super::FlushNet();
	}
}

void UDemoRecConnection::HandleClientPlayer( APlayerController* PC )
{
	Super::HandleClientPlayer(PC);
	PC->bDemoOwner = TRUE;
}

IMPLEMENT_CLASS(UDemoRecConnection);

/*-----------------------------------------------------------------------------
	UDemoRecDriver.
-----------------------------------------------------------------------------*/

UDemoRecDriver::UDemoRecDriver()
{}
UBOOL UDemoRecDriver::InitBase( UBOOL Connect, FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error )
{
	DemoFilename	= ConnectURL.Map;
	Time			= 0;
	FrameNum	    = 0;
	bHasDemoEnded	= FALSE;

	return TRUE;
}

UBOOL UDemoRecDriver::InitConnect( FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error )
{
	// handle default initialization
	if (!Super::InitConnect(InNotify, ConnectURL, Error))
	{
		return FALSE;
	}
	if (!InitBase(1, InNotify, ConnectURL, Error))
	{
		return FALSE;
	}

	// Playback, local machine is a client, and the demo stream acts "as if" it's the server.
	ServerConnection = ConstructObject<UNetConnection>(UDemoRecConnection::StaticClass());
	ServerConnection->InitConnection(this, USOCK_Pending, ConnectURL, 1000000);

	// open the pre-recorded demo file
	FileAr = GFileManager->CreateFileReader(*DemoFilename);
	if( !FileAr )
	{
		Error = FString::Printf( TEXT("Couldn't open demo file %s for reading"), *DemoFilename );//@todo demorec: localize
		return 0;
	}

	LoopURL								= ConnectURL;
	bNoFrameCap							= ConnectURL.HasOption(TEXT("timedemo"));
	bShouldExitAfterPlaybackFinished	= ConnectURL.HasOption(TEXT("exitafterplayback"));
	PlayCount							= appAtoi( ConnectURL.GetOption(TEXT("playcount="), TEXT("1")) );
	if( PlayCount == 0 )
	{
		PlayCount = INT_MAX;
	}
	bShouldSkipPackageChecking			= ConnectURL.HasOption(TEXT("skipchecks"));
	
	PlaybackStartTime					= appSeconds();
	LastFrameTime						= appSeconds();

	return TRUE;
}

UBOOL UDemoRecDriver::InitListen( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error )
{
	if( !Super::InitListen( InNotify, ConnectURL, Error ) )
	{
		return 0;
	}
	if( !InitBase( 0, InNotify, ConnectURL, Error ) )
	{
		return 0;
	}

	class AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	if ( !WorldInfo )
	{
		Error = TEXT("No WorldInfo!!");
		return FALSE;
	}

	// Recording, local machine is server, demo stream acts "as if" it's a client.
	UDemoRecConnection* Connection = ConstructObject<UDemoRecConnection>(UDemoRecConnection::StaticClass());
	Connection->InitConnection(this, USOCK_Open, ConnectURL, 1000000);
	Connection->InitOut();

	FileAr = GFileManager->CreateFileWriter( *DemoFilename );
	ClientConnections.AddItem( Connection );

	if( !FileAr )
	{
		Error = FString::Printf( TEXT("Couldn't open demo file %s for writing"), *DemoFilename );//@todo demorec: localize
		return 0;
	}

	// Setup
	UGameEngine* GameEngine = CastChecked<UGameEngine>(GEngine);

	// Build package map.
	MasterMap->AddNetPackages();
	// fixup the RemoteGeneration to be LocalGeneration
	for (INT InfoIndex = 0; InfoIndex < MasterMap->List.Num(); InfoIndex++)
	{
		FPackageInfo& Info = MasterMap->List(InfoIndex);
		Info.RemoteGeneration = Info.LocalGeneration;
	}
	MasterMap->Compute();

#if WILL_NEED
	UPackage::NetObjectNotify = NetDriver;
#endif

#if MAYBE_NEEDED
	UBOOL bNeedsReCompute=false;
	for( INT PackageIndex=0; PackageIndex<MasterMap->List.Num(); PackageIndex++ )
	{
		if ( MasterMap->List(PackageIndex).URL.Right(4) == TEXT(".utx") )
		{
			GWarn->Logf(TEXT("Fixing up %s from %i to 1"),*MasterMap->List(PackageIndex).URL,MasterMap->List(PackageIndex).RemoteGeneration);
			MasterMap->List(PackageIndex).RemoteGeneration = 1;
			bNeedsReCompute=true;
		}

		if (bNeedsReCompute)
		{
			MasterMap->Compute();
		}
	}
#endif

	// Create the control channel.
	Connection->CreateChannel( CHTYPE_Control, 1, 0 );

	// Send initial message.
	Connection->Logf( TEXT("HELLO P=%i REVISION=0 MINVER=%i VER=%i"),(INT)appGetPlatformType(), GEngineMinNetVersion, GEngineVersion );
	Connection->FlushNet();

	// Welcome the player to the level.
	FString WelcomeExtra;

	if( WorldInfo->NetMode == NM_Client )
	{
		WelcomeExtra = TEXT("NETCLIENTDEMO");
	}
	else if( WorldInfo->NetMode == NM_Standalone )
	{
		WelcomeExtra = TEXT("CLIENTDEMO");
	}
	else
	{
		WelcomeExtra = TEXT("SERVERDEMO");
	}

	WelcomeExtra = WelcomeExtra + FString::Printf(TEXT(" TICKRATE=%d"), WorldInfo->NetMode == NM_DedicatedServer ? appRound(GameEngine->GetMaxTickRate(0, FALSE)) : appRound(NetServerMaxTickRate) );

	GWorld->WelcomePlayer(Connection, *WelcomeExtra);

	// Spawn the demo recording spectator.
	SpawnDemoRecSpectator(Connection);

	return 1;
}


void UDemoRecDriver::StaticConstructor()
{
	new(GetClass(),TEXT("DemoSpectatorClass"), RF_Public)UStrProperty(CPP_PROPERTY(DemoSpectatorClass), TEXT("Client"), CPF_Config);
}

void UDemoRecDriver::LowLevelDestroy()
{
	debugf( TEXT("Closing down demo driver.") );

	// Shut down file.
	if( FileAr )
	{	
		delete FileAr;
		FileAr = NULL;
	}
}

UBOOL UDemoRecDriver::UpdateDemoTime( FLOAT* DeltaTime, FLOAT TimeDilation )
{
	UBOOL Result = 0;
	bNoRender = FALSE;

	// Playback.
	if( ServerConnection )
	{
		// This will be triggered several times during initial handshake.
		if( FrameNum == 0 )
		{
			PlaybackStartTime = appSeconds();
		}

		// Ensure LastFrameTime is inside a valid range, so we don't lock up if things get very out of sync.
		LastFrameTime = Clamp<DOUBLE>( LastFrameTime, appSeconds() - 1.0, appSeconds() );

		FrameNum++;
		if( ServerConnection->State==USOCK_Open ) 
		{
			if( !FileAr->AtEnd() && !FileAr->IsError() )
			{
				// peek at next delta time.
				FLOAT NewDeltaTime;
				INT NewFrameNum;

				*FileAr << NewDeltaTime << NewFrameNum;
				FileAr->Seek(FileAr->Tell() - sizeof(NewDeltaTime) - sizeof(NewFrameNum));

				// If the real delta time is too small, sleep for the appropriate amount.
				if( !bNoFrameCap )
				{
					if ( (appSeconds() > LastFrameTime+(DOUBLE)NewDeltaTime/(1.1*TimeDilation)) )
					{
						bNoRender = TRUE;
					}
					else
					{
						while(appSeconds() < LastFrameTime+(DOUBLE)NewDeltaTime/(1.1*TimeDilation))
						{
							appSleep(0);
						}
					}
				}
				// Lie to the game about the amount of time which has passed.
				*DeltaTime = NewDeltaTime;
			}
		}
		LastDeltaTime = *DeltaTime;
	 	LastFrameTime = appSeconds();
	}
	// Recording.
	else
	{
		BYTE NetMode = GWorld->GetWorldInfo()->NetMode;

		// Accumulate the current DeltaTime for the real frames this demo frame will represent.
		DemoRecMultiFrameDeltaTime += *DeltaTime;

		// Cap client demo recording rate (but not framerate).
		if( NetMode==NM_DedicatedServer || ( (appSeconds()-LastClientRecordTime) >= (DOUBLE)(1.f/NetServerMaxTickRate) ) )
		{
			// record another frame.
			FrameNum++;
			LastClientRecordTime		= appSeconds();
			LastDeltaTime				= DemoRecMultiFrameDeltaTime;
			DemoRecMultiFrameDeltaTime	= 0.f;
			Result						= 1;

			// Save the new delta-time and frame number, with no data, in case there is nothing to replicate.
			INT Count = 0;
			*FileAr << LastDeltaTime << FrameNum << Count;
		}
	}

	return Result;
}

void UDemoRecDriver::TickDispatch( FLOAT DeltaTime )
{
	Super::TickDispatch( DeltaTime );

	if( ServerConnection && (ServerConnection->State==USOCK_Pending || ServerConnection->State==USOCK_Open) )
	{	
		BYTE Data[PACKETSIZE + 8];
		// Read data from the demo file
		DWORD PacketBytes;
		INT PlayedThisTick = 0;
		for( ; ; )
		{
			// At end of file?
			if( FileAr->AtEnd() || FileAr->IsError() )
			{
AtEnd:
				ServerConnection->State = USOCK_Closed;
				bHasDemoEnded = TRUE;
				PlayCount--;

				FLOAT Seconds = appSeconds()-PlaybackStartTime;
				if( bNoFrameCap )
				{
					FString Result = FString::Printf(TEXT("Demo %s ended: %d frames in %lf seconds (%.3f fps)"), *DemoFilename, FrameNum, Seconds, FrameNum/Seconds );
					debugf(TEXT("%s"),*Result);
					ServerConnection->Actor->eventClientMessage( *Result, NAME_None );//@todo demorec: localize
				}
				else
				{
					ServerConnection->Actor->eventClientMessage( *FString::Printf(TEXT("Demo %s ended: %d frames in %f seconds"), *DemoFilename, FrameNum, Seconds ), NAME_None );//@todo demorec: localize
				}

				// Exit after playback of last loop iteration has finished.
				if( bShouldExitAfterPlaybackFinished && PlayCount == 0 )
				{
					GIsRequestingExit = TRUE;
				}

				if( PlayCount > 0 )
				{
					// Play while new loop count.
					LoopURL.AddOption( *FString::Printf(TEXT("playcount=%i"),PlayCount) );
					
					// Start over again.
					GWorld->Exec( *(FString(TEXT("DEMOPLAY "))+(*LoopURL.String())), *GLog );
				}

				return;
			}
	
			INT ServerFrameNum;
			FLOAT ServerDeltaTime;

			*FileAr << ServerDeltaTime;
			*FileAr << ServerFrameNum;
			if( ServerFrameNum > FrameNum )
			{
				FileAr->Seek(FileAr->Tell() - sizeof(ServerFrameNum) - sizeof(ServerDeltaTime));
				break;
			}
			*FileAr << PacketBytes;

			if( PacketBytes )
			{
				// Read data from file.
				FileAr->Serialize( Data, PacketBytes );
				if( FileAr->IsError() )
				{
					debugf( NAME_DevNet, TEXT("Failed to read demo file packet") );
					goto AtEnd;
				}

				// Update stats.
				PlayedThisTick++;

				// Process incoming packet.
				ServerConnection->ReceivedRawPacket( Data, PacketBytes );
			}

			// Only play one packet per tick on demo playback, until we're 
			// fully connected.  This is like the handshake for net play.
			if(ServerConnection->State == USOCK_Pending)
			{
				FrameNum = ServerFrameNum;
				break;
			}
		}
	}
}

FString UDemoRecDriver::LowLevelGetNetworkNumber()
{
	return TEXT("");
}

UBOOL UDemoRecDriver::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( bHasDemoEnded )
	{
		return 0;
	}
	if( ParseCommand(&Cmd,TEXT("DEMOREC")) || ParseCommand(&Cmd,TEXT("DEMOPLAY")) )
	{
		if( ServerConnection )
		{
			Ar.Logf( TEXT("Demo playback currently active: %s"), *DemoFilename );//@todo demorec: localize
		}
		else
		{
			Ar.Logf( TEXT("Demo recording currently active: %s"), *DemoFilename );//@todo demorec: localize
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("DEMOSTOP")) )
	{
		PlayCount = 0;
		Ar.Logf( TEXT("Demo %s stopped at frame %d"), *DemoFilename, FrameNum );//@todo demorec: localize
		if( !ServerConnection )
		{
			// let GC cleanup the object
			GWorld->DemoRecDriver=NULL;
		}
		else
		{
			// flush out any pending network traffic
			ServerConnection->FlushNet();
			ServerConnection->State = USOCK_Closed;
		}

		delete FileAr;
		FileAr = NULL;
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}

void UDemoRecDriver::SpawnDemoRecSpectator( UNetConnection* Connection )
{
	UClass* C = StaticLoadClass( AActor::StaticClass(), NULL, *DemoSpectatorClass, NULL, LOAD_None, NULL );
	APlayerController* Controller = CastChecked<APlayerController>(GWorld->SpawnActor( C ));

	for (FActorIterator It; It; ++It)
	{
		if (It->IsA(APlayerStart::StaticClass()))
		{
			Controller->Location = It->Location;
			Controller->Rotation = It->Rotation;
			break;
		}
	}

	Controller->SetPlayer(Connection);
}
IMPLEMENT_CLASS(UDemoRecDriver);


/*-----------------------------------------------------------------------------
	UDemoPlayPendingLevel implementation.
-----------------------------------------------------------------------------*/

//
// Constructor.
//
UDemoPlayPendingLevel::UDemoPlayPendingLevel(const FURL& InURL)
:	UPendingLevel( InURL )
{
	NetDriver = NULL;

	// Try to create demo playback driver.
	UClass* DemoDriverClass = StaticLoadClass( UDemoRecDriver::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.DemoRecordingDevice"), NULL, LOAD_None, NULL );
	DemoRecDriver = ConstructObject<UDemoRecDriver>( DemoDriverClass );
	if( DemoRecDriver->InitConnect( this, URL, Error ) )
	{
	}
	else
	{
		DemoRecDriver = NULL;
	}
}

//
// FNetworkNotify interface.
//
void UDemoPlayPendingLevel::NotifyReceivedText( UNetConnection* Connection, const TCHAR* Text )
{
	//debugf( TEXT("DemoPlayPendingLevel received: %s"), Text );
	if( ParseCommand( &Text, TEXT("USES") ) )
	{
		// Dependency information.
		FPackageInfo& Info = *new(Connection->PackageMap->List)FPackageInfo(NULL);
		Connection->ParsePackageInfo(Text, Info);

		// in the seekfree loading case, we load the requested map first and then attempt to load requested packages that haven't been loaded yet
		// as packages referenced by the map might be forced exports and not actually have a file associated with them
		//@see UGameEngine::LoadMap()
		//@todo: figure out some early-out code to detect when missing downloadable content, etc so we don't have to load the level first
		if( !GUseSeekFreeLoading )
		{
			// verify that we have this package, or it is downloadable
			FString Filename;
			if (GPackageFileCache->FindPackageFile(*Info.PackageName.ToString(), &Info.Guid, Filename))
			{
				Info.Parent = CreatePackage(NULL, *Info.PackageName.ToString());
				// check that the GUID matches (meaning it is the same package or it has been conformed)
				BeginLoad();
				ULinkerLoad* Linker = GetPackageLinker(Info.Parent, NULL, LOAD_NoWarn | LOAD_NoVerify | LOAD_Quiet, NULL, &Info.Guid);
				EndLoad();
				if (Linker == NULL || Linker->Summary.Guid != Info.Guid)
				{
					// incompatible files
					//@todo FIXME: we need to be able to handle this better - have the client ignore this version of the package and download the correct one
					debugf(NAME_DevNet, TEXT("Package '%s' mismatched - Server GUID: %s Client GUID: %s"), *Info.Parent->GetName(), *Info.Guid.String(), (Linker != NULL) ? *Linker->Summary.Guid.String() : TEXT("None"));
					Error = FString::Printf(TEXT("Package '%s' version mismatch"), *Info.Parent->GetName());
					NetDriver->ServerConnection->Close();
					return;
				}
				else
				{
					Info.LocalGeneration = Linker->Summary.Generations.Num();
					// tell the server what we have
					DemoRecDriver->ServerConnection->Logf(TEXT("HAVE GUID=%s GEN=%i"), *Linker->Summary.Guid.String(), Info.LocalGeneration);
				}
			}
			else
			{
				// we need to download this package
				Error = FString::Printf(TEXT("Demo requires missing/mismatched package '%s'"), *Info.PackageName.ToString());
				DemoRecDriver->ServerConnection->Close();
				return;
			}
		}

#if PROLLY_NOT_NEEDED
		// Dependency information.
		FPackageInfo& Info = *new(Connection->PackageMap->List)FPackageInfo(NULL);
		TCHAR PackageName[NAME_SIZE]=TEXT("");
		Parse( Text, TEXT("GUID=" ), Info.Guid );
		Parse( Text, TEXT("GEN=" ),  Info.RemoteGeneration );
		Parse( Text, TEXT("SIZE="),  Info.FileSize );
		Info.DownloadSize = Info.FileSize;
		Parse( Text, TEXT("FLAGS="), Info.PackageFlags );
		Parse( Text, TEXT("PKG="), PackageName, ARRAY_COUNT(PackageName) );
		Parse( Text, TEXT("FNAME="), Info.URL );
		Info.Parent = CreatePackage(NULL,PackageName);
#endif
	}
	else if( ParseCommand( &Text, TEXT("WELCOME") ) )
	{
		// Parse welcome message.
		Parse( Text, TEXT("LEVEL="), URL.Map );

		// only check the package versions if desired
		if (!DemoRecDriver->bShouldSkipPackageChecking)
		{
			UBOOL bHasMissingPackages = FALSE;

			// Make sure all packages we need available
			for( INT i=0; i<Connection->PackageMap->List.Num(); i++ )
			{
				FPackageInfo& Info = Connection->PackageMap->List(i);
				FString Filename;
				if (!GPackageFileCache->FindPackageFile(*Info.PackageName.ToString(), &Info.Guid, Filename, NULL))
				{
					// We need to download this package.
					FilesNeeded++;
					Info.PackageFlags |= PKG_Need;

#if MAYBE_NEEDED
#if !DEMOVERSION
					if( DemoRecDriver->ClientRedirectURLs.Num()==0 || !DemoRecDriver->AllowDownloads || !(Info.PackageFlags & PKG_AllowDownload) )
#endif
#endif
					{
						if (GEngine->GamePlayers.Num() )
						{
							GEngine->GamePlayers(0)->Actor->eventClientMessage( *FString::Printf(*LocalizeError(TEXT("DemoFileMissing"),TEXT("Engine")), *Info.Parent->GetName()), NAME_None );
						}

						Error = FString::Printf( *LocalizeError(TEXT("DemoFileMissing"),TEXT("Engine")), *Info.Parent->GetName() );
						Connection->State = USOCK_Closed;
						bHasMissingPackages = TRUE;
					}
				}
			}

			if( bHasMissingPackages )
			{
				return;
			}

#if MAYBE_NEEDED
#if !DEMOVERSION
			// Send first download request.
			ReceiveNextFile( Connection, 0 );
#endif
#endif
		}

		DemoRecDriver->Time = 0;
		Success = 1;
	}
}

//
// UPendingLevel interface.
//
void UDemoPlayPendingLevel::Tick( FLOAT DeltaTime )
{
	check(DemoRecDriver);
	check(DemoRecDriver->ServerConnection);

	if( DemoRecDriver->ServerConnection && DemoRecDriver->ServerConnection->Download )
	{
		DemoRecDriver->ServerConnection->Download->Tick();
	}

	if( !FilesNeeded )
	{
		// Update demo recording driver.
		DemoRecDriver->UpdateDemoTime( &DeltaTime, 1.f );
		DemoRecDriver->TickDispatch( DeltaTime );
		DemoRecDriver->TickFlush();
	}
}

UNetDriver* UDemoPlayPendingLevel::GetDriver()
{
	return DemoRecDriver;
}

IMPLEMENT_CLASS(UDemoPlayPendingLevel);

