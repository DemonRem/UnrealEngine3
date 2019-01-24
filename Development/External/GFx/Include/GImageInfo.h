/**********************************************************************

Filename    :   GImageInfo.h
Content     :   Defines GImageInfo family of classes, which associate
                a logical image with a texture.
Created     :   January 30, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GIMAGEINFO_H
#define INC_GIMAGEINFO_H

#include "GRenderer.h"

// ***** Classes Declared

class GImageInfoBase;
class GImageInfo;

// Forward declarations:
class GImage;


// ***** GImageInfo Related classes.

// GImageInfoBase - a base class that provides a way to obtain a texture
// corresponding to an image. Image resources will store pointers to this class.

// All GImageInfoBase derived classes in GFx are created by either GFxImageCreator::CreateImage
// or GImageInfoBase::CreateSubImage functions. If the user overrides GFxImageCreator to return
// a custom implementation of GImageInfoBase, that implementation can be used to
// manage image and texture references throughout the game instead of GImageInfo.

class GImageInfoBase : public GRefCountBaseNTS<GImageInfoBase, GStat_Default_Mem>,
                       public GTexture::ChangeHandler
{
public:

    enum ImageInfoType
    {        
        IIT_ImageInfo,      // GImageInfo        
        IIT_SubImageInfo,   // GSubImageInfo
        IIT_Other
    };
    
    // Obtains logical dimensions of images. These are usually the dimensions of the 
    // image that should be displayed even even if the image itself is rounded up.
    virtual UInt        GetWidth() const                    = 0;
    virtual UInt        GetHeight() const                   = 0;

#ifdef GFX_AMP_SERVER
    // GFx-owned memory consumed by this image, to be reported by AMP
    virtual UPInt       GetBytes() const                    = 0;
    // Non-GFx memory consumed by this image, to be reported by AMP
    virtual UPInt       GetExternalBytes() const            = 0;
#endif

    // Obtains the texture pointer from the data,
    // for a given renderer. Optionally create the texture
    virtual GTexture*   GetTexture(GRenderer* prenderer)    = 0;

    // Called to create GImageInfoBase that references a subregion of this image, as happens
    // when GFxImagePackParams is used (or when loading images packed by gfxexport).
    // Default implementation creates GSubImageInfo that references this GImageInfoBase for its data.
    virtual GImageInfoBase* CreateSubImage(const GRect<SInt>& rect,
                                           GMemoryHeap* pheap = GMemory::GetGlobalHeap());

    // Return sub-rectangle of texture to use; by default full texture rectangle is reported.
    // A smaller packed rectangle can be reported for GSubImageInfo.
    virtual GRect<SInt> GetRect() const { return GRect<SInt>(0, 0, GetWidth(), GetHeight()); }
    
    // RTTI for GImageInfoBase.
    virtual UInt        GetImageInfoType() const   { return IIT_Other; }

#ifdef GFX_AMP_SERVER
    virtual UInt32      GetImageId() const          { return 0; }
#endif
};


// GImageInfoBaseImpl - partial implementation of GImageInfoBase that stores a
// cached texture, relying on virtual Recreate function to initialize its data.
// Note that Recreate is inherited from GTexture::ChangeHandler in base; user
// is responsible for overriding Recreate in order to initialize the texture.
class GImageInfoBaseImpl : public GImageInfoBase
{
protected:
    // Texture, in the future could be a hash from renderer to texture.
    GPtr<GTexture>  pTexture;
    UInt            TextureUsage;

    // Protected constructors, intended for derived classes only.
    GImageInfoBaseImpl() { }
    GImageInfoBaseImpl(GTexture *ptexture);
public:

    ~GImageInfoBaseImpl();
        
    // Relies of Recreate() to initialize / re-initaliase ptexture.
    virtual GTexture*   GetTexture(GRenderer* prenderer);
    // Partial GTexture::ChangeHandler implementation.
    virtual void        OnChange(GRenderer* prenderer, EventType changeType);    
};



// ***** GImageInfo

// GImageInfo wraps a texture optionally backed by GImage.
//  - If GImage constructor is used, in-memory image is uses to create a texture
//   and re-initialize it as necessary.
//  - If GTexture constructor is used, texture is served directly and can not
//    be recreated unless Recreate is overridden in the derived class. In an event
//    of data loss, texture pointer is cleared and must be re-initialized by users.
// 
// GImageInfo stores additional {TargetWidth, TargetHeight} values that represent
// the size that Flash thinks the image has. These values are returned by
// GetWidth/GetHeight APIs and used for normalizing/scaling the fill texture
// matrix before rendering. In most cases these values are the same as the image
// and texture sizes; however, if RenderTexture substitution is used images 
// developer may want to substitute a texture of a different size into Flash.
// When such substitution happens original {TargetWidth, TargetHeight} still need
// to be maintained and returned to ensure proper scaling.

class GImageInfo : public GImageInfoBaseImpl
{
protected:
    // Source image data for the texture. If this is null, texture has no backup
    // data and recreate will fail.
    GPtr<GImage>    pImage;
    // Logical dimensions passed during texture creation to ensure
    // that renderer performs correct scaling.
    UInt            TargetWidth, TargetHeight; 
    // Specifies if pImage should be released then pTexture is created
    bool            ReleaseImage;
#ifdef GFX_AMP_SERVER
    // Unique ID for shape highlighting
    UInt32                      ImageId;
    static UInt32               GetNextImageId();
#endif
public:

    // *** GImageInfo constructors

    // Initialized GImageInfo with an image that will later be used for
    // texture creation. Passing an image has an advantage of allowing the
    // data to be re-created if the texture is lost. Storing image has no
    // benefit, however, on platforms that do not incur texture loss. 
    // if releaseImage flag is set to true the image will be released 
    // after texture is created
    GImageInfo(GImage *pimage = 0, bool releaseImage = false);
    GImageInfo(GImage *pimage, UInt targetWidth, UInt targetHeight,
               bool releaseImage = false);

    // Initializes GImageInfo with a texture. Since no image is provided,
    // texture will be cleared out in the event of data loss.
    GImageInfo(GTexture *ptexture, UInt targetWidth, UInt targetHeight);

    virtual ~GImageInfo();


    // *** Data Assignment

    // SetImage and SetTexture APIs have the same behavior as constructors;
    // they re-initalize the GImageInfo object.

    // Set the image, keeping original target size. This will re-initialize the texture.
    void                SetImage(GImage* pimage);
    // Set the image, assigning the new target dimensions.
    void                SetImage(GImage* pimage, UInt targetWidth, UInt targetHeight);

    // Set image into to a texture, keeping original target size. If a reference to
    // an image was maintained, it will be released.
    void                SetTexture(GTexture *ptexture);
    // Sets the texture and potential target dimensions.
    void                SetTexture(GTexture *ptexture, UInt targetWidth, UInt targetHeight);
    // Set usage flags passed to renderer on creation (only for textures created from pImage)
    void                SetTextureUsage(UInt usage);

    // GImage query, if one exists.
    inline GImage*      GetImage() const { return pImage; }

    
    // *** GImageInfoBase Implementation

    virtual UInt        GetWidth() const;
    virtual UInt        GetHeight() const;
#ifdef GFX_AMP_SERVER
    // GFx-owned memory consumed by this image, to be reported by AMP
    virtual UPInt       GetBytes() const;
    // Non-GFx-owned memory consumed by this image, to be reported by AMP
    virtual UPInt       GetExternalBytes() const;
    // Unique image ID to be used for AMP-triggered highlighting
    virtual UInt32      GetImageId() const          { return ImageId; }
#endif
    virtual UInt        GetImageInfoType() const   { return IIT_ImageInfo; }
    virtual GTexture*   GetTexture(GRenderer* prenderer);
    virtual void        OnChange(GRenderer* prenderer, EventType changeType);    
    virtual bool        Recreate(GRenderer* prenderer);
};



// GSubImageInfo is an implementation of GImageInfoBase that references a sub-rectangle
// of an original image. Objects of this class are created by GImageInfoBase::CreateSubImage
// to support image packing.

class GSubImageInfo : public GImageInfoBase
{
    GPtr<GImageInfoBase>    pBaseImage; 
    GRect<SInt>             Rect;

public:
    GSubImageInfo(GImageInfoBase* pbase, GRect<SInt> rect)
        : pBaseImage(pbase), Rect(rect)
    { }

    virtual UInt        GetWidth() const                    { return pBaseImage->GetWidth(); }
    virtual UInt        GetHeight() const                   { return pBaseImage->GetHeight(); }
#ifdef GFX_AMP_SERVER
    virtual UPInt       GetBytes() const                    { return pBaseImage->GetBytes(); }
    virtual UPInt       GetExternalBytes() const            { return pBaseImage->GetExternalBytes(); }
#endif
    virtual GTexture*   GetTexture(GRenderer* prenderer)    { return pBaseImage->GetTexture(prenderer); }
    virtual GRect<SInt> GetRect() const                     { return Rect; }

    virtual GImageInfoBase* CreateSubImage(const GRect<SInt>& rect,
                                           GMemoryHeap* pheap = GMemory::GetGlobalHeap());

    virtual UInt        GetImageInfoType() const    { return IIT_SubImageInfo; }
    virtual GImageInfoBase *GetBaseImage()          { return pBaseImage; }
};


/*
// GImageInfoBase version which loads textures directly from file.
class GImageFileInfo : public GImageInfoBase
{
public:
    char*               pFileName;
    GPtr<GTexture>      pTexture;
    // Target sizes describe the desired size of the image, they can be 
    // smaller then the texture size in case the texture was rounded
    // up to the next power of two.
    UInt                TargetWidth, TargetHeight;

    GImageFileInfo(UInt targetWidth, UInt targetHeight)
        : pFileName(0), TargetWidth(targetWidth), TargetHeight(targetHeight) 
    {
    }
    ~GImageFileInfo();

    void        SetFileName(const char* name);
    // Returned size can be incorrect (zero) if not specified.
    UInt        GetWidth() const  { return TargetWidth;  }
    UInt        GetHeight() const { return TargetHeight; }
    GTexture*   GetTexture(class GRenderer* prenderer);

    // GTexture::ChangeHandler implementation           
    virtual void    OnChange(GRenderer* prenderer, EventType changeType);
    virtual bool    Recreate(GRenderer* prenderer);         
};
*/

#endif
