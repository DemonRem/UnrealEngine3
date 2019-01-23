#ifndef PHYSX_LOADER_H
#define PHYSX_LOADER_H
/*----------------------------------------------------------------------------*\
|
|						Public Interface to AGEIA PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

#include "NxPhysicsSDK.h"
#include "NxUtilLib.h"

#ifdef _USRDLL
	#define NXPHYSXLOADERDLL_API extern "C" __declspec(dllexport)
#elif defined NX_USE_SDK_STATICLIBS
	#define NXPHYSXLOADERDLL_API
#else
	#define NXPHYSXLOADERDLL_API extern "C" __declspec(dllimport)
#endif


/** \addtogroup PhysXLoader
  @{
*/

/**
\brief Creates an instance of the physics SDK.

Creates an instance of this class. May not be a class member to avoid name mangling.
Pass the constant NX_PHYSICS_SDK_VERSION as the argument.
Because the class is a singleton class, multiple calls return the same object. However, each call must be
matched by a corresponding call to NxReleasePhysicsSDK, as the SDK uses a reference counter to the singleton.

\param sdkVersion Version number we are expecting(should be NX_PHYSICS_SDK_VERSION)
\param allocator User supplied interface for allocating memory(see #NxUserAllocator)
\param outputStream User supplied interface for reporting errors and displaying messages(see #NxUserOutputStream)
\param desc Optional descriptor used to define hardware allocation parameters
*/
NXPHYSXLOADERDLL_API NxPhysicsSDK*	NxCreatePhysicsSDK(NxU32 sdkVersion, NxUserAllocator* allocator = NULL, NxUserOutputStream* outputStream = NULL, const NxPhysicsSDKDesc& desc = NxPhysicsSDKDesc());

/**
\brief Creates an instance of the physics SDK with an ID string for application identification.

Creates an instance of this class. May not be a class member to avoid name mangling.
Pass the constant NX_PHYSICS_SDK_VERSION as the argument.
Because the class is a singleton class, multiple calls return the same object. However, each call must be
matched by a corresponding call to NxReleasePhysicsSDK, as the SDK uses a reference counter to the singleton.

\param sdkVersion Version number we are expecting(should be NX_PHYSICS_SDK_VERSION)
\param companyNameStr Character string for the game or application developer company name
\param appNameStr Character string for the game or application name
\param appVersionStr Character string for the game or application version
\param appUserDefinedStr Character string for additional, user defined data
\param allocator User supplied interface for allocating memory(see #NxUserAllocator)
\param outputStream User supplied interface for reporting errors and displaying messages(see #NxUserOutputStream)
\param desc Optional descriptor used to define hardware allocation parameters
*/
NXPHYSXLOADERDLL_API NxPhysicsSDK  *NxCreatePhysicsSDKWithID(NxU32 sdkVersion, 
                                                             char *companyNameStr, 
                                                             char *appNameStr,
                                                             char *appVersionStr,
                                                             char *appUserDefinedStr,
                                                             NxUserAllocator* allocator = NULL, 
                                                             NxUserOutputStream* outputStream = NULL, 
                                                             const NxPhysicsSDKDesc &desc = NxPhysicsSDKDesc());


/**
\brief Release an instance of the PhysX SDK.

Note that this must be called once for each prior call to NxCreatePhysicsSDK or NxCreatePhysicsSDKWithID, as
there is a reference counter.
*/
NXPHYSXLOADERDLL_API void			NxReleasePhysicsSDK(NxPhysicsSDK* sdk);

/**
\brief Retrieves the Physics SDK allocator.

Used by NxAllocateable's inlines and other macros below.

Before using this function the user must call #NxCreatePhysicsSDK(). If #NxCreatePhysicsSDK()
has not been called then NULL will be returned.
*/
NXPHYSXLOADERDLL_API NxUserAllocator* NxGetPhysicsSDKAllocator();

/**
\brief Retrieves the Foundation SDK after it has been created.

Before using this function the user must call #NxCreatePhysicsSDK(). If #NxCreatePhysicsSDK()
has not been called then NULL will be returned..
*/
NXPHYSXLOADERDLL_API NxFoundationSDK* NxGetFoundationSDK();

/**
\brief Retrieves the Physics SDK after it has been created.

Before using this function the user must call #NxCreatePhysicsSDK(). If #NxCreatePhysicsSDK()
has not been called then NULL will be returned.
*/
NXPHYSXLOADERDLL_API NxPhysicsSDK*    NxGetPhysicsSDK();

/**
\brief Retrieves the Physics SDK Utility Library

Before using this function the user must call #NxCreatePhysicsSDK(). If #NxCreatePhysicsSDK()
has not been called then NULL will be returned.
*/

NXPHYSXLOADERDLL_API NxUtilLib* NxGetUtilLib();

/**
\brief Retrieves the Physics SDK Cooking Library for a specific version of the SDK.

*/
class NxCookingInterface;
NXPHYSXLOADERDLL_API NxCookingInterface * NxGetCookingLib(NxU32 sdk_version_number);
NXPHYSXLOADERDLL_API NxCookingInterface * NxGetCookingLibWithID(NxU32 sdk_version_number, 
                                                                char *companyNameStr, 
                                                                char *appNameStr,
                                                                char *appVersionStr,
                                                                char *appUserDefinedStr);

/** @} */

#endif


//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2006 AGEIA Technologies, Inc.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND

