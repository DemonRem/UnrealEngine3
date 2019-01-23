/*=============================================================================
	FFileManagerWindows.h: Unreal Windows OS based file manager.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "FFileManagerGeneric.h"

/*-----------------------------------------------------------------------------
	FArchiveFileReaderWindows
-----------------------------------------------------------------------------*/

// File manager.
class FArchiveFileReaderWindows : public FArchive
{
public:
	FArchiveFileReaderWindows( HANDLE InHandle, const TCHAR* InFilename, FOutputDevice* InError, INT InSize );
	~FArchiveFileReaderWindows();

	virtual void Seek( INT InPos );
	virtual INT Tell();
	virtual INT TotalSize();
	virtual UBOOL Close();
	virtual void Serialize( void* V, INT Length );

protected:
	UBOOL InternalPrecache( INT PrecacheOffset, INT PrecacheSize );

	HANDLE          Handle;
	/** Handle for stats tracking */
	INT				StatsHandle;
	/** Filename for debugging purposes. */
	FString			Filename;
	FOutputDevice*  Error;
	INT             Size;
	INT             Pos;
	INT             BufferBase;
	INT             BufferCount;
	BYTE            Buffer[1024];
};


/*-----------------------------------------------------------------------------
	FArchiveFileWriterWindows
-----------------------------------------------------------------------------*/

class FArchiveFileWriterWindows : public FArchive
{
public:
	FArchiveFileWriterWindows( HANDLE InHandle, const TCHAR* InFilename, FOutputDevice* InError, INT InPos );
	~FArchiveFileWriterWindows();

	virtual void Seek( INT InPos );
	virtual INT Tell();
	virtual UBOOL Close();
	virtual void Serialize( void* V, INT Length );
	virtual void Flush();

protected:
	HANDLE          Handle;
	/** Handle for stats tracking */
	INT				StatsHandle;
	/** Filename for debugging purposes */
	FString			Filename;
	FOutputDevice*  Error;
	INT             Pos;
	INT             BufferCount;
	BYTE            Buffer[4096];
};


/*-----------------------------------------------------------------------------
	FFileManagerWindows
-----------------------------------------------------------------------------*/

class FFileManagerWindows : public FFileManagerGeneric
{
public:
	void Init(UBOOL Startup);

	UBOOL SetDefaultDirectory();
	FString GetCurrentDirectory();

	FArchive* CreateFileReader( const TCHAR* InFilename, DWORD Flags, FOutputDevice* Error );
	FArchive* CreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error );
	INT FileSize( const TCHAR* Filename );
	DWORD Copy( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, FCopyProgress* Progress );
	UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 );
	UBOOL IsReadOnly( const TCHAR* Filename );
	UBOOL Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 );
	UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 );
	UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 );
	void FindFiles( TArray<FString>& Result, const TCHAR* Filename, UBOOL Files, UBOOL Directories );
	DOUBLE GetFileAgeSeconds( const TCHAR* Filename );
	DOUBLE GetFileTimestamp( const TCHAR* Filename );
	UBOOL GetTimestamp( const TCHAR* Filename, timestamp& Timestamp );

	/**
	 * Updates the modification time of the file on disk to right now, just like the unix touch command
	 * @param Filename Path to the file to touch
	 * @return TRUE if successful
	 */
	UBOOL TouchFile(const TCHAR* Filename);

	/**
	 * Converts a path pointing into the installed directory (C:\Program Files\MyGame\ExampleGame\Config\ExampleEngine.ini)
	 * to a path that a least-privileged user can write to (C:\<UserDir>\MyGame\ExampleGame\Config\ExampleEngine.ini)
	 *
	 * @param AbsolutePath Source path to convert
	 *
	 * @return Path to the user directory
	 */
	FString ConvertAbsolutePathToUserPath(const TCHAR* AbsolutePath);

protected:
	FArchive* InternalCreateFileReader( const TCHAR* InFilename, DWORD Flags, FOutputDevice* Error );
	FArchive* InternalCreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error );
	INT InternalFileSize( const TCHAR* Filename );
	DWORD InternalCopy( const TCHAR* DestFile, const TCHAR* SrcFile, UBOOL ReplaceExisting, UBOOL EvenIfReadOnly, UBOOL Attributes, FCopyProgress* Progress );
	UBOOL InternalDelete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 );
	UBOOL InternalIsReadOnly( const TCHAR* Filename );
	UBOOL InternalMove( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 );
	UBOOL InternalMakeDirectory( const TCHAR* Path, UBOOL Tree=0 );
	UBOOL InternalDeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 );
	void InternalFindFiles( TArray<FString>& Result, const TCHAR* Filename, UBOOL Files, UBOOL Directories );
	DOUBLE InternalGetFileAgeSeconds( const TCHAR* Filename );
	DOUBLE InternalGetFileTimestamp( const TCHAR* Filename );
	UBOOL InternalGetTimestamp( const TCHAR* Filename, timestamp& Timestamp );

	/**
	 * Updates the modification time of the file on disk to right now, just like the unix touch command
	 * @param Filename Path to the file to touch
	 * @return TRUE if successful
	 */
	UBOOL InternalTouchFile(const TCHAR* Filename);

	/**
	 * Converts passed in filename to use an absolute path.
	 *
	 * @param	Filename	filename to convert to use an absolute path, safe to pass in already using absolute path
	 * 
	 * @return	filename using absolute path
	 */
	FString ConvertToAbsolutePath( const TCHAR* Filename );

	/** Directory where a Standard User can write to (to save settings, etc) */
	FString WindowsUserDir;

	/** Directory where the game in installed to */
	FString WindowsRootDir;

	/** Is the game running as if installed, ie, out of a directory a Standard User can't write to? */
	UBOOL bIsRunningInstalled;

};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

