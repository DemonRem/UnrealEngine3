/*=============================================================================
	WindowsSupport/ByteStreamConverter.h: Conversion from / to data stream for basic types.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#pragma once

/**
 *	Simple byte stream converter for basic types: int, string, etc.
 */
class CByteStreamConverter
{
public:
	/// Retrieves int from buffer assuming big endian is used
	static unsigned int LoadIntBE(char*& Data)
	{
		const unsigned int Value =
			((unsigned char) Data[0]) << 24 |
			((unsigned char) Data[1]) << 16 |
			((unsigned char) Data[2]) << 8 |
			((unsigned char) Data[3]);
		Data += 4;
		return Value;
	}

	/// Retrieves int from buffer assuming little endian is used
	static unsigned int LoadIntLE(char*& Data)
	{
		const unsigned int Value =
			((unsigned char) Data[3]) << 24 |
			((unsigned char) Data[2]) << 16 |
			((unsigned char) Data[1]) << 8 |
			((unsigned char) Data[0]);
		Data += 4;
		return Value;
	}

	/// Retrieves string from buffer assuming big endian is used
	static string LoadStringBE(char*& Data)
	{
		const int StringLen = LoadIntBE(Data);

		char* Buffer = new char[StringLen + 1];
		memcpy(Buffer, Data, StringLen);
		Buffer[StringLen] = '\0';

		Data += StringLen;

		string Result = Buffer;
		delete[] Buffer;
		return Result;
	}

	/// Retrieves string from buffer assuming big endian is used
	static string LoadStringLE(char*& Data)
	{
		const int StringLen = LoadIntLE(Data);

		char* Buffer = new char[StringLen + 1];
		memcpy(Buffer, Data, StringLen);
		Buffer[StringLen] = '\0';

		Data += StringLen;

		string Result = Buffer;
		delete[] Buffer;
		return Result;
	}

	static void StoreIntBE(char* Data, unsigned int Value)
	{
		Data[0] = (char) (Value >> 24) & 255;
		Data[1] = (char) (Value >> 16) & 255;
		Data[2] = (char) (Value >> 8) & 255;
		Data[3] = (char) (Value & 255);
	}
};
