/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "PIBCommon.h"

#define RUN_LOCALLY		0

// Container for a file description
FileDescCommon::FileDescCommon( INT ID, string RootFolder, string InName, DWORD InSize, string InMD5Sum )
{
	UniqueID = ID;
	Size = InSize;
	MD5Sum = InMD5Sum;
	FileHandle = INVALID_HANDLE_VALUE;

	// e.g. Binaries\Win32\UDKGameDLL.dll
	Name = InName;

	// e.g. C:\Users\john.scott\AppData\Local\Temp\PIB\Cache\01234567-0123-0123-0123-0123456789abcdef
	SourceFileName = RootFolder + string( "Cache" ) + "\\" + MD5Sum;

	// e.g. C:\Users\john.scott\AppData\Local\Temp\PIB\UnrealEngine3\Binaries\Win32\UDKGameDLL.dll
	DestFileName = RootFolder + string( "UnrealEngine3" ) + "\\" + Name;
}

FileDescCommon::~FileDescCommon( void )
{
	assert( FileHandle == INVALID_HANDLE_VALUE );
}

void FileDescCommon::Create( void )
{
	if( FileHandle == INVALID_HANDLE_VALUE )
	{
		FileHandle = ::CreateFile( SourceFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL );
		assert( FileHandle != INVALID_HANDLE_VALUE );
	}
}

void FileDescCommon::Write( void* Buffer, DWORD Length )
{
	assert( FileHandle != INVALID_HANDLE_VALUE );
	
	DWORD Written = 0;
	::WriteFile( FileHandle, Buffer, Length, &Written, NULL );

	assert( Written == Length );
}

void FileDescCommon::Close( void )
{
	if( FileHandle != INVALID_HANDLE_VALUE )
	{
		::CloseHandle( FileHandle );
		FileHandle = INVALID_HANDLE_VALUE;
	}
}

void FileDescCommon::Copy( void )
{
#if !RUN_LOCALLY
	if( !::CopyFile( SourceFileName.c_str(), DestFileName.c_str(), FALSE ) )
	{
		OutputDebugStringA( " *** COPY FAILED!!!!!!!!!!\r\n" );
	}
#endif
}

// Checks to see if a file exists locally
bool FileDescCommon::Resident( void )
{
	DWORD FileSize = ( DWORD )-1;

	HANDLE FileHandle = ::CreateFile( SourceFileName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( FileHandle != INVALID_HANDLE_VALUE )
	{
		// Do a quick size check
		FileSize = GetFileSize( FileHandle, NULL );

		CloseHandle( FileHandle );
	}

	return( FileSize == Size );
}

PluginCommon::PluginCommon( void ) :
	PluginStage( Stage_Starting ),
	Module( NULL ),
	PIBWindow( NULL ),
	IPIB( NULL ),

	ContextHandle( INVALID_HANDLE_VALUE ),
	ContextToken( NULL ),
	ThreadHandle( INVALID_HANDLE_VALUE ),
	PluginThreadId( 0 ),

	DebugMode( false ),

	Width( 800 ),
	Height( 600 )
{
	// Set up the root folder to somewhere always writable by less privileged users
	char TempPath[MAX_PATH] = { 0 };
	GetTempPath( MAX_PATH, TempPath );
	strcat_s( TempPath, MAX_PATH, "PIB\\" );
	RootFolder = TempPath;
#if RUN_LOCALLY
	RootFolder = "d:\\depot\\";
#endif
	InitCacheFolder();
}

// Make sure the required folders exist and the cache folder is empty
void PluginCommon::InitCacheFolder( void )
{
	// Ensure the RootFolder exists
	CreateDirectory( RootFolder.c_str(), NULL );

	// Ensure the cache folder exists
	string CacheFolder = RootFolder + string( "Cache" );
	CreateDirectory( CacheFolder.c_str(), NULL );

	// Ensure the build folder exists
	string BuildFolder = RootFolder + string( "UnrealEngine3" );
	CreateDirectory( BuildFolder.c_str(), NULL );

	//FIXME: Need to delete the existing build folder here to ensure deleted files are deleted
	//FIXME: What about game specific files
}

// Ensure the folder structure exists
void PluginCommon::CreateAllFolders( void )
{
	for( map<string, string>::iterator It = Folders.begin(); It != Folders.end(); It++ )
	{
		string FullPath = string( RootFolder ) + string( "UnrealEngine3\\" ) + ( *It ).second;
		CreateDirectory( FullPath.c_str(), NULL );
	}
}

// Start an alternate thread to tick the engine
void PluginCommon::InitTickLoop( LPTHREAD_START_ROUTINE TickLoop, void* Parameter )
{
	// Make sure all operations run on the same thread
	PluginThreadId = GetCurrentThreadId();

	// Create a thread to handle the initialisation
	DWORD ThreadId = 0;
#if RUN_LOCALLY
	PluginStage = Stage_InitDLL;
#else
	Stage_GetFileList;
#endif

	ThreadHandle = CreateThread( NULL, 0, TickLoop, Parameter, 0, &ThreadId );
}

// Parse the xml retrieved from the server to extract info about all required files
void PluginCommon::ParseFileList( const char* ManifestFileName )
{
	int NextFileIndex = 1;

	OutputDebugString( "Parsing manifest...\r\n" );

	TiXmlDocument ManifestDocument( "Manifest" );

	if( ManifestDocument.LoadFile( ManifestFileName ) )
	{
		// First child element is 'GetFileList'
		TiXmlHandle RootNode = ManifestDocument.FirstChildElement();

		// Find the 'Files' element
		TiXmlElement* FileElement = RootNode.FirstChild( "Files" ).ToElement();
		while( FileElement )
		{
			string FileName = FileElement->Attribute( "Name" );
			string MD5Sum = FileElement->Attribute( "MD5Sum" );

			int Size = -1;
			FileElement->QueryIntAttribute( "Size", &Size );

			// Convert all \ to /
			for( string::iterator It = FileName.begin(); It != FileName.end(); It++ )
			{
				if( ( *It ) == '\\' )
				{
					( *It ) = '/';
				}
			}

			FileDescCommon* Desc = new FileDescCommon( NextFileIndex++, RootFolder, FileName, Size, MD5Sum );
			FilesToDownLoad.push( Desc );

			// Extract the folders
			size_t LastOffset = 0;
			size_t Offset = FileName.find_first_of( '/', LastOffset );
			while( Offset != string::npos )
			{
				string Folder = FileName.substr( 0, Offset );
				if( Folders.find( Folder ) == Folders.end() )
				{
					Folders[Folder] = Folder;
				}

				LastOffset = Offset + 1;
				Offset = FileName.find_first_of( '/', LastOffset );
			}

			FileElement = FileElement->NextSiblingElement();
		}
	}
}

bool PluginCommon::InitDLL( void )
{
	ACTCTX Context = { sizeof( ACTCTX ), 0 };
	wchar_t CommandLine[MAX_PATH] = { 0 };
	char WorkingFolder[MAX_PATH] = { 0 };
	char Manifest[MAX_PATH] = { 0 };
	char Binary[MAX_PATH] = { 0 };

	// Set the working folder to where the binary exists so it can find its dependent dlls
	sprintf( WorkingFolder, "%sUnrealEngine3\\Binaries\\Win32", RootFolder.c_str() );
	SetCurrentDirectory( WorkingFolder );

	// Work out the name of the DLL to load and its associate manifest file
	if( DebugMode )
	{
		sprintf( Manifest, "%s\\%s", WorkingFolder, "DEBUG-UDKGameDLL.dll.manifest" );
		sprintf( Binary, "%s\\%s", WorkingFolder, "DEBUG-UDKGameDLL.dll" );
	}
	else
	{
		sprintf( Manifest, "%s\\%s", WorkingFolder, "UDKGameDLL.dll.manifest" );
		sprintf( Binary, "%s\\%s", WorkingFolder, "UDKGameDLL.dll" );
	}

	// Create an activation context to load the UDKGame dll into
	OutputDebugString( " ... creating activation context\r\n" );
	Context.lpSource = Manifest;
	ContextHandle = CreateActCtx( &Context );
	if( ContextHandle != INVALID_HANDLE_VALUE )
	{
		OutputDebugString( " ... activating activation context\r\n" );
		if( ActivateActCtx( ContextHandle, &ContextToken ) )
		{
			OutputDebugString( " ... loading dll\r\n" );
			Module = LoadLibrary( Binary );
			if( Module )
			{
				OutputDebugString( " ... getting proc address\r\n" );
				TPIBGetInterface PIBGetInterface = ( TPIBGetInterface )GetProcAddress( Module, "PIBGetInterface" );
				if( PIBGetInterface )
				{
					IPIB = PIBGetInterface( 0 );

					OutputDebugString( " ... initialising dll\r\n" );
					swprintf_s( CommandLine, MAX_PATH, L"-ResX=%d -ResY=%d -PosX=0 -PosY=0 -logtimes -buildmachine -seekfreeloadingpcconsole", Width, Height );

					if( IPIB->Init( Module, PIBWindow, CommandLine ) != 0 )
					{
						PluginStage = Stage_Running;
						OutputDebugString( " ... running\r\n" );
						return( true );
					}
					else
					{
						OutputDebugString( " *** FAILED TO INITIALISE DLL!\r\n" );
					}
				}
				else
				{
					OutputDebugString( " *** FAILED TO GPA ON PIBGetInterface!\r\n" );
				}
			}
			else
			{
				OutputDebugString( " *** FAILED TO LOAD DLL!\r\n" );
			}
		}
		else
		{
			OutputDebugString( " *** FAILED TO ACTIVATE ACTIVATION CONTEXT!\r\n" );
		}
	}
	else
	{
		OutputDebugString( " *** FAILED TO CREATE ACTIVATION CONTEXT - PROBABLY A MISSING MANIFEST!\r\n" );
	}

	return( false );
}

void PluginCommon::Destroy( void )
{
	// Kill all currently downloading files
	for( map<int, FileDescCommon*>::iterator It = CurrentDownloadingFiles.begin(); It != CurrentDownloadingFiles.end(); It ++ )
	{
		FileDescCommon* ReqFile = ( FileDescCommon* )( *It ).second;
		( *It ).second = NULL;

		ReqFile->Close();
		delete ReqFile;
	}

	// Shutdown the dll
	if( IPIB )
	{
		IPIB->Shutdown();
		IPIB = NULL;
	}

	// Close the tick loop thread
	if( ThreadHandle )
	{
		CloseHandle( ThreadHandle );
		ThreadHandle = NULL;
	}

	// Deactivate the activation context
	if( ContextToken )
	{
		DeactivateActCtx( 0, ContextToken );
		ContextToken = NULL;
	}

	// Release the activation context
	if( ContextHandle )
	{
		ReleaseActCtx( ContextHandle );
		ContextHandle = NULL;
	}
}