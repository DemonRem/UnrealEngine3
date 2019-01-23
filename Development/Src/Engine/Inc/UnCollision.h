/*=============================================================================
	UnCollision.h: Common collision code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _UN_COLLISION_H
#define _UN_COLLISION_H

/**
 * Collision stats
 */
enum ECollisionStats
{
	STAT_SingleLineCheck = STAT_CollisionFirstStat,
	STAT_MultiLineCheck,
	STAT_Col_Level,
	STAT_Col_Actors,
	STAT_Col_Sort,

	/** Terrain checks */
	STAT_TerrainExtentTime,
	STAT_TerrainZeroExtentTime,
	STAT_TerrainPointTime,
	/** Static mesh checks */
	STAT_StaticMeshZeroExtentTime,
	STAT_StaticMeshExtentTime,
	STAT_StaticMeshPointTime,
	/** BSP checks */
	STAT_BSPZeroExtentTime,
	STAT_BSPExtentTime,
	STAT_BSPPointTime
};


/** 
 *  Enable line check tracing, captures call stacks relating to the use of line checks
 *  in script and in native code
 */
#define LINE_CHECK_TRACING 1
#if LINE_CHECK_TRACING
extern void DumpLineChecks();   //dump line check stack traces to disk
extern void ResetLineChecks();  //reset stack gathering for line checks
extern void ToggleLineChecks(); //toggle line check stack gathering
extern void CaptureLineCheck(INT LineCheckFlags, const FVector* Extent, const FFrame* ScriptStackFrame);
#define LINE_CHECK_DUMP() do { DumpLineChecks(); } while(0)
#define LINE_CHECK_RESET() do { ResetLineChecks(); } while(0)
#define LINE_CHECK_TOGGLE() do { ToggleLineChecks(); } while(0)
#define LINE_CHECK_TRACE(flags, extent) do { CaptureLineCheck(flags, extent, NULL); } while(0)
#define LINE_CHECK_TRACE_SCRIPT(stack) do { CaptureLineCheck(0, NULL, stack); } while(0)
#else
#if COMPILER_SUPPORTS_NOOP
	#define LINE_CHECK_DUMP() __noop
	#define LINE_CHECK_RESET() __noop
	#define LINE_CHECK_TOGGLE() __noop
	#define LINE_CHECK_TRACE(flags, extent) __noop
	#define LINE_CHECK_TRACE_SCRIPT(stack) __noop
#else
	#define LINE_CHECK_DUMP()
	#define LINE_CHECK_RESET()
	#define LINE_CHECK_TOGGLE()
	#define LINE_CHECK_TRACE(flags, extent)
	#define LINE_CHECK_TRACE_SCRIPT(stack)
#endif //COMPILER_SUPPORTS_NOOP
#endif //LINE_CHECK_TRACING


//
//	Separating axis code.
//

static void CheckMinIntersectTime(FLOAT& MinIntersectTime,FVector& HitNormal,FLOAT Time,const FVector& Normal)
{
	if(Time > MinIntersectTime)
	{
		MinIntersectTime = Time;
		HitNormal = Normal;
	}
}

UBOOL FindSeparatingAxis(
	const FVector& V0,
	const FVector& V1,
	const FVector& V2,
	const FVector& Start,
	const FVector& End,
	const FVector& BoxExtent,
	const FVector& BoxX,
	const FVector& BoxY,
	const FVector& BoxZ,
	FLOAT& HitTime,
	FVector& OutHitNormal
	);

//
//	FSeparatingAxisPointCheck - Checks for intersection between an oriented bounding box and a triangle.
//	HitNormal: The normal of the separating axis the bounding box is penetrating the least.
//	BestDist: The amount the bounding box is penetrating the axis defined by HitNormal.
//

struct FSeparatingAxisPointCheck
{
	FVector	HitNormal;
	FLOAT	BestDist;
	UBOOL	Hit;

	const FVector&	V0,
					V1,
					V2;

	UBOOL TestSeparatingAxis(
		const FVector& Axis,
		FLOAT ProjectedPoint,
		FLOAT ProjectedExtent
		)
	{
		FLOAT	ProjectedV0 = Axis | V0,
				ProjectedV1 = Axis | V1,
				ProjectedV2 = Axis | V2,
				TriangleMin = Min(ProjectedV0,Min(ProjectedV1,ProjectedV2)) - ProjectedExtent,
				TriangleMax = Max(ProjectedV0,Max(ProjectedV1,ProjectedV2)) + ProjectedExtent;

		if(ProjectedPoint >= TriangleMin && ProjectedPoint <= TriangleMax)
		{
			// Use inverse sqrt because that is fast and we do more math with the inverse value anyway
			FLOAT	InvAxisMagnitude = appInvSqrt(Axis.X * Axis.X +	Axis.Y * Axis.Y + Axis.Z * Axis.Z),
					ScaledBestDist = BestDist / InvAxisMagnitude,
					MinPenetrationDist = ProjectedPoint - TriangleMin,
					MaxPenetrationDist = TriangleMax - ProjectedPoint;
			if(MinPenetrationDist < ScaledBestDist)
			{
				BestDist = MinPenetrationDist * InvAxisMagnitude;
				HitNormal = -Axis * InvAxisMagnitude;
			}
			if(MaxPenetrationDist < ScaledBestDist)
			{
				BestDist = MaxPenetrationDist * InvAxisMagnitude;
				HitNormal = Axis * InvAxisMagnitude;
			}
			return 1;
		}
		else
			return 0;
	}

	UBOOL TestSeparatingAxis(
		const FVector& Axis,
		const FVector& Point,
		FLOAT ProjectedExtent
		)
	{
		return TestSeparatingAxis(Axis,Axis | Point,ProjectedExtent);
	}

	UBOOL TestSeparatingAxis(
		const FVector& Axis,
		const FVector& Point,
		const FVector& BoxX,
		const FVector& BoxY,
		const FVector& BoxZ,
		const FVector& BoxExtent
		)
	{
		FLOAT	ProjectedExtent = BoxExtent.X * Abs(Axis | BoxX) + BoxExtent.Y * Abs(Axis | BoxY) + BoxExtent.Z * Abs(Axis | BoxZ);
		return TestSeparatingAxis(Axis,Axis | Point,ProjectedExtent);
	}

	UBOOL FindSeparatingAxis(
		const FVector& Point,
		const FVector& BoxExtent,
		const FVector& BoxX,
		const FVector& BoxY,
		const FVector& BoxZ
		)
	{
		// Box faces. We need to calculate this by crossing edges because BoxX etc are the _edge_ directions - not the faces.
		// The box may be sheared due to non-unfiform scaling and rotation so FaceX normal != BoxX edge direction

		if(!TestSeparatingAxis(BoxX ^ BoxY,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(BoxY ^ BoxZ,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(BoxZ ^ BoxX,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		// Triangle normal.

		if(!TestSeparatingAxis((V2 - V1) ^ (V1 - V0),Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		// Box X edge x triangle edges.

		if(!TestSeparatingAxis((V1 - V0) ^ BoxX,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis((V2 - V1) ^ BoxX,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis((V0 - V2) ^ BoxX,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		// Box Y edge x triangle edges.

		if(!TestSeparatingAxis((V1 - V0) ^ BoxY,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis((V2 - V1) ^ BoxY,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;
		
		if(!TestSeparatingAxis((V0 - V2) ^ BoxY,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;
		
		// Box Z edge x triangle edges.

		if(!TestSeparatingAxis((V1 - V0) ^ BoxZ,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		if(!TestSeparatingAxis((V2 - V1) ^ BoxZ,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;
		
		if(!TestSeparatingAxis((V0 - V2) ^ BoxZ,Point,BoxX,BoxY,BoxZ,BoxExtent))
			return 0;

		return 1;
	}

	FSeparatingAxisPointCheck(
		const FVector& InV0,
		const FVector& InV1,
		const FVector& InV2,
		const FVector& Point,
		const FVector& BoxExtent,
		const FVector& BoxX,
		const FVector& BoxY,
		const FVector& BoxZ,
		FLOAT InBestDist
		):
		HitNormal(0,0,0),
		BestDist(InBestDist),
		Hit(0),
		V0(InV0),
		V1(InV1),
		V2(InV2)
	{
		Hit = FindSeparatingAxis(Point,BoxExtent,BoxX,BoxY,BoxZ);
	}
};

/**
 *	Line Check With Triangle
 *	Algorithm based on "Fast, Minimum Storage Ray/Triangle Intersection"
 *	Returns TRUE if the line segment does hit the triangle
 */
FORCEINLINE UBOOL LineCheckWithTriangle(FCheckResult& Result,const FVector& V1,const FVector& V2,const FVector& V3,const FVector& Start,const FVector& End,const FVector& Direction)
{
	FVector	Edge1 = V3 - V1,
		Edge2 = V2 - V1,
		P = Direction ^ Edge2;
	FLOAT	Determinant = Edge1 | P;

	if(Determinant < DELTA)
	{
		return FALSE;
	}

	FVector	T = Start - V1;
	FLOAT	U = T | P;

	if(U < 0.0f || U > Determinant)
	{
		return FALSE;
	}

	FVector	Q = T ^ Edge1;
	FLOAT	V = Direction | Q;

	if(V < 0.0f || U + V > Determinant)
	{
		return FALSE;
	}

	FLOAT	Time = (Edge2 | Q) / Determinant;

	if(Time < 0.0f || Time > Result.Time)
	{
		return FALSE;
	}

	Result.Normal = ((V3-V2)^(V2-V1)).SafeNormal();
	Result.Time = ((V1 - Start)|Result.Normal) / (Result.Normal|Direction);

	return TRUE;
}


#if XBOX
	#include "UnCollisionXe.h"
#elif PS3 && 0 //@todo joeg -- do ps3 version
	#include "UnCollisionPS3.h"
#else
/**
 * Performs a sphere vs box intersection test using Arvo's algorithm:
 *
 *	for each i in (x, y, z)
 *		if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
 *		else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2
 *
 * @param Sphere the center of the sphere being tested against the AABB
 * @param RadiusSquared the size of the sphere being tested
 * @param AABB the box being tested against
 *
 * @return Whether the sphere/box intersect or not.
 */
FORCEINLINE UBOOL SphereAABBIntersectionTest(const FVector& Sphere,const FLOAT RadiusSquared,const FBox& AABB)
{
	// Accumulates the distance as we iterate axis
	FLOAT DistSquared = 0.f;
	// Check each axis for min/max and add the distance accordingly
	// NOTE: Loop manually unrolled for > 2x speed up
	if (Sphere.X < AABB.Min.X)
	{
		DistSquared += Square(Sphere.X - AABB.Min.X);
	}
	else if (Sphere.X > AABB.Max.X)
	{
		DistSquared += Square(Sphere.X - AABB.Max.X);
	}
	if (Sphere.Y < AABB.Min.Y)
	{
		DistSquared += Square(Sphere.Y - AABB.Min.Y);
	}
	else if (Sphere.Y > AABB.Max.Y)
	{
		DistSquared += Square(Sphere.Y - AABB.Max.Y);
	}
	if (Sphere.Z < AABB.Min.Z)
	{
		DistSquared += Square(Sphere.Z - AABB.Min.Z);
	}
	else if (Sphere.Z > AABB.Max.Z)
	{
		DistSquared += Square(Sphere.Z - AABB.Max.Z);
	}
	// If the distance is less than or equal to the radius, they intersect
	return DistSquared <= RadiusSquared;
}

/**
 * Converts a sphere into a point plus radius squared for the test above
 */
FORCEINLINE UBOOL SphereAABBIntersectionTest(const FSphere& Sphere,const FBox& AABB)
{
	FLOAT RadiusSquared = Square(Sphere.W);
	// If the distance is less than or equal to the radius, they intersect
	return SphereAABBIntersectionTest(Sphere,RadiusSquared,AABB);
}
#endif

#endif
