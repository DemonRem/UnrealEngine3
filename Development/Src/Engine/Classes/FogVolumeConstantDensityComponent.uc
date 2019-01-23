/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeConstantDensityComponent extends FogVolumeDensityComponent
	native(FogVolume)
	collapsecategories
	hidecategories(Object)
	editinlinenew;

/** The constant density coefficient */
var()	const	interp	float	Density;

cpptext
{
public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
}

defaultproperties
{
	Density=0.0005
}
