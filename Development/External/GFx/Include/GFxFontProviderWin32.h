/**********************************************************************

Filename    :   GFxFontProviderWin32.h
Content     :   Win32 API Font provider (GetGlyphOutline)
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.
**********************************************************************/

#ifndef INC_GFxFontProviderWin32_H
#define INC_GFxFontProviderWin32_H

#include <windows.h>
#include "GFxFont.h"
#include "GFxLoader.h"

class GFxShapeNoStyles;

//------------------------------------------------------------------------
struct GFxFontSysDataWin32
{
    enum { BufSizeInc = 1024-8 };

    GArray<UByte>   GlyphBuffer;
    HDC             WinHDC;
    UInt            RasterFormat;
    UByte           Gamma[256];

    GFxFontSysDataWin32(HDC dc) : WinHDC(dc), RasterFormat(GGO_BITMAP)
    {
        Gamma[0] = 0;
        Gamma[1] = 96;
        Gamma[2] = 192;
        Gamma[3] = 240;
        Gamma[4] = 255;
    }
};



//------------------------------------------------------------------------
class GFxFontProviderWin32;

class GFxExternalFontWin32 : public GFxFont
{
    // GGO_FontHeight is the font height to request the 
    // glyphs with the nominal size (not the specific hinted size). 
    // It appears to be extremelly expensive to call GetGlyphOutline
    // with font height of 256 or more. So, the GGO_FontHeight
    // is the "maximum inexpensive size" for GetGlyphOutline.
    enum 
    { 
        GGO_FontHeight = 240, 
        ShapePageSize = 256-2 - 8-4
    };
public:
    GFxExternalFontWin32(GFxFontProviderWin32 *pprovider,
                         GFxFontSysDataWin32* pSysData, 
                         const char* name, UInt fontFlags);

    virtual ~GFxExternalFontWin32();


    bool    IsValid() const { return MasterFont != 0; }
    void    SetHinting(const GFxFont::NativeHintingType& pnh);

    virtual GFxTextureGlyphData*    GetTextureGlyphData() const { return 0; }

    virtual int                     GetGlyphIndex(UInt16 code) const;
    virtual bool                    IsHintedVectorGlyph(UInt glyphIndex, UInt glyphSize) const;
    virtual bool                    IsHintedRasterGlyph(UInt glyphIndex, UInt glyphSize) const;
    virtual GFxShapeBase*           GetGlyphShape(UInt glyphIndex, UInt glyphSize);
    virtual GFxGlyphRaster*         GetGlyphRaster(UInt glyphIndex, UInt glyphSize);
    virtual Float                   GetAdvance(UInt glyphIndex) const;
    virtual Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    virtual Float                   GetGlyphWidth(UInt glyphIndex) const;
    virtual Float                   GetGlyphHeight(UInt glyphIndex) const;
    virtual GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    virtual const char*             GetName() const;

private:
    bool    decomposeGlyphOutline(const UByte* data, UInt size, GFxShapeNoStyles* shape);
    void    decomposeGlyphBitmap(const UByte* data, int w, int h, int x, int y, GFxGlyphRaster* raster);
    void    loadKerningPairs();

    static inline int FxToTwips(FIXED v)
    {
        return int(v.value) * 20 + int(v.fract) * 20 / 65536;
    }

    static inline int FxToS1024(FIXED v)
    {
        return ((int(v.value) << 8) + (int(v.fract) >> 8)) * 4 / GGO_FontHeight;
    }

    struct GlyphType
    {
        UInt                Code;
        Float               Advance;
        GRectF              Bounds;
    };

    struct KerningPairType
    {           
        UInt16  Char0, Char1;
        bool    operator==(const KerningPairType& k) const
            { return Char0 == k.Char0 && Char1 == k.Char1; }
    };

    // AddRef for font provider since it contains our SysData.
    GPtr<GFxFontProviderWin32>          pFontProvider;    
    GFxFontSysDataWin32*                pSysData;
    GArrayLH<char>                      Name; 
    GArrayLH<WCHAR>                     NameW;
    HFONT                               MasterFont;
    HFONT                               HintedFont;
    UInt                                LastHintedFontSize;
    GArrayLH<GlyphType>                 Glyphs;
    GHashIdentityLH<UInt16, UInt>       CodeTable;
    GHashLH<KerningPairType, Float>     KerningPairs;
    GPtr<GFxGlyphRaster>                pRaster;
    Float                               Scale1024;
    NativeHintingType                   Hinting;
};



//------------------------------------------------------------------------
class GFxFontProviderWin32 : public GFxFontProvider
{
public:
    GFxFontProviderWin32(HDC dc);
    virtual ~GFxFontProviderWin32();

    void SetHintingAllFonts(GFxFont::NativeHintingRange vectorRange, 
                            GFxFont::NativeHintingRange rasterRange, 
                            UInt maxVectorHintedSize=24,
                            UInt maxRasterHintedSize=24);
    void SetHinting(const char* name, 
                    GFxFont::NativeHintingRange vectorRange, 
                    GFxFont::NativeHintingRange rasterRange, 
                    UInt maxVectorHintedSize=24,
                    UInt maxRasterHintedSize=24);

    void SetRasterFormat(UInt format, const UByte* gamma=0);
    
    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);    

    virtual void        LoadFontNames(GStringHash<GString>& fontnames);

private:
    GFxFont::NativeHintingType* findNativeHinting(const char* name);

    GFxFontSysDataWin32                 SysData;
    GArray<GFxFont::NativeHintingType>  NativeHinting;
};



#endif
