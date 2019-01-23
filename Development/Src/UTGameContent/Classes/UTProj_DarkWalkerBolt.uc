/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_DarkWalkerBolt extends UTProjectile;

/**
attenuate damage over time
*/
function AttenuateDamage()
{
	if ( LifeSpan < 1.0 )
		Damage = 0.75 * Default.Damage;
	else
		Damage = Default.Damage * FMin(3, Square(MaxSpeed)/VSizeSq(Velocity));
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
	ProjFlightTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_Darkwalker_Secondary_Projectile'
	ProjExplosionTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_Darkwalker_Secondary_Impact'

    Speed=4000
    MaxSpeed=10000
    AccelRate=20000.0

    Damage=30
	DamageRadius=0
    MomentumTransfer=4000

    MyDamageType=class'UTDmgType_DarkWalkerBolt'
    LifeSpan=1.6

    bCollideWorld=true
    DrawScale=1.2

	Components.Remove(ProjectileMesh)

}
