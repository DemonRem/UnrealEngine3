//------------------------------------------------------------------------------
// This class implements a current time node in the face graph.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxCurrentTimeNode.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxCurrentTimeNodeVersion 0

FX_IMPLEMENT_CLASS(FxCurrentTimeNode, kCurrentFxCurrentTimeNodeVersion, FxFaceGraphNode)

FxCurrentTimeNode::FxCurrentTimeNode()
{
}

FxCurrentTimeNode::~FxCurrentTimeNode()
{
}

FxFaceGraphNode* FxCurrentTimeNode::Clone( void )
{
	FxCurrentTimeNode* pNode = new FxCurrentTimeNode();
	CopyData(pNode);
	return pNode;
}

void FxCurrentTimeNode::CopyData( FxFaceGraphNode* pOther )
{
	if( pOther )
	{
		Super::CopyData(pOther);
	}
}

FxReal FxCurrentTimeNode::GetValue( void )
{
	if( FxInvalidValue == _value )
	{
		// Set the value of the node.
		_value = _trackValue;

		// Operate with the user-defined value.
		if( _userValue != FxInvalidValue )
		{
			switch( _userValueOperation )
			{
			case VO_Multiply:
				_value *= _userValue;
				break;
			case VO_Add:
				_value += _userValue;
				break;
			case VO_Replace:
				_value = _userValue;
				break;
			default:
				FxAssert( !"Unknown value operation" );
			}
		}
	}
	// If the value is still invalid value, clamp it to zero.
	if( FxInvalidValue == _value )
	{
		_value = 0.0f;
	}
	return _value;
}

FxBool FxCurrentTimeNode::AddInputLink( const FxFaceGraphNodeLink& /*UNUSED - newInputLink*/ )
{
	return FxFalse;
}

void FxCurrentTimeNode::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxCurrentTimeNode);
	arc << version;
}

} // namespace Face

} // namespace OC3Ent
