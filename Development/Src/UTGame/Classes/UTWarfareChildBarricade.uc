/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWarfareChildBarricade extends Actor
	abstract
	placeable;

var()		UTWarfareBarricade ParentBarricade;
var			UTWarfareChildBarricade NextBarricade;

simulated event PostBeginPlay()
{
	local UTWarfareBarricade B;
	local float NewDist, BestDist;

	Super.PostBeginPlay();

	if ( ParentBarricade == None )
	{
		// find a barricade
		`log(self$" does not have parent barricade set!");
		ForEach WorldInfo.AllNavigationPoints(class'UTWarfareBarricade', B)
		{
			NewDist = VSize(Location - B.Location);
			if ( (ParentBarricade == None) || (NewDist < BestDist) )
			{
				ParentBarricade = B;
				BestDist = NewDist;
			}
		}
	}
	if ( ParentBarricade != None )
	{
		ParentBarricade.RegisterChild(self);
	}
}

event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if ( ParentBarricade != None )
	{
		ParentBarricade.TakeDamage(Damage, InstigatedBy, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	}
}

simulated function DisableBarricade()
{
	SetCollision(false, false);
	SetHidden(true);
	CollisionComponent.SetBlockRigidBody(false);
}

simulated function EnableBarricade()
{
	SetCollision(true, true);
	SetHidden(false);
	CollisionComponent.SetBlockRigidBody(true);
}

defaultproperties
{
	// this will make it such that this can be statically lit
	bMovable=FALSE
	bNoDelete=true

	bProjTarget=true
	bCollideActors=true
	bBlockActors=true

	RemoteRole=Role_None
}
