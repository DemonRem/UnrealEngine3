/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class BrushComponent extends PrimitiveComponent
	native
	noexport
	collapsecategories
	dependson(KMeshProps)
	editinlinenew;

var const Model						Brush;

/** Mirror for FKCachedConvexData struct. */
struct KCachedConvexData_Mirror
{
	var array<int>	CachedConvexElements;
};

/** Simplified collision data for the mesh. */
var KMeshProps.KAggregateGeom		BrushAggGeom;

/** Physics engine shapes created for this BrushComponent. */
var native private const transient noimport pointer						BrushPhysDesc;

/** Cached brush convex-mesh data for use with the physics engine. */
var native private const transient noimport KCachedConvexData_Mirror	CachedPhysBrushData;

/**
 *	Indicates version that CachedPhysBrushData was created at.
 *	Compared against CurrentCachedPhysDataVersion.
 */
var private const int				CachedPhysBrushDataVersion;

defaultproperties
{
	HiddenGame=True
	bAcceptsLights=false
	AlwaysLoadOnClient=false
	AlwaysLoadOnServer=false
	bUseAsOccluder=TRUE
}
