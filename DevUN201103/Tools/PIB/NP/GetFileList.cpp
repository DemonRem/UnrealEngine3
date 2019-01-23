/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "plugin.h"

FileDescNP::FileDescNP( const FileDescCommon* Copy ) :
	FileDescCommon( Copy )
{
}

FileDescNP::FileDescNP( INT ID, string RootFolder, string InName, DWORD InSize, string InMD5Sum )
	: FileDescCommon( ID, RootFolder, InName, InSize, InMD5Sum )
{
}

FileDescNP::~FileDescNP( void )
{
}

// Check to see if we already have the file locally, and request it if we don't
void NPUE3PluginInstance::RequestFile( void )
{
	// No files to download
	if( FilesToDownLoad.size() == 0 )
	{
		PluginStage = Stage_WaitForAllFiles;
		OutputDebugString( "Finished requesting files\r\n" );
		return;
	}

	// Throttle downloading to 3 files at a time
	if( CurrentDownloadingFiles.size() < 3 )
	{
		NextFile = FilesToDownLoad.front();
		FilesToDownLoad.pop();

		if( NextFile->Resident() )
		{
			string Message = string( " ... file already resident: " ) + NextFile->Name + string( "\r\n" );
			OutputDebugString( Message.c_str() );

			NextFile->Copy();
		}
		else
		{
			// Make sure the temp file and the final destination file no longer exist
			DeleteFile( NextFile->SourceFileName.c_str() );

			string FileRequestURL = "http://pib/data/LatestBuild/UnrealEngine3/" + NextFile->Name;
			SendMessage( PIBWindow, WM_PIB_GETURL, Stage_WaitForFiles, ( LPARAM )FileRequestURL.c_str() );

			string Message = string( " ... requesting non resident file: " ) + NextFile->Name + string( "\r\n" );
			OutputDebugString( Message.c_str() );
		}
	}
}

// end
