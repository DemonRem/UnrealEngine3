/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

// This is the main DLL file.

#include "stdafx.h"
#include "Tag.h"
#include "TagSet.h"
#include "SharedSettings.h"
#include "GameSettings.h"
#include "FileGroup.h"
#include "FileSet.h"
#include "FileFilter.h"
#include "ConsoleInterface.h"
#include "EnumeratingPlatformsForm.h"
#include "SharedSettings.h"

using namespace System::IO;
using namespace System::Xml;
using namespace System::Xml::Serialization;
using namespace System::Threading;

// freacken macro's clash with .net types
#undef GetCurrentDirectory
#undef SetCurrentDirectory

namespace ConsoleInterface
{
	Tag::Tag() : Name(nullptr), Alt(nullptr)
	{
	}

	TagSet::TagSet() : Name(String::Empty), Tags(gcnew array<Tag^>(0))
	{
	}

	SharedSettings::SharedSettings() 
		: TagSets(gcnew array<TagSet^>(0)), KnownLanguages(gcnew array<Tag^>(0)), KnownPlatforms(gcnew array<Tag^>(0)),	FileGroups(gcnew array<FileGroup^>(0))
	{
	}

	FileFilter::FileFilter() : Name(String::Empty), bIsForFilename(true), bIsForDirectory(false)
	{
	}

	FileSet::FileSet() 
		: Path(nullptr), bIsRecursive(false), FileFilters(gcnew array<FileFilter^>(0))
	{
	}

	GameSettings::GameSettings()
		: FileGroups(gcnew array<FileGroup^>(0)), XGDFileRelativePath(String::Empty), PS3TitleID(String::Empty)
	{
	}

	FileGroup::FileGroup() 
		: bIsForSync(true), bIsForTOC(true), Tag(String::Empty), Platform(nullptr), Files(gcnew array<FileSet^>(0))
	{
	}

	ICollection<Platform^>^ DLLInterface::Platforms::get()
	{
		return mPlatforms->Values;
	}

	int DLLInterface::NumberOfPlatforms::get()
	{
		return mPlatforms->Count;
	}

	SharedSettings^ DLLInterface::Settings::get()
	{
		return mSharedSettings;
	}

	void DLLInterface::EnumeratingPlatformsUIThread(Object ^State)
	{
		ManualResetEvent ^Event = (ManualResetEvent^)State;

		if(Event != nullptr)
		{
			Application::Run(gcnew EnumeratingPlatformsForm(Event));
		}
	}

	FConsoleSupport* DLLInterface::LoadPlatformDLL(String ^DllPath)
	{
		if(DllPath == nullptr)
		{
			throw gcnew ArgumentNullException(L"Path");
		}

		//NOTE: We have to do this in case the target dll links to any dll's at load time.
		String^ CurDir = Directory::GetCurrentDirectory();
		DllPath = Path::GetFullPath(DllPath);
		String^ NewDir = Path::GetDirectoryName(DllPath);

		if(Directory::Exists(NewDir))
		{
			Directory::SetCurrentDirectory(NewDir);

			pin_ptr<const wchar_t> NativePath = PtrToStringChars(Path::GetFileName(DllPath));

			try
			{
				//NOTE: we'll leave this in our address space and let it be unloaded when the process is destroyed
				HMODULE Module = LoadLibraryW(NativePath);

				// restore our working directory
				Directory::SetCurrentDirectory(CurDir);

				if(Module)
				{
					FuncGetConsoleSupport SupportFunc = (FuncGetConsoleSupport)GetProcAddress(Module, "GetConsoleSupport");

					if(SupportFunc)
					{
						return SupportFunc();
					}
				}
			}
			catch( Exception^ Ex )
			{
				String^ Error = Ex->ToString();
				Console::WriteLine( Error );
				System::Diagnostics::Debug::WriteLine( Error );
			}
		}

		return NULL;
	}

	bool DLLInterface::HasPlatform(PlatformType PlatformToCheck)
	{
		return mPlatforms->ContainsKey(PlatformToCheck);
	}

	bool DLLInterface::LoadCookerSyncManifest( void )
	{
		if( mSharedSettings == nullptr )
		{
			try
			{
				XmlReader^ Reader = XmlReader::Create( L"CookerSync.xml" );
				XmlSerializer^ Serializer = gcnew XmlSerializer( SharedSettings::typeid );

				mSharedSettings = ( SharedSettings^ )Serializer->Deserialize( Reader );

				delete Serializer;
				delete Reader;
			}
			catch( Exception^ Ex )
			{
				String^ Error = Ex->ToString();
				Console::WriteLine( Error );
				System::Diagnostics::Debug::WriteLine( Error );
				mSharedSettings = gcnew SharedSettings();

				System::Windows::Forms::MessageBox::Show( nullptr, 
														  "CookerSync.xml failed to load; UnrealFrontEnd will not function correctly.\r\n\r\n" + Error, 
														  "CookerSync.xml Load Error!", 
														  System::Windows::Forms::MessageBoxButtons::OK, 
														  System::Windows::Forms::MessageBoxIcon::Error );
				return( false );
			}
		}

		return( true );
	}

	PlatformType DLLInterface::LoadPlatform( PlatformType PlatformsToLoad, PlatformType CurrentType, String^ ToolFolder, String^ ToolPrefix )
	{
		if( ( PlatformsToLoad & CurrentType ) == CurrentType && !mPlatforms->ContainsKey( CurrentType ) )
		{
#if _WIN64
			FConsoleSupport *Support = LoadPlatformDLL( ToolFolder + L"\\" + ToolPrefix + L"Tools_x64.dll" );
#else
			FConsoleSupport *Support = LoadPlatformDLL( ToolFolder + L"\\" + ToolPrefix + L"Tools.dll" );
#endif
			if( Support )
			{
				mPlatforms->Add( CurrentType, gcnew Platform( CurrentType, Support, mSharedSettings ) );
			}
			else
			{
				PlatformsToLoad = ( PlatformType )( PlatformsToLoad & ~CurrentType );
			}
		}

		return( PlatformsToLoad );
	}

	PlatformType DLLInterface::LoadPlatforms( PlatformType PlatformsToLoad )
	{
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::PC, L"Windows", L"Windows" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::Xbox360, L"Xenon", L"Xe" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::PS3, L"PS3", L"PS3" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::IPhone, L"IPhone", L"IPhone" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::Android, L"Android", L"Android" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::NGP, L"NGP", L"NGP" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::PCServer, L"Windows", L"Windows" );
		PlatformsToLoad = LoadPlatform( PlatformsToLoad, PlatformType::PCConsole, L"Windows", L"Windows" );

		return PlatformsToLoad;
	}

	bool DLLInterface::TryGetPlatform(PlatformType PlatformToGet, Platform ^%OutPlatform)
	{
		return mPlatforms->TryGetValue(PlatformToGet, OutPlatform);
	}
}