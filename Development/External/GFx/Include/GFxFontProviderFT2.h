/**********************************************************************

Filename    :   GFxFontProviderFT2.h
Content     :   FreeType2 font provider
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

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

#ifndef INC_GFxFontProviderFT2_H
#define INC_GFxFontProviderFT2_H


#include <ft2build.h>
#include FT_FREETYPE_H

#include "GFxFont.h"
#include "GFxLoader.h"
#include "GFxString.h"

//------------------------------------------------------------------------
class GFxFontProviderFT2;
class GFxPathPacker;
class GFxShapeNoStyles;

class GFxExternalFontFT2 : public GFxFont
{
    enum { FontHeight = 1024, ShapePageSize = 256-2 - 8-4 };
public:
    GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                       FT_Library lib, 
                       const GString& fontName, 
                       UInt fontFlags,
                       const char* fileName, 
                       UInt faceIndex);

    GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                       FT_Library lib, 
                       const GString& fontName, 
                       UInt fontFlags,
                       const char* fontMem, 
                       UInt fontMemSize, 
                       UInt faceIndex);

    void SetHinting(NativeHintingRange vectorHintingRange,
                    NativeHintingRange rasterHintingRange, 
                    UInt maxVectorHintedSize,
                    UInt maxRasterHintedSize);

    virtual ~GFxExternalFontFT2();

    bool    IsValid() const { return Face != 0; }
    
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

    virtual const char*             GetName() const { return &Name[0]; }

private:
    void    setFontMetrics();
    bool    decomposeGlyphOutline(const FT_Outline& outline, GFxShapeBase* shape);
    void    decomposeGlyphBitmap(const FT_Bitmap& bitmap, int x, int y, GFxGlyphRaster* raster);

    static inline int GSTDCALL FtToTwips(int v)
    {
        return (v * 20) >> 6;
    }

    static inline int GSTDCALL FtToS1024(int v)
    {
        return v >> 6;
    }

    static void GSTDCALL cubicToQuadratic(GFxPathPacker& path, int hintedGlyphSize, 
                                 int x2, int y2, int x3, int y3, int x4, int y4);

    struct GlyphType
    {
        UInt                    Code;
        UInt                    FtIndex;
        Float                   Advance;
        GRectF                  Bounds;
    };

    struct KerningPairType
    {           
        UInt16  Char0, Char1;
        bool    operator==(const KerningPairType& k) const
            { return Char0 == k.Char0 && Char1 == k.Char1; }
    };

    // AddRef for font provider since it contains our cache.
    GPtr<GFxFontProviderFT2>            pFontProvider;    
    GString                             Name;
    FT_Face                             Face;
    GArray<GlyphType>                   Glyphs;
    GHashIdentity<UInt16, UInt>         CodeTable;
    GHash<KerningPairType, Float>       KerningPairs;
    GPtr<GFxGlyphRaster>                pRaster;
    UInt                                LastFontHeight;
    NativeHintingRange                  RasterHintingRange;
    NativeHintingRange                  VectorHintingRange;
    UInt                                MaxRasterHintedSize;
    UInt                                MaxVectorHintedSize;
};



//------------------------------------------------------------------------
class GFxFontProviderFT2 : public GFxFontProvider
{
public:
    GFxFontProviderFT2(FT_Library lib=0);
    virtual ~GFxFontProviderFT2();

    FT_Library GetFT_Library() { return Lib; }

    void MapFontToFile(const char* fontName, UInt fontFlags, 
                       const char* fileName, UInt faceIndex=0, 
                       GFxFont::NativeHintingRange vectorHintingRange = GFxFont::DontHint,
                       GFxFont::NativeHintingRange rasterHintingRange = GFxFont::HintCJK, 
                       UInt maxVectorHintedSize=24,
                       UInt maxRasterHintedSize=24);

    void MapFontToMemory(const char* fontName, UInt fontFlags, 
                         const char* fontData, UInt dataSize, 
                         UInt faceIndex=0, 
                         GFxFont::NativeHintingRange vectorHintingRange = GFxFont::DontHint,
                         GFxFont::NativeHintingRange rasterHintingRange = GFxFont::HintCJK, 
                         UInt maxVectorHintedSize=24,
                         UInt maxRasterHintedSize=24);

    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);

    virtual void        LoadFontNames(GStringHash<GString>& fontnames);

private:
    struct FontType
    {
        GString                     FontName;
        UInt                        FontFlags;
        GString                     FileName;
        const char*                 FontData;
        UInt                        FontDataSize;
        UInt                        FaceIndex;
        GFxFont::NativeHintingRange RasterHintingRange;
        GFxFont::NativeHintingRange VectorHintingRange;
        UInt                        MaxRasterHintedSize;
        UInt                        MaxVectorHintedSize;
    };

    GFxExternalFontFT2* createFont(const FontType& font);

    FT_Library          Lib;
    bool                ExtLibFlag;
    GArray<FontType>    Fonts;
    UInt                NamesEndIdx;
};




#endif
