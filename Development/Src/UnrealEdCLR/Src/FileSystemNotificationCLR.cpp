/*================================================================================
	FileSystemNotificationCLR.cpp: Code for watching directories for file notifications (file created, file changed)
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#include "UnrealEdCLR.h"
#include "FileSystemNotificationShared.h"

#ifdef __cplusplus_cli

#include "ManagedCodeSupportCLR.h"

using namespace System::IO;

/**
 * Struct to handle ref counting of "strings" and how many people care to listen for that extension
 */
ref class FFileFilter
{
public:
	String^ FilterString;
	int		RefCount;
};

/**
 * A wrapper class for listening to file system notification changes
 */
ref class MFileSystemNotificationWrapper
{
private:
	List <FileSystemWatcher^>^ Watchers;

	List <FFileFilter^>^ FileFilters;

	List <String^>^ ChangedEventQueue;
public:
	/**
	 * default constructor
	 */
	MFileSystemNotificationWrapper()
	{
		Watchers    = gcnew List <FileSystemWatcher^>();
		FileFilters = gcnew List <FFileFilter^>();
		ChangedEventQueue = gcnew List <String^>() ;

		//optional other directories for art depots, etc
		TArray<FString> FileListenerDirectories;
		GConfig->GetSingleLineArray(TEXT("FileListener"), TEXT("AdditionalFileListenerDirectories"), FileListenerDirectories, GEditorUserSettingsIni);

		//File watcher for the game directory
		FileListenerDirectories.AddItem(appRootDir());

		//create a file watcher per directory
		for (INT i = 0; i < FileListenerDirectories.Num(); ++i)
		{
			String^ DirectoryName = CLRTools::ToString(FileListenerDirectories(i));
			if (Directory::Exists(DirectoryName))
			{
				FileSystemWatcher^ NewWatcher = gcnew FileSystemWatcher();
				NewWatcher->Path = DirectoryName;
				NewWatcher->NotifyFilter = NotifyFilters::LastWrite | NotifyFilters::CreationTime;
				NewWatcher->Changed += gcnew FileSystemEventHandler(this, &MFileSystemNotificationWrapper::OnFileChanged);
				NewWatcher->EnableRaisingEvents = true;
				NewWatcher->IncludeSubdirectories = true;
				Watchers->Add(NewWatcher);
			}
			else
			{
				warnf(NAME_Warning, *FString::Printf(TEXT("Invalid File Notification Directory %s."), *FileListenerDirectories(i)));
			}
		}
	}

	/** Callback for when a file has changed that we want to watch*/
	void OnFileChanged(Object^ Owner, FileSystemEventArgs^ Args)
	{		
		// Args->FullPath comes in as relative/absolute path both at once
		// So converting to relative path and compare with it
		// When we tried absoluate path, it still does not remove relative path ..\..\
		// so we can't rely on being same for same directory. 
		FString RelativePath = GFileManager->ConvertToRelativePath(*CLRTools::ToFString(Args->FullPath));
		FFilename Filename = RelativePath;
		String^ FileExtension = CLRTools::ToString(Filename.GetExtension().ToLower());
		String^ CompareString = CLRTools::ToString(RelativePath);

		//run through all active file filters
		for (INT i = 0; i < FileFilters->Count; ++i)
		{
			//if this extension was found
			if (FileFilters[i]->FilterString == FileExtension)
			{
				//only add to the queue once this frame
				if (!ChangedEventQueue->Contains(CompareString))
				{
					ChangedEventQueue->Add(CompareString);
				}
				return;
			}
		}
	}

	/**Used for the delayed processing of file changed events*/
	void ProcessEvents ()
	{
		//run through all pending file events
		for (INT i = ChangedEventQueue->Count-1; i >= 0; --i)
		{
			FString FullPath = CLRTools::ToFString(ChangedEventQueue[i]);
			FString RelativePath = GFileManager->ConvertToRelativePath(*FullPath);
			//if the file still exists
			if (File::Exists(ChangedEventQueue[i]))
			{
				GCallbackEvent->Send( CALLBACK_FileChanged, RelativePath, NULL );
				ChangedEventQueue->RemoveAt(i);
			}
		}
	}

	/** Sets the extension for the file watcher */
	void AddExtension(String^ InExtension)
	{
		for (int i = 0; i < FileFilters->Count; ++i)
		{
			FFileFilter^ TestFilter = FileFilters[i];
			if (TestFilter->FilterString == InExtension)
			{
				TestFilter->RefCount++;
				return;
			}
		}
		//not found add a new filter to the list
		FFileFilter^ NewFilter = gcnew FFileFilter;
		NewFilter->FilterString = InExtension;
		NewFilter->RefCount = 1;
		FileFilters->Add(NewFilter);
	}

	/** Sets the extension for the file watcher */
	void RemoveExtension(String^ InExtension)
	{
		for (int i = 0; i < FileFilters->Count; ++i)
		{
			FFileFilter^ TestFilter = FileFilters[i];
			if (TestFilter->FilterString == InExtension)
			{
				TestFilter->RefCount--;
				if (TestFilter->RefCount == 0)
				{
					//this filter is no longer needed
					FileFilters->RemoveAt(i);
				}
				return;
			}
		}
	}
};

/**Global pointer to C# file notification class*/
GCRoot(MFileSystemNotificationWrapper^) GFileNotificationWrapper;

#endif  //__cplusplus_cli

/** Called from UUnrealEngine::Tick to send file notifications to avoid multiple events per file change */
void TickFileSystemNotifications()
{
#ifdef __cplusplus_cli
	if (GFileNotificationWrapper)
	{
		GFileNotificationWrapper->ProcessEvents();
	}
#endif
}


/**
 * Ensures that the file system notifications are active
 * @param InExtension - The extension of file to watch for
 * @param InStart - TRUE if this is a "turn on" event, FALSE if this is a "turn off" event
 */
void SetFileSystemNotification(const FString& InExtension, UBOOL InStart)
{
#ifdef __cplusplus_cli
	//if we're turning on file listening
	if (InStart)
	{
		//ensure a file notification object exists
		if (!GFileNotificationWrapper)
		{
			GFileNotificationWrapper = gcnew MFileSystemNotificationWrapper;
		}
		//add that extension to the ref counting
		GFileNotificationWrapper->AddExtension(CLRTools::ToString(InExtension));
	}
	else
	{
		//if we've ever started listening before, decrement ref count
		if (GFileNotificationWrapper)
		{
			GFileNotificationWrapper->RemoveExtension(CLRTools::ToString(InExtension));
		}
	}
#endif
}

/**
 * Shuts down global pointers before shut down
 */
void CloseFileSystemNotification()
{
#ifdef __cplusplus_cli
	//if the C# file notification system exists, we should delete it
	if (GFileNotificationWrapper)
	{
		delete GFileNotificationWrapper;
		GFileNotificationWrapper = NULL;
	}
#endif
}

