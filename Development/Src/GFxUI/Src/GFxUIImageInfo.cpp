/**********************************************************************

Filename    :   GFxUIImageInfo.cpp
Content     :   UE3 GFxImageInfo / texture creation implementation.

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxUI.h"


#if WITH_GFx

#include "GFxUIEngine.h"
#include "GFxUIRenderer.h"
#include "GFxUIImageInfo.h"

// Implement Recreate to init texture from a pack file.
bool    FGFxImageInfo::Recreate(class GRenderer* prenderer)
{
    GUNUSED(prenderer);

    if (pTexture)
    {
        if (!FileName.IsEmpty())
        {
            if (((FGFxTexture*)pTexture.GetPtr())->InitTextureFromFile(FileName))
                return true;
            else
            {
                GFC_DEBUG_WARNING(1, "FGFxImageInfo::Receate failed, InitTextureFromFile returned false. ");
            }
        }
        else
        {
            // This may happen if images were not loaded or bound by user correctly.
            GFC_DEBUG_WARNING(1, "FGFxImageInfo::Receate texture failed, FileName not available. ");
        }
    }

    return false;
}


GTexture*   FGFxImageInfo::GetTexture(GRenderer* prenderer)
{
    if (pTexture)
    {
        // We currently only support one renderer
        GASSERT(pTexture->GetRenderer() == prenderer);
        return pTexture;
    }

    pTexture = *prenderer->CreateTexture();
    if (pTexture)
    {
        // Use our function to override the texture.
        if (Recreate(prenderer))
            pTexture->AddChangeHandler(this);
        else
            pTexture = 0;
    }
    return pTexture.GetPtr();
}

UPInt FGFxImageInfo::GetExternalBytes() const
{
    if (pTexture)
    {
        return ((FGFxTexture*)pTexture.GetPtr())->GetSize();
    }
    return 0;
}

#endif // WITH_GFx