/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "LaunchPrivate.h"
// use wxWidgets as a DLL
#include <wx/evtloop.h>  // has our base callback class

FEngineLoop	GEngineLoop;
/** Whether to use wxWindows when running the game */
UBOOL		GUsewxWindows;

#if STATS
/** Global for tracking FPS */
FFPSCounter GFPSCounter;
#endif 
 
/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

extern TCHAR MiniDumpFilenameW[64];
extern char  MiniDumpFilenameA[64];
extern INT CreateMiniDump( LPEXCEPTION_POINTERS ExceptionInfo );

// use wxWidgets as a DLL
extern bool IsUnrealWindowHandle( HWND hWnd );

class WxUnrealCallbacks : public wxEventLoopBase::wxUnrealCallbacks
{
public:

	virtual bool IsUnrealWindowHandle(HWND hwnd) const
	{
		return ::IsUnrealWindowHandle(hwnd);
	}

	virtual bool IsRequestingExit() const 
	{ 
		return GIsRequestingExit ? true : false; 
	}

	virtual void SetRequestingExit( bool bRequestingExit ) 
	{ 
		GIsRequestingExit = bRequestingExit ? true : false; 
	}

};

static WxUnrealCallbacks s_UnrealCallbacks;


/**
 * Performs any required cleanup in the case of a fatal error.
 */
static void StaticShutdownAfterError()
{
	// Make sure Novodex is correctly torn down.
	DestroyGameRBPhys();

#if WITH_FACEFX
	// Make sure FaceFX is shutdown.
	UnShutdownFaceFX();
#endif // WITH_FACEFX

	// Unbind DLLs (e.g. SCC integration)
	WxLaunchApp* LaunchApp = (WxLaunchApp*) wxTheApp;
	if( LaunchApp )
	{
		LaunchApp->ShutdownAfterError();
	}
}

/**
 * Static guarded main function. Rolled into own function so we can have error handling for debug/ release builds depending
 * on whether a debugger is attached or not.
 */
static INT GuardedMain( const TCHAR* CmdLine, HINSTANCE hInInstance, HINSTANCE hPrevInstance, INT nCmdShow )
{
	// Set up minidump filename. We cannot do this directly inside main as we use an FString that requires 
	// destruction and main uses SEH.
	appStrcpy( MiniDumpFilenameW, *FString::Printf( TEXT("unreal-v%i-%s.dmp"), GEngineVersion, *appSystemTimeString() ) );	
	appStrcpyANSI( MiniDumpFilenameA, TCHAR_TO_ANSI( MiniDumpFilenameW ) );

	INT ErrorLevel	= GEngineLoop.PreInit( CmdLine );
	GUsewxWindows	= GIsEditor || ParseParam(appCmdLine(),TEXT("WXWINDOWS")) || ParseParam(appCmdLine(),TEXT("REMOTECONTROL"));

	if( GUsewxWindows && ErrorLevel == 0 && !GIsRequestingExit )
	{
		// use wxWidgets as a DLL
		// set the call back class here
		wxEventLoopBase::SetUnrealCallbacks( &s_UnrealCallbacks );

		// UnrealEd of game with wxWindows.
		ErrorLevel = wxEntry( hInInstance, hPrevInstance, "", nCmdShow);
	}
	else if( GIsUCC )
	{
		// UCC.
		UBOOL bInheritConsole = FALSE;

#if !CONSOLE
		if(NULL != GLogConsole)
		{
			// if we're running from a console we inherited, do not sleep indefinitely
			bInheritConsole = GLogConsole->IsInherited();
		}
#endif

		// Either close log window manually or press CTRL-C to exit if not in "silent" or "nopause" mode.
		UBOOL bShouldPause = !bInheritConsole && !GIsSilent && !ParseParam(appCmdLine(),TEXT("NOPAUSE"));
		// if it was specified to not pause if successful, then check that here
		if (ParseParam(appCmdLine(),TEXT("NOPAUSEONSUCCESS")) && ErrorLevel == 0)
		{
			// we succeeded, so don't pause 
			bShouldPause = FALSE;
		}

		// pause if we should
		if (bShouldPause)
		{
			GLog->TearDown();
			GEngineLoop.Exit();
			Sleep(INFINITE);
		}
	}
	else
	{
		// Game without wxWindows.
		if (!GIsRequestingExit)
		{
			FString SplashPath;
			if (appGetSplashPath(GIsEditor ? TEXT("PC\\EdSplash.bmp") : TEXT("PC\\Splash.bmp"), SplashPath) == TRUE)
			{
				appShowSplash( *SplashPath );
			}
			ErrorLevel = GEngineLoop.Init();
			appHideSplash();
		}
		while( !GIsRequestingExit )
		{
#if STATS
			GFPSCounter.Update(appSeconds());
#endif
			{
				SCOPE_CYCLE_COUNTER(STAT_FrameTime);
				GEngineLoop.Tick();
			}
#if STATS
			// Write all stats for this frame out
			GStatManager.AdvanceFrame();

			if(GIsThreadedRendering)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND(AdvanceFrame,{GStatManager.AdvanceFrameForThread();});
			}
#endif
		}
	}
	GEngineLoop.Exit();
	return ErrorLevel;
}

/**
 * Moved creation of the named mutex into a function so that the MutexName
 * array wasn't on the stack during the entire run of the game
 */
HANDLE MakeNamedMutex()
{
	TCHAR MutexName[MAX_SPRINTF]=TEXT("");
	appSprintf(MutexName, TEXT("UnrealEngine3_%d"), GAMENAME);
	return CreateMutex( NULL, TRUE, MutexName);
}

/**
* Handler for crt parameter validation. Triggers error
*
* @param Expression - the expression that failed crt validation
* @param Function - function which failed crt validation
* @param File - file where failure occured
* @param Line - line number of failure
* @param Reserved - not used
*/
void InvalidParameterHandler(const TCHAR* Expression,
							 const TCHAR* Function, 
							 const TCHAR* File, 
							 UINT Line, 
							 uintptr_t Reserved)
{
	if( Expression && Function && Line )
	{
		appErrorf(TEXT("SECURE CRT: Invalid parameter detected. Expression: %s Function: %s. File: %s Line: %d\n"), Expression, Function, File, Line );
	}
	else
	{
		appErrorf(TEXT("SECURE CRT: Invalid parameter detected"));
	}
}

INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// all crt validation should trigger the callback
	_set_invalid_parameter_handler(InvalidParameterHandler);

#ifdef _DEBUG
	// Disable the message box for assertions and just write to debugout instead
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
#endif

	// Initialize all timing info
	appInitTiming();
	// default to no game
	appStrcpy(GGameName, TEXT("None"));

	INT ErrorLevel			= 0;
	GIsStarted				= 1;
	hInstance				= hInInstance;
	const TCHAR* CmdLine	= GetCommandLine();
	
#if !SHIPPING_PC_GAME
	// Named mutex we use to figure out whether we are the first instance of the game running. This is needed to e.g.
	// make sure there is no contention when trying to save the shader cache.
	HANDLE NamedMutex = MakeNamedMutex();
	if( NamedMutex 
	&&	GetLastError() != ERROR_ALREADY_EXISTS 
	&& !ParseParam(CmdLine,TEXT("NEVERFIRST")) )
	{
		// We're the first instance!
		GIsFirstInstance = TRUE;
	}
	else
	{
		// There is already another instance of the game running.
		GIsFirstInstance = FALSE;
		ReleaseMutex( NamedMutex );
		NamedMutex = NULL;
	}
#endif

#ifdef _DEBUG
	if( TRUE )
#else
	if( IsDebuggerPresent() )
#endif
	{
		// Don't use exception handling when a debugger is attached to exactly trap the crash. This does NOT check
		// whether we are the first instance or not!
		ErrorLevel = GuardedMain( CmdLine, hInInstance, hPrevInstance, nCmdShow );
	}
	else
	{
		// Use structured exception handling to trap any crashs, walk the the stack and display a crash dialog box.
		__try
		{
			GIsGuarded = 1;
			// Run the guarded code.
			ErrorLevel = GuardedMain( CmdLine, hInInstance, hPrevInstance, nCmdShow );
			GIsGuarded = 0;
		}
		__except( CreateMiniDump( GetExceptionInformation() ) )
		{
#if !SHIPPING_PC_GAME
			// Release the mutex in the error case to ensure subsequent runs don't find it.
			ReleaseMutex( NamedMutex );
			NamedMutex = NULL;
#endif
			// Crashed.
			ErrorLevel = 1;
			GError->HandleError();
			StaticShutdownAfterError();
		}
	}

	// Final shut down.
	appExit();
#if !SHIPPING_PC_GAME
	// Release the named mutex again now that we are done.
	ReleaseMutex( NamedMutex );
	NamedMutex = NULL;
#endif
	GIsStarted = 0;
	return ErrorLevel;
}

// Work around for VS.NET 2005 bug with 'link library dependencies' option.
//
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=112962

#if _MSC_VER >= 1400 && !(defined XBOX)

#pragma comment(lib, "ALAudio.lib")
#pragma comment(lib, "Core.lib")
#pragma comment(lib, "D3DDrv.lib")
#pragma comment(lib, "Editor.lib")
#pragma comment(lib, "Engine.lib")
#if WITH_FACEFX
#pragma comment(lib, "FxStudio_Unreal.lib")
#endif
#pragma comment(lib, "GameFramework.lib")
#pragma comment(lib, "IpDrv.lib")
#pragma comment(lib, "UnrealEd.lib")
#pragma comment(lib, "UnrealScriptTest.lib")
#pragma comment(lib, "WinDrv.lib")

#if GAMENAME == WARGAME
#pragma comment(lib, "WarfareEditor.lib")
#pragma comment(lib, "WarfareGame.lib")
	#if !WITH_PANORAMA
		#pragma comment(lib, "OnlineSubsystemPC.lib")
	#else
		#pragma comment(lib, "OnlineSubsystemLive.lib")
	#endif
#elif GAMENAME == GEARGAME
#pragma comment(lib, "GearEditor.lib")
#pragma comment(lib, "GearGame.lib")
#elif GAMENAME == UTGAME
#pragma comment(lib, "UTEditor.lib")
#pragma comment(lib, "UTGame.lib")
#if WITH_GAMESPY
	#pragma comment(lib, "OnlineSubsystemGameSpy.lib")
#endif
#elif GAMENAME == EXAMPLEGAME
#pragma comment(lib, "ExampleEditor.lib")
#pragma comment(lib, "ExampleGame.lib")
#pragma comment(lib, "OnlineSubsystemPC.lib")
#else
#error Hook up your game name here
#endif

#endif // _MSC_VER >= 1400

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
