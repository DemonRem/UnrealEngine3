/**********************************************************************

Filename    :   GFxTextureFont.h
Content     :   Application-created texture fonts.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxTextureFont_H
#define INC_GFxTextureFont_H

#include "GFxFont.h"
#include "GFxImageResource.h"
#include "GFxLoader.h"

class GFxTextureFont : public GFxFont
{
public:
    struct AdvanceEntry
    {
        Float       Advance;
        SInt16      Left;   // offset by X-axis, in twips
        SInt16      Top;    // offset by Y-axis, in twips
        UInt16      Width;  // width, in twips
        UInt16      Height; // height, in twips
    };

    enum TextureFlags
    {
        TF_DistanceFieldAlpha = 1
    };

    GFxTextureFont(const char* name, UInt fontFlags, UInt numGlyphs);
    ~GFxTextureFont();

    void                    SetTextureParams(GFxFontPackParams::TextureConfig params, UInt texflags, UInt padding);
    void                    AddTextureGlyph(UInt glyphIndex, GFxImageResource* pimage,
                                            GRenderer::Rect UvBounds, GRenderer::Point UvOrigin, AdvanceEntry advance);
    inline void             SetCharMap(UInt16 from, UInt16 to)
    {
        CodeTable.Add(from, to);
    }
    void                    SetFontMetrics(Float leading, Float ascent, Float descent)
    {
        Leading = leading;
        Ascent  = ascent;
        Descent = descent;
    }

    // *** GFxFont implementation

    GFxTextureGlyphData*    GetTextureGlyphData() const { return pTextureGlyphData;  }
    int                     GetGlyphIndex(UInt16 code) const;
    bool                    IsHintedVectorGlyph(UInt, UInt) const { return false; }
    bool                    IsHintedRasterGlyph(UInt, UInt) const { return false; }
    GFxShapeBase*           GetGlyphShape(UInt glyphIndex, UInt glyphSize) { GUNUSED2(glyphSize, glyphIndex); return 0; }
    GFxGlyphRaster*         GetGlyphRaster(UInt, UInt) { return 0; }
    Float                   GetAdvance(UInt glyphIndex) const;
    Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    int                     GetCharValue(UInt glyphIndex) const;

    UInt                    GetGlyphShapeCount() const      { return (UInt)Glyphs.GetSize(); }

    Float                   GetGlyphWidth(UInt glyphIndex) const;
    Float                   GetGlyphHeight(UInt glyphIndex) const;
    GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    virtual const char*     GetName() const         { return Name; }

    // Only used for diagnostic purpose
    virtual GString         GetCharRanges() const { return GString(); }


    // *** Other APIs

    inline bool             HasVectorOrRasterGlyphs() const { return true; }    


private:
    // General font information. 
    char*                           Name;

    GPtr<GFxTextureGlyphData>       pTextureGlyphData;
    GArrayLH<GPtr<GFxShapeBase> >   Glyphs;

    // This table maps from Unicode character number to glyph index.
    // CodeTable[CharacterCode] = GlyphIndex    
    GHashIdentityLH<UInt16, UInt16> CodeTable;

    GArrayLH<AdvanceEntry>          AdvanceTable;

    struct KerningPair
    {           
        UInt16  Char0, Char1;
        bool    operator==(const KerningPair& k) const
        { return Char0 == k.Char0 && Char1 == k.Char1; }
    };
    GHashLH<KerningPair, Float, GFixedSizeHash<KerningPair> >  KerningPairs;
};

#endif
