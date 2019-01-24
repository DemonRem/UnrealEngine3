/**********************************************************************

Filename    :   GFxImageResource.h
Content     :   Image resource representation for GFxPlayer
Created     :   January 30, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXIMAGERESOURCE_H
#define INC_GFXIMAGERESOURCE_H

#include "GFxResource.h"
#include "GFxLoader.h"

#include "GRenderer.h"
#include "GImageInfo.h"


// **** Forward Decl

class GFxImageInfoBase;



// ***** GFxImageResource - image resource representation

struct GFxImageFileInfo : public GFxResourceFileInfo
{
    enum ImageInfoFlags 
    {
        Wrappable = 0x01
    };
    UInt16                      TargetWidth;
    UInt16                      TargetHeight;
    // Store Use, so that that we can pass it to ImageCreator 
    GFxResource::ResourceUse    Use;
    UInt8                       Flags;
    GString                     ExportName;

    GFxImageFileInfo()
    {
        TargetHeight = TargetHeight = 0;
        Use = GFxResource::Use_Bitmap;
    }
    GFxImageFileInfo(const GFxImageFileInfo& other) : GFxResourceFileInfo(other)
    {
        TargetWidth = other.TargetWidth;
        TargetHeight = other.TargetHeight;
        Use = other.Use;
    }
};


class GFxImageResource : public GFxResource
{
protected:

    GPtr<GImageInfoBase>    pImageInfo;
    // Key used to look up / resolve this object (contains file data).
    GFxResourceKey          Key;
    // What the resource is used for
    ResourceUse             UseType;

public:
    GFxImageResource(GImageInfoBase* pimage, ResourceUse use = Use_Bitmap)
    {
        pImageInfo = pimage;
        UseType    = use;
    }
    GFxImageResource(GImageInfoBase* pimage, const GFxResourceKey& key, ResourceUse use = Use_Bitmap)
    {
        pImageInfo  = pimage;
        Key         = key;
        UseType     = use;
    }

    // Returns the referenced image. All GImageInfoBase derived classes are created
    // by GFxImageCreator::CreateImage or GImageInfoBase::CreateSubImage. If the user
    // overrides both of those functions to return a custom class, they can use type-casts
    // to access they custom data structures.
    inline GImageInfoBase*  GetImageInfo() const
    {
        return pImageInfo.GetPtr();
    }
    inline void             SetImageInfo(GImageInfoBase* pimageInfo)
    {
        pImageInfo = pimageInfo;
    }

    inline UInt        GetWidth() const     { return pImageInfo ? pImageInfo->GetWidth() : 0;  }
    inline UInt        GetHeight() const    { return pImageInfo ? pImageInfo->GetHeight() : 0;  }

#ifdef GFX_AMP_SERVER
    inline UPInt       GetBytes() const         { return pImageInfo ? pImageInfo->GetBytes() : 0;  }
    inline UPInt       GetExternalBytes() const { return pImageInfo ? pImageInfo->GetExternalBytes() : 0;  }
#endif

    virtual GFxResourceId GetBaseImageId() { return GFxResourceId(0); }

    // Ref file info, if any.
    /*
        
    // MA: We may need to bring this back, however,
    // we can to do this by obtaining the info from key.
    // Instead, we would need to store it locally.

    const GFxImageFileInfo* GetFileInfo() const
    {
        // Image files
        return (const GFxImageFileInfo*)Key.GetFileInfo();
    }
    */

    const char* GetFileURL() const
    {
        // Image files
        return Key.GetFileURL();
    }

    // GImplement GFxResource interface.
    virtual GFxResourceKey  GetKey()                        { return Key; }
    virtual ResourceUse     GetImageUse() const             { return UseType; }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_Image, UseType); }


    // *** Methods for creating image keys

    // Create a key for an image file.    
    static  GFxResourceKey  CreateImageFileKey(GFxImageFileInfo* pfileInfo,
                                               GFxFileOpener* pfileOpener,
                                               GFxImageCreator* pimageCreator,
                                               GMemoryHeap* pimageHeap);
};


struct GFxSubImageResourceInfo : public GRefCountBaseNTS<GFxSubImageResourceInfo, GStat_Default_Mem>
{
    GFxResourceId           ImageId;
    GPtr<GFxImageResource>  Image;
    GRect<SInt>             Rect;

    UPInt  GetHashCode() const;
};

class GFxSubImageResource : public GFxImageResource
{
    GRect<SInt>     Rect;
    GFxResourceId   BaseImageId;

public:
    GFxSubImageResource(GFxImageResource* pbase, GFxResourceId baseid,
                        const GRect<SInt>& rect, GMemoryHeap* pheap)
    :   GFxImageResource(
            GPtr<GImageInfoBase>(*(pbase->GetImageInfo() ?
                                    pbase->GetImageInfo()->CreateSubImage(rect, pheap) :
                                    (GImageInfoBase*)0) ),
                                 pbase->GetImageUse()),
        Rect(rect), BaseImageId(baseid)
    {
    }

    virtual GFxResourceId GetBaseImageId() { return BaseImageId; }
};

// ***** GFxImageCreateInfo


// Image creation information passed GFxImageCreator::CreateImage and GFxStateBag::CreateImageInfo. 
// This data can be used to decide on the type of GFxImageInfoBase class to create and provides 
// the data which is stored there.
class GFxImageCreateInfo
{
public:
    // The type of image info provided. 
    enum InputType
    {
        Input_None,     // No image data provided, create empty container.
        Input_Image,    // Image data is provided through pImage pointer.
        Input_File,     // Image data should be accessed through ImageFileInfo.
    };
    
    // Image type - describes where to get image data from.
    InputType                Type;
    GFxResource::ResourceUse Use;
    // Export name for image resource; only reported for GFX files.
    const char*             pExportName;

    // Texture usage passed to renderer
    UInt                    TexUsage;

    union {
        GImage*             pImage;
        GFxImageFileInfo*   pFileInfo;
    };

    // States to be used during creation.
    // FileOpener should be used to open image files; it is only passed for Input_File.
    GFxFileOpener*          pFileOpener;
    // RenderConfig is passed if it is available at image creation time. This can be
    // used to create textures directly out of image data; however, renderer must then be
    // configured before file loading.
    GFxRenderConfig*        pRenderConfig;

    GFxLog*                 pLog;
    GFxJpegSupportBase*     pJpegSupport;
    GFxPNGSupportBase*      pPNGSupport;

    // Memory heap to use for GFxImageInfo (use global heap if null).
    GMemoryHeap*            pHeap;

    // set to true if multi-threaded loading is used
    bool                    ThreadedLoading; 
    
    GFxImageCreateInfo()
    {        
        Type            = Input_None;
        pFileInfo       = 0;
        pExportName     = 0;
        Use             = GFxResource::Use_None;
        TexUsage        = GTexture::Usage_Wrap;
        // Empty states by default.
        pFileOpener     = 0;
        pRenderConfig   = 0;
        pLog            = 0;
        pJpegSupport    = 0;
        pPNGSupport     = 0;
        pHeap           = 0;
        ThreadedLoading = false;
    }
    GFxImageCreateInfo(GImage* pimage,
                       GFxResource::ResourceUse ru = GFxResource::Use_None,
                       GFxRenderConfig* prconfig = 0)
    {
        Type        = Input_Image;
        pImage      = pimage;
        pExportName = 0;
        Use         = ru;
        TexUsage    = GTexture::Usage_Wrap;
        // States - for GImage based initialization we do not pass file opener.
        pFileOpener     = 0;
        pLog            = 0;
        pJpegSupport    = 0;
        pPNGSupport     = 0;
        pRenderConfig   = prconfig;
        pHeap           = 0;
        ThreadedLoading = false;
    }
    
    void    SetStates(GFxFileOpener* popener,  GFxRenderConfig* prconfig, GFxLog* plog, 
                      GFxJpegSupportBase* pjpegSupport, GFxPNGSupportBase* ppngSupport)
    {
        pFileOpener   = popener;
        pRenderConfig = prconfig;
        pLog          = plog;
        pJpegSupport  = pjpegSupport;
        pPNGSupport   = ppngSupport;  
    }

    void    SetType(InputType itype)   { Type = itype; }
    void    SetImage(GImage* pimage)   { SetType(Input_Image); pImage = pimage; }
    void    SetTextureUsage(UInt u)    { TexUsage = u; }
};



#endif

