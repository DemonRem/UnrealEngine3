/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "stdafx.h"
#include "ATLUE3Control.h"

FileDescAX::FileDescAX( const FileDescCommon* Copy, HINTERNET InConnection, HINTERNET InRequest )
	: FileDescCommon( Copy )
{
	Connection = InConnection;
	Request = InRequest;

	// Initialise the threading lock
	if( !InitializeCriticalSectionAndSpinCount( &Lock, 1000 ) )
	{
		UniqueID = -1;
	}
	
	LockInitialised = true;

	// Create an event for this object to be 'finished'
	RequestFinished = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( RequestFinished == NULL )
	{
		UniqueID = -1;
	}
}

FileDescAX::FileDescAX( INT ID, string RootFolder, string InName, DWORD InSize, string InMD5Sum )
: FileDescCommon( ID, RootFolder, InName, InSize, InMD5Sum )
{
}

FileDescAX::~FileDescAX( void )
{
	LockRequestHandle();

	Close();

	HINTERNET TempRequest = Request;
	Request = INVALID_HANDLE_VALUE;

	if( TempRequest != INVALID_HANDLE_VALUE )
	{
		WinHttpCloseHandle( TempRequest );
	}

	HINTERNET TempConnection = Connection;
	Connection = INVALID_HANDLE_VALUE;

	if( TempConnection != INVALID_HANDLE_VALUE )
	{
		WinHttpCloseHandle( TempConnection );
	}

	UnlockRequestHandle();

	if( LockInitialised )
	{
		DeleteCriticalSection( &Lock );
	}

	if( RequestFinished != INVALID_HANDLE_VALUE )
	{
		CloseHandle( RequestFinished );
	}
}

bool FileDescAX::LockRequestHandle( void )
{
	if( LockInitialised )
	{
		EnterCriticalSection( &Lock );

		if( Request )
		{
			return( true );
		}

		// Request handle is gone already, no point in trying to use it.
		LeaveCriticalSection( &Lock );
	}

	return( false );
}

void FileDescAX::UnlockRequestHandle( void )
{
	if( LockInitialised )
	{
		LeaveCriticalSection( &Lock );
	}
}

void FileDescAX::Cancel( void )
{
	LockRequestHandle();

	Close();

	HINTERNET TempRequest = Request;
	Request = NULL;

	WinHttpCloseHandle( TempRequest );

	UnlockRequestHandle();
}

bool CATLUE3Control::InitSession( void )
{
	Session = WinHttpOpen( L"PIB/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC );
	if( Session != INVALID_HANDLE_VALUE )
	{
		if( WinHttpSetStatusCallback( Session, ( WINHTTP_STATUS_CALLBACK )CATLUE3Control::AsyncCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 ) != WINHTTP_INVALID_STATUS_CALLBACK ) 
		{
			return( true );
		}
	}

	return( false );
}

void CATLUE3Control::DestroySession( void )
{
	if( Session != INVALID_HANDLE_VALUE )
	{
		WinHttpCloseHandle( Session );
		Session = INVALID_HANDLE_VALUE;
	}
}

bool CATLUE3Control::HeadersAvailable( FileDescAX* ReqFile )
{
	DWORD StatusCode = 0;
	DWORD StatusCodeLength = sizeof( StatusCode );

	if( !WinHttpQueryHeaders( ReqFile->Request, WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE, NULL, &StatusCode, &StatusCodeLength, NULL ) ) 
	{
		return( false );
	}

	if( !WinHttpReadData( ReqFile->Request, ReqFile->Buffer, sizeof( ReqFile->Buffer ), NULL ) )
	{
		return( false );
	}

	ReqFile->Create();
	return( true );
}

bool CATLUE3Control::ReadComplete( FileDescAX* ReqFile, DWORD StatusInformationLength )
{
	if( StatusInformationLength != 0 )
	{
		ReqFile->Write( ReqFile->Buffer, StatusInformationLength );

		if( !WinHttpReadData( ReqFile->Request, ReqFile->Buffer, sizeof( ReqFile->Buffer ), NULL ) )
		{
			return( false );
		}
	}
	else
	{
		ReqFile->Close();

		SetEvent( ReqFile->RequestFinished );
	}

	return( true );
}

VOID CALLBACK CATLUE3Control::AsyncCallback( HINTERNET, DWORD_PTR Context, DWORD InternetStatus, LPVOID, DWORD StatusInformationLength )
{
	FileDescAX* ReqFile = ( FileDescAX* )Context;
	if( ReqFile )
	{
		if( InternetStatus != WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING && InternetStatus != WINHTTP_CALLBACK_STATUS_REQUEST_ERROR )
		{
			ReqFile->LockRequestHandle();
		}

		switch( InternetStatus )
		{
		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			OutputDebugString( " ... send request complete\r\n" );
			WinHttpReceiveResponse( ReqFile->Request, NULL );
			break;

		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
			OutputDebugString( " ... headers available\r\n" );
			HeadersAvailable( ReqFile );
			break;

		case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
			ReadComplete( ReqFile, StatusInformationLength );
			break;

		case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
			OutputDebugString( " ... handle closing\r\n" );
			break;

		case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
			OutputDebugString( " ... request error\r\n" );
			break;
		}

		ReqFile->UnlockRequestHandle();
	}
}

FileDescAX* CATLUE3Control::InitiateConnection( const wchar_t* Server, const wchar_t* URL, INTERNET_PORT Port )
{
	HINTERNET Connection = WinHttpConnect( Session, Server, Port, 0 );

	PCWSTR AcceptAllTypes[] = { L"*/*", NULL };

	HINTERNET Request = WinHttpOpenRequest( Connection, L"GET", URL, NULL, WINHTTP_NO_REFERER, AcceptAllTypes, 0 );
	if( !Request )
	{
		OutputDebugString( "Failed to open request" );
		return( NULL );	
	}

	FileDescAX* ReqFile = new FileDescAX( NextFile, Connection, Request );
	delete NextFile;
	NextFile = NULL;

	if( ReqFile->UniqueID < 0 )
	{
		OutputDebugString( "Failed to init file request" );
		return( NULL );
	}

	ReqFile->LockRequestHandle();

	if( !WinHttpSetOption( ReqFile->Request, WINHTTP_OPTION_CONTEXT_VALUE, &ReqFile, sizeof( FileDescAX* ) ) )
	{
		return( NULL );
	}

	WinHttpSendRequest( ReqFile->Request, NULL, 0, NULL, 0, 0, 0 );

	ReqFile->UnlockRequestHandle();

	return( ReqFile );
}

FileDescAX* CATLUE3Control::InitiateConnection( const wchar_t* Server, const char* URL, INTERNET_PORT Port )
{
	wchar_t WideCharURL[MAX_PATH] = { 0 };
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, URL, strlen( URL ), WideCharURL, MAX_PATH );
	return( InitiateConnection( Server, WideCharURL, Port ) );
}

void CATLUE3Control::GetFileList( void )
{
	NextFile = new FileDescCommon( 0, RootFolder, string( "Files.Xml" ), 0, string( "0" ) );
	FileList = InitiateConnection( L"PIB", L"/PIBFileWebServices/GetFileList/1", 1805 );
	PluginStage = Stage_WaitingForFileList;
}

void CATLUE3Control::WaitForFileList( void )
{
	if( WaitForSingleObject( FileList->RequestFinished, INFINITE ) == WAIT_FAILED )
	{
		return;
	}

	ParseFileList( FileList->SourceFileName.c_str() );

	delete FileList;
	FileList = NULL;

	CreateAllFolders();

	PluginStage = Stage_GetFiles;
}

void CATLUE3Control::RequestFile( void )
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

			string FileRequestURL = "/data/LatestBuild/UnrealEngine3/" + NextFile->Name;
			FileDescAX* ReqFile = InitiateConnection( L"PIB", FileRequestURL.c_str(), INTERNET_DEFAULT_HTTP_PORT );

			CurrentDownloadingFiles[ReqFile->UniqueID] = ReqFile;
			PluginStage = Stage_WaitForFiles;

			string Message = string( " ... requesting non resident file: " ) + ReqFile->Name + string( "\r\n" );
			OutputDebugString( Message.c_str() );
		}
	}
}

void CATLUE3Control::WaitForFiles( void )
{
	// Iterate over all the downloading files to see if any have completed
	for( map<int, FileDescCommon*>::iterator It = CurrentDownloadingFiles.begin(); It != CurrentDownloadingFiles.end(); It++ )
	{
		FileDescAX* ReqFile = ( FileDescAX* )( *It ).second;
		if( WaitForSingleObject( ReqFile->RequestFinished, 40 ) == WAIT_OBJECT_0 )
		{
			ReqFile->Copy();

			delete ReqFile;
			CurrentDownloadingFiles.erase( It );
			break;
		}
	}

	// Check for this being the last file
	if( PluginStage == Stage_WaitForAllFiles && CurrentDownloadingFiles.size() == 0 )
	{
		PluginStage = Stage_InitDLL;
	}
	else
	{
		PluginStage = Stage_GetFiles;
	}
}


