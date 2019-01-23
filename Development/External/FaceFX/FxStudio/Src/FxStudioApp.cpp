//------------------------------------------------------------------------------
// Special stub version of FxStudioApp for Unreal.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2005 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxStudioApp.h"
#include "FxUtilityFunctions.h"

#ifdef __UNREAL__
	#include "Engine.h"
#endif

namespace OC3Ent
{

namespace Face
{

wxWindow* FxStudioApp::_pMainWindow = NULL;
wxFileName FxStudioApp::_appDirectory;
FxBool FxStudioApp::_isSafeMode = FxFalse;

wxWindow* FxStudioApp::GetMainWindow( void )
{
	return _pMainWindow;
}

void FxStudioApp::SetMainWindow( wxWindow* pMainWindow )
{
	_pMainWindow = pMainWindow;
	// Automatically execute the autoexec.fxl script if it exists.
	GetAppDirectory();
	wxFileName autoexecPath(_appDirectory);
	autoexecPath.SetName(wxT("autoexec.fxl"));
	FxString autoexecFile(autoexecPath.GetFullPath().mb_str(wxConvLibc));
	if( FxFileExists(autoexecFile) )
	{
		// Give the main window a little fun time alone before executing.
		wxSleep(1);
		FxString execCommand = "exec -file \"";
		execCommand += autoexecFile;
		execCommand += "\"";
		FxVM::ExecuteCommand(execCommand);
	}
}

FxBool FxStudioApp::IsCommandLineMode( void )
{
	return FxFalse;
}

FxBool FxStudioApp::IsSafeMode( void )
{
	return _isSafeMode;
}

void FxStudioApp::CheckForSafeMode( void )
{
	// If the user is holding the shift key, we should start up
	// in safe mode.
	if( wxGetKeyState(WXK_SHIFT) )
	{
		_isSafeMode = FxTrue;
	}
	else
	{
		_isSafeMode = FxFalse;
	}
}

const wxFileName& FxStudioApp::GetAppDirectory( void )
{
	_appDirectory = wxString(appBaseDir());
	_appDirectory.AppendDir(wxString(wxT("FaceFX")));
	return _appDirectory;
}

wxString FxStudioApp::GetIconPath(const wxString& iconFileName)
{
	GetAppDirectory();
	wxFileName retval(_appDirectory);
	retval.AppendDir(wxT("res"));
	retval.AppendDir(wxT("icons"));
	retval.SetName(iconFileName);
	return retval.GetFullPath();
}

wxString FxStudioApp::GetBitmapPath(const wxString& bitmapFileName)
{
	GetAppDirectory();
	wxFileName retval(_appDirectory);
	retval.AppendDir(wxT("res"));
	retval.AppendDir(wxT("bmp"));
	retval.SetName(bitmapFileName);
	return retval.GetFullPath();
}

} // namespace Face

} // namespace OC3Ent
