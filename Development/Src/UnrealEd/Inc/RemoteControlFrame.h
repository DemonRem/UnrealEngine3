/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __REMOTECONTROLFRAME_H__
#define __REMOTECONTROLFRAME_H__

// Forward declarations.
class FRemoteControlGame;

class WxRemoteControlFrame : public wxFrame
{
	/** FRemoteControlPageFactory is a friend so that it can call RegisterPageFactory. */
	friend class FRemoteControlPageFactory;

public:
	WxRemoteControlFrame(FRemoteControlGame *InGame, wxWindow *InParent, wxWindowID InID);

	/**
	 * Override of show so we notify FRemoteControlGame of visibility state.
	 */
	virtual UBOOL Show(UBOOL bShow);

	/**
	 * Repositions RemoteControl so its locked to upper right of game window.
	 */
	void Reposition();

private:
	FRemoteControlGame*	Game;
	wxPanel*			MainPanel;
	wxSizer*			MainSizer;
	wxNotebook*			Notebook;

	/**
	 * Register a RemoteControl page factory.
	 */
	static void RegisterPageFactory(FRemoteControlPageFactory *pPageFactory);

	/**
	 * Helper function to refresh the selected page.
	 */
	void RefreshSelectedPage();

	///////////////////////////////////////////////////
	// Wx events.

	/**
	 * Selected when user decides to refresh a page.
	 */
	void OnSwitchToGame(wxCommandEvent &In);

	/**
	 * Close event -- prevents window destruction; instead, hides the RemoteControl window.
	 */
	void OnClose(wxCloseEvent &In);

	/**
	 * Selected when user decides to refresh the active page.
	 */
	void OnPageRefresh(wxCommandEvent &In);

	/**
	 * Selected when the window is activated; refreshes the current page and updates property windows.
	 */
	void OnActivate(wxActivateEvent &In);

	/**
	 * Called when the active page changes.
	 */
	void OnPageChanged(wxNotebookEvent &In);

	DECLARE_EVENT_TABLE()
};

#endif // __REMOTECONTROLFRAME_H__
