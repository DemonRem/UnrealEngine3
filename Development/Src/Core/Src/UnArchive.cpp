/*=============================================================================
	UnArchive.cpp: Core archive classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FArchive implementation.
-----------------------------------------------------------------------------*/

/**
 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
 * is in when a loading error occurs.
 *
 * This is overridden for the specific Archive Types
 **/
FName FArchive::GetArchiveName() const
{
	return FName( TEXT("FArchive") );
}

/** 
 * FCompressedChunkInfo serialize operator.
 */
FArchive& operator<<( FArchive& Ar, FCompressedChunkInfo& Chunk )
{
	// The order of serialization needs to be identical to the memory layout as the async IO code is reading it in bulk.
	// The size of the structure also needs to match what is being serialized.
	Ar << Chunk.CompressedSize;
	Ar << Chunk.UncompressedSize;
	return Ar;
}

/**
 * Serializes and compresses/ uncompresses data. This is a shared helper function for compression
 * support. The data is saved in a way compatible with FIOSystem::LoadCompressedData.
 *
 * @note: the way this code works needs to be in line with FIOSystem::LoadCompressedData implementations
 * @note: the way this code works needs to be in line with FAsyncIOSystemBase::FulfillCompressedRead
 *
 * @param	V		Data pointer to serialize data from/to, or a FileReader if bTreatBufferAsFileReader is TRUE
 * @param	Length	Length of source data if we're saving, unused otherwise
 * @param	Flags	Flags to control what method to use for [de]compression and optionally control memory vs speed when compressing
 * @param	bTreatBufferAsFileReader TRUE if V is actually an FArchive, which is used when saving to read data - helps to avoid single huge allocations of source data
 */
void FArchive::SerializeCompressed( void* V, INT Length, ECompressionFlags Flags, UBOOL bTreatBufferAsFileReader )
{
	if( IsLoading() )
	{
		// Serialize package file tag used to determine endianess.
		FCompressedChunkInfo PackageFileTag;
		PackageFileTag.CompressedSize	= 0;
		PackageFileTag.UncompressedSize	= 0;
		*this << PackageFileTag;
		UBOOL bWasByteSwapped = PackageFileTag.CompressedSize != PACKAGE_FILE_TAG;

		// Read in base summary.
		FCompressedChunkInfo Summary;
		*this << Summary;

		if (bWasByteSwapped)
		{
			check( PackageFileTag.CompressedSize   == PACKAGE_FILE_TAG_SWAPPED );
			Summary.CompressedSize = BYTESWAP_ORDER32(Summary.CompressedSize);
			Summary.UncompressedSize = BYTESWAP_ORDER32(Summary.UncompressedSize);
		}
		else
		{
			check( PackageFileTag.CompressedSize   == PACKAGE_FILE_TAG );
		}

		// Handle change in compression chunk size in backward compatible way.
		INT LoadingCompressionChunkSize = PackageFileTag.UncompressedSize;
		if (LoadingCompressionChunkSize == PACKAGE_FILE_TAG)
		{
			if( Ver() < VER_CHANGED_COMPRESSION_CHUNK_SIZE_TO_128 )
			{
				LoadingCompressionChunkSize = LOADING_COMPRESSION_CHUNK_SIZE_PRE_369;
			}
			else
			{
				LoadingCompressionChunkSize = LOADING_COMPRESSION_CHUNK_SIZE;
			}
		}

		// Figure out how many chunks there are going to be based on uncompressed size and compression chunk size.
		INT	TotalChunkCount	= (Summary.UncompressedSize + LoadingCompressionChunkSize - 1) / LoadingCompressionChunkSize;
		
		// Allocate compression chunk infos and serialize them, keeping track of max size of compression chunks used.
		FCompressedChunkInfo*	CompressionChunks	= new FCompressedChunkInfo[TotalChunkCount];
		INT						MaxCompressedSize	= 0;
		for( INT ChunkIndex=0; ChunkIndex<TotalChunkCount; ChunkIndex++ )
		{
			*this << CompressionChunks[ChunkIndex];
			if (bWasByteSwapped)
			{
				CompressionChunks[ChunkIndex].CompressedSize	= BYTESWAP_ORDER32( CompressionChunks[ChunkIndex].CompressedSize );
				CompressionChunks[ChunkIndex].UncompressedSize	= BYTESWAP_ORDER32( CompressionChunks[ChunkIndex].UncompressedSize );
			}
			MaxCompressedSize = Max( CompressionChunks[ChunkIndex].CompressedSize, MaxCompressedSize );
		}

		// Set up destination pointer and allocate memory for compressed chunk[s] (one at a time).
		BYTE*	Dest				= (BYTE*) V;
		void*	CompressedBuffer	= appMalloc( MaxCompressedSize );

		// Iteratre over all chunks, serialize them into memory and decompress them directly into the destination pointer
		for( INT ChunkIndex=0; ChunkIndex<TotalChunkCount; ChunkIndex++ )
		{
			const FCompressedChunkInfo& Chunk = CompressionChunks[ChunkIndex];
			// Read compressed data.
			Serialize( CompressedBuffer, Chunk.CompressedSize );
			// Decompress into dest pointer directly.
			verify( appUncompressMemory( Flags, Dest, Chunk.UncompressedSize, CompressedBuffer, Chunk.CompressedSize ) );
			// And advance it by read amount.
			Dest += Chunk.UncompressedSize;
		}

		// Free up allocated memory.
		appFree( CompressedBuffer );
		delete [] CompressionChunks;
	}
	else if( IsSaving() )
	{	
		check( Length > 0 );

		// Serialize package file tag used to determine endianess in LoadCompressedData.
		FCompressedChunkInfo PackageFileTag;
		PackageFileTag.CompressedSize	= PACKAGE_FILE_TAG;
		PackageFileTag.UncompressedSize	= GSavingCompressionChunkSize;
		*this << PackageFileTag;

		// Figure out how many chunks there are going to be based on uncompressed size and compression chunk size.
		INT	TotalChunkCount	= (Length + GSavingCompressionChunkSize - 1) / GSavingCompressionChunkSize + 1;
		
		// Keep track of current position so we can later seek back and overwrite stub compression chunk infos.
		INT StartPosition = Tell();

		// Allocate compression chunk infos and serialize them so we can later overwrite the data.
		FCompressedChunkInfo* CompressionChunks	= new FCompressedChunkInfo[TotalChunkCount];
		for( INT ChunkIndex=0; ChunkIndex<TotalChunkCount; ChunkIndex++ )
		{
			*this << CompressionChunks[ChunkIndex];
		}

		// The uncompressd size is equal to the passed in length.
		CompressionChunks[0].UncompressedSize	= Length;
		// Zero initialize compressed size so we can update it during chunk compression.
		CompressionChunks[0].CompressedSize		= 0;

		// Set up source pointer amount of data to copy (in bytes)
		BYTE*	Src;
		// allocate memory to read into
		if (bTreatBufferAsFileReader)
		{
			Src = (BYTE*)appMalloc(GSavingCompressionChunkSize);
			check(((FArchive*)V)->IsLoading());
		}
		else
		{
			Src = (BYTE*) V;
		}
		INT		BytesRemaining			= Length;
		// Start at index 1 as first chunk info is summary.
		INT		CurrentChunkIndex		= 1;
		// 2 times the uncompressed size should be more than enough; the compressed data shouldn't be that much larger
		INT		CompressedBufferSize	= 2 * GSavingCompressionChunkSize;
		void*	CompressedBuffer		= appMalloc( CompressedBufferSize );

		while( BytesRemaining > 0 )
		{
			INT BytesToCompress = Min( BytesRemaining, GSavingCompressionChunkSize );
			INT CompressedSize	= CompressedBufferSize;

			// read in the next chunk from the reader
			if (bTreatBufferAsFileReader)
			{
				((FArchive*)V)->Serialize(Src, BytesToCompress);
			}

			verify( appCompressMemory( Flags, CompressedBuffer, CompressedSize, Src, BytesToCompress ) );
			// move to next chunk if not reading from file
			if (!bTreatBufferAsFileReader)
			{
				Src += BytesToCompress;
			}
			Serialize( CompressedBuffer, CompressedSize );
			// Keep track of total compressed size, stored in first chunk.
			CompressionChunks[0].CompressedSize	+= CompressedSize;

			// Update current chunk.
			check(CurrentChunkIndex<TotalChunkCount);
			CompressionChunks[CurrentChunkIndex].CompressedSize		= CompressedSize;
			CompressionChunks[CurrentChunkIndex].UncompressedSize	= BytesToCompress;
			CurrentChunkIndex++;
			
			BytesRemaining -= GSavingCompressionChunkSize;
		}

		// free the buffer we read into
		if (bTreatBufferAsFileReader)
		{
			appFree(Src);
		}

		// Free allocated memory.
		appFree( CompressedBuffer );

		// Overrwrite chunk infos by seeking to the beginning, serializing the data and then
		// seeking back to the end.
		INT EndPosition = Tell();
		// Seek to the beginning.
		Seek( StartPosition );
		// Serialize chunk infos.
		for( INT ChunkIndex=0; ChunkIndex<TotalChunkCount; ChunkIndex++ )
		{
			*this << CompressionChunks[ChunkIndex];
		}
		// Seek back to end.
		Seek( EndPosition );

		// Free intermediate data.
		delete [] CompressionChunks;
	}
}

VARARG_BODY( void, FArchive::Logf, const TCHAR*, VARARG_NONE )
{
	// We need to use malloc here directly as GMalloc might not be safe, e.g. if called from within GMalloc!
	INT		BufferSize	= 1024;
	TCHAR*	Buffer		= NULL;
	INT		Result		= -1;

	while(Result == -1)
	{
		Buffer = (TCHAR*) appSystemRealloc( Buffer, BufferSize * sizeof(TCHAR) );
		GET_VARARGS_RESULT( Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result );
		BufferSize *= 2;
	};
	Buffer[Result] = 0;

	// Convert to ANSI and serialize as ANSI char.
	ANSICHAR* AnsiBuffer = (ANSICHAR*) appSystemMalloc( Result + appStrlen( LINE_TERMINATOR ) );
	for( INT i=0; i<Result; i++ )
	{
		ANSICHAR Char = ToAnsi( Buffer[i] );
		Serialize( &Char, 1 );
	}

	// Write out line terminator.
	for( INT i=0; LINE_TERMINATOR[i]; i++ )
	{
		ANSICHAR Char = LINE_TERMINATOR[i];
		Serialize( &Char, 1 );
	}

	// Free temporary buffers.
	appSystemFree( Buffer );
	appSystemFree( AnsiBuffer );
}

/*-----------------------------------------------------------------------------
	UObject in-memory archivers.
-----------------------------------------------------------------------------*/
/** Constructor */
FReloadObjectArc::FReloadObjectArc()
: FArchive(), Reader(Bytes), Writer(Bytes), RootObject(NULL), InstanceGraph(NULL)
, bAllowTransientObjects(TRUE), bInstanceSubobjectsOnLoad(TRUE)
{
}

/** Destructor */
FReloadObjectArc::~FReloadObjectArc()
{
	if ( InstanceGraph != NULL )
	{
		delete InstanceGraph;
		InstanceGraph = NULL;
	}
}

/**
 * Sets the root object for this memory archive.
 * 
 * @param	NewRoot		the UObject that should be the new root
 */
void FReloadObjectArc::SetRootObject( UObject* NewRoot )
{
	if ( NewRoot != NULL && InstanceGraph == NULL )
	{
		// if we are setting the top-level root object and we don't yet have an instance graph, create one
		InstanceGraph = new FObjectInstancingGraph(NewRoot);
		if ( IsLoading() )
		{
			// if we're reloading data onto objects, pre-initialize the instance graph with the objects instances that were serialized
			for ( INT ObjectIndex = 0; ObjectIndex < CompleteObjects.Num(); ObjectIndex++ )
			{
				UObject* InnerObject = CompleteObjects(ObjectIndex);

				// ignore previously saved objects that aren't contained within the current root object
				if ( InnerObject->IsIn(InstanceGraph->GetDestinationRoot()) )
				{
					UComponent* InnerComponent = Cast<UComponent>(InnerObject);
					if ( InnerComponent != NULL )
					{
						InstanceGraph->AddComponentPair(InnerComponent->GetArchetype<UComponent>(), InnerComponent);
					}
					else
					{
						InstanceGraph->AddObjectPair(InnerObject);
					}
				}
			}
		}
	}

	RootObject = NewRoot;
	if ( RootObject == NULL && InstanceGraph != NULL )
	{
		// if we have cleared the top-level root object and we have an instance graph, delete it
		delete InstanceGraph;
		InstanceGraph = NULL;
	}
}

/**
 * Begin serializing a UObject into the memory archive.  Note that this archive does not use
 * object flags (RF_TagExp|RF_TagImp) to prevent recursion, as do most other archives that perform
 * similar functionality.  This is because this archive is used in operations that modify those
 * object flags for other purposes.
 *
 * @param	Obj		the object to serialize
 */
void FReloadObjectArc::SerializeObject( UObject* Obj )
{
	if ( Obj != NULL )
	{
		TArray<UObject*>& ObjectList = IsLoading()
			? LoadedObjects
			: SavedObjects;

		// only process this top-level object once
		if ( !ObjectList.ContainsItem(Obj) )
		{
			ObjectList.AddUniqueItem(Obj);

			// store the current value of RootObject, in case our caller set it before calling SerializeObject
			UObject* PreviousRoot = RootObject;

			DWORD PreviousHackFlags = GUglyHackFlags;
			GUglyHackFlags |= HACK_IsReloadObjArc;

			// set the root object to this object so that we load complete object data
			// for any objects contained within the top-level object (such as components)
			SetRootObject(Obj);

			// set this to prevent recursion in serialization
			if ( IsLoading() )
			{
				// InitProperties will call CopyCompleteValue for any instanced object properties, so disable object instancing
				// because we probably already have an object that will be copied over that value; for any instanced object properties which
				// did not have a value when we were saving object data, we'll call InstanceSubobjects to instance those.  Also disable component
				// instancing as this object may not have the correct values for its component properties until its serialized, which result in
				// possibly creating lots of unnecessary components that will be thrown away anyway
				InstanceGraph->EnableObjectInstancing(FALSE);
				InstanceGraph->EnableComponentInstancing(FALSE);
				Obj->InitializeProperties(NULL, InstanceGraph);
			}

			if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
			{
				Obj->GetClass()->SerializeDefaultObject(Obj, *this);
			}
			else
			{
				// save / load the data for this object that is different from its class
				Obj->Serialize(*this);
			}

			if ( IsLoading() )
			{
				if ( InstanceGraph != NULL )
				{
					InstanceGraph->EnableObjectInstancing(TRUE);
					InstanceGraph->EnableComponentInstancing(TRUE);

					if ( bInstanceSubobjectsOnLoad )
					{
						// serializing the stored data for this object should have replaced all of its original instanced object references
						// but there might have been new subobjects added to the object's archetype in the meantime (in the case of updating 
						// an prefab from a prefab instance, for example), so enable subobject instancing and instance those now.
						Obj->InstanceSubobjectTemplates(InstanceGraph);

						// components which were identical to their archetypes weren't stored into this archive's object data, so re-instance those components now
						Obj->InstanceComponentTemplates(InstanceGraph);
					}
				}

				if ( !Obj->HasAnyFlags(RF_ClassDefaultObject) )
				{
					// allow the object to perform any cleanup after re-initialization
					Obj->PostLoad();
				}
			}

			// restore the RootObject - we're done with it.
			SetRootObject(PreviousRoot);
			GUglyHackFlags = PreviousHackFlags;
		}
	}
}

/**
 * Resets the archive so that it can be loaded from again from scratch
 * as if it was never serialized as a Reader
 */
void FReloadObjectArc::Reset()
{
	// empty the list of objects that were loaded, so we can load again
	LoadedObjects.Empty();
	// reset our location in the buffer
	Seek(0);
}


/**
 * I/O function for FName
 */
FArchive& FReloadObjectArc::operator<<( class FName& Name )
{
	NAME_INDEX NameIndex;
	INT NameInstance;
	if ( IsLoading() )
	{
		Reader << NameIndex << NameInstance;

		// recreate the FName using the serialized index and instance number
		Name = FName((EName)NameIndex, NameInstance);
	}
	else if ( IsSaving() )
	{
		NameIndex = Name.GetIndex();
		NameInstance = Name.GetNumber();

		Writer << NameIndex << NameInstance;
	}
	return *this;
}

/**
 * I/O function for UObject references
 */
FArchive& FReloadObjectArc::operator<<( class UObject*& Obj )
{
	if ( IsLoading() )
	{
		PACKAGE_INDEX Index = 0;
		Reader << Index;

		// An index of 0 indicates that the value of the object was NULL
		if ( Index == 0 )
		{
			Obj = NULL;
		}
		else if ( Index < 0 )
		{
			// An index less than zero indicates an object for which we only stored the object pointer
			Obj = ReferencedObjects(-Index-1);
		}
		else if ( Index > 0 )
		{
			// otherwise, the memory archive contains the entire UObject data for this UObject, so load
			// it from the archive
			Obj = CompleteObjects(Index-1);

			// Ensure we don't load it more than once.
			if ( !LoadedObjects.ContainsItem(Obj) )
			{
				LoadedObjects.AddItem(Obj);

				// scope any local variable declarations so that we reduce the risk of stack overflow
				{
					// find the location for this UObject's data in the memory archive
					INT* ObjectOffset = ObjectMap.Find(Obj);
					check(ObjectOffset);

					// move the reader to that position
					Reader.Seek(*ObjectOffset);
				}

				DWORD PreviousHackFlags = GUglyHackFlags;
				GUglyHackFlags |= HACK_IsReloadObjArc;

				// make sure object instancing is disabled before calling InitializeProperties; otherwise new copies of objects will be
				// created for any instanced object properties owned by this object and then immediately overwritten when its serialized
				InstanceGraph->EnableObjectInstancing(FALSE);
				InstanceGraph->EnableComponentInstancing(FALSE);

				// Call InitializeProperties to propagate base change to 'within' objects (like Components).
				Obj->InitializeProperties( NULL, InstanceGraph );

				// read in the data for this UObject
				Obj->Serialize(*this);

				// we should never have RootObject serialized as an object contained by the root object
				checkSlow(Obj != RootObject);

				Obj->PostLoad();

				// serializing the stored data for this object should have replaced all of its original instanced object references
				// but there might have been new subobjects added to the object's archetype in the meantime (in the case of updating 
				// an prefab from a prefab instance, for example), so enable subobject instancing and instance those now.
				InstanceGraph->EnableObjectInstancing(TRUE);
				InstanceGraph->EnableComponentInstancing(TRUE);

				if ( bInstanceSubobjectsOnLoad )
				{
					// we just called InitializeProperties, so any instanced components which were identical to their template weren't serialized into the
					// object data, thus those properties are now pointing to the component contained by the archetype.  So now we need to reinstance those components
					Obj->InstanceSubobjectTemplates(InstanceGraph);

					// components which were identical to their archetypes weren't stored into this archive's object data, so re-instance those components now
					Obj->InstanceComponentTemplates(InstanceGraph);
				}

				GUglyHackFlags = PreviousHackFlags;
			}
		}
	}
	else if ( IsSaving() )
	{
		// Don't save references to transient or deleted objects.
		if ( Obj == NULL || (Obj->HasAnyFlags(RF_Transient) && !bAllowTransientObjects) || Obj->IsPendingKill() )
		{
			// null objects are stored as 0 indexes
			PACKAGE_INDEX Index = 0;
			Writer << Index;
			return *this;
		}

		// See if we have already written this object out.
		INT CompleteIndex = CompleteObjects.FindItemIndex(Obj);
		PACKAGE_INDEX ReferencedIndex = ReferencedObjects.FindItemIndex(Obj);

		// The same object can't be in both arrays.
		check( !(CompleteIndex != INDEX_NONE && ReferencedIndex != INDEX_NONE) );

		if(CompleteIndex != INDEX_NONE)
		{
			PACKAGE_INDEX Index = CompleteIndex + 1;
			Writer << Index;
		}
		else if(ReferencedIndex != INDEX_NONE)
		{
			PACKAGE_INDEX Index = -ReferencedIndex - 1;
			Writer << Index;
		}
		// New object - was not already saved.
		// if the object is in the SavedObjects array, it means that the object was serialized into this memory archive as a root object
		// in this case, just serialize the object as a simple reference
		else if ( Obj->IsIn(RootObject) && !SavedObjects.ContainsItem(Obj) )
		{
			SavedObjects.AddItem(Obj);

			// we need to store the UObject data for this object
			check(ObjectMap.Find(Obj) == NULL);

			// only the location of the UObject in the CompleteObjects
			// array is stored in the memory archive, using PACKAGE_INDEX
			// notation
			PACKAGE_INDEX Index = CompleteObjects.AddUniqueItem(Obj) + 1;
			Writer << Index;

			// remember the location of the beginning of this UObject's data
			ObjectMap.Set(Obj,Writer.Tell());

			DWORD PreviousHackFlags = GUglyHackFlags;
			GUglyHackFlags |= HACK_IsReloadObjArc;

			Obj->Serialize(*this);

			GUglyHackFlags = PreviousHackFlags;
		}
		else
		{
#if 0
			// this code is for finding references from actors in PrefabInstances to actors in other levels
			if ( !Obj->HasAnyFlags(RF_Public) && RootObject->GetOutermost() != Obj->GetOutermost() )
			{
				debugf(TEXT("FReloadObjectArc: Encountered reference to external private object %s while serializing references for %s   (prop:%s)"), *Obj->GetFullName(), *RootObject->GetFullName(), GSerializedProperty ? *GSerializedProperty->GetPathName() : TEXT("NULL"));
			}
#endif

			// Referenced objects will be indicated by negative indexes
			PACKAGE_INDEX Index = -ReferencedObjects.AddUniqueItem(Obj) - 1;
			Writer << Index;
		}
	}
	return *this;
}

/*----------------------------------------------------------------------------
	FArchetypePropagationArc.
----------------------------------------------------------------------------*/
/** Constructor */
FArchetypePropagationArc::FArchetypePropagationArc()
: FReloadObjectArc()
{
	// enable "write" mode for this archive - objects will serialize their data into the archive
	ActivateWriter();

	// don't wipe out transient objects
	bAllowTransientObjects = TRUE;

	// setting this flag indicates that component references should only be serialized into this archive if there the component has different values that its archetype,
	// and only when the component is being compared to its archetype (as opposed to another component instance, for example)
	SetPortFlags(PPF_DeepCompareInstances);
}

/*----------------------------------------------------------------------------
	FArchiveShowReferences.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	inOutputAr		archive to use for logging results
 * @param	LimitOuter		only consider objects that have this object as its Outer
 * @param	inTarget		object to show referencers to
 * @param	inExclude		list of objects that should be ignored if encountered while serializing Target
 */
FArchiveShowReferences::FArchiveShowReferences( FOutputDevice& inOutputAr, UObject* inOuter, UObject* inSource, TArray<UObject*>& inExclude )
: SourceObject(inSource)
, SourceOuter(inOuter)
, OutputAr(inOutputAr)
, Exclude(inExclude)
, DidRef(FALSE)
{
	ArIsObjectReferenceCollector = TRUE;

	// there are several types of objects we don't want listed, for different reasons.
	// Prevent them from being logged by adding them to our Found list before we start
	// serialization, so that they won't be listed

	// quick sanity check
	check(SourceObject);
	check(SourceObject->IsValid());

	// every object we serialize obviously references our package
	Found.AddUniqueItem(SourceOuter);

	// every object we serialize obviously references its linker
	Found.AddUniqueItem(SourceObject->GetLinker());

	// every object we serialize obviously references its class and its class's parent classes
	for ( UClass* ObjectClass = SourceObject->GetClass(); ObjectClass; ObjectClass = ObjectClass->GetSuperClass() )
	{
		Found.AddUniqueItem( ObjectClass );
	}

	// similarly, if the object is a class, they all obviously reference their parent classes
	if ( SourceObject->IsA(UClass::StaticClass()) )
	{
		for ( UClass* ParentClass = Cast<UClass>(SourceObject)->GetSuperClass(); ParentClass; ParentClass = ParentClass->GetSuperClass() )
		{
			Found.AddUniqueItem( ParentClass );
		}
	}

	// OK, now we're all set to go - let's see what Target is referencing.
	SourceObject->Serialize( *this );
}

FArchive& FArchiveShowReferences::operator<<( UObject*& Obj )
{
	if( Obj && Obj->GetOuter() != SourceOuter )
	{
		INT i;
		for( i=0; i<Exclude.Num(); i++ )
		{
			if( Exclude(i) == Obj->GetOuter() )
			{
				break;
			}
		}

		if( i==Exclude.Num() )
		{
			if( !DidRef )
			{
				OutputAr.Logf( TEXT("   %s references:"), *Obj->GetFullName() );
			}

			OutputAr.Logf( TEXT("      %s"), *Obj->GetFullName() );

			DidRef=1;
		}
	}
	return *this;
}

/*----------------------------------------------------------------------------
	FArchiveTraceRoute
----------------------------------------------------------------------------*/

TMap<UObject*,UProperty*> FArchiveTraceRoute::FindShortestRootPath( UObject* Obj, UBOOL bIncludeTransients, EObjectFlags KeepFlags )
{
	// Take snapshot of object flags that will be restored once marker goes out of scope.
	FScopedObjectFlagMarker ObjectFlagMarker;

	TMap<UObject*,FTraceRouteRecord> Routes;
	FArchiveTraceRoute Rt( Routes, bIncludeTransients, KeepFlags );

	TMap<UObject*,UProperty*> Result;

	FTraceRouteRecord* Rec = Routes.Find(Obj);
	if( Rec )
	{
		Result.Set( Obj, NULL );
		for( ; ; )
		{
			Rec = Routes.Find(Obj);
			if( Rec->Depth==0 )
			{
				break;
			}

			Obj = Rec->Referencer;
			Result.Set(Obj, Rec->ReferencerProperty);
		}
	}
	return Result;
}

/**
 * Retuns path to root created by e.g. FindShortestRootPath via a string.
 *
 * @param TargetObject	object marking one end of the route
 * @param Route			route to print to log.
 * @param String of root path
 */
FString FArchiveTraceRoute::PrintRootPath( const TMap<UObject*,UProperty*>& Route, const UObject* TargetObject )
{
	FString RouteString;
	for( TMap<UObject*,UProperty*>::TConstIterator MapIt(Route); MapIt; ++MapIt )
	{
		UObject*	Object		= MapIt.Key();
		UProperty*	Property	= MapIt.Value();

		FString	ObjectReachability;
		
		if( Object == TargetObject )
		{
			ObjectReachability = TEXT(" (target)");
		}
		
		if( Object->HasAnyFlags(RF_RootSet) )
		{
			ObjectReachability += TEXT(" (root)");
		}
		else
		if( Object->HasAnyFlags(RF_Native) )
		{
			ObjectReachability += TEXT(" (native)");
		}
		else
		if( Object->HasAnyFlags(RF_Standalone) )
		{
			ObjectReachability += TEXT(" (standalone)");
		}
		else if( ObjectReachability == TEXT("") )
		{
			ObjectReachability = TEXT(" ");
		}
			
		FString ReferenceSource;
		if( Property != NULL )
		{
			ReferenceSource = FString::Printf(TEXT("%s (%s)"), *ObjectReachability, *Property->GetFullName());
		}
		else
		{
			ReferenceSource = ObjectReachability;
		}

		RouteString += FString::Printf(TEXT("   %s%s%s"), *Object->GetFullName(), *ReferenceSource, LINE_TERMINATOR );
	}

	if( !Route.Num() )
	{
		RouteString = TEXT("   (Object is not currently rooted)\r\n");
	}
	return RouteString;
}

FArchiveTraceRoute::FArchiveTraceRoute( TMap<UObject*,FTraceRouteRecord>& InRoutes, UBOOL bShouldIncludeTransients, EObjectFlags KeepFlags )
:	Routes(InRoutes)
,	Depth(0)
,	Referencer(NULL)
,	bIncludeTransients(bShouldIncludeTransients)
{
	ArIsObjectReferenceCollector = TRUE;
	
	for( FObjectIterator It; It; ++It )
	{
		It->SetFlags( RF_TagExp );
	}

	UObject::SerializeRootSet( *this, KeepFlags );

	for( FObjectIterator It; It; ++It )
	{
		It->ClearFlags( RF_TagExp );
	}
}

FArchive& FArchiveTraceRoute::operator<<( class UObject*& Obj )
{
	if( Obj )
	{
		if ( bIncludeTransients || !Obj->HasAnyFlags(RF_Transient) )
		{
			FTraceRouteRecord* Rec = Routes.Find(Obj);
			if( !Rec || Depth<Rec->Depth )
			{
				Routes.Set( Obj, FTraceRouteRecord(Depth,Referencer) );
			}

			if( Obj->HasAnyFlags(RF_TagExp) )
			{
				Obj->ClearFlags( RF_TagExp );
				UObject* SavedPrev = Referencer;
				Referencer = Obj;
				Depth++;
				Obj->Serialize( *this );
				Depth--;
				Referencer = SavedPrev;
			}
		}
	}
	return *this;
}
/*----------------------------------------------------------------------------
	FFindReferencersArchive.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	PotentialReferencer		the object to serialize which may contain references to our target objects
 * @param	InTargetObjects			array of objects to search for references to
 */
FFindReferencersArchive::FFindReferencersArchive( UObject* PotentialReferencer, TArray<UObject*> InTargetObjects )
{
	// use the optimized RefLink to skip over properties which don't contain object references
	ArIsObjectReferenceCollector = TRUE;

	// ALL objects reference their outers...it's just log spam here
	ArIgnoreOuterRef = TRUE;

	// initialize the map
	for ( INT ObjIndex = 0; ObjIndex < InTargetObjects.Num(); ObjIndex++ )
	{
		UObject* TargetObject = InTargetObjects(ObjIndex);
		if ( TargetObject != NULL && TargetObject != PotentialReferencer )
		{
			TargetObjects.Set(TargetObject, 0);
		}
	}

	// now start the search
	PotentialReferencer->Serialize( *this );
}

/**
 * Retrieves the number of references from PotentialReferencer to the object specified.
 *
 * @param	TargetObject	the object to might be referenced
 * @param	out_ReferencingProperties
 *							receives the list of properties which were holding references to TargetObject
 *
 * @return	the number of references to TargetObject which were encountered when PotentialReferencer
 *			was serialized.
 */
INT FFindReferencersArchive::GetReferenceCount( UObject* TargetObject, TArray<UProperty*>* out_ReferencingProperties/*=NULL*/ ) const
{
	INT Result = 0;
	if ( TargetObject != NULL )
	{
		const INT* pCount = TargetObjects.Find(TargetObject);
		if ( pCount != NULL && (*pCount) > 0 )
		{
			Result = *pCount;
			if ( out_ReferencingProperties != NULL )
			{
				TArray<UProperty*> PropertiesReferencingObj;
				ReferenceMap.MultiFind(TargetObject, PropertiesReferencingObj);

				out_ReferencingProperties->Empty(PropertiesReferencingObj.Num());
				for ( INT PropIndex = PropertiesReferencingObj.Num() - 1; PropIndex >= 0; PropIndex-- )
				{
					out_ReferencingProperties->AddItem(PropertiesReferencingObj(PropIndex));
				}
			}
		}
	}

	return Result;
}

/**
 * Retrieves the number of references from PotentialReferencer list of TargetObjects
 *
 * @param	out_ReferenceCounts		receives the number of references to each of the TargetObjects
 *
 * @return	the number of objects which were referenced by PotentialReferencer.
 */
INT FFindReferencersArchive::GetReferenceCounts( TMap<UObject*, INT>& out_ReferenceCounts ) const
{
	out_ReferenceCounts.Empty();
	for ( TMap<UObject*,INT>::TConstIterator It(TargetObjects); It; ++It )
	{
		if ( It.Value() > 0 )
		{
			out_ReferenceCounts.Set(It.Key(), It.Value());
		}
	}

	return out_ReferenceCounts.Num();
}

/**
 * Retrieves the number of references from PotentialReferencer list of TargetObjects
 *
 * @param	out_ReferenceCounts			receives the number of references to each of the TargetObjects
 * @param	out_ReferencingProperties	receives the map of properties holding references to each referenced object.
 *
 * @return	the number of objects which were referenced by PotentialReferencer.
 */
INT FFindReferencersArchive::GetReferenceCounts( TMap<UObject*, INT>& out_ReferenceCounts, TMultiMap<UObject*,UProperty*>& out_ReferencingProperties ) const
{
	GetReferenceCounts(out_ReferenceCounts);
	if ( out_ReferenceCounts.Num() > 0 )
	{
		out_ReferencingProperties.Empty();
		for ( TMap<UObject*,INT>::TIterator It(out_ReferenceCounts); It; ++It )
		{
			UObject* Object = It.Key();

			TArray<UProperty*> PropertiesReferencingObj;
			ReferenceMap.MultiFind(Object, PropertiesReferencingObj);

			for ( INT PropIndex = PropertiesReferencingObj.Num() - 1; PropIndex >= 0; PropIndex-- )
			{
				out_ReferencingProperties.Add(Object, PropertiesReferencingObj(PropIndex));
			}
		}
	}

	return out_ReferenceCounts.Num();
}

/**
 * Serializer - if Obj is one of the objects we're looking for, increments the reference count for that object
 */
FArchive& FFindReferencersArchive::operator<<( UObject*& Obj )
{
	if ( Obj != NULL )
	{
		INT* pReferenceCount = TargetObjects.Find(Obj);
		if ( pReferenceCount != NULL )
		{
			// if this object was serialized via a UProperty, add it to the list
			if ( GSerializedProperty != NULL )
			{
				ReferenceMap.AddUnique(Obj, GSerializedProperty);
			}

			// now increment the reference count for this target object
			(*pReferenceCount)++;
		}
	}

	return *this;
}

/*----------------------------------------------------------------------------
	FArchiveFindCulprit.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	InFind				the object that we'll be searching for references to
 * @param	Src					the object to serialize which may contain a reference to InFind
 * @param	InPretendSaving		if TRUE, marks the archive as saving and persistent, so that a different serialization codepath is followed
 */
FArchiveFindCulprit::FArchiveFindCulprit( UObject* InFind, UObject* Src, UBOOL InPretendSaving )
: Find(InFind), Count(0), PretendSaving(InPretendSaving)
{
	// use the optimized RefLink to skip over properties which don't contain object references
	ArIsObjectReferenceCollector = TRUE;

	// ALL objects reference their outers...it's just log spam here
	ArIgnoreOuterRef = TRUE;

	if( PretendSaving )
	{
		ArIsSaving		= TRUE;
		ArIsPersistent	= TRUE;
	}

	Src->Serialize( *this );
}

FArchive& FArchiveFindCulprit::operator<<( UObject*& Obj )
{
	if( Obj==Find )
	{
		if ( GSerializedProperty != NULL )
		{
			Referencers.AddUniqueItem(GSerializedProperty);
		}
		Count++;
	}

	if( PretendSaving && Obj && !Obj->IsPendingKill() )
	{
		if( (!Obj->HasAnyFlags(RF_Transient) || Obj->HasAnyFlags(RF_Public)) && !Obj->HasAnyFlags(RF_TagExp) )
		{
			if ( Obj->HasAnyFlags(RF_Native|RF_Standalone|RF_RootSet) )
			{
				// serialize the object's Outer if this object could potentially be rooting the object we're attempting to find references to
				// otherwise, it's just spam
				*this << Obj->Outer;
			}

			// serialize the object's ObjectArchetype
			*this << Obj->ObjectArchetype;
		}
	}
	return *this;
}

/*----------------------------------------------------------------------------
	FDuplicateDataReader.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	InDuplicatedObjects		map of original object to copy of that object
 * @param	InObjectData			object data to read from
 */
FDuplicateDataReader::FDuplicateDataReader(const TMap<UObject*,FDuplicatedObjectInfo*>& InDuplicatedObjects,const TArray<BYTE>& InObjectData)
: DuplicatedObjects(InDuplicatedObjects)
, ObjectData(InObjectData)
, Offset(0)
{
	ArIsLoading			= TRUE;
	ArIsPersistent		= TRUE;
	ArPortFlags |= PPF_Duplicate;
}

FArchive& FDuplicateDataReader::operator<<( UObject*& Object )
{
	UObject*	SourceObject = Object;
	Serialize(&SourceObject,sizeof(UObject*));
	const FDuplicatedObjectInfo*	ObjectInfo = DuplicatedObjects.FindRef(SourceObject);
	if(ObjectInfo)
	{
		Object = ObjectInfo->DupObject;
	}
	else
	{
		Object = SourceObject;
	}

	return *this;
}

/*----------------------------------------------------------------------------
	FDuplicateDataWriter.
----------------------------------------------------------------------------*/
/**
 * Constructor
 *
 * @param	InDuplicatedObjects		will contain the original object -> copy mappings
 * @param	InObjectData			will store the serialized data
 * @param	SourceObject			the object to copy
 * @param	DestObject				the object to copy to
 * @param	InFlagMask				the flags that should be copied when the object is duplicated
 * @param	InApplyFlags			the flags that should always be set on the duplicated objects (regardless of whether they're set on the source)
 * @param	InInstanceGraph			the instancing graph to use when creating the duplicate objects.
 */
FDuplicateDataWriter::FDuplicateDataWriter(TMap<UObject*,FDuplicatedObjectInfo*>& InDuplicatedObjects,TArray<BYTE>& InObjectData,UObject* SourceObject,
										   UObject* DestObject,EObjectFlags InFlagMask, EObjectFlags InApplyFlags, FObjectInstancingGraph* InInstanceGraph)
: DuplicatedObjects(InDuplicatedObjects)
, ObjectData(InObjectData)
, Offset(0)
, FlagMask(InFlagMask)
, ApplyFlags(InApplyFlags)
, InstanceGraph(InInstanceGraph)
{
	ArIsSaving			= TRUE;
	ArIsPersistent		= TRUE;
	ArAllowLazyLoading	= FALSE;
	ArPortFlags |= PPF_Duplicate;

	AddDuplicate(SourceObject,DestObject);
}

/**
 * I/O function
 */
FArchive& FDuplicateDataWriter::operator<<(UObject*& Object)
{
	GetDuplicatedObject(Object);

	// store the pointer to this object
	Serialize(&Object,sizeof(UObject*));
	return *this;
}

/**
 * Places a new duplicate in the DuplicatedObjects map as well as the UnserializedObjects list
 *
 * @param	SourceObject		the original version of the object
 * @param	DuplicateObject		the copy of the object
 *
 * @return	a pointer to the copy of the object
 */
UObject* FDuplicateDataWriter::AddDuplicate(UObject* SourceObject,UObject* DupObject)
{
	// Check for an existing duplicate of the object; if found, use that one instead of creating a new one.
	FDuplicatedObjectInfo* Info = DuplicatedObjects.FindRef(SourceObject);
	if ( Info == NULL )
	{
		Info = DuplicatedObjects.Set(SourceObject,new FDuplicatedObjectInfo());
	}
	Info->DupObject = DupObject;

	TMap<FName,UComponent*>	ComponentInstanceMap;
	SourceObject->CollectComponents(ComponentInstanceMap, FALSE);

	for(TMap<FName,UComponent*>::TIterator It(ComponentInstanceMap);It;++It)
	{
		const FName& ComponentName = It.Key();
		UComponent*& Component = It.Value();

		UObject* DuplicatedComponent = GetDuplicatedObject(Component);
		Info->ComponentInstanceMap.Set( Component, Cast<UComponent>(DuplicatedComponent) );
	}

	UnserializedObjects.AddItem(SourceObject);
	return DupObject;
}

/**
 * Returns a pointer to the duplicate of a given object, creating the duplicate object if necessary.
 *
 * @param	Object	the object to find a duplicate for
 *
 * @return	a pointer to the duplicate of the specified object
 */
UObject* FDuplicateDataWriter::GetDuplicatedObject( UObject* Object )
{
	UObject* Result = NULL;
	if( Object != NULL )
	{
		// Check for an existing duplicate of the object.
		FDuplicatedObjectInfo*	DupObjectInfo = DuplicatedObjects.FindRef(Object);
		if( DupObjectInfo != NULL )
		{
			Result = DupObjectInfo->DupObject;
		}
		else
		{
			// Check to see if the object's outer is being duplicated.
			UObject*	DupOuter = GetDuplicatedObject(Object->GetOuter());
			if(DupOuter != NULL)
			{
				// The object's outer is being duplicated, create a duplicate of this object.
				Result = AddDuplicate(Object,UObject::StaticConstructObject(Object->GetClass(),DupOuter,*Object->GetName(),ApplyFlags|Object->GetMaskedFlags(FlagMask),
					Object->GetArchetype(),GError,INVALID_OBJECT,InstanceGraph));
			}
		}
	}

	return Result;
}

/*----------------------------------------------------------------------------
	Saving Packages.
----------------------------------------------------------------------------*/
/**
 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
 * is in when a loading error occurs.
 *
 * This is overridden for the specific Archive Types
 **/
FName FArchiveSaveTagExports::GetArchiveName() const
{
	return Outer != NULL
		? *FString::Printf(TEXT("SaveTagExports (%s)"), *Outer->GetName())
		: TEXT("SaveTagExports");
}

FArchive& FArchiveSaveTagExports::operator<<( UObject*& Obj )
{
	if( Obj && (Obj->IsIn(Outer) || Obj->HasAnyFlags(RF_ForceTagExp)) && !Obj->HasAnyFlags(RF_Transient|RF_TagExp) )
	{
#if 0
		// the following line should be used to track down the cause behind
		// "Load flags don't match result of *" errors.  The most common reason
		// is a child component template is modifying the load flags of it parent
		// component template (that is, the component template that it is overriding
		// from a parent class).
		// @MISMATCHERROR
		if ( Obj->GetFName() == TEXT("BrushComponent0") && Obj->GetOuter()->GetFName() == TEXT("Default__Volume") )
		{
			debugf(TEXT(""));
		}
#endif
		// Object is in outer so we don't need RF_ForceTagExp.
		if( Obj->IsIn(Outer) )
		{
			Obj->ClearFlags(RF_ForceTagExp);
		}

		// Set flags.
		Obj->SetFlags(RF_TagExp);

		// first, serialize this object's archetype so that if the archetype's load flags are set correctly if this object
		// is encountered by the SaveTagExports archive before its archetype.  This is necessary for the code below which verifies
		// that the load flags for the archetype and the load flags for the object match
		UObject* Template = Obj->GetArchetype();
		*this << Template;

		if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
		{
			if ( Obj->GetClass()->HasAnyClassFlags(CLASS_Intrinsic) )
			{
				// if this class is an intrinsic class, its class default object
				// shouldn't be saved, as it does not contain any data that should be saved
				Obj->ClearFlags(RF_TagExp);
			}
			else
			{
				// class default objects should always be loaded
				Obj->SetFlags(RF_LoadContextFlags);
			}
		}
		else
		{
			if( Obj->NeedsLoadForEdit() )
			{
				Obj->SetFlags(RF_LoadForEdit);
			}

			if( Obj->NeedsLoadForClient() )
			{
				Obj->SetFlags(RF_LoadForClient);
			}

			if( Obj->NeedsLoadForServer() )
			{
				Obj->SetFlags(RF_LoadForServer);
			}

			// skip these checks if the Template object is the CDO for an intrinsic class, as they will never have any load flags set.
			if ( Template != NULL 
			&& (!Template->GetClass()->HasAnyClassFlags(CLASS_Intrinsic) || !Template->HasAnyFlags(RF_ClassDefaultObject)) )
			{
				EObjectFlags PropagateFlags = Obj->GetMaskedFlags(RF_LoadContextFlags);

				// if the object's Template is not in the same package, we can't adjust its load flags to ensure that this object can be loaded
				// therefore make this a critical error so that we don't discover later that we won't be able to load this object.
				if ( !Template->IsIn(Obj->GetOutermost())

				// if the template or the object have the RF_ForceTagExp flag, then they'll be saved into the same cooked package even if they don't
				// share the same Outermost...
				&& !Template->HasAnyFlags(RF_ForceTagExp)

				// if the object has the forcetagexp flag, no need to worry about mismatched load flags unless its template is not going to be saved into this package
				&& (!Obj->HasAnyFlags(RF_ForceTagExp) || !Template->IsIn(Outer)) )
				{
					// if the component's archetype is not in the same package, we won't be able
					// to propagate the load flags to the archetype, so make sure that the
					// derived template's load flags are <= the parent's load flags.
					FString LoadFlagsString;
					if ( Obj->HasAnyFlags(RF_LoadForEdit) && !Template->NeedsLoadForEdit() )
					{
						LoadFlagsString = TEXT("RF_LoadForEdit");
					}
					if ( Obj->HasAnyFlags(RF_LoadForClient) && !Template->NeedsLoadForClient() )
					{
						if ( LoadFlagsString.Len() > 0 )
						{
							LoadFlagsString += TEXT(",");
						}
						LoadFlagsString += TEXT("RF_LoadForClient");
					}
					if ( Obj->HasAnyFlags(RF_LoadForServer) && !Template->NeedsLoadForServer() )
					{
						if ( LoadFlagsString.Len() > 0 )
						{
							LoadFlagsString += TEXT(",");
						}
						LoadFlagsString += TEXT("RF_LoadForServer");
					}

					if ( LoadFlagsString.Len() > 0 )
					{
						if ( Obj->IsA(UComponent::StaticClass()) && Template->IsTemplate() )
						{
							//@todo localize
							appErrorf(TEXT("Mismatched load flag/s (%s) on component template parent from different package.  '%s' cannot be derived from '%s'"),
								*LoadFlagsString, *Obj->GetPathName(), *Template->GetPathName());
						}
						else
						{
							//@todo localize
							appErrorf(TEXT("Mismatched load flag/s (%s) on object archetype from different package.  Loading '%s' would fail because its archetype '%s' wouldn't be created."),
								*LoadFlagsString, *Obj->GetPathName(), *Template->GetPathName());
						}
					}
				}

				// this is a normal object - it's template MUST be loaded anytime
				// the object is loaded, so make sure the load flags match the object's
				// load flags (note that it's OK for the template itself to be loaded
				// in situations where the object is not, but vice-versa is not OK)
				Template->SetFlags( PropagateFlags );
			}
		}

		// Recurse with this object's class and package.
		UObject* Class  = Obj->GetClass();
		UObject* Parent = Obj->GetOuter();
		*this << Class << Parent;

		// Recurse with this object's children.
		if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
		{
			((UClass*)Class)->SerializeDefaultObject(Obj, *this);
		}
		else
		{
			Obj->Serialize( *this );
		}
	}
	return *this;
}


/**
 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
 * is in when a loading error occurs.
 *
 * This is overridden for the specific Archive Types
 **/
FName FArchiveSaveTagImports::GetArchiveName() const
{
	if ( Linker != NULL && Linker->LinkerRoot )
	{
		return *FString::Printf(TEXT("SaveTagImports (%s)"), *Linker->LinkerRoot->GetName());
	}

	return TEXT("SaveTagImports");
}

FArchive& FArchiveSaveTagImports::operator<<( UObject*& Obj )
{
	if( Obj && !Obj->IsPendingKill() )
	{
		if( !Obj->HasAnyFlags(RF_Transient) || Obj->HasAllFlags(RF_Native) )
		{
			// remember it as a dependency, unless it's a top level pacakge or native
			UBOOL bIsTopLevelPackage = Obj->GetOuter() == NULL && Obj->IsA(UPackage::StaticClass());
			UBOOL bIsNative = Obj->HasAnyFlags(RF_Native);
			UObject* Outer = Obj->GetOuter();
			
			// go up looking for native classes
			while (!bIsNative && Outer)
			{
				if (Outer->IsA(UClass::StaticClass()) && Outer->HasAnyFlags(RF_Native))
				{
					bIsNative = true;
				}
				Outer = Outer->GetOuter();
			}

			// only add valid objects
			if (!bIsTopLevelPackage && !bIsNative)
			{
				Dependencies.AddUniqueItem(Obj);
			}

			if( !Obj->HasAnyFlags(RF_TagExp) )
			{
				// mark this object as an import
				Obj->SetFlags( RF_TagImp );

				if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
				{
					Obj->SetFlags(RF_LoadContextFlags);
				}
				else
				{
					if( !Obj->HasAnyFlags( RF_NotForEdit  ) )
					{
						Obj->SetFlags(RF_LoadForEdit);
					}

					if( !Obj->HasAnyFlags( RF_NotForClient) )
					{
						Obj->SetFlags(RF_LoadForClient);
					}

					if( !Obj->HasAnyFlags( RF_NotForServer) )
					{
						Obj->SetFlags(RF_LoadForServer);
					}
				}

				UObject* Parent = Obj->GetOuter();
				if( Parent )
				{
					*this << Parent;
				}
			}
		}
	}
	return *this;
}

/*----------------------------------------------------------------------------
	FArchiveObjectReferenceCollector.
----------------------------------------------------------------------------*/

/** 
 * UObject serialize operator implementation
 *
 * @param Object	reference to Object reference
 * @return reference to instance of this class
 */
FArchive& FArchiveObjectReferenceCollector::operator<<( UObject*& Object )
{
	// Avoid duplicate entries.
	if ( Object != NULL )
	{
		if ( (LimitOuter == NULL || (Object->GetOuter() == LimitOuter || (!bRequireDirectOuter && Object->IsIn(LimitOuter)))) )
		{
			if ( !ObjectArray->ContainsItem(Object) )
			{
				check( Object->IsValid() );
				ObjectArray->AddItem( Object );

				// check this object for any potential object references
				if ( bSerializeRecursively )
				{
					Object->Serialize(*this);
				}
			}
		}
	}
	return *this;
}






