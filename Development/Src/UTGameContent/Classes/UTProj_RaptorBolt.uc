/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_RaptorBolt extends UTProjectile;

/**
attenuate damage over time
*/
function AttenuateDamage()
{
	if ( LifeSpan < 1.0 )
		Damage = 0.75 * Default.Damage;
	else
		Damage = Default.Damage * FMin(2.5, Square(MaxSpeed)/VSizeSq(Velocity));
}

simulated function HitWall(vector HitNormal, actor Wall, PrimitiveComponent WallComp)
{
	AttenuateDamage();
	Super.HitWall(HitNormal, Wall, WallComp);
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	AttenuateDamage();
	Super.ProcessTouch(Other, HitLocation, HitNormal);
}

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'VH_Raptor.Effects.PS_Raptor_Projectile_Blue'
	ProjExplosionTemplate=ParticleSystem'VH_Raptor.EffectS.PS_Raptor_Gun_Impact'

    Speed=2000
    MaxSpeed=12500
    AccelRate=20000.0

    Damage=20
    DamageRadius=200
    MomentumTransfer=4000

    MyDamageType=class'UTDmgType_RaptorBolt'
    LifeSpan=1.6

    bCollideWorld=true
    DrawScale=1.2

	Components.Remove(ProjectileMesh)
}
