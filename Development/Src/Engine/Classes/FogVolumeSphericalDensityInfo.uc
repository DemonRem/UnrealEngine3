/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeSphericalDensityInfo extends FogVolumeDensityInfo
	showcategories(Movement)
	native(FogVolume)
	placeable;

cpptext
{
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
}

defaultproperties
{
	Begin Object Class=DrawLightRadiusComponent Name=DrawSphereRadius0
	End Object
	Components.Add(DrawSphereRadius0)

	Begin Object Class=FogVolumeSphericalDensityComponent Name=FogVolumeComponent0
		PreviewSphereRadius=DrawSphereRadius0
	End Object
	DensityComponent=FogVolumeComponent0
	Components.Add(FogVolumeComponent0)
}
