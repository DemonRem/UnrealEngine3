/**********************************************************************

Filename    :   GFxUIImageInfo.h
Content     :   Implements FGFxImageInfo used to create GFx textures
                wrapped around UTexture based on a filename.

Copyright   :   (c) 2006-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxUIImageInfo_h
#define GFxUIImageInfo_h

#if WITH_GFx

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GImageInfo.h"
#include "GFxString.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop, 8)
#endif

class FGFxImageInfo : public GImageInfo
{
    // File name we will be loading from.
    GFxString           FileName;
public:

	FGFxImageInfo(const GFxString& fileName, UInt targetWidth, UInt targetHeight)
		:  GImageInfo((GImage*)NULL, targetWidth, targetHeight), FileName(fileName)
	{ }

    // Implement Recreate to init texture from a pack file.
    virtual bool    Recreate(class GRenderer* prenderer);

	virtual GTexture* GetTexture(class GRenderer* prenderer);

    virtual UPInt       GetExternalBytes() const;
};

#endif // WITH_GFx

#endif // GFxUIImageInfo_h
