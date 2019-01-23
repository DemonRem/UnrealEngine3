/**********************************************************************

Filename    :   GStd.h
Content     :   Standard C function interface
Created     :   2/25/2007
Authors     :   Ankur Mohan, Michael Antonov, Artem Bolgar
Copyright   :   (c) 2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GStd_H
#define INC_GStd_H

#include "GTypes.h"
#include <stdarg.h> // for va_list args
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if !defined(GFC_OS_WINCE) && defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
#define GFC_MSVC_SAFESTRING
#include <errno.h>
#endif


#if defined(GFC_OS_WIN32)
inline char* GCDECL G_itoa(int val, char *dest, UPInt destsize, int radix)
{
#if defined(GFC_MSVC_SAFESTRING)
    _itoa_s(val, dest, destsize, radix);
    return dest;
#else
    GUNUSED(destsize);
    return itoa(val, dest, radix);
#endif
}
#else // GFC_OS_WIN32
inline char* G_itoa(int val, char* dest, unsigned int len, int radix)
{
    if (val == 0)
    {
        if (len > 1)
        {
            dest[0] = '0';
            dest[1] = '\0';  
        }
        return dest;
    }

    int cur = val;
    unsigned int i    = 0; 
    unsigned int sign = 0;

    if (val < 0)
    {
        val = -val;
        sign = 1;
    }

    while ((val != 0) && (i < (len - 1 - sign)))        
    {
        cur    = val % radix;
        val   /= radix;

        if (radix == 16)
        {
            switch(cur)
            {
            case 10:
                dest[i] = 'a';
                break;
            case 11:
                dest[i] = 'b';
                break;
            case 12:
                dest[i] = 'c';
                break;
            case 13:
                dest[i] = 'd';
                break;
            case 14:
                dest[i] = 'e';
                break;
            case 15:
                dest[i] = 'f';
                break;
            default:
                dest[i] = (char)('0' + cur);
                break;
            }
        } 
        else
        {
            dest[i] = (char)('0' + cur);
        }
        ++i;
    }

    if (sign)
    {
        dest[i++] = '-';
    }

    for (unsigned int j = 0; j < i / 2; ++j)
    {
        char tmp        = dest[j];
        dest[j]         = dest[i - 1 - j];
        dest[i - 1 - j] = tmp;
    }
    dest[i] = '\0';

    return dest;
}

#endif


// String functions

inline UPInt GCDECL G_strlen(const char* str)
{
    return strlen(str);
}

inline char* GCDECL G_strcpy(char* dest, UPInt destsize, const char* src)
{
#if defined(GFC_MSVC_SAFESTRING)
    strcpy_s(dest, destsize, src);
    return dest;
#else
    GUNUSED(destsize);
    return strcpy(dest, src);
#endif
}

inline char* GCDECL G_strncpy(char* dest, UPInt destsize, const char* src, UPInt count)
{
#if defined(GFC_MSVC_SAFESTRING)
    strncpy_s(dest, destsize, src, count);
    return dest;
#else
    GUNUSED(destsize);
    return strncpy(dest, src, count);
#endif
}

inline char * GCDECL G_strcat(char* dest, UPInt destsize, const char* src)
{
#if defined(GFC_MSVC_SAFESTRING)
    strcat_s(dest, destsize, src);
    return dest;
#else
    GUNUSED(destsize);
    return strcat(dest, src);
#endif
}

inline int GCDECL G_strcmp(const char* dest, const char* src)
{
    return strcmp(dest, src);
}

inline const char* GCDECL G_strchr(const char* str, char c)
{
    return strchr(str, c);
}

inline char* GCDECL G_strchr(char* str, char c)
{
    return strchr(str, c);
}

inline const char* G_strrchr(const char* str, char c)
{
    UPInt len = G_strlen(str);
    for (UPInt i=len; i>0; i--)     
        if (str[i]==c) 
            return str+i;
    return 0;
}

inline const UByte* GCDECL G_memrchr(const UByte* str, UPInt size, UByte c)
{
    for (SPInt i = (SPInt)size - 1; i >= 0; i--)     
    {
        if (str[i] == c) 
            return str + i;
    }
    return 0;
}

inline char* GCDECL G_strrchr(char* str, char c)
{
    UPInt len = G_strlen(str);
    for (UPInt i=len; i>0; i--)     
        if (str[i]==c) 
            return str+i;
    return 0;
}


double GCDECL G_strtod(const char* string, char** tailptr);

inline long GCDECL G_strtol(const char* string, char** tailptr, int radix)
{
    return strtol(string, tailptr, radix);
}

inline long GCDECL G_strtoul(const char* string, char** tailptr, int radix)
{
    return strtoul(string, tailptr, radix);
}

inline int GCDECL G_strncmp(const char* ws1, const char* ws2, UPInt size)
{
    return strncmp(ws1, ws2, size);
}

inline UInt64 GCDECL G_strtouq(const char *nptr, char **endptr, int base)
{
#if defined(GFC_CC_MSVC) && !defined(GFC_OS_WINCE)
    return _strtoui64(nptr, endptr, base);
#elif (defined(GFC_OS_PS2) && defined(GFC_CC_MWERKS)) || (defined(GFC_OS_PSP) && defined(GFC_CC_SNC)) || defined(GFC_OS_WINCE)
    return strtoul(nptr, endptr, base);
#else
    return strtoull(nptr, endptr, base);
#endif
}

inline SInt64 GCDECL G_strtoq(const char *nptr, char **endptr, int base)
{
#if defined(GFC_CC_MSVC) && !defined(GFC_OS_WINCE)
    return _strtoi64(nptr, endptr, base);
#elif (defined(GFC_OS_PS2) && defined(GFC_CC_MWERKS)) || (defined(GFC_OS_PSP) && defined(GFC_CC_SNC)) || defined(GFC_OS_WINCE)
    return strtol(nptr, endptr, base);
#else
    return strtoll(nptr, endptr, base);
#endif
}


inline SInt64 GCDECL G_atoq(const char* string)
{
#if defined(GFC_CC_MSVC) && !defined(GFC_OS_WINCE)
    return _atoi64(string);
#elif (defined(GFC_OS_PS2) && defined(GFC_CC_MWERKS)) || (defined(GFC_OS_PSP) && defined(GFC_CC_SNC)) || defined(GFC_OS_WINCE)
    return strtol(string, NULL, 10);
#elif defined(GFC_OS_PS2)
    return strtoll(string, NULL, 10);
#else
    return atoll(string);
#endif
}

inline UInt64 GCDECL G_atouq(const char* string)
{
  return G_strtouq(string, NULL, 10);
}


// Implemented in GStd.cpp in platform-specific manner.
int GCDECL G_stricmp(const char* dest, const char* src);
int GCDECL G_strnicmp(const char* dest, const char* src, UPInt count);

inline UPInt GCDECL G_sprintf(char *dest, UPInt destsize, const char* format, ...)
{
    va_list argList;
    va_start(argList,format);
    UPInt ret;
#if defined(GFC_CC_MSVC)
    #if defined(GFC_MSVC_SAFESTRING)
        ret = _vsnprintf_s(dest, destsize, _TRUNCATE, format, argList);
        GASSERT(ret != -1);
    #else
        GUNUSED(destsize);
        ret = _vsnprintf(dest, destsize - 1, format, argList); // -1 for space for the null character
        GASSERT(ret != -1);
        dest[destsize-1] = 0;
    #endif
#else
    GUNUSED(destsize);
    ret = vsprintf(dest, format, argList);
    GASSERT(ret < destsize);
#endif
    va_end(argList);
    return ret;
}

inline UPInt GCDECL G_vsprintf(char *dest, UPInt destsize, const char * format, va_list argList)
{
    UPInt ret;
#if defined(GFC_CC_MSVC)
    #if defined(GFC_MSVC_SAFESTRING)
        dest[0] = '\0';
        int rv = vsnprintf_s(dest, destsize, _TRUNCATE, format, argList);
        if (rv == -1)
        {
            dest[destsize - 1] = '\0';
            ret = destsize - 1;
        }
        else
            ret = (UPInt)rv;
    #else
        GUNUSED(destsize);
        int rv = _vsnprintf(dest, destsize - 1, format, argList);
        GASSERT(rv != -1);
        ret = (UPInt)rv;
        dest[destsize-1] = 0;
    #endif
#else
    GUNUSED(destsize);
    ret = (UPInt)vsprintf(dest, format, argList);
    GASSERT(ret < destsize);
#endif
    return ret;
}

// Wide-char funcs
#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS) && !defined(GFC_OS_PS2)
# include <wchar.h>
#endif

#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS) && !defined(GFC_OS_PS2) && !defined(GFC_CC_SNC)
# include <wctype.h>
#endif

wchar_t* GCDECL G_wcscpy(wchar_t* dest, UPInt destsize, const wchar_t* src);
wchar_t* GCDECL G_wcsncpy(wchar_t* dest, UPInt destsize, const wchar_t* src, UPInt count);
wchar_t* GCDECL G_wcscat(wchar_t* dest, UPInt destsize, const wchar_t* src);
UPInt    GCDECL G_wcslen(const wchar_t* str);
int      GCDECL G_wcscmp(const wchar_t* a, const wchar_t* b);
int      GCDECL G_wcsicmp(const wchar_t* a, const wchar_t* b);

inline int GCDECL G_wcsicoll(const wchar_t* a, const wchar_t* b)
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WII)
#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    return ::_wcsicoll(a, b);
#else
    return ::wcsicoll(a, b);
#endif
#else
    // not supported, use regular wcsicmp
    return G_wcsicmp(a, b);
#endif
}

inline int GCDECL G_wcscoll(const wchar_t* a, const wchar_t* b)
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_PS3) || defined(GFC_OS_WII)
    return ::wcscoll(a, b);
#else
    // not supported, use regular wcscmp
    return ::G_wcscmp(a, b);
#endif
}

#ifndef GFC_NO_WCTYPE

inline int GCDECL G_UnicodeCharIs(const UInt16* table, wchar_t charCode)
{
    UInt offset = table[charCode >> 8];
    if (offset == 0) return 0;
    if (offset == 1) return 1;
    return (table[offset + ((charCode >> 4) & 15)] & (1 << (charCode & 15))) != 0;
}

extern const UInt16 G_UnicodeAlnumBits[];
extern const UInt16 G_UnicodeAlphaBits[];
extern const UInt16 G_UnicodeDigitBits[];
extern const UInt16 G_UnicodeSpaceBits[];
extern const UInt16 G_UnicodeXDigitBits[];

// Uncomment if necessary
//extern const UInt16 G_UnicodeCntrlBits[];
//extern const UInt16 G_UnicodeGraphBits[];
//extern const UInt16 G_UnicodeLowerBits[];
//extern const UInt16 G_UnicodePrintBits[];
//extern const UInt16 G_UnicodePunctBits[];
//extern const UInt16 G_UnicodeUpperBits[];

inline int GCDECL G_iswalnum (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeAlnumBits,  charCode); }
inline int GCDECL G_iswalpha (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeAlphaBits,  charCode); }
inline int GCDECL G_iswdigit (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeDigitBits,  charCode); }
inline int GCDECL G_iswspace (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeSpaceBits,  charCode); }
inline int GCDECL G_iswxdigit(wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeXDigitBits, charCode); }

// Uncomment if necessary
//inline int GCDECL G_iswcntrl (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeCntrlBits,  charCode); }
//inline int GCDECL G_iswgraph (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeGraphBits,  charCode); }
//inline int GCDECL G_iswlower (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeLowerBits,  charCode); }
//inline int GCDECL G_iswprint (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodePrintBits,  charCode); }
//inline int GCDECL G_iswpunct (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodePunctBits,  charCode); }
//inline int GCDECL G_iswupper (wchar_t charCode) { return G_UnicodeCharIs(G_UnicodeUpperBits,  charCode); }

int G_towupper(wchar_t charCode);
int G_towlower(wchar_t charCode);

#else // GFC_NO_WCTYPE

inline int  GCDECL G_iswspace(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || defined(GFC_CC_SNC)
    return ((c) < 128 ? isspace((char)c) : 0);
#else
    return iswspace(c);
#endif
}

inline int  GCDECL G_iswdigit(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || defined(GFC_CC_SNC)
    return ((c) < 128 ? isdigit((char)c) : 0);
#else
    return iswdigit(c);
#endif
}

inline int  GCDECL G_iswxdigit(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || defined(GFC_CC_SNC)
    return ((c) < 128 ? isxdigit((char)c) : 0);
#else
    return iswxdigit(c);
#endif
}

inline int  GCDECL G_iswalpha(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || defined(GFC_CC_SNC)
    return ((c) < 128 ? isalpha((char)c) : 0);
#else
    return iswalpha(c);
#endif
}

inline int  GCDECL G_iswalnum(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_OS_WII) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || defined(GFC_CC_SNC)
    return ((c) < 128 ? isalnum((char)c) : 0);
#else
    return iswalnum(c);
#endif
}

inline wchar_t GCDECL G_towlower(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || (defined(GFC_OS_PS3) && defined(GFC_CC_SNC))
    return (wchar_t)tolower((char)c);
#else
    return (wchar_t)towlower(c);
#endif
}

inline wchar_t G_towupper(wchar_t c)
{
#if defined(GFC_OS_SYMBIAN) || defined(GFC_CC_RENESAS) || defined(GFC_OS_PS2) || (defined(GFC_OS_PS3) && defined(GFC_CC_SNC))
    return (wchar_t)toupper((char)c);
#else
    return (wchar_t)towupper(c);
#endif
}

#endif // GFC_NO_WCTYPE

// ASCII versions of tolower and toupper. Don't use "char"
inline int GCDECL G_tolower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

inline int GCDECL G_toupper(int c)
{
    return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}



inline double GCDECL G_wcstod(const wchar_t* string, wchar_t** tailptr)
{
#if defined(GFC_OS_PSP) || defined (GFC_OS_PS2) || defined(GFC_OS_OTHER)
    GUNUSED(tailptr);
    char buffer[64];
    char* tp = NULL;
    UPInt max = G_wcslen(string);
    if (max > 63) max = 63;
    unsigned char c = 0;
    for (UPInt i=0; i < max; i++)
    {
        c = (unsigned char)string[i];
        buffer[i] = ((c) < 128 ? (char)c : '!');
    }
    buffer[max] = 0;
    return G_strtod(buffer, &tp);
#else
    return wcstod(string, tailptr);
#endif
}

inline long GCDECL G_wcstol(const wchar_t* string, wchar_t** tailptr, int radix)
{
#if defined(GFC_OS_PSP) || defined(GFC_OS_PS2) || defined(GFC_OS_OTHER)
    GUNUSED(tailptr);
    char buffer[64];
    char* tp = NULL;
    UPInt max = G_wcslen(string);
    if (max > 63) max = 63;
    unsigned char c = 0;
    for (UPInt i=0; i < max; i++)
    {
        c = (unsigned char)string[i];
        buffer[i] = ((c) < 128 ? (char)c : '!');
    }
    buffer[max] = 0;
    return strtol(buffer, &tp, radix);
#else
    return wcstol(string, tailptr, radix);
#endif
}

#endif // INC_GSTD_H
