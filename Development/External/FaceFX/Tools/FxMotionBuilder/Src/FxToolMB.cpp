//------------------------------------------------------------------------------
// The FaceFx Max plug-in interface.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxActorInstance.h"
#include "FxToolMB.h"
#include "FxFastFaceGraphBaker.h"
#include "FxToolLog.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxToolMBVersion 0

// Implement the class.
FX_IMPLEMENT_CLASS(FxToolMB, kCurrentFxToolMBVersion, FxTool);

// Constructor.
FxToolMB::FxToolMB()
{
}

// Destructor.
FxToolMB::~FxToolMB()
{
}

//------------------------------------------------------------------------------
// Initialization and shutdown.
//------------------------------------------------------------------------------

// Starts up the tool.  If this is overloaded, make sure to call the
// base class version first.
FxBool FxToolMB::Startup( void )
{
	return Super::Startup();
}

// Shuts down the tool.  If this is overloaded, make sure to call the
// base class version first.
FxBool FxToolMB::Shutdown( void )
{
	return Super::Shutdown();
}

//------------------------------------------------------------------------------
// Actor level operations.
//------------------------------------------------------------------------------

// Creates a new actor with the specified name.
FxBool FxToolMB::CreateActor( const FxString& name )
{
	FxBool result;
	if( 0 == name.Length() )
	{
		FxToolLog("Attempted to create new actor with the NULL name. Creating new actor called NewActor instead");
		result = Super::CreateActor("NewActor");
	}
	else
	{
		FxToolLog("attempting to create actor %s", name.GetData());
		result = Super::CreateActor(name);
	}
	if( result && _actor )
	{
		FxToolLog("created actor %s", _actor->GetName().GetAsCstr());
	}
	return result;
}

// Loads an actor.
FxBool FxToolMB::LoadActor( const FxString& filename )
{
	FxToolLog("attempting to load actor file %s", filename.GetData());
	FxBool result = Super::LoadActor(filename);
	if( result && _actor )
	{
		FxToolLog("loaded actor named %s", _actor->GetName().GetAsCstr());
	}
	else
	{
		if( _shouldDisplayWarningDialogs )
		{
			if(IsNoSave())
			{
				FBMessageBox("Error", "Could not load file.  Only authorized content can be loaded by this version of the product.", "OK");	
			}
			else
			{
				FBMessageBox("Error", "Could not load file.  Make sure the most recent version of the plug-in is installed.", "OK");	
			}
		}
		FxToolLog("failed to load actor file %s", filename.GetData());
	}
	return result;
}

// Saves an actor.
FxBool FxToolMB::SaveActor( const FxString& filename )
{
	FxToolLog("attempting to save actor file %s", filename.GetData());
	FxBool result = Super::SaveActor(filename);
	if( result )
	{
		FxToolLog("saved actor");
	}
	else
	{
		FxToolLog("failed to save actor file %s", filename.GetData());
	}
	return result;
}

//------------------------------------------------------------------------------
// Bone level operations.
//------------------------------------------------------------------------------

// Imports the reference pose to the specified frame.
FxBool FxToolMB::ImportRefPose( FxInt32 frame )
{
	if( _actor ) 
	{
		// Stop any currently playing animation.
		_stopAnimationPlayback();

		// Import each bone from the reference pose.
		for( FxSize i = 0; i < _actor->GetMasterBoneList().GetNumRefBones(); ++i )
		{
			const FxBone& refBone = _actor->GetMasterBoneList().GetRefBone(i);
			FBTime time;
			FxReal seconds = (FxReal)frame / mPlayerControl.GetTransportFpsValue();
			time.SetSecondDouble(seconds);
			if( !_setBoneTransform(refBone, time, FxTrue) )
			{
				FxToolLog("_setBoneTransform failed for bone %s", refBone.GetNameAsCstr());
			}
		}
		FxToolLog("attempting to import reference pose. to frame %d", frame);
		return FxTrue;
	}
	return FxFalse;
}

// Exports the reference pose from the specified frame containing the 
// specified bones.
FxBool FxToolMB::ExportRefPose( FxInt32 frame, const FxArray<FxString>& bones  )
{
	FxToolLog("attempting to export reference pose from frame %d", frame);
	if( _actor )
	{
		// Stop any currently playing animation.
		_stopAnimationPlayback();

		FxArray<FxBone> boneArray;
		FBModelList	lSelectedModelList;

		FBGetSelectedModels( lSelectedModelList );
		for(FxSize i = 0; i < (FxSize)lSelectedModelList.GetCount(); ++i)
		{
			FBTime time;
			FxReal seconds = (FxReal)frame / mPlayerControl.GetTransportFpsValue();
			time.SetSecondDouble(seconds);
			FxBone bone;
			_getBoneTransform(lSelectedModelList.GetAt(i), bone, time);
			boneArray.PushBack(bone);
		}
		if(boneArray.Length() == 0)
		{
			FBMessageBox("Error", "Please select the bones you want included in the reference pose.", "OK");	
		}
		else
		{
			// Clear the current reference pose.
			_actor->GetMasterBoneList().Clear();

			FxToolLog("exported reference pose containing %d bones:", boneArray.Length());

			// Export the reference pose.
			for( FxSize i = 0; i < boneArray.Length(); ++i )
			{
				_actor->GetMasterBoneList().AddRefBone(boneArray[i]);
				FxToolLog("    %s", boneArray[i].GetName().GetAsCstr());
			}

			// Prune any bone pose nodes.
			_actor->GetMasterBoneList().Prune(_actor->GetDecompiledFaceGraph());
			// Compile - this also "pulls" the bones.
			_actor->CompileFaceGraph();
			
			return FxTrue;
		}
	}
	return FxFalse;
}

// Imports specified bone pose to the specified frame.
FxBool FxToolMB::ImportBonePose( const FxString& name, FxInt32 frame )
{

	FxToolLog("attempting to import bone pose %s to the current frame", name.GetData());
	if( _actor )
	{
		// Stop any currently playing animation.
		_stopAnimationPlayback();

		// Search for the bone pose node.
		FxBonePoseNode* pBonePoseNode = NULL;
		FxFaceGraph::Iterator bonePoseNodeIter    = _actor->GetDecompiledFaceGraph().Begin(FxBonePoseNode::StaticClass());
		FxFaceGraph::Iterator bonePoseNodeIterEnd = _actor->GetDecompiledFaceGraph().End(FxBonePoseNode::StaticClass());
		for( ; bonePoseNodeIter != bonePoseNodeIterEnd; ++bonePoseNodeIter )
		{
			FxFaceGraphNode* pNode = bonePoseNodeIter.GetNode();
			if( pNode )
			{
				if( pNode->GetName().GetAsString() == name )
				{
					pBonePoseNode = FxCast<FxBonePoseNode>(pNode);
					break;
				}
			}
		}
		if( pBonePoseNode )
		{
			FBTime time;
			FxReal seconds = (FxReal)frame / mPlayerControl.GetTransportFpsValue();
			time.SetSecondDouble(seconds);

            // Import each bone from the bone pose node.  If there is a bone in
			// the reference pose that is not in the bone pose node, use the
			// reference pose transform.  This is essential because bones are
			// pruned from the bone pose nodes.
			for( FxSize i = 0; i < _actor->GetMasterBoneList().GetNumRefBones(); ++i )
			{
				const FxBone& refBone = _actor->GetMasterBoneList().GetRefBone(i);
				FxBone bone = refBone;
				// Search for the bone in the bone pose node.
				for( FxSize j = 0; j < pBonePoseNode->GetNumBones(); ++j )
				{
					if( pBonePoseNode->GetBone(j).GetName() == refBone.GetName() )
					{
						bone = pBonePoseNode->GetBone(j);
						break;
					}
				}
				if( !_setBoneTransform(bone, time, FxTrue) )
				{
					FxToolLog("_setBoneTransform failed for bone %s", bone.GetNameAsCstr());
				}
			}

			FxToolLog("imported bone pose");
			return FxTrue;
		}
		else
		{
			FxToolLog("could not find bone pose!");
		}
	}
	return FxFalse;
}

// Exports a bone pose from a specified frame with the specified name.
FxBool FxToolMB::ExportBonePose( FxInt32 frame, const FxString& name )
{
	FxToolLog("attempting to export bone pose from current frame %d.", frame);
	if( _actor )
	{

        // Stop any currently playing animation.
		_stopAnimationPlayback();

		if( 0 == _actor->GetMasterBoneList().GetNumRefBones() )
		{
			if( _shouldDisplayWarningDialogs )
			{
				FBMessageBox("Error", "Please export a reference pose first.", "OK");	
			}
			return FxFalse;
		}

		// Create the bone pose node.
		FxBonePoseNode* pBonePoseNode = new FxBonePoseNode();
		pBonePoseNode->SetName(name.GetData());

		FBTime time;
		FxReal seconds = (FxReal)frame / mPlayerControl.GetTransportFpsValue();
		time.SetSecondDouble(seconds);
		mPlayerControl.Goto(time);

		// Export each reference bone to the bone pose node.
		for( FxSize i = 0; i < _actor->GetMasterBoneList().GetNumRefBones(); ++i )
		{
			FxBone bone;
			HFBModel model = FBFindModelByName( const_cast<FxChar*>(_actor->GetMasterBoneList().GetRefBone(i).GetNameAsCstr()) );
			if( _getBoneTransform(model, bone, time) )
			{
				pBonePoseNode->AddBone(bone);
			}
		}
		// Add the bone pose node to the face graph.
		if( FxFalse == _actor->GetDecompiledFaceGraph().AddNode(pBonePoseNode) )
		{
			// If the node was already in the face graph, we should
			// overwrite it but issue an indication to the user.
			FxToolLog("overwriting face graph node named %s", name.GetData());

			FxFaceGraphNode* pNodeToDelete = _actor->GetDecompiledFaceGraph().FindNode(pBonePoseNode->GetName());
			if( pNodeToDelete )
			{
				// Overwriting a non-bone pose node issues a special warning because the new bone pose
				// node can't possibly maintain all of the previous node properties.
				if( !pNodeToDelete->IsA(FxBonePoseNode::StaticClass()) )
				{
					FxToolLog("WARNING face graph node being overwritten is not a bone pose node");
				}

				// Overwrite the node.
				if( !_overwriteFaceGraphNode(pNodeToDelete, pBonePoseNode) )
				{
					FxToolLog("WARNING could not overwrite face graph node");
				}
			}
		}

		// Prune any bone pose nodes.
		_actor->GetMasterBoneList().Prune(_actor->GetDecompiledFaceGraph());
		FxSize numPruned = _actor->GetMasterBoneList().GetNumRefBones() - pBonePoseNode->GetNumBones();
		// Compile - this also "pulls" the bones.
		_actor->CompileFaceGraph();
		

		if( 0 == pBonePoseNode->GetNumBones() )
		{
			if( !_isBatchOp )
			{
				if( _shouldDisplayWarningDialogs )
				{
					FBMessageBox("Warning", "Exported bone pose is identical to the Reference Pose and contains no bones.", "OK");	
				}
			}
			else
			{
				_allBonesPruned = FxTrue;
			}
		}
		FxToolLog("exported bone pose named %s with %d bones removed via pruning", name.GetData(), numPruned);
		return FxTrue;
	}

	return FxFalse;
}

// Batch imports bone poses using the specified configuration file.
FxBool FxToolMB::BatchImportBonePoses( const FxString& filename, 
									    FxInt32& startFrame, FxInt32& endFrame,
										FxBool onlyFindFrameRange )
{
	FxBool result = FxFalse;
	if( _actor )
	{
		FBProgress	lProgress;
		lProgress.Caption	= "Batch Importing Bone Poses.";
		lProgress.Text		= "Please wait...";

		FxToolLog("attempting to batch import bone poses from file %s", filename.GetData());
		// Stop any currently playing animation.
		_stopAnimationPlayback();

		// Batch import the bone poses.
		startFrame = FxInvalidIndex;
		endFrame   = FxInvalidIndex;
		// Import the poses.
		result = Super::BatchImportBonePoses(filename, startFrame, endFrame);
		if( result )
		{
			FxToolLog("batch imported bone poses - range [%d, %d]", startFrame, endFrame);
		}
	}
	return result;
}

// Batch exports bone poses using the specified configuration file.
FxBool FxToolMB::BatchExportBonePoses( const FxString& filename )
{
	FxBool result = FxFalse;
	if( _actor )
	{
		FBProgress	lProgress;
		lProgress.Caption	= "Batch Exporting Bone Poses.";
		lProgress.Text		= "Please wait...";

		FxToolLog("attempting to batch export bone poses from file %s", filename.GetData());
		if( 0 == _actor->GetMasterBoneList().GetNumRefBones() )
		{			
			FBMessageBox("Warning", "Please export a reference pose first.", "OK");					
			return FxFalse;
		}
		result = Super::BatchExportBonePoses(filename);
		if( result )
		{
			FxToolLog("batch exported bone poses");
		}
		if( _allBonesPruned )
		{
			if( _shouldDisplayWarningDialogs )
			{
				FBMessageBox("Warning", "Some exported bone poses are identical to the Reference Pose and contain no bones.\n  Check the log file for details.", "OK");					
			}
			_allBonesPruned = FxFalse;
		}
	}
	return result;
}

//------------------------------------------------------------------------------
// Animation level operations.
//------------------------------------------------------------------------------

// Cleans all animation curves associated with bones in the master bone list.
FxBool FxToolMB::Clean( )
{
	FxBool bAllFound = FxTrue;
	for( FxSize j = 0; j < _actor->GetMasterBoneList().GetNumRefBones(); ++j )
	{
		FxBone bone = _actor->GetMasterBoneList().GetRefBone(j);
		HFBModel model = FBFindModelByName( const_cast<FxChar*>(bone.GetNameAsCstr()) );
		if(model)
		{

			HFBAnimationNode lAnimNode = model->AnimationNode;
			HFBAnimationNode lTransNode = lAnimNode->Nodes.Find("Lcl Translation");
			HFBAnimationNode lRotNode = lAnimNode->Nodes.Find("Lcl Rotation");
			HFBAnimationNode lScaleNode = lAnimNode->Nodes.Find("Lcl Scaling");

			_removeKeys(lTransNode->Nodes.Find("X"));
			_removeKeys(lTransNode->Nodes.Find("Y"));
			_removeKeys(lTransNode->Nodes.Find("Z"));

			_removeKeys(lRotNode->Nodes.Find("X"));
			_removeKeys(lRotNode->Nodes.Find("Y"));
			_removeKeys(lRotNode->Nodes.Find("Z"));

			_removeKeys(lScaleNode->Nodes.Find("X"));
			_removeKeys(lScaleNode->Nodes.Find("Y"));
			_removeKeys(lScaleNode->Nodes.Find("Z"));
		}
		else
		{
			FxToolLog("Warning.  Could not delete keys from bone %s.  Bone not found.", bone.GetNameAsCstr());
			bAllFound = FxFalse;
		}
	}
	return bAllFound;
}

// Import the specified animation at the specified frame rate.
FxBool 
FxToolMB::ImportAnim( const FxString& group, const FxString& name, FxReal fps )
{
	
#ifdef MOD_DEVELOPER_VERSION
	return FxTrue;
#else
	FxToolLog("attempting to import animation %s from group %s", name.GetData(), group.GetData());
	if( _actor )
	{
		FBProgress	lProgress;
		lProgress.Caption	= "Importing animtion.";
		lProgress.Text		= "Please wait...";

		// Stop any currently playing animation.
		_stopAnimationPlayback();
		// Get the animation group.
		FxSize groupIndex = _actor->FindAnimGroup(group.GetData());
		if( FxInvalidIndex != groupIndex )
		{
			FxAnimGroup& animGroup = _actor->GetAnimGroup(groupIndex);
			// Get the animation from the group.
			FxSize animIndex = animGroup.FindAnim(name.GetData());
			if( FxInvalidIndex != animIndex )
			{
				// Remove existing keys.
				Clean();

				// Import the animation.
                FxActorInstance actorInstance;
				actorInstance.SetActor(_actor);
				actorInstance.PlayAnim(name.GetData(), group.GetData());

				FxDReal animStep = 1.0 / static_cast<FxDReal>(fps);
				FxDReal startTime = static_cast<FxDReal>(actorInstance.GetCurrentAnim()->GetStartTime());
				FxDReal endTime = static_cast<FxDReal>(actorInstance.GetCurrentAnim()->GetEndTime());

				
				for( FxDReal i = startTime; i < endTime; i += animStep )
				{
					// Tick the animation.
					actorInstance.Tick(i, -1.0f);
					
					FBTime mbTime;
					mbTime.SetSecondDouble((FxReal)i);

					// Import each bone.
					for( FxSize j = 0; j < _actor->GetMasterBoneList().GetNumRefBones(); ++j )
					{
						FxVec3 pos, scale;
						FxQuat rot;
						FxReal weight;
						actorInstance.GetBone(j, pos, rot, scale, weight);
						FxBone bone = _actor->GetMasterBoneList().GetRefBone(j);
						bone.SetPos(pos);
						bone.SetRot(rot);
						bone.SetScale(scale);
						// Set the bone transform.  For speed, do not remove duplicate keys as there can be none.
						if( !_setBoneTransform(bone, mbTime, FxFalse) )
						{
							FxToolLog("_setBoneTransform failed for bone %s", bone.GetNameAsCstr());
						}
					}
				}
				FxToolLog("imported animation");
				return FxTrue;
			}
			else
			{
				FxToolLog("could not find animation");
			}
		}
		else
		{
			FxToolLog("could not find group");
		}
	}
	return FxFalse;
#endif
}


FxBool FxToolMB::_getBoneTransform( HFBModel model, FxBone& bone, FBTime time )
{
	if(model)
	{
		const char* boneName  = model->Name;
		bone.SetName(boneName);
		// The following code gets the transform correctly at the current time,
		// But can't get it at an arbitrary time.
		//FBVector3d pos;
		//FBVector3d rot;
		//FBVector3d scale;
		//FBQuaternion quat;
		//model->GetVector( pos, kModelTranslation, false);
		//model->GetVector( rot, kModelRotation, false);
		//model->GetVector( scale, kModelScaling, false);
		//FBRotationToQuaternion(quat, rot);
		//bone.SetPos( FxVec3(-pos[0], pos[1], pos[2]));
		//bone.SetRot(FxQuat(-quat[3],-quat[0], quat[1], quat[2]));
		//bone.SetScale(FxVec3(scale[0], scale[1], scale[2]));
		//bone.SetName(boneName);

		HFBAnimationNode lAnimNode = model->AnimationNode;
		HFBAnimationNode lTransNode = lAnimNode->Nodes.Find("Lcl Translation");
		HFBAnimationNode lRotNode = lAnimNode->Nodes.Find("Lcl Rotation");
		HFBAnimationNode lScaleNode = lAnimNode->Nodes.Find("Lcl Scaling");

		HFBAnimationNode lNodeTrans_X		= lTransNode->Nodes.Find("X");
		HFBAnimationNode lNodeTrans_Y		= lTransNode->Nodes.Find("Y");
		HFBAnimationNode lNodeTrans_Z		= lTransNode->Nodes.Find("Z");

		HFBAnimationNode lNodeRot_X		= lRotNode->Nodes.Find("X");
		HFBAnimationNode lNodeRot_Y		= lRotNode->Nodes.Find("Y");
		HFBAnimationNode lNodeRot_Z		= lRotNode->Nodes.Find("Z");

		HFBAnimationNode lNodeScale_X		= lScaleNode->Nodes.Find("X");
		HFBAnimationNode lNodeScale_Y		= lScaleNode->Nodes.Find("Y");
		HFBAnimationNode lNodeScale_Z		= lScaleNode->Nodes.Find("Z");


		FxVec3 bonePosition = FxVec3(_getKeyData(lNodeTrans_X, time),
					_getKeyData(lNodeTrans_Y, time),
					_getKeyData(lNodeTrans_Z, time) );

#ifndef __FX_USE_NATIVE_COORDS__
//	A FaceFX coordinate version would require knowing the source package.  
//  FxMB only works with Native Coordinates.
//	bonePosition.x = -bonePosition.x;
#endif

#ifdef MB_PRE_2009
		// MotionBuilder keys don't take into account the model's "limits"
		FBModelLimits limits(model);
		// Find out the base rotation of the bone in Motion Builder.
		FBVector3d initRot = limits.RotationCenter;
#else
		FBVector3d initRot = model->PreRotation;
#endif

		bone.SetPos(bonePosition);

		FBVector3d rotation = FBVector3d(	_getKeyData(lNodeRot_X, time),
											_getKeyData(lNodeRot_Y, time),
											_getKeyData(lNodeRot_Z, time) );

		FBQuaternion quaternion;
		FBRotationToQuaternion(quaternion, rotation);


		FBQuaternion initFBQuat;
		FBRotationToQuaternion(initFBQuat, initRot);

		FxQuat initFxQuat = _getFxQuat(initFBQuat);
		FxQuat mbKeyQuat = _getFxQuat(quaternion);

		// Add the base rotation to the rotation in the MB keys.
		FxQuat finalQuat = initFxQuat * mbKeyQuat;
		finalQuat.w = -finalQuat.w;
#ifndef __FX_USE_NATIVE_COORDS__
//	A FaceFX coordinate version would require knowing the source package.  
//  FxMB only works with Native Coordinates.
//	finalQuat.x = -finalQuat.x;
#endif
		bone.SetRot(finalQuat);
		bone.SetScale(FxVec3(	_getKeyData(lNodeScale_X, time),
								_getKeyData(lNodeScale_Y, time),
								_getKeyData(lNodeScale_Z, time) ));

		
		return FxTrue;
	}
	FxToolLog("could not find model %s", 	model->Name);
	return FxFalse;
}

FxReal FxToolMB::_getKeyData( HFBAnimationNode Node, FBTime time)
{
	HFBFCurve lCurve = Node->FCurve;
	if( !lCurve )
	{
		lCurve = new FBFCurve;
		Node->FCurve = lCurve;;
	}
	return lCurve->Evaluate(time);
}

void FxToolMB::_copyKeyData( HFBAnimationNode Node, double Data, FBTime time, FxBool removeDuplicateKeys)
{
	HFBFCurve lCurve = NULL;
	lCurve = Node->FCurve;
	if( !lCurve )
	{
		lCurve = new FBFCurve;
	}
	int numKeys = lCurve->Keys.GetCount();
	if(removeDuplicateKeys)
	{
		for( int i = 0; i < numKeys; ++i)
		{
			FBTime keyTime = lCurve->Keys[i].Time;
			if(keyTime.GetMilliSeconds() == time.GetMilliSeconds())
			{
				// Remove conflicting keys.  If we don['t do this,
				// You can get artifacts when inserting bone poses in Take 001.
				lCurve->KeyRemove(i);
				break;
			}
		}
	}
	lCurve->EditBegin();
	lCurve->KeyAdd(time, Data);
	Node->FCurve = lCurve;
	lCurve->EditEnd();
}
FxQuat FxToolMB::_getFxQuat(FBQuaternion fbquat)
{
	return FxQuat(fbquat[3],fbquat[0],fbquat[1],fbquat[2]);
}
FBQuaternion FxToolMB::_getFbQuat(FxQuat fxquat)
{
	return FBQuaternion(fxquat.x, fxquat.y, fxquat.z, fxquat.w);
}
// Sets the specified bone's transform for the current frame.
// Returns FxTrue if the transform was set or FxFalse otherwise.
FxBool FxToolMB::_setBoneTransform( const FxBone& bone, FBTime time, FxBool removeDuplicateKeys )
{
	// MotionBuilder doesn't seem to be const-correct.  
	// Warning:  May be harming small babies...
	HFBModel model = FBFindModelByName( const_cast<FxChar*>(bone.GetNameAsCstr()) );
	if(model)
	{
		//This code appeared to move bones correctly, but did not set keys.
		//FBVector3d pos(bone.GetPos().x, bone.GetPos().y,bone.GetPos().z);
		//FBVector3d rot;
		//FBQuaternion quat(bone.GetRot().x, bone.GetRot().y, bone.GetRot().z, bone.GetRot().w);
		//FBQuaternionToRotation(rot, quat);
		//FBVector3d scale(bone.GetScale().x, bone.GetScale().y, bone.GetScale().z);
		//model->SetVector( pos, kModelTranslation, false);
		//model->SetVector( rot, kModelRotation, false);
		//model->SetVector( scale, kModelScaling, false);
		
		HFBAnimationNode lAnimNode = model->AnimationNode;
		HFBAnimationNode lTransNode = lAnimNode->Nodes.Find("Lcl Translation");
		HFBAnimationNode lRotNode = lAnimNode->Nodes.Find("Lcl Rotation");
		HFBAnimationNode lScaleNode = lAnimNode->Nodes.Find("Lcl Scaling");

		HFBAnimationNode lNodeTrans_X		= lTransNode->Nodes.Find("X");
		HFBAnimationNode lNodeTrans_Y		= lTransNode->Nodes.Find("Y");
		HFBAnimationNode lNodeTrans_Z		= lTransNode->Nodes.Find("Z");

		HFBAnimationNode lNodeRot_X		= lRotNode->Nodes.Find("X");
		HFBAnimationNode lNodeRot_Y		= lRotNode->Nodes.Find("Y");
		HFBAnimationNode lNodeRot_Z		= lRotNode->Nodes.Find("Z");

		HFBAnimationNode lNodeScale_X		= lScaleNode->Nodes.Find("X");
		HFBAnimationNode lNodeScale_Y		= lScaleNode->Nodes.Find("Y");
		HFBAnimationNode lNodeScale_Z		= lScaleNode->Nodes.Find("Z");

#ifdef MB_PRE_2009
		// MotionBuilder keys don't take into account the model's "limits"
		FBModelLimits limits(model);
		// Find out the base rotation of the bone in Motion Builder.
		FBVector3d initRot = limits.RotationCenter;
#else
		FBVector3d initRot = model->PreRotation;
#endif


		FxVec3 bonePosition = bone.GetPos();
#ifndef __FX_USE_NATIVE_COORDS__
//	A FaceFX coordinate version would require knowing the source package.  
//  FxMB only works with Native Coordinates.
//	bonePosition.x = -bonePosition.x;
#endif

		_copyKeyData(lNodeTrans_X, bonePosition.x, time, removeDuplicateKeys);
		_copyKeyData(lNodeTrans_Y, bonePosition.y, time, removeDuplicateKeys);
		_copyKeyData(lNodeTrans_Z, bonePosition.z, time, removeDuplicateKeys);

		FBQuaternion initFBQuat;
		FBRotationToQuaternion(initFBQuat, initRot);

		FxQuat initFxQuat = _getFxQuat(initFBQuat);
		// Get the FXBone's quaternion in MB's coordinate system.
		FxQuat fxboneQuat = FxQuat(-bone.GetRot().w, bone.GetRot().x, bone.GetRot().y, bone.GetRot().z);
#ifndef __FX_USE_NATIVE_COORDS__
//	A FaceFX coordinate version would require knowing the source package.  
//  FxMB only works with Native Coordinates.
//	fxboneQuat.x = -fxboneQuat.x;
#endif
		// remove the limit rotation from the bone prior to setting kesy in MB.
		FxQuat finalQuat = fxboneQuat * initFxQuat.GetInverse();
		FBQuaternion quat = _getFbQuat(finalQuat);

		FBVector3d rot;
		FBQuaternionToRotation(rot, quat);

		_copyKeyData(lNodeRot_X, (rot[0]), time, removeDuplicateKeys);
		_copyKeyData(lNodeRot_Y, (rot[1]), time, removeDuplicateKeys);
		_copyKeyData(lNodeRot_Z, (rot[2]), time, removeDuplicateKeys);
		
		_copyKeyData(lNodeScale_X, (bone.GetScale().x), time, removeDuplicateKeys);
		_copyKeyData(lNodeScale_Y, (bone.GetScale().y), time, removeDuplicateKeys);
		_copyKeyData(lNodeScale_Z, (bone.GetScale().z), time, removeDuplicateKeys);

		return FxTrue;
	}	
	return FxFalse;
}

// Stops any currently playing animation in Max.
void FxToolMB::_stopAnimationPlayback( void )
{
	mPlayerControl.Stop();
}
void FxToolMB::_removeKeys( HFBAnimationNode Node )
{
	HFBFCurve lCurve = Node->FCurve;
	if( lCurve )
	{
		lCurve->Keys.RemoveAll();
	}
}
} // namespace Face

} // namespace OC3Ent
