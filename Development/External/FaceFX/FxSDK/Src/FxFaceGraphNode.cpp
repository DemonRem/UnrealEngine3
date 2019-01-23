//------------------------------------------------------------------------------
// This abstract class implements a node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxFaceGraphNode.h"
#include "FxMath.h"
#include "FxUtil.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxFaceGraphNodeUserPropertyVersion 0

FX_IMPLEMENT_CLASS(FxFaceGraphNodeUserProperty, kCurrentFxFaceGraphNodeUserPropertyVersion, FxNamedObject)

FxFaceGraphNodeUserProperty::
FxFaceGraphNodeUserProperty( FxFaceGraphNodeUserPropertyType propertyType )
	: _propertyType(propertyType)
	, _integerProperty(0)
	, _realProperty(0.0f)
{
	if( UPT_String == _propertyType )
	{
		_choices.Reserve(1);
		_choices.PushBack(FxString());
	}
}

FxFaceGraphNodeUserProperty::
FxFaceGraphNodeUserProperty( const FxFaceGraphNodeUserProperty& other )
	: Super(other)
	, _propertyType(other._propertyType)
	, _integerProperty(other._integerProperty)
	, _realProperty(other._realProperty)
	, _choices(other._choices)
{
}

FxFaceGraphNodeUserProperty& 
FxFaceGraphNodeUserProperty::operator=( const FxFaceGraphNodeUserProperty& other )
{
	if( this == &other ) return *this;
	Super::operator=(other);
	_propertyType	 = other._propertyType;
	_integerProperty = other._integerProperty;
	_realProperty	 = other._realProperty;
	_choices		 = other._choices;
	return *this;
}

FxFaceGraphNodeUserProperty::~FxFaceGraphNodeUserProperty()
{
}

FxFaceGraphNodeUserPropertyType 
FxFaceGraphNodeUserProperty::GetPropertyType( void ) const
{
	return _propertyType;
}

FxInt32 FxFaceGraphNodeUserProperty::GetIntegerProperty( void ) const
{
	return _integerProperty;
}

void FxFaceGraphNodeUserProperty::SetIntegerProperty( FxInt32 integerProperty )
{
	if( UPT_Integer == _propertyType )
	{
		_integerProperty = integerProperty;
	}
}

FxBool FxFaceGraphNodeUserProperty::GetBoolProperty( void ) const
{
	if( 0 == _integerProperty )
	{
		return FxFalse;
	}
	return FxTrue;
}

void FxFaceGraphNodeUserProperty::SetBoolProperty( FxBool boolProperty )
{
	if( FxFalse == boolProperty )
	{
		_integerProperty = 0;
	}
	else
	{
		_integerProperty = 1;
	}
}

FxReal FxFaceGraphNodeUserProperty::GetRealProperty( void ) const
{
	return _realProperty;
}

void FxFaceGraphNodeUserProperty::SetRealProperty( FxReal realProperty )
{
	if( UPT_Real == _propertyType )
	{
		_realProperty = realProperty;
	}
}

const FxString& FxFaceGraphNodeUserProperty::GetStringProperty( void ) const
{
		return _choices[0];
}

void FxFaceGraphNodeUserProperty::SetStringProperty( const FxString& stringProperty )
{
	if( UPT_String == _propertyType )
	{
		_choices[0] = stringProperty;
	}
}

FxString FxFaceGraphNodeUserProperty::GetChoiceProperty( void ) const
{
	return _choices[_integerProperty];
}

void FxFaceGraphNodeUserProperty::SetChoiceProperty( const FxString& choiceProperty )
{
	if( UPT_Choice == _propertyType )
	{
		_integerProperty = 0;
		FxSize numChoices = _choices.Length();
		for( FxSize i = 0; i < numChoices; ++i )
		{
			if( _choices[i] == choiceProperty )
			{
				_integerProperty = static_cast<FxInt32>(i);
			}
		}
	}
}

FxSize FxFaceGraphNodeUserProperty::GetNumChoices( void ) const
{
	return _choices.Length();
}

const FxString& FxFaceGraphNodeUserProperty::GetChoice( FxSize index ) const
{
	return _choices[index];
}

void FxFaceGraphNodeUserProperty::AddChoice( const FxString& choice )
{
	_choices.PushBack(choice);
}

void FxFaceGraphNodeUserProperty::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxFaceGraphNodeUserProperty);
	arc << version;

	FxInt32 tempPropertyType = static_cast<FxInt32>(_propertyType);
	arc << tempPropertyType << _integerProperty << _realProperty << _choices;
	_propertyType = static_cast<FxFaceGraphNodeUserPropertyType>(tempPropertyType);
}

#define kCurrentFxFaceGraphNodeVersion 2

FX_IMPLEMENT_CLASS(FxFaceGraphNode, kCurrentFxFaceGraphNodeVersion, FxNamedObject)

FxFaceGraphNode::FxFaceGraphNode()
	: FxDataContainer()
	, _value(FxInvalidValue)
	, _trackValue(FxInvalidValue)
	, _min(0.0f)
	, _max(1.0f)
	, _userValue( FxInvalidValue )
	, _inputOperation(IO_Sum)
	, _isPlaceable(FxTrue)
{
}

FxFaceGraphNode::~FxFaceGraphNode()
{
}

void FxFaceGraphNode::CopyData( FxFaceGraphNode* pOther )
{
	if( pOther )
	{
		pOther->SetName(GetName());
		pOther->SetMin(GetMin());
		pOther->SetMax(GetMax());
		pOther->SetNodeOperation(GetNodeOperation());
		FxSize numUserProperties = GetNumUserProperties();
		for( FxSize i = 0; i < numUserProperties; ++i )
		{
			pOther->ReplaceUserProperty(GetUserProperty(i));
		}
	}
}

FxReal FxFaceGraphNode::GetValue( void )
{
	if( _value == FxInvalidValue )
	{
		FxSize numInputs = _inputs.Length();
		if( numInputs )
		{
			// Initialize value.  Use 0 as a starting point for summing and 1 
			// as a starting point for multiplying.
			FxReal value = (_inputOperation == IO_Sum) ? 0.0f : 1.0f;
			FxReal correction = FX_REAL_MAX;

			// For each input, apply its influence on the final node value based
			// on the node's input operation.  If the input link was corrective,
			// special case logic must be applied.
			for( FxSize i = 0; i < numInputs; ++i )
			{
				if( !_inputs[i].IsCorrective() )
				{
					if( _inputOperation == IO_Sum )
					{
						value += _inputs[i].GetTransformedValue();
					}
					else if( _inputOperation == IO_Multiply )
					{
						value *= _inputs[i].GetTransformedValue();
					}
					else
					{
						FxAssert( !"Unknown input operation" );
					}
				}
				else
				{
					correction = _inputs[i].GetCorrectiveValue(correction);
				}
			}

			// Add in the offset track value.
			if( _trackValue != FxInvalidValue )
			{
				value += _trackValue;
			}

			// Operate with the user-defined value.
			if( _userValue != FxInvalidValue )
			{
				switch( _userValueOperation )
				{
				case VO_Multiply:
					value *= _userValue;
					break;
				case VO_Add:
					value += _userValue;
					break;
				case VO_Replace:
					value = _userValue;
					break;
				default:
					FxAssert( !"Unknown value operation" );
				}
			}

			// Correct the value, if there were corrective links.
			if( correction != FX_REAL_MAX )
			{
				value *= (1.0f - correction);
			}

			_value = value;
		}
		else
		{
			if( _trackValue != FxInvalidValue )
			{
				_value = _trackValue;
			}

			// Operate with the user-defined value.
			if( _userValue != FxInvalidValue )
			{
				if( _value == FxInvalidValue )
				{
					// Initialize value.  Use 0 as a starting point for summing and 1 
					// as a starting point for multiplying.
					_value = (_userValueOperation == VO_Add) ? 0.0f : 1.0f;
				}
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

		// Clamp the value.
		if( _value == FxInvalidValue )
		{
			_value = 0.0f;
		}
		else
		{
			_value = FxClamp(_min, _value, _max);
		}
	}
	
	return _value;
}

void FxFaceGraphNode::SetMin( FxReal iMin )
{
	_min = iMin;
}

void FxFaceGraphNode::SetMax( FxReal iMax )
{
	_max = iMax;
}

void FxFaceGraphNode::SetNodeOperation( FxInputOp iNodeOperation )
{
	_inputOperation = iNodeOperation;
}

void FxFaceGraphNode::SetTrackValue( FxReal trackValue )
{
	_trackValue = trackValue;
}

void FxFaceGraphNode::SetUserValue( FxReal userValue, FxValueOp userValueOp )
{
	_userValue = userValue;
	_userValueOperation = userValueOp;
}

FxBool FxFaceGraphNode::AddInputLink( const FxFaceGraphNodeLink& newInputLink )
{
	_inputs.PushBack(newInputLink);
	return FxTrue;
}

void FxFaceGraphNode::ModifyInputLink( FxSize index, 
									   const FxFaceGraphNodeLink& inputLink )
{
	FxAssert( index < _inputs.Length() );
	_inputs[index] = inputLink;
}

void FxFaceGraphNode::RemoveInputLink( FxSize index )
{
	_inputs.Remove(index);
}

void FxFaceGraphNode::AddOutput( FxFaceGraphNode* pOutput )
{
	_outputs.PushBack(pOutput);
}

FxBool FxFaceGraphNode::IsPlaceable( void ) const
{
	return _isPlaceable;
}

void FxFaceGraphNode::AddUserProperty( const FxFaceGraphNodeUserProperty& userProperty )
{
	if( FxInvalidIndex == FindUserProperty(userProperty.GetName()) )
	{
		_userProperties.PushBack(userProperty);
	}
}

FxSize FxFaceGraphNode::GetNumUserProperties( void ) const
{
	return _userProperties.Length();
}

FxSize FxFaceGraphNode::FindUserProperty( const FxName& userPropertyName ) const
{
	FxSize numUserProperties = _userProperties.Length();	
	for( FxSize i = 0; i < numUserProperties; ++i )
	{
		if( _userProperties[i].GetName() == userPropertyName )
		{
			return i;
		}
	}
	return FxInvalidIndex;
}

const FxFaceGraphNodeUserProperty& 
FxFaceGraphNode::GetUserProperty( FxSize index ) const
{
	return _userProperties[index];
}

void FxFaceGraphNode::
ReplaceUserProperty( const FxFaceGraphNodeUserProperty& userProperty )
{
	FxSize index = FindUserProperty(userProperty.GetName());
	if( FxInvalidIndex != index )
	{
		_userProperties[index] = userProperty;
	}
}

void FxFaceGraphNode::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxFaceGraphNode);
	arc << version;

	FxInt32 tempNodeOp = static_cast<FxInt32>(_inputOperation);
	arc << _min << _max << tempNodeOp << _inputs;
	_inputOperation = static_cast<FxInputOp>(tempNodeOp);

	if( version >= 1 )
	{
		arc << _userProperties;
	}
	if( version >= 2 )
	{
		FxSize numOutputs = _outputs.Length();
		arc << numOutputs;
		if( arc.IsLoading() )
		{
			_outputs.Reserve(numOutputs);
		}
	}
}

// A "visited" flag used in _hasCycles.
static const FxReal FxNodeVisitedMarker = static_cast<FxReal>(FX_INT32_MAX);

FxBool FxFaceGraphNode::_hasCycles( void ) const
{
	if( FxRealEquality(_value, FxNodeVisitedMarker) )
	{
		return FxTrue;
	}
	else
	{
		_value = FxNodeVisitedMarker;
		FxBool inputHasCycles = FxFalse;
		FxSize numInputs = _inputs.Length();
		for( FxSize i = 0; i < numInputs; ++i )
		{
			inputHasCycles |= _inputs[i].GetNode()->_hasCycles();
		}
		_value = FxInvalidValue;
		return inputHasCycles;
	}
}

} // namespace Face

} // namespace OC3Ent
