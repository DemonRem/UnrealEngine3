/**
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_LinkPlasma extends UTProjectile;

var vector ColorLevel;
var vector ExplosionColor;

simulated function ProcessTouch (Actor Other, vector HitLocation, vector HitNormal)
{
	if ( Other != Instigator )
	{
		if ( !Other.IsA('Projectile') || Other.bProjTarget )
		{
			MomentumTransfer = (UTPawn(Other) != None) ? 0.0 : 1.0;
			Other.TakeDamage(Damage, InstigatorController, HitLocation, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
			Explode(HitLocation, HitNormal);
		}
	}
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	MomentumTransfer = 1.0;

	Super.HitWall(HitNormal, Wall, WallComp);
}

simulated function SpawnFlightEffects()
{
	Super.SpawnFlightEffects();
	if(ProjEffects != none)
	{
		ProjEffects.SetVectorParameter('LinkProjectileColor',ColorLevel);
	}
}


simulated function SpawnExplosionEffects(vector HitLocation, vector HitNormal)
{
	Super.SpawnExplosionEffects(HitLocation, HitNormal);
	if(WorldInfo.NetMode != NM_DedicatedServer && ProjExplosion != none)
	{
		ProjExplosion.SetVectorParameter('LinkImpactColor',ExplosionColor);
	}
}

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Projectile'
	ProjExplosionTemplate=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Impact'
	MaxEffectDistance=7000.0

	Speed=1400
	MaxSpeed=5000
	AccelRate=3000.0

	Damage=26
	DamageRadius=0
	MomentumTransfer=0
	CheckRadius=30.0

	MyDamageType=class'UTDmgType_LinkPlasma'
	LifeSpan=3.0
	NetCullDistanceSquared=+144000000.0

	bCollideWorld=true
	DrawScale=0.7

	Components.Remove(ProjectileMesh)

	// overflows max simultaneous sounds to have an ambient sound on link plasma
	//AmbientSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_TravelCue'
	ExplosionSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_ImpactCue'
	ColorLevel=(X=1,Y=1.3,Z=1)
	ExplosionColor=(X=1,Y=1,Z=1);
}