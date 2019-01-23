/*=============================================================================
	FFileManagerGeneric.h: Unreal generic file manager support code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.

	This base class simplifies FFileManager implementations by providing
	simple, unoptimized implementations of functions whose implementations
	can be derived from other functions.

=============================================================================*/

// for compression
#include "CorePrivate.h"
#include "FFileManagerGeneric.h"

#define COPYBLOCKSIZE	32768

/*-----------------------------------------------------------------------------
	File I/O tracking.
-----------------------------------------------------------------------------*/

#if TRACK_FILEIO_STATS

/** Returns global file I/O stats collector and creates it if necessary. */
FFileIOStats* GetFileIOStats()
{
	static FFileIOStats* GFileIOStats = NULL;
	if( !GFileIOStats )
	{
		GFileIOStats = new FFileIOStats();
	}
	return GFileIOStats;
}

/**
 * Returns a handle associated with the passed in file if already existing, otherwise
 * it will create it first.
 *
 * @param	Filename	Filename to map to a handle
 * @return	unique handle associated with filename
 */
INT FFileIOStats::GetHandle( const TCHAR* InFilename )
{
	// Code is not re-entrant and can be called from multiple threads.
	FScopeLock ScopeLock(&CriticalSection);

	// Unique handle counter. 0 means invalid handle.
	static INT UniqueFileIOStatsHandle = 0;

	// Make sure multiple ways to accessing a file via relative paths leads to the same handle.
	FString FullyQualifiedFilename = appConvertRelativePathToFull( InFilename );

	// Check whether we already have a handle associated with this filename.
	INT Handle = FilenameToHandleMap.FindRef( FullyQualifiedFilename );
	// And create one if not.
	if( !Handle )
	{
		// Pre-increment as 0 means invalid.
		Handle = ++UniqueFileIOStatsHandle;
		// Associate handle with fresh summary.
		HandleToSummaryMap.Set( Handle, FFileIOSummary( *FullyQualifiedFilename ) );
	}

	return Handle;
}

/**
 * Adds I/O request to the stat collection.
 *
 * @param	StatsHandle	Handle this request is for
 * @param	Size		Size of request
 * @param	Offset		Offset of request
 * @param	Duration	Time request took
 * @param	RequestType	Determines type of request
 */
void FFileIOStats::AddRequest( INT StatsHandle, INT Size, INT Offset, DOUBLE Duration, EFileIOType RequestType )
{
	// Code is not re-entrant and can be called from multiple threads.
	FScopeLock ScopeLock(&CriticalSection);

	// Look up summary associated with request.
	FFileIOSummary* Summary = HandleToSummaryMap.Find( StatsHandle );
	check(Summary);
	
	switch( RequestType )
	{
	case FIOT_ReadRequest:
		// We're reading from file via file manager.
		Summary->ReadSize += Size;
		Summary->ReadTime += Duration;
		Summary->ReadCount++;
		break;
	case FIOT_AsyncReadRequest:
		// We're reading from file via async IO.
		Summary->AsyncReadSize += Size;
		Summary->AsyncReadTime += Duration;
		Summary->AsyncReadCount++;
		break;
	case FIOT_WriteRequest:
		// We're writing to file via file manager.
		Summary->WriteSize += Size;
		Summary->WriteTime += Duration;
		Summary->WriteCount++;
		break;
	default:
		break;
	}
}

/**
 * Dumps a file summary to the log.
 *
 * @param Summary	Summary to dump
 * @param FileSize	Current size of associated file
 */
void FFileIOStats::DumpSummary( const FFileIOSummary& Summary, SIZE_T FileSize )
{
	// Code is not re-entrant and can be called from multiple threads.
	FScopeLock ScopeLock(&CriticalSection);

	// Log in CSV format.
	debugf(	TEXT(",%s")
		TEXT(",%i,%i,%7.2f")
		TEXT(",%i,%i,%7.2f")
		TEXT(",%i,%5.2f,%5.2f")
		TEXT(",%i,%i,%7.2f"),

		*Summary.Filename,

		Summary.ReadSize,
		Summary.ReadCount,
		Summary.ReadTime,

		Summary.AsyncReadSize,
		Summary.AsyncReadCount,
		Summary.AsyncReadTime,

		FileSize,
		FileSize ? 100.f * Summary.ReadSize / FileSize : 0,
		FileSize ? 100.f * Summary.AsyncReadSize / FileSize : 0,

		Summary.WriteSize,
		Summary.WriteCount,
		Summary.WriteTime );
}

/**
 * Dumps collected stats to log in CSV format to make it easier to import into Excel for sorting.
 */
void FFileIOStats::DumpStats()
{	
	// Code is not re-entrant and can be called from multiple threads.
	FScopeLock ScopeLock(&CriticalSection);

	// Header row. Initial comma allows getting rid of "Log: " prefix.
	debugf(TEXT(",Filename,Read Size,Read Count,Read Time,Async Read Size,Async Read Count,Async Read Time,File Size,Read Percentage,Async Read Percentage,Write Size,Write Count,Write Time"));
	
	// Iterate over all files and gather totals.
	FFileIOSummary	Total( TEXT("Total") );
	SIZE_T			TotalFileSize = 0;
	for( TMap<INT,FFileIOSummary>::TConstIterator It(HandleToSummaryMap); It; ++It )
	{
		const FFileIOSummary& Summary = It.Value();
		Total			+= Summary;
		TotalFileSize	+= GFileManager->FileSize( *Summary.Filename );
	}
	// Dump totals.
	DumpSummary( Total, TotalFileSize );

	// Iterate over all files and emit a row per file with gathered data.
	for( TMap<INT,FFileIOSummary>::TConstIterator It(HandleToSummaryMap); It; ++It )
	{
		const FFileIOSummary& Summary = It.Value();
		// Use file size to determine read percentages.
		SIZE_T FileSize = GFileManager->FileSize( *Summary.Filename );
		// Dump entry for file.
		DumpSummary( Summary, FileSize );
	}
}

#endif // TRACK_FILEIO_STATS

/*-----------------------------------------------------------------------------
	File Manager.
-----------------------------------------------------------------------------*/

INT FFileManagerGeneric::FileSize( const TCHAR* Filename )
{
	// Create a generic file reader, get its size, and return it.
	FArchive* Ar = CreateFileReader( Filename );
	if( !Ar )
		return -1;
	INT Result = Ar->TotalSize();
	delete Ar;
	return Result;
}

INT FFileManagerGeneric::UncompressedFileSize( const TCHAR* Filename )
{
	// if the platform doesn't support knowing uncompressed file sizes, then indicate it here
	return -1;
}

INT FFileManagerGeneric::GetFileStartSector( const TCHAR* Filename )
{
	// if the platform doesn't support knowing start sectors, then indicate it here
	return -1;
}

DWORD FFileManagerGeneric::Copy( const TCHAR* InDestFile, const TCHAR* InSrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, FCopyProgress* Progress )
{
	// Direct file copier.
	if( Progress && !Progress->Poll( 0.0 ) )
	{
		return COPY_Canceled;
	}
	DWORD	Result		= COPY_OK;
	FString SrcFile		= InSrcFile;
	FString DestFile	= InDestFile;
	
	FArchive* Src = CreateFileReader( *SrcFile );
	if( !Src )
	{
		Result = COPY_ReadFail;
	}
	else
	{
		INT Size = Src->TotalSize();
		FArchive* Dest = CreateFileWriter( *DestFile, (ReplaceExisting?0:FILEWRITE_NoReplaceExisting) | (EvenIfReadOnly?FILEWRITE_EvenIfReadOnly:0) );
		if( !Dest )
		{
			Result = COPY_WriteFail;
		}
		else
		{
			INT Percent=0, NewPercent=0;
			BYTE Buffer[COPYBLOCKSIZE];
			for( INT Total=0; Total<Size; Total+=sizeof(Buffer) )
			{
				INT Count = Min( Size-Total, (INT)sizeof(Buffer) );
				Src->Serialize( Buffer, Count );
				if( Src->IsError() )
				{
					Result = COPY_ReadFail;
					break;
				}
				Dest->Serialize( Buffer, Count );
				if( Dest->IsError() )
				{
					Result = COPY_WriteFail;
					break;
				}
				NewPercent = Total * 100 / Size;
				if( Progress && Percent != NewPercent && !Progress->Poll( (FLOAT)NewPercent / 100.f ) )
				{
					Result = COPY_Canceled;
					break;
				}
				Percent = NewPercent;
			}
			if( Result == COPY_OK )
			{
				if( !Dest->Close() )
				{
					Result = COPY_WriteFail;
				}
			}
			delete Dest;
			if( Result != COPY_OK )
			{
				Delete( *DestFile );
			}
		}
		if( Result == COPY_OK )
		{
			if( !Src->Close() )
			{
				Result = COPY_ReadFail;
			}
		}
		delete Src;
	}
	if( Progress && Result==COPY_OK && !Progress->Poll( 1.0 ) )
	{
		Result = COPY_Canceled;
	}
	return Result;
}

UBOOL FFileManagerGeneric::MakeDirectory( const TCHAR* Path, UBOOL Tree )
{
	// Support code for making a directory tree.
	check(Tree);
	INT CreateCount=0;
	for( TCHAR Full[256]=TEXT(""), *Ptr=Full; ; *Ptr++=*Path++ )
	{
		if( appIsPathSeparator(*Path) || *Path==0 )
		{
			*Ptr = 0;
			if( Ptr != Full && !IsDrive(Full) )
			{
				if( !MakeDirectory( Full, 0 ) )
					return 0;
				CreateCount++;
			}
		}
		if( *Path==0 )
			break;
	}
	return CreateCount!=0;
}

UBOOL FFileManagerGeneric::DeleteDirectory( const TCHAR* Path, UBOOL RequireExists, UBOOL Tree )
{
	// Support code for removing a directory tree.
	check(Tree);
	if( !appStrlen(Path) )
		return 0;
	FString Spec = FString(Path) * TEXT("*");
	TArray<FString> List;
	FindFiles( List, *Spec, 1, 0 );
	for( INT i=0; i<List.Num(); i++ )
		if( !Delete(*(FString(Path) * List(i)),1,1) )
			return 0;
	// clear out the list of found files
	List.Empty();
	FindFiles( List, *Spec, 0, 1 );
	for( INT i=0; i<List.Num(); i++ )
		if( !DeleteDirectory(*(FString(Path) * List(i)),1,1) )
			return 0;
	return DeleteDirectory( Path, RequireExists, 0 );
}

UBOOL FFileManagerGeneric::Move( const TCHAR* Dest, const TCHAR* Src, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes )
{
	// Move file manually.
	if( Copy(Dest,Src,ReplaceExisting,EvenIfReadOnly,Attributes,NULL) != COPY_OK )
		return 0;
	Delete( Src, 1, 1 );
	return 1;
}

UBOOL FFileManagerGeneric::IsDrive( const TCHAR* Path )
{
	// Does Path refer to a drive letter or BNC path?
	if( appStricmp(Path,TEXT(""))==0 )
		return 1;
	else if( appToUpper(Path[0])!=appToLower(Path[0]) && Path[1]==':' && Path[2]==0 )
		return 1;
	else if( appStricmp(Path,TEXT("\\"))==0 )
		return 1;
	else if( appStricmp(Path,TEXT("\\\\"))==0 )
		return 1;
	else if( Path[0]=='\\' && Path[1]=='\\' && !appStrchr(Path+2,'\\') )
		return 1;
	else if( Path[0]=='\\' && Path[1]=='\\' && appStrchr(Path+2,'\\') && !appStrchr(appStrchr(Path+2,'\\')+1,'\\') )
		return 1;
	else
		return 0;
}

INT FFileManagerGeneric::FindAvailableFilename( const TCHAR* Base, const TCHAR* Extension, FString& OutFilename, INT StartVal )
{
	check( Base );
	check( Extension );

	FString FullPath( Base );
	const INT IndexMarker = FullPath.Len();			// Marks location of the four-digit index.
	FullPath += TEXT("0000.");
	FullPath += Extension;

	// Iterate over indices, searching for a file that doesn't exist.
	for ( DWORD i = StartVal+1 ; i < 10000 ; ++i )
	{
		FullPath[IndexMarker  ] = i / 1000     + '0';
		FullPath[IndexMarker+1] = (i/100) % 10 + '0';
		FullPath[IndexMarker+2] = (i/10)  % 10 + '0';
		FullPath[IndexMarker+3] =   i     % 10 + '0';

		if ( GFileManager->FileSize( *FullPath ) == -1 )
		{
			// The file doesn't exist; output success.
			OutFilename = FullPath;
			return static_cast<INT>( i );
		}
	}

	// Can't find an empty filename slot with index in (StartVal, 9999].
	return -1;
}
