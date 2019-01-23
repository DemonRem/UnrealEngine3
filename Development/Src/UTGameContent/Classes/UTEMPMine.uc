/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTEMPMine extends UTDeployedActor;
var float MaxLifetime;
var float ExplodeRadius;
var float EMPDamage;


var SoundCue ExplosionSound;
var ParticleSystem ExplosionEffect;

var UTEmitter Explosion;

simulated function Explode()
{
	if(ExplosionSound != none)
		PlaySound(ExplosionSound);
	if(ExplosionEffect != none)
	{
		Explosion = Spawn(class'UTEmitter', self,, Location, rotation);
		if(Explosion != none)
			Explosion.SetTemplate(ExplosionEffect,true);
		
	}
	class'UTEMPExplosion'.static.Explode(self,ExplodeRadius,EMPDamage);
	Destroy();
}

simulated function PostBeginPlay()
{
	SetTimer(MaxLifetime,false,'Explode');
}

event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
	local UTVehicle utv;
	utv = UTVehicle(Other);
	if(UTVehicle_Hoverboard(utv) != none)
	{
		utv = none;
	}
	if(utv != none)
	{
		if (!WorldInfo.GRI.OnSameTeam(self, other))
		{
			Explode();
		}
	}
}
defaultproperties
{
	Health=500
	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_HU_Deco_SM_FanBox'
		CollideActors=true
		BlockActors=false
		Scale3D=(X=0.1,Y=0.1,Z=0.75)
		Translation=(Z=-85.0)
		CastShadow=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(DeployableMesh)

    MaxLifetime=30.0
	ExplodeRadius=500.0
	EMPDamage=500.0
	ExplosionEffect=ParticleSystem'Pickups.Deployables.P_EMPMineExplosion'
	ExplosionSound=SoundCue'A_Pickups.Weapons.Cue.EMPDeployableExplosionCue'

}
