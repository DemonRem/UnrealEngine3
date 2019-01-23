/*=============================================================================
	FFileManagerWindows.h: Unreal Windows OS based file manager.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

#if _MSC_VER

#include "FFileManagerWindows.h"

#pragma pack (push,8)
#include <sys/stat.h>
#include <sys/utime.h>
#undef clock
#include <time.h>
#include <shlobj.h>
#pragma pack(pop)

/*-----------------------------------------------------------------------------
	FArchiveFileReaderWindows implementation
-----------------------------------------------------------------------------*/

FArchiveFileReaderWindows::FArchiveFileReaderWindows( HANDLE InHandle, const TCHAR* InFilename, FOutputDevice* InError, INT InSize )
:   Handle          ( InHandle )
,	Filename		( InFilename )
,   Error           ( InError )
,   Size            ( InSize )
,   Pos             ( 0 )
,   BufferBase      ( 0 )
,   BufferCount     ( 0 )
{
	ArIsLoading = ArIsPersistent = 1;
	StatsHandle = FILE_IO_STATS_GET_HANDLE( InFilename );
}

FArchiveFileReaderWindows::~FArchiveFileReaderWindows()
{
	if( Handle )
	{
		Close();
	}
}

UBOOL FArchiveFileReaderWindows::InternalPrecache( INT PrecacheOffset, INT PrecacheSize )
{
	// Only precache at current position and avoid work if precaching same offset twice.
	if( Pos == PrecacheOffset && (!BufferBase || !BufferCount || BufferBase != Pos) )
	{
		BufferBase = Pos;
		BufferCount = Min( Min( PrecacheSize, (INT)(ARRAY_COUNT(Buffer) - (Pos&(ARRAY_COUNT(Buffer)-1))) ), Size-Pos );
		INT Count=0;

		// Read data from device via Win32 ReadFile API.
		{
			SCOPED_FILE_IO_READ_STATS( StatsHandle, BufferCount, Pos );
			ReadFile( Handle, Buffer, BufferCount, (DWORD*)&Count, NULL );
		}

		if( Count!=BufferCount )
		{
			ArIsError = 1;
			Error->Logf( TEXT("ReadFile failed: Count=%i BufferCount=%i Error=%s"), Count, BufferCount, appGetSystemErrorMessage() );
		}
	}
	return TRUE;
}

void FArchiveFileReaderWindows::Seek( INT InPos )
{
	check(InPos>=0);
	check(InPos<=Size);
	if( SetFilePointer( Handle, InPos, 0, FILE_BEGIN )==INVALID_SET_FILE_POINTER )
	{
		ArIsError = 1;
		Error->Logf( TEXT("SetFilePointer Failed %i/%i: %i %s"), InPos, Size, Pos, appGetSystemErrorMessage() );
	}
	Pos         = InPos;
	BufferBase  = Pos;
	BufferCount = 0;
}

INT FArchiveFileReaderWindows::Tell()
{
	return Pos;
}

INT FArchiveFileReaderWindows::TotalSize()
{
	return Size;
}

UBOOL FArchiveFileReaderWindows::Close()
{
	if( Handle )
	{
		CloseHandle( Handle );
	}
	Handle = NULL;
	return !ArIsError;
}

void FArchiveFileReaderWindows::Serialize( void* V, INT Length )
{
	while( Length>0 )
	{
		INT Copy = Min( Length, BufferBase+BufferCount-Pos );
		if( Copy==0 )
		{
			if( Length >= ARRAY_COUNT(Buffer) )
			{
				INT Count=0;
				// Read data from device via Win32 ReadFile API.
				{
					SCOPED_FILE_IO_READ_STATS( StatsHandle, Length, Pos );
					ReadFile( Handle, V, Length, (DWORD*)&Count, NULL );
				}
				if( Count!=Length )
				{
					ArIsError = 1;
					Error->Logf( TEXT("ReadFile failed: Count=%i Length=%i Error=%s for file %s"), 
						Count, Length, appGetSystemErrorMessage(), *Filename );
				}
				Pos += Length;
				BufferBase += Length;
				return;
			}
			InternalPrecache( Pos, MAXINT );
			Copy = Min( Length, BufferBase+BufferCount-Pos );
			if( Copy<=0 )
			{
				ArIsError = 1;
				Error->Logf( TEXT("ReadFile beyond EOF %i+%i/%i for file %s"), 
					Pos, Length, Size, *Filename );
			}
			if( ArIsError )
			{
				return;
			}
		}
		appMemcpy( V, Buffer+Pos-BufferBase, Copy );
		Pos       += Copy;
		Length    -= Copy;
		V          = (BYTE*)V + Copy;
	}
}



/*-----------------------------------------------------------------------------
	FArchiveFileWriterWindows implementation
-----------------------------------------------------------------------------*/

FArchiveFileWriterWindows::FArchiveFileWriterWindows( HANDLE InHandle, const TCHAR* InFilename, FOutputDevice* InError, INT InPos )
:   Handle      ( InHandle )
,	Filename	( InFilename )
,   Error       ( InError )
,   Pos         ( InPos )
,   BufferCount ( 0 )
{
	ArIsSaving = ArIsPersistent = 1;
	StatsHandle = FILE_IO_STATS_GET_HANDLE( InFilename );
}

FArchiveFileWriterWindows::~FArchiveFileWriterWindows()
{
	if( Handle )
	{
		Close();
	}
	Handle = NULL;
}

void FArchiveFileWriterWindows::Seek( INT InPos )
{
	Flush();
	if( SetFilePointer( Handle, InPos, 0, FILE_BEGIN )==0xFFFFFFFF )
	{
		ArIsError = 1;
		Error->Logf( *LocalizeError("SeekFailed",TEXT("Core")) );
	}
	Pos = InPos;
}
	
INT FArchiveFileWriterWindows::Tell()
{
	return Pos;
}

UBOOL FArchiveFileWriterWindows::Close()
{
	Flush();
	if( Handle && !CloseHandle(Handle) )
	{
		ArIsError = 1;
		Error->Logf( *LocalizeError("WriteFailed",TEXT("Core")) );
	}
	Handle = NULL;
	return !ArIsError;
}

void FArchiveFileWriterWindows::Serialize( void* V, INT Length )
{
	Pos += Length;
	INT Copy;
	while( Length > (Copy=ARRAY_COUNT(Buffer)-BufferCount) )
	{
		appMemcpy( Buffer+BufferCount, V, Copy );
		BufferCount += Copy;
		Length      -= Copy;
		V            = (BYTE*)V + Copy;
		Flush();
	}
	if( Length )
	{
		appMemcpy( Buffer+BufferCount, V, Length );
		BufferCount += Length;
	}
}

void FArchiveFileWriterWindows::Flush()
{
	if( BufferCount )
	{
		SCOPED_FILE_IO_WRITE_STATS( StatsHandle, BufferCount, Pos );

		INT Result=0;
		if( !WriteFile( Handle, Buffer, BufferCount, (DWORD*)&Result, NULL ) )
		{
			ArIsError = 1;
			Error->Logf( *LocalizeError("WriteFailed",TEXT("Core")) );
		}
	}
	BufferCount = 0;
}


/*-----------------------------------------------------------------------------
	FFileManagerWindows implementation
-----------------------------------------------------------------------------*/


/**
 * Converts passed in filename to use an absolute path.
 *
 * @param	Filename	filename to convert to use an absolute path, safe to pass in already using absolute path
 * 
 * @return	filename using absolute path
 */
FString FFileManagerWindows::ConvertToAbsolutePath( const TCHAR* Filename )
{
	FString AbsolutePath = Filename;

	// See whether it is a relative path.
	if( AbsolutePath.StartsWith( TEXT("..") ) )
	{
		// And if that's the case append the base dir.
		AbsolutePath = FString(appBaseDir()) + AbsolutePath;
	}

	return AbsolutePath;
}

/**
 * Converts a path pointing into the installed directory (C:\Program Files\MyGame\ExampleGame\Config\ExampleEngine.ini)
 * to a path that a least-privileged user can write to (C:\<UserDir>\MyGame\ExampleGame\Config\ExampleEngine.ini)
 *
 * @param AbsolutePath Source path to convert
 *
 * @return Path to the user directory
 */
FString FFileManagerWindows::ConvertAbsolutePathToUserPath(const TCHAR* AbsolutePath)
{
	// if the user directory has been setup, use it
	if (WindowsUserDir.Len() > 0)
	{
		// replace C:\Program Files\MyGame\Binaries with C:\<UserDir>\Binaries
		return FString(AbsolutePath).Replace(*WindowsRootDir, *WindowsUserDir);
	}
	else
	{
		// in the non-installed (dev) case, just return the original path in all cases
		return AbsolutePath;
	}
}


/*-----------------------------------------------------------------------------
	Public interface
-----------------------------------------------------------------------------*/

void FFileManagerWindows::Init(UBOOL Startup)
{
	// a shipped PC game will always run as if installed
#if SHIPPING_PC_GAME
	bIsRunningInstalled = TRUE;
#else
	// for development, use a commandline param (-installed)
	bIsRunningInstalled = ParseParam(appCmdLine(),TEXT("INSTALLED"));
#endif

	if (bIsRunningInstalled)
	{
		TCHAR UserPath[MAX_PATH];
		// get the My Documents directory
		HRESULT Ret = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, UserPath);

		// get the per-game directory name to use inside the My Documents directory 
		FString DefaultIniContents;
		// load the DefaultEngine.ini config file into a string for later parsing (ConvertAbsolutePathToUserPath will use
		// original location since WindowsUserDir hasn't been set yet)
		if (!appLoadFileToString(DefaultIniContents, *(appGameDir() + TEXT("Config\\DefaultEngine.ini")), this))
		{
			// appMsgf won't write to a log if GWarn is NULL, which it should be at this point
			appMsgf(AMT_OK, TEXT("Failed to find default engine .ini file to retrieve My Documents subdirectory to use. Force quitting."));
			exit(1);
			return;
		}

		#define MYDOC_KEY_NAME TEXT("MyDocumentsSubDirName=")

		// find special key in the .ini file (can't use GConfig because it can't be used yet until after filemanager is made)
		INT KeyLocation = DefaultIniContents.InStr(MYDOC_KEY_NAME, FALSE, TRUE);
		if (KeyLocation == INDEX_NONE)
		{
			// appMsgf won't write to a log if GWarn is NULL, which it should be at this point
			appMsgf(AMT_OK, TEXT("Failed to find %s key in DefaultEngine.ini. Force quitting."), MYDOC_KEY_NAME);
			exit(1);
			return;
		}

		// skip over the key to get the value (skip key and = sign) and everything after it
		FString ValueAndLeftover = DefaultIniContents.Mid(KeyLocation + appStrlen(MYDOC_KEY_NAME));
		
		// now chop off this string at an end of line
		TArray<FString> Tokens;
		ValueAndLeftover.ParseIntoArray(&Tokens, TEXT("\r\n"), TRUE);

		// make the base user dir path
		WindowsUserDir = FString(UserPath) + TEXT("\\My UE3 Games\\") + Tokens(0) + TEXT("\\");

		// find out our executable path
		WindowsRootDir = appBaseDir();
		// strip off the Binaries directory
		WindowsRootDir = WindowsRootDir.Left(WindowsRootDir.InStr(TEXT("\\Binaries\\"), TRUE) + 1);
	}
}

UBOOL FFileManagerWindows::SetDefaultDirectory()
{
	return TCHAR_CALL_OS(SetCurrentDirectoryW(appBaseDir()),SetCurrentDirectoryA(TCHAR_TO_ANSI(appBaseDir())))!=0;
}

FString FFileManagerWindows::GetCurrentDirectory()
{
	return appBaseDir();
}

FArchive* FFileManagerWindows::CreateFileReader( const TCHAR* InFilename, DWORD Flags, FOutputDevice* Error )
{
	// first look in User Directory
	FArchive* ReturnValue = InternalCreateFileReader( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(InFilename)), Flags, Error );

	// if not found there, then look in the install directory
	if (ReturnValue == NULL)
	{
		ReturnValue = InternalCreateFileReader( *ConvertToAbsolutePath(InFilename), Flags, Error );
	}

	return ReturnValue;
}

FArchive* FFileManagerWindows::CreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
{
	return InternalCreateFileWriter( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)), Flags, Error );
}

INT FFileManagerWindows::FileSize( const TCHAR* Filename )
{
	// try user directory first
	INT ReturnValue = InternalFileSize( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)) );
	if (ReturnValue == -1)
	{
		ReturnValue = InternalFileSize( *ConvertToAbsolutePath(Filename) );
	}
	return ReturnValue;
}

DWORD FFileManagerWindows::Copy( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, FCopyProgress* Progress )
{
	// we can only write to user directory, but source may be user or install (try user first)
	DWORD ReturnValue = InternalCopy( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(DestFile)), *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(SrcFile)), ReplaceExisting, EvenIfReadOnly, Attributes, Progress );
	if (ReturnValue != COPY_OK)
	{
		ReturnValue = InternalCopy( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(DestFile)), *ConvertToAbsolutePath(SrcFile), ReplaceExisting, EvenIfReadOnly, Attributes, Progress );
	}

	return ReturnValue;
}

UBOOL FFileManagerWindows::Delete( const TCHAR* Filename, UBOOL RequireExists, UBOOL EvenReadOnly )
{
	// we can only delete from user directory
	return InternalDelete( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)), RequireExists, EvenReadOnly );
}

UBOOL FFileManagerWindows::IsReadOnly( const TCHAR* Filename )
{
	return InternalIsReadOnly( *ConvertToAbsolutePath(Filename) );
}

UBOOL FFileManagerWindows::Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace, UBOOL EvenIfReadOnly, UBOOL Attributes )
{
	// we can only write to user directory, but source may be user or install (try user first)
	UBOOL ReturnValue = InternalMove( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Dest)), *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Src)), Replace, EvenIfReadOnly, Attributes );
	if (ReturnValue == FALSE)
	{
		ReturnValue = InternalMove( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Dest)), *ConvertToAbsolutePath(Src), Replace, EvenIfReadOnly, Attributes );
	}

	return ReturnValue;
}

UBOOL FFileManagerWindows::MakeDirectory( const TCHAR* Path, UBOOL Tree )
{
	return InternalMakeDirectory( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Path)), Tree );
}

UBOOL FFileManagerWindows::DeleteDirectory( const TCHAR* Path, UBOOL RequireExists, UBOOL Tree )
{
	return InternalDeleteDirectory( *ConvertToAbsolutePath(Path), RequireExists, Tree );
}

void FFileManagerWindows::FindFiles( TArray<FString>& Result, const TCHAR* Filename, UBOOL Files, UBOOL Directories )
{
	// first perform the find in the User directory
	TArray<FString> UserDirResults;
	InternalFindFiles( Result, *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)), Files, Directories );
	
	// now do the find in the install directory
	TArray<FString> InstallDirResults;
	InternalFindFiles( InstallDirResults, *ConvertToAbsolutePath(Filename), Files, Directories );

	// now add any new files to the results (so that user dir files override install dir files)
	for (INT InstallFileIndex = 0; InstallFileIndex < InstallDirResults.Num(); InstallFileIndex++)
	{
		Result.AddUniqueItem(*InstallDirResults(InstallFileIndex));
	}
}

DOUBLE FFileManagerWindows::GetFileAgeSeconds( const TCHAR* Filename )
{
	// first look for the file in the user dir
	DOUBLE ReturnValue = InternalGetFileAgeSeconds( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)) );
	// then look in install dir if it failed to be found
	if (ReturnValue == -1.0)
	{
		ReturnValue = InternalGetFileAgeSeconds( *ConvertToAbsolutePath(Filename) );
	}

	return ReturnValue;
}

DOUBLE FFileManagerWindows::GetFileTimestamp( const TCHAR* Filename )
{
	// first look for the file in the user dir
	DOUBLE ReturnValue = InternalGetFileTimestamp( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)) );
	// then look in install dir if it failed to be found
	if (ReturnValue == -1.0)
	{
		ReturnValue = InternalGetFileTimestamp( *ConvertToAbsolutePath(Filename) );
	}

	return ReturnValue;
}

UBOOL FFileManagerWindows::GetTimestamp( const TCHAR* Filename, timestamp& Timestamp )
{
	// first look for the file in the user dir
	UBOOL ReturnValue = InternalGetTimestamp( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)), Timestamp );
	// then look in install dir if it failed to be found
	if (ReturnValue == FALSE)
	{
		ReturnValue = InternalGetTimestamp( *ConvertToAbsolutePath(Filename), Timestamp );
	}

	return ReturnValue;
}


/**
 * Updates the modification time of the file on disk to right now, just like the unix touch command
 * @param Filename Path to the file to touch
 * @return TRUE if successful
 */
UBOOL FFileManagerWindows::TouchFile(const TCHAR* Filename)
{
	// first look for the file in the user dir
	UBOOL ReturnValue = InternalTouchFile( *ConvertAbsolutePathToUserPath(*ConvertToAbsolutePath(Filename)) );
	// then look in install dir if it failed to be found
	if (ReturnValue == FALSE)
	{
		ReturnValue = InternalGetFileTimestamp( *ConvertToAbsolutePath(Filename) );
	}

	return ReturnValue;
}

/*-----------------------------------------------------------------------------
	Internal interface
-----------------------------------------------------------------------------*/

FArchive* FFileManagerWindows::InternalCreateFileReader( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
{
	DWORD  Access    = GENERIC_READ;
	DWORD  WinFlags  = FILE_SHARE_READ;
	DWORD  Create    = OPEN_EXISTING;
	HANDLE Handle    = TCHAR_CALL_OS( CreateFileW( Filename, Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ), CreateFileA( TCHAR_TO_ANSI(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ) );
	if( Handle==INVALID_HANDLE_VALUE )
	{
		if( Flags & FILEREAD_NoFail )
		{
			appErrorf( TEXT("Failed to read file: %s"), Filename );
		}
		return NULL;
	}
	return new FArchiveFileReaderWindows(Handle,Filename,Error,GetFileSize(Handle,NULL));
}

FArchive* FFileManagerWindows::InternalCreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
{
	MakeDirectory(*FFilename(Filename).GetPath(), TRUE);

	if( (GFileManager->FileSize (Filename) >= 0) && (Flags & FILEWRITE_EvenIfReadOnly) )
	{
		TCHAR_CALL_OS(SetFileAttributesW(Filename, 0),SetFileAttributesA(TCHAR_TO_ANSI(Filename), 0));
	}
	DWORD  Access    = GENERIC_WRITE;
	DWORD  WinFlags  = (Flags & FILEWRITE_AllowRead) ? FILE_SHARE_READ : 0;
	DWORD  Create    = (Flags & FILEWRITE_Append) ? OPEN_ALWAYS : (Flags & FILEWRITE_NoReplaceExisting) ? CREATE_NEW : CREATE_ALWAYS;
	HANDLE Handle    = TCHAR_CALL_OS( CreateFileW( Filename, Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ), CreateFileA( TCHAR_TO_ANSI(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ) );
	INT    Pos       = 0;
	if( Handle==INVALID_HANDLE_VALUE )
	{
		if( Flags & FILEWRITE_NoFail )
		{
			appErrorf( TEXT("Failed to create file: %s"), Filename );
		}
		return NULL;
	}
	if( Flags & FILEWRITE_Append )
	{
		Pos = SetFilePointer( Handle, 0, 0, FILE_END );
	}
	return new FArchiveFileWriterWindows(Handle,Filename,Error,Pos);
}

INT FFileManagerWindows::InternalFileSize( const TCHAR* Filename )
{
	HANDLE Handle = TCHAR_CALL_OS( CreateFileW( Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ), CreateFileA( TCHAR_TO_ANSI(Filename), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) );
	if( Handle==INVALID_HANDLE_VALUE )
	{
		return -1;
	}
	DWORD Result = GetFileSize( Handle, NULL );
	CloseHandle( Handle );
	return Result;
}

DWORD FFileManagerWindows::InternalCopy( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, FCopyProgress* Progress )
{
	if( EvenIfReadOnly )
	{
		TCHAR_CALL_OS(SetFileAttributesW(DestFile, 0),SetFileAttributesA(TCHAR_TO_ANSI(DestFile), 0));
	}
	DWORD Result;
	if( Progress )
	{
		Result = FFileManagerGeneric::Copy( DestFile, SrcFile, ReplaceExisting, EvenIfReadOnly, Attributes, Progress );
	}
	else
	{
		if( TCHAR_CALL_OS(CopyFileW(SrcFile, DestFile, !ReplaceExisting),CopyFileA(TCHAR_TO_ANSI(SrcFile), TCHAR_TO_ANSI(DestFile), !ReplaceExisting)) != 0)
		{
			Result = COPY_OK;
		}
		else
		{
			Result = COPY_MiscFail;
		}
	}
	if( Result==COPY_OK && !Attributes )
	{
		TCHAR_CALL_OS(SetFileAttributesW(DestFile, 0),SetFileAttributesA(TCHAR_TO_ANSI(DestFile), 0));
	}
	return Result;
}

UBOOL FFileManagerWindows::InternalDelete( const TCHAR* Filename, UBOOL RequireExists, UBOOL EvenReadOnly )
{
	if( EvenReadOnly )
	{
		TCHAR_CALL_OS(SetFileAttributesW(Filename,FILE_ATTRIBUTE_NORMAL),SetFileAttributesA(TCHAR_TO_ANSI(Filename),FILE_ATTRIBUTE_NORMAL));
	}
	INT Result = TCHAR_CALL_OS(DeleteFile(Filename),DeleteFileA(TCHAR_TO_ANSI(Filename)))!=0 || (!RequireExists && GetLastError()==ERROR_FILE_NOT_FOUND);
	if( !Result )
	{
		DWORD error = GetLastError();
		debugf( NAME_Error, TEXT("Error deleting file '%s' (GetLastError: %d)"), Filename, error );
	}
	return Result!=0;
}

UBOOL FFileManagerWindows::InternalIsReadOnly( const TCHAR* Filename )
{
	DWORD rc;
	if( FileSize( Filename ) < 0 )
	{
		return( 0 );
	}
	rc = TCHAR_CALL_OS(GetFileAttributesW(Filename),GetFileAttributesA(TCHAR_TO_ANSI(Filename)));
	if (rc != 0xFFFFFFFF)
	{
		return ((rc & FILE_ATTRIBUTE_READONLY) != 0);
	}
	else
	{
		debugf( NAME_Error, TEXT("Error reading attributes for '%s'"), Filename );
		return (0);
	}
}

UBOOL FFileManagerWindows::InternalMove( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace, UBOOL EvenIfReadOnly, UBOOL Attributes )
{
	//warning: MoveFileEx is broken on Windows 95 (Microsoft bug).
	Delete( Dest, 0, EvenIfReadOnly );
	INT Result = TCHAR_CALL_OS( MoveFileW(Src,Dest), MoveFileA(TCHAR_TO_ANSI(Src),TCHAR_TO_ANSI(Dest)) );
	if( !Result )
	{
		DWORD error = GetLastError();
		debugf( NAME_Error, TEXT("Error moving file '%s' to '%s' (GetLastError: %d)"), Src, Dest, error );
	}
	return Result!=0;
}

UBOOL FFileManagerWindows::InternalMakeDirectory( const TCHAR* Path, UBOOL Tree )
{
	if( Tree )
	{
		return FFileManagerGeneric::MakeDirectory( Path, Tree );
	}
	return TCHAR_CALL_OS( CreateDirectoryW(Path,NULL), CreateDirectoryA(TCHAR_TO_ANSI(Path),NULL) )!=0 || GetLastError()==ERROR_ALREADY_EXISTS;
}

UBOOL FFileManagerWindows::InternalDeleteDirectory( const TCHAR* Path, UBOOL RequireExists, UBOOL Tree )
{
	if( Tree )
		return FFileManagerGeneric::DeleteDirectory( Path, RequireExists, Tree );
	return TCHAR_CALL_OS( RemoveDirectoryW(Path), RemoveDirectoryA(TCHAR_TO_ANSI(Path)) )!=0 || (!RequireExists && GetLastError()==ERROR_FILE_NOT_FOUND);
}

void FFileManagerWindows::InternalFindFiles( TArray<FString>& Result, const TCHAR* Filename, UBOOL Files, UBOOL Directories )
{
	HANDLE Handle=NULL;
#if UNICODE
	if( GUnicodeOS )
	{
		WIN32_FIND_DATAW Data;
		Handle=FindFirstFileW(Filename,&Data);
		if( Handle!=INVALID_HANDLE_VALUE )
		{
			do
			{
				if
				(   appStricmp(Data.cFileName,TEXT("."))
				&&  appStricmp(Data.cFileName,TEXT(".."))
				&&  ((Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?Directories:Files) )
				{
					new(Result)FString(Data.cFileName);
				}
			}
			while( FindNextFileW(Handle,&Data) );
		}
	}
	else
#endif
	{
		WIN32_FIND_DATAA Data;
		Handle=FindFirstFileA(TCHAR_TO_ANSI(Filename),&Data);
		if( Handle!=INVALID_HANDLE_VALUE )
		{
			do
			{
				TStringConversion<TCHAR,ANSICHAR,FANSIToTCHAR_Convert,MAX_PATH> Name(Data.cFileName);
				if (appStricmp(Name,TEXT(".")) && appStricmp(Name,TEXT("..")) &&
					((Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? Directories : Files) )
				{
					new(Result)FString(Name);
				}
			}
			while( FindNextFileA(Handle,&Data) );
		}
	}
	if( Handle!=INVALID_HANDLE_VALUE )
	{
		FindClose( Handle );
	}
}

DOUBLE FFileManagerWindows::InternalGetFileAgeSeconds( const TCHAR* Filename )
{
	struct _stat FileInfo;
	if( 0 == TCHAR_CALL_OS( _wstat( Filename, &FileInfo ), _stat( TCHAR_TO_ANSI( Filename ), &FileInfo ) ) )
	{
		time_t CurrentTime, FileTime;	
		FileTime = FileInfo.st_mtime;
		time( &CurrentTime );
		return difftime( CurrentTime, FileTime );
	}
	return -1.0;
}

DOUBLE FFileManagerWindows::InternalGetFileTimestamp( const TCHAR* Filename )
{
	struct _stat FileInfo;
	if( 0 == TCHAR_CALL_OS( _wstat( Filename, &FileInfo ), _stat( TCHAR_TO_ANSI( Filename ), &FileInfo ) ) )
	{
		time_t FileTime;	
		FileTime = FileInfo.st_mtime;
		return FileTime;
	}
	return -1.0;
}

UBOOL FFileManagerWindows::InternalGetTimestamp( const TCHAR* Filename, timestamp& Timestamp )
{
	appMemzero( &Timestamp, sizeof(Timestamp) );
	struct _stat FileInfo;
	if( 0 == TCHAR_CALL_OS( _wstat( Filename, &FileInfo ), _stat( TCHAR_TO_ANSI( Filename ), &FileInfo ) ) )
	{
		time_t	FileTime;	
		FileTime = FileInfo.st_mtime;
#if USE_SECURE_CRT
		tm Time;
		gmtime_s(&Time,&FileTime);
#else
		tm& Time = *gmtime(&FileTime);
#endif
		Timestamp.Day       = Time.tm_mday;
		Timestamp.Month     = Time.tm_mon;
		Timestamp.DayOfWeek = Time.tm_wday;
		Timestamp.DayOfYear = Time.tm_yday;
		Timestamp.Hour      = Time.tm_hour;
		Timestamp.Minute    = Time.tm_min;
		Timestamp.Second    = Time.tm_sec;
		Timestamp.Year      = Time.tm_year + 1900;
		return TRUE;
	}
	return FALSE;
}

/**
 * Updates the modification time of the file on disk to right now, just like the unix touch command
 * @param Filename Path to the file to touch
 * @return TRUE if successful
 */
UBOOL FFileManagerWindows::InternalTouchFile(const TCHAR* Filename)
{
	time_t Now;
	// get the current time
	time(&Now);

	// use now as the time to set
	_utimbuf Time;
	Time.modtime = Now;
	Time.actime = Now;

	// set it to the file
	INT ReturnCode = TCHAR_CALL_OS( _wutime(Filename, &Time), _utime(TCHAR_TO_ANSI(Filename), &Time));

	// 0 return code was a success
	return ReturnCode == 0;
}

#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

