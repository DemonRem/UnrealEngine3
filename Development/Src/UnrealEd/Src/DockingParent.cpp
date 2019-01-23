/*=============================================================================
DockingParent.cpp: This file holds the FDockingParent class that should be inherited by classes that wish to become parents for wxDockWindow objects.
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "DockingParent.h"

FDockingParent::FDockingParent(wxWindow* InParent) : 
Parent(InParent)
{
	LayoutManager = new wxLayoutManager(InParent);

	// Initialize Layout Manager and add default docking hosts.
	LayoutManager->Init();
	LayoutManager->AddDefaultHosts();

	const INT DefaultDockAreaSize = 300;
	wxHostInfo HostInfo;
	HostInfo = LayoutManager->GetDockHost( wxDEFAULT_RIGHT_HOST );
	HostInfo.GetHost()->SetAreaSize(DefaultDockAreaSize);

	HostInfo = LayoutManager->GetDockHost( wxDEFAULT_LEFT_HOST );
	HostInfo.GetHost()->SetAreaSize(DefaultDockAreaSize);

	HostInfo = LayoutManager->GetDockHost( wxDEFAULT_TOP_HOST );
	HostInfo.GetHost()->SetAreaSize(DefaultDockAreaSize);

	HostInfo = LayoutManager->GetDockHost( wxDEFAULT_BOTTOM_HOST );
	HostInfo.GetHost()->SetAreaSize(DefaultDockAreaSize);
}

FDockingParent::~FDockingParent()
{
	if(LayoutManager)
	{
		delete LayoutManager;
		LayoutManager = NULL;
	}
}

/**
 * Loads the docking window layout from an XML file.  This should be called after all docking windows have been added.
 * This function checks to see if the docking parent version number stored in the INI file is less than the current version number.
 * If it is, it will not load the settings and let the new defaults be loaded instead.
 * @return Whether or not a layout was loaded, if a layout was NOT loaded, the calling function should set default docking window positions.
 */
UBOOL FDockingParent::LoadDockingLayout()
{
	check(LayoutManager);

	UBOOL bFileLoadedOK = FALSE;

	// Check to see if the version of the layout that was previously saved out is outdated.
	INT SavedVersion = -1;
	
	GConfig->GetInt(TEXT("DockingLayoutVersions"), GetDockingParentName(), SavedVersion, GEditorUserSettingsIni);
	
	const INT CurrentVersion = GetDockingParentVersion();
	const UBOOL bOldVersion = CurrentVersion > SavedVersion;

	if( bOldVersion == FALSE)
	{
		// The version is OK to load, go ahead and load the XML file from disk, checking to make sure
		// it loads properly.
		FString FileName;
		GetLayoutFileName(FileName);

		const UBOOL bFileExists = ::wxFileExists(*FileName);
		if(bFileExists == TRUE)
		{
			wxFileInputStream InFile(*FileName);

			if( InFile.Ok() )
			{
				bFileLoadedOK = LayoutManager->LoadFromXML( InFile );
			}
		}
	}

	return bFileLoadedOK;
}

/**
 * Saves the current docking layout to an XML file.  This should be called in the destructor of the subclass.
 */
void FDockingParent::SaveDockingLayout()
{
	check(LayoutManager);

	// Save the layout of this file out to the config folder.
	FString LayoutFileName;
	GetLayoutFileName(LayoutFileName);


	wxFileOutputStream OutFile(*LayoutFileName);

	if(OutFile.Ok())
	{
		LayoutManager->SaveToXML(OutFile);
	}

	// Save the current version of the file to a INI file.
	GConfig->SetInt(TEXT("DockingLayoutVersions"), GetDockingParentName(), GetDockingParentVersion(), GEditorUserSettingsIni);
}

/**
 * Creates a new docking window using the parameters passed in. 
 *
 * @param ClientWindow	The client window for this docking window, most likely a wxPanel with some controls on it.
 * @param DockHost		The dock host that the new docking widget should be attached to.
 * @param WindowTitle	The title for this window.
 * @param WindowName	Unique identifying name for this dock window, should be set when the window title is auto generated.
 */
wxDockWindow* FDockingParent::AddDockingWindow(wxWindow* ClientWindow, EDockHost DockHost, const TCHAR* WindowTitle, const TCHAR* WindowName)
{
	if(WindowName == NULL)
	{
		WindowName = WindowTitle;
	}

	wxDockWindow* DockWindow = new wxDockWindow(Parent, wxID_ANY, WindowTitle, wxDefaultPosition, wxDefaultSize, WindowName);
	DockWindow->SetClient(ClientWindow);

	LayoutManager->AddDockWindow(DockWindow);


	// Dock the new dock window to a dock host if one was specified.
	const TCHAR* DockHostString = NULL;

	switch(DockHost)
	{
	case DH_Left:
		DockHostString = wxDEFAULT_LEFT_HOST;
		break;
	case DH_Right:
		DockHostString = wxDEFAULT_RIGHT_HOST;
		break;
	case DH_Top:
		DockHostString = wxDEFAULT_TOP_HOST;
		break;
	case DH_Bottom:
		DockHostString = wxDEFAULT_BOTTOM_HOST;
		break;
	default:
		DockHostString = NULL;
		break;
	}

	if(DockHostString != NULL)
	{
		wxHostInfo HostInfo;

		HostInfo = LayoutManager->GetDockHost( DockHostString  );
		LayoutManager->DockWindow( DockWindow, HostInfo );
	}
	


	return DockWindow;
}

/**
 * Appends toggle items to the menu passed in that control visibility of all of the docking windows registered with the layout manager.
 * @param MenuBar	wxMenuBar to append the docking menu to.
 */
void FDockingParent::AppendDockingMenu(wxMenuBar* MenuBar)
{
	wxMenu* DockingMenu = new wxMenu;
	LayoutManager->SetWindowMenu(DockingMenu);
	MenuBar->Append(DockingMenu, *LocalizeUnrealEd("&Window"));	
}

/**
* Sets the default area size of a dock host.
* @param Size	New Size for the dock host.
*/
void FDockingParent::SetDockHostSize(EDockHost DockHost, INT Size)
{
	// Dock the new dock window to a dock host if one was specified.
	const TCHAR* DockHostString = NULL;

	switch(DockHost)
	{
	case DH_Left:
		DockHostString = wxDEFAULT_LEFT_HOST;
		break;
	case DH_Right:
		DockHostString = wxDEFAULT_RIGHT_HOST;
		break;
	case DH_Top:
		DockHostString = wxDEFAULT_TOP_HOST;
		break;
	case DH_Bottom:
		DockHostString = wxDEFAULT_BOTTOM_HOST;
		break;
	default:
		DockHostString = NULL;
		break;
	}

	if(DockHostString != NULL)
	{
		wxHostInfo HostInfo;

		HostInfo = LayoutManager->GetDockHost( DockHostString  );
		HostInfo.GetHost()->SetAreaSize(Size);
	}
}

/**
 * Generates a filename using the docking parent name, the game config directory, and the current game name for saving/loading layout files.
 * @param OutFileName	Storage string for the generated filename.
 */
void FDockingParent::GetLayoutFileName(FString &OutFileName)
{
	OutFileName = FString::Printf(TEXT("%s%s%s_layout.xml"),*appGameConfigDir(), GGameName,GetDockingParentName());
}

