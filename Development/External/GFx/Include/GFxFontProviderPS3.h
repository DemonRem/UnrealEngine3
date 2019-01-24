/**********************************************************************

Filename    :   GFxFontProviderPS3.h
Content     :   PS3 Cell font provider
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

#ifndef INC_GFxFontProviderPS3_H
#define INC_GFxFontProviderPS3_H

#include <cell/font/libfont.h>
#include <cell/font/libfontFT.h>
#include <cell/font/ps3fontset.h>

#include "GFxFont.h"
#include "GFxLoader.h"
#include "GFxString.h"

//------------------------------------------------------------------------
class GFxFontProviderPS3;
class GFxPathPacker;
class GFxShapeNoStyles;


struct GFxFontInfoPS3
{
    GString      FontName;
    UInt         FontFlags;
    GString      FileName;
    const char*  FontData;
    UInt         FontDataSize;
    uint32_t     SubNum;
    int32_t      UniqueId;
    CellFontType Type;

    GFxFont::NativeHintingRange RasterHintingRange;
    GFxFont::NativeHintingRange VectorHintingRange;
    UInt                        MaxRasterHintedSize;
    UInt                        MaxVectorHintedSize;
};


class GFxExternalFontPS3 : public GFxFont
{
    enum { FontHeight = 1024, ShapePageSize = 256-2 - 8-4 };
public:
    GFxExternalFontPS3(GFxFontProviderPS3* pprovider, 
                       const CellFontLibrary* library, 
                       GFxFontInfoPS3* fontInfo);

    virtual ~GFxExternalFontPS3();

    bool    IsValid() const { return FontOK == CELL_OK; }
    
    virtual GFxTextureGlyphData* GetTextureGlyphData() const { return 0; }

    virtual int             GetGlyphIndex(UInt16 code) const;
    virtual bool            IsHintedVectorGlyph(UInt glyphIndex, UInt glyphSize) const;
    virtual bool            IsHintedRasterGlyph(UInt glyphIndex, UInt glyphSize) const;
    virtual GFxShapeBase*   GetGlyphShape(UInt glyphIndex, UInt glyphSize);
    virtual GFxGlyphRaster* GetGlyphRaster(UInt glyphIndex, UInt glyphSize);
    virtual Float           GetAdvance(UInt glyphIndex) const;
    virtual Float           GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    virtual Float           GetGlyphWidth(UInt glyphIndex) const;
    virtual Float           GetGlyphHeight(UInt glyphIndex) const;
    virtual GRectF&         GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    virtual const char*     GetName() const { return &Name[0]; }

private:
    void    setFontMetrics();
    bool    decomposeGlyphOutline(const CellFontGlyph* glyph, GFxShapeNoStyles* shape);
//    void    decomposeGlyphBitmap(const FT_Bitmap& bitmap, int x, int y, GFxGlyphRaster* raster);

/*
    static inline int FtToTwips(int v)
    {
        return (v * 20) >> 6;
    }

    static inline int FtToS1024(int v)
    {
        return v >> 6;
    }
*/

    struct GlyphType
    {
        UInt                    Code;
        //UInt                    FtIndex;
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
    GPtr<GFxFontProviderPS3>            pFontProvider;    
    GString                             Name;
    CellFont                            Font;
    GArray<GlyphType>                   Glyphs;
    GHashIdentity<UInt16, UInt>         CodeTable;
    GHash<KerningPairType, Float>       KerningPairs;
    GPtr<GFxGlyphRaster>                pRaster;
    UInt                                LastFontHeight;
    NativeHintingRange                  RasterHintingRange;
    NativeHintingRange                  VectorHintingRange;
    UInt                                MaxRasterHintedSize;
    UInt                                MaxVectorHintedSize;
    int                                 FontOK;
};



//------------------------------------------------------------------------
class GFxFontProviderPS3 : public GFxFontProvider
{
public:
    GFxFontProviderPS3(UInt fontCacheSize=1024*1024, UInt numUserFontEntries=8, UInt firstId=0);
    virtual ~GFxFontProviderPS3();

    void MapSystemFont(const char* fontName, UInt fontFlags, 
                       uint32_t fontType, uint32_t fontMap,
                       GFxFont::NativeHintingRange vectorHintingRange = GFxFont::DontHint,
                       GFxFont::NativeHintingRange rasterHintingRange = GFxFont::HintCJK, 
                       UInt maxVectorHintedSize=24,
                       UInt maxRasterHintedSize=24);

    void MapFontToFile(const char* fontName, UInt fontFlags, 
                       const char* fileName, uint32_t subNum=0,
                       GFxFont::NativeHintingRange vectorHintingRange = GFxFont::DontHint,
                       GFxFont::NativeHintingRange rasterHintingRange = GFxFont::HintCJK, 
                       UInt maxVectorHintedSize=24,
                       UInt maxRasterHintedSize=24);

    void MapFontToMemory(const char* fontName, UInt fontFlags, 
                         const char* fontData, UInt dataSize, 
                         uint32_t subNum=0,
                         GFxFont::NativeHintingRange vectorHintingRange = GFxFont::DontHint,
                         GFxFont::NativeHintingRange rasterHintingRange = GFxFont::HintCJK, 
                         UInt maxVectorHintedSize=24,
                         UInt maxRasterHintedSize=24);

    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags);

    virtual void        LoadFontNames(GStringHash<GString>& fontnames);

private:
    GFxExternalFontPS3* createFont(const GFxFontInfoPS3& font);

    GArray<uint32_t>            FontFileCache;
    GArray<CellFontEntry>       UserFontEntries;
    CellFontConfig              FontConfig;
    CellFontLibraryConfigFT     FtConfig;
    const CellFontLibrary*      Library;
    int32_t                     NextFontId;

    int                         FontOK;

    GArray<GFxFontInfoPS3>      Fonts;
};


#endif
