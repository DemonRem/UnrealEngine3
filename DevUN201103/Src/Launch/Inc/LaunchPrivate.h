/*=============================================================================
	LaunchPrivate.h: Unreal launcher.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if _WINDOWS

#if WITH_EDITOR //have to have the editor compiled in at least
#define HAVE_WXWIDGETS 1
#else
#define HAVE_WXWIDGETS 0
#endif

//@warning: this needs to be the very first include
#if _WINDOWS && WITH_EDITOR
#include "UnrealEd.h"
#endif

#include "Engine.h"
#include "UnIpDrv.h"
#include "DemoRecording.h"

#if _WINDOWS
#include "WinDrv.h"
#endif

#include "LaunchGames.h"
#include "FMallocAnsi.h"
#include "FMallocDebug.h"
#include "FMallocProfiler.h"
#include "FMallocProxySimpleTrack.h"
#include "FMallocProxySimpleTag.h"
#include "FMallocThreadSafeProxy.h"
#include "FFeedbackContextAnsi.h"
#include "FCallbackDevice.h"
#include "FConfigCacheIni.h"
#include "LaunchEngineLoop.h"

#if _WINDOWS
#if _WIN64
#include "FMallocTBB.h"
#else
#include "FMallocWindows.h"
#endif
#include "FFeedbackContextWindows.h"
#include "FFileManagerWindows.h"
#include "UnThreadingWindows.h"
#else
#error Please define your platform.
#endif

#endif
