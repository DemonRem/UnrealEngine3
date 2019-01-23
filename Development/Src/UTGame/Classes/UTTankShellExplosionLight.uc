/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTTankShellExplosionLight extends UTExplosionLight;

defaultproperties
{
	HighDetailFrameTime=+0.02
	Brightness=32
	Radius=384
	LightColor=(R=255,G=255,B=255,A=255)

	TimeShift=((StartTime=0.0,Radius=384,Brightness=32,LightColor=(R=255,G=255,B=255,A=255)),(StartTime=0.4,Radius=256,Brightness=8,LightColor=(R=255,G=255,B=128,A=255)),(StartTime=0.5,Radius=192,Brightness=0,LightColor=(R=255,G=255,B=96,A=255)))
}
