/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPortal extends PortalTeleporter
	placeable;

defaultproperties
{
	Begin Object Name=SceneCapturePortalComponent0
		bSkipUpdateIfOwnerOccluded=true
		RelativeEntryRotation=(Yaw=32768)
	End Object

	Begin Object Name=StaticMeshComponent2
		Rotation=(Yaw=32768)
	End Object

	Begin Object Name=Arrow
		Rotation=(Yaw=32768)
	End Object

	bEdShouldSnap=true
}
