/*=============================================================================
	UnAsyncLoading.cpp: Unreal async loading code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	Async loading stats.
-----------------------------------------------------------------------------*/

DECLARE_STATS_GROUP(TEXT("AsyncIO"),STATGROUP_AsyncIO);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Fulfilled read count"),STAT_AsyncIO_FulfilledReadCount,STATGROUP_AsyncIO);
DECLARE_MEMORY_STAT(TEXT("Fulfilled read size"),STAT_AsyncIO_FulfilledReadSize,STATGROUP_AsyncIO);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Canceled read count"),STAT_AsyncIO_CanceledReadCount,STATGROUP_AsyncIO);
DECLARE_MEMORY_STAT(TEXT("Canceled read size"),STAT_AsyncIO_CanceledReadSize,STATGROUP_AsyncIO);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Outstanding read count"),STAT_AsyncIO_OutstandingReadCount,STATGROUP_AsyncIO);
DECLARE_MEMORY_STAT(TEXT("Outstanding read size"),STAT_AsyncIO_OutstandingReadSize,STATGROUP_AsyncIO);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Uncompressor wait time"),STAT_AsyncIO_UncompressorWaitTime,STATGROUP_AsyncIO);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Main thread block time"),STAT_AsyncIO_MainThreadBlockTime,STATGROUP_AsyncIO);


/*-----------------------------------------------------------------------------
	FAsyncPackage implementation.
-----------------------------------------------------------------------------*/

/**
 * Returns an estimated load completion percentage.
 */
FLOAT FAsyncPackage::GetLoadPercentage() const
{
	return LoadPercentage;
}

/**
 * @return Time load begun. This is NOT the time the load was requested in the case of other pending requests.
 */
DOUBLE FAsyncPackage::GetLoadStartTime() const
{
	return LoadStartTime;
}

/**
 * Emulates UObject::ResetLoaders for the package's Linker objects, hence deleting it. 
 */
void FAsyncPackage::ResetLoader()
{
	// Reset loader.
	if( Linker )
	{
		Linker->Detach( FALSE );
		Linker = NULL;
	}
}

/**
 * Returns whether time limit has been exceeded.
 *
 * @return TRUE if time limit has been exceeded (and is used), FALSE otherwise
 */
UBOOL FAsyncPackage::IsTimeLimitExceeded()
{
	if( !bTimeLimitExceeded && bUseTimeLimit )
	{
		DOUBLE CurrentTime = appSeconds();
		bTimeLimitExceeded = CurrentTime - TickStartTime > TimeLimit;
		if( GUseSeekFreeLoading )
		{
			// Log single operations that take longer than timelimit.
			if( (CurrentTime - TickStartTime) > (2.5 * TimeLimit) )
			{
 				debugfSuppressed(NAME_DevStreaming,TEXT("FAsyncPackage: %s %s took (less than) %5.2f ms"), 
 					LastTypeOfWorkPerformed, 
 					LastObjectWorkWasPerformedOn ? *LastObjectWorkWasPerformedOn->GetFullName() : TEXT(""), 
 					(CurrentTime - TickStartTime) * 1000);
			}
		}
	}
	return bTimeLimitExceeded;
}

/**
 * Gives up time slice if time limit is enabled.
 *
 * @return TRUE if time slice can be given up, FALSE otherwise
 */
UBOOL FAsyncPackage::GiveUpTimeSlice()
{
	bTimeLimitExceeded = bUseTimeLimit;
	return bTimeLimitExceeded;
}

/**
 * Begin async loading process. Simulates parts of UObject::BeginLoad.
 *
 * Objects created during BeginAsyncLoad and EndAsyncLoad will have RF_AsyncLoading set
 */
void FAsyncPackage::BeginAsyncLoad()
{
	// Manually increase the GObjBeginLoadCount as done in BeginLoad();
	UObject::GObjBeginLoadCount++;
	// All objects created from now on should be flagged as RF_AsyncLoading so StaticFindObject doesn't return them.
	GIsAsyncLoading = TRUE;
}

/**
 * End async loading process. Simulates parts of UObject::EndLoad(). FinishObjects 
 * simulates some further parts once we're fully done loading the package.
 */
void FAsyncPackage::EndAsyncLoad()
{
	// We're no longer async loading.
	GIsAsyncLoading = FALSE;
	// Simulate EndLoad();
	UObject::GObjBeginLoadCount--;
}

#if TRACK_DETAILED_ASYNC_STATS
#define DETAILED_ASYNC_STATS_TIMER(x) FScopeSecondsCounter ScopeSecondsCounter##x(x)
#define DETAILED_ASYNC_STATS(x) x
#else
#define DETAILED_ASYNC_STATS(x)
#define DETAILED_ASYNC_STATS_TIMER(x)
#endif

/**
 * Ticks the async loading code.
 *
 * @param	InbUseTimeLimit		Whether to use a time limit
 * @param	InTimeLimit			Soft limit to time this function may take
 *
 * @return	TRUE if package has finished loading, FALSE otherwise
 */
UBOOL FAsyncPackage::Tick( UBOOL InbUseTimeLimit, FLOAT InTimeLimit )
{
	DETAILED_ASYNC_STATS_TIMER( TickTime );
	DETAILED_ASYNC_STATS( TickCount++ );

 	check(LastObjectWorkWasPerformedOn==NULL);
	check(LastTypeOfWorkPerformed==NULL);

	// Set up tick relevant variables.
	bUseTimeLimit			= InbUseTimeLimit;
	bTimeLimitExceeded		= FALSE;
	TimeLimit				= InTimeLimit;
	TickStartTime			= appSeconds();

	// Keep track of time when we start loading.
	if( LoadStartTime == 0.0 )
	{
		LoadStartTime = TickStartTime;
	}

	// Whether we should execute the next step.
	UBOOL bExecuteNextStep	= TRUE;

	// Make sure we finish our work if there's no time limit. The loop is required as PostLoad
	// might cause more objects to be loaded in which case we need to Preload them again.
	do
	{
		DETAILED_ASYNC_STATS( TickLoopCount++ );

		// Reset value to TRUE at beginning of loop.
		bExecuteNextStep	= TRUE;

		// Begin async loading, simulates BeginLoad and sets GIsAsyncLoading to TRUE.
		BeginAsyncLoad();

		// Create raw linker. Needs to be async created via ticking before it can be used.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( CreateLinkerTime );
			bExecuteNextStep = CreateLinker();
		}

		// Async create linker.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( FinishLinkerTime );
			bExecuteNextStep = FinishLinker();
		}

		// Create imports from linker import table.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( CreateImportsTime );
			bExecuteNextStep = CreateImports();
		}
		
		// Create exports from linker export table and also preload them.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( CreateExportsTime );
			bExecuteNextStep = CreateExports();
		}
		
		// Call Preload on the linker for all loaded objects which causes actual serialization.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( PreLoadObjectsTime );
			bExecuteNextStep = PreLoadObjects();
		}
		
		// Call PostLoad on objects, this could cause new objects to be loaded that require
		// another iteration of the PreLoad loop.
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( PostLoadObjectsTime );
			bExecuteNextStep = PostLoadObjects();
		}

		// End async loading, simulates EndLoad and sets GIsAsyncLoading to FALSE.
		EndAsyncLoad();

		// Finish objects (removing RF_AsyncLoading, dissociate imports and forced exports, 
		// call completion callback, ...
		if( bExecuteNextStep )
		{
			DETAILED_ASYNC_STATS_TIMER( FinishObjectsTime );
			bExecuteNextStep = FinishObjects();
		}
	// Only exit loop if we're either done or the time limit has been exceeded.
	} while( !IsTimeLimitExceeded() && !bExecuteNextStep );	

	check( bUseTimeLimit || bExecuteNextStep );

	// We can't have a reference to a UObject.
	LastObjectWorkWasPerformedOn	= NULL;
	// Reset type of work performed.
	LastTypeOfWorkPerformed			= NULL;

	// TRUE means that we're done loading this package.
	return bExecuteNextStep;
}

/**
 * Create linker async. Linker is not finalized at this point.
 *
 * @return TRUE
 */
UBOOL FAsyncPackage::CreateLinker()
{
	if( Linker == NULL )
	{
		DETAILED_ASYNC_STATS(CreateLinkerCount++);

		LastObjectWorkWasPerformedOn	= NULL;
		LastTypeOfWorkPerformed			= TEXT("creating Linker");

		// Try to find existing package or create it if not already present.
		UPackage* Package = UObject::CreatePackage( NULL, *PackageName );
			
		// Retrieve filename on disk for package name. Errors are fatal here.
		FString PackageFileName;
		if( !GPackageFileCache->FindPackageFile( *PackageName, NULL, PackageFileName ) )
		{
			appErrorf(TEXT("Couldn't find file for package %s request by async loading code."),*PackageName);
		}

		// Create raw async linker, requiring to be ticked till finished creating.
		Linker = ULinkerLoad::CreateLinkerAsync( Package, *PackageFileName, (GIsGame && !GIsEditor) ? (LOAD_SeekFree | LOAD_NoVerify) : LOAD_None  );
	}
	return TRUE;
}

/**
 * Finalizes linker creation till time limit is exceeded.
 *
 * @return TRUE if linker is finished being created, FALSE otherwise
 */
UBOOL FAsyncPackage::FinishLinker()
{
	if( !Linker->HasFinishedInitializtion() )
	{
		DETAILED_ASYNC_STATS(FinishLinkerCount++);
		LastObjectWorkWasPerformedOn	= Linker->LinkerRoot;
		LastTypeOfWorkPerformed			= TEXT("ticking linker");
	
		// Operation still pending if Tick returns FALSE
		if( !Linker->Tick( TimeLimit, bUseTimeLimit ) )
		{
			// Give up remainder of timeslice if there is one to give up.
			GiveUpTimeSlice();
			return FALSE;
		}
	}
	return TRUE;
}

/** 
 * Create imports till time limit is exceeded.
 *
 * @return TRUE if we finished creating all imports, FALSE otherwise
 */
UBOOL FAsyncPackage::CreateImports()
{
	DETAILED_ASYNC_STATS( if( ImportIndex < Linker->ImportMap.Num() ) CreateImportsCount++; );

	// Create imports.
	while( ImportIndex < Linker->ImportMap.Num() && !IsTimeLimitExceeded() )
	{
 		UObject* Object	= Linker->CreateImport( ImportIndex++ );
		LastObjectWorkWasPerformedOn	= Object;
		LastTypeOfWorkPerformed			= TEXT("creating imports for");
	}

	return ImportIndex == Linker->ImportMap.Num();
}

/**
 * Create exports till time limit is exceeded.
 *
 * @return TRUE if we finished creating and preloading all exports, FALSE otherwise.
 */
UBOOL FAsyncPackage::CreateExports()
{
	DETAILED_ASYNC_STATS( if( ExportIndex < Linker->ExportMap.Num() ) CreateExportsCount++; );

	// Create exports.
	while( ExportIndex < Linker->ExportMap.Num() && !IsTimeLimitExceeded() )
	{
		const FObjectExport& Export = Linker->ExportMap(ExportIndex);
		
		// Precache data and see whether it's already finished.

		// We have sufficient data in the cache so we can load.
		if( Linker->Precache( Export.SerialOffset, Export.SerialSize ) )
		{
			// Create the object...
			UObject* Object	= Linker->CreateExport( ExportIndex++ );
			// ... and preload it.
			if( Object )
			{
				// This will cause the object to be serialized. We do this here for all objects and
				// not just UClass and template objects, for which this is required in order to ensure
				// seek free loading, to be able introduce async file I/O.
				Linker->Preload( Object );
			}
			LastObjectWorkWasPerformedOn	= Object;
			LastTypeOfWorkPerformed			= TEXT("creating exports for");
			// This assumes that only CreateExports is taking a significant amount of time.
			LoadPercentage = 100.f * ExportIndex / Linker->ExportMap.Num();
		}
		// Data isn't ready yet. Give up remainder of time slice if we're not using a time limit.
		else if( GiveUpTimeSlice() )
		{
			return FALSE;
		}
	}
	
	return ExportIndex == Linker->ExportMap.Num();
}

/**
 * Preloads aka serializes all loaded objects.
 *
 * @return TRUE if we finished serializing all loaded objects, FALSE otherwise.
 */
UBOOL FAsyncPackage::PreLoadObjects()
{
	DETAILED_ASYNC_STATS( if( PreLoadIndex < UObject::GObjLoaded.Num() ) PreLoadObjectsCount++; );

	// Preload (aka serialize) the objects.
	while( PreLoadIndex < UObject::GObjLoaded.Num() && !IsTimeLimitExceeded() )
	{
		//@todo async: make this part async as well.
		UObject* Object = UObject::GObjLoaded( PreLoadIndex++ );
		check( Object && Object->GetLinker() );
		Object->GetLinker()->Preload( Object );
		LastObjectWorkWasPerformedOn	= Object;
		LastTypeOfWorkPerformed			= TEXT("preloading");
	}

	return PreLoadIndex == UObject::GObjLoaded.Num();
}

/**
 * Route PostLoad to all loaded objects. This might load further objects!
 *
 * @return TRUE if we finished calling PostLoad on all loaded objects and no new ones were created, FALSE otherwise
 */
UBOOL FAsyncPackage::PostLoadObjects()
{
	DETAILED_ASYNC_STATS( if( PostLoadIndex < UObject::GObjLoaded.Num() ) PostLoadObjectsCount++; );

	// PostLoad objects.
	while( PostLoadIndex < UObject::GObjLoaded.Num() && !IsTimeLimitExceeded() )
	{
		UObject* Object	= UObject::GObjLoaded( PostLoadIndex++ );
		check(Object);
		Object->ConditionalPostLoad();
		LastObjectWorkWasPerformedOn	= Object;
		LastTypeOfWorkPerformed			= TEXT("postloading");
	}

	// New objects might have been loaded during PostLoad.
	return (PreLoadIndex == UObject::GObjLoaded.Num()) && (PostLoadIndex == UObject::GObjLoaded.Num());
}

/**
 * Finish up objects and state, which means clearing the RF_AsyncLoading flag on newly created ones
 *
 * @return TRUE
 */
UBOOL FAsyncPackage::FinishObjects()
{
	DETAILED_ASYNC_STATS(FinishObjectsCount++);
	LastObjectWorkWasPerformedOn	= NULL;
	LastTypeOfWorkPerformed			= TEXT("finishing all objects");

	// All loaded objects are now finished loading so we can clear the RF_AsyncLoading flag so
	// StaticFindObject returns them by default.
	for( INT ObjectIndex=0; ObjectIndex<UObject::GObjConstructedDuringAsyncLoading.Num(); ObjectIndex++ )
	{
		UObject* Object = UObject::GObjConstructedDuringAsyncLoading(ObjectIndex);
		Object->ClearFlags( RF_AsyncLoading );
	}
	UObject::GObjConstructedDuringAsyncLoading.Empty();
			
	// Simulate what UObject::EndLoad does.
	UObject::GObjLoaded.Empty();
	UObject::DissociateImportsAndForcedExports(); //@todo: this should be avoidable

	// Mark package as having been fully loaded and update load time.
	if( Linker->LinkerRoot )
	{
		Linker->LinkerRoot->MarkAsFullyLoaded();
		Linker->LinkerRoot->SetLoadTime( appSeconds() - LoadStartTime );
	}

	// Call any completion callbacks specified.
	for (INT i = 0; i < CompletionCallbacks.Num(); i++)
	{
		(*CompletionCallbacks(i).Callback)(Linker->LinkerRoot, CompletionCallbacks(i).UserData);
	}

	return TRUE;
}


/*-----------------------------------------------------------------------------
	UObject async (pre)loading.
-----------------------------------------------------------------------------*/

/**
 * Asynchronously load a package and all contained objects that match context flags. Non- blocking.
 *
 * @param	InPackageName		Name of package to load
 * @param	CompletionCallback	Callback called on completion of loading
 * @param	CallbackUserData	User data passed to callback
 */
void UObject::LoadPackageAsync( const FString& InPackageName, FAsyncCompletionCallback CompletionCallback, void* CallbackUserData )
{
	// The comments clearly state that it should be a package name but we also handle it being a filename as this function is not perf critical
	// and LoadPackage handles having a filename being passed in as well.
	FString PackageName = FFilename(InPackageName).GetBaseFilename();

	// Check whether the file is already in the queue.
	for( INT PackageIndex=0; PackageIndex<GObjAsyncPackages.Num(); PackageIndex++ )
	{
		FAsyncPackage& PendingPackage = GObjAsyncPackages(PackageIndex);
		if( PendingPackage.GetPackageName() == PackageName )
		{
			// make sure package has this completion callback
			if (CompletionCallback != NULL)
			{
				PendingPackage.AddCompletionCallback(FAsyncCompletionCallbackInfo(CompletionCallback, CallbackUserData));
			}
			// Early out as package is already being preloaded.
			return;
		}
	}
	// Add to (FIFO) queue.
	FAsyncPackage Package(PackageName);
	if (CompletionCallback != NULL)
	{
		Package.AddCompletionCallback(FAsyncCompletionCallbackInfo(CompletionCallback, CallbackUserData));
	}
	GObjAsyncPackages.AddItem( Package );
}

/**
 * Returns the async load percentage for a package in flight with the passed in name or -1 if there isn't one.
 *
 * @param	PackageName			Name of package to query load percentage for
 * @return	Async load percentage if package is currently being loaded, -1 otherwise
 */
FLOAT UObject::GetAsyncLoadPercentage( const FString& PackageName )
{
	FLOAT LoadPercentage = -1.f;
	for( INT PackageIndex=0; PackageIndex<GObjAsyncPackages.Num(); PackageIndex++ )
	{
		const FAsyncPackage& PendingPackage = GObjAsyncPackages(PackageIndex);
		if( PendingPackage.GetPackageName() == PackageName )
		{
			LoadPercentage = PendingPackage.GetLoadPercentage();
			break;
		}
	}
	return LoadPercentage;
}


/**
 * Blocks till all pending package/ linker requests are fulfilled.
 */
void UObject::FlushAsyncLoading()
{
	if( GObjAsyncPackages.Num() )
	{
		// Disallow low priority requests like texture streaming while we are flushing streaming
		// in order to avoid excessive seeking.
		FIOSystem* AsyncIO = GIOManager->GetIOSystem( IOSYSTEM_GenericAsync );
		AsyncIO->SetMinPriority( AIOP_Normal );

		// Flush async loaders by not using a time limit. Needed for e.g. garbage collection.
		debugf( NAME_Log, TEXT("Flushing async loaders.") );
		ProcessAsyncLoading( FALSE, 0 );
		check( !IsAsyncLoading() );

		// Reset min priority again.
		AsyncIO->SetMinPriority( AIOP_MIN );
	}
}

/**
 * Returns whether we are currently async loading a package.
 *
 * @return TRUE if we are async loading a package, FALSE otherwise
 */
UBOOL UObject::IsAsyncLoading()
{
	return GObjAsyncPackages.Num() > 0 ? TRUE : FALSE;
}

DECLARE_CYCLE_STAT(TEXT("Async Loading Time"),STAT_AsyncLoadingTime,STATGROUP_Streaming);

/**
 * Serializes a bit of data each frame with a soft time limit. The function is designed to be able
 * to fully load a package in a single pass given sufficient time.
 *
 * @param	bUseTimeLimit	Whether to use a time limit
 * @param	TimeLimit		Soft limit of time this function is allowed to consume
 */
void UObject::ProcessAsyncLoading( UBOOL bUseTimeLimit, FLOAT TimeLimit )
{
	SCOPE_CYCLE_COUNTER(STAT_AsyncLoadingTime);
	// Whether to continue execution.
	UBOOL bExecuteNextStep = TRUE;

	// We need to loop as the function has to handle finish loading everything given no time limit
	// like e.g. when called from FlushAsyncLoading.
	while( bExecuteNextStep && GObjAsyncPackages.Num() )
	{
		// Package to be loaded.
		FAsyncPackage& Package = GObjAsyncPackages(0);

		// Package tick returns TRUE on completion.
		bExecuteNextStep = Package.Tick( bUseTimeLimit, TimeLimit );
		if( bExecuteNextStep )
		{
#if TRACK_DETAILED_ASYNC_STATS
			DOUBLE LoadTime = appSeconds() - Package.GetLoadStartTime();
			debugfSuppressed( NAME_DevStreaming, TEXT("Detailed async load stats for package '%s', finished loading in %5.2f seconds"), *Package.GetPackageName(), LoadTime );
			debugfSuppressed( NAME_DevStreaming, TEXT("Tick             : %6.2f ms [%3i, %3i iterations]"), Package.TickTime * 1000, Package.TickCount, Package.TickLoopCount );
			debugfSuppressed( NAME_DevStreaming, TEXT("CreateLinker     : %6.2f ms [%3i iterations]"), Package.CreateLinkerTime    * 1000, Package.CreateLinkerCount    );
			debugfSuppressed( NAME_DevStreaming, TEXT("FinishLinker     : %6.2f ms [%3i iterations]"), Package.FinishLinkerTime    * 1000, Package.FinishLinkerCount    );
			debugfSuppressed( NAME_DevStreaming, TEXT("CreateImports    : %6.2f ms [%3i iterations]"), Package.CreateImportsTime	 * 1000, Package.CreateImportsCount   );
			debugfSuppressed( NAME_DevStreaming, TEXT("CreateExports    : %6.2f ms [%3i iterations]"), Package.CreateExportsTime   * 1000, Package.CreateExportsCount   );
			debugfSuppressed( NAME_DevStreaming, TEXT("PreLoadObjects   : %6.2f ms [%3i iterations]"), Package.PreLoadObjectsTime  * 1000, Package.PreLoadObjectsCount  );
			debugfSuppressed( NAME_DevStreaming, TEXT("PostLoadObjects  : %6.2f ms [%3i iterations]"), Package.PostLoadObjectsTime * 1000, Package.PostLoadObjectsCount );
			debugfSuppressed( NAME_DevStreaming, TEXT("FinishObjects    : %6.2f ms [%3i iterations]"), Package.FinishObjectsTime   * 1000, Package.FinishObjectsCount   );
#endif
	
			if( GUseSeekFreeLoading )
			{
				// Emulates UObject::ResetLoaders on the package linker's linkerroot.
				Package.ResetLoader();
			}

			// We're done so we can remove the package now. @warning invalidates local Package variable!.
			GObjAsyncPackages.Remove( 0 );
		}

		// We cannot access Package anymore!
	}
}


/*-----------------------------------------------------------------------------
	FAsyncIOSystemBase implementation.
-----------------------------------------------------------------------------*/

#define BLOCK_ON_ASYNCIO 0

/**
 * Packs IO request into a FAsyncIORequest and queues it in OutstandingRequests array
 *
 * @param	FileName			File name associated with request
 * @param	Offset				Offset in bytes from beginning of file to start reading from
 * @param	Size				Number of bytes to read
 * @param	UncompressedSize	Uncompressed size in bytes of original data, 0 if data is not compressed on disc	
 * @param	Dest				Pointer holding to be read data
 * @param	CompressionFlags	Flags controlling data decompression
 * @param	Counter				Thread safe counter associated with this request; will be decremented when fulfilled
 * @param	Priority			Priority of request
 * 
 * @return	unique ID for request
 */
QWORD FAsyncIOSystemBase::QueueIORequest( 
	const FString& FileName, 
	INT Offset, 
	INT Size, 
	INT UncompressedSize, 
	void* Dest, 
	ECompressionFlags CompressionFlags, 
	FThreadSafeCounter* Counter,
	EAsyncIOPriority Priority )
{
	// Create an IO request containing passed in information.
	FAsyncIORequest IORequest;
	IORequest.RequestIndex			= RequestIndex++;
	IORequest.FileStartSector		= GFileManager->GetFileStartSector( *FileName );
	IORequest.FileName				= FileName;
	IORequest.Offset				= Offset;
	IORequest.Size					= Size;
	IORequest.UncompressedSize		= UncompressedSize;
	IORequest.Dest					= Dest;
	IORequest.CompressionFlags		= CompressionFlags;
	IORequest.Counter				= Counter;
	IORequest.Priority				= Priority;

	INC_DWORD_STAT( STAT_AsyncIO_OutstandingReadCount );
	INC_DWORD_STAT_BY( STAT_AsyncIO_OutstandingReadSize, IORequest.Size );

	// Add to end of queue.
	OutstandingRequests.AddItem( IORequest );

	// Trigger event telling IO thread to wake up to perform work.
	OutstandingRequestsEvent->Trigger();

	// Return unique ID associated with request which can be used to cancel it.
	return IORequest.RequestIndex;
}

/**
 * Fulfills a compressed read request in a blocking fashion by falling back to using
 * PlatformSeek and various PlatformReads in combination with FAsyncUncompress to allow
 * decompression while we are waiting for file I/O.
 *
 * @note: the way this code works needs to be in line with FArchive::SerializeCompressed
 */
void FAsyncIOSystemBase::FulfillCompressedRead( const FAsyncIORequest& IORequest )
{
	// Initialize variables.
	FAsyncUncompress*		Uncompressor			= NULL;
	BYTE*					UncompressedBuffer		= (BYTE*) IORequest.Dest;
	// First compression chunk contains information about total size so we skip that one.
	INT						CurrentChunkIndex		= 1;
	INT						CurrentBufferIndex		= 0;
	UBOOL					bHasProcessedAllData	= FALSE;

	// read the first two ints, which will contain the magic bytes (to detect byteswapping)
	// and the original size the chunks were compressed from
	INT						HeaderData[2];
	INT						HeaderSize = sizeof(HeaderData);

	PlatformRead(IORequest.FileHandle, IORequest.Offset, HeaderSize, HeaderData);

	// if the magic bytes don't match, then we are byteswapped (or corrupted)
	UBOOL bIsByteswapped = HeaderData[0] != PACKAGE_FILE_TAG;
	// if its potentially byteswapped, make sure it's not just corrupted
	if (bIsByteswapped)
	{
		// if it doesn't euqal the swapped version, then dta is corrupted
		if (HeaderData[0] != PACKAGE_FILE_TAG_SWAPPED)
		{
#if DO_CHECK
			appErrorf( TEXT("Detected data corruption trying to read %i bytes at offset %i from '%s'. Please delete file and recook."),
				IORequest.UncompressedSize, 
				IORequest.Offset ,
				*IORequest.FileName );
#else
			// Try to handle it gracefully by returning all 0s.
			appMemzero( IORequest.Dest, IORequest.UncompressedSize );
			return;
#endif
		}
		// otherwise, we have a valid byteswapped file, so swap the chunk size
		else
		{
			HeaderData[1] = BYTESWAP_ORDER32(HeaderData[1]);
		}
	}

	INT						CompressionChunkSize	= HeaderData[1];
	
	// handle old packages that don't have the chunk size in the header, in which case
	// we can use the old hardcoded size
	if (CompressionChunkSize == PACKAGE_FILE_TAG)
	{
		CompressionChunkSize = LOADING_COMPRESSION_CHUNK_SIZE;
	}

	// calculate the number of chunks based on the size they were compressed from
	INT						TotalChunkCount = (IORequest.UncompressedSize + CompressionChunkSize - 1) / CompressionChunkSize + 1;

	// allocate chunk info data based on number of chunks
	FCompressedChunkInfo*	CompressionChunks		= new FCompressedChunkInfo[TotalChunkCount];
	INT						ChunkInfoSize			= (TotalChunkCount) * sizeof(FCompressedChunkInfo);
	void*					CompressedBuffer[2];
	
	// Read table of compression chunks after seeking to offset (after the initial header data)
	PlatformRead( IORequest.FileHandle, IORequest.Offset + sizeof(HeaderData), ChunkInfoSize, CompressionChunks );

	// Handle byte swapping. This is required for opening a cooked file on the PC.
	if (bIsByteswapped)
	{
		for( INT ChunkIndex=0; ChunkIndex<TotalChunkCount; ChunkIndex++ )
		{
			CompressionChunks[ChunkIndex].CompressedSize	= BYTESWAP_ORDER32(CompressionChunks[ChunkIndex].CompressedSize);
			CompressionChunks[ChunkIndex].UncompressedSize	= BYTESWAP_ORDER32(CompressionChunks[ChunkIndex].UncompressedSize);
		}
	}

#if DO_CHECK
	// We only check for IO request undershooting as it is safe to request more, which e.g. will happen when fully reading a file
	// that had its size aligned by padding.
	if (ChunkInfoSize + HeaderSize + CompressionChunks[0].CompressedSize > IORequest.Size )
	{
		appErrorf( TEXT("Detected data corruption trying to read %i bytes at offset %i from '%s'. Please delete file and recook."),
			IORequest.UncompressedSize, 
			IORequest.Offset ,
			*IORequest.FileName );
	}
#endif

	// Figure out maximum size of compressed data chunk.
	INT MaxCompressedSize = 0;
	for (INT ChunkIndex = 1; ChunkIndex < TotalChunkCount; ChunkIndex++)
	{
		MaxCompressedSize = Max(MaxCompressedSize, CompressionChunks[ChunkIndex].CompressedSize);
		check( CompressionChunks[ChunkIndex].UncompressedSize <= CompressionChunkSize );
	}

	// Allocate memory for compressed data.
	CompressedBuffer[0]	= appMalloc( MaxCompressedSize );
	CompressedBuffer[1] = appMalloc( MaxCompressedSize );

	// Initial read request.
	PlatformRead( IORequest.FileHandle, INDEX_NONE, CompressionChunks[CurrentChunkIndex].CompressedSize, CompressedBuffer[CurrentBufferIndex] );

	// Loop till we're done decompressing all data.
	while( !bHasProcessedAllData )
	{
		// Create uncompressor object performing async decompression.
		Uncompressor = new FAsyncUncompress	(
								IORequest.CompressionFlags,
								UncompressedBuffer,
								CompressionChunks[CurrentChunkIndex].UncompressedSize,
								CompressedBuffer[CurrentBufferIndex],
								CompressionChunks[CurrentChunkIndex].CompressedSize
											);
		// Add to queued work pool for processing.
		GThreadPool->AddQueuedWork( Uncompressor );

		// Advance destination pointer.
		UncompressedBuffer += CompressionChunks[CurrentChunkIndex].UncompressedSize;
	
		// Check whether we are already done reading.
		if( CurrentChunkIndex < TotalChunkCount-1 )
		{
			// Can't postincrement in if statement as we need it to remain at valid value for one more loop iteration to finish
			// the decompression.
			CurrentChunkIndex++;
			// Swap compression buffers to read into.
			CurrentBufferIndex = 1 - CurrentBufferIndex;
			// Read more data.
			PlatformRead( IORequest.FileHandle, INDEX_NONE, CompressionChunks[CurrentChunkIndex].CompressedSize, CompressedBuffer[CurrentBufferIndex] );
		}
		// We were already done reading the last time around so we are done processing now.
		else
		{
			bHasProcessedAllData = TRUE;
		}
		
		//@todo async loading: should use event for this
		STAT(DOUBLE UncompressorWaitTime = 0);
		{
			SCOPE_SECONDS_COUNTER(UncompressorWaitTime);
			// Sync with decompressor.
			while( !Uncompressor->IsDone() )
			{
				appSleep( 0 );
			}
		}
		INC_FLOAT_STAT_BY(STAT_AsyncIO_UncompressorWaitTime,(FLOAT)UncompressorWaitTime);
		delete Uncompressor;
		Uncompressor = NULL;
	}

	delete [] CompressionChunks;
	appFree( CompressedBuffer[0] );
	appFree( CompressedBuffer[1] );
}

/**
 * Retrieves cached file handle or caches it if it hasn't been already
 *
 * @param	FileName	file name to retrieve cached handle for
 * @return	cached file handle
 */
FAsyncIOHandle FAsyncIOSystemBase::GetCachedFileHandle( const FString& FileName )
{
	// We can't make any assumptions about NULL being an invalid handle value so we need to use the indirection.
	FAsyncIOHandle*	FileHandlePtr	= NameToHandleMap.Find( FileName );
	FAsyncIOHandle	FileHandle;

	// We have an already cached handle, let's use it.
	if( FileHandlePtr )
	{
		FileHandle = *FileHandlePtr;
	}
	// The filename doesn't have a handle associated with it.
	else
	{
		// So let the platform specific code create one.
		FileHandle = PlatformCreateHandle( *FileName );
		// Make sure it's valid before caching and using it.
		if( PlatformIsHandleValid( FileHandle ) )
		{
			NameToHandleMap.Set( *FileName, FileHandle );
		}
	}

	return FileHandle;
}

/**
 * Requests data to be loaded async. Returns immediately.
 *
 * @param	FileName	File name to load
 * @param	Offset		Offset into file
 * @param	Size		Size of load request
 * @param	Dest		Pointer to load data into
 * @param	Counter		Thread safe counter to decrement when loading has finished
 * @param	Priority	Priority of request
 *
 * @return Returns an index to the request that can be used for canceling or 0 if the request failed.
 */
QWORD FAsyncIOSystemBase::LoadData( 
	const FString& FileName, 
	INT Offset, 
	INT Size, 
	void* Dest, 
	FThreadSafeCounter* Counter,
	EAsyncIOPriority Priority )
{
	QWORD TheRequestIndex;
	{
		FScopeLock ScopeLock( CriticalSection );	
		TheRequestIndex = QueueIORequest( FileName, Offset, Size, 0, Dest, COMPRESS_None, Counter, Priority );
	}
#if BLOCK_ON_ASYNCIO
	BlockTillAllRequestsFinished(); 
#endif
	return TheRequestIndex;
}

/**
 * Requests compressed data to be loaded async. Returns immediately.
 *
 * @param	FileName			File name to load
 * @param	Offset				Offset into file
 * @param	Size				Size of load request
 * @param	UncompressedSize	Size of uncompressed data
 * @param	Dest				Pointer to load data into
 * @param	CompressionFlags	Flags controlling data decompression
 * @param	Counter				Thread safe counter to decrement when loading has finished, can be NULL
 * @param	Priority			Priority of request
 *
 * @return Returns an index to the request that can be used for canceling or 0 if the request failed.
 */
QWORD FAsyncIOSystemBase::LoadCompressedData( 
	const FString& FileName, 
	INT Offset, 
	INT Size, 
	INT UncompressedSize, 
	void* Dest, 
	ECompressionFlags CompressionFlags, 
	FThreadSafeCounter* Counter,
	EAsyncIOPriority Priority )
{
	QWORD TheRequestIndex;
	{
		FScopeLock ScopeLock( CriticalSection );	
		TheRequestIndex = QueueIORequest( FileName, Offset, Size, UncompressedSize, Dest, CompressionFlags, Counter, Priority );
	}
#if BLOCK_ON_ASYNCIO
	BlockTillAllRequestsFinished(); 
#endif
	return TheRequestIndex;
}

/**
 * Removes N outstanding requests from the queue in an atomic all or nothing operation.
 * NOTE: Requests are only canceled if ALL requests are still pending and neither one
 * is currently in flight.
 *
 * @param	RequestIndices	Indices of requests to cancel.
 * @return	TRUE if all requests were still outstanding, FALSE otherwise
 */
UBOOL FAsyncIOSystemBase::CancelRequests( QWORD* RequestIndices, INT NumIndices )
{
	FScopeLock ScopeLock( CriticalSection );

	INT		NumFound			= 0;
	INT*	OutstandingIndices	= (INT*) appAlloca( sizeof(INT) * NumIndices );

	for( INT OutstandingIndex=0; OutstandingIndex<OutstandingRequests.Num() && NumFound<NumIndices; OutstandingIndex++ )
	{
		for( INT TheRequestIndex=0; TheRequestIndex<NumIndices; TheRequestIndex++ )
		{
			const FAsyncIORequest IORequest = OutstandingRequests(OutstandingIndex);
			if( IORequest.RequestIndex == RequestIndices[TheRequestIndex] )
			{
				OutstandingIndices[NumFound] = OutstandingIndex;
				NumFound++;
			}
		}
	}

	if( NumFound == NumIndices )
	{
		for( INT TheRequestIndex=0; TheRequestIndex<NumFound; TheRequestIndex++ )
		{
#if STATS
			// IORequest only valid till remove below.
			const FAsyncIORequest IORequest = OutstandingRequests( OutstandingIndices[TheRequestIndex] - TheRequestIndex );
			INC_DWORD_STAT( STAT_AsyncIO_CanceledReadCount );
			INC_DWORD_STAT_BY( STAT_AsyncIO_CanceledReadSize, IORequest.Size );
			DEC_DWORD_STAT( STAT_AsyncIO_OutstandingReadCount );
			DEC_DWORD_STAT_BY( STAT_AsyncIO_OutstandingReadSize, IORequest.Size );
#endif
			OutstandingRequests.Remove( OutstandingIndices[TheRequestIndex] - TheRequestIndex );
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * Constrains bandwidth to value set in .ini
 *
 * @param BytesRead		Number of bytes read
 * @param ReadTime		Time it took to read
 */
void FAsyncIOSystemBase::ConstrainBandwidth( INT BytesRead, FLOAT ReadTime )
{
	// Constrain bandwidth if wanted. Value is in MByte/ sec.
	if( GSys->AsyncIOBandwidthLimit > 0 )
	{
		// Figure out how long to wait to throttle bandwidth.
		FLOAT WaitTime = BytesRead / (GSys->AsyncIOBandwidthLimit * 1024.f * 1024.f) - ReadTime;
		// Only wait if there is something worth waiting for.
		if( WaitTime > 0 )
		{
			// Time in seconds to wait.
			appSleep(WaitTime);
		}
	}
}

/**
 * Initializes critical section, event and other used variables. 
 *
 * This is called in the context of the thread object that aggregates 
 * this, not the thread that passes this runnable to a new thread.
 *
 * @return True if initialization was successful, false otherwise
 */
UBOOL FAsyncIOSystemBase::Init()
{
	CriticalSection				= GSynchronizeFactory->CreateCriticalSection();
	OutstandingRequestsEvent	= GSynchronizeFactory->CreateSynchEvent();
	RequestIndex				= 1;
	MinPriority					= AIOP_MIN;
	IsRunning.Increment();
	return TRUE;
}

/**
 * This is called if a thread is requested to suspend its' IO activity
 */
void FAsyncIOSystemBase::Suspend()
{
	SuspendCount.Increment();
}

/**
 * This is called if a thread is requested to resume its' IO activity
 */
void FAsyncIOSystemBase::Resume()
{
	SuspendCount.Decrement();
}

/**
 * Sets the min priority of requests to fulfill. Lower priority requests will still be queued and
 * start to be fulfilled once the min priority is lowered sufficiently. This is only a hint and
 * implementations are free to ignore it.
 *
 * @param	InMinPriority		Min priority of requests to fulfill
 */
void FAsyncIOSystemBase::SetMinPriority( EAsyncIOPriority InMinPriority )
{
	FScopeLock ScopeLock( CriticalSection );
	// Trigger event telling IO thread to wake up to perform work if we are lowering the min priority.
	if( InMinPriority < MinPriority )
	{
		OutstandingRequestsEvent->Trigger();
	}
	// Update priority.
	MinPriority = InMinPriority;
}

/**
 * Called in the context of the aggregating thread to perform cleanup.
 */
void FAsyncIOSystemBase::Exit()
{
	FlushHandles();
	GSynchronizeFactory->Destroy( CriticalSection );
	GSynchronizeFactory->Destroy( OutstandingRequestsEvent );
}

/**
 * This is called if a thread is requested to terminate early
 */
void FAsyncIOSystemBase::Stop()
{
	// Make sure that the thread awakens even if there is no work currently outstanding.
	OutstandingRequestsEvent->Trigger();
	IsRunning.Decrement();
}

/**
 * This is where all the actual loading is done. This is only called
 * if the initialization was successful.
 *
 * @return always 0
 */
DWORD FAsyncIOSystemBase::Run()
{
	// IsRunning gets decremented by Stop.
	while( IsRunning.GetValue() > 0 )
	{
		// Copy of read request.
		FAsyncIORequest IORequest				= {0};
		UBOOL			bIsReadRequestPending	= FALSE;
		{
			FScopeLock ScopeLock( CriticalSection );
			if( OutstandingRequests.Num() )
			{
				// Gets next request index based on platform specific criteria like layout on disc.
				INT TheRequestIndex = PlatformGetNextRequestIndex();
				if( TheRequestIndex != INDEX_NONE )
				{					
					// We need to copy as we're going to remove it...
					IORequest = OutstandingRequests( TheRequestIndex );
					// ...right here.
					OutstandingRequests.Remove( TheRequestIndex );		
					// We're busy reading. Updated inside scoped lock to ensure BlockTillAllRequestsFinished works correctly.
					BusyReading.Increment();
					bIsReadRequestPending = TRUE;
				}
			}
		}

		// We only have work to do if there's a v
		if( bIsReadRequestPending )
		{
			// Retrieve cached handle or create it if it wasn't cached.
			IORequest.FileHandle = GetCachedFileHandle( IORequest.FileName );
			if( PlatformIsHandleValid(IORequest.FileHandle) )
			{
				if( IORequest.UncompressedSize )
				{
					// Data is compressed on disc so we need to also decompress.
					FulfillCompressedRead( IORequest );
				}
				else
				{
					// Read data after seeking.
					PlatformRead( IORequest.FileHandle, IORequest.Offset, IORequest.Size, IORequest.Dest );
				}
				INC_DWORD_STAT( STAT_AsyncIO_FulfilledReadCount );
				INC_DWORD_STAT_BY( STAT_AsyncIO_FulfilledReadSize, IORequest.Size );
			}
			else
			{
				//@todo streaming: add warning once we have thread safe logging.
			}

			DEC_DWORD_STAT( STAT_AsyncIO_OutstandingReadCount );
			DEC_DWORD_STAT_BY( STAT_AsyncIO_OutstandingReadSize, IORequest.Size );

			// Request fulfilled.
			IORequest.Counter->Decrement(); 
			// We're done reading for now.
			BusyReading.Decrement();	
		}
		else
		{
			// Wait till the calling thread signals further work.
			OutstandingRequestsEvent->Wait();
		}
	}

	return 0;
}

/**
 * Blocks till all requests are finished.
 *
 * @todo streaming: this needs to adjusted to signal the thread to not accept any new requests from other 
 * @todo streaming: threads while we are blocking till all requests are finished.
 */
void FAsyncIOSystemBase::BlockTillAllRequestsFinished()
{
	// Block till all requests are fulfilled.
	while( TRUE ) 
	{
		UBOOL bHasFinishedReading = FALSE;
		{
			FScopeLock ScopeLock( CriticalSection );
			bHasFinishedReading = (OutstandingRequests.Num() == 0) && (BusyReading.GetValue() == 0);
		}	
		if( bHasFinishedReading )
		{
			break;
		}
		else
		{
			//@todo streaming: this should be replaced by waiting for an event.
			appSleep( 0.01f );
		}
	}
}

/**
 * Blocks till all requests are finished and also flushes potentially open handles.
 */
void FAsyncIOSystemBase::BlockTillAllRequestsFinishedAndFlushHandles()
{
	// Block till all requests are fulfilled.
	BlockTillAllRequestsFinished();

	// Flush all file handles.
	FlushHandles();
}

/**
 * Flushes all file handles.
 */
void FAsyncIOSystemBase::FlushHandles()
{
	FScopeLock ScopeLock( CriticalSection );
	// Iterate over all file handles, destroy them and empty name to handle map.
	for( TMap<FString,FAsyncIOHandle>::TIterator It(NameToHandleMap); It; ++It )
	{
		PlatformDestroyHandle( It.Value() );
	}
	NameToHandleMap.Empty();
}

/*----------------------------------------------------------------------------
	FArchiveAsync.
----------------------------------------------------------------------------*/

/**
 * Constructor, initializing all member variables.
 */
FArchiveAsync::FArchiveAsync( const TCHAR* InFileName )
:	FileName					( InFileName	)
,	FileSize					( INDEX_NONE	)
,	UncompressedFileSize		( INDEX_NONE	)
,	CurrentPos					( 0				)
,	CompressedChunks			( NULL			)
,	CurrentChunkIndex			( 0				)
,	CompressionFlags			( COMPRESS_None	)
{
	ArIsLoading		= TRUE;
	ArIsPersistent	= TRUE;

	PrecacheStartPos[CURRENT]	= 0;
	PrecacheEndPos[CURRENT]		= 0;
	PrecacheBuffer[CURRENT]		= NULL;

	PrecacheStartPos[NEXT]		= 0;
	PrecacheEndPos[NEXT]		= 0;
	PrecacheBuffer[NEXT]		= NULL;

	// Relies on default constructor initializing to 0.
	check( PrecacheReadStatus[CURRENT].GetValue() == 0 );
	check( PrecacheReadStatus[NEXT].GetValue() == 0 );

	// Cache file size.
	FileSize = GFileManager->FileSize( *FileName );
	
	// Check whether file existed.
	if( FileSize >= 0 )
	{
		// No error.
		ArIsError	= FALSE;

		// Retrieved uncompressed file size.
		UncompressedFileSize = GFileManager->UncompressedFileSize( *FileName );

		// Package wasn't compressed so use regular file size.
		if( UncompressedFileSize == INDEX_NONE )
		{
			UncompressedFileSize = FileSize;
		}
	}
	else
	{
		// Couldn't open the file.
		ArIsError	= TRUE;
	}
}

/**
 * Flushes cache and frees internal data.
 */
void FArchiveAsync::FlushCache()
{
	// Wait on all outstanding requests.
	while( PrecacheReadStatus[CURRENT].GetValue() || PrecacheReadStatus[NEXT].GetValue() )
	{
		appSleep(0);
	}

	// Invalidate any precached data and free memory for current buffer.
	appFree( PrecacheBuffer[CURRENT] );
	PrecacheBuffer[CURRENT]		= NULL;
	PrecacheStartPos[CURRENT]	= 0;
	PrecacheEndPos[CURRENT]		= 0;
	
	// Invalidate any precached data and free memory for next buffer.
	appFree( PrecacheBuffer[NEXT] );
	PrecacheBuffer[NEXT]		= NULL;
	PrecacheStartPos[NEXT]		= 0;
	PrecacheEndPos[NEXT]		= 0;
}

/**
 * Virtual destructor cleaning up internal file reader.
 */
FArchiveAsync::~FArchiveAsync()
{
	// Invalidate any precached data and free memory.
	FlushCache();
}

/**
 * Close archive and return whether there has been an error.
 *
 * @return	TRUE if there were NO errors, FALSE otherwise
 */
UBOOL FArchiveAsync::Close()
{
	// Invalidate any precached data and free memory.
	FlushCache();
	// Return TRUE if there were NO errors, FALSE otherwise.
	return !ArIsError;
}

/**
 * Sets mapping from offsets/ sizes that are going to be used for seeking and serialization to what
 * is actually stored on disk. If the archive supports dealing with compression in this way it is 
 * going to return TRUE.
 *
 * @param	InCompressedChunks	Pointer to array containing information about [un]compressed chunks
 * @param	InCompressionFlags	Flags determining compression format associated with mapping
 *
 * @return TRUE if archive supports translating offsets & uncompressing on read, FALSE otherwise
 */
UBOOL FArchiveAsync::SetCompressionMap( TArray<FCompressedChunk>* InCompressedChunks, ECompressionFlags InCompressionFlags )
{
	// Set chunks. A value of NULL means to use direct reads again.
	CompressedChunks	= InCompressedChunks;
	CompressionFlags	= InCompressionFlags;
	CurrentChunkIndex	= 0;
	// Invalidate any precached data and free memory.
	FlushCache();
	// We support translation as requested.
	return TRUE;
}

/**
 * Swaps current and next buffer. Relies on calling code to ensure that there are no outstanding
 * async read operations into the buffers.
 */
void FArchiveAsync::BufferSwitcheroo()
{
	check( PrecacheReadStatus[CURRENT].GetValue() == 0 );
	check( PrecacheReadStatus[NEXT].GetValue() == 0 );

	// Switcheroo.
	appFree( PrecacheBuffer[CURRENT] );
	PrecacheBuffer[CURRENT]		= PrecacheBuffer[NEXT];
	PrecacheStartPos[CURRENT]	= PrecacheStartPos[NEXT];
	PrecacheEndPos[CURRENT]		= PrecacheEndPos[NEXT];

	// Next buffer is unused/ free.
	PrecacheBuffer[NEXT]		= NULL;
	PrecacheStartPos[NEXT]		= 0;
	PrecacheEndPos[NEXT]		= 0;
}

/**
 * Whether the current precache buffer contains the passed in request.
 *
 * @param	RequestOffset	Offset in bytes from start of file
 * @param	RequestSize		Size in bytes requested
 *
 * @return TRUE if buffer contains request, FALSE othwerise
 */
UBOOL FArchiveAsync::PrecacheBufferContainsRequest( INT RequestOffset, INT RequestSize )
{
	// TRUE if request is part of precached buffer.
	if( (RequestOffset >= PrecacheStartPos[CURRENT]) 
	&&  (RequestOffset+RequestSize <= PrecacheEndPos[CURRENT]) )
	{
		return TRUE;
	}
	// FALSE if it doesn't fit 100%.
	else
	{
		return FALSE;
	}
}

/**
 * Finds and returns the compressed chunk index associated with the passed in offset.
 *
 * @param	RequestOffset	Offset in file to find associated chunk index for
 *
 * @return Index into CompressedChunks array matching this offset
 */
INT FArchiveAsync::FindCompressedChunkIndex( INT RequestOffset )
{
	// Find base start point and size. @todo optimization: avoid full iteration
	CurrentChunkIndex = 0;
	while( CurrentChunkIndex < CompressedChunks->Num() )
	{
		const FCompressedChunk& Chunk = (*CompressedChunks)(CurrentChunkIndex);
		// Check whether request offset is encompassed by this chunk.
		if( Chunk.UncompressedOffset <= RequestOffset 
		&&  Chunk.UncompressedOffset + Chunk.UncompressedSize > RequestOffset )
		{
			break;
		}
		CurrentChunkIndex++;
	}
	check( CurrentChunkIndex < CompressedChunks->Num() );
	return CurrentChunkIndex;
}

/**
 * Precaches compressed chunk of passed in index using buffer at passed in index.
 *
 * @param	ChunkIndex	Index of compressed chunk
 * @param	BufferIndex	Index of buffer to precache into	
 */
void FArchiveAsync::PrecacheCompressedChunk( INT ChunkIndex, INT BufferIndex )
{
	// Request generic async IO system.
	FIOSystem* IO = GIOManager->GetIOSystem( IOSYSTEM_GenericAsync ); 
	check(IO);

	// Compressed chunk to request.
	FCompressedChunk ChunkToRead = (*CompressedChunks)(ChunkIndex);

	// Update start and end position...
	PrecacheStartPos[BufferIndex]	= ChunkToRead.UncompressedOffset;
	PrecacheEndPos[BufferIndex]		= ChunkToRead.UncompressedOffset + ChunkToRead.UncompressedSize;

	// In theory we could use appRealloc if it had a way to signal that we don't want to copy
	// the data (implicit realloc behavior).
	appFree( PrecacheBuffer[BufferIndex] );
	PrecacheBuffer[BufferIndex]		= (BYTE*) appMalloc( PrecacheEndPos[BufferIndex] - PrecacheStartPos[BufferIndex] );

	// Increment read status, request load and make sure that request was possible (e.g. filename was valid).
	check( PrecacheReadStatus[BufferIndex].GetValue() == 0 );
	PrecacheReadStatus[BufferIndex].Increment();
	QWORD RequestId = IO->LoadCompressedData( 
							FileName, 
							ChunkToRead.CompressedOffset, 
							ChunkToRead.CompressedSize, 
							ChunkToRead.UncompressedSize, 
							PrecacheBuffer[BufferIndex], 
							CompressionFlags, 
							&PrecacheReadStatus[BufferIndex],
							AIOP_Normal);
	check(RequestId);
}

/**
 * Hint the archive that the region starting at passed in offset and spanning the passed in size
 * is going to be read soon and should be precached.
 *
 * The function returns whether the precache operation has completed or not which is an important
 * hint for code knowing that it deals with potential async I/O. The archive is free to either not 
 * implement this function or only partially precache so it is required that given sufficient time
 * the function will return TRUE. Archives not based on async I/O should always return TRUE.
 *
 * This function will not change the current archive position.
 *
 * @param	RequestOffset	Offset at which to begin precaching.
 * @param	RequestSize		Number of bytes to precache
 * @return	FALSE if precache operation is still pending, TRUE otherwise
 */
UBOOL FArchiveAsync::Precache( INT RequestOffset, INT RequestSize )
{
	// Check whether we're currently waiting for a read request to finish.
	UBOOL bFinishedReadingCurrent	= PrecacheReadStatus[CURRENT].GetValue()==0 ? TRUE : FALSE;
	UBOOL bFinishedReadingNext		= PrecacheReadStatus[NEXT].GetValue()==0 ? TRUE : FALSE;

	// Return read status if the current request fits entirely in the precached region.
	if( PrecacheBufferContainsRequest( RequestOffset, RequestSize ) )
	{
		return bFinishedReadingCurrent;
	}
	// We're not fitting into the precached region and we have a current read request outstanding
	// so wait till we're done with that. This can happen if we're skipping over large blocks in
	// the file because the object has been found in memory.
	// @todo async: implement cancelation
	else if( !bFinishedReadingCurrent )
	{
		return FALSE;
	}
	// We're still in the middle of fulfilling the next read request so wait till that is done.
	else if( !bFinishedReadingNext )
	{
		return FALSE;
	}
	// We need to make a new read request.
	else
	{
		// Compressed read. The passed in offset and size were requests into the uncompressed file and
		// need to be translated via the CompressedChunks map first.
		if( CompressedChunks )
		{
			// Switch to next buffer.
			BufferSwitcheroo();

			// Check whether region is precached after switcheroo.
			UBOOL	bIsRequestCached	= PrecacheBufferContainsRequest( RequestOffset, RequestSize );
			// Find chunk associated with request.
			INT		RequestChunkIndex	= FindCompressedChunkIndex( RequestOffset );

			// Precache chunk if it isn't already.
			if( !bIsRequestCached )
			{
				PrecacheCompressedChunk( RequestChunkIndex, CURRENT );
			}

			// Precache next chunk if there is one.
			if( RequestChunkIndex + 1 < CompressedChunks->Num() )
			{
				PrecacheCompressedChunk( RequestChunkIndex + 1, NEXT );
			}
		}
		// Regular read.
		else
		{
			// Request generic async IO system.
			FIOSystem* IO = GIOManager->GetIOSystem( IOSYSTEM_GenericAsync ); 
			check(IO);

			PrecacheStartPos[CURRENT]	= RequestOffset;
			// We always request at least a few KByte to be read/ precached to avoid going to disk for
			// a lot of little reads.
			PrecacheEndPos[CURRENT]		= RequestOffset + Max( RequestSize, DVD_MIN_READ_SIZE );
			// Ensure that we're not trying to read beyond EOF.
			PrecacheEndPos[CURRENT]		= Min( PrecacheEndPos[CURRENT], FileSize );
			// In theory we could use appRealloc if it had a way to signal that we don't want to copy
			// the data (implicit realloc behavior).
			appFree( PrecacheBuffer[CURRENT] );
			PrecacheBuffer[CURRENT]		= (BYTE*) appMalloc( PrecacheEndPos[CURRENT] - PrecacheStartPos[CURRENT] );
		
			// Increment read status, request load and make sure that request was possible (e.g. filename was valid).
			PrecacheReadStatus[CURRENT].Increment();
			QWORD RequestId = IO->LoadData( 
									FileName, 
									PrecacheStartPos[CURRENT], 
									PrecacheEndPos[CURRENT] - PrecacheStartPos[CURRENT], 
									PrecacheBuffer[CURRENT], 
									&PrecacheReadStatus[CURRENT],
									AIOP_Normal );
			check(RequestId);
		}

		return FALSE;
	}
}

/**
 * Serializes data from archive.
 *
 * @param	Data	Pointer to serialize to
 * @param	Count	Number of bytes to read
 */
void FArchiveAsync::Serialize( void* Data, INT Count )
{
	DOUBLE	StartTime	= 0;
	UBOOL	bIOBlocked	= FALSE;

	// Make sure serialization request fits entirely in already precached region.
	if( !PrecacheBufferContainsRequest( CurrentPos, Count ) )
	{
		// Keep track of time we started to block.
		StartTime	= appSeconds();
		bIOBlocked	= TRUE;

		// Busy wait for region to be precached.
		while( !Precache( CurrentPos, Count ) )
		{
			appSleep(0);
		}

		// There shouldn't be any outstanding read requests for the main buffer at this point.
		check( PrecacheReadStatus[CURRENT].GetValue() == 0 );
	}
	
	// Make sure to wait till read request has finished before progressing. This can happen if PreCache interface
	// is not being used for serialization.
	while( PrecacheReadStatus[CURRENT].GetValue() != 0 )
	{
		// Only update StartTime if we haven't already started blocking I/O above.
		if( !bIOBlocked )
		{
			// Keep track of time we started to block.
			StartTime	= appSeconds();
			bIOBlocked	= TRUE;
		}
		appSleep(0);
	}

	// Update stats if we were blocked.
#if STATS
	if( bIOBlocked )
	{
		DOUBLE BlockingTime = appSeconds() - StartTime;
		INC_FLOAT_STAT_BY(STAT_AsyncIO_MainThreadBlockTime,(FLOAT)BlockingTime);
#if LOOKING_FOR_PERF_ISSUES
		debugf( NAME_PerfWarning, TEXT("FArchiveAsync::Serialize: %5.2fms blocking on read from '%s' (Offset: %i, Size: %i)"), 
			1000 * BlockingTime, 
			*FileName, 
			CurrentPos, 
			Count );
#endif
	}
#endif

	// Copy memory to destination.
	appMemcpy( Data, PrecacheBuffer[CURRENT] + (CurrentPos - PrecacheStartPos[CURRENT]), Count );
	// Serialization implicitly increases position in file.
	CurrentPos += Count;
}

/**
 * Returns the current position in the archive as offset in bytes from the beginning.
 *
 * @return	Current position in the archive (offset in bytes from the beginning)
 */
INT FArchiveAsync::Tell()
{
	return CurrentPos;
}

/**
 * Returns the total size of the archive in bytes.
 *
 * @return total size of the archive in bytes
 */
INT FArchiveAsync::TotalSize()
{
	return UncompressedFileSize;
}

/**
 * Sets the current position.
 *
 * @param InPos	New position (as offset from beginning in bytes)
 */
void FArchiveAsync::Seek( INT InPos )
{
	check( InPos >= 0 );
	CurrentPos = InPos;
}


/*----------------------------------------------------------------------------
	End.
----------------------------------------------------------------------------*/