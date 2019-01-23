//------------------------------------------------------------------------------
// All built-in FaceFX console varaibles should be included here.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxInternalConsoleVariables.h"
#include "FxVM.h"
#include "FxStudioApp.h"
#include "FxSDK.h"
#include "FxAudioWidget.h"
#include "FxAudioPlayerWidget.h"
#include "FxAnalysisWidget.h"
//@todo Deliberate exposure of Fonix core here.  We may want to rethink this at 
//      some point in the future?
#include "FxAnalysisCoreFonix.h"

#ifdef USE_FX_RENDERER
	#include "FxRenderWidgetOC3.h"
#endif // USE_FX_RENDERER

namespace OC3Ent
{

namespace Face
{

void FxInternalConsoleVariables::RegisterAll( void )
{
	// Register the console variable class.
	FxConsoleVariable::StaticClass();

	FxChar str[32] = {0};

	// Register the console variables.
	FxVM::RegisterConsoleVariable("g_appdirectory", 
		                          FxString(FxStudioApp::GetAppDirectory().GetPath().mb_str(wxConvLibc)), 
								  FX_CONSOLE_VARIABLE_READONLY, 
								  NULL);
	FxItoa(FxSDKGetVersion(), str);
	FxVM::RegisterConsoleVariable("g_sdkversion", 
								  str, 
								  FX_CONSOLE_VARIABLE_READONLY, 
							      NULL);
	FxVM::RegisterConsoleVariable("g_licenseename", 
								  FxSDKGetLicenseeName(), 
								  FX_CONSOLE_VARIABLE_READONLY, 
								  NULL);
	FxVM::RegisterConsoleVariable("g_licenseeprojectname", 
								  FxSDKGetLicenseeProjectName(), 
								  FX_CONSOLE_VARIABLE_READONLY, 
								  NULL);
	FxItoa(FxSDKGetLicenseeVersion(), str);
	FxVM::RegisterConsoleVariable("g_licenseeversion", 
								  str, 
								  FX_CONSOLE_VARIABLE_READONLY, 
								  NULL);
	FxVM::RegisterConsoleVariable("g_playbackspeed",
								  "1.0",
								  0,
								  NULL);
	FxVM::RegisterConsoleVariable("g_audiosystem",
								  "openal",
								  0,
								  FxAudioPlayerWidget::OnAudioSystemChanged);
	FxVM::RegisterConsoleVariable("g_loopallfromhere",
								  "0",
								  0,
								  NULL);
	FxVM::RegisterConsoleVariable("aw_showstress",
								  "0",
								  0,
								  FxAudioWidget::OnDrawOptionUpdated);
	FxVM::RegisterConsoleVariable("cw_historylength",
								  "250",
								  0,
								  NULL);
	FxVM::RegisterConsoleVariable("pl_realtimeupdate",
								  "0",
								  0,
								  NULL);
	FxVM::RegisterConsoleVariable("a_audiomin",
								  "0.5",
								  0,
								  FxAnalysisWidget::OnAudioMinimumUpdated);
	FxVM::RegisterConsoleVariable("a_audiomax",
								  "90.0",
								  0,
							      FxAnalysisWidget::OnAudioMaximumUpdated);
	FxVM::RegisterConsoleVariable("a_detectspeech",
								  "auto",
								  0,
								  NULL);
	FxVM::RegisterConsoleVariable("a_debugfonix",
								  "0",
								  0,
								  FxAnalysisCoreFonix::OnDebugFonixUpdated);

#ifdef USE_FX_RENDERER
	FxVM::RegisterConsoleVariable("rw_showbindpose",
								  "0",
								  0,
								  FxRenderWidgetOC3::OnShowBindPoseUpdated);
	FxVM::RegisterConsoleVariable("rw_scalefactor",
								  "1.0",
								  0,
								  FxRenderWidgetOC3::OnScaleFactorUpdated);
#endif // USE_FX_RENDERER
}

void FxInternalConsoleVariables::UnregisterAll( void )
{
	// Unregister the console variables.
	FxVM::UnregisterConsoleVariable("g_appdirectory");
	FxVM::UnregisterConsoleVariable("g_sdkversion");
	FxVM::UnregisterConsoleVariable("g_licenseename");
	FxVM::UnregisterConsoleVariable("g_licenseeprojectname");
	FxVM::UnregisterConsoleVariable("g_licenseeversion");
	FxVM::UnregisterConsoleVariable("g_playbackspeed");
	FxVM::UnregisterConsoleVariable("g_audiosystem");
	FxVM::UnregisterConsoleVariable("g_loopallfromhere");
	FxVM::UnregisterConsoleVariable("aw_showstress");
	FxVM::UnregisterConsoleVariable("cw_historylength");
	FxVM::UnregisterConsoleVariable("pl_realtimeupdate");
	FxVM::UnregisterConsoleVariable("a_audiomin");
	FxVM::UnregisterConsoleVariable("a_audiomax");
	FxVM::UnregisterConsoleVariable("a_detectspeech");
	FxVM::UnregisterConsoleVariable("a_debugfonix");

#ifdef USE_FX_RENDERER
	FxVM::UnregisterConsoleVariable("rw_showbindpose");
	FxVM::UnregisterConsoleVariable("rw_scalefactor");
#endif // USE_FX_RENDERER
}

} // namespace Face

} // namespace OC3Ent