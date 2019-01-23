/**********************************************************************

Filename    :   GString.h
Content     :   GString UTF8 string implementation with copy-on
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

#ifndef INC_GSTRING_H
#define INC_GSTRING_H

#include "GTypes.h"
#include "GArray.h"
#include "GUTF8Util.h"
#include "GAtomic.h"
#include "GStd.h"
#include <string.h>

// ***** Classes

class GString;
class GStringLH;
class GStringDH;
class GStringBuffer;


// ***** GString Class 

// GString is UTF8 based string class with heap support. There are several
// versions of string used to manage different heaps:
//
//   GString   - Allocates memory from global heap by default.
//   GStringLH - Allocates memory from local heap based on 'this' pointer,
//               so it can only be constructed in allocated data structures.
//   GStringDH - Allocates from a heap explicitly specified in constructor
//                 argument; mostly used for temporaries which are later assigned
//                 to the local heap strings.
//
// GString is a base to the other two classes, allowing all string argument
// passing to be done by GString&, passing a specific derived class is not
// necessary. Strings do NOT mutate heaps on assignment, so a data copy will
// be made if a string is copied to a different heap; this copy and allocation
// is avoided if heaps on both sides of assignment do match.


class GString
{
    friend class GStringLH;
    friend class GStringDH;

protected:

    enum FlagConstants
    {
        //Flag_GetLength      = 0x7FFFFFFF,
        // This flag is set if GetLength() == GetSize() for a string.
        // Avoid extra scanning is Substring and indexing logic.
        Flag_LengthIsSizeShift   = (sizeof(UPInt)*8 - 1)
    };


    // Internal structure to hold string data
    struct DataDesc
    {
        // Number of bytes. Will be the same as the number of chars if the characters
        // are ascii, may not be equal to number of chars in case string data is UTF8.
        UPInt   Size;       
        volatile SInt32 RefCount;
        char    Data[1];

        void    AddRef()
        {
            GAtomicOps<SInt32>::ExchangeAdd_NoSync(&RefCount, 1);
        }
        // Decrement ref count. This needs to be thread-safe, since
        // a different thread could have also decremented the ref count.
        // For example, if u start off with a ref count = 2. Now if u
        // decrement the ref count and check against 0 in different
        // statements, a different thread can also decrement the ref count
        // in between our decrement and checking against 0 and will find
        // the ref count = 0 and delete the object. This will lead to a crash
        // when context switches to our thread and we'll be trying to delete
        // an already deleted object. Hence decrementing the ref count and
        // checking against 0 needs to made an atomic operation.
        void    Release()
        {
            if ((GAtomicOps<SInt32>::ExchangeAdd_NoSync(&RefCount, -1) - 1) == 0)
                GFREE(this);
        }

        static UPInt GetLengthFlagBit()     { return UPInt(1) << Flag_LengthIsSizeShift; }
        UPInt       GetSize() const         { return Size & ~GetLengthFlagBit() ; }
        UPInt       GetLengthFlag()  const  { return Size & GetLengthFlagBit(); }
        bool        LengthIsSize() const    { return GetLengthFlag() != 0; }
    };

    // Heap type of the string is encoded in the lower bits.
    enum HeapType
    {
        HT_Global   = 0,    // Heap is global.
        HT_Local    = 1,    // GString_loc: Heap is determined based on string's address.
        HT_Dynamic  = 2,    // GString_temp: Heap is stored as a part of the class.
        HT_Mask     = 3
    };

    union {
        DataDesc* pData;
        UPInt     HeapTypeBits;
    };
    typedef union {
        DataDesc* pData;
        UPInt     HeapTypeBits;
    } DataDescUnion;

    inline HeapType    GetHeapType() const { return (HeapType) (HeapTypeBits & HT_Mask); }

    inline DataDesc*   GetData() const
    {
        DataDescUnion u;
        u.pData    = pData;
        u.HeapTypeBits = (u.HeapTypeBits & ~(UPInt)HT_Mask);
        return u.pData;
    }
    
    inline void        SetData(DataDesc* pdesc)
    {
        HeapType ht = GetHeapType();
        pData = pdesc;
        GASSERT((HeapTypeBits & HT_Mask) == 0);
        HeapTypeBits |= ht;        
    }

    
    DataDesc*   AllocData(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize);
    DataDesc*   AllocDataCopy1(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize,
                               const char* pdata, UPInt copySize);
    DataDesc*   AllocDataCopy2(GMemoryHeap* pheap, UPInt size, UPInt lengthIsSize,
                               const char* pdata1, UPInt copySize1,
                               const char* pdata2, UPInt copySize2);

    // Special constructor to avoid data initalization when used in derived class.
    struct NoConstructor { };
    GString(const NoConstructor&) { }

public:

    // For initializing string with dynamic buffer
    struct InitStruct
    {
        virtual ~InitStruct() { }
        virtual void InitString(char* pbuffer, UPInt size) const = 0;
    };


    // Constructors / Destructors.
    GString();
    GString(const char* data);
    GString(const char* data1, const char* pdata2, const char* pdata3 = 0);
    GString(const char* data, UPInt buflen);
    GString(const GString& src);
    GString(const GStringBuffer& src);
    GString(const InitStruct& src, UPInt size);
    explicit GString(const wchar_t* data);      

    // Destructor
    ~GString()
    {
        GetData()->Release();
    }

    // Declaration of NullString
    static DataDesc NullData;

    GMemoryHeap* GetHeap() const;


    // *** General Functions

    void        Clear();

    // For casting to a pointer to char.
    operator const char*() const        { return GetData()->Data; }
    // Pointer to raw buffer.
    const char* ToCStr() const          { return GetData()->Data; }

    // Returns number of bytes
    UPInt       GetSize() const         { return GetData()->GetSize() ; }
    // Tells whether or not the string is empty
    bool        IsEmpty() const         { return GetSize() == 0; }

    // Returns  number of characters
    UPInt       GetLength() const;

    // Returns  character at the specified index
    UInt32      GetCharAt(UPInt index) const;
    UInt32      GetFirstCharAt(UPInt index, const char** offset) const;
    UInt32      GetNextChar(const char** offset) const;

    // Appends a character
    void        AppendChar(UInt32 ch);

    // Append a string
    void        AppendString(const wchar_t* pstr, SPInt len = -1);
    void        AppendString(const char* putf8str, SPInt utf8StrSz = -1);

    // Assigned a string with dynamic data (copied through initializer).
    void        AssignString(const InitStruct& src, UPInt size);
    // Assigns string with known size.
    void        AssignString(const char* putf8str, UPInt size);

    //  Resize the string to the new size
//  void        Resize(UPInt _size);

    // Removes the character at posAt
    void        Remove(UPInt posAt, SPInt len = 1);

    // Returns a GString that's a substring of this.
    //  -start is the index of the first UTF8 character you want to include.
    //  -end is the index one past the last UTF8 character you want to include.
    GString   Substring(UPInt start, UPInt end) const;

    // Case-conversion
    GString   ToUpper() const;
    GString   ToLower() const;

    // Inserts substr at posAt
    GString&    Insert (const char* substr, UPInt posAt, SPInt len = -1);

    // Inserts character at posAt
    UPInt       InsertCharAt(UInt32 c, UPInt posAt);

    // Inserts substr at posAt, which is an index of a character (not byte).
    // Of size is specified, it is in bytes.
//  GString&    Insert(const UInt32* substr, UPInt posAt, SPInt size = -1);

    // Get Byte index of the character at position = index
    UPInt       GetByteIndex(UPInt index) const { return (UPInt)GUTF8Util::GetByteIndex(index, GetData()->Data); }

    // Utility: case-insensitive string compare.  stricmp() & strnicmp() are not
    // ANSI or POSIX, do not seem to appear in Linux.
    static int  GSTDCALL    CompareNoCase(const char* a, const char* b);
    static int  GSTDCALL    CompareNoCase(const char* a, const char* b, SPInt len);

    // Hash function, case-insensitive
    static UPInt GSTDCALL   BernsteinHashFunctionCIS(const void* pdataIn, UPInt size, UPInt seed = 5381);

    // Hash function, case-sensitive
    static UPInt GSTDCALL   BernsteinHashFunction(const void* pdataIn, UPInt size, UPInt seed = 5381);

    // Encode/decode special html chars
    static void GSTDCALL EscapeSpecialHTML(const char* psrc, UPInt length, GString* pescapedStr);
    static void GSTDCALL UnescapeSpecialHTML(const char* psrc, UPInt length, GString* punescapedStr);

    // Operators
    // Assignment
    void        operator =  (const char* str);
    void        operator =  (const wchar_t* str);
    void        operator =  (const GString& src);
    void        operator =  (const GStringBuffer& src);

    // Addition
    void        operator += (const GString& src);
    void        operator += (const char* psrc)       { AppendString(psrc); }
    void        operator += (const wchar_t* psrc)    { AppendString(psrc); }
    void        operator += (char  ch)               { AppendChar(ch); }
    GString   operator +    (const char* str) const ;
    GString   operator +    (const GString& src)  const ;

    // Comparison
    bool        operator == (const GString& str) const
    {
        return (G_strcmp(GetData()->Data, str.GetData()->Data)== 0);
    }

    bool        operator != (const GString& str) const
    {
        return !operator == (str);
    }

    bool        operator == (const char* str) const
    {
        return G_strcmp(GetData()->Data, str) == 0;
    }

    bool        operator != (const char* str) const
    {
        return !operator == (str);
    }

    bool        operator <  (const char* pstr) const
    {
        return G_strcmp(GetData()->Data, pstr) < 0;
    }

    bool        operator <  (const GString& str) const
    {
        return *this < str.GetData()->Data;
    }

    bool        operator >  (const char* pstr) const
    {
        return G_strcmp(GetData()->Data, pstr) > 0;
    }

    bool        operator >  (const GString& str) const
    {
        return *this > str.GetData()->Data;
    }

    int CompareNoCase(const char* pstr) const
    {
        return CompareNoCase(GetData()->Data, pstr);
    }
    int CompareNoCase(const GString& str) const
    {
        return CompareNoCase(GetData()->Data, str.ToCStr());
    }

    // Accesses raw bytes
    const char&     operator [] (int index) const
    {
        GASSERT(index >= 0 && (UPInt)index < GetSize());
        return GetData()->Data[index];
    }
    const char&     operator [] (UPInt index) const
    {
        GASSERT(index < GetSize());
        return GetData()->Data[index];
    }


    // Case insensitive keys are used to look up insensitive string in hash tables
    // for SWF files with version before SWF 7.
    struct NoCaseKey
    {   
        const GString* pStr;
        NoCaseKey(const GString &str) : pStr(&str){};
    };

    bool    operator == (const NoCaseKey& strKey) const
    {
        return (CompareNoCase(ToCStr(), strKey.pStr->ToCStr()) == 0);
    }
    bool    operator != (const NoCaseKey& strKey) const
    {
        return !(CompareNoCase(ToCStr(), strKey.pStr->ToCStr()) == 0);
    }

    // Hash functor used for strings.
    struct HashFunctor
    {    
        UPInt  operator()(const GString& data) const
        {
            UPInt  size = data.GetSize();
            return GString::BernsteinHashFunction((const char*)data, size);
        }        
    };
    // Case-insensitive hash functor used for strings. Supports additional
    // lookup based on NoCaseKey.
    struct NoCaseHashFunctor
    {    
        UPInt  operator()(const GString& data) const
        {
            UPInt  size = data.GetSize();
            return GString::BernsteinHashFunctionCIS((const char*)data, size);
        }
        UPInt  operator()(const NoCaseKey& data) const
        {       
            UPInt  size = data.pStr->GetSize();
            return GString::BernsteinHashFunctionCIS((const char*)data.pStr->ToCStr(), size);
        }
    };

};



// ***** Local Heap String - GStringLH

// A structure-local version of GString, which determined its own memory
// heap based on 'this' pointer location. This class can not be created on
// stack or outside of an allocated memory heap. If cross-heap string assignment
// takes place, a copy of string data may be created.

class GStringLH : public GString
{
    // Initializes pData with Local heap flag.
    void        SetDataLcl(DataDesc* pdesc)
    {        
        pData = pdesc;
        GASSERT((HeapTypeBits & HT_Mask) == 0);
        HeapTypeBits |= HT_Local;
    }

    void CopyConstructHelper(const GString& src);

public:
    // Constructors / Destructors.
    GStringLH();
    GStringLH(const char* data);    
    GStringLH(const char* data, UPInt buflen);
    // We MUST have copy constructor, or compiler will generate an incorrect one calling base.    
    GStringLH(const GStringLH& src) : GString(NoConstructor())        { CopyConstructHelper(src); }
    explicit GStringLH(const GString& src) : GString(NoConstructor()) { CopyConstructHelper(src); }
    explicit GStringLH(const InitStruct& src, UPInt size);
    explicit GStringLH(const wchar_t* data);        

    // Assignment
    void        operator =  (const char* str)       { GString::operator = (str); }
    void        operator =  (const wchar_t* str)    { GString::operator = (str); }
    void        operator =  (const GString& src)  { GString::operator = (src); }
    void        operator =  (const GStringBuffer& src) { GString::operator = (src);  }
};


// ***** Dynamic Heap String - GStringDH

// A dynamic heap version of GString, which stores its memory heap in an
// internal pointer. Note that assigning the string does NOT copy the heap.

class GStringDH : public GString
{    
    GMemoryHeap* pHeap;

    // Initializes pData with Local heap flag.
    void        SetDataLcl(DataDesc* pdesc)
    {        
        pData = pdesc;
        GASSERT((HeapTypeBits & HT_Mask) == 0);
        HeapTypeBits |= HT_Dynamic;
    }

      void CopyConstructHelper(const GString& src, GMemoryHeap* pheap);

public:
    // Constructors / Destructors.
    GStringDH(GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    GStringDH(GMemoryHeap* pheap, const char* data);
    GStringDH(GMemoryHeap* pheap, const char* data1, const char* pdata2, const char* pdata3 = 0);
    GStringDH(GMemoryHeap* pheap, const char* data, UPInt buflen);
    GStringDH(const GStringDH& src) : GString(NoConstructor())                                { CopyConstructHelper(src, src.GetHeap()); }
    explicit GStringDH(const GString& src) : GString(NoConstructor())                         { CopyConstructHelper(src, 0); }
    explicit GStringDH(GMemoryHeap* pheap, const GString& src) : GString(NoConstructor())     { CopyConstructHelper(src, pheap); }
    explicit GStringDH(GMemoryHeap* pheap, const InitStruct& src, UPInt size);
    explicit GStringDH(GMemoryHeap* pheap, const wchar_t* data);
    
    void        operator =  (const char* str)       { GString::operator = (str); }
    void        operator =  (const wchar_t* str)    { GString::operator = (str); }
    void        operator =  (const GString& src)    { GString::operator = (src); }
    void        operator =  (const GStringBuffer& src) { GString::operator = (src);  }

    GMemoryHeap* GetHeap() const { return pHeap; }
};




// ***** String Buffer used for Building Strings

class GStringBuffer
{
    char*           pData;
    UPInt           Size;
    UPInt           BufferSize;
    UPInt           GrowSize;    
    mutable bool    LengthIsSize;
    GMemoryHeap*    pHeap;

public:

    // Constructors / Destructor.
    GStringBuffer(GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    explicit GStringBuffer(UPInt growSize,        GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    GStringBuffer(const char* data,               GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    GStringBuffer(const char* data, UPInt buflen, GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    GStringBuffer(const GString& src,           GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    GStringBuffer(const GStringBuffer& src,     GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    explicit GStringBuffer(const wchar_t* data,   GMemoryHeap* pheap = GMemory::GetGlobalHeap());
    ~GStringBuffer();
    

    GMemoryHeap* GetHeap() const { return pHeap; }

    // Modify grow size used for growing/shrinking the buffer.
    UPInt       GetGrowSize() const         { return GrowSize; }
    void        SetGrowSize(UPInt growSize);
    

    // *** General Functions
    // Does not release memory, just sets Size to 0
    void        Clear();

    // For casting to a pointer to char.
    operator const char*() const        { return (pData) ? pData : ""; }
    // Pointer to raw buffer.
    const char* ToCStr() const          { return (pData) ? pData : ""; }

    // Returns number of bytes.
    UPInt       GetSize() const         { return Size ; }
    // Tells whether or not the string is empty.
    bool        IsEmpty() const         { return GetSize() == 0; }

    // Returns  number of characters
    UPInt       GetLength() const;

    // Returns  character at the specified index
    UInt32      GetCharAt(UPInt index) const;
    UInt32      GetFirstCharAt(UPInt index, const char** offset) const;
    UInt32      GetNextChar(const char** offset) const;


    //  Resize the string to the new size
    void        Resize(UPInt _size);
    void        Reserve(UPInt _size);

    // Appends a character
    void        AppendChar(UInt32 ch);

    // Append a string
    void        AppendString(const wchar_t* pstr, SPInt len = -1);
    void        AppendString(const char* putf8str, SPInt utf8StrSz = -1);

    // Assigned a string with dynamic data (copied through initializer).
    //void        AssignString(const InitStruct& src, UPInt size);

    // Inserts substr at posAt
    void        Insert (const char* substr, UPInt posAt, SPInt len = -1);
    // Inserts character at posAt
    UPInt       InsertCharAt(UInt32 c, UPInt posAt);

    // Assignment
    void        operator =  (const char* str);
    void        operator =  (const wchar_t* str);
    void        operator =  (const GString& src);

    // Addition
    void        operator += (const GString& src)   { AppendString(src.ToCStr(),src.GetSize()); }
    void        operator += (const char* psrc)       { AppendString(psrc); }
    void        operator += (const wchar_t* psrc)    { AppendString(psrc); }
    void        operator += (char  ch)               { AppendChar(ch); }
    //GString   operator +  (const char* str) const ;
    //GString   operator +  (const GString& src)  const ;

    // Accesses raw bytes
    char&       operator [] (int index)
    {
        GASSERT(((UPInt)index) < GetSize());
        return pData[index];
    }
    char&       operator [] (UPInt index)
    {
        GASSERT(index < GetSize());
        return pData[index];
    }

    const char&     operator [] (int index) const 
    {
        GASSERT(((UPInt)index) < GetSize());
        return pData[index];
    }
    const char&     operator [] (UPInt index) const
    {
        GASSERT(index < GetSize());
        return pData[index];
    }
};


//
// Wrapper for string data. The data must have a guaranteed 
// lifespan throughout the usage of the wrapper. Not intended for 
// cached usage. Not thread safe.
//
class GStringDataPtr
{
public:
    GStringDataPtr() : pStr(NULL), Size(0) {}
    GStringDataPtr(const GStringDataPtr& p)
        : pStr(p.pStr), Size(p.Size) {}
    GStringDataPtr(const char* pstr, UPInt sz)
        : pStr(pstr), Size(sz) {}
    GStringDataPtr(const char* pstr)
        : pStr(pstr), Size((pstr != NULL) ? G_strlen(pstr) : 0) {}
    explicit GStringDataPtr(const GString& str)
        : pStr(str.ToCStr()), Size(str.GetSize()) {}
    template <typename T, int N> 
    GStringDataPtr(const T (&v)[N])
        : pStr(v), Size(N) {}

public:
    const char* ToCStr() const { return pStr; }
    UPInt       GetSize() const { return Size; }
    bool        IsEmpty() const { return GetSize() == 0; }

    // value is a prefix of this string
    // Character's values are not compared.
    bool        IsPrefix(const GStringDataPtr& value) const
    {
        return ToCStr() == value.ToCStr() && GetSize() >= value.GetSize();
    }
    // value is a suffix of this string
    // Character's values are not compared.
    bool        IsSuffix(const GStringDataPtr& value) const
    {
        return ToCStr() <= value.ToCStr() && (End()) == (value.End());
    }

    SPInt       FindChar(char c) const 
    {
        for (UPInt i = 0; i < GetSize(); ++i)
        {
            if (pStr[i] == c)
                return static_cast<SPInt>(i);
        }

        return -1; 
    }

    // Create new object and trim size bytes from the left.
    GStringDataPtr  GetTrimLeft(UPInt size) const
    {
        // Limit trim size to the size of the string.
        size = G_Min(GetSize(), size);

        return GStringDataPtr(ToCStr() + size, GetSize() - size);
    }
    // Create new object and trim size bytes from the right.
    GStringDataPtr  GetTrimRight(UPInt size) const
    {
        // Limit trim to the size of the string.
        size = G_Min(GetSize(), size);

        return GStringDataPtr(ToCStr(), GetSize() - size);
    }

    // Create new object, which contains next token.
    // Useful for parsing.
    GStringDataPtr GetNextToken(char separator = ':') const
    {
        UPInt cur_pos = 0;
        const char* cur_str = ToCStr();

        for (; cur_pos < GetSize() && cur_str[cur_pos]; ++cur_pos)
        {
            if (cur_str[cur_pos] == separator)
            {
                break;
            }
        }

        return GStringDataPtr(ToCStr(), cur_pos);
    }

    // Trim size bytes from the left.
    GStringDataPtr& TrimLeft(UPInt size)
    {
        // Limit trim size to the size of the string.
        size = G_Min(GetSize(), size);
        pStr += size;
        Size -= size;

        return *this;
    }
    // Trim size bytes from the right.
    GStringDataPtr& TrimRight(UPInt size)
    {
        // Limit trim to the size of the string.
        size = G_Min(GetSize(), size);
        Size -= size;

        return *this;
    }

    const char* Begin() const { return ToCStr(); }
    const char* End() const { return ToCStr() + GetSize(); }

    // Hash functor used string data pointers
    struct HashFunctor
    {    
        UPInt operator()(const GStringDataPtr& data) const
        {
            return GString::BernsteinHashFunction(data.ToCStr(), data.GetSize());
        }        
    };

    bool operator== (const GStringDataPtr& data) const 
    {
        return (G_strncmp(pStr, data.pStr, data.Size) == 0);
    }

protected:
    const char* pStr;
    UPInt       Size;
};


#endif
