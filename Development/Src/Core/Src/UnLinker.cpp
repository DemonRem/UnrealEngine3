/*=============================================================================
	UnLinker.cpp: Unreal object linker.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"
#include "DebuggingDefines.h"

#define TIME_PACKAGE_LOADING 0

/** Map that keeps track of any precached full package reads															*/
TMap<FString, ULinkerLoad::FPackagePrecacheInfo> ULinkerLoad::PackagePrecacheMap;

/*-----------------------------------------------------------------------------
	Helper functions.
-----------------------------------------------------------------------------*/

/** 
 * Returns whether we should ignore the fact that this class has been removed instead of deprecated. 
 * Normally the script compiler would spit out an error but it makes sense to silently ingore it in 
 * certain cases in which case the below code should be extended to include the class' name.
 *
 * @param	ClassName	Name of class to find out whether we should ignore complaining about it not being present
 * @return	TRUE if we should ignore the fact that it doesn't exist, FALSE otherwise
 */
static UBOOL IgnoreMissingReferencedClass( FName ClassName )
{
	static TArray<FName>	MissingClassesToIgnore;
	static UBOOL			bAlreadyInitialized = FALSE;
	if( !bAlreadyInitialized )
	{
		//@deprecated with VER_RENDERING_REFACTOR
		MissingClassesToIgnore.AddItem( FName(TEXT("SphericalHarmonicMap")) );
		MissingClassesToIgnore.AddItem( FName(TEXT("LightMap1D")) );
		MissingClassesToIgnore.AddItem( FName(TEXT("LightMap2D")) );
		bAlreadyInitialized = TRUE;
	}
	return MissingClassesToIgnore.FindItemIndex( ClassName ) != INDEX_NONE;
}

static inline INT HashNames( FName A, FName B, FName C )
{
	return A.GetIndex() + 7 * B.GetIndex() + 31*C.GetIndex();
}

/*-----------------------------------------------------------------------------
	FObjectResource
-----------------------------------------------------------------------------*/

FObjectResource::FObjectResource()
{}

FObjectResource::FObjectResource( UObject* InObject )
:	ObjectName		( InObject ? InObject->GetFName() : FName(NAME_None)		)
,	OuterIndex		( 0															)
{
}

/*-----------------------------------------------------------------------------
	FObjectExport.
-----------------------------------------------------------------------------*/

FObjectExport::FObjectExport()
:	FObjectResource	()
,	ClassIndex		( 0															)
,	SuperIndex		( 0															)
,	ArchetypeIndex	( 0															)
,	ObjectFlags		( 0															)
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	ScriptSerializationStartOffset	( 0											)
,	ScriptSerializationEndOffset	( 0											)
,	_Object			( NULL														)
,	_iHashNext		( INDEX_NONE												)
,	ExportFlags		( EF_None													)
,	PackageGuid		( FGuid(0,0,0,0)											)
{}

FObjectExport::FObjectExport( UObject* InObject )
:	FObjectResource	( InObject													)
,	ClassIndex		( 0															)
,	SuperIndex		( 0															)
,	ArchetypeIndex	( 0															)
,	ObjectFlags		( InObject ? InObject->GetMaskedFlags(RF_Load) : 0			)
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	ScriptSerializationStartOffset	( 0											)
,	ScriptSerializationEndOffset	( 0											)
,	_Object			( InObject													)
,	_iHashNext		( INDEX_NONE												)
,	ExportFlags		( EF_None													)
,	PackageGuid		( FGuid(0,0,0,0)											)
{
	// Objects that are forced into the export table need to be dissociated after
	// loading as they might have been associated via FindObject and will have
	// their linker reset so they won't correctly dissociate with this export via 
	// SetLinker in the GC code.
	if( _Object && _Object->HasAnyFlags( RF_ForceTagExp ) )
	{
		UObject::GForcedExportCount++;
		SetFlags( EF_ForcedExport );
	}
}

FArchive& operator<<( FArchive& Ar, FObjectExport& E )
{
	Ar << E.ClassIndex;
	Ar << E.SuperIndex;
	Ar << E.OuterIndex;
	Ar << E.ObjectName;
	Ar << E.ArchetypeIndex;	
	Ar << E.ObjectFlags;

	Ar << E.SerialSize;
	if( E.SerialSize || (Ar.Ver() >= VER_MOVED_EXPORTIMPORTMAPS_ADDED_TOTALHEADERSIZE) )
	{
		Ar << E.SerialOffset;
	}
	Ar << E.ComponentMap;

	if( Ar.Ver() < VER_FOBJECTEXPORT_EXPORTFLAGS )
	{
		E.ExportFlags = EF_None;
	}
	else
	{
		Ar << E.ExportFlags;
	}

	if (Ar.Ver() >= VER_LINKERFREE_PACKAGEMAP)
	{
		Ar << E.GenerationNetObjectCount;
		Ar << E.PackageGuid;
	}

	return Ar;
}

/*-----------------------------------------------------------------------------
	FObjectImport.
-----------------------------------------------------------------------------*/

FObjectImport::FObjectImport()
:	FObjectResource	()
{}

FObjectImport::FObjectImport( UObject* InObject )
:	FObjectResource	( InObject																)
,	ClassPackage	( InObject ? InObject->GetClass()->GetOuter()->GetFName()	: NAME_None	)
,	ClassName		( InObject ? InObject->GetClass()->GetFName()				: NAME_None	)
,	XObject			( InObject																)
,	SourceLinker	( NULL																	)
,	SourceIndex		( INDEX_NONE															)
{
	if( XObject != NULL )
	{
		UObject::GImportCount++;
	}
}

FArchive& operator<<( FArchive& Ar, FObjectImport& I )
{
	Ar << I.ClassPackage << I.ClassName;
	Ar << I.OuterIndex;
	Ar << I.ObjectName;
	if( Ar.IsLoading() )
	{
		I.SourceLinker	= NULL;
		I.SourceIndex	= INDEX_NONE;
		I.XObject		= NULL;
	}
	return Ar;
}

/*----------------------------------------------------------------------------
	FCompressedChunk.
----------------------------------------------------------------------------*/

FCompressedChunk::FCompressedChunk()
:	UncompressedOffset(0)
,	UncompressedSize(0)
,	CompressedOffset(0)
,	CompressedSize(0)
{}

/** I/O function */
FArchive& operator<<(FArchive& Ar,FCompressedChunk& Chunk)
{
	Ar << Chunk.UncompressedOffset;
	Ar << Chunk.UncompressedSize;
	Ar << Chunk.CompressedOffset;
	Ar << Chunk.CompressedSize;
	return Ar;
}


/*----------------------------------------------------------------------------
	Items stored in Unrealfiles.
----------------------------------------------------------------------------*/

FGenerationInfo::FGenerationInfo(INT InExportCount, INT InNameCount, INT InNetObjectCount)
: ExportCount(InExportCount), NameCount(InNameCount), NetObjectCount(InNetObjectCount)
{}

/** I/O function
 * we use a function instead of operator<< so we can pass in the package file summary for version tests, since Ar.Ver() hasn't been set yet
 */
void FGenerationInfo::Serialize(FArchive& Ar, const struct FPackageFileSummary& Summary)
{
	Ar << ExportCount << NameCount;
	if (Summary.GetFileVersion() >= VER_LINKERFREE_PACKAGEMAP)
	{
		Ar << NetObjectCount;
	}
}

FPackageFileSummary::FPackageFileSummary()
{
	appMemzero( this, sizeof(*this) );
}

FArchive& operator<<( FArchive& Ar, FPackageFileSummary& Sum )
{
	Ar << Sum.Tag;
	// only keep loading if we match the magic
	if( Sum.Tag == PACKAGE_FILE_TAG || Sum.Tag == PACKAGE_FILE_TAG_SWAPPED )
	{
		// The package has been stored in a separate endianness than the linker expected so we need to force
		// endian conversion. Latent handling allows the PC version to retrieve information about cooked packages.
		if( Sum.Tag == PACKAGE_FILE_TAG_SWAPPED )
		{
			// Set proper tag.
			Sum.Tag = PACKAGE_FILE_TAG;
			// Toggle forced byte swapping.
			if( Ar.ForceByteSwapping() )
			{
				Ar.SetByteSwapping( FALSE );
			}
			else
			{
				Ar.SetByteSwapping( TRUE );
			}
		}

		Ar << Sum.FileVersion;

		// We cannot use Ar.Ver() as it hasn't been set yet so we manually copy the code from UnLinker.h.
		if( (Sum.FileVersion & 0xffff) >= VER_MOVED_EXPORTIMPORTMAPS_ADDED_TOTALHEADERSIZE )
		{
			Ar << Sum.TotalHeaderSize;
		}
		else
		{
			Sum.TotalHeaderSize = 0;
		}

		if ( (Sum.FileVersion & 0xffff) >= VER_FOLDER_ADDED )
		{
			Ar << Sum.FolderName;
		}
		else
		{
			Sum.FolderName = TEXT("");
		}

		Ar << Sum.PackageFlags;
		Ar << Sum.NameCount     << Sum.NameOffset;
		Ar << Sum.ExportCount   << Sum.ExportOffset;
		Ar << Sum.ImportCount   << Sum.ImportOffset;

		// We cannot use Ar.Ver() as it hasn't been set yet so we manually copy the code from UnLinker.h.
		if ((Sum.FileVersion & 0xffff) >= VER_ADDED_LINKER_DEPENDENCIES)
		{
			Ar << Sum.DependsOffset;
		}

		INT GenerationCount = Sum.Generations.Num();
		Ar << Sum.Guid << GenerationCount;
		if( Ar.IsLoading() && GenerationCount > 0 )
		{
			Sum.Generations = TArray<FGenerationInfo>( GenerationCount );
		}
		for( INT i=0; i<GenerationCount; i++ )
		{
			Sum.Generations(i).Serialize(Ar, Sum);
		}

		// We cannot use Ar.Ver() as it hasn't been set yet so we manually copy the code from UnLinker.h.
		if( (Sum.FileVersion & 0xffff) >= VER_PACKAGEFILESUMMARY_CHANGE )
		{
			Ar << Sum.EngineVersion;
		}
		else
		{
			Sum.EngineVersion = 0;
		}

		// We cannot use Ar.Ver() as it hasn't been set yet so we manually copy the code from UnLinker.h.
		if( (Sum.FileVersion & 0xffff) >= VER_PACKAGEFILESUMMARY_CHANGE_COOK_VER_ADDED )
		{
			// grab the CookedContentVersion if we are saving while cooking or loading 
			if( ( GIsCooking == TRUE ) || ( Ar.IsLoading() == TRUE ) )
			{
				Ar << Sum.CookedContentVersion;
			}
			else
			{
				INT Temp = 0;
				Ar << Temp;  // just put a zero as it not a cooked package and we should not dirty the waters
			}
		}
		else
		{
			Sum.CookedContentVersion = 0;
		}

		// We cannot use Ar.Ver() as it hasn't been set yet so we manually copy the code from UnLinker.h.
		if( (Sum.FileVersion & 0xffff) >= VER_ADDED_PACKAGE_COMPRESSION_SUPPORT )
		{
			Ar << Sum.CompressionFlags;
			Ar << Sum.CompressedChunks;
		}
	}

	return Ar;
}

/*----------------------------------------------------------------------------
	ULinker.
----------------------------------------------------------------------------*/

ULinker::ULinker( UPackage* InRoot, const TCHAR* InFilename )
:	LinkerRoot( InRoot )
,	Summary()
,	Filename( InFilename )
,	_ContextFlags( 0 )
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	check(LinkerRoot);
	check(InFilename);

	// Set context flags.
	if( GIsEditor ) 
	{
		_ContextFlags |= RF_LoadForEdit;
	}
	if( GIsClient ) 
	{
		_ContextFlags |= RF_LoadForClient;
	}
	if( GIsServer ) 
	{
		_ContextFlags |= RF_LoadForServer;
	}
}

/**
 * Static constructor called once per class during static initialization via IMPLEMENT_CLASS
 * macro. Used to e.g. emit object reference tokens for realtime garbage collection or expose
 * properties for native- only classes.
 */
void ULinker::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference( STRUCT_OFFSET( ULinker, LinkerRoot ) );
	const DWORD SkipIndexIndex = TheClass->EmitStructArrayBegin( STRUCT_OFFSET( ULinker, ImportMap ), sizeof(FObjectImport) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( FObjectImport, SourceLinker ) );
	TheClass->EmitStructArrayEnd( SkipIndexIndex );
}

void ULinker::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( Ar.IsCountingMemory() )
	{
		// Can't use CountBytes as ExportMap is array of structs of arrays.
		Ar << ImportMap;
		Ar << ExportMap;
	}

	// Prevent garbage collecting of linker's names and package.
	Ar << NameMap << LinkerRoot;
	{for( INT i=0; i<ExportMap.Num(); i++ )
	{
		FObjectExport& E = ExportMap(i);
		Ar << E.ObjectName;
	}}
	{for( INT i=0; i<ImportMap.Num(); i++ )
	{
		FObjectImport& I = ImportMap(i);
		Ar << *(UObject**)&I.SourceLinker;
		Ar << I.ClassPackage << I.ClassName;
	}}
}

// ULinker interface.
/**
 * Return the path name of the UObject represented by the specified import. 
 * (can be used with StaticFindObject)
 * 
 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
 *
 * @return	the path name of the UObject represented by the resource at ImportIndex
 */
FString ULinker::GetImportPathName(INT ImportIndex)
{
	FString S;
	for (INT LinkerIndex = -ImportIndex - 1; LinkerIndex != 0;)
	{
		if (LinkerIndex != -ImportIndex - 1)
		{
			S = US + TEXT(".") + S;
		}

		// seek free loading can make import's outers be exports, not imports
		if ((LinkerRoot->PackageFlags & PKG_Cooked) && !IS_IMPORT_INDEX(LinkerIndex))
		{
			S = ExportMap(LinkerIndex - 1).ObjectName.ToString() + S;
			LinkerIndex = ExportMap(LinkerIndex - 1).OuterIndex;
		}
		else
		{
			S = ImportMap(-LinkerIndex - 1).ObjectName.ToString() + S;
			LinkerIndex = ImportMap(-LinkerIndex - 1).OuterIndex;
		}
	}
	return S;
}

/**
 * Return the path name of the UObject represented by the specified export.
 * (can be used with StaticFindObject)
 * 
 * @param	ExportIndex				index into the ExportMap for the resource to get the name for
 * @param	FakeRoot				Optional name to replace use as the root package of this object instead of the linker
 * @param	bResolveForcedExports	if TRUE, the package name part of the return value will be the export's original package,
 *									not the name of the package it's currently contained within.
 *
 * @return	the path name of the UObject represented by the resource at ExportIndex
 */
FString ULinker::GetExportPathName(INT ExportIndex, const TCHAR* FakeRoot,UBOOL bResolveForcedExports/*=FALSE*/)
{
	FString Result;

	UBOOL bForcedExport = FALSE;
	for ( INT LinkerIndex = ExportIndex + 1; LinkerIndex != ROOTPACKAGE_INDEX; LinkerIndex = ExportMap(LinkerIndex - 1).OuterIndex)
	{ 
		if (LinkerIndex != ExportIndex + 1)
			Result = US + TEXT(".") + Result;
		Result = ExportMap(LinkerIndex - 1).ObjectName.ToString() + Result;
		bForcedExport = ExportMap(LinkerIndex - 1).HasAnyFlags(EF_ForcedExport);
	}

	if ( bForcedExport && FakeRoot == NULL && bResolveForcedExports )
	{
		// if the last export in the chain was a forced export, Result already contains the correct path name for this export
		return Result;
	}

	return (FakeRoot ? FakeRoot : LinkerRoot->GetPathName()) + TEXT(".") + Result;
}

/**
 * Return the full name of the UObject represented by the specified import.
 * 
 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
 *
 * @return	the full name of the UObject represented by the resource at ImportIndex
 */
FString ULinker::GetImportFullName(INT ImportIndex)
{
	return ImportMap(ImportIndex).ClassName.ToString() + TEXT(" ") + GetImportPathName(ImportIndex);
}

/**
 * Return the full name of the UObject represented by the specified export.
 * 
 * @param	ExportIndex				index into the ExportMap for the resource to get the name for
 * @param	FakeRoot				Optional name to replace use as the root package of this object instead of the linker
 * @param	bResolveForcedExports	if TRUE, the package name part of the return value will be the export's original package,
 *									not the name of the package it's currently contained within.
 *
 * @return	the full name of the UObject represented by the resource at ExportIndex
 */
FString ULinker::GetExportFullName(INT ExportIndex, const TCHAR* FakeRoot,UBOOL bResolveForcedExports/*=FALSE*/)
{
	INT ClassIndex = ExportMap(ExportIndex).ClassIndex;
	FName ClassName = ClassIndex > 0 ? ExportMap(ClassIndex - 1).ObjectName : ClassIndex < 0 ? ImportMap(-ClassIndex - 1).ObjectName : FName(NAME_Class);

	return ClassName.ToString() + TEXT(" ") + GetExportPathName(ExportIndex, FakeRoot, bResolveForcedExports);
}


/*----------------------------------------------------------------------------
	ULinkerLoad.
----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	ULinkerLoad creation BEGIN
-----------------------------------------------------------------------------*/

/**
 * Creates and returns a ULinkerLoad object.
 *
 * @param	Parent		Parent object to load into, can be NULL (most likely case)
 * @param	Filename	Name of file on disk to load
 * @param	LoadFlags	Load flags determining behavior
 *
 * @return	new ULinkerLoad object for Parent/ Filename
 */
ULinkerLoad* ULinkerLoad::CreateLinker( UPackage* Parent, const TCHAR* Filename, DWORD LoadFlags )
{
	ULinkerLoad* Linker = CreateLinkerAsync( Parent, Filename, LoadFlags );
	Linker->Tick( 0.f, FALSE );
	return Linker;
}

/**
 * Creates a ULinkerLoad object for async creation. Tick has to be called manually till it returns
 * TRUE in which case the returned linker object has finished the async creation process.
 *
 * @param	Parent		Parent object to load into, can be NULL (most likely case)
 * @param	Filename	Name of file on disk to load
 * @param	LoadFlags	Load flags determining behavior
 *
 * @return	new ULinkerLoad object for Parent/ Filename
 */
ULinkerLoad* ULinkerLoad::CreateLinkerAsync( UPackage* Parent, const TCHAR* Filename, DWORD LoadFlags )
{
	// See whether there already is a linker for this parent/ linker root.
	ULinkerLoad* Linker = NULL;
	for( INT LoaderIndex=0; LoaderIndex<GObjLoaders.Num(); LoaderIndex++ )
	{
		if( UObject::GetLoader(LoaderIndex)->LinkerRoot == Parent )
		{
			debugf(TEXT("ULinkerLoad::CreateLinkerAsync: Found existing linker for '%s'"), *Parent->GetName());
			Linker = UObject::GetLoader(LoaderIndex);
			break;
		}
	}

	// Create a new linker if there isn't an existing one.
	if( Linker == NULL )
	{
		if( GUseSeekFreeLoading )
		{
			LoadFlags |= LOAD_SeekFree;
		}
		Linker = new ULinkerLoad( Parent, Filename, LoadFlags );
	}
	return Linker;
}

/**
 * Ticks an in-flight linker and spends InTimeLimit seconds on creation. This is a soft time limit used
 * if bInUseTimeLimit is TRUE.
 *
 * @param	InTimeLimit		Soft time limit to use if bInUseTimeLimit is TRUE
 * @param	bInUseTimeLimit	Whether to use a (soft) timelimit
 * 
 * @return	TRUE if linker has finished creation, FALSE if it is still in flight
 */
UBOOL ULinkerLoad::Tick( FLOAT InTimeLimit, UBOOL bInUseTimeLimit )
{
	UBOOL bExecuteNextStep	= TRUE;

	if( bHasFinishedInitialization == FALSE )
	{
		// Store variables used by functions below.
		TickStartTime		= appSeconds();
		bTimeLimitExceeded	= FALSE;
		bUseTimeLimit		= bInUseTimeLimit;
		TimeLimit			= InTimeLimit;

		do
		{
			// Create loader, aka FArchive used for serialization and also precache the package file summary. FALSE is returned
			// till the first 128 KByte are cached.
			if( TRUE )
			{
				bExecuteNextStep = CreateLoader();
			}

			// Serialize the package file summary and presize the various arrays (name, import & export map)
			if( bExecuteNextStep )
			{
				bExecuteNextStep = SerializePackageFileSummary();
			}

			// Serialize the name map and register the names.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = SerializeNameMap();
			}

			// Serialize the import map.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = SerializeImportMap();
			}

			// Fix up import map for backward compatible serialization.
			if( bExecuteNextStep )
			{	
				bExecuteNextStep = FixupImportMap();
			}

			// Serialize the export map.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = SerializeExportMap();
			}

			// Serialize the export map.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = SerializeDependsMap();
			}

			// Hash exports.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = CreateExportHash();
			}

			// Find existing objects matching exports and associate them with this linker.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = FindExistingExports();
			}

			// Finalize creation process.
			if( bExecuteNextStep )
			{
				bExecuteNextStep = FinalizeCreation();
			}
		}
		// Loop till we are done if no time limit is specified.
		while( !bUseTimeLimit && !bExecuteNextStep );
	}

	// Return whether we completed or not.
	return bExecuteNextStep;
}

/**
 * Private constructor, passing arguments through from CreateLinker.
 *
 * @param	Parent		Parent object to load into, can be NULL (most likely case)
 * @param	Filename	Name of file on disk to load
 * @param	LoadFlags	Load flags determining behavior
 */
ULinkerLoad::ULinkerLoad( UPackage* InParent, const TCHAR* InFilename, DWORD InLoadFlags )
:	ULinker( InParent, InFilename )
,	LoadFlags( InLoadFlags )
,	bHaveImportsBeenVerified( FALSE )
{	
	check(!HasAnyFlags(RF_ClassDefaultObject));
}

/**
 * Returns whether the time limit allotted has been exceeded, if enabled.
 *
 * @param CurrentTask	description of current task performed for logging spilling over time limit
 * @param Granularity	Granularity on which to check timing, useful in cases where appSeconds is slow (e.g. PC)
 *
 * @return TRUE if time limit has been exceeded (and is enabled), FALSE otherwise (including if time limit is disabled)
 */
UBOOL ULinkerLoad::IsTimeLimitExceeded( const TCHAR* CurrentTask, INT Granularity )
{
	IsTimeLimitExceededCallCount++;
	if( !bTimeLimitExceeded 
	&&	bUseTimeLimit 
	&&  (IsTimeLimitExceededCallCount % Granularity) == 0 )
	{
		DOUBLE CurrentTime = appSeconds();
		bTimeLimitExceeded = CurrentTime - TickStartTime > TimeLimit;
#if CONSOLE
		// Log single operations that take longer than timelimit.
		if( (CurrentTime - TickStartTime) > (2.5 * TimeLimit) )
		{
 			debugfSuppressed(NAME_DevStreaming,TEXT("ULinkerLoad: %s took (less than) %5.2f ms"), 
 				CurrentTask, 
 				(CurrentTime - TickStartTime) * 1000);
		}
#endif
	}
	return bTimeLimitExceeded;
}

/**
 * Creates loader used to serialize content.
 */
UBOOL ULinkerLoad::CreateLoader()
{
	if( !Loader )
	{
		UBOOL bIsSeekFree = LoadFlags & LOAD_SeekFree;

		// NOTE: Precached memory read gets highest priority, then memory reader, then seek free, then normal

		// check to see if there is was an async preload request for this file
		FPackagePrecacheInfo* PrecacheInfo = PackagePrecacheMap.Find(*Filename);
		// if so, serialize from memory (note this will have uncompressed a fully compressed package)
		if (PrecacheInfo)
		{
#if TIME_PACKAGE_LOADING
			FLOAT Start = appSeconds();
#endif
			// block until the async read is complete
			while (PrecacheInfo->SynchronizationObject->GetValue() != 0)
			{
				appSleep(0);
			}
#if TIME_PACKAGE_LOADING
			debugf(TEXT(">>>>>>>>>>>>> Waited %.3f seconds for %s"), appSeconds() - Start, *Filename);
#endif

			// create a buffer reader using the read in data
			Loader = new FBufferReaderWithSHA(PrecacheInfo->PackageData, PrecacheInfo->PackageDataSize, TRUE, *Filename, TRUE);

			// remove the precache info from the map
			PackagePrecacheMap.Remove(*Filename);
		}
		// if we're seekfree loading, and we got here, that means we are trying to read a fully compressed package
		// without an async precache... this should no longer happen
		else if( GUseSeekFreeLoading && GFileManager->UncompressedFileSize(*Filename) != -1 )
		{
			appErrorf(TEXT("Loading a fully compressed package should only happen during startup with precaching."));
		}
		// if we aren't using seek free loading, check for the presence of a .uncompressed_size manifest file
		// if it exists, then the package was fully compressed, and must be uncompressed before using it
		else if( !GUseSeekFreeLoading && GFileManager->FileSize(*(Filename + TEXT(".uncompressed_size"))) != -1 )
		{
			FString SizeString;
			appLoadFileToString(SizeString, *(Filename + TEXT(".uncompressed_size")));
			check(SizeString.Len());
		
			// get the uncompressed size from the file
			INT UncompressedSize = appAtoi(*SizeString);

			// allocate space for uncompressed file
			void* Buffer = appMalloc(UncompressedSize);

			// open the file
            FArchive* CompressedFileReader = GFileManager->CreateFileReader(*Filename);
			check(CompressedFileReader);

			// read in and decompress data
#if WITH_LZO
			CompressedFileReader->SerializeCompressed(Buffer, UncompressedSize, COMPRESS_LZO);
#else	//#if WITH_LZO
			// NOTE: If this is reading in garbage, it is likely that the package was compressed with LZO...
			CompressedFileReader->SerializeCompressed(Buffer, UncompressedSize, COMPRESS_ZLIB);
#endif	//#if WITH_LZO

			// close the file
			delete CompressedFileReader;

			// create a buffer reader
			Loader = new FBufferReader(Buffer, UncompressedSize, TRUE, TRUE);
		}
		else if ((LoadFlags & LOAD_MemoryReader) || !bIsSeekFree)
		{
			// Create file reader used for serialization.
			FArchive* FileReader = GFileManager->CreateFileReader( *Filename, 0, GError );
			if( !FileReader )
			{
				appThrowf( *FString::Printf(*LocalizeError(TEXT("OpenFailed"),TEXT("Core")), *Filename, *GFileManager->GetCurrentDirectory()) );
			}

			if( LoadFlags & LOAD_MemoryReader )
			{
				// Serialize data from memory instead of from disk.
				check(FileReader);
				UINT	BufferSize	= FileReader->TotalSize();
				void*	Buffer		= appMalloc( BufferSize );
				FileReader->Serialize( Buffer, BufferSize );
				Loader = new FBufferReader( Buffer, BufferSize, TRUE, TRUE );
				delete FileReader;
			}
			else
			{
				Loader = FileReader;
			}
		}
		else if (bIsSeekFree)
		{
			// Use the async archive as it supports proper Precache and package compression.
			Loader = new FArchiveAsync( *Filename );
			// An error signifies that the package couldn't be opened.
			if( Loader->IsError() )
			{
				delete Loader;
				appThrowf( *FString::Printf(*LocalizeError(TEXT("OpenFailed"),TEXT("Core")), *Filename, *GFileManager->GetCurrentDirectory()) );
			}
		}
		check( Loader );
		check( !Loader->IsError() );

		// Error if linker is already loaded.
		for( INT i=0; i<GObjLoaders.Num(); i++ )
		{
			if( GetLoader(i)->LinkerRoot == LinkerRoot )
			{
				appThrowf( *LocalizeError(TEXT("LinkerExists"),TEXT("Core")), *LinkerRoot->GetName() );
			}
		}

		// Begin.
		GWarn->StatusUpdatef( 0, 0, *LocalizeProgress(TEXT("Loading"),TEXT("Core")), *Filename );

		// Set status info.
		ArVer			= GPackageFileVersion;
		ArLicenseeVer	= GPackageFileLicenseeVersion;
		ArIsLoading		= TRUE;
		ArIsPersistent	= TRUE;
		ArForEdit		= GIsEditor;
		ArForClient		= TRUE;
		ArForServer		= TRUE;
	}

	UBOOL bExecuteNextStep = TRUE;
	if( bHasSerializedPackageFileSummary == FALSE )
	{
		// Precache up to 128 KByte before serializing package file summary.
		INT PrecacheSize = Min( 128 * 1024, Loader->TotalSize() );
		check( PrecacheSize > 0 );
		// Wait till we're finished precaching before executing the next step.
		bExecuteNextStep = Loader->Precache( 0, PrecacheSize);
	}

	return bExecuteNextStep && !IsTimeLimitExceeded( TEXT("creating loader") );
}

/**
 * Serializes the package file summary.
 */
UBOOL ULinkerLoad::SerializePackageFileSummary()
{
	if( bHasSerializedPackageFileSummary == FALSE )
	{
		// Read summary from file.
		*this << Summary;

		// editor can load imports when opening cooked data
		if (GIsEditor && !GIsUCC)
		{
			Summary.PackageFlags &= ~PKG_RequireImportsAlreadyLoaded;
		}

		// Loader needs to be the same version.
		Loader->SetVer(Summary.GetFileVersion());
		Loader->SetLicenseeVer(Summary.GetFileVersionLicensee());
		ArVer = Summary.GetFileVersion();
		ArLicenseeVer = Summary.GetFileVersionLicensee();

		// Package has been stored compressed.
#if DEBUG_DISTRIBUTED_COOKING
		if( FALSE )
#else
		if( Summary.PackageFlags & PKG_StoreCompressed )
#endif
		{
			// Set compression mapping. Failure means Loader doesn't support package compression.
			check( Summary.CompressedChunks.Num() );
			if( !Loader->SetCompressionMap( &Summary.CompressedChunks, (ECompressionFlags) Summary.CompressionFlags ) )
			{
				// Current loader doesn't support it, so we need to switch to one known to support it.
				
				// We need keep track of current position as we already serialized the package file summary.
				INT		CurrentPos				= Loader->Tell();
				// Serializing the package file summary determines whether we are forcefully swapping bytes
				// so we need to propage this information from the old loader to the new one.
				UBOOL	bHasForcedByteSwapping	= Loader->ForceByteSwapping();

				// Delete existing loader...
				delete Loader;
				// ... and create new one using FArchiveAsync as it supports package compression.
				Loader = new FArchiveAsync( *Filename );
				check( !Loader->IsError() );
				
				// Seek to current position as package file summary doesn't need to be serialized again.
				Loader->Seek( CurrentPos );
				// Propagate byte-swapping behavior.
				Loader->SetByteSwapping( bHasForcedByteSwapping );
				
				// Set the compression map and verify it won't fail this time.
				verify( Loader->SetCompressionMap( &Summary.CompressedChunks, (ECompressionFlags) Summary.CompressionFlags ) );
			}
		}

		UPackage* LinkerRootPackage = Cast<UPackage>(LinkerRoot);
		if( LinkerRootPackage )
		{
			// Propagate package flags, except the trash flag (which will be reset below if the package is still in the trash)
			LinkerRootPackage->PackageFlags = Summary.PackageFlags & ~PKG_Trash;

			// Propagate package folder name
			LinkerRootPackage->SetFolderName(*Summary.FolderName);

			if( Summary.EngineVersion > GEngineVersion )
			{
				// Warn user that package isn't saveable.
				if( GIsEditor )
				{
					// Only add the error if we haven't already seen it for this package
					if( ( LinkerRootPackage->PackageFlags & PKG_SavedWithNewerVersion ) == 0 )
					{
						GWarn->MapCheck_Add( MCTYPE_ERROR, NULL, *FString::Printf( *LocalizeError(TEXT("SavedWithNewerVersion"),TEXT("Core")), *Filename,GEngineVersion, Summary.EngineVersion ) );
					}
				}
				else
				{
					debugf( NAME_Warning, *LocalizeError(TEXT("SavedWithNewerVersion"),TEXT("Core")), *Filename,GEngineVersion, Summary.EngineVersion );
				}

				// Mark package as having been saved with a version that is newer then the current one. This is done
				// to later on avoid saving the package and potentially silently clobbering important changes.
				LinkerRootPackage->PackageFlags |= PKG_SavedWithNewerVersion;
			}
		}
		
		// Propagate fact that package cannot use lazy loading to archive (aka this).
		if( Summary.PackageFlags & PKG_DisallowLazyLoading )
		{
			ArAllowLazyLoading = FALSE;
		}
		else
		{
			ArAllowLazyLoading = TRUE;
		}

		// if this package is in the trashcan, mark the linker's root package as Trash
		if( LinkerRootPackage && Filename.InStr(TRASHCAN_DIRECTORY_NAME) != -1 )
		{
			LinkerRootPackage->PackageFlags |= PKG_Trash;
		}

		// Check tag.
		if( Summary.Tag != PACKAGE_FILE_TAG )
		{
			appThrowf( *LocalizeError(TEXT("BinaryFormat"),TEXT("Core")), *Filename );
		}

		// Validate the summary.
		if( Summary.GetFileVersion() < GPackageFileMinVersion )
		{
			appThrowf( *LocalizeError(TEXT("OldVersionFile"),TEXT("Core")), *Filename );
		}
		
		// Don't load packages that were saved with an engine version newer than the current one.
		if( (Summary.GetFileVersion() > GPackageFileVersion) || (Summary.GetFileVersionLicensee() > GPackageFileLicenseeVersion) )
		{
			warnf(*LocalizeError(TEXT("FileVersionDump"),TEXT("Core")), 
				Summary.GetFileVersion(), GPackageFileVersion, Summary.GetFileVersionLicensee(), GPackageFileLicenseeVersion);
			appThrowf( *LocalizeError(TEXT("FileVersionNewerThanEngineVersion"),TEXT("Core")), *Filename );
		}

#if CONSOLE
		// check that the package being loaded has the correct CookedContentVersion
 		if( Summary.GetCookedContentVersion() != GPackageFileCookedContentVersion )
 		{
 			appErrorf( *LocalizeError(TEXT("CookedPackagedVersionOlderThanEnginePackageFileCookedContentVersion"),TEXT("Core")), *Filename, GPackageFileCookedContentVersion, Summary.GetCookedContentVersion() );
 		}
#endif // CONSOLE

		// Slack everything according to summary.
		ImportMap   .Empty( Summary.ImportCount   );
		ExportMap   .Empty( Summary.ExportCount   );
		NameMap		.Empty( Summary.NameCount     );
		// Depends map gets pre-sized in SerializeDependsMap if used.

		// Avoid serializing it again.
		bHasSerializedPackageFileSummary = TRUE;
	}

	return !IsTimeLimitExceeded( TEXT("serializing package file summary") );
}

/**
 * Serializes the name table.
 */
UBOOL ULinkerLoad::SerializeNameMap()
{
	// The name map is the first item serialized. We wait till all the header information is read
	// before any serialization. @todo async, @todo seamless: this could be spread out across name,
	// import and export maps if the package file summary contained more detailed information on
	// serialized size of individual entries.
	UBOOL bFinishedPrecaching = TRUE;

	if( NameMapIndex == 0 && Summary.NameCount > 0 )
	{
		Seek( Summary.NameOffset );
		// Make sure there is something to precache first.
		if( Summary.TotalHeaderSize > 0 )
		{
			// Precache name, import and export map.
			bFinishedPrecaching = Loader->Precache( Summary.NameOffset, Summary.TotalHeaderSize - Summary.NameOffset );
		}
		// Backward compat code for VER_MOVED_EXPORTIMPORTMAPS_ADDED_TOTALHEADERSIZE.
		else
		{
			bFinishedPrecaching = TRUE;
		}
	}

	while( bFinishedPrecaching && NameMapIndex < Summary.NameCount && !IsTimeLimitExceeded(TEXT("serializing name map"),100) )
	{
		// Read the name entry from the file.
		FNameEntry NameEntry;
		*this << NameEntry;
		// Add it to the name table if it's needed in this context 

		// for older versions, we want to split the name when we load the package because it may have been
		// written out unsplit
		if (Ver() < VER_NAME_TABLE_LOADING_CHANGE)
		{
			NameMap.AddItem( NameEntry.HasAnyFlags(_ContextFlags) ? FName(NameEntry.Name) : FName(NAME_None) );
		}
		// now, we make sure we DO NOT split the name here because it will have been written out
		// split, and we don't want to keep splitting A_3_4_9 every time
		else
		{
			NameMap.AddItem( NameEntry.HasAnyFlags(_ContextFlags) ? FName(ENAME_LinkerConstructor, NameEntry.Name) : FName(NAME_None) );
		}
		NameMapIndex++;
	}

	// Return whether we finished this step and it's safe to start with the next.
	return (NameMapIndex == Summary.NameCount) && !IsTimeLimitExceeded( TEXT("serializing name map") );
}

/**
 * Serializes the import map.
 */
UBOOL ULinkerLoad::SerializeImportMap()
{
	if( ImportMapIndex == 0 && Summary.ImportCount > 0 )
	{
		Seek( Summary.ImportOffset );
	}

	while( ImportMapIndex < Summary.ImportCount && !IsTimeLimitExceeded(TEXT("serializing import map"),100) )
	{
		FObjectImport* Import = new(ImportMap)FObjectImport;
		*this << *Import;
		ImportMapIndex++;
	}
	
	// Return whether we finished this step and it's safe to start with the next.
	return (ImportMapIndex == Summary.ImportCount) && !IsTimeLimitExceeded( TEXT("serializing import map") );
}

/**
 * Fixes up the import map, performing remapping for backward compatibility and such.
 */
UBOOL ULinkerLoad::FixupImportMap()
{
	if( bHasFixedUpImportMap == FALSE )
	{
		// Fix up imports.
		for( INT i=0; i<ImportMap.Num(); i++ )
		{
			FObjectImport& Import = ImportMap(i);

			//////////////////
 			// Remap SoundCueLocalized class to SoundCue.
 			if( Import.ObjectName == NAME_SoundCueLocalized && Import.ClassName == NAME_Class )
 			{
 				// Verify that the outer for this class is Engine, so that a licensee class named
				// SoundCueLocalized won't be remapped.
 				const PACKAGE_INDEX OuterIndex = Import.OuterIndex;
 				if ( IS_IMPORT_INDEX( OuterIndex ) )
 				{
 					if ( ImportMap(-OuterIndex-1).ObjectName == NAME_Engine )
 					{
 						Import.ObjectName = NAME_SoundCue;
 					}
 				}
 			}
			// Remap SoundCueLocalized references to SoundCue.
			else if( Import.ClassName == NAME_SoundCueLocalized && Import.ClassPackage == NAME_Engine )
			{
				Import.ClassName = NAME_SoundCue;
			}

			//////////////////
			//@hack: The below fixes up the import table so maps referring to the old SequenceObjects package look for those 
			// classes & objects in the Engine package where they were moved to.
			if( Import.ObjectName == NAME_SequenceObjects && Import.ClassName == NAME_Package )
			{
				Import.ObjectName = NAME_Engine;
			}
			if( Import.ClassPackage == NAME_SequenceObjects )
			{
				Import.ClassPackage = NAME_Engine;
			}

			// objects of type TextureRenderTarget now need to be TextureRenderTarget2D in Gemini
			if ( Loader->Ver() < VER_REMPAP_TEXTURE_RENDER_TARGET )
			{
				if ( Import.ClassName == NAME_Class && Import.ObjectName == FName(TEXT("TextureRenderTarget")) )
			{
				Import.ObjectName = FName(TEXT("TextureRenderTarget2D"));
			}
				else if ( Import.ClassName == TEXT("TextureRenderTarget") )
				{
					Import.ClassName = TEXT("TextureRenderTarget2D");
					if ( Import.ObjectName == TEXT("Default__TextureRenderTarget") )
					{
						Import.ObjectName = TEXT("Default__TextureRenderTarget2D");
					}
				}
			}
		}
		// Avoid duplicate work in async case.
		bHasFixedUpImportMap = TRUE;
	}
	return !IsTimeLimitExceeded( TEXT("fixing up import map") );
}

/**
 * Serializes the export map.
 */
UBOOL ULinkerLoad::SerializeExportMap()
{
	if( ExportMapIndex == 0 && Summary.ExportCount > 0 )
	{
		Seek( Summary.ExportOffset );
	}

	while( ExportMapIndex < Summary.ExportCount && !IsTimeLimitExceeded(TEXT("serializing export map"),100) )
	{
		FObjectExport* Export = new(ExportMap)FObjectExport;
		*this << *Export;
		ExportMapIndex++;
	}
	
	// Return whether we finished this step and it's safe to start with the next.
	return (ExportMapIndex == Summary.ExportCount) && !IsTimeLimitExceeded( TEXT("serializing export map") );
}

/**
 * Serializes the depends map.
 */
UBOOL ULinkerLoad::SerializeDependsMap()
{
	// Skip serializing depends map if package is too old
	if( Ver() < VER_ADDED_LINKER_DEPENDENCIES 
	// or we are using seekfree loading
	||	GUseSeekFreeLoading 
	// or we are neither Editor nor commandlet
	|| !(GIsEditor || GIsUCC) )
	{
		return TRUE;
	}

	// depends map size is same as export map size
	if (DependsMapIndex == 0 && Summary.ExportCount > 0)
	{
		Seek(Summary.DependsOffset);

		// Pre-size array to avoid re-allocation of array of arrays!
		DependsMap.AddZeroed(Summary.ExportCount);
	}

	while (DependsMapIndex < Summary.ExportCount && !IsTimeLimitExceeded(TEXT("serializing depends map"), 100))
	{
		TArray<INT>& Depends = DependsMap(DependsMapIndex);
		*this << Depends;
		DependsMapIndex++;
	}
	
	// Return whether we finished this step and it's safe to start with the next.
	return (DependsMapIndex == Summary.ExportCount) && !IsTimeLimitExceeded( TEXT("serializing depends map") );
}


/** 
 * Creates the export hash. This relies on the import and export maps having already been serialized.
 */
UBOOL ULinkerLoad::CreateExportHash()
{
	// Zero initialize hash on first iteration.
	if( ExportHashIndex == 0 )
	{
		for( INT i=0; i<ARRAY_COUNT(ExportHash); i++ )
		{
			ExportHash[i] = INDEX_NONE;
		}
	}

	// Set up export hash, potentially spread across several frames.
	while( ExportHashIndex < ExportMap.Num() && !IsTimeLimitExceeded(TEXT("creating export hash"),100) )
	{
		FObjectExport& Export = ExportMap(ExportHashIndex);

		INT iHash = HashNames( Export.ObjectName, GetExportClassName(ExportHashIndex), GetExportClassPackage(ExportHashIndex) ) & (ARRAY_COUNT(ExportHash)-1);
		Export._iHashNext = ExportHash[iHash];
		ExportHash[iHash] = ExportHashIndex;

		ExportHashIndex++;
	}

	// Return whether we finished this step and it's safe to start with the next.
	return (ExportHashIndex == ExportMap.Num()) && !IsTimeLimitExceeded( TEXT("creating export hash") );
}

/**
 * Finds existing exports in memory and matches them up with this linker. This is required for PIE to work correctly
 * and also for script compilation as saving a package will reset its linker and loading will reload/ replace existing
 * objects without a linker.
 */
UBOOL ULinkerLoad::FindExistingExports()
{
	if( bHasFoundExistingExports == FALSE )
	{
#if !CONSOLE
		// only look for existing exports in the editor after it has started up
		if( GIsEditor && GIsRunning )
		{
			// Hunt down any existing objects and hook them up to this linker unless the user is either currently opening this
			// package manually via the generic browser or the package is a map package. We want to overwrite (aka load on top)
			// the objects in those cases, so don't try to find existing exports.
			//
			// @todo: document why we also do it for GIsUCCMake
			UPackage* LinkerRootPackage = Cast<UPackage>(LinkerRoot);
			UBOOL bContainsMap			= LinkerRootPackage ? LinkerRootPackage->ContainsMap() : FALSE;
			UBOOL bRequestFindExisting	= GCallbackQuery->Query(CALLBACK_LoadObjectsOnTop, Filename) == FALSE;
			if( (!GIsUCC && bRequestFindExisting && !bContainsMap) || GIsUCCMake )
			{
				for (INT ExportIndex = 0; ExportIndex < ExportMap.Num(); ExportIndex++)
				{
					FindExistingExport(ExportIndex);
				}
			}
		}
#endif
		// Avoid duplicate work in the case of async linker creation.
		bHasFoundExistingExports = TRUE;
	}
	return !IsTimeLimitExceeded( TEXT("finding existing exports") );
}

/**
 * Finalizes linker creation, adding linker to loaders array and potentially verifying imports.
 */
UBOOL ULinkerLoad::FinalizeCreation()
{
	if( bHasFinishedInitialization == FALSE )
	{
		// Add this linker to the object manager's linker array.
		GObjLoaders.AddItem( this );

		// tell the root package to set up netplay info
		UPackage* RootPackage = Cast<UPackage>(LinkerRoot);
		if (RootPackage != NULL)
		{
			RootPackage->InitNetInfo(this, INDEX_NONE);
		}

		if( !(LoadFlags & LOAD_NoVerify) )
		{
			Verify();
		}

		// This means that _Linker references are not NULL'd when using FArchiveReplaceObjectRef
		SetFlags(RF_Public);

		// Avoid duplicate work in the case of async linker creation.
		bHasFinishedInitialization = TRUE;
	}
	return !IsTimeLimitExceeded( TEXT("finalizing creation") );
}

/*-----------------------------------------------------------------------------
	ULinkerLoad creation END
-----------------------------------------------------------------------------*/

/**
 * Before loading anything objects off disk, this function can be used to discover
 * the object in memory. This could happen in the editor when you save a package (which
 * destroys the linker) and then play PIE, which would cause the Linker to be
 * recreated. However, the objects are still in memory, so there is no need to reload
 * them.
 *
 * @param ExportIndex	The index of the export to hunt down
 * @return The object that was found, or NULL if it wasn't found
 */
UObject* ULinkerLoad::FindExistingExport(INT ExportIndex)
{
	FObjectExport& Export = ExportMap(ExportIndex);

	// we only want to import classes, not use classes that are in this package
	// or if we were already found, leave early
	if (Export.ClassIndex >= 0 || Export._Object)
	{
		return Export._Object;
	}

	// find the outer package for this object, if it's already loaded
	UObject* OuterObject = NULL;
	if (Export.OuterIndex == 0)
	{
		// this export's outer is the UPackage root of this loader
		OuterObject = LinkerRoot;
	}
	else
	{
		// if we have a PackageIndex, then we are in a group or other obhect, and we should look for it
		OuterObject = FindExistingExport(Export.OuterIndex-1);
	}

	// if we found one, keep going. if we didn't find one, then this package has never been loaded before
	// things inside a class however should not be touched, as they are in .u files and shouldn't have SetLinker called on them
	if (OuterObject && !Outer->IsInA(UClass::StaticClass()))
	{
		// find the class of this object
		UClass* TheClass = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *ImportMap(-Export.ClassIndex - 1).ObjectName.ToString(), 1);

		// if the class exists, try to find the object
		if (TheClass)
		{
			Export._Object = StaticFindObject(TheClass, OuterObject, *Export.ObjectName.ToString(), 1);

			// if we found an object, set it's linker to us
			if (Export._Object)
			{
                Export._Object->SetLinker(this, ExportIndex);
			}
		}
	}

	return Export._Object;
}

void ULinkerLoad::Verify()
{
	if( !(LinkerRoot->PackageFlags & PKG_Cooked)  && (!GIsGame || GIsEditor || GIsUCC) )
	{
		if( !bHaveImportsBeenVerified && !(Summary.PackageFlags & PKG_RequireImportsAlreadyLoaded) )
		{
#if !EXCEPTIONS_DISABLED
			try
#endif
		{
			// Validate all imports and map them to their remote linkers.
			for( INT i=0; i<Summary.ImportCount; i++ )
			{
				VerifyImport( i );
			}
		}
#if !EXCEPTIONS_DISABLED
		catch( TCHAR* Error )
		{
				GObjLoaders.RemoveItem( this );
				throw( Error );
			}
#endif
		}
	}
	bHaveImportsBeenVerified = TRUE;
}

FName ULinkerLoad::GetExportClassPackage( INT i )
{
	FObjectExport& Export = ExportMap( i );
	if( Export.ClassIndex < 0 )
	{
		check( ImportMap.IsValidIndex(-Export.ClassIndex-1) );
		FObjectImport& Import = ImportMap( -Export.ClassIndex-1 );
		checkSlow(Import.OuterIndex<0);
		check( ImportMap.IsValidIndex(-Import.OuterIndex-1) );
		return ImportMap( -Import.OuterIndex-1 ).ObjectName;
	}
	else if( Export.ClassIndex > 0 )
	{
		return LinkerRoot->GetFName();
	}
	else
	{
		return NAME_Core;
	}
}

FName ULinkerLoad::GetExportClassName( INT i )
{
	FObjectExport& Export = ExportMap(i);
	if( Export.ClassIndex < 0 )
	{
		check( ImportMap.IsValidIndex(-Export.ClassIndex-1) );
		return ImportMap( -Export.ClassIndex-1 ).ObjectName;
	}
	else if( Export.ClassIndex > 0 )
	{
		check( ExportMap.IsValidIndex(Export.ClassIndex-1) );
		return ExportMap( Export.ClassIndex-1 ).ObjectName;
	}
	else
	{
		return NAME_Class;
	}
}


FName ULinkerLoad::GetArchiveName() const
{
	return *Filename;
}


/**
 * Recursively gathers the dependencies of a given export (the recursive chain of imports
 * and their imports, and so on)

 * @param ExportIndex Index into the linker's ExportMap that we are checking dependencies
 * @param Dependencies Array of all dependencies needed
 */
void ULinkerLoad::GatherExportDependencies(INT ExportIndex, TArray<FDependencyRef>& Dependencies)
{
	// make sure we have dependencies
	// @todo: remove this check after all packages have been saved up to VER_ADDED_LINKER_DEPENDENCIES
	if (DependsMap.Num() == 0)
	{
		return;
	}

	// validate data
	check(DependsMap.Num() == ExportMap.Num());

	// get the list of imports the export needs
	TArray<INT>& ExportDependencies = DependsMap(ExportIndex);

//warnf(TEXT("Gathering dependencies for %s"), *GetExportFullName(ExportIndex));

	for (INT DependIndex = 0; DependIndex < ExportDependencies.Num(); DependIndex++)
	{
		INT ObjectIndex = ExportDependencies(DependIndex);

		// if it's an import, use the import version to recurse (which will add the export the import points to to the array)
		if (IS_IMPORT_INDEX(ObjectIndex))
		{
			GatherImportDependencies(-ObjectIndex - 1, Dependencies);
		}
		else
		{
			INT RefExportIndex = ObjectIndex - 1;
			FObjectExport& Export = ExportMap(RefExportIndex);

			if (Export._Object)
			{
				continue;
			}

			// fill out the ref
			FDependencyRef NewRef;
			NewRef.Linker = this;
			NewRef.ExportIndex = RefExportIndex;

			// try to add it, and check it it was added to the end (if not, it already existed, so
			// we don't need to recurse)
			// @todo: use tagging to do the check for us, instead of looping looking for unique?
			INT IndexIfNew = Dependencies.Num();
			if (Dependencies.AddUniqueItem(NewRef) == IndexIfNew)
			{
				NewRef.Linker->GatherExportDependencies(RefExportIndex, Dependencies);
			}
		}
	}
}

/**
 * Recursively gathers the dependencies of a given import (the recursive chain of imports
 * and their imports, and so on). Will add itself to the list of dependencies

 * @param ImportIndex Index into the linker's ImportMap that we are checking dependencies
 * @param Dependencies Array of all dependencies needed
 */
void ULinkerLoad::GatherImportDependencies(INT ImportIndex, TArray<FDependencyRef>& Dependencies)
{
	// get the import
	FObjectImport& Import = ImportMap(ImportIndex);

	// we don't need the top level package imports to be checked, since there is no real object associated with them
	if (Import.OuterIndex == ROOTPACKAGE_INDEX)
	{
		return;
	}
	//	warnf(TEXT("  Dependency import %s [%x, %d]"), *GetImportFullName(ImportIndex), Import.SourceLinker, Import.SourceIndex);

	// if the object already exists, we don't need this import
	if (Import.XObject)
	{
		return;
	}

	UObject::BeginLoad();

	// load the linker and find export in sourcelinker
	if (Import.SourceLinker == NULL || Import.SourceIndex == INDEX_NONE)
	{
#if DO_CHECK
		INT NumObjectsBefore = UObject::GetObjectArrayNum();
#endif

		// temp storage we can ignore
		FString Unused;

		// remember that we are gathering imports so that VerifyImportInner will no verify all imports
		bIsGatheringDependencies = TRUE;

		// if we failed to find the object, ignore this import
		// @todo: Tag the import to not be searched again
		VerifyImportInner(ImportIndex, Unused);

		// turn off the flag
		bIsGatheringDependencies = FALSE;

		UBOOL bIsValidImport =
			(Import.XObject != NULL && !Import.XObject->HasAnyFlags(RF_Native)) ||
			(Import.SourceLinker != NULL && Import.SourceIndex != INDEX_NONE);

		// make sure it succeeded
		if (!bIsValidImport)
		{
			// don't worry about 
			warnf(TEXT("  >>> VerifyImportInner failed [(%x, %d), (%x, %d)]!"), Import.XObject, Import.XObject ? (Import.XObject->HasAnyFlags(RF_Native) ? 1 : 0) : 0, Import.SourceLinker, Import.SourceIndex);
			UObject::EndLoad();
			return;
		}

#if DO_CHECK
		// only object we should create are one ULinkerLoad for source linker
		if (UObject::GetObjectArrayNum() - NumObjectsBefore > 1)
		{
			warnf(TEXT("Created %d objects checking %s"), UObject::GetObjectArrayNum() - NumObjectsBefore, *GetImportFullName(ImportIndex));
		}
#endif
	}

	// save off information BEFORE calling EndLoad so that the Linkers are still associated
	FDependencyRef NewRef;
	if (Import.XObject)
	{
		warnf(TEXT("Using non-native XObject %s!!!"), *Import.XObject->GetFullName());
		NewRef.Linker = Import.XObject->_Linker;
		NewRef.ExportIndex = Import.XObject->_LinkerIndex;
	}
	else
	{
		NewRef.Linker = Import.SourceLinker;
		NewRef.ExportIndex = Import.SourceIndex;
	}

	UObject::EndLoad();

	// try to add it, and check it it was added to the end (if not, it already existed, so
	// we don't need to recurse)
	// @todo: use tagging to do the check for us, instead of looping looking for unique?
	INT IndexIfNew = Dependencies.Num();
	if (Dependencies.AddUniqueItem(NewRef) == IndexIfNew)
	{
		NewRef.Linker->GatherExportDependencies(NewRef.ExportIndex, Dependencies);
	}
}




/**
 * A wrapper around VerifyImportInner. If the VerifyImportInner (previously VerifyImport) fails, this function
 * will look for a UObjectRedirector that will point to the real location of the object. You will see this if
 * an object was renamed to a different package or group, but something that was referencing the object was not
 * not currently open. (Rename fixes up references of all loaded objects, but naturally not for ones that aren't
 * loaded).
 *
 * @param	i	The index into this packages ImportMap to verify
 */
void ULinkerLoad::VerifyImport( INT i )
{
	check( !(Summary.PackageFlags & PKG_RequireImportsAlreadyLoaded) );

	FObjectImport& Import = ImportMap(i);

	// keep a string of modifiers to add to the Editor Warning dialog
	FString WarningAppend;

	// try to load the object, but don't print any warnings on error (so we can try the redirector first)
	// note that a true return value here does not mean it failed or succeeded, just tells it how to respond to a further failure
	UBOOL bCrashOnFail = VerifyImportInner(i,WarningAppend);

	// by default, we haven't failed yet
	UBOOL bFailed = false;
	UBOOL bRedir = false;

	// these checks find out if the VerifyImportInner was successful or not 
	if (Import.SourceLinker && Import.SourceIndex == INDEX_NONE && Import.XObject == NULL && Import.OuterIndex != 0 && Import.ObjectName != NAME_ObjectRedirector)
	{
		// if we found the package, but not the object, look for a redirector
		FObjectImport OriginalImport = Import;
		Import.ClassName = NAME_ObjectRedirector;
		Import.ClassPackage = NAME_Core;

		// try again for the redirector
		VerifyImportInner(i,WarningAppend);

		// if the redirector wasn't found, then it truly doesn't exist
		if (Import.SourceIndex == INDEX_NONE)
		{
			bFailed = true;
		}
		// otherwise, we found that the redirector exists
		else
		{
			// this notes that for any load errors we get that a ObjectRedirector was involved (which may help alleviate confusion
			// when people don't understand why it was trying to load an object that was redirected from or to)
			WarningAppend += LocalizeError(TEXT("LoadWarningSuffix_redirection"),TEXT("Editor"));

			// Create the redirector (no serialization yet)
			UObjectRedirector* Redir = Cast<UObjectRedirector>(Import.SourceLinker->CreateExport(Import.SourceIndex));
			// this should probably never fail, but just in case
			if (!Redir)
			{
				bFailed = true;
			}
			else
			{
				// serialize in the properties of the redirector (to get the object the redirector point to)
				Preload(Redir);

				// check to make sure the destination obj was loaded,
				if (Redir->DestinationObject == NULL)
				{
					bFailed = true;
				}
				// check that in fact it was the type we thought it should be
				else if (Redir->DestinationObject->GetClass()->GetFName() != OriginalImport.ClassName)
				{
					bFailed = true;
					// if the destination is a ObjectRedirector you've most likely made a nasty circular loop
					if( Redir->DestinationObject->GetClass() == UObjectRedirector::StaticClass() )
					{
						WarningAppend += LocalizeError(TEXT("LoadWarningSuffix_circularredirection"),TEXT("Editor"));
					}
				}
				else
				{
					// send a callback saying we followed a redirector successfully
					GCallbackEvent->Send(CALLBACK_RedirectorFollowed, Filename, Redir);

					// now, fake our Import to be what the redirector pointed to
					Import.XObject = Redir->DestinationObject;
					GImportCount++;
				}
			}
		}

		// fix up the import. We put the original data back for the ClassName and ClassPackage (which are read off disk, and
		// are expected not to change)
		Import.ClassName = OriginalImport.ClassName;
		Import.ClassPackage = OriginalImport.ClassPackage;

		// if nothing above failed, then we are good to go
		if (!bFailed)
		{
			// we update the runtime information (SourceIndex, SourceLinker) to point to the object the redirector pointed to
			Import.SourceIndex = Import.XObject->_LinkerIndex;
			Import.SourceLinker = Import.XObject->_Linker;
		}
		else
		{
			// put us back the way we were and peace out
			Import = OriginalImport;
			// if the original VerifyImportInner told us that we need to throw an exception if we weren't redirected,
			// then do the throw here
			if (bCrashOnFail)
			{
				debugf( TEXT("Failed import: %s %s (file %s)"), *Import.ClassName.ToString(), *GetImportFullName(i), *Import.SourceLinker->Filename );
				appThrowf( *LocalizeError(TEXT("FailedImport"),TEXT("Core")), *Import.ClassName.ToString(), *GetImportFullName(i) );
			}
			// otherwise just printout warnings, and if in the editor, popup the EdLoadWarnings box
			else
			{
				// try to get a pointer to the class of the original object so that we can display the class name of the missing resource
				UObject* ClassPackage = FindObject<UPackage>( NULL, *Import.ClassPackage.ToString() );
				UClass* FindClass = ClassPackage ? FindObject<UClass>( ClassPackage, *OriginalImport.ClassName.ToString() ) : NULL;
				if( GIsEditor && !GIsUCC )
				{
					// put somethng into the load warnings dialog, with any extra information from above (in WarningAppend)
					EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, *FString::Printf(TEXT("%s%s"), *GetImportFullName(i), *WarningAppend) );
				}

				if( !IgnoreMissingReferencedClass( Import.ObjectName ) )
				{
					// failure to load a class, most likely deleted instead of deprecated
					if ( (!GIsEditor || GIsUCC) && FindClass == UClass::StaticClass() )
					{
						warnf(NAME_Warning, TEXT("Missing Class '%s' referenced by package '%s'.  Classes should not be removed if referenced by content; mark the class 'deprecated' instead."), *GetImportFullName(i), *LinkerRoot->GetName());
					}
					// ignore warnings for missing imports if the object's class has been deprecated.
					else if ( FindClass == NULL || !FindClass->HasAnyClassFlags(CLASS_Deprecated) )
					{
						warnf(NAME_Warning, TEXT("Missing %s referenced by package '%s'"), *GetImportFullName(i), *LinkerRoot->GetName()); 
					}
				}
			}
		}
	}
}

/**
 * Safely verify that an import in the ImportMap points to a good object. This decides whether or not
 * a failure to load the object redirector in the wrapper is a fatal error or not (return value)
 *
 * @param	i	The index into this packages ImportMap to verify
 *
 * @return TRUE if the wrapper should crash if it can't find a good object redirector to load
 */
UBOOL ULinkerLoad::VerifyImportInner(INT ImportIndex, FString& WarningSuffix)
{
	check(GObjBeginLoadCount>0);

	FObjectImport& Import = ImportMap(ImportIndex);

	if
	(	(Import.SourceLinker && Import.SourceIndex != INDEX_NONE)
	||	Import.ClassPackage	== NAME_None
	||	Import.ClassName	== NAME_None
	||	Import.ObjectName	== NAME_None )
	{
		// Already verified, or not relevent in this context.
		return FALSE;
	}


	UBOOL SafeReplace = FALSE;
	UObject* Pkg=NULL;

	// Find or load the linker load that contains the FObjectExport for this import
	if( Import.OuterIndex == ROOTPACKAGE_INDEX )
	{
		// our Outer is a UPackage
		check(Import.ClassName==NAME_Package);
		check(Import.ClassPackage==NAME_Core);
		UPackage* TmpPkg = CreatePackage( NULL, *Import.ObjectName.ToString() );

#if !EXCEPTIONS_DISABLED
		try
#endif
		{
			// if we're currently compiling this package, don't allow
			// it to be linked to any other package's ImportMaps
			if ( (TmpPkg->PackageFlags&PKG_Compiling) != 0 )
			{
				return FALSE;
			}

			DWORD TheLoadFlags = LOAD_Throw;
			if ( GIsUCCMake )
			{
				TheLoadFlags |= LOAD_FindIfFail;
			}

			// while gathering dependencies, there is no need to verify all of the imports for the entire package
			if (bIsGatheringDependencies)
			{
				TheLoadFlags |= LOAD_NoVerify;
			}

			Import.SourceLinker = GetPackageLinker( TmpPkg, NULL, TheLoadFlags, NULL, NULL );
		}
#if !EXCEPTIONS_DISABLED
		catch( const TCHAR* Error )
		{
			if( LoadFlags & LOAD_FindIfFail )
			{
				// clear the Import's SourceLinker so that VerifyImport doesn't attempt to
				// do anything else with this FObjectImport (such as search for a redirect
				// for this missing resource, etc.)
				Import.SourceLinker = NULL;
			}
			else
			{
				appThrowf( Error );
			}
		}
#endif
	}
	else
	{
		if (LinkerRoot->PackageFlags & PKG_Cooked)
		{
			// outer can be an export if the package is cooked
			if (!IS_IMPORT_INDEX(Import.OuterIndex))
			{
				// @todo: This is possibly not the right thing to do here!
				// it could attempt to find the original PackageLinker from disk and use that
				return FALSE;
			}
		}
		else
		{
			// this resource's Outer is not a UPackage
			check(IS_IMPORT_INDEX(Import.OuterIndex));
		}

		VerifyImport( -Import.OuterIndex-1 );

		// Copy the SourceLinker from the FObjectImport for our Outer
		Import.SourceLinker = ImportMap(-Import.OuterIndex-1).SourceLinker;

		//check(Import.SourceLinker);
		//@todo what does it mean if we don't have a SourceLinker here?
		if( Import.SourceLinker )
		{
			FObjectImport* Top;
			for (Top = &Import;	IS_IMPORT_INDEX(Top->OuterIndex); Top = &ImportMap(-Top->OuterIndex-1))
			{
				// for loop does what we need
			}

			// Top is now pointing to the top-level UPackage for this resource
			Pkg = CreatePackage(NULL, *Top->ObjectName.ToString());

			// Find this import within its existing linker.
			INT iHash = HashNames( Import.ObjectName, Import.ClassName, Import.ClassPackage) & (ARRAY_COUNT(ExportHash)-1);

			for( INT j=Import.SourceLinker->ExportHash[iHash]; j!=INDEX_NONE; j=Import.SourceLinker->ExportMap(j)._iHashNext )
			{
				FObjectExport& SourceExport = Import.SourceLinker->ExportMap( j );
				if
				(	(SourceExport.ObjectName	                  ==Import.ObjectName               )
				&&	(Import.SourceLinker->GetExportClassName   (j)==Import.ClassName                )
				&&  (Import.SourceLinker->GetExportClassPackage(j)==Import.ClassPackage) )
				{
					// at this point, SourceExport is an FObjectExport in another linker that looks like it
					// matches the FObjectImport we're trying to load - double check that we have the correct one
					if( IS_IMPORT_INDEX(Import.OuterIndex) )
					{
						// OuterImport is the FObjectImport for this resource's Outer
						FObjectImport& OuterImport = ImportMap(-Import.OuterIndex-1);
						if( OuterImport.SourceLinker )
						{
							// if the import for our Outer doesn't have a SourceIndex, it means that
							// we haven't found a matching export for our Outer yet.  This should only
							// be the case if our Outer is a top-level UPackage
							if( OuterImport.SourceIndex==INDEX_NONE )
							{
								// At this point, we know our Outer is a top-level UPackage, so
								// if the FObjectExport that we found has an Outer that is
								// not a linker root, this isn't the correct resource
								if( SourceExport.OuterIndex != ROOTPACKAGE_INDEX )
								{
									continue;
								}
							}

							// The import for our Outer has a matching export - make sure that the import for
							// our Outer is pointing to the same export as the SourceExport's Outer
							else if( OuterImport.SourceIndex + 1 != SourceExport.OuterIndex )
							{
								continue;
							}
						}
					}
					if( !(SourceExport.ObjectFlags & RF_Public) )
					{
						SafeReplace = SafeReplace || (GIsEditor && !GIsUCC);

						// determine if this find the thing that caused this import to be saved into the map
						PACKAGE_INDEX FoundIndex = -(ImportIndex + 1);
						for ( INT i = 0; i < Summary.ExportCount; i++ )
						{
							FObjectExport& Export = ExportMap(i);
							if ( Export.SuperIndex == FoundIndex )
							{
								debugf(TEXT("Private import was referenced by export '%s' (parent)"), *Export.ObjectName.ToString());
								SafeReplace = FALSE;
							}
							else if ( Export.ClassIndex == FoundIndex )
							{
								debugf(TEXT("Private import was referenced by export '%s' (class)"), *Export.ObjectName.ToString());
								SafeReplace = FALSE;
							}
							else if ( Export.OuterIndex == FoundIndex )
							{
								debugf(TEXT("Private import was referenced by export '%s' (outer)"), *Export.ObjectName.ToString());
								SafeReplace = FALSE;
							}
							else if ( Export.ArchetypeIndex == FoundIndex )
							{
								debugf(TEXT("Private import was referenced by export '%s' (template)"), *Export.ObjectName.ToString());
								SafeReplace = FALSE;
							}
						}
						for ( INT i = 0; i < Summary.ImportCount; i++ )
						{
							if ( i != FoundIndex )
							{
								FObjectImport& TestImport = ImportMap(i);
								if ( TestImport.OuterIndex == FoundIndex )
								{
									debugf(TEXT("Private import was referenced by import '%s' (outer)"), *Import.ObjectName.ToString());
									SafeReplace = FALSE;
								}
							}
						}

						if ( !SafeReplace )
						{
							appThrowf( *LocalizeError(TEXT("FailedImportPrivate"),TEXT("Core")), *Import.ClassName.ToString(), *GetImportFullName(ImportIndex) );
						}
						else
						{
							FString Suffix = LocalizeError(TEXT("LoadWarningSuffix_privateobject"),TEXT("Editor"));
							if ( WarningSuffix.InStr(Suffix) == INDEX_NONE )
								WarningSuffix += Suffix;
							break;
						}
					}

					// Found the FObjectExport for this import
					Import.SourceIndex = j;
					break;
				}
			}
		}
	}

	if( (Pkg == NULL) && ((LoadFlags & LOAD_FindIfFail) != 0) )
	{
		Pkg = ANY_PACKAGE;
	}

	// If not found in file, see if it's a public native transient class or field.
	if( Import.SourceIndex==INDEX_NONE && Pkg!=NULL )
	{
		UObject* ClassPackage = FindObject<UPackage>( NULL, *Import.ClassPackage.ToString() );
		if( ClassPackage )
		{
			UClass* FindClass = FindObject<UClass>( ClassPackage, *Import.ClassName.ToString() );
			if( FindClass )
			{
				UObject* FindOuter			= Pkg;

				if ( IS_IMPORT_INDEX(Import.OuterIndex) )
				{
					// if this import corresponds to an intrinsic class, OuterImport's XObject will be NULL if this import
					// belongs to the same package that the import's class is in; in this case, the package is the correct Outer to use
					// for finding this object
					// otherwise, this import represents a field of an intrinsic class, and OuterImport's XObject should be non-NULL (the object
					// that contains the field)
					FObjectImport& OuterImport	= ImportMap(-Import.OuterIndex-1);
					if ( OuterImport.XObject != NULL )
					{
						FindOuter = OuterImport.XObject;
					}
				}

				UObject* FindObject			= StaticFindObject( FindClass, FindOuter, *Import.ObjectName.ToString() );
				UBOOL	 IsNativeTransient	= FindObject && FindObject->HasAllFlags(RF_Public|RF_Native|RF_Transient);
		
				if(	FindObject && ((LoadFlags & LOAD_FindIfFail) || IsNativeTransient) )
				{
					Import.XObject = FindObject;
					GImportCount++;
				}
				else
				{
					SafeReplace = TRUE;
				}
			}
			else
			{
				SafeReplace = TRUE;
			}
		}

		if (!Import.XObject && !SafeReplace)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Loads all objects in package.
 *
 * @param bForcePreload	Whether to explicitly call Preload (serialize) right away instead of being
 *						called from EndLoad()
 */
void ULinkerLoad::LoadAllObjects( UBOOL bForcePreload )
{
	if ( (LoadFlags&LOAD_SeekFree) != 0 )
	{
		bForcePreload = TRUE;
	}

	for( INT i=0; i<Summary.ExportCount; i++ )
	{
		UObject* Object = CreateExport( i );
		if( Object && bForcePreload )
		{
			Preload( Object );
		}
	}

	// Mark package as having been fully loaded.
	if( LinkerRoot )
	{
		LinkerRoot->MarkAsFullyLoaded();
	}
}

/**
 * Returns the ObjectName associated with the resource indicated.
 * 
 * @param	ResourceIndex	location of the object resource
 *
 * @return	ObjectName for the FObjectResource at ResourceIndex, or NAME_None if not found
 */
FName ULinkerLoad::ResolveResourceName( PACKAGE_INDEX ResourceIndex )
{
	if ( ResourceIndex > 0 )
	{
		check(ExportMap.IsValidIndex(ResourceIndex-1));
		return ExportMap(ResourceIndex-1).ObjectName;
	}
	else if ( ResourceIndex < 0 )
	{
		check(ImportMap.IsValidIndex(-ResourceIndex-1));
		return ImportMap(-ResourceIndex-1).ObjectName;
	}
	return NAME_None;
}

// Find the index of a specified object without regard to specific package.
INT ULinkerLoad::FindExportIndex( FName ClassName, FName ClassPackage, FName ObjectName, INT ExportOuterIndex )
{
	INT iHash = HashNames( ObjectName, ClassName, ClassPackage ) & (ARRAY_COUNT(ExportHash)-1);
	for( INT i=ExportHash[iHash]; i!=INDEX_NONE; i=ExportMap(i)._iHashNext )
	{
		if
		(  (ExportMap(i).ObjectName  ==ObjectName                              )
		&& (ExportMap(i).OuterIndex  ==ExportOuterIndex || ExportOuterIndex==INDEX_NONE)
		&& (GetExportClassPackage(i) ==ClassPackage                            )
		&& (GetExportClassName   (i) ==ClassName                               ) )
		{
			return i;
		}
	}

	// If an object with the exact class wasn't found, look for objects with a subclass of the requested class.
	for(INT ExportIndex = 0;ExportIndex < ExportMap.Num();ExportIndex++)
	{
		FObjectExport&	Export = ExportMap(ExportIndex);

		if(Export.ObjectName == ObjectName && (ExportOuterIndex == INDEX_NONE || Export.OuterIndex == ExportOuterIndex))
		{
			UClass*	ExportClass = Cast<UClass>(IndexToObject(Export.ClassIndex));

			// See if this export's class inherits from the requested class.
			for(UClass* ParentClass = ExportClass;ParentClass;ParentClass = ParentClass->GetSuperClass())
			{
				if(ParentClass->GetFName() == ClassName)
				{
					return ExportIndex;
				}
			}
		}
	}

	return INDEX_NONE;
}

/**
 * Function to create the instance of, or verify the presence of, an object as found in this Linker.
 *
 * @param ObjectClass	The class of the object
 * @param ObjectName	The name of the object
 * @param Outer			Find the object inside this outer (and only directly inside this outer, as we require fully qualified names)
 * @param LoadFlags		Flags used to determine if the object is being verified or should be created
 * @param Checked		Whether or not a failure will throw an error
 * @return The created object, or (UObject*)-1 if this is just verifying
 */
UObject* ULinkerLoad::Create( UClass* ObjectClass, FName ObjectName, UObject* Outer, DWORD LoadFlags, UBOOL Checked )
{
	// We no longer handle a NULL outer, which used to mean look in any outer, but we need fully qualified names now
	// The other case where this was NULL is if you are calling StaticLoadObject on the top-level package, but
	// you should be using LoadPackage. If for some weird reason you need to load the top-level package with this,
	// then I believe you'd want to set OuterIndex to 0 when Outer is NULL, but then that could get confused with
	// loading A.A (they both have OuterIndex of 0, according to Ron)
	check(Outer);

	INT OuterIndex = INDEX_NONE;

	// if the outer is the outermost of the package, then we want OuterIndex to be 0, as objects under the top level
	// will have an OuterIndex to 0
	if (Outer == Outer->GetOutermost())
	{
		OuterIndex = 0;
	}
	// otherwise get the linker index of the outer to be the outer index that we look in
	else
	{
		OuterIndex = Outer->GetLinkerIndex();
		// we _need_ the linker index of the outer to look in, which means that the outer must have been actually 
		// loaded off disk, and not just CreatePackage'd
		check(OuterIndex != INDEX_NONE);

		// we have to add 1 to put it into the Package Index format (- for imports, + for exports)
		OuterIndex += 1;
	}

	INT Index = FindExportIndex(ObjectClass->GetFName(), ObjectClass->GetOuter()->GetFName(), ObjectName, OuterIndex);
	if (Index != INDEX_NONE)
	{
		return (LoadFlags & LOAD_Verify) ? INVALID_OBJECT : CreateExport(Index);
	}

	// since we didn't it, see if we can find an object redirector with the same name
	Index = FindExportIndex(UObjectRedirector::StaticClass()->GetFName(), NAME_Core, ObjectName, OuterIndex);
	// if we found a redirector, create it, and move on down the line
	if (Index != INDEX_NONE)
	{
		// create the redirector,
		UObjectRedirector* Redir = (UObjectRedirector*)CreateExport(Index);
		Preload(Redir);
		// if we found what it was point to, then return it
		if (Redir->DestinationObject && Redir->DestinationObject->GetClass() == ObjectClass)
		{
			// send a callback saying we followed a redirector successfully
			GCallbackEvent->Send(CALLBACK_RedirectorFollowed, Filename, Redir);
			// and return the object we are being redirected to
			return Redir->DestinationObject;
		}
	}


// Set this to 1 to find nonqualified names anyway
#define FIND_OBJECT_NONQUALIFIED 0
// Set this to 1 if you want to see what it would have found previously. This is useful for fixing up hundreds
// of now-illegal references in script code.
#define DEBUG_PRINT_NONQUALIFIED_RESULT 1

#if DEBUG_PRINT_NONQUALIFIED_RESULT || FIND_OBJECT_NONQUALIFIED
	Index = FindExportIndex(ObjectClass->GetFName(), ObjectClass->GetOuter()->GetFName(), ObjectName, INDEX_NONE);
	if (Index != INDEX_NONE)
	{
#if DEBUG_PRINT_NONQUALIFIED_RESULT
		warnf(NAME_Warning, TEXT("Using a non-qualified name (would have) found: %s"), *GetExportFullName(Index));
#endif
#if FIND_OBJECT_NONQUALIFIED
		return (LoadFlags & LOAD_Verify) ? INVALID_OBJECT : CreateExport(Index);
#endif
	}
#endif


	// if we are checking for failure cases, and we failed, throw an error
	if( Checked )
	{
		appThrowf(*LocalizeError(TEXT("FailedCreate"),TEXT("Core")), *ObjectClass->GetName(), *ObjectName.ToString());
	}
	return NULL;
}

/**
 * Serialize the object data for the specified object from the unreal package file.  Loads any
 * additional resources required for the object to be in a valid state to receive the loaded
 * data, such as the object's Outer, Class, or ObjectArchetype.
 *
 * When this function exits, Object is guaranteed to contain the data stored that was stored on disk.
 *
 * @param	Object	The object to load data for.  If the data for this object isn't stored in this
 *					ULinkerLoad, routes the call to the appropriate linker.  Data serialization is 
 *					skipped if the object has already been loaded (as indicated by the RF_NeedLoad flag
 *					not set for the object), so safe to call on objects that have already been loaded.
 *					Note that this function assumes that Object has already been initialized against
 *					its template object.
 *					If Object is a UClass and the class default object has already been created, calls
 *					Preload for the class default object as well.
 */
void ULinkerLoad::Preload( UObject* Object )
{
	check(IsValid());
	check(Object);
	// Preload the object if necessary.
	if( Object->HasAnyFlags(RF_NeedLoad) )
	{
		if( Object->GetLinker()==this )
		{
			UClass* Cls = NULL;

			// If this is a struct, make sure that its parent struct is completely loaded
			if(	Object->IsA(UStruct::StaticClass()) )
			{
				Cls = Cast<UClass>(Object);
				if( ((UStruct*)Object)->SuperField )
				{
					Preload( ((UStruct*)Object)->SuperField );
				}
			}

			// make sure this object didn't get loaded in the above Preload call
			if (Object->HasAnyFlags(RF_NeedLoad))
			{
				// grab the resource for this Object
				FObjectExport& Export = ExportMap( Object->_LinkerIndex );
				check(Export._Object==Object);
				INT SavedPos = Loader->Tell();

				// move to the position in the file where this object's data
				// is stored
				Loader->Seek( Export.SerialOffset );

				// tell the file reader to read the raw data from disk
				Loader->Precache( Export.SerialOffset, Export.SerialSize );

				// mark the object to indicate that it has been loaded
				Object->ClearFlags ( RF_NeedLoad );

				if ( Object->HasAnyFlags(RF_ClassDefaultObject) )
				{
					// In order to ensure that the CDO inherits config & localized property values from the parent class, we can't initialize the CDO until
					// the parent class's CDO has serialized its data from disk and called LoadConfig/LoadLocalized. Since ULinkerLoad::Preload guarantees that
					// Preload is always called on the archetype first, at this point we know that the CDO's archetype (the CDO of the parent class) has already
					// loaded its config/localized data, so it's now safe to initialize the CDO
					Object->InitClassDefaultObject(Object->GetClass());

#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
					DOUBLE PreviousLocalTime = SerializationPerfTracker.GetTotalEventTime(Object->GetClass());
					DOUBLE PreviousGlobalTime = GObjectSerializationPerfTracker->GetTotalEventTime(Object->GetClass());

					DOUBLE StartTime = appSeconds();
					Object->GetClass()->SerializeDefaultObject(Object, *this);

					DOUBLE TimeSpent = (appSeconds() - StartTime) * 1000;

					// add the data to the linker-specific tracker
					SerializationPerfTracker.TrackEvent(Object->GetClass(), PreviousLocalTime, TimeSpent);

					// add the data to the global tracker
					GObjectSerializationPerfTracker->TrackEvent(Object->GetClass(), PreviousGlobalTime, TimeSpent);
#else
					Object->GetClass()->SerializeDefaultObject(Object, *this);
#endif
				}
				else
				{
#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
					DOUBLE PreviousLocalTime = SerializationPerfTracker.GetTotalEventTime(Object->GetClass());
					DOUBLE PreviousGlobalTime = GObjectSerializationPerfTracker->GetTotalEventTime(Object->GetClass());
					DOUBLE PreviousClassGlobalTime=0.f;

					if ( Object->GetClass() == UClass::StaticClass() )
					{
						PreviousClassGlobalTime = GClassSerializationPerfTracker->GetTotalEventTime(static_cast<UClass*>(Object));
					}

					DOUBLE StartTime = appSeconds();
					Object->Serialize( *this );

					DOUBLE TimeSpent = (appSeconds() - StartTime) * 1000;

					// add the data to the linker-specific tracker
					SerializationPerfTracker.TrackEvent(Object->GetClass(), PreviousLocalTime, TimeSpent);

					// add the data to the global tracker
					GObjectSerializationPerfTracker->TrackEvent(Object->GetClass(), PreviousGlobalTime, TimeSpent);

					if ( Object->GetClass() == UClass::StaticClass() )
					{
						GClassSerializationPerfTracker->TrackEvent(static_cast<UClass*>(Object), PreviousClassGlobalTime, TimeSpent);
					}
#else
					Object->Serialize( *this );
#endif
				}

				// Make sure we serialized the right amount of stuff.
				if( Tell()-Export.SerialOffset != Export.SerialSize )
				{
					appErrorf( *LocalizeError(TEXT("SerialSize"),TEXT("Core")), *Object->GetFullName(), Tell()-Export.SerialOffset, Export.SerialSize );
				}

				Loader->Seek( SavedPos );

				if ( Object->HasAnyFlags(RF_ClassDefaultObject) && !GIsUCCMake )
				{
					// if this is the class default object, we'll need to import all
					// config and localized data here, so that when InitClassDefaultObject
					// is called for child classes' default object, the values for this class
					// are correct when the derived class's default object is initialized in
					// InitProperties
					Object->LoadConfig();
					Object->LoadLocalized();
				}
				// if this is a UClass object and it already has a class default object
				else if ( Cls != NULL && Cls->GetDefaultsCount() )
				{
					// make sure that the class default object is completely loaded as well
					Preload(Cls->GetDefaultObject());
				}
			}
		}
		else if( Object->GetLinker() )
		{
			// Send to the object's linker.
			Object->GetLinker()->Preload( Object );
		}
	}
}

UObject* ULinkerLoad::CreateExport( INT Index )
{
	// Map the object into our table.
	FObjectExport& Export = ExportMap( Index );

	// Check whether we already loaded the object and if not whether the context flags allow loading it.
	if( !Export._Object && (Export.ObjectFlags & _ContextFlags) )
	{
		check(Export.ObjectName!=NAME_None || !(Export.ObjectFlags&RF_Public));
		check(GObjBeginLoadCount>0);

		// editor loads force exported object from original package, not cooked package
		if (GIsEditor && !GIsUCC && Export.HasAnyFlags(EF_ForcedExport))
		{
			FName ClassName = GetExportClassName(Index);
			FName OuterClassName;

			// don't bother loading top level packages
			if (Export.OuterIndex == ROOTPACKAGE_INDEX)
			{
				return NULL;
			}

			// get the class name of the outer, for imports or export outers
			if (IS_IMPORT_INDEX(Export.OuterIndex))
			{
				OuterClassName = ImportMap(-Export.OuterIndex-1).ClassName;
			}
			else
			{
				OuterClassName = GetExportClassName(Export.OuterIndex - 1);
			}

			// get the export's class
			UClass* LoadClass = Export.ClassIndex == 0 ? UClass::StaticClass() : (UClass*)IndexToObject(Export.ClassIndex);

			// don't load packages or subobjects (objects not directory in a packge)
			if (ClassName == NAME_Package || OuterClassName != NAME_Package)
			{
				// but try to at least find them
				UObject* Outer = IndexToObject(Export.OuterIndex);
				if (Outer)
				{
					Outer->GetLinker()->Preload(Outer);
					// must use FindObjectFast so that we don't create packages while looking for subobjects
					return UObject::StaticFindObjectFast(LoadClass, Outer, Export.ObjectName, TRUE);
				}
				return NULL;
			}

			// load from original package
			UObject* OriginalLinkerObject = UObject::StaticLoadObject(LoadClass, NULL, *GetExportPathName(Index, NULL, TRUE), NULL, LOAD_None, NULL);

			if (!OriginalLinkerObject)
			{
				warnf(TEXT("Failed to load forceexport object %s from original package"), *GetExportFullName(Index));
			}

			// this will cause Detach to crash because the _Linker is mismatched
			//Export._Object = OriginalLinkerObject;

			// return the object we just found (or NULL if we didn't)
			return OriginalLinkerObject;
		}

		// Get the object's class.
		UClass* LoadClass = (UClass*)IndexToObject( Export.ClassIndex );
		if( !LoadClass && Export.ClassIndex!=UCLASS_INDEX ) // Hack to load packages with classes which do not exist.
		{
			return NULL;
		}
		if( !LoadClass )
		{
			LoadClass = UClass::StaticClass();
		}
		check(LoadClass);
		check(LoadClass->GetClass()==UClass::StaticClass());

		// Only UClass objects and UProperty objects of intrinsic classes can have RF_Native set. Those property objects are never
		// serialized so we only have to worry about classes. If we encounter an object that is not a class and has RF_Native set
		// we warn about it and remove the flag.
		if( (Export.ObjectFlags & RF_Native) != 0 && !LoadClass->IsChildOf(UField::StaticClass()) )
		{
			warnf(NAME_Warning,TEXT("%s %s has RF_Native set but is not a UField derived class"),*LoadClass->GetName(),*Export.ObjectName.ToString());
			// Remove RF_Native;
			Export.ObjectFlags &= ~RF_Native;
		}

		if ( !LoadClass->HasAnyClassFlags(CLASS_Intrinsic) )
		{
			Preload( LoadClass );
			if ( LoadClass->HasAnyClassFlags(CLASS_Deprecated) && GIsEditor && !GIsUCC && !GIsGame )
			{
				if ( (Export.ObjectFlags&RF_ClassDefaultObject) == 0 )
				{
					warnf( NAME_Warning, *LocalizeError(TEXT("LoadedDeprecatedClassInstance"), TEXT("Core")), *GetExportFullName(Index), *LoadClass->GetPathName() );
				}
				EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, *LocalizeError(TEXT("LoadedDeprecatedClassInstance"), TEXT("Core")), *GetExportFullName(Index), *LoadClass->GetPathName() );
			}
		}
		else if ( !LoadClass->HasAnyClassFlags(CLASS_IsAUProperty|CLASS_IsAUState|CLASS_IsAUFunction) )
		{
			// if this object's class is intrinsic, we need to make sure that the class has been linked before creating any instances
			// of this class; otherwise, the instance will be initialized against incorrect defaults when InitProperties is called
			LoadClass->ConditionalLink();
		}

		// detect cases where a class has been made transient when there are existing instances of this class in content packages,
		// and this isn't the class default object; when this happens, it can cause issues which are difficult to debug since they'll
		// only appear much later after this package has been loaded
		if ( LoadClass->HasAnyClassFlags(CLASS_Transient) && (Export.ObjectFlags&RF_ClassDefaultObject) == 0 )
		{
			//@todo - should this actually be an assertion?
			warnf(NAME_Warning, *LocalizeError(TEXT("LoadingTransientInstance"), TEXT("Core")), *Filename, *Export.ObjectName.ToString(), *LoadClass->GetPathName());

			if ( GIsEditor && !GIsUCC && !GIsGame )
			{
				EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, *LocalizeError(TEXT("LoadingTransientInstance"), TEXT("Core")), *Filename, *Export.ObjectName.ToString(), *LoadClass->GetPathName() );
			}
		}

		// Find or create the object's Outer.
		UObject* ThisParent = NULL;
		if( Export.OuterIndex != ROOTPACKAGE_INDEX )
		{
			ThisParent = IndexToObject(Export.OuterIndex);
		}
		else if( Export.HasAnyFlags( EF_ForcedExport ) )
		{
			// Create the forced export in the TopLevel instead of LinkerRoot. Please note that CreatePackage
			// will find and return an existing object if one exists and only create a new one if there doesn't.
			Export._Object = CreatePackage( NULL, *Export.ObjectName.ToString() );
			check(Export._Object);
			((UPackage*)Export._Object)->InitNetInfo(this, Index);
			GForcedExportCount++;
		}
		else
		{
			ThisParent = LinkerRoot;
		}

		// If loading the object's Outer caused the object to be loaded or if it was a forced export package created
		// above, return it.
		if( Export._Object != NULL )
		{
			return Export._Object;
		}

		if( ThisParent == NULL )
		{
			// mark this export as unloadable (so that other exports that
			// reference this one won't continue to execute the above logic), then return NULL
			Export.ObjectFlags &= ~_ContextFlags;
			if ( GIsEditor && !GIsUCC )
			{
				EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, *FString::Printf(TEXT("Outer object for %s"), *Export.ObjectName.ToString()) );
			}
			else
			{
				// otherwise, return NULL and let the calling code determine what to do
				FString OuterName;
				if ( IS_IMPORT_INDEX(Export.OuterIndex) )
				{
					OuterName = GetImportFullName( -Export.OuterIndex - 1 );
				}
				else if ( Export.OuterIndex > 0 )
				{
					OuterName = GetExportFullName(Export.OuterIndex - 1 );
				}
				else
				{
					OuterName = LinkerRoot->GetFullName();
				}
				warnf(NAME_Error, TEXT("Failed to load Outer for resource '%s': %s"), *Export.ObjectName.ToString(), *OuterName);
			}
			return NULL;
		}

		if ( Export.ArchetypeIndex == Index + 1 )
		{
			// this export's archetype is pointing to itself!!
			// mark this export as unloadable (so that other exports that
			// reference this one won't continue to execute the above logic), then return NULL
			Export.ObjectFlags &= ~_ContextFlags;
			if ( GIsEditor && !GIsUCC )
			{
				EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, *FString::Printf(TEXT("Circular reference to archetype for %s.%s"), *ThisParent->GetPathName(), *Export.ObjectName.ToString()) );
			}
			else
			{
				warnf(NAME_Error, TEXT("Circular reference to archetype for %s.%s"), *ThisParent->GetPathName(), *Export.ObjectName.ToString());
			}

			return NULL;
		}

		// make sure an imported Template exists 
		if( !(LinkerRoot->PackageFlags & PKG_Cooked)  && (!GIsGame || GIsEditor || GIsUCC) && IS_IMPORT_INDEX(Export.ArchetypeIndex) ) 
		{
			VerifyImport(-Export.ArchetypeIndex - 1); 
		}

		// Find the Archetype object for the one we are loading.
		UObject* Template = NULL;
		if(Export.ArchetypeIndex != CLASSDEFAULTS_INDEX)
		{
			Template = IndexToObject(Export.ArchetypeIndex);

			// we couldn't load the exports's ObjectArchetype; unless the object's class is deprecated, issue a warning that the archetype is missing
			if( Template == NULL && !LoadClass->HasAnyClassFlags(CLASS_Deprecated) )
			{
				if( GIsEditor && !GIsUCC )
				{
					// if we're running in the editor, allow the resource to be loaded against the class default object
					// but flag this as a warning
					FString ArchetypeName;
					if ( IS_IMPORT_INDEX(Export.ArchetypeIndex) )
					{
						ArchetypeName = GetImportFullName( -Export.ArchetypeIndex - 1 );
					}
					else if ( Export.ArchetypeIndex > 0 )
					{
						ArchetypeName = GetExportFullName(Export.ArchetypeIndex - 1 );
					}
					else
					{
						ArchetypeName = TEXT("ClassDefaultObject");
					}
					EdLoadErrorf( FEdLoadError::TYPE_RESOURCE, TEXT("ObjectArchetype for '%s': %s"), *Export.ObjectName.ToString(), *ArchetypeName );
				}
				else
				{
					// otherwise, log a warning and just use the class default object
					FString ArchetypeName;
					if ( IS_IMPORT_INDEX(Export.ArchetypeIndex) )
					{
						ArchetypeName = GetImportFullName( -Export.ArchetypeIndex - 1 );
					}
					else if ( Export.ArchetypeIndex > 0 )
					{
						ArchetypeName = GetExportFullName(Export.ArchetypeIndex - 1 );
					}
					else
					{
						ArchetypeName = TEXT("ClassDefaultObject");
					}
					debugf( NAME_Warning, TEXT("Failed to load ObjectArchetype for resource '%s': %s"), *Export.ObjectName.ToString(), *ArchetypeName );
				}
			}
		}

		// If we could not find the Template (eg. the package containing it was deleted), use the class defaults instead.
		if(Template == NULL)
		{
			// force the default object to be created because we can't create an object
			// of this class unless it has a template to initialize itself against
			Template = LoadClass->GetDefaultObject(TRUE);
		}

		check(Template);
		checkSlow((Export.ObjectFlags&RF_ClassDefaultObject)!=0 || Template->IsA(LoadClass));

		// make sure the object's archetype is fully loaded before creating the object
		Preload(Template);

		// Try to find existing object first in case we're a forced export to be able to reconcile. Also do it for the
		// case of async loading as we cannot in-place replace objects.
		if( (LinkerRoot->PackageFlags & PKG_Cooked) 
		|| (GIsGame && !GIsEditor && !GIsUCC) 
		|| GIsAsyncLoading 
		|| Export.HasAnyFlags( EF_ForcedExport ) 
		|| LinkerRoot->ShouldFindExportsInMemoryFirst() )
		{
			// Find object after making sure it isn't already set. This would be bad as the code below NULLs it in a certain
			// case, which if it had been set would cause a linker detach mismatch.
			check( Export._Object == NULL );
			Export._Object = StaticFindObjectFastInternal( LoadClass, ThisParent, Export.ObjectName, TRUE, FALSE, 0 );
		
			// Object is found in memory.
			if( Export._Object )
			{
				// Native classes need to be in-place replaced for their initial load.
				if( LoadClass == UClass::StaticClass() && (Export.ObjectFlags&RF_Native) != 0 && ((UClass*)Export._Object)->bNeedsPropertiesLinked )
				{
					Export._Object = NULL;
				}
				// Found object, associate and return.
				else
				{
					// Mark that we need to dissociate forced exports later on if we are a forced export.
					if( Export.HasAnyFlags( EF_ForcedExport ) )
					{
						GForcedExportCount++;
					}
					// Associate linker with object to avoid detachment mismatches.
					else
					{
						Export._Object->SetLinker( this, Index );
					}
					return Export._Object;
				}
			}
		}

		// Create the export object, marking it with the appropriate flags to
		// indicate that the object's data still needs to be loaded.

#if CONSOLE
		EObjectFlags LoadFlags = (Export.ObjectFlags&RF_Load)|RF_NeedLoad|RF_NeedPostLoad|RF_NeedPostLoadSubobjects;
#else
		EObjectFlags LoadFlags = (Export.ObjectFlags&RF_Load);
		// if we are loading objects just to verify an object reference during script compilation,
		if ((GUglyHackFlags&HACK_VerifyObjectReferencesOnly) == 0
		||	(LoadFlags&RF_ClassDefaultObject) != 0					// only load this object if it's a class default object
		||	(LinkerRoot->PackageFlags&PKG_ContainsScript) != 0		// or we're loading an existing package and it's a script package
		||	ThisParent->IsTemplate(RF_ClassDefaultObject)			// or if its a subobject template in a CDO
		||	LoadClass->IsChildOf(UField::StaticClass()) )			// or if it is a UField
		{
			LoadFlags |= (RF_NeedLoad|RF_NeedPostLoad|RF_NeedPostLoadSubobjects);
		}
#endif
		Export._Object = StaticConstructObject
		(
			LoadClass,
			ThisParent,
			Export.ObjectName,
			LoadFlags | (GIsInitialLoad ? (RF_RootSet | RF_DisregardForGC) : 0),
			Template
		);
		
		if( Export._Object )
		{
			Export._Object->SetLinker( this, Index );

			// we created the object, but the data stored on disk for this object has not yet been loaded,
			// so add the object to the list of objects that need to be loaded, which will be processed
			// in UObject::EndLoad()
			GObjLoaded.AddItem( Export._Object );
			debugfSlow( NAME_DevLoad, TEXT("Created %s"), *Export._Object->GetFullName() );
		}
		else
		{
			debugf( NAME_Warning, TEXT("ULinker::CreatedExport failed to construct object %s %s"), *LoadClass->GetName(), *Export.ObjectName.ToString() );
		}

		if ( Export._Object != NULL )
		{
			// If it's a struct or class, set its parent.
			if( Export._Object->IsA(UStruct::StaticClass()) && Export.SuperIndex != NULLSUPER_INDEX )
			{
				((UStruct*)Export._Object)->SuperField = (UStruct*)IndexToObject( Export.SuperIndex );
			}

			// If it's a class, bind it to C++.
			if( Export._Object->IsA( UClass::StaticClass() ) )
			{
				((UClass*)Export._Object)->Bind();
			}
	
			// Mark that we need to dissociate forced exports later on.
			if( Export.HasAnyFlags( EF_ForcedExport ) )
			{
				GForcedExportCount++;
			}
		}
	}
	return Export._Object;
}

// Return the loaded object corresponding to an import index; any errors are fatal.
UObject* ULinkerLoad::CreateImport( INT Index )
{
	FObjectImport& Import = ImportMap( Index );
	if( Import.XObject == NULL )
	{
		// Look in memory first.
		if( !GIsEditor && !GIsUCC )
		{
			// Try to find existing version in memory first.
			UObject* ClassPackage = StaticFindObjectFast( UPackage::StaticClass(), NULL, Import.ClassPackage, FALSE, FALSE ); 
			if( ClassPackage )
			{
				UClass*	FindClass = (UClass*) StaticFindObjectFast( UClass::StaticClass(), ClassPackage, Import.ClassName, FALSE, FALSE ); 
				if( FindClass )
				{
					UObject*	FindObject		= NULL;
	
					// Import is a toplevel package.
					if( Import.OuterIndex == ROOTPACKAGE_INDEX )
					{
						FindObject = CreatePackage( NULL, *Import.ObjectName.ToString() );
					}
					// Import is regular import/ export.
					else
					{
						// Find the imports' outer.
						UObject* FindOuter = NULL;
						// Import.
						if( IS_IMPORT_INDEX(Import.OuterIndex) )
						{
							FObjectImport& OuterImport = ImportMap(-Import.OuterIndex-1);
							// Outer already in memory.
							if( OuterImport.XObject )
							{
								FindOuter = OuterImport.XObject;
							}
							// Outer is toplevel package, create/ find it.
							else if( OuterImport.OuterIndex == ROOTPACKAGE_INDEX )
							{
								FindOuter = CreatePackage( NULL, *OuterImport.ObjectName.ToString() );
							}
							// Outer is regular import/ export, use IndexToObject to potentially recursively load/ find it.
							else
							{
								FindOuter = IndexToObject( Import.OuterIndex );
							}
						}
						// Export.
						else 
						{
							// Create/ find the object's outer.
							FindOuter = IndexToObject( Import.OuterIndex );
						}
						if (!FindOuter)
						{
							FString OuterName;
							if ( IS_IMPORT_INDEX(Import.OuterIndex) )
							{
								OuterName = GetImportFullName( -Import.OuterIndex - 1 );
							}
							else if ( Import.OuterIndex > 0 )
							{
								OuterName = GetExportFullName(Import.OuterIndex - 1 );
							}
							else
							{
								OuterName = LinkerRoot->GetFullName();
							}
	
							warnf(NAME_Error, TEXT("Failed to load Outer for resource '%s': %s"), *Import.ObjectName.ToString(), *OuterName);
							return NULL;
						}
	
						// Find object now that we know it's class, outer and name.
						FindObject = StaticFindObjectFast( FindClass, FindOuter, Import.ObjectName, FALSE, FALSE );
					}

					if( FindObject )
					{		
						// Associate import and indicate that we associated an import for later cleanup.
						Import.XObject = FindObject;
						GImportCount++;
					}
				}
			}
		}

		if( Import.XObject == NULL && !(Summary.PackageFlags & PKG_RequireImportsAlreadyLoaded) )
		{
			if( Import.SourceLinker == NULL )
			{
				VerifyImport(Index);
			}
			if(Import.SourceIndex != INDEX_NONE)
			{
				check(Import.SourceLinker);
				Import.XObject = Import.SourceLinker->CreateExport( Import.SourceIndex );
				GImportCount++;
			}
		}
	}
	return Import.XObject;
}

// Map an import/export index to an object; all errors here are fatal.
UObject* ULinkerLoad::IndexToObject( PACKAGE_INDEX Index )
{
	if( Index > 0 )
	{
		if( !ExportMap.IsValidIndex( Index-1 ) )
			appErrorf( *LocalizeError(TEXT("ExportIndex"),TEXT("Core")), Index-1, ExportMap.Num() );			
		return CreateExport( Index-1 );
	}
	else if( Index < 0 )
	{
		if( !ImportMap.IsValidIndex( -Index-1 ) )
			appErrorf( *LocalizeError(TEXT("ImportIndex"),TEXT("Core")), -Index-1, ImportMap.Num() );
		return CreateImport( -Index-1 );
	}
	else return NULL;
}

// Detach an export from this linker.
void ULinkerLoad::DetachExport( INT i )
{
	FObjectExport& E = ExportMap( i );
	check(E._Object);
	if( !E._Object->IsValid() )
	{
		appErrorf( TEXT("Linker object %s %s.%s is invalid"), *GetExportClassName(i).ToString(), *LinkerRoot->GetName(), *E.ObjectName.ToString() );
	}
	if( E._Object->GetLinker()!=this )
	{
		UObject* Object = E._Object;
		debugf(TEXT("Object            : %s"), *Object->GetFullName() );
		debugf(TEXT("Object Linker     : %s"), *Object->GetLinker()->GetFullName() );
		debugf(TEXT("Linker LinkerRoot : %s"), Object->GetLinker() ? *Object->GetLinker()->LinkerRoot->GetFullName() : TEXT("None") );
		debugf(TEXT("Detach Linker     : %s"), *GetFullName() );
		debugf(TEXT("Detach LinkerRoot : %s"), *LinkerRoot->GetFullName() );
		appErrorf( TEXT("Linker object %s %s.%s mislinked!"), *GetExportClassName(i).ToString(), *LinkerRoot->GetName(), *E.ObjectName.ToString() );
	}
	if( E._Object->_LinkerIndex!=i )
	{
		appErrorf( TEXT("Linker object %s %s.%s misindexed!"), *GetExportClassName(i).ToString(), *LinkerRoot->GetName(), *E.ObjectName.ToString() );
	}
	ExportMap(i)._Object->SetLinker( NULL, INDEX_NONE );
}

/**
 * Detaches linker from bulk data/ exports and removes itself from array of loaders.
 *
 * @param	bEnsureAllBulkDataIsLoaded	Whether to load all bulk data first before detaching.
 */
void ULinkerLoad::Detach( UBOOL bEnsureAllBulkDataIsLoaded )
{
	// Detach all lazy loaders.
	DetachAllBulkData( bEnsureAllBulkDataIsLoaded );

	// Detach all objects linked with this linker.
	for( INT i=0; i<ExportMap.Num(); i++ )
	{	
		if( ExportMap(i)._Object )
		{
			DetachExport( i );
		}
	}

	// Remove from object manager, if it has been added.
	GObjLoaders.RemoveItem( this );
	if( Loader )
	{
		delete Loader;
	}
	Loader = NULL;

	// Empty out no longer used arrays.
	NameMap.Empty();
	ImportMap.Empty();
	ExportMap.Empty();

	// Make sure we're never associated with LinkerRoot again.
	LinkerRoot = NULL;
}

void ULinkerLoad::BeginDestroy()
{
	// Detaches linker.
	Detach( FALSE );
	Super::BeginDestroy();
}

/**
 * Attaches/ associates the passed in bulk data object with the linker.
 *
 * @param	Owner		UObject owning the bulk data
 * @param	BulkData	Bulk data object to associate
 */
void ULinkerLoad::AttachBulkData( UObject* Owner, FUntypedBulkData* BulkData )
{
	check( BulkDataLoaders.FindItemIndex(BulkData)==INDEX_NONE );
	BulkDataLoaders.AddItem( BulkData );
}

/**
 * Detaches the passed in bulk data object from the linker.
 *
 * @param	BulkData	Bulk data object to detach
 * @param	bEnsureBulkDataIsLoaded	Whether to ensure that the bulk data is loaded before detaching
 */
void ULinkerLoad::DetachBulkData( FUntypedBulkData* BulkData, UBOOL bEnsureBulkDataIsLoaded )
{
	INT RemovedCount = BulkDataLoaders.RemoveItem( BulkData );
	if( RemovedCount!=1 )
	{	
		appErrorf( TEXT("Detachment inconsistency: %i (%s)"), RemovedCount, *Filename );
	}
	BulkData->DetachFromArchive( this, bEnsureBulkDataIsLoaded );
}

/**
 * Detaches all attached bulk  data objects.
 *
 * @param	bEnsureBulkDataIsLoaded	Whether to ensure that the bulk data is loaded before detaching
 */
void ULinkerLoad::DetachAllBulkData( UBOOL bEnsureAllBulkDataIsLoaded )
{
	for( INT BulkDataIndex=0; BulkDataIndex<BulkDataLoaders.Num(); BulkDataIndex++ )
	{
		FUntypedBulkData* BulkData = BulkDataLoaders(BulkDataIndex);
		check( BulkData );
		BulkData->DetachFromArchive( this, bEnsureAllBulkDataIsLoaded );
	}
	BulkDataLoaders.Empty();
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
 * @param	PrecacheOffset	Offset at which to begin precaching.
 * @param	PrecacheSize	Number of bytes to precache
 * @return	FALSE if precache operation is still pending, TRUE otherwise
 */
UBOOL ULinkerLoad::Precache( INT PrecacheOffset, INT PrecacheSize )
{
	return Loader->Precache( PrecacheOffset, PrecacheSize );
}

void ULinkerLoad::Seek( INT InPos )
{
	Loader->Seek( InPos );
}

INT ULinkerLoad::Tell()
{
	return Loader->Tell();
}

INT ULinkerLoad::TotalSize()
{
	return Loader->TotalSize();
}

FArchive& ULinkerLoad::operator<<( UObject*& Object )
{
	INT Index;
	FArchive& Ar = *this;
	Ar << Index;

	UObject* Temporary = IndexToObject( Index );
	appMemcpy(&Object, &Temporary, sizeof(UObject*));
	return *this;
}

FArchive& ULinkerLoad::operator<<( FName& Name )
{
	NAME_INDEX NameIndex;
	FArchive& Ar = *this;
	Ar << NameIndex;

	if( !NameMap.IsValidIndex(NameIndex) )
	{
		appErrorf( TEXT("Bad name index %i/%i"), NameIndex, NameMap.Num() );
	}

	// @GEMINI_TODO: Are names ever not loaded because of NotForClient, etc? If not, remove this check
	// if the name wasn't loaded (because it wasn't valid in this context)
	if (NameMap(NameIndex) == NAME_None)
	{
		if (Ver() >= VER_FNAME_CHANGE_NAME_SPLIT)
		{
			INT TempNumber;
			Ar << TempNumber;
		}
		Name = NAME_None;
	}
	else
	{
		// split up old names
		if (Ver() < VER_FNAME_CHANGE_NAME_SPLIT)
		{
			// create a new FName using the name and number frmo the name map (since the name was split up when being put into the name map)
			Name = NameMap(NameIndex);
		}
		// split names that were saved out un-split before we were forcing them to be split
		else if (Ver() < VER_NAME_TABLE_LOADING_CHANGE)
		{
			// a number was written out, but it may not have been split off, so look in the serialized number and in
			// the name map, and use the one that is not 0, if there is one
			INT Number;
			Ar << Number;
			// if there was a number and the namemap got a number from being split by the namemap serialization
			// then that means we had a name like A_6_2 that was split before to A_6, 2 and now it was split again
			// so we need to reconstruct the full thing
			if (Number && NameMap(NameIndex).GetNumber())
			{
				Name = FName(*FString::Printf(TEXT("%s_%d"), NameMap(NameIndex).GetName(), NAME_INTERNAL_TO_EXTERNAL(NameMap(NameIndex).GetNumber())), Number);
			}
			else
			{
				// otherwise, we can just add them (since at least 1 is zero) to get the actual number
				Name = FName((EName)NameMap(NameIndex).GetIndex(), Number + NameMap(NameIndex).GetNumber());
			}
		}
		else
		{
			INT Number;
			Ar << Number;
			// simply create the name from the NameMap's name index and the serialized instance number
			Name = FName((EName)NameMap(NameIndex).GetIndex(), Number);
		}
	}

	return *this;
}

void ULinkerLoad::Serialize( void* V, INT Length )
{
	Loader->Serialize( V, Length );
}

/**
* Kick off an async load of a package file into memory
* 
* @param PackageName Name of package to read in. Must be the same name as passed into LoadPackage
*/
void ULinkerLoad::AsyncPreloadPackage(const TCHAR* PackageName)
{
	// get package filename
	FString PackageFilename;
	if (!GPackageFileCache->FindPackageFile(PackageName, NULL, PackageFilename))
	{
		appErrorf(TEXT("Failed to find file for package %s for async preloading."), PackageName);
	}

	// make sure it wasn't already there
	check(PackagePrecacheMap.Find(*PackageFilename) == NULL);

	// add a new one to the map
	FPackagePrecacheInfo& PrecacheInfo = PackagePrecacheMap.Set(*PackageFilename, FPackagePrecacheInfo());

	// make a new sync object (on heap so the precache info can be copied in the map, etc)
	PrecacheInfo.SynchronizationObject = new FThreadSafeCounter;

	// increment the sync object, later we'll wait for it to be decremented
	PrecacheInfo.SynchronizationObject->Increment();
	
	// request generic async IO system.
	FIOSystem* IO = GIOManager->GetIOSystem(IOSYSTEM_GenericAsync); 

	// default to not compressed
	UBOOL bWasCompressed = FALSE;

	// get filesize (first checking if it was compressed)
	INT UncompressedSize = GFileManager->UncompressedFileSize(*PackageFilename);;
	INT FileSize = GFileManager->FileSize(*PackageFilename);

	// if we were compressed, the size we care about on the other end is the uncompressed size
	PrecacheInfo.PackageDataSize = UncompressedSize == -1 ? FileSize : UncompressedSize;
	
	// allocate enough space
	PrecacheInfo.PackageData = appMalloc(PrecacheInfo.PackageDataSize);

	QWORD RequestId;
	// kick off the async read (uncompressing if needed) of the whole file and make sure it worked
	if (UncompressedSize != -1)
	{
		PrecacheInfo.PackageDataSize = UncompressedSize;
		// @todo: support other compressed methods?
		RequestId = IO->LoadCompressedData(
						PackageFilename, 
						0, 
						FileSize, 
						UncompressedSize, 
						PrecacheInfo.PackageData, 
#if WITH_LZO
						COMPRESS_LZO, 
#else	//#if WITH_LZO
						COMPRESS_ZLIB,
#endif	//#if WITH_LZO
						PrecacheInfo.SynchronizationObject,
						AIOP_Normal);
	}
	else
	{
		PrecacheInfo.PackageDataSize = FileSize;
		RequestId = IO->LoadData(
						PackageFilename, 
						0, 
						PrecacheInfo.PackageDataSize, 
						PrecacheInfo.PackageData, 
						PrecacheInfo.SynchronizationObject, 
						AIOP_Normal);
	}
	check(RequestId);
}

/**
 * Called when an object begins serializing property data using script serialization.
 */
void ULinkerLoad::MarkScriptSerializationStart( const UObject* Obj )
{
	if ( Obj != NULL && Obj->GetLinker() == this && ExportMap.IsValidIndex(Obj->GetLinkerIndex()) )
	{
		FObjectExport& Export = ExportMap(Obj->GetLinkerIndex());
		Export.ScriptSerializationStartOffset = Tell();
	}
}

/**
 * Called when an object stops serializing property data using script serialization.
 */
void ULinkerLoad::MarkScriptSerializationEnd( const UObject* Obj )
{
	if ( Obj != NULL && Obj->GetLinker() == this && ExportMap.IsValidIndex(Obj->GetLinkerIndex()) )
	{
		FObjectExport& Export = ExportMap(Obj->GetLinkerIndex());
		Export.ScriptSerializationEndOffset = Tell();
	}
}

/*----------------------------------------------------------------------------
	ULinkerSave.
----------------------------------------------------------------------------*/

ULinkerSave::ULinkerSave( UPackage* InParent, const TCHAR* InFilename, UBOOL bForceByteSwapping )
:	ULinker( InParent, InFilename )
,	Saver( NULL )
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	// Create file saver.
	Saver = GFileManager->CreateFileWriter( InFilename, 0, GThrow );
	if( !Saver )
	{
		appThrowf( *FString::Printf(*LocalizeError(TEXT("OpenFailed"),TEXT("Core")), InFilename, *GFileManager->GetCurrentDirectory()) );
	}

	// Set main summary info.
	Summary.Tag           = PACKAGE_FILE_TAG;
	Summary.SetFileVersions( GPackageFileVersion, GPackageFileLicenseeVersion );
	Summary.EngineVersion =	GEngineVersion;
	Summary.CookedContentVersion = GPackageFileCookedContentVersion;
	Summary.PackageFlags  = Cast<UPackage>(LinkerRoot) ? Cast<UPackage>(LinkerRoot)->PackageFlags : 0;

	UPackage *Package = Cast<UPackage>(LinkerRoot);
	if (Package)
	{
		Summary.FolderName = Package->GetFolderName().ToString();
	}

	// Set status info.
	ArIsSaving			= 1;
	ArIsPersistent		= 1;
	ArForEdit			= GIsEditor;
	ArForClient			= 1;
	ArForServer			= 1;
	ArForceByteSwapping	= bForceByteSwapping;

	// Allocate indices.
	ObjectIndices.AddZeroed( UObject::GObjObjects.Num() );
	NameIndices  .AddZeroed( FName::GetMaxNames() );
}

/**
 * Detaches file saver and hence file handle.
 */
void ULinkerSave::Detach()
{
	if( Saver )
	{
		delete Saver;
	}
	Saver = NULL;
}

void ULinkerSave::BeginDestroy()
{
	// Detach file saver/ handle.
	Detach();
	Super::BeginDestroy();
}

// FArchive interface.
INT ULinkerSave::MapName( const FName* Name ) const
{
	return NameIndices(Name->GetIndex());
}

INT ULinkerSave::MapObject( const UObject* Object ) const
{
	return Object != NULL 
		? ObjectIndices(Object->GetIndex())
		: 0;
}

void ULinkerSave::Seek( INT InPos )
{
	Saver->Seek( InPos );
}

INT ULinkerSave::Tell()
{
	return Saver->Tell();
}

void ULinkerSave::Serialize( void* V, INT Length )
{
	Saver->Serialize( V, Length );
}

IMPLEMENT_CLASS(ULinker);
IMPLEMENT_CLASS(ULinkerLoad);
IMPLEMENT_CLASS(ULinkerSave);
