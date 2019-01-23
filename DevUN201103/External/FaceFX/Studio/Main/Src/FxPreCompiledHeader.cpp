//------------------------------------------------------------------------------
// This file generates the pre-compiled header file.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#ifndef __UNREAL__

#ifdef FX_ENABLE_SECURITY
	#pragma message("Building nodelocked version")
#else
	#pragma message("Building non-nodelocked version")
#endif

#endif // __UNREAL__

#ifdef __UNREAL__
        // Avoid LNK4221.
        INT DummySymbolToSuppressLinkerWarningInFxPreCompiledHeader;
#endif // __UNREAL__
