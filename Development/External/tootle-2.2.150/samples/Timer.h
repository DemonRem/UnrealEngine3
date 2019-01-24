//=================================================================================================================================
//
// Author: Probably Josh Barczak (checked in by Budirijanto Purnomo)
//         3D Application Research Group
//         ATI Research, Inc.
//
//  The interface of the Timer class.
//
//=================================================================================================================================
// $Id: //depot/3darg/Tools/main/Tootle/samples/Timer.h#1 $ 
//
// Last check-in:  $DateTime: 2009/06/03 12:26:06 $ 
// Last edited by: $Author: callan $
//=================================================================================================================================
//   (C) ATI Research, Inc. 2008 All rights reserved. 
//=================================================================================================================================
#ifndef _TIMER_H
#define _TIMER_H

class Timer {
public:
    Timer();
    void Reset(void);
    double GetElapsed(void);
    double Get(void);
private:
    double time;
};

#endif // _TIMER_H
