/**********************************************************************

Filename    :   GImage.h
Content     :   Image storage / manipulation classes
Created     :   July 29, 1998
Authors     :   Michael Antonov, Brendan Iribe

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GIMAGE_H
#define INC_GIMAGE_H

#include "GTypes.h"
#include "GRefCount.h"
#include "GArray.h"
#include "GColor.h"


// ***** Declared Classes
class GImageBase;
class GImage;

// ***** External Classes
class GFile;
class GJPEGInput;
class GJPEGSystem;


// ***** Image Base class

class GImageBase
{

public:
    enum ImageFormat
    {
        Image_None      = 0,
        Image_ARGB_8888 = 1,
        Image_RGB_888   = 2,
    //  Image_RGB_565   = 3,
    //  Image_ARGB_4444 = 4,
        Image_L_8       = 8,
        Image_A_8       = 9,
        Image_DXT1      = 10,
        Image_DXT3      = 11,
        Image_DXT5      = 12,

        Image_P_8       = 100,// Palette8, just 256-color TGA is supported; 
                              // being converted to RGB_888 during loading by default.

        // Formats for video textures. Not usuable for any other purpose.
        Image_YUV_822   = 200,
        Image_YUVA_8228 = 201,

        // Formats for render targets.
        Image_Depth        = 300,
        Image_Stencil      = 301,
        Image_DepthStencil = 302,
    };

    ImageFormat Format;
    UInt32      Width, Height;
    UInt32      Pitch;

    // Pointer to data
    UByte*      pData;
    UInt        DataSize;

    // MipMap count (for DDS textures)
    UInt        MipMapCount;

    GArray<GColor> ColorMap;


    // Scanline access
    UByte*          GetScanline(UInt y)
        { GASSERT(Pitch * y < DataSize); return pData + Pitch * y; }
    const UByte*    GetScanline(UInt y) const
        { GASSERT(Pitch * y < DataSize); return pData + Pitch * y; }

    static UInt GSTDCALL GetBytesPerPixel(ImageFormat fmt);
    UInt            GetBytesPerPixel() const
        { return GetBytesPerPixel(Format);  }

    static UInt GSTDCALL GetPitch(ImageFormat fmt, UInt width);
    UInt            GetPitch() const
        { return Pitch; }

    // Base constructor
    GImageBase()
        { }
    GImageBase(ImageFormat format, UInt32 width, UInt32 height, UInt32 pitch)
        { Format = format; Width = width; Height = height; Pitch = pitch; MipMapCount = 1; pData = 0; DataSize = 0; }

    void    ClearImageBase()
    {
        Format      = Image_None;
        Width       = Height = Pitch = 0;
        pData       = 0;
        DataSize    = 0;
        MipMapCount = 1;
        ColorMap.Clear();
    }

    // Set pixel, sets only the appropriate channels
    void    SetPixelAlpha(SInt x, SInt y, UByte alpha);
    void    SetPixelLum(SInt x, SInt y, UByte lum);
    void    SetPixelRGBA(SInt x, SInt y, UInt32 color);
    void    SetPixelRGBA(SInt x, SInt y, UByte r, UByte g, UByte b, UByte a)
        { SetPixelRGBA(x,y, ((UInt32)r) | (((UInt32)g)<<8) | (((UInt32)b)<<16) | (((UInt32)a)<<24) ); }

    // Compute a hash code based on image contents.  Can be useful
    // for comparing images. Will return 0 if pData is null.
    UPInt   ComputeHash() const;

    inline bool           IsDataCompressed() const { return Format >= Image_DXT1 && Format <= Image_DXT5; }
    // level = 0 for main texture
    // if pwidth and pheight are not NULL they will contain dimensions of mipmap level.
    UByte*                GetMipMapLevelData(UInt level, UInt* pwidth, UInt* pheight, UInt* ppitch = 0);
    static UInt GSTDCALL  GetMipMapLevelSize(ImageFormat format, UInt w, UInt h);
};


// ***** Image class

class GImage : public GRefCountBaseNTS<GImage, GStat_Image_Mem>, public GImageBase
{
protected:
    // Helper function for constructors
    void    CreateImageCopy(const GImageBase &src);

public:
    
    // Constructors
    // End users should rely on the create function instead
    GEXPORT GImage();
    GEXPORT GImage(ImageFormat format, UInt32 width, UInt32 height);
    GEXPORT GImage(const GImageBase &src)
        : GRefCountBaseNTS<GImage, GStat_Image_Mem>(), GImageBase()
    { CreateImageCopy(src); }
    GEXPORT GImage(const GImage &src)
        : GRefCountBaseNTS<GImage, GStat_Image_Mem>(), GImageBase()
    { CreateImageCopy(src); }
    GEXPORT ~GImage();

    // An image is considered non-empty if it has allocated data
    bool                IsEmpty() const
        { GASSERT(pData != 0 || DataSize == 0); return pData == 0; }
    // Deletes image data
    void                Clear();

    // Raw comparison of data
    bool    operator == (const GImage &src) const;


    // Create an image (return 0 if allocation failed)
    static  GImage*     GSTDCALL CreateImage(ImageFormat format, UInt32 width, UInt32 height,
                                             GMemoryHeap* pimageHeap = 0);

    
    // *** JPEG I/O
    
    // Returns created images, need to *
    static GImage*      GSTDCALL ReadJpeg(const char* pfilename, GJPEGSystem *psystem,
                                          GMemoryHeap* pimageHeap = 0);
    static GImage*      GSTDCALL ReadJpeg(GFile* in, GJPEGSystem *psystem,
                                          GMemoryHeap* pimageHeap = 0);
    bool                         WriteJpeg(GFile* pout, int quality, GJPEGSystem *psystem);


    // *** TGA I/O

    // Loads TGA, only 32/24 bit formats supported.
    static GImage*      GSTDCALL ReadTga(GFile* in, ImageFormat destFormat = Image_None,
                                         GMemoryHeap* pimageHeap = 0);

    bool                WriteTga(GFile* pout);


    // *** DDS Format loading

    // These routines do not rely on D3DX API and can therefore be used for compressed
    // textures with OpenGL as well as on other platforms.

    // Loads DDS from file creating GImage.
    static GImage*      GSTDCALL ReadDDS(GFile* in, GMemoryHeap* pimageHeap = 0);

    // Loads DDS from a chunk of memory, data is copied.
    static GImage*      GSTDCALL ReadDDSFromMemory(const UByte* ddsData, UPInt dataSize,
                                                   GMemoryHeap* pheap = 0);


    // *** PNG format loading
    static GImage*      GSTDCALL ReadPng(const char* pfilename, GMemoryHeap* pimageHeap = 0);
    static GImage*      GSTDCALL ReadPng(GFile* in, GMemoryHeap* pimageHeap = 0);
    
    bool                WritePng(GFile* pout);

    // Converts an image to specified ImageFormat. New image is allocated in the 
    // specified memory heap (pass null for global heap).
    GImage*             ConvertImage(ImageFormat destFormat, GMemoryHeap* pimageHeap = 0);
};


#endif 
