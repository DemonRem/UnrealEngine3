/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_SPMAShell extends UTProjectile
	abstract;

var class<UTProjectile> ChildProjectileClass;
var float SpreadFactor;							/** determines spread of child shells */

function SetFuse(float PredictedImpactTime)
{
	SetTimer( FMax(0.6*PredictedImpactTime, 0.85*PredictedImpactTime - 0.7), false, 'SpawnChildren');
}

function SpawnChildren()
{
	local int i,j;
	local UTProjectile Child;
	local float Mag;
	local vector CurrentVelocity;

	CurrentVelocity = 0.85*Velocity;

	Explode( Location,vector(Rotation) );

	// one shell in each of 9 zones
	for ( i=-1; i<2; i++)
	{
		for ( j=-1; j<2; j++ )
		{
			Mag = (abs(i)+abs(j) > 1) ? 0.7 : 1.0;
			Child = Spawn(ChildProjectileClass,self,,Location);
			if (Child != None)
			{
				Child.Velocity = CurrentVelocity;
				Child.Velocity.X += (0.3 + 0.7*FRand())*Mag*i*SpreadFactor;
				Child.Velocity.Y += (0.3 + 0.7*FRand())*Mag*j*SpreadFactor;
				Child.Velocity.Z = Child.Velocity.Z + SpreadFactor * (FRand() - 0.5);
				Child.InstigatorController = InstigatorController;
			}
		}
	}
}

defaultproperties
{
	Speed=4000
	MaxSpeed=4000
	MaxEffectDistance=10000.0
	TerminalVelocity=4500

	Damage=250
	DamageRadius=660
	MomentumTransfer=175000

	LifeSpan=8.0

	bCollideWorld=true
	bCollideComplex=false

	Begin Object Name=CollisionCylinder
		CollisionRadius=10
		CollisionHeight=10
		AlwaysLoadOnClient=True
		AlwaysLoadOnServer=True
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		CollideActors=true
	End Object

	Physics=PHYS_Falling
	bProjTarget=true

	bRotationFollowsVelocity=true
	bNetTemporary=false
	SpreadFactor=400.0
}
