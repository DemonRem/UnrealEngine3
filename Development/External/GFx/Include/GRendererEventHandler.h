/**********************************************************************

Filename    :   GRendererEventHandler.h
Content     :   Event handler interface for Renderer
Created     :   Apr 30, 2010
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GRendererEventHandler_H
#define INC_GRendererEventHandler_H

#include "GTypes.h"
#include "GAtomic.h"
#include "GRefCount.h"
#include "GStats.h"
#include "GList.h"

class GRendererEventHandler : protected GListNode<GRendererEventHandler>,
                              public GNewOverrideBase<GStat_Default_Mem>
{
    friend class GRenderer;
    friend class GList<GRendererEventHandler>;
    friend struct GListNode<GRendererEventHandler>;

protected:
    class GRenderer* pRenderer;

    void RemoveAndClear()
    {
        if (pNext && pPrev)
            Remove();

        pNext = pPrev = NULL;
        pRenderer = NULL;
    }

public:
    GRendererEventHandler() { pNext = pPrev = NULL; pRenderer = NULL; }
    virtual ~GRendererEventHandler() 
    {
        GASSERT(!pNext && !pPrev);
    }

    void BindRenderer(class GRenderer* pr) { pRenderer = pr;   }
    class GRenderer* GetRenderer() const   { return pRenderer; }

    // calls renderer->RemoveEventHandler
    void RemoveFromList();

    enum EventType
    {
        // EndFrame, application can free data required to be retained for entire frame
        Event_EndFrame,
        // Renderer was released, no more data operations are possible
        Event_RendererReleased
    };

    // Called when renderer texture data is lost, or when it changes
    virtual void OnEvent(class GRenderer* prenderer, EventType changeType)
        { GUNUSED2(prenderer, changeType); }
};

#endif //INC_GRendererEventHandler_H

