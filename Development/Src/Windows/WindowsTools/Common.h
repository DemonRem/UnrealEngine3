/*=============================================================================
	WindowsSupport/Common.h: Common utitilities used by Windows Support.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#pragma pack(push,8)

#include <windows.h>
#include <objbase.h>
#include <assert.h>
#include <stdio.h>
#include <process.h>
#include <string>
#include <vector>
#include <comutil.h>
#include <time.h>

using namespace std;

#ifndef TESTING_WINDOWS_TOOLS
#include "..\..\Engine\Inc\UnConsoleTools.h"
#endif
