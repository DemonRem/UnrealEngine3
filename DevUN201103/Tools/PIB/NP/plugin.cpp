/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#include "plugin.h"

//
// general initialization and shutdown
//
NPError NS_PluginInitialize( void )
{
	return( NPERR_NO_ERROR );
}

void NS_PluginShutdown( void )
{
}

//
// construction and destruction of our plugin instance object
//
nsPluginInstanceBase* NS_NewPluginInstance( nsPluginCreateData* aCreateDataStruct )
{
	if( !aCreateDataStruct )
	{
		return( NULL );
	}

	// Create a new instance of the plugin
	NPUE3PluginInstance* plugin = new NPUE3PluginInstance( aCreateDataStruct->instance );

	// Extract the parameters defined via html
	plugin->ExtractParameters( aCreateDataStruct );

	return( plugin );
}

void NS_DestroyPluginInstance( nsPluginInstanceBase* aPlugin )
{
	if( aPlugin )
	{
		delete ( NPUE3PluginInstance* )aPlugin;
	}
}

//
// NPUE3PluginInstance class implementation
//
NPUE3PluginInstance::NPUE3PluginInstance( NPP aInstance ) :
	nsPluginInstanceBase(),
	PluginCommon(),
	OldProc( NULL ),
	PluginInstance( aInstance )
{
}

NPUE3PluginInstance::~NPUE3PluginInstance( void )
{
}

void NPUE3PluginInstance::Destroy( void )
{
	// Subclass WndProc back
	if( OldProc )
	{
		SubclassWindow( PIBWindow, OldProc );
	}

	PluginCommon::Destroy();
}

static LRESULT CALLBACK PluginWinProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	NPUE3PluginInstance* Plugin = ( NPUE3PluginInstance* )GetWindowLong( hWnd, GWL_USERDATA );

	switch( Message )
	{
	case WM_ERASEBKGND: 
		return( 1 );

	case WM_PAINT:
		// Tick the game thread
		if( Plugin->PluginStage == Stage_Running )
		{
			Plugin->IPIB->Tick();
			return( 1 );
		}
		break;

	case WM_PIB_GETURL:
		NPN_GetURL( Plugin->PluginInstance, ( const char* )lParam, NULL );
		Plugin->PluginStage = ( EPluginStage )wParam;
		break;

	case WM_PIB_INITDLL:
		Plugin->InitDLL();
		break;
	}

	return( CallWindowProc( Plugin->OldProc, hWnd, Message, wParam, lParam ) );
}

DWORD __stdcall TickLoop( void* Parameter )
{
	NPUE3PluginInstance* Plugin = ( NPUE3PluginInstance* )Parameter;

	while( Plugin->PluginStage != Stage_Exiting )
	{
		switch( Plugin->PluginStage )
		{
		case Stage_GetFileList:
			SendMessage( Plugin->PIBWindow, WM_PIB_GETURL, Stage_WaitingForFileList, ( LPARAM )"http://pib:1805/PIBFileWebServices/GetFileList/1" );
			break;

		case Stage_WaitingForFileList:
			Sleep( 100 );
			break;

		case Stage_GetFiles:
			Plugin->RequestFile();
			Sleep( 100 );
			break;

		case Stage_WaitForFiles:
			Sleep( 100 );
			break;

		case Stage_WaitForAllFiles:
			if( Plugin->IsDownloading() == 0 )
			{
				Plugin->PluginStage = Stage_InitDLL;
			}

			Sleep( 100 );
			break;

		case Stage_InitDLL:
			SendMessage( Plugin->PIBWindow, WM_PIB_INITDLL, 0, 0 );
			break;

		case Stage_Running:
			// Tick the game thread
			while( Plugin->PluginStage != Stage_Exiting )
			{
				InvalidateRect( Plugin->PIBWindow, NULL, FALSE );
				Sleep( 15 );
			}
			break;
		}
	}

	return( 1 );
}

void NPUE3PluginInstance::InitTickLoop( void )
{
	PluginCommon::InitTickLoop( &TickLoop, this );

	// subclass window so we can intercept window messages and do our drawing to it
	OldProc = SubclassWindow( PIBWindow, ( WNDPROC )PluginWinProc );

	// Associate window with our NPUE3PluginInstance object so we can access it in the window procedure
	SetWindowLong( PIBWindow, GWL_USERDATA, ( LONG )this );
}

void NPUE3PluginInstance::ExtractParameters( nsPluginCreateData* CreateData )
{
	for( int ParmIndex = 0; ParmIndex < CreateData->argc; ParmIndex++ )
	{
		if( !_stricmp( CreateData->argn[ParmIndex], "width" ) )
		{
			Width = atol( CreateData->argv[ParmIndex] );
		}
		else if( !_stricmp( CreateData->argn[ParmIndex], "height" ) )
		{
			Height = atol( CreateData->argv[ParmIndex] );
		}	
		else if( !_stricmp( CreateData->argn[ParmIndex], "debug" ) )
		{
			DebugMode = true;
		}	
	}
}

NPBool NPUE3PluginInstance::init( NPWindow* aWindow )
{
	if( aWindow == NULL )
	{
		return( FALSE );
	}

	PIBWindow = ( HWND )aWindow->window;
	if( PIBWindow == NULL )
	{
		return( FALSE );
	}

	InitTickLoop();

	return( TRUE );
}

void NPUE3PluginInstance::InitDLL( void )
{
	if( !PluginCommon::InitDLL() )
	{
		shut();
	}
}

void NPUE3PluginInstance::shut( void )
{
	PluginStage = Stage_Exiting;

	OutputDebugString( "Waiting for tick loop to finish" );
	WaitForSingleObject( ThreadHandle, INFINITE );

	OutputDebugString( "Destroying plugin instance" );
	Destroy();
}

NPBool NPUE3PluginInstance::isInitialized( void )
{
	return( PluginStage > Stage_Starting );
}

NPError NPUE3PluginInstance::NewStream( NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype )
{
	assert( GetCurrentThreadId() == PluginThreadId );

	// Always stream - using files in the cache is flaky and has a 64MB hard coded limit
	*stype = NP_NORMAL;

	switch( PluginStage )
	{
	case Stage_WaitingForFileList:
		stream->pdata = ( void* )( new FileDescNP( 0, RootFolder, string( "Files.Xml" ), 0, string( "0" ) ) );
		break;

	case Stage_WaitForFiles:
		{
			// NextFile is the name of this file we're trying to download
			FileDescNP* ReqFile = new FileDescNP( NextFile );
			CurrentDownloadingFiles[ReqFile->UniqueID] = ReqFile;
			stream->pdata = ( void* )ReqFile;

			delete NextFile;
			NextFile = NULL;

			PluginStage = Stage_GetFiles;
		}
		break;

	default:
		assert( 0 );
		break;
	}

	return( NPERR_NO_ERROR ); 
}

int32 NPUE3PluginInstance::WriteReady( NPStream* stream )
{
	return( DOWNLOAD_CHUNK_SIZE );
}

int32 NPUE3PluginInstance::Write( NPStream* stream, int32 offset, int32 len, void* buffer )
{
	assert( GetCurrentThreadId() == PluginThreadId );

	FileDescNP* ReqFile = ( FileDescNP* )stream->pdata;

	// Create the file if it doesn't exist
	ReqFile->Create();

	// Write out the next chunk of data
	ReqFile->Write( buffer, ( DWORD )len );

	return( len );
}

NPError NPUE3PluginInstance::DestroyStream( NPStream* stream, NPError reason )
{
	assert( GetCurrentThreadId() == PluginThreadId );

	FileDescNP* ReqFile = ( FileDescNP* )stream->pdata;
	ReqFile->Close();

	switch( PluginStage )
	{
	case Stage_WaitingForFileList:
		ParseFileList( ReqFile->SourceFileName.c_str() );

		delete ReqFile;

		CreateAllFolders();
		PluginStage = Stage_GetFiles;
		break;

	case Stage_GetFiles:
	case Stage_WaitForFiles:
	case Stage_WaitForAllFiles:
		{
			map<int, FileDescCommon*>::iterator It = CurrentDownloadingFiles.find( ReqFile->UniqueID );
			if( It != CurrentDownloadingFiles.end() )
			{
				ReqFile->Copy();

				delete ReqFile;
				CurrentDownloadingFiles.erase( It );
			}
		}
		break;

	default:
		assert( 0 );
		break;
	}

	return( NPERR_NO_ERROR );
}

// end
