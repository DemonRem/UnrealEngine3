/*=============================================================================
	UnNovodexLibs.cpp: Novodex library imports
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#if WITH_NOVODEX

// Novodex library imports

#define	USE_DEBUG_NOVODEX
//#define	USE_PROFILE_NOVODEX

#include "UnNovodexSupport.h"

#if defined(XBOX)

#if (defined _DEBUG) && (defined USE_DEBUG_NOVODEX)
	#pragma message("Linking Xenon DEBUG Novodex Libs")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/NxCookingDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysXCoreDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysXLoaderDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/FoundationDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/FrameworkDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/OpcodeDEBUG.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysXDEBUG.lib")
	#if USE_QUICKLOAD_CONVEX
		#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/NxQuickLoadDEBUG.lib")
	#endif
#else
	#pragma message("Linking Xenon RELEASE Novodex Libs")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/NxCooking.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysXCore.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysXLoader.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/Foundation.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/Framework.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/Opcode.lib")
	#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/PhysX.lib")
	#if USE_QUICKLOAD_CONVEX
		#pragma comment(lib, "External/PhysX/SDKs/lib/xbox360/NxQuickLoad.lib")
	#endif
#endif

#elif PS3

#else // Win32

#if (defined _DEBUG) && (defined USE_DEBUG_NOVODEX)
	#pragma message("Linking Win32 DEBUG Novodex Libs")
	#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/NxCookingDEBUG.lib")
	#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/PhysXLoaderDEBUG.lib")
	#if USE_QUICKLOAD_CONVEX
		#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/NxQuickLoadDEBUG.lib")
	#endif
#else
	#pragma message("Linking Win32 RELEASE Novodex Libs")
	#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/NxCooking.lib")
	#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/PhysXLoader.lib")
	#if USE_QUICKLOAD_CONVEX
		#pragma comment(lib, "../../External/PhysX/SDKs/lib/win32/NxQuickLoad.lib")
	#endif
#endif

//#pragma comment(lib, "../../External/Novodex/NxuStream2/lib/win32/NxuStream2.lib")

#endif	//#if defined(XBOX)

NxPhysicsSDK*			GNovodexSDK = NULL;
TMap<INT, NxScenePair>	GNovodexSceneMap;	// hardware scene support - using NxScenePair
TArray<NxActor*>		GNovodexPendingKillActor;
TArray<NxConvexMesh*>	GNovodexPendingKillConvex;
TArray<NxTriangleMesh*>	GNovodexPendingKillTriMesh;
TArray<NxHeightField*>	GNovodexPendingKillHeightfield;
TArray<NxCCDSkeleton*>	GNovodexPendingKillCCDSkeletons;
INT						GNumPhysXConvexMeshes = 0;
INT						GNumPhysXTriMeshes = 0;

// Novodex fluids
#ifndef NX_DISABLE_FLUIDS
TArray<NxFluid*>		GNovodexPendingKillFluids;
#endif

#if !NX_DISABLE_CLOTH
TArray<NxCloth*>		GNovodexPendingKillCloths;
TArray<NxClothMesh*>	GNovodexPendingKillClothMesh;
#endif // !NX_DISABLE_CLOTH

#endif // WITH_NOVODEX
