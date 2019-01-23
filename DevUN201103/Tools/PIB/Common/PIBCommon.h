/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef __PIBCOMMON_H__
#define __PIBCOMMON_H__

#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include <string>
#include <map>
#include <list>
#include <queue>
using namespace std;

#include "../../../Src/Launch/Inc/PIB.h"
#include "../../../External/tinyXML/tinyxml.h"

typedef enum
{	
	Stage_Starting,
	Stage_GetFileList,
	Stage_WaitingForFileList,
	Stage_GetFiles,
	Stage_WaitForFiles,
	Stage_WaitForAllFiles,
	Stage_InitDLL,
	Stage_Running,
	Stage_Exiting
} EPluginStage;

class FileDescCommon
{
public:
	// Unique integer identifier
	INT UniqueID;
	// Simple relative name to the root folders
	string Name;
	// Temporary file the required file is downloaded to
	string SourceFileName;
	// Final file to be used by the game
	string DestFileName;
	// Size of the file to download
	DWORD Size;
	// MD5 checksum of file (created by CookerSync)
	string MD5Sum;
	// The file handle to write to when streaming from the server
	HANDLE FileHandle;

	FileDescCommon( void )
	{
		UniqueID = -1;
		Name = "";
		SourceFileName = "";
		DestFileName = "";
		Size = ( DWORD )-1;
		MD5Sum = "";
		FileHandle = INVALID_HANDLE_VALUE;
	}

	FileDescCommon( INT ID, string RootFolder, string InName, DWORD InSize, string InMD5Sum );

	FileDescCommon( const FileDescCommon* Copy )
	{
		UniqueID = Copy->UniqueID;
		Name = Copy->Name;
		SourceFileName = Copy->SourceFileName;
		DestFileName = Copy->DestFileName;
		Size = Copy->Size;
		MD5Sum = Copy->MD5Sum;
		FileHandle = INVALID_HANDLE_VALUE;
	}

	virtual ~FileDescCommon( void );

	void Create( void );
	void Write( void* Buffer, DWORD Length );
	void Close( void );
	void Copy( void );
	bool Resident( void );
};

class PluginCommon
{
private:
	void InitCacheFolder( void );

protected:
	HANDLE						ContextHandle;
	ULONG_PTR					ContextToken;
	HANDLE						ThreadHandle;
	DWORD						PluginThreadId;

	string						RootFolder;
	map<string, string>			Folders;

	bool						DebugMode;

	INT							Width;
	INT							Height;

	FileDescCommon*				NextFile;
	queue<FileDescCommon*>		FilesToDownLoad;
	map<int, FileDescCommon*>	CurrentDownloadingFiles;

public:
	EPluginStage				PluginStage;
	HMODULE						Module;
	HWND						PIBWindow;
	FPIB*						IPIB;

	PluginCommon( void );

	void Destroy( void );
	void InitTickLoop( LPTHREAD_START_ROUTINE TickLoop, void* Parameter );
	void ParseFileList( const char* ManifestFileName );
	void CreateAllFolders( void );
	bool InitDLL( void );

	bool IsDownloading( void )
	{ 
		return( CurrentDownloadingFiles.size() > 0 ); 
	}
};

#endif // __PIBCOMMON_H__
