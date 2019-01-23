/**********************************************************************

Filename    :   GMsgFormat.h 
Content     :   Formatting of strings
Created     :   January 26, 2009
Authors     :   Sergey Sikorskiy

Notes       :   

History     :   

Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FORMAT_STR_H
#define INC_FORMAT_STR_H

#include "GString.h"

#include "GArray.h"
#include "GStringHash.h"

#include "GStackMemPool.h"
#include "GLocale.h"

#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////
// Forward declaration.

// ***** GFmtInfo
// Formatter traits class.
template <typename T> struct GFmtInfo;

class GMsgFormat;


////////////////////////////////////////////////////////////////////////////////

// ***** GFmtResource

// Base class for all formatting resources.
// Main purpose of this class is to declare method Reflect, which "reflects"/returns
// set of available attributes.

class GFmtResource
{
public:
    virtual ~GFmtResource() {};

public:
    struct TAttrs 
    {
        TAttrs() : Num(0), Attrs(NULL) {}

        UInt    Num; // Number of attributes.
        UPInt*  Attrs; // Array of UPInt values.
    };

public:
    // attrs - pointer to a structure, which containts size on an array of 
    //         attributes and a pointer to the array.
    // Return number of attributes.
    virtual UInt Reflect(const TAttrs*& attrs) const;
};

// ***** GFormatter

// Abstract base class for all formatters.
// Formatter can be used as a standalone object and as a part of GMsgFormat.
// Formatters can be organized into trees.
class GFormatter : public GFmtResource
{
public:
    enum requirement_t 
    {
        rtNone = 0,      // No requirements.
        rtPrevStr = 1,   // Previous string.
        rtPrevStrSS = 2, // Sentence separator. It is used to prevent cyclic dependencies.
        rtNextStr = 4,   // Next string.
        rtParent = 8     // Parent formatter.
    };
    // Set of requirements.
    typedef UInt8 requirements_t;

    // Position of a parent formatter.
    enum ParentRef 
    {
        prNone, // No parent formatter.
        prPrev, // Previous. Relative position.
        prNext, // Next. Relative position.
        prPos   // Absolute position (from the left).
    };

public:
    GFormatter()
    : pParentFmt(NULL)
    , IsConverted(false)
    {
    }
    GFormatter(GMsgFormat& f)
    : pParentFmt(&f)
    , IsConverted(false)
    {
    }
    virtual ~GFormatter();

public:
    // Parse formatting string.
    virtual void Parse(const GStringDataPtr& str);
    // Convert value.
    virtual void Convert() = 0;
    // Retrieve conversion result.
    virtual GStringDataPtr GetResult() const = 0;
    // Retrieve size of a converted value.
    // May be called only after Convert().
    virtual UPInt GetSize() const = 0;
    // Retrieve formatter's requirements. 
    virtual requirements_t GetRequirements() const;
    // Check conversion status for optimization purposes.
    bool Converted() const { return IsConverted; }

public:
    // Dependency-related ...

    // Prev string ...
    virtual void SetPrevStr(const GStringDataPtr& /*ptr*/);
    virtual GStringDataPtr GetPrevStr() const;

    // Next string ...
    virtual void SetNextStr(const GStringDataPtr& /*ptr*/);
    virtual GStringDataPtr GetNextStr() const;

    // Parent formatter ...
    virtual ParentRef GetParentRef() const;
    virtual unsigned char GetParentPos() const;
    virtual void SetParent(const GFmtResource&);

protected:
    bool HasParent() const
    {
        return pParentFmt != NULL;
    }
    // Non-const version is required for formatter substitution.
    GMsgFormat& GetParent()
    {
        GASSERT(pParentFmt);
        return *pParentFmt;
    }
    const GMsgFormat& GetParent() const
    {
        GASSERT(pParentFmt);
        return *pParentFmt;
    }

    // Parent formatter ...
    // Set relative position.
    virtual void SetParentRef(ParentRef);
    // Set absolute position of a parent formatter.
    virtual void SetParentPos(unsigned char);

    // Set IsConverted flag.
    void SetConverted(bool value = true)
    {
        IsConverted = value;
    }

private:
    GFormatter& operator = (const GFormatter&);

private:
    GMsgFormat* pParentFmt;
    bool        IsConverted;
};

////////////////////////////////////////////////////////////////////////////////

// ***** GStrFormatter

// String formatter. Implementation of GFormatter, which serves as a proxy 
// for string data types.

class GStrFormatter : public GFormatter
{
public:
    GStrFormatter(const char* v);
    GStrFormatter(const GStringDataPtr& v);
    GStrFormatter(const GString& v);

    GStrFormatter(GMsgFormat& f, const char* v);
    GStrFormatter(GMsgFormat& f, const GStringDataPtr& v);
    GStrFormatter(GMsgFormat& f, const GString& v);

public:
    virtual void        Parse(const GStringDataPtr& str);
    virtual void        Convert();
    virtual GStringDataPtr GetResult() const;
    virtual UPInt       GetSize() const;

private:
    GStrFormatter& operator = (const GStrFormatter&);

private:
    const GStringDataPtr    Value;
};

template <>
struct GFmtInfo<const char*>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<char*>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const unsigned char*>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<unsigned char*>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const signed char*>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<signed char*>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<const char[N]>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<char[N]>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<const unsigned char[N]>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<unsigned char[N]>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<const signed char[N]>
{
    typedef GStrFormatter formatter;
};

template <int N>
struct GFmtInfo<signed char[N]>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const GStringDataPtr>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<GStringDataPtr>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const GString>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<GString>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const GStringLH>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<GStringLH>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<const GStringDH>
{
    typedef GStrFormatter formatter;
};

template <>
struct GFmtInfo<GStringDH>
{
    typedef GStrFormatter formatter;
};


////////////////////////////////////////////////////////////////////////////////

// ***** GBoolFormatter

// Bool formatter. Implementation of GFormatter, which can transform boolean value
// into a string. By default true is transformed into string "true", and false 
// into "false" accordingly. This default behavior can be changed by using a build-in
// switch formatter. For example:
//
//   G_Format(sink, "{0:sw:yes:no}", true);
//   G_Format(sink, "{0:sw:yes:no}", false);
//
// where "sw" is a argument, which enables "switch-formatter". Next two arguments are
// string values corresponding to true and false.

class GBoolFormatter : public GFormatter
{
public:
    GBoolFormatter(GMsgFormat& f, bool v);

public:
    virtual void        Parse(const GStringDataPtr& str);
    virtual void        Convert();
    virtual GStringDataPtr GetResult() const;
    virtual UPInt       GetSize() const;

private:
    GBoolFormatter& operator = (const GBoolFormatter&);

private:
    bool            Value:1;
    bool            SwitchStmt:1;
    GStringDataPtr  result;
};

template <>
struct GFmtInfo<bool>
{
    typedef GBoolFormatter formatter;
};


////////////////////////////////////////////////////////////////////////////////

// ***** GNumericBase
// Base class for GLongFormatter and GDoubleFormatter.
// It just keeps data common to its children.
class GNumericBase
{
public:
    GNumericBase();

public:
    char*   ToCStr() const { return ValueStr; }

public:
    void    SetPrecision(UInt prec = 1)
    {
        Precision = prec;
    }
    void    SetWidth(UInt width = 1)
    {
        Width = width;
    }
    // In case of hexadecimal and octal numbers it also means "show prefix".
    void    SetShowSign(bool flag = true)
    {
        ShowSign = flag;
    }
    void    SetPrefixChar(char prefix = ' ')
    {
        PrefixChar = prefix;
    }
    void    SetSeparatorChar(char sep = ',')
    {
        SeparatorChar = sep;
    }
    void    SetBigLetters(bool flag = true)
    {
        BigLetters = flag;
    }
    void    SetBlankPrefix(bool flag = true)
    {
        BlankPrefix = flag;
    }
    void    SetAlignLeft(bool flag = true)
    {
        AlignLeft = flag;
    }
    void    SetSharpSign(bool flag = true)
    {
        SharpSign = flag;
    }

protected:
    void    ReadPrintFormat(GStringDataPtr token);
    void    ReadWidth(GStringDataPtr token);

    void    ULongLong2String(char* buff, UInt64 value, bool separator, UInt base = 10);
    void    ULong2String(char* buff, UInt32 value, bool separator, UInt base = 10);

protected:
    UInt    Precision:5;
    UInt    Width:5;
    char    PrefixChar:7;
    char    SeparatorChar:7;
    bool    ShowSign:1;
    bool    BigLetters:1;
    bool    BlankPrefix:1;
    bool    AlignLeft:1; // Required for sprintf().
    bool    SharpSign:1; // Required for sprintf().

    char*   ValueStr;
};

////////////////////////////////////////////////////////////////////////////////

// ***** GLongFormatter

// Long formatter. This class is capable of formatting of integer data types.
// This class can be used either with G_Format or as a standalone formatter.
// It also has a built-in switch formatter. Example:
//
//   G_Format(sink, "{0:sw:0:zero:1:one:2:two:many}", int_value);
//
// Where argument "sw" enables switch formatter. Arguments of the switch formatter
// should follow the format below.
//
// sw:(int_value:str_value)+(:default_str_value)?
//
// For each pair "int_value:str_value" int_value will be replaced with the str_value.
// If a value does not correspond to any of int_value, it will be replaced with 
// a default string value "default_str_value".
class GLongFormatter : public GFormatter, public GNumericBase, public GString::InitStruct
{
public:
    GLongFormatter(int v);
    GLongFormatter(unsigned int v);

    GLongFormatter(long v);
    GLongFormatter(unsigned long v);

#if !defined(GFC_OS_PS2) && !defined(GFC_64BIT_POINTERS) || defined(GFC_OS_WIN32) || (defined(GFC_OS_MAC) && defined(GFC_CPU_X86_64))
    // SInt64 and UInt64 on PS2 are equivalent to long and unsigned long.
    GLongFormatter(SInt64 v);
    GLongFormatter(UInt64 v);
#endif

    GLongFormatter(GMsgFormat& f, int v);
    GLongFormatter(GMsgFormat& f, unsigned int v);

    GLongFormatter(GMsgFormat& f, long v);
    GLongFormatter(GMsgFormat& f, unsigned long v);

#if !defined(GFC_OS_PS2) && !defined(GFC_64BIT_POINTERS) || defined(GFC_OS_WIN32) || (defined(GFC_OS_MAC) && defined(GFC_CPU_X86_64))
    // SInt64 and UInt64 on PS2 are equivalent to long and unsigned long.
    GLongFormatter(GMsgFormat& f, SInt64 v);
    GLongFormatter(GMsgFormat& f, UInt64 v);
#endif

public:
    virtual void        Parse(const GStringDataPtr& str);
    virtual void        Convert();
    virtual GStringDataPtr GetResult() const;
    virtual UPInt       GetSize() const;
    virtual void        InitString(char* pbuffer, UPInt size) const;

public:
    GLongFormatter& SetPrecision(UInt prec = 1)
    {
        GASSERT(prec <= 20);
        GNumericBase::SetPrecision(prec);
        return *this;
    }
    GLongFormatter& SetWidth(UInt width = 1)
    {
        GASSERT(width <= 20);
        GNumericBase::SetWidth(width);
        return *this;
    }
    GLongFormatter& SetShowSign(bool flag = true)
    {
        GNumericBase::SetShowSign(flag);
        return *this;
    }

    // base should be in range [2..16].
    GLongFormatter& SetBase(UInt base)
    {
        GASSERT(base >= 2 && base <= 16);
        Base = base;
        return *this;
    }
    GLongFormatter& SetPrefixChar(char prefix = ' ')
    {
        GNumericBase::SetPrefixChar(prefix);
        return *this;
    }
    GLongFormatter& SetSeparatorChar(char sep = ',')
    {
        GNumericBase::SetSeparatorChar(sep);
        return *this;
    }
    GLongFormatter& SetBigLetters(bool flag = true)
    {
        GNumericBase::SetBigLetters(flag);
        return *this;
    }

private:
    void AppendSignCharLeft(bool negative);
    GLongFormatter& operator = (const GLongFormatter&);

    UPInt GetSizeInternal() const
    {
        return Buff + sizeof(Buff) - 1 - ValueStr;
    }

private:
    UInt            Base:5;
    const bool      SignedValue:1;
    bool            IsLongLong:1;

    const SInt64    Value;
    // Big-enough to keep signed long long with separators plus zero terminator.
    // Should be increased for Unicode separators.
    char            Buff[29]; 
};

template <>
struct GFmtInfo<char>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<unsigned char>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<signed char>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<short>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<unsigned short>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<int>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<unsigned int>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<long>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<unsigned long>
{
    typedef GLongFormatter formatter;
};

#if !defined(GFC_OS_PS2) && !defined(GFC_64BIT_POINTERS) || defined(GFC_OS_WIN32)
template <>
struct GFmtInfo<SInt64>
{
    typedef GLongFormatter formatter;
};

template <>
struct GFmtInfo<UInt64>
{
    typedef GLongFormatter formatter;
};
#endif

////////////////////////////////////////////////////////////////////////////////
//#define INTERNAL_D2A

// ***** GDoubleFormatter

// Double formatter. This class is capable of formatting of numeric data types with 
// floating point.
class GDoubleFormatter : public GFormatter, public GNumericBase, public GString::InitStruct
{
public:
    enum PresentationType { FmtDecimal, FmtScientific, FmtSignificant };
    
public:
    GDoubleFormatter(Double v);
    GDoubleFormatter(GMsgFormat& f, Double v);

public:
    virtual void        Parse(const GStringDataPtr& str);
    virtual void        Convert();
    virtual GStringDataPtr GetResult() const;
    virtual UPInt       GetSize() const;
    virtual void        InitString(char* pbuffer, UPInt size) const;

public:
    GDoubleFormatter& SetType(PresentationType ptype)
    {
        Type = ptype;
        return *this;
    }
    GDoubleFormatter& SetPrecision(UInt prec = 1)
    {
        GNumericBase::SetPrecision(prec);
        return *this;
    }
    GDoubleFormatter& SetWidth(UInt width = 1)
    {
        GNumericBase::SetWidth(width);
        return *this;
    }
    GDoubleFormatter& SetShowSign(bool flag = true)
    {
        GNumericBase::SetShowSign(flag);
        return *this;
    }
    GDoubleFormatter& SetPrefixChar(char prefix = ' ')
    {
        GNumericBase::SetPrefixChar(prefix);
        return *this;
    }
    GDoubleFormatter& SetSeparatorChar(char sep = ',')
    {
        GNumericBase::SetSeparatorChar(sep);
        return *this;
    }
    GDoubleFormatter& SetBigLetters(bool flag = true)
    {
        GNumericBase::SetBigLetters(flag);
        return *this;
    }
    GDoubleFormatter& SetAlignLeft(bool flag = true)
    {
        GNumericBase::SetAlignLeft(flag);
        return *this;
    }

private:
    GDoubleFormatter& operator = (const GDoubleFormatter&);

    void AppendSignCharLeft(bool negative, bool show_sign = false);

#ifdef INTERNAL_D2A
    // In case of underflow it can switch to scientific format.
    void DecimalFormat(Double v);
    void ScientificFormat();
#endif

    UPInt GetSizeInternal() const
    {
        return Buff + sizeof(Buff) - 1 - ValueStr;
    }

private:
    PresentationType    Type;
    const Double        Value;
    UPInt               Len;

    char                Buff[347 + 1]; 
};

#ifndef GFC_NO_DOUBLE

template <>
struct GFmtInfo<float>
{
    typedef GDoubleFormatter formatter;
};

#endif

template <>
struct GFmtInfo<Double>
{
    typedef GDoubleFormatter formatter;
};


////////////////////////////////////////////////////////////////////////////////
class GResouceProvider;

////////////////////////////////////////////////////////////////////////////////

// ***** GResourceFormatter

// Formatter, which is responsible for formatting of localization resources.

class GResourceFormatter : public GFormatter
{
public:
    class ValueType
    {
    public:
        ValueType(UPInt rc);
        ValueType(UPInt rc, const GResouceProvider& provider);
        // SInt version of RC is supposed to be used with NULL argument only.
        // It is interpreted as a string pointer.
        explicit ValueType(SInt rc);
        // SInt version of RC is supposed to be used with NULL argument only.
        // It is interpreted as a string pointer.
        explicit ValueType(SInt rc, const GResouceProvider& provider);
        ValueType(const char* rc);
        ValueType(const char* rc, const GResouceProvider& provider);

    public:
        bool IsStrResource() const
        {
            return IsString;
        }
        const char* GetStrResource() const
        {
            GASSERT(IsStrResource());
            return Resource.RStr;
        }
        UPInt GetLongResource() const
        {
            GASSERT(!IsStrResource());
            return Resource.RLong;
        }

    public:
        const GResouceProvider* GetResouceProvider() const
        {
            return RC_Provider;
        }
        void SetResouceProvider(const GResouceProvider* rp)
        {
            RC_Provider = rp;
        }

    private:
        ValueType& operator = (const ValueType&);

    private:
        union ResourceType
        {
            const char*         RStr;
            UPInt               RLong;
        };

        ResourceType            Resource;
        const bool              IsString;
        const GResouceProvider* RC_Provider;
    };

public:
    GResourceFormatter(GMsgFormat& f, const ValueType& v);
    virtual ~GResourceFormatter();

public:
    virtual void            Parse(const GStringDataPtr& str);
    virtual void            Convert();
    virtual GStringDataPtr  GetResult() const;
    virtual UPInt           GetSize() const;

private:
    GStringDataPtr MakeString(const TAttrs& attrs) const;

private:
    GResourceFormatter& operator = (const GResourceFormatter&);

private:
    const ValueType         Value;
    const GResouceProvider* pRP;

    GStringDataPtr      Result;
};

template <>
struct GFmtInfo<GResourceFormatter::ValueType>
{
    typedef GResourceFormatter formatter;
};

////////////////////////////////////////////////////////////////////////////////

// ***** GResouceProvider

// Localization resource provider.

class GResouceProvider : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    GResouceProvider(const GResouceProvider* next = NULL) : nextRP(next) {}
    virtual ~GResouceProvider() {}

public:
    virtual UInt Reflect(const GResourceFormatter::ValueType& /*rc*/, const GFmtResource::TAttrs** attrs) const = 0;
    virtual GStringDataPtr MakeString(const GResourceFormatter::ValueType& /*rc*/, const GFmtResource::TAttrs& /*attrs*/) const = 0;

protected:
    const GResouceProvider* GetNext() const { return nextRP; }

private:
    const GResouceProvider* nextRP;
};


////////////////////////////////////////////////////////////////////////////////

// ***** GFormatterFactory

// Abstract base class for all formatter factories.
class GFormatterFactory
{
public:
    virtual ~GFormatterFactory() {}

public:
    struct Args
    {
        Args(GMsgFormat&f, const GStringDataPtr& n, const GResourceFormatter::ValueType& v) 
            : Fmt(f), Name(n), Value(v)
        {
        }

        GMsgFormat&                             Fmt;
        const GStringDataPtr&                   Name;
        const GResourceFormatter::ValueType&    Value;

    private:
        Args& operator = (const Args&);
    };

public:
    virtual GFormatter* MakeFormatter(const Args& args) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

// ***** GLocaleProvider

// Formatter factory, which is also aware of locale. This class is supposed to
// provide all information related to localization.

class GLocaleProvider : public GFormatterFactory
{
public:
    virtual ~GLocaleProvider() {}

public:
    virtual const GLocale&      GetLocale() const = 0;
    virtual GResouceProvider*   GetDefaultRCProvider() const = 0;
};


////////////////////////////////////////////////////////////////////////////////

// ***** GMsgFormat

// Class, which parses formatting string, applies formatter's values, and
// puts final formatted string into a Sink.
//
// Sink can be of three data types:
//   1) GString. Resulting string will be copied into GString.
//   2) GStringBuffer. Resulting string will be added to GStringBuffer.
//   3) GStringDataPtr. This data type represents a buffer. Resulting string will 
//        be copied into the buffer.
class GMsgFormat : GString::InitStruct
{
public:
    class Sink
    {
        friend class GMsgFormat;

    public:
        Sink(GString& str) 
            : Type(tStr), SinkData(str) {}
        Sink(GStringBuffer& buffer) 
            : Type(tStrBuffer), SinkData(buffer) {}
        Sink(const GStringDataPtr& strData) 
            : Type(tDataPtr), SinkData(strData) {}
        template <typename T, int N> 
        Sink(const T (&v)[N])
            : Type(tDataPtr), SinkData(GStringDataPtr(v, N)) {}

    private:
        enum DataType {tStr, tStrBuffer, tDataPtr};

        struct StrDataType
        {
            const char*     pStr;
            UPInt           Size;
        };

        union SinkDataType
        {
        public:
            SinkDataType(GString& str)
                : pStr(&str)
            {
            }
            SinkDataType(GStringBuffer& buffer)
                : pStrBuffer(&buffer)
            {
            }
            SinkDataType(const GStringDataPtr& strData)
            {
                DataPtr.pStr = strData.ToCStr();
                DataPtr.Size = strData.GetSize();
            }

        public:
            GString*        pStr;
            GStringBuffer*  pStrBuffer;
            StrDataType     DataPtr;
        };

        DataType            Type;
        SinkDataType        SinkData;
    };

public:
    GMsgFormat(const Sink& r);
    GMsgFormat(const Sink& r, const GLocaleProvider& loc);
    ~GMsgFormat();

public:
    typedef UInt8 ArgNumType;
    typedef GStackMemPool<> MemoryPoolType;

    // Parse formatting string in "Scaleform" format. 
    // Split a string onto tokens, which can be of two types:
    // text and parameter. For parameters retrieve an argument number.
    void Parse(const char* fmt);

    // Parse formatting string in "sprintf" format.
    // This function creates all necessary formatters.
    void FormatF(const GStringDataPtr& fmt, va_list argList);

    // Replace an old formatter with the new one.
    // Argument "allocated" should be true if new formatter is dynamically allocated.
    // Return value: true on success.
    bool ReplaceFormatter(GFormatter* oldf, GFormatter* newf, bool allocated);

    ArgNumType GetFirstArgNum() const
    {
        return FirstArgNum;
    }
    void SetFirstArgNum(ArgNumType num)
    {
        FirstArgNum = num;
    }

    // Get current locale provider if any.
    const GLocaleProvider* GetLocaleProvider() const
    {
        return pLocaleProvider;
    }

    // Get memory pool, which can be used by formatters.
    MemoryPoolType& GetMemoryPool()
    {
        return MemPool;
    }

    // Get formatted string size.
    UPInt GetStrSize() const
    {
        return StrSize;
    }

public:
    UInt8 GetEscapeChar() const
    {
        return EscapeChar;
    }
    void SetEscapeChar(UInt8 c)
    {
        EscapeChar = c;
    }

public:
    // "D" stands for "Dynamic".

    // Create a binding for one argument.
    // This function will allocate a formatter only if this argument is mentioned in a
    // formatting string.
    template <typename T>
    void FormatD1(const T& v)
    {
        while (NextFormatter())
        {
            Bind(new (GetMemoryPool()) typename GFmtInfo<T>::formatter(*this, v), true);
        }

        ++FirstArgNum;
    }

    void FinishFormatD();

private:
    enum ERecType {eStrType, eParamStrType, eFmtType};

    struct str_ptr
    {
        const char*     Str;
        unsigned char   Len;
        unsigned char   ArgNum;
    };

    struct fmt_ptr
    {
        GFormatter*     Formatter;
        bool            Allocated;
    };

    union fmt_value
    {
        str_ptr         String;
        fmt_ptr         Formatter;
    };

    class fmt_record
    {
    public:
        fmt_record(ERecType type, const fmt_value& value)
        : RecType(type)
        , RecValue(value)
        {
        }

        ERecType GetType() const
        {
            return RecType;
        }
        const fmt_value& GetValue() const
        {
            return RecValue;
        }

    private:
        ERecType    RecType;
        fmt_value   RecValue;
    };

private:
    // Function with side effects.
    // Calculate next DataInd, which is supposed to be used by Bind().
    bool NextFormatter();
    // Bind a formatter at DataInd,
    void Bind(GFormatter* formatter, const bool allocated);
    void BindNonPos();
    void MakeString();
    void Evaluate(UPInt ind);

    void AddStringRecord(const GStringDataPtr& str);
    void AddFormatterRecord(GFormatter* f, bool allocated);

    // InitStruct-related.
    virtual void InitString(char* pbuffer, UPInt size) const;

private:
    GMsgFormat& operator = (const GMsgFormat&);

private:
    // Minimalistic implementation of Array, which allocates first SS elements
    // on stack.
    template <typename T, UPInt SS = 8, class DA = GArray<T> >
    class GStackArray
    {
    public:
        typedef T       ValueType;
    
    public:
        GStackArray()
            : Size(0)
        {
        }
        
        UPInt GetSize() const { return Size; }

        void PushBack(const ValueType& val)
        {
            if (Size < SS)
                * reinterpret_cast<ValueType*>(StaticArray + Size * sizeof(T)) = val;
            else
                DynamicArray.PushBack(val);

            ++Size;
        }

        ValueType& operator [] (UPInt index)
        {
            if (index < SS)
                return * reinterpret_cast<ValueType*>(StaticArray + index * sizeof(T));

            return DynamicArray[index - SS];
        }
        const ValueType& operator [] (UPInt index) const
        {
            if (index < SS)
                return * reinterpret_cast<const ValueType*>(StaticArray + index * sizeof(T));

            return DynamicArray[index - SS];
        }

    private:
        UPInt   Size;
        DA      DynamicArray;
        char    StaticArray[SS * sizeof(T)];
    };

private:
    // GStackArray, which allocates first 16 elements on stack.
    typedef GStackArray<fmt_record, 16, GArrayPOD<fmt_record> > DataType;

    UInt8                   EscapeChar;
    UInt8                   FirstArgNum;
    UInt16                  NonPosParamNum;
    UInt16                  UnboundFmtrInd;
    UPInt                   StrSize;
    SPInt                   DataInd;
    const GLocaleProvider*  pLocaleProvider;
    const Sink              Result;

    DataType                Data;
    MemoryPoolType          MemPool;
};


////////////////////////////////////////////////////////////////////////////////
// Formatter type traits.

template <typename T>
struct GFmtInfo
{
};

////////////////////////////////////////////////////////////////////////////////

// ***** G_Format

// G_Format is supposed to replace the sprintf function. 
// It doesn't not to declare argument's data types in a formatted string.

// 0
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

////////////////////////////////////////////////////////////////////////////////
// G_Format is developed as a list of FormatD1 calls in order to produce smaller
// code with CodeWarrior for Wii.

// 1 ...
template <typename T1>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 2 ...
template <typename T1, typename T2>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 3 ...
template <typename T1, typename T2, typename T3>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 4 ...
template <typename T1, typename T2, typename T3, typename T4>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 5 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 6 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 7 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 8 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 9 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8,
    const T9& v9
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FormatD1(v9);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8,
    const T9& v9
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FormatD1(v9);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

// 10 ...
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8,
    const T9& v9,
    const T10& v10
    )
{
    GMsgFormat parsed_format(result);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FormatD1(v9);
    parsed_format.FormatD1(v10);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline
UPInt G_Format(
    const GMsgFormat::Sink& result, 
    const GLocaleProvider& loc,
    const char* fmt, 
    const T1& v1, 
    const T2& v2,
    const T3& v3,
    const T4& v4,
    const T5& v5,
    const T6& v6,
    const T7& v7,
    const T8& v8,
    const T9& v9,
    const T10& v10
    )
{
    GMsgFormat parsed_format(result, loc);

    parsed_format.Parse(fmt);
    parsed_format.FormatD1(v1);
    parsed_format.FormatD1(v2);
    parsed_format.FormatD1(v3);
    parsed_format.FormatD1(v4);
    parsed_format.FormatD1(v5);
    parsed_format.FormatD1(v6);
    parsed_format.FormatD1(v7);
    parsed_format.FormatD1(v8);
    parsed_format.FormatD1(v9);
    parsed_format.FormatD1(v10);
    parsed_format.FinishFormatD();
    return parsed_format.GetStrSize();
}

////////////////////////////////////////////////////////////////////////////////

// ***** G_SPrintF

// This function behaves identically to sprintf.
// Return value: size of resulting string.
UPInt G_SPrintF(const GMsgFormat::Sink& result, const char* fmt, ...);

////////////////////////////////////////////////////////////////////////////////

// ***** G_ReadInteger

// Utility function, which tries to read an integer.
// It returns read integer value on success, or defaultValue otherwise.
// It will trim string if an integer was successfully read.
SInt G_ReadInteger(GStringDataPtr& str, SInt defaultValue, char separator = ':');

#endif // INC_FORMAT_STR_H
