//------------------------------------------------------------------------------
// An animation curve.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxAnimCurve.h"
#include "FxFaceGraph.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxAnimCurveVersion 1

FX_IMPLEMENT_CLASS(FxAnimCurve, kCurrentFxAnimCurveVersion, FxNamedObject)

FxAnimCurve::FxAnimCurve()
	: FxDataContainer()
	, _cachedPos(FxInvalidIndex)
	, _interp(IT_Hermite)
	, _controlNode(NULL)
{
}

FxAnimCurve::FxAnimCurve( const FxAnimCurve& other )
	: Super(other)
	, FxDataContainer(other)
	, _keys(other._keys)
	, _cachedPos(FxInvalidIndex)
	, _interp(other._interp)
	, _controlNode(other._controlNode)
{
}

FxAnimCurve& FxAnimCurve::operator=( const FxAnimCurve& other )
{
	if( this == &other ) return *this;
	Super::operator=(other);
	FxDataContainer::operator=(other);
	_keys           = other._keys;
	_cachedPos		= FxInvalidIndex;
	_interp			= other._interp;
	_controlNode    = other._controlNode;
	_userData       = other._userData;
	return *this;
}

FxAnimCurve::~FxAnimCurve()
{
}

void FxAnimCurve::SetInterpolator( const FxInterpolationType& interp )
{
	_interp = interp;
}

FxSize FxAnimCurve::InsertKey( const FxAnimKey& key, FxBool autocomputeSlope )
{
	// Sorted insertion based on key time
	// Find the index of the key whose time is greater than the given key's time
	FxSize numKeys = _keys.Length();
	FxSize index = 0;
	for( ; index < numKeys; ++index )
	{
		if( _keys[index].GetTime() > key.GetTime() ) 
		{
			break;
		}
	}

	if( autocomputeSlope && index != 0 && index != numKeys )
	{
		FxAnimKey* prev = &_keys[index-1];
		FxAnimKey* next = &_keys[index];

		// Compute parametric time.
		FxReal deltaTime = (next->GetTime() - prev->GetTime());
		if( deltaTime > 0.0f )
		{
			FxReal t = (key.GetTime() - prev->GetTime()) / deltaTime;
			FxReal s0 = prev->GetSlopeOut() * deltaTime;
			FxReal s1 = next->GetSlopeOut() * deltaTime;
			FxReal slope = (6.0f * prev->GetValue() - 6.0f * next->GetValue() + 3.0f * s0 + 3.0f * s1) * (t * t) + 
				(-6.0f * prev->GetValue() + 6.0f * next->GetValue() - 4.0f * s0 - 2.0f * s1) * t + 
				s0;
			slope /= deltaTime;

			FxAnimKey toInsert(key);
			toInsert.SetSlopeIn(slope);
			toInsert.SetSlopeOut(slope);
			_keys.Insert(toInsert, index);
		}
	}
	else
	{
		_keys.Insert(key, index);
	}
	_cachedPos = FxInvalidIndex;

	return index;
}

void FxAnimCurve::SetKeys( const FxReal* pKeys, FxSize numKeys )
{
	FxAssert(pKeys);
	if( pKeys && numKeys > 0 )
	{
		FxArrayBase<FxAnimKey> arrayBase(numKeys);
		arrayBase._usedCount = numKeys;
		FxSize keyIndex = 0;
		for( FxSize i = 0; i < numKeys; ++i )
		{
			FxReal time     = pKeys[keyIndex++];
			FxReal value    = pKeys[keyIndex++];
			FxReal slopeIn  = pKeys[keyIndex++];
			FxReal slopeOut = pKeys[keyIndex++];
			FxConstructDirect(arrayBase._v + i, FxAnimKey(time, value, slopeIn, slopeOut));
		}
		_keys.SwapArrayBase(arrayBase);
		_cachedPos = FxInvalidIndex;
	}
}

void FxAnimCurve::RemoveKey( FxSize index )
{
	_keys.Remove(index);
	_cachedPos = FxInvalidIndex;
}

void FxAnimCurve::RemoveAllKeys( void )
{
	_keys.Clear();
	_cachedPos = FxInvalidIndex;
}

FxSize FxAnimCurve::FindKey( FxReal time ) const
{
	FxSize retval  = FxInvalidIndex;
	FxSize numKeys = _keys.Length();
	for( FxSize i = 0; i < numKeys; ++i )
	{
		FxReal currTime = _keys[i].GetTime();
		// If we found the key, set the index and break early.
		if( FxRealEquality(currTime, time) )
		{
			retval = i;
			break;
		}
		// If we've passed the place where the key should be, break early.
		if( currTime > time )
		{
			break;
		}
	}
	return retval;
}

FxSize FxAnimCurve::ModifyKey( FxSize index, const FxAnimKey& key )
{
	RemoveKey(index);
	return InsertKey(key);
}

FxReal FxAnimCurve::EvaluateAt( const FxReal time ) const
{
	FxReal value = 0.0f;

	if( _keys.Length() > 0 )
	{
		// Check for out-of-range time and clamp to end points of curve.
		if( time <= _keys.Front().GetTime() )
		{
			value = _keys.Front().GetValue();
		}
		else if( time >= _keys.Back().GetTime() )
		{
			value = _keys.Back().GetValue();
		}
		else
		{
			// The time is in range.
			if( _keys.Length() == 1 )
			{
				value = _keys.Front().GetValue();
			}
			else
			{
				const FxAnimKey* thisKey = NULL;
				const FxAnimKey* nextKey = NULL;
				_findBoundingKeys(time, thisKey, nextKey);

				if( _interp == IT_Hermite )
				{
					value = FxHermiteInterpolate(*thisKey, *nextKey, time);
				}
				else if( _interp == IT_Linear )
				{
					value = FxLinearInterpolate(*thisKey, *nextKey, time);
				}
			}
		}
	}

	// Set the value if we're linked to a node in the face graph.
	if( _controlNode )
	{
		_controlNode->SetTrackValue(value);
	}

	return value;
}

void FxAnimCurve::FindTimeExtents( FxReal& minTime, FxReal& maxTime ) const
{
	// Start with a very large minTime and a very small maxTime.
	minTime = FX_REAL_MAX;
	maxTime = FX_REAL_MIN;

	FxSize numKeys = GetNumKeys();
	if( numKeys > 0 )
	{
		maxTime = GetKey(numKeys-1).GetTime();
		minTime = GetKey(0).GetTime();
	}

	// If no keys were found, clamp minTime and maxTime to reasonable values.
	if( FX_REAL_MAX == minTime && FX_REAL_MIN == maxTime )
	{
		minTime = 0.0f;
		maxTime = 0.0f;
	}
}

void FxAnimCurve::FindValueExtents( FxReal& minValue, FxReal& maxValue ) const
{
	// Start with a very large minValue and a very small maxValue.
	minValue = FX_REAL_MAX;
	maxValue = FX_REAL_MIN;

	FxSize numKeys = GetNumKeys();
	if( numKeys > 0 )
	{
        // Pad the start and end times so we don't miss anything important and 
		// so that the case of only one key is handled gracefully without the 
		// need for another code path.
		FxReal timeStep  = 0.0166666667f; // 60fps.
		FxReal startTime = GetKey(0).GetTime() - (timeStep * 3);
		FxReal endTime   = GetKey(numKeys-1).GetTime() + (timeStep * 3);
		for( FxReal time = startTime; time < endTime; time += timeStep )
		{
			FxReal value = EvaluateAt(time);
			minValue = value < minValue ? value : minValue;
			maxValue = value > maxValue ? value : maxValue;
		}
	}

	// If no values were found, clamp minValue and maxValue to reasonable 
	// values.
	if( FX_REAL_MAX == minValue && FX_REAL_MIN == maxValue )
	{
		minValue = 0.0f;
		maxValue = 0.0f;
	}

	// Make sure that the node's minimum and maximum values are properly
	// taken into account.
	if( _controlNode )
	{
		// We only want to clamp to the node's minimum and maximum values
		// if the curve is a non-offset curve.
		if( 0 == _controlNode->GetNumInputLinks() )
		{
			FxReal nodeMin = _controlNode->GetMin();
			FxReal nodeMax = _controlNode->GetMax();
			minValue = minValue < nodeMin ? nodeMin : minValue;
			minValue = minValue > nodeMax ? nodeMax : minValue;
			maxValue = maxValue > nodeMax ? nodeMax : maxValue;
			maxValue = maxValue < nodeMin ? nodeMin : maxValue;
		}
	}

	// Make a pass through the keys to adjust minValue and maxValue so that
	// no keys become "hidden" in FaceFX Studio.
	for( FxSize i = 0; i < numKeys; ++i )
	{
		FxReal keyValue = GetKey(i).GetValue();
		minValue = minValue > keyValue ? keyValue : minValue;
		maxValue = maxValue < keyValue ? keyValue : maxValue;
	}
}

void FxAnimCurve::Link( const FxFaceGraph* faceGraph )
{
	// Cast away const so that we can get the pointer to the node.
	_controlNode = const_cast<FxFaceGraph*>(faceGraph)->FindNode(GetName());
}

void FxAnimCurve::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxAnimCurve);
	arc << version;

	FxInt32 tempInterp = static_cast<FxInt32>(_interp);
	arc << tempInterp;
	_interp = static_cast<FxInterpolationType>(tempInterp);

	if( arc.IsLoading() )
	{
		if( version < 1 )
		{
			arc << _keys;
		}
	}
}

} // namespace Face

} // namespace OC3Ent
