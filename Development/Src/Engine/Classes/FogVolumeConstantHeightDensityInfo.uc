/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeConstantHeightDensityInfo extends FogVolumeDensityInfo
	showcategories(Movement)
	native(FogVolume)
	deprecated
	placeable;

defaultproperties
{
	Begin Object Class=FogVolumeConstantHeightDensityComponent Name=FogVolumeComponent0
	End Object
	DensityComponent=FogVolumeComponent0
	Components.Add(FogVolumeComponent0)
}
