/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_LinkPowerPlasma extends UTProj_LinkPlasma;

var int Strength;

replication
{
	if (bNetInitial && (Role == ROLE_Authority) )
		Strength;
}

function SetStrength(int NewStrength)
{
	Strength = NewStrength;
}

simulated function ProcessTouch (Actor Other, vector HitLocation, vector HitNormal)
{
	if ( Other != Instigator )
	{
		if ( !Other.IsA('Projectile') || Other.bProjTarget )
		{
			MomentumTransfer = (UTPawn(Other) != None) ? 0.0 : 1.0;
			Other.TakeDamage(Damage * float(Strength), InstigatorController, HitLocation, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
			Explode(HitLocation, HitNormal);
		}
	}
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	Damage *= Strength;
	Super.HitWall(HitNormal, Wall, WallComp);
}



defaultproperties
{
	ColorLevel =(X=3.0,Y=2.3,Z=0.8)
	ExplosionColor=(X=1.2,Y=2.0,Z=0.075)
}
