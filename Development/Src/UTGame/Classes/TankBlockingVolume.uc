/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class TankBlockingVolume extends BlockingVolume
	placeable;

defaultproperties
{
	Begin Object Name=BrushComponent0
		CollideActors=false
		BlockActors=false
		BlockZeroExtent=false
		BlockNonZeroExtent=false
		BlockRigidBody=true
		RBChannel=RBCC_Untitled1
		RBCollideWithChannels=(Default=false)
	End Object

	bColored=true
	BrushColor=(R=64,G=128,B=200,A=255)

	bWorldGeometry=true
	bCollideActors=false
	bBlockActors=false
	bClampFluid=false
}