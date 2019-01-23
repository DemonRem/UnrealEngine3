/*================================================================================
	InteropCLR.cpp: Code for interfacing C++ with C++/CLI and WPF
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#include "UnrealEdCLR.h"

#include "InteropShared.h"
#include "ManagedCodeSupportCLR.h"

using namespace System::IO;
using namespace System::Windows::Markup;


namespace InteropTools
{

	/**
	 * Unreal editor backend interface.  This allows UnrealEdCSharp to call native C++ and C++/CLI
	 * editor functions.
	 */
	ref class MEditorBackend : UnrealEd::IEditorBackendInterface
	{

	public:

		/** Writes the specified text to the warning log */
		virtual void LogWarningMessage( String^ Text )
		{
			CLRTools::LogWarningMessage( Text );
		}


		// ...

	};


	/** Initializes the backend interface for the the editor's companion module: UnrealEdCSharp */
	void InitUnrealEdCSharpBackend()
	{
		MEditorBackend^ EditorBackend = gcnew MEditorBackend();

		// NOTE: If this throws an exception then it's likely that UnrealEdCSharp.dll could not
		//       be loaded for some reason
		UnrealEd::Backend::InitBackend( EditorBackend );
	}


	/** Loads any resource dictionaries needed by our WPF controls (localized text, etc.) */
	void LoadResourceDictionaries()
	{
		// OK, first make sure that an Application singleton is allocated.  Because we're hosting WPF
		// controls in a Win32 app, there may not be an Application yet.  WPF will always search the
		// Application's resource dictionaries if it can't find a resource inside of the control itself.
		if( Application::Current == nullptr )
		{
			Application^ MyApp = gcnew Application();
		}


		// Grab the executable path
		TCHAR ApplicationPathChars[ MAX_PATH ];
		GetModuleFileName( NULL, ApplicationPathChars, MAX_PATH - 1 );
		FString ApplicationPath( ApplicationPathChars );



		// Setup parser context.  This is needed so that files that are referenced from within the .xaml file
		// (such as other .xaml files or images) can be located with paths relative to the application folder.
		ParserContext^ MyParserContext = gcnew ParserContext();
		{
			// Create and assign the base URI for the parser context
			Uri^ BaseUri = Packaging::PackUriHelper::Create( gcnew Uri( CLRTools::ToString( ApplicationPath ) ) );
			MyParserContext->BaseUri = BaseUri;
		}




		// Load up localized (string) resources first
		{
			const FString WPFResourcePath = FString::Printf( TEXT( "%sWPF\\Localized\\" ), *GetEditorResourcesDir() );

			// First look for "INT" files.  We'll always fall back on that if we can't find a file for
			// the editor's currently active language
			const TCHAR* DefaultLanguageName = TEXT( "INT" );
			FString DefaultLanguageFileNameSuffix = FString::Printf( TEXT( ".%s.xaml" ), DefaultLanguageName );
			TArray< FString > DefaultLanguageResourceFileNames;
			GFileManager->FindFiles(
				DefaultLanguageResourceFileNames,
				*FString::Printf( TEXT( "%s*%s" ), *WPFResourcePath, *DefaultLanguageFileNameSuffix ),
				TRUE,
				FALSE );

			for( INT CurResourceFileIndex = 0; CurResourceFileIndex < DefaultLanguageResourceFileNames.Num(); ++CurResourceFileIndex )
			{
				FString DefaultLanguageFileName = DefaultLanguageResourceFileNames( CurResourceFileIndex );
				FString WPFResourcePathAndFileName = WPFResourcePath * DefaultLanguageFileName;
				
				// First try to load default language (INT) version.
				try
				{
					// Allocate Xaml file reader
					auto_handle< StreamReader > MyStreamReader =
						gcnew StreamReader( CLRTools::ToString( WPFResourcePathAndFileName ) );

					// Load the file
					ResourceDictionary^ MyDictionary =
						static_cast< ResourceDictionary^ >( XamlReader::Load( MyStreamReader->BaseStream, MyParserContext ) );

					// Add to the application's list of dictionaries
					Application::Current->Resources->MergedDictionaries->Add( MyDictionary );
				}

				catch( Exception^ E )
				{
					// Error loading .xaml file
					appMsgf( AMT_OK,
						*FString::Printf( TEXT( "Error loading default .xaml resource dictionary from file [%s]; reason [%s]" ),
						*WPFResourcePathAndFileName,
						*CLRTools::ToFString( E->Message ) ) );
				}

				// Now check to see if we have a version of this file for the currently active language
				const TCHAR* LanguageName = UObject::GetLanguage();
				if( appStricmp(LanguageName, DefaultLanguageName) != 0 )
				{
					FString LanguageFileNameSuffix = FString::Printf( TEXT( ".%s.xaml" ), LanguageName );
					FString ChoppedFileName = DefaultLanguageFileName.Left( DefaultLanguageFileName.Len() - DefaultLanguageFileNameSuffix.Len() );
					FString LocalizedResourceFileName = ChoppedFileName + LanguageFileNameSuffix;
					FString LocalizedResourcePathAndFileName = WPFResourcePath * LocalizedResourceFileName;
					if( GFileManager->FileSize( *LocalizedResourcePathAndFileName ) >= 0 )
					{
						// Great, we found a localized version of the file!

						try
						{
							// Allocate Xaml file reader
							auto_handle< StreamReader > MyStreamReader =
								gcnew StreamReader( CLRTools::ToString( LocalizedResourcePathAndFileName ) );

							// Load the file
							ResourceDictionary^ MyDictionary =
								static_cast< ResourceDictionary^ >( XamlReader::Load( MyStreamReader->BaseStream, MyParserContext ) );

							// Add to the application's list of dictionaries
							// According to http://msdn.microsoft.com/en-us/library/aa350178.aspx in case of the same key in multiple dictionaries,
							// the key returned will be the one from the merged dictionary added LAST to the MergedDictionaries collection.
							Application::Current->Resources->MergedDictionaries->Add( MyDictionary );
						}

						catch( Exception^ E )
						{
							// Error loading .xaml file
							appMsgf( AMT_OK,
								*FString::Printf( TEXT( "Error loading localized .xaml resource dictionary from file [%s]; reason [%s]" ),
								*LocalizedResourcePathAndFileName,
								*CLRTools::ToFString( E->Message ) ) );
						}
					}
					else
					{
						// Print a warning about missing LOC file and continue loading.  We'll just use the INT
						// resource file for these strings.
						warnf( TEXT( "Editor warning: Missing localized version of WPF XAML resource file [%s] (expected [%s] for current language)" ), *WPFResourcePathAndFileName, *LocalizedResourcePathAndFileName );
					}
				}
			}
		}
	}

}