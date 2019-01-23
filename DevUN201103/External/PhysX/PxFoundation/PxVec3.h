// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES 
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable. 
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation.
//
// Copyright � 2009 NVIDIA Corporation. All rights reserved.
// Copyright � 2002-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright � 2001-2006 NovodeX. All rights reserved.   

#ifndef PX_FOUNDATION_PXVEC3_H
#define PX_FOUNDATION_PXVEC3_H

/** \addtogroup foundation
@{
*/

#include "PxMath.h"

namespace physx
{
namespace pubfnd2
{

PX_PUSH_PACK_DEFAULT

/**
\brief 3 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xyz data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining PxReal.
This has been chosen as a cleaner alternative to a template class.
*/
class PxVec3
{
public:

	/**
	\brief default constructor leaves data uninitialized.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3() {}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3(PxReal a): x(a), y(a), z(a) {}

	/**
	\brief Initializes from 3 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	\param[in] nz Value to initialize Z component.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3(PxReal nx, PxReal ny, PxReal nz): x(nx), y(ny), z(nz) {}

	/**
	\brief Copy ctor.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3(const PxVec3& v): x(v.x), y(v.y), z(v.z) {}

	//Operators

	/**
	\brief Assignment operator
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE	PxVec3&	operator=(const PxVec3& p)			{ x = p.x; y = p.y; z = p.z;	return *this;		}

	/**
	\brief element access
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxReal& operator[](int index)					{ PX_ASSERT(index>=0 && index<=2); return (&x)[index]; }

	/**
	\brief element access
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE const PxReal& operator[](int index) const		{ PX_ASSERT(index>=0 && index<=2); return (&x)[index]; }

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE bool operator==(const PxVec3&v) const	{ return x == v.x && y == v.y && z == v.z; }

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE bool operator!=(const PxVec3&v) const	{ return x != v.x || y != v.y || z != v.z; }

	/**
	\brief tests for exact zero vector
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE bool isZero()	const					{ return x==0.0f && y==0.0f && z == 0.0f;			}

	/**
	\brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
	*/
	PX_CUDA_MEMBER PX_INLINE bool isFinite() const
	{
		return PxIsFinite(x) && PxIsFinite(y) && PxIsFinite(z);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE bool isNormalized() const
	{
		const float unitTolerance = PxReal(1e-4);
		return isFinite() && PxAbs(magnitude()-1)<unitTolerance;
	}

	/**
	\brief returns the squared magnitude

	Avoids calling PxSqrt()!
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxReal magnitudeSquared() const		{	return x * x + y * y + z * z;					}

	/**
	\brief returns the magnitude
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxReal magnitude() const				{	return PxSqrt(magnitudeSquared());		}

	/**
	\brief negation
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator -() const
	{
		return PxVec3(-x, -y, -z);
	}

	/**
	\brief vector addition
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator +(const PxVec3& v) const		{	return PxVec3(x + v.x, y + v.y, z + v.z);	}

	/**
	\brief vector difference
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator -(const PxVec3& v) const		{	return PxVec3(x - v.x, y - v.y, z - v.z);	}

	/**
	\brief scalar post-multiplication
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator *(PxReal f) const				{	return PxVec3(x * f, y * f, z * f);			}

	/**
	\brief scalar division
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator /(PxReal f) const
	{
		f = PxReal(1) / f;	// PT: inconsistent notation with operator /=
		return PxVec3(x * f, y * f, z * f);
	}

	/**
	\brief vector addition
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3& operator +=(const PxVec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	
	/**
	\brief vector difference
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3& operator -=(const PxVec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3& operator *=(PxReal f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3& operator /=(PxReal f)
	{
		f = 1.0f/f;	// PT: inconsistent notation with operator /
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxReal dot(const PxVec3& v) const		
	{	
		return x * v.x + y * v.y + z * v.z;				
	}

	/**
	\brief cross product
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 cross(const PxVec3& v) const
	{
		return PxVec3(y * v.z - z * v.y, 
					  z * v.x - x * v.z, 
					  x * v.y - y * v.x);
	}

	/** return a unit vector */

	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 getNormalized() const
	{
		const PxReal m = magnitudeSquared();
		return m>0 ? *this * PxRecipSqrt(m) : PxVec3(0,0,0);
	}

	/**
	\brief normalizes the vector in place
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxReal normalize()
	{
		const PxReal m = magnitude();
		if (m>0) 
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 multiply(const PxVec3& a) const
	{
		return PxVec3(x*a.x, y*a.y, z*a.z);
	}

	/**
	\brief element-wise minimum
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 minimum(const PxVec3& v) const
	{ 
		return PxVec3(PxMin(x, v.x), PxMin(y,v.y), PxMin(z,v.z));	
	}

	/**
	\brief returns MIN(x, y, z);
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE float minElement()	const
	{
		return PxMin(x, PxMin(y, z));
	}
	
	/**
	\brief element-wise maximum
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 maximum(const PxVec3& v) const
	{ 
		return PxVec3(PxMax(x, v.x), PxMax(y,v.y), PxMax(z,v.z));	
	} 

	/**
	\brief returns MAX(x, y, z);
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE float maxElement()	const
	{
		return PxMax(x, PxMax(y, z));
	}

	/**
	\brief set vector elements to zero
	*/
	PX_CUDA_MEMBER PX_FORCE_INLINE void setZero() {	x = y = z = PxReal(0); }

	/** DEPRECATED FUNCTIONS */
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE void set(PxReal x0, PxReal y0, PxReal z0) { x = x0; y = y0; z = z0; }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE PxReal distance(const PxVec3& p) const { return (*this -p).magnitude(); }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE PxReal distanceSquared(const PxVec3& p) const { return (*this -p).magnitudeSquared(); }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE static PxVec3 zero() { PxVec3 result; result.setZero(); return result; }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 operator^(const PxVec3& other) const { return this->cross(other); }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE PxReal operator|(const PxVec3& other) const { return this->dot(other); }
	PX_DEPRECATED PX_CUDA_MEMBER PX_FORCE_INLINE PxVec3 arrayMultiply(const PxVec3& v) const { return PxVec3(x*v.x, y*v.y, z*v.z); }

	PxReal x,y,z;
};

PX_CUDA PX_FORCE_INLINE PxVec3 operator *(PxReal f, const PxVec3& v)
{
	return PxVec3(f * v.x, f * v.y, f * v.z);
}

PX_POP_PACK

} // namespace pubfnd2
} // end namespace physx

#endif