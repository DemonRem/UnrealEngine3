//------------------------------------------------------------------------------
// FaceFx SDK startup and shutdown.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxSDK_H__
#define FxSDK_H__

#include "FxMemory.h"
#include "FxPlatform.h"
#include "FxString.h"

namespace OC3Ent
{

namespace Face
{

/// Starts up and initializes the FaceFX SDK.
/// Initializes the \ref memory "memory system", ensures node types are 
/// correctly linked in, even if they are never used, and 
/// \ref OC3Ent::Face::FxName::Startup() "initializes the name table".
/// This may only be called exactly once in your application.  This version
/// of FxSDKStartup() uses the default memory allocation policy for the FaceFX
/// SDK which will use an allocation type of MAT_FreeStore or MAT_Heap depending
/// on the value of the FX_DEFAULT_TO_OPERATOR_NEW #define used to compile the 
/// library.  If you wish to provide your own memory allocation hooks for the
/// FaceFX SDK to use or you wish to toggle memory pooling on or off, you must 
/// use the version of FxSDKStartup() that takes an FxMemoryAllocationPolicy 
/// parameter.
void FX_CALL FxSDKStartup( void );
/// Starts up and initializes the FaceFX SDK.
/// Initializes the \ref memory "memory system", ensures node types are 
/// correctly linked in, even if they are never used, and
/// \ref OC3Ent::Face::FxName::Startup() "initializes the name table".
/// This may only be called exactly once in your application.
/// \param allocationPolicy The memory allocation policy to use.  If 
/// allocationPolicy is default constructed the memory allocation policy will
/// use an allocation type of MAT_FreeStore or MAT_Heap depending on the value
/// of the FX_DEFAULT_TO_OPERATOR_NEW #define used to compile the library.
void FX_CALL FxSDKStartup( const FxMemoryAllocationPolicy& allocationPolicy );
/// Shuts down the FaceFX SDK.
/// Shuts down the \ref memory "memory system", 
/// \ref OC3Ent::Face::FxActor::FlushActors() "flushes" all managed 
/// \ref OC3Ent::Face::FxActor "actors" actors, 
/// \ref OC3Ent::Face::FxArchive::UnmountAllArchives() "unmounts" all managed 
/// \ref OC3Ent::Face::FxArchive "archives" and  
/// \ref OC3Ent::Face::FxName::Shutdown() "shuts down the name table".
/// This must be called exactly once in your application.
void FX_CALL FxSDKShutdown( void );

/// Returns the current FaceFX SDK version number.  This version number has been
/// multiplied by 1000, so for example version 1.0 returns 1000, version 1.1
/// returns 1100, and so on.
FxSize FX_CALL FxSDKGetVersion( void );

/// Returns the FaceFX SDK licensee name.
FxString FX_CALL FxSDKGetLicenseeName( void );

/// Returns the FaceFX SDK licensee project name.
FxString FX_CALL FxSDKGetLicenseeProjectName( void );

/// Returns the FaceFX SDK licensee version number.  This version number has
/// been multiplied by 1000, so for example version 1.0 returns 1000, version 
/// 1.1 returns 1100, and so on.
FxSize FX_CALL FxSDKGetLicenseeVersion( void );

/// Returns the FaceFX SDK build id.
FxString FX_CALL FxSDKGetBuildId( void );

} // namespace Face

} // namespace OC3Ent

#endif
