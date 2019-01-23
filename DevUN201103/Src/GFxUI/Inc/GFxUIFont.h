/**********************************************************************

Filename    :   GFxUIFont.h
Content     :   

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxUIFont_h
#define GFxUIFont_h

#if WITH_GFx

#include "GFxUIRenderer.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GFxLoader.h"
#include "GFxFont.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif


//------------------------------------------------------------------------
class FGFxUFontProvider : public GFxFontProvider
{
public:
    FGFxUFontProvider();
    virtual ~FGFxUFontProvider();

    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);
    virtual void        LoadFontNames(GStringHash<GString>& fontnames);
};


#endif
#endif
