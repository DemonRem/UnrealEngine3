//------------------------------------------------------------------------------
// A pseudorandom number generator.
// 
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxRandomGenerator.h"
#include "FxMath.h"

namespace OC3Ent
{

namespace Face
{

FxRange::FxRange( FxReal iMin, FxReal iMax )
	: min(iMin)
	, max(iMax)
{
}

FxCentredRange::FxCentredRange( FxReal iCentre, FxReal iRange )
	: centre(iCentre)
	, range(iRange)
{
}


FxRandomGenerator::FxRandomGenerator( FxInt32 seed )
	: _idum(seed)
{
}

void FxRandomGenerator::ResetSeed( FxInt32 seed )
{
	_idum = seed;
}

// A random integer in [rangeMin, rangeMax]
FxInt32 FxRandomGenerator::Int32InRange( FxInt32 rangeMin, FxInt32 rangeMax )
{
	FxReal random = ran0();

	// Pad the range to ensure the exterior elements have the same probability
	// as the interior elements.
	FxReal left  = rangeMin - 0.5f;
	FxReal right = rangeMax + 0.5f;

	FxReal res = random * (right - left) + left;
	FxInt32 retval = FxNearestInteger(res);

	return FxClamp(rangeMin, retval, rangeMax);
}

// A random integer in [centre-halfRange, centre+halfRange]
FxInt32 FxRandomGenerator::Int32InCentredRange( FxInt32 centre, FxInt32 halfRange )
{
	FxReal random = ran0();

	// Pad the range to ensure the exterior elements have the same probability
	// as the interior elements.
	FxReal left  = centre - halfRange - 0.5f;
	FxReal right = centre + halfRange + 0.5f;

	FxReal res = random * (right - left) + left;
	FxInt32 retval = FxNearestInteger(res);

	return FxClamp(centre - halfRange, retval, centre + halfRange);
}

// A random real in [rangeMin, rangeMax]
FxReal FxRandomGenerator::RealInRange( FxReal rangeMin, FxReal rangeMax )
{
	return FxClamp(rangeMin, ran0() * (rangeMax - rangeMin) + rangeMin, rangeMax);
}

// A random real in the range.
FxReal FxRandomGenerator::RealInRange( const FxRange& range )
{
	return RealInRange(range.min, range.max);
}

// A random real in [centre-halfRange, centre+halfRange]
FxReal FxRandomGenerator::RealInCentredRange( FxReal centre, FxReal halfRange )
{
	FxReal left  = centre - halfRange;
	FxReal right = centre + halfRange;

	return FxClamp(left, ran0() * (right - left) + left, right);
}

// A random real in the centred range.
FxReal FxRandomGenerator::RealInCentredRange( const FxCentredRange& centredRange )
{
	return RealInCentredRange(centredRange.centre, centredRange.range);
}

// A random bool with a given probability of being true.
FxBool FxRandomGenerator::Bool( FxReal trueProbability )
{
	return ran0() < trueProbability;
}

// A random index in a weighted array.
FxSize FxRandomGenerator::Index( const FxArray<FxReal>& weights )
{
	FxSize retval = FxInvalidIndex;
	FxSize numWeights = weights.Length();

	FxReal invSum = 0.0f;
	for( FxSize i = 0; i < numWeights; ++i )
	{
		invSum += weights[i];
	}
	invSum = (1.0f / invSum);

	FxReal randomWeight = ran0();

	FxReal left = 0.0f;
	for( FxSize i = 0; i < numWeights; ++i )
	{
		FxReal right = left + weights[i] * invSum;
		if( left <= randomWeight && randomWeight < right )
		{
			retval = i;
			break;
		}
		left = right;
	}

	if( retval == FxInvalidIndex )
	{
		retval = numWeights - 1;
	}

	return retval;
}

// Implementation of the "minimal" random number generator of Park and Miller.
// Adapted from "Numerical Recipes in C".
#define IA 16807
#define IM 2147483647
#define AM (1.0f/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876
FxReal FxRandomGenerator::ran0()
{
	_idum ^= MASK;
	FxInt32 k = _idum / IQ;
	_idum = IA * (_idum - k * IQ) - IR * k;
	if( _idum < 0 )
	{
		_idum += IM;
	}
	FxReal ans = AM * _idum;
	_idum ^= MASK;

	return ans;
}

} // namespace Face

} // namespace OC3Ent
