/**********************************************************************

Filename    :   GFxUIGenericBrowserContextMenus.h
Content     :   Header for SwfMovie factory used to
create USwfMovie.

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxUIGenericBrowserContextMenus_h
#define GFxUIGenericBrowserContextMenus_h

#if WITH_GFx

#include "GenericBrowserContextMenus.h"


class WxMBGenericBrowserContext_GFxMovieInfo : public WxMBGenericBrowserContext
{
public:
    typedef WxMBGenericBrowserContext Super;
    WxMBGenericBrowserContext_GFxMovieInfo();

protected:
    wxMenuItem *Reimport;

    /** Enables/disables menu items based on whether the selection set contains cooked objects. */
    virtual void ToggleItemsForCookedContent(UBOOL bSomeObjectsAreCooked);
};


#endif // WITH_GFx

#endif
