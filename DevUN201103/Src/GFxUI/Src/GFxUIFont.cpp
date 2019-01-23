/**********************************************************************

Filename    :   GFxUIFont.cpp
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

#include "GFxUI.h"

#if WITH_GFx

#include "GFxUIEngine.h"
#include "GFxUIFont.h"
#include "GFxUIRenderer.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GFxLoader.h"
#include "GFxTextureFont.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

FGFxUFontProvider::FGFxUFontProvider()
{
}
FGFxUFontProvider::~FGFxUFontProvider()
{
}

GFxFont* FGFxUFontProvider::CreateFont(const char* name, UInt fontFlags)
{
    if (!GGFxEngine)
        return NULL;

    UFont* Font = LoadObject<UFont>(NULL, *FString(name), NULL, LOAD_None, NULL);
    if (!Font)
        return NULL;

    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont | GFxFont::FF_GlyphShapesStripped;

    const INT PageIndex = Font->GetResolutionPageIndex(800);
    const FLOAT FontScale = Font->GetScalingFactor(800);

	if ( Font->EmScale == 0 )
	{
		warnf( NAME_Warning, TEXT("The font %s has an EmScale of 0. It will render as blanks; please reimport font."), *FString(name) );
	}

    Float XScale = Font->EmScale;
    Float TexScale = 1536.f / XScale;

    FGFxRenderer* Renderer = GGFxEngine->GetRenderer();
    GFxTextureFont* OutFont = new GFxTextureFont(name, fontFlags, Font->NumCharacters);

    OutFont->SetFontMetrics(Font->Leading, Font->Ascent, Font->Descent);

    TArray<GFxImageResource*> Textures;
    for (INT Tex = 0; Tex < Font->Textures.Num(); Tex++)
    {
        FGFxTexture* Texture = Renderer->CreateTexture();
        Texture->InitTexture(Font->Textures(Tex));
        Textures.AddItem(new GFxImageResource(new GImageInfo(Texture, Font->Textures(Tex)->SizeX, Font->Textures(Tex)->SizeY)));

        if (Tex == 0)
        {
            GFxFontPackParams::TextureConfig TexParams;
            TexParams.NominalSize = appTrunc(TexScale);
            TexParams.TextureWidth = Font->Textures(Tex)->SizeX;
            TexParams.TextureHeight = Font->Textures(Tex)->SizeY;
            OutFont->SetTextureParams(TexParams,
                Font->ImportOptions.bUseDistanceFieldAlpha ? GFxTextureFont::TF_DistanceFieldAlpha : 0,
                Min<INT>(Font->ImportOptions.XPadding,Font->ImportOptions.YPadding));
        }

        Texture->Release();
    }

    for (INT GlyphIndex = 0; GlyphIndex < Font->Characters.Num(); GlyphIndex++)
    {
        const FFontCharacter&           Glyph = Font->Characters(PageIndex + GlyphIndex);
        UTexture2D*                     Texture = Font->Textures(Glyph.TextureIndex);
        GFxTextureFont::AdvanceEntry    Advance;
        GRenderer::Rect                 Bounds (
            Float(Glyph.StartU)/Float(Texture->OriginalSizeX),             Float(Glyph.StartV)/Float(Texture->OriginalSizeY),
            Float(Glyph.StartU+Glyph.USize)/Float(Texture->OriginalSizeX), Float(Glyph.StartV+Glyph.VSize)/Float(Texture->OriginalSizeY));
        GRenderer::Point                Origin (Bounds.Left,Bounds.Top-(Glyph.VerticalOffset-Font->Ascent/XScale)/Float(Texture->OriginalSizeY));

        Advance.Advance = (Glyph.USize + Font->Kerning) * XScale;
        Advance.Left = 0;
        Advance.Top = appTrunc(-Glyph.VerticalOffset*XScale+Font->Ascent);
        Advance.Width = appTrunc(Glyph.USize * XScale);
        Advance.Height = appTrunc(Glyph.VSize * XScale);

        OutFont->AddTextureGlyph(GlyphIndex, Textures(Glyph.TextureIndex), Bounds, Origin, Advance);
    }

    if (Font->IsRemapped)
        for (TMap<WORD,WORD>::TIterator It(Font->CharRemap); It; ++It)
            OutFont->SetCharMap(It.Key(), It.Value());
    else
        for (INT Ch = 0; Ch < Font->Characters.Num(); Ch++)
            OutFont->SetCharMap(Ch, Ch);

    return OutFont;
}

void FGFxUFontProvider::LoadFontNames(GStringHash<GString>& fontnames)
{

}


#endif
