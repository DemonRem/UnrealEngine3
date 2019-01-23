//=================================================================================================================================
//
// Author: Probably Josh Barczak (checked in by Budirijanto Purnomo)
//         3D Application Research Group
//         ATI Research, Inc.
//
//  The implementation of the Timer class.
//
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Tootle/samples/Timer.cpp#1 $ 
//
// Last check-in:  $DateTime: 2009/06/03 12:26:06 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   (C) ATI Research, Inc. 2008 All rights reserved. 
//=================================================================================================================================
//#include "TootlePCH.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

#include "Timer.h"

Timer::
Timer() { 
    Reset(); 
}

void 
Timer::
Reset(void) { 
    time = Get(); 
}

double 
Timer::
GetElapsed(void) { 
    return Get() - time; 
}

double 
Timer::Get(void) {
#ifdef _WIN32
#if 0
   return GetTickCount();
#else
   FILETIME ft;
   GetSystemTimeAsFileTime(&ft);
   return ft.dwLowDateTime/1.0e7 + ft.dwHighDateTime*(4294967296.0/1.0e7);
#endif
#else
    struct timeval v;
    gettimeofday(&v, (struct timezone *) NULL);
    return v.tv_sec + v.tv_usec/1.0e6;
#endif
}
