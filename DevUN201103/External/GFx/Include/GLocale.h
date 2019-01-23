/**********************************************************************

Filename    :   GLocale.h 
Content     :   Language-specific formatting information (locale)
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

#ifndef INC_GLOCALE_H
#define INC_GLOCALE_H

#include "GString.h"


class GLocale
{
public:
    GLocale();

public:
    const GString& GetEnglishName() const
    {
        return EnglishName;
    }
    const GString& GetNativeName() const
    {
        return NativeName;
    }
    const GString& GetGrouping() const
    {
        return Grouping;
    }

    UInt32 GetGroupSeparator() const
    {
        return GroupSeparator;
    }
    UInt32 GetDecimalSeparator() const
    {
        return DecimalSeparator;
    }
    UInt32 GetPositiveSign() const
    {
        return PositiveSign;
    }
    UInt32 GetNegativeSign() const
    {
        return NegativeSign;
    }
    UInt32 GetExponentialSign() const
    {
        return ExponentialSign;
    }

protected:
    void SetEnglishName(const GString& name)
    {
        EnglishName = name;
    }
    void SetNativeName(const GString& name)
    {
        NativeName = name;
    }
    void SetGrouping(const GString& value)
    {
        Grouping = value;
    }

    void SetGroupSeparator(UInt32 value)
    {
        GroupSeparator = value;
    }
    void SetDecimalSeparator(UInt32 value)
    {
        DecimalSeparator = value;
    }
    void SetPositiveSign(UInt32 value)
    {
        PositiveSign = value;
    }
    void SetNegativeSign(UInt32 value)
    {
        NegativeSign = value;
    }
    void SetExponentialSign(UInt32 value)
    {
        ExponentialSign = value;
    }

private:
    GString     EnglishName;
    GString     NativeName;

    GString     Grouping;
    UInt32      GroupSeparator;
    UInt32      DecimalSeparator;
    UInt32      PositiveSign;
    UInt32      NegativeSign;
    UInt32      ExponentialSign;
};

////////////////////////////////////////////////////////////////////////////////
class GSystemLocale : public GLocale
{
public:
    GSystemLocale();
};


#endif // INC_GLOCALE_H
