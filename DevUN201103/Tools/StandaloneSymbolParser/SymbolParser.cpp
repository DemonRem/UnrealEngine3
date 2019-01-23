/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #include "Stdafx.h"
#include "SymbolParser.h"

namespace StandaloneSymbolParser
{
	/**
	 * Gets the symbol server location.
	 */
	String^ SymbolParser::SymbolServer::get()
	{
		return DefaultSymbolServer;
	}

	/**
	* Gets the symbol server location.
	*/
	void SymbolParser::SymbolServer::set(String ^value)
	{
		if(value == nullptr)
		{
			throw gcnew ArgumentNullException(L"value");
		}

		DefaultSymbolServer = value;
	}

	/**
	 * Constructor.
	 */
	SymbolParser::SymbolParser() : DefaultSymbolServer(L"R:\\BuilderSymbols")
	{

	}

	/**
	 * Converted to virtual void Dispose().
	 */
	SymbolParser::~SymbolParser()
	{

	}

	/**
	 * Creates a formatted line of text for a callstack.
	 *
	 * @param	FileName		The file that the function call was made in.
	 * @param	Function		The name of the function that was called.
	 * @param	LineNumber		The line number the function call occured on.
	 *
	 * @return	A string formatted using "{0}() [{1}:{2}]\r\n" as the template.
	 */
	String^ SymbolParser::FormatCallstackLine(String ^FileName, String ^Function, int LineNumber)
	{
		return String::Format(L"{0}() [{1}:{2}]\r\n", Function, FileName->Length > 0 ? FileName : L"???", LineNumber.ToString());
	}
}
