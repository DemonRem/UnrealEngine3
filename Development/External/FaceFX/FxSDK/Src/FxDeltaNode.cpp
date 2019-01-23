//------------------------------------------------------------------------------
// This class implements a delta node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxDeltaNode.h"
#include "FxMath.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxDeltaNodeVersion 0

FX_IMPLEMENT_CLASS(FxDeltaNode, kCurrentFxDeltaNodeVersion, FxFaceGraphNode)

FxDeltaNode::FxDeltaNode()
	: _previousFirstInputValue(FxInvalidValue)
{
}

FxDeltaNode::~FxDeltaNode()
{
}

FxFaceGraphNode* FxDeltaNode::Clone( void )
{
	FxDeltaNode* pNode = new FxDeltaNode();
	CopyData(pNode);
	return pNode;
}

void FxDeltaNode::CopyData( FxFaceGraphNode* pOther )
{
	if( pOther )
	{
		Super::CopyData(pOther);
	}
}

FxReal FxDeltaNode::GetValue( void )
{
	if( FxInvalidValue == _value )
	{
		if( _inputs.Length() > 0 )
		{
			FxReal currentValue = _inputs[0].GetTransformedValue();
			if( FxInvalidValue != _previousFirstInputValue )
			{
				_value = currentValue - _previousFirstInputValue;
			}
			else
			{
				_value = 0.0f;
			}
			_previousFirstInputValue = currentValue;
		}
	}
	// If the value is still invalid value, clamp it to zero.
	if( FxInvalidValue == _value )
	{
		_value = 0.0f;
	}
	return _value;
}

FxBool FxDeltaNode::AddInputLink( const FxFaceGraphNodeLink& newInputLink )
{
	if( _inputs.Length() == 0 )
	{
		Super::AddInputLink(newInputLink);
		const FxFaceGraphNode* pNode = _inputs[0].GetNode();
		if( pNode )
		{
			FxReal difference =  pNode->GetMax() - pNode->GetMin();
			SetMax(difference);
			SetMin(-difference);
		}
		return FxTrue;
	}
	return FxFalse;
}

void FxDeltaNode::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxDeltaNode);
	arc << version;
}
	
} // namespace Face

} // namespace OC3Ent
