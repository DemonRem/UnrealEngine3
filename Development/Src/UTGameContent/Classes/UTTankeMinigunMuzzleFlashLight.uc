/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTTankeMinigunMuzzleFlashLight extends UTExplosionLight;

defaultproperties
{
	HighDetailFrameTime=+0.02
	Brightness=8
	Radius=192
	LightColor=(R=255,G=255,B=255,A=255)

	TimeShift=((StartTime=0.0,Radius=128,Brightness=50,LightColor=(R=255,G=255,B=255,A=255)),(StartTime=0.1,Radius=96,Brightness=10,LightColor=(R=255,G=255,B=128,A=255)),(StartTime=0.15,Radius=64,Brightness=0,LightColor=(R=255,G=255,B=64,A=255)))
}
