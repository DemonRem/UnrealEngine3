//=============================================================================
// ReachSpec.
//
// A Reachspec describes the reachability requirements between two NavigationPoints
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ReachSpec extends Object
	native;

const BLOCKEDPATHCOST = 10000000; // any path cost >= this value indicates the path is blocked to the pawn

/** pointer to object in navigation octree */
var native transient const editconst pointer NavOctreeObject{struct FNavigationOctreeObject};

cpptext
{
	/*
	supports() -
	returns true if it supports the requirements of aPawn.  Distance is not considered.
	*/
	inline UBOOL supports (INT iRadius, INT iHeight, INT moveFlags, INT iMaxFallVelocity)
	{
		return ( (CollisionRadius >= iRadius)
			&& (CollisionHeight >= iHeight)
			&& ((reachFlags & moveFlags) == reachFlags)
			&& (MaxLandingVelocity <= iMaxFallVelocity) );
	}

	/** CostFor()
	Returns the "cost" in unreal units
	for Pawn P to travel from the start to the end of this reachspec
	*/
	virtual INT CostFor(APawn* P);
	virtual UBOOL PrepareForMove( AController * C );
	virtual UBOOL IsForced() { return false; }
	virtual UBOOL IsProscribed() const { return false; }
	virtual INT defineFor (class ANavigationPoint *begin, class ANavigationPoint * dest, class APawn * Scout);
	int operator<= (const UReachSpec &Spec);
	virtual FPlane PathColor();
	virtual void AddToDebugRenderProxy(class FDebugRenderSceneProxy* DRSP);
	int findBestReachable(class AScout *Scout);
	UBOOL ShouldPruneAgainst( UReachSpec* Spec );

	/** If bAddToNavigationOctree is true, adds the ReachSpec to the navigation octree */
	void AddToNavigationOctree();
	void RemoveFromNavigationOctree();
	/** returns whether TestBox overlaps the path this ReachSpec represents
	 * @note this function assumes that TestBox has already passed a bounding box overlap check
	 * @param TestBox the box to check
	 * @return true if the box doesn't overlap this path, false if it does
	 */
	UBOOL NavigationOverlapCheck(const FBox& TestBox);
	/** returns whether Point is within MaxDist units of the path this ReachSpec represents
	 * @param Point the point to check
	 * @param MaxDist the maximum distance the point can be from the path
	 * @return true if the point is within MaxDist units, false otherwise
	 */
	UBOOL IsOnPath(const FVector& Point, FLOAT MaxDist);
	/** returns whether this path is currently blocked and unusable to the given pawn */
	UBOOL IsBlockedFor(APawn* P);

	virtual void FinishDestroy();

	/** Get path size for a forced path to this nav point */
	virtual FVector GetForcedPathSize( class ANavigationPoint* Nav, class AScout* Scout );
}

var	int		Distance;
var() const editconst NavigationPoint	Start;		// navigationpoint at start of this path
var() const editconst NavReference		End;
var() const editconst int				CollisionRadius;
var() const editconst int				CollisionHeight;
var	int		reachFlags;					// see EReachSpecFlags definition in UnPath.h
var	int		MaxLandingVelocity;
var	byte	bPruned;
var byte	PathColorIndex;				// used to look up pathcolor, set when reachspec is created
/** whether or not this ReachSpec should be added to the navigation octree */
var const editconst bool bAddToNavigationOctree;
/** If true, pawns moving along this path can cut corners transitioning between this reachspec and adjacent reachspecs */
var bool bCanCutCorners;
/** Prune paths should skip trying to prune along these */
var const bool	bSkipPrune;
/** Can always prune against these types of specs (even though class doesn't match) */
var const array< class<ReachSpec> > PruneSpecList;

/** Name of path size to use for forced reach spec */
var Name	ForcedPathSizeName;


/** Actor that is blocking this ReachSpec, making it temporarily unusable */
var Actor BlockedBy;

/** CostFor()
Returns the "cost" in unreal units
for Pawn P to travel from the start to the end of this reachspec
*/
native final noexport function int CostFor(Pawn P);
/** AdjustedCostFor
 *	Used by NewBestPathTo for heuristic weighting
 */
native function int AdjustedCostFor( Pawn P, NavigationPoint Anchor, NavigationPoint Goal, int Cost );

function bool IsBlockedFor(Pawn P)
{
	return (CostFor(P) >= BLOCKEDPATHCOST);
}

defaultproperties
{
	bAddToNavigationOctree=true
	bCanCutCorners=true
	ForcedPathSizeName=Common
}
