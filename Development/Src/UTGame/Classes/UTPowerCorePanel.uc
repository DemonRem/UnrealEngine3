/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTPowerCorePanel extends Actor
	abstract;

/** sound of the panel breaking off, played on spawn */
var SoundCue BreakSound;
/** sound when hitting something */
var SoundCue HitSound;

var StaticMeshComponent Mesh;

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	PlaySound(BreakSound, true);
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const CollisionImpactData RigidCollisionData, int ContactIndex )
{
	if (LifeSpan < 7.3)
	{
		PlaySound(HitSound, true);
		Mesh.SetNotifyRigidBodyCollision(false);
	}
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
