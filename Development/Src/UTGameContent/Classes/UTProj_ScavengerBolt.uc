/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTProj_ScavengerBolt extends UTProjectile;

/** # of times they can bounce */
var int Bounces;

/** reference to impact effect we created; we wait for this before being destroyed */
var array<ParticleSystemComponent> ImpactEffects;

var ParticleSystem BounceTemplate;

simulated function Landed(vector HitNormal, Actor FloorActor)
{
	HitWall(HitNormal, FloorActor, None);
}

simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	local int i;

	if (ImpactEffects.length == 1)
	{
		Destroy();
	}
	else
	{
		i = ImpactEffects.Find(PSC);
		if (i != -1)
		{
			ImpactEffects.Remove(i, 1);
		}
		DetachComponent(PSC);
	}
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	local ParticleSystemComponent ImpactEffect;

	bBlockedByInstigator = true;
	Acceleration = vect(0,0,0);

	if (!Wall.bStatic && !Wall.bWorldGeometry)
	{
		Wall.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		Shutdown();
	}
	else
	{
		// spawn impact effect
		if (EffectIsRelevant(Location, false, MaxEffectDistance))
		{
			ImpactEffect = new(Outer) class'UTParticleSystemComponent';
			ImpactEffect.SetAbsolute(true, true, true);
			ImpactEffect.SetTranslation(Location);
			ImpactEffect.SetRotation(rotator(HitNormal));
			ImpactEffect.SetTemplate(BounceTemplate);
			ImpactEffect.OnSystemFinished = MyOnParticleSystemFinished;
			AttachComponent(ImpactEffect);
			ImpactEffects[ImpactEffects.length] = ImpactEffect;
		}

		SetPhysics(PHYS_Falling);
		if (Bounces > 0)
		{
			// @todo play impact sound
			PlaySound(ExplosionSound);
			Velocity = 0.8 * (Velocity - 2.0 * HitNormal * (Velocity dot HitNormal));
			Bounces = Bounces - 1;
		}
		else
		{
			bBounce = false;
			SetPhysics(PHYS_None);
			Explode(Location, HitNormal);
		}
	}
}

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
	ProjFlightTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Projectile'
	ProjExplosionTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Gun_Explode'
	BounceTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Gun_Impact'
	ExplosionSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_PrimaryFireImpactCue'  // TODO Make Scavenger Sounds

	MyDamageType=class'UTDmgType_ScavengerBolt' // TODO: Make Scavenger damage type
	Bounces=3
	bBounce=true
	bRotationFollowsVelocity=true
}
