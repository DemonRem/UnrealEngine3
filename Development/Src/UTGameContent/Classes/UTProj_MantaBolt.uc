/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_MantaBolt extends UTProjectile;

simulated function SpawnExplosionEffects(vector HitLocation, vector HitNormal)
{
	local vector x;
	if ( WorldInfo.NetMode != NM_DedicatedServer && EffectIsRelevant(Location,false,MaxEffectDistance) )
	{
		x = normal(Velocity cross HitNormal);
		x = normal(HitNormal cross x);

		ProjExplosion = Spawn(class'UTEmitter', self,, HitLocation, rotator(x));
		if (ProjExplosion != None)
		{
			ProjExplosion.SetTemplate(ProjExplosionTemplate,true);
		}
	}

	if (ExplosionSound!=None)
	{
		PlaySound(ExplosionSound);
	}
}


defaultproperties
{
	ProjFlightTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Projectile'
	ProjExplosionTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Gun_Impact'
	ExplosionSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Impact_Cue'
    Speed=2000
    MaxSpeed=7000
    AccelRate=16000.0

    Damage=33
    DamageRadius=0
    MomentumTransfer=4000

    MyDamageType=class'UTDmgType_MantaBolt'
    LifeSpan=1.6

    bCollideWorld=true
    DrawScale=1.2

	Components.Remove(ProjectileMesh)
}
