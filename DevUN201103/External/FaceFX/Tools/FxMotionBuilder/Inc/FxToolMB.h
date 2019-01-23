//------------------------------------------------------------------------------
// The main module for the FaceFx Motion Builder plug-in.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef __FxToolMB_H__
#define __FxToolMB_H__

#include <fbsdk/fbsdk.h>
// MotionBuilder redefines new, making it impossible to use the FaceFX
// SDK unless we undef it.
#if defined(new)
	#undef new 
#endif // defined(new)


#include "FxTool.h"

namespace OC3Ent
{

namespace Face
{

// The interface for the FaceFx Max plug-in.
class FxToolMB : public FxTool
{
	// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxToolMB, FxTool);
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxToolMB);
public:
	// Constructor.
	FxToolMB();
	// Destructor.
	~FxToolMB();

//------------------------------------------------------------------------------
// Initialization and shutdown.
//------------------------------------------------------------------------------

	// Starts up the tool.  If this is overloaded, make sure to call the
	// base class version first.
	virtual FxBool Startup( void );
	// Shuts down the tool.  If this is overloaded, make sure to call the
	// base class version first.
	virtual FxBool Shutdown( void );

//------------------------------------------------------------------------------
// Actor level operations.
//------------------------------------------------------------------------------

	// Creates a new actor with the specified name.
	virtual FxBool CreateActor( const FxString& name );
	// Loads an actor.
	virtual FxBool LoadActor( const FxString& filename );
	// Saves an actor.
	virtual FxBool SaveActor( const FxString& filename );

//------------------------------------------------------------------------------
// Bone level operations.
//------------------------------------------------------------------------------

	// Imports the reference pose to the specified frame.
	virtual FxBool ImportRefPose( FxInt32 frame );
	// Exports the reference pose from the specified frame containing the 
	// specified bones.
	virtual FxBool ExportRefPose( FxInt32 frame, const FxArray<FxString>& bones );

	// Imports specified bone pose to the specified frame.
	virtual FxBool ImportBonePose( const FxString& name, FxInt32 frame );
	// Exports a bone pose from a specified frame with the specified name.
	virtual FxBool ExportBonePose( FxInt32 frame, const FxString& name );
	// Batch imports bone poses using the specified configuration file.
	virtual FxBool BatchImportBonePoses( const FxString& filename, 
		FxInt32& startFrame, FxInt32& endFrame, FxBool onlyFindFrameRange = FxFalse );
	// Batch exports bone poses using the specified configuration file.
	virtual FxBool BatchExportBonePoses( const FxString& filename );

//------------------------------------------------------------------------------
// Animation level operations.
//------------------------------------------------------------------------------

	// Import the specified animation at the specified frame rate.
	virtual FxBool ImportAnim( const FxString& group, const FxString& name, 
		FxReal fps );
	// Cleans animation data from bones in ref pose.
	FxBool Clean();

protected:

	FBSystem        mSystem;
	FBPlayerControl mPlayerControl;

	// Sets a key in MotionBuilder.
	void _copyKeyData( HFBAnimationNode Node, double Data, FBTime time, FxBool removeDuplicateKeys);
	FxReal _getKeyData( HFBAnimationNode Node, FBTime time);
	// Finds the specified bone in the Max scene and returns it's transformation.
	// This only returns the bone's transformation in the current frame.  
	// Returns FxTrue if the bone was found and the transform is valid or
	// FxFalse otherwise.
	FxBool _getBoneTransform( HFBModel model, FxBone& bone, FBTime time);
	// Sets the specified bone's transform for the current frame.
	// Returns FxTrue if the transform was set or FxFalse otherwise.
	FxBool _setBoneTransform( const FxBone& bone, FBTime time, FxBool removeDuplicateKeys);
	// Stops any currently playing animation in Max.
	void _stopAnimationPlayback( void );
	// Removes all keys from node.
	void _removeKeys( HFBAnimationNode Node );

	FxQuat _getFxQuat(FBQuaternion fbquat);
	FBQuaternion _getFbQuat(FxQuat fxquat);


};

} // namespace Face

} // namespace OC3Ent

#endif
