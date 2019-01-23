//------------------------------------------------------------------------------
// This class implements a generic target node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxGenericTargetNode.h"
#include "FxUtil.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxGenericTargetProxyVersion 0

FX_IMPLEMENT_CLASS(FxGenericTargetProxy, kCurrentFxGenericTargetProxyVersion, FxObject)

FxGenericTargetNode* FxGenericTargetProxy::GetParentGenericTargetNode( void )
{
	return _pParentGenericTargetNode;
}

void FxGenericTargetProxy::
SetParentGenericTargetNode( FxGenericTargetNode* pParentGenericTargetNode )
{
	_pParentGenericTargetNode = pParentGenericTargetNode;
}

FxGenericTargetProxy::FxGenericTargetProxy()
	: _pParentGenericTargetNode(NULL)
{
}

FxGenericTargetProxy::~FxGenericTargetProxy()
{
}

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
		FxGenericTargetNode* pGenericTargetNode = FxCast<FxGenericTargetNode>(pOther);
		if( pGenericTargetNode )
		{
			FxGenericTargetProxy* pOldGenericTargetProxy = GetGenericTargetProxy();
			if( pOldGenericTargetProxy )
			{
				FxGenericTargetProxy* pNewGenericTargetProxy = 
					FxCast<FxGenericTargetProxy>(pOldGenericTargetProxy->GetClassDesc()->ConstructObject());
				pNewGenericTargetProxy->Copy(pOldGenericTargetProxy);
				pNewGenericTargetProxy->SetParentGenericTargetNode(pGenericTargetNode);
				pGenericTargetNode->SetGenericTargetProxy(pNewGenericTargetProxy);
			}
		}
	}
}

FxGenericTargetProxy* FxGenericTargetNode::GetGenericTargetProxy( void )
{
	return _pGenericTargetProxy;
}

void FxGenericTargetNode::
SetGenericTargetProxy( FxGenericTargetProxy* pGenericTargetProxy )
{
	_deleteGenericTargetProxy();
	_pGenericTargetProxy = pGenericTargetProxy;
	_pGenericTargetProxy->SetParentGenericTargetNode(this);
}

FxBool FxGenericTargetNode::ShouldLink( void ) const
{
	return _shouldLink;
}

void FxGenericTargetNode::SetShouldLink( FxBool shouldLink )
{
	_shouldLink = shouldLink;
}

void FxGenericTargetNode::UpdateTarget( void )
{
	if( _pGenericTargetProxy )
	{
		_pGenericTargetProxy->Update(Super::GetValue());
	}
}

void FxGenericTargetNode::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxGenericTargetNode);
	arc << version;
}

FxGenericTargetNode::FxGenericTargetNode()
	: _pGenericTargetProxy(NULL)
	, _shouldLink(FxTrue)
{
	_isPlaceable = FxFalse;
}

FxGenericTargetNode::~FxGenericTargetNode()
{
	_deleteGenericTargetProxy();
}

void FxGenericTargetNode::_deleteGenericTargetProxy( void )
{
	if( _pGenericTargetProxy )
	{
		const FxSize size = _pGenericTargetProxy->GetClassDesc()->GetSize();
		_pGenericTargetProxy->Destroy();
		FxFree(_pGenericTargetProxy, size);
		_pGenericTargetProxy = NULL;
	}
}

} // namespace Face

} // namespace OC3Ent
