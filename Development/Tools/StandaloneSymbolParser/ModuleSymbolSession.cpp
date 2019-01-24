/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #include "Stdafx.h"
#include "ModuleSymbolSession.h"
#include "ModuleInfo.h"

namespace StandaloneSymbolParser
{
	/**
	 * Gets information about the module associated with the session.
	 */
	ModuleInfo^ ModuleSymbolSession::ModuleInformation::get()
	{
		return ModInfo;
	}

	/**
	 * Constructor.
	 *
	 * @param	Session		The pointer to the DIA session object.
	 * @param	Info		Information about the module the session represents.
	 */
	ModuleSymbolSession::ModuleSymbolSession(IDiaSession *Session, ModuleInfo ^Info) : SessionPtr(Session), ModInfo(Info)
	{
		if(Session == NULL)
		{
			throw gcnew ArgumentNullException(L"Session");
		}

		if(Info == nullptr)
		{
			throw gcnew ArgumentNullException(L"Info");
		}

		Session->AddRef();
	}

	/**
	 * Dispose().
	 */
	ModuleSymbolSession::~ModuleSymbolSession()
	{
		this->!ModuleSymbolSession();
		GC::SuppressFinalize(this);
	}

	/**
	 * Finalizer.
	 */
	ModuleSymbolSession::!ModuleSymbolSession()
	{
		if(SessionPtr)
		{
			SessionPtr->Release();
			SessionPtr = NULL;
		}
	}

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
	bool ModuleSymbolSession::GetSymbolInformation(unsigned __int64 Address, String ^%OutFileName, String ^%OutFunction, int %OutLineNumber)
	{
		OutFileName = String::Empty;
		OutFunction = String::Empty;
		OutLineNumber = 0;

		if(!SessionPtr)
		{
			throw gcnew ObjectDisposedException(L"SessionPtr");
		}

		CComBSTR FuncName;
		CComBSTR FileNameBSTR;
		CComPtr<IDiaSymbol> Symbol;
		pin_ptr<int> InteropLineNumber = &OutLineNumber;
		bool bSucceeded = false;

		if(SUCCEEDED(SessionPtr->findSymbolByVA(Address, SymTagFunction, &Symbol)) && Symbol)
		{
			Symbol->get_name(&FuncName);
			OutFunction = System::Runtime::InteropServices::Marshal::PtrToStringBSTR(IntPtr(FuncName.m_str));

			CComPtr<IDiaEnumLineNumbers> LineEnumerator;

			if(SUCCEEDED(SessionPtr->findLinesByVA(Address, 4, &LineEnumerator)))
			{
				// We could loop over all of the source lines that map to this instruction,
				// but there is probably at most one, and if there are multiple source
				// lines we still only want one.
				CComPtr<IDiaLineNumber> Line;
				DWORD celt;

				if(SUCCEEDED(LineEnumerator->Next(1, &Line, &celt)) && celt == 1)
				{
					Line->get_lineNumber((DWORD*)InteropLineNumber);

					CComPtr<IDiaSourceFile> SrcFile;

					if(SUCCEEDED(Line->get_sourceFile(&SrcFile)))
					{
						if(SUCCEEDED(SrcFile->get_fileName(&FileNameBSTR)))
						{
							bSucceeded = true;
							OutFileName = System::Runtime::InteropServices::Marshal::PtrToStringBSTR(IntPtr(FileNameBSTR.m_str));
						}
					}
				}
			}
		}

		return bSucceeded;
	}
}
