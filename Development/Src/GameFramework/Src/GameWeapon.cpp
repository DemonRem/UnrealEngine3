/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "GameFramework.h"

IMPLEMENT_CLASS(UGameExplosion);
IMPLEMENT_CLASS(AGameExplosionActor);

/** 
  * Returns distance from bounding box to point
  */
FLOAT AGameExplosionActor::BoxDistanceToPoint(FVector Start, FBox BBox)
{
	return appSqrt(BBox.ComputeSquaredDistanceToPoint(Start));
}


