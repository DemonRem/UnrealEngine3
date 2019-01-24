//------------------------------------------------------------------------------
// This class implements a generic target node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxGenericTargetNode.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxGenericTargetNodeVersion 0

FX_IMPLEMENT_CLASS(FxGenericTargetNode, kCurrentFxGenericTargetNodeVersion, FxFaceGraphNode)

FxFaceGraphNode* FxGenericTargetNode::Clone( void )
{
	FxGenericTargetNode* pNode = new FxGenericTargetNode();
	CopyData(pNode);
	return pNode;
}

void FxGenericTargetNode::CopyData( FxFaceGraphNode* pOther )
{
	if( pOther )
	{
		Super::CopyData(pOther);
	}
}

void FxGenericTargetNode::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	arc.SerializeClassVersion("FxGenericTargetNode");
}

FxGenericTargetNode::FxGenericTargetNode()
{}

FxGenericTargetNode::~FxGenericTargetNode()
{}

} // namespace Face

} // namespace OC3Ent
