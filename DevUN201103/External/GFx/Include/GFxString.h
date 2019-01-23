/**********************************************************************

Filename    :   GFxString.h
Content     :   GFxString UTF8 string implementation with copy-on
                write semantics (thread-safe for assignment but not
                modification).
Created     :   April 27, 2007
Authors     :   Ankur Mohan, Michael Antonov

Notes       :
History     :

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxString_H
#define INC_GFxString_H

#include "GString.h"

// GFxString... classes have been renamed to GString.. classes and moved to GKernel
// typedefs are here only for backward compatibility


typedef GString       GFxString;
typedef GStringLH     GFxStringLH;
typedef GStringDH     GFxStringDH;
typedef GStringBuffer GFxStringBuffer;
typedef GStringDataPtr GFxStringDataPtr;



// ***** GFxWStringBuffer Class

// A class used to store a string buffer of wchar_t. This is used by GFxTranslator
// interface, or when a variable size Unicode character buffer is necessary.

class GFxWStringBuffer
{
public:

    // Reservation header, keeps size and reservation pointer. This structure allows
    // us to not have the rest of the class be a template.
    struct ReserveHeader
    {
        wchar_t* pBuffer;
        UPInt    Size;

        ReserveHeader() : pBuffer(0), Size(0) { }
        ReserveHeader(wchar_t* pbuffer, UPInt size) : pBuffer(pbuffer), Size(size) { }
        ReserveHeader(const ReserveHeader &other) : pBuffer(other.pBuffer), Size(other.Size) { }
    };
    // Buffer data reservation object. Creates a buffer of fixed size,
    // must be passed on constructor.
    template<unsigned int size>
    struct Reserve : public ReserveHeader
    {
        wchar_t  Buffer[size];
        Reserve() : ReserveHeader(Buffer, size) { }
    };

private:
    wchar_t*      pText;
    UPInt         Length;
    ReserveHeader Reserved;

public:

    GFxWStringBuffer()
        : pText(0), Length(0), Reserved(0,0) { }
    GFxWStringBuffer(const ReserveHeader& reserve)
        : pText(reserve.pBuffer), Length(0), Reserved(reserve) { }
    GFxWStringBuffer(const GFxWStringBuffer& other);

    ~GFxWStringBuffer();

    // Resize the buffer to desired size.
    bool     Resize(UPInt size);
    // Resets buffer contents to empty.
    void     Clear() { Resize(0); }

    inline const wchar_t* ToWStr() const { return (pText != 0) ? pText : L""; }
    inline UPInt    GetLength() const    { return Length; }
    inline wchar_t* GetBuffer() const    { return pText; }

    // Character access and mutation.
    inline wchar_t operator [] (UPInt index) const { GASSERT(pText && (index <= Length)); return pText[index]; }
    inline wchar_t& operator [] (UPInt index)            { GASSERT(pText && (index <= Length)); return pText[index]; }

    // Assign buffer data.
    GFxWStringBuffer& operator = (const GFxWStringBuffer& buff);
    GFxWStringBuffer& operator = (const GString& str);
    GFxWStringBuffer& operator = (const wchar_t *pstr);
    GFxWStringBuffer& operator = (const char* putf8str);

    void ResizeInternal(UPInt _size, bool flag = false);

    void SetString(const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
    void SetString(const char* putf8str, UPInt length = GFC_MAX_UPINT);

    void StripTrailingNewLines();
};


template <int sz>
class GFxWString_Reserve
{
protected:
    GFxWStringBuffer::Reserve<sz> Res;
};

template <int sz>
class GFxWStringBufferReserve : GFxWString_Reserve<sz>, public GFxWStringBuffer
{   
public:
    GFxWStringBufferReserve(const char* putf8str) : GFxWString_Reserve<sz>(), GFxWStringBuffer(GFxWString_Reserve<sz>::Res) 
    { 
        *(GFxWStringBuffer*)this = putf8str; 
    }
        GFxWStringBufferReserve(const GString &str) : GFxWString_Reserve<sz>(), GFxWStringBuffer(GFxWString_Reserve<sz>::Res) 
    { 
        *(GFxWStringBuffer*)this = str; 
}
};


#endif
