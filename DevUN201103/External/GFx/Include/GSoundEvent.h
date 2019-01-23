/**********************************************************************

Filename    :   GSoundEvent.h
Content     :   GSoundEvent abstract class
Created     :   March 2010
Authors     :   Vladislav Merker

Copyright   :   (c) 1998-2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSOUNDEVENT_H
#define INC_GSOUNDEVENT_H

#include "GConfig.h"
#ifndef GFC_NO_SOUND

#include "GRefCount.h"
#include "GTypes.h"
#include "GString.h"

//////////////////////////////////////////////////////////////////////////
//

class GSoundEvent : public GRefCountBase<GSoundEvent,GStat_Sound_Mem>
{
public:
    GSoundEvent() : pUserData(NULL) {}
    virtual ~GSoundEvent() {}

    virtual void PostEvent(GString event, GString eventId = "") = 0;
    virtual void SetParam (GString param, Float paramValue, GString eventId = "") = 0;

    void* pUserData;
};

#endif // GFC_NO_SOUND

#endif // INC_GSOUNDEVENT_H
