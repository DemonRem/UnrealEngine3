/*============================================================================
	Karma Integration Support
    
    - MeMemory/MeMessage glue
    - Debug line drawing
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 ===========================================================================*/

#include "EnginePrivate.h"

/* *********************************************************************** */
/* *********************************************************************** */
/* *********************** MODELTOHULLS  ********************************* */
/* *********************************************************************** */
/* *********************************************************************** */

#define LOCAL_EPS (0.01f)

static void AddVertexIfNotPresent(TArray<FVector> &vertices, const FVector& newVertex)
{
	UBOOL isPresent = 0;

	for(INT i=0; i<vertices.Num() && !isPresent; i++)
	{
		FLOAT diffSqr = (newVertex - vertices(i)).SizeSquared();
		if(diffSqr < LOCAL_EPS * LOCAL_EPS)
			isPresent = 1;
	}

	if(!isPresent)
		vertices.AddItem(newVertex);
}

/** Returns FALSE if ModelToHulls operation should halt because of vertex count overflow. */
static UBOOL AddConvexPrim(FKAggregateGeom* outGeom, TArray<FPlane> &planes, UModel* inModel)
{
	// Add Hull.
	const int ex = outGeom->ConvexElems.AddZeroed();
	FKConvexElem* c = &outGeom->ConvexElems(ex);

	FLOAT TotalPolyArea = 0;

	for(INT i=0; i<planes.Num(); i++)
	{
		FPoly	Polygon;
		Polygon.Normal = planes(i);

		FVector AxisX, AxisY;
		Polygon.Normal.FindBestAxisVectors(AxisX,AxisY);

		const FVector Base = planes(i) * planes(i).W;

		new(Polygon.Vertices) FVector(Base + AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);
		new(Polygon.Vertices) FVector(Base - AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);
		new(Polygon.Vertices) FVector(Base - AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);
		new(Polygon.Vertices) FVector(Base + AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);

		for(INT j=0; j<planes.Num(); j++)
		{
			if(i != j)
			{
				if(!Polygon.Split(-FVector(planes(j)), planes(j) * planes(j).W))
				{
					Polygon.Vertices.Empty();
					break;
				}
			}
		}

		// Do nothing if poly was completely clipped away.
		if(Polygon.Vertices.Num() > 0)
		{
			TotalPolyArea += Polygon.Area();

			// Add vertices of polygon to convex primitive.
			for(INT j=0; j<Polygon.Vertices.Num(); j++)
			{
				// Because of errors with the polygon-clipping, we dont use the vertices we just generated,
				// but the ones stored in the model. We find the closest.
				INT nearestVert = INDEX_NONE;
				FLOAT nearestDistSqr = BIG_NUMBER;

				for(INT k=0; k<inModel->Verts.Num(); k++)
				{
					// Find vertex vector. Bit of  hack - sometimes FVerts are uninitialised.
					const INT pointIx = inModel->Verts(k).pVertex;
					if(pointIx < 0 || pointIx >= inModel->Points.Num())
					{
						continue;
					}

					const FLOAT distSquared = (Polygon.Vertices(j) - inModel->Points(pointIx)).SizeSquared();

					if( distSquared < nearestDistSqr )
					{
						nearestVert = k;
						nearestDistSqr = distSquared;
					}
				}

				// If we have found a suitably close vertex, use that
				if( nearestVert != INDEX_NONE && nearestDistSqr < LOCAL_EPS )
				{
					const FVector localVert = ((inModel->Points(inModel->Verts(nearestVert).pVertex)));
					AddVertexIfNotPresent(c->VertexData, localVert);
				}
				else
				{
					const FVector localVert = (Polygon.Vertices(j));
					AddVertexIfNotPresent(c->VertexData, localVert);
				}
			}
		}
	}

#if 1
	// If the collision volume isn't closed, return an error so the model can be discarded
	if(TotalPolyArea < 0.001f)
	{
		debugf( TEXT("Total Polygon Area invalid: %f"), TotalPolyArea );
		return FALSE;
	}

	// We need at least 4 vertices to make a convex hull with non-zero volume.
	// We shouldn't have the same vertex multiple times (using AddVertexIfNotPresent above)
	if(c->VertexData.Num() < 4)
	{
		outGeom->ConvexElems.Remove(ex);
		return TRUE;
	}

	// Check that not all vertices lie on a line (ie. find plane)
	// Again, this should be a non-zero vector because we shouldn't have duplicate verts.
	UBOOL found;
	FVector dir2, dir1;
	
	dir1 = c->VertexData(1) - c->VertexData(0);
	dir1.Normalize();

	found = 0;
	for(INT i=2; i<c->VertexData.Num() && !found; i++)
	{
		dir2 = c->VertexData(i) - c->VertexData(0);
		dir2.Normalize();

		// If line are non-parallel, this vertex forms our plane
		if((dir1 | dir2) < (1 - LOCAL_EPS))
		{
			found = 1;
		}
	}

	if(!found)
	{
		outGeom->ConvexElems.Remove(ex);
		return TRUE;
	}

	// Now we check that not all vertices lie on a plane, by checking at least one lies off the plane we have formed.
	FVector normal = dir1 ^ dir2;
	normal.Normalize();

	const FPlane plane(c->VertexData(0), normal);

	found = 0;
	for(INT i=2; i<c->VertexData.Num() ; i++)
	{
		if(plane.PlaneDot(c->VertexData(i)) > LOCAL_EPS)
		{
			found = 1;
			break;
		}
	}
	
	// If we did not find a vert off the line - discard this hull.
	if(!found)
	{
		outGeom->ConvexElems.Remove(ex);
		return TRUE;
	}
#endif

	// Generate face/surface/plane data needed for Unreal collision.
	UBOOL bHullIsGood = c->GenerateHullData();

	// If this fails, we don't want this hull. Throw it away.
	if(!bHullIsGood)
	{
		outGeom->ConvexElems.Remove(ex);
		return TRUE;
	}

	// We can continue adding primitives (mesh is not hirribly broken)
	return TRUE;
}

// Worker function for traversing collision mode/blocking volumes BSP.
// At each node, we record, the plane at this node, and carry on traversing.
// We are interested in 'inside' ie solid leafs.
/** Returns FALSE if ModelToHulls operation should halt because of vertex count overflow. */
static UBOOL ModelToHullsWorker(FKAggregateGeom* outGeom,
								UModel* inModel, 
								INT nodeIx, 
								UBOOL bOutside, 
								TArray<FPlane> &planes)
{
	FBspNode* node = &inModel->Nodes(nodeIx);
	if(node)
	{
		// BACK
		if(node->iBack != INDEX_NONE) // If there is a child, recurse into it.
		{
			planes.AddItem(node->Plane);
			if ( !ModelToHullsWorker(outGeom, inModel, node->iBack, node->ChildOutside(0, bOutside), planes) )
			{
				return FALSE;
			}
			planes.Remove(planes.Num()-1);
		}
		else if(!node->ChildOutside(0, bOutside)) // If its a leaf, and solid (inside)
		{
			planes.AddItem(node->Plane);
			if ( !AddConvexPrim(outGeom, planes, inModel) )
			{
				return FALSE;
			}
			planes.Remove(planes.Num()-1);
		}

		// FRONT
		if(node->iFront != INDEX_NONE)
		{
			planes.AddItem(node->Plane.Flip());
			if ( !ModelToHullsWorker(outGeom, inModel, node->iFront, node->ChildOutside(1, bOutside), planes) )
			{
				return FALSE;
			}
			planes.Remove(planes.Num()-1);
		}
		else if(!node->ChildOutside(1, bOutside))
		{
			planes.AddItem(node->Plane.Flip());
			if ( !AddConvexPrim(outGeom, planes, inModel) )
			{
				return FALSE;
			}
			planes.Remove(planes.Num()-1);
		}
	}

	return TRUE;
}

// Converts a UModel to a set of convex hulls for.  If flag deleteContainedHull is set any convex elements already in
// outGeom will be destroyed.  WARNING: the input model can have no single polygon or
// set of coplanar polygons which merge to more than FPoly::MAX_VERTICES vertices.
// Creates it around the model origin, and applies the Unreal->Physics scaling.
UBOOL KModelToHulls(FKAggregateGeom* outGeom, UModel* inModel, UBOOL deleteContainedHulls/*=TRUE*/ )
{
	UBOOL bSuccess = TRUE;

	if ( deleteContainedHulls )
	{
		outGeom->ConvexElems.Empty();
	}

	const INT NumHullsAtStart = outGeom->ConvexElems.Num();
	
	if( inModel )
	{
		TArray<FPlane>	planes;
		bSuccess = ModelToHullsWorker(outGeom, inModel, 0, inModel->RootOutside, planes);
		if ( !bSuccess )
		{
			// ModelToHulls failed.  Clear out anything that may have been created.
			outGeom->ConvexElems.Remove( NumHullsAtStart, outGeom->ConvexElems.Num() - NumHullsAtStart );
		}
	}

	return bSuccess;
}

/** Set the status of a particular channel in the structure. */
void FRBCollisionChannelContainer::SetChannel(ERBCollisionChannel Channel, UBOOL bNewState)
{
	INT ChannelShift = (INT)Channel;

#if !__INTEL_BYTE_ORDER__
	DWORD ChannelBit = (1 << (31 - ChannelShift));
#else
	DWORD ChannelBit = (1 << ChannelShift);
#endif

	if(bNewState)
	{
		Bitfield = Bitfield | ChannelBit;
	}
	else
	{
		Bitfield = Bitfield & ~ChannelBit;
	}
}

