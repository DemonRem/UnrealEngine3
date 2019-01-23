//------------------------------------------------------------------------------
// FaceFx SDK startup and shutdown.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxSDK.h"
#include "FxClass.h"
#include "FxActor.h"
#include "FxActorInstance.h"
#include "FxName.h"
#include "FxRefObject.h"
#include "FxRefString.h"
#include "FxArchive.h"
#include "FxVersionInfo.h"
#include "FxLinkFn.h"
#include "FxBonePoseNode.h"
#include "FxCombinerNode.h"
#include "FxGenericTargetNode.h"
#include "FxMorphTargetNode.h"
#include "FxCurrentTimeNode.h"
#include "FxDeltaNode.h"

#ifdef __UNREAL__
	#include "UnFaceFXNode.h"
	#include "UnFaceFXMaterialNode.h"
	#include "UnFaceFXMorphNode.h"
#endif

#ifdef FX_LITTLE_ENDIAN
	#pragma message( "FX_LITTLE_ENDIAN is defined." )
#endif

#ifdef FX_BIG_ENDIAN
	#pragma message( "FX_BIG_ENDIAN is defined." )
#endif

namespace OC3Ent
{

namespace Face
{

/// The version number of the FaceFX SDK.
/// This is the version number multiplied by 1000.  For example, version 1.0
/// is 1000, version 1.1 is 1100, and so on.
static const FxSize FX_SDK_VERSION = 1610;

/// This is the licensee name.  This is modified by the build script.
#define FX_LICENSEE_NAME "Unreal Engine 3 Licensee"

/// This is the licensee project name.  This is modified by the build script.
#define FX_LICENSEE_PROJECT_NAME "Unreal Engine 3 Project"

/// This is the unique build id.  This is FX_SDK_VERSION with a unique build
/// id appended (ie 1600100000 etc).  This is modified by the build script.
#define FX_BUILD_ID "1610100087"

/// This is the licensee version number.  This allows licensees to embed their
/// own custom data into FaceFX archives with backwards compatibility support.
/// This is the version number multiplied by 1000.  For example, version 1.0
/// is 1000, version 1.1 is 1100, and so on.  This is the version numbers that
/// licensees should modify if they need to support backwards compatibility of
/// custom data in FaceFX SDK archives.  Licensees should never modify the
/// main \ref OC3Ent::Face::FX_SDK_VERSION "FX_SDK_VERSION".  Licensees should
/// also never directly modify the built-in FaceFX class version numbers.  
/// Instead, always use FX_LICENSEE_VERSION in combination with the class 
/// version numbers in your custom serialization code.  This ensures that 
/// licensee modified code is still backwards compatible, even with future
/// versions of the FaceFX SDK.
static const FxSize FX_LICENSEE_VERSION = 1000;

void FX_CALL FxSDKStartup( void )
{
	FxMemoryAllocationPolicy defaultAllocationPolicy;
	FxSDKStartup(defaultAllocationPolicy);
}

void FX_CALL FxSDKStartup( const FxMemoryAllocationPolicy& allocationPolicy )
{
	// Startup the memory system.
	FxMemoryStartup(allocationPolicy);

	// Start up the class system.
	FxClass::Startup();

	// Start up the name system.
	FxName::Startup();

	// Register the built-in classes.
	FxObject::StaticClass();
	FxRefObject::StaticClass();
	FxRefString::StaticClass();
	FxNamedObject::StaticClass();
	FxBone::StaticClass();
	FxFaceGraphNode::StaticClass();
	FxFaceGraphNodeUserProperty::StaticClass();
	FxFaceGraphNodeLink::StaticClass();
	FxBonePoseNode::StaticClass();
	FxCombinerNode::StaticClass();
	FxCurrentTimeNode::StaticClass();
	FxDeltaNode::StaticClass();
	FxGenericTargetProxy::StaticClass();
	FxGenericTargetNode::StaticClass();
	FxMorphTargetNode::StaticClass();
	FxFaceGraph::StaticClass();
	FxAnimCurve::StaticClass();
	FxAnimGroup::StaticClass();
	FxAnimSet::StaticClass();
	FxAnim::StaticClass();
	FxActor::StaticClass();
	FxActorInstance::StaticClass();
		
#ifdef __UNREAL__
	FUnrealFaceFXNode::StaticClass();
	FUnrealFaceFXMaterialParameterNode::StaticClass();
	FUnrealFaceFXMorphNode::StaticClass();
#endif

	// Start up the link function system.
	FxLinkFn::Startup();

	// Start up the archive management system.
	FxArchive::Startup();

	// Start up the actor management system.
	FxActor::Startup();
}

void FX_CALL FxSDKShutdown( void )
{
	// Shut down the actor management system.
	FxActor::Shutdown();

	// Shut down the archive management system.
	FxArchive::Shutdown();

	// Shutdown the link function system.
	FxLinkFn::Shutdown();

	// Shutdown the name system.
	FxName::Shutdown();

	// Shutdown the class system.
	FxClass::Shutdown();

	// Shutdown the memory system.
	FxMemoryShutdown();
}

FxSize FX_CALL FxSDKGetVersion( void )
{
	return FX_SDK_VERSION;
}

FxString FX_CALL FxSDKGetLicenseeName( void )
{
	return FxString(FX_LICENSEE_NAME);
}

FxString FX_CALL FxSDKGetLicenseeProjectName( void )
{
	return FxString(FX_LICENSEE_PROJECT_NAME);
}

FxSize FX_CALL FxSDKGetLicenseeVersion( void )
{
	return FX_LICENSEE_VERSION;
}

FxString FX_CALL FxSDKGetBuildId( void )
{
	return FxString(FX_BUILD_ID);
}

} // namespace Face

} // namespace OC3Ent
