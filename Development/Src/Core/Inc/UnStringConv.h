/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This file contains the classes used when converting strings between
 * standards (ANSI, UNICODE, etc.)
 */

#ifndef _UN_STRING_CONV_H
#define _UN_STRING_CONV_H

/** Class that handles the TCHAR to ANSI conversion */
class FTCHARToANSI_Convert
{
	/**
	 * The code page to use during conversion
	 */
	DWORD CodePage;

public:
	/**
	 * Defaults the code page to CP_ACP
	 */
	FORCEINLINE FTCHARToANSI_Convert(DWORD InCodePage = CP_ACP) :
		CodePage(InCodePage)
	{
	}

	/**
	 * Converts the string to the desired format. Allocates memory if the
	 * specified destination buffer isn't large enough
	 *
	 * @param Source The source string to convert
	 * @param Dest the destination buffer that holds the converted data
	 * @param Size the size of the dest buffer in bytes
	 */
	FORCEINLINE ANSICHAR* Convert(const TCHAR* Source,ANSICHAR* Dest,DWORD Size)
	{
		// Determine whether we need to allocate memory or not
#if _MSC_VER
		DWORD LengthW = (DWORD)lstrlenW(Source) + 1;
#else
		DWORD LengthW = (DWORD)wcslen(Source) + 1;
#endif
		// Needs to be 2x the wide in case each converted char is multibyte
		DWORD LengthA = LengthW * 2;
		if (LengthA > Size)
		{
			// Need to allocate memory because the string is too big
			Dest = new char[LengthA * sizeof(ANSICHAR)];
		}
		// Now do the conversion
#if _MSC_VER
		WideCharToMultiByte(CodePage,0,Source,LengthW,Dest,LengthA,NULL,NULL);
#else
		for (INT C = 0; C < LengthW; C++)
		{
			Dest[C] = Source[C] & 0xFF;
		}
#endif
		return Dest;
	}
};

/** Class that converts TCHAR to OEM */
class FTCHARToOEM_Convert :
	public FTCHARToANSI_Convert
{
public:
	/**
	 * Sets the code page to OEM
	 */
	FORCEINLINE FTCHARToOEM_Convert() :
		FTCHARToANSI_Convert(CP_OEMCP)
	{
	}
};

/** Class that handles the ANSI to TCHAR conversion */
class FANSIToTCHAR_Convert
{
public:
	/**
	 * Converts the string to the desired format. Allocates memory if the
	 * specified destination buffer isn't large enough
	 *
	 * @param Source The source string to convert
	 * @param Dest the destination buffer that holds the converted data
	 * @param Size the size of the dest buffer in bytes
	 */
	FORCEINLINE TCHAR* Convert(const ANSICHAR* Source,TCHAR* Dest,DWORD Size)
	{
		// Determine whether we need to allocate memory or not
#if _MSC_VER
		DWORD Length = (DWORD)lstrlenA(Source) + 1;
#else
		DWORD Length = (DWORD)strlen(Source) + 1;
#endif
		if (Length > Size)
		{
			// Need to allocate memory because the string is too big
			Dest = new TCHAR[Length * sizeof(TCHAR)];
		}
		// Now do the conversion
#if _MSC_VER
		MultiByteToWideChar(CP_ACP,0,Source,Length,Dest,Length);
#else
		for (INT C = 0; C < Length; C++)
		{
			Dest[C] = Source[C];
		}
#endif

		return Dest;
	}
};

/**
 * Class takes one type of string and converts it to another. The class includes a
 * chunk of presized memory of the destination type. If the presized array is
 * too small, it mallocs the memory needed and frees on the class going out of
 * scope. Uses the overloaded cast operator to return the converted data.
 */
template<typename CONVERT_TO,typename CONVERT_FROM,typename BASE_CONVERTER,
	DWORD DefaultConversionSize = 128>
class TStringConversion :
	public BASE_CONVERTER
{
	/**
	 * Holds the converted data if the size is large enough
	 */
	CONVERT_TO Buffer[DefaultConversionSize];
	/**
	 * Points to the converted data. If this pointer doesn't match Buffer, then
	 * memory was allocated and needs to be freed.
	 */
	CONVERT_TO* ConvertedString;

	/** Hide the default ctor */
	TStringConversion();

public:
	/**
	 * Converts the data by using the Convert() method on the base class
	 */
	explicit inline TStringConversion(const CONVERT_FROM* Source)
	{
		if (Source != NULL)
		{
			// Use base class' convert method
			ConvertedString = BASE_CONVERTER::Convert(Source,Buffer,DefaultConversionSize);
		}
		else
		{
			ConvertedString = NULL;
		}
	}

	/**
	 * If memory was allocated, then it is freed below
	 */
	inline ~TStringConversion()
	{
		if (ConvertedString != Buffer && ConvertedString != NULL)
		{
			delete [] ConvertedString;
		}
	}

	/** Operator to get access to the converted string */
	inline operator CONVERT_TO*(void) const
	{
		return ConvertedString;
	}
};

// Conversion typedefs
typedef TStringConversion<TCHAR,ANSICHAR,FANSIToTCHAR_Convert> FANSIToTCHAR;
typedef TStringConversion<ANSICHAR,TCHAR,FTCHARToANSI_Convert> FTCHARToANSI;
typedef TStringConversion<ANSICHAR,TCHAR,FTCHARToOEM_Convert> FTCHARToOEM;

#endif
