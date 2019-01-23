/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDarkWalkerBeamLight extends Actor;

var() PointLightComponent BeamLight;
var() AudioComponent AmbientSound;

defaultproperties
{
	RemoteRole=ROLE_None
	bGameRelevant=true

    Begin Object Class=PointLightComponent Name=LightComponentB
		bEnabled=true
		Brightness=10
		CastShadows=false
        LightColor=(R=255,G=128,B=64,A=255)
        Radius=300
    End Object
    BeamLight=LightComponentB
	Components.Add(LightComponentB)
	
	Begin Object Class=AudioComponent Name=ImpactSound
		SoundCue=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FireBeamRipEarthCue'
	End Object
	AmbientSound=ImpactSound
	Components.Add(ImpactSound);
	
	
}
