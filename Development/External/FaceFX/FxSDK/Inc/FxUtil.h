//------------------------------------------------------------------------------
// FaceFx utility code.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxUtil_H__
#define FxUtil_H__

#include "FxPlatform.h"

namespace OC3Ent
{

namespace Face
{

/// Declares a function argument unused (for compiling in warning level 4).
#define FxUnused(var) /* var */

/// An invalid index.
/// \ingroup support
static const FxSize FxInvalidIndex = static_cast<FxSize>(-1);
/// An invalid value.
/// \ingroup support
static const FxReal FxInvalidValue = static_cast<FxReal>(FX_INT32_MIN);

/// Creates a FOURCC (four character code) from four characters.
/// \ingroup support
#define FX_MAKE_FOURCC(ch0, ch1, ch2, ch3) \
	                  ((FxUInt32)(FxUInt8)(ch0)        | \
	                  ((FxUInt32)(FxUInt8)(ch1) << 8)  | \
	                  ((FxUInt32)(FxUInt8)(ch2) << 16) | \
	                  ((FxUInt32)(FxUInt8)(ch3) << 24))

/// Performs an XOR swap.
/// \ingroup support
#define FX_XOR_SWAP(x, y) (x)^=(y); (y)^=(x); (x)^=(y)

/// Performs byte swapping.
/// \ingroup support
FX_INLINE void FX_CALL FxByteSwap( FxByte* byte, FxSize size )
{
	FxSize i = 0;
	FxSize j = size - 1;
	while( i < j )
	{
		FX_XOR_SWAP(byte[i], byte[j]);
		i++, j--;
	}
}

// Some handy macros to aid in calling FxByteSwap() correctly.  Note that 
// currently these macros are not commented in the Doxygen style on purpose.
//@todo Comment these in the Doxygen style when we make the final pass through
//      the documentation to pick up the macro definitions.
#if defined(FX_LITTLE_ENDIAN)
	#define FX_SWAP_BIG_ENDIAN_BYTES(x) FxByteSwap(reinterpret_cast<FxByte*>(&x), sizeof(x))
	#define FX_SWAP_LITTLE_ENDIAN_BYTES(x)
#elif defined(FX_BIG_ENDIAN)
	#define FX_SWAP_BIG_ENDIAN_BYTES(x)
	#define FX_SWAP_LITTLE_ENDIAN_BYTES(x) FxByteSwap(reinterpret_cast<FxByte*>(&x), sizeof(x))
#else
	#define FX_SWAP_BIG_ENDIAN_BYTES(x)
	#define FX_SWAP_LITTLE_ENDIAN_BYTES(x)
	#error "Endianness not defined for target platform.  Check FxUtil.h."
#endif

/// Square function.
template<typename T> FX_INLINE T FX_CALL FxSquare( const T& a )
{
	return a * a;
}

/// Max function.
template<typename T> FX_INLINE T FX_CALL FxMax( const T& a, const T& b )
{
	return (a >= b) ? a : b;
}

/// Min function.
template<typename T> FX_INLINE T FX_CALL FxMin( const T& a, const T& b )
{
	return (a <= b) ? a : b;
}

/// Clamp function.
template<typename T> FX_INLINE T FX_CALL FxClamp( const T& valMin, const T& val, 
								                 const T& valMax )
{
	return FxMax(FxMin(val, valMax), valMin);
}

/// Swap function.
template<typename T> FX_INLINE void FX_CALL FxSwap( T& first, T& second )
{
	T temp = first;
	first  = second;
	second = temp;
}

/// Returns the nearest integer to an FxDReal value.
FX_INLINE FxInt32 FX_CALL FxNearestInteger( FxDReal value )
{
	FxInt32 result = 0;
	if( value > 0.0 )
	{
		value += 0.5;
		if( value >= FX_INT32_MAX )
		{
			result = FX_INT32_MAX;
		}
		else
		{
			result = static_cast<FxInt32>(value);
		}
	}
	else if( value < 0.0 )
	{
		value -= 0.5;
		if( value <= FX_INT32_MIN )
		{
			result = FX_INT32_MIN;
		}
		else
		{
			result = static_cast<FxInt32>(value);
		}
	}
	return result;
}

/// Returns the nearest integer to an FxReal value.
FX_INLINE FxInt32 FX_CALL FxNearestInteger( FxReal value )
{
	FxInt32 result = 0;
	if( value > 0.0f )
	{
		value += 0.5f;
		if( value >= FX_INT32_MAX )
		{
			result = FX_INT32_MAX;
		}
		else
		{
			result = static_cast<FxInt32>(value);
		}
	}
	else if( value < 0.0f )
	{
		value -= 0.5f;
		if( value <= FX_INT32_MIN )
		{
			result = FX_INT32_MIN;
		}
		else
		{
			result = static_cast<FxInt32>(value);
		}
	}
	return result;
}

} // namespace Face

} // namespace OC3Ent

#endif
