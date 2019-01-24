/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #pragma once

using namespace System;

namespace StandaloneSymbolParser
{
	// forward declarations
	ref class ModuleInfo;

	/**
	 * Represents a DIA symbol session for a module.
	 */
	public ref class ModuleSymbolSession
	{
	private:
		IDiaSession *SessionPtr;
		ModuleInfo ^ModInfo;

	protected:
		/**
		 * Finalizer.
		 */
		!ModuleSymbolSession();

	public:
		/**
		 * Gets information about the module associated with the session.
		 */
		property ModuleInfo^ ModuleInformation
		{
			ModuleInfo^ get();
		}

	public:
		/**
		 * Constructor.
		 *
		 * @param	Session		The pointer to the DIA session object.
		 * @param	Info		Information about the module the session represents.
		 */
		ModuleSymbolSession(IDiaSession *Session, ModuleInfo ^Info);

		/**
		 * Dispose().
		 */
		~ModuleSymbolSession();

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
		bool GetSymbolInformation(unsigned __int64 Address, String ^%OutFileName, String ^%OutFunction, int %OutLineNumber);
	};
}
