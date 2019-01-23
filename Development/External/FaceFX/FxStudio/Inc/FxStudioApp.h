//------------------------------------------------------------------------------
// Special stub version of FxStudioApp for Unreal.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2005 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxStudioApp_H__
#define FxStudioApp_H__

#include "FxStudioMainWin.h"

namespace OC3Ent
{

namespace Face
{

// FxStudioApp stub.
class FxStudioApp
{
public:
	// Returns a pointer to the main window.
	static wxWindow* GetMainWindow( void );
	// Sets the pointer to the main window.
	static void SetMainWindow( wxWindow* pMainWindow );
	// Returns FxTrue if FaceFX Studio is in Command Line Mode.
	static FxBool IsCommandLineMode( void );
	// Returns FxTrue if FaceFX Studio was started in "safe" mode.
	static FxBool IsSafeMode( void );

	// Checks for and sets the safe mode states.
	static void CheckForSafeMode( void );

	// Returns the application directory.
	static const wxFileName& GetAppDirectory( void );
	// Returns an icon path.
	static wxString GetIconPath(const wxString& iconFileName);
	// Returns a bitmap path.
	static wxString GetBitmapPath(const wxString& bitmapFileName);

protected:
	// The main window.
	static wxWindow* _pMainWindow;
	// The application directory.
	static wxFileName _appDirectory;
	// The safe mode state.
	static FxBool _isSafeMode;
};

} // namespace Face

} // namespace OC3Ent

#endif
