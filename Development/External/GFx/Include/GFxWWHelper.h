/**********************************************************************

Filename    :   GFxWWHelper.h
Content     :   Default implementation of Asian (Japanese, Korean, Chinese) word-wrapping
Created     :   7/22/2009
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXWWHELPER_H
#define INC_GFXWWHELPER_H

#include "GTypes.h"
#include "GStd.h"

// Word-wrapping helper class.
class GFxWWHelper
{
public:
    enum BreakInfoFlags
    {
        CBI_NonStartingChar    = 1,
        CBI_NonTerminatingChar = 2
    };
    enum WordWrappingTypes
    {
        WWT_Default       = 0,   // OnWordWrapping will not be invoked
        WWT_Asian         = 0x1, // mostly Chinese
        WWT_Prohibition   = 0x2, // Japanese prohibition rule
        WWT_NoHangulWrap  = 0x4, // Korean-specific rule
        WWT_Last          = WWT_NoHangulWrap,
        WWT_All           = (WWT_Asian|WWT_Prohibition|WWT_NoHangulWrap)
    };
    struct CharBreakInfo 
    { 
        wchar_t Char;
        UByte   Flags; // see BreakInfoFlags
    }; 
    static CharBreakInfo CharBreakInfoArray[];

    static bool    GSTDCALL FindCharWithFlags(UInt wwMode, wchar_t c, UInt charBreakFlags);
    static bool    GSTDCALL IsAsianChar(UInt wwMode, wchar_t c)
    {
        if (wwMode & WWT_NoHangulWrap)
        {
            if ((c >= 0x1100 && c <= 0x11FF) ||   // Hangul Jamo
                (c >= 0x3130 && c <= 0x318F) ||   // Hangul Compatibility Jamo
                (c >= 0xAC00 && c <= 0xD7A3))     // Hangul Syllables
                return false;
        }

        return
            ((c >= 0x1100 && c <= 0x11FF) ||       // Hangul Jamo
            (c >= 0x3000 && c <= 0xD7AF) ||       // CJK / Hangul Syllables
            (c >= 0xF900 && c <= 0xFAFF) ||       // CJK Compatibility Ideographs
            (c >= 0xFF00 && c <= 0xFFDC));        // Halfwidth / Fullwidth
    }
    static bool GSTDCALL IsNonStartingChar(UInt wwMode, wchar_t c) 
    { 
        return FindCharWithFlags(wwMode, c, CBI_NonStartingChar); 
    }
    static bool GSTDCALL IsNonTerminatingChar(UInt wwMode, wchar_t c)    
    { 
        return FindCharWithFlags(wwMode, c, CBI_NonTerminatingChar); 
    }
    static bool GSTDCALL IsWhiteSpaceChar(wchar_t c) 
    { 
        return c == L'\t' || c == L'\r' || c == L' ' || c == 0x3000; 
    }
    static bool GSTDCALL IsLineFeedChar(wchar_t c)   
    { 
        return c == L'\n'; 
    }
    static bool GSTDCALL IsLineBreakOpportunityAt(UInt wwMode, const wchar_t* pwstr, UPInt index)
    {
        if (index == 0) // no breaks at position 0, no empty line
            return false;

        return IsLineBreakOpportunityAt(wwMode, pwstr[index-1], pwstr[index]);
    }
    static bool GSTDCALL IsLineBreakOpportunityAt(UInt wwMode, wchar_t prevChar, wchar_t curChar)
    {
        if (prevChar == 0) // no breaks at position 0, no empty line
            return false;

        return 
            ((IsWhiteSpaceChar(prevChar) || IsAsianChar(wwMode, curChar) ||
            IsAsianChar(wwMode, prevChar) || prevChar == L'-')) &&
            !IsNonStartingChar(wwMode, curChar) && !IsNonTerminatingChar(wwMode, prevChar);
    }
    static UPInt GSTDCALL FindNextNonWhiteSpace(const wchar_t* pwstr, UPInt pos, UPInt maxPos)
    {
        while (IsWhiteSpaceChar(pwstr[pos]))
            ++pos;
        if (pwstr && IsLineFeedChar(pwstr[pos]))
            ++pos;

        return (pos <= maxPos) ? pos : GFC_MAX_UPINT;
    }
    static UPInt GSTDCALL FindPrevNonWhiteSpace(const wchar_t* pwstr, UPInt pos)
    {
        SPInt p = (SPInt)pos;
        while (p >= 0 && (IsWhiteSpaceChar(pwstr[p]) || IsLineFeedChar(pwstr[p])))
            --p;
        return (p < 0) ? GFC_MAX_UPINT : (UPInt)p;
    }
    static bool GSTDCALL IsVowel(wchar_t c)
    {
        c = (wchar_t)G_towlower(c);
        if (c == 'a' || c == 'e' || c == 'o' || c == 'u' || c == 'i')
            return true;
        return false;
    }
    // Finds word wrap position according to wwMode bitmask. Returns new word wrap position
    // in the line (pos 0 corresponds to lineStartPos); returns GFC_MAX_UPINT if can't find the pos.
    static UPInt GSTDCALL FindWordWrapPos
        (UInt wwMode, UPInt wordWrapPos, const wchar_t* pparaText, UPInt paraLen, UPInt lineStartPos, UPInt lineLen);
};
#endif // INC_GFXWWHELPER_H

