//------------------------------------------------------------------------------
// This class defines a link between two nodes in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxFaceGraphNodeLink.h"
#include "FxUtil.h"
#include "FxLinkFn.h"
#include "FxFaceGraphNode.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxFaceGraphNodeLinkVersion 0

FX_IMPLEMENT_CLASS(FxFaceGraphNodeLink, kCurrentFxFaceGraphNodeLinkVersion, FxObject)

FxFaceGraphNodeLink::FxFaceGraphNodeLink()
	: FxDataContainer()
	, _inputName(FxName::NullName)
	, _input(NULL)
	, _linkFnName(FxName::NullName)
	, _linkFn(NULL)
{
}

FxFaceGraphNodeLink::FxFaceGraphNodeLink( const FxName& inputName, 
	FxFaceGraphNode* inputNode, const FxName& linkFunctionName, 
	const FxLinkFn* linkFunction, const FxLinkFnParameters& linkFunctionParams )
	: FxDataContainer()
	, _inputName(inputName)
	, _input(inputNode)
	, _linkFnName(linkFunctionName)
	, _linkFn(linkFunction)
	, _linkFnParams(linkFunctionParams)
{
}

FxFaceGraphNodeLink::FxFaceGraphNodeLink( const FxFaceGraphNodeLink& other )
	: Super(other)
	, FxDataContainer(other)
	, _inputName(other._inputName)
	, _input(other._input)
	, _linkFnName(other._linkFnName)
	, _linkFn(other._linkFn)
	, _linkFnParams(other._linkFnParams)
{
}

FxFaceGraphNodeLink& FxFaceGraphNodeLink::operator=( const FxFaceGraphNodeLink& other )
{	
	if( this == &other ) return *this;
	Super::operator=(other);
	FxDataContainer::operator=(other);
	_inputName    = other._inputName;
	_input        = other._input;
	_linkFnName   = other._linkFnName;
	_linkFn       = other._linkFn;
	_linkFnParams = other._linkFnParams;
	return *this;
}

FxFaceGraphNodeLink::~FxFaceGraphNodeLink()
{
}

FxReal FxFaceGraphNodeLink::GetTransformedValue( void ) const
{
	FxAssert(_input);
	FxAssert(_linkFn);
	return _linkFn->Evaluate(_input->GetValue(), _linkFnParams);
}

FxReal FxFaceGraphNodeLink::GetCorrectiveValue( FxReal previousValue ) const
{
	FxAssert(_input);
	if( previousValue == FX_REAL_MAX )
	{
		previousValue = 0.0f;
	}
	FxReal correction = _input->GetValue();
	if( correction > 0.0f )
	{
		correction = correction / _input->GetMax();
	}
	else if( correction < 0.0f )
	{
		correction = (-correction) / _input->GetMin();
	}
	else
	{
		correction = 0.0f;
	}
	return FxMin(1.0f, previousValue + (correction * GetCorrectionFactor()));
}

void FxFaceGraphNodeLink::SetNode( FxFaceGraphNode* inputNode )
{
	FxAssert(inputNode);
	if( inputNode )
	{
		_input     = inputNode;
		_inputName = _input->GetName();
	}
	else
	{
		_inputName = FxName::NullName;
	}
}

FxName FxFaceGraphNodeLink::GetLinkFnName( void ) const 
{ 
	return _linkFnName; 
}

void FxFaceGraphNodeLink::SetLinkFnName( const FxName& linkFn )
{
	_linkFnName = linkFn;
	_linkFn     = FxLinkFn::FindLinkFunction(_linkFnName);
}

void FxFaceGraphNodeLink::SetLinkFn( const FxLinkFn* linkFn )
{
	FxAssert(linkFn);
	if( linkFn )
	{
		_linkFn     = linkFn;
		_linkFnName = _linkFn->GetName();
	}
	else
	{
		_linkFnName = FxName::NullName;
	}
}

FxLinkFnParameters FxFaceGraphNodeLink::GetLinkFnParams( void ) const 
{ 
	return _linkFnParams; 
}

void FxFaceGraphNodeLink::SetLinkFnParams( const FxLinkFnParameters& linkFnParams )
{
	_linkFnParams = linkFnParams;
}

void FxFaceGraphNodeLink::SetCorrectionFactor( FxReal newValue )
{
	FxAssert(_linkFn);
	if( _linkFnParams.parameters.Length() < 1 )
	{
		_linkFnParams.parameters.PushBack(newValue);
	}
	else
	{
		_linkFnParams.parameters[0] = newValue;
	}
}

void FxFaceGraphNodeLink::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxFaceGraphNodeLink);
	arc << version;

	arc << _inputName << _linkFnName << _linkFnParams;
	
	if( arc.IsLoading() )
	{
		_linkFn = FxLinkFn::FindLinkFunction(_linkFnName);
		FxAssert(_linkFn);
	}
}

} // namespace Face

} // namespace OC3Ent
