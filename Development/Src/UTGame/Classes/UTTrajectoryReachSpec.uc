/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTrajectoryReachSpec extends AdvancedReachSpec
	native
	abstract;

cpptext
{
	virtual void AddToDebugRenderProxy(class FDebugRenderSceneProxy* DRSP);
	virtual FVector GetInitialVelocity() { return FVector(0.f,0.f,0.f); };
}

defaultproperties
{
	bAddToNavigationOctree=false
}
