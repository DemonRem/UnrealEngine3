//------------------------------------------------------------------------------
// This file generates the pre-compiled header file.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#ifndef __UNREAL__
#if defined(__WXMSW__) && !defined(__WXWINCE__) && defined(_MSC_VER) && _MSC_VER >= 1400
	#pragma message("Linking Windows Common Controls - Should only happen in VS8+")
	#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")
#endif
#endif // __UNREAL__