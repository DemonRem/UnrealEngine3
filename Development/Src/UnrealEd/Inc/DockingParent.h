/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DOCKINGPARENT_H__
#define __DOCKINGPARENT_H__

/**
 *	Abstract class that encapsulates wxWidgets docking functionality.  Should be inherited by frames that wish to have dockable children.
 */
class FDockingParent
{
public:
	enum EDockHost
	{
		DH_None,
		DH_Top,
		DH_Bottom,
		DH_Left,
		DH_Right
	};

	FDockingParent(wxWindow* InParent);
	virtual ~FDockingParent();

	/**
	 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
	 *  @return A string representing a name to use for this docking parent.
	 */
	virtual const TCHAR* GetDockingParentName() const = 0;

	/**
	 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
	 */
	virtual const INT GetDockingParentVersion() const = 0;

	/**
	 * Loads the docking window layout from an XML file.  This should be called after all docking windows have been added.
	 * This function checks to see if the docking parent version number stored in the INI file is less than the current version number.
	 * If it is, it will not load the settings and let the new defaults be loaded instead.
	 * @return Whether or not a layout was loaded, if a layout was NOT loaded, the calling function should set default docking window positions.
	 */
	UBOOL LoadDockingLayout();

	/**
	 * Saves the current docking layout to an XML file.  This should be called in the destructor of the subclass.
	 */
	void SaveDockingLayout();

	/**
	 * Creates a new docking window using the parameters passed in. 
	 *
	 * @param ClientWindow	The client window for this docking window, most likely a wxPanel with some controls on it.
	 * @param DockHost		The dock host that the new docking widget should be attached to.
	 * @param WindowTitle	The title for this window.
	 * @param WindowName	Unique identifying name for this dock window, should be set when the window title is auto generated.
	 */
	wxDockWindow* AddDockingWindow(wxWindow* ClientWindow, EDockHost DockHost, const TCHAR* WindowTitle, const TCHAR* WindowName=NULL);


	/**
	 * Appends toggle items to the menu passed in that control visibility of all of the docking windows registered with the layout manager.
	 * @param MenuBar	wxMenuBar to append the docking menu to.
	 */
	void AppendDockingMenu(wxMenuBar* MenuBar);

	/**
	 * Sets the default area size of a dock host.
	 * @param Size	New Size for the dock host.
	 */
	void SetDockHostSize(EDockHost DockHost, INT Size);

protected:

	/**
	 * Generates a filename using the docking parent name, the game config directory, and the current game name for saving/loading layout files.
	 * @param OutFileName	Storage string for the generated filename.
	 */
	void GetLayoutFileName(FString &OutFileName);



	wxWindow* Parent;
	class wxLayoutManager* LayoutManager;
};

#endif

