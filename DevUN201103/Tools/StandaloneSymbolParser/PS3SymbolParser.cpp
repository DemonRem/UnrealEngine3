/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
 #include "Stdafx.h"
#include "PS3SymbolParser.h"
#include <stdio.h>
#include <string>

using namespace System::IO;
using namespace System::Text;

namespace StandaloneSymbolParser
{
	/**
	 * Constructor.
	 */
	PS3SymbolParser::PS3SymbolParser() : CmdLineProcess(new FCommandLineWrapper())
	{

	}

	/**
	 * Converted to virtual void Dispose().
	 */
	PS3SymbolParser::~PS3SymbolParser()
	{
		this->!PS3SymbolParser();
		GC::SuppressFinalize(this);
	}

	/**
	 * Finalizer frees native memory.
	 */
	PS3SymbolParser::!PS3SymbolParser()
	{
		if(CmdLineProcess)
		{
			delete CmdLineProcess;
			CmdLineProcess = NULL;
		}
	}

	/**
	 * Loads symbols for an executable.
	 *
	 * @param	ExePath		The path to the executable whose symbols are going to be loaded.
	 * @param	Modules		The list of modules loaded by the process.
	 */
	bool PS3SymbolParser::LoadSymbols(String ^ExePath, array<ModuleInfo^> ^/*Modules*/)
	{
		// convert .xelf to .elf
		if(ExePath->EndsWith(L".xelf"))
		{
			ExePath = ExePath->Replace(".xelf", ".elf");
		}

		// use the .symbols.elf for LTCG builds (which were stripped)
		if(ExePath->EndsWith(L"LTCG.elf"))
		{
			ExePath = ExePath->Replace("LTCG.elf", "LTCG.symbols.elf");
		}

		// make sure the file exists
		if(!File::Exists(ExePath))
		{
			System::Windows::Forms::OpenFileDialog ^OpenFileDlg = gcnew System::Windows::Forms::OpenFileDialog();

			OpenFileDlg->Title = L"Find .elf file for the running PS3 game";
			OpenFileDlg->Filter = L".elf Files|*.elf";
			OpenFileDlg->RestoreDirectory = true;
			OpenFileDlg->DefaultExt = "elf";
			OpenFileDlg->AddExtension = true;

			if(OpenFileDlg->ShowDialog() == System::Windows::Forms::DialogResult::OK)
			{
				ExePath = OpenFileDlg->FileName;
			}
			else
			{
				return false;
			}
		}

		// cache the exe path
		ExecutablePath = ExePath;

		// start an Ps3Bin process (currently uses PS3BinEx to mean it has the -i option to allow for stdin input like addr2line)
		String^ SceRoot = Environment::GetEnvironmentVariable("SCE_PS3_ROOT");
		String ^CmdLine = String::Format(L"{0}\\host-win32\\sn\\bin\\ps3BinEx -a2l -i {1}", SceRoot, ExecutablePath);

		pin_ptr<Byte> NativeCmdLine = &Encoding::UTF8->GetBytes(CmdLine)[0];

		// attempt to create it; if it fails, then we do ps3bin in ResolveAddressToSymboInfo with one call per line
		CmdLineProcess->Create((char*)NativeCmdLine);


		// if we got here, we are good to go
		return true;
	}

	/**
	 * Unloads any currently loaded symbols.
	 */
	void PS3SymbolParser::UnloadSymbols()
	{
		CmdLineProcess->Terminate();
	}

	/**
	 * Retrieves the symbol info for an address.
	 *
	 * @param	Address			The address to retrieve the symbol information for.
	 * @param	OutFileName		The file that the instruction at Address belongs to.
	 * @param	OutFunction		The name of the function that owns the instruction pointed to by Address.
	 * @param	OutLineNumber	The line number within OutFileName that is represented by Address.
	 * @return	True if the function succeeds.
	 */
	bool PS3SymbolParser::ResolveAddressToSymboInfo(unsigned __int64 Address, String ^%OutFileName, String ^%OutFunction, int %OutLineNumber)
	{
		// if we didn't already create a PS3Bin process with stdin support, create it now for each line
		if (!CmdLineProcess->IsCreated())
		{
			// start an addr2line process
			String^ SceRoot = Environment::GetEnvironmentVariable("SCE_PS3_ROOT");
			String ^CmdLine = String::Format(L"{0}\\host-win32\\sn\\bin\\ps3bin {1} -a2l", SceRoot, ExecutablePath);

			pin_ptr<Byte> NativeCmdLine = &Encoding::UTF8->GetBytes(CmdLine)[0];

			if(!CmdLineProcess->Create((char*)NativeCmdLine))
			{
				System::Windows::Forms::MessageBox::Show(nullptr, L"Failed to create ps3bin process. Check your SCE_PS3_ROOT env variable.", L"Process creation error", System::Windows::Forms::MessageBoxButtons::OK, System::Windows::Forms::MessageBoxIcon::Error);
				return false;
			}
		}

		// send an address to addr2line
		char Temp[128];
		sprintf_s(Temp, 128, "0x%I64x", Address);

		CmdLineProcess->Write(Temp);
		CmdLineProcess->Write("\n");

		// now read the output; sample:
		// Address:       0x0004F28A
		// Directory:     Core/Inc
		// File Name:     UnCoreNative.h
		// Line Number:   38
		// Symbol:        CreateProfileFilename(FString const&, unsigned int)

		char* Addr = CmdLineProcess->ReadLine(2000);
		
		// handle the no debug info case (right now PS3Bin will try to write a warning, but PeekNamedPipe will just return 0 in the PS3BinEx case)
		// then we kill the process, which will kill PS3BinEx, and next call to this will use PS3Bin which will not handle
		// @todo: Remove all of this when PS3Bin returns ?? instead of No Dwarf Debug info found
		if (Addr == NULL || stricmp(Addr, "No Dwarf debug data found") == 0)
		{
			OutFileName = gcnew String("??");
			OutFunction = gcnew String("??");
			OutLineNumber = 0;

			// if there's no debug info, just terminate the child, there's nothing else to do
			CmdLineProcess->Terminate();
			
			return false;
		}
		char* Directory = CmdLineProcess->ReadLine(5000);
		char* Filename = CmdLineProcess->ReadLine(5000);
		char* LineNumber = CmdLineProcess->ReadLine(5000);
		char* Symbol = CmdLineProcess->ReadLine(5000);

		// all of the goods start at 15 columns
		int TextOffset = 15;

		std::string DirectoryStr = Directory + TextOffset;
		std::string FilenameStr = Filename + TextOffset;
		std::string SymbolStr = Symbol + TextOffset;
		std::string LineNumberStr = LineNumber + TextOffset;

		OutLineNumber = atoi(LineNumberStr.c_str());
		std::string FullPath = DirectoryStr + std::string("\\") + FilenameStr;
		OutFileName = gcnew String(FullPath.c_str());
		OutFunction = gcnew String(SymbolStr.c_str());

		return true;
	}

	/**
	 * Creates a formatted line of text for a callstack.
	 *
	 * @param	FileName		The file that the function call was made in.
	 * @param	Function		The name of the function that was called.
	 * @param	LineNumber		The line number the function call occured on.
	 *
	 * @return	A string formatted using "{0} [{1}:{2}]\r\n" as the template.
	 */
	String^ PS3SymbolParser::FormatCallstackLine(String ^FileName, String ^Function, int LineNumber)
	{
		return String::Format(L"{0} [{1}:{2}]\r\n", Function->EndsWith(L")") ? Function : Function + L"()", FileName->Length > 0 ? FileName : L"???", LineNumber.ToString());
	}
}
