/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTWaterVolume extends WaterVolume
	placeable;


// event untouch(Actor Other)
// {
// 	local UTPawn UTP;
// 	local UTPlayerController UTPC;
// 
// 	super.UnTouch( Other );
// 
// 	UTP = UTPawn(Other);
// 	if( UTP != none )
// 	{
// 		UTPC = UTPlayerController(UTP.Controller);
// 
// 		if( UTPC != none )
// 		{
// 			UTPC.ClientSpawnCameraEffect( UTP, class'UTEmitCameraEffect_Water' );
// 		}
// 	}
// }


defaultproperties
{
	TerminalVelocity=+01500.000000
	EntrySound=SoundCue'A_Character_Footsteps.FootSteps.A_Character_Footstep_WaterDeepLandCue'
	ExitSound=SoundCue'A_Character_Footsteps.FootSteps.A_Character_Footstep_WaterDeepCue'
}
