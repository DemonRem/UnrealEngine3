/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_Pancake extends UTDmgType_RanOver
	abstract;

static function ScoreKill(PlayerReplicationInfo KillerPRI, PlayerReplicationInfo KilledPRI, Pawn KilledPawn)
{
	local UTPlayerReplicationInfo UTPRI;

	if ( (KillerPRI == KilledPRI) || (UTPlayerController(KillerPRI.Owner) == None) )
		return;

	UTPRI = UTPlayerReplicationInfo(KillerPRI);
	if (UTPRI != None)
	{
		UTPRI.RanOverCount++;
		if ( UTPRI.RanOverCount == 10 )
		{
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage(class'UTVehicleKillMessage', 7);
			return;
		}
		UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage(class'UTVehicleKillMessage', 4);
	}
}

defaultproperties
{
	bAlwaysGibs=true
	GibPerterbation=1.0
	bLocationalHit=false
	bArmorStops=false

	//DeathCameraEffectVictim=class'UTEmitCameraEffect_BloodSplatter'
}
