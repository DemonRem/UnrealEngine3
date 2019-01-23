/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #pragma once

#include "SymbolParser.h"
#include <vector>

using namespace System;
using namespace System::Collections::Generic;

namespace StandaloneSymbolParser
{
	// forward declarations
	ref class ModuleSymbolSession;

	/**
	 * Class for loading xbox 360 symbols.
	 */
	public ref class XenonSymbolParser : public SymbolParser
	{
	private:
		static HMODULE DbgLibraryHandle;

		List<ModuleSymbolSession^> ^SymbolSessions;

		/**
		 * Loads global debug information for xbox 360's.
		 */
		static bool LoadDbgHelp();

	public:
		/**
		 * Constructor.
		 */
		XenonSymbolParser();

		/**
		 * Dispse()
		 */
		virtual ~XenonSymbolParser();

		/**
		 * Loads symbols for an executable.
		 *
		 * @param	ExePath		The path to the executable whose symbols are going to be loaded.
		 * @param	Modules		The list of modules loaded by the process.
		 */
		virtual bool LoadSymbols(String ^ExePath, array<ModuleInfo^> ^Modules) override;

		/**
		 * Unloads any currently loaded symbols.
		 */
		virtual void UnloadSymbols() override;

		/**
		 * Retrieves the symbol info for an address.
		 *
		 * @param	Address			The address to retrieve the symbol information for.
		 * @param	OutFileName		The file that the instruction at Address belongs to.
		 * @param	OutFunction		The name of the function that owns the instruction pointed to by Address.
		 * @param	OutLineNumber	The line number within OutFileName that is represented by Address.
		 *
		 * @return	True if the function succeeds.
		 */
		virtual bool ResolveAddressToSymboInfo(unsigned __int64 Address, String ^%OutFileName, String ^%OutFunction, int %OutLineNumber) override;
	};
}
