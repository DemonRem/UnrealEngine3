/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#include "Windows.h"
#include "Windowsx.h"
#include <Psapi.h>
#include <stdio.h>

#include "resource.h"
#include "../../../Src/Launch/Inc/PIB.h"

int Width = 800;
int Height = 1200;

bool bIsRunning = false;
char LaunchName[MAX_PATH] = { 0 };
FPIB* IPIB = NULL;

void ExtractParameters( const char* CommandLine )
{
	// Get the module that was launched
	GetModuleBaseName( GetCurrentProcess(), NULL, LaunchName, MAX_PATH );

	_strlwr_s( LaunchName, MAX_PATH );
	char* Extension = strstr( LaunchName, "launcher.exe" );
	if( Extension )
	{
		*Extension = 0;
	}

	// Extract any command line parameters
	const char* WidthParm = strstr( CommandLine, "width=" );
	if( WidthParm )
	{
		Width = atol( WidthParm + strlen( "width=" ) );
	}

	const char* HeightParm = strstr( CommandLine, "height=" );
	if( HeightParm )
	{
		Height = atol( HeightParm + strlen( "height=" ) );
	}

	// Clamp to minimal values
	if( Width < 320 )
	{
		Width = 320;
	}
	if( Height < 240 )
	{
		Height = 240;
	}
}

LRESULT CALLBACK FakeWindowProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	RECT rc = { 0 };

	switch( Message )
	{
	case WM_CREATE:
		GetWindowRect( hWnd, &rc ) ;
		SetWindowPos( hWnd, 0, ( GetSystemMetrics( SM_CXSCREEN ) - rc.right ) / 2, ( GetSystemMetrics( SM_CYSCREEN ) - rc.bottom ) / 2,	0, 0, SWP_NOZORDER | SWP_NOSIZE );
		return( 0 );

	case WM_PAINT:
		// Tick the game thread
		if( IPIB )
		{
			IPIB->Tick();
			return( 1 );
		}
		break;
	}

	return( DefWindowProc( hWnd, Message, wParam, lParam ) );
}

HWND CreateFakeWindow( HINSTANCE Instance )
{
	WNDCLASSEX WindowClass =
	{
		sizeof( WNDCLASSEX ),
		CS_OWNDC,
		FakeWindowProc,
		0,
		0,
		Instance,
		LoadIcon( Instance, MAKEINTRESOURCE( IDI_ICON1 ) ),
		NULL,
		NULL,
		NULL,
		"PIBClass",
		NULL
	};

	RegisterClassEx( &WindowClass );
	HWND Window = CreateWindow( "PIBClass", "DLLTest", WS_POPUP | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, Instance, NULL );

	return( Window );
}

bool LaunchDLL( HWND PIBWindow )
{
	HANDLE ContextHandle = INVALID_HANDLE_VALUE;
	ULONG ContextToken = 0;
	HMODULE Module = NULL;

	ACTCTX Context = { sizeof( ACTCTX ), 0 };

	// The manifest of the binary to load
	wchar_t CommandLine[MAX_PATH] = { 0 };

	char Manifest[MAX_PATH] = { 0 };
	char Binary[MAX_PATH] = { 0 };

	swprintf_s( CommandLine, MAX_PATH, L"-ResX=%d -ResY=%d -PosX=0 -PosY=0 -logtimes -buildmachine -seekfreeloadingpcconsole", Width, Height );
	sprintf_s( Manifest, MAX_PATH, "%s.dll.manifest", LaunchName );
	sprintf_s( Binary, MAX_PATH, "%s.dll", LaunchName );

	Context.lpSource = Manifest;
	ContextHandle = CreateActCtx( &Context );

	if( ContextHandle != INVALID_HANDLE_VALUE )
	{
		if( ActivateActCtx( ContextHandle, &ContextToken ) )
		{
			Module = LoadLibrary( Binary );
			if( Module )
			{
				TPIBGetInterface PIBGetInterface = ( TPIBGetInterface )GetProcAddress( Module, "PIBGetInterface" );
				if( PIBGetInterface )
				{
					IPIB = PIBGetInterface( 0 );
					if( IPIB )
					{
						if( IPIB->Init( Module, PIBWindow, CommandLine ) != 0 )
						{
							return( true );
						}
						else
						{
							MessageBox( NULL, "Failed to initialise!", "DLLLauncher Error", MB_ICONERROR );
						}
					}
					else
					{
						MessageBox( NULL, "Failed to get get interface!", "DLLLauncher Error", MB_ICONERROR );
					}
				}
				else
				{
					MessageBox( NULL, "Failed to get GetProcAddress!", "DLLLauncher Error", MB_ICONERROR );
				}
			}
			else
			{
				MessageBox( NULL, "Failed to load module!", "DLLLauncher Error", MB_ICONERROR );
			}
		}
		else
		{
			MessageBox( NULL, "Failed to activate context!", "DLLLauncher Error", MB_ICONERROR );
		}
	}
	else
	{
		MessageBox( NULL, "Invalid context handle! Is the working folder correct?", "DLLLauncher Error", MB_ICONERROR );
	}

	return( false );
}

INT WINAPI WinMain( HINSTANCE Instance, HINSTANCE, char* CommandLine, INT )
{
	ExtractParameters( CommandLine );

	HWND PIBWindow = CreateFakeWindow( Instance );

	if( LaunchDLL( PIBWindow ) )
	{
		bIsRunning = true;

		ShowWindow( PIBWindow, SW_SHOWNORMAL );

		MSG Msg = { 0 };
		while( bIsRunning )
		{
			InvalidateRect( PIBWindow, NULL, FALSE );

			while( bIsRunning && PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
			{
				if( Msg.message == WM_QUIT )
				{
					bIsRunning = false;
				}

				TranslateMessage( &Msg );
				DispatchMessage( &Msg );
			}
		}

		CloseWindow( PIBWindow );
		UnregisterClass( "PIBClass", Instance );

		IPIB->Shutdown();
		return( 0 );
	}

	return( 1 );
}
