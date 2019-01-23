/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeConstantHeightDensityComponent extends FogVolumeDensityComponent
	native(FogVolume)
	collapsecategories
	hidecategories(Object)
	editinlinenew
	deprecated;

/** The constant density coefficient */
var()	const	interp	float	Density;
/** The height at which the fog stops */
var()	const	interp	float	Height;

cpptext
{
protected:
	// ActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
}

defaultproperties
{
	Density=0.0005
	Height=500.0
}
