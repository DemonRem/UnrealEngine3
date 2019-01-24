/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #include "Stdafx.h"
#include "XenonSymbolParser.h"
#include "ModuleInfo.h"
#include "ModuleSymbolSession.h"
#include <string>

using namespace System::Text;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Windows::Forms;

namespace StandaloneSymbolParser
{
	/**
	 * Constructor.
	 */
	XenonSymbolParser::XenonSymbolParser() : SymbolSessions(gcnew List<ModuleSymbolSession^>())
	{

	}

	XenonSymbolParser::~XenonSymbolParser()
	{
		if(SymbolSessions->Count > 0)
		{
			UnloadSymbols();
		}
	}

	/**
	 * Loads global debug information for xbox 360's.
	 */
	bool XenonSymbolParser::LoadDbgHelp()
	{
		if(!DbgLibraryHandle)
		{
			// Get the XEDK environment variable.
			String ^XedkDir = Environment::GetEnvironmentVariable(L"xedk");
			String ^PathEnv = Environment::GetEnvironmentVariable(L"path");
			
			if(XedkDir == nullptr)
			{
				OutputDebugStringW(L"Couldn't read xedk environment variable.\n");
				return false;
			}


			if (IntPtr::Size == 8)
			{
				XedkDir = Path::Combine(XedkDir, L"bin\\x64");
			}
			else
			{
				XedkDir = Path::Combine(XedkDir, L"bin\\win32");
			}


			if(PathEnv->IndexOf(XedkDir, StringComparison::OrdinalIgnoreCase) == -1)
			{
				PathEnv = String::Format(L"{0};{1}", PathEnv, XedkDir);
				Environment::SetEnvironmentVariable(L"path", PathEnv, EnvironmentVariableTarget::Process);
			}

			// Create a fully specified path to the XEDK version of dbghelp.dll
			// This is necessary because symsrv.dll must be in the same directory
			// as dbghelp.dll, and only a fully specified path can guarantee which
			// version of dbghelp.dll we load.
			String ^DbgHelpPath = Path::Combine(XedkDir, L"dbghelp.dll");
			// Call LoadLibrary on DbgHelp.DLL with our fully specified path.
			pin_ptr<const wchar_t> StrPtr = PtrToStringChars(DbgHelpPath);
			DbgLibraryHandle = LoadLibrary(StrPtr);

			// Load $(CommonProgramFiles)\microsoft shared\VC\msdia90.dll (installed by the UE3Redist).
			// Note that CommonProgramFiles is a special environment variable. It will return different path
			// depending on if the current process is 32-bit running on 64-bit OS (WOW64) or not, so we'll
			// always get the right version of msdia90.dll.
			String ^VisualStudio2008Dir = Environment::GetEnvironmentVariable(L"CommonProgramFiles");
			String ^DiaPath = Path::Combine(VisualStudio2008Dir, L"microsoft shared\\VC\\msdia90.dll");
			StrPtr = PtrToStringChars(DiaPath);
			HMODULE DiaHandle = LoadLibrary(StrPtr);

			// Print an error message and return FALSE if DbgHelp.DLL or msdia90.dll didn't load.
			if(!DbgLibraryHandle)
			{
				OutputDebugStringW(L"ERROR: Couldn't load DbgHelp.dll from %xedk%\\bin\\win32.\n");
				return false;
			}
			if(!DiaHandle)
			{
				OutputDebugStringW(L"ERROR: Couldn't load msdia90.dll from %CommonProgramFiles%\\microsoft shared\\VC.\n");
				return false;
			}
		}

		return true;
	}

	/**
	 * Loads symbols for an executable.
	 *
	 * @param	ExePath		The path to the executable whose symbols are going to be loaded.
	 * @param	Modules		The list of modules loaded by the process.
	 */
	bool XenonSymbolParser::LoadSymbols(String ^ExePath, array<ModuleInfo^> ^Modules)
	{
		if(ExePath == nullptr)
		{
			throw gcnew ArgumentNullException(L"ExePath");
		}

		if(ExePath->Length == 0)
		{
			throw gcnew ArgumentException(L"ExePath has 0 length");
		}

		if(Modules == nullptr)
		{
			throw gcnew ArgumentNullException(L"Modules");
		}

		const int BUFSIZE = 2048;
		char ResultPath[MAX_PATH];
		char ASCIIFileName[BUFSIZE];
		char *ANSIPtr = NULL;

		if(LoadDbgHelp())
		{
			String ^XDKSymSrv = Path::Combine(Environment::GetEnvironmentVariable(L"xedk"), L"bin\\xbox\\symsrv");
			String ^CurrentDir = Directory::GetCurrentDirectory();
			String ^RootDir = Path::GetFullPath(Path::Combine(CurrentDir, L".."));
			String ^LocalSymbolSearchPath = String::Format(L"{0};{1};{2}", Path::GetDirectoryName(ExePath), XDKSymSrv, RootDir);

			if(DefaultSymbolServer->Length > 0)
			{
				LocalSymbolSearchPath = String::Format(L"{0};{1}", DefaultSymbolServer, LocalSymbolSearchPath);
			}

			LocalSymbolSearchPath = String::Format(L"SRV*{0}", LocalSymbolSearchPath );

			ANSIPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(LocalSymbolSearchPath).ToPointer();
			strcpy_s(ASCIIFileName, BUFSIZE, ANSIPtr);
			System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(ANSIPtr));

			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_EXACT_SYMBOLS | SYMOPT_CASE_INSENSITIVE);

			if(!SymInitialize((HANDLE)GetHashCode(), ASCIIFileName, FALSE))
			{
				UnloadSymbols();
				return false;
			}

			String ^ExeName = Path::GetFileNameWithoutExtension(ExePath);

			for each(ModuleInfo ^CurModule in Modules)
			{
				CComPtr<IDiaDataSource> DiaSource;
				if(FAILED(NoRegCoCreate(L"msdia90.dll", CLSID_DiaSource, IID_IDiaDataSource, (void**)&DiaSource)))
				{
					OutputDebugStringW(L"(StandaloneSymbolParser.dll) XenonSymbolParser::LoadSymbols(): Failed to create IDiaDataSource!\n");
					continue;
				}

				pin_ptr<Guid> PdbSignature = &CurModule->PdbSig70;
				ANSIPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(CurModule->ImageName).ToPointer(); // full path
				strcpy_s(ASCIIFileName, BUFSIZE, ANSIPtr);
				System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(ANSIPtr));

				if(!SymFindFileInPath((HANDLE)GetHashCode(), NULL, ASCIIFileName, PdbSignature, CurModule->PdbAge, 0, SSRVOPT_GUIDPTR, ResultPath, NULL, NULL))
				{
					OutputDebugStringW(L"(StandaloneSymbolParser.dll) XenonSymbolParser::LoadSymbols(): Could not find symbols in search path!\n");
					ZeroMemory(ResultPath, MAX_PATH);
				}

				CComBSTR BstrResultPath(ResultPath);
				CComBSTR BstrOriginalPath(ASCIIFileName);

				HRESULT ResultA = DiaSource->loadAndValidateDataFromPdb(BstrResultPath, (GUID*)PdbSignature, 0, CurModule->PdbAge);
				HRESULT ResultB = DiaSource->loadAndValidateDataFromPdb(BstrOriginalPath, (GUID*)PdbSignature, 0, CurModule->PdbAge);
				if(FAILED(ResultA) && FAILED(ResultB))
				{
					OutputDebugStringW(L"(StandaloneSymbolParser.dll) XenonSymbolParser::LoadSymbols(): Failed to load and validate pdb!\n");

					System::Media::SystemSounds::Exclamation->Play();
					if(ExeName->Equals(Path::GetFileNameWithoutExtension(CurModule->ModuleName), StringComparison::OrdinalIgnoreCase))
					{
						String ^ErrorString = String::Format(
							L"Could not locate the symbols for the executable. Please make sure they are in the correct directory. (Searched '{0}' and '{1}', exepath is '{2}'). ",
							gcnew String(ResultPath),
							gcnew String(ASCIIFileName),
							ExePath);

						if (ResultA == 0x806d0007 || ResultB == 0x806d0007 )
						{
							ErrorString += L" (pdb age mismatch)";
						}
						MessageBox::Show(ErrorString, L"Standalone Symbol Parser", MessageBoxButtons::OK, MessageBoxIcon::Warning);
					}
					continue;
				}

				CComPtr<IDiaSession> Session;

				if(FAILED(DiaSource->openSession(&Session)))
				{
					OutputDebugStringW(L"(StandaloneSymbolParser.dll) XenonSymbolParser::LoadSymbols(): Failed to open DIA session to pdb!\n");
					continue;
				}

				if(FAILED(Session->put_loadAddress(CurModule->BaseOfImage)))
				{
					OutputDebugStringW(L"(StandaloneSymbolParser.dll) CFXenonConsole::LoadSymbolsForModules(): Failed to set base address for DIA session!\n");
					continue;
				}

				SymbolSessions->Add(gcnew ModuleSymbolSession(Session.p, CurModule));
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	/**
	 * Unloads any currently loaded symbols.
	 */
	void XenonSymbolParser::UnloadSymbols()
	{
		for each(ModuleSymbolSession ^CurSession in SymbolSessions)
		{
			delete CurSession;
		}

		SymbolSessions->Clear();

		SymCleanup((HANDLE)GetHashCode());
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
	bool XenonSymbolParser::ResolveAddressToSymboInfo(unsigned __int64 Address, String ^%OutFileName, String ^%OutFunction, int %OutLineNumber)
	{
		OutFileName = String::Empty;
		OutFunction = String::Empty;
		OutLineNumber = 0;

		bool bSucceeded = false;

		for each(ModuleSymbolSession ^CurSession in SymbolSessions)
		{
			if(Address > CurSession->ModuleInformation->BaseOfImage && Address < CurSession->ModuleInformation->BaseOfImage + CurSession->ModuleInformation->ImageSize)
			{
				bSucceeded = CurSession->GetSymbolInformation(Address, OutFileName, OutFunction, OutLineNumber);
				break;
			}
		}

		if(OutFileName == String::Empty)
		{
			OutFileName = L"???";
		}

		if(OutFunction == String::Empty)
		{
			OutFunction = Address.ToString(L"X");
		}

		return bSucceeded;
	}
}
