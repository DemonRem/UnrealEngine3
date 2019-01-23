/*=============================================================================
	UnActor.cpp: AActor implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnCollision.h"

//
//	Separating axis code.
//

static UBOOL TestSeparatingAxis(
								const FVector& V0,
								const FVector& V1,
								const FVector& V2,
								const FVector& Line,
								FLOAT ProjectedStart,
								FLOAT ProjectedEnd,
								FLOAT ProjectedExtent,
								FLOAT& MinIntersectTime,
								FLOAT& MaxIntersectTime,
								FVector& HitNormal
								)
{
	FLOAT	ProjectedDirection = ProjectedEnd - ProjectedStart,
		ProjectedV0 = Line | V0,
		ProjectedV1 = Line | V1,
		ProjectedV2 = Line | V2,
		TriangleMin = Min(ProjectedV0,Min(ProjectedV1,ProjectedV2)) - ProjectedExtent,
		TriangleMax = Max(ProjectedV0,Max(ProjectedV1,ProjectedV2)) + ProjectedExtent;

	// If zero - check vector is perp to test axis, so just check if we start outside. If so, we can't collide.
	if( Abs(ProjectedDirection) < SMALL_NUMBER )
	{
		if( ProjectedStart < TriangleMin ||
			ProjectedStart > TriangleMax )
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	FLOAT OneOverProjectedDirection = 1.f / ProjectedDirection;
	// Moving in a positive direction - enter ConvexMin and exit ConvexMax
	FLOAT EntryTime, ExitTime;
	FVector ImpactNormal;
	if(ProjectedDirection > 0.f)
	{
		EntryTime = (TriangleMin - ProjectedStart) * OneOverProjectedDirection;
		ExitTime = (TriangleMax - ProjectedStart) * OneOverProjectedDirection;
		ImpactNormal = -Line;
	}
	// Moving in a negative direction - enter ConvexMax and exit ConvexMin
	else
	{
		EntryTime = (TriangleMax - ProjectedStart) * OneOverProjectedDirection;
		ExitTime = (TriangleMin - ProjectedStart) * OneOverProjectedDirection;
		ImpactNormal = Line;
	}	

	// See if entry time is further than current furthest entry time. If so, update and remember normal
	if(EntryTime > MinIntersectTime)
	{
		MinIntersectTime = EntryTime;
		HitNormal = ImpactNormal;
	}

	// See if exit time is close than current closest exit time.
	MaxIntersectTime = Min(MaxIntersectTime, ExitTime);

	// If exit is ever before entry - we don't intersect
	if( MaxIntersectTime < MinIntersectTime )
	{
		return FALSE;
	}

	// If exit is ever before start of line check - we don't intersect
	if( MaxIntersectTime < 0.f )
	{
		return FALSE;
	}

	return TRUE;
}


UBOOL TestSeparatingAxis(
						 const FVector& V0,
						 const FVector& V1,
						 const FVector& V2,
						 const FVector& TriangleEdge,
						 const FVector& BoxEdge,
						 const FVector& Start,
						 const FVector& End,
						 const FVector& BoxX,
						 const FVector& BoxY,
						 const FVector& BoxZ,
						 const FVector& BoxExtent,
						 FLOAT& MinIntersectTime,
						 FLOAT& MaxIntersectTime,
						 FVector& HitNormal
						 )
{
	// Separating axis is cross product of box and triangle edges.
	FVector	Line = BoxEdge ^ TriangleEdge;

	// Check separating axis is non-zero. If it is, just don't use this axis.
	if(Line.SizeSquared() < SMALL_NUMBER)
	{
		return TRUE;
	}

	// Calculate extent of box projected onto separating axis.
	FLOAT ProjectedExtent = BoxExtent.X * Abs(Line | BoxX) + BoxExtent.Y * Abs(Line | BoxY) + BoxExtent.Z * Abs(Line | BoxZ);


	return TestSeparatingAxis(V0,V1,V2,Line,Line | Start,Line | End,ProjectedExtent,MinIntersectTime,MaxIntersectTime,HitNormal);
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
						)
{
	FLOAT	MinIntersectTime = -BIG_NUMBER,
			MaxIntersectTime = BIG_NUMBER;

	// Calculate normalized edge directions. We normalize here to minimize precision issues when doing cross products later.
	const FVector& EdgeDir0 = (V1 - V0).SafeNormal();
	const FVector& EdgeDir1 = (V2 - V1).SafeNormal();
	const FVector& EdgeDir2 = (V0 - V2).SafeNormal();

	// Used to set the hit normal locally and apply the best normal only upon a full hit
	FVector HitNormal;

	// Box faces. We need to calculate this by crossing edges because BoxX etc are the _edge_ directions - not the faces.
	// The box may be sheared due to non-uniform scaling and rotation so FaceX normal != BoxX edge direction

	if(!TestSeparatingAxis(V0,V1,V2,BoxX,BoxY,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,BoxY,BoxZ,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,BoxZ,BoxX,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	// Triangle normal.

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir1,EdgeDir0,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	// Box X edge x triangle edges.

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir0,BoxX,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir1,BoxX,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir2,BoxX,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	// Box Y edge x triangle edges.

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir0,BoxY,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir1,BoxY,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir2,BoxY,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	// Box Z edge x triangle edges.

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir0,BoxZ,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir1,BoxZ,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	if(!TestSeparatingAxis(V0,V1,V2,EdgeDir2,BoxZ,Start,End,BoxX,BoxY,BoxZ,BoxExtent,MinIntersectTime,MaxIntersectTime,HitNormal))
		return FALSE;

	// If hit is beyond end of ray, no collision occurs.
	if(MinIntersectTime > HitTime)
	{
		return FALSE;
	}

	// If hit time is positive- we start outside and hit the hull
	if(MinIntersectTime >= 0.0f)
	{
		HitTime = MinIntersectTime;
		OutHitNormal = HitNormal;
	}
	// If hit time is negative- the entry is behind the start - so we started colliding
	else
	{
		HitTime = 0.f;
		OutHitNormal = -(End - Start).SafeNormal();
	}

	return TRUE;
}

