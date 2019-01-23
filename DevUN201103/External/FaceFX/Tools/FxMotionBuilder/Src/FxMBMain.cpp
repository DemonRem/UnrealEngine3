//------------------------------------------------------------------------------
// The main module for the FaceFx Motion Builder plug-in.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include <fbsdk/fbsdk.h>
// MotionBuilder redefines new, making it impossible to use the FaceFX
// SDK unless we undef it.
#if defined(new)
	#undef new 
#endif // defined(new)

#include "FxMBData.h"
#include "FxToolLog.h"

#ifdef KARCH_ENV_WIN
	#include <windows.h>
#endif


// Library declaration.
FBLibraryDeclare( FaceFXMotionBuilderTool )
{
	FBLibraryRegister( FaceFXMotionBuilderTool );
}
FBLibraryDeclareEnd;

bool FBLibrary::LibInit()		
{ 
	FBSystem system;
	// Start up the FaceFX plug-in interface.
	OC3Ent::Face::FxMBData::mbInterface.Startup();
	char *appdir =  system.ApplicationPath;
	OC3Ent::Face::FxString logPath(appdir);
	logPath += "\\FaceFX_Log.txt";
	OC3Ent::Face::FxToolLogFile::SetPath(logPath);
	FxToolLog("Loaded FaceFX for MotionBuilder.");
	return true;
}
bool FBLibrary::LibOpen()		{ return true; }
bool FBLibrary::LibReady()		{ return true; }
bool FBLibrary::LibClose()		
{ 
	// Shut down the FaceFX plug-in interface.
	return OC3Ent::Face::FxMBData::mbInterface.Shutdown();
	FxToolLog("Unloaded FaceFX for MotionBuilder");
}
bool FBLibrary::LibRelease()	{ return true; }
