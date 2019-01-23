/*=============================================================================
	SavePackage.cpp: File saving support.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"
#include "DebuggingDefines.h"

static const INT MAX_UNREAL_FILENAME_LENGTH = 30;
static const INT MAX_XENON_FILENAME_LENGTH = 44;

static const INT MAX_MERGED_COMPRESSION_CHUNKSIZE = 1024 * 1024;

/**
 * Helper class for package compression.
 */
struct FFileCompressionHelper
{
public:
	/**
	 * Compresses the passed in src file and writes it to destination.
	 *
	 * @param	SrcFilename		Filename to compress
	 * @param	DstFilename		Output name of compressed file, cannot be the same as SrcFilename
	 * @param	SrcLinker		ULinkerSave object used to save src file
	 *
	 * @return TRUE if sucessful, FALSE otherwise
	 */
	UBOOL CompressFile( const TCHAR* SrcFilename, const TCHAR* DstFilename, ULinkerSave* SrcLinker )
	{
		FFilename TmpFilename = TEXT("SaveCompressed.tmp");

		// Create file reader and writer...
		FArchive* FileReader = GFileManager->CreateFileReader( SrcFilename );
		FArchive* FileWriter = GFileManager->CreateFileWriter( *TmpFilename );

		// ... and abort if either operation wasn't successful.
		if( !FileReader || !FileWriter )
		{
			// Delete potentially created reader or writer.
			delete FileReader;
			delete FileWriter;
			// Delete temporary file.
			GFileManager->Delete( *TmpFilename );
			// Failure.
			return FALSE;
		}

		// Read package file summary from source file.
		FPackageFileSummary FileSummary;
		(*FileReader) << FileSummary;

		// Propagate byte swapping.
		FileWriter->SetByteSwapping( SrcLinker->ForceByteSwapping() );

		// We don't compress the package file summary but treat everything afterwards
		// till the first export as a single chunk. This basically lumps name and import 
		// tables into one compressed block.
		INT StartOffset			= FileReader->Tell();
		INT RemainingHeaderSize	= SrcLinker->Summary.TotalHeaderSize - StartOffset;
		CurrentChunk.UncompressedSize	= RemainingHeaderSize;
		CurrentChunk.UncompressedOffset	= StartOffset;
		
		// Iterate over all exports and add them separately. The underlying code will take
		// care of merging small blocks.
		for( INT ExportIndex=0; ExportIndex<SrcLinker->ExportMap.Num(); ExportIndex++ )
		{
			const FObjectExport& Export = SrcLinker->ExportMap(ExportIndex);		
			AddToChunk( Export.SerialSize );
		}

		// Finish chunk in flight and reset current chunk with size 0.
		FinishCurrentAndCreateNewChunk( 0 );

		// Write base version of package file summary after updating compressed chunks array and compression flags.
#if WITH_LZO
		FileSummary.CompressionFlags = COMPRESS_LZO;
#else	//#if WITH_LZO
		FileSummary.CompressionFlags = COMPRESS_ZLIB;
#endif	//#if WITH_LZO
		FileSummary.CompressedChunks = CompressedChunks;
		(*FileWriter) << FileSummary;

		// Reset internal state so subsequent calls will work.
		CompressedChunks.Empty();
		CurrentChunk = FCompressedChunk();

		// Allocate temporary buffers for reading and compression.
		INT		SrcBufferSize	= RemainingHeaderSize;
		void*	SrcBuffer		= appMalloc( SrcBufferSize );

		// Iterate over all chunks, read the data, compress and write it out to destination file.
		for( INT ChunkIndex=0; ChunkIndex<FileSummary.CompressedChunks.Num(); ChunkIndex++ )
		{
			FCompressedChunk& Chunk = FileSummary.CompressedChunks(ChunkIndex);

			// Increase temporary buffer sizes if they are too small.
			if( SrcBufferSize < Chunk.UncompressedSize )
			{
				SrcBufferSize = Chunk.UncompressedSize;
				SrcBuffer = appRealloc( SrcBuffer, SrcBufferSize );
			}
			
			// Verify that we're not skipping any data.
			check( Chunk.UncompressedOffset == FileReader->Tell() );

			// Read src/ uncompressed data.
			FileReader->Serialize( SrcBuffer, Chunk.UncompressedSize );

			// Keep track of offset.
			Chunk.CompressedOffset	= FileWriter->Tell();
			// Serialize compressed. This is compatible with async LoadCompressedData.
			FileWriter->SerializeCompressed( SrcBuffer, Chunk.UncompressedSize, (ECompressionFlags) FileSummary.CompressionFlags );
			// Keep track of compressed size.
			Chunk.CompressedSize	= FileWriter->Tell() - Chunk.CompressedOffset;		
		}

		// Verify that we've compressed everything.
		check( FileReader->AtEnd() );

		// Serialize file summary again - this time CompressedChunks array is going to contain compressed size and offsets.
		FileWriter->Seek( 0 );
		(*FileWriter) << FileSummary;

		// Free intermediate buffers.
		appFree( SrcBuffer );

		// Tear down file reader and write. This will flush the writer first.
		delete FileReader;
		delete FileWriter;

		// Compression was successful, now move file.
		UBOOL bMoveSucceded = GFileManager->Move( DstFilename, *TmpFilename );
		return bMoveSucceded;
	}

	/**
	 * Compresses the passed in src file and writes it to destination.
	 *
	 * @param	SrcFilename			Filename to compress
	 * @param	DstFilename			Output name of compressed file, cannot be the same as SrcFilename
	 * @param	bForceByteswapping	Should the output file be force-byteswapped (we can't tell as there's no Source Linker with the info)
	 *
	 * @return TRUE if successful, FALSE otherwise
	 */
	UBOOL FullyCompressFile( const TCHAR* SrcFilename, const TCHAR* DstFilename, UBOOL bForceByteSwapping )
	{
		FFilename TmpFilename = TEXT("SaveCompressed.tmp");

		// Create file reader and writer...
		FArchive* FileReader = GFileManager->CreateFileReader( SrcFilename );
		FArchive* FileWriter = GFileManager->CreateFileWriter( *TmpFilename );

		// ... and abort if either operation wasn't successful.
		if( !FileReader || !FileWriter )
		{
			// Delete potentially created reader or writer.
			delete FileReader;
			delete FileWriter;
			// Delete temporary file.
			GFileManager->Delete( *TmpFilename );
			// Failure.
			return FALSE;
		}

		// read in source file
		const INT FileSize = FileReader->TotalSize();
		
		// Force byte swapping if needed
		FileWriter->SetByteSwapping(bForceByteSwapping);
		
		// write it out compressed (passing in the FileReader so that the writer can read in small chunks to avoid 
		// single huge allocation of the entire source package size)
#if WITH_LZO
		FileWriter->SerializeCompressed(FileReader, FileSize, COMPRESS_LZO, TRUE);
#else	//#if WITH_LZO
		FileWriter->SerializeCompressed(FileReader, FileSize, COMPRESS_ZLIB, TRUE);
#endif	//#if WITH_LZO
		
		delete FileReader;
		delete FileWriter;
		
		// make the 
		// Compression was successful, now move file.
		UBOOL bMoveSucceded = GFileManager->Move( DstFilename, *TmpFilename );

		// if the move succeeded, create a .uncompressed_size file in the cooked directory so that the 
		// cooker frontend can create the table of contents
		if (bMoveSucceded)
		{
			// write out the size of the original file to the string
			FString SizeString = FString::Printf(TEXT("%d%s"), FileSize, LINE_TERMINATOR);
			appSaveStringToFile(SizeString, *(FString(DstFilename) + TEXT(".uncompressed_size")));
		}
		return bMoveSucceded;
	}

private:
	/**
	 * Tries to add bytes to current chunk and creates a new one if there is not enough space.
	 *
	 * @param	Size	Number of bytes to try to add to current chunk
	 */
	void AddToChunk( INT Size )
	{
		// Resulting chunk would be too big.
		if( CurrentChunk.UncompressedSize + Size > MAX_MERGED_COMPRESSION_CHUNKSIZE )
		{
			// Finish up current chunk and create a new one of passed in size.
			FinishCurrentAndCreateNewChunk( Size );
		}
		// Add bytes to existing chunk.
		else
		{
			CurrentChunk.UncompressedSize += Size;
		}
	}

	/**
	 * Finish current chunk and add it to the CompressedChunks array. This also creates a new
	 * chunk with a base size passed in.
	 *
	 * @param	Size	Size in bytes of new chunk to create.
	 */
	void FinishCurrentAndCreateNewChunk( INT Size )
	{
		CompressedChunks.AddItem( CurrentChunk );
		INT Offset = CurrentChunk.UncompressedOffset + CurrentChunk.UncompressedSize;
		CurrentChunk = FCompressedChunk();
		CurrentChunk.UncompressedOffset	= Offset;
		CurrentChunk.UncompressedSize	= Size;
	}
	
	/** Compressed chunks, populated by AddToChunk/ FinishCurrentAndCreateNewChunk.	*/
	TArray<FCompressedChunk>	CompressedChunks;
	/** Current chunk, used by merging code.										*/
	FCompressedChunk			CurrentChunk;
};


/**
 * Find most likely culprit that caused the objects in the passed in array to be considered for saving.
 *
 * @param	BadObjects	array of objects that are considered "bad" (e.g. non- RF_Public, in different map package, ...)
 * @return	UObject that is considered the most likely culprit causing them to be referenced or NULL
 */
static void FindMostLikelyCulprit( TArray<UObject*> BadObjects, UObject*& MostLikelyCulprit, UProperty*& PropertyRef )
{
	MostLikelyCulprit	= NULL;

	// Iterate over all objects that are marked as unserializable/ bad and print out their referencers.
	for( INT BadObjIndex=0; BadObjIndex<BadObjects.Num(); BadObjIndex++ )
	{
		UObject* Obj = BadObjects(BadObjIndex);

		warnf( TEXT("\r\nReferencers of %s:"), *Obj->GetFullName() );
		INT RefObjIndex = 0;
		for( FObjectIterator RefIt; RefIt; ++RefIt, RefObjIndex++ )
		{
			UObject* RefObj = *RefIt;

			if( RefObj->HasAnyFlags(RF_TagExp|RF_TagImp) )
			{
				if ( RefObj->GetFName() == NAME_PersistentLevel || RefObj->GetFName() == NAME_TheWorld )
				{
					// these types of references should be ignored
					continue;
				}

				FArchiveFindCulprit ArFind(Obj,RefObj,true);
				TArray<UProperty*> Referencers;

				INT Count = ArFind.GetCount(Referencers);
				if ( Count > 0 )
				{
					warnf( TEXT("\t%s (%i refs)"), *RefObj->GetFullName(), Count );
					for ( INT i = 0; i < Referencers.Num(); i++ )
					{
						UProperty* Prop = Referencers(i);
						warnf( TEXT("\t\t%i) %s"), i, *Prop->GetFullName());
						PropertyRef = Prop;
					}
			
					MostLikelyCulprit = Obj;
				}
			}
		}
	}

	check(MostLikelyCulprit != NULL );
}

/**
 * Helper structure encapsulating functionality to sort a linker's name map according to the order of the names a package being conformed against.
 */
struct FObjectNameSortHelper
{
private:
	/** the linker that we're sorting names for */
	friend void Sort<FName,FObjectNameSortHelper>( FName*, INT );

	/** Comparison function used by Sort */
	static INT Compare( const FName& A, const FName& B )
	{
		return appStricmp(*A.ToString(), *B.ToString());
	}

public:
	/**
	 * Sorts names according to the order in which they occur in the list of NameIndices.  If a package is specified to be conformed against, ensures that the order
	 * of the names match the order in which the corresponding names occur in the old package.
	 *
	 * @param	Linker				linker containing the names that need to be sorted
	 * @param	LinkerToConformTo	optional linker to conform against.
	 */
	void SortNames( ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo=NULL )
	{
		INT SortStartPosition = 0;

		if ( LinkerToConformTo != NULL )
		{
			SortStartPosition = LinkerToConformTo->NameMap.Num();
			TArray<FName> ConformedNameMap = LinkerToConformTo->NameMap;
			for ( INT NameIndex = 0; NameIndex < Linker->NameMap.Num(); NameIndex++ )
			{
				FName& CurrentName = Linker->NameMap(NameIndex);
				if ( !ConformedNameMap.ContainsItem(CurrentName) )
				{
					ConformedNameMap.AddItem(CurrentName);
				}
			}

			Linker->NameMap = ConformedNameMap;
			for ( INT NameIndex = 0; NameIndex < Linker->NameMap.Num(); NameIndex++ )
			{
				FName& CurrentName = Linker->NameMap(NameIndex);
				CurrentName.SetFlags(RF_LoadContextFlags);
			}
		}

		if ( SortStartPosition < Linker->NameMap.Num() )
		{
			Sort<FName,FObjectNameSortHelper>( &Linker->NameMap(SortStartPosition), Linker->NameMap.Num() - SortStartPosition );
		}
	}
};

/**
 * Helper structure to encapsulate sorting a linker's import table according to the of the import table of the package being conformed against.
 */
struct FObjectImportSortHelper
{
private:
	/**
	 * Allows Compare access to the object full name lookup map
	 */
	static FObjectImportSortHelper* Sorter;

	/**
	 * Map of UObject => full name; optimization for sorting.
	 */
	TMap<UObject*,FString>			ObjectToFullNameMap;

	/** the linker that we're sorting names for */
	friend void Sort<FObjectImport,FObjectImportSortHelper>( FObjectImport*, INT );

	/** Comparison function used by Sort */
	static INT Compare( const FObjectImport& A, const FObjectImport& B )
	{
		checkSlow(Sorter);

		INT Result = 0;
		if ( A.XObject == NULL )
		{
			Result = 1;
		}
		else if ( B.XObject == NULL )
		{
			Result = -1;
		}
		else
		{
			FString* FullNameA = Sorter->ObjectToFullNameMap.Find(A.XObject);
			FString* FullNameB = Sorter->ObjectToFullNameMap.Find(B.XObject);
			checkSlow(FullNameA);
			checkSlow(FullNameB);

			Result = appStricmp(**FullNameA, **FullNameB);
		}

		return Result;
	}

public:

	/**
	 * Sorts imports according to the order in which they occur in the list of imports.  If a package is specified to be conformed against, ensures that the order
	 * of the imports match the order in which the corresponding imports occur in the old package.
	 *
	 * @param	Linker				linker containing the imports that need to be sorted
	 * @param	LinkerToConformTo	optional linker to conform against.
	 */
	void SortImports( ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo=NULL )
	{
		INT SortStartPosition=0;
		TArray<FObjectImport>& Imports = Linker->ImportMap;
		if ( LinkerToConformTo )
		{
			// intended to be a copy
			TArray<FObjectImport> Orig = Imports;
			Imports.Empty(Imports.Num());

			// this array tracks which imports from the new package exist in the old package
			TArray<BYTE> Used;
			Used.AddZeroed(Orig.Num());

			TMap<FString,INT> OriginalImportIndexes;
			for ( INT i = 0; i < Orig.Num(); i++ )
			{
				FObjectImport& Import = Orig(i);
				FString ImportFullName = Import.XObject->GetFullName();

				OriginalImportIndexes.Set( *ImportFullName, i );
				ObjectToFullNameMap.Set(Import.XObject, *ImportFullName);
			}

			for( INT i=0; i<LinkerToConformTo->ImportMap.Num(); i++ )
			{
				// determine whether the new version of the package contains this export from the old package
				INT* OriginalImportPosition = OriginalImportIndexes.Find( *LinkerToConformTo->GetImportFullName(i) );
				if( OriginalImportPosition )
				{
					// this import exists in the new package as well,
					// create a copy of the FObjectImport located at the original index and place it
					// into the matching position in the new package's import map
					FObjectImport* NewImport = new(Imports) FObjectImport( Orig(*OriginalImportPosition) );
					check(NewImport->XObject == Orig(*OriginalImportPosition).XObject);
					Used( *OriginalImportPosition ) = 1;
				}
				else
				{
					// this import no longer exists in the new package
					new(Imports)FObjectImport( NULL );
				}
			}

			SortStartPosition = LinkerToConformTo->ImportMap.Num();
			for( INT i=0; i<Used.Num(); i++ )
			{
				if( !Used(i) )
				{
					// the FObjectImport located at pos "i" in the original import table did not
					// exist in the old package - add it to the end of the import table
					new(Imports) FObjectImport( Orig(i) );
				}
			}
		}
		else
		{
			for ( INT ImportIndex = 0; ImportIndex < Linker->ImportMap.Num(); ImportIndex++ )
			{
				const FObjectImport& Import = Linker->ImportMap(ImportIndex);
				if ( Import.XObject )
				{
					ObjectToFullNameMap.Set(Import.XObject, *Import.XObject->GetFullName());
				}
			}
		}

		if ( SortStartPosition < Linker->ImportMap.Num() )
		{
			Sorter = this;
			Sort<FObjectImport,FObjectImportSortHelper>( &Linker->ImportMap(SortStartPosition), Linker->ImportMap.Num() - SortStartPosition );
			Sorter = NULL;
		}
	}
};
FObjectImportSortHelper* FObjectImportSortHelper::Sorter = NULL;

/**
 * Helper structure to encapsulate sorting a linker's export table alphabetically, taking into account conforming to other linkers.
 */
struct FObjectExportSortHelper
{
private:
	/**
	 * Allows Compare access to the object full name lookup map
	 */
	static FObjectExportSortHelper* Sorter;

	/**
	 * Map of UObject => full name; optimization for sorting.
	 */
	TMap<UObject*,FString>			ObjectToFullNameMap;

	/** the linker that we're sorting exports for */
	friend void Sort<FObjectExport,FObjectExportSortHelper>( FObjectExport*, INT );

	/** Comparison function used by Sort */
	static INT Compare( const FObjectExport& A, const FObjectExport& B )
	{
		checkSlow(Sorter);

		INT Result = 0;
		if ( A._Object == NULL )
		{
			Result = 1;
		}
		else if ( B._Object == NULL )
		{
			Result = -1;
		}
		else
		{
			FString* FullNameA = Sorter->ObjectToFullNameMap.Find(A._Object);
			FString* FullNameB = Sorter->ObjectToFullNameMap.Find(B._Object);
			checkSlow(FullNameA);
			checkSlow(FullNameB);

			Result = appStricmp(**FullNameA, **FullNameB);
		}

		return Result;
	}

public:
	/**
	 * Sorts exports alphabetically.  If a package is specified to be conformed against, ensures that the order
	 * of the exports match the order in which the corresponding exports occur in the old package.
	 *
	 * @param	Linker				linker containing the exports that need to be sorted
	 * @param	LinkerToConformTo	optional linker to conform against.
	 */
	void SortExports( ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo=NULL )
	{
		INT SortStartPosition=0;
		if ( LinkerToConformTo )
		{
			// build a map of object full names to the index into the new linker's export map prior to sorting.
			// we need to do a little trickery here to generate an object path name that will match what we'll get back
			// when we call GetExportFullName on the LinkerToConformTo's exports, due to localized packages and forced exports.
			const FString LinkerName = Linker->LinkerRoot->GetName();
			const FString PathNamePrefix = LinkerName + TEXT(".");

			// Populate object to current index map.
			TMap<FString,INT> OriginalExportIndexes;
			for( INT ExportIndex=0; ExportIndex < Linker->ExportMap.Num(); ExportIndex++ )
			{
				const FObjectExport& Export = Linker->ExportMap(ExportIndex);
				if( Export._Object )
				{
					// get the path name for this object; if the object is contained within the package we're saving,
					// we don't want the returned path name to contain the package name since we'll be adding that on
					// to ensure that forced exports have the same outermost name as the non-forced exports
					FString ObjectPathName = Export._Object->GetPathName(Linker->LinkerRoot);
					FString ExportFullName = Export._Object->GetClass()->GetName() + TEXT(" ") + PathNamePrefix + ObjectPathName;

					// Set the index (key) in the map to the index of this object into the export map.
					OriginalExportIndexes.Set( *ExportFullName, ExportIndex );
					ObjectToFullNameMap.Set(Export._Object, *ExportFullName);
				}
			}

			// backup the existing export list so we can empty the linker's actual list
			TArray<FObjectExport> OldExportMap = Linker->ExportMap;
			Linker->ExportMap.Empty(Linker->ExportMap.Num());

			// this array tracks which exports from the new package exist in the old package
			TArray<BYTE> Used;
			Used.AddZeroed(OldExportMap.Num());

			for( INT i = 0; i<LinkerToConformTo->ExportMap.Num(); i++ )
			{
				// determine whether the new version of the package contains this export from the old package
				FString ExportFullName = LinkerToConformTo->GetExportFullName(i, *LinkerName);
				INT* OriginalExportPosition = OriginalExportIndexes.Find( *ExportFullName );
				if( OriginalExportPosition )
				{
					// this export exists in the new package as well,
					// create a copy of the FObjectExport located at the original index and place it
					// into the matching position in the new package's export map
					FObjectExport* NewExport = new(Linker->ExportMap) FObjectExport( OldExportMap(*OriginalExportPosition) );
					check(NewExport->_Object == OldExportMap(*OriginalExportPosition)._Object);
					Used( *OriginalExportPosition ) = 1;
				}
				else
				{
					// this export no longer exists in the new package; to ensure that the _LinkerIndex matches, add an empty entry to pad the list
					new(Linker->ExportMap)FObjectExport( NULL );
					debugf(TEXT("No matching export found in original package for export %i: %s"), i, *ExportFullName);
				}
			}

			SortStartPosition = LinkerToConformTo->ExportMap.Num();
			for( INT i=0; i<Used.Num(); i++ )
			{
				if( !Used(i) )
				{
					// the FObjectExport located at pos "i" in the original export table did not
					// exist in the old package - add it to the end of the export table
					new(Linker->ExportMap) FObjectExport( OldExportMap(i) );
				}
			}

#if DO_GUARD_SLOW

			// sanity-check: make sure that all exports which existed in the linker before we sorted exist in the linker's export map now
			for( INT OldExportMapIndex=0; OldExportMapIndex<OldExportMap.Num(); OldExportMapIndex++ )
			{
				INT ExportMapIndex = 0;
				for( ExportMapIndex=0; ExportMapIndex<Linker->ExportMap.Num(); ExportMapIndex++ )
				{
					if( Linker->ExportMap(ExportMapIndex)._Object == OldExportMap(OldExportMapIndex)._Object )
					{
						break;
					}
				}
				check(ExportMapIndex<Linker->ExportMap.Num());
			}
#endif
		}
		else
		{
			for ( INT ExportIndex = 0; ExportIndex < Linker->ExportMap.Num(); ExportIndex++ )
			{
				const FObjectExport& Export = Linker->ExportMap(ExportIndex);
				if ( Export._Object )
				{
					ObjectToFullNameMap.Set(Export._Object, *Export._Object->GetFullName());
				}
			}
		}

		if ( SortStartPosition < Linker->ExportMap.Num() )
		{
			Sorter = this;
			Sort<FObjectExport,FObjectExportSortHelper>( &Linker->ExportMap(SortStartPosition), Linker->ExportMap.Num() - SortStartPosition );
			Sorter = NULL;
		}
	}
};
FObjectExportSortHelper* FObjectExportSortHelper::Sorter = NULL;


class FExportReferenceSorter : public FArchive
{
	/**
	 * Verifies that all objects which will be force-loaded when the export at RelativeIndex is created and/or loaded appear in the sorted list of exports
	 * earlier than the export at RelativeIndex.
	 *
	 * Used for tracking down the culprit behind dependency sorting bugs.
	 *
	 * @param	RelativeIndex	the index into the sorted export list to check dependencies for
	 * @param	CheckObject		the object that will be force-loaded by the export at RelativeIndex
	 * @param	ReferenceType	the relationship between the object at RelativeIndex and CheckObject (archetype, class, etc.)
	 * @param	out_ErrorString	if incorrect sorting is detected, receives data containing more information about the incorrectly sorted object.
	 *
	 * @param	TRUE if the export at RelativeIndex appears later than the exports associated with any objects that it will force-load; FALSE otherwise.
	 */
	UBOOL VerifyDependency( const INT RelativeIndex, UObject* CheckObject, const FString& ReferenceType, FString& out_ErrorString )
	{
		UBOOL bResult = FALSE;

		checkf(ReferencedObjects.IsValidIndex(RelativeIndex), TEXT("Invalid index specified: %i (of %i)"), RelativeIndex, ReferencedObjects.Num());

		UObject* SourceObject = ReferencedObjects(RelativeIndex);
		checkf(SourceObject, TEXT("NULL Object at location %i in ReferencedObjects list"), RelativeIndex);
		checkf(CheckObject, TEXT("CheckObject is NULL for %s (%s)"), *SourceObject->GetFullName(), *ReferenceType);

		if ( SourceObject->GetOutermost() != CheckObject->GetOutermost() )
		{
			// not in the same package; therefore we can assume that the dependent object will exist
			bResult = TRUE;
		}
		else
		{
			INT OtherIndex = ReferencedObjects.FindItemIndex(CheckObject);
			if ( OtherIndex != INDEX_NONE )
			{
				if ( OtherIndex < RelativeIndex )
				{
					bResult = TRUE;
				}
				else
				{
					out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s appears later in ReferencedObjects list)!  %i) %s   =>  %i) %s"), *ReferenceType, RelativeIndex,
						*SourceObject->GetFullName(), OtherIndex, *CheckObject->GetFullName());

					bResult = FALSE;
				}
			}
			else
			{
				// the object isn't in the list of ReferencedObjects, which means it wasn't processed as a result of processing the source object; this
				// might indicate a bug, but might also just mean that the CheckObject was first referenced by an earlier export
				INT* pOtherIndex = ProcessedObjects.Find(CheckObject);
				if ( pOtherIndex )
				{
					OtherIndex = *pOtherIndex;
					INT* pSourceIndex = ProcessedObjects.Find(SourceObject);
					check(pSourceIndex);

					if ( OtherIndex < *pSourceIndex )
					{
						bResult = TRUE;
					}
					else
					{
						out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s was processed but not added to ReferencedObjects list)!  %i/%i) %s   =>  %i) %s"),
							*ReferenceType, RelativeIndex, *pSourceIndex, *SourceObject->GetFullName(), OtherIndex, *CheckObject->GetFullName());
						bResult = FALSE;
					}
				}
				else
				{
					INT* pSourceIndex = ProcessedObjects.Find(SourceObject);
					check(pSourceIndex);

					out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s has not yet been processed)!  %i/%i) %s   =>  %s"),
						*ReferenceType, RelativeIndex, *pSourceIndex, *SourceObject->GetFullName(), *CheckObject->GetFullName());

					bResult = FALSE;
				}
			}
		}

		return bResult;
	}

	/**
	 * Pre-initializes the list of processed objects with the boot-strap classes.
	 */
	void InitializeCoreClasses()
	{
		// initialize the tracking maps with the core classes
		UClass* CoreClassList[]=
		{
			UObject::StaticClass(),				UField::StaticClass(),				UStruct::StaticClass(),
			UScriptStruct::StaticClass(),		UFunction::StaticClass(),			UState::StaticClass(),
			UEnum::StaticClass(),				UClass::StaticClass(),				UConst::StaticClass(),
			UProperty::StaticClass(),			UByteProperty::StaticClass(),		UIntProperty::StaticClass(),
			UBoolProperty::StaticClass(),		UFloatProperty::StaticClass(),		UObjectProperty::StaticClass(),
			UComponentProperty::StaticClass(),	UClassProperty::StaticClass(),		UInterfaceProperty::StaticClass(),
			UNameProperty::StaticClass(),		UStrProperty::StaticClass(),		UArrayProperty::StaticClass(),
			UMapProperty::StaticClass(),		UStructProperty::StaticClass(),		UDelegateProperty::StaticClass(),
			UComponent::StaticClass(),			UInterface::StaticClass(),			USubsystem::StaticClass(),
		};

		for ( INT CoreClassIndex = 0; CoreClassIndex < ARRAY_COUNT(CoreClassList); CoreClassIndex++ )
		{
			UClass* CoreClass = CoreClassList[CoreClassIndex];
			CoreClasses.AddUniqueItem(CoreClass);

			ReferencedObjects.AddItem(CoreClass);
			ReferencedObjects.AddItem(CoreClass->GetDefaultObject());
		}

		for ( INT CoreClassIndex = 0; CoreClassIndex < CoreClasses.Num(); CoreClassIndex++ )
		{
			UClass* CoreClass = CoreClasses(CoreClassIndex);
			ProcessStruct(CoreClass);
		}

		CoreReferencesOffset = ReferencedObjects.Num();
	}

	/**
	 * Adds an object to the list of referenced objects, ensuring that the object is not added more than one.
	 *
	 * @param	Object			the object to add to the list
	 * @param	InsertIndex		the index to insert the object into the export list
	 */
	void AddReferencedObject( UObject* Object, INT InsertIndex )
	{
		if ( Object != NULL && !ReferencedObjects.ContainsItem(Object) )
		{
			ReferencedObjects.InsertItem(Object, InsertIndex);
		}
	}

	/**
	 * Handles serializing and calculating the correct insertion point for an object that will be force-loaded by another object (via an explicit call to Preload).
	 * If the RequiredObject is a UStruct or TRUE is specified for bProcessObject, the RequiredObject will be inserted into the list of exports just before the object
	 * that has a dependency on this RequiredObject.
	 *
	 * @param	RequiredObject		the object which must be created and loaded first
	 * @param	bProcessObject		normally, only the class and archetype for non-UStruct objects are inserted into the list;  specify TRUE to override this behavior
	 *								if RequiredObject is going to be force-loaded, rather than just created
	 */
	void HandleDependency( UObject* RequiredObject, UBOOL bProcessObject=FALSE )
	{
#ifdef _DEBUG
		// while debugging, makes it much easier to see the object's name in the watch window if all object parameters have the same name
		UObject* Object = RequiredObject;
#endif
		if ( RequiredObject != NULL )
		{
			check(CurrentInsertIndex!=INDEX_NONE);

			const INT PreviousReferencedObjectCount = ReferencedObjects.Num();
			const INT PreviousInsertIndex = CurrentInsertIndex;

			if ( RequiredObject->IsA(UStruct::StaticClass()) )
			{
				// if this is a struct/class/function/state, it may have a super that needs to be processed first
				ProcessStruct((UStruct*)RequiredObject);
			}
			else
			{
				if ( bProcessObject )
				{
					// this means that RequiredObject is being force-loaded by the referencing object, rather than simply referenced
					ProcessObject(RequiredObject);
				}
				else
				{
					// only the object's class and archetype are force-loaded, so only those objects need to be in the list before
					// whatever object was referencing RequiredObject
					if ( ProcessedObjects.Find(RequiredObject->GetOuter()) == NULL )
					{
						HandleDependency(RequiredObject->GetOuter());
					}

					// class is needed before archetype, but we need to process these in reverse order because we are inserting into the list.
					ProcessObject(RequiredObject->GetArchetype());
					ProcessStruct(RequiredObject->GetClass());
				}
			}

			// InsertIndexOffset is the amount the CurrentInsertIndex was incremented during the serialization of SuperField; we need to
			// subtract out this number to get the correct location of the new insert index
			const INT InsertIndexOffset = CurrentInsertIndex - PreviousInsertIndex;
			const INT InsertIndexAdvanceCount = (ReferencedObjects.Num() - PreviousReferencedObjectCount) - InsertIndexOffset;
			if ( InsertIndexAdvanceCount > 0 )
			{
				// if serializing SuperField added objects to the list of ReferencedObjects, advance the insertion point so that
				// subsequence objects are placed into the list after the SuperField and its dependencies.
				CurrentInsertIndex += InsertIndexAdvanceCount;
			}
		}
	}

public:
	/**
	 * Constructor
	 */
	FExportReferenceSorter()
	: FArchive(), CurrentInsertIndex(INDEX_NONE)
	{
		ArIsObjectReferenceCollector = TRUE;
		ArIsPersistent = TRUE;
		ArIsSaving = TRUE;

		InitializeCoreClasses();
	}

	/**
	 * Verifies that the sorting algorithm is working correctly by checking all objects in the ReferencedObjects array to make sure that their
	 * required objects appear in the list first
	 */
	void VerifySortingAlgorithm()
	{
		FString ErrorString;
		for ( INT VerifyIndex = CoreReferencesOffset; VerifyIndex < ReferencedObjects.Num(); VerifyIndex++ )
		{
			UObject* Object = ReferencedObjects(VerifyIndex);
			
			// first, make sure that the object's class and archetype appear earlier in the list
			UClass* ObjectClass = Object->GetClass();
			if ( !VerifyDependency(VerifyIndex, ObjectClass, TEXT("Class"), ErrorString) )
			{
				debugf(*ErrorString);
			}

			UObject* ObjectArchetype = Object->GetArchetype();
			if ( ObjectArchetype != NULL && !VerifyDependency(VerifyIndex, ObjectArchetype, TEXT("Archetype"), ErrorString) )
			{
				debugf(*ErrorString);
			}

			// if this is a UComponent, it will force-load its TemplateOwnerClass and SourceDefaultObject when it is loaded
			UComponent* Component = Cast<UComponent>(Object);
			if ( Component != NULL )
			{
				if ( Component->TemplateOwnerClass != NULL )
				{
					if ( !VerifyDependency(VerifyIndex, Component->TemplateOwnerClass, TEXT("TemplateOwnerClass"), ErrorString) )
					{
						debugf(*ErrorString);
					}
				}
				else if ( !Component->HasAnyFlags(RF_ClassDefaultObject) && Component->TemplateName == NAME_None && Component->IsTemplate() )
				{
					UObject* SourceDefaultObject = Component->ResolveSourceDefaultObject();
					if ( SourceDefaultObject != NULL )
					{
						if ( !VerifyDependency(VerifyIndex, SourceDefaultObject, TEXT("SourceDefaultObject"), ErrorString) )
						{
							debugf(*ErrorString);
						}
					}
				}
			}
			else
			{
				// UObjectRedirectors are always force-loaded as the loading code needs immediate access to the object pointed to by the Redirector
				UObjectRedirector* Redirector = Cast<UObjectRedirector>(Object);
				if ( Redirector != NULL && Redirector->DestinationObject != NULL )
				{
					// the Redirector does not force-load the destination object, so we only need its class and archetype.
					UClass* RedirectorDestinationClass = Redirector->DestinationObject->GetClass();
					if ( !VerifyDependency(VerifyIndex, RedirectorDestinationClass, TEXT("Redirector DestinationObject Class"), ErrorString) )
					{
						debugf(*ErrorString);
					}

					UObject* RedirectorDestinationArchetype = Redirector->DestinationObject->GetArchetype();
					if ( RedirectorDestinationArchetype != NULL 
					&& !VerifyDependency(VerifyIndex, RedirectorDestinationArchetype, TEXT("Redirector DestinationObject Archetype"), ErrorString) )
					{
						debugf(*ErrorString);
					}
				}
			}
		}
	}

	/**
	 * Clears the list of encountered objects; should be called if you want to re-use this archive.
	 */
	void Clear()
	{
		ReferencedObjects.Remove(CoreReferencesOffset, ReferencedObjects.Num() - CoreReferencesOffset);
	}

	/**
	 * Get the list of new objects which were encountered by this archive; excludes those objects which were passed into the constructor
	 */
	void GetExportList( TArray<UObject*>& out_Exports, UBOOL bIncludeCoreClasses=FALSE )
	{
		if ( !bIncludeCoreClasses )
		{
			const INT NumReferencedObjects = ReferencedObjects.Num() - CoreReferencesOffset;
			if ( NumReferencedObjects > 0 )
			{
				INT OutputIndex = out_Exports.Num();

				out_Exports.Add(NumReferencedObjects);
				for ( INT RefIndex = CoreReferencesOffset; RefIndex < ReferencedObjects.Num(); RefIndex++ )
				{
					out_Exports(OutputIndex++) = ReferencedObjects(RefIndex);
				}
			}
		}
		else
		{
			out_Exports += ReferencedObjects;
		}
	}

	/** 
	 * UObject serialization operator
	 *
	 * @param	Object	an object encountered during serialization of another object
	 *
	 * @return	reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object )
	{
		// we manually handle class default objects, so ignore those here
		if ( Object != NULL && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			if ( ProcessedObjects.Find(Object) == NULL )
			{
				// if this object is not a UField, it is an object instance that is referenced through script or defaults (when processing classes) or
				// through an normal object reference (when processing the non-class exports).  Since classes and class default objects
				// are force-loaded (and thus, any objects referenced by the class's script or defaults will be created when the class
				// is force-loaded), we'll need to be sure that the referenced object's class and archetype are inserted into the list
				// of exports before the class, so that when CreateExport is called for this object reference we don't have to seek.
				// Note that in the non-UField case, we don't actually need the object itself to appear before the referencing object/class because it won't
				// be force-loaded (thus we don't need to add the referenced object to the ReferencedObject list)
				if ( Object->IsA(UField::StaticClass()) )
				{
					// when field processing is enabled, ignore any referenced classes since a class's class and CDO are both intrinsic and
					// attempting to deal with them here will only cause problems
					if ( !bIgnoreFieldReferences && !Object->IsA(UClass::StaticClass()) )
					{
						if ( CurrentClass == NULL || Object->GetOuter() != CurrentClass )
						{
							if ( Object->IsA(UStruct::StaticClass()) )
							{
								// if this is a struct/class/function/state, it may have a super that needs to be processed first (Preload force-loads UStruct::SuperField)
								ProcessStruct((UStruct*)Object);
							}
							else
							{
								// a normal field - property, enum, const; just insert it into the list and keep going
								ProcessedObjects.AddItem(Object);
								
								AddReferencedObject(Object, CurrentInsertIndex);
								if ( SerializedObjects.Find(Object) == NULL )
								{
									SerializedObjects.AddItem(Object);
									Object->Serialize(*this);
								}
							}
						}
					}
				}
				else
				{
					// since normal references to objects aren't force-loaded, we do not need to pass TRUE for bProcessObject
					// (which would indicate that Object must be inserted into the sorted export list before the object that contains
					// this object reference - i.e. the object we're currently serializing)
					HandleDependency(Object);
				}
			}
		}

		return *this;
	}

	/**
	 * Adds a normal object to the list of sorted exports.  Ensures that any objects which will be force-loaded when this object is created or loaded are inserted into
	 * the list before this object.
	 *
	 * @param	Object	the object to process.
	 */
	void ProcessObject( UObject* Object )
	{
		// we manually handle class default objects, so ignore those here
		if ( Object != NULL )
		{
			if ( !Object->HasAnyFlags(RF_ClassDefaultObject) )
			{
				if ( ProcessedObjects.Find(Object) == NULL )
				{
					ProcessedObjects.AddItem(Object);

					const UBOOL bRecursiveCall = CurrentInsertIndex != INDEX_NONE;
					if ( !bRecursiveCall )
					{
						CurrentInsertIndex = ReferencedObjects.Num();
					}

					// when an object is created (CreateExport), its class and archetype will be force-loaded, so we'll need to make sure that those objects
					// are placed into the list before this object so that when CreateExport calls Preload on these objects, no seeks occur
					// The object's Outer isn't force-loaded, but it will be created before the current object, so we'll need to ensure that its archetype & class
					// are placed into the list before this object.
					HandleDependency(Object->GetClass(), TRUE);
					HandleDependency(Object->GetOuter());
					HandleDependency(Object->GetArchetype(), TRUE);

					// if this is a UComponent, it will force-load its TemplateOwnerClass and SourceDefaultObject when it is loaded
					UComponent* Component = Cast<UComponent>(Object);
					if ( Component != NULL )
					{
						if ( Component->TemplateOwnerClass != NULL )
						{
							HandleDependency(Component->TemplateOwnerClass);
						}
						else if ( !Component->HasAnyFlags(RF_ClassDefaultObject) && Component->TemplateName == NAME_None && Component->IsTemplate() )
						{
							UObject* SourceDefaultObject = Component->ResolveSourceDefaultObject();
							if ( SourceDefaultObject != NULL )
							{
								HandleDependency(SourceDefaultObject, TRUE);
							}
						}
					}
					else
					{
						// UObjectRedirectors are always force-loaded as the loading code needs immediate access to the object pointed to by the Redirector
						UObjectRedirector* Redirector = Cast<UObjectRedirector>(Object);
						if ( Redirector != NULL && Redirector->DestinationObject != NULL )
						{
							// the Redirector does not force-load the destination object, so we only need its class and archetype.
							HandleDependency(Redirector->DestinationObject);
						}
					}

					// now we add this object to the list
					AddReferencedObject(Object, CurrentInsertIndex);

					// then serialize the object - any required references encountered during serialization will be inserted into the list before this object, but after this object's
					// class and archetype
					if ( SerializedObjects.Find(Object) == NULL )
					{
						SerializedObjects.AddItem(Object);
						Object->Serialize(*this);
					}

					if ( !bRecursiveCall )
					{
						CurrentInsertIndex = INDEX_NONE;
					}
				}
			}
		}
	}

	/**
	 * Adds a UStruct object to the list of sorted exports.  Handles serialization and insertion for any objects that will be force-loaded by this struct (via an explicit call to Preload).
	 *
	 * @param	StructObject	the struct to process
	 */
	void ProcessStruct( UStruct* StructObject )
	{
		if ( StructObject != NULL )
		{
			if ( ProcessedObjects.Find(StructObject) == NULL )
			{
				ProcessedObjects.AddItem(StructObject);

				const UBOOL bRecursiveCall = CurrentInsertIndex != INDEX_NONE;
				if ( !bRecursiveCall )
				{
					CurrentInsertIndex = ReferencedObjects.Num();
				}

				// this must be done after we've established a CurrentInsertIndex
				HandleDependency(StructObject->GetInheritanceSuper());

				// insert the class/function/state/struct into the list
				AddReferencedObject(StructObject, CurrentInsertIndex);
				if ( SerializedObjects.Find(StructObject) == NULL )
				{
					const UBOOL bPreviousIgnoreFieldReferences = bIgnoreFieldReferences;

					// first thing to do is collect all actual objects referenced by this struct's script or defaults
					// so we turn off field serialization so that we don't have to worry about handling this struct's fields just yet
					bIgnoreFieldReferences = TRUE;

					SerializedObjects.AddItem(StructObject);
					StructObject->Serialize(*this);

					// at this point, any objects which were referenced through this struct's script or defaults will be in the list of exports, and 
					// the CurrentInsertIndex will have been advanced so that the object processed will be inserted just before this struct in the array
					// (i.e. just after class/archetypes for any objects which were referenced by this struct's script)

					// now re-enable field serialization and process the struct's properties, functions, enums, structs, etc.  They will be inserted into the list
					// just ahead of the struct itself, so that those objects are created first during seek-free loading.
					bIgnoreFieldReferences = FALSE;

					// invoke the serialize operator rather than calling Serialize directly so that the object is handled correctly (i.e. if it is a struct, then we should
					// call ProcessStruct, etc. and all this logic is already contained in the serialization operator)
					if ( StructObject->GetClass() != UClass::StaticClass() )
					{
						// before processing the Children reference, set the CurrentClass to the class which contains this StructObject so that we
						// don't inadvertently serialize other fields of the owning class too early.
						CurrentClass = StructObject->GetOwnerClass();
					}
					(*this) << (UObject*&)StructObject->Children;
					CurrentClass = NULL;

					(*this) << (UObject*&)StructObject->Next;

					bIgnoreFieldReferences = bPreviousIgnoreFieldReferences;
				}

				// Preload will force-load the class default object when called on a UClass object, so make sure that the CDO is always immediately after its class
				// in the export list; we can't resolve this circular reference, but hopefully we the CDO will fit into the same memory block as the class during 
				// seek-free loading.
				UClass* ClassObject = Cast<UClass>(StructObject);
				if ( ClassObject != NULL )
				{
					UObject* CDO = ClassObject->GetDefaultObject();
					if ( ProcessedObjects.Find(CDO) == NULL )
					{
						ProcessedObjects.AddItem(CDO);

						if ( SerializedObjects.Find(CDO) == NULL )
						{
							SerializedObjects.AddItem(CDO);
							CDO->Serialize(*this);
						}

						INT ClassIndex = ReferencedObjects.FindItemIndex(ClassObject);
						check(ClassIndex != INDEX_NONE);

						// we should be the only one adding CDO's to the list, so this assertion is to catch cases where someone else
						// has added the CDO to the list (as it will probably be in the wrong spot).
						check(!ReferencedObjects.ContainsItem(CDO) || CoreClasses.ContainsItem(ClassObject));
						AddReferencedObject(CDO, ClassIndex + 1);
					}
				}

				if ( !bRecursiveCall )
				{
					CurrentInsertIndex = INDEX_NONE;
				}
			}
		}
	}

private:

	/**
	 * The index into the ReferencedObjects array to insert new objects
	 */
	INT CurrentInsertIndex;

	/**
	 * The index into the ReferencedObjects array for the first object not referenced by one of the core classes
	 */
	INT CoreReferencesOffset;

	/**
	 * The classes which are pre-added to the array of ReferencedObjects.  Used for resolving a number of circular dependecy issues between
	 * the boot-strap classes.
	 */
	TArray<UClass*> CoreClasses;

	/**
	 * The list of objects that have been evaluated by this archive so far.
	 */
	TLookupMap<UObject*> ProcessedObjects;

	/**
	 * The list of objects that have been serialized; used to prevent calling Serialize on an object more than once.
	 */
	TLookupMap<UObject*> SerializedObjects;

	/**
	 * The list of new objects that were encountered by this archive
	 */
	TArray<UObject*> ReferencedObjects;

	/**
	 * Controls whether to process UField objects encountered during serialization of an object.
	 */
	UBOOL bIgnoreFieldReferences;

	/**
	 * The UClass currently being processed.  This is used to prevent serialization of a UStruct's Children member causing other fields of the same class to be processed too early due
	 * to being referenced (directly or indirectly) by that field.  For example, if a class has two functions which both have a struct parameter of a struct type which is declared in the same class,
	 * the struct would be inserted into the list immediately before the first function processed.  The second function would be inserted into the list just before the struct.  At runtime,
	 * the "second" function would be created first, which would end up force-loading the struct.  This would cause an unacceptible seek because the struct appears later in the export list, thus
	 * hasn't been created yet.
	 */
	UClass* CurrentClass;
};

#define EXPORT_SORTING_DETAILED_LOGGING 0

/**
 * Helper structure encapsulating functionality to sort a linker's export map to allow seek free
 * loading by creating the exports in the order they are in the export map.
 */
struct FObjectExportSeekFreeSorter
{
	/**
	 * Sorts exports in passed in linker in order to avoid seeking when creating them in order and also
	 * conform the order to an already existing linker if non- NULL.
	 *
	 * @param	Linker				LinkerSave to sort export map
	 * @param	LinkerToConformTo	LinkerLoad to conform LinkerSave to if non- NULL
	 */
	void SortExports( ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo )
	{
		INT					FirstSortIndex = LinkerToConformTo ? LinkerToConformTo->ExportMap.Num() : 0;
		TMap<UObject*,INT>	OriginalExportIndexes;

		// Populate object to current index map.
		for( INT ExportIndex=FirstSortIndex; ExportIndex<Linker->ExportMap.Num(); ExportIndex++ )
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if( Export._Object )
			{
				// Set the index (key) in the map to the index of this object into the export map.
				OriginalExportIndexes.Set( Export._Object, ExportIndex );
			}
		}

		UBOOL bRetrieveInitialReferences = TRUE;

		// Now we need to sort the export list according to the order in which objects will be loaded.  For the sake of simplicity, 
		// process all classes first so they appear in the list first (along with any objects those classes will force-load) 
		for( INT ExportIndex=FirstSortIndex; ExportIndex<Linker->ExportMap.Num(); ExportIndex++ )
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if( Export._Object && Export._Object->IsA(UClass::StaticClass()) )
			{
				SortArchive.Clear();
				SortArchive.ProcessStruct((UClass*)Export._Object);
#if EXPORT_SORTING_DETAILED_LOGGING
				TArray<UObject*> ReferencedObjects;
				SortArchive.GetExportList(ReferencedObjects, bRetrieveInitialReferences);

				debugf(TEXT("Referenced objects for (%i) %s in %s"), ExportIndex, *Export._Object->GetFullName(), *Linker->LinkerRoot->GetName());
				for ( INT RefIndex = 0; RefIndex < ReferencedObjects.Num(); RefIndex++ )
				{
					debugf(TEXT("\t%i) %s"), RefIndex, *ReferencedObjects(RefIndex)->GetFullName());
				}
				if ( ReferencedObjects.Num() > 1 )
				{
					// insert a blank line to make the output more readable
					debugf(TEXT(""));
				}

				SortedExports += ReferencedObjects;
#else
				SortArchive.GetExportList(SortedExports, bRetrieveInitialReferences);
#endif
				bRetrieveInitialReferences = FALSE;
			}

		}

#if EXPORT_SORTING_DETAILED_LOGGING
		debugf(TEXT("*************   Processed %i classes out of %i possible exports for package %s.  Beginning second pass...   *************"), SortedExports.Num(), Linker->ExportMap.Num() - FirstSortIndex, *Linker->LinkerRoot->GetName());
#endif

		// All UClasses, CDOs, functions, properties, etc. are now in the list - process the remaining objects now
		for ( INT ExportIndex = FirstSortIndex; ExportIndex < Linker->ExportMap.Num(); ExportIndex++ )
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if ( Export._Object )
			{
				SortArchive.Clear();
				SortArchive.ProcessObject(Export._Object);
#if EXPORT_SORTING_DETAILED_LOGGING
				TArray<UObject*> ReferencedObjects;
				SortArchive.GetExportList(ReferencedObjects, bRetrieveInitialReferences);

				debugf(TEXT("Referenced objects for (%i) %s in %s"), ExportIndex, *Export._Object->GetFullName(), *Linker->LinkerRoot->GetName());
				for ( INT RefIndex = 0; RefIndex < ReferencedObjects.Num(); RefIndex++ )
				{
					debugf(TEXT("\t%i) %s"), RefIndex, *ReferencedObjects(RefIndex)->GetFullName());
				}
				if ( ReferencedObjects.Num() > 1 )
				{
					// insert a blank line to make the output more readable
					debugf(TEXT(""));
				}

				SortedExports += ReferencedObjects;
#else
				SortArchive.GetExportList(SortedExports, bRetrieveInitialReferences);
#endif
				bRetrieveInitialReferences = FALSE;
			}
		}
#if EXPORT_SORTING_DETAILED_LOGGING
		SortArchive.VerifySortingAlgorithm();
#endif
		// Back up existing export map and empty it so we can repopulate it in a sorted fashion.
		TArray<FObjectExport> OldExportMap = Linker->ExportMap;
		Linker->ExportMap.Empty( OldExportMap.Num() );

		// Add exports that can't be re-jiggled as they are part of the exports of the to be
		// conformed to Linker.
		for( INT ExportIndex=0; ExportIndex<FirstSortIndex; ExportIndex++ )
		{
			Linker->ExportMap.AddItem( OldExportMap(ExportIndex) );
		}

		// Create new export map from sorted exports.
		for( INT ObjectIndex=0; ObjectIndex<SortedExports.Num(); ObjectIndex++ )
		{
			// See whether this object was part of the to be sortable exports map...
			UObject* Object		= SortedExports(ObjectIndex);
			INT* ExportIndexPtr	= OriginalExportIndexes.Find( Object );
			if( ExportIndexPtr )
			{
				// And add it if it has been.
				Linker->ExportMap.AddItem( OldExportMap(*ExportIndexPtr) );
			}
		}

		// Manually add any new NULL exports last as they won't be in the SortedExportsObjects list. 
		// A NULL Export._Object can occur if you are e.g. saving an object in the game that is 
		// RF_NotForClient.
		for( INT ExportIndex=FirstSortIndex; ExportIndex<OldExportMap.Num(); ExportIndex++ )
		{
			const FObjectExport& Export = OldExportMap(ExportIndex);
			if( Export._Object == NULL )
			{
				Linker->ExportMap.AddItem( Export );
			}
		}
	}

private:
	/**
	 * Archive for sorting an objects references according to the order in which they'd be loaded.
	 */
 	FExportReferenceSorter SortArchive;

	/** Array of regular objects encountered by CollectExportsInOrderOfUse					*/
	TArray<UObject*>	SortedExports;
};

// helper class for clarification, encapsulation, and elimination of duplicate code
struct FPackageExportTagger
{
	UObject*		Base;
	EObjectFlags	TopLevelFlags;
	UObject*		Outer;

	FPackageExportTagger( UObject* CurrentBase, EObjectFlags CurrentFlags, UObject* InOuter )
	:	Base(CurrentBase)
	,	TopLevelFlags(CurrentFlags)
	,	Outer(InOuter)
	{}

	void TagPackageExports( FArchiveSaveTagExports& ExportTagger, UBOOL bRoutePresave )
	{
		// Route PreSave on Base and serialize it for export tagging.
		if( Base )
		{
			if ( bRoutePresave )
			{
				Base->PreSave();
			}
			ExportTagger << Base;
		}

		// Serialize objects to tag them as RF_TagExp.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Obj = *It;
			if( Obj->HasAnyFlags(TopLevelFlags) && Obj->IsIn(Outer) )
			{
				ExportTagger << Obj;
			}
		}

		if ( bRoutePresave )
		{
			// Route PreSave.
			for( FObjectIterator It; It; ++It )
			{
				UObject* Obj = *It;
				if( Obj->HasAnyFlags(RF_TagExp) )
				{
					//@warning: Objects created from within PreSave will NOT have PreSave called on them!!!
					Obj->PreSave();
				}
			}
		}
	}
};

/**
 * Save one specific object (along with any objects it references contained within the same Outer) into an Unreal package.
 * 
 * @param	InOuter							the outer to use for the new package
 * @param	Base							the object that should be saved into the package
 * @param	TopLevelFlags					For all objects which are not referenced [either directly, or indirectly] through Base, only objects
 *											that contain any of these flags will be saved.  If 0 is specified, only objects which are referenced
 *											by Base will be saved into the package.
 * @param	Filename						the name to use for the new package file
 * @param	Error							error output
 * @param	Conform							if non-NULL, all index tables for this will be sorted to match the order of the corresponding index table
 *											in the conform package
 * @param	bForceByteSwapping				whether we should forcefully byte swap before writing to disk
 * @param	bWarnOfLongFilename				[opt] If TRUE (the default), warn when saving to a long filename.
 * @param	SaveFlags						Flags to control saving
 *
 * @return	TRUE if the package was saved successfully.
 */
UBOOL UObject::SavePackage( UPackage* InOuter, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error, ULinkerLoad* Conform, UBOOL bForceByteSwapping, UBOOL bWarnOfLongFilename, DWORD SaveFlags )
{
	// We need to fulfill all pending streaming and async loading requests to then allow us to lock the global IO manager. 
	// The latter implies flushing all file handles which is a pre-requisite of saving a package. The code basically needs 
	// to be sure that we are not reading from a file that is about to be overwritten and that there is no way we might 
	// start reading from the file till we are done overwriting it.
	UObject::FlushAsyncLoading();
	(*GFlushStreamingFunc)();
	FIOManagerScopedLock ScopedLock(GIOManager);

	check(InOuter);
	check(Filename);
	DWORD Time=0; clock(Time);

	// Ensure that we are not saving a package that has been originally saved with a newer engine version than the current.
	if( InOuter->PackageFlags & PKG_SavedWithNewerVersion )
	{
		if (!(SaveFlags & SAVE_NoError))
		{
			// We cannot save packages that were saved with an engine version that is newer than the current one.
			Error->Logf( NAME_Warning, *LocalizeError(TEXT("CannotSaveWithOlderVersion"),TEXT("Core")), Filename );
		}
		return FALSE;
	}

	// Make sure package is fully loaded before saving. The exceptions being script compilation, which potentially replaces
	// an existing script file without loading it and passing in a Base. We don't care about the package being fully loaded
	// in this case as everything that is going to be saved is already be loaded as it is referenced by the base.
	if( !Base && !InOuter->IsFullyLoaded() && !GIsUCCMake )
	{
		if (!(SaveFlags & SAVE_NoError))
		{
			// We cannot save packages that aren't fully loaded as it would clobber existing not loaded content.
			Error->Logf( NAME_Warning, *LocalizeError(TEXT("CannotSavePartiallyLoaded"),TEXT("Core")), Filename );
		}
		return FALSE;
	}

	// make sure that we're not attempting to conform a cooked package
	checkf(Conform==NULL||(InOuter->PackageFlags&PKG_Cooked)==0, TEXT("Illegal to conform cooked packages (%s)"), *InOuter->GetName());

	// Route PreSaveRoot to allow e.g. the world to attach components for the persistent level.
	UBOOL bCleanupIsRequired = FALSE;
	if( Base )
	{
		bCleanupIsRequired = Base->PreSaveRoot(Filename);
	}

	// Make temp file.
	TCHAR TempFilename[256];
	appStrncpy( TempFilename, Filename, ARRAY_COUNT(TempFilename) );
	INT c = appStrlen(TempFilename);
	while( c>0 && TempFilename[c-1]!=PATH_SEPARATOR[0] && TempFilename[c-1]!='/' && TempFilename[c-1]!=':' )
	{
		c--;
	}
	TempFilename[c]=0;
	appStrcat( TempFilename, TEXT("Save.tmp") );

	// Init.
	GWarn->StatusUpdatef( 0, 0, *LocalizeProgress(TEXT("Saving"),TEXT("Core")), Filename );
	UBOOL Success = 0;

	// If we have a loader for the package, unload it to prevent conflicts if we are resaving to the same filename
	for( INT i=0; i<GObjLoaders.Num(); i++ )
	{
		ULinkerLoad* Loader = GetLoader(i);

		// This is the loader corresponding to the package we're saving.
		if( Loader->LinkerRoot == InOuter )
		{
			// Compare absolute filenames to see whether we're trying to save over an existing file.
			if( appConvertRelativePathToFull( Filename ) == appConvertRelativePathToFull( Loader->Filename ) )
			{
				// Detach all exports from the linker and dissociate the linker.
				ResetLoaders( InOuter );
			}
		}
	}

	// Untag all objects and names.
	for( FObjectIterator It; It; ++It )
	{
		// Clear flags from previous SavePackage calls.
		QWORD ClearFlags = RF_TagImp | RF_TagExp | RF_Saved;
		// Clear context flags for objects that are going to be saved into package.
		if ( It->IsIn(InOuter) || It->HasAnyFlags( RF_ForceTagExp ) )
		{
			ClearFlags |= RF_LoadContextFlags;
		}
		It->ClearFlags( ClearFlags );

		// if the object class has been marked as deprecated, mark this
		// object as transient so that it isn't serialized
		if ( It->GetClass()->HasAnyClassFlags(CLASS_Deprecated) )
		{
			It->SetFlags(RF_Transient);
			if ( !It->HasAnyFlags(RF_ClassDefaultObject) && It->GetClass()->HasAnyClassFlags(CLASS_HasComponents) )
			{
				TArray<UComponent*> ComponentReferences;
				TArchiveObjectReferenceCollector<UComponent> ComponentCollector(&ComponentReferences, *It, FALSE, TRUE, TRUE);
				It->Serialize(ComponentCollector);

				for ( INT Index = 0; Index < ComponentReferences.Num(); Index++ )
				{
					ComponentReferences(Index)->SetFlags(RF_Transient);
				}
			}
		}

		if ( It->HasAnyFlags(RF_ClassDefaultObject) )
		{
			// if this is the class default object, make sure it's not
			// marked transient for any reason, as we need it to be saved
			// to disk
			It->ClearFlags(RF_Transient);
		}
	}

	for( INT i=0; i<FName::GetMaxNames(); i++ )
	{
		if( FName::GetEntry(i) )
		{
			FName::GetEntry(i)->Flags &= ~(RF_TagImp | RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
		}
	}

	DWORD ComparisonFlags = 0;
	if ( (InOuter->PackageFlags&PKG_ContainsScript) == 0 || !GIsUCCMake )
	{
		ComparisonFlags |= PPF_DeepCompareInstances;
	}

	// Export objects (tags them as RF_TagExp).
	FArchiveSaveTagExports ExportTaggerArchive( InOuter );
	ExportTaggerArchive.SetPortFlags( ComparisonFlags );
	
	// Tag exports and route presave.
	FPackageExportTagger PackageExportTagger( Base, TopLevelFlags, InOuter );
	PackageExportTagger.TagPackageExports( ExportTaggerArchive, TRUE );

	// Clear RF_TagExp again as we need to redo tagging below.
	for( FObjectIterator It; It; ++It )
	{
		UObject* Obj = *It;
		Obj->ClearFlags( RF_TagExp );
	}

	// We need to serialize objects yet again to tag objects that were created by PreSave as RF_TagExp.
	PackageExportTagger.TagPackageExports( ExportTaggerArchive, FALSE );

	// structure to track what ever export needs to import
	TMap<UObject*, TArray<UObject*> > ObjectDependencies;

	ULinkerSave* Linker = NULL;
#if !EXCEPTIONS_DISABLED
	try
#endif
	{
		// Allocate the linker, forcing byte swapping if wanted.
		Linker = new ULinkerSave( InOuter, TempFilename, bForceByteSwapping );
		Linker->SetPortFlags(ComparisonFlags);

		// Import objects & names.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Obj = *It;
			if( Obj->HasAnyFlags(RF_TagExp) )
			{
				// Build list.
				FArchiveSaveTagImports ImportTagger( Linker, Obj->GetMaskedFlags(RF_LoadContextFlags) );
				ImportTagger.SetPortFlags(ComparisonFlags);

				UClass* Class = Obj->GetClass();

				if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
				{
					Class->SerializeDefaultObject(Obj, ImportTagger);
				}
				else
				{
					Obj->Serialize( ImportTagger );
				}

#if PS3_SNC
				// convert the class to an object to avoid multiple matches to the operator in SNC 
				UObject* ClassObject = Class;
				ImportTagger << ClassObject;
#else
				ImportTagger << Class;
#endif

				UObject* Template = Obj->GetArchetype();
				if ( Template && Template != Class->GetDefaultObject() )
				{
					ImportTagger << Template;
				}

				if( Obj->IsIn(GetTransientPackage()) )
				{
					appErrorf( *LocalizeError(TEXT("TransientImport"),TEXT("Core")), *Obj->GetFullName() );
				}

				// add the list of dependencies to the dependency map
				ObjectDependencies.Set(*It, ImportTagger.Dependencies);
			}
		}

		TArray<UObject*> PrivateObjects;
		TArray<UObject*> ObjectsInOtherMaps;
		TArray<UObject*> LevelObjects;

		// Tag the names for all relevant object, classes, and packages.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Obj = *It;
			if( Obj->HasAnyFlags(RF_TagExp|RF_TagImp) )
			{
				Obj->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
				if( Obj->GetOuter() )
				{
					Obj->GetOuter()->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
				}

				if( Obj->HasAnyFlags(RF_TagImp) )
				{
					Obj->Class->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
					check(Obj->Class->GetOuter());
					Obj->Class->GetOuter()->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
					
					// if a private object was marked by the cooker, it will be in memory on load, and will be found. helps with some objects
					// from a package in a package being moved into Startup_int.xxx, but not all
					// Imagine this:
					// Package P:
					//   - A (private)
					//   - B (public, references A)
					//   - C (public, references A)
					// Map M:
					//   - MObj (references B)
					// Startup Package S:
					//   - SObj (referneces C)
					// When Startup is cooked, it will pull in C and A. When M is cooked, it will pull in B, but not A, because
					// A was already marked by the cooker. M.xxx now has a private import to A, which is normally illegal, hence
					// the RF_MarkedByCooker check below
					if( !Obj->HasAnyFlags(RF_Public) && !Obj->HasAnyFlags(RF_MarkedByCooker))
					{
						PrivateObjects.AddItem(Obj);
					}

					// See whether the object we are referencing is in another map package.
					UPackage* ObjPackage = Cast<UPackage>(Obj->GetOutermost());
					if( ObjPackage && ObjPackage->ContainsMap() )
					{
						if ( ObjPackage != Obj && Obj->GetFName() != NAME_PersistentLevel && Obj->GetFName() != NAME_TheWorld )
						{
							ObjectsInOtherMaps.AddItem(Obj);
							warnf( TEXT( "Obj in another map: %s"), *Obj->GetFullName() ); 
						}
						else
						{
							LevelObjects.AddItem(Obj);
						}
					}

				}
				else
				{
					debugfSlow( NAME_DevSave, TEXT("Saving %s"), *Obj->GetFullName() );
				}
			}
		}

		if ( LevelObjects.Num() > 0 && ObjectsInOtherMaps.Num() == 0 )
		{
			ObjectsInOtherMaps = LevelObjects;
		}

		// The graph is linked to objects in a different map package!
		if( ObjectsInOtherMaps.Num() )
		{
			UObject* MostLikelyCulprit = NULL;
			UProperty* PropertyRef = NULL;

			const FString FindCulpritQuestionString = FString::Printf( *LocalizeError(TEXT("LinkedToObjectsInOtherMap_FindCulpritQ"),TEXT("Core")), Filename );
			const UBOOL bProceed = appMsgf( AMT_YesNo, *FindCulpritQuestionString );
			if ( bProceed )
			{
				FindMostLikelyCulprit( ObjectsInOtherMaps, MostLikelyCulprit, PropertyRef );
			}

			check(MostLikelyCulprit && PropertyRef);
			const FString CulpritString = FString::Printf(TEXT("%s (%s)"), *MostLikelyCulprit->GetFullName(), *PropertyRef->GetName());

			if (!(SaveFlags & SAVE_NoError))
			{
				// the cooker needs to treat this as an error as the cooked package did not get saved
				if( GIsCooking == TRUE )
				{
					warnf( NAME_Error, *LocalizeError(TEXT("FailedSavePrivate"),TEXT("Core")), Filename, *MostLikelyCulprit->GetFullName() );
				}

				appThrowf( *LocalizeError(TEXT("FailedSaveOtherMapPackage"),TEXT("Core")), Filename, *CulpritString );
			}
			else
			{
				return FALSE;
			}
		}

		// The graph is linked to private objects!
		if ( PrivateObjects.Num() )
		{
			UObject* MostLikelyCulprit = NULL;
			UProperty* PropertyRef = NULL;
			const FString FindCulpritQuestionString = FString::Printf( *LocalizeError(TEXT("LinkedToPrivateObjects_FindCulpritQ"),TEXT("Core")), Filename );
			const UBOOL bProceed = appMsgf( AMT_YesNo, *FindCulpritQuestionString );
			if ( bProceed )
			{
				FindMostLikelyCulprit( PrivateObjects, MostLikelyCulprit, PropertyRef );
			}

			if (!(SaveFlags & SAVE_NoError))
			{
				// the cooker needs to treat this as an error as the cooked package did not get saved
				if( GIsCooking == TRUE )
				{
					warnf( NAME_Error, *LocalizeError(TEXT("FailedSavePrivate"),TEXT("Core")), Filename, *MostLikelyCulprit->GetFullName() );
				}
				appThrowf( *LocalizeError(TEXT("FailedSavePrivate"),TEXT("Core")), Filename, *MostLikelyCulprit->GetFullName() );
			}
			else
			{
				return FALSE;
			}

		}

		// Write fixed-length file summary to overwrite later.
		if( Conform )
		{
			// Conform to previous generation of file.
			debugf( TEXT("Conformal save, relative to: %s, Generation %i"), *Conform->Filename, Conform->Summary.Generations.Num()+1 );
			Linker->Summary.Guid        = Conform->Summary.Guid;
			Linker->Summary.Generations = Conform->Summary.Generations;
		}
		else
		{
			// First generation file.
			// if it's a cooked package, use the pre-cooked Guid and Generations so it matches with any copies of this package that are forced exports in other packages
			if ((InOuter->PackageFlags & PKG_Cooked))
			{
				Linker->Summary.Guid = InOuter->GetGuid();
				if (!(InOuter->PackageFlags & PKG_ServerSideOnly))
				{
					const INT GenerationNetObjectCount = InOuter->GetGenerationNetObjectCount().Num();
					if (GenerationNetObjectCount > 0)
					{
						Linker->Summary.Generations.Empty(GenerationNetObjectCount);
						//@note: the last generation is handled at the end
						Linker->Summary.Generations.AddZeroed(GenerationNetObjectCount - 1);
						for (INT i = 0; i < GenerationNetObjectCount - 1; i++)
						{
							Linker->Summary.Generations(i).NetObjectCount = InOuter->GetNetObjectCount(i);
						}
					}
				}
			}
			else
			{
				Linker->Summary.Guid = appCreateGuid();
				Linker->Summary.Generations = TArray<FGenerationInfo>();
			}
		}
		new(Linker->Summary.Generations)FGenerationInfo(0, 0, 0);
		*Linker << Linker->Summary;
		INT OffsetAfterPackageFileSummary = Linker->Tell();
	
		// Build NameMap.
		Linker->Summary.NameOffset = Linker->Tell();
		for( INT i=0; i<FName::GetMaxNames(); i++ )
		{
			if( FName::GetEntry(i) )
			{
				FName Name( (EName)i );
				if( Name.HasAnyFlags(RF_TagExp) )
				{
					Linker->NameMap.AddItem( Name );
				}
			}
		}

		// Sort names.
		FObjectNameSortHelper NameSortHelper;
		NameSortHelper.SortNames( Linker, Conform );

		// Save names.
		Linker->Summary.NameCount = Linker->NameMap.Num();
		for( INT i=0; i<Linker->NameMap.Num(); i++ )
		{
			*Linker << *FName::GetEntry( Linker->NameMap(i).GetIndex() );
			Linker->NameIndices(Linker->NameMap(i).GetIndex()) = i;
		}

		// Build ImportMap.
		for( FObjectIterator It; It; ++It )
		{
			if( It->HasAnyFlags(RF_TagImp) )
			{
				new( Linker->ImportMap )FObjectImport( *It );
			}
		}

		// sort and conform imports
		FObjectImportSortHelper ImportSortHelper;
		ImportSortHelper.SortImports( Linker, Conform );
		Linker->Summary.ImportCount = Linker->ImportMap.Num();

		// Build ExportMap.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Object = *It;
			if( Object->HasAnyFlags(RF_TagExp) )
			{
				new( Linker->ExportMap )FObjectExport( Object );
			}
		}

		// Sort exports alphabetically and conform the export table (if necessary)
		FObjectExportSortHelper ExportSortHelper;
		ExportSortHelper.SortExports( Linker, Conform );
		Linker->Summary.ExportCount = Linker->ExportMap.Num();

		// Sort exports for seek-free loading.
		FObjectExportSeekFreeSorter SeekFreeSorter;
		SeekFreeSorter.SortExports( Linker, Conform );

		// Pre-size depends map.
		Linker->DependsMap.AddZeroed( Linker->ExportMap.Num() );

		// go back over the (now sorted) exports and fill out the DependsMap
		for (INT ExpIndex = 0; ExpIndex < Linker->ExportMap.Num(); ExpIndex++)
		{
			UObject* Object = Linker->ExportMap(ExpIndex)._Object;

			// add a dependency map entry also
			TArray<INT>& DependIndices = Linker->DependsMap(ExpIndex);

			// for now, we don't save out any dependencies in cooked packages, but we do add an entry to the DependsMap
			// to keep the assertion that ExportMap.Num() == DependsMap.Num()
			if (!GIsCooking)
			{
				// find all the objects needed by this export
				TArray<UObject*>* SrcDepends = ObjectDependencies.Find(Object);
				check(SrcDepends);

				// go through each object and...
				for (INT DependIndex = 0; DependIndex < SrcDepends->Num(); DependIndex++)
				{
					UObject* DependentObject = (*SrcDepends)(DependIndex);

					INT DependencyIndex = 0;
					// if the dependency is in the same pacakge, we need to save an index into our ExportMap
					if (DependentObject->GetOutermost() == Linker->LinkerRoot)
					{
						// ... find the associated ImportIndex
						for (INT ExportIndex = 0; ExportIndex < Linker->ExportMap.Num() && DependencyIndex == 0; ExportIndex++)
						{
							if (Linker->ExportMap(ExportIndex)._Object == DependentObject)
							{
								//warnf(TEXT("  Found it [%d]!"), ImportIndex);
								DependencyIndex = ExportIndex + 1;
							}
						}
					}
					// otherwise we need to save an index into the ImportMap
					else
					{
						// ... find the associated ImportIndex
						for (INT ImportIndex = 0; ImportIndex < Linker->ImportMap.Num() && DependencyIndex == 0; ImportIndex++)
						{
							if (Linker->ImportMap(ImportIndex).XObject == DependentObject)
							{
								DependencyIndex = -(ImportIndex + 1);
							}
						}
					}
				
					if (DependencyIndex == 0)
					{
						// if we didn't find it, then we are in trouble, something went wrong somewhere
						checkf(DependencyIndex != 0, TEXT("Failed to find dependency index for %s"), *DependentObject->GetFullName());
					}

					// add the import as an import for this export
					DependIndices.AddItem(DependencyIndex);
				}
			}
		}

		// Set linker reverse mappings.
		// also set netplay required data for any UPackages in the export map
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			UObject* Object = Linker->ExportMap(i)._Object;
			if( Object )
			{
				Linker->ObjectIndices(Object->GetIndex()) = i+1;

				UPackage* Package = Cast<UPackage>(Object);
				if (Package != NULL && !(Package->PackageFlags & PKG_ServerSideOnly))
				{
					Linker->ExportMap(i).GenerationNetObjectCount = Package->GetGenerationNetObjectCount();
					Linker->ExportMap(i).PackageGuid = Package->GetGuid();
				}
			}
		}
		for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		{
			UObject* Object = Linker->ImportMap(i).XObject;
			if( Object != NULL )
			{
				Linker->ObjectIndices(Object->GetIndex()) = -i-1;
			}
		}

		// Find components referenced by exports.
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			FObjectExport&	Export = Linker->ExportMap(i);

			// if this export corresponds to a UComponent, add the ExportMap index to the component outer's ComponentMap array

			// this is no longer necessary; this was necessary only because components weren't always serialized in cases where they should have been,
			// due to incorrect logic in AreComponentsIdentical, which was sometimes incorrectly reporting that components were identical to their templates.
			// so the FObjectExport::ComponentMap was the only place that was actually serializing these components...
			//
			// however, we'll leave it in for now as it makes debugging much easier
			UComponent* TaggedComponent = Cast<UComponent>(Export._Object);
			if ( TaggedComponent != NULL && !TaggedComponent->HasAnyFlags(RF_ClassDefaultObject) )
			{
				PACKAGE_INDEX OuterMapIndex = Linker->ObjectIndices(TaggedComponent->GetOuter()->GetIndex());
				check(!IS_IMPORT_INDEX(OuterMapIndex));

				FObjectExport& OuterExport = Linker->ExportMap(OuterMapIndex - 1);
				OuterExport.ComponentMap.Set( TaggedComponent->GetInstanceMapName(), i );
			}
		}

		// Save dummy import map, overwritten later.
		Linker->Summary.ImportOffset = Linker->Tell();
		for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		{
			FObjectImport& Import = Linker->ImportMap( i );
			*Linker << Import;
		}
		INT OffsetAfterImportMap = Linker->Tell();

		// Save dummy export map, overwritten later.
		Linker->Summary.ExportOffset = Linker->Tell();
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			FObjectExport& Export = Linker->ExportMap( i );
			*Linker << Export;
		}
		INT OffsetAfterExportMap = Linker->Tell();

		// save depends map (no need for later patching)
		check(Linker->DependsMap.Num()==Linker->ExportMap.Num());
		Linker->Summary.DependsOffset = Linker->Tell();
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			TArray<INT>& Depends = Linker->DependsMap( i );
			*Linker << Depends;
		}

		Linker->Summary.TotalHeaderSize	= Linker->Tell();

		// Save exports.
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			FObjectExport& Export = Linker->ExportMap(i);
			if( Export._Object )
			{
				// Set class index.
				if( !Export._Object->IsA(UClass::StaticClass()) )
				{
					Export.ClassIndex = Linker->ObjectIndices(Export._Object->GetClass()->GetIndex());
					check(Export.ClassIndex != UCLASS_INDEX);
				}
				else
				{
					// this is a UClass object
					Export.ClassIndex = UCLASS_INDEX;
				}

				// Set the parent index, if this export represents a UStruct-derived object
				if( Export._Object->IsA(UStruct::StaticClass()) )
				{
					UStruct* Struct = (UStruct*)Export._Object;
					if( Struct->SuperField )
					{
						Export.SuperIndex = Linker->ObjectIndices(Struct->SuperField->GetIndex());
						check(Export.SuperIndex!=NULLSUPER_INDEX);
					}
					else
					{
						Export.SuperIndex = NULLSUPER_INDEX;
					}
				}
				else
				{
					Export.SuperIndex = NULLSUPER_INDEX;
				}

				// Set PACKAGE_INDEX for this export's Outer. If the export's Outer
				// is the UPackage corresponding to this package's LinkerRoot, the
				// OuterIndex will remain 0 == ROOTPACKAGE_INDEX.
				if( Export._Object->GetOuter() != InOuter )
				{
					if( Export._Object->GetOuter() )
					{
						check(Export._Object->GetOuter()->IsIn(InOuter) || Export._Object->HasAllFlags(RF_ForceTagExp));
						Export.OuterIndex = Linker->ObjectIndices(Export._Object->GetOuter()->GetIndex());
						check(!IS_IMPORT_INDEX(Export.OuterIndex));
						check(Export.OuterIndex != ROOTPACKAGE_INDEX);
					}
					else
					{
						check(Export._Object->HasAllFlags(RF_ForceTagExp));
						check(Export.HasAllFlags(EF_ForcedExport));
						Export.OuterIndex = ROOTPACKAGE_INDEX;
					}			
				}
				else
				{
					// this export's Outer is the LinkerRoot for this package
					Export.OuterIndex = ROOTPACKAGE_INDEX;
				}

				// Set the PACKAGE_INDEX for this export's object archetype.  If the
				// export's archetype is a class default object, the ArchetypeIndex
				// is set to 0
				UObject* Template = Export._Object->GetArchetype();
				if ( Template != NULL && Template != Export._Object->GetClass()->GetDefaultObject() )
				{
					Export.ArchetypeIndex = Linker->ObjectIndices(Template->GetIndex());
					//@todo: verify
					//check(Export.ArchetypeIndex!=CLASSDEFAULTS_INDEX);
				}
				else
				{
					Export.ArchetypeIndex = CLASSDEFAULTS_INDEX;
				}

				// Save the object data.
				Export.SerialOffset = Linker->Tell();
				if ( Export._Object->HasAnyFlags(RF_ClassDefaultObject) )
				{
					Export._Object->GetClass()->SerializeDefaultObject(Export._Object, *Linker);
				}
				else
				{
					Export._Object->Serialize( *Linker );
				}
				Export.SerialSize = Linker->Tell() - Export.SerialOffset;

				// Mark object as having been saved.
				Export._Object->SetFlags( RF_Saved );
			}
		}

		// Save the import map.
		Linker->Seek( Linker->Summary.ImportOffset );
		for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		{
			FObjectImport& Import = Linker->ImportMap( i );

			// Set the package index.
			if( Import.XObject != NULL && Import.XObject->GetOuter() )
			{
				if ( Import.XObject->GetOuter()->IsIn(InOuter) )
				{
					if ( !Import.XObject->HasAllFlags(RF_Native|RF_Transient) )
					{
						warnf(TEXT("Bad Object=%s"),*Import.XObject->GetFullName());
					}
					else
					{
						// if an object is marked RF_Transient|RF_Native, it is either an intrinsic class or
						// a property of an intrinsic class.  Only properties of intrinsic classes will have
						// an Outer that passes the check for "GetOuter()->IsIn(InOuter)" (thus ending up in this
						// block of code).  Just verify that the Outer for this property is also marked RF_Transient|RF_Native
						check(Import.XObject->GetOuter()->HasAllFlags(RF_Native|RF_Transient));
					}
				}
				check(!Import.XObject->GetOuter()->IsIn(InOuter)||Import.XObject->HasAllFlags(RF_Native|RF_Transient));
				Import.OuterIndex = Linker->ObjectIndices(Import.XObject->GetOuter()->GetIndex());
				// check(IS_IMPORT_INDEX(Import.OuterIndex)); // RF_ForceTagExp can break this assumption.
			}

			// Save it.
			*Linker << Import;
		}
		check( Linker->Tell() == OffsetAfterImportMap );

		// Save the export map.
		Linker->Seek( Linker->Summary.ExportOffset );
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			*Linker << Linker->ExportMap( i );
		}
		check( Linker->Tell() == OffsetAfterExportMap );
		
		// Rewrite updated file summary.
		GWarn->StatusUpdatef( 0, 0, *LocalizeProgress(TEXT("Closing"),TEXT("Core")) );
		//@todo: remove ExportCount and NameCount - no longer used
		Linker->Summary.Generations.Last().ExportCount = Linker->Summary.ExportCount;
		Linker->Summary.Generations.Last().NameCount   = Linker->Summary.NameCount;
		// if it's a cooked package, use the pre-cooked NetObjectCount; otherwise, use the current ExportCount
		if (InOuter->PackageFlags & PKG_Cooked)
		{
			if (InOuter->GetGenerationNetObjectCount().Num() == 0)
			{
				if (!(InOuter->PackageFlags & PKG_ServerSideOnly))
				{
					debugf(NAME_Warning, TEXT("Cooked replicated package '%s' with no net object count table! Forcing server side only."), *InOuter->GetName());
					InOuter->PackageFlags |= PKG_ServerSideOnly;
					Linker->Summary.PackageFlags |= PKG_ServerSideOnly;
				}
				Linker->Summary.Generations.Last().NetObjectCount = 0;
			}
			else
			{
				Linker->Summary.Generations.Last().NetObjectCount = InOuter->GetGenerationNetObjectCount().Last();
			}
		}
		else
		{
			Linker->Summary.Generations.Last().NetObjectCount = Linker->Summary.ExportCount;
		}
		Linker->Seek(0);
		*Linker << Linker->Summary;
		check( Linker->Tell() == OffsetAfterPackageFileSummary );
		Success = TRUE;
	}
#if !EXCEPTIONS_DISABLED
	catch( const TCHAR* Msg )
	{
		// Delete the temporary file.
		GFileManager->Delete( TempFilename );

		if (!(SaveFlags & SAVE_NoError))
		{
			Error->Logf( NAME_Warning, TEXT("%s"), Msg );
		}
		return FALSE;
	}
#endif
	// Detach archive used for saving, closing file handle.
	if( Linker )
	{
		Linker->Detach();
	}
	unclock(Time);
	debugf( NAME_Log, TEXT("Save=%f"), GSecondsPerCycle*1000*Time );
	
	if( Success == TRUE )
	{
#if DEBUG_DISTRIBUTED_COOKING
		//@{
		//@temp for debugging
		InOuter->PackageFlags &= ~(PKG_StoreCompressed|PKG_StoreFullyCompressed);
		//@}
#endif

		// Compress the temporarily file to destination.
		if( InOuter->PackageFlags & PKG_StoreCompressed )
		{
			debugf( NAME_Log, TEXT("Compressing '%s' to '%s'"), TempFilename, Filename );
			FFileCompressionHelper CompressionHelper;
			Success = CompressionHelper.CompressFile( TempFilename, Filename, Linker );
		}
		// Fully compress package in one "block".
		else if( InOuter->PackageFlags & PKG_StoreFullyCompressed )
		{
			debugf( NAME_Log, TEXT("Full-package compressing '%s' to '%s'"), TempFilename, Filename );
			FFileCompressionHelper CompressionHelper;
			Success = CompressionHelper.FullyCompressFile( TempFilename, Filename, Linker->ForceByteSwapping() );
		}
		// Move the temporary file.
		else
		{
			debugf( NAME_Log, TEXT("Moving '%s' to '%s'"), TempFilename, Filename );
			Success = GFileManager->Move( Filename, TempFilename );
		}
		
		// ECC align files if we're cooking for console.
		if( GCookingTarget & UE3::PLATFORM_Console )
		{
			// Retrieve the size of the file and only perform work if it exists.
			INT	FileSize = GFileManager->FileSize( Filename );
			if( FileSize > 0 )
			{
				// Create file writer used to append padding. Don't allow failure.
				FArchive* FileWriter = GFileManager->CreateFileWriter( Filename, FILEWRITE_Append | FILEWRITE_NoFail );
				if( FileWriter )
				{
					// Figure out how much to pad.
					INT	FileAlignment	= DVD_ECC_BLOCK_SIZE;
					INT	BytesToPad		= Align( FileSize, FileAlignment ) - FileSize;
					// If there is something to pad, append by serializing 0s.
					if( BytesToPad )
					{
						void* PaddingData = appMalloc( FileAlignment );
						appMemzero( PaddingData, FileAlignment );
                        FileWriter->Serialize( PaddingData, BytesToPad );
						appFree( PaddingData );
					}
					// Delete archive to release file handle.
					delete FileWriter;
					FileWriter = NULL;
				}
			}
		}
		
		if( Success == FALSE )
		{
			GFileManager->Delete( TempFilename );
			warnf((SaveFlags & SAVE_NoError) ? NAME_Log : NAME_Error, *LocalizeError(TEXT("SaveWarning"),TEXT("Core")), Filename );
		}
		else
		{
			// Clear dirty flag.
			InOuter->SetDirtyFlag(FALSE);
	
			// Update package file cache
			GPackageFileCache->CachePackage( Filename, 0, !GIsUCC );

			// Warn about long package names, which may be bad for consoles with limited filename lengths.
			if( bWarnOfLongFilename == TRUE )
			{
				FString BaseFilename = FFilename(Filename).GetBaseFilename();
				INT MaxFilenameLength = MAX_UNREAL_FILENAME_LENGTH;

				// If the name is of the form "_LOC_xxx.ext", remove the loc data before the length check
				if( BaseFilename.InStr( "_LOC_" ) == BaseFilename.Len() - 8 )
				{
					BaseFilename = BaseFilename.LeftChop( 8 );
				}

				if( BaseFilename.Len() > MaxFilenameLength )
				{
					warnf( (SaveFlags & SAVE_NoError) ? NAME_Log : NAME_Error, *FString::Printf( *LocalizeError(TEXT("Error_FilenameIsTooLongForCooking"),TEXT("Core")), *BaseFilename, MaxFilenameLength ) );
				}
			}
		}
	}

	// Route PostSaveRoot to allow e.g. the world to detach components for the persistent level that where
	// attached in PreSaveRoot.
	if( Base )
	{
		Base->PostSaveRoot( bCleanupIsRequired );
	}

	// send a message that the package was saved
	if( Success == TRUE )
	{
		GCallbackEvent->Send(CALLBACK_PackageSaved, Filename, InOuter);
	}

	return Success;
}


