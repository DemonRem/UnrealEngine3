/**********************************************************************

Filename    :   GTimer.h
Content     :   Provides static functions for precise timing
Created     :   June 28, 2005
Authors     :   
Notes       :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GTIMER_H
#define INC_GTIMER_H

#include "GTypes.h"

#define GFX_MICROSECONDS_PER_SECOND 1000000
#define GFX_MILLISECONDS_PER_SECOND 1000

#define GFX_DOUBLE_CLICK_TIME_MS    300  // in ms

// Hi-res timer for CPU profiling.
class GTimer
{
public:

    // General-purpose wall-clock timer.
    // May not be hi-res enough for profiling.               
    static UInt64   GSTDCALL GetTicks();

    // Return a hi-res timer value in ns (1/1000000 of a sec).
    // Generally you want to call this at the start and end of an
    // operation, and pass the difference to
    // ProfileTicksToSeconds() to find out how long the operation took. 
    static UInt64   GSTDCALL GetProfileTicks();

    // Get the raw cycle counter value, providing the maximum possible timer resolution.
    static UInt64   GSTDCALL GetRawTicks();
    static UInt64   GSTDCALL GetRawFrequency();

    // Convert a ticks into seconds
    static Double   GSTDCALL TicksToSeconds(UInt64 ticks);
    static Double   GSTDCALL ProfileTicksToSeconds(UInt64 profileTicks);
    static Double   GSTDCALL RawTicksToSeconds(UInt64 rawTicks, UInt64 rawFrequency);
};


#endif // INC_GTIMER_H
