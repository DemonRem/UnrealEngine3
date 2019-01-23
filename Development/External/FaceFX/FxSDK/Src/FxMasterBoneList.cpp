//------------------------------------------------------------------------------
// This class defines the master bone list for an FxActor.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxMasterBoneList.h"
#include "FxUtil.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxMasterBoneListVersion 1

FxMasterBoneList::FxMasterBoneListEntry::
FxBoneLink::FxBoneLink()
	: valueIndex(FxInvalidIndex)
{
}

FxMasterBoneList::FxMasterBoneListEntry::
FxBoneLink::FxBoneLink( FxSize iValueIndex, 
					    const FxName& optimizedBoneName,
					    const FxVec3& optimizedBonePos, 
					    const FxQuat& optimizedBoneRot, 
					    const FxVec3& optimizedBoneScale )
	: valueIndex(iValueIndex)
	, optimizedBone(optimizedBoneName, optimizedBonePos, optimizedBoneRot, optimizedBoneScale)
{
}

FxMasterBoneList::FxMasterBoneListEntry::
FxBoneLink::~FxBoneLink()
{
}

FxMasterBoneList::
FxMasterBoneListEntry::FxMasterBoneListEntry()
	: index(FX_INT32_MAX)
	, refWeight(1.0f)
	, weight(1.0f)
{
}

FxMasterBoneList::
FxMasterBoneListEntry::FxMasterBoneListEntry( const FxBone& bone )
	: refBone(bone)
	, refBoneInverseRot(bone.GetRot().GetInverse())
	, index(FX_INT32_MAX)
	, refWeight(1.0f)
	, weight(1.0f)
{
}

FxMasterBoneList::
FxMasterBoneListEntry::~FxMasterBoneListEntry()
{
}

void FxMasterBoneList::
FxMasterBoneListEntry::Link( FxFaceGraph& faceGraph )
{
	// Clear out the links and reserve some space.  This used to be a two phase
	// algorithm which counted the number of links required and allocated
	// only the exact number of links but that proved to be too costly in terms
	// of execution time.
	links.Clear();
	//@todo Set this as a constant somewhere?  This will probably go away with
	//      a future face graph revision to reduce run-time cost of face graph
	//      iteration and evaluation.  For most content this should be a good
	//      estimate.
	links.Reserve(32);
	// Find the reference bone in any of the bone pose nodes and create the 
	// links.  
	FxFaceGraph::Iterator bonePoseNodeIter    = faceGraph.Begin(FxBonePoseNode::StaticClass());
	FxFaceGraph::Iterator bonePoseNodeEndIter = faceGraph.End(FxBonePoseNode::StaticClass());
	FxSize bonePoseNodeIndex = 0;
	for( ; bonePoseNodeIter != bonePoseNodeEndIter; ++bonePoseNodeIter, ++bonePoseNodeIndex )
	{
		FxBonePoseNode* pBonePoseNode = static_cast<FxBonePoseNode*>(bonePoseNodeIter.GetNode());
		if( pBonePoseNode )
		{
			FxSize numBones = pBonePoseNode->GetNumBones();
			for( FxSize i = 0; i < numBones; ++i )
			{
				const FxBone& poseBone = pBonePoseNode->GetBone(i);
				if( poseBone.GetName() == refBone.GetName() )
				{
					links.PushBack(FxBoneLink(bonePoseNodeIndex, poseBone.GetName(), 
						           (poseBone.GetPos() - refBone.GetPos()), poseBone.GetRot(), 
						           (poseBone.GetScale() - refBone.GetScale())));
				}
			}
		}
	}
}

FX_INLINE void FxMasterBoneList::
FxMasterBoneListEntry::Prune( FxFaceGraph& faceGraph )
{
	// Find the reference bone in any of the bone pose nodes and prune if required.
	FxFaceGraph::Iterator bonePoseNodeIter = faceGraph.Begin(FxBonePoseNode::StaticClass());
	FxFaceGraph::Iterator faceGraphEndIter = faceGraph.End(FxBonePoseNode::StaticClass());
	for( ; bonePoseNodeIter != faceGraphEndIter; ++bonePoseNodeIter )
	{
		FxBonePoseNode* pBonePoseNode = static_cast<FxBonePoseNode*>(bonePoseNodeIter.GetNode());
		if( pBonePoseNode )
		{
			for( FxSize i = 0; i < pBonePoseNode->GetNumBones(); ++i )
			{
				const FxBone& poseBone = pBonePoseNode->GetBone(i);
				if( poseBone.GetName() == refBone.GetName() )
				{
					if( poseBone == refBone )
					{
						pBonePoseNode->RemoveBone(i);
						i--;
					}
				}
			}
		}
	}
}

FxMasterBoneList::FxMasterBoneList()
{
}

FxMasterBoneList::~FxMasterBoneList()
{
}

void FxMasterBoneList::Clear( void )
{
	_refBones.Clear();
}

void FxMasterBoneList::AddRefBone( const FxBone& bone )
{
	// If the bone is already in the list, simply update it.
	FxBone temp(bone);
	FxQuat rot(bone.GetRot());
	rot.Normalize();
	temp.SetRot(rot);
	FxBool alreadyInList = FxFalse;
	FxSize numRefBones = _refBones.Length();
	for( FxSize i = 0; i < numRefBones; ++i )
	{
		if( _refBones[i].refBone.GetName() == temp.GetName() )
		{
			_refBones[i].refBone = temp;
			alreadyInList = FxTrue;
		}
	}
	// Otherwise add it to the list.
	if( !alreadyInList )
	{
		FxMasterBoneListEntry newEntry(temp);
		_refBones.PushBack(newEntry);
	}
}

void FxMasterBoneList::
SetRefBoneClientIndex( FxSize refBoneIndex, FxInt32 clientIndex )
{
	_refBones[refBoneIndex].index = clientIndex;
}

void FxMasterBoneList::Link( FxFaceGraph& faceGraph )
{
	// Reserve enough room in _bonePoseNodeValues for all FxBonePoseNodes in
	// faceGraph.
	_bonePoseNodeValues.Clear();
	FxSize numBonePoseNodes = faceGraph.CountNodes(FxBonePoseNode::StaticClass());
	_bonePoseNodeValues.Reserve(numBonePoseNodes);
	for( FxSize i = 0; i < numBonePoseNodes; ++i )
	{
		_bonePoseNodeValues.PushBack(FxInvalidValue);
	}
	FxSize numRefBones = _refBones.Length();
	for( FxSize i = 0; i < numRefBones; ++i )
	{
		_refBones[i].Link(faceGraph);
	}
}

void FxMasterBoneList::Prune( FxFaceGraph& faceGraph )
{
	// Clean out any bones from all bone pose nodes that are not in the master
	// bone list.
	FxSize numRefBones = _refBones.Length();
	FxFaceGraph::Iterator bonePoseNodeIter = faceGraph.Begin(FxBonePoseNode::StaticClass());
	FxFaceGraph::Iterator faceGraphEndIter = faceGraph.End(FxBonePoseNode::StaticClass());
	for( ; bonePoseNodeIter != faceGraphEndIter; ++bonePoseNodeIter )
	{
		FxBonePoseNode* pBonePoseNode = FxCast<FxBonePoseNode>(bonePoseNodeIter.GetNode());
		if( pBonePoseNode )
		{
			for( FxSize i = 0; i < pBonePoseNode->GetNumBones(); ++i )
			{
				const FxBone& poseBone = pBonePoseNode->GetBone(i);
				FxBool foundBone = FxFalse;
				for( FxSize j = 0; j < numRefBones; ++j )
				{
					if( poseBone.GetName() == _refBones[j].refBone.GetName() )
					{
						foundBone = FxTrue;
					}
				}
				if( !foundBone )
				{
					pBonePoseNode->RemoveBone(i);
					i--;
				}
			}
		}
	}
	for( FxSize i = 0; i < numRefBones; ++i )
	{
		_refBones[i].Prune(faceGraph);
	}
}

FxArchive& operator<<( FxArchive& arc, FxMasterBoneList& mbl )
{
	if( arc.IsLoading() )
	{
		mbl._refBones.Clear();
	}

	FxUInt16 version = kCurrentFxMasterBoneListVersion;
	arc << version;

	FxSize length = mbl._refBones.Length();
	arc << length;

	if( arc.IsSaving() )
	{
		for( FxSize i = 0; i < length; ++i )
		{
			arc << mbl._refBones[i].refBone;
			arc << mbl._refBones[i].index;
			arc << mbl._refBones[i].refWeight;
		}
	}
	else
	{
		mbl._refBones.Reserve(length);
		for( FxSize i = 0; i < length; ++i )
		{
			FxBone bone;
			arc << bone;
			FxMasterBoneList::FxMasterBoneListEntry newEntry(bone);
			FxInt32 index;
			arc << index;
			newEntry.index = index;
			if( version >= 1 )
			{
				FxReal refWeight;
				arc << refWeight;
				newEntry.refWeight = refWeight;
			}
			mbl._refBones.PushBack(newEntry);
		}
	}

	return arc;
}

} // namespace Face

} // namespace OC3Ent
