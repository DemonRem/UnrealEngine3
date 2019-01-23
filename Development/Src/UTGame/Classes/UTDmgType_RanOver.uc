/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 */
class UTDmgType_RanOver extends UTDamageType
	abstract;

var int NumMessages;

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
		UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage(class'UTVehicleKillMessage', UTPRI.RanOverCount % 4);
	}
}

static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation)
{
	Super.SpawnHitEffect(P,Damage,Momentum,BoneName,HitLocation);
	if(UTPawn(P) != none)
	{
		UTPawn(P).SoundGroupClass.Static.PlayCrushedSound(P);
	}
}

/**
 * @todo: We want
 *  [2007-04-17--13:54:47] msew@epic: so no gibbing but rip off some limbs and spawn those and then have the body go flying?
 * [2007-04-17--13:55:09] mattO: yeah maybe
 * [2007-04-17--13:55:29] mattO: like closest bone in the direction the vehicle is pointing, rip that off
 * [2007-04-17--13:55:36] mattO: or something
 * [2007-04-17--13:55:39] mattO: we can experiment
 * [2007-04-17--13:55:49] mattO: but I like the bodies flying around after you run into them so I'd hate to lose it
 * [2007-04-17--13:56:17] msew@epic: that makes sense.
 **/


defaultproperties
{
	GibPerterbation=0.5
	GibModifier=2.0
	bLocationalHit=false
	bNeverGibs=true
	bUseTearOffMomentum=true
	bExtraMomentumZ=false
	bVehicleHit=true

	NumMessages=4

	//DeathCameraEffectVictim=class'UTEmitCameraEffect_BloodSplatter'
	//DeathCameraEffectInstigator=class'UTEmitCameraEffect_BloodAndChunks'
}


