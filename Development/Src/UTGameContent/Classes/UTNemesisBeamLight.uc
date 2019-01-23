/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTNemesisBeamLight extends UTExplosionLight;

defaultproperties
{
	HighDetailFrameTime=+0.02
	Brightness=200
	Radius=128
	LightColor=(R=255,G=192,B=64,A=255)

	TimeShift=((StartTime=0.0,Radius=128,Brightness=150,LightColor=(R=255,G=192,B=128,A=255)),(StartTime=0.14,Radius=128,Brightness=50,LightColor=(R=255,G=192,B=64,A=255)),(StartTime=0.2,Radius=96,Brightness=0,LightColor=(R=255,G=192,B=64,A=255)))
}
