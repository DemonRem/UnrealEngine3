/*=============================================================================
	UnObjVer.cpp: Unreal version definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

// Defined separately so the build script can get to it easily (DO NOT CHANGE THIS MANUALLY)
#define	ENGINE_VERSION	3127

#define	BUILT_FROM_CHANGELIST	170553


INT	GEngineVersion				= ENGINE_VERSION;
INT	GBuiltFromChangeList		= BUILT_FROM_CHANGELIST;

INT	GEngineMinNetVersion		= 3077;
INT	GEngineNegotiationVersion	= 3077;

// @see UnObjVer.h for the list of changes/defines
INT	GPackageFileVersion			= VER_LATEST_ENGINE;
INT	GPackageFileMinVersion		= 225; // Code still handles 224 if saved with changelist #93355 or later
INT	GPackageFileLicenseeVersion = 0;
INT GPackageFileCookedContentVersion = VER_LATEST_COOKED_PACKAGE;

